from pathlib import Path
import json,subprocess,threading,time,http.server,re,datetime
base=Path('<development>');root=Path('<repo>');parent=root/'.media-cache/owned-live-dash-restart-9.76';assert json.loads((parent/'qualification.json').read_text())['passed'];area=root/'.media-cache/owned-live-dash-finish-refresh-9.77';assert not area.exists();area.mkdir()
scope={'__file__':str(area/'config.py')};exec((base/'finishing-8.34/config.py').read_text(encoding='utf-8-sig'),scope);globals().update({k:v for k,v in scope.items() if not k.startswith('__')});common+=['/I'+str(native),'/I'+str(native/'vendor/brotli/c/include')]
assets=parent/'assets';baselineInputs=json.loads((parent/'inputs.json').read_text());candidate=json.loads((parent/'resume-candidate.json').read_text());captureExe=Path(candidate['path']);assert sha(captureExe)==candidate['sha256'];build=root/'.media-cache/owned-live-dash-continuation-build-9.77';b=json.loads((build/'build-result.json').read_text())
for r in b['sources']:assert sha(root/r['path'])==r['sha256']
for r in b['coreObjects']:assert sha(Path(r['path']))==r['sha256']
for r in baselineInputs['assets']:assert sha(assets/r['path'])==r['sha256']
source=root/'benchmarks/speed/live_dash_finish_refresh.cpp';exe=release/'Udm.LiveDashFinishRefresh.exe'
with (area/'build.log').open('w') as log:p=subprocess.run([str(compiler),*common,str(source),*[r['path'] for r in b['coreObjects']],str(build/'App.res'),'/Fo'+str(out/'LiveDashFinishRefresh.obj'),'/Fe'+str(exe),'/link',*system,'/SUBSYSTEM:CONSOLE','/INCREMENTAL:NO','/MANIFEST:NO'],cwd=root,env=env,stdout=log,stderr=subprocess.STDOUT)
assert p.returncode==0,(area/'build.log').read_text()[-3500:]
tools=release/'tools';p=subprocess.run(['powershell','-NoProfile','-Command',"New-Item -ItemType Junction -Path '"+str(tools)+"' -Target 'C:/Program Files/UDM/tools' | Out-Null"],capture_output=True,text=True);assert p.returncode==0,p.stderr
original=base/'run-live-dash-restart-9.76.py';text=original.read_text(encoding='utf-8');definitions=text.split('guard=threading.Lock()',1)[1].split('server=http.server.ThreadingHTTPServer',1)[0];exec('guard=threading.Lock()'+definitions,globals());work=parent/'finish';destination=base/'owned-live-dash-restart-9.76-C/finish';assert not work.exists() and not destination.exists();releaseHeld=threading.Event();heldRequests=[]
class HoldingHandler(Handler):
 def serve(self,body):
  with guard:state=states['positive'].copy()
  if body and self.path=='/positive/manifest.mpd' and state['stage']==2:
   heldRequests.append(dict(method=self.command,path=self.path,headersWithheld=True));(work/'refresh-held.ready').write_text('Owned response headers held');releaseHeld.wait(25);self.close_connection=True;return
  return super().serve(body)
new_case('positive');server=http.server.ThreadingHTTPServer(('127.0.0.1',0),HoldingHandler);thread=threading.Thread(target=server.serve_forever,daemon=True);thread.start();url=f'http://127.0.0.1:{server.server_port}/positive/manifest.mpd'
try:
 with (area/'capture.log').open('w') as log:p=subprocess.Popen([str(captureExe),'capture',str(work),url,str(destination)],cwd=root,stdout=log,stderr=subprocess.STDOUT);(area/'capture-process.json').write_text(json.dumps(dict(pid=p.pid,binarySha256=sha(captureExe))),encoding='utf-8');code=p.wait()
 captured=json.loads((work/'capture-results.json').read_text());assert code==0 and captured['passed']
 with guard:states['positive']['stage']=2;states['positive']['step']='manual-finish'
 with (area/'finish.log').open('w') as log:p=subprocess.Popen([str(exe),str(work),url,str(destination)],cwd=root,stdout=log,stderr=subprocess.STDOUT);(area/'finish-process.json').write_text(json.dumps(dict(pid=p.pid,binarySha256=sha(exe))),encoding='utf-8');code=p.wait()
 d=json.loads((work/'finish-results.json').read_text());assert code==0 and d['passed'],d;assert d['frames']==baselineInputs['expectedFrames'][:10] and len(heldRequests)==1 and not releaseHeld.is_set();assert all(sum(r['method']=='GET' and r['path']==path for r in requests)==1 for path in ['p0/init.mp4','p0/segment.m4s'])
 (area/'qualification.json').write_text(json.dumps(dict(passed=True,version='9.77',fixtureSha256=sha(source),runnerSha256=sha(Path(__file__)),binarySha256=sha(exe),objects=b['coreObjects'],captureChecks=len(captured['checks']),finishChecks=len(d['checks']),framesMatched=10,completedWhileOriginHeadersHeld=True,retainedBodiesDownloadedOnce=True,finalBytes=d['savedJob']['Size']),indent=2),encoding='utf-8');print('Save during held Resume refresh completed using all ten retained frames',flush=True)
finally:releaseHeld.set();server.shutdown();server.server_close();thread.join();(area/'requests.json').write_text(json.dumps(dict(requests=requests,heldRequests=heldRequests,originClosed=True),indent=2),encoding='utf-8')
