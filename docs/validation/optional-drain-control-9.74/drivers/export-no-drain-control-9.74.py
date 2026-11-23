from pathlib import Path
import json,hashlib,statistics,re
root=Path('<repo>');base=Path('<development>')
area=root/'.media-cache/owned-no-drain-control-9.74';work=root/'.media-cache/owned-no-drain-comparison-9.74';out=root/'docs/validation/optional-drain-control-9.74'
def read(p):return json.loads(p.read_text(encoding='utf-8-sig'))
def sha(p):return hashlib.sha256(p.read_bytes()).hexdigest().upper()
normal=read(root/'.media-cache/owned-advisory-intent-build-9.74/build-result.json')
for r in normal['sources']:assert sha(root/r['path'])==r['sha256']
retained=normal['coreObjects']+[r for r in normal['outputs'] if r['kind']=='product']
for r in retained:assert sha(Path(r['path']))==r['sha256']
protected=read(base/'publication-overlap-9.61-protected-before.json')
for r in protected:assert sha(Path(r['path']))==r['sha256']
b=read(area/'build.json');assert b['experimentalOnly'] and b['passed']
for r in b['outputs']:
 assert sha(Path(r['path']))==r['sha256']
 for o in r['objects']:assert sha(Path(o['path']))==o['sha256']
data=read(work/'results.json');rows=data['results'];assert len(rows)==6
assert read(area/'policy-verification.json')['passed']
assert all(r['hashMatches'] and r['launcherExitCode']==0 and r['finishPhases']['PublishedFlushError']==0 and r['finishPhases']['PublishedFileFlush']>0 for r in rows)
assert all(r['removed'] and not Path(r['path']).exists() for r in data['payloads'])
metrics=['firstAvailabilityTailSeconds','processExitTailSeconds','processExitWallSeconds','durableTailSeconds','durableWallSeconds']
medians={client:{k:statistics.median(r[k] for r in rows if r['client']==client)*1000 for k in metrics} for client in ['udm','baseline']}
pairs=[dict(trial=t,notSlower={k:next(r[k] for r in rows if r['trial']==t and r['client']=='udm')<=next(r[k] for r in rows if r['trial']==t and r['client']=='baseline') for k in metrics}) for t in range(1,4)]
report=dict(sourceVersion='9.74',extensionVersion='0.73.38',experimentalOnly=True,productionSourceChanged=False,comparison='No optional split drainage versus unchanged split drainage; common App object; mandatory final flush retained',mediansMs=medians,pairedEndpoints=pairs,admitted=False,reason='Disabled drainage loses every paired finishing and total endpoint; retain current policy.',payloadBytesRemoved=sum(r['bytes'] for r in data['payloads']),newIdmComparison=False)
(area/'analysis.json').write_text(json.dumps(report,indent=2),encoding='utf-8')
rea=base/'rea-idm-layout-9.74';response=read(rea/'inspect_binary_layout.json')['result'];assert response['isError']
failure=json.loads(response['content'][0]['text'])['error'];session=read(rea/'binary_session.json')['result']['structuredContent']['result']
static=dict(transportExitedCleanly=read(rea/'terminal.json')['serverExitCode']==0,layoutOperationPassed=False,layoutError=failure,deepProviders=session['analysis_provider_candidates'],targetExecuted=False,processAttached=False,artifactUnchanged=read(rea/'terminal.json')['artifactUnchanged'],importsAreNotCallOrder=True,warning='The original terminal passed flag describes RPC completion only, not successful layout analysis.')
(area/'rea-status.json').write_text(json.dumps(static,indent=2),encoding='utf-8')
out.mkdir(exist_ok=False);manifest=[]
def clean(text):
 for a,b in [(str(base),'<development>'),(str(root),'<repo>'),('<user>','<user>')]:
  for x in sorted({a,a.replace('\\','/'),a.replace('\\','\\\\'),a.replace('\\','\\\\\\\\')},key=len,reverse=True):text=text.replace(x,b)
 return text
def export(p,name):
 q=out/name;q.parent.mkdir(parents=True,exist_ok=True);q.write_text(clean(p.read_text(encoding='utf-8-sig',errors='replace')),encoding='utf-8');manifest.append(dict(path=name,sourceSha256=sha(p),exportSha256=sha(q)))
for p in area.iterdir():
 if p.is_file() and p.suffix in {'.json','.log','.patch'}:export(p,'control/'+p.name)
for name in ['results.json','terminal.json','idm-settings-original.json','idm-settings-restored.json']:export(work/name,'comparison/'+name)
for p in sorted(work.glob('split-64m-*/catalog/state.json')):export(p,'comparison/'+p.parent.parent.name+'/state.json')
for name in ['build-no-drain-control-9.74.py','run-no-drain-comparison-9.74.py','prepare-no-drain-comparison-9.74.py','owned-pair-derived-no-drain-9.74.py','owned-pair-no-drain-9.74-provenance.json','export-no-drain-control-9.74.py']:export(base/name,'drivers/'+name)
for name in ['terminal.json','inspect_binary_layout.json','pe-imports.txt']:export(rea/name,'rea/'+name)
for p in out.rglob('*'):
 if p.is_file():assert not re.search(r'C:(?:\\+|/)Users(?:\\+|/)Usama',p.read_text(encoding='utf-8')),p
(out/'export-manifest.json').write_text(json.dumps(manifest,indent=2),encoding='utf-8')
files=[]
for p in [area/'objects/App.obj',area/'objects/TransferNoDrain.obj',area/'release-native/UDM.ControlBaseline.exe',area/'release-native/UDM.NoOptionalDrain.exe']:
 assert p.resolve().is_relative_to(area.resolve()) and str(p) not in {r['path'] for r in retained+protected}
 files.append(dict(path=str(p),sha256=sha(p),bytes=p.stat().st_size))
(base/'optional-drain-cleanup-9.74.json').write_text(json.dumps(dict(allowedRoot=str(area),files=files),indent=2),encoding='utf-8')
print(json.dumps(dict(exports=len(manifest),mediansMs=medians,payloadBytesRemoved=report['payloadBytesRemoved'],disposableFiles=len(files),disposableLogicalBytes=sum(r['bytes'] for r in files)),indent=2))
