import os,json,subprocess,threading,queue,time,hashlib,shutil
from pathlib import Path
root=Path(r'<research>');root.mkdir(exist_ok=False);(root/'.udm-build-owned').write_text('Owned read-only IDM static analysis; no live attachment or installation',encoding='utf-8');requests=root/'requests';responses=root/'responses';requests.mkdir();responses.mkdir();(root/'temp').mkdir()
target=Path(r'<idm-install>\IDMan.exe');sha=lambda p:hashlib.file_digest(p.open('rb'),'sha256').hexdigest().upper();digest=sha(target);assert digest=='03CC62E9ADB77A380F9DC12F67CCAAEE5106F12844AA73CE32C914DDD16D607C'
inputs={'target':'Installed IDM 6.43.10.2','sha256':digest,'analysisOnly':True,'liveAppInstrumented':False,'provider':'Existing local Ghidra 12.1.3 / REA 6.5.0','startupBudgetMs':900000,'requestBudgetSeconds':1100,'wholeSessionBudgetSeconds':1800,'minimumDFreeBytes':3500*1024**2,'minimumCFreeBytes':1500*1024**2,'maxAdditionalQueries':10,'childOnlyEnvironment':True,'globalConfigurationOrProviderCodeChanged':False};(root/'session-inputs.json').write_text(json.dumps(inputs,indent=2),encoding='utf-8')
env={**os.environ,'GHIDRA_INSTALL_DIR':r'<repo>\.media-cache\reverse-tools\ghidra_12.1.3_PUBLIC','REA_GHIDRA_STARTUP_TIMEOUT_MS':'900000','TEMP':str(root/'temp'),'TMP':str(root/'temp')};log=(root/'mcp-static-stderr.log').open('w',encoding='utf-8');p=subprocess.Popen(['cmd.exe','/d','/c','rea','--mcp'],stdin=subprocess.PIPE,stdout=subprocess.PIPE,stderr=log,text=True,encoding='utf-8',env=env);q=queue.Queue();start=time.monotonic();(root/'session-process.json').write_text(json.dumps({'pythonPid':os.getpid(),'serverPid':p.pid,'startedUtc':time.strftime('%Y-%m-%dT%H:%M:%SZ',time.gmtime())}),encoding='utf-8')
def reader():
 for line in p.stdout:
  try:q.put(json.loads(line))
  except ValueError:pass
threading.Thread(target=reader,daemon=True).start()
def rpc(i,method,params,cap=1100):
 p.stdin.write(json.dumps({'jsonrpc':'2.0','id':i,'method':method,'params':params})+'\n');p.stdin.flush();begun=time.monotonic();last=begun
 while True:
  try:item=q.get(timeout=1)
  except queue.Empty:
   if p.poll() is not None:raise RuntimeError('REA server exited: '+str(p.returncode))
   item=None
  now=time.monotonic()
  if now-last>=30:print('Waiting on '+method+' '+str(i)+'; server alive; '+str(round(now-begun))+' seconds',flush=True);last=now
  if now-begun>cap or shutil.disk_usage(root).free<inputs['minimumDFreeBytes'] or shutil.disk_usage('C:/').free<inputs['minimumCFreeBytes']:
   p.stdin.write(json.dumps({'jsonrpc':'2.0','method':'notifications/cancelled','params':{'requestId':i,'reason':'Bounded research time or free-space budget'}})+'\n');p.stdin.flush();raise RuntimeError('Cancellation sent; inspect original process before any reuse')
  if item and item.get('id')==i:return item
seq=10
failed=False

def call(tool,args,label):
 global seq
 seq+=1;result=rpc(seq,'tools/call',{'name':tool,'arguments':args});(responses/(label+'.json')).write_text(json.dumps(result,indent=2),encoding='utf-8');bad=result.get('result',{}).get('isError',False) or 'error' in result;print(label+': '+('ERROR' if bad else 'response retained')+' ('+str(len(json.dumps(result)))+' bytes)',flush=True);return result,bad
terminal={'staticQueriesSucceeded':False,'closed':False}
try:
 rpc(1,'initialize',{'protocolVersion':'2025-03-26','capabilities':{},'clientInfo':{'name':'UDM read-only storage interoperability investigation','version':'1.0'}},60);p.stdin.write(json.dumps({'jsonrpc':'2.0','method':'notifications/initialized'})+'\n');p.stdin.flush()
 _,bad=call('open_binary',{'path':str(target),'provider_id':'ghidra'},'open-idm')
 if bad:raise RuntimeError('Target admission failed; no analysis retried')
 result,bad=call('search_procedures',{'pattern':'FlushFileBuffers|CreateFileA|CreateFileW|NtFlushBuffersFile','mode':'regex','case_sensitive':False},'io-procedures')
 if bad:raise RuntimeError('Static initialization/query failed; no repeated startup retries in this session')
 terminal['staticQueriesSucceeded']=True;print('STATIC QUERY SESSION READY',flush=True);seen=set();idle=time.monotonic()+600;allowed={'search_procedures','procedure_callers','procedure_info','procedure_pseudo_code','procedure_references','procedure_assembly','xrefs','inspect_native_api'}
 while time.monotonic()-start<1800 and time.monotonic()<idle and len(seen)<10 and not (root/'stop-session').exists():
  for file in sorted(requests.glob('*.json')):
   if (root/'stop-session').exists():break
   if file.name in seen:continue
   try:data=json.loads(file.read_text(encoding='utf-8-sig'))
   except (FileNotFoundError,json.JSONDecodeError):continue
   assert data['tool'] in allowed
   _,bad=call(data['tool'],data.get('arguments',{}),file.stem);seen.add(file.name);idle=time.monotonic()+600
   if bad:print('Query failed; no automatic retry',flush=True)
  time.sleep(.25)
except Exception as error:terminal['error']=str(error);print(str(error),flush=True)
finally:
 try:
  _,bad=call('close_binary',{},'close-idm');terminal['closed']=not bad
 except Exception as error:terminal['closeError']=str(error)
 try:p.stdin.close()
 except Exception:pass
 try:terminal['serverExitCode']=p.wait(timeout=20)
 except subprocess.TimeoutExpired:terminal['serverStillStoppingPid']=p.pid
 terminal['targetUnchanged']=sha(target)==digest;terminal['elapsedSeconds']=round(time.monotonic()-start,3);(root/'terminal.json').write_text(json.dumps(terminal,indent=2),encoding='utf-8');log.close();print(json.dumps(terminal),flush=True)
