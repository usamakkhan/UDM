#pragma once
#include "CaptureReceipts.hpp"
static void captureReceiptChecks(const fs::path& root){
 auto token=[](i64 now=0){auto value=guid();value.insert(20,"-");value.insert(16,"-");value.insert(12,"-");value.insert(8,"-");return std::to_string(now?now:epoch())+"-"+value;};
 auto dir=root/L"capture-receipts";std::string accepted,identity;
 {
  Manager m(dir);auto settings=m.state["Settings"];settings["SkipBrowserFileInfo"]=false;settings["PrefetchFileInfo"]=false;settings["DuplicatePolicy"]="Numbered";m.setSettings(settings);
  accepted=token();Json request={{"action","add"},{"url","https://example.invalid/fixture.zip"},{"captureToken",accepted},{"downloadLater",true}};
  auto first=m.receive(request);identity=first->id();check(m.jobs.size()==1&&str(reconcileCapture(m,accepted),"status")=="accepted","Saved browser capture has an accepted receipt");
  auto again=m.receive(request);check(again==first&&m.jobs.size()==1,"Repeated capture token cannot create a second download");
  auto released=token();check(str(reconcileCapture(m,released),"status")=="released","Missing fresh capture creates a durable release barrier");request["captureToken"]=released;
  rejects([&]{m.receive(request);},"A delayed Add after browser release is rejected");check(m.jobs.size()==1,"Released capture never creates a late native job");
  auto pending=token();m.state["BrowserCaptures"][pending]={{"created",epoch()},{"status","pending"}};m.save();
  check(str(reconcileCapture(m,pending),"status")=="uncertain","Interrupted native reservation is never mistaken for a missing job");request["captureToken"]=pending;
  rejects([&]{m.receive(request);},"Uncertain native reservations cannot replay a download");
  auto expired=token(epoch()-600001);request["captureToken"]=expired;
  rejects([&]{m.receive(request);},"Expired Add token cannot replay after receipt cleanup");
  check(str(reconcileCapture(m,expired),"status")=="uncertain","Missing expired receipt does not release an ambiguous capture");
  request["captureToken"]=token(epoch()+61000);rejects([&]{m.receive(request);},"Far-future capture identity is rejected");
  request["captureToken"]="not-a-token";rejects([&]{m.receive(request);},"Malformed capture identity is rejected");
  check(m.state["BrowserCaptures"].dump().find("https://")==std::string::npos,"Native capture receipts contain no addresses or request bodies");
 }
 {
  Manager m(dir);check(str(reconcileCapture(m,accepted),"id")==identity,"Accepted capture remains discoverable after app restart");
  auto request=Json{{"action","add"},{"url","https://example.invalid/fixture.zip"},{"captureToken",accepted}};check(m.receive(request)->id()==identity&&m.jobs.size()==1,"Restarted native app suppresses repeat capture");
  auto job=m.jobs[0];m.remove(job);check(str(reconcileCapture(m,accepted),"status")=="accepted","Removing history cannot authorize replay of an accepted capture");
  rejects([&]{m.receive(request);},"Deleted accepted job is not recreated by replay");
 }
 {
  Manager m(root/L"capture-receipt-write-failure");auto block=m.root/L"state.json";fs::create_directory(block);auto id=token();
  rejects([&]{reconcileCapture(m,id);},"Release barrier requires a successful durable checkpoint");
  check(!m.state.contains("BrowserCaptures"),"Failed receipt checkpoint rolls back memory");
  fs::remove(block);
 }
}
