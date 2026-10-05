#pragma once
#include "MediaAdmission.hpp"
static std::string mediaToken(i64 when=0){auto id=guid();for(int i:{20,16,12,8})id.insert(i,"-");return std::to_string(when?when:epoch())+"-"+id;}
static Json mediaRequest(const std::string& token){
 return {{"action","adaptive"},{"mediaToken",token},{"url","https://media.example.test/watch"},{"filename","receipt fixture"},{"plan",{{"type","hls"},{"height",360},{"audioExpected",false},{"container","mp4"},{"tracks",Json::array({{{"kind","video"},{"segments",Json::array({{{"url","https://cdn.example.test/segment.ts"}}})}}})}}},{"originCookies",{{"https://cdn.example.test","session=synthetic"}}}};
}
static void mediaSettings(Manager& m,const fs::path& root){auto prefs=m.state["Settings"];prefs["DownloadFolder"]=utf8((root/L"files").wstring());prefs["CategoryFolders"]=false;prefs["SkipBrowserFileInfo"]=false;prefs["PrefetchFileInfo"]=false;m.setSettings(prefs);}
static int mediaCrashChild(const fs::path& root,const std::string& phase){
 Manager m(root/L"state");auto request=Json::parse(readText(root/L"request.json"));
 if(phase=="refresh-applied"){auto job=m.jobs.front();m.beginAddressRefresh(job,true);m.applyMediaRefresh(job);TerminateProcess(GetCurrentProcess(),197);ExitProcess(198);}
 m.receiveBrowserMedia(request,[&](const char* point){if(phase==point){TerminateProcess(GetCurrentProcess(),197);ExitProcess(198);}});
 return 9;
}
static DWORD mediaCrashRun(const fs::path& root,const std::string& phase){
 auto exe=appDir()/L"Udm.NativeTests.exe";auto command=quote(exe.wstring())+L" --media-crash-fixture "+quote(root.wstring())+L" "+quote(wide(phase));
 STARTUPINFOW si{sizeof(si)};PROCESS_INFORMATION pi{};if(!CreateProcessW(exe.c_str(),command.data(),nullptr,nullptr,FALSE,CREATE_NO_WINDOW,nullptr,appDir().c_str(),&si,&pi))throw std::runtime_error("Cannot launch media crash fixture.");
 Handle thread(pi.hThread),process(pi.hProcess);if(WaitForSingleObject(process.h,20000)!=WAIT_OBJECT_0){TerminateProcess(process.h,199);WaitForSingleObject(process.h,5000);throw std::runtime_error("Media crash fixture timed out.");}
 DWORD result=0;GetExitCodeProcess(process.h,&result);return result;
}
static void mediaAdmissionChecks(const fs::path& root){
 auto folder=root/L"media-receipts";auto request=mediaRequest(mediaToken());std::string id;
 {
  Manager m(folder);mediaSettings(m,root);auto job=m.receive(request);id=job->id();
  check(m.jobs.size()==1&&str(mediaAdmissionStatus(m,str(request,"mediaToken")),"id")==id,"Adaptive admission persists its job and accepted receipt");
  check(m.receive(request)==job&&m.jobs.size()==1,"Repeated adaptive submission returns the same job");
  auto envelope=request;envelope["requestId"]=17;check(m.receive(envelope)==job,"Native reply correlation ID is not part of the media selection");
  job->data["Status"]="Complete";m.save();check(m.receive(request)==job&&m.jobs.size()==1,"Receipt replay cannot duplicate a completed media job");
  auto changed=request;changed["filename"]="changed";rejects([&]{m.receive(changed);},"Receipt token cannot be reused for changed selection metadata");
  changed=request;changed["originCookies"]["https://cdn.example.test"]="session=changed";rejects([&]{m.receive(changed);},"Receipt token binds the exact media credential context");
  check(m.state["BrowserCaptures"].dump().find("synthetic")==std::string::npos&&m.state["BrowserCaptures"].dump().find("https://")==std::string::npos,"Media receipts do not store plaintext credentials or URLs");
 }
 {
  Manager m(folder);check(str(mediaAdmissionStatus(m,str(request,"mediaToken")),"id")==id&&m.receive(request)->id()==id,"Restart retains accepted media identity");
  m.remove(m.jobs.front());auto reply=mediaAdmissionStatus(m,str(request,"mediaToken"));check(str(reply,"status")=="accepted"&&!yes(reply,"present"),"Removed history remains an accepted receipt with no current record");
  rejects([&]{m.receive(request);},"Deleting history cannot authorize replay");
  auto released=mediaToken();check(str(mediaAdmissionStatus(m,released),"status")=="released","Missing fresh media receipt creates a durable late-submission barrier");
  rejects([&]{m.receive(mediaRequest(released));},"Submission after receipt release cannot create a late job");
  auto old=mediaToken(epoch()-600001);check(str(mediaAdmissionStatus(m,old),"status")=="uncertain","Expired missing receipt remains uncertain");rejects([&]{m.receive(mediaRequest(old));},"Expired media token cannot create a new job");
  rejects([&]{m.receive(mediaRequest("invalid"));},"Malformed media identity is rejected");
  auto other=mediaToken();m.state["BrowserCaptures"][other]={{"created",epoch()},{"status","released"}};m.save();rejects([&]{mediaAdmissionStatus(m,other);},"Ordinary-download token cannot be interpreted as media receipt");
  auto bad=mediaRequest(mediaToken());bad["plan"]["tracks"][0]["segments"][0]["url"]="file:///C:/secret";auto before=m.snapshot();rejects([&]{m.receive(bad);},"Invalid media is rejected before admission");check(m.snapshot()==before,"Rejected media cannot leave a job or receipt");
 }
 for(const auto& phase:{"deferred-save","before-commit","temporary-flushed"}){
  Manager m(root/wide(std::string("failure-")+phase));mediaSettings(m,root);auto before=m.snapshot();auto r=mediaRequest(mediaToken());
  rejects([&]{m.receiveBrowserMedia(r,[&](const char* step){if(std::string(step)==phase)throw std::runtime_error("Injected persistence failure.");});},std::string("Media admission reports failure at ")+phase);
  check(m.snapshot()==before&&m.jobs.empty(),std::string("Media admission rolls back before commit at ")+phase);
  check(m.receive(r)!=nullptr&&m.jobs.size()==1,"A rolled-back media admission can be retried once");
 }
 {
  Manager m(root/L"lost-ack");mediaSettings(m,root);auto r=mediaRequest(mediaToken());
  rejects([&]{m.receiveBrowserMedia(r,[](const char* p){if(std::string(p)=="committed")throw std::runtime_error("Lost reply.");});},"Lost acknowledgement is observable after commit");
  check(m.jobs.size()==1&&str(mediaAdmissionStatus(m,str(r,"mediaToken")),"status")=="accepted","Lost reply preserves the committed receipt and job");
  check(m.receive(r)==m.jobs.front()&&m.jobs.size()==1,"Retry after a lost reply reuses the committed job");
 }
 for(const auto& phase:{"deferred-save","before-commit","temporary-flushed","committed"}){
  auto dir=root/L"media-crash"/wide(phase);auto r=mediaRequest(mediaToken());
  {Manager m(dir/L"state");mediaSettings(m,dir);}atomicText(dir/L"request.json",r.dump());
  check(mediaCrashRun(dir,phase)==197,std::string("Media process terminates at ")+phase);
  Manager restored(dir/L"state");const bool committed=std::string(phase)=="committed";
  check(restored.jobs.size()==(committed?1:0),std::string("Restart has no orphan media job at ")+phase);
  auto reply=mediaAdmissionStatus(restored,str(r,"mediaToken"));check(str(reply,"status")==(committed?"accepted":"released"),"Restart reconciles media acceptance with durable state");
  if(committed)check(restored.receive(r)->id()==str(reply,"id")&&restored.jobs.size()==1,"Committed crash recovery cannot duplicate media");
  else rejects([&]{restored.receive(r);},"Uncommitted crash reconciliation rejects a delayed old submission");
 }
 {
  Manager m(root/L"media-refresh");mediaSettings(m,root);auto original=m.receive(mediaRequest(mediaToken()));original->data["Status"]="Paused";m.save();m.beginAddressRefresh(original);
  auto replacement=mediaRequest(mediaToken());replacement["plan"]["tracks"][0]["segments"][0]["url"]="https://cdn.example.test/segment.ts?renewed=1";
  check(m.receive(replacement)==original&&m.jobs.size()==1,"Receipt admission preserves an explicitly armed adaptive refresh target");
  check(str(m.addressRefreshCandidate(original),"kind")=="adaptive"&&!m.pendingBrowserPresentations().empty(),"Accepted adaptive refresh retains its review presentation");
  auto saved=m.addressRefreshCandidate(original);m.cancelAddressRefresh(original);check(m.receive(replacement)==original&&m.jobs.size()==1,"Replaying a refresh receipt does not create another download");
  m.beginAddressRefresh(original,true);check(m.addressRefreshCandidate(original)==saved,"Recovered media refresh preserves the captured replacement");
  m.cancelAddressRefresh(original);m.beginAddressRefresh(original);check(m.addressRefreshCandidate(original).is_null(),"Explicit new media recovery clears the previous captured session");

 }
 {
  auto dir=root/L"media-refresh-atomic";auto initial=mediaRequest(mediaToken()),replacement=mediaRequest(mediaToken());replacement["plan"]["tracks"][0]["segments"][0]["url"]="https://cdn.example.test/segment.ts?atomic=1";
  {Manager m(dir/L"state");mediaSettings(m,dir);auto job=m.receive(initial);
   for(const auto& offer:m.pendingBrowserPresentations())m.finishBrowserPresentation(offer);
   m.beginAddressRefresh(job);m.receive(replacement);auto before=m.snapshot();
   {Handle reader(CreateFileW((m.root/L"state.json").c_str(),GENERIC_READ,FILE_SHARE_READ,nullptr,OPEN_EXISTING,0,nullptr));
    check(bool(reader),"Atomic media refresh fixture locks catalog replacement");
    rejects([&]{m.applyMediaRefresh(job);},"Failed session save reports catalog write failure");
    check(m.snapshot()==before,"Failed session save restores job and review receipt together");
   }
   check(!m.pendingBrowserPresentations().empty(),"Failed session save retains the review prompt for retry");
  }
  atomicText(dir/L"request.json",replacement.dump());
  check(mediaCrashRun(dir,"refresh-applied")==197,"Process terminates immediately after applying a media session");
  Manager restored(dir/L"state");auto job=restored.jobs.front();
  check(Json::parse(reveal(str(job->data,"ProtectedAdaptive"))).dump().find("?atomic=1")!=std::string::npos,"Applied media session survives termination");
  check(restored.pendingBrowserPresentations().empty(),"Applied session cannot resurrect its consumed refresh prompt after termination");
  check(str(mediaAdmissionStatus(restored,str(replacement,"mediaToken")),"id")==job->id()&&restored.jobs.size()==1,"Consuming the prompt preserves receipt identity and the original job");
 }

}
