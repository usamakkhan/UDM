from pathlib import Path
import sys,json,subprocess,time,uuid,importlib.util,ctypes,struct,collections,os
root=Path("D:/UDM");area=root/".media-cache/owned-zone-call-timeline-9.77";sys.path.insert(0,str(root/"benchmarks/speed"))
spec=importlib.util.spec_from_file_location("completion",root/"benchmarks/speed/compare-completion.py");m=importlib.util.module_from_spec(spec);spec.loader.exec_module(m)
b=json.loads((area/"build.json").read_text());assert b["passed"] and b["diagnosticOnly"];source=Path(b["binary"]);assert m.sha(source)==b["sha256"]
k=ctypes.WinDLL("kernel32",use_last_error=True);k.QueryPerformanceCounter.argtypes=[ctypes.POINTER(ctypes.c_longlong)];k.QueryPerformanceFrequency.argtypes=[ctypes.POINTER(ctypes.c_longlong)];hz=ctypes.c_longlong();assert k.QueryPerformanceFrequency(ctypes.byref(hz));qpc=ctypes.c_longlong();runs=[]
for case,size,connections,date in [("capped-32m",32*1024**2,8,False),("sequential-date-4m",4*1024**2,1,True)]:
 folder=area/case;folder.mkdir(exist_ok=False);(folder/".udm-live-file-info-fixture").write_text("Owned diagnostic operation observation, not speed comparison");native=folder/"native";native.mkdir();exe=native/"UDM.exe";os.link(source,exe)
 data=folder/"catalog";data.mkdir();dest=folder/"files";dest.mkdir();output=dest/"sample.bin";trace=folder/"ui-timing.bin"
 m.save(data/"state.json",{"Schema":1,"Settings":{"Connections":connections,"Parallel":1,"ProxyMode":"Connect directly","UseServerDate":date,"DownloadFolder":str(dest),"CategoryFolders":False,"SuppressProgressDialog":True,"SuppressCompletionDialog":True,"Sound":False,"ShowTipsOnStartup":False,"ClipboardMonitor":False,"PrefetchFileInfo":False,"StopOnWebPage":False,"DuplicatePolicy":"Add","Retries":3},"Downloads":[],"Queues":[]})
 origin=m.Origin(m.Options(size=size,rate=2*1024**2,max_conns=2,latency_ms=20,chunk=64*1024,seed=20261113,no_range=connections==1,accept_ranges="none" if connections==1 else None));clock=m.anchor();before=time.perf_counter();assert k.QueryPerformanceCounter(ctypes.byref(qpc));after=time.perf_counter();offset=(before+after)/2-qpc.value/hz.value;start=time.perf_counter()
 command=[str(exe),"--background","--instance-tag","zone-"+uuid.uuid4().hex[:16],"--data-dir",str(data),"/d",origin.url,"/p",str(dest),"/f",output.name,"/n","/q"]
 env={**os.environ,"UDM_TEST_UI_TRACE_FILE":str(trace)};log=(folder/"native.log").open("w");p=subprocess.Popen(command,cwd=native,env=env,stdout=log,stderr=subprocess.STDOUT);m.save(folder/"process.json",{"pid":p.pid,"executableSha256":m.sha(exe)})
 first=None;pending=False
 while p.poll() is None:
  if first is None and output.exists() and output.stat().st_size==size and m.available(output):first=time.perf_counter()
  if not pending and time.perf_counter()-start>90:pending=True;print("Still-live owned diagnostic process "+str(p.pid)+"; continuing observation without restart",flush=True)
  time.sleep(.01)
 log.close();m.save(folder/"exit.json",{"exitCode":p.returncode});endticks=m.exit_ticks(int(p._handle));completed=clock[0]+(endticks-clock[1])/1e7;afterclock=m.anchor();assert abs((afterclock[0]-clock[0])-(afterclock[1]-clock[1])/1e7)<.005
 assert p.returncode==0 and output.is_file() and output.stat().st_size==size and m.available(output)
 snap=origin.snapshot();last=max(r["last_body_sent"] for c in snap["detail"] for r in c["requests"] if r.get("last_body_sent") is not None);lastperf=origin.epoch+last
 durable=m.durable_output(output);digest=m.sha(output);assert digest==m.expected_sha256(size,20261113).upper()
 state=m.load(data/"state.json");job=state["Downloads"][0];assert job["Status"]=="Complete" and job["Received"]==size
 zone=Path(str(output)+":Zone.Identifier").read_bytes();assert zone==b"[ZoneTransfer]\r\nZoneId=3\r\n"
 if date:
  import email.utils
  assert abs(output.stat().st_mtime-email.utils.parsedate_to_datetime("Sat, 18 Oct 2026 00:00:00 GMT").timestamp())<.001
 raw=trace.read_bytes();magic,version,capacity,frequency,pid,count,dropped,eventsize,anchor,wall=struct.unpack_from("<8sIIqIiiIqq8x",raw);assert magic==b"UDMUIT1\0" and version==1 and capacity==2048 and frequency==hz.value and pid==p.pid and dropped==0 and 0<count<=capacity and eventsize==64 and len(raw)==64+capacity*64
 events=[]
 for i in range(count):
  ready,tid,tick,end,label=struct.unpack_from("<iIqq40s",raw,64+i*64);assert ready==1 and end>=tick>0
  events.append({"stage":label.split(b"\0",1)[0].decode(),"thread":tid,"counter":tick,"finished":end,"afterLastOriginByteMs":(tick/frequency+offset-lastperf)*1000})
 events.sort(key=lambda e:e["counter"]);spans=[];stacks=collections.defaultdict(list)
 for event in events:
  stage=event["stage"]
  if stage.endswith("-begin"):stacks[(event["thread"],stage[:-6])].append(event)
  elif stage.endswith("-end") and stacks[(event["thread"],stage[:-4])]:
   begin=stacks[(event["thread"],stage[:-4])].pop();spans.append({"stage":stage[:-4],"thread":event["thread"],"durationMs":(event["counter"]-begin["counter"])/frequency*1000,"startMs":begin["afterLastOriginByteMs"],"endMs":event["afterLastOriginByteMs"]})
 for name in ["zone","zone-open","zone-write","zone-close","metadata-block","finish-snapshot"]:assert sum(s["stage"]==name for s in spans)==1,name
 assert sum(e["stage"]=="exit-instance-end" for e in events)==1
 row={"case":case,"configuredConnections":connections,"serverDateEnabled":date,"sizeBytes":size,"passed":True,"diagnosticOnly":True,"exactZoneMarkerVerified":True,"serverDateVerified":date,"normalProductSpeedComparison":False,"normalExit":True,"exitCode":p.returncode,"sha256":digest,"executableSha256":m.sha(exe),"totalToExitSeconds":completed-start,"lastBodyToExitSeconds":completed-lastperf,"postExitToExplicitFlushSeconds":durable[0]-completed,"postExitOperations":durable[1],"job":{"status":job["Status"],"received":job["Received"],"segments":len(job.get("Segments",[])),"dynamicSplits":job.get("DynamicSplits"),"finishPhases":job.get("FinishPhases"),"publicationPhases":job.get("PublicationPhases")},"origin":snap,"trace":{"events":events,"spans":spans,"dropped":dropped,"qpcMappingBoundSeconds":(after-before)/2},"losslessTrace":{"rawSha256":m.sha(trace),"headerHex":raw[:64].hex(),"usedRecordsHex":raw[64:64+count*64].hex(),"zeroTailBytes":len(raw[64+count*64:])}}
 assert not any(raw[64+count*64:]);m.save(folder/"result.json",row);runs.append(row);m.save(area/"runs.json",{"passed":len(runs)==2,"runs":runs});origin.close()
 for owned in [output,trace]:
  assert owned.resolve().is_relative_to(area.resolve()) and not owned.is_symlink()
  expected=digest if owned==output else row["losslessTrace"]["rawSha256"];assert m.sha(owned)==expected;owned.unlink()
 m.save(folder/"cleanup.json",{"payloadRemoved":True,"traceExportedLosslesslyAndRemoved":True,"logicalPayloadBytes":size,"sha256":digest})
 print(json.dumps({"case":case,"exitTailMs":row["lastBodyToExitSeconds"]*1000,"spans":[s for s in spans if s["stage"].startswith(("zone","metadata","finish-snapshot"))]}),flush=True)
m.save(area/"terminal.json",{"passed":True,"cases":2,"payloadsRemoved":True,"rawTracesExportedLosslesslyAndRemoved":True,"diagnosticOnly":True,"normalVersionUnchanged":"9.77","idmComparison":False})
