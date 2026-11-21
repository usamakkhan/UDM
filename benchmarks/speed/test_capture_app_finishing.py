"""Fail-closed admission and process-aware wait controls; no real processes/IO traces."""
import copy
import json
from types import SimpleNamespace
import capture_app_finishing as runner


def evidence():
    calls=[dict(thread=9,beginCounter=a,endCounter=a+20) for a in (10,50)]
    disk=[dict(opcode='flushbuffers',linkedToTargetInit=True,targetInitOpcode='flushinit',
               targetInitThread=9,targetInitCounter=a+1,counter=a+19) for a in (10,50)]
    report=dict(traceProfile='disk-scheduling-target-threads-v3',started=True,
                eventsLost=0,buffersLost=0,limit=False,callbackFailed=False,
                sessionAbsent=True,stopStatus=0,
                targets=[dict(pid=123,pendingDiskOperations=0,ambiguous=False,diskEvents=disk)])
    native=dict(passed=True,elevated=False,bytes=8192,pid=123,flushCalls=calls)
    prefix=dict(passed=True,targetThreadFilter=True)
    reply=dict(accepted=True,collectorSha256='A'*64)
    return report,native,prefix,reply


def main():
    checks=0
    def accepts(values):
        try: runner.qualification(*values,'A'*64); return True
        except (ValueError,KeyError,TypeError): return False
    assert accepts(evidence());checks+=1
    for field,value in [('traceProfile','old'),('eventsLost',1),('buffersLost',1),
                        ('limit',True),('callbackFailed',True),('sessionAbsent',False)]:
        v=evidence();v[0][field]=value;assert not accepts(v);checks+=1
    for field,value in [('passed',False),('elevated',True),('bytes',0)]:
        v=evidence();v[1][field]=value;assert not accepts(v);checks+=1
    v=evidence();v[2]['passed']=False;assert not accepts(v);checks+=1
    v=evidence();v[3]['collectorSha256']='B'*64;assert not accepts(v);checks+=1
    v=evidence();v[0]['targets'][0]['pendingDiskOperations']=1;assert not accepts(v);checks+=1
    for field,value in [('linkedToTargetInit',False),('targetInitThread',10),
                        ('targetInitCounter',9),('counter',31)]:
        v=evidence();v[0]['targets'][0]['diskEvents'][0][field]=value
        assert not accepts(v);checks+=1

    class Path:
        name='mock-record'
        def __init__(self,exist,read): self.exist=exist;self.read=read
        def exists(self): return self.exist()
    class Kernel:
        def __init__(self,state=258,handle=1):self.state=state;self.handle=handle;self.closed=0
        def OpenProcess(self,*args):return self.handle
        def WaitForSingleObject(self,*args):return self.state
        def CloseHandle(self,*args):self.closed+=1
    original_load,original_sleep,original_error=runner.load,runner.time.sleep,runner.ctypes.get_last_error
    runner.load=lambda path:path.read()
    runner.time.sleep=lambda seconds:None
    try:
        path=Path(lambda:True,lambda:{'done':True});kernel=Kernel()
        assert runner.wait_record(path,utils=SimpleNamespace(k=kernel),process_id=123)=={'done':True}
        assert kernel.closed==0;checks+=1
        path=Path(lambda:False,lambda:None);terminal=Path(lambda:True,lambda:{'ended':True})
        try: runner.wait_record(path,utils=SimpleNamespace(k=Kernel()),process_id=123,terminal_paths=(terminal,));raise AssertionError('Terminal job ignored')
        except RuntimeError:checks+=1
        kernel=Kernel(state=0)
        try:runner.wait_record(path,utils=SimpleNamespace(k=kernel),process_id=123);raise AssertionError('Exited process ignored')
        except RuntimeError:assert kernel.closed==1;checks+=1
        attempts=[0]
        def partial():
            attempts[0]+=1
            if attempts[0]==1:raise json.JSONDecodeError('in progress','',0)
            return {'complete':True}
        kernel=Kernel();path=Path(lambda:True,partial)
        assert runner.wait_record(path,utils=SimpleNamespace(k=kernel),process_id=123)=={'complete':True}
        assert kernel.closed==1;checks+=1
        runner.ctypes.get_last_error=lambda:87;kernel=Kernel(handle=None)
        try:runner.wait_record(Path(lambda:False,lambda:None),utils=SimpleNamespace(k=kernel),process_id=123);raise AssertionError('Missing process ignored')
        except RuntimeError:checks+=1
        queries=[False,True];kernel=Kernel(state=0)
        path=Path(lambda:queries.pop(0),lambda:{'final':True})
        assert runner.wait_record(path,utils=SimpleNamespace(k=kernel),process_id=123)=={'final':True}
        assert kernel.closed==1;checks+=1
    finally:
        runner.load,runner.time.sleep,runner.ctypes.get_last_error=original_load,original_sleep,original_error
    print(json.dumps(dict(passed=True,policyChecks=checks,actualProcessLaunches=0,
                         newElevationRequested=False,liveCaptureQualified=False)))


if __name__=='__main__':main()
