from pathlib import Path
import json,subprocess,threading,time,http.server,re,datetime
base=Path('<development>');root=Path('<repo>');area=root/'.media-cache/owned-dynamic-dash-workflow-9.76';assert area.exists() and not (area/'run').exists() and not (area/'process.json').exists()
scope={'__file__':str(area/'config.py')};exec((base/'finishing-8.34/config.py').read_text(encoding='utf-8-sig'),scope);globals().update({k:v for k,v in scope.items() if not k.startswith('__')});common+=['/I'+str(native),'/I'+str(native/'vendor/brotli/c/include')]
build=root/'.media-cache/owned-dash-output-build-9.76';b=json.loads((build/'build-result.json').read_text());protected=json.loads((base/'publication-overlap-9.61-protected-before.json').read_text());inputs=json.loads((area/'inputs.json').read_text());assets=area/'assets';source=root/'benchmarks/speed/dynamic_dash_workflow.cpp';exe=release/'Udm.DynamicDashWorkflow.exe'
for r in b['sources']:assert sha(root/r['path'])==r['sha256']
for r in b['coreObjects']+protected:assert sha(Path(r['path']))==r['sha256']
for r in inputs['assetHashes']:assert sha(assets/r['path'])==r['sha256']
initial=base/'run-dynamic-dash-workflow-9.76.py';(area/'driver-initial.py').write_bytes(initial.read_bytes());(area/'inputs-initial-build.json').write_bytes((area/'inputs.json').read_bytes());(area/'build.log').rename(area/'build-initial.log');inputs['fixtureSha256']=sha(source);inputs['resumeRunnerSha256']=sha(Path(__file__));inputs['correction']='Fixture now uses public Manager::resume; initial private-start invocation failed compilation before any origin or native actor started. Assets, frame oracle and production core unchanged.';(area/'inputs.json').write_text(json.dumps(inputs,indent=2),encoding='utf-8')
body=initial.read_text(encoding='utf-8').split("with (area/'build.log').open('w') as log:",1)[1];exec("with (area/'build.log').open('w') as log:"+body,globals())
