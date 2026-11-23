from pathlib import Path
import os,json,subprocess,hashlib,http.server,threading,base64
base=Path(os.environ['LOCALAPPDATA'])/'UDM-Development';area=Path('[repo]/.media-cache/owned-post-checksum-legacy-9.80')
scope={'__file__':str(area/'config.py')};exec((base/'finishing-8.34/config.py').read_text(encoding='utf-8-sig'),scope);globals().update({k:v for k,v in scope.items() if not k.startswith('__')});common+=['/I'+str(native),'/I'+str(native/'vendor/brotli/c/include')]
versions=[('baseline',root/'.media-cache/owned-fixed-volume-build-9.79'),('candidate',root/'.media-cache/owned-post-checksum-build-9.80')];source=area/'Legacy.cpp';records=[]
for name,build in versions:
 b=json.loads((build/'build-result.json').read_text());objects=[Path(r['path']) for r in b['coreObjects']]
 for row in b['coreObjects']:assert sha(Path(row['path']))==row['sha256']
 shared={r['path']:r['sha256'] for r in b['sources']}
 for p in ['native/Core.hpp','native/PostResponse.hpp']:assert sha(root/p)==shared[p]
 exe=area/(name+'.exe');inputs={'role':name,'sourceSha256':sha(source),'coreObjects':b['coreObjects'],'resourceSha256':sha(build/'App.res')};(area/(name+'-inputs.json')).write_text(json.dumps(inputs,indent=2))
 with (area/(name+'-build.log')).open('w') as log:p=subprocess.run([str(compiler),*common,str(source),*map(str,objects),str(build/'App.res'),'/Fo'+str(area/(name+'.obj')),'/Fe'+str(exe),'/link',*system,'/SUBSYSTEM:CONSOLE','/INCREMENTAL:NO','/MANIFEST:NO'],cwd=root,env=env,stdout=log,stderr=subprocess.STDOUT)
 assert p.returncode==0,(area/(name+'-build.log')).read_text(errors='replace')[-2000:]
 records.append({'role':name,'path':str(exe),'sha256':sha(exe)})
(area/'build.json').write_text(json.dumps({'passed':True,'sourceSha256':sha(source),'binaries':records},indent=2))
payload=b'Owned retained form checksum body\x00\xff'*17;expected=hashlib.sha256(payload).hexdigest();requests=[]
class Handler(http.server.BaseHTTPRequestHandler):
 def log_message(self,*args):pass
 def do_POST(self):
  data=self.rfile.read(int(self.headers.get('Content-Length',0)));assert data==b'owned';requests.append({'path':self.path,'bodyBytes':len(data),'bodySha256':hashlib.sha256(data).hexdigest()})
  self.send_response(200);self.send_header('Content-Type','application/octet-stream');self.send_header('Content-Length',str(len(payload)));self.send_header('Repr-Digest','sha-256=:'+base64.b64encode(bytes(32)).decode()+':');self.end_headers();self.wfile.write(payload)
server=http.server.ThreadingHTTPServer(('127.0.0.1',0),Handler);thread=threading.Thread(target=server.serve_forever);thread.start();observations=[]
def run(exe,mode,folder,url,label):
 with (area/(label+'.log')).open('w') as log:
  p=subprocess.Popen([str(exe),mode,str(folder),url],cwd=root,stdout=log,stderr=subprocess.STDOUT);(area/'active.json').write_text(json.dumps({'pid':p.pid,'stage':label}));code=p.wait()
 assert code==0,(area/(label+'.log')).read_text(errors='replace')[-2000:]
 result=json.loads((folder/(mode+'-result.json')).read_text());result.update(stage=label,requestsSeen=len(requests));observations.append(result);return result
try:
 old=Path(records[0]['path']);new=Path(records[1]['path'])
 for label in ['old-control','new-resume']:
  folder=area/label;url=f'http://127.0.0.1:{server.server_address[1]}/{label}'
  capture=run(old,'capture',folder,url,label+'-capture')
  assert capture['status']=='Failed' and not capture['published'] and capture['completedResponseCandidate'] and capture['rawSha256']==expected and capture['serverSha256']=='0'*64
  count=len(requests);resume=run(old if label=='old-control' else new,'resume',folder,url,label+'-resume');assert len(requests)==count,'Retained Resume must not send a POST again';assert resume['userSha256']==''
  if label=='old-control':assert resume['status']=='Complete' and resume['published'] and resume['publishedSha256']==expected
  else:assert resume['status']=='Failed' and not resume['published'] and 'checksum' in resume['error'] and resume['rawSha256']==capture['rawSha256'] and resume['reusedResponse'] and resume['completedResponseCandidate']
 (area/'qualification.json').write_text(json.dumps({'passed':True,'baselineBugReproducedAfterUserChecksumRemoval':True,'candidateRejectsSameLegacyResponseClass':True,'formRequests':len(requests),'expectedSha256':expected,'rawPayloadBytes':len(payload),'observations':observations,'requests':requests},indent=2));print('Legacy captures retained; 9.79 publishes after user-checksum removal, 9.80 rejects; zero replay requests',flush=True)
finally:
 server.shutdown();server.server_close();thread.join();(area/'server-terminal.json').write_text(json.dumps({'stopped':True,'requests':requests},indent=2))
