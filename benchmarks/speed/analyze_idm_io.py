"""Read-only validation of the bounded single-part IDM file-I/O diagnostic.

This qualifies observed user-mode calls, not kernel/device writeback, power-loss
durability, uninstrumented performance or all IDM paths. Never double-count
nested Win32/native writes. Opaque parts need a proven move to the owned output.
"""
import argparse
import hashlib
import json
import ntpath
from pathlib import Path

TARGET = '03CC62E9ADB77A380F9DC12F67CCAAEE5106F12844AA73CE32C914DDD16D607C'
FLUSH = {'FlushFileBuffers', 'NtFlushBuffersFile', 'NtFlushBuffersFileEx', 'file-part-flush'}


def analyze(data):
    inputs, receipt = data['inputs'], data['acceptance']
    evidence = data['frida']['result']['structuredContent']
    observed = evidence['normalized_result']
    assert inputs['targetSha256'] == TARGET and receipt['targetBinaryUnchanged']
    assert inputs['agentSha256'].lower() == observed['source_sha256'].lower(), 'Observer source mismatch'
    assert observed['mode'] == 'spawn' and observed['state'] == 'detached'
    assert observed['pid'] == receipt['targetPid'] and not receipt['currentSessionIdmRemaining']
    assert receipt['settingsRestored'] and receipt['hashMatches'] and receipt['bytes'] == inputs['bytes']
    assert not observed['messages_truncated'] and observed['cleanup_error'] is None
    assert all(m['type'] == 'send' for m in observed['messages']), 'Script error in observation'
    payloads = [m['payload'] for m in observed['messages']]
    assert not any(p.get('kind') == 'capture-cap' for p in payloads), 'Agent capture cap reached'
    ready = [p for p in payloads if p.get('kind') == 'observer-ready']
    assert len(ready) == 1 and ready[0]['architecture'] == 'ia32' and ready[0]['pid'] == observed['pid']
    assert ready[0]['scope'] == inputs['token']
    required = {'CreateFileW', 'WriteFile', 'NtWriteFile', 'FlushFileBuffers',
                'NtFlushBuffersFile', 'CloseHandle', 'file-part-open', 'file-part-close', 'file-part-flush', 'MoveFileW'}
    assert required <= set(ready[0]['hooks']), 'Required observer hook missing'
    events = [p for p in payloads if p.get('kind') == 'owned-io']
    assert [e['sequence'] for e in events] == list(range(1, len(events) + 1)), 'Incomplete event sequence'
    assert all(e['pid'] == observed['pid'] and e['begin'] <= e['end'] for e in events)
    adopted = [e for e in events if e['api'] == 'candidate-part-adopted']
    assert len(adopted) == 1, 'Exactly one opaque candidate part is required'
    part = adopted[0]['path']
    part_events = [e for e in events if e.get('path') == part]
    methods = [e for e in events if e['api'] in {'file-part-open', 'file-part-close'}]
    assert len(methods) == 2 and all(e.get('matchedScope') and e.get('path') == part for e in methods), 'Unscoped file-part operation'
    assert {e['api'] for e in methods} == {'file-part-open', 'file-part-close'}
    assert next(e for e in methods if e['api'] == 'file-part-open')['returnCode'] == 0
    opens = [e for e in part_events if e['api'] == 'CreateFileW' and e.get('disposition') == 2]
    assert len(opens) == 1 and opens[0]['ok'] and opens[0]['access'] == 0xc0000000
    assert opens[0]['flags'] == 0 and opens[0]['share'] == 1, 'Observed create mode differs from the investigated cached part path'
    handle = opens[0]['handle']
    writes = [e for e in part_events if e['api'] == 'WriteFile']
    natives = [e for e in part_events if e['api'] == 'NtWriteFile']
    assert writes and len(writes) == len(natives), 'Missing Win32/native write coverage'
    by_sequence = {e['sequence']: e for e in events}
    for w in writes:
        assert w['handle'] == handle and w['ok'] and not w['overlapped'] and w['completed'] == w['requested']
        n = by_sequence.get(w['sequence'] - 1, {})
        assert n.get('api') == 'NtWriteFile' and n['handle'] == handle and n['tid'] == w['tid'], 'Native write is not paired with its Win32 envelope'
        assert n['ok'] and not n['pending'] and n['requested'] == n['completed'] == w['completed']
        assert w['begin'] <= n['begin'] <= n['end'] <= w['end'], 'Native write is outside its enclosing Win32 interval'
    assert sum(w['completed'] for w in writes) == inputs['bytes'], 'Body write total does not match the complete output'
    last_write = max(w['sequence'] for w in writes)
    closes = [e for e in part_events if e['api'] == 'CloseHandle' and e['handle'] == handle and e['ok'] and e['sequence'] > last_write]
    assert closes, 'Writer close was not observed'
    close = min(closes, key=lambda e: e['sequence'])
    assert opens[0]['sequence'] < min(w['sequence'] for w in writes) < close['sequence']
    wrapper_close = next(e for e in methods if e['api'] == 'file-part-close')
    assert wrapper_close['handle'] == handle and wrapper_close['sequence'] > close['sequence']
    moves = [e for e in events if e['api'] == 'MoveFileW' and e.get('ok') and e.get('from') == part
             and ntpath.basename(e.get('to', '')).lower() == inputs['token'].lower() + '.bin' and e['sequence'] > close['sequence']]
    assert len(moves) == 1, 'Opaque part is not associated with the exact owned output'
    flushes = [e for e in part_events if e['api'] in FLUSH and e.get('handle') == handle
               and opens[0]['sequence'] < e['sequence'] < close['sequence']]
    counts = {api: sum(e['api'] == api for e in flushes) for api in sorted(FLUSH)}
    return dict(coverageQualified=True, bytes=inputs['bytes'], targetSha256=TARGET,
                win32BodyWrites=len(writes), nativeWriteEnvelopesValidated=len(natives),
                logicalBodyBytes=sum(w['completed'] for w in writes), nativeBytesCountedAgain=False,
                creationFlags=opens[0]['flags'], desiredAccess=opens[0]['access'], share=opens[0]['share'],
                writerSessionFlushCalls=counts, noObservedWriterSessionFlushCalls=not flushes,
                observedFlushBeforeFinalWrite=any(e['sequence'] < last_write for e in flushes),
                observedFlushAfterFinalWrite=any(e['sequence'] > last_write for e in flushes),
                successfulMoveAssociatesOpaquePartWithOwnedOutput=True,
                partDrive=ntpath.splitdrive(part)[0] or data.get('volumeRoles', {}).get('part'),
                outputDrive=ntpath.splitdrive(moves[0]['to'])[0] or data.get('volumeRoles', {}).get('output'),
                postExitOutputFlushMs=receipt.get('postExitExplicitFlushMs'),
                wholeApplicationFlushAbsenceProven=False, kernelOrDeviceWritebackMeasured=False,
                performanceComparison=False, productionChangeAdmitted=False,
                scope='One owned single-part transfer. Native opens inside OS move/copy, mappings, indirect paths and kernel writeback are not exhaustively observed. Clock readings are diagnostic only.')


def load(folder):
    names = {'inputs': 'inputs.json', 'acceptance': 'acceptance.json', 'frida': 'frida-response.json'}
    data = {key: json.loads((folder / name).read_text(encoding='utf-8-sig')) for key, name in names.items()}
    if (folder / 'volume-roles.json').is_file():
        data['volumeRoles'] = json.loads((folder / 'volume-roles.json').read_text(encoding='utf-8-sig'))
    return data


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('folder', type=Path)
    parser.add_argument('--output', type=Path, required=True)
    args = parser.parse_args()
    data = load(args.folder)
    report = analyze(data)
    token = data['inputs']['token']
    payload = args.folder / (token + '.bin')
    report['payloadRehashedNow'] = payload.is_file()
    if payload.is_file():
        assert hashlib.sha256(payload.read_bytes()).hexdigest().upper() == data['acceptance']['sha256']
    args.output.write_text(json.dumps(report, indent=2) + '\n', encoding='utf-8')
    print(json.dumps(report))
