from pathlib import Path
import sys,json,subprocess,time,uuid,importlib.util
root=Path("D:/UDM");sys.path.insert(0,str(root/"benchmarks/speed"))
spec=importlib.util.spec_from_file_location("completion",root/"benchmarks/speed/compare-completion.py");m=importlib.util.module_from_spec(spec);spec.loader.exec_module(m)
work=root/".media-cache/owned-capped-split-lifecycle-final-9.77";work.mkdir(exist_ok=False)
exe=root/".media-cache/owned-live-dash-continuation-build-9.77/release-native/UDM.exe"
assert m.sha(exe)=="F60C3839D38CD67DB1DFEE54CCF5BB82DA8AFB3AB087933DC3FD28206FA71A60"
results=[]
for connections in (2,8):
 folder=work/str(connections);folder.mkdir();data=folder/"catalog";data.mkdir();dest=folder/"files";dest.mkdir();output=dest/"sample.bin";size=32*1024**2
 m.save(data/"state.json",{"Schema":1,"Settings":{"Connections":connections,"Parallel":1,"ProxyMode":"Connect directly","DownloadFolder":str(dest),"CategoryFolders":False,"SuppressProgressDialog":True,"SuppressCompletionDialog":True,"Sound":False,"ShowTipsOnStartup":False,"ClipboardMonitor":False,"PrefetchFileInfo":False,"StopOnWebPage":False,"DuplicatePolicy":"Add","Retries":3},"Downloads":[],"Queues":[]})
 origin=m.Origin(m.Options(size=size,rate=2*1024**2,max_conns=2,latency_ms=20,chunk=64*1024,seed=20261113));clock=m.anchor();start=time.perf_counter()
 command=[str(exe),"--background","--instance-tag","cap-lifecycle-"+uuid.uuid4().hex[:16],"--data-dir",str(data),"/d",origin.url,"/p",str(dest),"/f",output.name,"/n","/q"]
 log=(folder/"native.log").open("w");p=subprocess.Popen(command,cwd=exe.parent,stdout=log,stderr=subprocess.STDOUT);m.save(folder/"process.json",{"pid":p.pid,"executableSha256":m.sha(exe)})
 first=None;pending=False
 while p.poll() is None:
  if first is None and output.exists() and output.stat().st_size==size and m.available(output):first=time.perf_counter()
  if not pending and time.perf_counter()-start>90:
   pending=True;print("Still-live owned process "+str(p.pid)+"; continuing observation without restart",flush=True)
  time.sleep(.01)
 log.close();m.save(folder/"exit.json",{"exitCode":p.returncode});endticks=m.exit_ticks(int(p._handle));completed=clock[0]+(endticks-clock[1])/1e7;after=m.anchor();assert abs((after[0]-clock[0])-(after[1]-clock[1])/1e7)<.005
 assert p.returncode==0 and output.is_file() and output.stat().st_size==size and m.available(output)
 snap=origin.snapshot();last=max(r["last_body_sent"] for c in snap["detail"] for r in c["requests"] if r.get("last_body_sent") is not None);lastperf=origin.epoch+last
 durable=m.durable_output(output);digest=m.sha(output);assert digest==m.expected_sha256(size,20261113).upper()
 state=m.load(data/"state.json");job=state["Downloads"][0];assert job["Status"]=="Complete" and job["Received"]==size
 row={"configuredConnections":connections,"sizeBytes":size,"passed":True,"normalExit":True,"exitCode":p.returncode,"sha256":digest,"executableSha256":m.sha(exe),"totalToExitSeconds":completed-start,"lastBodyToExitSeconds":completed-lastperf,"firstAvailabilityTailSeconds":first-lastperf if first is not None else None,"postExitToExplicitFlushSeconds":durable[0]-completed,"lastBodyToExplicitFlushSeconds":durable[0]-lastperf,"postExitOperations":durable[1],"job":{"status":job["Status"],"received":job["Received"],"segments":len(job.get("Segments",[])),"dynamicSplits":job.get("DynamicSplits"),"finishPhases":job.get("FinishPhases")},"origin":snap}
 m.save(folder/"result.json",row);results.append(row);m.save(work/"results.json",{"passed":len(results)==2,"results":results});print(json.dumps({k:row[k] for k in ("configuredConnections","passed","totalToExitSeconds","lastBodyToExitSeconds","postExitToExplicitFlushSeconds")}|{"refused":snap["rejected_connections"],"requests":snap["requests"]}),flush=True)
 origin.close()
 assert output.resolve().is_relative_to(work.resolve()) and not output.is_symlink() and output.stat().st_size==size and m.sha(output)==digest
 output.unlink();m.save(folder/"cleanup.json",{"payloadRemoved":True,"bytes":size,"sha256":digest})
m.save(work/"terminal.json",{"passed":True,"cases":len(results),"payloadsRemoved":True,"logicalPayloadBytesRemoved":2*32*1024**2,"versionUnchanged":"9.77","idmComparison":False,"scope":"Normal UDM primary process exit and explicit post-exit output flush, not system-service completion or GUI timing."})
