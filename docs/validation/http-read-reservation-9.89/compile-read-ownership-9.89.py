from pathlib import Path
import os,json,subprocess,hashlib
base=Path(os.environ['LOCALAPPDATA'])/'UDM-Development'; area=base/'owned-range-read-reservation-build-9.89'
scope={'__file__':str(area/'config.py')};exec((base/'finishing-8.34/config.py').read_text(encoding='utf-8-sig'),scope)
globals().update({k:v for k,v in scope.items() if not k.startswith('__')})
manifest=json.loads((area/'build-result.json').read_text()); objects=[Path(x['path']) for x in manifest['coreObjects']]
assert all(sha(p)==r['sha256'] for p,r in zip(objects,manifest['coreObjects']))
source=root/'benchmarks/speed/http_read_ownership.cpp'; exe=area/'release-native/Udm.HttpReadOwnership.exe'
(area/'read-ownership-input.json').write_text(json.dumps({'sourceSha256':sha(source),'objects':manifest['coreObjects']},indent=2))
args=[str(compiler),*common,'/I'+str(native),'/I'+str(native/'vendor/brotli/c/include'),str(source),*map(str,objects),str(area/'App.res'),'/Fo'+str(area/'objects/HttpReadOwnership.obj'),'/Fe'+str(exe),'/link',*system,'/SUBSYSTEM:CONSOLE','/DYNAMICBASE','/NXCOMPAT','/HIGHENTROPYVA','/INCREMENTAL:NO','/MANIFEST:NO']
p=subprocess.run(args,cwd=root,env=env,capture_output=True,text=True);(area/'build-HttpReadOwnership.log').write_text(p.stdout+p.stderr);print((p.stdout+p.stderr)[-3500:]);assert p.returncode==0
print('Integration driver built',sha(exe))
