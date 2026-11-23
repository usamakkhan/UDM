from pathlib import Path
import importlib.util,json,hashlib,struct,copy
root=Path('<repo>');area=root/'.media-cache/owned-storage-timeline-9.74';source=root/'benchmarks/speed/analyze_storage_timeline.py';spec=importlib.util.spec_from_file_location('timeline',source);m=importlib.util.module_from_spec(spec);spec.loader.exec_module(m);actual=m.load;checks=[]
assert m.analyze(area)['passed']
for mode in ['changed-launch-data-dir','duplicate-case','invented-span','changed-binary','false-payload-receipt','extra-catalog-transaction','corrupt-encoded-trace','lost-events-with-valid-encoding-hash']:
 def load(path):
  d=actual(path)
  if path==area/'runs.json':
   if mode=='duplicate-case':d['runs'].append(copy.deepcopy(d['runs'][0]))
   if mode=='invented-span':d['runs'][0]['spans'][0]['stage']='unobserved-operation'
   if mode=='changed-binary':d['runs'][0]['binarySha256']='0'*64
   if mode=='extra-catalog-transaction':d['runs'][0]['spans'].append(copy.deepcopy(next(s for s in d['runs'][0]['spans'] if s['stage']=='cat-atomic' and s['afterLastBodyWriteMs']>=0)))
  if path==area/'owned-single-4m/launch.json' and mode=='changed-launch-data-dir':d['args'][d['args'].index('--data-dir')+1]=str(area/'unclaimed-catalog')
  if path==area/'owned-single-4m/payload-removed.json' and mode=='false-payload-receipt':d['passed']=False
  if path==area/'owned-single-4m/trace-lossless.json':
   if mode=='corrupt-encoded-trace':d['usedRecordsHex']=('0' if d['usedRecordsHex'][0]!='0' else '1')+d['usedRecordsHex'][1:]
   if mode=='lost-events-with-valid-encoding-hash':
    header=bytearray.fromhex(d['headerHex']);struct.pack_into('<i',header,32,1);d['headerHex']=header.hex();d['rawSha256']=hashlib.sha256(header+bytes.fromhex(d['usedRecordsHex'])+bytes(d['zeroTailBytes'])).hexdigest().upper()
  return d
 m.load=load;rejected=False
 try:m.analyze(area)
 except AssertionError:rejected=True
 checks.append(dict(mode=mode,rejected=rejected));assert rejected,mode
m.load=actual;assert m.analyze(area)['passed'];(area/'analyzer-checks.json').write_text(json.dumps(dict(passed=True,positiveOriginalAccepted=True,negativeControls=checks,targetsLaunched=0,payloadFilesCreated=0,sourceSha256=hashlib.sha256(source.read_bytes()).hexdigest().upper()),indent=2),encoding='utf-8');print('Original traces pass; eight corrupted-evidence controls reject without launching a target')

