from pathlib import Path
import json,subprocess,threading,time,http.server,re,datetime
base=Path('<development>');generator=base/'run-dash-assembly-workflow-9.76.py';prefix=generator.read_text(encoding='utf-8').split('document=',1)[0].replace('owned-dash-assembly-workflow-9.76','owned-live-dash-restart-9.76');exec(prefix,globals())
source=root/'benchmarks/speed/live_dash_restart_workflow.cpp';exe=release/'Udm.LiveDashRestartBaseline.exe';inputs=dict(baselineVersion='9.76',fixtureSha256=sha(source),runnerSha256=sha(Path(__file__)),generatorSha256=sha(generator),sources=b['sources'],objects=b['coreObjects'],expectedFrames=expected[0]+expected[1],assets=[dict(path=p.relative_to(assets).as_posix(),sha256=sha(p),bytes=p.stat().st_size) for p in assets.rglob('*') if p.is_file()]);(area/'inputs.json').write_text(json.dumps(inputs,indent=2),encoding='utf-8')
with (area/'build-baseline.log').open('w') as log:p=subprocess.run([str(compiler),*common,str(source),*[r['path'] for r in b['coreObjects']],str(build/'App.res'),'/Fo'+str(out/'LiveDashRestartBaseline.obj'),'/Fe'+str(exe),'/link',*system,'/SUBSYSTEM:CONSOLE','/INCREMENTAL:NO','/MANIFEST:NO'],cwd=root,env=env,stdout=log,stderr=subprocess.STDOUT)
assert p.returncode==0,(area/'build-baseline.log').read_text()[-3500:]
guard=threading.Lock();requests=[];states={};records=[]
def clock():return datetime.datetime.now(datetime.timezone.utc)
def stamp(value):return value.isoformat(timespec='milliseconds').replace('+00:00','Z')
def new_case(name):
 with guard:states[name]=dict(stage=1,step='capture',origin=clock()-datetime.timedelta(seconds=4))
def manifest(case,state):
 stage=state['stage'];dynamic=stage<3;now=clock();attrs=f'type="dynamic" availabilityStartTime="{stamp(state["origin"])}" minimumUpdatePeriod="PT0.2S" suggestedPresentationDelay="PT3600S" timeShiftBufferDepth="PT3600S"' if dynamic else 'type="static" mediaPresentationDuration="PT3S"'
 text=f'<MPD xmlns="urn:mpeg:dash:schema:mpd:2011" id="owned-restart-{case}" {attrs} publishTime="{stamp(now)}">'
 if dynamic:text+=f'<UTCTiming schemeIdUri="urn:mpeg:dash:utc:direct:2014" value="{stamp(now)}"/>'
 for n in ([0] if stage==1 else [1]):
  start=0 if n==0 else (1 if case=='positive' else 2);duration=2 if n==0 and case=='incomplete' else 1
  text+=f'<Period id="p{n}" start="PT{start}S" duration="PT{duration}S"><AdaptationSet id="v" mimeType="video/mp4" codecs="avc1.42c01e" width="640" height="360"><Representation id="r" bandwidth="100000"><SegmentList timescale="1000" startNumber="1"><SegmentTimeline><S t="0" d="1000"/></SegmentTimeline><Initialization sourceURL="p{n}/init.mp4"/><SegmentURL media="p{n}/segment.m4s"/></SegmentList></Representation></AdaptationSet></Period>'
 return (text+'</MPD>').encode()
class Handler(http.server.BaseHTTPRequestHandler):
 protocol_version='HTTP/1.1'
 def log_message(self,*args):pass
 def do_HEAD(self):self.serve(False)
 def do_GET(self):self.serve(True)
 def serve(self,body):
  case,relative=self.path.split('?',1)[0].lstrip('/').split('/',1)
  with guard:state=states[case].copy()
  if state['stage']>=2 and relative.startswith('p0/'):
   with guard:requests.append(dict(case=case,step=state['step'],method=self.command,path=relative,status=410))
   self.send_error(410);return
  if relative=='manifest.mpd':data=manifest(case,state)
  else:
   path=(assets/relative).resolve();assert path.is_relative_to(assets.resolve());data=path.read_bytes()
  etag='"'+__import__('hashlib').sha256(data).hexdigest()+'"';first=0;last=len(data)-1;status=200;requested=self.headers.get('Range')
  if self.headers.get('If-Match',etag)!=etag:self.send_error(412);return
  if requested:
   match=re.fullmatch(r'bytes=(\d+)-(\d*)',requested);assert match;first=int(match[1]);last=min(last,int(match[2]) if match[2] else last);assert 0<=first<=last;status=206
  with guard:requests.append(dict(case=case,step=state['step'],stage=state['stage'],method=self.command,path=relative,status=status,range=requested,bytes=last-first+1))
  self.send_response(status);self.send_header('Content-Type','application/dash+xml' if relative=='manifest.mpd' else 'video/mp4');self.send_header('Content-Length',str(last-first+1));self.send_header('ETag',etag);self.send_header('Accept-Ranges','bytes');self.send_header('Connection','close')
  if status==206:self.send_header('Content-Range',f'bytes {first}-{last}/{len(data)}')
  self.end_headers()
  if body:
   self.wfile.write(data[first:last+1])
   if relative=='p1/segment.m4s':
    with guard:states[case]['stage']=3
  self.close_connection=True
server=http.server.ThreadingHTTPServer(('127.0.0.1',0),Handler);thread=threading.Thread(target=server.serve_forever,daemon=True);thread.start();destinationRoot=base/'owned-live-dash-restart-9.76-C';assert not destinationRoot.exists()
def actor(binary,stage,case):
 work=area/case;url=f'http://127.0.0.1:{server.server_port}/{case}/manifest.mpd';destination=destinationRoot/case
 with guard:states[case]['step']=stage
 with (area/(case+'-'+stage+'.log')).open('w') as log:
  p=subprocess.Popen([str(binary),stage,str(work),url,str(destination)],cwd=root,env=env,stdout=log,stderr=subprocess.STDOUT);(area/'active-actor.json').write_text(json.dumps(dict(pid=p.pid,binary=str(binary),sha256=sha(binary),stage=stage,case=case)),encoding='utf-8');code=p.wait()
 result=json.loads((work/(stage+'-results.json')).read_text());row=dict(case=case,stage=stage,exitCode=code,binarySha256=sha(binary),result=str(work/(stage+'-results.json')));records.append(row);(area/'actors.json').write_text(json.dumps(records,indent=2),encoding='utf-8');print(case+' '+stage+': exit '+str(code)+'; '+result['error'],flush=True);return result,code
def retained(case):
 work=area/case;data=json.loads((work/'capture-results.json').read_text());folder=Path(data['savedJob']['MediaFolder'])/'live-dash';return {p.relative_to(folder).as_posix():sha(p) for p in folder.rglob('*') if p.is_file() and p.suffix not in ['.json','.bak']}
try:
 new_case('positive');captured,code=actor(exe,'capture','positive');assert code==0 and captured['passed'] and captured['retainedEnd']==captured['retainedPeriodDuration']==1;before=retained('positive');assert before
 with guard:states['positive']['stage']=2
 failed,code=actor(exe,'baseline','positive');assert code==1 and not failed['passed'] and 'no longer present' in failed['savedJob']['Error'].lower() and failed['publications']==0;assert retained('positive')==before
 (area/'baseline-reproduction.json').write_text(json.dumps(dict(reproduced=True,completeRetiredPeriodPreserved=True,error=failed['savedJob']['Error'],resourceHashes=before,sourceVersion='9.76'),indent=2),encoding='utf-8');print('Baseline reproduces retired-Period Resume failure; retained bytes unchanged. Waiting for candidate.',flush=True)
 signal=area/'resume-candidate.json';deadline=time.monotonic()+1800
 while not signal.exists() and time.monotonic()<deadline:time.sleep(.2)
 assert signal.exists(),'Candidate was not provided before bounded diagnostic wait ended';candidate=json.loads(signal.read_text());candidateExe=Path(candidate['path']);assert candidateExe.resolve()==(root/'.media-cache/owned-live-dash-restart-candidate-9.77/release-native/Udm.LiveDashRestartCandidate.exe').resolve();assert candidate['version']=='9.77' and sha(candidateExe)==candidate['sha256'] and sha(source)==inputs['fixtureSha256']
 fixed,code=actor(candidateExe,'candidate','positive');assert code==0 and fixed['passed'] and fixed['frames']==inputs['expectedFrames'] and fixed['publications']==1;assert all(sum(r['case']=='positive' and r['method']=='GET' and r['path']==path for r in requests)==1 for path in ['p0/init.mp4','p0/segment.m4s','p1/init.mp4','p1/segment.m4s'])
 for case in ['incomplete','gap']:
  new_case(case);captured,code=actor(candidateExe,'capture',case);assert code==0 and captured['passed'];beforeCase=retained(case)
  if case=='incomplete':assert captured['retainedEnd']==1 and captured['retainedPeriodDuration']==2
  with guard:states[case]['stage']=2
  rejected,code=actor(candidateExe,'candidate',case);assert code==1 and not rejected['passed'] and rejected['publications']==0 and rejected['savedJob']['Status']!='Complete';assert retained(case)==beforeCase
 for r in protected:assert sha(Path(r['path']))==r['sha256']
 (area/'qualification.json').write_text(json.dumps(dict(passed=True,baselineReproduced=True,freshProcessContinuationCompleted=True,sourceFramesMatched=20,positivePrePublicationBytes=fixed['prePublicationBytes'],positiveFinalBytes=fixed['finalBytes'],oldResourcesNeverRedownloaded=True,incompletePrefixRejected=True,gapRejected=True,allRejectedResourcesPreserved=True),indent=2),encoding='utf-8');print('Restart qualification and both rejection controls passed',flush=True)
finally:
 server.shutdown();server.server_close();thread.join();(area/'requests.json').write_text(json.dumps(dict(requests=requests,originClosed=True),indent=2),encoding='utf-8')
