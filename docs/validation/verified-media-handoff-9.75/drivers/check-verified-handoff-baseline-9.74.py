from pathlib import Path
import json,subprocess,hashlib
base=Path('<development>');area=Path('<repo>/.media-cache/owned-verified-handoff-baseline-9.74')
scope={'__file__':str(area/'config.py')};exec((base/'finishing-8.34/config.py').read_text(encoding='utf-8-sig'),scope);globals().update({k:v for k,v in scope.items() if not k.startswith('__')})
(area/'BaselineMediaPublication.hpp').write_bytes((area/'MediaPublication.hpp').read_bytes())
b=json.loads((root/'.media-cache/owned-advisory-intent-build-9.74/build-result.json').read_text());objects=[Path(r['path']) for r in b['coreObjects']]
for r in b['coreObjects']:assert sha(Path(r['path']))==r['sha256']
exe=release/'Udm.VerifiedMediaHandoffBaseline.exe';command=[str(compiler),*common,'/I'+str(area),'/I'+str(native),'/I'+str(native/'vendor/brotli/c/include'),'/DUDM_BASELINE_MEDIA_HANDOFF',str(native/'VerifiedMediaHandoffTests.cpp'),*map(str,objects),str(root/'.media-cache/owned-advisory-intent-build-9.74/App.res'),'/Fo'+str(out/'Baseline.obj'),'/Fe'+str(exe),'/link',*system,'/SUBSYSTEM:CONSOLE','/INCREMENTAL:NO','/MANIFEST:NO']
with (area/'build.log').open('w') as log:p=subprocess.run(command,cwd=root,env=env,stdout=log,stderr=subprocess.STDOUT)
assert p.returncode==0,(area/'build.log').read_text()[-3000:]
folder=area/'probe';p=subprocess.run([str(exe),str(folder)],cwd=root,env=env,timeout=90);result=json.loads((folder/'results.json').read_text());assert p.returncode==1 and not result['passed'] and not result['error'] and len(result['checks'])==6 and all(not r['passed'] for r in result['checks']),result
(area/'baseline-reproduction.json').write_text(json.dumps(dict(reproduced=True,checks=6,allProtectionAssertionsFailed=True,executableSha256=sha(exe),sourceSha256=sha(native/'VerifiedMediaHandoffTests.cpp'),oldHeaderSha256=sha(area/'MediaPublication.hpp'),normalCoreObjects=b['coreObjects']),indent=2),encoding='utf-8')
print('Frozen 9.74 reproduces the verification-to-publication gap in both destination modes',flush=True)
