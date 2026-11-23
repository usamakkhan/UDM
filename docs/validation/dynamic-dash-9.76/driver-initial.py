from pathlib import Path
import json,subprocess,threading,time,http.server,re,datetime
base=Path('<development>');initial=base/'run-dash-assembly-workflow-9.76.py';prefix=initial.read_text(encoding='utf-8').split('document=',1)[0].replace('owned-dash-assembly-workflow-9.76','owned-dynamic-dash-workflow-9.76');exec(prefix,globals())
source=root/'benchmarks/speed/dynamic_dash_workflow.cpp';exe=release/'Udm.DynamicDashWorkflow.exe';inputs=dict(version='9.76',fixtureSha256=sha(source),runnerSha256=sha(Path(__file__)),assetGeneratorSha256=sha(initial),sources=b['sources'],objects=b['coreObjects'],expectedFrames=expected[0]+expected[1],assetHashes=[dict(path=p.relative_to(assets).as_posix(),sha256=sha(p),bytes=p.stat().st_size) for p in assets.rglob('*') if p.is_file()]);(area/'inputs.json').write_text(json.dumps(inputs,indent=2),encoding='utf-8')
with (area/'build.log').open('w') as log:p=subprocess.run([str(compiler),*common,str(source),*[r['path'] for r in b['coreObjects']],str(build/'App.res'),'/Fo'+str(out/'DynamicDashWorkflow.obj'),'/Fe'+str(exe),'/link',*system,'/SUBSYSTEM:CONSOLE','/INCREMENTAL:NO','/MANIFEST:NO'],cwd=root,env=env,stdout=log,stderr=subprocess.STDOUT)
assert p.returncode==0,(area/'build.log').read_text()[-3500:]
requests=[];state=dict(p0=False,p1=False);guard=threading.Lock();started=datetime.datetime.now(datetime.timezone.utc)-datetime.timedelta(seconds=4)
def stamp(value):return value.isoformat(timespec='milliseconds').replace('+00:00','Z')
def manifest(phase):
 now=datetime.datetime.now(datetime.timezone.utc);dynamic=phase<3;attrs=f'type="dynamic" availabilityStartTime="{stamp(started)}" minimumUpdatePeriod="PT0.2S" suggestedPresentationDelay="PT30S" timeShiftBufferDepth="PT60S"' if dynamic else 'type="static" mediaPresentationDuration="PT2S"'
 text=f'<MPD xmlns="urn:mpeg:dash:schema:mpd:2011" id="owned-changing-stream" {attrs} publishTime="{stamp(now)}">'
 if dynamic:text+=f'<UTCTiming schemeIdUri="urn:mpeg:dash:utc:direct:2014" value="{stamp(now)}"/>'
 for n in range(1 if phase==1 else 2):text+=f'<Period id="p{n}" start="PT{n}S" duration="PT1S"><AdaptationSet id="v" mimeType="video/mp4" codecs="avc1.42c01e" width="640" height="360"><Representation id="r" bandwidth="100000"><SegmentList timescale="1000" duration="1000" startNumber="1"><Initialization sourceURL="p{n}/init.mp4"/><SegmentURL media="p{n}/segment.m4s"/></SegmentList></Representation></AdaptationSet></Period>'
 return (text+'</MPD>').encode()
class Handler(http.server.BaseHTTPRequestHandler):
 protocol_version='HTTP/1.1'
 def log_message(self,*args):pass
 def do_HEAD(self):self.serve(False)
 def do_GET(self):self.serve(True)
 def serve(self,body):
  with guard:phase=3 if state['p1'] else 2 if state['p0'] else 1
  name=self.path.split('?',1)[0]
  if name=='/manifest.mpd':data=manifest(phase)
  else:
   path=(assets/name.lstrip('/')).resolve();assert path.is_relative_to(assets.resolve());data=path.read_bytes()
  etag='"'+__import__('hashlib').sha256(data).hexdigest()+'"';first=0;last=len(data)-1;status=200;requested=self.headers.get('Range')
  if self.headers.get('If-Match',etag)!=etag:self.send_error(412);return
  if requested:
   match=re.fullmatch(r'bytes=(\d+)-(\d*)',requested);assert match;first=int(match[1]);last=min(last,int(match[2]) if match[2] else last);assert 0<=first<=last;status=206
  with guard:requests.append(dict(method=self.command,path=name,stage=phase,status=status,range=requested,bytes=last-first+1))
  self.send_response(status);self.send_header('Content-Type','application/dash+xml' if name=='/manifest.mpd' else 'video/mp4');self.send_header('Content-Length',str(last-first+1));self.send_header('ETag',etag);self.send_header('Accept-Ranges','bytes');self.send_header('Connection','close')
  if status==206:self.send_header('Content-Range',f'bytes {first}-{last}/{len(data)}')
  self.end_headers()
  if body:
   self.wfile.write(data[first:last+1])
   if name in ['/p0/segment.m4s','/p1/segment.m4s']:
    with guard:state['p0' if name.startswith('/p0/') else 'p1']=True
  self.close_connection=True
server=http.server.ThreadingHTTPServer(('127.0.0.1',0),Handler);thread=threading.Thread(target=server.serve_forever,daemon=True);thread.start();work=area/'run';destination=base/'owned-dynamic-dash-workflow-9.76-C';assert not destination.exists();url=f'http://127.0.0.1:{server.server_port}/manifest.mpd'
with (area/'runtime.log').open('w') as log:
 p=subprocess.Popen([str(exe),str(work),url,str(destination)],cwd=root,env=env,stdout=log,stderr=subprocess.STDOUT);(area/'process.json').write_text(json.dumps(dict(pid=p.pid,binarySha256=sha(exe),work=str(work),destination=str(destination))),encoding='utf-8');code=p.wait()
server.shutdown();server.server_close();thread.join();(area/'requests.json').write_text(json.dumps(dict(requests=requests,state=state,originClosed=True),indent=2),encoding='utf-8');d=json.loads((work/'results.json').read_text());(area/'terminal.json').write_text(json.dumps(dict(passed=code==0 and d['passed'],exitCode=code,binarySha256=sha(exe),checks=len(d['checks']),result=str(work/'results.json'),destination=str(destination)),indent=2),encoding='utf-8');assert code==0 and d['passed'],d
assert d['frames']==inputs['expectedFrames'] and d['publications']==1;stages=[r['stage'] for r in requests if r['path']=='/manifest.mpd' and r['method']=='GET'];assert set(stages)=={1,2,3},stages
assert all(sum(r['method']=='GET' and r['path']==path for r in requests)==1 for path in ['/p0/init.mp4','/p0/segment.m4s','/p1/init.mp4','/p1/segment.m4s']),requests
for r in protected:assert sha(Path(r['path']))==r['sha256']
(area/'frame-oracle.json').write_text(json.dumps(dict(passed=True,framesMatched=20,periodOrderMatched=True,manifestStages=stages,mediaBodiesDownloadedOnce=True,naturalStaticCompletion=True,publicationCount=1),indent=2),encoding='utf-8');print(json.dumps(dict(checks=len(d['checks']),frames=20,manifestStages=stages,prePublicationBytes=d['prePublicationBytes'],finalBytes=d['finalBytes'],publications=d['publications']),indent=2),flush=True)
