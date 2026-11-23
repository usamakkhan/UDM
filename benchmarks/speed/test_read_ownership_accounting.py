import unittest
from analyze_read_ownership import analyze


def fixture():
    return dict(transfer=dict(size=256, transferredBytes=320,
                             segments=[dict(Start=0, End=127), dict(Start=128, End=255)]),
                origin=dict(bytes_sent=336, detail=[
                    dict(id=1, requests=[dict(start=0, end=255, sent=144, method='GET', status=200)]),
                    dict(id=2, requests=[dict(start=64, end=255, sent=192, method='GET', status=206)])]))


class OwnershipAccounting(unittest.TestCase):
    def test_distinguishes_boundary_proof_from_shortened_tail(self):
        result = analyze(fixture())
        self.assertEqual((result['overlapPrefixBytes'], result['beyondFinalOwnerBytes']), (64, 16))
        self.assertEqual(result['coverage']['repeatedBytes'], 80)
        self.assertEqual(result['clientChargedSurplusBytes'], 64)

    def test_rejects_unmapped_retry_offset(self):
        record = fixture()
        record['origin']['detail'][1]['requests'][0]['start'] += 1
        with self.assertRaisesRegex(AssertionError, 'Unknown final owner'):
            analyze(record)

    def test_rejects_duplicate_owner(self):
        record = fixture()
        record['origin']['detail'].append(record['origin']['detail'][1])
        with self.assertRaisesRegex(AssertionError, 'Retried owner'):
            analyze(record)

    def test_rejects_missing_coverage(self):
        record = fixture()
        record['origin']['detail'][1]['requests'][0]['sent'] -= 1
        record['origin']['bytes_sent'] -= 1
        with self.assertRaises(AssertionError):
            analyze(record)

    def test_rejects_body_beyond_declared_end(self):
        record = fixture()
        record['origin']['detail'][0]['requests'][0]['end'] = 100
        with self.assertRaises(AssertionError):
            analyze(record)

    def test_rejects_unreconciled_origin_total(self):
        record = fixture()
        record['origin']['bytes_sent'] += 1
        with self.assertRaises(AssertionError):
            analyze(record)


if __name__ == '__main__':
    unittest.main()
