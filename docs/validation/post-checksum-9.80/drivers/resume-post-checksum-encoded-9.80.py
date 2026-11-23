from pathlib import Path
import json,hashlib,subprocess,http.server,threading,base64,gzip
root=Path('[repo]');area=root/'.media-cache/owned-post-checksum-encoded-9.80';base=Path('[local-diagnostics]');sha=lambda p:hashlib.sha256(p.read_bytes()).hexdigest().upper()
build=json.loads((area/'build.json').read_text());records=build['binaries'];assert sha(area/'Encoded.cpp')==build['sourceSha256']
for r in records:assert sha(Path(r['path']))==r['sha256']
initial=json.loads((area/'server-terminal.json').read_text());assert initial['stopped'];(area/'server-terminal-initial.json').write_bytes((area/'server-terminal.json').read_bytes());(area/'active-initial.json').write_bytes((area/'active.json').read_bytes())
text=(base/'run-post-checksum-encoded-9.80.py').read_text();snippet=text[text.index("payload=b'Owned gzip"):text.index('try:\n for record in records:')]
exec(snippet,globals());priorRequests=initial['requests'];observations=[];oldRequestIndex=0;reused=0;launched=0
try:
 for record in records:
  role=record['role']
  for route in ['direct-good','direct-bad','redirect-good','redirect-bad']:
   folder=area/(role+'-'+route);resultFile=folder/'download-result.json';needed=2 if route.startswith('redirect') else 1
   if resultFile.exists():
    result=json.loads(resultFile.read_text());trace=priorRequests[oldRequestIndex:oldRequestIndex+needed];oldRequestIndex+=needed;reused+=1
   else:
    assert not folder.exists();url=f'http://127.0.0.1:{server.server_address[1]}/{route}';before=len(requests)
    with (area/(role+'-'+route+'.log')).open('w') as log:
     p=subprocess.Popen([record['path'],'download',str(folder),url],cwd=root,stdout=log,stderr=subprocess.STDOUT);(area/'active.json').write_text(json.dumps({'pid':p.pid,'case':role+'-'+route}));code=p.wait()
    assert code==0,(area/(role+'-'+route+'.log')).read_text(errors='replace')[-2000:]
    result=json.loads(resultFile.read_text());trace=requests[before:];launched+=1
   bad=route.endswith('bad');oldRedirect=role=='baseline' and route.startswith('redirect');reject=bad and not oldRedirect
   if reject:assert result['status']=='Failed' and not result['published'] and 'encoded HTTP response' in result['error'] and result['wireReceiptInvalidated'] and result['postAttempted'] and result['rawSha256']==wireHash,result
   else:assert result['status']=='Complete' and result['published'] and result['publishedSha256']==decodedHash,result
   assert result['userSha256']=='' and len(trace)==needed
   if not bad:assert result['serverSha256']==('' if oldRedirect else wireHash),result
   expectedMethods=['POST','GET'] if route.startswith('redirect') else ['POST'];expectedPaths=['/'+route,'/final/'+route] if route.startswith('redirect') else ['/'+route]
   assert [r['method'] for r in trace]==expectedMethods and [r['path'] for r in trace]==expectedPaths,trace
   result.update(role=role,route=route,expectedRejection=reject,requests=trace);observations.append(result)
 assert oldRequestIndex==len(priorRequests) and reused==3 and launched==5
 (area/'qualification.json').write_text(json.dumps({'passed':True,'wireSha256':wireHash,'decodedSha256':decodedHash,'wireBytes':len(wire),'decodedBytes':len(payload),'baselineRedirectChecksumBugReproduced':True,'candidateAllFourOutcomesCorrect':True,'completedInitialFlowsReused':reused,'remainingFlowsLaunched':launched,'observations':observations},indent=2))
 (area/'assessment-correction.json').write_text(json.dumps({'initialControllerSha256':sha(base/'run-post-checksum-encoded-9.80.py'),'resumedControllerSha256':sha(Path(__file__)),'reason':'The old redirected path legitimately has no recorded ServerSha256: this is the missing-hasher defect being reproduced. The initial controller incorrectly required the field on its matching-checksum baseline control.','initialActorsAlreadyTerminal':3,'completedFlowsRerun':0,'remainingFlowsRun':5,'productCodeChanged':False},indent=2));print('Gzip wire-checksum controls pass; all three initial flows retained and only five unfinished flows run',flush=True)
finally:
 server.shutdown();server.server_close();thread.join();(area/'server-terminal-resumed.json').write_text(json.dumps({'stopped':True,'requests':requests},indent=2))
