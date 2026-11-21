"""Independent qualification for scheduling-only traces; never FileIo or cause proof."""
import copy
import json
import argparse
from pathlib import Path


def evaluate(report, native, pid, image):
    failures = []
    def require(ok, label):
        if not ok:
            failures.append(label)
    integer = lambda v: type(v) is int
    require(report.get('traceProfile') == 'disk-scheduling-no-fileio-v1', 'profile')
    require(report.get('fileOperationAttributionSupported') is False, 'scope')
    # Process, Thread, DiskIO, DiskIOInit, CSwitch, Dispatcher, NoSysConfig.
    require(report.get('enabledFlags') == 0x10000D13, 'provider flags')
    require(report.get('started') is True, 'start')
    for key in ('eventsLost', 'buffersLost', 'stopStatus', 'processTraceStatus',
                'unknownMetadataEvents'):
        require(integer(report.get(key)) and report[key] == 0, key)
    for key in ('callbackFailed', 'ambiguous', 'limit', 'normalWorkloadLaunchedByCollector'):
        require(report.get(key) is False, key)
    require(report.get('sessionAbsent') is True and report.get('postStopQueryStatus') == 4201, 'session absence')
    require(report.get('reason') == 'owner-complete', 'complete recording')
    require(report.get('closeTraceStatus') in (0, 7007), 'close status')
    require(integer(report.get('counterFrequency')) and report['counterFrequency'] > 0, 'frequency')
    require(integer(report.get('actualBuffers')) and integer(report.get('bufferSizeKiB'))
            and 0 < report['actualBuffers'] * report['bufferSizeKiB'] <= 16384, 'buffers')
    targets = report.get('targets', [])
    require(integer(pid) and pid > 0 and len(targets) == 1, 'target count/PID')
    target = targets[0] if len(targets) == 1 else {}
    require(target.get('pid') == pid and target.get('image') == image.lower()
            and target.get('imageMatches') is True, 'identity')
    require(target.get('starts') == target.get('ends') == 1, 'lifetime count')
    begin, end = target.get('beginCounter'), target.get('endCounter')
    lifetime = integer(begin) and integer(end) and 0 < begin < end
    require(lifetime, 'lifetime interval')
    for key in ('callbackFailed', 'ambiguous', 'limit'):
        require(target.get(key) is False, 'target '+key)
    require(target.get('targetSchemaErrors') == 0
            and target.get('targetSchemaFailureInventory') == {}, 'target schema')
    require(target.get('pendingFlushes') == 0 and target.get('flushPairs') == [], 'unexpected FileIo')
    require(native.get('pid') == pid and native.get('elevated') is False
            and native.get('passed') is True and native.get('bytes') == 8192, 'fixture provenance')
    calls = native.get('flushCalls', [])
    require(len(calls) == 2, 'native calls')
    for call in calls:
        a, b = call.get('beginCounter'), call.get('endCounter')
        require(lifetime and integer(a) and integer(b) and begin <= a <= b <= end
                and call.get('ok') is True and call.get('error') == 0
                and integer(call.get('thread')) and call['thread'] > 0, 'native interval')
    switches = target.get('threadScheduling', [])
    disks = target.get('diskEvents', [])
    require(bool(switches) and bool(disks), 'positive scheduling and disk evidence')
    for row in switches + disks:
        time = row.get('counter')
        require(lifetime and integer(time) and begin <= time <= end, 'event interval')
    require(any(row.get('opcode') == 'cswitch' for row in switches), 'context switch evidence')
    # Positive scheduling elsewhere in the process is insufficient for this probe.
    # Require a decoded switch involving an actual measured flush's issuing thread.
    linked = False
    for call in calls:
        for row in switches:
            if row.get('opcode') != 'cswitch':
                continue
            a, b, time = call.get('beginCounter'), call.get('endCounter'), row.get('counter')
            if (all(integer(x) for x in (a, b, time, call.get('thread')))
                    and a <= time <= b and call['thread'] > 0
                    and call['thread'] in (row.get('oldThread'), row.get('newThread'))):
                linked = True
    require(linked, 'native flush thread scheduling overlap')
    return dict(passed=not failures, failures=failures, kernelCauseVerified=False,
                fileOperationAttributionSupported=False,
                scope='fresh normal-token fixture lifetime, decoded disk and scheduling events; no file-operation or cause attribution')


def self_test():
    target = dict(pid=123, image='u12345678f.exe', imageMatches=True, starts=1, ends=1,
                  beginCounter=1, endCounter=100, callbackFailed=False, ambiguous=False,
                  limit=False, targetSchemaErrors=0, targetSchemaFailureInventory={},
                  pendingFlushes=0, flushPairs=[],
                  threadScheduling=[dict(opcode='cswitch', counter=11, oldThread=9, newThread=10)],
                  diskEvents=[dict(opcode='write', counter=16)])
    report = dict(traceProfile='disk-scheduling-no-fileio-v1',
                  fileOperationAttributionSupported=False, enabledFlags=0x10000D13,
                  started=True, eventsLost=0, buffersLost=0, stopStatus=0,
                  processTraceStatus=0, unknownMetadataEvents=0, callbackFailed=False,
                  ambiguous=False, limit=False, normalWorkloadLaunchedByCollector=False,
                  sessionAbsent=True, postStopQueryStatus=4201, reason='owner-complete',
                  closeTraceStatus=7007, counterFrequency=10000000, actualBuffers=32,
                  bufferSizeKiB=64, targets=[target])
    native = dict(pid=123, elevated=False, passed=True, bytes=8192,
                  flushCalls=[dict(beginCounter=a, endCounter=a+3, thread=9,
                                   ok=True, error=0) for a in (10, 30)])
    check = lambda r, n=native: evaluate(r, n, 123, 'u12345678f.exe')['passed']
    assert check(report)
    count = 1
    for key, value in [('traceProfile', 'fileio-disk-scheduling-v1'),
                       ('fileOperationAttributionSupported', True), ('enabledFlags', 0),
                       ('unknownMetadataEvents', 1), ('eventsLost', 1), ('buffersLost', 1),
                       ('stopStatus', 5), ('sessionAbsent', False), ('reason', 'deadline'),
                       ('actualBuffers', 257), ('counterFrequency', True),
                       ('callbackFailed', True), ('normalWorkloadLaunchedByCollector', True)]:
        r = copy.deepcopy(report); r[key] = value
        assert not check(r); count += 1
    for key, value in [('ends', 0), ('starts', 2), ('imageMatches', False),
                       ('targetSchemaErrors', 26), ('threadScheduling', []),
                       ('diskEvents', []), ('flushPairs', [{}]), ('pendingFlushes', 1)]:
        r = copy.deepcopy(report); r['targets'][0][key] = value
        assert not check(r); count += 1
    n = copy.deepcopy(native); n['elevated'] = True
    assert not check(report, n); count += 1
    r = copy.deepcopy(report); r['targets'][0]['threadScheduling'][0]['counter'] = 101
    assert not check(r); count += 1
    r = copy.deepcopy(report); r['targets'][0]['threadScheduling'][0]['oldThread'] = 11
    assert not check(r); count += 1
    r = copy.deepcopy(report); r['targets'][0]['threadScheduling'][0]['counter'] = 20
    assert not check(r); count += 1
    print(json.dumps(dict(passed=True, policyChecks=count, liveCoverageProven=False)))


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--self-test', action='store_true')
    parser.add_argument('--report', type=Path)
    parser.add_argument('--native', type=Path)
    parser.add_argument('--expected-pid', type=int)
    parser.add_argument('--expected-image')
    parser.add_argument('--output', type=Path)
    args = parser.parse_args()
    if args.self_test:
        self_test()
    else:
        if not all((args.report, args.native, args.expected_pid, args.expected_image)):
            parser.error('report, native, independent PID and image are required')
        def load(path):
            with path.open('rb') as stream:
                data = stream.read(32*1024*1024+1)
            if len(data) > 32*1024*1024:
                raise ValueError('report too large')
            return json.loads(data.decode('utf-8-sig'))
        result = evaluate(load(args.report), load(args.native),
                          args.expected_pid, args.expected_image)
        text = json.dumps(result, indent=2)
        if args.output:
            args.output.write_text(text+'\n', encoding='utf-8')
        print(text)
        raise SystemExit(0 if result['passed'] else 1)
