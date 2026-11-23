from pathlib import Path
import json,subprocess,threading,time,http.server,re,datetime
base=Path('<development>');root=Path('<repo>');area=root/'.media-cache/owned-live-dash-restart-9.76';assert json.loads((area/'qualification.json').read_text())['passed']
def sha(p):return __import__('hashlib').sha256(p.read_bytes()).hexdigest().upper()
signal=json.loads((area/'resume-candidate.json').read_text());exe=Path(signal['path']);assert sha(exe)==signal['sha256'];assets=area/'assets';inputs=json.loads((area/'inputs.json').read_text());assert sha(root/'benchmarks/speed/live_dash_restart_workflow.cpp')==inputs['fixtureSha256']
for r in inputs['assets']:assert sha(assets/r['path'])==r['sha256']
original=base/'run-live-dash-restart-9.76.py';text=original.read_text(encoding='utf-8');definitions=text.split('guard=threading.Lock()',1)[1].split('server=http.server.ThreadingHTTPServer',1)[0];definitions=definitions.replace("if state['stage']>=2 and relative.startswith('p0/'):","if False and state['stage']>=2 and relative.startswith('p0/'):");exec('guard=threading.Lock()'+definitions,globals())
originalManifest=manifest;manifestPhases=[];shownNext=False
def manifest(case,state):
 global shownNext
 if state['stage']==2:
  if not shownNext:shownNext=True;manifestPhases.append('current-only');return originalManifest(case,{**state,'stage':1})
  manifestPhases.append('current-and-next');current=originalManifest(case,{**state,'stage':1});nextBody=originalManifest(case,state);oldPeriod=re.search(rb'<Period .*?</Period>',current).group(0);at=nextBody.index(b'<Period ');return nextBody[:at]+oldPeriod+nextBody[at:]
 manifestPhases.append('initial' if state['stage']==1 else 'static');return originalManifest(case,state)
new_case('positive');server=http.server.ThreadingHTTPServer(('127.0.0.1',0),Handler);thread=threading.Thread(target=server.serve_forever,daemon=True);thread.start();work=area/'visible';destination=base/'owned-live-dash-restart-9.76-C/visible';assert not work.exists() and not destination.exists();url=f'http://127.0.0.1:{server.server_port}/positive/manifest.mpd';runs=[]
try:
 for stage in ['capture','candidate']:
  if stage=='candidate':
   with guard:states['positive']['stage']=2
  with guard:states['positive']['step']=stage
  with (area/('visible-'+stage+'.log')).open('w') as log:
   p=subprocess.Popen([str(exe),stage,str(work),url,str(destination)],cwd=root,stdout=log,stderr=subprocess.STDOUT);(area/'visible-active.json').write_text(json.dumps(dict(pid=p.pid,stage=stage,binarySha256=sha(exe))),encoding='utf-8');code=p.wait()
  d=json.loads((work/(stage+'-results.json')).read_text());assert code==0 and d['passed'],d;runs.append(dict(stage=stage,checks=len(d['checks']),result=str(work/(stage+'-results.json')),binarySha256=sha(exe)))
 assert d['frames']==inputs['expectedFrames'] and d['publications']==1;assert 'current-only' in manifestPhases and 'current-and-next' in manifestPhases and 'static' in manifestPhases
 assert all(sum(r['method']=='GET' and r['path']==path for r in requests)==1 for path in ['p0/init.mp4','p0/segment.m4s','p1/init.mp4','p1/segment.m4s'])
 (area/'visible-qualification.json').write_text(json.dumps(dict(passed=True,version='9.77',runnerSha256=sha(Path(__file__)),runs=runs,manifestPhases=manifestPhases,framesMatched=20,currentPeriodRematchAndPreparationVerified=True,mediaBodiesDownloadedOnce=True,finalBytes=d['finalBytes'],prePublicationBytes=d['prePublicationBytes']),indent=2),encoding='utf-8');print('Still-visible Period restart, later continuation and all twenty frames verified',flush=True)
finally:server.shutdown();server.server_close();thread.join();(area/'visible-requests.json').write_text(json.dumps(dict(requests=requests,originClosed=True),indent=2),encoding='utf-8')
