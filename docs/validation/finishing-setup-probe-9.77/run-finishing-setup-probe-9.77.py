from pathlib import Path
import sys,json,subprocess,time,uuid,importlib.util,ctypes,struct,collections,os
root=Path('D:/UDM');area=root/'.media-cache/owned-finishing-setup-probe-9.77'
sys.path.insert(0,str(root/'benchmarks/speed'))
spec=importlib.util.spec_from_file_location('completion',root/'benchmarks/speed/compare-completion.py')
m=importlib.util.module_from_spec(spec);spec.loader.exec_module(m)
build=json.loads((area/'build.json').read_text());assert build['passed'] and build['diagnosticOnly']
source=Path(build['binary']);assert m.sha(source)==build['sha256']
k=ctypes.WinDLL('kernel32',use_last_error=True)
k.QueryPerformanceCounter.argtypes=[ctypes.POINTER(ctypes.c_longlong)]
k.QueryPerformanceFrequency.argtypes=[ctypes.POINTER(ctypes.c_longlong)]
hz=ctypes.c_longlong();assert k.QueryPerformanceFrequency(ctypes.byref(hz));qpc=ctypes.c_longlong()
cases=[('sequential-4m',4*1024**2,1,0),('capped-32m',32*1024**2,8,2),('healthy-32m',32*1024**2,8,0)]
m.save(area/'run-inputs.json',{'driverSha256':m.sha(Path(__file__)),'originSha256':m.sha(root/'benchmarks/speed/origin.py'),'helperSha256':m.sha(root/'benchmarks/speed/compare-completion.py'),'diagnosticBinarySha256':m.sha(source),'componentOnly':True,'cases':cases,'ratePerConnection':2*1024**2})
runs=[]
for case,size,connections,cap in cases:
 folder=area/case;folder.mkdir(exist_ok=False)
 (folder/'.udm-live-file-info-fixture').write_text('Owned actual worker operation probe; no normal App or IDM timing')
 native=folder/'native';native.mkdir();exe=native/'Udm.WorkerProbe.exe';os.link(source,exe)
 destination=folder/'files';destination.mkdir();output=destination/'sample.bin';trace=folder/'ui-timing.bin'
 origin=m.Origin(m.Options(size=size,rate=2*1024**2,max_conns=cap,latency_ms=20,chunk=64*1024,seed=20261113,no_range=connections==1,accept_ranges='none' if connections==1 else None))
 m.save(folder/'config.json',{'url':origin.url,'connections':connections})
 before=time.perf_counter();assert k.QueryPerformanceCounter(ctypes.byref(qpc));after=time.perf_counter();offset=(before+after)/2-qpc.value/hz.value
 clock=m.anchor();start=time.perf_counter();log=(folder/'native.log').open('w')
 p=subprocess.Popen([str(exe),str(folder),str(folder/'config.json')],cwd=native,env={**os.environ,'UDM_TEST_UI_TRACE_FILE':str(trace)},stdout=log,stderr=subprocess.STDOUT)
 m.save(folder/'process.json',{'pid':p.pid,'componentOnly':True,'executableSha256':m.sha(exe)})
 pending=False
 while p.poll() is None:
  if not pending and time.perf_counter()-start>90:pending=True;print('Owned actor '+str(p.pid)+' remains live; observing without restart',flush=True)
  time.sleep(.01)
 log.close();m.save(folder/'exit.json',{'code':p.returncode});assert p.returncode==0
 completed=clock[0]+(m.exit_ticks(int(p._handle))-clock[1])/1e7;checkclock=m.anchor();assert abs((checkclock[0]-clock[0])-(checkclock[1]-clock[1])/1e7)<.005
 actor=m.load(folder/'actor-result.json');assert actor['passed'] and actor['componentOnly'];job=actor['job'];assert job['Status']=='Complete' and job['Received']==size
 assert output.is_file() and output.stat().st_size==size and m.available(output)
 durable=m.durable_output(output);digest=m.sha(output);assert digest==m.expected_sha256(size,20261113).upper()
 assert Path(str(output)+':Zone.Identifier').read_bytes()==b'[ZoneTransfer]\r\nZoneId=3\r\n'
 snapshot=origin.snapshot();last=origin.epoch+max(r['last_body_sent'] for c in snapshot['detail'] for r in c['requests'] if r.get('last_body_sent') is not None)
 raw=trace.read_bytes();magic,version,capacity,frequency,pid,count,dropped,eventsize,anchor,wall=struct.unpack_from('<8sIIqIiiIqq8x',raw)
 assert magic==b'UDMUIT1\0' and version==1 and capacity==2048 and frequency==hz.value and pid==p.pid and dropped==0 and 0<count<=capacity and eventsize==64 and len(raw)==64+capacity*64
 events=[]
 for i in range(count):
  ready,tid,tick,end,label=struct.unpack_from('<iIqq40s',raw,64+i*64);assert ready==1 and end>=tick>0
  events.append({'stage':label.split(b'\0',1)[0].decode(),'thread':tid,'counter':tick,'finished':end,'afterLastOriginByteMs':(tick/frequency+offset-last)*1000})
 events.sort(key=lambda e:e['counter']);spans=[];stacks=collections.defaultdict(list)
 for event in events:
  stage=event['stage']
  if stage.endswith('-begin'):stacks[(event['thread'],stage[:-6])].append(event)
  elif stage.endswith('-end') and stacks[(event['thread'],stage[:-4])]:
   begin=stacks[(event['thread'],stage[:-4])].pop();spans.append({'stage':stage[:-4],'thread':event['thread'],'durationMs':(event['counter']-begin['counter'])/frequency*1000,'startMs':begin['afterLastOriginByteMs'],'endMs':event['afterLastOriginByteMs']})
 for name in ['posthash-validation','stage-output','final-eligibility','advisory-setup','finish-snapshot','mark-zone','actor-stop']:assert sum(s['stage']==name for s in spans)==1,name
 if size>4*1024**2:
  for name in ['cache-transfer','async-submit','advisory-body']:assert sum(s['stage']==name for s in spans)==1,name
 assert sum(e['stage']=='actor-terminal' for e in events)==1 and not any(raw[64+count*64:])
 row={'case':case,'passed':True,'componentOnly':True,'normalProductSpeedComparison':False,'normalSourceVersion':'9.77','bytes':size,'sha256':digest,'normalExit':True,'bodyToActorExitMs':(completed-last)*1000,'actorLifetimeIncludesFixtureReceiptWrite':True,'explicitPostExitFlush':durable[1],'finishPhases':job.get('FinishPhases'),'publicationPhases':job.get('PublicationPhases'),'origin':snapshot,'trace':{'events':events,'spans':spans,'dropped':dropped,'qpcMappingBoundSeconds':(after-before)/2},'losslessTrace':{'rawSha256':m.sha(trace),'headerHex':raw[:64].hex(),'usedRecordsHex':raw[64:64+count*64].hex(),'zeroTailBytes':len(raw[64+count*64:])}}
 m.save(folder/'result.json',row);runs.append(row);m.save(area/'runs.json',{'passed':len(runs)==len(cases),'runs':runs});origin.close()
 for owned in [output,trace]:
  assert owned.resolve().is_relative_to(area.resolve()) and not owned.is_symlink()
  expected=digest if owned==output else row['losslessTrace']['rawSha256'];assert m.sha(owned)==expected;owned.unlink()
 m.save(folder/'cleanup.json',{'payloadRemoved':True,'traceExportedLosslesslyAndRemoved':True,'payloadBytes':size,'sha256':digest})
 print(json.dumps({'case':case,'spans':[s for s in spans if s['stage'] in ['posthash-validation','stage-output','final-eligibility','advisory-setup','cache-transfer','async-submit','finish-snapshot','mark-zone']]}),flush=True)
m.save(area/'terminal.json',{'passed':True,'cases':len(cases),'componentOnly':True,'normalProductionUnchanged':True,'payloadsRemoved':True,'rawTracesExportedLosslesslyAndRemoved':True,'idmComparison':False})
