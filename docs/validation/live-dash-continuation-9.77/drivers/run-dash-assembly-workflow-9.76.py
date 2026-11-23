from pathlib import Path
import os,json,subprocess,threading,time,http.server,functools,struct
base=Path('<development>');root=Path('<repo>');area=root/'.media-cache/owned-dash-assembly-workflow-9.76';assert not area.exists();area.mkdir()
scope={'__file__':str(area/'config.py')};exec((base/'finishing-8.34/config.py').read_text(encoding='utf-8-sig'),scope);globals().update({k:v for k,v in scope.items() if not k.startswith('__')});common+=['/I'+str(native),'/I'+str(native/'vendor/brotli/c/include')]
build=root/'.media-cache/owned-dash-output-build-9.76';b=json.loads((build/'build-result.json').read_text())
for r in b['sources']:assert sha(root/r['path'])==r['sha256']
for r in b['coreObjects']:assert sha(Path(r['path']))==r['sha256']
protected=json.loads((base/'publication-overlap-9.61-protected-before.json').read_text())
for r in protected:assert sha(Path(r['path']))==r['sha256']
tools=Path('C:/Program Files/UDM/tools');link=release/'tools';p=subprocess.run(['powershell','-NoProfile','-Command',"New-Item -ItemType Junction -Path '"+str(link)+"' -Target 'C:/Program Files/UDM/tools' | Out-Null"],capture_output=True,text=True);assert p.returncode==0,p.stderr
assets=area/'assets';assets.mkdir();expected=[]
def frames(path):
 text=subprocess.check_output([str(tools/'ffmpeg.exe'),'-hide_banner','-loglevel','error','-xerror','-nostdin','-i',str(path),'-map','0:v:0','-f','framemd5','-'],text=True);return [line.rsplit(',',1)[1].strip() for line in text.splitlines() if line and not line.startswith('#')]
for n,color in enumerate(['blue','red']):
 folder=assets/('p'+str(n));folder.mkdir();movie=folder/'original.mp4';subprocess.run([str(tools/'ffmpeg.exe'),'-hide_banner','-loglevel','error','-nostdin','-y','-f','lavfi','-i',f'color=c={color}:s=640x360:r=10:d=1','-c:v','libx264','-preset','ultrafast','-profile:v','baseline','-level','3.0','-bf','0','-g','10','-pix_fmt','yuv420p','-video_track_timescale','1000','-movflags','empty_moov+frag_keyframe+default_base_moof',str(movie)],check=True)
 blob=movie.read_bytes();boxes=[];at=0
 while at<len(blob):
  length,kind=struct.unpack('>I4s',blob[at:at+8]);assert length>=8 and at+length<=len(blob);boxes.append((kind,blob[at:at+length]));at+=length
 assert [k for k,_ in boxes].count(b'moof')==1
 (folder/'init.mp4').write_bytes(b''.join(v for k,v in boxes if k in [b'ftyp',b'moov']));(folder/'segment.m4s').write_bytes(b''.join(v for k,v in boxes if k in [b'moof',b'mdat']));oracle=frames(movie);assert len(oracle)==10;expected.append(oracle)
document='<MPD xmlns="urn:mpeg:dash:schema:mpd:2011" type="static" id="owned-two-periods" mediaPresentationDuration="PT2S">'
for n in range(2):document+=f'<Period id="p{n}" start="PT{n}S" duration="PT1S"><AdaptationSet id="v" mimeType="video/mp4" codecs="avc1.42c01e" width="640" height="360"><Representation id="r" bandwidth="100000"><SegmentList timescale="1000" duration="1000" startNumber="1"><Initialization sourceURL="p{n}/init.mp4"/><SegmentURL media="p{n}/segment.m4s"/></SegmentList></Representation></AdaptationSet></Period>'
document+='</MPD>';(assets/'manifest.mpd').write_text(document,encoding='utf-8');source=root/'benchmarks/speed/dash_assembly_workflow.cpp';exe=release/'Udm.DashAssemblyWorkflow.exe'
inputs=dict(version='9.76',fixtureSha256=sha(source),runnerSha256=sha(Path(__file__)),sources=b['sources'],objects=b['coreObjects'],sharedHelpers=[r for r in protected if Path(r['path']).name in ['ffmpeg.exe','ffprobe.exe']],expectedFrames=dict(single=expected[0],joined=expected[0]+expected[1]),assetHashes=[dict(path=p.relative_to(assets).as_posix(),sha256=sha(p),bytes=p.stat().st_size) for p in assets.rglob('*') if p.is_file()]);(area/'inputs.json').write_text(json.dumps(inputs,indent=2),encoding='utf-8')
with (area/'build.log').open('w') as log:p=subprocess.run([str(compiler),*common,str(source),*[r['path'] for r in b['coreObjects']],str(build/'App.res'),'/Fo'+str(out/'DashAssemblyWorkflow.obj'),'/Fe'+str(exe),'/link',*system,'/SUBSYSTEM:CONSOLE','/INCREMENTAL:NO','/MANIFEST:NO'],cwd=root,env=env,stdout=log,stderr=subprocess.STDOUT)
assert p.returncode==0,(area/'build.log').read_text()[-4000:]
requests=[]
class Handler(http.server.SimpleHTTPRequestHandler):
 def do_GET(self):requests.append(self.path);return super().do_GET()
 def log_message(self,*args):pass
 def guess_type(self,path):return 'application/dash+xml' if path.endswith('.mpd') else super().guess_type(path)
server=http.server.ThreadingHTTPServer(('127.0.0.1',0),functools.partial(Handler,directory=str(assets)));thread=threading.Thread(target=server.serve_forever,daemon=True);thread.start();work=area/'run';destination=base/'owned-dash-assembly-workflow-9.76-C';assert not destination.exists();stopped=False;url=f'http://127.0.0.1:{server.server_port}/manifest.mpd'
with (area/'runtime.log').open('w') as log:
 p=subprocess.Popen([str(exe),str(work),url,str(destination)],cwd=root,env=env,stdout=log,stderr=subprocess.STDOUT);(area/'process.json').write_text(json.dumps(dict(pid=p.pid,binarySha256=sha(exe),work=str(work),destination=str(destination))),encoding='utf-8')
 while p.poll() is None:
  if not stopped and (work/'capture-complete.json').exists():server.shutdown();server.server_close();thread.join();stopped=True;(work/'offline.ready').write_text('Origin stopped before assembly');print('Owned origin stopped after real segment capture; assembly now offline',flush=True)
  time.sleep(.05)
 code=p.wait()
if not stopped:server.shutdown();server.server_close();thread.join()
(area/'requests.json').write_text(json.dumps(dict(requests=requests,stoppedBeforeAssembly=stopped),indent=2),encoding='utf-8');d=json.loads((work/'results.json').read_text());(area/'terminal.json').write_text(json.dumps(dict(passed=code==0 and d['passed'],exitCode=code,binarySha256=sha(exe),result=str(work/'results.json'),checks=len(d['checks']),destination=str(destination),originStoppedBeforeAssembly=stopped),indent=2),encoding='utf-8');assert code==0 and d['passed'],d
assert stopped and len(d['outputs'])==2
for output in d['outputs']:assert output['frames']==inputs['expectedFrames'][output['case']],output
for r in protected:assert sha(Path(r['path']))==r['sha256']
(area/'frame-oracle.json').write_text(json.dumps(dict(passed=True,independentSourceFrames=20,finalDecodedFrames=30,singleAllFramesMatch=True,joinedBothPeriodsInOrder=True,prePublicationBytesEqualFinalBytes=all(o['prePublicationBytes']==o['finalBytes'] for o in d['outputs']),originStoppedBeforeAssembly=True),indent=2),encoding='utf-8');print(json.dumps(dict(checks=len(d['checks']),outputs=[dict(case=o['case'],bytes=o['finalBytes'],frames=len(o['frames'])) for o in d['outputs']],frameOraclePassed=True),indent=2),flush=True)
