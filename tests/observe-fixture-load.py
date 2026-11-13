"""Read load counters around an owned HLS fixture; never operates a personal app."""
from pathlib import Path
import ctypes as c
from ctypes import wintypes as w
import hashlib,json,os,shutil,subprocess,sys,time

k=c.WinDLL('kernel32',use_last_error=True)
class Entry(c.Structure):
 _fields_=[('size',w.DWORD),('usage',w.DWORD),('pid',w.DWORD),('heap',c.c_size_t),('module',w.DWORD),('threads',w.DWORD),('parent',w.DWORD),('priority',w.LONG),('flags',w.DWORD),('name',w.WCHAR*260)]
class Memory(c.Structure):
 _fields_=[('size',w.DWORD),('load',w.DWORD)]+[(name,c.c_ulonglong) for name in ['totalPhys','availPhys','totalPageFile','availPageFile','totalVirtual','availVirtual','availExtendedVirtual']]
def bind(name,args,result):
 f=getattr(k,name);f.argtypes=args;f.restype=result;return f
open_process=bind('OpenProcess',[w.DWORD,w.BOOL,w.DWORD],w.HANDLE)
close=bind('CloseHandle',[w.HANDLE],w.BOOL)
snapshot=bind('CreateToolhelp32Snapshot',[w.DWORD,w.DWORD],w.HANDLE)
first=bind('Process32FirstW',[w.HANDLE,c.POINTER(Entry)],w.BOOL)
next_entry=bind('Process32NextW',[w.HANDLE,c.POINTER(Entry)],w.BOOL)
image=bind('QueryFullProcessImageNameW',[w.HANDLE,w.DWORD,w.LPWSTR,c.POINTER(w.DWORD)],w.BOOL)
system_times=bind('GetSystemTimes',[c.POINTER(w.FILETIME)]*3,w.BOOL)
process_times=bind('GetProcessTimes',[w.HANDLE]+[c.POINTER(w.FILETIME)]*4,w.BOOL)
cycles=bind('QueryProcessCycleTime',[w.HANDLE,c.POINTER(c.c_ulonglong)],w.BOOL)
memory=bind('GlobalMemoryStatusEx',[c.POINTER(Memory)],w.BOOL)
counter=bind('QueryPerformanceCounter',[c.POINTER(c.c_longlong)],w.BOOL)
frequency=bind('QueryPerformanceFrequency',[c.POINTER(c.c_longlong)],w.BOOL)
processors=bind('GetActiveProcessorCount',[w.WORD],w.DWORD)
groups=bind('GetActiveProcessorGroupCount',[],w.WORD)
wait=bind('WaitForSingleObject',[w.HANDLE,w.DWORD],w.DWORD)
freq=c.c_longlong();assert frequency(c.byref(freq))
def require(ok):
 if not ok:raise c.WinError(c.get_last_error())
def ticks(value):return (value.dwHighDateTime<<32)|value.dwLowDateTime
def qpc():
 value=c.c_longlong();require(counter(c.byref(value)));return value.value*1000/freq.value
def child_pid(parent,name):
 handle=snapshot(2,0)
 if handle==c.c_void_p(-1).value:raise c.WinError(c.get_last_error())
 try:
  entry=Entry();entry.size=c.sizeof(entry);valid=first(handle,c.byref(entry));found=[]
  while valid:
   if entry.parent==parent and entry.name.casefold()==name.casefold():found.append(entry.pid)
   valid=next_entry(handle,c.byref(entry))
  if len(found)>1:raise RuntimeError('More than one owned fixture child')
  return found[0] if found else None
 finally:close(handle)
def full_image(handle):
 size=w.DWORD(32768);text=c.create_unicode_buffer(size.value);require(image(handle,0,text,c.byref(size)));return Path(text.value).resolve()
def sample(handle):
 start=qpc();idle,kernel,user=w.FILETIME(),w.FILETIME(),w.FILETIME();require(system_times(c.byref(idle),c.byref(kernel),c.byref(user)))
 created,exited,pk,pu=w.FILETIME(),w.FILETIME(),w.FILETIME(),w.FILETIME();require(process_times(handle,c.byref(created),c.byref(exited),c.byref(pk),c.byref(pu)))
 mem=Memory();mem.size=c.sizeof(mem);require(memory(c.byref(mem)))
 value=c.c_ulonglong();cycle_ok=cycles(handle,c.byref(value))
 result={'epochMs':time.time_ns()/1e6,'qpcMs':start,'systemIdleTicks':ticks(idle),'systemKernelTicks':ticks(kernel),'systemUserTicks':ticks(user),'processCpuTicks':ticks(pk)+ticks(pu),'processCreationTicks':ticks(created),'physicalMemoryLoadPct':mem.load,'availablePhysicalBytes':mem.availPhys,'samplerAvailableCommitBytes':mem.availPageFile}
 if cycle_ok:result['processCycles']=value.value
 result['readWallMs']=qpc()-start
 return result
def self_test():
 handle=open_process(0x101000,False,os.getpid());require(handle)
 try:
  get_pid=bind('GetProcessId',[w.HANDLE],w.DWORD)
  assert get_pid(handle)==os.getpid() and full_image(handle).is_file()
  a=sample(handle);time.sleep(.03);b=sample(handle)
  assert b['qpcMs']>a['qpcMs'] and b['processCpuTicks']>=a['processCpuTicks'] and b['systemKernelTicks']>=a['systemKernelTicks']
  assert 0<=b['physicalMemoryLoadPct']<=100 and b['availablePhysicalBytes']>0
  print(json.dumps({'passed':True,'entryBytes':c.sizeof(Entry),'memoryBytes':c.sizeof(Memory),'logicalProcessors':processors(0xffff),'processorGroups':groups(),'sampleReadMs':[a['readWallMs'],b['readWallMs']]}))
 finally:close(handle)
def run(folder,enabled):
 project=Path(__file__).resolve().parent.parent;folder=folder.resolve()
 assert folder.is_relative_to(project/'candidates') and not (folder/'results.json').exists(),'Use a fresh owned candidate folder'
 expected=Path(os.environ['UDM_FILE_INFO_TEST_EXE']).resolve();assert expected.name=='Udm.CapturePresentationUiTests.exe'
 node=shutil.which('node');assert node
 folder.mkdir(parents=True,exist_ok=True)
 samples=[];handle=None;pid=None;error=None;sampler_cpu_start=time.process_time();start=time.time_ns()/1e6
 with (folder/'runner.log').open('w',encoding='utf-8') as log:
  process=subprocess.Popen([node,str(project/'tests/hls-catalog.native-live.cjs'),str(folder)],cwd=project,stdout=log,stderr=subprocess.STDOUT,creationflags=0x08000000)
  try:
   while process.poll() is None:
    if enabled and not handle:
     pid=child_pid(process.pid,expected.name)
     if pid:
      assert (folder/'.udm-live-file-info-fixture').is_file(),'Not an owned File Info fixture'
      handle=open_process(0x101000,False,pid);require(handle)
      assert full_image(handle)==expected,'Unexpected executable for fixture child'
    if handle and wait(handle,0)==258:samples.append(sample(handle))
    time.sleep(.025 if handle else .1)
  except Exception as caught:error=str(caught)
  finally:
   if handle:close(handle)
   # Keep the original handle until terminal; do not restart the test on a timeout.
   result=process.wait()
 record={'enabled':enabled,'testExitCode':result,'error':error,'ownedChildObserved':bool(pid),'logicalProcessors':processors(0xffff),'processorGroups':groups(),'samplerCpuMs':(time.process_time()-sampler_cpu_start)*1000,'startEpochMs':start,'endEpochMs':time.time_ns()/1e6,'samples':samples,'sourceSha256':hashlib.sha256(Path(__file__).read_bytes()).hexdigest(),'limitations':['Process counters include all owned fixture threads, not just the UI thread.','GetSystemTimes is aggregate across the caller processor group when multiple groups exist.','Available commit here belongs to the sampler API context, not an exact system-wide commit measurement.','Sampling is observational and can perturb scheduling. CPU and memory correlations do not identify a wait owner or prove causation.','Only the exact child executable is queried; other process identities and command lines are not retained.']}
 (folder/'load-observation.json').write_text(json.dumps(record,indent=2)+'\n',encoding='utf-8')
 assert result==0 and error is None and (not enabled or len(samples)>2),record.get('error') or 'Fixture/load observation failed'
 print(json.dumps({'case':folder.name,'observed':enabled,'samples':len(samples),'samplerCpuMs':record['samplerCpuMs'],'passed':True}),flush=True)
if __name__=='__main__':
 if sys.argv[1:]==['--self-test']:self_test()
 else:
  assert len(sys.argv)==3 and sys.argv[2] in ['on','off']
  run(Path(sys.argv[1]),sys.argv[2]=='on')
