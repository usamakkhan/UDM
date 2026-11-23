"""Independent direct-curl resource reuse/cookie isolation over mapped loopback."""
from pathlib import Path
import hashlib,http.server,json,os,socket,subprocess,sys,threading
root=Path(sys.argv[1]).resolve();exe=Path(sys.argv[2]).resolve();baseline='--baseline' in sys.argv[3:]
assert not root.exists();root.mkdir(parents=True);requests=[];cases=[];data=bytes(range(256))*8
class Handler(http.server.BaseHTTPRequestHandler):
    protocol_version='HTTP/1.1'
    def setup(self):super().setup();self.connection.setsockopt(socket.IPPROTO_TCP,socket.TCP_NODELAY,1)
    def log_message(self,*args):pass
    def do_GET(self):
        run,leaf=self.path.strip('/').split('/');requests.append(dict(run=run,leaf=leaf,clientPort=self.client_address[1],authorization=self.headers.get('Authorization'),cookie=self.headers.get('Cookie')))
        if leaf=='manifest.mpd':
            links=''.join(f'<SegmentURL media="s{n}.bin"/>' for n in range(4))
            body=f'<MPD xmlns="urn:mpeg:dash:schema:mpd:2011" id="owned-curl-reuse" type="static" mediaPresentationDuration="PT1S"><Period id="p" duration="PT1S"><AdaptationSet id="a" mimeType="audio/mp4" codecs="mp4a.40.2" audioSamplingRate="48000"><Representation id="r" bandwidth="64000"><SegmentList timescale="1000" duration="250"><Initialization sourceURL="init.bin"/>{links}</SegmentList></Representation></AdaptationSet></Period></MPD>'.encode()
        else:body=data
        self.send_response(200);self.send_header('Content-Length',str(len(body)))
        if leaf!='manifest.mpd':self.send_header('ETag','"'+leaf+'"');self.send_header('Set-Cookie','unexpected=curl-must-not-replay; Path=/')
        self.end_headers();self.wfile.write(body)
server=http.server.ThreadingHTTPServer(('127.0.0.1',0),Handler);threading.Thread(target=server.serve_forever,daemon=True).start()
origin=f'http://[::ffff:127.0.0.1]:{server.server_port}';config=root/'config.json';config.write_text(json.dumps(dict(workers=1,preferences={'ProxyMode':'Connect directly'},origins={origin:{'Cookie':'owned=curl','Authorization':'Bearer owned-curl'}})),encoding='utf-8')
env=os.environ.copy();env.update(TEMP=str(root),TMP=str(root));actor_pin=hashlib.sha256(exe.read_bytes()).hexdigest();recipe_pin=hashlib.sha256(Path(__file__).read_bytes()).hexdigest()
try:
    for n in range(2):
        name='run'+str(n);folder=root/name;p=subprocess.run([str(exe),str(folder),origin+'/'+name+'/manifest.mpd','capture',str(config)],env=env,capture_output=True,timeout=30)
        (root/(name+'.log')).write_bytes(p.stdout+p.stderr);r=json.loads((folder/'capture-results.json').read_text(encoding='utf-8-sig'));assert p.returncode==0 and r['passed'],r
        media=[q for q in requests if q['run']==name and q['leaf']!='manifest.mpd'];ports={q['clientPort'] for q in media}
        assert len(media)==5 and len(ports)==(5 if baseline else 1),media
        assert all(q['cookie']=='owned=curl' and q['authorization']=='Bearer owned-curl' for q in media)
        assert r['accountedBytes']==5*len(data)
        track=r['index']['tracks'][0];resources=[track['initialization']]+[q['resource'] for q in track['segments'].values()]
        for resource in resources:
            f=folder/resource['file'];assert f.read_bytes()==data and hashlib.sha256(f.read_bytes()).hexdigest()==resource['sha256']
        cases.append(dict(passed=True,case=name,resourceRequests=5,resourceTcpConnections=len(ports),clientAccountedBytes=r['accountedBytes'],storedResources=len(resources),cookieAndAuthorizationExact=True))
    assert hashlib.sha256(exe.read_bytes()).hexdigest()==actor_pin and hashlib.sha256(Path(__file__).read_bytes()).hexdigest()==recipe_pin
    (root/'qualification.json').write_text(json.dumps(dict(passed=True,baseline=baseline,cases=cases,actorSha256=actor_pin,recipeSha256=recipe_pin,mappedIpv6SelectsCurl=True,tlsTest=False,installed=False),indent=2),encoding='utf-8')
finally:
    server.shutdown();server.server_close();(root/'requests.json').write_text(json.dumps(requests,indent=2),encoding='utf-8')
