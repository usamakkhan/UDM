from pathlib import Path
import os,json,subprocess,difflib
base=Path(os.environ["LOCALAPPDATA"])/"UDM-Development";area=Path("D:/UDM/.media-cache/owned-zone-call-timeline-9.77");area.mkdir(exist_ok=False);(area/"source").mkdir();(area/"temp").mkdir();(area/".udm-build-owned").write_text("Owned diagnostic only; 9.77 operation observation")
scope={"__file__":str(area/"config.py")};exec((base/"finishing-8.34/config.py").read_text(encoding="utf-8-sig"),scope);globals().update({k:v for k,v in scope.items() if not k.startswith("__")});common += ["/I"+str(area/"source"),"/I"+str(native),"/I"+str(native/"vendor/brotli/c/include")];env={**env,"TEMP":str(area/"temp"),"TMP":str(area/"temp")}
normal=root/".media-cache/owned-live-dash-continuation-build-9.77";b=json.loads((normal/"build-result.json").read_text());assert b["passed"]
for r in b["sources"]:assert sha(root/r["path"])==r["sha256"]
for r in b["coreObjects"]:assert sha(Path(r["path"]))==r["sha256"]
helper=r'''#pragma once
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
(area/"source/DiagnosticSpan.hpp").write_text(helper,encoding="utf-8")
original={n:(native/n).read_text(encoding="utf-8-sig") for n in ["Core.cpp","Transfer.cpp"]};modified=original.copy()
def change(n,old,new):
 assert modified[n].count(old)==1,(n,old[:60],modified[n].count(old));modified[n]=modified[n].replace(old,new)
for n in original:modified[n]='#include "DiagnosticSpan.hpp"\n'+modified[n]
change("Core.cpp","periodicCheckpoint([this]{checkpointPeriodic();}){fs::create_directories(root);",'periodicCheckpoint([this]{checkpointPeriodic();}){if(!nativeUiTraceSink.view&&!nativeUiTraceInitialize())throw std::runtime_error("Owned trace initialization failed");nativeUiTraceRecord("diag-manager-entry");fs::create_directories(root);')
change("Core.cpp",'void markZone(const fs::path& p){Handle f(CreateFileW((p.wstring()+L":Zone.Identifier").c_str(),GENERIC_WRITE,FILE_SHARE_READ,nullptr,CREATE_ALWAYS,0,nullptr));if(f){const char s[]="[ZoneTransfer]\\r\\nZoneId=3\\r\\n";DWORD n;WriteFile(f.h,s,sizeof(s)-1,&n,nullptr);}}'.replace('\\','\'),r'''void markZone(const fs::path& p){DiagnosticSpan total("zone-begin","zone-end");Handle f([&]{DiagnosticSpan open("zone-open-begin","zone-open-end");return CreateFileW((p.wstring()+L":Zone.Identifier").c_str(),GENERIC_WRITE,FILE_SHARE_READ,nullptr,CREATE_ALWAYS,0,nullptr);}());if(f){const char s[]="[ZoneTransfer]\r\nZoneId=3\r\n";DWORD n;{DiagnosticSpan write("zone-write-begin","zone-write-end");WriteFile(f.h,s,sizeof(s)-1,&n,nullptr);}{DiagnosticSpan close("zone-close-begin","zone-close-end");CloseHandle(f.h);f.h=INVALID_HANDLE_VALUE;}}}''')
change("Transfer.cpp",'{Lock lock(m.mutex);job->data["FinishPhases"]=phases;}\n const auto metadataTarget=', '{DiagnosticSpan snapshot("finish-snapshot-begin","finish-snapshot-end");Lock lock(m.mutex);job->data["FinishPhases"]=phases;}\n {DiagnosticSpan metadata("metadata-block-begin","metadata-block-end");const auto metadataTarget=')
change("Transfer.cpp",'if(f)SetFileTime(f.h,&stamp,nullptr,&stamp);}}\n // The file', 'if(f)SetFileTime(f.h,&stamp,nullptr,&stamp);}}}\n // The file')
private=[]
for n,s in modified.items():
 p=area/"source"/n;p.write_text(s,encoding="utf-8");private.append({"name":n,"path":str(p),"sha256":sha(p),"originalSha256":sha(native/n)});(area/"source"/(n+".patch")).write_text("".join(difflib.unified_diff(original[n].splitlines(True),s.splitlines(True),fromfile="native/"+n,tofile="diagnostic/"+n)),encoding="utf-8")
private.append({"name":"DiagnosticSpan.hpp","path":str(area/"source/DiagnosticSpan.hpp"),"sha256":sha(area/"source/DiagnosticSpan.hpp")})
(area/"inputs.json").write_text(json.dumps({"productionInputs":b["sources"],"privateSources":private,"diagnosticOnly":True,"resourceSha256":sha(normal/"App.res"),"driverSha256":sha(Path(__file__))},indent=2))
def invoke(args,name):
 with (area/(name+".log")).open("w",encoding="utf-8") as log:p=subprocess.run(args,cwd=root,env=env,stdout=log,stderr=subprocess.STDOUT)
 if p.returncode:print((area/(name+".log")).read_text(encoding="utf-8",errors="replace")[-2500:],flush=True)
 assert p.returncode==0,name
for stem in ["Core","Transfer","Duplicates"]:
 src=area/"source"/(stem+".cpp") if stem in ["Core","Transfer"] else native/(stem+".cpp")
 invoke([str(compiler),*common,"/DUDM_FINISH_TIMING_TEST","/c",str(src),"/Fo"+str(out/(stem+".obj"))],stem);print("Diagnostic "+stem+" compiled",flush=True)
invoke([str(compiler),*common,"/DUDM_FINISH_TIMING_TEST","/c",str(native/"App.cpp"),"/Fo"+str(out/"App.obj")],"App");print("Unmodified App compiled with finishing events",flush=True)
objects=[out/(Path(r["path"]).stem+".obj") if Path(r["path"]).stem in ["Core","Transfer","Duplicates"] else Path(r["path"]) for r in b["coreObjects"]];exe=release/"UDM.exe"
invoke([str(compiler),"/nologo",str(out/"App.obj"),*map(str,objects),str(normal/"App.res"),"/Fe"+str(exe),"/link",*system,"/ENTRY:wWinMainCRTStartup","/SUBSYSTEM:WINDOWS","/DYNAMICBASE","/NXCOMPAT","/HIGHENTROPYVA","/INCREMENTAL:NO","/MANIFEST:NO"],"link")
assert all(sha(root/r["path"])==r["sha256"] for r in b["sources"])
(area/"build.json").write_text(json.dumps({"passed":True,"diagnosticOnly":True,"normalVersion":"9.77","binary":str(exe),"sha256":sha(exe),"privateSources":private,"coreObjects":[{"path":str(p),"sha256":sha(p)} for p in objects]},indent=2));print("Diagnostic linked; normal source and build unchanged",flush=True)
