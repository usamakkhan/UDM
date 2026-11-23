from pathlib import Path
import json,os,subprocess
base=Path(os.environ['LOCALAPPDATA'])/'UDM-Development';area=Path('<repo>/.media-cache/owned-advisory-intent-build-9.73');scope={'__file__':str(area/'config.py')};exec((base/'finishing-8.34/config.py').read_text(encoding='utf-8-sig'),scope);globals().update({k:v for k,v in scope.items() if not k.startswith('__')});common+=['/I'+str(native),'/I'+str(native/'vendor/brotli/c/include')]
b=json.loads((area/'build-result.json').read_text());s=json.loads((area/'suite-results.json').read_text());assert b['passed'] and s['passed'] and len(s['suites'])==21
for r in b['coreObjects']:assert sha(Path(r['path']))==r['sha256']
source=native/'WrittenBodyCacheTests.cpp';wanted=next(r['sha256'] for r in b['sources'] if r['path']=='native/WrittenBodyCacheTests.cpp');assert sha(source)==wanted
exe=release/'Udm.WrittenBodyCacheTests.exe';obj=out/'WrittenBodyCacheTests.obj';env={**env,'TEMP':str(root/'.media-cache/owned-advisory-intent-native-temp-9.73'),'TMP':str(root/'.media-cache/owned-advisory-intent-native-temp-9.73')}
args=[str(compiler),*common,str(source),*[r['path'] for r in b['coreObjects']],'/Fo'+str(obj),'/Fe'+str(exe),'/link',*system,'/SUBSYSTEM:CONSOLE','/DYNAMICBASE','/NXCOMPAT','/HIGHENTROPYVA','/INCREMENTAL:NO','/MANIFEST:NO']
(area/'body-budget-inputs.json').write_text(json.dumps(dict(sourceSha256=wanted,args=args),indent=2),encoding='utf-8')
with (area/'build-WrittenBodyCacheTests.log').open('w',encoding='utf-8') as log:p=subprocess.run(args,cwd=root,env=env,stdout=log,stderr=subprocess.STDOUT)
assert p.returncode==0
folder=root/'.media-cache/owned-advisory-body-budget-9.73';assert not folder.exists()
p=subprocess.run([str(exe),str(folder)],cwd=root,env=env,timeout=90);data=json.loads((folder/'results.json').read_text(encoding='utf-8-sig'));record=dict(passed=p.returncode==0 and data['passed'],checks=len(data['checks']),result=str(folder/'results.json'),sourceSha256=wanted,binaryPath=str(exe),binarySha256=sha(exe));(area/'body-budget-result.json').write_text(json.dumps(record,indent=2),encoding='utf-8');assert record['passed'],data;print(record['checks'],'body-budget/cache checks passed')
import sys
sys.path.insert(0,str(root/'benchmarks/speed'));from origin import Origin,Options,expected_sha256
source=native/'HttpSequentialHashTests.cpp';exe=release/'Udm.HttpSequentialHashTests.exe';obj=out/'HttpSequentialHashTests.obj';sourceHash=sha(source);args=[str(compiler),*common,str(source),*[r['path'] for r in b['coreObjects']],'/Fo'+str(obj),'/Fe'+str(exe),'/link',*system,'/SUBSYSTEM:CONSOLE','/DYNAMICBASE','/NXCOMPAT','/HIGHENTROPYVA','/INCREMENTAL:NO','/MANIFEST:NO']
(area/'required-hash-inputs.json').write_text(json.dumps(dict(sourceSha256=sourceHash,originSha256=sha(root/'benchmarks/speed/origin.py'),args=args),indent=2),encoding='utf-8')
with (area/'build-HttpSequentialHashTests.log').open('w',encoding='utf-8') as log:p=subprocess.run(args,cwd=root,env=env,stdout=log,stderr=subprocess.STDOUT)
assert p.returncode==0
rows=[]
for mode in ['plain','verified','mismatch','ranged']:
 folder=root/'.media-cache'/('owned-advisory-http-hash-9.73-'+mode);assert not folder.exists();origin=Origin(Options(size=8*1048576,rate=40_000_000,latency_ms=5,no_range=mode!='ranged',accept_ranges='none' if mode!='ranged' else None,seed=20261113))
 try:
  p=subprocess.run([str(exe),str(folder),origin.url,expected_sha256(8*1048576,20261113),mode],cwd=root,env=env,timeout=90);data=json.loads((folder/'results.json').read_text(encoding='utf-8-sig'));row=dict(mode=mode,passed=p.returncode==0 and data['passed'],checks=len(data['checks']),result=str(folder/'results.json'),origin=origin.snapshot());rows.append(row);(area/'required-hash-results.json').write_text(json.dumps(dict(passed=all(r['passed'] for r in rows),checks=sum(r['checks'] for r in rows),binaryPath=str(exe),binarySha256=sha(exe),sourceSha256=sourceHash,rows=rows),indent=2),encoding='utf-8');assert row['passed'],data;print(mode,row['checks'],'HTTP hash checks passed',flush=True)
 finally:origin.close()
