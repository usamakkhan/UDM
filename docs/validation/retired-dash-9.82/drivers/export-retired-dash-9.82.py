from pathlib import Path
import os,json,hashlib,stat,difflib
root=Path('[repo]');base=Path(os.environ['LOCALAPPDATA'])/'UDM-Development';build=root/'.media-cache/owned-retired-dash-build-9.82';area=root/'.media-cache/owned-retired-dash-baseline-9.81';correction=root/'.media-cache/owned-retired-dash-test-correction-9.82';dest=root/'docs/validation/retired-dash-9.82';dest.mkdir(exist_ok=False);sha=lambda p:hashlib.sha256(p.read_bytes()).hexdigest().upper()
b=json.loads((build/'build-result.json').read_text());s=json.loads((build/'suite-results.json').read_text());q=json.loads((area/'qualification.json').read_text());meta=json.loads((area/'server-state.json').read_text());assert b['passed'] and b['backendQualificationPassed'] and s['passed'] and q['passed'];assert all(sha(root/x['path'])==x['sha256'] for x in b['sources'])
def sanitize(t):
 for a,z in [(str(root),'[repo]'),(root.as_posix(),'[repo]'),(str(base),'[local-diagnostics]'),(base.as_posix(),'[local-diagnostics]'),('[user-profile]','[user-profile]'),('[user-profile]','[user-profile]')]:t=t.replace(a.replace('\\','\\\\'),z).replace(a,z)
 return t
def export(p,name):
 d=dest/name;d.parent.mkdir(parents=True,exist_ok=True);d.write_text(sanitize(p.read_text(encoding='utf-8-sig',errors='replace')),encoding='utf-8')
def write(name,obj):
 d=dest/name;d.parent.mkdir(parents=True,exist_ok=True);d.write_text(sanitize(json.dumps(obj,indent=2)),encoding='utf-8')
for folder,label in [(build,'build'),(correction,'fixture-correction')]:
 for p in folder.iterdir():
  if p.is_file() and p.suffix in ['.json','.log','.patch','.cpp']:export(p,Path(label)/p.name)
for x in s['suites']:export(Path(x['result']),Path('suites')/(x['suite']+'.json'))
for folder,label in [(root/'.media-cache/owned-retired-dash-boundary-preflight-9.82-D','preflight'),(root/'.media-cache/owned-retired-dash-9.82-D-RetiredDashBoundaryTests','initial-build')]:export(folder/'results.json',Path('fixture-correction')/(label+'-results.json'))
for p in area.iterdir():
 if p.is_file() and p.suffix in ['.json','.log','.py'] and p.name!='active.json':export(p,Path('workflow')/p.name)
allowed=['Status','Error','Sha256','Size','Received','LiveCapturedSeconds','LiveRecording','LiveCaptureSealed','AdaptiveAssemblyReused','TransferredBytes','AdaptiveMetadataBytes','TransferSeconds','ElapsedSeconds','MediaPhase']
for row in meta['records']:
 p=Path(row['result']);d=json.loads(p.read_text());job=d.pop('savedJob',{});d['ownedJob']={k:job[k] for k in allowed if k in job};write(Path('workflow')/row['case']/(row['stage']+'-results.json'),d)
assetRows=[]
for p in sorted((area/'assets').rglob('*')):
 if p.is_file():assetRows.append({'path':p.relative_to(area/'assets').as_posix(),'sha256':sha(p),'bytes':p.stat().st_size})
write('workflow/assets-observed-before-cleanup.json',{'files':assetRows,'oracleDistinctFrames':len(set(meta['expected'][0]+meta['expected'][1]))})
patch=''
for old in json.loads((area/'inputs.json').read_text())['sources']:
 f=old['path'];p=root/f;before=area/f
 if p.is_file() and sha(p)!=old['sha256']:patch+=''.join(difflib.unified_diff(before.read_text(encoding='utf-8-sig').splitlines(True),p.read_text(encoding='utf-8-sig').splitlines(True),fromfile='9.81/'+f,tofile='9.82/'+f))
(dest/'retired-period.patch').write_text(patch,encoding='utf-8')
for name in ['build-retired-dash-9.82.py','run-retired-dash-workflow-9.82.py','correct-retired-dash-fixture-9.82.py']:export(base/name,Path('drivers')/name)
# Export only the helper's media-generation block, not old private catalogs.
generator=base/'run-dash-assembly-workflow-9.76.py';snippet=generator.read_text(encoding='utf-8').split("assets=area/'assets';assets.mkdir();expected=[]",1)[1].split('document=',1)[0];snippet=snippet.replace("f'color=c={color}:s=640x360:r=10:d=1'","('testsrc2=s=640x360:r=10:d=1'+(',hue=h=90' if n else ''))");(dest/'drivers/assets-generation.py').write_text("assets=area/'assets';assets.mkdir();expected=[]"+snippet,encoding='utf-8');write('drivers/generator-observed.json',{'generatorSha256':sha(generator),'observedAfterQualification':True,'extractedBlockSha256':sha(dest/'drivers/assets-generation.py')})
protect={Path(x['path']).resolve() for x in b['coreObjects']}|{Path(x['path']).resolve() for x in b['outputs'] if x['kind']=='product'}|{(build/'App.res').resolve()};files=[];excluded=[]
def add(p,boundary):
 p=p.resolve();boundary=boundary.resolve();st=p.stat(follow_symlinks=False);assert p.is_relative_to(boundary) and p not in protect and stat.S_ISREG(st.st_mode) and not getattr(st,'st_file_attributes',0)&1024
 files.append({'path':str(p),'root':str(boundary),'sha256':sha(p),'bytes':st.st_size})
def scan(folder):
 if not folder.exists():return
 stack=[folder]
 while stack:
  d=stack.pop()
  for e in os.scandir(d):
   st=e.stat(follow_symlinks=False)
   if getattr(st,'st_file_attributes',0)&1024:excluded.append(str(e.path));continue
   if e.is_dir(follow_symlinks=False):stack.append(Path(e.path))
   elif e.is_file(follow_symlinks=False):add(Path(e.path),folder)
roots=[Path(x['result']).parent for x in s['suites']]+[root/'.media-cache/owned-retired-dash-boundary-preflight-9.82-D',root/'.media-cache/owned-retired-dash-9.82-D-RetiredDashBoundaryTests',base/'owned-retired-dash-boundary-9.82-C']
for f in roots:scan(f)
for name in ['assets','native','browser','benchmarks','objects','release-native','syntax',*q['cases']]:scan(area/name)
for p in area.iterdir():
 if p.is_file() and p.suffix=='.cpp':add(p,area)
for folder in [build/'release-native',build/'objects',correction/'release-native',correction/'objects']:
 for p in folder.iterdir():
  if p.is_file() and p.resolve() not in protect:add(p,folder.parent)
for p in correction.iterdir():
 if p.is_file() and p.suffix=='.cpp':add(p,correction)
scan(root/'.media-cache/owned-retired-dash-native-temp-9.82')
prior=json.loads((root/'.media-cache/owned-prefix-checksum-build-9.81/build-result.json').read_text());retired=[]
for x in prior['outputs']:
 if x['kind']=='product' and x['name']!='UDM.exe':
  p=Path(x['path']);assert sha(p)==x['sha256'];add(p,p.parent.parent);retired.append(x)
for x in prior['coreObjects']:
 p=Path(x['path'])
 if p.resolve() not in protect:
  assert sha(p)==x['sha256'];add(p,p.parent.parent);retired.append(x)
assert len(retired)==7
assert len({x['path'].casefold() for x in files})==len(files)
manifest={'allowedRoots':sorted(set(x['root'] for x in files)),'files':files,'protectedCurrentFiles':[str(p) for p in sorted(protect)],'retiredPriorArtifacts':retired,'reparseEntriesExcluded':excluded,'allEvidenceExported':True}
(base/'retired-dash-cleanup-9.82.json').write_text(json.dumps(manifest,indent=2));write('retired-artifacts.json',retired)
print(json.dumps({'evidenceFiles':len([p for p in dest.rglob('*') if p.is_file()]),'cleanupCandidates':len(files),'logicalBytes':sum(x['bytes'] for x in files),'protectedCurrentFiles':len(protect),'excludedReparse':excluded,'retiredArtifacts':len(retired)}))
