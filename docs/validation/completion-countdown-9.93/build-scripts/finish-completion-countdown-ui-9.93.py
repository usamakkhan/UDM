from pathlib import Path
import json,os,subprocess
base=Path(os.environ['LOCALAPPDATA'])/'UDM-Development';area=base/'owned-completion-countdown-build-9.93';scope={'__file__':str(area/'config.py')};exec((base/'finishing-8.34/config.py').read_text(encoding='utf-8-sig'),scope);globals().update({k:v for k,v in scope.items() if not k.startswith('__')});record=json.loads((area/'build-result.json').read_text(encoding='utf-8'));oldSources={e['path']:e['sha256'] for e in record['sources']};changed=[p for p,h in oldSources.items() if sha(root/p)!=h];assert changed==['native/CompletionCountdownUiTests.cpp'],changed
inputs={'capturedBeforeCompilation':True,'changedSources':changed,'sourceSha256':sha(native/'CompletionCountdownUiTests.cpp'),'previousSha256':oldSources[changed[0]],'reason':'EndDialog destroys modal window only after callback return; verify disappearance after DoModal unwinds while the writer remains held.','normalProductsUnchanged':[{k:e[k] for k in ['name','sha256']} for e in record['outputs'] if e['kind']=='product']};(area/'final-ui-inputs.json').write_text(json.dumps(inputs,indent=2),encoding='utf-8')
objects=[Path(e['path']) for e in record['coreObjects']];assert all(sha(p)==e['sha256'] for p,e in zip(objects,record['coreObjects']));exe=release/'Udm.CompletionCountdownUiTests.exe';flags=[x for x in common if x!='/O2']+['/Od','/I'+str(native),'/I'+str(native/'vendor/brotli/c/include')]
with (area/'build-CompletionCountdownUiTests-final.log').open('w',encoding='utf-8') as log:p=subprocess.run([str(compiler),*flags,str(native/'CompletionCountdownUiTests.cpp'),*map(str,objects),str(area/'App.res'),'/Fo'+str(out/'CompletionCountdownUiTests.obj'),'/Fe'+str(exe),'/link',*system,'/ENTRY:wWinMainCRTStartup','/SUBSYSTEM:WINDOWS','/DYNAMICBASE','/NXCOMPAT','/HIGHENTROPYVA','/INCREMENTAL:NO','/MANIFEST:NO'],cwd=root,env=env,stdout=log,stderr=subprocess.STDOUT)
assert p.returncode==0
for e in record['sources']:e['sha256']=sha(root/e['path'])
for e in record['outputs']:
 if e['name']==exe.name:e['sha256']=sha(exe)
 else:assert sha(Path(e['path']))==e['sha256']
record['finalUiInputs']='final-ui-inputs.json';(area/'build-result.json').write_text(json.dumps(record,indent=2),encoding='utf-8')
oldResults=json.loads((area/'suite-results.json').read_text(encoding='utf-8'));(area/'initial-suite-results.json').write_text(json.dumps(oldResults,indent=2),encoding='utf-8');runs=[r for r in oldResults['suites'] if r['suite']=='CompletionCountdownTests'];assert runs[0]['passed']
for name,args in [('CompletionCountdownUiTests',[]),('CompletionClaimUiTests',['candidate'])]:
 folder=area/'ui-runs-final'/name;folder.parent.mkdir(exist_ok=True);target=release/('Udm.'+name+'.exe')
 with (area/('run-'+name+'-final.log')).open('w',encoding='utf-8') as log:p=subprocess.run([str(target),*args,str(folder)],cwd=root,env=env,stdout=log,stderr=subprocess.STDOUT,timeout=120)
 data=json.loads((folder/'results.json').read_text(encoding='utf-8-sig'));row={'suite':name,'passed':p.returncode==0 and data['passed'],'checks':len(data['checks']),'sha256':sha(target)};runs.append(row);(area/'suite-results.json').write_text(json.dumps({'passed':all(r['passed'] for r in runs),'suites':runs},indent=2),encoding='utf-8');print(json.dumps(row),flush=True);assert row['passed'],data
print('Focused countdown and retained completion-claim checks passed',flush=True)
