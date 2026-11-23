from pathlib import Path
import os,json,hashlib,subprocess,threading,time,http.server,re,datetime,struct,sys
root=Path('[repo]');base=Path(os.environ['LOCALAPPDATA'])/'UDM-Development';mode=sys.argv[1];assert mode in ['baseline','candidate'];area=root/'.media-cache/owned-retired-dash-workflow-9.82';old=root/'.media-cache/owned-retired-dash-baseline-9.81'
if mode=='baseline':area.mkdir(exist_ok=False)
else:assert (area/'baseline-reproduction.json').exists()
scope={'__file__':str((old if mode=='baseline' else area)/'config.py')};exec((base/'finishing-8.34/config.py').read_text(encoding='utf-8-sig'),scope);globals().update({k:v for k,v in scope.items() if not k.startswith('__')});common+=['/I'+str(native),'/I'+str(native/'vendor/brotli/c/include')];tools=Path('C:/Program Files/UDM/tools');env['TEMP']=str(out);env['TMP']=str(out)
link=release/'tools'
if not link.exists():
 p=subprocess.run(['powershell','-NoProfile','-Command',"New-Item -ItemType Junction -Path '"+str(link)+"' -Target 'C:/Program Files/UDM/tools' | Out-Null"],capture_output=True,text=True);assert p.returncode==0,p.stderr
if mode=='baseline':
 code=(base/'run-dash-assembly-workflow-9.76.py').read_text(encoding='utf-8').split("assets=area/'assets';assets.mkdir();expected=[]",1)[1].split('document=',1)[0];code=code.replace("f'color=c={color}:s=640x360:r=10:d=1'","('testsrc2=s=640x360:r=10:d=1'+(',hue=h=90' if n else ''))");exec("assets=area/'assets';assets.mkdir();expected=[]"+code,globals());assert len(set(expected[0]+expected[1]))==20
 inputs=json.loads((old/'inputs.json').read_text());objects=inputs['coreObjects'];resource=Path(inputs['resourcePath']);source=old/'benchmarks/speed/live_dash_restart_workflow.cpp'
else:
 assets=area/'assets';meta=json.loads((area/'server-state.json').read_text());expected=meta['expected'];build=root/'.media-cache/owned-retired-dash-build-9.82';inputs=json.loads((build/'build-result.json').read_text());assert inputs['passed'];objects=inputs['coreObjects'];resource=build/'App.res';source=root/'benchmarks/speed/live_dash_restart_workflow.cpp'
assert all(sha(Path(x['path']))==x['sha256'] for x in objects)
exe=release/('Udm.RetiredDashBaseline.exe' if mode=='baseline' else 'Udm.RetiredDashCandidate.exe')
with (area/('build-'+mode+'.log')).open('w') as log:p=subprocess.run([str(compiler),*common,str(source),*[x['path'] for x in objects],str(resource),'/Fo'+str(out/('RetiredDash'+mode+'.obj')),'/Fe'+str(exe),'/link',*system,'/SUBSYSTEM:CONSOLE','/INCREMENTAL:NO','/MANIFEST:NO'],cwd=root,env=env,stdout=log,stderr=subprocess.STDOUT)
assert p.returncode==0,(area/('build-'+mode+'.log')).read_text(errors='replace')[-4000:]
(area/('actor-'+mode+'.json')).write_text(json.dumps({'sourceSha256':sha(source),'binarySha256':sha(exe),'binaryPath':str(exe),'resourceSha256':sha(resource),'coreObjects':objects,'runnerSha256':sha(Path(__file__))},indent=2))
guard=threading.Lock();requests=[];states={};records=[]
clock=lambda:datetime.datetime.now(datetime.timezone.utc)
stamp=lambda v:v.isoformat(timespec='milliseconds').replace('+00:00','Z')
if mode=='candidate':states=meta['states'];requests=meta['requests'];records=meta['records'];states={k:{**v,'origin':datetime.datetime.fromisoformat(v['origin'])} for k,v in states.items()}
def new_case(name):
 with guard:states[name]={'stage':1,'step':'capture','origin':clock()-datetime.timedelta(seconds=4)}
def manifest(case,state):
 stage=state['stage'];dynamic=stage<3;now=clock();attrs=f'type="dynamic" availabilityStartTime="{stamp(state["origin"])}" minimumUpdatePeriod="PT0.2S" suggestedPresentationDelay="PT3600S" timeShiftBufferDepth="PT3600S"' if dynamic else 'type="static" mediaPresentationDuration="PT3S"'
 text=f'<MPD xmlns="urn:mpeg:dash:schema:mpd:2011" id="owned-retired-{case}{"-changed" if case=="changed" and stage>1 else ""}" {attrs} publishTime="{stamp(now)}">'
 if dynamic:text+=f'<UTCTiming schemeIdUri="urn:mpeg:dash:utc:direct:2014" value="{stamp(now)}"/>'
 for n in ([0] if stage==1 else [0,1] if case=='visible' else [1]):
  start=0 if n==0 else (2 if case in ['gap','incomplete'] else .5 if case=='overlap' else 1);duration=f' duration="PT{2 if case=="incomplete" else 1}S"' if n or case in ['known','incomplete'] else ''
  text+=f'<Period id="p{n}" start="PT{start}S"{duration}><AdaptationSet id="v" mimeType="video/mp4" codecs="avc1.42c01e" width="640" height="360"><Representation id="r" bandwidth="100000"><SegmentList timescale="1000" startNumber="1"><SegmentTimeline><S t="0" d="1000"/></SegmentTimeline><Initialization sourceURL="p{n}/init.mp4"/><SegmentURL media="p{n}/segment.m4s"/></SegmentList></Representation></AdaptationSet></Period>'
 return (text+'</MPD>').encode()
class Handler(http.server.BaseHTTPRequestHandler):
 protocol_version='HTTP/1.1'
 def log_message(self,*args):pass
 def do_HEAD(self):self.serve(False)
 def do_GET(self):self.serve(True)
 def serve(self,body):
  case,relative=self.path.split('?',1)[0].lstrip('/').split('/',1)
  with guard:state=states[case].copy()
  if state['stage']>=2 and relative.startswith('p0/') and case!='visible':
   with guard:requests.append({'case':case,'step':state['step'],'method':self.command,'path':relative,'status':410})
   self.send_error(410);return
  if relative=='manifest.mpd':data=manifest(case,state)
  else:
   path=(assets/relative).resolve();assert path.is_relative_to(assets.resolve());data=path.read_bytes()
  etag='"'+hashlib.sha256(data).hexdigest()+'"';first=0;last=len(data)-1;status=200;requested=self.headers.get('Range')
  if self.headers.get('If-Match',etag)!=etag:self.send_error(412);return
  if requested:
   match=re.fullmatch(r'bytes=(\d+)-(\d*)',requested);assert match;first=int(match[1]);last=min(last,int(match[2]) if match[2] else last);assert 0<=first<=last;status=206
  with guard:requests.append({'case':case,'step':state['step'],'stage':state['stage'],'method':self.command,'path':relative,'status':status,'range':requested,'bytes':last-first+1})
  self.send_response(status);self.send_header('Content-Type','application/dash+xml' if relative=='manifest.mpd' else 'video/mp4');self.send_header('Content-Length',str(last-first+1));self.send_header('ETag',etag);self.send_header('Accept-Ranges','bytes');self.send_header('Connection','close')
  if status==206:self.send_header('Content-Range',f'bytes {first}-{last}/{len(data)}')
  self.end_headers()
  if body:
   self.wfile.write(data[first:last+1])
   if relative=='p1/segment.m4s':
    with guard:states[case]['stage']=3
  self.close_connection=True
server=http.server.ThreadingHTTPServer(('127.0.0.1',0 if mode=='baseline' else meta['port']),Handler);thread=threading.Thread(target=server.serve_forever,daemon=True);thread.start();destinationRoot=base/'owned-retired-dash-boundary-9.82-C'
def actor(stage,case):
 work=area/case;url=f'http://127.0.0.1:{server.server_port}/{case}/manifest.mpd';destination=destinationRoot/case
 with guard:states[case]['step']=stage
 with (area/(case+'-'+stage+'.log')).open('w') as log:
  p=subprocess.Popen([str(exe),stage,str(work),url,str(destination)],cwd=root,env=env,stdout=log,stderr=subprocess.STDOUT);(area/'active.json').write_text(json.dumps({'pid':p.pid,'mode':mode,'case':case,'stage':stage}));code=p.wait()
 result=json.loads((work/(stage+'-results.json')).read_text());row={'case':case,'stage':stage,'exitCode':code,'binarySha256':sha(exe),'result':str(work/(stage+'-results.json'))};records.append(row);print(case+' '+stage+': '+str(code)+'; '+result['error'],flush=True);return result,code
def retained(case):
 data=json.loads((area/case/'capture-results.json').read_text());folder=Path(data['savedJob']['MediaFolder'])/'live-dash';return {p.relative_to(folder).as_posix():sha(p) for p in folder.rglob('*') if p.is_file() and p.suffix not in ['.json','.bak']}
try:
 if mode=='baseline':
  new_case('positive');captured,code=actor('capture','positive');assert code==0 and captured['passed'] and captured['retainedEnd']==1 and captured['retainedPeriodDuration']==0;before=retained('positive');assert before
  with guard:states['positive']['stage']=2
  failed,code=actor('baseline','positive');assert code==1 and not failed['passed'] and 'no longer present' in failed['savedJob']['Error'].lower() and failed['publications']==0 and retained('positive')==before
  (area/'baseline-reproduction.json').write_text(json.dumps({'reproduced':True,'version':'9.81','error':failed['savedJob']['Error'],'resourceHashes':before,'retainedEnd':captured['retainedEnd'],'retainedPeriodDuration':None,'noPublication':True},indent=2));print('Unknown-duration retired-Period failure reproduced with original bytes unchanged',flush=True)
 else:
  fixed,code=actor('candidate','positive');assert code==0 and fixed['passed'] and fixed['frames']==expected[0]+expected[1] and fixed['publications']==1
  for case in ['known','visible','revision-only','gap','overlap','incomplete','changed']:
   new_case(case);captured,code=actor('capture',case);assert code==0 and captured['passed'];before=retained(case)
   if case=='revision-only':
    revised,code=actor('revision-only',case);assert code==0 and revised['passed'] and retained(case)==before
   with guard:states[case]['stage']=2
   result,code=actor('candidate',case)
   if case in ['known','visible','revision-only']:assert code==0 and result['passed'] and result['frames']==expected[0]+expected[1] and result['publications']==1
   else:assert code==1 and not result['passed'] and result['publications']==0 and not result['savedJob']['Status']=='Complete' and retained(case)==before
  for case in states:
   assert all(sum(x['case']==case and x['method']=='GET' and x['path']==path for x in requests)==1 for path in ['p0/init.mp4','p0/segment.m4s'])
  (area/'qualification.json').write_text(json.dumps({'passed':True,'cases':list(states),'sourceDistinctFrames':20,'allPositiveFramesMatch':True,'noOldResourceRedownload':True,'rejectionsPreserveResources':True,'revisionOnlyCrashRecoveryPassed':True,'positivePrePublicationBytes':fixed['prePublicationBytes'],'positiveFinalBytes':fixed['finalBytes']},indent=2))
finally:
 server.shutdown();server.server_close();thread.join();(area/'server-state.json').write_text(json.dumps({'port':server.server_port,'states':{k:{**v,'origin':v['origin'].isoformat()} for k,v in states.items()},'requests':requests,'records':records,'expected':expected},indent=2));(area/(mode+'-terminal.json')).write_text(json.dumps({'allActorsTerminal':True,'originClosed':True,'records':records,'requests':requests},indent=2))
