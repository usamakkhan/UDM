import http.server,json,pathlib,subprocess,sys,threading
tag=bytearray(128);tag[:3]=b'TAG'
for offset,value in [(3,b'Fixture title'),(33,b'Fixture artist'),(63,b'Fixture album')]:tag[offset:offset+len(value)]=value
body=pathlib.Path(sys.argv[3]).read_bytes() if len(sys.argv)>3 else b'\xff\xfb\x90\x00'+bytes(996)+tag
class Handler(http.server.BaseHTTPRequestHandler):
 def do_HEAD(self):
  self.send_response(200);self.send_header('Content-Type','audio/mpeg');self.send_header('Content-Length',str(len(body)));self.end_headers()
 def do_GET(self):
  self.do_HEAD();self.wfile.write(body)
 def log_message(self,*args):pass
server=http.server.ThreadingHTTPServer(('127.0.0.1',0),Handler)
threading.Thread(target=server.serve_forever,daemon=True).start()
try:
 expect_empty=len(sys.argv)>4 and sys.argv[4]=='--expect-no-metadata'
 result=subprocess.run([sys.argv[1],f'http://127.0.0.1:{server.server_port}/song',sys.argv[2]]+(['--expect-no-metadata'] if expect_empty else []),timeout=60)
 assert result.returncode==0
 report=json.loads((pathlib.Path(sys.argv[2])/'result.json').read_text())
 assert pathlib.Path(report['file']).read_bytes()==body,'Downloaded bytes changed'
 expected_source=sys.argv[4] if len(sys.argv)>4 else ('ID3v2.'+str(body[3]) if body[:3]==b'ID3' else 'ID3v1')
 assert (report['metadata']=={} if expect_empty else report['metadata']['Source']==expected_source),report
 print('PASS downloaded bytes unchanged')
finally:server.shutdown();server.server_close()
