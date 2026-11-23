import copy
import json
from pathlib import Path
import unittest
from analyze_dash_boundary import analyze_data, join_native_guard

ROOT = Path(__file__).resolve().parents[2] / 'docs/validation/dash-boundary-research-10.0'


class BoundaryEvidenceTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.continuous = json.loads((ROOT / 'continuous/results.json').read_text(encoding='utf-8'))
        cls.independent = json.loads((ROOT / 'independent/results.json').read_text(encoding='utf-8'))
        cls.guard = json.loads((ROOT / 'native-guard/results.json').read_text(encoding='utf-8'))

    def corrupt(self, edit):
        data = copy.deepcopy(self.continuous)
        edit(data)
        with self.assertRaises(AssertionError):
            analyze_data(data)

    def test_positive_control_and_losses_remain_separate(self):
        report = analyze_data(self.continuous)
        self.assertTrue(report['evidenceValid'])
        self.assertFalse(report['allCasesExact'])
        self.assertTrue(report['cases'][0]['allExact'])
        self.assertEqual([0, 128, 672], [row['sampleDelta'] for row in report['cases']])
        self.assertFalse(report['allMediaRehashed'])

    def test_native_guard_join_retains_five_failures(self):
        reports = {'dash-boundary-clipping-10.0': analyze_data(self.independent),
                   'dash-boundary-continuous-10.0': analyze_data(self.continuous)}
        report = join_native_guard(reports, self.guard)
        self.assertEqual(5, report['coarseAcceptedButOracleFailed'])
        self.assertFalse(report['allMediaExact'])

    def test_delta_corruption(self):
        self.corrupt(lambda data: data['cases'][1].update(sampleDelta=0))

    def test_false_exact_audio_claim(self):
        self.corrupt(lambda data: data['cases'][1].update(audioExact=True))

    def test_boolean_sample_count(self):
        self.corrupt(lambda data: data['cases'][0].update(expectedSamples=True))

    def test_short_digest(self):
        self.corrupt(lambda data: data['cases'][0].update(expectedPcmSha256='short'))

    def test_duplicate_period(self):
        self.corrupt(lambda data: data['cases'][0]['clips'][1].update(period=0))

    def test_unsafe_artifact_name(self):
        self.corrupt(lambda data: data['sourceFiles'][0].update(name='../outside.mp4'))

    def test_false_overall_success(self):
        self.corrupt(lambda data: data.update(allCasesExact=True))

    def test_missing_native_guard_case(self):
        guard = copy.deepcopy(self.guard)
        guard['cases'].pop()
        reports = {'dash-boundary-clipping-10.0': analyze_data(self.independent),
                   'dash-boundary-continuous-10.0': analyze_data(self.continuous)}
        with self.assertRaises(AssertionError):
            join_native_guard(reports, guard)


if __name__ == '__main__':
    unittest.main()
