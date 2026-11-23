from pathlib import Path
import os,sys,json,subprocess,time,uuid,ctypes,struct,hashlib,importlib.util,collections
root=Path('<repo>');area=root/'.media-cache/owned-storage-timeline-9.74';b=json.loads((area/'build.json').read_text());assert b['passed'] and b['diagnosticOnly'] and b.get('appEntryRecompiledWithFinishTimingOnly');sys.path.insert(0,str(root/'benchmarks/speed'));from origin import Origin,Options,expected_sha256
spec=importlib.util.spec_from_file_location('completion',root/'benchmarks/speed/compare-completion.py');h=importlib.util.module_from_spec(spec);spec.loader.exec_module(h)
def sha(p):return hashlib.sha256(Path(p).read_bytes()).hexdigest().upper()
def save(p,v):p.write_text(json.dumps(v,indent=2)+'\n',encoding='utf-8')
normal=json.loads((root/'.media-cache/owned-advisory-intent-build-9.74/build-result.json').read_text());assert all(sha(root/r['path'])==r['sha256'] for r in normal['sources']);assert sha(area/'source/App.cpp')==next(r['sha256'] for r in normal['sources'] if r['path']=='native/App.cpp');assert all(sha(r['path'])==r['sha256'] for r in b['privateSources']);save(area/'run-inputs.json',dict(driverSha256=sha(Path(__file__)),originSha256=sha(root/'benchmarks/speed/origin.py'),comparisonHelperSha256=sha(root/'benchmarks/speed/compare-completion.py'),binarySha256=b['sha256'],caseCount=3,rateBytesPerSecond=8000000,latencyMs=20,diagnosticOnly=True))
source=Path(b['binary']);assert sha(source)==b['sha256'];k=ctypes.WinDLL('kernel32',use_last_error=True);k.QueryPerformanceCounter.argtypes=[ctypes.POINTER(ctypes.c_longlong)];k.QueryPerformanceFrequency.argtypes=[ctypes.POINTER(ctypes.c_longlong)];hz=ctypes.c_longlong();assert k.QueryPerformanceFrequency(ctypes.byref(hz));qpc=ctypes.c_longlong();runs=json.loads((area/'runs.json').read_text())['runs'] if (area/'runs.json').exists() else []
for case,size,connections in [('single-4m',4*1048576,1),('single-64m',64*1048576,1),('split-64m',64*1048576,8)]:
 folder=area/('owned-'+case)
 if (folder/'result.json').is_file():continue
 folder.mkdir(exist_ok=False);(folder/'.udm-live-file-info-fixture').write_text('Owned source-operation diagnostic; not a normal-product timing run');native=folder/'native';native.mkdir();exe=native/'UDM.exe';os.link(source,exe);assert sha(exe)==b['sha256'];data=folder/'catalog';data.mkdir();files=folder/'files';files.mkdir();output=files/'sample.bin';trace=folder/'ui-timing.bin'
 save(data/'state.json',{'Schema':1,'Settings':{'Connections':connections,'Parallel':1,'DownloadFolder':str(files),'CategoryFolders':False,'SuppressProgressDialog':True,'SuppressCompletionDialog':True,'Sound':False,'ShowTipsOnStartup':False,'ClipboardMonitor':False,'PrefetchFileInfo':False,'StopOnWebPage':False,'DuplicatePolicy':'Add'},'Downloads':[],'Queues':[]})
 origin=Origin(Options(size=size,rate=8_000_000,latency_ms=20,no_range=connections==1,accept_ranges='none' if connections==1 else None,seed=20261113));env={**os.environ,'UDM_TEST_UI_TRACE_FILE':str(trace)};args=[str(exe),'--background','--instance-tag','storage-'+uuid.uuid4().hex[:12],'--data-dir',str(data),'/d',origin.url+'?trace='+uuid.uuid4().hex,'/p',str(files),'/f',output.name,'/n','/q']
 before=time.perf_counter();k.QueryPerformanceCounter(ctypes.byref(qpc));after=time.perf_counter();offset=(before+after)/2-qpc.value/hz.value;clock=h.anchor();started=time.perf_counter();save(folder/'launch.json',dict(args=args,binarySha256=b['sha256']));p=subprocess.Popen(args,cwd=native,env=env);save(area/'active-process.json',dict(pid=p.pid,case=case,binary=str(exe),sourceSha256=b['sha256'],folder=str(folder),diagnosticOnly=True));available=None
 try:
  while p.poll() is None:
   if available is None and output.exists() and output.stat().st_size==size and h.available(output):available=time.perf_counter()
   if time.perf_counter()-started>90:raise TimeoutError('Owned target remains live; inspect this exact process handle before any retry; no process was terminated')
   time.sleep(.002)
  p.wait();assert p.returncode==0;completed=clock[0]+(h.exit_ticks(int(p._handle))-clock[1])/1e7;lastClock=h.anchor();drift=(lastClock[0]-clock[0])-(lastClock[1]-clock[1])/1e7;assert abs(drift)<.005;snapshot=origin.snapshot();last=origin.epoch+max(r['last_body_sent'] for c in snapshot['detail'] for r in c['requests'] if r.get('last_body_sent') is not None);assert completed>=last
  expected=expected_sha256(size,20261113).upper();assert output.stat().st_size==size and h.available(output);durable=h.durable_output(output);assert sha(output)==expected;catalog=json.loads((data/'state.json').read_text(encoding='utf-8-sig'));assert len(catalog['Downloads'])==1;row=catalog['Downloads'][0];assert row['Status']=='Complete' and row['Sha256'].upper()==expected
  save(folder/'origin.json',snapshot);save(folder/'transfer-receipt.json',dict(pid=p.pid,case=case,bytes=size,expectedSha256=expected,sourceBinarySha256=b['sha256'],qpcOffset=offset,lastOriginPerf=last,exitPerf=completed,durablePerf=durable[0],startedPerf=started,exclusiveAvailabilityPerf=available,qpcMappingBoundSeconds=(after-before)/2,clockDriftSeconds=drift))
  assert output.parent==files and files.parent==folder and output.stat().st_nlink==1 and not output.is_symlink() and sha(output)==expected;output.unlink();save(folder/'payload-removed.json',dict(passed=not output.exists(),bytes=size,sha256=expected))
  raw=trace.read_bytes();magic,version,capacity,frequency,pid,count,dropped,eventSize,anchor,wall=struct.unpack_from('<8sIIqIiiIqq8x',raw);assert magic==b'UDMUIT1\0' and version==1 and capacity==8192 and frequency==hz.value and pid==p.pid and dropped==0 and 0<count<=capacity and eventSize==64 and len(raw)==64+capacity*64;events=[]
  for i in range(count):
   ready,tid,tick,end,label=struct.unpack_from('<iIqq40s',raw,64+64*i);assert ready==1 and tid>0 and end>=tick>0;events.append(dict(stage=label.split(b'\0',1)[0].decode(),thread=tid,counter=tick,finished=end,afterLastOriginByteMs=(tick/frequency+offset-last)*1000))
  events.sort(key=lambda e:e['counter']);assert not any(raw[64+count*64:]);spans=[];stacks=collections.defaultdict(list)
  for e in events:
   stage=e['stage'];begin=None
   if stage.endswith('-begin'):stacks[(e['thread'],stage[:-6])].append(e)
   elif stage.endswith('-end'):
    key=(e['thread'],stage[:-4]);
    if stacks[key]:begin=stacks[key].pop()
    else:assert stage in {'automation-reset-end','manager-reset-end','ole-term-end','file-info-stop-end','command-files-stop-end','windows-cleanup-end','catalog-resolve-end','publication-cleanup-end'},('Missing begin',stage,e['thread'])
   elif stage=='stop-entry':stacks[(e['thread'],'manager-stop-native')].append(e)
   elif stage=='stop-return':begin=stacks[(e['thread'],'manager-stop-native')].pop()
   if begin:
    start=begin['counter'];end=e['counter'];assert end>=start;spans.append(dict(stage=begin['stage'].replace('-begin','').replace('stop-entry','manager-stop-native'),thread=e['thread'],beginCounter=start,endCounter=end,durationMs=(end-start)/frequency*1000,startMs=begin['afterLastOriginByteMs'],endMs=e['afterLastOriginByteMs']))
  assert not any(stacks.values()),{str(key):len(v) for key,v in stacks.items() if v}
  body=[s for s in spans if s['stage']=='body-write'];assert body;lastWrite=max(s['endCounter'] for s in body);bodyFlush=[s for s in spans if s['stage']=='body-flush'];assert len(bodyFlush)==1;intent=[s for s in spans if s['stage']=='intent-write'];cat=[s for s in spans if s['stage']=='catalog-write'];assert len(intent)==len(cat)==1;assert bodyFlush[0]['endCounter']<=cat[0]['beginCounter'];assert sum(e['stage']=='exit-instance-end' for e in events)==1
  for s in spans:s['afterLastBodyWriteMs']=(s['beginCounter']-lastWrite)/frequency*1000
  overlap=max(0,min(bodyFlush[0]['endCounter'],intent[0]['endCounter'])-max(bodyFlush[0]['beginCounter'],intent[0]['beginCounter']))/frequency*1000
  unused=raw[64+count*64:];save(folder/'trace-lossless.json',dict(rawSha256=sha(trace),headerHex=raw[:64].hex(),usedRecordsHex=raw[64:64+count*64].hex(),zeroTailBytes=len(unused)));save(folder/'origin.json',snapshot)
  result=dict(passed=True,case=case,diagnosticOnly=True,normalProductSpeedComparison=False,sourceVersion='9.74',binarySha256=b['sha256'],bytes=size,sha256=expected,hashMatches=True,nativeEvents=count,dropped=dropped,qpcMappingBoundSeconds=(after-before)/2,clockDriftSeconds=drift,exitTailMs=(completed-last)*1000,totalExitMs=(completed-started)*1000,durableTailMs=(durable[0]-last)*1000,exclusiveAvailabilityTailMs=(available-last)*1000 if available else None,lastBodyWriteToExitMs=(completed-(lastWrite/frequency+offset))*1000,bodyIntentOverlapMs=overlap,events=events,spans=spans,finishPhases=row.get('FinishPhases'),publicationPhases=row.get('PublicationPhases'));save(folder/'result.json',result);runs.append(result);save(area/'runs.json',dict(passed=True,diagnosticOnly=True,runs=runs))
  print(json.dumps(dict(case=case,events=count,exitTailMs=round(result['exitTailMs'],3),bodyIntentOverlapMs=round(overlap,3),keySpans={label:[round(s['durationMs'],3) for s in spans if s['stage']==label] for label in ['body-flush','intent-write','catalog-write','body-join','catalog-read','manager-stop-native','integration-stop','exit-instance']},payloadRemoved=True)),flush=True)
 finally:origin.close()
save(area/'terminal.json',dict(passed=True,diagnosticOnly=True,transfers=len(runs),allFinalFileHashesVerified=True,allPayloadsRemoved=True,normalSourceVersionUnchanged='9.74',noIdmComparison=True,noKernelQueueOrFilterAttribution=True));print('Three diagnostic operation timelines complete; normal source unchanged',flush=True)




