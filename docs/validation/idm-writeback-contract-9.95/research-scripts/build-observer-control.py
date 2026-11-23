from pathlib import Path
import os,json,subprocess,hashlib
base=Path(os.environ['LOCALAPPDATA'])/'UDM-Development';area=Path(r'<research>');scope={'__file__':str(area/'config.py')};exec((base/'finishing-8.34/config.py').read_text(encoding='utf-8-sig'),scope);globals().update({k:v for k,v in scope.items() if not k.startswith('__')});source=area/'observer-control.cpp';(area/'observer-build-inputs.json').write_text(json.dumps({'sourceSha256':sha(source),'compilerSha256':sha(compiler),'capturedBeforeCompilation':True,'nativeOrBrowserProductChanged':False},indent=2),encoding='utf-8')
exe=area/'release-native/observer-control.exe'
with (area/'observer-build.log').open('w',encoding='utf-8') as log:p=subprocess.run([str(compiler),*common,str(source),'/Fo'+str(area/'objects/observer-control.obj'),'/Fe'+str(exe),'/link','kernel32.lib','shell32.lib','user32.lib','/ENTRY:wWinMainCRTStartup','/SUBSYSTEM:WINDOWS','/DYNAMICBASE','/NXCOMPAT','/HIGHENTROPYVA','/INCREMENTAL:NO','/MANIFEST:NO'],cwd=root,env=env,stdout=log,stderr=subprocess.STDOUT)
assert p.returncode==0
(area/'observer-build-result.json').write_text(json.dumps({'passed':True,'exeSha256':sha(exe),'sourceSha256':sha(source)},indent=2),encoding='utf-8');print('Owned 8 KiB observer qualification program built')
