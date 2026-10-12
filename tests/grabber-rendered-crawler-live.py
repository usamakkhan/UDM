import http.server,threading,subprocess,sys,json,pathlib
requests=[]
class Handler(http.server.BaseHTTPRequestHandler):
 def do_HEAD(self):self.serve(False)
 def do_GET(self):self.serve(True)
 def serve(self,body):
  requests.append([self.command,self.path]);data=b'<!doctype html><title>Empty static page</title>' if self.path=='/page' else b'data'
  self.send_response(200);self.send_header('Content-Type','text/html' if self.path=='/page' else 'application/zip');self.send_header('Content-Length',str(len(data)));self.end_headers()
  if body:self.wfile.write(data)
 def log_message(self,*args):pass
server=http.server.ThreadingHTTPServer(('127.0.0.1',0),Handler);thread=threading.Thread(target=server.serve_forever,daemon=True);thread.start()
try:
 result=subprocess.run([sys.argv[1],f'http://127.0.0.1:{server.server_port}/page'],capture_output=True,text=True,timeout=60);print(result.stdout,end='');print(result.stderr,end='');assert result.returncode==0
 assert any(p=='/runtime.zip' for _,p in requests);assert not any(p in ['/blocked.zip','/wrong.pdf'] for _,p in requests)
 print('PASS excluded browser links never requested');pathlib.Path(sys.argv[2]).write_text(json.dumps({'stdout':result.stdout,'requests':requests,'excludedNeverRequested':True},indent=2))
finally:server.shutdown();server.server_close()
