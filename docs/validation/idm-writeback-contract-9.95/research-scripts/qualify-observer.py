from pathlib import Path
import os,json,subprocess,threading,queue,time,uuid,hashlib
root=Path(r'<research>');folder=root/('qualification-'+uuid.uuid4().hex);folder.mkdir();(folder/'.udm-io-observer-owned').write_text('Owned observer API qualification only',encoding='utf-8');token=uuid.uuid4().hex[:12];output=folder/(token+'.bin');exe=root/'release-native/observer-control.exe';assert hashlib.sha256(exe.read_bytes()).hexdigest().upper()==json.loads((root/'observer-build-result.json').read_text(encoding='utf-8'))['exeSha256'];source=(root/'owned-io-agent.js').read_text(encoding='utf-8-sig').replace('__TOKEN__',json.dumps(token));(folder/'inputs.json').write_text(json.dumps({'programSha256':hashlib.sha256(exe.read_bytes()).hexdigest().upper(),'agentSha256':hashlib.sha256(source.encode()).hexdigest().upper(),'token':token,'bytes':8192,'capturedBeforeExecution':True},indent=2),encoding='utf-8')
log=(folder/'mcp-stderr.log').open('w',encoding='utf-8');p=subprocess.Popen(['cmd.exe','/d','/c','rea','--mcp'],stdin=subprocess.PIPE,stdout=subprocess.PIPE,stderr=log,text=True,encoding='utf-8',env={**os.environ,'TEMP':str(root/'temp'),'TMP':str(root/'temp')});q=queue.Queue()
def reader():
 for line in p.stdout:
  try:q.put(json.loads(line))
  except ValueError:pass
threading.Thread(target=reader,daemon=True).start()
def rpc(i,m,params):
 p.stdin.write(json.dumps({'jsonrpc':'2.0','id':i,'method':m,'params':params})+'\n');p.stdin.flush();end=time.monotonic()+60
 while time.monotonic()<end:
  try:d=q.get(timeout=1)
  except queue.Empty:continue
  if d.get('id')==i:return d
 raise TimeoutError('Owned qualification request timeout; inspect original state')
try:
 rpc(1,'initialize',{'protocolVersion':'2025-03-26','capabilities':{},'clientInfo':{'name':'UDM owned IO observer qualification','version':'1.0'}});p.stdin.write(json.dumps({'jsonrpc':'2.0','method':'notifications/initialized'})+'\n');p.stdin.flush();result=rpc(2,'tools/call',{'name':'instrument_with_frida','arguments':{'mode':'spawn','device_id':'local','program':str(exe),'argv':[str(output)],'source_kind':'inline','source':source,'duration_ms':2500}});(folder/'frida-response.json').write_text(json.dumps(result,indent=2),encoding='utf-8');(root/'qualification-location.json').write_text(json.dumps({'folder':str(folder)}),encoding='utf-8');print(json.dumps(result)[:2100]);print('outputExists',output.exists());
 if output.exists():print('hashMatches',hashlib.sha256(output.read_bytes()).hexdigest()==hashlib.sha256(bytes(i%251 for i in range(8192))).hexdigest())
finally:
 p.stdin.close()
 try:p.wait(timeout=20)
 except subprocess.TimeoutExpired:print('Qualification server still stopping',p.pid)
 log.close()
