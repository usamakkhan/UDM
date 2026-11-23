"""Validate the owned engine-only data-drain ABBA control; never runs an app."""
from pathlib import Path
import argparse
import hashlib
import json
import statistics
import struct


def load(path):
    return json.loads(path.read_text(encoding='utf-8-sig'))


def analyze(area):
    build, data = load(area / 'build.json'), load(area / 'runs.json')
    assert build['passed'] and build['engineHarnessNotNormalApp']
    assert data['passed'] and len(data['runs']) == 4
    assert [r['name'] for r in data['runs']] == [
        'normal-a', 'suppressed-b', 'suppressed-c', 'normal-d']
    reports = []
    for run in data['runs']:
        encoded = load(area / run['name'] / 'trace-lossless.json')
        raw = bytes.fromhex(encoded['headerHex']) + bytes.fromhex(encoded['usedRecordsHex'])
        raw += bytes(encoded['zeroTailBytes'])
        assert hashlib.sha256(raw).hexdigest().upper() == encoded['rawSha256']
        magic, version, capacity, frequency, pid, count, dropped, width, _, _ = struct.unpack_from(
            '<8sIIqIiiIqq8x', raw)
        assert magic == b'UDMUIT1\0' and version == 1 and capacity == 8192
        assert frequency > 0 and pid == run['pid'] and 0 < count <= capacity
        assert dropped == 0 and width == 64 and len(raw) == 64 + capacity * width
        events = []
        for index in range(count):
            ready, thread, tick, end, label = struct.unpack_from('<iIqq40s', raw, 64 + index * width)
            assert ready == 1 and thread > 0 and end >= tick > 0
            events.append((tick, thread, label.split(b'\0', 1)[0].decode()))
        events.sort()
        assert events == [(e['tick'], e['thread'], e['stage']) for e in run['events']]
        lookup = set(events)
        for span in run['spans']:
            assert (span['begin'], span['thread'], span['stage'] + '-begin') in lookup
            assert (span['end'], span['thread'], span['stage'] + '-end') in lookup
            assert abs((span['end'] - span['begin']) * 1000 / frequency - span['durationMs']) < 1e-6
        physical = sum(e[2] in ['data-drain-ok', 'data-drain-failed'] for e in events)
        suppressed = sum(e[2] == 'data-drain-suppressed' for e in events)
        assert not any(e[2] == 'data-drain-failed' for e in events)
        assert physical == run['physicalDataFlushCalls']
        assert suppressed == run['suppressedCallbacks']
        if run['optionalDataDrainSuppressed']:
            assert physical == 0 and suppressed > 0
        else:
            assert physical > 0 and suppressed == 0
        phases = run['finishPhases']
        assert phases['PublishedFlushError'] == phases['PublishedOpenError'] == 0
        assert phases['PublishedFileFlush'] >= 0
        assert run['hashMatches'] and run['allOwnedThreadsTerminatedByNormalExit']
        assert run['binarySha256'] == build['sha256'] and run['exitCode'] == 0
        removed = load(area / run['name'] / 'payload-removed.json')
        assert removed['passed'] and removed['sha256'] == run['expectedSha256']
        reports.append({
            'name': run['name'], 'suppressed': run['optionalDataDrainSuppressed'],
            'wallSeconds': run['wallSeconds'], 'exitTailMs': run['exitTailMs'],
            'availabilityTailMs': run['exclusiveAvailabilityTailMs'],
            'explicitFlushTailMs': run['explicitOutputFlushTailMs'],
            'physicalOptionalFlushCalls': physical, 'suppressedCallbacks': suppressed,
            'requiredFinalFlushMs': phases['PublishedFileFlush'] * 1000,
            'slowBodyWritesAtLeast20Ms': len(run['slowBodyWrites']),
            'longestBodyWriteMs': run['longestBodyWriteMs'],
            'slowWritesOverlapCountMeansCallbackScopeNotNecessarilyPhysicalIo': True})
    groups = {mode: [x for x in reports if x['suppressed'] == mode] for mode in [False, True]}
    means = {str(mode): {key: statistics.mean(x[key] for x in rows)
                        for key in ['wallSeconds', 'exitTailMs', 'availabilityTailMs']}
             for mode, rows in groups.items()}
    finishing_regresses = means['True']['exitTailMs'] > means['False']['exitTailMs']
    return {'diagnosticOnly': True, 'engineHarnessNotNormalApp': True,
            'sameBinaryAllFourRuns': True, 'runs': reports, 'groupMeans': means,
            'suppressionFinishingRegresses': finishing_regresses,
            'suppressionEligibleForProduction': False,
            'engineControlAloneCannotAcceptProductionChange': True,
            'slowWritesRemainWithoutPhysicalOptionalFlush': all(
                x['slowBodyWritesAtLeast20Ms'] > 0 for x in groups[True]),
            'normalAppOrIdmSpeedGateAccepted': False}


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('area', type=Path)
    parser.add_argument('--output', type=Path, required=True)
    args = parser.parse_args()
    report = analyze(args.area)
    args.output.write_text(json.dumps(report, indent=2) + '\n', encoding='utf-8')
    print(json.dumps({'validatedRuns': len(report['runs']),
                      'suppressionFinishingRegresses': report['suppressionFinishingRegresses']}))
