from pathlib import Path
import os,json,subprocess,difflib
base=Path(os.environ['LOCALAPPDATA'])/'UDM-Development';area=Path('<repo>/.media-cache/owned-no-drain-control-9.74');area.mkdir(exist_ok=False);(area/'source').mkdir();(area/'temp').mkdir();scope={'__file__':str(area/'config.py')};exec((base/'finishing-8.34/config.py').read_text(encoding='utf-8-sig'),scope);globals().update({k:v for k,v in scope.items() if not k.startswith('__')});common+=['/I'+str(native),'/I'+str(native/'vendor/brotli/c/include')];env={**env,'TEMP':str(area/'temp'),'TMP':str(area/'temp')}
normal=root/'.media-cache/owned-advisory-intent-build-9.74';b=json.loads((normal/'build-result.json').read_text());assert b['passed']
for r in b['sources']:assert sha(root/r['path'])==r['sha256']
for r in b['coreObjects']:assert sha(Path(r['path']))==r['sha256']
original=(native/'Transfer.cpp').read_text(encoding='utf-8-sig');old='if(size>=1048576&&(connections==1||(connections<=8&&size>=32*1048576LL&&size<=64*1048576LL))&&fixedVolume(dataFile))try{';new='if(connections==1&&size>=1048576&&fixedVolume(dataFile))try{';assert original.count(old)==1;candidate=original.replace(old,new);source=area/'source/Transfer.cpp';source.write_text(candidate,encoding='utf-8');(area/'optional-drain-control.patch').write_text(''.join(difflib.unified_diff(original.splitlines(True),candidate.splitlines(True),fromfile='native/Transfer.cpp@9.74',tofile='control/Transfer.cpp')),encoding='utf-8')
inputs=dict(normalInputs=b['sources'],normalCoreObjects=b['coreObjects'],privateSourceSha256=sha(source),originalSourceSha256=sha(native/'Transfer.cpp'),onlyPolicyDelta='Split transfers omit optional reception-time data-only/full prefinish work. Sequential policy and all final mandatory barriers remain.',instrumentationDefines=[],normalSourceChanged=False,resourceSha256=sha(normal/'App.res'))
(area/'inputs.json').write_text(json.dumps(inputs,indent=2),encoding='utf-8')
def invoke(args,name):
 with (area/(name+'.log')).open('w',encoding='utf-8') as log:p=subprocess.run(args,cwd=root,env=env,stdout=log,stderr=subprocess.STDOUT)
 if p.returncode:print((area/(name+'.log')).read_text(encoding='utf-8',errors='replace')[-3000:],flush=True)
 assert p.returncode==0,name
invoke([str(compiler),*common,'/c',str(source),'/Fo'+str(out/'TransferNoDrain.obj')],'TransferNoDrain');print('Private split-drain control compiled',flush=True)
invoke([str(compiler),*common,'/c',str(native/'App.cpp'),'/Fo'+str(out/'App.obj')],'App');print('Common unmodified App entry compiled',flush=True)
baseline=[Path(r['path']) for r in b['coreObjects']];changed=[out/'TransferNoDrain.obj' if Path(r['path']).stem=='Transfer' else Path(r['path']) for r in b['coreObjects']];assert sum(p==out/'TransferNoDrain.obj' for p in changed)==1;outputs=[]
for name,objects in [('UDM.ControlBaseline',baseline),('UDM.NoOptionalDrain',changed)]:
 exe=release/(name+'.exe');invoke([str(compiler),'/nologo',str(out/'App.obj'),*map(str,objects),str(normal/'App.res'),'/Fe'+str(exe),'/link',*system,'/ENTRY:wWinMainCRTStartup','/SUBSYSTEM:WINDOWS','/DYNAMICBASE','/NXCOMPAT','/HIGHENTROPYVA','/INCREMENTAL:NO','/MANIFEST:NO'],name);outputs.append(dict(name=name,path=str(exe),sha256=sha(exe),objects=[dict(path=str(p),sha256=sha(p)) for p in objects]))
assert all(sha(root/r['path'])==r['sha256'] for r in b['sources']);(area/'build.json').write_text(json.dumps(dict(passed=True,experimentalOnly=True,normalVersion='9.74',normalSourceChanged=False,commonAppObjectSha256=sha(out/'App.obj'),appSourceSha256=sha(native/'App.cpp'),privateSourceSha256=sha(source),outputs=outputs),indent=2),encoding='utf-8');print('Matched uninstrumented control pair built; neither is an installed release',flush=True)
