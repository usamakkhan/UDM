"""HTTP clock-source delay controls for the actual live planner; owned loopback only."""
from pathlib import Path
import datetime,hashlib,http.server,json,subprocess,sys,threading,time
root=Path(sys.argv[1]).resolve()
assert not root.exists()
root.mkdir(parents=True)
exe=Path(sys.argv[2]).resolve()
baseline='--baseline' in sys.argv[3:]
counts={k:dict(manifest=0,clock=0) for k in ['once','always']}
requests=[]
now=lambda:datetime.datetime.now(datetime.timezone.utc)
stamp=lambda v:v.isoformat(timespec='milliseconds').replace('+00:00','Z')
origin=now()-datetime.timedelta(seconds=20)
class Handler(http.server.BaseHTTPRequestHandler):
    def log_message(self,*args):pass
    def do_GET(self):
        case,leaf=self.path.strip('/').split('/')
        kind='manifest' if leaf=='manifest.mpd' else 'clock'
        assert case in counts and leaf in ['manifest.mpd','clock']
        counts[case][kind]+=1
        n=counts[case][kind]
        started=time.monotonic()
        if kind=='clock' and (case=='always' or n==1):time.sleep(.35)
        if kind=='clock':data=stamp(now()).encode()
        else:
            data=(f'<MPD xmlns="urn:mpeg:dash:schema:mpd:2011" id="owned-clock-{case}" type="dynamic" availabilityStartTime="{stamp(origin)}" publishTime="{stamp(now())}" minimumUpdatePeriod="PT0.2S" timeShiftBufferDepth="PT3600S">'
                  f'<UTCTiming schemeIdUri="urn:mpeg:dash:utc:http-xsdate:2014" value="/{case}/clock"/>'
                  '<Period id="p0" start="PT0S" duration="PT60S"><AdaptationSet id="v" mimeType="video/mp4" codecs="avc1.42c01e" width="160" height="90"><Representation id="r" bandwidth="100000"><SegmentTemplate timescale="1000" duration="1000" startNumber="1" initialization="init.mp4" media="segment-$Number$.m4s"/></Representation></AdaptationSet></Period></MPD>').encode()
        self.send_response(200)
        self.send_header('Content-Type','application/dash+xml' if kind=='manifest' else 'text/plain')
        self.send_header('Content-Length',str(len(data)))
        self.send_header('ETag','"'+str(n)+'"')
        self.send_header('Connection','close')
        self.end_headers()
        self.wfile.write(data)
        requests.append(dict(case=case,kind=kind,request=n,responseMs=(time.monotonic()-started)*1000))
        self.close_connection=True
server=http.server.ThreadingHTTPServer(('127.0.0.1',0),Handler)
threading.Thread(target=server.serve_forever,daemon=True).start()
records=[]
source_pin=hashlib.sha256(Path(__file__).read_bytes()).hexdigest()
try:
    for case in counts:
        # This existing actor mode disables synthetic accounting waits. Only the
        # independent HTTP timing server creates the measured delay here.
        p=subprocess.run([str(exe),str(root/case),f'http://127.0.0.1:{server.server_port}/{case}/manifest.mpd','cached304'],capture_output=True,timeout=20)
        (root/(case+'.log')).write_bytes(p.stdout+p.stderr)
        result=json.loads((root/case/'results.json').read_text(encoding='utf-8-sig'))
        row=dict(case=case,counts=counts[case],exitCode=p.returncode,result=result)
        records.append(row)
        (root/'results.json').write_text(json.dumps(dict(records=records,requests=requests),indent=2),encoding='utf-8')
        assert result['inputSnapshotUnchanged']
        if baseline:assert p.returncode==1 and counts[case]==dict(manifest=2,clock=1) and 'expired while synchronizing' in result['error'],row
        elif case=='once':assert p.returncode==0 and result['freshAtReturn'] and result['segments']>0 and counts[case]==dict(manifest=3,clock=2),row
        else:assert p.returncode==1 and 'all three bounded' in result['error'] and counts[case]==dict(manifest=4,clock=3),row
    assert hashlib.sha256(Path(__file__).read_bytes()).hexdigest()==source_pin
    (root/'qualification.json').write_text(json.dumps(dict(passed=True,baseline=baseline,cases=2,recipeSha256=source_pin,actorSha256=hashlib.sha256(exe.read_bytes()).hexdigest()),indent=2),encoding='utf-8')
finally:
    server.shutdown()
    (root/'requests.json').write_text(json.dumps(requests,indent=2),encoding='utf-8')
