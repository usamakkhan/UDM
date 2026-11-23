from pathlib import Path
import os,json,subprocess
base=Path(os.environ['LOCALAPPDATA'])/'UDM-Development';area=Path('D:/UDM/.media-cache/owned-finishing-setup-probe-9.77')
scope={'__file__':str(area/'config.py')};exec((base/'finishing-8.34/config.py').read_text(encoding='utf-8-sig'),scope)
globals().update({k:v for k,v in scope.items() if not k.startswith('__')})
common+=['/I'+str(area/'source'),'/I'+str(native),'/I'+str(native/'vendor/brotli/c/include')];env={**env,'TEMP':str(area/'temp'),'TMP':str(area/'temp')}
assert not (area/'build.json').exists()
entry=area/'source/Actor.cpp';old=entry.read_bytes();(area/'source/Actor.initial.cpp').write_bytes(old)
entry.write_text(old.decode().replace('#include <iostream>','#include <iostream>\n#include <fstream>'),encoding='utf-8')
inputs=json.loads((area/'inputs.json').read_text());(area/'inputs-initial.json').write_bytes((area/'inputs.json').read_bytes())
for row in inputs['privateSources']:
 if Path(row['path'])==entry:row['sha256']=sha(entry)
 else:assert sha(Path(row['path']))==row['sha256']
inputs['fixtureCorrection']={'reason':'Missing direct fstream include in private console fixture','initialActorSha256':sha(area/'source/Actor.initial.cpp'),'correctedActorSha256':sha(entry),'correctedBeforeAnyActorLaunch':True,'resumeDriverSha256':sha(Path(__file__))}
(area/'inputs.json').write_text(json.dumps(inputs,indent=2))
normal=root/'.media-cache/owned-live-dash-continuation-build-9.77';build=json.loads((normal/'build-result.json').read_text())
for row in build['sources']:assert sha(root/row['path'])==row['sha256']
for row in build['coreObjects']:assert sha(Path(row['path']))==row['sha256']
objects=[out/'Transfer.obj' if Path(row['path']).stem in ['Transfer','DefaultTransfer'] else Path(row['path']) for row in build['coreObjects']]
exe=release/'Udm.WorkerProbe.exe'
args=[str(compiler),*common,'/DUDM_FINISH_TIMING_TEST',str(entry),*map(str,objects),str(normal/'App.res'),'/Fo'+str(out/'Actor.obj'),'/Fe'+str(exe),'/link',*system,'/SUBSYSTEM:CONSOLE','/INCREMENTAL:NO','/MANIFEST:NO']
with (area/'Actor-corrected.log').open('w') as log:p=subprocess.run(args,cwd=root,env=env,stdout=log,stderr=subprocess.STDOUT)
assert p.returncode==0,(area/'Actor-corrected.log').read_text(errors='replace')[-3000:]
assert all(sha(root/row['path'])==row['sha256'] for row in build['sources'])
assert all(sha(Path(row['path']))==row['sha256'] for row in inputs['privateSources'])
(area/'build.json').write_text(json.dumps({'passed':True,'diagnosticOnly':True,'binary':str(exe),'sha256':sha(exe),'privateSources':inputs['privateSources'],'coreObjects':[{'path':str(p),'sha256':sha(p)} for p in objects],'fixtureCorrection':inputs['fixtureCorrection']},indent=2))
print('Corrected actual worker probe built; production unchanged',flush=True)
