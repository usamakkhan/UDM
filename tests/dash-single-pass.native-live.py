"""Owned HTTP resources: one-pass identity admission, cancellation and restart."""
from pathlib import Path
import hashlib,http.server,json,subprocess,sys,threading
root=Path(sys.argv[1]).resolve();exe=Path(sys.argv[2]).resolve()
assert not root.exists();root.mkdir(parents=True)
requests=[];results=[];changed=False
payload=bytes(range(256))*16
class Handler(http.server.BaseHTTPRequestHandler):
    def log_message(self,*args):pass
    def note(self):requests.append(dict(method=self.command,path=self.path,range=self.headers.get('Range'),ifMatch=self.headers.get('If-Match')))
    def do_HEAD(self):
        self.note()
        if self.path.startswith(('/chunked/','/head-supported/','/ranged/')):
            self.send_response(200);self.send_header('Content-Length',str(len(payload)));self.send_header('ETag','"first"');self.end_headers()
        else:
            self.send_response(501);self.send_header('Content-Length','0');self.end_headers()
    def do_GET(self):
        self.note();case=self.path.split('/')[1]
        if self.path.endswith('.mpd'):
            init='<Initialization sourceURL="init.bin"/>' if case=='init' else ''
            byte_range=' mediaRange="0-4095"' if case=='ranged' else ''
            data=f'<MPD xmlns="urn:mpeg:dash:schema:mpd:2011" id="owned-single-pass" type="static" mediaPresentationDuration="PT1S"><Period id="p" duration="PT1S"><AdaptationSet id="video" mimeType="video/mp4" codecs="avc1.42c01e" width="160" height="90"><Representation id="v" bandwidth="100000"><SegmentList timescale="1000" duration="1000">{init}<SegmentURL media="segment.bin"{byte_range}/></SegmentList></Representation></AdaptationSet></Period></MPD>'.encode()
            self.send_response(200);self.send_header('Content-Length',str(len(data)));self.end_headers();self.wfile.write(data);return
        tag='"second"' if changed and case=='cancel' else '"first"'
        if self.headers.get('If-Match') and self.headers['If-Match']!=tag:
            self.send_response(412);self.send_header('Content-Length','0');self.end_headers();return
        self.send_response(206 if case=='ranged' and self.headers.get('Range') else 200)
        if case=='ranged' and self.headers.get('Range'):self.send_header('Content-Range','bytes 0-4095/4096')
        self.send_header('Content-Type','text/html' if case=='html' else 'video/mp4')
        if case=='encoding':self.send_header('Content-Encoding','gzip')
        if case=='content-range':self.send_header('Content-Range','bytes 0-4095/4096')
        if case!='missing-tag':self.send_header('ETag','W/"weak"' if case=='weak' else tag)
        if case=='duplicate':self.send_header('ETag','"duplicate"')
        if case=='chunked':self.send_header('Transfer-Encoding','chunked')
        else:self.send_header('Content-Length',str(len(payload)))
        self.send_header('Connection','close');self.end_headers()
        try:
            if case=='chunked':self.wfile.write(b'1000\r\n'+payload+b'\r\n0\r\n\r\n')
            else:self.wfile.write(payload)
        except (ConnectionError,OSError):pass
        self.close_connection=True
server=http.server.ThreadingHTTPServer(('127.0.0.1',0),Handler)
threading.Thread(target=server.serve_forever,daemon=True).start()
base=f'http://127.0.0.1:{server.server_port}'
def run(case,mode='capture',success=True,folder=None):
    target=root/(folder or case);start=len(requests)
    p=subprocess.run([str(exe),str(target),base+'/'+case+'/manifest.mpd',mode],capture_output=True,timeout=35)
    (root/(case+'-'+mode+'.log')).write_bytes(p.stdout+p.stderr)
    result=json.loads((target/(mode+'-results.json')).read_text(encoding='utf-8-sig'))
    assert result['passed']==success and p.returncode==(0 if success else 1),result
    results.append(dict(case=case,mode=mode,expectedSuccess=success,result=result,requests=requests[start:]))
    return result,requests[start:]
try:
    for case,count in [('fresh',1),('head-supported',1),('init',2)]:
        result,wire=run(case)
        resources=[r for r in wire if not r['path'].endswith('.mpd')]
        assert len(resources)==count and all(r['method']=='GET' and r['range'] is None and r['ifMatch'] is None for r in resources),wire
        assert result['accountedBytes']==count*len(payload)
        assert result['index']['schema']==2
    result,wire=run('ranged')
    resources=[r for r in wire if not r['path'].endswith('.mpd')]
    assert [r['method'] for r in resources]==['HEAD','GET'] and resources[-1]['range']=='bytes=0-4095' and resources[-1]['ifMatch']=='"first"',wire
    assert result['index']['schema']==1
    result,wire=run('fresh',folder='fresh')
    assert all(r['path'].endswith('.mpd') for r in wire),'Verified receipts must avoid refetch'
    for case in ['html','encoding','content-range','duplicate','weak','missing-tag']:
        result,wire=run(case,success=False)
        assert not list((root/case).glob('*.receipt.json'))
    result,wire=run('chunked')
    resources=[r for r in wire if not r['path'].endswith('.mpd')]
    assert [r['method'] for r in resources]==['GET','HEAD','GET'] and resources[-1]['ifMatch']=='"first"',wire
    result,wire=run('cancel','cancel')
    pinned=result['index']['tracks'][0]['segments']['1']['resourceVersion']
    assert pinned['etag']=='"first"' and pinned['total']==len(payload)
    assert not list((root/'cancel').glob('*.receipt.json'))
    result,wire=run('cancel','reconcile')
    assert not result['index']['tracks'][0]['segments']['1']['committed']
    changed=True
    result,wire=run('cancel',success=False)
    assert any(r['ifMatch']=='"first"' for r in wire),wire
    result,wire=run('cancel','reconcile')
    assert result['index']['tracks'][0]['segments']['1']['resourceVersion']==pinned
    changed=False
    result,wire=run('cancel')
    assert any(r['ifMatch']=='"first"' for r in wire),wire
    record=result['index']['tracks'][0]['segments']['1']['resource']
    stored=root/'cancel'/record['file']
    assert stored.read_bytes()==payload and hashlib.sha256(payload).hexdigest()==record['sha256']
    recovery=[]
    if len(sys.argv)==5:
        for mode,tool in [('unversioned',sys.argv[3]),('versioned',sys.argv[3]),('old-reader',sys.argv[4])]:
            target=root/('recovery-'+mode);start=len(requests)
            p=subprocess.run([tool,str(root/'fresh'),str(target),mode],capture_output=True,timeout=35)
            (root/(mode+'-recovery.log')).write_bytes(p.stdout+p.stderr)
            r=json.loads((target/'results.json').read_text(encoding='utf-8-sig'))
            assert r['passed'] and p.returncode==0,r
            wire=requests[start:]
            assert len(wire)==(0 if mode=='old-reader' else 1),wire
            if mode=='versioned':assert wire[0]['ifMatch']=='"first"'
            recovery.append(dict(mode=mode,result=r,requests=wire,actorSha256=hashlib.sha256(Path(tool).read_bytes()).hexdigest()))
    (root/'qualification.json').write_text(json.dumps(dict(passed=True,cases=len(results),results=results,recovery=recovery,actorSha256=hashlib.sha256(exe.read_bytes()).hexdigest(),recipeSha256=hashlib.sha256(Path(__file__).read_bytes()).hexdigest()),indent=2),encoding='utf-8')
finally:
    server.shutdown();server.server_close()
    (root/'requests.json').write_text(json.dumps(requests,indent=2),encoding='utf-8')
