from pathlib import Path
import json,subprocess
base=Path('<development>');root=Path('<repo>');area=root/'.media-cache/owned-dash-output-probe-9.76';assert not area.exists();area.mkdir()
scope={'__file__':str(area/'config.py')};exec((base/'finishing-8.34/config.py').read_text(encoding='utf-8-sig'),scope);globals().update({k:v for k,v in scope.items() if not k.startswith('__')})
fixture=root/'benchmarks/speed/dash_output_ownership.cpp';old=root/'.media-cache/owned-dash-output-baseline-9.75';records=[];snapshots=[]
for version,baseline in [('9.75',True),('9.76',False)]:
 build=root/'.media-cache'/('owned-verified-handoff-build-9.75' if baseline else 'owned-dash-output-build-9.76');b=json.loads((build/'build-result.json').read_text());objects=[Path(r['path']) for r in b['coreObjects'] if Path(r['path']).stem!='DashManifest'];assert len(objects)==41
 if baseline:
  for name in ['DashManifest.cpp','DashManifest.hpp','LiveDash.hpp','PublicationSourcePin.hpp','MediaPublication.hpp']:
   expected=next(r['sha256'] for r in b['sources'] if r['path']=='native/'+name);assert sha(old/name)==expected;snapshots.append(dict(path='native/'+name,sha256=expected))
 for r in b['coreObjects']:assert sha(Path(r['path']))==r['sha256']
 source=fixture
 if baseline:
  source=area/'baseline-probe.cpp';source.write_text('#define UDM_DASH_BASELINE_SOURCE "'+(old/'DashManifest.cpp').as_posix()+'"\n#include "'+fixture.as_posix()+'"\n',encoding='utf-8')
 exe=release/('Udm.DashOutput-'+version+'.exe');args=[str(compiler),*common,*(['/I'+str(old)] if baseline else []),'/I'+str(native),'/I'+str(native/'vendor/brotli/c/include'),str(source),*map(str,objects),str(build/'App.res'),'/Fo'+str(out/('Probe-'+version+'.obj')),'/Fe'+str(exe),'/link',*system,'/SUBSYSTEM:CONSOLE','/INCREMENTAL:NO','/MANIFEST:NO']
 with (area/('build-'+version+'.log')).open('w') as log:p=subprocess.run(args,cwd=root,env=env,stdout=log,stderr=subprocess.STDOUT)
 assert p.returncode==0,(area/('build-'+version+'.log')).read_text()[-3500:]
 cases=[('D',[])] if baseline else [('D',[]),('C',[str(base/'owned-dash-output-probe-9.76-C')])]
 for drive,extra in cases:
  folder=area/('run-'+version+'-'+drive);p=subprocess.run([str(exe),str(folder),*extra],cwd=root,env=env,timeout=90);d=json.loads((folder/'results.json').read_text());assert not d['error'],d
  assert (p.returncode==1 and len(d['checks'])==6 and sum(not r['passed'] for r in d['checks'])==4) if baseline else (p.returncode==0 and d['passed'] and len(d['checks'])==8),d
  records.append(dict(version=version,drive=drive,expectedResultConfirmed=True,result=str(folder/'results.json'),binarySha256=sha(exe),fixtureSha256=sha(fixture),objects=[r for r in b['coreObjects'] if Path(r['path']).stem!='DashManifest']));print(version+' '+drive+': expected '+('failure reproduced' if baseline else 'ownership/copy checks passed'),flush=True)
(area/'comparison.json').write_text(json.dumps(dict(passed=True,records=records,frozenBaselineSources=snapshots),indent=2),encoding='utf-8')
