from pathlib import Path
import sys,os,json,subprocess,time,uuid,importlib.util
root=Path('[repo]');base=Path(os.environ['LOCALAPPDATA'])/'UDM-Development'
area=root/'.media-cache/owned-network-route-smoke-9.79';area.mkdir(exist_ok=False)
(area/'.udm-build-owned').write_text('Owned normal-app local-volume smoke; not a speed ranking')
sys.path.insert(0,str(root/'benchmarks/speed'));spec=importlib.util.spec_from_file_location('completion',root/'benchmarks/speed/compare-completion.py');m=importlib.util.module_from_spec(spec);spec.loader.exec_module(m)
build=root/'.media-cache/owned-fixed-volume-build-9.79';b=json.loads((build/'build-result.json').read_text());s=json.loads((build/'suite-results.json').read_text());assert b['passed'] and s['passed'] and len(s['suites'])==23
image=next(r for r in b['outputs'] if r['name']=='UDM.exe');exe=Path(image['path']);assert m.sha(exe)==image['sha256']
m.save(area/'inputs.json',{'driverSha256':m.sha(Path(__file__)),'binarySha256':image['sha256'],'originSha256':m.sha(root/'benchmarks/speed/origin.py'),'helperSha256':m.sha(root/'benchmarks/speed/compare-completion.py'),'cases':['C','D'],'bytesPerFile':4*1024**2,'comparison':False})
records=[]
for drive,folder in [('C',base/'owned-network-route-smoke-9.79-C'),('D',area/'D')]:
 folder.mkdir(exist_ok=False);(folder/'.udm-build-owned').write_text('Owned local-volume smoke and private catalog');data=folder/'catalog';data.mkdir();files=folder/'files';files.mkdir();output=files/'sample.bin';size=4*1024**2
 m.save(data/'state.json',{'Schema':1,'Settings':{'Connections':1,'Parallel':1,'ProxyMode':'Connect directly','DownloadFolder':str(files),'CategoryFolders':False,'SuppressProgressDialog':True,'SuppressCompletionDialog':True,'Sound':False,'ShowTipsOnStartup':False,'ClipboardMonitor':False,'PrefetchFileInfo':False,'StopOnWebPage':False,'DuplicatePolicy':'Add'},'Downloads':[],'Queues':[]})
 origin=m.Origin(m.Options(size=size,rate=8_000_000,latency_ms=20,no_range=True,accept_ranges='none',seed=20261113));clock=m.anchor();start=time.perf_counter();log=(folder/'native.log').open('w')
 command=[str(exe),'--background','--instance-tag','route-'+uuid.uuid4().hex[:16],'--data-dir',str(data),'/d',origin.url,'/p',str(files),'/f',output.name,'/n','/q'];p=subprocess.Popen(command,cwd=exe.parent,stdout=log,stderr=subprocess.STDOUT);m.save(area/('process-'+drive+'.json'),{'pid':p.pid,'binarySha256':image['sha256']});pending=False
 while p.poll() is None:
  if not pending and time.perf_counter()-start>60:pending=True;print('Owned smoke PID '+str(p.pid)+' still live; continuing observation without restart',flush=True)
  time.sleep(.01)
 log.close();m.save(area/('exit-'+drive+'.json'),{'exitCode':p.returncode});assert p.returncode==0
 completed=clock[0]+(m.exit_ticks(int(p._handle))-clock[1])/1e7;after=m.anchor();assert abs((after[0]-clock[0])-(after[1]-clock[1])/1e7)<.005
 assert output.is_file() and output.stat().st_size==size and m.available(output)
 durable=m.durable_output(output);digest=m.sha(output);assert digest==m.expected_sha256(size,20261113).upper()
 job=m.load(data/'state.json')['Downloads'][0];assert job['Status']=='Complete' and job['Received']==size and job['Sha256'].upper()==digest
 assert Path(str(output)+':Zone.Identifier').read_bytes()==b'[ZoneTransfer]\r\nZoneId=3\r\n'
 traffic=origin.snapshot();assert traffic['requests']==1 and traffic['bytes_sent']==size and traffic['rejected_connections']==0
 last=origin.epoch+max(r['last_body_sent'] for c in traffic['detail'] for r in c['requests'] if r.get('last_body_sent') is not None)
 row={'drive':drive,'passed':True,'normalProduct':True,'binarySha256':image['sha256'],'bytes':size,'sha256':digest,'normalExit':True,'savedStatus':'Complete','exactZoneMarker':True,'requests':traffic['requests'],'servedBytes':traffic['bytes_sent'],'bodyToExitSeconds':completed-last,'bodyToExplicitFlushSeconds':durable[0]-last,'explicitFlushOperations':durable[1],'finishPhases':job.get('FinishPhases'),'publicationPhases':job.get('PublicationPhases'),'performanceComparison':False}
 m.save(area/('result-'+drive+'.json'),row);records.append(row);origin.close()
 for node in [folder,files,output]:assert not node.lstat().st_file_attributes&1024
 assert output.resolve().is_relative_to(folder.resolve()) and output.stat().st_nlink==1 and m.sha(output)==digest
 output.unlink();m.save(area/('cleanup-'+drive+'.json'),{'payloadRemoved':True,'bytes':size,'sha256':digest});print(json.dumps({'drive':drive,'passed':True,'normalExit':True,'requests':1,'hashMatches':True,'payloadRemoved':True}),flush=True)
m.save(area/'terminal.json',{'passed':True,'records':records,'payloadsRemoved':True,'logicalPayloadBytesRemoved':8*1024**2,'scope':'Normal App local routing and completion correctness; two smokes, not an IDM or speed-gate comparison'})
