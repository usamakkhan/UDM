"""Cancel an actual held metadata response before releasing its headers."""
from pathlib import Path
import hashlib,http.server,json,subprocess,sys,threading
root=Path(sys.argv[1]).resolve()
assert not root.exists()
root.mkdir(parents=True)
exe=Path(sys.argv[2]).resolve()
release=threading.Event()
entered=threading.Event()
class Handler(http.server.BaseHTTPRequestHandler):
    def log_message(self,*args):pass
    def do_GET(self):
        assert self.path=='/held.mpd'
        (root/'actor/hold-entered').write_text('Owned response intentionally held',encoding='utf-8')
        entered.set()
        release.wait(12)
        try:
            self.send_response(200)
            self.send_header('Content-Length','0')
            self.end_headers()
        except (BrokenPipeError,ConnectionResetError):pass
server=http.server.ThreadingHTTPServer(('127.0.0.1',0),Handler)
threading.Thread(target=server.serve_forever,daemon=True).start()
recipe=hashlib.sha256(Path(__file__).read_bytes()).hexdigest()
try:
    p=subprocess.run([str(exe),str(root/'actor'),f'http://127.0.0.1:{server.server_port}/held.mpd'],capture_output=True,timeout=9)
    (root/'actor.log').write_bytes(p.stdout+p.stderr)
    result=json.loads((root/'actor/results.json').read_text(encoding='utf-8-sig'))
    assert p.returncode==0 and result['passed'] and entered.is_set() and not release.is_set(),result
    assert hashlib.sha256(Path(__file__).read_bytes()).hexdigest()==recipe
    (root/'qualification.json').write_text(json.dumps(dict(passed=True,headersStillHeldWhenActorExited=True,result=result,recipeSha256=recipe,actorSha256=hashlib.sha256(exe.read_bytes()).hexdigest()),indent=2),encoding='utf-8')
finally:
    release.set()
    server.shutdown()
