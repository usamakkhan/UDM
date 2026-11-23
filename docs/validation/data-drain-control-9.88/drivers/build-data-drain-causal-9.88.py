from pathlib import Path
import os,json,hashlib,subprocess,difflib
r=Path('[repo]');base=Path(os.environ['LOCALAPPDATA'])/'UDM-Development';w=r/'.media-cache/owned-data-drain-causal-9.88';w.mkdir(exist_ok=False);(w/'source').mkdir();(w/'temp').mkdir();cfg={'__file__':str(w/'config.py')};exec((base/'finishing-8.34/config.py').read_text(encoding='utf-8-sig'),cfg);sha=lambda p:hashlib.sha256(p.read_bytes()).hexdigest().upper();b=json.loads((r/'.media-cache/owned-completion-modal-build-9.88/build-result.json').read_text());assert b['passed'] and all(sha(Path(x['path']))==x['sha256']for x in b['coreObjects']);mismatch=[x['path']for x in b['sources']if sha(r/x['path'])!=x['sha256']];assert mismatch==['benchmarks/speed/compare-completion.py'];src=w/'source';native=r/'native';common=cfg['common']+['/I'+str(src),'/I'+str(native),'/I'+str(native/'vendor/brotli/c/include'),'/DUDM_FINISH_TIMING_TEST'];env={**cfg['env'],'TEMP':str(w/'temp'),'TMP':str(w/'temp')}
helper='''#pragma once
#include "Core.hpp"
#include "ProductionUiTiming.hpp"
namespace udm {struct DiagnosticSpan {const char* end;DiagnosticSpan(const char* begin,const char* finish)noexcept:end(finish){const auto error=GetLastError();nativeUiTraceRecord(begin);SetLastError(error);}~DiagnosticSpan(){const auto error=GetLastError();nativeUiTraceRecord(end);SetLastError(error);}};}
''';(src/'DiagnosticSpan.hpp').write_bytes(helper.encode());p=src/'ProductionUiTiming.hpp';p.write_bytes((native/'ProductionUiTiming.hpp').read_bytes().replace(b'nativeUiTraceCapacity=2048',b'nativeUiTraceCapacity=8192'))
original=(native/'Transfer.cpp').read_text(encoding='utf-8-sig');text='#include "DiagnosticSpan.hpp"\n'+original
changes=[('static void writeAt(HANDLE file,const void* data,size_t count,i64 offset){','static void writeAt(HANDLE file,const void* data,size_t count,i64 offset){DiagnosticSpan write("body-write-begin","body-write-end");'),(' const auto httpStartTick=GetTickCount64();',' const auto httpStartTick=GetTickCount64();wchar_t switchValue[2]{};const bool suppressDataDrain=GetEnvironmentVariableW(L"UDM_TEST_SKIP_DATA_DRAIN",switchValue,2)==1&&switchValue[0]==L\'1\';'),('[&,dataFlush]{return flushBodyData(dataFlush,prefinishFile.h);}','[&,dataFlush]{DiagnosticSpan drain("data-drain-begin","data-drain-end");if(suppressDataDrain){nativeUiTraceRecord("data-drain-suppressed");return true;}const bool ok=flushBodyData(dataFlush,prefinishFile.h);nativeUiTraceRecord(ok?"data-drain-ok":"data-drain-failed");return ok;}')]
for old,new in changes:assert text.count(old)==1,old;text=text.replace(old,new)
(src/'Transfer.cpp').write_bytes(text.encode());(src/'Transfer.cpp.patch').write_text(''.join(difflib.unified_diff(original.splitlines(True),text.splitlines(True),fromfile='native/Transfer.cpp',tofile='diagnostic/Transfer.cpp')),encoding='utf-8')
main='''#include "DiagnosticSpan.hpp"
#include <iostream>
using namespace udm;
int wmain(int argc,wchar_t** argv){
 if(argc!=4)return 2;const fs::path root=fs::absolute(argv[1]);
 if(!nativeUiTraceInitialize())return 3;
 int result=1;try{WSADATA sockets{};if(WSAStartup(MAKEWORD(2,2),&sockets))throw std::runtime_error("WSA startup");
 {Manager manager(root/L"catalog");auto prefs=manager.state["Settings"];prefs["DownloadFolder"]=utf8((root/L"files").wstring());prefs["CategoryFolders"]=false;prefs["Connections"]=8;prefs["Parallel"]=1;prefs["SuppressProgressDialog"]=true;prefs["SuppressCompletionDialog"]=true;prefs["StopOnWebPage"]=false;prefs["PrefetchFileInfo"]=false;manager.setSettings(prefs);auto job=manager.add(utf8(argv[2]),"","sample.bin","Main queue",true);{Lock lock(manager.mutex);manager.start(job);}const auto began=GetTickCount64();
 for(;;){bool active;{Lock lock(manager.mutex);active=manager.isActive(job);}if(!active)break;if(GetTickCount64()-began>90000)throw std::runtime_error("Owned engine timeout");Sleep(2);}
 nativeUiTraceRecord("engine-workers-finished");manager.stop();nativeUiTraceRecord("engine-stop-finished");
 if(str(job->data,"Status")!="Complete"||lower(str(job->data,"Sha256"))!=lower(utf8(argv[3])))throw std::runtime_error("Engine completion/hash mismatch");
 atomicText(root/L"engine-result.json",Json{{"passed",true},{"job",job->snapshot()},{"diagnosticOnly",true}}.dump(2));}
 nativeUiTraceRecord("engine-manager-destroyed");result=0;
 }catch(const std::exception& error){std::cerr<<error.what()<<"\\n";}return result;
}
''';(src/'EngineMain.cpp').write_bytes(main.encode());inputs={'diagnosticOnly':True,'capturedBeforeCompilation':True,'normalVersion':'9.88','productionSourceHashes':b['sources'],'researchOnlyInventoryDrift':mismatch,'sources':[{'name':p.name,'sha256':sha(p)}for p in src.iterdir()if p.is_file()],'compilerFlags':common,'driverSha256':sha(Path(__file__))};(w/'inputs.json').write_text(json.dumps(inputs,indent=2))
def invoke(args,name):
 with (w/(name+'.log')).open('w',encoding='utf-8')as log:p=subprocess.run(args,cwd=r,env=env,stdout=log,stderr=subprocess.STDOUT)
 if p.returncode:print((w/(name+'.log')).read_text(encoding='utf-8',errors='replace')[-3500:],flush=True)
 assert p.returncode==0,name
for name in ['Transfer','EngineMain']:
 invoke([str(cfg['compiler']),*common,'/c',str(src/(name+'.cpp')),'/Fo'+str(w/'objects'/(name+'.obj'))],name);print('Compiled '+name,flush=True)
objects=[w/'objects/Transfer.obj'if Path(x['path']).stem=='Transfer'else Path(x['path'])for x in b['coreObjects']];exe=w/'release-native/UDM.exe';invoke([str(cfg['compiler']),'/nologo',str(w/'objects/EngineMain.obj'),*map(str,objects),str(r/'.media-cache/owned-completion-modal-build-9.88/App.res'),'/Fe'+str(exe),'/link',*cfg['system'],'/SUBSYSTEM:CONSOLE','/ENTRY:wmainCRTStartup','/INCREMENTAL:NO','/MANIFEST:NO'],'link');(w/'build.json').write_text(json.dumps({'passed':True,'diagnosticOnly':True,'engineHarnessNotNormalApp':True,'binary':str(exe),'sha256':sha(exe),'coreObjects':[{'path':str(p),'sha256':sha(p)}for p in objects]},indent=2));print('Built engine-only data-drain causal diagnostic',flush=True)
