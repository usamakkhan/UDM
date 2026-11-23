from pathlib import Path
import json,os,subprocess,difflib
base=Path(os.environ['LOCALAPPDATA'])/'UDM-Development';area=Path('<repo>/.media-cache/owned-storage-timeline-9.74');assert area.exists() and (area/'source/DiagnosticSpan.hpp').is_file() and not (area/'build.json').exists() and not any((area/'objects').glob('*.obj'));(area/'source').mkdir(exist_ok=True);(area/'temp').mkdir(exist_ok=True);scope={'__file__':str(area/'config.py')};exec((base/'finishing-8.34/config.py').read_text(encoding='utf-8-sig'),scope);globals().update({k:v for k,v in scope.items() if not k.startswith('__')});common+=['/I'+str(area/'source'),'/I'+str(native),'/I'+str(native/'vendor/brotli/c/include')];env={**env,'TEMP':str(area/'temp'),'TMP':str(area/'temp')}
normal=root/'.media-cache/owned-advisory-intent-build-9.74';b=json.loads((normal/'build-result.json').read_text());assert b['passed'] and len(b['sources'])==691
for r in b['sources']:assert sha(root/r['path'])==r['sha256'],r['path']
for r in b['coreObjects']:assert sha(Path(r['path']))==r['sha256']
names=['Core.cpp','Queue.cpp','Transfer.cpp','Duplicates.cpp','PreparedTransferCommit.hpp','PublishedWriteback.hpp','ProductionUiTiming.hpp'];original={n:(native/n).read_text(encoding='utf-8-sig') for n in names};modified=original.copy()
helper='''#pragma once
#include "Core.hpp"
#include "ProductionUiTiming.hpp"
namespace udm {
struct DiagnosticSpan {
 const char* end;
 DiagnosticSpan(const char* begin,const char* finished)noexcept:end(finished){const auto saved=GetLastError();nativeUiTraceRecord(begin);SetLastError(saved);}
 ~DiagnosticSpan(){const auto saved=GetLastError();nativeUiTraceRecord(end);SetLastError(saved);}
};
}
'''
(area/'source/DiagnosticSpan.hpp').write_text(helper,encoding='utf-8')
def change(name,old,new,count=1):
 assert modified[name].count(old)==count,(name,old[:90],modified[name].count(old));modified[name]=modified[name].replace(old,new)
for n in ['PreparedTransferCommit.hpp','PublishedWriteback.hpp']:change(n,'#include "Core.hpp"','#include "Core.hpp"\n#include "DiagnosticSpan.hpp"')
change('ProductionUiTiming.hpp','nativeUiTraceCapacity=2048','nativeUiTraceCapacity=8192')
change('Core.cpp','periodicCheckpoint([this]{checkpointPeriodic();}){fs::create_directories(root);','periodicCheckpoint([this]{checkpointPeriodic();}){if(!nativeUiTraceSink.view&&!nativeUiTraceInitialize())throw std::runtime_error("Owned trace initialization failed");nativeUiTraceRecord("diag-manager-entry");fs::create_directories(root);')
change('Core.cpp','void atomicText(const fs::path& p,const std::string& s,bool backup,const std::function<void(const char*)>& checkpoint){','void atomicText(const fs::path& p,const std::string& s,bool backup,const std::function<void(const char*)>& checkpoint){DiagnosticSpan total(p.filename()==L"state.json"?"cat-atomic-begin":"journal-atomic-begin",p.filename()==L"state.json"?"cat-atomic-end":"journal-atomic-end");')
change('Core.cpp','#endif\n}if(checkpoint)checkpoint("temporary-flushed");if(fs::exists(p))','#endif\n{DiagnosticSpan close("atomic-close-begin","atomic-close-end");CloseHandle(f.h);f.h=INVALID_HANDLE_VALUE;}\n}if(checkpoint)checkpoint("temporary-flushed");if(fs::exists(p))')
change('Core.cpp','if(ReplaceFileW(p.c_str(),temp.c_str(),backup?bak.c_str():nullptr,0,nullptr,nullptr))break;','if([&]{DiagnosticSpan replace("atomic-replace-begin","atomic-replace-end");return ReplaceFileW(p.c_str(),temp.c_str(),backup?bak.c_str():nullptr,0,nullptr,nullptr);}())break;')
change('Core.cpp','Sleep(25*(attempt+1));','{DiagnosticSpan retry("atomic-backoff-begin","atomic-backoff-end");Sleep(25*(attempt+1));}')
change('Core.cpp','else if(!MoveFileExW(temp.c_str(),p.c_str(),MOVEFILE_WRITE_THROUGH))','else if(![&]{DiagnosticSpan move("atomic-move-begin","atomic-move-end");return MoveFileExW(temp.c_str(),p.c_str(),MOVEFILE_WRITE_THROUGH);}())')
change('Core.cpp','void markZone(const fs::path& p){','void markZone(const fs::path& p){DiagnosticSpan metadata("zone-begin","zone-end");')
count=modified['Core.cpp'].count('readPinnedCatalogText(disk.h,32*1024*1024)');assert count==3
change('Core.cpp','readPinnedCatalogText(disk.h,32*1024*1024)','[&]{DiagnosticSpan read("catalog-read-begin","catalog-read-end");return readPinnedCatalogText(disk.h,32*1024*1024);}()',count)
change('PublishedWriteback.hpp','bool flush(HANDLE file){return FlushFileBuffers(file)!=FALSE;}','bool flush(HANDLE file){DiagnosticSpan flush("body-flush-begin","body-flush-end");return FlushFileBuffers(file)!=FALSE;}')
change('PublishedWriteback.hpp','bool close(HANDLE file){return CloseHandle(file)!=FALSE;}','bool close(HANDLE file){DiagnosticSpan close("body-close-begin","body-close-end");return CloseHandle(file)!=FALSE;}')
change('PreparedTransferCommit.hpp',' bool startWriteback(const fs::path& path,bool normalize){',' bool startWriteback(const fs::path& path,bool normalize){DiagnosticSpan capture("body-capture-begin","body-capture-end");')
change('PreparedTransferCommit.hpp',' void finishWriteback(){',' void finishWriteback(){DiagnosticSpan join("body-join-begin","body-join-end");')
change('PreparedTransferCommit.hpp',' std::string finishAdvisoryHash(){',' std::string finishAdvisoryHash(){DiagnosticSpan join("hash-join-begin","hash-join-end");')
change('Transfer.cpp','static void writeAt(HANDLE file,const void* data,size_t count,i64 offset){','static void writeAt(HANDLE file,const void* data,size_t count,i64 offset){DiagnosticSpan write("body-write-begin","body-write-end");')
change('Transfer.cpp','[&]{return FlushFileBuffers(prefinishFile.h)!=FALSE;}','[&]{DiagnosticSpan flush("prefinish-flush-begin","prefinish-flush-end");return FlushFileBuffers(prefinishFile.h)!=FALSE;}')
change('Transfer.cpp','\n networkEnd=std::chrono::steady_clock::now();','\n nativeUiTraceRecord("body-settled");networkEnd=std::chrono::steady_clock::now();')
change('Duplicates.cpp','  if(!MoveFileExW(staging.c_str(),target.c_str(),MOVEFILE_WRITE_THROUGH))','  if(![&]{DiagnosticSpan rename("rename-begin","rename-end");return MoveFileExW(staging.c_str(),target.c_str(),MOVEFILE_WRITE_THROUGH);}())')
private=[]
for name,text in modified.items():
 p=area/'source'/name;p.write_text(text,encoding='utf-8');private.append(dict(name=name,path=str(p),sha256=sha(p),originalSha256=sha(native/name)));(area/'source'/(name+'.patch')).write_text(''.join(difflib.unified_diff(original[name].splitlines(True),text.splitlines(True),fromfile='native/'+name,tofile='diagnostic/'+name)),encoding='utf-8')
private.append(dict(name='DiagnosticSpan.hpp',path=str(area/'source/DiagnosticSpan.hpp'),sha256=sha(area/'source/DiagnosticSpan.hpp')))
(area/'inputs.json').write_text(json.dumps(dict(productionInputs=b['sources'],privateSources=private,normalResourceSha256=sha(normal/'App.res'),diagnosticOnly=True,appEntryRecompiledFromUnmodifiedSource=True,ringCapacity=8192),indent=2),encoding='utf-8')
def invoke(args,name):
 with (area/(name+'.log')).open('w',encoding='utf-8') as log:p=subprocess.run(args,cwd=root,env=env,stdout=log,stderr=subprocess.STDOUT)
 if p.returncode:print((area/(name+'.log')).read_text(encoding='utf-8',errors='replace')[-3500:],flush=True)
 assert p.returncode==0,name
for stem in ['Core','Queue','Transfer','Duplicates']:
 invoke([str(compiler),*common,'/DUDM_FINISH_TIMING_TEST','/DUDM_PRODUCTION_UI_TIMING_TEST','/c',str(area/'source'/(stem+'.cpp')),'/Fo'+str(out/(stem+'.obj'))],stem);print('Private diagnostic '+stem,flush=True)
invoke([str(compiler),*common,'/c',str(native/'App.cpp'),'/Fo'+str(out/'App.obj')],'App');print('Unmodified App entry compiled without diagnostic defines',flush=True)
objects=[out/(Path(r['path']).stem+'.obj') if Path(r['path']).stem in ['Core','Queue','Transfer','Duplicates'] else Path(r['path']) for r in b['coreObjects']];exe=release/'UDM.exe';invoke([str(compiler),'/nologo',str(out/'App.obj'),*map(str,objects),str(normal/'App.res'),'/Fe'+str(exe),'/link',*system,'/ENTRY:wWinMainCRTStartup','/SUBSYSTEM:WINDOWS','/DYNAMICBASE','/NXCOMPAT','/HIGHENTROPYVA','/INCREMENTAL:NO','/MANIFEST:NO'],'link')
assert all(sha(root/r['path'])==r['sha256'] for r in b['sources'])
(area/'build.json').write_text(json.dumps(dict(passed=True,diagnosticOnly=True,binary=str(exe),sha256=sha(exe),normalVersion='9.74',normalAppEntrySourceSha256=sha(native/'App.cpp'),coreObjects=[dict(path=str(p),sha256=sha(p)) for p in objects],privateSources=private),indent=2),encoding='utf-8');print('Built owned source-operation timeline; production source unchanged',flush=True)

