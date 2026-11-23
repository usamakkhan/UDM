from pathlib import Path
import os,json,subprocess,copy
base=Path(os.environ['LOCALAPPDATA'])/'UDM-Development';area=Path('<repo>/.media-cache/owned-staging-admission-build-9.67');scope={'__file__':str(area/'config.py')};exec((base/'finishing-8.34/config.py').read_text(encoding='utf-8-sig'),scope);globals().update({k:v for k,v in scope.items() if not k.startswith('__')});common+=['/I'+str(native),'/I'+str(native/'vendor/brotli/c/include')]
b=json.loads((area/'build-result.json').read_text());initialSuites=json.loads((area/'suite-results.json').read_text());assert not initialSuites['passed'] and initialSuites['suites'][-1]['suite']=='StagingLinkReuseTests';fixture=native/'StagingLinkReuseTests.cpp';correct=root/'.media-cache/owned-staging-reuse-corrected-probe-9.67/StagingLinkReuseTests.cpp';expected=next(r['sha256'] for r in b['sources'] if r['path']=='native/StagingLinkReuseTests.cpp');assert sha(fixture)==expected
for r in b['sources']:assert sha(root/r['path'])==r['sha256']
for r in b['coreObjects']+b['outputs']:assert sha(Path(r['path']))==r['sha256']
(area/'build-result.initial-fixture.json').write_bytes((area/'build-result.json').read_bytes());(area/'suite-results.initial-fixture.json').write_bytes((area/'suite-results.json').read_bytes());(area/'StagingLinkReuseTests.initial-fixture.cpp').write_bytes(fixture.read_bytes());fixture.write_bytes(correct.read_bytes());correction=dict(path='native/StagingLinkReuseTests.cpp',beforeSha256=expected,afterSha256=sha(fixture),reason='Initial test omitted a valid ETag/range/segment resume plan, causing correct transport reset of its seeded partial file. Corrected fixture retains the intended file identity under a real valid resume.',capturedBeforeCorrectedCompilation=True,noProductionSourceChanged=True);(area/'source-correction.json').write_text(json.dumps(correction,indent=2),encoding='utf-8')
for r in b['sources']:
 if r['path']==correction['path']:r['sha256']=correction['afterSha256']
objects=[Path(r['path']) for r in b['coreObjects']];exe=release/'Udm.StagingLinkReuseTests.exe'
with (area/'build-StagingLinkReuseTests-corrected.log').open('w',encoding='utf-8') as log:p=subprocess.run([str(compiler),*common,str(fixture),*map(str,objects),str(area/'App.res'),'/Fo'+str(out/'StagingLinkReuseTests.obj'),'/Fe'+str(exe),'/link',*system,'/SUBSYSTEM:CONSOLE','/DYNAMICBASE','/NXCOMPAT','/HIGHENTROPYVA','/INCREMENTAL:NO','/MANIFEST:NO'],cwd=root,env=env,stdout=log,stderr=subprocess.STDOUT)
assert p.returncode==0,(area/'build-StagingLinkReuseTests-corrected.log').read_text()[-2000:]
for r in b['outputs']:
 if r['source']=='StagingLinkReuseTests':r['sha256']=sha(exe)
b['sourceCorrections']=[correction];b['productionSourcesUnchangedAfterOriginalCompilation']=True;(area/'build-result.json').write_text(json.dumps(b,indent=2),encoding='utf-8');runs=[]
for r in b['outputs']:
 if r['kind']!='backend':continue
 folder=root/'.media-cache'/('owned-staging-admission-final-9.67-D-'+r['source']);assert not folder.exists()
 with (area/('run-final-'+r['source']+'.log')).open('w',encoding='utf-8') as log:p=subprocess.run([r['path'],str(folder)],cwd=root,env=env,stdout=log,stderr=subprocess.STDOUT,timeout=120)
 data=json.loads((folder/'results.json').read_text(encoding='utf-8-sig'));row=dict(suite=r['source'],drive='D',passed=p.returncode==0 and data['passed'],checks=len(data['checks']),binarySha256=sha(Path(r['path'])),result=str(folder/'results.json'));runs.append(row);(area/'suite-results.json').write_text(json.dumps(dict(passed=all(r['passed'] for r in runs),checks=sum(r['checks'] for r in runs),suites=runs),indent=2),encoding='utf-8');print(r['source']+': '+str(row['checks'])+'; '+str(row['passed']),flush=True);assert row['passed'],data
assert len(runs)==16 and sum(r['checks'] for r in runs)==1333
croot=base/'owned-staging-admission-final-9.67-C';assert not croot.exists();croot.mkdir();(croot/'.udm-build-owned').write_text('Owned tiny C-volume staging validation',encoding='utf-8');cruns=[]
for name in ['StagingAdmissionTests','StagingLinkReuseTests']:
 exe=release/('Udm.'+name+'.exe');folder=croot/name;p=subprocess.run([str(exe),str(folder)],cwd=root,env=env,capture_output=True,text=True,timeout=90);data=json.loads((folder/'results.json').read_text(encoding='utf-8-sig'));assert p.returncode==0 and data['passed'];cruns.append(dict(suite=name,drive='C',checks=len(data['checks']),passed=True,result=str(folder/'results.json'),binarySha256=sha(exe)))
(area/'c-drive-results.json').write_text(json.dumps(dict(passed=True,checks=sum(r['checks'] for r in cruns),suites=cruns),indent=2),encoding='utf-8')
for r in b['sources']:assert sha(root/r['path'])==r['sha256']
(area/'terminal.json').write_text(json.dumps(dict(passed=True,version='9.67',nativeProducts=5,backendSuites=16,backendAssertions=1333,additionalCDriveAssertions=18,sourceFixtureCorrectedWithoutProductionChanges=True,installedFilesChanged=False),indent=2),encoding='utf-8');print('Full native build and corrected staging/recovery qualification passed',flush=True)
