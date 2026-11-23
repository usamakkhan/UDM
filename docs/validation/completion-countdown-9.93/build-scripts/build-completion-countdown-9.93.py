from pathlib import Path
import os,json,subprocess,hashlib
base=Path(os.environ['LOCALAPPDATA'])/'UDM-Development';area=base/'owned-completion-countdown-build-9.93';area.mkdir(exist_ok=False);(area/'.udm-build-owned').write_text('Owned UDM file countdown native build; no installer or deployment',encoding='utf-8')
scope={'__file__':str(area/'config.py')};exec((base/'finishing-8.34/config.py').read_text(encoding='utf-8-sig'),scope);globals().update({k:v for k,v in scope.items() if not k.startswith('__')});common+=['/I'+str(native),'/I'+str(native/'vendor/brotli/c/include')]
prior=json.loads((base/'owned-completion-claim-build-9.92/build-result.json').read_text(encoding='utf-8'));paths={e['path'] for e in prior['sources']}|{'native/CompletionCountdown.hpp','native/CompletionCountdownTests.cpp','native/CompletionCountdownUiTests.cpp'};sources=[{'path':p,'sha256':sha(root/p)} for p in sorted(paths)];old={e['path']:e['sha256'] for e in prior['sources']};changed=[e['path'] for e in sources if old.get(e['path'])!=e['sha256']];allowed={'native/App.cpp','native/Version.hpp','native/backend-tests.json','browser/chromium/manifest.json','browser/firefox/manifest.json','benchmarks/speed/compare-completion.py','native/CompletionCountdown.hpp','native/CompletionCountdownTests.cpp','native/CompletionCountdownUiTests.cpp'};assert set(changed)==allowed,changed
for e in prior['coreObjects']:assert sha(Path(e['path']))==e['sha256']
inputs={'capturedBeforeCompilation':True,'sources':sources,'changedSources':changed,'baseline':'Qualified normal 9.92; compare-completion.py is the separately recorded research amendment','priorCoreObjects':prior['coreObjects'],'newCoreUnits':['Bridge'],'normalOptimization':'/O2','isolatedUiOptimization':'/Od','installedAppChanged':False};(area/'inputs.json').write_text(json.dumps(inputs,indent=2),encoding='utf-8')
def invoke(args,name):
 with (area/(name+'.log')).open('w',encoding='utf-8') as log:p=subprocess.run(args,cwd=root,env=env,stdout=log,stderr=subprocess.STDOUT)
 if p.returncode:print((area/(name+'.log')).read_text(encoding='utf-8',errors='replace')[-6000:],flush=True)
 assert p.returncode==0,name
invoke([str(compiler),*common,'/c',str(native/'Bridge.cpp'),'/Fo'+str(out/'Bridge.obj')],'compile-Bridge')
objects=[out/'Bridge.obj' if Path(e['path']).stem=='Bridge' else Path(e['path']) for e in prior['coreObjects']];invoke([str(cache/'microsoft.windows.sdk.cpp/c/bin/10.0.26100.0/x64/rc.exe'),'/nologo','/fo'+str(area/'App.res'),str(native/'App.rc')],'resources')
outputs=[]
for source,name,windows,kind in [('CompletionCountdownTests','Udm.CompletionCountdownTests',False,'backend'),('App','UDM',True,'product'),('HostMain','Udm.NativeHost',False,'product'),('MonitorMain','Udm.Monitor',False,'product'),('SetupMain','Udm.SetupHelper',False,'product'),('AutomationMain','Udm.Automation',True,'product'),('CompletionCountdownUiTests','Udm.CompletionCountdownUiTests',True,'ui'),('CompletionClaimUiTests','Udm.CompletionClaimUiTests',True,'ui')]:
 flags=([x for x in common if x!='/O2']+['/Od']) if kind=='ui' else common
 exe=release/(name+'.exe');invoke([str(compiler),*flags,str(native/(source+'.cpp')),*map(str,objects),str(area/'App.res'),'/Fo'+str(out/(source+'.obj')),'/Fe'+str(exe),'/link',*system,*(['/ENTRY:wWinMainCRTStartup'] if windows else []),'/SUBSYSTEM:'+('WINDOWS' if windows else 'CONSOLE'),'/DYNAMICBASE','/NXCOMPAT','/HIGHENTROPYVA','/INCREMENTAL:NO','/MANIFEST:NO'],'build-'+source)
 outputs.append({'name':exe.name,'kind':kind,'path':str(exe),'sha256':sha(exe)});print('Built '+source,flush=True)
assert all(sha(root/e['path'])==e['sha256'] for e in sources)
record={'passed':True,'version':'9.93','extensionVersion':'0.73.57','sources':sources,'coreObjects':[{'path':str(p),'sha256':sha(p)} for p in objects],'outputs':outputs,'installedAppChanged':False};(area/'build-result.json').write_text(json.dumps(record,indent=2),encoding='utf-8')
runs=[]
for name,args in [('CompletionCountdownTests',[]),('CompletionCountdownUiTests',[]),('CompletionClaimUiTests',['candidate'])]:
 folder=area/'runs'/name;folder.parent.mkdir(exist_ok=True);exe=release/('Udm.'+name+'.exe')
 with (area/('run-'+name+'.log')).open('w',encoding='utf-8') as log:p=subprocess.run([str(exe),*args,str(folder)],cwd=root,env=env,stdout=log,stderr=subprocess.STDOUT,timeout=120)
 result=json.loads((folder/'results.json').read_text(encoding='utf-8-sig'));row={'suite':name,'passed':p.returncode==0 and result['passed'],'checks':len(result['checks']),'sha256':sha(exe)};runs.append(row);(area/'suite-results.json').write_text(json.dumps({'passed':all(e['passed'] for e in runs),'suites':runs},indent=2),encoding='utf-8');print(json.dumps(row),flush=True);assert row['passed'],result
print('Normal native products and focused UI/backend checks completed; not installed',flush=True)
