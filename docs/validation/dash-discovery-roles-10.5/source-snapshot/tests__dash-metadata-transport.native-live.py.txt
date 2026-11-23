"""Real HTTP cookie/authorization/clock isolation for recording-owned metadata pools."""
from pathlib import Path
import datetime,hashlib,http.server,json,subprocess,sys,threading
root=Path(sys.argv[1]).resolve()
assert not root.exists()
root.mkdir(parents=True)
exe=Path(sys.argv[2]).resolve()
records=[]
class Handler(http.server.BaseHTTPRequestHandler):
    def log_message(self,*args):pass
    def note(self):
        records.append(dict(origin=self.server.label,path=self.path,method=self.command,range=self.headers.get('Range'),authorization=self.headers.get('Authorization'),cookie=self.headers.get('Cookie')))
    def do_HEAD(self):
        assert self.path=='/resource.bin'
        self.note()
        self.send_response(501)
        self.send_header('Content-Length','0')
        self.end_headers()
    def do_GET(self):
        label=self.server.label
        self.note()
        if self.path=='/resource.bin':
            assert self.headers.get('Range')=='bytes=0-0'
            self.send_response(206)
            self.send_header('Content-Range','bytes 0-0/8')
            self.send_header('ETag','"owned-resource"')
            data=b'x'
        elif self.path=='/redirect':
            self.send_response(302)
            self.send_header('Location',b+'/manifest.mpd')
            data=b''
        elif self.path=='/clock':
            self.send_response(200)
            data=datetime.datetime.now(datetime.timezone.utc).isoformat(timespec='milliseconds').replace('+00:00','Z').encode()
        else:
            assert self.path=='/manifest.mpd'
            self.send_response(200)
            if label=='a':self.send_header('Set-Cookie','owned=rotated; Path=/')
            data=b'<MPD xmlns="urn:mpeg:dash:schema:mpd:2011" id="owned-wire" type="static" mediaPresentationDuration="PT2S"><UTCTiming schemeIdUri="urn:mpeg:dash:utc:http-xsdate:2014" value="/clock"/><Period id="p0" duration="PT2S"><AdaptationSet id="v" mimeType="video/mp4" codecs="avc1.42c01e" width="160" height="90"><Representation id="r" bandwidth="100000"><SegmentTemplate timescale="1000" duration="1000" initialization="init.mp4" media="s-$Number$.m4s"/></Representation></AdaptationSet></Period></MPD>'
        self.send_header('Content-Length',str(len(data)))
        self.send_header('Connection','close')
        self.end_headers()
        if data:self.wfile.write(data)
        self.close_connection=True
servers=[]
for label in ['a','b']:
    server=http.server.ThreadingHTTPServer(('127.0.0.1',0),Handler)
    server.label=label
    servers.append(server)
    threading.Thread(target=server.serve_forever,daemon=True).start()
a,b=[f'http://127.0.0.1:{s.server_port}' for s in servers]
recipe=hashlib.sha256(Path(__file__).read_bytes()).hexdigest()
try:
    p=subprocess.run([str(exe),str(root/'actor'),a,b],capture_output=True,timeout=25)
    (root/'actor.log').write_bytes(p.stdout+p.stderr)
    result=json.loads((root/'actor/results.json').read_text(encoding='utf-8-sig'))
    assert p.returncode==0 and result['passed'],result
    own=[r for r in records if r['origin']=='a' and r['path']=='/manifest.mpd']
    assert len(own)==5 and [r['authorization'] for r in own]==['Bearer owned-token','Bearer owned-token','Bearer changed-token',None,None],own
    assert [r['cookie'] or None for r in own]==[None,'owned=rotated','owned=rotated','owned=rotated',None],own
    foreign=[r for r in records if r['origin']=='b']
    clock=[r for r in records if r['path']=='/clock']
    assert len(foreign)==2 and len(clock)==2
    assert all(not r['cookie'] and r['authorization'] is None for r in foreign+clock),records
    resources=[r for r in records if r['path']=='/resource.bin']
    assert len(resources)==4 and [r['method'] for r in resources]==['HEAD','GET','HEAD','GET'],resources
    assert all(r['authorization']=='Bearer resource-token' and not r['cookie'] for r in resources),resources
    assert hashlib.sha256(Path(__file__).read_bytes()).hexdigest()==recipe
    (root/'qualification.json').write_text(json.dumps(dict(passed=True,actorChecks=result['checks'],requestCount=len(records),rotatedCookieReused=True,nextRecordingCookieIsolated=True,authorizationIsRequestLocal=True,clockAndRedirectCredentialsIsolated=True,recipeSha256=recipe,actorSha256=hashlib.sha256(exe.read_bytes()).hexdigest()),indent=2),encoding='utf-8')
finally:
    for server in servers:server.shutdown()
    (root/'requests.json').write_text(json.dumps(records,indent=2),encoding='utf-8')
