"""Frozen/current recovery of an initialization alias pinned by another track."""
from pathlib import Path
import hashlib,http.server,json,os,shutil,subprocess,sys,threading,time
root=Path(sys.argv[1]).resolve();current,control=map(lambda p:Path(p).resolve(),sys.argv[2:4])
assert not root.exists();root.mkdir(parents=True)
payload=bytes(range(256))*16;requests=[];shared_gets=0;hold=True
entered=threading.Event();release=threading.Event();finished=threading.Event()
def manifest():
    return b'<MPD xmlns="urn:mpeg:dash:schema:mpd:2011" id="owned-init-alias" type="static" mediaPresentationDuration="PT1S"><Period id="p" duration="PT1S"><AdaptationSet id="v" mimeType="video/mp4" codecs="avc1.42c01e" width="160" height="90"><Representation id="v1" bandwidth="100000"><SegmentList timescale="1000" duration="1000"><SegmentURL media="shared.bin"/></SegmentList></Representation></AdaptationSet><AdaptationSet id="a" mimeType="audio/mp4" codecs="mp4a.40.2" audioSamplingRate="48000"><Representation id="a1" bandwidth="64000"><SegmentList timescale="1000" duration="1000"><Initialization sourceURL="shared.bin"/><SegmentURL media="audio.bin"/></SegmentList></Representation></AdaptationSet></Period></MPD>'
class Handler(http.server.BaseHTTPRequestHandler):
    def log_message(self,*args):pass
    def do_GET(self):
        global shared_gets
        row=dict(path=self.path,ifMatch=self.headers.get('If-Match'),range=self.headers.get('Range'));requests.append(row)
        if self.path=='/manifest.mpd':
            data=manifest();self.send_response(200);self.send_header('Content-Length',str(len(data)));self.end_headers();self.wfile.write(data);return
        assert self.path in ['/shared.bin','/audio.bin']
        held=False
        if self.path=='/shared.bin':
            shared_gets+=1
            if hold and shared_gets==2:held=True;entered.set();assert release.wait(20)
        try:
            self.send_response(200);self.send_header('Content-Length',str(len(payload)));self.send_header('Content-Type','video/mp4');self.send_header('ETag','"original"');self.end_headers();self.wfile.write(payload)
        except (ConnectionError,OSError):pass
        finally:
            if held:finished.set()
server=http.server.ThreadingHTTPServer(('127.0.0.1',0),Handler)
threading.Thread(target=server.serve_forever,daemon=True).start()
url=f'http://127.0.0.1:{server.server_port}/manifest.mpd';env=os.environ.copy();env.update(TEMP=str(root),TMP=str(root))
def invoke(actor,folder,mode,label):
    p=subprocess.run([str(actor),str(folder),url,mode],capture_output=True,timeout=30,env=env)
    (root/(label+'.log')).write_bytes(p.stdout+p.stderr)
    r=json.loads((folder/(mode+'-results.json')).read_text(encoding='utf-8-sig'));(root/(label+'.json')).write_text(json.dumps(dict(exitCode=p.returncode,result=r),indent=2),encoding='utf-8');return p.returncode,r
baseline=root/'baseline';candidate=root/'candidate';process=None
try:
    with (root/'interrupted-control.log').open('wb') as log:
        process=subprocess.Popen([str(control),str(baseline),url,'capture'],stdout=log,stderr=subprocess.STDOUT,env=env)
        assert entered.wait(15),'Initialization alias not reached'
        assert list(baseline.glob('*.receipt.json')),'First track has no committed receipt'
        assert process.poll() is None;process.kill();exit_code=process.wait(timeout=10)
        release.set();assert finished.wait(10);hold=False
    code,recovered=invoke(control,baseline,'reconcile','control-reconcile');assert code==0 and recovered['passed']
    tracks=recovered['index']['tracks'];assert len(tracks)==2 and tracks[0]['segments']['1']['committed']
    assert tracks[1]['pendingInitializationRequiresVersion'] and tracks[1].get('pendingInitializationVersion') is None
    accepted=baseline/tracks[0]['segments']['1']['resource']['file'];accepted_hash=hashlib.sha256(accepted.read_bytes()).hexdigest();assert accepted.read_bytes()==payload
    shutil.copytree(baseline,candidate)
    before=len(requests);code,old=invoke(control,baseline,'capture','control-retry')
    assert code==1 and 'pending initialization version changed' in old['error'] and not any(r['path']!='/manifest.mpd' for r in requests[before:]),old
    before=len(requests);code,new=invoke(current,candidate,'capture','candidate-retry');assert code==0 and new['passed'],new
    wire=requests[before:];media=[r for r in wire if r['path']!='/manifest.mpd']
    assert [(r['path'],r['ifMatch']) for r in media]==[('/shared.bin','"original"'),('/audio.bin',None)],media
    assert new['accountedBytes']==2*len(payload)
    physical=[]
    for track in new['index']['tracks']:
        for resource in [r['resource'] for r in track['segments'].values()]+([track['initialization']] if 'initialization' in track else []):
            path=candidate/resource['file'];assert path.read_bytes()==payload and hashlib.sha256(path.read_bytes()).hexdigest()==resource['sha256']
            physical.append(dict(bytes=path.stat().st_size,sha256=resource['sha256']))
    assert len(physical)==3 and hashlib.sha256(accepted.read_bytes()).hexdigest()==accepted_hash
    result=dict(passed=True,frozenControlVersion='10.6',controlError=old['error'],acceptedTrackPreserved=True,pendingInitializationAdoptedPinnedAlias=True,clientBodyBytes=new['accountedBytes'],mediaRequests=media,physicalResources=physical,injectedExitCode=exit_code,currentActorSha256=hashlib.sha256(current.read_bytes()).hexdigest(),controlActorSha256=hashlib.sha256(control.read_bytes()).hexdigest(),recipeSha256=hashlib.sha256(Path(__file__).read_bytes()).hexdigest(),installed=False)
    (root/'qualification.json').write_text(json.dumps(result,indent=2),encoding='utf-8');print(json.dumps(result))
finally:
    release.set()
    if process is not None and process.poll() is None:process.kill();process.wait(timeout=10)
    server.shutdown();server.server_close();(root/'requests.json').write_text(json.dumps(requests,indent=2),encoding='utf-8')
