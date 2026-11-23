import copy
import unittest
from analyze_request_gaps import analyze


def fixture():
    return dict(passed=True, kernelJoined=True, nativePhaseValidated=True,
                diskCorrelationsValidated=True, failures=[], fullSchemaQualificationPassed=False,
                kernelCauseVerified=False, performanceComparison=False,
                fileAttributionFromKernelVerified=False, pendingTargetDiskOperations=0,
                unmatchedOwnedDiskCompletions=0, finalNativeFlushMs=10,
                linkedDiskRequestIntervalUnionMs=5, observedOffCpuMs=6,
                linkedDiskRequests=[dict(beginAfterNativeStartMs=1, durationMs=3),
                                    dict(beginAfterNativeStartMs=3, durationMs=3)],
                offCpuIntervals=[dict(beginAfterNativeStartMs=0, offCpuMs=2),
                                 dict(beginAfterNativeStartMs=6, offCpuMs=4)])


class RequestGapTests(unittest.TestCase):
    def test_overlap_partition(self):
        result = analyze(fixture())
        self.assertEqual((result['gapCount'], result['noRetainedLinkedRequestMs']), (2, 5))
        self.assertEqual(result['gapObservedOffCpuMs'], 5)
        self.assertEqual(result['largestGaps'][0]['durationMs'], 4)
        self.assertFalse(result['deviceIdleProven'])

    def test_empty_requests(self):
        data = fixture(); data['linkedDiskRequests'] = []; data['linkedDiskRequestIntervalUnionMs'] = 0
        self.assertEqual(analyze(data)['largestGaps'][0]['durationMs'], 10)

    def test_full_coverage(self):
        data = fixture(); data['linkedDiskRequests'] = [dict(beginAfterNativeStartMs=0, durationMs=10)]
        data['linkedDiskRequestIntervalUnionMs'] = 10
        self.assertEqual(analyze(data)['gapCount'], 0)

    def test_adjacent_and_zero_length(self):
        data = fixture(); data['linkedDiskRequests'] = [dict(beginAfterNativeStartMs=1, durationMs=2),
            dict(beginAfterNativeStartMs=3, durationMs=3), dict(beginAfterNativeStartMs=6, durationMs=0)]
        self.assertEqual(analyze(data)['gapCount'], 2)

    def test_bad_scope(self):
        for key in ('passed', 'kernelJoined', 'nativePhaseValidated', 'diskCorrelationsValidated'):
            with self.subTest(key=key):
                data = fixture(); data[key] = False
                with self.assertRaises(ValueError): analyze(data)
        for key in ('pendingTargetDiskOperations', 'unmatchedOwnedDiskCompletions'):
            data = fixture(); data[key] = 1
            with self.assertRaises(ValueError): analyze(data)

    def test_bad_intervals(self):
        for start, duration in ((-1, 2), (9, 2), (0, float('nan')), (True, 1)):
            data = fixture(); data['linkedDiskRequests'][0] = dict(beginAfterNativeStartMs=start, durationMs=duration)
            with self.assertRaises(ValueError): analyze(data)

    def test_bad_totals(self):
        for key in ('linkedDiskRequestIntervalUnionMs', 'observedOffCpuMs'):
            data = fixture(); data[key] += 1
            with self.assertRaises(ValueError): analyze(data)

    def test_overlapping_waits(self):
        data = copy.deepcopy(fixture()); data['offCpuIntervals'].append(dict(beginAfterNativeStartMs=1, offCpuMs=1))
        with self.assertRaises(ValueError): analyze(data)


if __name__ == '__main__':
    unittest.main()
