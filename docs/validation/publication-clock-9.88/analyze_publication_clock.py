"""Validate owned publication traces and separate barriers from pin-close time.

Reads exported lossless trace records; never launches apps or deletes files.
These instrumented observations are attribution, not normal-product speed gates.
"""
from pathlib import Path
import argparse
import hashlib
import json
import struct


def load(path):
    return json.loads(path.read_text(encoding='utf-8-sig'))


def analyze(area):
    build, data = load(area / 'build.json'), load(area / 'runs.json')
    assert build['passed'] and build['diagnosticOnly'] and data['passed']
    reports = []
    for run in data['runs']:
        folder = area / ('owned-' + run['case'])
        receipt = load(folder / 'transfer-receipt.json')
        encoded = load(folder / 'trace-lossless.json')
        raw = bytes.fromhex(encoded['headerHex']) + bytes.fromhex(encoded['usedRecordsHex'])
        raw += bytes(encoded['zeroTailBytes'])
        assert hashlib.sha256(raw).hexdigest().upper() == encoded['rawSha256']
        magic, version, capacity, frequency, pid, count, dropped, width, _, _ = struct.unpack_from(
            '<8sIIqIiiIqq8x', raw)
        assert magic == b'UDMUIT1\0' and version == 1 and capacity == 8192
        assert frequency > 0 and pid == receipt['pid'] and 0 < count <= capacity
        assert dropped == 0 and width == 64 and len(raw) == 64 + capacity * width
        events = []
        for index in range(count):
            ready, thread, counter, finished, label = struct.unpack_from(
                '<iIqq40s', raw, 64 + index * width)
            assert ready == 1 and thread > 0 and finished >= counter > 0
            events.append((counter, thread, finished, label.split(b'\0', 1)[0].decode()))
        events.sort()
        assert events == [(e['counter'], e['thread'], e['finished'], e['stage'])
                          for e in run['events']]
        labels = {(counter, thread, label) for counter, thread, _, label in events}
        for span in run['spans']:
            if span['stage'] == 'manager-stop-native':
                begin, end = 'stop-entry', 'stop-return'
            else:
                begin, end = span['stage'] + '-begin', span['stage'] + '-end'
            assert (span['beginCounter'], span['thread'], begin) in labels
            assert (span['endCounter'], span['thread'], end) in labels
            duration = (span['endCounter'] - span['beginCounter']) * 1000 / frequency
            assert duration >= 0 and abs(duration - span['durationMs']) < 1e-6

        def event(name):
            found = [e for e in events if e[3] == name]
            assert len(found) == 1, (run['case'], name, len(found))
            return found[0][0]

        resolved, pin_begin, pin_end, cleanup = map(event, [
            'catalog-resolve-end', 'file-pins-begin', 'file-pins-end',
            'publication-cleanup-end'])
        assert resolved <= pin_begin <= pin_end <= cleanup
        offset, last = receipt['qpcOffset'], receipt['lastOriginPerf']
        pin_perf = pin_end / frequency + offset
        selected = {}
        for name in ['body-flush', 'body-join', 'hash-join', 'intent-write',
                     'rename', 'catalog-write', 'file-pins', 'manager-stop-native']:
            selected[name] = [
                {'startAfterLastSendMs': s['startMs'], 'endAfterLastSendMs': s['endMs'],
                 'durationMs': s['durationMs']}
                for s in run['spans'] if s['stage'] == name]
        reports.append({
            'case': run['case'], 'bytes': run['bytes'], 'events': count,
            'lastBodyWriteAfterLastSendMs': max(
                s['endMs'] for s in run['spans'] if s['stage'] == 'body-write'),
            'slowBodyWritesOverlappingAfterLastSend': [
                {'startMs': s['startMs'], 'endMs': s['endMs'], 'durationMs': s['durationMs']}
                for s in run['spans'] if s['stage'] == 'body-write'
                and s['endMs'] > 0 and s['durationMs'] > 20],
            'filePinsReleasedAfterCatalogResolved': True,
            'catalogResolveToPinCloseBeginMs': (pin_begin - resolved) * 1000 / frequency,
            'pinCloseMs': (pin_end - pin_begin) * 1000 / frequency,
            'lastSendToPinReleaseMs': (pin_perf - last) * 1000,
            'pinReleaseToExclusiveObservationMs': (
                (receipt['exclusiveAvailabilityPerf'] - pin_perf) * 1000
                if receipt['exclusiveAvailabilityPerf'] is not None else None),
            'regions': selected, 'exclusiveAvailabilityTailMs': run['exclusiveAvailabilityTailMs'],
            'processExitTailMs': run['exitTailMs'],
            'qpcMappingBoundSeconds': receipt['qpcMappingBoundSeconds']})
        assert run['diagnosticOnly'] and not run['normalProductSpeedComparison']
        assert run['hashMatches'] and run['binarySha256'] == build['sha256']
    return {'diagnosticOnly': True, 'normalProductSpeedGateAccepted': False,
            'overlappingRegionsMustNotBeSummed': True, 'runs': reports}


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('area', type=Path)
    parser.add_argument('--output', type=Path, required=True)
    args = parser.parse_args()
    report = analyze(args.area)
    args.output.write_text(json.dumps(report, indent=2) + '\n', encoding='utf-8')
    print(json.dumps({'validatedRuns': len(report['runs']), 'diagnosticOnly': True}))
