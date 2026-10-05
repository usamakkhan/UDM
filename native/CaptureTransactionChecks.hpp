#pragma once
#include "CaptureTransactions.hpp"
static void captureTransactionChecks(const fs::path& root){
 auto token=[](i64 now=0){auto value=guid();for(int at:{20,16,12,8})value.insert(at,"-");return std::to_string(now?now:epoch())+"-"+value;};
 auto request=[](const std::string& name){return Json{{"action","add"},{"url","https://example.invalid/"+name+".zip"},{"headers",{{"Authorization","Basic dGVzdDpzZWNyZXQ="}}},{"downloadLater",true}};};
 auto dir=root/L"capture-transactions";auto id=token();std::string jobId;
 {
  Manager m(dir);auto settings=m.state["Settings"];settings["DuplicatePolicy"]="Numbered";settings["SkipBrowserFileInfo"]=false;settings["PrefetchFileInfo"]=false;m.setSettings(settings);
  auto message=Json{{"captureToken",id},{"download",request("first")}};
  check(str(prepareCapture(m,message),"status")=="prepared"&&m.jobs.empty(),"Preparing a browser handoff creates no native job");
  check(str(prepareCapture(m,message),"status")=="prepared"&&m.jobs.empty(),"Lost prepare reply can be retried without admission");
  auto stored=m.state["BrowserCaptures"].dump();check(stored.find("https:")==std::string::npos&&stored.find("Authorization")==std::string::npos&&stored.find("dGVzd")==std::string::npos,"Prepared URLs and credentials are protected in the native catalog");
  check(preparedCaptureEnvelope(m.state["BrowserCaptures"][id]).at("download")==request("first"),"Prepared request decrypts without changing headers or URL");
  auto altered=message;altered["download"]["url"]="https://other.invalid/file.zip";rejects([&]{prepareCapture(m,altered);},"A prepare token cannot be rebound to another request");
  check(str(reconcileCapture(m,id),"status")=="uncertain","Legacy reconciliation cannot release a prepared transaction");
  auto legacy=request("first");legacy["captureToken"]=id;rejects([&]{m.receive(legacy);},"Legacy Add cannot bypass the prepared commit boundary");
  auto missing=token();check(str(captureTransactionStatus(m,missing),"status")=="released","Status before prepare establishes a durable release barrier");
  check(str(prepareCapture(m,{{"captureToken",missing},{"download",request("late")}}),"status")=="released"&&m.jobs.empty(),"Delayed preparation cannot cross a release barrier");
  auto cancel=token();prepareCapture(m,{{"captureToken",cancel},{"download",request("cancel")}});
  check(str(releasePreparedCapture(m,cancel),"status")=="released"&&!m.state["BrowserCaptures"][cancel].contains("request"),"Releasing a prepared handoff removes its protected request");
  check(str(commitPreparedCapture(m,cancel),"status")=="released"&&m.jobs.empty(),"Commit after release never creates a native download");
  auto bad=request("bad");bad["action"]="media";rejects([&]{prepareCapture(m,{{"captureToken",token()},{"download",bad}});},"Preparation rejects nonordinary download actions");
  bad=request("bad");bad["url"]="file:///C:/private.txt";rejects([&]{prepareCapture(m,{{"captureToken",token()},{"download",bad}});},"Preparation rejects nonnetwork URLs");
  bad=request("bad");bad["headers"]["Authorization"]="bad\r\nheader";rejects([&]{prepareCapture(m,{{"captureToken",token()},{"download",bad}});},"Preparation validates browser request headers before cancellation");
  rejects([&]{prepareCapture(m,{{"captureToken",token(epoch()-600001)},{"download",request("old")}});},"Fresh preparation rejects expired capture identities");
 }
 {
  Manager m(dir);check(str(captureTransactionStatus(m,id),"status")=="prepared"&&m.jobs.empty(),"Prepared handoff survives desktop restart without starting a transfer");
  auto first=commitPreparedCapture(m,id);jobId=str(first,"id");check(str(first,"status")=="accepted"&&m.jobs.size()==1,"Confirmed browser cancellation permits one native admission");
  check(commitPreparedCapture(m,id)==first&&m.jobs.size()==1,"Lost commit reply does not create a second native job");
  check(!m.state["BrowserCaptures"][id].contains("request"),"Accepted transaction removes the extra encrypted request copy");
  check(str(releasePreparedCapture(m,id),"status")=="accepted","Late release cannot revoke an accepted native job");
 }
 {
  Manager m(dir);check(str(captureTransactionStatus(m,id),"id")==jobId&&m.jobs.size()==1,"Accepted transaction remains identifiable after restart");
  m.remove(m.jobs.front());check(str(commitPreparedCapture(m,id),"status")=="accepted"&&m.jobs.empty(),"Deleted history never resurrects an accepted transaction");
  auto pending=token();m.state["BrowserCaptures"][pending]={{"created",epoch()-172800000},{"protocol",2},{"status","pending"},{"request",protect(request("uncertain").dump())}};m.save();
  check(str(commitPreparedCapture(m,pending),"status")=="uncertain"&&str(releasePreparedCapture(m,pending),"status")=="uncertain","Interrupted native admission cannot replay or release ambiguously");
  auto rows=captureStore(m);captureSpace(rows,epoch());check(rows.contains(pending),"Unresolved protected requests are not discarded by receipt aging");
  auto fresh=token();prepareCapture(m,{{"captureToken",fresh},{"download",request("disk")}});
  auto blocked=m.root/L"state.json.tmp";fs::create_directory(blocked);
  rejects([&]{commitPreparedCapture(m,fresh);},"Commit refuses admission when the durable reservation cannot be written");
  check(str(captureTransactionStatus(m,fresh),"status")=="prepared"&&m.jobs.empty(),"Failed commit checkpoint preserves the prepared request and empty catalog");
  rejects([&]{releasePreparedCapture(m,fresh);},"Release requires a durable barrier before browser resumption");
  fs::remove(blocked);check(str(releasePreparedCapture(m,fresh),"status")=="released","Release succeeds after storage recovers");
 }
}
