"""Validate and redact owned MFC-app observer runs; no target execution/deletion."""
import argparse
import hashlib
import json
from pathlib import Path


def normalized(path):
    return path.removeprefix('\\\\?\\').replace('\\', '/').lower()


def summarize(folder):
    source = folder / 'result-private.json'
    raw = source.read_bytes()
    run = json.loads(raw)
    cleanup = json.loads((folder / 'payload-cleanup.json').read_bytes())
    assert run['passed'] and run['normalProcessExit'] and run['hashMatches']
    assert cleanup['passed'] and cleanup['bytesRemoved'] == run['size']
    result = {k: run[k] for k in ('mode', 'drive', 'size', 'connections',
              'normalProcessExit', 'hashMatches', 'sourceBinarySha256',
              'agentSha256')}
    result['readAddonSha256'] = run.get('readAddonSha256')
    result.update(sourceReceiptSha256=hashlib.sha256(raw).hexdigest().upper(),
                  payloadRemoved=True, performanceComparison=False,
                  fullProcessLifetimeTiming=False, kernelCauseVerified=False)
    trace = run['snapshot']
    if trace is None:
        return result
    events = trace['records']
    assert trace['dropped'] == trace['openFiles'] == trace['openMappings'] == 0
    assert not run['messages']
    assert all(e['end'] >= e['begin'] for e in events)
    frequency = trace['frequency']
    ms = lambda ticks: round(ticks * 1000 / frequency, 6)
    body = [e for e in events if e['api'] == 'NtWriteFile'
            and normalized(e['path']).endswith('.data')]
    assert body and all(e['ok'] and not e['pending'] for e in body)
    assert len({normalized(e['path']) for e in body}) == 1
    cursor = 0
    for event in sorted(body, key=lambda e: int(e['byteOffset'])):
        assert int(event['byteOffset']) == cursor
        assert event['completed'] == event['requested'] > 0
        cursor += event['completed']
    assert cursor == run['size']
    native = [e for e in events if e['api'] in
              ('NtWriteFile', 'NtFlushBuffersFile', 'NtFlushBuffersFileEx')]
    for event in native:
        api = 'WriteFile' if event['api'] == 'NtWriteFile' else 'FlushFileBuffers'
        parents = [p for p in events if p['api'] == api
                   and p['tid'] == event['tid'] and p['handle'] == event['handle']
                   and normalized(p['path']) == normalized(event['path'])
                   and p['begin'] <= event['begin'] <= event['end'] <= p['end']]
        assert len(parents) == 1, (event['id'], len(parents))
    last_write = max(e['end'] for e in body)
    app_return = run['appReturn']['appReturnedCounter']
    assert app_return >= last_write

    def role(event):
        path = normalized(event['path'])
        if path.endswith('.data'):
            return 'body'
        if '/publications/' in path:
            return 'publication-journal'
        if path.endswith('/state.json.tmp'):
            return 'catalog-temporary'
        if path.endswith('/state.json'):
            return 'catalog'
        if ':zone.identifier' in path:
            return 'zone-metadata'
        return 'other-owned-file'

    def describe(event):
        value = dict(api=event['api'], role=role(event),
                     beginAfterLastBodyWriteMs=ms(event['begin']-last_write),
                     durationMs=ms(event['end']-event['begin']), ok=event['ok'])
        if 'completed' in event:
            value['completedBytes'] = event['completed']
        return value

    # Native intervals nest inside Win32 intervals: deliberately do not add them.
    tail = [e for e in events if e['begin'] >= last_write
            and e['end'] <= app_return]
    result.update(
        events=len(events), dropped=0, openFiles=0, openMappings=0,
        nativeBodyWrites=len(body), contiguousBodyBytes=cursor,
        nativeCallsWithUniqueWin32Envelopes=len(native),
        lastBodyWriteToMfcReturnMs=ms(app_return-last_write),
        nativeFlushes=[describe(e) for e in native if 'Flush' in e['api']],
        tailFileOperations=[describe(e) for e in tail],
        joinWaits=run['job'].get('HttpJoinWaits'),
        hostingRuntimeWarning='Unable to start idund: idund did not start listening',
        warningProvenance='Observed on controller console; not target stderr',
        hooksLeftLoadedThroughNormalExit=run.get('hooksLeftLoadedThroughNormalExit', False))
    return result


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('area', type=Path)
    parser.add_argument('output', type=Path)
    args = parser.parse_args()
    folders = sorted(args.area.glob('run-*'))
    assert folders
    results = [summarize(folder) for folder in folders]
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_text(json.dumps(dict(schema=1, passed=True, runs=results),
                                     indent=2)+'\n', encoding='utf-8')
    for result in results:
        print(json.dumps({k: result[k] for k in ('mode', 'drive', 'size',
                         'connections', 'lastBodyWriteToMfcReturnMs') if k in result}))


if __name__ == '__main__':
    main()
