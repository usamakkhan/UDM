from pathlib import Path
import os,json,subprocess,hashlib,time,collections
root=Path('[repo]');base=Path(os.environ['LOCALAPPDATA'])/'UDM-Development';area=root/'.media-cache/owned-prefix-checksum-qualification-9.81';area.mkdir(exist_ok=False)
sha=lambda p:hashlib.sha256(p.read_bytes()).hexdigest().upper()
build=root/'.media-cache/owned-prefix-checksum-build-9.81';b=json.loads((build/'build-result.json').read_text());s=json.loads((build/'suite-results.json').read_text());assert b['passed'] and s['passed'] and s['checks']==1559
assert all(sha(root/r['path'])==r['sha256'] for r in b['sources'])
old=root/'.media-cache/owned-prefix-checksum-baseline-9.80';previous=json.loads((old/'inputs.json').read_text());assert all(sha(Path(r['path']))==r['sha256'] for r in previous['coreObjects'])
scope={'__file__':str(old/'config.py')};exec((base/'finishing-8.34/config.py').read_text(encoding='utf-8-sig'),scope)
source=(root/'native/HttpPostIdentityTests.cpp').read_text();needle='str(job->data.at("FinishPhases"),"ServerHashMode")=="prefix-copy"';assert source.count(needle)==1
source=source.replace(needle,'str(job->data.at("FinishPhases"),"ServerHashMode").empty()').replace('The raw server checksum is computed during the existing tail copy','The frozen baseline has no combined prefix-copy checksum path')
actor=old/'Baseline.cpp';actor.write_text(source);exe=old/'release-native/Udm.PrefixBaseline.exe';env={**scope['env'],'TEMP':str(old/'objects'),'TMP':str(old/'objects')}
args=[str(scope['compiler']),*scope['common'],'/I'+str(root/'native'),'/I'+str(root/'native/vendor/brotli/c/include'),str(actor),*[r['path'] for r in previous['coreObjects']],str(root/'.media-cache/owned-post-checksum-build-9.80/App.res'),'/Fo'+str(old/'objects/Baseline.obj'),'/Fe'+str(exe),'/link',*scope['system'],'/SUBSYSTEM:CONSOLE','/DYNAMICBASE','/NXCOMPAT','/HIGHENTROPYVA','/INCREMENTAL:NO','/MANIFEST:NO']
with (old/'build-Baseline.log').open('w') as log:p=subprocess.run(args,cwd=root,env=env,stdout=log,stderr=subprocess.STDOUT)
assert p.returncode==0,(old/'build-Baseline.log').read_text()[-3000:]
identity=build/'release-native/Udm.HttpPostIdentityTests.exe';gzip=build/'release-native/Udm.HttpPostGzipTests.exe'
(area/'inputs.json').write_text(json.dumps({'driverSha256':sha(Path(__file__)),'baselineFixtureSha256':sha(actor),'baselineResourceSha256':sha(root/'.media-cache/owned-post-checksum-build-9.80/App.res'),'currentResourceSha256':sha(build/'App.res'),'baselineBinarySha256':sha(exe),'identityBinarySha256':sha(identity),'gzipBinarySha256':sha(gzip),'runnerSha256':sha(root/'tests/http-post-identity.native-live.cjs'),'baselineDifference':'Only fixture expectation for absent ServerHashMode; all production objects and resource frozen 9.80'},indent=2))
runs=[]
plans=[('baseline-D-large',area/'baseline-D-large','http-post-identity.native-live.cjs',exe,'prefix-all',True),('current-D-small',area/'current-D-small','http-post-identity.native-live.cjs',identity,'prefix-all',False),('current-D-large',area/'current-D-large','http-post-identity.native-live.cjs',identity,'prefix-all',True),('current-C-large',base/'owned-prefix-checksum-qualification-9.81-C','http-post-identity.native-live.cjs',identity,'prefix-all',True),('existing-identity',area/'existing-identity','http-post-identity.native-live.cjs',identity,'',False),('existing-post-gzip',area/'existing-post-gzip','http-post-gzip.native-live.cjs',gzip,'',False),('digest-D',area/'digest-D','http-post-identity.native-live.cjs',identity,'all',False)]
for label,folder,runner,binary,mode,large in plans:
 assert not folder.exists();env={**os.environ,'UDM_POST_IDENTITY_TEST_EXE':str(binary),'UDM_POST_GZIP_TEST_EXE':str(gzip),'UDM_GZIP_ENCODING':'gzip'}
 for key in ['UDM_POST_DIGEST_CASE','UDM_POST_PREFIX_LARGE']:env.pop(key,None)
 if mode:env['UDM_POST_DIGEST_CASE']=mode
 if large:env['UDM_POST_PREFIX_LARGE']='1'
 with (area/(label+'.log')).open('w') as log:
  p=subprocess.Popen(['node',str(root/'tests'/runner),str(folder)],cwd=root,env=env,stdout=log,stderr=subprocess.STDOUT);(area/'active.json').write_text(json.dumps({'pid':p.pid,'case':label,'root':str(folder)},indent=2));code=p.wait()
 result=folder/'native/results.json'
 if not result.exists():raise RuntimeError((area/(label+'.log')).read_text(errors='replace')[-4000:])
 data=json.loads(result.read_text(encoding='utf-8-sig'));traffic=json.loads((folder/'requests.json').read_text());row={'case':label,'root':str(folder),'exitCode':code,'passed':code==0 and data['passed'],'assertions':len(data['checks']),'requests':len(traffic['requests']),'binarySha256':sha(binary),'result':str(result)}
 if mode:
  expected=collections.Counter()
  for obs in data['observations']:
   route=obs['route'];expected[route]=1
   if '-redirect-' in route:expected[route+'-get']=1
   if '-repost-' in route:expected[route+'-post']=1
  row['exactRequestCounts']=collections.Counter(r['route'] for r in traffic['requests'])==expected;row['cases']=len(data['observations']);row['passed'] &= row['exactRequestCounts']
 runs.append(row);(area/'results.json').write_text(json.dumps({'passed':all(r['passed'] for r in runs),'runs':runs},indent=2));print(json.dumps(row),flush=True)
 assert row['passed'],(area/(label+'.log')).read_text(errors='replace')[-4000:]
(area/'terminal.json').write_text(json.dumps({'passed':True,'allChildProcessesTerminal':True,'allServersClosed':True,'currentAssertions':sum(r['assertions'] for r in runs if not r['case'].startswith('baseline')),'baselineAssertions':sum(r['assertions'] for r in runs if r['case'].startswith('baseline')),'allCapturedInputsMatch':all(sha(root/r['path'])==r['sha256'] for r in b['sources'])},indent=2));print('Prefix checksum and existing form workflows complete',flush=True)
