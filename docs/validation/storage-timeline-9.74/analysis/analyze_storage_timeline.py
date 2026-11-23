"""Validate and summarize saved owned source-operation traces; no app launch or deletion."""
from pathlib import Path
import argparse
import hashlib
import json
import struct


def load(path):
    return json.loads(path.read_text(encoding='utf-8-sig'))


def analyze(area, expected_cases=None):
    build = load(area / 'build.json')
    data = load(area / 'runs.json')
    assert build['passed'] and build['diagnosticOnly'] and data['passed']
    expected_cases = set(expected_cases or ['single-4m', 'single-64m', 'split-64m', 'visible-4m'])
    assert len(data['runs']) == len(expected_cases)
    assert {r['case'] for r in data['runs']} == expected_cases
    summaries = []
    for run in data['runs']:
        folder = area / ('owned-' + run['case'])
        receipt = load(folder / 'transfer-receipt.json')
        launch = load(folder / 'launch.json')
        args = launch['args']
        assert Path(args[0]).resolve() == (folder / 'native/UDM.exe').resolve()
        assert Path(args[args.index('--data-dir') + 1]).resolve() == (folder / 'catalog').resolve()
        assert Path(args[args.index('/p') + 1]).resolve() == (folder / 'files').resolve()
        assert args[args.index('/f') + 1] == 'sample.bin'
        assert ('--background' in args) == (run['case'] != 'visible-4m')
        assert launch['binarySha256'] == build['sha256']
        removed = load(folder / 'payload-removed.json')
        encoded = load(folder / 'trace-lossless.json')
        assert run['passed'] and run['diagnosticOnly']
        assert not run['normalProductSpeedComparison'] and run['hashMatches']
        assert run['binarySha256'] == receipt['sourceBinarySha256'] == build['sha256']
        assert removed['passed'] and removed['sha256'] == run['sha256'] == receipt['expectedSha256']
        assert removed['bytes'] == run['bytes'] == receipt['bytes']
        assert not (folder / 'files/sample.bin').exists()
        raw = bytes.fromhex(encoded['headerHex']) + bytes.fromhex(encoded['usedRecordsHex'])
        raw += bytes(encoded['zeroTailBytes'])
        assert hashlib.sha256(raw).hexdigest().upper() == encoded['rawSha256']
        magic, version, capacity, frequency, pid, count, dropped, size, _, _ = struct.unpack_from(
            '<8sIIqIiiIqq8x', raw
        )
        assert magic == b'UDMUIT1\0' and version == 1 and capacity == 8192
        assert frequency > 0 and pid == receipt['pid'] and dropped == run['dropped'] == 0
        assert size == 64 and 0 < count <= capacity and len(raw) == 64 + capacity * size
        assert count == run['nativeEvents'] == len(run['events'])
        decoded = []
        for index in range(count):
            ready, thread, counter, finished, label = struct.unpack_from(
                '<iIqq40s', raw, 64 + index * 64
            )
            assert ready == 1 and thread > 0 and finished >= counter > 0
            decoded.append((counter, thread, finished, label.split(b'\0', 1)[0].decode()))
        decoded.sort(key=lambda r: r[0])
        assert decoded == [(e['counter'], e['thread'], e['finished'], e['stage'])
                           for e in run['events']]
        labels = {}
        for counter, thread, _, label in decoded:
            labels.setdefault((thread, counter), set()).add(label)
        spans = run['spans']
        for span in spans:
            first, last = (("stop-entry", "stop-return") if span['stage'] == 'manager-stop-native'
                           else (span['stage'] + '-begin', span['stage'] + '-end'))
            assert first in labels.get((span['thread'], span['beginCounter']), set())
            assert last in labels.get((span['thread'], span['endCounter']), set())
            assert span['endCounter'] >= span['beginCounter']
            assert abs(span['durationMs'] -
                       (span['endCounter'] - span['beginCounter']) / frequency * 1000) < 1e-6
        body = [s for s in spans if s['stage'] == 'body-flush']
        intent = [s for s in spans if s['stage'] == 'intent-write']
        catalog = [s for s in spans if s['stage'] == 'catalog-write']
        assert len(body) == len(intent) == len(catalog) == 1
        writes = [s for s in spans if s['stage'] == 'body-write']
        assert writes
        last_write = max(s['endCounter'] for s in writes)
        assert body[0]['endCounter'] <= catalog[0]['beginCounter']
        overlap = max(0, min(body[0]['endCounter'], intent[0]['endCounter']) -
                      max(body[0]['beginCounter'], intent[0]['beginCounter'])) / frequency * 1000
        assert abs(overlap - run['bodyIntentOverlapMs']) < 1e-6 and overlap > 0
        final_catalog = [s for s in spans if s['stage'] == 'cat-atomic'
                         and s['beginCounter'] >= last_write]
        assert len(final_catalog) == 1
        assert catalog[0]['beginCounter'] <= final_catalog[0]['beginCounter']
        assert final_catalog[0]['endCounter'] <= catalog[0]['endCounter']
        reads = [s['durationMs'] for s in spans if s['stage'] == 'catalog-read']
        drains = [s for s in spans if s['stage'] == 'data-only-flush']
        statistics = run.get('prefinishWriteback')
        if statistics is not None and statistics.get('Mode') == 'data-only':
            assert len(drains) == statistics['Flushes']
        overlap_writes = [w for w in writes if any(
            max(w['beginCounter'], drain['beginCounter']) <
            min(w['endCounter'], drain['endCounter']) for drain in drains)]
        points = {e['stage']: e['counter'] for e in run['events']}
        close_ms = (points['close-return'] - points['windows-cleanup-end']) / frequency * 1000
        notify_ms = (points['close-entry'] - points['worker-notify-end']) / frequency * 1000
        assert close_ms >= 0 and notify_ms >= 0
        final_atomic = final_catalog[0]
        child_flushes = [s for s in spans if s['stage'] == 'atomic-flush'
                         and s['thread'] == final_atomic['thread']
                         and final_atomic['beginCounter'] <= s['beginCounter']
                         <= s['endCounter'] <= final_atomic['endCounter']]
        child_replaces = [s for s in spans if s['stage'] == 'atomic-replace'
                          and s['thread'] == final_atomic['thread']
                          and final_atomic['beginCounter'] <= s['beginCounter']
                          <= s['endCounter'] <= final_atomic['endCounter']]
        assert len(child_flushes) == len(child_replaces) == 1
        summaries.append(dict(
            case=run['case'], events=count, diagnosticOnly=True,
            requestedBackground=run['case'] != 'visible-4m',
            bodyFlushMs=body[0]['durationMs'], intentMs=intent[0]['durationMs'],
            bodyIntentOverlapMs=overlap, catalogMs=catalog[0]['durationMs'],
            catalogFlushMs=child_flushes[0]['durationMs'],
            catalogReplaceMs=child_replaces[0]['durationMs'],
            catalogTransactionsAfterLastBodyWrite=len(final_catalog),
            replacementBackoffs=sum(s['stage'] == 'atomic-backoff' for s in spans),
            catalogReadMs=reads, closeBaseIntervalMs=close_ms,
            dataOnlyCallMs=[s['durationMs'] for s in drains],
            dataOnlyStatistics=statistics,
            bodyWriteCount=len(writes),
            longestBodyWriteMs=max(s['durationMs'] for s in writes),
            writesOverlappingDataOnlyCalls=len(overlap_writes),
            longestOverlappingWriteMs=max([s['durationMs'] for s in overlap_writes] or [0]),
            notifyToCloseMs=notify_ms, actualProcessExitTailMs=run['exitTailMs'],
            exclusiveAvailabilityTailMs=run['exclusiveAvailabilityTailMs']
        ))
    return dict(passed=True, diagnosticOnly=True, runs=summaries,
                normalProductSpeedRanking=False, newIdmComparison=False,
                kernelQueueFilterOrSchedulingCauseProven=False,
                removedPayloadLogicalBytes=sum(r['bytes'] for r in data['runs']))


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('area', type=Path)
    parser.add_argument('output', type=Path)
    parser.add_argument('--case', action='append', dest='cases')
    args = parser.parse_args()
    report = analyze(args.area, args.cases)
    args.output.write_text(json.dumps(report, indent=2) + '\n', encoding='utf-8')
    print(f"Validated {len(report['runs'])} saved operation timelines; no target launched")
