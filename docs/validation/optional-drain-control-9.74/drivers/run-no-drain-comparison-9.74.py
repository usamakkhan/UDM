from pathlib import Path
import os,json,hashlib,subprocess,sys
root=Path('<repo>');base=Path(os.environ['LOCALAPPDATA'])/'UDM-Development';area=root/'.media-cache/owned-no-drain-control-9.74'
def read(p):return json.loads(p.read_text(encoding='utf-8-sig'))
def sha(p):return hashlib.sha256(p.read_bytes()).hexdigest().upper()
b=read(area/'build.json');assert b['passed'] and b['experimentalOnly'];normal=read(root/'.media-cache/owned-advisory-intent-build-9.74/build-result.json')
for r in normal['sources']:assert sha(root/r['path'])==r['sha256']
for r in b['outputs']:
 assert sha(Path(r['path']))==r['sha256']
 for o in r['objects']:assert sha(Path(o['path']))==o['sha256']
prepare=(base/'prepare-owned-pair-9.72.py').read_text(encoding='utf-8-sig').replace('9.72','no-drain-9.74');path=base/'prepare-no-drain-comparison-9.74.py';path.write_text(prepare,encoding='utf-8');subprocess.run([sys.executable,str(path)],check=True);driver=base/'owned-pair-derived-no-drain-9.74.py';candidate=next(r for r in b['outputs'] if r['name']=='UDM.NoOptionalDrain');baseline=next(r for r in b['outputs'] if r['name']=='UDM.ControlBaseline');work=root/'.media-cache/owned-no-drain-comparison-9.74';assert not work.exists(),'Inspect existing processes/results; do not restart a live run'
args=[sys.executable,str(driver),'--output-root',str(work),'--udm',candidate['path'],'--expected-udm-sha256',candidate['sha256'],'--baseline-udm',baseline['path'],'--expected-baseline-sha256',baseline['sha256'],'--label','Private uninstrumented split optional-drain control on frozen 9.74; same App object; no IDM comparison','--case','split-64m','--trials','3','--flush-after-exit','--remove-owned-payloads'];(area/'comparison-request.json').write_text(json.dumps(dict(args=args,commonAppObjectSha256=b['commonAppObjectSha256'],experimentalOnly=True,normalSourceChanged=False),indent=2),encoding='utf-8')
with (area/'comparison.log').open('w',encoding='utf-8') as log:
 p=subprocess.Popen(args,cwd=root,stdout=subprocess.PIPE,stderr=subprocess.STDOUT,text=True);(area/'comparison-process.json').write_text(json.dumps(dict(pid=p.pid,work=str(work))),encoding='utf-8')
 for line in p.stdout:log.write(line);log.flush();print(line,end='',flush=True)
 code=p.wait()
(area/'comparison-terminal.json').write_text(json.dumps(dict(exitCode=code,terminal=True),indent=2),encoding='utf-8');assert code==0
d=read(work/'results.json');assert len(d['results'])==6
for row in d['results']:
 assert row['hashMatches'] and row['finishPhases']['PublishedFlushError']==0 and row['finishPhases']['PublishedFileFlush']>0
 stats=row['prefinishWriteback'];assert (stats['Flushes']==0) if row['client']=='udm' else (stats['Mode']=='data-only' and stats['Flushes']>0 and stats['Errors']==0)
assert all(p['removed'] and not Path(p['path']).exists() for p in d['payloads']);(area/'policy-verification.json').write_text(json.dumps(dict(passed=True,transfers=6,allFinalFullFlushesConfirmed=True,optionalOperationsAbsentOnlyInCandidate=True,allPayloadsRemoved=True),indent=2),encoding='utf-8');print('Six matched controls verified; mandatory final flush present in both policies',flush=True)
