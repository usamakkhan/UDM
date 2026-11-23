from pathlib import Path
import hashlib,json,subprocess,sys
root=Path('<repo>');base=Path('<development>')
def read(p):return json.loads(p.read_text(encoding='utf-8-sig'))
def sha(p):return hashlib.sha256(p.read_bytes()).hexdigest().upper()
prep=(base/'prepare-owned-pair-9.72.py').read_text(encoding='utf-8-sig').replace('9.72','9.74');prepare=base/'prepare-owned-pair-9.74.py';prepare.write_text(prep,encoding='utf-8');subprocess.run([sys.executable,str(prepare)],check=True)
area=root/'.media-cache/owned-advisory-intent-build-9.74';b=read(area/'build-result.json');s=read(area/'suite-results.json');extra=read(area/'body-budget-result.json');assert b['passed'] and s['passed'] and s['checks']==1522 and len(s['suites'])==21 and extra['passed']
for r in b['sources']:assert sha(root/r['path'])==r['sha256'],r['path']
old=read(root/'.media-cache/owned-advisory-intent-build-9.73/build-result.json');app=next(r for r in b['outputs'] if r['name']=='UDM.exe');baseline=next(r for r in old['outputs'] if r['name']=='UDM.exe')
for r in [app,baseline]:assert sha(Path(r['path']))==r['sha256']
work=root/'.media-cache/owned-advisory-pair-9.74';assert not work.exists(),'Inspect recorded process; do not blindly restart a run'
args=[sys.executable,str(base/'owned-pair-derived-9.74.py'),'--output-root',str(work),'--udm',app['path'],'--expected-udm-sha256',app['sha256'],'--baseline-udm',baseline['path'],'--expected-baseline-sha256',baseline['sha256'],'--label','Normal 9.74 early completed-buffer release versus frozen 9.73; no IDM comparison','--trials','3','--flush-after-exit','--remove-owned-payloads']
(base/'advisory-pair-9.74-request.json').write_text(json.dumps(dict(args=args,currentVersion='9.74',baselineVersion='9.73',noIdmComparison=True,noPrivateProductInstrumentation=True),indent=2),encoding='utf-8')
with (base/'advisory-pair-9.74-terminal.log').open('w',encoding='utf-8') as log:
 p=subprocess.Popen(args,cwd=root,stdout=subprocess.PIPE,stderr=subprocess.STDOUT,text=True);(base/'advisory-pair-9.74-process.json').write_text(json.dumps(dict(pid=p.pid,work=str(work)),indent=2),encoding='utf-8')
 for line in p.stdout:log.write(line);log.flush();print(line,end='',flush=True)
 code=p.wait()
(base/'advisory-pair-9.74-terminal.json').write_text(json.dumps(dict(exitCode=code,terminal=True),indent=2),encoding='utf-8');raise SystemExit(code)

