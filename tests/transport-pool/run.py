from pathlib import Path
import base64,hashlib,json,socket,socketserver,select,threading,http.server,subprocess,time,sys
import argparse
parser=argparse.ArgumentParser(description='Isolated UDM HTTP connection-pool acceptance')
parser.add_argument('--probe',type=Path,required=True)
parser.add_argument('--output',type=Path,required=True)
args=parser.parse_args();probe=args.probe.resolve(strict=True);run=args.output.resolve();run.mkdir(parents=True)
payload=bytes((i*31+7)%251 for i in range(384*1024+731));requests=[];routes=[];errors=[]
def digest(b):return hashlib.sha256(b).hexdigest()
class Origin(http.server.BaseHTTPRequestHandler):
 protocol_version='HTTP/1.1'
 def log_message(self,*args):pass
 def do_HEAD(self):self.respond(True)
 def do_GET(self):self.respond(False)
 def do_POST(self):self.respond(False)
 def respond(self,head):
  try:
   body=self.rfile.read(int(self.headers.get('Content-Length','0')))
   requests.append(dict(path=self.path,method=self.command,body=base64.b64encode(body).decode(),headers=dict(self.headers),connection=self.client_address[1]))
   if self.path=='/stall':time.sleep(1);return
   if self.path=='/context':data=json.dumps({'cookie':self.headers.get('Cookie'),'authorization':self.headers.get('Authorization')}).encode();status=200
   elif self.path=='/echo':data=body;status=200
   elif self.path=='/truncated':data=b'short';status=200
   elif self.path=='/redirect':data=b'';status=302
   else:data=payload;status=200
   content_range=None
   if self.headers.get('Range') and self.path=='/file':
    bounds=self.headers['Range'].removeprefix('bytes=').split('-');a=int(bounds[0]);b=int(bounds[1]) if bounds[1] else len(data)-1
    content_range=f'bytes {a}-{b}/{len(data)}';data=data[a:b+1];status=206
   self.send_response(status);self.send_header('Content-Length',str(500 if self.path=='/truncated' else len(data)));self.send_header('Connection','close' if self.path in ('/truncated','/close') else 'keep-alive')
   self.send_header('Set-Cookie','a=1');self.send_header('Set-Cookie','b=2')
   if content_range:self.send_header('Content-Range',content_range)
   if status==302:self.send_header('Location','/file')
   self.end_headers()
   if not head:self.wfile.write(data)
  except (BrokenPipeError,ConnectionResetError):pass
origin=http.server.ThreadingHTTPServer(('127.0.0.1',0),Origin);threading.Thread(target=origin.serve_forever,daemon=True).start()
allowed={('127.0.0.1',origin.server_port)}
hosts=['httpbingo.org','sha256.badssl.com','self-signed.badssl.com','wrong.host.badssl.com','github.com','release-assets.githubusercontent.com']
for host in hosts:
 for info in socket.getaddrinfo(host,443,type=socket.SOCK_STREAM):allowed.add((info[4][0],443))
def exact(s,n):
 data=b''
 while len(data)<n:
  block=s.recv(n-len(data))
  if not block:raise EOFError()
  data+=block
 return data
class Socks(socketserver.BaseRequestHandler):
 def handle(self):
  upstream=None
  try:
   s=self.request;s.settimeout(10);v,n=exact(s,2);methods=exact(s,n)
   if v!=5 or 0 not in methods:raise ValueError('Invalid SOCKS greeting')
   s.sendall(b'\x05\x00');v,cmd,reserved,kind=exact(s,4)
   if v!=5 or cmd!=1 or reserved:raise ValueError('Invalid SOCKS request')
   if kind==1:host=socket.inet_ntop(socket.AF_INET,exact(s,4))
   elif kind==4:host=socket.inet_ntop(socket.AF_INET6,exact(s,16))
   else:raise ValueError('This fixture expects local DNS and a literal address')
   port=int.from_bytes(exact(s,2),'big')
   if (host,port) not in allowed:raise ValueError('Target is outside the explicit fixture allowlist')
   routes.append(dict(host=host,port=port,type=kind))
   upstream=socket.create_connection((host,port),timeout=10);s.sendall(b'\x05\x00\x00\x01\x7f\x00\x00\x01\x00\x00')
   deadline=time.monotonic()+40
   while time.monotonic()<deadline:
    readable,_,_=select.select([s,upstream],[],[],0.25)
    for source in readable:
     data=source.recv(65536)
     if not data:return
     (upstream if source is s else s).sendall(data)
  except (EOFError,ConnectionResetError,BrokenPipeError):pass
  except Exception as e:errors.append(str(e))
  finally:
   if upstream:upstream.close()
class Server(socketserver.ThreadingTCPServer):allow_reuse_address=False;daemon_threads=True
socks=Server(('127.0.0.1',0),Socks);threading.Thread(target=socks.serve_forever,daemon=True).start()

settings={'ProxyMode':'Use a SOCKS5 proxy','Proxy':f'127.0.0.1:{socks.server_address[1]}','CapturedProxyDNS':False,'ProxyBypass':'','UseTls13':True,'UserAgent':'UDM-Pool-Acceptance'}
local=f'http://127.0.0.1:{origin.server_port}';checks=[];runs=[]
def check(name,ok):
 checks.append(dict(name=name,passed=bool(ok)))
 print(('PASS ' if ok else 'FAIL ')+name,flush=True)
def case(id,url,session='shared',**extra):return dict(id=id,url=url,settings=settings,session=session,**extra)
def group(id,cases,parallel=False):return dict(id=id,cases=cases,parallel=parallel)
def execute(name,groups):
 folder=run/name;folder.mkdir();(folder/'spec.json').write_text(json.dumps(dict(groups=groups),indent=2))
 a=len(routes);b=len(requests);started=time.monotonic()
 with (folder/'probe.log').open('w') as log:
  r=subprocess.run([str(probe),str(folder/'spec.json'),str(folder/'results.json')],stdout=log,stderr=subprocess.STDOUT,creationflags=0x08000000,timeout=100)
 assert r.returncode==0,(name,r.returncode)
 result=json.loads((folder/'results.json').read_text());record=dict(name=name,seconds=time.monotonic()-started,routes=routes[a:],requests=requests[b:],results=result);runs.append(record)
 (folder/'observations.json').write_text(json.dumps(record,indent=2))
 return {c['id']:c for c in result['cases']},record
public=bytes(97+i%26 for i in range(65536));form=b'one_use=%00%FF&value=UDM'
try:
 by,record=execute('h1-sequential',[group('sequential',[case('first',local+'/file'),case('second',local+'/file',start=200,end=50000),case('head',local+'/file',head=True),case('third',local+'/file')])])
 check('HTTP/1.1 sequential GET/range/HEAD reuse exactly one TCP connection',len(record['routes'])==1 and len({q['connection'] for q in record['requests']})==1 and all(c.get('ok') for c in by.values()))
 check('Reused HTTP/1.1 bodies and range are byte-exact',by['first'].get('sha256')==digest(payload) and by['second'].get('sha256')==digest(payload[200:50001]) and by['head'].get('bytes')==0 and by['third'].get('sha256')==digest(payload))
 by,record=execute('h1-post-isolation',[group('requests',[case('warm',local+'/file'),case('post',local+'/echo',body=base64.b64encode(form).decode()),case('after',local+'/file')])])
 check('POST has a dedicated connection and is submitted exactly once',len(record['routes'])==2 and len([q for q in record['requests'] if q['method']=='POST'])==1 and by['post'].get('sha256')==digest(form))
 qs=record['requests'];check('GET pool survives the separate POST',len(qs)==3 and qs[0]['connection']==qs[2]['connection'] and qs[1]['connection']!=qs[0]['connection'] and by['after'].get('sha256')==digest(payload))
 by,record=execute('session-isolation',[group('requests',[case('one',local+'/file',session='one'),case('two',local+'/file',session='two'),case('one-again',local+'/file',session='one')])])
 qs=record['requests'];check('Independent HttpSession instances do not share connections',len(record['routes'])==2 and len(qs)==3 and qs[0]['connection']==qs[2]['connection'] and qs[1]['connection']!=qs[0]['connection'])
 by,record=execute('h1-failure',[group('warm',[case('warm',local+'/file')]),group('concurrent',[case('cancel',local+'/stall',cancelAfterMs=300),case('truncated',local+'/truncated'),case('peer',local+'/file'),case('cookie-a',local+'/context',headers={'Cookie':'a=one','Authorization':'Bearer fixture-a'}),case('cookie-b',local+'/context',headers={'Cookie':'b=two','Authorization':'Bearer fixture-b'})],True),group('after',[case('after',local+'/file'),case('close',local+'/close'),case('after-close',local+'/file')])])
 check('One canceled stalled request does not cancel its peer',by['cancel'].get('cancelled') and by['cancel']['elapsedMs']<1500 and by['peer'].get('sha256')==digest(payload))
 check('Truncated request failure does not poison the session',not by['truncated'].get('ok') and by['after'].get('sha256')==digest(payload))
 check('Server Connection: close is followed by a successful new connection',by['close'].get('sha256')==digest(payload) and by['after-close'].get('sha256')==digest(payload))
 for key,cookie,auth in [('cookie-a','a=one','Bearer fixture-a'),('cookie-b','b=two','Bearer fixture-b')]:
  context=json.loads(Path(by[key]['output']).read_text()) if by[key].get('ok') else {}
  check(key+' keeps request authentication and cookies separate',context==dict(cookie=cookie,authorization=auth))
 by,record=execute('concurrent-cancellation',[group('sixteen-stalls',[case('cancel-'+str(i),local+'/stall',cancelAfterMs=400) for i in range(16)]+[case('peer',local+'/file')],True),group('after',[case('after',local+'/file')])])
 check('Sixteen concurrent stalled requests all cancel within two seconds',all(by['cancel-'+str(i)].get('cancelled') and by['cancel-'+str(i)]['elapsedMs']<2000 for i in range(16)))
 check('Concurrent cancellation leaves peer and subsequent requests intact',by['peer'].get('sha256')==digest(payload) and by['after'].get('sha256')==digest(payload))
 by,record=execute('h2-multiplex',[group('warm',[case('warm','https://httpbingo.org/range/65536')]),group('streams',[case('drip-a','https://httpbingo.org/drip?numbytes=128&duration=4&delay=0'),case('drip-b','https://httpbingo.org/drip?numbytes=128&duration=4&delay=0'),case('cancel','https://httpbingo.org/drip?numbytes=1024&duration=6&delay=0',cancelAfterMs=2000),case('slow-reader','https://httpbingo.org/range/65536',delayReadMs=1500),case('range','https://httpbingo.org/range/65536',start=1234,end=9999)],True),group('after',[case('after','https://httpbingo.org/range/65536')])])
 check('Seven HTTP/2 requests including overlapping streams use one TCP connection',len(record['routes'])==1 and all(c.get('protocol')=='HTTP/2' for c in by.values()))
 check('Two four-second streams run concurrently on that connection',record['results']['groups'][1]['elapsedMs']<7000 and all(by[k].get('sha256')==digest(b'*'*128) for k in ('drip-a','drip-b')))
 check('Canceling one HTTP/2 stream preserves the other streams',by['cancel'].get('cancelled') and by['cancel']['elapsedMs']<3500 and all(by[k].get('ok') for k in ('drip-a','drip-b','slow-reader','range','after')))
 check('A delayed reader preserves exact bytes without blocking peers',by['slow-reader'].get('sha256')==digest(public) and by['range'].get('sha256')==digest(public[1234:10000]) and by['after'].get('sha256')==digest(public))
 by,record=execute('tls-failure',[group('warm',[case('warm','https://httpbingo.org/range/65536')]),group('mixed',[case('bad','https://self-signed.badssl.com/'),case('peer','https://httpbingo.org/range/65536')],True),group('after',[case('after','https://httpbingo.org/range/65536')])])
 check('TLS validation failure stays local to the failed request',not by['bad'].get('ok') and 'certificate' in by['bad'].get('error','').lower() and by['peer'].get('sha256')==digest(public) and by['after'].get('sha256')==digest(public))
 check('All proxy destinations remain on the test allowlist',not errors)
finally:
 origin.shutdown();origin.server_close();socks.shutdown();socks.server_close()
 (run/'checks.json').write_text(json.dumps(dict(checks=checks,passed=sum(c['passed'] for c in checks),failed=sum(not c['passed'] for c in checks),runs=runs,proxyErrors=errors,probeSha256=digest((probe).read_bytes())),indent=2))
print(json.dumps(dict(path=str(run),passed=sum(c['passed'] for c in checks),failed=[c['name'] for c in checks if not c['passed']]),indent=2),flush=True)
raise SystemExit(1 if any(not c['passed'] for c in checks) else 0)
