"""Reject incomplete/foreign evidence before deriving off-CPU intervals."""
import copy
import json
import struct
from analyze_cswitch_prefix import analyze, KEY, PROVIDER


def fixture():
    controls = dict(passed=True, elevated=False, syntheticMetadataQueriesOnly=True,
                    prefixFieldsVerified=48, knownSchemaPrefixControls=[])
    for version in (2, 3, 4):
        for length in (24, 28):
            for pattern in (0, 1):
                n, o, r, s = ((0x87654321, 0x10203040, 0, 1) if pattern
                              else (0x12345678, 0x76543210, 37, 5))
                controls['knownSchemaPrefixControls'].append(dict(version=version,
                    payloadBytes=length, pattern=pattern, headerFlags=848,
                    newThread=n, oldThread=o, oldWaitReason=r, oldState=s, passed=True))
    schema = dict(provider=PROVIDER, eventId=0, opcode=36, version=5,
                  headerFlags=848, payloadBytes=28, requeryStatus=1168, count=100)
    raw = []
    for counter, new, old, state in [(2,9,0,2),(11,10,9,5),(29,9,10,1),
                                     (51,10,9,5),(69,9,10,1),(80,10,9,4)]:
        payload = bytearray(28); struct.pack_into('<II', payload, 0, new, old)
        payload[14] = state
        raw.append(dict(counter=counter, schemaKey=KEY, headerProcess=0xffffffff,
                        headerThread=0xffffffff, payloadHex=payload.hex()))
    inventory = dict(capped=False, payloadsDecoded=False, schemas={KEY:schema},
                     threadWindowEvents=len(raw), threadWindowRecords=raw)
    target = dict(pid=123, image='u12345678f.exe', imageMatches=True, starts=1, ends=1,
                  beginCounter=1, endCounter=100, callbackFailed=False,
                  ambiguous=False, limit=False, targetSchemaErrors=0,
                  targetSchemaFailureInventory={}, pendingFlushes=0, flushPairs=[],
                  threadScheduling=[dict(opcode='readythread', readyThread=9,
                                         counter=t) for t in (28,68)])
    report = dict(traceProfile='disk-scheduling-no-fileio-v2-inventory',
                  enabledFlags=0x10000D13, fileOperationAttributionSupported=False,
                  eventsLost=0, buffersLost=0, stopStatus=0, processTraceStatus=0,
                  callbackFailed=False, ambiguous=False, limit=False,
                  normalWorkloadLaunchedByCollector=False, sessionAbsent=True,
                  postStopQueryStatus=4201, reason='owner-complete', closeTraceStatus=7007,
                  counterFrequency=1000, actualBuffers=32, bufferSizeKiB=64,
                  targets=[target], unknownSchemaInventory=inventory,
                  unknownMetadataEvents=100)
    native = dict(pid=123, elevated=False, passed=True, bytes=8192,
                  flushCalls=[dict(beginCounter=a, endCounter=a+20,
                                   thread=9, ok=True, error=0) for a in (10,50)])
    return report, native, controls


def main():
    checks = 0
    def run(r, n, c):
        return analyze(r, n, c, 123, 'u12345678f.exe')
    def rejected(mutator):
        nonlocal checks
        r, n, c = fixture(); mutator(r, n, c)
        assert not run(r,n,c)['passed']; checks += 1
    r,n,c=fixture();value=run(r,n,c)
    assert value['passed'] and not value['fullSchemaQualificationPassed']
    assert [v['observedOffCpuMs'] for v in value['flushes']] == [18,18]
    assert all(v['intervals'][0]['afterFirstReadyNotificationMs']==1 for v in value['flushes'])
    checks += 1
    for key, value in [('traceProfile','disk-scheduling-no-fileio-v1'),
                       ('enabledFlags',0),('eventsLost',1),('buffersLost',1),
                       ('stopStatus',5),('sessionAbsent',False),('limit',True),
                       ('counterFrequency',True),('actualBuffers',257),('unknownMetadataEvents',99),
                       ('reason','deadline'),('normalWorkloadLaunchedByCollector',True)]:
        rejected(lambda r,n,c,k=key,v=value:r.__setitem__(k,v))
    for key, value in [('ends',0),('imageMatches',False),('pid',124),('image','foreign.exe'),
                       ('targetSchemaErrors',1),('pendingFlushes',1)]:
        rejected(lambda r,n,c,k=key,v=value:r['targets'][0].__setitem__(k,v))
    for key,value in [('version',4),('version',6),('payloadBytes',24),('headerFlags',320),
                      ('provider','foreign'),('requeryStatus',0)]:
        rejected(lambda r,n,c,k=key,v=value:r['unknownSchemaInventory']['schemas'][KEY].__setitem__(k,v))
    rejected(lambda r,n,c:r['unknownSchemaInventory'].__setitem__('capped',True))
    rejected(lambda r,n,c:r['unknownSchemaInventory'].__setitem__('threadWindowEvents',7))
    rejected(lambda r,n,c:r['unknownSchemaInventory']['schemas'].__setitem__('foreign',{}))
    for key,value in [('payloadHex','zz'),('payloadHex','00'*24),('headerProcess',123),
                      ('schemaKey','foreign'),('counter',101)]:
        rejected(lambda r,n,c,k=key,v=value:r['unknownSchemaInventory']['threadWindowRecords'][0].__setitem__(k,v))
    def missing_switch(r,n,c):
        del r['unknownSchemaInventory']['threadWindowRecords'][2]
        r['unknownSchemaInventory']['threadWindowEvents']-=1
    rejected(missing_switch)
    rejected(lambda r,n,c:r['unknownSchemaInventory']['threadWindowRecords'].reverse())
    rejected(lambda r,n,c:n.__setitem__('elevated',True))
    rejected(lambda r,n,c:n['flushCalls'][0].__setitem__('ok',False))
    rejected(lambda r,n,c:n['flushCalls'][0].__setitem__('thread',10))
    rejected(lambda r,n,c:c.__setitem__('elevated',True))
    rejected(lambda r,n,c:c.__setitem__('prefixFieldsVerified',44))
    rejected(lambda r,n,c:c['knownSchemaPrefixControls'][0].__setitem__('oldThread',0))
    rejected(lambda r,n,c:c['knownSchemaPrefixControls'].__setitem__(1,c['knownSchemaPrefixControls'][0]))
    # Missing readiness notifications must never invent a scheduling latency.
    r,n,c=fixture();r['targets'][0]['threadScheduling']=[];value=run(r,n,c)
    assert value['passed'] and all('afterFirstReadyNotificationMs' not in row
            for v in value['flushes'] for row in v['intervals']);checks+=1
    # The unsupported extension is deliberately not interpreted.
    r,n,c=fixture()
    for row in r['unknownSchemaInventory']['threadWindowRecords']:
        row['payloadHex']=row['payloadHex'][:48]+'ffffffff'
    assert run(r,n,c)['passed'];checks+=1
    def filtered_fixture():
        r,n,c=fixture();r['traceProfile']='disk-scheduling-target-threads-v3'
        r['targets'][0]['threadLifecycle']=[dict(opcode='start',counter=1,thread=9),
                                            dict(opcode='end',counter=90,thread=9)]
        inv=r['unknownSchemaInventory'];del inv['payloadsDecoded']
        inv.update(targetThreadFilter=True,selectedIdPrefixReadForFiltering=True,
                   payloadsFullyDecoded=False,threadWindowEvents=16,
                   retainedTargetThreadEvents=6,unmatchedTargetThreadEvents=10,
                   unsupportedTargetThreadForms=0)
        return r,n,c
    r,n,c=filtered_fixture();assert run(r,n,c)['passed'];checks+=1
    for key,value in [('targetThreadFilter',False),('unsupportedTargetThreadForms',1),
                      ('retainedTargetThreadEvents',5),('unmatchedTargetThreadEvents',9)]:
        r,n,c=filtered_fixture();r['unknownSchemaInventory'][key]=value
        assert not run(r,n,c)['passed'];checks+=1
    r,n,c=filtered_fixture();r['targets'][0]['threadLifecycle']=[]
    assert not run(r,n,c)['passed'];checks+=1
    r,n,c=filtered_fixture();r['targets'][0]['threadLifecycle'][1]['counter']=60
    assert not run(r,n,c)['passed'];checks+=1
    r,n,c=filtered_fixture()
    row=r['unknownSchemaInventory']['threadWindowRecords'][2]
    raw=bytearray.fromhex(row['payloadHex']);struct.pack_into('<II',raw,0,99,98)
    row['payloadHex']=raw.hex()
    assert not run(r,n,c)['passed'];checks+=1
    def filtered_terminal():
        r,n,c=filtered_fixture();inv=r['unknownSchemaInventory']
        inv['threadWindowRecords'].pop();inv['threadWindowEvents']-=1
        inv['retainedTargetThreadEvents']-=1
        # Last switch-in occurs after both measured calls.
        inv['threadWindowRecords'][-1]['counter']=71
        return r,n,c
    r,n,c=filtered_terminal();value=run(r,n,c)
    assert value['passed'] and value['terminalBoundary']=='tdh-thread-end-after-post-call-switch-in';checks+=1
    r,n,c=filtered_terminal();r['targets'][0]['threadLifecycle'].pop()
    assert not run(r,n,c)['passed'];checks+=1
    r,n,c=filtered_terminal();r['unknownSchemaInventory']['threadWindowRecords'][-1]['counter']=69
    assert not run(r,n,c)['passed'];checks+=1
    r,n,c=fixture();inv=r['unknownSchemaInventory'];inv['threadWindowRecords'].pop();inv['threadWindowEvents']-=1
    assert not run(r,n,c)['passed'];checks+=1
    print(json.dumps(dict(passed=True, policyChecks=checks, liveCoverageProven=False)))


if __name__ == '__main__':
    main()
