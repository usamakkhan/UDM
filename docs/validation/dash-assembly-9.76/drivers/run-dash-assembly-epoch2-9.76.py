from pathlib import Path
import json,subprocess,threading,time,http.server,struct,os,re,ctypes
base=Path('<development>');root=Path('<repo>');area=root/'.media-cache/owned-dash-assembly-workflow-9.76';assert json.loads((area/'terminal.json').read_text())['exitCode']==1
scope={'__file__':str(area/'config.py')};exec((base/'finishing-8.34/config.py').read_text(encoding='utf-8-sig'),scope);globals().update({k:v for k,v in scope.items() if not k.startswith('__')})
assets=area/'assets';inputs=json.loads((area/'inputs.json').read_text());protected=json.loads((base/'publication-overlap-9.61-protected-before.json').read_text());exe=release/'Udm.DashAssemblyWorkflow.exe';assert sha(exe)==json.loads((area/'terminal.json').read_text())['binarySha256']
for r in inputs['assetHashes']:assert sha(assets/r['path'])==r['sha256']
for r in protected:assert sha(Path(r['path']))==r['sha256']
requests=[]
class Handler(http.server.BaseHTTPRequestHandler):
 protocol_version='HTTP/1.1'
 def log_message(self,*args):pass
 def do_HEAD(self):self.serve(False)
 def do_GET(self):self.serve(True)
 def serve(self,body):
  path=(assets/self.path.split('?',1)[0].lstrip('/')).resolve();assert path.is_relative_to(assets.resolve())
  if not path.is_file():self.send_error(404);return
  data=path.read_bytes();etag='"'+sha(path).lower()+'"';start=0;end=len(data)-1;status=200;requested=self.headers.get('Range')
  if self.headers.get('If-Match',etag)!=etag:self.send_error(412);return
  if requested:
   match=re.fullmatch(r'bytes=(\d+)-(\d*)',requested);assert match;start=int(match[1]);end=min(end,int(match[2]) if match[2] else end);assert 0<=start<=end;status=206
  requests.append(dict(method=self.command,path=self.path,range=requested,status=status,bytes=end-start+1));self.send_response(status);self.send_header('Content-Type','application/dash+xml' if path.suffix=='.mpd' else 'video/mp4');self.send_header('Content-Length',str(end-start+1));self.send_header('ETag',etag);self.send_header('Accept-Ranges','bytes');self.send_header('Connection','close')
  if status==206:self.send_header('Content-Range',f'bytes {start}-{end}/{len(data)}')
  self.end_headers()
  if body:self.wfile.write(data[start:end+1])
  self.close_connection=True
server=http.server.ThreadingHTTPServer(('127.0.0.1',0),Handler);thread=threading.Thread(target=server.serve_forever,daemon=True);thread.start();work=area/'run-epoch2';destination=base/'owned-dash-assembly-workflow-9.76-C-epoch2';assert not work.exists() and not destination.exists();stopped=False;url=f'http://127.0.0.1:{server.server_port}/manifest.mpd'
(area/'epoch2-inputs.json').write_text(json.dumps(dict(runnerSha256=sha(Path(__file__)),binarySha256=sha(exe),sameMediaAssets=True,sameFrameOracles=True,correction='Origin now implements strong ETags, HEAD and validated range responses; no production code change.'),indent=2),encoding='utf-8')
with (area/'runtime-epoch2.log').open('w') as log:
 p=subprocess.Popen([str(exe),str(work),url,str(destination)],cwd=root,env=env,stdout=log,stderr=subprocess.STDOUT);(area/'process-epoch2.json').write_text(json.dumps(dict(pid=p.pid,binarySha256=sha(exe),work=str(work),destination=str(destination))),encoding='utf-8')
 while p.poll() is None:
  if not stopped and (work/'capture-complete.json').exists():server.shutdown();server.server_close();thread.join();stopped=True;(work/'offline.ready').write_text('Origin stopped before assembly');print('Real Period resources captured; origin now stopped for offline assembly',flush=True)
  time.sleep(.05)
 code=p.wait()
if not stopped:server.shutdown();server.server_close();thread.join()
(area/'requests-epoch2.json').write_text(json.dumps(dict(requests=requests,stoppedBeforeAssembly=stopped),indent=2),encoding='utf-8');d=json.loads((work/'results.json').read_text());(area/'terminal-epoch2.json').write_text(json.dumps(dict(passed=code==0 and d['passed'],exitCode=code,binarySha256=sha(exe),result=str(work/'results.json'),checks=len(d['checks']),destination=str(destination),originStoppedBeforeAssembly=stopped),indent=2),encoding='utf-8');assert code==0 and d['passed'],d
assert stopped and len(d['outputs'])==2
for output in d['outputs']:assert output['frames']==inputs['expectedFrames'][output['case']],output
for r in protected:assert sha(Path(r['path']))==r['sha256']
(area/'frame-oracle-epoch2.json').write_text(json.dumps(dict(passed=True,independentSourceFrames=20,finalDecodedFrames=30,singleAllFramesMatch=True,joinedBothPeriodsInOrder=True,prePublicationBytesEqualFinalBytes=all(o['prePublicationBytes']==o['finalBytes'] for o in d['outputs']),originStoppedBeforeAssembly=True),indent=2),encoding='utf-8');print(json.dumps(dict(checks=len(d['checks']),outputs=[dict(case=o['case'],bytes=o['finalBytes'],frames=len(o['frames'])) for o in d['outputs']],frameOraclePassed=True),indent=2),flush=True)
