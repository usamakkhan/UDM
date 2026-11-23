from pathlib import Path
import os,json,hashlib,shutil,subprocess
r=Path('[repo]');base=Path(os.environ['LOCALAPPDATA'])/'UDM-Development';a=r/'.media-cache/owned-completion-actions-baseline-9.83';a.mkdir(exist_ok=False);cfg={'__file__':str(a/'config.py')};exec((base/'finishing-8.34/config.py').read_text(encoding='utf-8-sig'),cfg);sha=lambda p:hashlib.sha256(p.read_bytes()).hexdigest().upper();b=json.loads((r/'.media-cache/owned-continuous-dash-build-9.83/build-result.json').read_text());assert all(sha(r/x['path'])==x['sha256'] for x in b['sources']);assert all(sha(Path(x['path']))==x['sha256'] for x in b['coreObjects'])
files=['native/App.cpp','native/Version.hpp','browser/chromium/manifest.json','browser/firefox/manifest.json']
for f in files:
 p=a/f;p.parent.mkdir(parents=True,exist_ok=True);shutil.copyfile(r/f,p)
source=r/'native/CompletionActionContentionUiTests.cpp';resource=r/'.media-cache/owned-continuous-dash-build-9.83/App.res';exe=a/'release-native/Udm.CompletionActionsBaseline.exe';(a/'inputs.json').write_text(json.dumps({'version':'9.83','sources':[{'path':f,'sha256':sha(r/f)} for f in files],'fixtureSha256':sha(source),'coreObjects':b['coreObjects'],'resourceSha256':sha(resource),'productInputsAllMatch':True},indent=2))
args=[str(cfg['compiler']),*cfg['common'],'/I'+str(r/'native'),'/I'+str(r/'native/vendor/brotli/c/include'),str(source),*[x['path'] for x in b['coreObjects']],str(resource),'/Fo'+str(a/'objects/CompletionActions.obj'),'/Fe'+str(exe),'/link',*cfg['system'],'/ENTRY:wWinMainCRTStartup','/SUBSYSTEM:WINDOWS','/INCREMENTAL:NO','/MANIFEST:NO'];env={**cfg['env'],'TEMP':str(a/'objects'),'TMP':str(a/'objects')}
with (a/'build.log').open('w') as log:p=subprocess.run(args,cwd=r,env=env,stdout=log,stderr=subprocess.STDOUT)
assert p.returncode==0,(a/'build.log').read_text(errors='replace')[-5000:]
work=r/'.media-cache/owned-completion-actions-baseline-9.83-D';(a/'actor.json').write_text(json.dumps({'path':str(exe),'sha256':sha(exe),'fixtureSha256':sha(source),'work':str(work)},indent=2))
with (a/'runtime.log').open('w') as log:p=subprocess.Popen([str(exe),'baseline',str(work)],cwd=r,env=env,stdout=log,stderr=subprocess.STDOUT);(a/'active.json').write_text(json.dumps({'pid':p.pid}));code=p.wait()
(a/'terminal.json').write_text(json.dumps({'exitCode':code,'allChildrenTerminal':True},indent=2));data=json.loads((work/'results.json').read_text());print(json.dumps(data),flush=True);assert code==0 and data['passed']
