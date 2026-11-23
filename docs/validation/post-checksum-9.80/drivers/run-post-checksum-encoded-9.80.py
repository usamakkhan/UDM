from pathlib import Path
import os,json,subprocess,hashlib,http.server,threading,base64,gzip
base=Path(os.environ['LOCALAPPDATA'])/'UDM-Development';area=Path('[repo]/.media-cache/owned-post-checksum-encoded-9.80');area.mkdir(exist_ok=False);(area/'.udm-build-owned').write_text('Owned gzip checksum byte-domain controls')
scope={'__file__':str(area/'config.py')};exec((base/'finishing-8.34/config.py').read_text(encoding='utf-8-sig'),scope);globals().update({k:v for k,v in scope.items() if not k.startswith('__')});common+=['/I'+str(native),'/I'+str(native/'vendor/brotli/c/include')]
original=root/'.media-cache/owned-post-checksum-legacy-9.80/Legacy.cpp';text=original.read_text();text=text.replace('if(mode=="capture")','if(mode=="capture"||mode=="download")');old='std::string(64,\'0\'),Json{{"method","POST"}';new='(mode=="capture"?std::string(64,\'0\'):std::string{}),Json{{"method","POST"}';assert text.count(old)==1;text=text.replace(old,new);text=text.replace('Json result={{"mode",mode}', 'Json result={{"wireReceiptInvalidated",yes(job->data,"HttpPartsResetPending")},{"postAttempted",yes(job->data,"PostAttempted")},{"mode",mode}')
source=area/'Encoded.cpp';source.write_text(text);records=[]
for name,build in [('baseline',root/'.media-cache/owned-fixed-volume-build-9.79'),('candidate',root/'.media-cache/owned-post-checksum-build-9.80')]:
 b=json.loads((build/'build-result.json').read_text());objects=[Path(r['path']) for r in b['coreObjects']]
 for row in b['coreObjects']:assert sha(Path(row['path']))==row['sha256']
 exe=area/(name+'.exe');(area/(name+'-inputs.json')).write_text(json.dumps({'sourceSha256':sha(source),'coreObjects':b['coreObjects'],'resourceSha256':sha(build/'App.res')},indent=2))
 with (area/(name+'-build.log')).open('w') as log:p=subprocess.run([str(compiler),*common,str(source),*map(str,objects),str(build/'App.res'),'/Fo'+str(area/(name+'.obj')),'/Fe'+str(exe),'/link',*system,'/SUBSYSTEM:CONSOLE','/INCREMENTAL:NO','/MANIFEST:NO'],cwd=root,env=env,stdout=log,stderr=subprocess.STDOUT)
 assert p.returncode==0,(area/(name+'-build.log')).read_text(errors='replace')[-2000:]
 records.append({'role':name,'path':str(exe),'sha256':sha(exe)})
(area/'build.json').write_text(json.dumps({'passed':True,'sourceSha256':sha(source),'derivedFromSourceSha256':sha(original),'binaries':records},indent=2))
payload=b'Owned gzip form byte-domain fixture\x00\xff'*41;wire=gzip.compress(payload,mtime=0);decodedHash=hashlib.sha256(payload).hexdigest();wireHash=hashlib.sha256(wire).hexdigest();assert decodedHash!=wireHash;requests=[]
class Handler(http.server.BaseHTTPRequestHandler):
 def log_message(self,*args):pass
 def do_POST(self):
  body=self.rfile.read(int(self.headers.get('Content-Length',0)));assert body==b'owned';requests.append({'method':'POST','path':self.path,'bodySha256':hashlib.sha256(body).hexdigest()})
  if self.path.startswith('/redirect'):
   self.send_response(303);self.send_header('Content-Length','0');self.send_header('Location','/final'+self.path);self.end_headers();return
  self.respond()
 def do_GET(self):requests.append({'method':'GET','path':self.path,'bodySha256':hashlib.sha256(b'').hexdigest()});self.respond()
 def respond(self):
  self.send_response(200);self.send_header('Content-Type','application/octet-stream');self.send_header('Content-Encoding','gzip');self.send_header('Content-Length',str(len(wire)));self.send_header('Repr-Digest','sha-256=:'+base64.b64encode(bytes(32) if self.path.endswith('bad') else bytes.fromhex(wireHash)).decode()+':');self.end_headers();self.wfile.write(wire)
server=http.server.ThreadingHTTPServer(('127.0.0.1',0),Handler);thread=threading.Thread(target=server.serve_forever);thread.start();observations=[]
try:
 for record in records:
  role=record['role']
  for route in ['direct-good','direct-bad','redirect-good','redirect-bad']:
   folder=area/(role+'-'+route);url=f'http://127.0.0.1:{server.server_address[1]}/{route}';before=len(requests)
   with (area/(role+'-'+route+'.log')).open('w') as log:
    p=subprocess.Popen([record['path'],'download',str(folder),url],cwd=root,stdout=log,stderr=subprocess.STDOUT);(area/'active.json').write_text(json.dumps({'pid':p.pid,'case':role+'-'+route}));code=p.wait()
   assert code==0,(area/(role+'-'+route+'.log')).read_text(errors='replace')[-2000:]
   result=json.loads((folder/'download-result.json').read_text());bad=route.endswith('bad');shouldReject=bad and not(role=='baseline' and route=='redirect-bad')
   if shouldReject:assert result['status']=='Failed' and not result['published'] and 'encoded HTTP response' in result['error'] and result['wireReceiptInvalidated'] and result['postAttempted'] and result['rawSha256']==wireHash,result
   else:assert result['status']=='Complete' and result['published'] and result['publishedSha256']==decodedHash,result
   assert result['userSha256']==''
   if not bad:assert result['serverSha256']==wireHash,result
   assert len(requests)-before==(2 if route.startswith('redirect') else 1)
   result.update(role=role,route=route,expectedRejection=shouldReject,requests=len(requests)-before);observations.append(result)
 (area/'qualification.json').write_text(json.dumps({'passed':True,'wireSha256':wireHash,'decodedSha256':decodedHash,'wireBytes':len(wire),'decodedBytes':len(payload),'baselineRedirectChecksumBugReproduced':True,'candidateAllFourOutcomesCorrect':True,'observations':observations,'requests':requests},indent=2));print('Gzip checksums cover wire bytes; old redirected checksum bypass reproduced, all four candidate outcomes pass',flush=True)
finally:
 server.shutdown();server.server_close();thread.join();(area/'server-terminal.json').write_text(json.dumps({'stopped':True,'requests':requests},indent=2))
