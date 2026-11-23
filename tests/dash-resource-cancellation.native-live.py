"""Cancel all three resource requests while their response headers remain held."""
from pathlib import Path
import hashlib,http.server,json,os,subprocess,sys,threading,time
root=Path(sys.argv[1]).resolve();exe=Path(sys.argv[2]).resolve()
assert not root.exists();root.mkdir(parents=True);held=threading.Event();release=threading.Event();requests=[];lock=threading.Lock();active=0;peak=0
class Handler(http.server.BaseHTTPRequestHandler):
    def log_message(self,*args):pass
    def do_GET(self):
        global active,peak
        with lock:requests.append(self.path)
        if self.path=='/manifest.mpd':
            tracks=''.join(f'<AdaptationSet id="{n}" mimeType="video/mp4" codecs="avc1.42c01e" width="160" height="90"><Representation id="r" bandwidth="100000"><SegmentList timescale="1000" duration="1000"><SegmentURL media="s{n}.bin"/></SegmentList></Representation></AdaptationSet>' for n in range(3))
            data=('<MPD xmlns="urn:mpeg:dash:schema:mpd:2011" id="owned-held-body" type="static" mediaPresentationDuration="PT1S"><Period id="p" duration="PT1S">'+tracks+'</Period></MPD>').encode();self.send_response(200);self.send_header('Content-Length',str(len(data)));self.end_headers();self.wfile.write(data);return
        with lock:
            active+=1;peak=max(peak,active)
            if active==3:held.set()
        try:
            assert release.wait(15),'Held response release timeout'
            self.send_response(200);self.send_header('Content-Length','4');self.send_header('ETag','"owned"');self.end_headers();self.wfile.write(b'data')
        except (ConnectionError,OSError):pass
        finally:
            with lock:active-=1
server=http.server.ThreadingHTTPServer(('127.0.0.1',0),Handler);threading.Thread(target=server.serve_forever,daemon=True).start()
env=os.environ.copy();env.update(TEMP=str(root),TMP=str(root));process=None
try:
    process=subprocess.Popen([str(exe),str(root/'actor'),f'http://127.0.0.1:{server.server_port}/manifest.mpd'],stdout=subprocess.PIPE,stderr=subprocess.STDOUT,env=env)
    assert held.wait(12),'All three requests did not reach their origin'
    assert process.poll() is None and not release.is_set()
    (root/'actor/cancel.signal').write_text('cancel owned capture',encoding='utf-8')
    output,_=process.communicate(timeout=5);(root/'actor.log').write_bytes(output)
    result=json.loads((root/'actor/results.json').read_text(encoding='utf-8-sig'));assert process.returncode==0 and result['passed'] and result['transportReleased'],result
    assert not release.is_set() and peak==3 and len(requests)==4
    (root/'qualification.json').write_text(json.dumps(dict(passed=True,result=result,headersStillHeldWhenActorExited=True,peakActiveRequests=peak,actorSha256=hashlib.sha256(exe.read_bytes()).hexdigest(),recipeSha256=hashlib.sha256(Path(__file__).read_bytes()).hexdigest(),installed=False),indent=2),encoding='utf-8')
finally:
    release.set()
    if process is not None and process.poll() is None:process.kill();process.communicate(timeout=10)
    server.shutdown();server.server_close();(root/'requests.json').write_text(json.dumps(requests,indent=2),encoding='utf-8')
