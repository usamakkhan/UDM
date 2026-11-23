"""Count real TCP connections for sequential, independently pinned resources."""
from pathlib import Path
import hashlib,http.server,json,os,socket,subprocess,sys,threading,time
root=Path(sys.argv[1]).resolve();exe=Path(sys.argv[2]).resolve();baseline='--baseline' in sys.argv[3:]
assert not root.exists();root.mkdir(parents=True);records=[];runs=[]
payload=bytes(range(256))*16
class Handler(http.server.BaseHTTPRequestHandler):
    protocol_version='HTTP/1.1'
    def setup(self):
        super().setup();self.connection.setsockopt(socket.IPPROTO_TCP,socket.TCP_NODELAY,1)
    def log_message(self,*args):pass
    def do_GET(self):
        run,leaf=self.path.strip('/').split('/');manifest=leaf=='manifest.mpd'
        records.append(dict(run=run,path=leaf,clientPort=self.client_address[1],authorization=self.headers.get('Authorization'),cookie=self.headers.get('Cookie'),encoding=self.headers.get_all('Accept-Encoding')))
        if manifest:
            segments=''.join(f'<SegmentURL media="s-{n}.bin"/>' for n in range(8))
            data=f'<MPD xmlns="urn:mpeg:dash:schema:mpd:2011" id="owned-connections" type="static" mediaPresentationDuration="PT1S"><Period id="p" duration="PT1S"><AdaptationSet id="v" mimeType="video/mp4" codecs="avc1.42c01e" width="160" height="90"><Representation id="r" bandwidth="100000"><SegmentList timescale="1000" duration="125"><Initialization sourceURL="init.bin"/>{segments}</SegmentList></Representation></AdaptationSet></Period></MPD>'.encode()
        else:data=payload
        self.send_response(200);self.send_header('Content-Length',str(len(data)));self.send_header('Content-Type','application/dash+xml' if manifest else 'video/mp4')
        if not manifest:self.send_header('ETag','"'+leaf+'"');self.send_header('Set-Cookie','unexpected=must-not-replay; Path=/')
        self.end_headers();self.wfile.write(data)
server=http.server.ThreadingHTTPServer(('127.0.0.1',0),Handler);threading.Thread(target=server.serve_forever,daemon=True).start()
origin=f'http://127.0.0.1:{server.server_port}';config=root/'config.json';config.write_text(json.dumps(dict(workers=1,origins={origin:{'Authorization':'Bearer owned-fixture','Cookie':'owned=explicit'}},manifestHeaders={'Authorization':'Bearer owned-fixture','Cookie':'owned=explicit'},preferences={'ProxyMode':'Connect directly'})),encoding='utf-8')
env=os.environ.copy();env.update(TEMP=str(root),TMP=str(root));actor_pin=hashlib.sha256(exe.read_bytes()).hexdigest();recipe_pin=hashlib.sha256(Path(__file__).read_bytes()).hexdigest()
try:
    for n in range(3):
        name='run'+str(n);folder=root/name;begun=time.perf_counter();p=subprocess.run([str(exe),str(folder),origin+'/'+name+'/manifest.mpd','capture',str(config)],capture_output=True,timeout=30,env=env);elapsed=(time.perf_counter()-begun)*1000
        (root/(name+'.log')).write_bytes(p.stdout+p.stderr);result=json.loads((folder/'capture-results.json').read_text(encoding='utf-8-sig'));assert p.returncode==0 and result['passed'],result
        own=[r for r in records if r['run']==name];media=[r for r in own if r['path']!='manifest.mpd'];ports={r['clientPort'] for r in media}
        assert len(media)==9 and all(r['authorization']=='Bearer owned-fixture' and r['cookie']=='owned=explicit' and r['encoding']==['identity'] for r in media)
        assert len(ports)==(9 if baseline else 1),(baseline,ports)
        assert result['accountedBytes']==9*len(payload)
        track=result['index']['tracks'][0];resources=[track['initialization']]+[r['resource'] for r in track['segments'].values()]
        assert len(resources)==9
        for resource in resources:
            f=folder/resource['file'];assert f.read_bytes()==payload and hashlib.sha256(f.read_bytes()).hexdigest()==resource['sha256']
        runs.append(dict(passed=True,case=name,resourceRequests=9,resourceTcpConnections=len(ports),nativeProcessElapsedMs=elapsed,clientAccountedBytes=result['accountedBytes'],storedResources=len(resources),cookieAndAuthorizationExact=True))
    assert hashlib.sha256(exe.read_bytes()).hexdigest()==actor_pin and hashlib.sha256(Path(__file__).read_bytes()).hexdigest()==recipe_pin
    (root/'qualification.json').write_text(json.dumps(dict(passed=True,baseline=baseline,runs=runs,actorSha256=actor_pin,recipeSha256=recipe_pin,tlsTest=False,latencyComparedWithIdm=False,installed=False),indent=2),encoding='utf-8')
finally:
    server.shutdown();server.server_close();(root/'requests.json').write_text(json.dumps(records,indent=2),encoding='utf-8')
