from pathlib import Path
import os,json,subprocess,concurrent.futures,hashlib
base=Path(os.environ['LOCALAPPDATA'])/'UDM-Development';area=base/'owned-completion-claim-build-9.92'
scope={'__file__':str(area/'config.py')};exec((base/'finishing-8.34/config.py').read_text(encoding='utf-8-sig'),scope);globals().update({k:v for k,v in scope.items() if not k.startswith('__')})
prior=json.loads((base/'owned-initial-range-queue-build-9.91/build-result.json').read_text());objects=[area/'objects/Bridge.obj' if Path(x['path']).stem=='Bridge' else Path(x['path']) for x in prior['coreObjects']];assert all(p.is_file() for p in objects)
record=[{'path':str(p),'sha256':sha(p)} for p in objects];ui=area/'ui-fixtures';ui.mkdir(exist_ok=True)
frozen=base/'completion-claim-frozen-9.91';assert sha(frozen/'App.cpp')==json.loads((frozen/'source-proof.json').read_text())['sha256'];(frozen/'CompletionClaimUiTests.cpp').write_bytes((native/'CompletionClaimUiTests.cpp').read_bytes())
jobs=[('claim-current',native/'CompletionClaimUiTests.cpp'),('claim-frozen',frozen/'CompletionClaimUiTests.cpp'),('modal-regression',native/'CompletionModalOwnerUiTests.cpp'),('suppression-regression',native/'CompletionSuppressionUiTests.cpp')]
inputs={'coreObjects':record,'sourceSha256':{str(p):sha(p) for _,p in jobs},'currentAppSha256':sha(native/'App.cpp'),'frozenAppSha256':sha(frozen/'App.cpp'),'capturedBeforeCompilation':True,'optimization':'/Od for both current/frozen isolated UI fixtures; behavior under controlled checkpoint wait, not product throughput timing.'};(area/'ui-inputs.json').write_text(json.dumps(inputs,indent=2))
flags=[x for x in common if x!='/O2']+['/Od','/I'+str(native),'/I'+str(native/'vendor/brotli/c/include')]
def build(job):
 name,source=job;exe=ui/(name+'.exe');args=[str(compiler),*flags,str(source),*map(str,objects),str(area/'App.res'),'/Fo'+str(ui/(name+'.obj')),'/Fe'+str(exe),'/link',*system,'/ENTRY:wWinMainCRTStartup','/SUBSYSTEM:WINDOWS','/DYNAMICBASE','/NXCOMPAT','/HIGHENTROPYVA','/INCREMENTAL:NO','/MANIFEST:NO']
 with (ui/(name+'-build.log')).open('w',encoding='utf-8') as log:p=subprocess.run(args,cwd=root,env=env,stdout=log,stderr=subprocess.STDOUT)
 if p.returncode:print((ui/(name+'-build.log')).read_text(encoding='utf-8',errors='replace')[-4500:],flush=True)
 assert p.returncode==0,name;print('Built '+name,flush=True);return {'name':name,'sha256':sha(exe),'path':str(exe)}
with concurrent.futures.ThreadPoolExecutor(max_workers=2) as pool:outputs=list(pool.map(build,jobs))
assert all(sha(Path(p))==v for p,v in inputs['sourceSha256'].items()) and sha(native/'App.cpp')==inputs['currentAppSha256'];(area/'ui-builds.json').write_text(json.dumps({'passed':True,'outputs':outputs},indent=2))
