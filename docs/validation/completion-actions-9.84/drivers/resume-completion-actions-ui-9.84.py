from pathlib import Path
import os,json,hashlib,subprocess
root=Path('[repo]');base=Path(os.environ['LOCALAPPDATA'])/'UDM-Development';build=root/'.media-cache/owned-completion-actions-build-9.84';area=root/'.media-cache/owned-completion-actions-ui-9.84'
sha=lambda p:hashlib.sha256(p.read_bytes()).hexdigest().upper()
b=json.loads((build/'build-result.json').read_text());r=json.loads((area/'results.json').read_text());assert len(r['runs'])==1 and r['runs'][0]['passed'] and r['runs'][0]['assertions']==42
assert sha(Path(r['runs'][0]['binary']))==r['runs'][0]['binarySha256'];assert all(sha(root/x['path'])==x['sha256'] for x in b['sources']);assert all(sha(Path(x['path']))==x['sha256'] for x in b['coreObjects'])
partial=area/'release-native/Udm.CompletionDialogUiTests.exe'
(area/'interrupted-build.json').write_text(json.dumps({'reason':'Execution session disappeared during reference fixture build; no compiler, linker or driver remained. No source failure was observed. Passed contention checks are reused without rerunning.','partialBinary':str(partial),'bytes':partial.stat().st_size,'sha256':sha(partial),'resumedDriverSha256':sha(Path(__file__))},indent=2))
cfg={'__file__':str(area/'config.py')};exec((base/'finishing-8.34/config.py').read_text(encoding='utf-8-sig'),cfg);env={**cfg['env'],'TEMP':str(area/'objects'),'TMP':str(area/'objects')}
stem='CompletionDialogUiTests';source=root/'native'/(stem+'.cpp');exe=area/'release-native'/('Udm.'+stem+'.Resumed.exe');reference=root/'docs/reference/completion-dialog.json'
args=[str(cfg['compiler']),*cfg['common'],'/I'+str(root/'native'),'/I'+str(root/'native/vendor/brotli/c/include'),str(source),*[x['path'] for x in b['coreObjects']],str(build/'App.res'),'/Fo'+str(area/'objects'/(stem+'.Resumed.obj')),'/Fe'+str(exe),'/link',*cfg['system'],'/ENTRY:wWinMainCRTStartup','/SUBSYSTEM:WINDOWS','/INCREMENTAL:NO','/MANIFEST:NO']
with (area/'build-reference-resumed.log').open('w') as log:p=subprocess.run(args,cwd=root,env=env,stdout=log,stderr=subprocess.STDOUT)
assert p.returncode==0,(area/'build-reference-resumed.log').read_text(errors='replace')[-4000:]
folder=root/'.media-cache/owned-completion-actions-9.84-D-reference';assert not folder.exists()
with (area/'reference-resumed.log').open('w') as log:
 p=subprocess.Popen([str(exe),str(folder),str(reference)],cwd=root,env=env,stdout=log,stderr=subprocess.STDOUT);(area/'active.json').write_text(json.dumps({'pid':p.pid,'mode':'reference','binary':str(exe)},indent=2));code=p.wait()
data=json.loads((folder/'results.json').read_text());row={'fixture':stem,'mode':'reference','exitCode':code,'passed':code==0 and data['passed'],'assertions':len(data['checks']),'result':str(folder/'results.json'),'binary':str(exe),'binarySha256':sha(exe),'sourceSha256':sha(source)};r['runs'].append(row);r['passed']=all(x['passed'] for x in r['runs']);(area/'results.json').write_text(json.dumps(r,indent=2));print(json.dumps(row),flush=True);assert row['passed'],data
(area/'terminal.json').write_text(json.dumps({'passed':True,'allActorsTerminal':True,'assertions':sum(x['assertions'] for x in r['runs']),'allCapturedInputsStillMatch':all(sha(root/x['path'])==x['sha256'] for x in b['sources'])},indent=2));print('Reference UI qualification completed; previous contention result reused.',flush=True)
