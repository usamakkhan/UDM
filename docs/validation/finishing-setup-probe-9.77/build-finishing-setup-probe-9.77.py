from pathlib import Path
import os, json, subprocess, difflib

base=Path(os.environ['LOCALAPPDATA'])/'UDM-Development'
area=Path('D:/UDM/.media-cache/owned-finishing-setup-probe-9.77')
area.mkdir(exist_ok=False)
(area/'source').mkdir()
(area/'temp').mkdir()
(area/'.udm-build-owned').write_text('Owned component operation probe; no production change')
scope={'__file__':str(area/'config.py')}
exec((base/'finishing-8.34/config.py').read_text(encoding='utf-8-sig'),scope)
globals().update({k:v for k,v in scope.items() if not k.startswith('__')})
common+=['/I'+str(area/'source'),'/I'+str(native),'/I'+str(native/'vendor/brotli/c/include')]
env={**env,'TEMP':str(area/'temp'),'TMP':str(area/'temp')}
normal=root/'.media-cache/owned-live-dash-continuation-build-9.77'
build=json.loads((normal/'build-result.json').read_text())
for row in build['sources']:assert sha(root/row['path'])==row['sha256']
for row in build['coreObjects']:assert sha(Path(row['path']))==row['sha256']
helper='''#pragma once
#include "Core.hpp"
#include "ProductionUiTiming.hpp"
namespace udm {
struct DiagnosticSpan {
 const char* end;
 DiagnosticSpan(const char* begin,const char* finished)noexcept:end(finished){auto saved=GetLastError();nativeUiTraceRecord(begin);SetLastError(saved);}
 ~DiagnosticSpan(){auto saved=GetLastError();nativeUiTraceRecord(end);SetLastError(saved);}
};
}
'''
(area/'source/DiagnosticSpan.hpp').write_text(helper)
original=(native/'Transfer.cpp').read_text(encoding='utf-8-sig')
changed='#include "DiagnosticSpan.hpp"\n'+original
def replace(old,new):
 global changed
 assert changed.count(old)==1,(old[:80],changed.count(old))
 changed=changed.replace(old,new)
old=next(line for line in original.splitlines() if line.startswith('static bool fixedVolume('))
replace(old,'''static bool fixedVolume(const fs::path& path){DiagnosticSpan total("fixed-volume-begin","fixed-volume-end");wchar_t root[MAX_PATH]{};const auto full=fs::absolute(path).wstring();if(![&]{DiagnosticSpan call("volume-path-begin","volume-path-end");return GetVolumePathNameW(full.c_str(),root,MAX_PATH);}())return false;return [&]{DiagnosticSpan call("drive-type-begin","drive-type-end");return GetDriveTypeW(root)==DRIVE_FIXED;}();}''')
replace(' if(stripped>0){Lock lock(m.mutex);job->data["StrippedPrefixBytes"]=stripped;}', ' nativeUiTraceRecord("posthash-validation-begin");\n if(stripped>0){Lock lock(m.mutex);job->data["StrippedPrefixBytes"]=stripped;}')
replace(' auto stageMark=std::chrono::steady_clock::now();',' nativeUiTraceRecord("posthash-validation-end");nativeUiTraceRecord("stage-output-begin");\n auto stageMark=std::chrono::steady_clock::now();')
replace(' phases["StageOutput"]=phase(stageMark);',' phases["StageOutput"]=phase(stageMark);nativeUiTraceRecord("stage-output-end");nativeUiTraceRecord("final-eligibility-begin");')
replace(' std::shared_ptr<Cancel> advisoryCancel;std::future<AdvisoryHashResult> advisoryHash;bool ownedAdvisory=false;', ' nativeUiTraceRecord("final-eligibility-end");nativeUiTraceRecord("advisory-setup-begin");\n std::shared_ptr<Cancel> advisoryCancel;std::future<AdvisoryHashResult> advisoryHash;bool ownedAdvisory=false;')
replace('   auto body=mediumStage.releaseStorage();auto state=std::move(hasher);hasher.valid=false;', '   nativeUiTraceRecord("cache-transfer-begin");auto body=mediumStage.releaseStorage();auto state=std::move(hasher);hasher.valid=false;nativeUiTraceRecord("cache-transfer-end");nativeUiTraceRecord("async-submit-begin");')
replace('    AdvisoryHashResult result;const auto began=std::chrono::steady_clock::now();\n    try{', '    DiagnosticSpan task("advisory-body-begin","advisory-body-end");AdvisoryHashResult result;const auto began=std::chrono::steady_clock::now();\n    try{')
replace('   });ownedAdvisory=true;', '   });nativeUiTraceRecord("async-submit-end");ownedAdvisory=true;')
replace(' }catch(...){/* Resource exhaustion retains the existing serial fallback. */}', ' }catch(...){/* Resource exhaustion retains the existing serial fallback. */}\n nativeUiTraceRecord("advisory-setup-end");')
replace(' {Lock lock(m.mutex);job->data["FinishPhases"]=phases;}\n const auto metadataTarget=', ' {DiagnosticSpan snapshot("finish-snapshot-begin","finish-snapshot-end");Lock lock(m.mutex);job->data["FinishPhases"]=phases;}\n const auto metadataTarget=')
replace('const auto metadataTarget=prepareFinal?staging:job->target();markZone(metadataTarget);', 'const auto metadataTarget=prepareFinal?staging:job->target();{DiagnosticSpan zone("mark-zone-begin","mark-zone-end");markZone(metadataTarget);}')
source=area/'source/Transfer.cpp'
source.write_text(changed,encoding='utf-8')
(area/'source/Transfer.cpp.patch').write_text(''.join(difflib.unified_diff(original.splitlines(True),changed.splitlines(True),fromfile='native/Transfer.cpp',tofile='diagnostic/Transfer.cpp')))
actor='''#include "Core.hpp"
#include "ProductionUiTiming.hpp"
#include <iostream>
using namespace udm;
int wmain(int argc,wchar_t** argv){
 try{
  if(argc!=3)throw std::runtime_error("Owned folder and input required");
  const auto folder=fs::absolute(argv[1]);
  if(!fs::is_regular_file(folder/L".udm-live-file-info-fixture"))throw std::runtime_error("Owned marker required");
  const auto config=Json::parse(readText(argv[2]));Url url(str(config,"url"));
  if(url.host!="127.0.0.1")throw std::runtime_error("Loopback origin required");
  if(!nativeUiTraceInitialize()||!nativeUiTraceSink.view)throw std::runtime_error("Owned trace required");
  nativeUiTraceRecord("actor-manager-begin");
  Manager manager(folder/L"catalog");auto prefs=manager.state["Settings"];
  prefs["DownloadFolder"]=utf8((folder/L"files").wstring());prefs["CategoryFolders"]=false;
  prefs["ProxyMode"]="Connect directly";prefs["PrefetchFileInfo"]=false;prefs["StopOnWebPage"]=false;
  prefs["Connections"]=num(config,"connections");prefs["Retries"]=3;
  manager.setSettings(prefs);
  auto job=manager.add(str(config,"url"),utf8((folder/L"files").wstring()),"sample.bin","Main queue",true);
  manager.resume(job);manager.dispatchQueueWork();nativeUiTraceRecord("actor-manager-end");
  bool complete=false;
  for(;;){bool active;std::string status;{Lock lock(manager.mutex);active=manager.isActive(job);status=str(job->data,"Status");}
   if(!active&&(status=="Complete"||status=="Failed")){complete=status=="Complete";break;}Sleep(5);
  }
  nativeUiTraceRecord("actor-stop-begin");manager.stop();nativeUiTraceRecord("actor-stop-end");
  Json result={{"passed",complete},{"job",job->data},{"componentOnly",true}};
  std::ofstream output(folder/L"actor-result.json");output<<result.dump(2);output.close();
  nativeUiTraceRecord("actor-terminal");return complete?0:1;
 }catch(const std::exception& error){std::cerr<<error.what();return 1;}
}
'''
entry=area/'source/Actor.cpp';entry.write_text(actor)
private=[{'path':str(p),'sha256':sha(p)} for p in [source,entry,area/'source/DiagnosticSpan.hpp']]
(area/'inputs.json').write_text(json.dumps({'normalSources':build['sources'],'privateSources':private,'driverSha256':sha(Path(__file__)),'diagnosticOnly':True,'scope':'Actual Manager worker, not normal App GUI or speed ranking'},indent=2))
def invoke(args,name):
 with (area/(name+'.log')).open('w') as log:process=subprocess.run(args,cwd=root,env=env,stdout=log,stderr=subprocess.STDOUT)
 if process.returncode:print((area/(name+'.log')).read_text(errors='replace')[-3000:],flush=True)
 assert process.returncode==0,name
invoke([str(compiler),*common,'/DUDM_FINISH_TIMING_TEST','/c',str(source),'/Fo'+str(out/'Transfer.obj')],'Transfer')
print('Private Transfer operation probe compiled',flush=True)
objects=[out/'Transfer.obj' if Path(row['path']).stem in ['Transfer','DefaultTransfer'] else Path(row['path']) for row in build['coreObjects']]
exe=release/'Udm.WorkerProbe.exe'
invoke([str(compiler),*common,'/DUDM_FINISH_TIMING_TEST',str(entry),*map(str,objects),str(normal/'App.res'),'/Fo'+str(out/'Actor.obj'),'/Fe'+str(exe),'/link',*system,'/SUBSYSTEM:CONSOLE','/INCREMENTAL:NO','/MANIFEST:NO'],'Actor')
assert all(sha(root/row['path'])==row['sha256'] for row in build['sources'])
(area/'build.json').write_text(json.dumps({'passed':True,'diagnosticOnly':True,'binary':str(exe),'sha256':sha(exe),'privateSources':private,'coreObjects':[{'path':str(p),'sha256':sha(p)} for p in objects]},indent=2))
print('Actual worker probe linked; production unchanged',flush=True)
