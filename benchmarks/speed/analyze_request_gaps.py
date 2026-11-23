"""Read-only partition of an already qualified, public finishing-phase report.

Uses the report's decimal milliseconds, not reconstructed raw QPC timestamps.
An uncovered interval means no retained linked request, never an idle device.
"""
import argparse
from decimal import Decimal, InvalidOperation
import hashlib
import json
from pathlib import Path


def require(condition, message):
    if not condition:
        raise ValueError(message)


def number(value):
    require(not isinstance(value, bool) and isinstance(value, (int, float, Decimal)),
            'Expected numeric milliseconds')
    try:
        result = Decimal(str(value))
    except InvalidOperation as exc:
        raise ValueError('Invalid milliseconds') from exc
    require(result.is_finite() and result >= 0, 'Invalid milliseconds')
    return result


def merge(intervals):
    merged = []
    for lo, hi in sorted(intervals):
        if merged and lo <= merged[-1][1]:
            merged[-1] = (merged[-1][0], max(hi, merged[-1][1]))
        else:
            merged.append((lo, hi))
    return merged


def length(intervals):
    return sum((hi - lo for lo, hi in merge(intervals)), Decimal(0))


def intervals(rows, duration_key, end):
    result = []
    require(isinstance(rows, list) and len(rows) <= 16384, 'Missing or excessive intervals')
    for row in rows:
        lo = number(row['beginAfterNativeStartMs'])
        hi = lo + number(row[duration_key])
        require(hi <= end, 'Interval outside native call')
        result.append((lo, hi))
    return result


def analyze(report):
    require(all(report.get(key) is True for key in
                ('passed', 'kernelJoined', 'nativePhaseValidated', 'diskCorrelationsValidated')),
            'Requires qualified scoped phase analysis')
    require(report.get('failures') == [], 'Phase failures')
    require(all(report.get(key) is False for key in
                ('fullSchemaQualificationPassed', 'kernelCauseVerified', 'performanceComparison',
                 'fileAttributionFromKernelVerified')), 'Unsupported evidence scope')
    require(report.get('pendingTargetDiskOperations') == 0
            and report.get('unmatchedOwnedDiskCompletions') == 0, 'Incomplete request accounting')
    end = number(report['finalNativeFlushMs'])
    require(end > 0, 'Empty native call')
    disk = intervals(report['linkedDiskRequests'], 'durationMs', end)
    off = intervals(report['offCpuIntervals'], 'offCpuMs', end)
    require(length(off) == sum((hi-lo for lo, hi in off), Decimal(0)),
            'Overlapping off-CPU intervals')
    # Separate decimal fields were exported from integer QPC differences.
    # Allow one last displayed 0.0001-ms unit; never change interval boundaries.
    tolerance = Decimal('0.0001')
    require(abs(length(disk)-number(report['linkedDiskRequestIntervalUnionMs'])) <= tolerance,
            'Request union disagrees with qualified report')
    require(abs(length(off)-number(report['observedOffCpuMs'])) <= tolerance,
            'Off-CPU union disagrees with qualified report')
    cursor = Decimal(0)
    gaps = []
    for lo, hi in merge(disk):
        if lo > cursor:
            gaps.append((cursor, lo))
        cursor = max(cursor, hi)
    if cursor < end:
        gaps.append((cursor, end))
    rows = []
    for lo, hi in gaps:
        overlap = length([(max(lo, a), min(hi, b)) for a, b in off
                          if min(hi, b) > max(lo, a)])
        rows.append(dict(beginAfterNativeStartMs=float(lo), durationMs=float(hi-lo),
                         observedOffCpuMs=float(overlap),
                         notClassifiedOffCpuMs=float(hi-lo-overlap)))
    gap_time = sum((hi-lo for lo, hi in gaps), Decimal(0))
    require(length(disk)+gap_time == end, 'Partition does not cover native call')
    return dict(passed=True, sourceScope='retained linked requests on measured flush thread',
                nativeFlushMs=float(end), linkedRequestUnionMs=float(length(disk)),
                noRetainedLinkedRequestMs=float(gap_time), gapCount=len(rows),
                gapObservedOffCpuMs=float(sum((Decimal(str(row['observedOffCpuMs']))
                                              for row in rows), Decimal(0))),
                largestGaps=sorted(rows, key=lambda row: row['durationMs'], reverse=True),
                rawCounterReplay=False, newCapture=False, payloadIntegrityReverified=False,
                deviceIdleProven=False, kernelCauseVerified=False, performanceComparison=False,
                productionChangeAdmitted=False,
                limitations='Reanalysis of exported decimal milliseconds. Only requests retained '
                'by the source join, wholly inside the native call and attributed to its issuing '
                'thread, are included. Other threads, crossing requests, filesystem/filter work '
                'and complete device activity are outside this partition. Off-CPU overlap is '
                'observed scheduling evidence, not exclusive wait-cause or exact CPU accounting.')


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('report', type=Path)
    args = parser.parse_args()
    raw = args.report.read_bytes()
    require(len(raw) <= 32*1024*1024, 'Input too large')
    result = analyze(json.loads(raw, parse_float=Decimal))
    result['inputSha256'] = hashlib.sha256(raw).hexdigest().upper()
    print(json.dumps(result, indent=2))


if __name__ == '__main__':
    main()
