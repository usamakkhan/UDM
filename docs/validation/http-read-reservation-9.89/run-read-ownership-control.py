import sys,json,subprocess,time,os
from pathlib import Path
sys.path.insert(0,r'<repo>\benchmarks\speed')
from origin import Origin,Options,expected_sha256
area=Path(os.environ['LOCALAPPDATA'])/'UDM-Development/owned-range-read-reservation-build-9.89'; dest=area/'split-control'; dest.mkdir(exist_ok=False)
rows=[];size=64<<20;digest=expected_sha256(size,20261018)
for n,variant in enumerate(['frozen','current','current','frozen']):
 origin=Origin(Options(size=size,rate=8<<20,chunk=64<<10,slow_stride=3,slow_rate=2<<20))
 folder=dest/(str(n)+'-'+variant)
 exe=area/'release-native'/('Udm.HttpReadOwnershipFrozenTransfer.exe' if variant=='frozen' else 'Udm.HttpReadOwnership.exe')
 try:
  begun=time.perf_counter();p=subprocess.run([str(exe),str(folder),origin.url,digest,str(size),'split'],capture_output=True,text=True,timeout=90);lifetime=(time.perf_counter()-begun)*1000
  deadline=time.monotonic()+3
  while time.monotonic()<deadline:
   with origin.lock:active=origin.active
   if not active:break
   time.sleep(.01)
  result=json.loads((folder/'results.json').read_text(encoding='utf-8-sig'));result.update(variant=variant,origin=origin.snapshot(),exitCode=p.returncode,parentLifetimeIncludingProofMs=lifetime)
  (dest/(str(n)+'-'+variant+'.json')).write_text(json.dumps(result,indent=2))
  rows.append({'variant':variant,'passed':result['passed'] and p.returncode==0,'checks':len(result['checks']),'bytesSent':result['origin']['bytes_sent'],'requests':result['origin']['requests'],'engineThroughJoinedStopMs':result['transfer'].get('engineThroughJoinedStopMs')});print(json.dumps(rows[-1]),flush=True)
 finally:origin.close()
(dest/'summary.json').write_text(json.dumps({'passed':all(x['passed'] for x in rows),'runs':rows,'scope':'C: engine-only ABBA, two samples per variant, independent hash after timed joined stop; no normal App/IDM or speed acceptance.'},indent=2))
assert all(x['passed'] for x in rows)
