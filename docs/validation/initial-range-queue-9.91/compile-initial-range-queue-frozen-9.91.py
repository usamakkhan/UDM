exec(open(r'<development>\compile-initial-range-queue-9.91.py').read().split('args=')[0])
prior=json.loads((base/'owned-range-read-reservation-build-9.89/build-result.json').read_text())
old=next(x for x in prior['coreObjects'] if Path(x['path']).stem in ['DefaultTransfer','Transfer'])
assert sha(Path(old['path']))==old['sha256']
objects=[Path(old['path']) if p.stem=='Transfer' else p for p in objects]
exe=area/'release-native/Udm.HttpReadOwnershipFrozenTransfer.exe'
args=[str(compiler),*common,'/I'+str(native),'/I'+str(native/'vendor/brotli/c/include'),str(source),*map(str,objects),str(area/'App.res'),'/Fo'+str(area/'objects/HttpReadOwnershipFrozen.obj'),'/Fe'+str(exe),'/link',*system,'/SUBSYSTEM:CONSOLE','/DYNAMICBASE','/NXCOMPAT','/HIGHENTROPYVA','/INCREMENTAL:NO','/MANIFEST:NO']
p=subprocess.run(args,cwd=root,env=env,capture_output=True,text=True);(area/'build-HttpReadOwnershipFrozen.log').write_text(p.stdout+p.stderr);print((p.stdout+p.stderr)[-1000:]);assert p.returncode==0
(area/'frozen-transfer-control.json').write_text(json.dumps({'scope':'Same current harness and other objects, only frozen qualified 9.89 Transfer object substituted; not whole 9.89 App or IDM','oldTransfer':old,'binarySha256':sha(exe)},indent=2))
