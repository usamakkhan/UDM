"""Actual WinHTTP/curl request wire identity, methods, ranges and body bytes."""
from pathlib import Path
import hashlib,http.server,json,os,subprocess,sys,threading
root=Path(sys.argv[1]).resolve();exe=Path(sys.argv[2]).resolve();baseline='--baseline' in sys.argv[3:]
assert not root.exists();root.mkdir(parents=True);records=[]
class Handler(http.server.BaseHTTPRequestHandler):
    def log_message(self,*args):pass
    def do_GET(self):self.respond()
    def do_HEAD(self):self.respond()
    def do_POST(self):self.respond()
    def respond(self):
        body=self.rfile.read(int(self.headers.get('Content-Length','0')))
        records.append(dict(path=self.path,method=self.command,encoding=self.headers.get_all('Accept-Encoding'),range=self.headers.get('Range'),ifRange=self.headers.get('If-Range'),accept=self.headers.get('Accept'),contentType=self.headers.get('Content-Type'),bodyHex=body.hex()))
        self.send_response(206 if self.path.endswith('/range') else 200);self.send_header('Content-Length','5');self.send_header('ETag','"owned"')
        if self.path.endswith('/range'):self.send_header('Content-Range','bytes 0-4/5')
        self.end_headers()
        if self.command!='HEAD':self.wfile.write(b'owned')
server=http.server.ThreadingHTTPServer(('127.0.0.1',0),Handler);threading.Thread(target=server.serve_forever,daemon=True).start()
env=os.environ.copy();env.update(TEMP=str(root),TMP=str(root))
try:
    port=server.server_port
    p=subprocess.run([str(exe),str(root/'actor'),f'http://127.0.0.1:{port}/winhttp',f'http://[::ffff:127.0.0.1]:{port}/curl'],capture_output=True,timeout=45,env=env)
    (root/'actor.log').write_bytes(p.stdout+p.stderr);result=json.loads((root/'actor/results.json').read_text(encoding='utf-8-sig'))
    assert p.returncode==0 and result['passed'],result
    assert len(records)==16 and len(result['requests'])==16
    for row in records:
        case=row['path'].split('/')[-1];tokens=[p.strip() for value in row['encoding'] for p in value.split(',')]
        if baseline:
            expected=['identity'] if case=='default' else ['identity','identity','identity'] if case=='mixed-case' else ['identity','identity','gzip'] if case=='conflicting' else ['identity','identity']
            assert tokens==expected,row
        else:assert row['encoding']==['identity'],row
        assert row['method']==('POST' if case=='post' else 'HEAD' if case=='head' else 'GET')
        assert row['bodyHex']==('000102ff' if case=='post' else '')
        assert row['range']==('bytes=0-4' if case=='range' else None)
        assert row['ifRange']==('"owned"' if case=='range' else None)
        if case=='post':assert row['contentType']=='application/x-protobuf' and row['accept']=='application/vnd.yt-ump'
    (root/'qualification.json').write_text(json.dumps(dict(passed=True,baseline=baseline,wireRequests=len(records),actorChecks=len(result['checks']),canonicalIdentity=not baseline,backends=['winhttp','curl-direct-mapped-ipv6'],actorSha256=hashlib.sha256(exe.read_bytes()).hexdigest(),recipeSha256=hashlib.sha256(Path(__file__).read_bytes()).hexdigest(),installed=False),indent=2),encoding='utf-8')
finally:
    server.shutdown();server.server_close();(root/'requests.json').write_text(json.dumps(records,indent=2),encoding='utf-8')
