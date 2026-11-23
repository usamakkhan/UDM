from pathlib import Path
import os,json,hashlib,stat,difflib
root=Path('[repo]');base=Path(os.environ['LOCALAPPDATA'])/'UDM-Development';build=root/'.media-cache/owned-continuous-dash-build-9.83';old=root/'.media-cache/owned-continuous-dash-baseline-9.82';actor=root/'.media-cache/owned-continuous-dash-actor-9.83';work=root/'.media-cache/owned-continuous-dash-workflow-9.83-corrected';initial=root/'.media-cache/owned-continuous-dash-workflow-9.83';dest=root/'docs/validation/continuous-dash-9.83';dest.mkdir(exist_ok=False);sha=lambda p:hashlib.sha256(p.read_bytes()).hexdigest().upper()
b=json.loads((build/'build-result.json').read_text());s=json.loads((build/'suite-results.json').read_text());q=json.loads((work/'qualification.json').read_text());audit=json.loads((actor/'independent-audit.json').read_text());assert b['passed'] and s['passed'] and q['passed'] and audit['passed'];assert all(sha(root/x['path'])==x['sha256'] for x in b['sources'])
def clean(text):
 for a,z in [(str(root),'[repo]'),(root.as_posix(),'[repo]'),(str(base),'[local-diagnostics]'),(base.as_posix(),'[local-diagnostics]'),('[user-profile]','[user-profile]'),('[user-profile]','[user-profile]')]:text=text.replace(a.replace('\\','\\\\'),z).replace(a,z)
 return text
def export(p,name):
 target=dest/name;target.parent.mkdir(parents=True,exist_ok=True);target.write_text(clean(p.read_text(encoding='utf-8-sig',errors='replace')),encoding='utf-8')
def write(name,value):
 target=dest/name;target.parent.mkdir(parents=True,exist_ok=True);target.write_text(clean(json.dumps(value,indent=2)),encoding='utf-8')
for folder,label in [(build,'build'),(old,'baseline'),(actor,'actor'),(work,'workflow')]:
 for p in folder.iterdir():
  if p.is_file() and p.suffix in ['.json','.log'] and p.name!='active.json':export(p,Path(label)/p.name)
for row in s['suites']:export(Path(row['result']),Path('suites')/(row['suite']+'.json'))
state=json.loads((work/'server-state.json').read_text())
for row in state['records']:export(Path(row['result']),Path('workflow')/row['case']/(row['mode']+'-results.json'))
write('initial-fixture-assets.json',{'nativeActorNeverStarted':not (initial/'active.json').exists(),'files':[{'path':p.relative_to(initial).as_posix(),'sha256':sha(p),'bytes':p.stat().st_size} for p in sorted((initial/'assets').rglob('*')) if p.is_file()]})
write('expected-save-disconnect.json',{'nativeSaveExitCode':0,'headersWereStillHeldAtCompletion':q['saveFinishedWithHeadersStillHeld'],'serverHoldTimedOut':state['states']['save']['holdTimedOut'],'expectedFixtureServerException':'ConnectionResetError 10054 when the held response is released after native Save has already closed its request.','notAnApplicationFailure':True,'originalLogPreserved':True})
patch=''
for f in ['native/DashManifest.cpp','native/DashManifest.hpp','native/LiveDash.hpp','native/Version.hpp','native/RetiredDashBoundaryTests.cpp','browser/chromium/manifest.json','browser/firefox/manifest.json']:
 patch+=''.join(difflib.unified_diff((old/f).read_text(encoding='utf-8-sig').splitlines(True),(root/f).read_text(encoding='utf-8-sig').splitlines(True),fromfile='9.82/'+f,tofile='9.83/'+f))
for f in ['benchmarks/speed/live_dash_continuous_workflow.cpp','tests/live-dash-continuous.native-live.py']:patch+=''.join(difflib.unified_diff([], (root/f).read_text(encoding='utf-8').splitlines(True),fromfile='/dev/null',tofile='9.83/'+f))
(dest/'continuous-refresh.patch').write_text(patch,encoding='utf-8')
for name in ['run-continuous-dash-baseline-9.82.py','resume-continuous-dash-baseline-9.82.py','build-continuous-dash-9.83.py','run-continuous-dash-candidate-9.83.py']:export(base/name,Path('drivers')/name)
protect={Path(x['path']).resolve() for x in b['coreObjects']}|{Path(x['path']).resolve() for x in b['outputs'] if x['kind']=='product'}|{(build/'App.res').resolve()};files=[];reparse=[]
def add(p,boundary):
 p=p.resolve();boundary=boundary.resolve();st=p.stat(follow_symlinks=False);assert p.is_relative_to(boundary) and p not in protect and stat.S_ISREG(st.st_mode) and not getattr(st,'st_file_attributes',0)&1024;files.append({'path':str(p),'root':str(boundary),'sha256':sha(p),'bytes':st.st_size})
def scan(folder):
 if not folder.exists():return
 stack=[folder]
 while stack:
  current=stack.pop()
  for entry in os.scandir(current):
   st=entry.stat(follow_symlinks=False)
   if getattr(st,'st_file_attributes',0)&1024:reparse.append(str(entry.path));continue
   if entry.is_dir(follow_symlinks=False):stack.append(Path(entry.path))
   elif entry.is_file(follow_symlinks=False):add(Path(entry.path),folder)
for folder in [initial,base/'owned-continuous-dash-workflow-9.83-C',*[Path(x['result']).parent for x in s['suites']]]:scan(folder)
for name in ['assets',*state['states']]:scan(work/name)
for folder in [old/'native',old/'browser',old/'objects',old/'release-native',actor/'objects',actor/'release-native']:scan(folder)
for folder in [build/'objects',build/'release-native']:
 for p in folder.iterdir():
  if p.is_file() and p.resolve() not in protect:add(p,build)
scan(root/'.media-cache/owned-continuous-dash-native-temp-9.83')
prior=json.loads((root/'.media-cache/owned-retired-dash-build-9.82/build-result.json').read_text());retired=[]
for x in prior['outputs']:
 if x['kind']=='product' and x['name']!='UDM.exe':
  p=Path(x['path']);assert sha(p)==x['sha256'];add(p,p.parent.parent);retired.append(x)
for x in prior['coreObjects']:
 p=Path(x['path'])
 if p.resolve() not in protect:
  assert sha(p)==x['sha256'];add(p,p.parent.parent);retired.append(x)
assert len(retired)==7 and len({x['path'].casefold() for x in files})==len(files);assert len(reparse)==2
manifest={'allowedRoots':sorted(set(x['root'] for x in files)),'files':files,'protectedCurrentFiles':[str(p) for p in sorted(protect)],'retiredArtifacts':retired,'ownedJunctions':reparse,'allEvidenceExported':True};(base/'continuous-dash-cleanup-9.83.json').write_text(json.dumps(manifest,indent=2));write('retired-artifacts.json',retired);print(json.dumps({'evidenceFiles':len([p for p in dest.rglob('*') if p.is_file()]),'cleanupCandidates':len(files),'logicalBytes':sum(x['bytes'] for x in files),'protectedCurrentFiles':len(protect),'junctions':len(reparse)}))
