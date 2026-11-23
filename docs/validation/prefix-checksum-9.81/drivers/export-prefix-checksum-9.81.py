from pathlib import Path
import json,hashlib,os,stat,shutil
root=Path('[repo]');base=Path(os.environ['LOCALAPPDATA'])/'UDM-Development';build=root/'.media-cache/owned-prefix-checksum-build-9.81';area=root/'.media-cache/owned-prefix-checksum-qualification-9.81';old=root/'.media-cache/owned-prefix-checksum-baseline-9.80';dest=root/'docs/validation/prefix-checksum-9.81'
sha=lambda p:hashlib.sha256(p.read_bytes()).hexdigest().upper()
b=json.loads((build/'build-result.json').read_text());s=json.loads((build/'suite-results.json').read_text());q=json.loads((area/'results.json').read_text());t=json.loads((area/'terminal.json').read_text());assert b['passed'] and s['passed'] and q['passed'] and t['allCapturedInputsMatch']
assert all(sha(root/x['path'])==x['sha256'] for x in b['sources'])
def sanitize(text):
 for a,z in [(str(root),'[repo]'),(root.as_posix(),'[repo]'),(str(base),'[local-diagnostics]'),(base.as_posix(),'[local-diagnostics]'),('[user-profile]','[user-profile]'),('[user-profile]','[user-profile]')]:
  text=text.replace(a.replace('\\','\\\\'),z).replace(a,z)
 return text
def export(p,target):
 target=dest/target;target.parent.mkdir(parents=True,exist_ok=True);target.write_text(sanitize(p.read_text(encoding='utf-8-sig',errors='replace')),encoding='utf-8')
for p in build.iterdir():
 if p.is_file() and p.suffix in ['.json','.log']:export(p,Path('build')/p.name)
for row in s['suites']:export(Path(row['result']),Path('suites')/(row['suite']+'.json'))
for p in area.iterdir():
 if p.is_file() and p.suffix in ['.json','.log'] and p.name!='active.json':export(p,Path('workflow')/p.name)
for row in q['runs']:
 folder=Path(row['root'])
 for name in ['requests.json','native.log','native/results.json']:export(folder/name,Path('workflow')/row['case']/name)
for name in ['inputs.json','Baseline.cpp','build-Baseline.log']:
 export(old/name,Path('baseline')/name)
for name in ['build-prefix-checksum-9.81.py','run-prefix-checksum-qualification-9.81.py']:export(base/name,Path('drivers')/name)
# Only known fresh owned fixture roots, disposable current target objects/binaries,
# and individually matched superseded 9.80 artifacts are eligible for removal.
protect={Path(x['path']).resolve() for x in b['coreObjects']}|{Path(x['path']).resolve() for x in b['outputs'] if x['kind']=='product'}|{(build/'App.res').resolve()}
roots=[Path(x['result']).parent for x in s['suites']]+[Path(x['root']) for x in q['runs']]
rows=[];excluded=[]
def add(p,allowed):
 p=p.resolve();allowed=allowed.resolve();assert p.is_relative_to(allowed) and p not in protect
 st=p.stat(follow_symlinks=False);assert stat.S_ISREG(st.st_mode) and not getattr(st,'st_file_attributes',0)&1024
 rows.append({'path':str(p),'root':str(allowed),'bytes':st.st_size,'sha256':sha(p)})
def scan(folder):
 if not folder.exists():return
 stack=[folder]
 while stack:
  d=stack.pop()
  for entry in os.scandir(d):
   st=entry.stat(follow_symlinks=False)
   if getattr(st,'st_file_attributes',0)&1024:excluded.append(str(entry.path));continue
   if entry.is_dir(follow_symlinks=False):stack.append(Path(entry.path))
   elif entry.is_file(follow_symlinks=False):add(Path(entry.path),folder)
for folder in roots:scan(folder)
for p in (build/'release-native').iterdir():
 if p.is_file() and p.resolve() not in protect:add(p,build)
for p in (build/'objects').iterdir():
 if p.is_file() and p.resolve() not in protect:add(p,build)
# The older exact source snapshots are reconstructible from the archived patch
# and hashes; keep compact baseline metadata and logs, not duplicate source trees.
for folder in [old/'native',old/'browser',old/'tests',old/'objects',old/'release-native']:scan(folder)
if (old/'Baseline.cpp').exists():add(old/'Baseline.cpp',old)
tmp=root/'.media-cache/owned-prefix-checksum-native-temp-9.81';scan(tmp)
prior=json.loads((root/'.media-cache/owned-post-checksum-build-9.80/build-result.json').read_text());retired=[]
for x in prior['outputs']:
 p=Path(x['path'])
 if x['kind']=='product' and p.name!='UDM.exe':
  assert sha(p)==x['sha256'];add(p,p.parent.parent);retired.append(x)
for x in prior['coreObjects']:
 p=Path(x['path'])
 if p.resolve() not in protect:
  assert sha(p)==x['sha256'];add(p,p.parent.parent);retired.append(x)
assert len(retired)==6
assert len({x['path'].casefold() for x in rows})==len(rows)
manifest={'allowedRoots':sorted(set(x['root'] for x in rows)),'files':rows,'protectedCurrentFiles':[str(x) for x in sorted(protect)],'reparseEntriesExcluded':excluded,'retiredPriorArtifacts':retired,'allEvidenceExported':True}
(base/'prefix-checksum-cleanup-9.81.json').write_text(json.dumps(manifest,indent=2))
(dest/'retired-9.80-artifacts.json').write_text(sanitize(json.dumps(retired,indent=2)))
print(json.dumps({'exportedFiles':len(list(dest.rglob('*.*'))),'cleanupCandidates':len(rows),'logicalBytes':sum(x['bytes'] for x in rows),'currentFilesProtected':len(protect),'retiredPriorArtifacts':len(retired),'reparseEntriesExcluded':len(excluded)}))
