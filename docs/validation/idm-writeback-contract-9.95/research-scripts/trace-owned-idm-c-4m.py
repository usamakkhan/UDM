from pathlib import Path
import os,json,subprocess,threading,queue,time,uuid,hashlib,sys,importlib.util,collections
repo=Path(r'<repo>');root=repo/'.media-cache/rea-io-research-9.95';sys.path.insert(0,str(repo/'benchmarks/speed'));from origin import Origin,Options,expected_sha256
spec=importlib.util.spec_from_file_location('owned_idm_comparison',repo/'benchmarks/speed/compare-completion.py');comparison=importlib.util.module_from_spec(spec);spec.loader.exec_module(comparison)
qual=Path(json.loads((root/'qualification-location.json').read_text(encoding='utf-8'))['folder']);assert json.loads((qual/'acceptance.json').read_text(encoding='utf-8'))['passed'];assert not comparison.idm_pids(),'Current-session IDM must be exited before this owned trace'
program=Path(r'<idm-install>\IDMan.exe');sha=lambda p:hashlib.sha256(p.read_bytes()).hexdigest().upper();assert sha(program)=='03CC62E9ADB77A380F9DC12F67CCAAEE5106F12844AA73CE32C914DDD16D607C';token=uuid.uuid4().hex[:8];folder=Path(os.environ['LOCALAPPDATA'])/'UDM-Development'/('rea-idm-c-4m-'+token);folder.mkdir();(folder/'.udm-io-observer-owned').write_text('Owned C: 4 MiB IDM IO trace; not a performance comparison',encoding='utf-8');output=folder/(token+'.bin');source=(root/'owned-io-agent.js').read_text(encoding='utf-8-sig').replace('__TOKEN__',json.dumps(token));original=comparison.read_settings();(folder/'idm-settings-original.json').write_text(json.dumps(original,indent=2),encoding='utf-8');origin=Origin(Options(size=4194304,rate=8000000,latency_ms=20,no_range=True,accept_ranges='none',seed=20261113));url=origin.url+'?owned-io='+token
(folder/'inputs.json').write_text(json.dumps({'targetSha256':sha(program),'agentSha256':hashlib.sha256(source.encode()).hexdigest().upper(),'bytes':4194304,'token':token,'capturedBeforeExecution':True,'qualification':str(qual),'scope':'Owned current-session IDM spawn only; other-session IDM left alone','loopbackOnly':True,'unmodifiedFunctionArgumentsAndReturns':True},indent=2),encoding='utf-8')
log=(folder/'mcp-stderr.log').open('w',encoding='utf-8');server=subprocess.Popen(['cmd.exe','/d','/c','rea','--mcp'],stdin=subprocess.PIPE,stdout=subprocess.PIPE,stderr=log,text=True,encoding='utf-8',env={**os.environ,'TEMP':str(root/'temp'),'TMP':str(root/'temp')});q=queue.Queue()
def reader():
 for line in server.stdout:
  try:q.put(json.loads(line))
  except ValueError:pass
threading.Thread(target=reader,daemon=True).start()
def rpc(i,m,params):
 server.stdin.write(json.dumps({'jsonrpc':'2.0','id':i,'method':m,'params':params})+'\n');server.stdin.flush();end=time.monotonic()+60
 while time.monotonic()<end:
  try:d=q.get(timeout=1)
  except queue.Empty:continue
  if d.get('id')==i:return d
 raise TimeoutError('Owned IDM trace RPC timeout; inspect original owned PID before retry')
terminal={'passed':False,'folder':str(folder)}
try:
 rpc(1,'initialize',{'protocolVersion':'2025-03-26','capabilities':{},'clientInfo':{'name':'UDM owned C: 4 MiB IDM file IO diagnostic','version':'1.0'}});server.stdin.write(json.dumps({'jsonrpc':'2.0','method':'notifications/initialized'})+'\n');server.stdin.flush();assert not comparison.idm_pids();comparison.put_settings({n:[1,comparison.winreg.REG_DWORD] for n in comparison.regnames});result=rpc(2,'tools/call',{'name':'instrument_with_frida','arguments':{'mode':'spawn','device_id':'local','program':str(program),'argv':['/d',url,'/p',str(folder),'/f',output.name,'/n','/q'],'source_kind':'inline','source':source,'duration_ms':7000}});(folder/'frida-response.json').write_text(json.dumps(result,indent=2),encoding='utf-8');content=result.get('result',{}).get('structuredContent');terminal['toolError']=result.get('result',{}).get('isError',False);terminal['outputExists']=output.exists();terminal['currentSessionIdmRemaining']=comparison.idm_pids();
 if content and 'normalized_result' in content:
  observed=content['normalized_result'];terminal['targetPid']=observed['pid'];payloads=[m.get('payload',{}) for m in observed['messages'] if m.get('type')=='send'];terminal['counts']=dict(collections.Counter(m['api'] for m in payloads if m.get('kind')=='owned-io'));terminal['unknownFilePartEvents']=sum(1 for m in payloads if m.get('kind')=='owned-io' and m.get('api','').startswith('file-part-') and not m.get('matchedScope'));terminal['readyArchitectures']=[m.get('architecture') for m in payloads if m.get('kind')=='observer-ready'];terminal['scriptErrors']=[m for m in observed['messages'] if m.get('type')!='send'];terminal['truncated']=observed['messages_truncated'];terminal['cleanupError']=observed['cleanup_error']
 if output.exists():
  terminal['bytes']=output.stat().st_size;terminal['sha256']=sha(output);terminal['hashMatches']=terminal['bytes']==4194304 and terminal['sha256']==expected_sha256(4194304,20261113).upper()
  if not terminal['currentSessionIdmRemaining']:
   begun=time.perf_counter();flushed=comparison.durable_output(output);terminal['postExitExplicitFlushMs']=(time.perf_counter()-begun)*1000;terminal['postExitFlushOperations']=flushed[1]
 terminal['targetBinaryUnchanged']=sha(program)=='03CC62E9ADB77A380F9DC12F67CCAAEE5106F12844AA73CE32C914DDD16D607C';terminal['passed']=bool(terminal.get('hashMatches') and not terminal['currentSessionIdmRemaining'] and not terminal.get('scriptErrors') and not terminal.get('truncated') and terminal.get('readyArchitectures')==['ia32'] and not terminal.get('cleanupError') and terminal['targetBinaryUnchanged'])
finally:
 comparison.put_settings(original);terminal['settingsRestored']=comparison.read_settings()==original;(folder/'idm-settings-restored.json').write_text(json.dumps({'matches':terminal['settingsRestored']}),encoding='utf-8');(folder/'origin.json').write_text(json.dumps(origin.snapshot(),indent=2),encoding='utf-8');origin.close();server.stdin.close()
 try:terminal['serverExitCode']=server.wait(timeout=20)
 except subprocess.TimeoutExpired:terminal['serverStillStoppingPid']=server.pid
 log.close();(folder/'acceptance.json').write_text(json.dumps(terminal,indent=2),encoding='utf-8');(root/'idm-trace-location.json').write_text(json.dumps({'folder':str(folder)}),encoding='utf-8');print(json.dumps(terminal))
