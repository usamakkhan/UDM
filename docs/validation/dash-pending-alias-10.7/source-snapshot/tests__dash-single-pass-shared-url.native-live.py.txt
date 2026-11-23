"""Diagnose pinned/unbound SegmentList aliases using existing native binaries."""
from pathlib import Path
import hashlib,http.server,json,os,subprocess,sys,threading
root=Path(sys.argv[1]).resolve();exe=Path(sys.argv[2]).resolve()
assert not root.exists();root.mkdir(parents=True)
payload=bytes(range(256))*16;requests=[];body_gets=0;restored=False
class Handler(http.server.BaseHTTPRequestHandler):
    def log_message(self,*args):pass
    def do_GET(self):
        global body_gets
        row=dict(path=self.path,ifMatch=self.headers.get('If-Match'));requests.append(row)
        if self.path=='/manifest.mpd':
            data=b'<MPD xmlns="urn:mpeg:dash:schema:mpd:2011" id="owned-alias" type="static" mediaPresentationDuration="PT1S"><Period id="p" duration="PT1S"><AdaptationSet id="v" mimeType="video/mp4" codecs="avc1.42c01e" width="160" height="90"><Representation id="r" bandwidth="100000"><SegmentList timescale="1000" duration="500"><SegmentURL media="shared.bin"/><SegmentURL media="shared.bin"/></SegmentList></Representation></AdaptationSet></Period></MPD>'
            tag=None
        else:
            assert self.path=='/shared.bin';body_gets+=1;tag='"original"' if restored or body_gets==1 else '"changed"';data=payload
        if self.headers.get('If-Match') and tag!=self.headers['If-Match']:
            self.send_response(412);self.send_header('Content-Length','0');self.end_headers();return
        self.send_response(200);self.send_header('Content-Length',str(len(data)))
        if tag:self.send_header('ETag',tag)
        self.send_header('Content-Type','video/mp4' if tag else 'application/dash+xml')
        self.end_headers();self.wfile.write(data)
server=http.server.ThreadingHTTPServer(('127.0.0.1',0),Handler)
threading.Thread(target=server.serve_forever,daemon=True).start()
url=f'http://127.0.0.1:{server.server_port}/manifest.mpd';folder=root/'actor'
env=os.environ.copy();env.update(TEMP=str(root),TMP=str(root))
def invoke(mode,label):
    p=subprocess.run([str(exe),str(folder),url,mode],capture_output=True,timeout=30,env=env)
    (root/(label+'.log')).write_bytes(p.stdout+p.stderr)
    r=json.loads((folder/(mode+'-results.json')).read_text(encoding='utf-8-sig'))
    (root/(label+'.json')).write_text(json.dumps(dict(exitCode=p.returncode,result=r),indent=2),encoding='utf-8')
    return p.returncode,r
try:
    code,initial=invoke('capture','conflict')
    assert code==1 and not initial['passed']
    code,recovered=invoke('reconcile','after-conflict');assert code==0 and recovered['passed']
    records=recovered['index']['tracks'][0]['segments'];assert records['1']['committed'] and not records['2']['committed'] and records['2'].get('resourceVersion') is None
    accepted=folder/records['1']['resource']['file'];accepted_hash=hashlib.sha256(accepted.read_bytes()).hexdigest();assert accepted.read_bytes()==payload
    restored=True;before=body_gets;code,retry=invoke('capture','restored-retry')
    defect=code==1 and 'refresh replaced an indexed segment' in retry['error'] and body_gets==before
    code,final=invoke('reconcile','after-retry');assert code==0 and final['passed']
    assert hashlib.sha256(accepted.read_bytes()).hexdigest()==accepted_hash
    result=dict(expectedBehavior='Resume the unbound pending alias using the already pinned original version, preserving accepted bytes',initialConflictRejected=True,acceptedFirstResourcePreserved=True,restoredRetryPassed=retry['passed'],knownRecoveryDefectReproduced=defect,retryError=retry['error'],bodyGetsDuringRetry=body_gets-before,actorSha256=hashlib.sha256(exe.read_bytes()).hexdigest(),recipeSha256=hashlib.sha256(Path(__file__).read_bytes()).hexdigest(),releaseQualified=False)
    assert defect or retry['passed'],result
    (root/'diagnosis.json').write_text(json.dumps(result,indent=2),encoding='utf-8')
    print(json.dumps(result))
finally:
    server.shutdown();server.server_close();(root/'requests.json').write_text(json.dumps(requests,indent=2),encoding='utf-8')
