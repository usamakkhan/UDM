#include "Core.hpp"
#include "OriginalResponsePolicy.hpp"
#include "../drivers/signed-network/DurableResponseSink.hpp"
#include "../drivers/signed-network/TrackedResponseSink.hpp"
#include "../drivers/signed-network/CaptureObservationsJson.hpp"
#include <iostream>
namespace udm { struct CapturedResponseTestAccess {static void saveCheckpoint(Manager& manager,std::function<void(const char*)> callback){manager.catalogCheckpoint=std::move(callback);}}; }
using namespace udm;
int main(int argc,char** argv){int passed=0,failed=0;auto root=fs::temp_directory_path()/wide("udm-response-catalog-"+guid());
 auto check=[&](bool ok,const char* message){(ok?++passed:++failed);std::cout<<(ok?"PASS ":"FAIL ")<<message<<std::endl;};
 auto rejects=[&](auto fn,const char* message){try{fn();check(false,message);}catch(const std::exception&){check(true,message);}};
 Json offer={{"Id",uint64_t(1)},{"Method","GET"},{"Status",200},{"WasInterceptable",true},{"Host","example.invalid"},{"Target","/original.bin"},{"Length",uint64_t(4)}};
 Json capture={{"Session",std::string(32,'a')},{"Generation",uint64_t(1)},{"ResponseAdmissionEnabled",true},{"PendingResponses",Json::array({offer})}};
 try{
  {Manager m(root/L"escaped-header-boundary");udmnet::HttpRequest request;request.host="example.invalid";request.target="/original.bin";
   for(const auto& key:{"cookie","authorization","user-agent"})request.replayHeaders[key]=std::string(16350,'\\');
   size_t raw=request.host.size()+request.target.size()+3;for(const auto& item:request.replayHeaders)raw+=item.first.size()+item.second.size();
   check(raw<48*1024,"Escaped-header fixture fits the capture metadata limit");auto candidate=offer;candidate["ProtectedRequestHeaders"]=udmnet::protectedReplayHeaders(request);
   std::cout<<"BOUNDARY raw="<<raw<<" protected="<<str(candidate,"ProtectedRequestHeaders").size()<<std::endl;
   auto cap=capture;cap["PendingResponses"]=Json::array({candidate});auto reserved=m.reserveOriginalResponse(cap,candidate);
   check(readHeaders(reserved.job->data)==request.replayHeaders,"Valid escaped replay headers survive protected handoff at the capture boundary");
  }
  for(unsigned invalid=0;invalid<6;++invalid){Manager m(root/(L"protected-invalid-"+std::to_wstring(invalid)));auto candidate=offer;
   Json envelope={{"Host","example.invalid"},{"Target","/original.bin"},{"Headers",{{"cookie","fixture_session=allowed"}}}};
   if(invalid==0)envelope["Host"]="other.invalid";
   if(invalid==1)envelope["Target"]="/other";
   if(invalid==2)envelope["Headers"]["connection"]="close";
   if(invalid==3)envelope["Headers"]["cookie"]=42;
   candidate["ProtectedRequestHeaders"]=invalid==5?std::string(192*1024+1,'x'):invalid==4?"invalid-ciphertext":protect(envelope.dump());auto cap=capture;cap["PendingResponses"]=Json::array({candidate});
   rejects([&]{m.reserveOriginalResponse(cap,candidate);},"Invalid protected headers are rejected before reservation");check(m.jobs.empty()&&!fs::exists(m.root/L"responses"),"Invalid header envelope creates no download or response files");
  }
  for(bool each:{false,true}){Manager m(root/(each?L"slow-save-each":L"slow-save-total"));auto candidate=offer;candidate["Length"]=uint64_t(2048);auto cap=capture;cap["PendingResponses"]=Json::array({candidate});auto reserved=m.reserveOriginalResponse(cap,candidate);auto job=reserved.job;
   job->data["OriginalResponse"]["State"]="Receiving";job->data["OriginalResponse"]["FlowControlled"]=true;job->data["OriginalResponse"]["GrantedBytes"]=0;
   m.state["Settings"]["LimitKbps"]=1;m.state["Settings"]["GlobalLimitMode"]=each?"Each download":"Total across downloads";m.save();
   CapturedResponseTestAccess::saveCheckpoint(m,[](const char* phase){if(std::string(phase)=="temporary-flushed")Sleep(600);});
   auto began=std::chrono::steady_clock::now();auto released=m.scheduledOriginalResponseCredits(job,512);CapturedResponseTestAccess::saveCheckpoint(m,{});auto elapsed=std::chrono::duration<double>(std::chrono::steady_clock::now()-began).count();
   check(elapsed>=0.5,"Delayed durable save consumes the reserved bandwidth interval");
   check(released==512,each?"Each-download credits paid during persistence release without an extra poll":"Global credits paid during persistence release without an extra poll");
   check(num(Json::parse(readText(m.root/L"state.json"))["Downloads"][0]["OriginalResponse"],"GrantedBytes")==512,"Released allowance remains durable before helper delivery");
   std::cout<<"TIMING slow save: "<<(each?"each":"total")<<", elapsed "<<elapsed<<", released "<<released<<std::endl;
  }
  {Manager m(root/L"timed-credit-writer");auto candidate=offer;candidate["Length"]=uint64_t(1024);auto cap=capture;cap["PendingResponses"]=Json::array({candidate});auto reservation=m.reserveOriginalResponse(cap,candidate);auto job=reservation.job;
   job->data["OriginalResponse"]["FlowControlled"]=true;job->data["OriginalResponse"]["State"]="Receiving";job->data["OriginalResponse"]["GrantedBytes"]=0;m.state["Settings"]["LimitKbps"]=1;m.state["Settings"]["QuotaMb"]=1;m.save();
   auto body=reservation.body.h,receipt=reservation.receipt.h;reservation.body.h=reservation.receipt.h=INVALID_HANDLE_VALUE;
   auto progress=std::make_shared<udmnet::ResponseProgress>(true);
   auto sink=std::make_unique<udmnet::TrackedResponseSink>(std::make_unique<udmnet::DurableResponseSink>(body,receipt,true,1024),progress);
   std::exception_ptr writerError;auto started=std::chrono::steady_clock::now();std::thread writer([&]{try{sink->write(std::string(1024,'x'));sink->complete();}catch(...){writerError=std::current_exception();}});
   try{Cancel wait;while(progress->phase.load()==udmnet::ResponsePhase::Receiving&&std::chrono::steady_clock::now()-started<std::chrono::seconds(5)){
     m.accountOriginalResponseBytes(job,static_cast<i64>(progress->received.load()));progress->grant(m.scheduledOriginalResponseCredits(job,64));wait.wait(10);
    }}catch(...){progress->cancel();writer.join();throw;}
   if(progress->phase.load()==udmnet::ResponsePhase::Receiving)progress->cancel();writer.join();sink.reset();
   if(writerError)std::rethrow_exception(writerError);
   auto elapsed=std::chrono::duration<double>(std::chrono::steady_clock::now()-started).count();m.accountOriginalResponseBytes(job,static_cast<i64>(progress->received.load()));
   check(progress->phase.load()==udmnet::ResponsePhase::Complete&&readText(m.root/L"responses"/wide(job->id())/L"body")==std::string(1024,'x'),"Scheduled credits deliver the exact body through the real helper writer");
   check(elapsed>=1.0&&elapsed<5.0,"Stopwatch confirms 1 KiB at 1 KiB/s is neither unrestricted nor stalled");
   check(num(m.state,"QuotaBytes")==1024&&m.capturedQuotaReserved()==0,"Timed writer enforces bandwidth and quota together without double charging");
   std::cout<<"TIMING captured writer: 1024 bytes, limit 1024 bytes/s, elapsed "<<elapsed<<" seconds"<<std::endl;
  }
  for(bool each:{false,true}){Manager m(root/(each?L"capture-cadence-each":L"capture-cadence-total"));m.state["Settings"]["LimitKbps"]=1;m.state["Settings"]["QuotaMb"]=1;m.state["Settings"]["GlobalLimitMode"]=each?"Each download":"Total across downloads";
   auto first=offer;first["Length"]=uint64_t(2048);auto second=first;second["Id"]=uint64_t(2);auto cap=capture;cap["PendingResponses"]=Json::array({first,second});
   std::vector<JobPtr> downloads;std::vector<std::shared_ptr<udmnet::ResponseProgress>> progress;std::vector<std::unique_ptr<udmnet::TrackedResponseSink>> sinks;std::vector<std::thread> writers;std::exception_ptr errors[2];
   for(const auto& candidate:{first,second}){auto reservation=m.reserveOriginalResponse(cap,candidate);auto job=reservation.job;job->data["OriginalResponse"]["FlowControlled"]=true;job->data["OriginalResponse"]["State"]="Receiving";job->data["OriginalResponse"]["GrantedBytes"]=0;downloads.push_back(job);
    auto body=reservation.body.h,receipt=reservation.receipt.h;reservation.body.h=reservation.receipt.h=INVALID_HANDLE_VALUE;progress.push_back(std::make_shared<udmnet::ResponseProgress>(true));sinks.push_back(std::make_unique<udmnet::TrackedResponseSink>(std::make_unique<udmnet::DurableResponseSink>(body,receipt,true,2048),progress.back()));
   }m.save();auto started=std::chrono::steady_clock::now();
   for(size_t i=0;i<2;++i)writers.emplace_back([&,i]{try{sinks[i]->write(std::string(2048,static_cast<char>('a'+i)));sinks[i]->complete();}catch(...){errors[i]=std::current_exception();}});
   auto cleanup=[&]{for(auto p:progress)if(p->phase.load()==udmnet::ResponsePhase::Receiving)p->cancel();for(auto& writer:writers)if(writer.joinable())writer.join();};
   try{Cancel wait;while(std::chrono::steady_clock::now()-started<std::chrono::seconds(10)){
     bool active=false;for(size_t i=0;i<2;++i){m.accountOriginalResponseBytes(downloads[i],static_cast<i64>(progress[i]->received.load()));if(progress[i]->phase.load()==udmnet::ResponsePhase::Receiving){active=true;progress[i]->grant(m.scheduledOriginalResponseCredits(downloads[i],64*1024*1024));}}
     if(!active)break;wait.wait(500);
    }}catch(...){cleanup();throw;}cleanup();sinks.clear();for(auto error:errors)if(error)std::rethrow_exception(error);
   double elapsed=0;for(size_t i=0;i<2;++i){elapsed=std::max(elapsed,progress[i]->seconds());m.accountOriginalResponseBytes(downloads[i],static_cast<i64>(progress[i]->received.load()));check(progress[i]->phase.load()==udmnet::ResponsePhase::Complete&&readText(m.root/L"responses"/wide(downloads[i]->id())/L"body")==std::string(2048,static_cast<char>('a'+i)),"Concurrent capture at production polling cadence preserves each exact body");}
   check(elapsed>=(each?2.0:4.0)&&elapsed<(each?3.5:6.0),each?"Each-download limit sustains independent streams at 500 ms polling":"Total limit sustains shared throughput at 500 ms polling");
   check(num(m.state,"QuotaBytes")==4096&&m.capturedQuotaReserved()==0,"Concurrent capture accounts all bytes once and releases reservations");
   std::cout<<"TIMING production cadence: two 2048-byte captures, 500 ms polling, "<<(each?"each":"total")<<" 1024 bytes/s, last writer completed in "<<elapsed<<" seconds"<<std::endl;
  }
  {Manager m(root/L"speed-credits");auto candidate=offer;candidate["Length"]=uint64_t(4096);auto cap=capture;cap["PendingResponses"]=Json::array({candidate});auto reservation=m.reserveOriginalResponse(cap,candidate);auto job=reservation.job;
   job->data["OriginalResponse"]["FlowControlled"]=true;job->data["OriginalResponse"]["State"]="Receiving";job->data["OriginalResponse"]["GrantedBytes"]=0;m.state["Settings"]["LimitKbps"]=1;m.save();
   auto started=std::chrono::steady_clock::now();
   check(m.scheduledOriginalResponseCredits(job,512)==0,"Rate-limited capture waits before releasing its first allowance");
   auto due=m.globalRate.next;
   check(due-started>=std::chrono::microseconds(500000),"Global timeline charges the exact captured byte duration");
   check(m.scheduledOriginalResponseCredits(job,512)==0&&m.globalRate.next==due,"Repeated pending grant does not charge bandwidth twice");
   Cancel wait;wait.wait(550);
   check(m.scheduledOriginalResponseCredits(job,512)==512,"Paid allowance becomes available after elapsed wall time");
   check(num(job->data["OriginalResponse"],"GrantedBytes")==1024,"One next allowance is scheduled while released bytes are in flight");
   auto secondDue=m.globalRate.next;
   check(m.scheduledOriginalResponseCredits(job,512)==512&&m.globalRate.next==secondDue,"Lost acknowledgement replays released bytes without another reservation");
   m.accountOriginalResponseBytes(job,512);
   check(m.scheduledOriginalResponseCredits(job,512)==512,"Accounting bytes cannot bypass the pending bandwidth wait");
   auto ordinary=m.add("https://example.invalid/shared-speed.bin");Cancel cancel;Rate rate;cancel.deadline=GetTickCount64()+10;bool cancelled=false;
   try{m.charge(512,cancel,rate,ordinary);}catch(const Cancelled&){cancelled=true;}
   check(cancelled&&m.globalRate.next>secondDue,"Ordinary transfers reserve behind captured traffic on the shared global timeline");
  }
  {Manager m(root/L"disable-global-retain-local");auto candidate=offer;candidate["Length"]=uint64_t(4096);auto cap=capture;cap["PendingResponses"]=Json::array({candidate});auto reserved=m.reserveOriginalResponse(cap,candidate);auto job=reserved.job;
   job->data["OriginalResponse"]["FlowControlled"]=true;job->data["OriginalResponse"]["State"]="Receiving";job->data["OriginalResponse"]["GrantedBytes"]=0;
   m.state["Settings"]["LimitKbps"]=1;job->sessionLimit=1;m.globalRate.next=std::chrono::steady_clock::now()+std::chrono::seconds(30);
   check(m.scheduledOriginalResponseCredits(job,512)==0,"Partial-disable fixture queues credits behind a busy global timeline");
   m.state["Settings"]["LimitKbps"]=0;
   check(m.scheduledOriginalResponseCredits(job,512)==0,"Removing global limit does not bypass an unpaid local allowance");
   Cancel wait;wait.wait(550);
   check(m.scheduledOriginalResponseCredits(job,512)==512,"Remaining local limit releases paid bytes without waiting for the disabled global queue");
  }
  {Manager m(root/L"disable-local-retain-global");auto candidate=offer;candidate["Length"]=uint64_t(4096);auto cap=capture;cap["PendingResponses"]=Json::array({candidate});auto reserved=m.reserveOriginalResponse(cap,candidate);auto job=reserved.job;
   job->data["OriginalResponse"]["FlowControlled"]=true;job->data["OriginalResponse"]["State"]="Receiving";job->data["OriginalResponse"]["GrantedBytes"]=0;
   m.state["Settings"]["LimitKbps"]=1;job->sessionLimit=1;m.captureCreditSchedules[job->id()].localNext=std::chrono::steady_clock::now()+std::chrono::seconds(30);
   check(m.scheduledOriginalResponseCredits(job,512)==0,"Local-disable fixture queues credits behind a busy local timeline");
   job->sessionLimit=0;
   check(m.scheduledOriginalResponseCredits(job,512)==0,"Removing the local limit does not bypass an unpaid global allowance");
   check(m.captureCreditSchedules.at(job->id()).localNext==std::chrono::steady_clock::time_point{},"Disabled local limit drops its obsolete reservation timeline");
   Cancel wait;wait.wait(550);
   check(m.scheduledOriginalResponseCredits(job,512)==512,"Remaining global limit releases paid bytes without waiting for the disabled local queue");
  }
  {Manager m(root/L"disable-capture-limit");auto reserved=m.reserveOriginalResponse(capture,offer);auto job=reserved.job;
   job->data["OriginalResponse"]["FlowControlled"]=true;job->data["OriginalResponse"]["State"]="Receiving";job->data["OriginalResponse"]["GrantedBytes"]=0;
   m.state["Settings"]["LimitKbps"]=1;m.state["Settings"]["QuotaMb"]=1;m.state["QuotaStart"]=date();m.state["QuotaBytes"]=1024*1024-2;job->sessionLimit=1;
   m.globalRate.next=std::chrono::steady_clock::now()+std::chrono::seconds(30);
   check(m.scheduledOriginalResponseCredits(job,4)==0,"Fixture reserves captured credits behind an occupied global bandwidth timeline");
   m.state["Settings"]["LimitKbps"]=0;
   m.scheduledOriginalResponseCredits(job,4);check(m.captureCreditSchedules.at(job->id()).ready==m.captureCreditSchedules.at(job->id()).localNext,"Disabling the global limit retains only the remaining per-download wait");
   job->sessionLimit=0;auto started=std::chrono::steady_clock::now();
   check(m.scheduledOriginalResponseCredits(job,4)==2&&std::chrono::steady_clock::now()-started<std::chrono::seconds(1),"Disabling all effective speed limits releases pending credits without the old delay");
   check(num(job->data["OriginalResponse"],"GrantedBytes")==2&&m.capturedQuotaReserved()==2,"Disabling speed limits cannot release more than the remaining quota");
  }
  {Manager m(root/L"speed-per-download");auto reserved=m.reserveOriginalResponse(capture,offer);auto job=reserved.job;job->data["OriginalResponse"]["FlowControlled"]=true;job->data["OriginalResponse"]["State"]="Receiving";job->data["OriginalResponse"]["GrantedBytes"]=0;
   m.state["Settings"]["GlobalLimitMode"]="Each download";m.state["Settings"]["LimitKbps"]=2;job->sessionLimit=1;
   auto began=std::chrono::steady_clock::now();auto released=m.scheduledOriginalResponseCredits(job,4);auto ended=std::chrono::steady_clock::now();
   auto& schedule=m.captureCreditSchedules.at(job->id());
   // Four bytes at 1 KiB/s take 3907 microseconds: persistence may finish later.
   check(m.globalRate.next==std::chrono::steady_clock::time_point{}&&schedule.localNext-began>=std::chrono::microseconds(3907),"Each-download mode uses a separate timeline and honors the lower session limit");
   check(schedule.ready>=schedule.localNext&&schedule.scheduled==4&&(released==0||(released==4&&ended>=schedule.localNext)),"Per-download allowance is delayed until its local reservation completes");
  }
  {Manager m(root/L"quota-credits");m.state["Settings"]["QuotaMb"]=1;m.state["QuotaStart"]=date(epoch());m.state["QuotaBytes"]=1024*1024-6;
   auto firstOffer=offer;firstOffer["Length"]=uint64_t(16);auto secondOffer=firstOffer;secondOffer["Id"]=uint64_t(2);auto cap=capture;cap["PendingResponses"]=Json::array({firstOffer,secondOffer});
   auto first=m.reserveOriginalResponse(cap,firstOffer),second=m.reserveOriginalResponse(cap,secondOffer);
   for(auto job:{first.job,second.job}){job->data["OriginalResponse"]["FlowControlled"]=true;job->data["OriginalResponse"]["State"]="Receiving";job->data["OriginalResponse"]["GrantedBytes"]=0;}
   m.save();
   check(m.reserveOriginalResponseCredits(first.job,4)==4,"First capture reserves its byte window");
   check(m.reserveOriginalResponseCredits(second.job,4)==2&&m.capturedQuotaReserved()==6,"Concurrent captures cannot overbook the shared quota");
   check(m.reserveOriginalResponseCredits(first.job,4)==4&&m.capturedQuotaReserved()==6,"Retry after lost grant acknowledgement reuses durable allowance");
   {auto ordinary=m.add("https://example.invalid/quota-wait.bin");Cancel cancel;Rate rate;cancel.deadline=GetTickCount64()+200;bool cancelled=false;
    try{m.charge(1,cancel,rate,ordinary);}catch(const Cancelled&){cancelled=true;}
    check(cancelled&&num(m.state,"QuotaBytes")==1024*1024-6,"Ordinary download cannot spend captured credits and remains cancellable");}
   m.accountOriginalResponseBytes(first.job,2);
   check(num(m.state,"QuotaBytes")==1024*1024-4&&m.capturedQuotaReserved()==4,"Received bytes move reservation into usage exactly once");
   rejects([&]{m.accountOriginalResponseBytes(second.job,3);},"Captured accounting rejects bytes above granted allowance");
   m.accountOriginalResponseBytes(first.job,4);m.accountOriginalResponseBytes(second.job,2);
   check(yes(m.quotaStatus(first.job),"Waiting")&&m.capturedQuotaReserved()==0,"Exhausted capture reports quota waiting");
   m.state["QuotaStart"]=date(epoch()-2*3600000);
   check(m.reserveOriginalResponseCredits(first.job,4)==8&&num(m.state,"QuotaBytes")==0,"Next quota period replenishes controlled capture");
   first.job->data["OriginalResponse"]["PauseRequested"]=true;
   check(m.reserveOriginalResponseCredits(first.job,8)==8,"Pause cannot grant additional response bytes");
   first.job->data["OriginalResponse"]["State"]="Interrupted";
   check(m.capturedQuotaReserved()==0,"Confirmed interrupted capture releases unused reservation");
   m.save();auto before=second.job->data;
   {Handle reader(CreateFileW((m.root/L"state.json.bak").c_str(),GENERIC_READ,FILE_SHARE_READ|FILE_SHARE_WRITE,nullptr,OPEN_EXISTING,0,nullptr));
    check(bool(reader),"Credit rollback fixture holds backup catalog");rejects([&]{m.reserveOriginalResponseCredits(second.job,4);},"Failed durable credit reservation is reported");
    check(second.job->data==before&&m.capturedQuotaReserved()==0,"Failed save rolls back credits before any grant can be sent");}
   check(m.reserveOriginalResponseCredits(second.job,4)==6,"Credit reservation can retry after catalog lock clears");
  }
  {auto data=root/L"quota-accounting";
   {Manager m(data);auto extended=offer;extended["Length"]=uint64_t(8);auto cap=capture;cap["PendingResponses"]=Json::array({extended});
    auto reserved=m.reserveOriginalResponse(cap,extended);auto job=reserved.job;auto body=reserved.body.h,receipt=reserved.receipt.h;reserved.body.h=reserved.receipt.h=INVALID_HANDLE_VALUE;
    udmnet::DurableResponseSink sink(body,receipt,true,8);sink.write("ab");m.accountOriginalResponseBytes(job,2);
    check(num(m.state,"QuotaBytes")==2&&num(job->data["OriginalResponse"],"AccountedBytes")==2,"Captured bytes enter quota with a durable watermark");
    auto ordinary=m.add("https://example.invalid/ordinary.bin");Cancel cancel;Rate rate;m.charge(3,cancel,rate,ordinary);
    sink.write("cd");m.accountOriginalResponseBytes(job,4);m.accountOriginalResponseBytes(job,4);
    check(num(m.state,"QuotaBytes")==7,"Captured and ordinary bytes share one quota without duplicate snapshot charging");
    sink.write("ef");
    {Handle reader(CreateFileW((data/L"state.json.bak").c_str(),GENERIC_READ,FILE_SHARE_READ|FILE_SHARE_WRITE,nullptr,OPEN_EXISTING,0,nullptr));
     check(bool(reader),"Quota fixture holds backup catalog");rejects([&]{m.accountOriginalResponseBytes(job,6);},"Failed quota checkpoint is reported");
     check(num(m.state,"QuotaBytes")==7&&num(job->data["OriginalResponse"],"AccountedBytes")==4,"Failed quota checkpoint rolls back usage and byte watermark together");}
    m.accountOriginalResponseBytes(job,6);check(num(m.state,"QuotaBytes")==9,"Retry charges only bytes missing from the durable watermark");
    m.state["QuotaStart"]=date(epoch()-2*3600000);m.save();sink.write("gh");m.accountOriginalResponseBytes(job,8);sink.complete();
    check(num(m.state,"QuotaBytes")==2&&num(job->data["OriginalResponse"],"AccountedBytes")==8,"New quota period charges only newly observed captured bytes");
    rejects([&]{m.accountOriginalResponseBytes(job,9);},"Accounting rejects bytes beyond the captured response length");
   }
   {Manager reopened(data);check(num(reopened.state,"QuotaBytes")==2&&num(reopened.jobs.at(0)->data["OriginalResponse"],"AccountedBytes")==8,"Receipt recovery does not recharge the captured prefix after restart");}
  }
  {auto data=root/L"quota-recovered-prefix";
   {Manager m(data);auto reserved=m.reserveOriginalResponse(capture,offer);auto body=reserved.body.h,receipt=reserved.receipt.h;reserved.body.h=reserved.receipt.h=INVALID_HANDLE_VALUE;udmnet::DurableResponseSink sink(body,receipt,true,4);sink.write("ab");}
   {Manager reopened(data);auto job=reopened.jobs.at(0);check(num(reopened.state,"QuotaBytes")==2&&num(job->data["OriginalResponse"],"AccountedBytes")==2,"Recovered partial bytes are accounted when their live snapshot was missed");
    reopened.state["QuotaBytes"]=std::numeric_limits<i64>::max();reopened.save();rejects([&]{reopened.accountOriginalResponseBytes(job,4);},"Quota accounting rejects overflow");check(num(reopened.state,"QuotaBytes")==std::numeric_limits<i64>::max()&&num(job->data["OriginalResponse"],"AccountedBytes")==2,"Overflow preserves quota and watermark");}
  }
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
  for(unsigned mode=0;mode<4;++mode){auto data=root/(L"validators-"+std::to_wstring(mode));auto candidate=offer;candidate["ETag"]=mode==1?"W/\"v1\"":mode==3?"bad\r\nheader":"\"v1\"";candidate["LastModified"]=mode==2?"invalid date":"Wed, 21 Oct 2015 07:28:00 GMT";auto cap=capture;cap["PendingResponses"]=Json::array({candidate});
   {Manager m(data);if(mode==3){rejects([&]{m.reserveOriginalResponse(cap,candidate);},"Header injection in validator is rejected before reservation");check(m.jobs.empty(),"Malformed validator creates no job");continue;}auto reservation=m.reserveOriginalResponse(cap,candidate);}
   Manager reopened(data);auto job=reopened.jobs.at(0);check(str(job->data,"ETag")== (mode==1?"":"\"v1\""),"Only strong original ETag survives restart as resume validator");check(str(job->data,"Modified")== (mode==2?"":"Wed, 21 Oct 2015 07:28:00 GMT"),"Only valid original modification date survives restart");
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
  if(argc>1)for(unsigned mode=0;mode<7;++mode){auto data=root/(L"resume-"+std::to_wstring(mode));Manager m(data);auto settings=m.state["Settings"];settings["DownloadFolder"]=utf8((data/L"files").wstring());settings["CategoryFolders"]=false;m.setSettings(settings);auto candidate=offer;candidate["Host"]=argv[1];candidate["Target"]="/resume-"+std::to_string(mode);candidate["ETag"]="\"v1\"";candidate["Length"]=uint64_t(8);
   if(mode>=4){udmnet::HttpRequest request;request.host=argv[1];request.target="/resume-"+std::to_string(mode);request.replayHeaders={{"cookie","fixture_session=allowed"},{"authorization","Bearer fixture-secret"}};candidate["ProtectedRequestHeaders"]=udmnet::protectedReplayHeaders(request);}
   auto cap=capture;cap["PendingResponses"]=Json::array({candidate});JobPtr job;
   {auto reserved=m.reserveOriginalResponse(cap,candidate);job=reserved.job;auto body=reserved.body.h,receipt=reserved.receipt.h;reserved.body.h=reserved.receipt.h=INVALID_HANDLE_VALUE;udmnet::DurableResponseSink sink(body,receipt,true,8);sink.write("abcd");}
   if(mode>=4){check(readHeaders(job->data).at("cookie")=="fixture_session=allowed","Native admission decrypts captured replay headers");check(readText(data/L"state.json").find("fixture_session")==std::string::npos,"Captured credentials are encrypted in the persisted catalog");Manager reopened(data);check(readHeaders(reopened.jobs.at(0)->data).at("authorization")=="Bearer fixture-secret","Captured replay credentials survive catalog reopen");}
   auto orphan=data/L"responses"/wide(job->id())/L"continuation"/L".seed-interrupted.tmp";fs::create_directories(orphan.parent_path());atomicText(orphan,"a",false);
   m.recoverOriginalResponses();m.resume(job);check(readText(orphan)=="a","An interrupted staging attempt does not block resume or get mistaken for the prefix");auto original=data/L"responses"/wide(job->id())/L"body";auto part=fs::path(wide(str(job->data,"PartsFolder")))/L"0000.part";
   check(!job->data.contains("OriginalResponse")&&yes(job->data,"PreserveResumeIdentity")&&readText(part)=="abcd"&&readText(original)=="abcd","Resume durably stages captured prefix without deleting original bytes");
   {Manager reopened(data);auto restored=reopened.jobs.at(0);check(yes(restored->data,"PreserveResumeIdentity")&&!restored->data.contains("OriginalResponse")&&num(restored->data["Segments"][0],"Done")==4&&readText(fs::path(wide(str(restored->data,"PartsFolder")))/L"0000.part")=="abcd","Restart retains staged captured prefix and strict identity validation");}
   if(mode==0||mode>=4){transfer(m,job,std::make_shared<Cancel>());check(str(job->data,"Status")=="Complete"&&readText(job->target())=="abcdefgh","Captured prefix and resumed suffix produce exact completed file");check(num(m.state,"QuotaBytes")==8,"Resumed capture charges the prefix and suffix exactly once");Manager completed(data);check(str(completed.jobs.at(0)->data,"Status")=="Complete"&&readText(completed.jobs.at(0)->target())=="abcdefgh","Resumed completion remains complete after reopening the catalog");}
   else{rejects([&]{transfer(m,job,std::make_shared<Cancel>());},"Changed or non-ranged server cannot replace captured prefix");check(readText(part)=="abcd"&&readText(original)=="abcd"&&!fs::exists(job->target()),"Failed range validation preserves both prefixes and publishes nothing");}
  }
  for(bool complete:{false,true}){Manager m(root/(complete?L"stopped-helper-complete":L"stopped-helper-partial"));JobPtr job;
   {auto reserved=m.reserveOriginalResponse(capture,offer);job=reserved.job;auto body=reserved.body.h,receipt=reserved.receipt.h;reserved.body.h=reserved.receipt.h=INVALID_HANDLE_VALUE;
    udmnet::DurableResponseSink sink(body,receipt,true,4);sink.write(complete?"abcd":"ab");if(complete)sink.complete();}
   job->data["OriginalResponse"]["State"]="Receiving";job->data["Status"]="Downloading";job->speed=1024;m.save();Monitor stopped;
   m.stopOriginalResponses(stopped);
   check(!stopped.running()&&str(job->data,"Status")=="Paused"&&job->speed==0&&!m.isActive(job),"Stopping a missing helper clears active download status without elevation");
   check(str(job->data["OriginalResponse"],"State")== (complete?"Ready":"Interrupted")&&num(job->data,"Received")== (complete?4:2),"Helper-free shutdown recovers the actual receipt state and byte count");
   check(readText(m.root/L"responses"/wide(job->id())/L"body")== (complete?"abcd":"ab")&&!fs::exists(job->target()),"Helper-free shutdown preserves captured bytes without false publication");
   m.stopOriginalResponses(stopped);check(num(m.state,"QuotaBytes")== (complete?4:2),"Repeated helper-free shutdown does not charge received bytes twice");
  }
  {Manager m(root/L"rollback");m.save();rejects([&]{m.reserveOriginalResponse(capture,offer,[](const char* phase){if(std::string(phase)=="before-commit")throw std::runtime_error("fixture save failure");});},"Failed catalog reservation is reported");check(m.jobs.empty()&&Json::parse(readText(m.root/L"state.json"))["Downloads"].empty(),"Failed reservation leaves no in-memory or persisted job");}
  {auto data=root/L"lost-ack";{Manager m(data);rejects([&]{m.reserveOriginalResponse(capture,offer,[](const char* phase){if(std::string(phase)=="committed")throw std::runtime_error("fixture lost reply");});},"Lost reservation acknowledgement is simulated after commit");}Manager reopened(data);check(reopened.jobs.size()==1&&str(reopened.jobs[0]->data["OriginalResponse"],"State")=="Unaccepted","Lost acknowledgement retains the durable reservation for recovery");}
 }catch(const std::exception& e){++failed;std::cout<<"FAIL "<<e.what()<<std::endl;}
 std::cout<<passed<<" passed, "<<failed<<" failed; evidence "<<utf8(root.wstring())<<std::endl;return failed?1:0;
}
