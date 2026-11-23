from pathlib import Path
import json,hashlib,subprocess,sys
root=Path('[repo]');base=Path('[local-diagnostics]')
def load(p):return json.loads(p.read_text(encoding='utf-8-sig'))
def sha(p):return hashlib.sha256(p.read_bytes()).hexdigest().upper()
build=load(root/'.media-cache/owned-fixed-volume-build-9.78/build-result.json');suites=load(root/'.media-cache/owned-fixed-volume-build-9.78/suite-results.json')
assert build['passed'] and suites['passed'] and len(suites['suites'])==23
assert all(sha(root/r['path'])==r['sha256'] for r in build['sources'])
old=load(root/'.media-cache/owned-live-dash-continuation-build-9.77/build-result.json')
current=next(r for r in build['outputs'] if r['name']=='UDM.exe');baseline=next(r for r in old['outputs'] if r['name']=='UDM.exe')
for row in [current,baseline]:assert sha(Path(row['path']))==row['sha256']
original=base/'owned-pair-derived-9.74.py';text=original.read_text()
oldloop='for client in clients[(trial-1)%len(clients):]+clients[:(trial-1)%len(clients)]:'
newloop='for client in clients[(trial+cases.index((case,size,connections)))%len(clients):]+clients[:(trial+cases.index((case,size,connections)))%len(clients)]:'
assert text.count(oldloop)==1;text=text.replace(oldloop,newloop)
driver=base/'owned-pair-derived-fixed-volume-9.78.py';assert not driver.exists();driver.write_text(text,encoding='utf-8')
work=root/'.media-cache/owned-fixed-volume-pilot-9.78';assert not work.exists()
args=[sys.executable,str(driver),'--output-root',str(work),'--udm',current['path'],'--expected-udm-sha256',current['sha256'],'--baseline-udm',baseline['path'],'--expected-baseline-sha256',baseline['sha256'],'--label','Normal 9.78 guarded opened-volume lookup versus frozen 9.77; one pilot pair per case, no IDM ranking','--trials','1','--flush-after-exit','--remove-owned-payloads']
(base/'fixed-volume-pilot-plan-9.78.json').write_text(json.dumps({'args':args,'sourceDriverSha256':sha(original),'derivedDriverSha256':sha(driver),'change':{'old':oldloop,'new':newloop},'order':['4 MiB baseline then candidate','64 MiB candidate then baseline','64 MiB split baseline then candidate'],'sampleCount':6,'normalProducts':True,'preplannedPilotNotPromotionGate':True,'idmSettingsNotReadOrChanged':True},indent=2))
p=subprocess.Popen(args,cwd=root);(base/'fixed-volume-pilot-process-9.78.json').write_text(json.dumps({'pid':p.pid,'work':str(work)}));code=p.wait()
(base/'fixed-volume-pilot-terminal-9.78.json').write_text(json.dumps({'exitCode':code,'terminal':True}));raise SystemExit(code)
