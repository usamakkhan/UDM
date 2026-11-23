"""Attribute fresh split snapshots to final owners; rejects retries/unknown starts.

Origin sends are socket acceptance, not client retention or device durability.
Only snapshots with a one-request-per-final-start mapping are supported.
"""
import argparse
import json
from pathlib import Path
from analyze_range_accounting import coverage


def analyze(record):
    transfer, origin = record['transfer'], record['origin']
    size, segments = transfer['size'], transfer['segments']
    by_start = {s['Start']: s for s in segments}
    assert len(by_start) == len(segments)
    rows, intervals, owners = [], [], set()
    for connection in origin['detail']:
        for request in connection['requests']:
            if not request['sent']:
                continue
            start, end = request['start'], request['start'] + request['sent']
            logical_start = start + min(64, start)
            # This report intentionally excludes arbitrary resume/retry offsets.
            assert logical_start in by_start, 'Unknown final owner: use a retry-aware trace'
            assert logical_start not in owners, 'Retried owner: unsupported by this attribution'
            owners.add(logical_start)
            segment = by_start[logical_start]
            assert request['method'] == 'GET' and request['status'] in (200, 206)
            assert end <= request['end'] + 1
            prefix = max(0, min(end, logical_start) - start)
            tail = max(0, end - max(start, segment['End'] + 1))
            rows.append(dict(connection=connection['id'], start=start,
                             sent=request['sent'], finalOwnerEnd=segment['End'],
                             overlapPrefixBytes=prefix, beyondFinalOwnerBytes=tail))
            intervals.append((start, end))
    account = coverage(intervals, size)
    assert account['sentBytes'] == origin['bytes_sent']
    assert account['uniqueBytes'] == size and not account['holes']
    prefix = sum(r['overlapPrefixBytes'] for r in rows)
    tail = sum(r['beyondFinalOwnerBytes'] for r in rows)
    assert prefix + tail == account['repeatedBytes'], 'Final-owner attribution does not reconcile'
    return dict(passed=True, overlapPrefixBytes=prefix, beyondFinalOwnerBytes=tail,
                clientChargedSurplusBytes=transfer['transferredBytes'] - size,
                coverage=account, requests=rows,
                scope='Origin socket sends versus final range owners; buffering location and client retention are not measured.')


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('input', type=Path)
    parser.add_argument('--output', type=Path, required=True)
    args = parser.parse_args()
    args.output.write_text(json.dumps(analyze(json.loads(args.input.read_text(encoding='utf-8-sig'))), indent=2), encoding='utf-8')
