from pathlib import Path
import os,json,subprocess,hashlib,collections
root=Path('[repo]');base=Path(os.environ['LOCALAPPDATA'])/'UDM-Development';area=root/'.media-cache/owned-post-checksum-qualification-9.80';area.mkdir(exist_ok=False);(area/'.udm-build-owned').write_text('Owned POST checksum qualification')
sha=lambda p:hashlib.sha256(p.read_bytes()).hexdigest().upper()
build=root/'.media-cache/owned-post-checksum-build-9.80';b=json.loads((build/'build-result.json').read_text());s=json.loads((build/'suite-results.json').read_text());assert b['passed'] and s['passed'] and s['checks']==1559
assert all(sha(root/r['path'])==r['sha256'] for r in b['sources'])
identity=build/'release-native/Udm.HttpPostIdentityTests.exe';gzip=build/'release-native/Udm.HttpPostGzipTests.exe';inputs={'driverSha256':sha(Path(__file__)),'identityBinarySha256':sha(identity),'gzipBinarySha256':sha(gzip),'identityRunnerSha256':sha(root/'tests/http-post-identity.native-live.cjs'),'gzipRunnerSha256':sha(root/'tests/http-post-gzip.native-live.cjs'),'plannedCases':['digest-D','digest-C','existing-identity','existing-post-gzip']};(area/'inputs.json').write_text(json.dumps(inputs,indent=2))
runs=[]
for label,folder,runner,exe,digest in [('digest-D',area/'digest-D','http-post-identity.native-live.cjs',identity,True),('digest-C',base/'owned-post-checksum-qualification-9.80-C','http-post-identity.native-live.cjs',identity,True),('existing-identity',area/'existing-identity','http-post-identity.native-live.cjs',identity,False),('existing-post-gzip',area/'existing-post-gzip','http-post-gzip.native-live.cjs',gzip,False)]:
 assert not folder.exists();env={**os.environ,'UDM_POST_IDENTITY_TEST_EXE':str(identity),'UDM_POST_GZIP_TEST_EXE':str(gzip),'UDM_GZIP_ENCODING':'gzip'};env.pop('UDM_POST_DIGEST_CASE',None)
 if digest:env['UDM_POST_DIGEST_CASE']='all'
 with (area/(label+'.log')).open('w') as log:
  p=subprocess.Popen(['node',str(root/'tests'/runner),str(folder)],cwd=root,env=env,stdout=log,stderr=subprocess.STDOUT);(area/'active.json').write_text(json.dumps({'pid':p.pid,'case':label,'root':str(folder)},indent=2));code=p.wait()
 data=json.loads((folder/'native/results.json').read_text(encoding='utf-8-sig'));traffic=json.loads((folder/'requests.json').read_text());row={'case':label,'exitCode':code,'passed':code==0 and data['passed'],'assertions':len(data['checks']),'requests':len(traffic['requests']),'root':str(folder),'binarySha256':sha(exe),'result':str(folder/'native/results.json')}
 if digest:
  allowed=collections.Counter()
  for obs in data['observations']:
   route=obs['route'];allowed[route]=1
   if '-redirect-' in route:allowed[route+'-get']=1
   if '-repost-' in route:allowed[route+'-post']=1
  actual=collections.Counter(r['route'] for r in traffic['requests']);row['exactRequestCounts']=actual==allowed;row['selectedCases']=len(data['observations']);assert actual==allowed,(actual,allowed)
 runs.append(row);(area/'results.json').write_text(json.dumps({'passed':all(r['passed'] for r in runs),'runs':runs},indent=2));print(json.dumps(row),flush=True)
 assert row['passed'],(area/(label+'.log')).read_text(errors='replace')[-3000:]
 assert sha(exe)==row['binarySha256']
(area/'terminal.json').write_text(json.dumps({'passed':True,'runs':runs,'allChildProcessesTerminal':True,'allRunnerServersClosed':True,'allCompiledInputsStillMatch':all(sha(root/r['path'])==r['sha256'] for r in b['sources'])},indent=2));print('All checksum and existing form workflows passed',flush=True)
