import http.server,threading,subprocess,sys,json,pathlib
requests=[]
pause_session="--cookie-pause" in sys.argv[3:]
cookie_session=pause_session or "--cookie-session" in sys.argv[3:]
deny_page="--deny-page" in sys.argv[3:]
deny_file="--deny-file" in sys.argv[3:]
redirect_page="--redirect-page" in sys.argv[3:]
unauthorized="--401" in sys.argv[3:]
class Handler(http.server.BaseHTTPRequestHandler):
 def do_HEAD(self):self.serve(False)
 def do_GET(self):self.serve(True)
 def serve(self,body):
  requests.append([self.command,self.path])
  if redirect_page and self.path=='/page':
   self.send_response(302);self.send_header('Location','/login');self.send_header('Content-Length','0');self.end_headers();return
  data=b'<!doctype html><title>Empty static page</title>' if self.path=='/page' else b'data'
  if self.path=='/child.html':data=b'<!doctype html><a href="/child.zip">Child file</a>'
  denied=(deny_page and self.path=='/page') or (deny_file and self.path=='/runtime.zip') or (redirect_page and self.path=='/login')
  if denied and self.path in ['/page','/login']:data=b'<a href="/error-body.zip">Error-page link</a>'
  self.send_response((401 if unauthorized else 403) if denied else 200);self.send_header('Set-Cookie','fixture=updated; Path=/') if cookie_session and self.path=='/page' else None;self.send_header('Content-Type','text/html' if self.path in ['/page','/child.html'] else 'application/zip');self.send_header('Content-Length',str(len(data)));self.end_headers()
  if body:self.wfile.write(data)
 def log_message(self,*args):pass
server=http.server.ThreadingHTTPServer(('127.0.0.1',0),Handler);thread=threading.Thread(target=server.serve_forever,daemon=True);thread.start()
try:
 result=subprocess.run([sys.argv[1],f'http://127.0.0.1:{server.server_port}/page']+(['--cookie-pause'] if pause_session else ['--cookie-session'] if cookie_session else ['--expect-empty'] if deny_file or redirect_page else []),capture_output=True,text=True,timeout=60);print(result.stdout,end='');print(result.stderr,end='');pathlib.Path(sys.argv[2]).write_text(json.dumps({'stdout':result.stdout,'stderr':result.stderr,'exitCode':result.returncode,'denyPage':deny_page,'denyFile':deny_file,'redirectPage':redirect_page,'unauthorized':unauthorized,'requests':requests},indent=2));assert result.returncode==0
 if pause_session:
  assert requests.count(['GET','/page'])==2,requests
  assert requests.count(['GET','/child.html'])==1,requests
  assert requests.count(['HEAD','/child.zip'])==1,requests
 assert any(p=='/runtime.zip' for _,p in requests)==(not redirect_page)
 assert not any(p=='/error-body.zip' for _,p in requests),requests
 assert not any(p=='/blocked.zip' for _,p in requests),requests
 # An include-pattern mismatch may be probed for a server-provided filename.
 # Explicit excludes must never be requested; a rejected candidate has no body fetch.
 assert all(method=='HEAD' for method,p in requests if p=='/wrong.pdf'),requests
 print('PASS explicit exclusions are not requested and rejected metadata candidates have no body fetch')
 pathlib.Path(sys.argv[2]).write_text(json.dumps({'stdout':result.stdout,'pauseResumeVerified':pause_session,'denyPage':deny_page,'denyFile':deny_file,'redirectPage':redirect_page,'unauthorized':unauthorized,'errorBodyIgnored':True,'requests':requests,'explicitExcludeNeverRequested':True,'rejectedCandidateBodyNotFetched':True},indent=2))
finally:server.shutdown();server.server_close()
