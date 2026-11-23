from pathlib import Path
import json,subprocess,threading,time,http.server,re,datetime
base=Path('<development>');root=Path('<repo>');area=root/'.media-cache/owned-dynamic-dash-workflow-9.76';assert json.loads((area/'terminal.json').read_text())['exitCode']==1
scope={'__file__':str(area/'config.py')};exec((base/'finishing-8.34/config.py').read_text(encoding='utf-8-sig'),scope);globals().update({k:v for k,v in scope.items() if not k.startswith('__')});common+=['/I'+str(native),'/I'+str(native/'vendor/brotli/c/include')]
build=root/'.media-cache/owned-dash-output-build-9.76';b=json.loads((build/'build-result.json').read_text());protected=json.loads((base/'publication-overlap-9.61-protected-before.json').read_text());inputs=json.loads((area/'inputs.json').read_text());assets=area/'assets';source=root/'benchmarks/speed/dynamic_dash_workflow.cpp';exe=release/'Udm.DynamicDashWorkflowEpoch2.exe'
for r in b['sources']:assert sha(root/r['path'])==r['sha256']
for r in b['coreObjects']+protected:assert sha(Path(r['path']))==r['sha256']
for r in inputs['assetHashes']:assert sha(assets/r['path'])==r['sha256']
initial=base/'run-dynamic-dash-workflow-9.76.py';(area/'inputs-queued-control.json').write_bytes((area/'inputs.json').read_bytes());inputs['fixtureSha256']=sha(source);inputs['epoch2RunnerSha256']=sha(Path(__file__));inputs['epoch2Correction']='Invoke public queue dispatcher after Resume; initial runtime stayed Queued and sent zero requests. No production code, asset or frame-oracle change.';(area/'inputs-epoch2.json').write_text(json.dumps(inputs,indent=2),encoding='utf-8')
body="with (area/'build.log').open('w') as log:"+initial.read_text(encoding='utf-8').split("with (area/'build.log').open('w') as log:",1)[1]
for old,new in [("area/'build.log'","area/'build-epoch2.log'"),('DynamicDashWorkflow.obj','DynamicDashWorkflowEpoch2.obj'),("area/'run'","area/'run-epoch2'"),('owned-dynamic-dash-workflow-9.76-C','owned-dynamic-dash-workflow-9.76-C-epoch2'),("area/'runtime.log'","area/'runtime-epoch2.log'"),("area/'process.json'","area/'process-epoch2.json'"),("area/'requests.json'","area/'requests-epoch2.json'"),("area/'terminal.json'","area/'terminal-epoch2.json'"),("area/'frame-oracle.json'","area/'frame-oracle-epoch2.json'")]:body=body.replace(old,new)
exec(body,globals())
