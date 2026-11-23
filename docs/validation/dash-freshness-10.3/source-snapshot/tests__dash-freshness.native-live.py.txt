"""Bounded live-MPD freshness recovery. Owned loopback metadata only.

Usage: SCRIPT FRESH_ROOT ACTOR [--baseline]. No media payloads or installed state.
"""
from pathlib import Path
import datetime,hashlib,http.server,json,subprocess,sys,threading,time

root=Path(sys.argv[1]).resolve()
assert not root.exists()
root.mkdir(parents=True)
actor=Path(sys.argv[2]).resolve()
baseline='--baseline' in sys.argv[3:]
cases=['transient','always','identity','http503','cached304','cancel']
counts={mode:0 for mode in cases}
records=[]
requests=[]
now=lambda:datetime.datetime.now(datetime.timezone.utc)
stamp=lambda v:v.isoformat(timespec='milliseconds').replace('+00:00','Z')
origin=now()-datetime.timedelta(seconds=20)

def manifest(mode,n):
    identity='changed' if mode=='identity' and n>=3 else 'owned-freshness-'+mode
    # A cached document keeps both publication and direct-time body anchors.
    published=origin+datetime.timedelta(seconds=20) if mode=='cached304' else now()
    return (f'<MPD xmlns="urn:mpeg:dash:schema:mpd:2011" id="{identity}" type="dynamic" '
            f'availabilityStartTime="{stamp(origin)}" publishTime="{stamp(published)}" '
            'minimumUpdatePeriod="PT0.2S" timeShiftBufferDepth="PT3600S">'
            f'<UTCTiming schemeIdUri="urn:mpeg:dash:utc:direct:2014" value="{stamp(published)}"/>'
            '<Period id="p0" start="PT0S" duration="PT60S"><AdaptationSet id="v" mimeType="video/mp4" '
            'codecs="avc1.42c01e" width="160" height="90"><Representation id="r" bandwidth="100000">'
            '<SegmentTemplate timescale="1000" duration="1000" startNumber="1" '
            'initialization="init.mp4" media="segment-$Number$.m4s"/>'
            '</Representation></AdaptationSet></Period></MPD>').encode()

class Handler(http.server.BaseHTTPRequestHandler):
    def log_message(self,*args):pass
    def do_GET(self):
        mode=self.path.strip('/').split('/')[0]
        assert mode in cases and self.path.endswith('/manifest.mpd'),self.path
        counts[mode]+=1
        n=counts[mode]
        start=time.monotonic()
        status=503 if mode=='http503' and n>=3 else 304 if mode=='cached304' and n>=2 else 200
        if mode=='cached304' and n==2:time.sleep(.35)
        data=manifest(mode,n) if status==200 else b''
        self.send_response(status)
        self.send_header('ETag','"cached"' if mode=='cached304' else '"'+str(n)+'"')
        self.send_header('Content-Type','application/dash+xml')
        self.send_header('Content-Length',str(len(data)))
        self.send_header('Connection','close')
        self.end_headers()
        if data:self.wfile.write(data)
        requests.append(dict(case=mode,request=n,status=status,conditional=self.headers.get('If-None-Match'),responseMs=(time.monotonic()-start)*1000))
        self.close_connection=True

server=http.server.ThreadingHTTPServer(('127.0.0.1',0),Handler)
threading.Thread(target=server.serve_forever,daemon=True).start()
try:
    for mode in cases:
        p=subprocess.run([str(actor),str(root/mode),f'http://127.0.0.1:{server.server_port}/{mode}/manifest.mpd',mode],capture_output=True,timeout=20)
        (root/(mode+'.log')).write_bytes(p.stdout+p.stderr)
        result=json.loads((root/mode/'results.json').read_text(encoding='utf-8-sig'))
        row=dict(case=mode,exitCode=p.returncode,requests=counts[mode],result=result)
        records.append(row)
        (root/'results.json').write_text(json.dumps(dict(records=records,requests=requests),indent=2),encoding='utf-8')
        assert result['inputSnapshotUnchanged']
        if baseline:
            assert p.returncode==1 and 'expired while synchronizing' in result['error'] and counts[mode]==2,row
        elif mode in ['transient','cached304']:
            assert p.returncode==0 and result['completed'] and result['freshAtReturn'] and result['segments']>0 and counts[mode]==3,row
            if mode=='cached304':assert result['originalBodyAnchorPreserved'] and result['metadataCallbacks']==0,row
        elif mode=='always':
            assert p.returncode==1 and 'expired' in result['error'] and counts[mode]==4,row
        elif mode=='identity':
            assert p.returncode==1 and 'identity changed' in result['error'] and counts[mode]==3,row
        elif mode=='http503':
            assert p.returncode==1 and counts[mode]==3 and not result.get('completed',False),row
        else:
            assert p.returncode==3 and result['cancelled'] and counts[mode]==3,row
    (root/'qualification.json').write_text(json.dumps(dict(passed=True,baseline=baseline,cases=len(records),actorSha256=hashlib.sha256(actor.read_bytes()).hexdigest()),indent=2),encoding='utf-8')
finally:
    server.shutdown()
    (root/'requests.json').write_text(json.dumps(requests,indent=2),encoding='utf-8')
