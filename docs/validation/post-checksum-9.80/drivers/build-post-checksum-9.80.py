from pathlib import Path
import os,json,re,subprocess,functools,concurrent.futures
base=Path(os.environ['LOCALAPPDATA'])/'UDM-Development';area=Path('[repo]/.media-cache/owned-post-checksum-build-9.80');area.mkdir(exist_ok=False);(area/'.udm-build-owned').write_text('Owned UDM 9.80 captured command-state native build',encoding='utf-8')
scope={'__file__':str(area/'config.py')};exec((base/'finishing-8.34/config.py').read_text(encoding='utf-8-sig'),scope);globals().update({k:v for k,v in scope.items() if not k.startswith('__')});common+=['/I'+str(native),'/I'+str(native/'vendor/brotli/c/include')]
temporary=root/'.media-cache/owned-post-checksum-native-temp-9.80';temporary.mkdir(exist_ok=True);(temporary/'.udm-build-owned').write_text('Owned navigation native compiler temporary files',encoding='utf-8');env={**env,'TEMP':str(temporary),'TMP':str(temporary)}
prior=json.loads((root/'.media-cache/owned-fixed-volume-build-9.79/build-result.json').read_text(encoding='utf-8'));previous=prior
paths={r['path'] for r in previous['sources']}|{'native/HttpPostIdentityTests.cpp','native/HttpPostGzipTests.cpp','tests/http-post-identity.native-live.cjs','tests/http-post-gzip.native-live.cjs','tests/http-gzip.native-live.cjs'}|{'native/FixedVolume.hpp','native/FixedVolumeTests.cpp'}|{'native/VerifiedMediaHandoffTests.cpp','benchmarks/speed/dash_output_ownership.cpp','benchmarks/speed/live_dash_restart_workflow.cpp'}|{'benchmarks/speed/advisory_owner_cost.cpp','native/AdvisoryPublicationTests.cpp','native/RelocationOwnershipTests.cpp','native/ScopedPublicationSource.hpp','native/GzipStageOwnershipTests.cpp','native/HttpGzipDecoderTests.cpp','native/HttpGzipTransferTests.cpp','native/TransferStageCleanupTests.cpp','native/CreatedStagingFile.hpp','native/StagingAdmissionTests.cpp','native/StagingLinkReuseTests.cpp','native/PreparedCleanupTests.cpp','native/PreparedCleanupHttpTests.cpp','native/PublicationFailureLifetimeTests.cpp','native/PublicationOverlapTests.cpp','native/CompletedCheckpointTests.cpp','native/IndependentFinishingTests.cpp','native/PublicationAvailabilityTests.cpp','native/PublicationPreparationTests.cpp','native/PublicationCommitTests.cpp','native/PublicationOwnershipTests.cpp','native/PublicationRecoveryTests.cpp','native/EarlyPublicationRecoveryTests.cpp','native/LaunchFailure.hpp','native/LaunchFailureTests.cpp','native/LaunchQueueTests.cpp','benchmarks/ui/silent_launch.py','native/BrowserIntegrationTask.hpp','native/BrowserIntegrationTaskTests.cpp','native/BrowserIntegrationUiTests.cpp','native/ToolbarSettingsUiTests.cpp','native/ToolbarSettingsTaskTests.cpp','native/AppearanceCommandUiTests.cpp','native/AppearanceCommandUi.hpp','native/AppearanceSettingsTask.hpp','native/AppearanceSettingsTaskTests.cpp','native/MainFontUiTests.cpp','native/MainFont.hpp','native/ToolbarCacheUiTests.cpp','native/ToolbarImageCache.hpp','native/ToolbarImageCacheTests.cpp','native/AppearanceRepaintUiTests.cpp','native/MainExitLayoutUiTests.cpp','native/MainExitLayoutTaskTests.cpp','native/ManagedModelWorkTests.cpp','native/ManagerModelWorkTests.cpp','native/MainCommandDispatchUiTests.cpp','native/GlobalLimitToggleTaskTests.cpp','native/GlobalLimitToggleUiTests.cpp','native/GlobalLimitSettingsTaskTests.cpp','native/GlobalLimitSettingsUiTests.cpp','native/ColumnSettingsTaskTests.cpp','native/ColumnsDialogUiTests.cpp','native/backend-tests.json','benchmarks/ui/owned_windows_process.py','benchmarks/ui/owned_desktop_probe.cpp'}|{p.relative_to(root).as_posix() for p in native.glob('*Tests.cpp') if 'closeOwnedMainWindow' in p.read_text(encoding='utf-8-sig')}
includeRoots=[native,root,native/'vendor/curl/include',native/'vendor/brotli/c/include',out,core];unresolved=set()
@functools.lru_cache(None)
def children(p):
 if p.suffix.lower() not in ['.cpp','.hpp','.h','.rc','.c']:return ()
 result=[]
 for name in re.findall(r'^\s*#\s*include\s*"([^"\n]+)"',p.read_text(encoding='utf-8-sig',errors='replace'),re.M):
  q=next((x.resolve() for x in [p.parent/name,*[r/name for r in includeRoots]] if x.is_file()),None)
  if q and q.is_relative_to(root.resolve()) and not q.is_relative_to((root/'.media-cache').resolve()):result.append(q)
  elif q is None:unresolved.add((p.relative_to(root).as_posix(),name))
 return tuple(result)
def closure(p,seen):
 if p in seen:return set()
 seen.add(p);result={p}
 for child in children(p):result|=closure(child,seen)
 return result
for p in list(paths):paths|={x.relative_to(root).as_posix() for x in closure((root/p).resolve(),set())}
sources=[dict(path=p,sha256=sha(root/p)) for p in sorted(paths)];known={r['path']:r['sha256'] for r in previous['sources']};changed={r['path'] for r in sources if r['sha256']!=known.get(r['path'])}
for row in prior['coreObjects']:assert sha(Path(row['path']))==row['sha256']
audit=[];units=[]
for row in prior['coreObjects']:
 p=Path(row['path']);stem='Transfer' if p.stem=='DefaultTransfer' else p.stem;source=native/(stem+'.cpp')
 if not source.is_file():continue
 hits=sorted({x.relative_to(root).as_posix() for x in closure(source.resolve(),set())}&changed);audit.append(dict(source=source.relative_to(root).as_posix(),changedDependencies=hits,recompiled=bool(hits)))
 if hits:units.append(stem)
assert 'Bridge' in units,'Version dependency must rebuild Bridge'
generated=[dict(name=n,path=str(core/n),sha256=sha(core/n)) for n in ['Udm.Automation_i.c','Udm.Automation_h.h']]
(area/'source-inputs.json').write_text(json.dumps(dict(sources=sources,changed=sorted(changed),units=audit,unresolvedQuotedIncludes=sorted(unresolved),capturedBeforeCompilation=True,baseline='9.79 qualified captured input inventory'),indent=2),encoding='utf-8');(area/'generated-com-inputs.json').write_text(json.dumps(dict(files=generated,capturedBeforeCompilation=True),indent=2),encoding='utf-8')
def invoke(args,name):
 with (area/(name+'.log')).open('w',encoding='utf-8') as log:p=subprocess.run(args,cwd=root,env=env,stdout=log,stderr=subprocess.STDOUT)
 if p.returncode:print((area/(name+'.log')).read_text(encoding='utf-8',errors='replace')[-4000:],flush=True)
 assert p.returncode==0,name
def compile_core(stem):
 invoke([str(compiler),*common,'/c',str(native/(stem+'.cpp')),'/Fo'+str(out/(stem+'.obj'))],stem);print('Compiled '+stem,flush=True)
with concurrent.futures.ThreadPoolExecutor(max_workers=1) as pool:list(pool.map(compile_core,units))
objects=[out/(('Transfer' if Path(r['path']).stem=='DefaultTransfer' else Path(r['path']).stem)+'.obj') if ('Transfer' if Path(r['path']).stem=='DefaultTransfer' else Path(r['path']).stem) in units else Path(r['path']) for r in prior['coreObjects']]
invoke([str(cache/'microsoft.windows.sdk.cpp/c/bin/10.0.26100.0/x64/rc.exe'),'/nologo','/fo'+str(area/'App.res'),str(native/'App.rc')],'resources')
tests=['FixedVolumeTests','VerifiedMediaHandoffTests','AdvisoryPublicationTests','RelocationOwnershipTests','GzipStageOwnershipTests','HttpGzipDecoderTests','TransferStageCleanupTests','StagingAdmissionTests','StagingLinkReuseTests','PreparedCleanupTests','PreparedCleanupHttpTests','PublicationFailureLifetimeTests','PublicationOverlapTests','CompletedCheckpointTests','IndependentFinishingTests','PublicationAvailabilityTests','PublicationPreparationTests','PublicationCommitTests','PublicationOwnershipTests','PublicationRecoveryTests','EarlyPublicationRecoveryTests','LaunchFailureTests','LaunchQueueTests'];products=[('App','UDM',True),('HostMain','Udm.NativeHost',False),('MonitorMain','Udm.Monitor',False),('SetupMain','Udm.SetupHelper',False),('AutomationMain','Udm.Automation',True)];targets=[(x,'Udm.'+x,False,'backend') for x in tests]+[(s,n,w,'product') for s,n,w in products]+[('HttpGzipTransferTests','Udm.HttpGzipTransferTests',False,'integration'),('HttpPostIdentityTests','Udm.HttpPostIdentityTests',False,'integration'),('HttpPostGzipTests','Udm.HttpPostGzipTests',False,'integration')];outputs=[]
for stem,name,windows,kind in targets:
 exe=release/(name+'.exe');invoke([str(compiler),*common,str(native/(stem+'.cpp')),*map(str,objects),str(area/'App.res'),'/Fo'+str(out/(stem+'.obj')),'/Fe'+str(exe),'/link',*system,*(['/ENTRY:wWinMainCRTStartup'] if windows else []),'/SUBSYSTEM:'+('WINDOWS' if windows else 'CONSOLE'),'/DYNAMICBASE','/NXCOMPAT','/HIGHENTROPYVA','/INCREMENTAL:NO','/MANIFEST:NO'],'build-'+stem);outputs.append(dict(source=stem,name=exe.name,path=str(exe),kind=kind,sha256=sha(exe)));print('Built '+stem,flush=True)
assert all(sha(root/r['path'])==r['sha256'] for r in sources) and all(sha(Path(r['path']))==r['sha256'] for r in generated)
record=dict(passed=True,version='9.80',extensionVersion='0.73.44',sources=sources,coreObjects=[dict(path=str(p),sha256=sha(p)) for p in objects],recompiledCore=units,outputs=outputs,installedAppChanged=False,qualificationFeatureDefine=None);(area/'build-result.json').write_text(json.dumps(record,indent=2),encoding='utf-8')
(area/'core-ready.json').write_text(json.dumps(dict(coreObjects=record['coreObjects']),indent=2))
runs=[]
for stem in tests:
 folder=root/'.media-cache'/('owned-post-checksum-9.80-D-'+stem);assert not folder.exists();exe=release/('Udm.'+stem+'.exe')
 with (area/('run-'+stem+'.log')).open('w',encoding='utf-8') as log:p=subprocess.run([str(exe),str(folder)],cwd=root,env=env,stdout=log,stderr=subprocess.STDOUT,timeout=120)
 data=json.loads((folder/'results.json').read_text(encoding='utf-8-sig'));row=dict(suite=stem,drive='D',passed=p.returncode==0 and data['passed'],checks=len(data['checks']),binarySha256=sha(exe),result=str(folder/'results.json'));runs.append(row);(area/'suite-results.json').write_text(json.dumps(dict(passed=all(r['passed'] for r in runs),checks=sum(r['checks'] for r in runs),suites=runs),indent=2),encoding='utf-8');print(stem+': '+str(row['checks'])+'; '+str(row['passed']),flush=True);assert row['passed'],data
print('Five native 9.80 products and prepared staging identity and publication backend checks passed',flush=True)



