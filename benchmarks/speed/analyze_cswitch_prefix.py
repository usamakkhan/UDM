"""Offline selected-prefix analysis; never converts failed full-schema qualification to a pass."""
import argparse
import hashlib
import json
import struct
from pathlib import Path

PROVIDER = '3d6fa8d1-fe05-11d0-9dda-00c04fd7ba7c'
KEY = PROVIDER+':0:36:5:848:28'


def require(value, message):
    if not value:
        raise ValueError(message)


def integer(value):
    return type(value) is int


def decode(schema, row):
    expected = dict(provider=PROVIDER, eventId=0, opcode=36, version=5,
                    headerFlags=848, payloadBytes=28, requeryStatus=1168)
    require(all(type(schema.get(k)) is type(v) and schema[k] == v
                for k, v in expected.items()), 'unsupported schema')
    require(row.get('schemaKey') == KEY, 'record schema key')
    require(row.get('headerProcess') == row.get('headerThread') == 0xffffffff,
            'unexpected header identity')
    raw = bytes.fromhex(row['payloadHex'])
    require(len(raw) == 28, 'payload length')
    new, old = struct.unpack_from('<II', raw)
    # Selected prefix fields agree with Microsoft's version-tolerant PerfView
    # parser. No wait mode/flags, priority, CPU mapping or extension is decoded.
    return dict(counter=row['counter'], newThread=new, oldThread=old,
                oldWaitReason=raw[12], oldState=raw[14])


def verify_controls(controls):
    require(controls.get('passed') is True and controls.get('elevated') is False
            and controls.get('syntheticMetadataQueriesOnly') is True,
            'TDH control provenance')
    rows = controls.get('knownSchemaPrefixControls', [])
    require(len(rows) == 12 and controls.get('prefixFieldsVerified') == 48,
            'TDH control coverage')
    seen = set()
    for row in rows:
        key = (row.get('version'), row.get('payloadBytes'), row.get('pattern'))
        require(key not in seen, 'duplicate TDH control')
        seen.add(key)
        require(row.get('headerFlags') == 848 and row.get('passed') is True,
                'TDH header control')
        pattern = row['pattern']
        expected = ((0x87654321, 0x10203040, 0, 1) if pattern
                    else (0x12345678, 0x76543210, 37, 5))
        require(tuple(row.get(k) for k in ('newThread', 'oldThread',
                                          'oldWaitReason', 'oldState')) == expected,
                'TDH prefix fields')
    require(seen == {(v, n, p) for v in (2, 3, 4) for n in (24, 28) for p in (0, 1)},
            'TDH version/length controls')


def analyze(report, native, controls, expected_pid, expected_image):
    result = dict(passed=False, fullSchemaQualificationPassed=False,
                  kernelCauseVerified=False, extensionBytesInterpreted=False,
                  payloadIntegrityReverified=False,
                  performanceComparison=False,
                  scope='selected thread-ID/state/wait-reason prefix fields and native-flush overlap only')
    try:
        verify_controls(controls)
        profile = report.get('traceProfile')
        filtered = profile == 'disk-scheduling-target-threads-v3'
        require(filtered or profile == 'disk-scheduling-no-fileio-v2-inventory', 'profile')
        require(report.get('enabledFlags') == 0x10000D13, 'provider flags')
        require(report.get('fileOperationAttributionSupported') is False, 'file-operation scope')
        for key in ('eventsLost', 'buffersLost', 'stopStatus', 'processTraceStatus'):
            require(integer(report.get(key)) and report[key] == 0, key)
        for key in ('callbackFailed', 'ambiguous', 'limit', 'normalWorkloadLaunchedByCollector'):
            require(report.get(key) is False, key)
        require(report.get('sessionAbsent') is True and report.get('postStopQueryStatus') == 4201,
                'session absence')
        require(report.get('reason') == 'owner-complete'
                and report.get('closeTraceStatus') in (0, 7007), 'complete recording')
        hz = report.get('counterFrequency')
        require(integer(hz) and hz > 0, 'frequency')
        require(integer(report.get('actualBuffers')) and integer(report.get('bufferSizeKiB'))
                and 0 < report['actualBuffers']*report['bufferSizeKiB'] <= 16384, 'buffers')
        targets = report.get('targets', [])
        require(len(targets) == 1 and integer(expected_pid) and expected_pid > 0, 'target count/PID')
        target = targets[0]
        require(target.get('pid') == expected_pid and target.get('image') == expected_image.lower()
                and target.get('imageMatches') is True, 'target identity')
        require(target.get('starts') == target.get('ends') == 1, 'lifetime count')
        begin, end = target.get('beginCounter'), target.get('endCounter')
        require(integer(begin) and integer(end) and 0 < begin < end, 'lifetime interval')
        require(all(target.get(k) is False for k in ('callbackFailed', 'ambiguous', 'limit')),
                'target capture status')
        require(target.get('targetSchemaErrors') == 0
                and target.get('targetSchemaFailureInventory') == {}, 'target-header schema errors')
        require(target.get('pendingFlushes') == 0 and target.get('flushPairs') == [], 'unexpected FileIo')
        require(native.get('pid') == expected_pid and native.get('elevated') is False
                and native.get('passed') is True and native.get('bytes') == 8192, 'native provenance')
        calls = native.get('flushCalls', [])
        require(len(calls) == 2, 'native calls')
        for call in calls:
            a, b = call.get('beginCounter'), call.get('endCounter')
            require(integer(a) and integer(b) and begin <= a < b <= end
                    and integer(call.get('thread')) and call['thread'] > 0
                    and call.get('ok') is True and call.get('error') == 0, 'native interval')
        require(calls[0]['endCounter'] <= calls[1]['beginCounter'], 'overlapping native calls')
        require(len({call['thread'] for call in calls}) == 1, 'fixture thread identity')
        inventory = report['unknownSchemaInventory']
        require(inventory.get('capped') is False, 'inventory cap')
        if filtered:
            require(inventory.get('targetThreadFilter') is True
                    and inventory.get('selectedIdPrefixReadForFiltering') is True
                    and inventory.get('payloadsFullyDecoded') is False, 'filter scope')
        else:
            require(inventory.get('payloadsDecoded') is False, 'inventory scope')
        require(set(inventory.get('schemas', {})) == {KEY}, 'unaccounted unknown schema')
        schema = inventory['schemas'][KEY]
        require(integer(schema.get('count')) and schema['count'] > 0
                and schema['count'] == report.get('unknownMetadataEvents'), 'unknown count accounting')
        raw = inventory.get('threadWindowRecords', [])
        require(0 < len(raw) <= 16384, 'window records')
        if filtered:
            for key in ('threadWindowEvents', 'retainedTargetThreadEvents',
                        'unmatchedTargetThreadEvents', 'unsupportedTargetThreadForms'):
                require(integer(inventory.get(key)) and inventory[key] >= 0, 'filter counter '+key)
            require(inventory['retainedTargetThreadEvents'] == len(raw)
                    and inventory['unsupportedTargetThreadForms'] == 0
                    and inventory['threadWindowEvents'] == len(raw)+inventory['unmatchedTargetThreadEvents'],
                    'filter accounting')
        else:
            require(inventory.get('threadWindowEvents') == len(raw), 'window record accounting')
        rows = [decode(schema, row) for row in raw]
        require(all(integer(r['counter']) and begin <= r['counter'] <= end for r in rows),
                'window interval')
        require(all(a['counter'] <= b['counter'] for a, b in zip(rows, rows[1:])), 'record order')
        if filtered:
            lifecycle = target.get('threadLifecycle', [])
            require(lifecycle, 'thread lifecycle missing')
            active, spans = {}, {}
            previous = begin
            for event in lifecycle:
                time, thread, op = event.get('counter'), event.get('thread'), event.get('opcode')
                require(integer(time) and previous <= time <= end and integer(thread)
                        and thread > 0, 'thread lifecycle interval')
                previous = time
                if op in ('start', 'dcstart'):
                    require(thread not in active, 'duplicate thread start')
                    active[thread] = time
                elif op == 'end':
                    require(thread in active, 'unmatched thread end')
                    spans.setdefault(thread, []).append((active.pop(thread), time))
                else:
                    raise ValueError('thread lifecycle opcode')
            for thread, start in active.items():
                spans.setdefault(thread, []).append((start, end))
            def belongs(thread, a, b):
                return any(lo <= a <= b <= hi for lo, hi in spans.get(thread, []))
            require(all(belongs(row['oldThread'], row['counter'], row['counter'])
                        or belongs(row['newThread'], row['counter'], row['counter']) for row in rows),
                    'retained record outside known thread lifetime')
            require(all(belongs(call['thread'], call['beginCounter'], call['endCounter']) for call in calls),
                    'native call outside known thread lifetime')
        tid = calls[0]['thread']
        relevant = [r for r in rows if tid in (r['oldThread'], r['newThread'])
                    and (not filtered or belongs(tid,r['counter'],r['counter']))]
        require(relevant and relevant[0]['newThread'] == tid, 'missing initial switch-in')
        running = False
        pending = None
        intervals = []
        for row in relevant:
            require(row['newThread'] != row['oldThread'], 'self context switch')
            if row['newThread'] == tid:
                require(not running, 'duplicate switch-in')
                running = True
                if pending is not None:
                    require(pending['counter'] <= row['counter'], 'switch interval')
                    intervals.append((pending, row))
                    pending = None
            else:
                require(running, 'unpaired switch-out')
                running = False
                pending = row
        terminal_boundary = 'switch-out'
        if running:
            # The filtered collector removes a thread at its TDH End event, so
            # its later terminating switch-out can legitimately be excluded.
            # Admit that boundary only after a witnessed post-call switch-in
            # and a unique, closed lifetime. Never synthesize an interval.
            ends = [r['counter'] for r in target.get('threadLifecycle', [])
                    if r.get('thread') == tid and r.get('opcode') == 'end']
            require(filtered and len(ends) == 1 and len([r for r in target['threadLifecycle']
                        if r.get('thread') == tid and r.get('opcode') in ('start','dcstart')]) == 1
                    and calls[-1]['endCounter'] <= relevant[-1]['counter'] < ends[0],
                    'missing final switch-out')
            terminal_boundary = 'tdh-thread-end-after-post-call-switch-in'
        # An unfinished post-call exit interval does not imply missing flush time.
        require(pending is None or pending['counter'] >= calls[-1]['endCounter'],
                'unfinished native-call off-CPU interval')
        ready = [r['counter'] for r in target.get('threadScheduling', [])
                 if r.get('opcode') == 'readythread' and r.get('readyThread') == tid
                 and integer(r.get('counter')) and begin <= r['counter'] <= end]
        summaries = []
        scale = 1000/hz
        for call in calls:
            a, b = call['beginCounter'], call['endCounter']
            spans = []
            total = 0
            for out, incoming in intervals:
                lo, hi = max(a, out['counter']), min(b, incoming['counter'])
                if hi <= lo:
                    continue
                total += hi-lo
                notifications = [t for t in ready if out['counter'] <= t <= incoming['counter']]
                row = dict(beginAfterNativeStartMs=(lo-a)*scale,
                           offCpuMs=(hi-lo)*scale, oldStateByte=out['oldState'],
                           oldWaitReasonByte=out['oldWaitReason'])
                if notifications:
                    first = min(notifications)
                    row['afterFirstReadyNotificationMs'] = max(0, hi-max(lo, first))*scale
                spans.append(row)
            require(spans and 0 < total <= b-a, 'native scheduling overlap')
            summaries.append(dict(nativeMs=(b-a)*scale, observedOffCpuMs=total*scale,
                                  notClassifiedOffCpuMs=(b-a-total)*scale,
                                  intervals=spans))
        result.update(passed=True, failures=[], selectedPrefixRecords=len(rows),
                      targetThreadFilter=filtered,
                      nativeThreadRecords=len(relevant), flushes=summaries,
                      terminalBoundary=terminal_boundary,
                      limitations='Offsets follow the Microsoft version-tolerant parser and known-schema TDH controls. Version-5 full TDH schema remains unavailable. Off-CPU time, state/reason bytes and ReadyThread overlap do not identify disk queueing, filesystem/filter work, exact CPU time or an IDM difference.')
    except (ValueError, KeyError, TypeError, struct.error) as error:
        result['failures'] = [str(error)]
    return result


def load(path):
    with path.open('rb') as stream:
        data = stream.read(32*1024*1024+1)
    require(len(data) <= 32*1024*1024, 'report too large')
    return json.loads(data.decode('utf-8-sig'))


if __name__ == '__main__':
    p = argparse.ArgumentParser(description=__doc__)
    for name in ('report', 'native', 'controls', 'output'):
        p.add_argument('--'+name, type=Path, required=True)
    p.add_argument('--expected-pid', type=int, required=True)
    p.add_argument('--expected-image', required=True)
    args = p.parse_args()
    value = analyze(load(args.report), load(args.native), load(args.controls),
                    args.expected_pid, args.expected_image)
    value['inputSha256'] = {name: hashlib.sha256(getattr(args, name).read_bytes()).hexdigest().upper()
                           for name in ('report', 'native', 'controls')}
    args.output.write_text(json.dumps(value, indent=2)+'\n', encoding='utf-8')
    print(json.dumps({k: v for k, v in value.items() if k not in ('flushes', 'limitations', 'inputSha256')}))
    raise SystemExit(0 if value['passed'] else 1)
