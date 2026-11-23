"""Owned continuous DASH workflow. Pass a fresh root; select actor with UDM_CONTINUOUS_DASH_TEST_EXE.

--baseline retains a failed original job. --resume continues that root with a new actor,
then runs fresh AV/video/audio, invalid-boundary and held-refresh Save controls.
"""
from pathlib import Path
from fractions import Fraction
import datetime, hashlib, http.server, json, os, re, struct, subprocess, sys, threading

root = Path(sys.argv[1]).resolve()
baseline = '--baseline' in sys.argv[2:]
resume = '--resume' in sys.argv[2:]
assert not (baseline and resume)
if resume:
    assert (root / 'baseline-reproduction.json').is_file()
else:
    assert not root.exists()
    root.mkdir(parents=True)
    (root / '.udm-owned').write_text('Owned continuous DASH workflow')
exe = Path(os.environ['UDM_CONTINUOUS_DASH_TEST_EXE']).resolve()
tools = Path(os.environ.get('ProgramFiles', 'C:/Program Files')) / 'UDM/tools'
ffmpeg = Path(os.environ.get('UDM_FFMPEG', str(tools / 'ffmpeg.exe')))
ffprobe = Path(os.environ.get('UDM_FFPROBE', str(tools / 'ffprobe.exe')))
destination = Path(os.environ.get('UDM_CONTINUOUS_DASH_DESTINATION', str(root / 'downloads'))).resolve()
sha = lambda p: hashlib.sha256(p.read_bytes()).hexdigest()
assets = root / 'assets'

def video_frames(text):
    tb = re.search(r'^#tb\s+0:\s*(\d+)/(\d+)', text, re.M)
    assert tb
    scale = Fraction(int(tb[1]), int(tb[2]))
    rows = []
    for line in text.splitlines():
        if not line or line.startswith('#'):
            continue
        cells = [x.strip() for x in line.split(',')]
        rows.append(dict(dts=str(int(cells[1]) * scale), pts=str(int(cells[2]) * scale),
                         duration=str(int(cells[3]) * scale), hash=cells[-1]))
    return rows

def decode_video(path):
    return video_frames(subprocess.check_output([str(ffmpeg), '-v', 'error', '-xerror', '-i', str(path),
                        '-map', '0:v:0', '-fps_mode', 'passthrough', '-f', 'framemd5', '-'], text=True))

def decode_audio(path, seconds=None):
    args = [str(ffmpeg), '-v', 'error', '-xerror', '-i', str(path)]
    if seconds:
        args += ['-t', seconds]
    return subprocess.check_output(args + ['-map', '0:a:0', '-ac', '1', '-ar', '48000',
                                           '-acodec', 'pcm_s16le', '-f', 's16le', '-'])

def audio_packets(path):
    data = json.loads(subprocess.check_output([str(ffprobe), '-v', 'error', '-select_streams', 'a:0',
        '-show_packets', '-show_data_hash', 'sha256', '-of', 'json', str(path)], text=True))
    return [p['data_hash'] for p in data['packets']]

def boxes(data):
    at = 0
    while at < len(data):
        size, kind = struct.unpack_from('>I4s', data, at)
        assert size >= 8 and at + size <= len(data)
        yield kind, bytearray(data[at:at + size])
        at += size

def rebase(fragment):
    def walk(start, stop):
        at = start
        while at < stop:
            size, kind = struct.unpack_from('>I4s', fragment, at)
            assert size >= 8 and at + size <= stop
            if kind in (b'moof', b'traf'):
                walk(at + 8, at + size)
            elif kind == b'tfdt':
                length = 8 if fragment[at + 8] == 1 else 4
                fragment[at + 12:at + 12 + length] = bytes(length)
            at += size
    walk(0, len(fragment))
    return fragment

def split_movie(path, kind, expected):
    data = list(boxes(path.read_bytes()))
    init = b''.join(v for k, v in data if k in (b'ftyp', b'moov'))
    fragments = []
    for n, (name, value) in enumerate(data):
        if name == b'moof':
            assert data[n + 1][0] == b'mdat'
            fragments.append(bytes(rebase(value)) + data[n + 1][1])
    assert len(fragments) == expected, (kind, len(fragments))
    for n, value in enumerate(fragments):
        folder = assets / ('p' + str(n)) / kind
        folder.mkdir(parents=True, exist_ok=True)
        (folder / 'init.mp4').write_bytes(init)
        (folder / 'segment.m4s').write_bytes(value)

if not resume:
    assets.mkdir()
    video = assets / 'original-video.mp4'
    audio = assets / 'original-audio.mp4'
    subprocess.run([str(ffmpeg), '-v', 'error', '-nostdin', '-f', 'lavfi', '-i',
        'testsrc2=s=640x360:r=125/16:d=3.072', '-vf', 'hue=h=90*floor(n/8)',
        '-frames:v', '24', '-c:v', 'libx264', '-preset', 'ultrafast', '-profile:v', 'baseline',
        '-level', '3.0', '-bf', '0', '-g', '8', '-keyint_min', '8', '-sc_threshold', '0',
        '-pix_fmt', 'yuv420p', '-video_track_timescale', '1000', '-movflags',
        'empty_moov+frag_keyframe+default_base_moof', str(video)], check=True)
    expression = 'aevalsrc=sin(2*PI*(440+220*floor(n/49152))*t):s=48000:d=3.072'
    subprocess.run([str(ffmpeg), '-v', 'error', '-nostdin', '-f', 'lavfi', '-i', expression,
        '-frames:a', '143', '-c:a', 'aac', '-b:a', '64000', '-ac', '1', '-use_editlist', '0',
        '-avoid_negative_ts', 'make_zero', '-movflags', 'empty_moov+default_base_moof',
        '-frag_duration', '1024000', str(audio)], check=True)
    split_movie(video, 'v', 3)
    split_movie(audio, 'a', 3)
    short = assets / 'short-audio.mp4'
    subprocess.run([str(ffmpeg), '-v', 'error', '-nostdin', '-f', 'lavfi', '-i',
        'sine=frequency=440:sample_rate=48000:duration=0.512', '-frames:a', '23', '-c:a', 'aac',
        '-b:a', '64000', '-ac', '1', '-use_editlist', '0', '-avoid_negative_ts', 'make_zero',
        '-movflags', 'empty_moov+default_base_moof', str(short)], check=True)
    split_movie(short, 'short-a', 1)
    frames = decode_video(video)
    pcm = decode_audio(audio)
    first_pcm = decode_audio(audio, '1.024')
    packets = audio_packets(audio)
    assert len(frames) == len({x['hash'] for x in frames}) == 24
    assert len(packets) == 144 and len(pcm) == 147456 * 2 and len(first_pcm) == 49152 * 2
    oracle = dict(videoFrames=frames, audioPackets=packets, audioPcmBytes=len(pcm),
        audioPcmSha256=hashlib.sha256(pcm).hexdigest(), firstAudioPcmBytes=len(first_pcm),
        firstAudioPcmSha256=hashlib.sha256(first_pcm).hexdigest(),
        assets=[dict(path=p.relative_to(assets).as_posix(), sha256=sha(p), bytes=p.stat().st_size)
                for p in sorted(assets.rglob('*')) if p.is_file()])
    (root / 'oracle.json').write_text(json.dumps(oracle, indent=2))
else:
    oracle = json.loads((root / 'oracle.json').read_text())

guard = threading.Lock()
states, requests, records, gates = {}, [], [], {}
if resume:
    old = json.loads((root / 'server-state.json').read_text())
    states, requests, records = old['states'], old['requests'], old['records']
    for state in states.values():
        state['origin'] = datetime.datetime.fromisoformat(state['origin'])

clock = lambda: datetime.datetime.now(datetime.timezone.utc)
stamp = lambda t: t.isoformat(timespec='milliseconds').replace('+00:00', 'Z')

def new_case(name, tracks, fault=''):
    states[name] = dict(period=0, ended=False, tracks=tracks, fault=fault, served=[],
                        origin=clock() - datetime.timedelta(seconds=20), holdTimedOut=False)
    gates[name] = threading.Event()

def manifest(name, state):
    n, now = state['period'], clock()
    attrs = ('type="static" mediaPresentationDuration="PT3.072S"' if state['ended'] else
        f'type="dynamic" availabilityStartTime="{stamp(state["origin"])}" '
        'minimumUpdatePeriod="PT0.2S" suggestedPresentationDelay="PT3600S" timeShiftBufferDepth="PT3600S"')
    changed = '-other' if state['fault'] == 'changed' and n else ''
    text = f'<MPD xmlns="urn:mpeg:dash:schema:mpd:2011" id="owned-continuous-{name}{changed}" {attrs} publishTime="{stamp(now)}">'
    if not state['ended']:
        text += f'<UTCTiming schemeIdUri="urn:mpeg:dash:utc:direct:2014" value="{stamp(now)}"/>'
    start = n * 1024 + (1024 if state['fault'] == 'gap' and n else 0)
    text += f'<Period id="p{n}" start="PT{start / 1000:.3f}S">'
    for kind in state['tracks']:
        if kind == 'v':
            text += '<AdaptationSet id="v" mimeType="video/mp4" codecs="avc1.42c01e" width="640" height="360">'
            scale, duration = 1000, 1024
        else:
            text += '<AdaptationSet id="a" mimeType="audio/mp4" codecs="mp4a.40.2" audioSamplingRate="48000"><AudioChannelConfiguration schemeIdUri="urn:mpeg:dash:23003:3:audio_channel_configuration:2011" value="1"/>'
            scale, duration = 48000, 24576 if state['fault'] == 'short-audio' and n == 0 else 49152
        text += f'<Representation id="r" bandwidth="100000"><SegmentList timescale="{scale}" startNumber="1"><SegmentTimeline><S t="0" d="{duration}"/></SegmentTimeline><Initialization sourceURL="p{n}/{kind}/init.mp4"/><SegmentURL media="p{n}/{kind}/segment.m4s"/></SegmentList></Representation></AdaptationSet>'
    return (text + '</Period></MPD>').encode()

class Handler(http.server.BaseHTTPRequestHandler):
    protocol_version = 'HTTP/1.1'
    def log_message(self, *args): pass
    def do_HEAD(self): self.serve(False)
    def do_GET(self): self.serve(True)
    def serve(self, body):
        name, relative = self.path.split('?', 1)[0].lstrip('/').split('/', 1)
        with guard:
            state = dict(states[name])
        held = relative == 'manifest.mpd' and state['fault'] == 'save' and state['period'] > 0
        if held:
            (root / 'hold-entered').write_text('Owned manifest request is held')
            if not gates[name].wait(10):
                with guard: states[name]['holdTimedOut'] = True
        if relative == 'manifest.mpd':
            data = manifest(name, state)
        else:
            pieces = relative.split('/')
            if state['fault'] == 'short-audio' and pieces[:2] == ['p0', 'a']:
                pieces[1] = 'short-a'
            path = (assets / '/'.join(pieces)).resolve()
            assert path.is_relative_to(assets.resolve())
            data = path.read_bytes()
        etag = '"' + hashlib.sha256(data).hexdigest() + '"'
        first, last, status = 0, len(data) - 1, 200
        requested = self.headers.get('Range')
        if requested:
            match = re.fullmatch(r'bytes=(\d+)-(\d*)', requested)
            assert match
            first = int(match[1]); last = min(last, int(match[2]) if match[2] else last)
            assert 0 <= first <= last
            status = 206
        with guard:
            requests.append(dict(case=name, method=self.command, path=relative, period=state['period'],
                                 bytes=last-first+1, status=status, held=held, range=requested))
        self.send_response(status)
        self.send_header('Content-Type', 'application/dash+xml' if relative == 'manifest.mpd' else 'video/mp4')
        self.send_header('Content-Length', str(last-first+1)); self.send_header('ETag', etag)
        self.send_header('Accept-Ranges', 'bytes'); self.send_header('Connection', 'close')
        if status == 206: self.send_header('Content-Range', f'bytes {first}-{last}/{len(data)}')
        self.end_headers()
        if body:
            try: self.wfile.write(data[first:last+1])
            except (BrokenPipeError, ConnectionResetError): pass
            if relative.endswith('/segment.m4s'):
                with guard:
                    current = states[name]
                    if relative not in current['served']: current['served'].append(relative)
                    n = current['period']
                    if all(f'p{n}/{kind}/segment.m4s' in current['served'] for kind in current['tracks']):
                        if n < 2: current['period'] += 1
                        else: current['ended'] = True
        self.close_connection = True

server = http.server.ThreadingHTTPServer(('127.0.0.1', old['port'] if resume else 0), Handler)
thread = threading.Thread(target=server.serve_forever, daemon=True); thread.start()

def config_for(name, save=False):
    state = states[name]; kinds = state['tracks']; audio_only = kinds == ['a']
    url = f'http://127.0.0.1:{server.server_port}/{name}/manifest.mpd'
    plan = dict(type='dash', live=True, manifest=url, container='m4a' if audio_only else 'mp4',
        audioOnly=audio_only, audioExpected='a' in kinds, height=0 if audio_only else 360,
        tracks=[dict(kind='video' if k=='v' else 'audio', periodId='p0', adaptationId=k,
                     representationId='r', credentialUrls=[]) for k in kinds])
    cfg = dict(plan=plan, destination=str(destination/name), expectedPeriods=1 if save else 3,
               saveExpected=save, saveTrigger=str(root/'hold-entered') if save else '')
    path = root/(name+'-config.json'); path.write_text(json.dumps(cfg, indent=2)); return path

def actor(name, mode='record'):
    cfg = config_for(name, states[name]['fault']=='save')
    with (root/(name+'-'+mode+'.log')).open('w') as log:
        process = subprocess.Popen([str(exe), mode, str(root/name), str(cfg)], stdout=log, stderr=subprocess.STDOUT)
        (root/'active.json').write_text(json.dumps(dict(pid=process.pid, case=name, mode=mode)))
        code = process.wait()
    data = json.loads((root/name/(mode+'-results.json')).read_text())
    row = dict(case=name, mode=mode, exitCode=code, binarySha256=sha(exe), result=str(root/name/(mode+'-results.json')))
    records.append(row); print(name+' '+mode+': '+str(code)+'; '+data.get('fixtureError',''), flush=True)
    return data, code

def retained(data):
    folder = Path(data['mediaFolder'])/'live-dash'
    return {p.relative_to(folder).as_posix():sha(p) for p in folder.rglob('*')
            if p.is_file() and p.suffix not in ('.json','.bak')}

def validate(name, data, count=3):
    kinds = states[name]['tracks']
    if 'v' in kinds: assert video_frames(data['videoFrameMd5']) == oracle['videoFrames'][:count*8]
    if 'a' in kinds:
        assert [p['data_hash'] for p in data['audioProbe']['packets']] == oracle['audioPackets'][:count*48]
        assert data['audioPcmBytes'] == oracle['audioPcmBytes' if count==3 else 'firstAudioPcmBytes']
        assert data['audioPcmSha256'] == oracle['audioPcmSha256' if count==3 else 'firstAudioPcmSha256']
    assert data['finalBytes'] == data['assembledBytes'] and data['publications'] == 1
    for n in range(count):
        for kind in kinds:
            for resource in ('init.mp4','segment.m4s'):
                assert sum(x['case']==name and x['method']=='GET' and x['path']==f'p{n}/{kind}/{resource}' for x in requests)==1

try:
    if baseline:
        new_case('baseline-av',['v','a'])
        data, code = actor('baseline-av')
        assert code==1 and data['status']=='Failed' and 'no longer present' in data['error'].lower() and data['publications']==0
        saved = retained(data); assert len(saved)==4
        (root/'baseline-reproduction.json').write_text(json.dumps(dict(reproduced=True,error=data['error'],resources=saved),indent=2))
    else:
        if resume:
            data, code = actor('baseline-av','resume'); assert code==0 and data['passed']; validate('baseline-av',data)
        for name,kinds in [('av',['v','a']),('video',['v']),('audio',['a'])]:
            new_case(name,kinds); data,code=actor(name); assert code==0 and data['passed']; validate(name,data)
        for name in ['gap','short-audio','changed']:
            new_case(name,['v','a'],name); data,code=actor(name)
            assert code==1 and data['status']=='Failed' and data['publications']==0 and retained(data)
            assert not any(x['case']==name and x['path'].startswith('p1/') for x in requests)
        new_case('save',['v','a'],'save'); data,code=actor('save')
        assert code==0 and data['passed'] and not states['save']['holdTimedOut']; validate('save',data,1)
        gates['save'].set()
        (root/'qualification.json').write_text(json.dumps(dict(passed=True,resumedBaseline=resume,
            uninterruptedAvVideoAudioPassed=True,videoFrameTimingMatched=True,audioPacketsAndPcmMatched=True,
            rejectedGapShortAudioAndPresentationChange=True,saveFinishedWithHeadersStillHeld=True),indent=2))
finally:
    for gate in gates.values(): gate.set()
    server.shutdown(); server.server_close(); thread.join()
    for state in states.values(): state['origin']=state['origin'].isoformat()
    (root/'server-state.json').write_text(json.dumps(dict(port=server.server_port,states=states,requests=requests,records=records),indent=2))
    (root/('baseline-terminal.json' if baseline else 'candidate-terminal.json')).write_text(json.dumps(dict(originClosed=True,allActorsTerminal=True,records=records),indent=2))
