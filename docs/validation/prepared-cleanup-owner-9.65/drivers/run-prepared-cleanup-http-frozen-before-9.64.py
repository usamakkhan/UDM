from pathlib import Path
import os,json,subprocess,re
base=Path(os.environ['LOCALAPPDATA'])/'UDM-Development';area=Path('<repo>/.media-cache/owned-prepared-cleanup-http-frozen-before-9.64');area.mkdir(exist_ok=False);(area/'.udm-build-owned').write_text('Owned frozen-header 9.64 real HTTP negative control',encoding='utf-8');scope={'__file__':str(area/'config.py')};exec((base/'finishing-8.34/config.py').read_text(encoding='utf-8-sig'),scope);globals().update({k:v for k,v in scope.items() if not k.startswith('__')});baseline=json.loads((root/'.media-cache/owned-publication-failure-owner-build-9.64/build-result.json').read_text());expected={r['path']:r['sha256'] for r in baseline['sources']};frozen=area/'frozen-native';frozen.mkdir();seen=set();records=[]
def original(p):
 s=p.read_text(encoding='utf-8-sig')
 if p.name=='PublicationSourcePin.hpp':s=s.replace(' const Json& capturedIdentity()const noexcept{return stable;}\n','')
 if p.name=='PreparedTransferCommit.hpp':
  s=s.replace('Json destination,cleanupIdentity;','Json destination;').replace('cleanupIdentity=std::move(identity);','')
  a=s.index('  // Do not query the synchronous file object');b=s.index('  Lock lock(manager.mutex);if(!str(job->data',a);s=s[:a]+s[b:]
  a=s.index('  if(ready&&!accepted)try{');b=s.index('  current=previous;',a);s=s[:a]+'  if(ready&&!accepted)try{if(!publicationNeedsRecovery(manager,job)){std::error_code error;fs::remove(staging,error);}}catch(...){/* Unknown recovery state retains the staged file. */}\n'+s[b:]
 if p.name=='Version.hpp':s=s.replace('9,65,0,0','9,64,0,0').replace('9.65','9.64')
 return s
# Hashes include original CRLF encoding, so reconstruct bytes by matching the baseline.
def capture(p):
 p=p.resolve()
 if p in seen:return
 seen.add(p);rel=p.relative_to(native.resolve());text=original(p);target=frozen/rel;target.parent.mkdir(parents=True,exist_ok=True);key=p.relative_to(root).as_posix();variants=[text.encode('utf-8'),text.replace('\n','\r\n').encode('utf-8')]
 raw=p.read_bytes()
 if sha(p)==expected.get(key):data=raw
 else:
  data=next((v for v in variants if __import__('hashlib').sha256(v).hexdigest().upper()==expected.get(key)),None);assert data is not None,key
 target.write_bytes(data);assert sha(target)==expected[key],key;records.append(dict(path=key,sha256=sha(target)))
 for name in re.findall(r'^\s*#\s*include\s*"([^"\n]+)"',text,re.M):
  child=next((x for x in [p.parent/name,native/name,root/name] if x.is_file()),None)
  if child and child.resolve().is_relative_to(native.resolve()):capture(child)
capture(native/'PreparedTransferCommit.hpp');fixture=native/'PreparedCleanupHttpTests.cpp';client=area/fixture.name;client.write_bytes(fixture.read_bytes());common+=['/I'+str(frozen),'/I'+str(native),'/I'+str(native/'vendor/brotli/c/include')];objects=[Path(r['path']) for r in baseline['coreObjects']];assert all(sha(p)==r['sha256'] for p,r in zip(objects,baseline['coreObjects']));exe=release/'Udm.PreparedCleanupHttpTests.exe';(area/'inputs.json').write_text(json.dumps(dict(fixtureSha256=sha(fixture),frozenHeaders=records,coreObjects=baseline['coreObjects'],allDefinitionsMatchFrozen964=True),indent=2),encoding='utf-8')
with (area/'compile.log').open('w',encoding='utf-8') as log:p=subprocess.run([str(compiler),*common,str(client),*map(str,objects),'/Fo'+str(out/'PreparedCleanupHttpTests.obj'),'/Fe'+str(exe),'/link',*system,'/SUBSYSTEM:CONSOLE','/INCREMENTAL:NO'],cwd=root,env=env,stdout=log,stderr=subprocess.STDOUT)
assert p.returncode==0,(area/'compile.log').read_text()[-3000:]
p=subprocess.run([str(exe),str(area/'owned-cases')],cwd=root,env=env,capture_output=True,text=True,timeout=90);data=json.loads((area/'owned-cases/results.json').read_text(encoding='utf-8-sig'));(area/'negative-control.json').write_text(json.dumps(dict(expectedFailure=True,exitCode=p.returncode,binarySha256=sha(exe),result=data),indent=2),encoding='utf-8');assert p.returncode==1 and not data['passed'] and not data['error'];print('Frozen-header/core HTTP control reproduced all four foreign-name losses; fixture SHA',sha(fixture),'headers',len(records))
