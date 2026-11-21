"""Independent gates for a completed normal-token fixture trace; never infers IO cause."""
import argparse
import copy
import json
from pathlib import Path


def integer(value):
    return type(value) is int


def evaluate(report, native, expected_pid, expected_image):
    failures = []
    def require(ok, message):
        if not ok:
            failures.append(message)
    require(integer(expected_pid) and expected_pid > 0, 'independent PID required')
    require(report.get('started') is True, 'logger did not start')
    for key in ('eventsLost', 'buffersLost', 'stopStatus', 'processTraceStatus'):
        require(integer(report.get(key)) and report[key] == 0, key)
    require(report.get('sessionAbsent') is True and report.get('postStopQueryStatus') == 4201, 'logger absence unproven')
    require(report.get('closeTraceStatus') in (0, 7007), 'consumer close failed')
    for key in ('callbackFailed', 'ambiguous', 'limit'):
        require(report.get(key) is False, 'global '+key)
    require(report.get('reason') == 'owner-complete', 'recording truncated or owner vanished')
    require(report.get('normalWorkloadLaunchedByCollector') is False, 'normal controller provenance missing')
    hz = report.get('counterFrequency')
    require(integer(hz) and hz > 0, 'invalid counter frequency')
    require(integer(report.get('actualBuffers')) and integer(report.get('bufferSizeKiB')) and 0 < report['actualBuffers'] * report['bufferSizeKiB'] <= 16384, 'buffer bound')
    targets = report.get('targets', [])
    matches = [t for t in targets if t.get('pid') == expected_pid]
    require(len(targets) == 1 and len(matches) == 1, 'expected exactly the fixture target')
    if not matches:
        return {'passed': False, 'failures': failures, 'kernelCauseVerified': False}
    t = matches[0]
    require(t.get('image') == expected_image.lower() and t.get('imageMatches') is True, 'image identity')
    require(t.get('starts') == 1 and t.get('ends') == 1, 'complete single lifetime')
    begin, end = t.get('beginCounter'), t.get('endCounter')
    lifetime = integer(begin) and integer(end) and 0 < begin < end
    require(lifetime, 'lifetime interval')
    for key in ('callbackFailed', 'ambiguous', 'limit'):
        require(t.get(key) is False, 'target '+key)
    require(t.get('pendingFlushes') == 0, 'unmatched flush')
    require(integer(t.get('targetSchemaErrors')) and t['targetSchemaErrors'] == 0, 'undecoded target events')
    require(not t.get('targetSchemaFailureInventory', {}), 'failed schema inventory')
    require(native.get('pid') == expected_pid and native.get('elevated') is False and native.get('passed') is True and native.get('bytes') == 8192, 'normal fixture identity/result')
    calls, pairs = native.get('flushCalls', []), t.get('flushPairs', [])
    require(len(calls) == 2 and len(pairs) == 2, 'exactly two measured flushes required')
    intervals = []
    for call, pair in zip(calls, pairs):
        fields = [call.get('beginCounter'), pair.get('beginCounter'), pair.get('endCounter'), call.get('endCounter')]
        ordered = all(integer(x) for x in fields) and fields == sorted(fields)
        in_lifetime = ordered and lifetime and begin <= fields[0] <= fields[-1] <= end
        same_thread = integer(call.get('thread')) and call['thread'] > 0 and call['thread'] == pair.get('thread')
        success = call.get('ok') is True and call.get('error') == 0 and pair.get('status') == 0
        owned = pair.get('relativePath') == 'probe.data'
        verified = ordered and in_lifetime and same_thread and success and owned
        require(verified, 'native/ETW flush containment, identity or status')
        intervals.append({'nativeMs': 1000*(fields[-1]-fields[0])/hz if ordered and integer(hz) and hz > 0 else None, 'eventContainedInNativeCall': verified})
    return {'passed': not failures, 'failures': failures, 'measuredFlushes': intervals,
            'completeTargetSchemaCoverage': t.get('targetSchemaErrors') == 0,
            'kernelCauseVerified': False, 'payloadIntegrityReverified': False,
            'limitations': 'Interval evidence alone does not decode unknown events, prove payload integrity, attribute filters/disk queueing or replace an uninstrumented comparison.'}


def self_test():
    target = dict(pid=123, image='u12345678f.exe', imageMatches=True, starts=1, ends=1, beginCounter=1, endCounter=100, callbackFailed=False, ambiguous=False, limit=False, pendingFlushes=0, targetSchemaErrors=0, targetSchemaFailureInventory={}, flushPairs=[dict(beginCounter=a+1,endCounter=a+2,thread=9,status=0,relativePath='probe.data')for a in (10,30)])
    report = dict(started=True,eventsLost=0,buffersLost=0,stopStatus=0,processTraceStatus=0,sessionAbsent=True,postStopQueryStatus=4201,closeTraceStatus=7007,callbackFailed=False,ambiguous=False,limit=False,reason='owner-complete',normalWorkloadLaunchedByCollector=False,counterFrequency=10000000,actualBuffers=32,bufferSizeKiB=64,targets=[target])
    native = dict(pid=123,elevated=False,passed=True,bytes=8192,flushCalls=[dict(beginCounter=a,endCounter=a+3,thread=9,ok=True,error=0)for a in (10,30)])
    def accepted(r=report,n=native,p=123,image='u12345678f.exe'):return evaluate(r,n,p,image)['passed']
    assert accepted(); checks=1
    for key,value in [('eventsLost',1),('buffersLost',1),('stopStatus',5),('processTraceStatus',1223),('sessionAbsent',False),('postStopQueryStatus',0),('callbackFailed',True),('ambiguous',True),('limit',True),('reason','deadline'),('counterFrequency',0),('counterFrequency',True),('actualBuffers',257),('normalWorkloadLaunchedByCollector',True)]:
        r=copy.deepcopy(report);r[key]=value;assert not accepted(r);checks+=1
    for key,value in [('starts',2),('ends',0),('imageMatches',False),('targetSchemaErrors',26),('pendingFlushes',1),('callbackFailed',True),('limit',True),('endCounter',20),('image','foreign.exe')]:
        r=copy.deepcopy(report);r['targets'][0][key]=value;assert not accepted(r);checks+=1
    for key,value in [('relativePath','other.data'),('thread',10),('status',5),('beginCounter',9),('endCounter',14)]:
        r=copy.deepcopy(report);r['targets'][0]['flushPairs'][0][key]=value;assert not accepted(r);checks+=1
    for key,value in [('elevated',True),('pid',124),('passed',False),('bytes',4096)]:
        n=copy.deepcopy(native);n[key]=value;assert not accepted(n=n);checks+=1
    assert not accepted(p=124);checks+=1
    print(json.dumps({'passed':True,'policyChecks':checks,'liveCoverageProven':False}))


def load(path):
    with path.open('rb') as f:data=f.read(32*1024*1024+1)
    if len(data)>32*1024*1024:raise ValueError('report too large')
    return json.loads(data.decode('utf-8-sig'))


if __name__ == '__main__':
    parser=argparse.ArgumentParser();parser.add_argument('--self-test',action='store_true');parser.add_argument('--report',type=Path);parser.add_argument('--native',type=Path);parser.add_argument('--expected-pid',type=int);parser.add_argument('--expected-image');parser.add_argument('--output',type=Path);args=parser.parse_args()
    if args.self_test:self_test()
    else:
        if not all((args.report,args.native,args.expected_pid,args.expected_image)):parser.error('report, native, independent PID and image required')
        result=evaluate(load(args.report),load(args.native),args.expected_pid,args.expected_image);text=json.dumps(result,indent=2)
        if args.output:args.output.write_text(text,encoding='utf-8')
        print(text);raise SystemExit(0 if result['passed'] else 1)
