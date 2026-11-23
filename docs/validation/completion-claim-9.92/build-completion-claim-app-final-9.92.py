from pathlib import Path
import os,json,subprocess
base=Path(os.environ['LOCALAPPDATA'])/'UDM-Development';area=base/'owned-completion-claim-build-9.92';scope={'__file__':str(area/'config.py')};exec((base/'finishing-8.34/config.py').read_text(encoding='utf-8-sig'),scope);globals().update({k:v for k,v in scope.items() if not k.startswith('__')})
prior=json.loads((area/'initial-build-result.json').read_text());sources=[{'path':r['path'],'sha256':sha(root/r['path'])} for r in prior['sources']];changes=[r['path'] for r,old in zip(sources,prior['sources']) if r['sha256']!=old['sha256']];assert changes==['native/App.cpp'],changes
objects=[Path(r['path']) for r in prior['coreObjects']];assert all(sha(p)==r['sha256'] for p,r in zip(objects,prior['coreObjects']))
inputs={'sources':sources,'changedSources':changes,'coreObjects':prior['coreObjects'],'resourceSha256':sha(area/'App.res'),'capturedBeforeCompilation':True,'reason':'Defer ready claim errors while the main modal owner is disabled; preserve accepted claims through actual UI review.'};(area/'final-app-inputs.json').write_text(json.dumps(inputs,indent=2))
exe=area/'release-native/UDM.exe';flags=common+['/I'+str(native),'/I'+str(native/'vendor/brotli/c/include')]
args=[str(compiler),*flags,str(native/'App.cpp'),*map(str,objects),str(area/'App.res'),'/Fo'+str(area/'objects/App.obj'),'/Fe'+str(exe),'/link',*system,'/ENTRY:wWinMainCRTStartup','/SUBSYSTEM:WINDOWS','/DYNAMICBASE','/NXCOMPAT','/HIGHENTROPYVA','/INCREMENTAL:NO','/MANIFEST:NO']
with (area/'build-App-final.log').open('w',encoding='utf-8') as log:p=subprocess.run(args,cwd=root,env=env,stdout=log,stderr=subprocess.STDOUT)
if p.returncode:print((area/'build-App-final.log').read_text(encoding='utf-8',errors='replace')[-4000:],flush=True)
assert p.returncode==0 and all(sha(root/r['path'])==r['sha256'] for r in sources) and sha(area/'App.res')==inputs['resourceSha256']
prior['sources']=sources;prior['finalAppInputs']='final-app-inputs.json';prior['priorAppBuildRecord']='initial-build-result.json';prior['recompiledProductsForFinalOwnerGuard']=['UDM.exe']
for row in prior['outputs']:
 if row['name']=='UDM.exe':row['sha256']=sha(exe)
 else:assert sha(Path(row['path']))==row['sha256']
(area/'build-result.json').write_text(json.dumps(prior,indent=2));print('Final UDM 9.92 built; four unchanged products and all core hashes verified',flush=True)
