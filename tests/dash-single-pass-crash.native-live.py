"""Abruptly terminate only owned test actors at observed durable capture states.

Uses existing 10.6 binaries. No installation, elevation, cleanup or large files.
"""
from pathlib import Path
import hashlib,http.server,json,os,subprocess,sys,threading,time

root=Path(sys.argv[1]).resolve();exe=Path(sys.argv[2]).resolve()
assert not root.exists();root.mkdir(parents=True)
payload=bytes(range(256))*512
sha=lambda p:hashlib.sha256(p.read_bytes()).hexdigest()
actor_pin=sha(exe);recipe_pin=sha(Path(__file__))
states={};requests=[];cases=[]
class State:
    def __init__(self):
        self.entered=threading.Event();self.headers=threading.Event()
        self.first=threading.Event();self.tail=threading.Event()
        self.finished=threading.Event();self.hold=True;self.changed=False

class Handler(http.server.BaseHTTPRequestHandler):
    def log_message(self,*args):pass
    def do_HEAD(self):
        requests.append(dict(method='HEAD',path=self.path))
        self.send_response(501);self.send_header('Content-Length','0');self.end_headers()
    def do_GET(self):
        case,leaf=self.path.strip('/').split('/');state=states[case]
        row=dict(method='GET',path=self.path,ifMatch=self.headers.get('If-Match'),range=self.headers.get('Range'),bodyBytesWritten=0)
        requests.append(row)
        if leaf=='manifest.mpd':
            data=b'<MPD xmlns="urn:mpeg:dash:schema:mpd:2011" id="owned-crash" type="static" mediaPresentationDuration="PT1S"><Period id="p" duration="PT1S"><AdaptationSet id="v" mimeType="video/mp4" codecs="avc1.42c01e" width="160" height="90"><Representation id="r" bandwidth="100000"><SegmentList timescale="1000" duration="1000"><SegmentURL media="segment.bin"/></SegmentList></Representation></AdaptationSet></Period></MPD>'
            self.send_response(200);self.send_header('Content-Length',str(len(data)));self.end_headers();self.wfile.write(data);row['bodyBytesWritten']=len(data);return
        assert leaf=='segment.bin'
        held=state.hold
        try:
            if held:
                state.entered.set();assert state.headers.wait(20),'Header release timeout'
            tag='"changed"' if state.changed else '"original"'
            if self.headers.get('If-Match') and self.headers['If-Match']!=tag:
                self.send_response(412);self.send_header('Content-Length','0');self.end_headers();return
            self.send_response(200);self.send_header('Content-Length',str(len(payload)))
            self.send_header('Content-Type','video/mp4');self.send_header('ETag',tag)
            self.send_header('Connection','close');self.end_headers();self.wfile.flush()
            row['headersSent']=True
            if held:assert state.first.wait(20),'First-body release timeout'
            self.wfile.write(payload[:65536]);self.wfile.flush();row['bodyBytesWritten']+=65536
            if held:assert state.tail.wait(20),'Tail-body release timeout'
            self.wfile.write(payload[65536:]);self.wfile.flush();row['bodyBytesWritten']+=len(payload)-65536
        except (ConnectionError,OSError):row['peerClosed']=True
        finally:
            if held:state.finished.set()
            self.close_connection=True

server=http.server.ThreadingHTTPServer(('127.0.0.1',0),Handler)
threading.Thread(target=server.serve_forever,daemon=True).start()
base=f'http://127.0.0.1:{server.server_port}'
env=os.environ.copy();env.update(TEMP=str(root),TMP=str(root))
def wait_for(predicate,label):
    until=time.monotonic()+12
    while time.monotonic()<until:
        try:
            if predicate():return
        except (FileNotFoundError,PermissionError):
            # The observed checkpoint is atomically replaced with exclusive
            # file ownership; a transient sharing race is not a terminal state.
            pass
        time.sleep(.01)
    raise AssertionError(label)
def invoke(case,mode,expected=True):
    folder=root/case
    p=subprocess.run([str(exe),str(folder),base+'/'+case+'/manifest.mpd',mode],capture_output=True,timeout=30,env=env)
    (root/(case+'-'+mode+'-'+str(len(requests))+'.log')).write_bytes(p.stdout+p.stderr)
    r=json.loads((folder/(mode+'-results.json')).read_text(encoding='utf-8-sig'))
    assert r['passed']==expected and p.returncode==(0 if expected else 1),r
    return r
try:
    for case in ['before-headers','after-admission','mid-body']:
        state=states[case]=State();folder=root/case
        process=None
        with (root/(case+'-killed.log')).open('wb') as log:
            try:
                process=subprocess.Popen([str(exe),str(folder),base+'/'+case+'/manifest.mpd','capture'],stdout=log,stderr=subprocess.STDOUT,env=env)
                assert state.entered.wait(15),'Resource GET not reached'
                checkpoint=folder/'recording.json';assert checkpoint.exists()
                pending_hash=sha(checkpoint)
                if case!='before-headers':
                    state.headers.set()
                    wait_for(lambda:checkpoint.exists() and sha(checkpoint)!=pending_hash,'Version admission was not checkpointed before body release')
                    wait_for(lambda:bool(list(folder.glob('*.part.tmp'))),'Body writer was not opened after admission')
                if case=='mid-body':
                    state.first.set()
                    wait_for(lambda:any(p.stat().st_size>0 for p in folder.glob('*.part.tmp')),'No partial body persisted before the injected stop')
                temporary=[dict(name=p.name,bytes=p.stat().st_size) for p in folder.glob('*.part.tmp')]
                assert not list(folder.glob('*.receipt.json'))
                assert process.poll() is None,'Actor exited before the injected stop'
                pid=process.pid;process.kill();exit_code=process.wait(timeout=10)
                state.headers.set();state.first.set();state.tail.set()
                assert state.finished.wait(10)
                state.hold=False
                recovered=invoke(case,'reconcile');record=recovered['index']['tracks'][0]['segments']['1']
                assert not record['committed'] and 'resource' not in record
                version=record.get('resourceVersion')
                if case=='before-headers':assert version is None
                else:
                    assert version['etag']=='"original"' and version['total']==len(payload)
                    state.changed=True;rejected=invoke(case,'capture',False)
                    again=invoke(case,'reconcile')['index']['tracks'][0]['segments']['1']
                    assert again['resourceVersion']==version and not again['committed']
                    assert not list(folder.glob('*.receipt.json'))
                    state.changed=False
                before=len(requests);completed=invoke(case,'capture');record=completed['index']['tracks'][0]['segments']['1'];resource=record['resource']
                final=folder/resource['file'];assert record['committed'] and final.read_bytes()==payload
                assert sha(final)==resource['sha256'] and resource['bytes']==len(payload)
                assert completed['accountedBytes']==len(payload)
                wire=requests[before:];media=[r for r in wire if r['path'].endswith('segment.bin')]
                assert len(media)==1 and media[0]['range'] is None and media[0]['bodyBytesWritten']==len(payload)
                assert media[0]['ifMatch']==(None if case=='before-headers' else '"original"')
                cases.append(dict(case=case,passed=True,ownedProcessId=pid,injectedExitCode=exit_code,partialFilesAtStop=temporary,versionPinnedBeforeStop=version is not None,recoveredUncommitted=True,changedVersionRejected=case!='before-headers',finalBytes=resource['bytes'],finalSha256=resource['sha256'],clientAccountedBytes=completed['accountedBytes'],serverFinalBodyBytesWritten=media[0]['bodyBytesWritten'],successfulResumeMediaRequests=len(media)))
                (root/'cases.json').write_text(json.dumps(cases,indent=2),encoding='utf-8')
            finally:
                state.headers.set();state.first.set();state.tail.set()
                if process is not None and process.poll() is None:process.kill();process.wait(timeout=10)
    assert sha(exe)==actor_pin and sha(Path(__file__))==recipe_pin
    (root/'qualification.json').write_text(json.dumps(dict(passed=True,cases=cases,actorSha256=actor_pin,recipeSha256=recipe_pin,processCrashTest=True,powerLossTest=False,receiptCommitTransitionsCovered=False,installed=False),indent=2),encoding='utf-8')
finally:
    for state in states.values():state.headers.set();state.first.set();state.tail.set()
    server.shutdown();server.server_close()
    (root/'requests.json').write_text(json.dumps(requests,indent=2),encoding='utf-8')
