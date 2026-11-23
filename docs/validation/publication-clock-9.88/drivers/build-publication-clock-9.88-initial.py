from pathlib import Path
import os,json,hashlib,subprocess,difflib
root=Path('[repo]');base=Path(os.environ['LOCALAPPDATA'])/'UDM-Development';area=root/'.media-cache/owned-publication-clock-9.88';area.mkdir(exist_ok=False);(area/'source').mkdir();(area/'temp').mkdir();scope={'__file__':str(area/'config.py')};exec((base/'finishing-8.34/config.py').read_text(encoding='utf-8-sig'),scope);compiler=scope['compiler'];common=scope['common']+['/I'+str(area/'source'),'/I'+str(root/'native'),'/I'+str(root/'native/vendor/brotli/c/include'),'/DUDM_FINISH_TIMING_TEST'];env={**scope['env'],'TEMP':str(area/'temp'),'TMP':str(area/'temp')};sha=lambda p:hashlib.sha256(p.read_bytes()).hexdigest().upper();normal=root/'.media-cache/owned-completion-modal-build-9.88';b=json.loads((normal/'build-result.json').read_text());assert b['passed'] and all(sha(root/x['path'])==x['sha256'] for x in b['sources']) and all(sha(Path(x['path']))==x['sha256'] for x in b['coreObjects']);native=root/'native';src=area/'source';out=area/'objects';release=area/'release-native'
names=['App.cpp','Core.cpp','Queue.cpp','Transfer.cpp','Duplicates.cpp','PreparedTransferCommit.hpp','PublishedWriteback.hpp','PublicationPreparation.hpp','ProductionUiTiming.hpp'];original={n:(native/n).read_text(encoding='utf-8-sig') for n in names};modified=original.copy()
helper='''#pragma once
#include "Core.hpp"
#include "ProductionUiTiming.hpp"
namespace udm {struct DiagnosticSpan {const char* end;DiagnosticSpan(const char* begin,const char* finish)noexcept:end(finish){const auto saved=GetLastError();nativeUiTraceRecord(begin);SetLastError(saved);}~DiagnosticSpan(){const auto saved=GetLastError();nativeUiTraceRecord(end);SetLastError(saved);}};}
''';(src/'DiagnosticSpan.hpp').write_bytes(helper.encode())
def change(name,old,new):
 assert modified[name].count(old)==1,(name,old[:80],modified[name].count(old));modified[name]=modified[name].replace(old,new)
for n in ['Core.cpp','Queue.cpp','Transfer.cpp','Duplicates.cpp']:modified[n]='#include "DiagnosticSpan.hpp"\n'+modified[n]
for n in ['PreparedTransferCommit.hpp','PublishedWriteback.hpp','PublicationPreparation.hpp']:change(n,'#include "Core.hpp"','#include "Core.hpp"\n#include "DiagnosticSpan.hpp"')
change('ProductionUiTiming.hpp','nativeUiTraceCapacity=2048','nativeUiTraceCapacity=8192')
change('Core.cpp','periodicCheckpoint([this]{checkpointPeriodic();}){fs::create_directories(root);','periodicCheckpoint([this]{checkpointPeriodic();}){if(!nativeUiTraceSink.view&&!nativeUiTraceInitialize())throw std::runtime_error("Owned trace initialization failed");nativeUiTraceRecord("diag-manager-entry");fs::create_directories(root);')
change('Core.cpp','void atomicText(const fs::path& p,const std::string& s,bool backup,const std::function<void(const char*)>& checkpoint){','void atomicText(const fs::path& p,const std::string& s,bool backup,const std::function<void(const char*)>& checkpoint){DiagnosticSpan total(p.filename()==L"state.json"?"cat-atomic-begin":"journal-atomic-begin",p.filename()==L"state.json"?"cat-atomic-end":"journal-atomic-end");')
change('Core.cpp','if(ReplaceFileW(p.c_str(),temp.c_str(),backup?bak.c_str():nullptr,0,nullptr,nullptr))break;','if([&]{DiagnosticSpan replace("atomic-replace-begin","atomic-replace-end");return ReplaceFileW(p.c_str(),temp.c_str(),backup?bak.c_str():nullptr,0,nullptr,nullptr);}())break;')
change('Core.cpp','Sleep(25*(attempt+1));','{DiagnosticSpan wait("atomic-backoff-begin","atomic-backoff-end");Sleep(25*(attempt+1));}')
change('PublishedWriteback.hpp','bool flush(HANDLE file){return FlushFileBuffers(file)!=FALSE;}','bool flush(HANDLE file){DiagnosticSpan flush("body-flush-begin","body-flush-end");return FlushFileBuffers(file)!=FALSE;}')
change('PreparedTransferCommit.hpp',' void finishWriteback(){',' void finishWriteback(){DiagnosticSpan join("body-join-begin","body-join-end");')
change('PreparedTransferCommit.hpp',' std::string finishAdvisoryHash(){',' std::string finishAdvisoryHash(){DiagnosticSpan join("hash-join-begin","hash-join-end");')
change('Transfer.cpp','static void writeAt(HANDLE file,const void* data,size_t count,i64 offset){','static void writeAt(HANDLE file,const void* data,size_t count,i64 offset){DiagnosticSpan write("body-write-begin","body-write-end");')
change('Transfer.cpp','\n networkEnd=std::chrono::steady_clock::now();','\n nativeUiTraceRecord("body-settled");networkEnd=std::chrono::steady_clock::now();')
change('Duplicates.cpp','  if(!MoveFileExW(staging.c_str(),target.c_str(),MOVEFILE_WRITE_THROUGH))','  if(![&]{DiagnosticSpan rename("rename-begin","rename-end");return MoveFileExW(staging.c_str(),target.c_str(),MOVEFILE_WRITE_THROUGH);}())')
change('PublicationPreparation.hpp','void releaseAcceptedFile(){releaseSource();publishedName.reset();}','void releaseAcceptedFile(){DiagnosticSpan release("file-pins-begin","file-pins-end");releaseSource();publishedName.reset();}')
private=[]
for name,t in modified.items():
 p=src/name;p.write_bytes(t.encode());private.append({'path':str(p),'name':name,'sha256':sha(p),'originalSha256':sha(native/name)});(src/(name+'.patch')).write_bytes(''.join(difflib.unified_diff(original[name].splitlines(True),t.splitlines(True),fromfile='native/'+name,tofile='diagnostic/'+name)).encode())
private.append({'path':str(src/'DiagnosticSpan.hpp'),'name':'DiagnosticSpan.hpp','sha256':sha(src/'DiagnosticSpan.hpp')});(area/'inputs.json').write_text(json.dumps({'capturedBeforeCompilation':True,'normalProductSources':b['sources'],'privateSources':private,'driverSha256':sha(Path(__file__)),'noProductionChanges':True,'diagnosticOnly':True,'resourceSha256':sha(normal/'App.res')},indent=2))
def invoke(args,name):
 with (area/(name+'.log')).open('w',encoding='utf-8')as log:p=subprocess.run(args,cwd=root,env=env,stdout=log,stderr=subprocess.STDOUT)
 if p.returncode:print((area/(name+'.log')).read_text(encoding='utf-8',errors='replace')[-3500:],flush=True)
 assert p.returncode==0,name
for stem in ['Core','Queue','Transfer','Duplicates','App']:
 invoke([str(compiler),*common,'/c',str(src/(stem+'.cpp')),'/Fo'+str(out/(stem+'.obj'))],stem);print('Compiled private '+stem,flush=True)
objects=[out/(Path(x['path']).stem+'.obj')if Path(x['path']).stem in ['Core','Queue','Transfer','Duplicates']else Path(x['path'])for x in b['coreObjects']];exe=release/'UDM.exe';invoke([str(compiler),'/nologo',str(out/'App.obj'),*map(str,objects),str(normal/'App.res'),'/Fe'+str(exe),'/link',*scope['system'],'/ENTRY:wWinMainCRTStartup','/SUBSYSTEM:WINDOWS','/DYNAMICBASE','/NXCOMPAT','/HIGHENTROPYVA','/INCREMENTAL:NO','/MANIFEST:NO'],'link');assert all(sha(root/x['path'])==x['sha256']for x in b['sources']);(area/'build.json').write_text(json.dumps({'passed':True,'diagnosticOnly':True,'binary':str(exe),'sha256':sha(exe),'version':'9.88','privateSources':private,'coreObjects':[{'path':str(p),'sha256':sha(p)}for p in objects],'normalAppEntryPrivateEdits':False,'appEntryCompiledWithFinishTimingOnly':True},indent=2));print('Private publication clock built; normal products unchanged',flush=True)
