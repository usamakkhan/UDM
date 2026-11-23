from pathlib import Path
import json,os,subprocess
base=Path(os.environ['LOCALAPPDATA'])/'UDM-Development';area=Path('<repo>/.media-cache/owned-advisory-intent-build-9.74');scope={'__file__':str(area/'config.py')};exec((base/'finishing-8.34/config.py').read_text(encoding='utf-8-sig'),scope);globals().update({k:v for k,v in scope.items() if not k.startswith('__')});common+=['/I'+str(native),'/I'+str(native/'vendor/brotli/c/include')]
b=json.loads((area/'build-result.json').read_text());s=json.loads((area/'suite-results.json').read_text());assert b['passed'] and s['passed'] and len(s['suites'])==21
for r in b['coreObjects']:assert sha(Path(r['path']))==r['sha256']
source=native/'WrittenBodyCacheTests.cpp';wanted=next(r['sha256'] for r in b['sources'] if r['path']=='native/WrittenBodyCacheTests.cpp');assert sha(source)==wanted
exe=release/'Udm.WrittenBodyCacheTests.exe';obj=out/'WrittenBodyCacheTests.obj';env={**env,'TEMP':str(root/'.media-cache/owned-advisory-intent-native-temp-9.74'),'TMP':str(root/'.media-cache/owned-advisory-intent-native-temp-9.74')}
args=[str(compiler),*common,str(source),*[r['path'] for r in b['coreObjects']],'/Fo'+str(obj),'/Fe'+str(exe),'/link',*system,'/SUBSYSTEM:CONSOLE','/DYNAMICBASE','/NXCOMPAT','/HIGHENTROPYVA','/INCREMENTAL:NO','/MANIFEST:NO']
(area/'body-budget-inputs.json').write_text(json.dumps(dict(sourceSha256=wanted,args=args),indent=2),encoding='utf-8')
with (area/'build-WrittenBodyCacheTests.log').open('w',encoding='utf-8') as log:p=subprocess.run(args,cwd=root,env=env,stdout=log,stderr=subprocess.STDOUT)
assert p.returncode==0
folder=root/'.media-cache/owned-advisory-body-budget-9.74';assert not folder.exists()
p=subprocess.run([str(exe),str(folder)],cwd=root,env=env,timeout=90);data=json.loads((folder/'results.json').read_text(encoding='utf-8-sig'));record=dict(passed=p.returncode==0 and data['passed'],checks=len(data['checks']),result=str(folder/'results.json'),sourceSha256=wanted,binaryPath=str(exe),binarySha256=sha(exe));(area/'body-budget-result.json').write_text(json.dumps(record,indent=2),encoding='utf-8');assert record['passed'],data;print(record['checks'],'body-budget/cache checks passed')

