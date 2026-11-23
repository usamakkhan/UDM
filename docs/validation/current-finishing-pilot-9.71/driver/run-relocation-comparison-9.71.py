from pathlib import Path
import json,hashlib,subprocess,sys
root=Path('<repo>');base=Path('<development>')
def read(p):return json.loads(p.read_text(encoding='utf-8-sig'))
def sha(p):return hashlib.sha256(p.read_bytes()).hexdigest().upper()
builds={v:read(root/f'.media-cache/{folder}/build-result.json') for v,folder in [('9.71','owned-relocation-ownership-build-9.71'),('9.63','owned-publication-snapshot-build-9.63')]}
apps={v:next(r for r in b['outputs'] if r['name']=='UDM.exe') for v,b in builds.items()}
for v,r in apps.items():assert sha(Path(r['path']))==r['sha256'],v
for r in builds['9.71']['sources']:assert sha(root/r['path'])==r['sha256'],r['path']
work=root/'.media-cache/owned-relocation-comparison-9.71'
assert not work.exists(),'Inspect existing run and its process handle; do not restart it'
args=[sys.executable,str(root/'benchmarks/speed/compare-completion.py'),'--output-root',str(work),'--udm',apps['9.71']['path'],'--expected-udm-sha256',apps['9.71']['sha256'],'--baseline-udm',apps['9.63']['path'],'--expected-baseline-sha256',apps['9.63']['sha256'],'--label','Normal 9.71 versus last performance-qualified 9.63 control and IDM; 9.63 previously failed parity gates','--trials','3','--flush-after-exit','--remove-owned-payloads']
request=dict(currentVersion='9.71',baselineVersion='9.63',baselinePerformanceGatesPreviouslyPassed=False,args=args,harnessSha256=sha(root/'benchmarks/speed/compare-completion.py'),originSha256=sha(root/'benchmarks/speed/origin.py'),currentBuildSourceInputs=689,noPrivateInstrumentation=True)
(base/'relocation-comparison-9.71-request.json').write_text(json.dumps(request,indent=2)+'\n',encoding='utf-8')
with (base/'relocation-comparison-9.71-terminal.log').open('w',encoding='utf-8') as log:
 p=subprocess.Popen(args,cwd=root,stdout=subprocess.PIPE,stderr=subprocess.STDOUT,text=True)
 (base/'relocation-comparison-9.71-process.json').write_text(json.dumps(dict(pid=p.pid,runner='run-relocation-comparison-9.71.py',work=str(work)),indent=2),encoding='utf-8')
 for line in p.stdout:log.write(line);log.flush();print(line,end='',flush=True)
 code=p.wait()
(base/'relocation-comparison-9.71-terminal.json').write_text(json.dumps(dict(exitCode=code,terminal=True),indent=2),encoding='utf-8')
raise SystemExit(code)
