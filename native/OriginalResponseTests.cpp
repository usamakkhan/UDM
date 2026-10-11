#include "Core.hpp"
#include "OriginalResponsePolicy.hpp"
#include "../drivers/signed-network/DurableResponseSink.hpp"
#include <iostream>
using namespace udm;
int main(){int passed=0,failed=0;auto root=fs::temp_directory_path()/wide("udm-response-catalog-"+guid());
 auto check=[&](bool ok,const char* message){(ok?++passed:++failed);std::cout<<(ok?"PASS ":"FAIL ")<<message<<std::endl;};
 auto rejects=[&](auto fn,const char* message){try{fn();check(false,message);}catch(const std::exception&){check(true,message);}};
 Json offer={{"Id",uint64_t(1)},{"Method","GET"},{"Status",200},{"WasInterceptable",true},{"Host","example.invalid"},{"Target","/original.bin"},{"Length",uint64_t(4)}};
 Json capture={{"Session",std::string(32,'a')},{"Generation",uint64_t(1)},{"ResponseAdmissionEnabled",true},{"PendingResponses",Json::array({offer})}};
 try{
  {FILETIME created{},ended{},kernel{},user{};if(!GetProcessTimes(GetCurrentProcess(),&created,&ended,&kernel,&user))throw std::runtime_error("Cannot inspect test process");
   auto ticks=(uint64_t(created.dwHighDateTime)<<32)|created.dwLowDateTime;Json identity={{"ProcessId",uint64_t(GetCurrentProcessId())},{"ProcessCreated100ns",ticks}};
   auto name=originalResponseProcess(identity);check(!name.empty(),"Capture process identity resolves from live Windows process");check(!originalResponseBrowser(name),"UDM test process is not admitted as a browser");
   identity["ProcessCreated100ns"]=ticks+1;check(originalResponseProcess(identity).empty(),"Reused or stale PID creation time cannot authorize capture");identity["ProcessId"]=uint64_t(MAXDWORD)+1;check(originalResponseProcess(identity).empty(),"Oversized process identifier is rejected before narrowing");
   check(originalResponseProcess(Json::object()).empty(),"Missing capture process identity cannot authorize capture");
  }
  {auto prefs=defaultSettings();auto candidate=offer;candidate["Target"]="/download.iso?token=1";
   check(originalResponseAllowed(prefs,"msedge.exe",candidate),"Configured file type is eligible for driver capture");
   prefs["CaptureExcludedHosts"]="example.invalid";check(!originalResponseAllowed(prefs,"msedge.exe",candidate),"Excluded server stays with browser");prefs["CaptureExcludedHosts"]="";
   prefs["CaptureExcludedUrls"]="=http://example.invalid/download.iso?token=1";check(!originalResponseAllowed(prefs,"msedge.exe",candidate),"Exact address exclusion includes query");candidate["Target"]="/download.iso?token=2";check(originalResponseAllowed(prefs,"msedge.exe",candidate),"Exact exclusion does not broaden to another signed address");
   prefs["CaptureExcludedUrls"]="http://*.example.invalid/*.iso*";candidate["Host"]="cdn.example.invalid";check(!originalResponseAllowed(prefs,"msedge.exe",candidate),"Wildcard address exclusions include subdomains");prefs["CaptureExcludedUrls"]="";
   candidate["ContentType"]="video/mp4";check(!originalResponseAllowed(prefs,"msedge.exe",candidate),"Playback response is not automatically stolen by driver");candidate.erase("ContentType");
   prefs["BrowserCaptureTargets"]={{"msedge.exe",{{"Enabled",false}}}};check(!originalResponseAllowed(prefs,"msedge.exe",candidate),"Disabled browser cannot bypass capture settings");
   check(!originalResponseAllowed(defaultSettings(),"",candidate),"Unidentified process has no automatic browser capture permission");
  }
  {auto named=offer;named["Target"]="/download?id=7";named["ContentDisposition"]="attachment; filename=archive.iso";
   check(originalResponseAllowed(defaultSettings(),"msedge.exe",named)&&originalResponseName(named)=="archive.iso","Server filename enables extensionless download endpoints");
   named["ContentDisposition"]="attachment; filename=old.zip; filename*=UTF-8'en'new%20name.iso";check(originalResponseName(named)=="new name.iso","UTF-8 extended filename takes precedence and supports language");
   named["ContentDisposition"]="attachment; filename=\"folder/part;two.iso\"";check(originalResponseName(named)=="part;two.iso","Quoted semicolon is preserved and server folders are stripped");
   named["ContentDisposition"]="attachment; filename=good.iso; filename*=UTF-8''bad%ZZ.iso";check(originalResponseName(named)=="good.iso","Malformed extended escaping falls back to plain filename");
   named["ContentDisposition"]="attachment; filename=a.iso; filename=b.exe";check(originalResponseName(named)=="download","Ambiguous duplicate filenames fall back to URL");
   named["ContentDisposition"]="attachment; filename=archive.iso";auto cap=capture;cap["PendingResponses"]=Json::array({named});Manager m(root/L"server-name");auto reservation=m.reserveOriginalResponse(cap,named);check(str(reservation.job->data,"FileName")=="archive.iso","Reserved catalog job uses the same server filename as capture policy");
  }
  for(unsigned mode=0;mode<6;++mode){auto data=root/std::to_wstring(mode);std::string id;
   {Manager m(data);auto settings=m.state["Settings"];settings["DownloadFolder"]=utf8((data/L"files").wstring());m.setSettings(settings);
    {auto reservation=m.reserveOriginalResponse(capture,offer);id=reservation.job->id();auto saved=Json::parse(readText(data/L"state.json"));
     check(saved["Downloads"].size()==1&&saved["Downloads"][0].contains("OriginalResponse")&&!yes(saved["Downloads"][0],"QueueMember"),"Job identity is durable before any response is accepted");
     m.recoverOriginalResponses();check(str(reservation.job->data["OriginalResponse"],"State")=="Reserved","Recovery does not disturb open response file handles");
     rejects([&]{m.reserveOriginalResponse(capture,offer);},"Repeated offer cannot create a second reservation");
     if(mode!=2){auto body=reservation.body.h,receipt=reservation.receipt.h;reservation.body.h=reservation.receipt.h=INVALID_HANDLE_VALUE;
      udmnet::DurableResponseSink sink(body,receipt,true,4);sink.write(mode==1?"ab":"abcd");if(mode!=1)sink.complete();}
    }
    if(mode==3||mode==5)fs::resize_file(data/L"responses"/wide(id)/L"receipt",mode==3?80:0);
    if(mode==4)fs::remove(data/L"responses"/wide(id)/L"receipt");
   }
   {Manager m(data);auto job=m.jobs.at(0);auto state=str(job->data["OriginalResponse"],"State");
    check(state==(mode==0?"Ready":mode==2?"Unaccepted":mode>=4?"Recovery required":"Interrupted"),"Restart distinguishes received, interrupted and unaccepted response ownership");
    check(str(job->data,"Status")=="Paused"&&!yes(job->data,"QueueMember"),"Recovered original response cannot start through the queue");
    check(num(job->data,"Received")== (mode==1?2:mode==2?0:4),"Recovery preserves the actual received byte count");
    rejects([&]{m.resume(job);},"Resume cannot replay the original browser request");
    check(readText(data/L"responses"/wide(id)/L"body")== (mode==1?"ab":mode==2?"":"abcd"),"Recovery preserves every received body byte");
   }
  }
  for(unsigned mode=0;mode<5;++mode){auto data=root/(L"publication-"+std::to_wstring(mode));fs::path target;
   {Manager m(data);auto prefs=m.state["Settings"];prefs["DownloadFolder"]=utf8((data/L"files").wstring());prefs["CategoryFolders"]=false;m.setSettings(prefs);JobPtr job;
    {auto reservation=m.reserveOriginalResponse(capture,offer);job=reservation.job;target=job->target();auto body=reservation.body.h,receipt=reservation.receipt.h;reservation.body.h=reservation.receipt.h=INVALID_HANDLE_VALUE;udmnet::DurableResponseSink sink(body,receipt,true,4);sink.write("abcd");sink.complete();}
    if(mode==2){fs::create_directories(target.parent_path());atomicText(target,"existing",false);}
    if(mode==3)job->data["ExpectedSha256"]=std::string(64,'0');
    if(mode==0){unsigned completedEvents=0;m.event=[&](JobPtr emitted,bool complete){check(emitted==job&&complete&&str(emitted->data,"Status")=="Complete"&&fs::exists(emitted->target()),"Completion event observes published file and saved job");++completedEvents;};m.publishOriginalResponse(job);check(str(job->data,"Status")=="Complete"&&readText(target)=="abcd"&&str(job->data,"Sha256")==fileHash(target),"Publication saves exact bytes and verified completion history");check(!fs::exists(data/L"responses"/wide(job->id())/L"body")&&fs::exists(data/L"responses"/wide(job->id())/L"receipt"),"Successful publication releases duplicate body and retains ownership receipt");m.publishOriginalResponse(job);check(m.jobs.size()==1&&completedEvents==1,"Repeated publication does not duplicate completion notification");}
    else if(mode==1||mode==4)rejects([&]{m.publishOriginalResponse(job,[mode](const char* phase){if(std::string(phase)==(mode==1?"file-published":"publication-prepared"))throw std::runtime_error("fixture crash after rename");});},"Crash after file rename leaves a saved publication intent");
    else rejects([&]{m.publishOriginalResponse(job);},"Collision or expected-hash mismatch blocks publication");
    if(mode!=0)check(readText(data/L"responses"/wide(job->id())/L"body")=="abcd","Uncommitted or failed publication preserves recovery body");
   }
   {Manager reopened(data);auto job=reopened.jobs.at(0);if(mode<=1||mode==4)check(str(job->data,"Status")=="Complete"&&str(job->data["OriginalResponse"],"State")=="Published"&&readText(target)=="abcd","Restart reconciles completed publication without a second request");else check(str(job->data,"Status")!="Complete"&&(mode==2?readText(target)=="existing":!fs::exists(target)),"Failed publication preserves existing destination and incomplete history");}
  }
  {Manager m(root/L"rollback");m.save();rejects([&]{m.reserveOriginalResponse(capture,offer,[](const char* phase){if(std::string(phase)=="before-commit")throw std::runtime_error("fixture save failure");});},"Failed catalog reservation is reported");check(m.jobs.empty()&&Json::parse(readText(m.root/L"state.json"))["Downloads"].empty(),"Failed reservation leaves no in-memory or persisted job");}
  {auto data=root/L"lost-ack";{Manager m(data);rejects([&]{m.reserveOriginalResponse(capture,offer,[](const char* phase){if(std::string(phase)=="committed")throw std::runtime_error("fixture lost reply");});},"Lost reservation acknowledgement is simulated after commit");}Manager reopened(data);check(reopened.jobs.size()==1&&str(reopened.jobs[0]->data["OriginalResponse"],"State")=="Unaccepted","Lost acknowledgement retains the durable reservation for recovery");}
 }catch(const std::exception& e){++failed;std::cout<<"FAIL "<<e.what()<<std::endl;}
 std::cout<<passed<<" passed, "<<failed<<" failed; evidence "<<utf8(root.wstring())<<std::endl;return failed?1:0;
}
