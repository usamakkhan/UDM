from pathlib import Path
import hashlib,json,subprocess,sys
root=Path('<repo>');base=Path('<development>');source=root/'benchmarks/speed/compare-completion.py';text=source.read_text(encoding='utf-8-sig')
def sha(p):return hashlib.sha256(p.read_bytes()).hexdigest().upper()
changes=[("assert not idm_pids(),'Current-session IDM must be closed before starting';",''),("clients=['udm','idm'];apps=","clients=['udm'];apps="),("original=read_settings();save(work/'idm-settings-original.json',original)","original={};save(work/'idm-settings-original.json',{'notReadOrChanged':True})"),("for client in (clients[(trial-1)%len(clients):]+clients[:(trial-1)%len(clients)] if args.baseline_udm else (['udm','idm']if(trial+(case=='single-64m'))%2 else['idm','udm'])):","for client in clients:"),("finally:put_settings(original);save(work/'idm-settings-restored.json',{'matches':read_settings()==original})","finally:save(work/'idm-settings-restored.json',{'notReadOrChanged':True})"),("'udmNotSlowerMedian':medians['udm']<=medians['idm'],","'idmComparisonPerformed':False,"),("'settingsRestored':read_settings()==original","'idmSettingsNotReadOrChanged':True")]
for old,new in changes:assert text.count(old)==1,old;text=text.replace(old,new)
text="import sys\nsys.path.insert(0,'<repo>/benchmarks/speed')\n"+text
driver=base/'current-pilot-derived-9.71.py';driver.write_text(text,encoding='utf-8')
work=root/'.media-cache/owned-current-finishing-pilot-9.71';assert not work.exists()
b=json.loads((root/'.media-cache/owned-relocation-ownership-build-9.71/build-result.json').read_text());app=next(r for r in b['outputs'] if r['name']=='UDM.exe');assert sha(Path(app['path']))==app['sha256']
args=[sys.executable,str(driver),'--output-root',str(work),'--udm',app['path'],'--expected-udm-sha256',app['sha256'],'--label','9.71 normal-product pilot only; no IDM comparison; all three cases retained','--trials','1','--flush-after-exit','--remove-owned-payloads']
receipt=dict(originalHarnessSha256=sha(source),derivedDriverSha256=sha(driver),transformations=changes,args=args,idmNotLaunched=True,idmSettingsNotReadOrChanged=True,processesNotTerminated=True)
(base/'current-pilot-9.71-request.json').write_text(json.dumps(receipt,indent=2)+'\n',encoding='utf-8');p=subprocess.Popen(args,cwd=root);(base/'current-pilot-9.71-process.json').write_text(json.dumps(dict(pid=p.pid,work=str(work))),encoding='utf-8');code=p.wait();(base/'current-pilot-9.71-terminal.json').write_text(json.dumps(dict(exitCode=code,terminal=True)),encoding='utf-8');raise SystemExit(code)
