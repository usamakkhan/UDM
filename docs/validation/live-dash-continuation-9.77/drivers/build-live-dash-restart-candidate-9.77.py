from pathlib import Path
import json,subprocess
base=Path('<development>');root=Path('<repo>');area=root/'.media-cache/owned-live-dash-restart-candidate-9.77';assert not area.exists();area.mkdir()
scope={'__file__':str(area/'config.py')};exec((base/'finishing-8.34/config.py').read_text(encoding='utf-8-sig'),scope);globals().update({k:v for k,v in scope.items() if not k.startswith('__')});common+=['/I'+str(native),'/I'+str(native/'vendor/brotli/c/include')]
build=root/'.media-cache/owned-live-dash-continuation-build-9.77';b=json.loads((build/'build-result.json').read_text());suites=json.loads((build/'suite-results.json').read_text());assert b['passed'] and b['version']=='9.77' and suites['passed'] and suites['checks']==1532 and len(suites['suites'])==22
for r in b['sources']:assert sha(root/r['path'])==r['sha256']
for r in b['coreObjects']:assert sha(Path(r['path']))==r['sha256']
source=root/'benchmarks/speed/live_dash_restart_workflow.cpp';exe=release/'Udm.LiveDashRestartCandidate.exe'
with (area/'build.log').open('w') as log:p=subprocess.run([str(compiler),*common,str(source),*[r['path'] for r in b['coreObjects']],str(build/'App.res'),'/Fo'+str(out/'LiveDashRestartCandidate.obj'),'/Fe'+str(exe),'/link',*system,'/SUBSYSTEM:CONSOLE','/INCREMENTAL:NO','/MANIFEST:NO'],cwd=root,env=env,stdout=log,stderr=subprocess.STDOUT)
assert p.returncode==0,(area/'build.log').read_text()[-3500:]
tools=release/'tools';p=subprocess.run(['powershell','-NoProfile','-Command',"New-Item -ItemType Junction -Path '"+str(tools)+"' -Target 'C:/Program Files/UDM/tools' | Out-Null"],capture_output=True,text=True);assert p.returncode==0,p.stderr
record=dict(version='9.77',path=str(exe),sha256=sha(exe),fixtureSha256=sha(source),objects=b['coreObjects'],productionInputs=b['sources']);(area/'build.json').write_text(json.dumps(record,indent=2),encoding='utf-8');signal=root/'.media-cache/owned-live-dash-restart-9.76/resume-candidate.json';assert not signal.exists();signal.write_text(json.dumps(record,indent=2),encoding='utf-8');print('Candidate linked to the validated 9.77 core; continuation signal published',flush=True)
