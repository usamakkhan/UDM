from pathlib import Path
import json,subprocess,threading,time,http.server,re,datetime
base=Path('<development>');root=Path('<repo>');area=root/'.media-cache/owned-dynamic-dash-workflow-9.76';assert json.loads((area/'terminal-epoch2.json').read_text())['exitCode']==1
scope={'__file__':str(area/'config.py')};exec((base/'finishing-8.34/config.py').read_text(encoding='utf-8-sig'),scope);globals().update({k:v for k,v in scope.items() if not k.startswith('__')})
protected=json.loads((base/'publication-overlap-9.61-protected-before.json').read_text());inputs=json.loads((area/'inputs-epoch2.json').read_text());assets=area/'assets';exe=release/'Udm.DynamicDashWorkflowEpoch2.exe';assert sha(exe)==json.loads((area/'terminal-epoch2.json').read_text())['binarySha256'];assert sha(root/'benchmarks/speed/dynamic_dash_workflow.cpp')==inputs['fixtureSha256']
for r in inputs['assetHashes']:assert sha(assets/r['path'])==r['sha256']
for r in protected:assert sha(Path(r['path']))==r['sha256']
(area/'epoch3-inputs.json').write_text(json.dumps(dict(runnerSha256=sha(Path(__file__)),binarySha256=sha(exe),sameAssets=True,sameFrameOracles=True,correction='The owned dynamic SegmentList now declares its explicit SegmentTimeline, as required by the live parser. No production change or recompilation.'),indent=2),encoding='utf-8')
initial=base/'run-dynamic-dash-workflow-9.76.py';body='requests=[]'+initial.read_text(encoding='utf-8').split('requests=[]',1)[1];old='<Initialization sourceURL=';assert body.count(old)==1;body=body.replace(old,'<SegmentTimeline><S t="0" d="1000"/></SegmentTimeline><Initialization sourceURL=',1)
for old,new in [("area/'run'","area/'run-epoch3'"),('owned-dynamic-dash-workflow-9.76-C','owned-dynamic-dash-workflow-9.76-C-epoch3'),("area/'runtime.log'","area/'runtime-epoch3.log'"),("area/'process.json'","area/'process-epoch3.json'"),("area/'requests.json'","area/'requests-epoch3.json'"),("area/'terminal.json'","area/'terminal-epoch3.json'"),("area/'frame-oracle.json'","area/'frame-oracle-epoch3.json'")]:body=body.replace(old,new)
exec(body,globals())
