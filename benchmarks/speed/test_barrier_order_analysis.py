import copy
import unittest
from analyze_barrier_order import analyze, METRICS


def fixture():
    rows = []
    for trial in range(1, 4):
        for client, mode, duration in [('udm', 'serial', .1), ('baseline', 'overlap', .2), ('idm', None, .3)]:
            row = dict(trial=trial, case='single-4m', bytes=4194304, hashMatches=True,
                       trackedProcesses=1, client=client, sourceExecutableSha256='A' * 64,
                       launcherExitCode=0,
                       publicationPhases=dict(ControlBarrierOrder=mode, ControlBodyJoinBeforeIntentSeconds=.01))
            row.update({key: duration for key in METRICS})
            rows.append(row)
    return dict(results=rows)


class BarrierAnalysis(unittest.TestCase):
    def test_single_losing_tail_cannot_be_hidden_by_winning_median(self):
        data = fixture()
        data['results'][3]['firstAvailabilityTailSeconds'] = .5
        result = analyze(data)
        self.assertTrue(result['rejectedForObservedRegressions'])
        self.assertEqual(result['serialLossCounts']['firstAvailabilityTailSeconds'], 1)
        self.assertLess(result['medianSerialMinusOverlapMs']['firstAvailabilityTailSeconds'], 0)

    def test_apparent_win_still_does_not_authorize_a_production_change(self):
        result = analyze(fixture())
        self.assertFalse(result['rejectedForObservedRegressions'])
        self.assertFalse(result['productionChangeAdmitted'])

    def test_mismatched_binary_is_not_a_same_binary_control(self):
        data = fixture()
        data['results'][1]['sourceExecutableSha256'] = 'B' * 64
        with self.assertRaisesRegex(AssertionError, 'Not the same binary'):
            analyze(data)

    def test_missing_serial_execution_proof_is_rejected(self):
        data = fixture()
        del data['results'][0]['publicationPhases']['ControlBodyJoinBeforeIntentSeconds']
        with self.assertRaises(KeyError):
            analyze(data)

    def test_incomplete_or_duplicate_pairs_are_rejected(self):
        for data in [dict(results=fixture()['results'][:-1]), dict(results=fixture()['results'] + [copy.deepcopy(fixture()['results'][0])])]:
            with self.assertRaises(AssertionError):
                analyze(data)

    def test_a_post_exit_flush_regression_is_retained(self):
        data = fixture()
        data['results'][0]['durableWallSeconds'] = .6
        result = analyze(data)
        self.assertEqual(result['trials'][0]['serialLosses'], ['durableWallSeconds'])


if __name__ == '__main__':
    unittest.main()
