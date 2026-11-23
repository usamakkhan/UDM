from pathlib import Path
import os,json,subprocess
base=Path('<development>');root=Path('<repo>');area=root/'.media-cache/owned-retained-handoff-probe-9.75';assert not area.exists();area.mkdir()
scope={'__file__':str(area/'config.py')};exec((base/'finishing-8.34/config.py').read_text(encoding='utf-8-sig'),scope);globals().update({k:v for k,v in scope.items() if not k.startswith('__')});common+=['/I'+str(native),'/I'+str(native/'vendor/brotli/c/include'),'/I'+str(root/'.media-cache/owned-verified-handoff-baseline-9.74')]
source=root/'benchmarks/speed/retained_media_handoff.cpp';runs=[]
for version,baseline in [('9.74',True),('9.75',False)]:
 build=root/'.media-cache'/('owned-advisory-intent-build-9.74' if baseline else 'owned-verified-handoff-build-9.75');b=json.loads((build/'build-result.json').read_text());objects=[Path(r['path']) for r in b['coreObjects']]
 for r in b['coreObjects']:assert sha(Path(r['path']))==r['sha256']
 exe=release/('Udm.RetainedHandoff-'+version+'.exe');args=[str(compiler),*common,*(['/DUDM_BASELINE_MEDIA_HANDOFF'] if baseline else []),str(source),*map(str,objects),str(build/'App.res'),'/Fo'+str(out/('Probe-'+version+'.obj')),'/Fe'+str(exe),'/link',*system,'/SUBSYSTEM:CONSOLE','/INCREMENTAL:NO','/MANIFEST:NO']
 with (area/('build-'+version+'.log')).open('w') as log:p=subprocess.run(args,cwd=root,env=env,stdout=log,stderr=subprocess.STDOUT)
 assert p.returncode==0,(area/('build-'+version+'.log')).read_text()[-2500:]
 folder=area/('run-'+version);p=subprocess.run([str(exe),str(folder)],cwd=root,env=env,timeout=90);d=json.loads((folder/'results.json').read_text());assert len(d['checks'])==6 and not d['error'],d
 assert (p.returncode==1 and not d['passed'] and sum(not r['passed'] for r in d['checks'])==2) if baseline else (p.returncode==0 and d['passed']),d
 runs.append(dict(version=version,expectedResultConfirmed=True,sourceSha256=sha(source),executableSha256=sha(exe),result=str(folder/'results.json'),objects=b['coreObjects']));print(version+': expected '+('reproduction' if baseline else 'protection')+' confirmed',flush=True)
(area/'comparison.json').write_text(json.dumps(dict(passed=True,runs=runs),indent=2),encoding='utf-8')
