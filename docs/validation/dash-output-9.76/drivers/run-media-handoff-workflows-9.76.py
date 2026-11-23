from pathlib import Path
import os,json,subprocess
base=Path('<development>');root=Path('<repo>');area=root/'.media-cache/owned-media-handoff-workflows-9.76';assert not area.exists();area.mkdir()
scope={'__file__':str(area/'config.py')};exec((base/'finishing-8.34/config.py').read_text(encoding='utf-8-sig'),scope);globals().update({k:v for k,v in scope.items() if not k.startswith('__')});common+=['/I'+str(native),'/I'+str(native/'vendor/brotli/c/include')]
source=base/'media-handoff-workflows-9.76.cpp';build=root/'.media-cache/owned-dash-output-build-9.76';b=json.loads((build/'build-result.json').read_text());objects=[Path(r['path']) for r in b['coreObjects']]
for r in b['coreObjects']:assert sha(Path(r['path']))==r['sha256']
protected=json.loads((base/'publication-overlap-9.61-protected-before.json').read_text());helpers=[r for r in protected if Path(r['path']).name in ['ffmpeg.exe','ffprobe.exe']];assert len(helpers)==2
for r in helpers:assert sha(Path(r['path']))==r['sha256']
tools=release/'tools';assert not tools.exists();p=subprocess.run(['powershell','-NoProfile','-Command',"New-Item -ItemType Junction -Path '"+str(tools)+"' -Target 'C:/Program Files/UDM/tools' | Out-Null"],capture_output=True,text=True);assert p.returncode==0,p.stderr
cross=base/'owned-media-handoff-workflows-9.76-C';assert not cross.exists();cross.mkdir();env={**env,'UDM_MEDIA_CROSS_VOLUME_TEST_ROOT':str(cross)}
exe=release/'Udm.MediaHandoffWorkflows.exe';args=[str(compiler),*common,str(source),*map(str,objects),str(build/'App.res'),'/Fo'+str(out/'MediaHandoffWorkflows.obj'),'/Fe'+str(exe),'/link',*system,'/SUBSYSTEM:CONSOLE','/INCREMENTAL:NO','/MANIFEST:NO']
with (area/'build.log').open('w') as log:p=subprocess.run(args,cwd=root,env=env,stdout=log,stderr=subprocess.STDOUT)
assert p.returncode==0,(area/'build.log').read_text()[-2500:]
folder=area/'run';p=subprocess.run([str(exe),str(folder)],cwd=root,env=env,timeout=180);d=json.loads((folder/'results.json').read_text());assert p.returncode==0 and d['passed'],d
for r in helpers:assert sha(Path(r['path']))==r['sha256']
(area/'terminal.json').write_text(json.dumps(dict(passed=True,checks=len(d['checks']),sourceSha256=sha(source),executableSha256=sha(exe),objects=b['coreObjects'],result=str(folder/'results.json'),sharedHelpers=helpers,sharedToolsJunction=str(tools),copiedMediaHelpers=False,crossRoot=str(cross)),indent=2),encoding='utf-8');print('Media workflow and complete EOF decoding: '+str(len(d['checks']))+' passed',flush=True)
