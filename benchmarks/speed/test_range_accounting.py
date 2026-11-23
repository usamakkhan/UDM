import unittest
from analyze_range_accounting import coverage


class RangeAccountingTests(unittest.TestCase):
    def test_adjacent_requests_cover_without_overlap(self):
        self.assertEqual(coverage([(0, 4), (4, 10)], 10), {
            'sentBytes': 10, 'uniqueBytes': 10, 'repeatedBytes': 0,
            'holes': [], 'overlap': []})

    def test_nested_retries_count_multiplicity(self):
        result = coverage([(0, 10), (2, 8), (4, 6)], 10)
        self.assertEqual((result['sentBytes'], result['uniqueBytes'], result['repeatedBytes']),
                         (18, 10, 8))
        self.assertIn({'start': 4, 'endExclusive': 6, 'multiplicity': 3}, result['overlap'])

    def test_disjoint_and_unsent_requests_preserve_holes(self):
        result = coverage([(0, 0), (2, 4), (6, 9)], 10)
        self.assertEqual(result['holes'], [[0, 2], [4, 6], [9, 10]])
        self.assertEqual(result['uniqueBytes'], 5)

    def test_reordered_identical_requests(self):
        result = coverage([(5, 10), (0, 5), (0, 5)], 10)
        self.assertEqual(result['repeatedBytes'], 5)
        self.assertEqual(result['holes'], [])

    def test_empty_snapshot_is_not_complete(self):
        self.assertEqual(coverage([], 10)['holes'], [[0, 10]])

    def test_out_of_resource_or_reversed_interval_rejected(self):
        for interval in [(-1, 3), (0, 11), (5, 4)]:
            with self.subTest(interval=interval), self.assertRaises(AssertionError):
                coverage([interval], 10)


if __name__ == '__main__':
    unittest.main()
