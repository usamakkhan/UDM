"""Launch real UDM against isolated state and a local server; never use personal history."""
from pathlib import Path
from http.server import ThreadingHTTPServer,BaseHTTPRequestHandler
import hashlib,json,threading,subprocess,time,sys,uuid,ctypes
from ctypes import wintypes
release=Path(sys.argv[1]).resolve();output=Path(sys.argv[2]).resolve()
assert not output.exists();output.mkdir(parents=True)
payload=(b'UDM independent startup download fixture\n'*65536);requests=[];checks=[];process=None
class Handler(BaseHTTPRequestHandler):
 def log_message(self,*args):pass
 def do_GET(self):
  start,end=0,len(payload)-1;partial=False
  if self.headers.get('Range'):
   first,last=self.headers['Range'].removeprefix('bytes=').split('-');start=int(first);end=min(int(last) if last else end,end);partial=True
  requests.append({'path':self.path,'range':self.headers.get('Range','')})
  self.send_response(206 if partial else 200);self.send_header('Content-Type','application/octet-stream');self.send_header('Content-Length',str(end-start+1));self.send_header('Accept-Ranges','bytes');self.send_header('ETag','"startup-fixture-v1"')
  if partial:self.send_header('Content-Range',f'bytes {start}-{end}/{len(payload)}')
  self.end_headers()
  try:self.wfile.write(payload[start:end+1])
  except (ConnectionAbortedError,ConnectionResetError,BrokenPipeError):pass
def record(value,name):
 checks.append({'passed':bool(value),'name':name});assert value,name;print('PASS '+name,flush=True)
def state():return json.loads((output/'state/state.json').read_text(encoding='utf-8-sig'))
def launch():return subprocess.Popen([str(release/'UDM.exe'),'--background','--data-dir',str(output/'state'),'--instance-tag',uuid.uuid4().hex],creationflags=0x08000000)
def owned_window_text(pid):
 # Diagnostics are limited to the exact candidate process created by this fixture.
 user=ctypes.WinDLL('user32',use_last_error=True);callback=ctypes.WINFUNCTYPE(wintypes.BOOL,wintypes.HWND,wintypes.LPARAM);rows=[]
 user.GetWindowThreadProcessId.argtypes=[wintypes.HWND,ctypes.POINTER(wintypes.DWORD)];user.GetWindowTextW.argtypes=[wintypes.HWND,wintypes.LPWSTR,ctypes.c_int];user.EnumWindows.argtypes=[callback,wintypes.LPARAM];user.EnumChildWindows.argtypes=[wintypes.HWND,callback,wintypes.LPARAM]
 def text(hwnd):
  value=ctypes.create_unicode_buffer(4096);user.GetWindowTextW(hwnd,value,len(value));return value.value
 def visit(hwnd,_):
  owner=wintypes.DWORD();user.GetWindowThreadProcessId(hwnd,ctypes.byref(owner))
  if owner.value==pid:
   children=[]
   def child(ch,_):children.append(text(ch));return True
   user.EnumChildWindows(hwnd,callback(child),0);rows.append({'title':text(hwnd),'children':children})
  return True
 user.EnumWindows(callback(visit),0);return rows
server=ThreadingHTTPServer(('127.0.0.1',0),Handler);threading.Thread(target=server.serve_forever,daemon=True).start()
origin=f'http://127.0.0.1:{server.server_port}'
try:
 (output/'state').mkdir();target=output/'downloads'
 jobs=[{'Id':uuid.uuid4().hex,'Url':origin+'/'+name,'FileName':name+'.udmfixture','Folder':str(target),'Status':'Paused','Queue':queue,'QueueMember':True,'Size':-1,'Received':0,'Segments':[]} for name,queue in [('startup','On startup'),('waiting','Manual')]]
 doc={'Schema':1,'Settings':{'DownloadFolder':str(target),'CategoryFolders':False,'ProxyMode':'Connect directly','SuppressProgressDialog':True,'SuppressCompletionDialog':True,'Sound':False,'Connections':4,'Retries':0},'Queues':[{'Name':'On startup','Enabled':False,'StartOnStartup':True,'Parallel':1},{'Name':'Manual','Enabled':False,'StartOnStartup':False,'Parallel':1}],'Downloads':jobs,'Projects':[]}
 (output/'state/state.json').write_text(json.dumps(doc),encoding='utf-8')
 process=launch();end=time.monotonic()+45
 while time.monotonic()<end:
  current=state();job=current['Downloads'][0]
  if job['Status']=='Failed':raise RuntimeError(job.get('Error'))
  if job['Status']=='Complete':break
  assert process.poll() is None,f'UDM exited before completing the fixture: {process.returncode}';time.sleep(.1)
 if job['Status']!='Complete':(output/'owned-window-diagnostics.json').write_text(json.dumps(owned_window_text(process.pid),indent=2),encoding='utf-8')
 record(job['Status']=='Complete','Real UDM startup starts an opted-in stopped queue')
 record(hashlib.sha256((target/jobs[0]['FileName']).read_bytes()).digest()==hashlib.sha256(payload).digest(),'Startup download SHA-256 matches server bytes')
 record(current['Downloads'][1]['Status']=='Paused' and not any(r['path']=='/waiting' for r in requests),'Queue without startup opt-in makes no request')
 process.terminate();process.wait(timeout=10);process=None
 current['Queues'][0]['StartOnStartup']=False;current['Queues'][0]['Enabled']=False;current['Downloads'][0]['Status']='Paused';current['Downloads'][0]['FileName']='opted-out.udmfixture';current['Downloads'][0]['Segments']=[];current['Downloads'][0]['Received']=0;current['Downloads'][0]['Size']=-1
 (output/'state/state.json').write_text(json.dumps(current),encoding='utf-8');before=len(requests);process=launch();time.sleep(4)
 record(process.poll() is None and state()['Downloads'][0]['Status']=='Paused' and len(requests)==before,'Actual app relaunch respects clearing startup opt-in')
finally:
 if process and process.poll() is None:process.terminate();process.wait(timeout=10)
 server.shutdown();server.server_close()
 (output/'results.json').write_text(json.dumps({'passed':len(checks)==4 and all(c['passed'] for c in checks),'checks':checks,'requests':requests,'payloadSha256':hashlib.sha256(payload).hexdigest(),'payloadBytes':len(payload)},indent=2),encoding='utf-8')
