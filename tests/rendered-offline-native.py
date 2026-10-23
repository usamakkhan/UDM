import base64,http.server,io,json,pathlib,re,subprocess,sys,threading,time,wave,zipfile
exe,root=sys.argv[1:3];root=pathlib.Path(root);requests=[]
video=pathlib.Path(sys.argv[3]).read_bytes() if len(sys.argv)>3 else None
png=base64.b64decode('iVBORw0KGgoAAAANSUhEUgAAAAEAAAABCAQAAAC1HAwCAAAAC0lEQVR42mP8/x8AAwMCAO+jRZkAAAAASUVORK5CYII=')
audio_buffer=io.BytesIO()
with wave.open(audio_buffer,'wb') as audio:
 audio.setnchannels(1);audio.setsampwidth(2);audio.setframerate(8000);audio.writeframes(b'\x00\x00'*8000)
wav=audio_buffer.getvalue();captions=b'WEBVTT\n\n00:00.000 --> 00:00.900\nSaved offline caption\n'
class Handler(http.server.BaseHTTPRequestHandler):
 def do_GET(self):
  requests.append(self.path)
  if self.path=='/assets/pixel.png' and requests.count(self.path)==1:
   (root/'pause-now').write_text('Pause after cached HTML and CSS')
   time.sleep(0.3)
  resources={'/ordinary':('text/html',b'<h1>Ordinary server page</h1>'),'/assets/style.css':('text/css',b'body { color: rgb(10, 80, 120); }'),'/assets/pixel.png':('image/png',png),'/assets/next.html':('text/html',b'<h1>Child saved page</h1><a href="/page">Back</a>')}
  resources['/assets/css-only.png']=('image/png',png)
  resources['/assets/tone.wav']=('audio/wav',wav)
  resources['/assets/captions.vtt']=('text/vtt',captions)
  if video is not None:resources['/assets/clip.mp4']=('video/mp4',video)
  kind,data=resources.get(self.path,('text/plain',b'Start page must not be requested'))
  try:
   self.send_response(200 if self.path in resources else 410);self.send_header('Content-Type',kind);self.send_header('Content-Length',str(len(data)));self.end_headers();self.wfile.write(data)
  except (BrokenPipeError,ConnectionResetError,ConnectionAbortedError):pass
 def log_message(self,*args):pass
server=http.server.ThreadingHTTPServer(('127.0.0.1',0),Handler);threading.Thread(target=server.serve_forever,daemon=True).start()
try:
 result=subprocess.run([exe,f'http://127.0.0.1:{server.server_port}/page',str(root)]+(['--video'] if video is not None else []),capture_output=True,text=True,timeout=60)
 print(result.stdout,end='');print(result.stderr,end='');assert result.returncode==0
 report=json.loads((root/'results.json').read_text());assert report['passed']
 assert '/page' not in requests,requests
 assert sorted(requests)==sorted((['/assets/next.html','/assets/pixel.png','/assets/style.css','/assets/css-only.png','/assets/tone.wav','/assets/captions.vtt']+(['/assets/clip.mp4'] if video is not None else []))*2+['/assets/pixel.png','/ordinary']),requests
 with zipfile.ZipFile(report['archive']) as archive:
  names=archive.namelist();html=archive.read('index.html').decode();manifest=json.loads(archive.read('UDM-offline-report.json'))
  assert 'Runtime-only heading' in html and '<base' not in html
  assert manifest['RenderedStartPage'] and manifest['Pages']==2 and not manifest['Errors']
  assert 'resource-' in html and 'form-action' in html
  selected=re.search(r'<img\b[^>]*\bsrc=[\x22\x27]([^\x22\x27]+)',html)
  assert selected and selected.group(1) in names,html
  assert archive.read(selected.group(1))==png,'Selected responsive image bytes changed or missing'
  assert '<picture>' in html and 'srcset=' not in html
  shadow=re.search(r'<template\b[^>]*shadowrootmode=[\x22\x27]open[\x22\x27][^>]*>(.*?)</template>',html,re.S)
  assert shadow and 'Saved shadow heading' in shadow.group(1),'Shadow content lost during archive publication'
  shadow_image=re.search(r'<img\b[^>]*src=[\x22\x27]([^\x22\x27]+)',shadow.group(1))
  assert shadow_image and shadow_image.group(1) in names,'Shadow image not rewritten to an archived resource'
  assert archive.read(shadow_image.group(1))==png,'Shadow image bytes changed'
  css_image=re.search(r'background-image:\s*url\(([^)]+)\)',shadow.group(1))
  assert css_image and css_image.group(1) in names,'CSS-only shadow image not archived'
  assert archive.read(css_image.group(1))==png,'CSS-only image bytes changed'
  for tag,expected in [('audio',wav),('track',captions)]+([('video',video)] if video is not None else []):
   media=re.search(r'<'+tag+r'\b[^>]*\bsrc=[\x22\x27]([^\x22\x27]+)',html)
   assert media and media.group(1) in names,tag+' not archived and rewritten'
   assert archive.read(media.group(1))==expected,tag+' bytes changed'
  destination=root/'extracted';destination.mkdir()
  for name in names:
   target=(destination/name).resolve();assert target.is_relative_to(destination.resolve())
  archive.extractall(destination)
 with zipfile.ZipFile(report['redownloadArchive']) as repeat:
  assert b'Runtime-only heading' in repeat.read('index.html')
  assert json.loads(repeat.read('UDM-offline-report.json'))['RenderedStartPage']
 with zipfile.ZipFile(report['ordinaryArchive']) as ordinary:
  assert b'Ordinary server page' in ordinary.read('index.html')
  assert not json.loads(ordinary.read('UDM-offline-report.json'))['RenderedStartPage']
 report.update({'httpRequests':requests,'startPageNotRequested':True,'localLinksRewritten':True,'archiveEntries':names,'renderedStartPage':True,'pauseAndCatalogReopen':True,'cachedHtmlAndCssNotRefetchedOnResume':True})
 (root/'results.json').write_text(json.dumps(report,indent=2));print('PASS rendered HTML archive, local assets, child navigation and no starting-page request')
finally:server.shutdown();server.server_close()
