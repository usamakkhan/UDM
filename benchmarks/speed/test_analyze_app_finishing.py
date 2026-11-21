"""Reject cross-process/clock joins, incomplete writes and unsupported attribution."""
import copy
import json
import struct
from analyze_app_finishing import native_phase, join_records, union_ticks
from test_cswitch_prefix import fixture as prefix_fixture
from analyze_cswitch_prefix import KEY, PROVIDER


def fixture():
    _,_,controls=prefix_fixture()
    path='C:\\owned\\body.data'
    common=dict(tid=9,handle='0x1',path=path,ok=True)
    records=[dict(common,api='WriteFile',id=1,begin=10,end=15),
             dict(common,api='NtWriteFile',id=2,begin=11,end=14,
                  pending=False,status=0,requested=8,completed=8,byteOffset='0'),
             dict(common,api='FlushFileBuffers',id=3,begin=20,end=41),
             dict(common,api='NtFlushBuffersFile',id=4,begin=21,end=40,pending=False,status=0)]
    snapshot=dict(frequency=1000,now=70,dropped=0,openFiles=0,openMappings=0,records=records)
    app_return=dict(passed=True,exitCode=0,appEntryCounter=5,appReturnedCounter=60)
    receipt=dict(passed=True,normalProcessExit=True,sourceAppSha256='A'*64,bytes=8,
                 sha256='B'*64,job=dict(Status='Complete',Sha256='B'*64),appReturn=app_return)
    raw=[]
    for time,new,old,state in [(6,9,0,2),(25,10,9,5),(35,9,10,1),(80,10,9,4)]:
        data=bytearray(28);struct.pack_into('<II',data,0,new,old);data[14]=state
        raw.append(dict(counter=time,schemaKey=KEY,headerProcess=0xffffffff,
                        headerThread=0xffffffff,payloadHex=data.hex()))
    schema=dict(provider=PROVIDER,eventId=0,opcode=36,version=5,headerFlags=848,
                payloadBytes=28,requeryStatus=1168,count=100)
    inventory=dict(capped=False,targetThreadFilter=True,selectedIdPrefixReadForFiltering=True,
                   payloadsFullyDecoded=False,schemas={KEY:schema},threadWindowRecords=raw,
                   retainedTargetThreadEvents=4,threadWindowEvents=14,
                   unmatchedTargetThreadEvents=10,unsupportedTargetThreadForms=0)
    disk=[dict(opcode='flushinit',counter=26,Irp=111,targetInitThread=9),
          dict(opcode='flushbuffers',counter=34,Irp=111,linkedToTargetInit=True,
               targetInitCounter=26,targetInitThread=9,targetInitOpcode='flushinit',DiskNumber=1)]
    target=dict(pid=123,image='u12345678a.exe',imageMatches=True,starts=1,ends=1,
                callbackFailed=False,ambiguous=False,limit=False,targetSchemaErrors=0,
                targetSchemaFailureInventory={},beginCounter=1,endCounter=100,
                pendingFlushes=0,flushPairs=[],pendingDiskOperations=0,
                unmatchedOwnedDiskCompletions=0,diskEvents=disk,
                threadLifecycle=[dict(opcode='start',counter=2,thread=9),
                                 dict(opcode='end',counter=90,thread=9)])
    kernel=dict(traceProfile='disk-scheduling-target-threads-v3',enabledFlags=0x10000D13,
                fileOperationAttributionSupported=False,started=True,reason='owner-complete',
                sessionAbsent=True,postStopQueryStatus=4201,eventsLost=0,buffersLost=0,
                stopStatus=0,processTraceStatus=0,callbackFailed=False,ambiguous=False,limit=False,
                normalWorkloadLaunchedByCollector=False,counterFrequency=1000,
                actualBuffers=32,bufferSizeKiB=64,closeTraceStatus=7007,targets=[target],
                unknownSchemaInventory=inventory,unknownMetadataEvents=100)
    identity=dict(pid=123,image='u12345678a.exe',frequency=1000)
    return kernel,snapshot,receipt,identity,controls


def main():
    checks=0
    def run(values):
        k,s,r,i,c=values
        return join_records(k,s,r,i,123,'u12345678a.exe','A'*64,c)
    baseline=fixture();r=run(baseline)
    assert r['passed'] and r['kernelJoined'] and not r['kernelCauseVerified']
    assert r['observedOffCpuMs']==10 and r['linkedDiskFlushIntervalUnionMs']==8
    assert r['finalNativeFlushMs']==19 and not r['fullSchemaQualificationPassed'];checks+=1
    def reject(mutator):
        nonlocal checks
        v=fixture();mutator(*v);assert not run(v)['passed'];checks+=1
    for key,value in [('frequency',2000),('dropped',1),('openFiles',1),('openMappings',1)]:
        reject(lambda k,s,r,i,c,a=key,b=value:s.__setitem__(a,b))
    for key,value in [('pid',124),('image','other.exe'),('frequency',2000)]:
        reject(lambda k,s,r,i,c,a=key,b=value:i.__setitem__(a,b))
    for key,value in [('counterFrequency',2000),('eventsLost',1),('buffersLost',1),
                      ('sessionAbsent',False),('reason','deadline'),('enabledFlags',0),
                      ('closeTraceStatus',5),('actualBuffers',257)]:
        reject(lambda k,s,r,i,c,a=key,b=value:k.__setitem__(a,b))
    for key,value in [('sourceAppSha256','C'*64),('normalProcessExit',False),('sha256','bad')]:
        reject(lambda k,s,r,i,c,a=key,b=value:r.__setitem__(a,b))
    reject(lambda k,s,r,i,c:r['job'].__setitem__('Status','Paused'))
    for key,value in [('byteOffset','1'),('pending',True),('completed',7),('status',5)]:
        reject(lambda k,s,r,i,c,a=key,b=value:s['records'][1].__setitem__(a,b))
    reject(lambda k,s,r,i,c:s['records'].__delitem__(0))
    reject(lambda k,s,r,i,c:s['records'][3].__setitem__('tid',10))
    reject(lambda k,s,r,i,c:r['appReturn'].__setitem__('appReturnedCounter',30))
    reject(lambda k,s,r,i,c:k['targets'][0].__setitem__('pid',124))
    reject(lambda k,s,r,i,c:k['targets'][0].__setitem__('endCounter',65))
    reject(lambda k,s,r,i,c:k['unknownSchemaInventory'].__setitem__('capped',True))
    reject(lambda k,s,r,i,c:k['targets'][0]['threadLifecycle'][1].__setitem__('counter',30))
    reject(lambda k,s,r,i,c:k['targets'][0]['diskEvents'][1].__setitem__('Irp',222))
    reject(lambda k,s,r,i,c:k['targets'][0]['diskEvents'][1].__setitem__('targetInitThread',10))
    reject(lambda k,s,r,i,c:k['targets'][0]['diskEvents'][1].__setitem__('counter',25))
    reject(lambda k,s,r,i,c:k['targets'][0].__setitem__('pendingDiskOperations',1))
    # Duplicate ends cannot manufacture disk-latency coverage.
    reject(lambda k,s,r,i,c:k['targets'][0]['diskEvents'].append(copy.deepcopy(k['targets'][0]['diskEvents'][1])))
    # Overlapping request intervals are unioned rather than double-counted.
    assert union_ticks([(1,5),(3,7),(7,9)])==8;checks+=1
    # Missing links do not prove there was no disk activity or establish a cause.
    v=fixture();v[0]['targets'][0]['diskEvents']=[];r=run(v)
    assert r['passed'] and r['linkedDiskFlushIntervalUnionMs']==0 and not r['kernelCauseVerified'];checks+=1
    # Two final native barriers need an explicit disambiguation, not a timing pick.
    def extra_flush(k,s,r,i,c):
        w=copy.deepcopy(s['records'][2]);w.update(id=5,begin=42,end=49)
        n=copy.deepcopy(s['records'][3]);n.update(id=6,begin=43,end=48)
        s['records'] += [w,n]
    reject(extra_flush)
    v=fixture();target=v[0]['targets'][0]
    target['diskEvents'] += [dict(opcode='writeinit',counter=23,Irp=222,targetInitThread=9),
        dict(opcode='write',counter=30,Irp=222,targetInitThread=9,targetInitCounter=23,
             targetInitOpcode='writeinit',linkedToTargetInit=True,TransferSize=4096,DiskNumber=1)]
    target['diskEvents'].sort(key=lambda e:e['counter']);value=run(v)
    assert value['passed'] and value['linkedDiskRequestIntervalUnionMs']==11
    assert value['linkedDiskRequestKinds']['write']==dict(requests=1,transferBytes=4096,intervalUnionMs=7);checks+=1
    target['diskEvents'][2]['TransferSize']=-1
    assert not run(v)['passed'];checks+=1
    v=fixture();inv=v[0]['unknownSchemaInventory'];del inv['threadWindowRecords'][2]
    inv['retainedTargetThreadEvents']-=1;inv['threadWindowEvents']-=1;value=run(v)
    assert not value['passed'] and not value['kernelJoined'] and value['diskCorrelationsValidated']
    assert value['linkedDiskFlushIntervalUnionMs']==8;checks+=1
    # A record retained for another target thread must not extend the native
    # thread's lifetime merely because its old-thread field has the same ID.
    v=fixture();target=v[0]['targets'][0]
    target['threadLifecycle']=[dict(opcode='start',counter=2,thread=9),
        dict(opcode='start',counter=3,thread=11),dict(opcode='end',counter=90,thread=9),
        dict(opcode='end',counter=99,thread=11)]
    inv=v[0]['unknownSchemaInventory'];data=bytearray(28);struct.pack_into('<II',data,0,11,9)
    inv['threadWindowRecords'].append(dict(counter=95,schemaKey=KEY,headerProcess=0xffffffff,
        headerThread=0xffffffff,payloadHex=data.hex()))
    inv['retainedTargetThreadEvents']+=1;inv['threadWindowEvents']+=1
    assert run(v)['passed'];checks+=1
    print(json.dumps(dict(passed=True,policyChecks=checks,actualCaptureStarted=False,
                         kernelCauseVerified=False)))


if __name__=='__main__':main()
