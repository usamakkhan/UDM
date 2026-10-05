#pragma once
#include "CaptureTransactions.hpp"
static void captureReviewChecks(const fs::path& root){
 auto fixture=[&](const std::string& name,const std::string& change){
  auto dir=root/L"capture-review"/wide(name);auto spec=captureAtomicFixture(dir,"refresh");Manager m(dir/L"state");auto job=m.jobs.front();
  auto part=m.root/L"parts"/wide(job->id())/L"0000.part";fs::create_directories(part.parent_path());writeBytes(part,Bytes(71,0x6b));
  job->data["Size"]=2048;job->data["Received"]=71;job->data["Segments"]=Json::array({{{"Index",0},{"Start",0},{"End",2047},{"Done",71}}});
  if(change=="removed")m.remove(job);
  else if(change=="complete")job->data["Status"]="Complete";
  else job->data["Url"]="https://example.invalid/changed.zip";
  m.save();spec["original"]=job->data;spec["partialFile"]=utf8(part.wstring());spec["partialHash"]=fileHash(part);
  spec["directory"]=utf8(m.root.wstring());auto before=m.snapshot();
  check(str(commitPreparedCapture(m,str(spec,"token")),"status")=="review","Changed refresh target durably enters review: "+name);
  check(m.snapshot()["Downloads"]==before["Downloads"]&&std::none_of(m.jobs.begin(),m.jobs.end(),[&](JobPtr j){return m.isActive(j);}),"Entering review never changes or starts a download: "+name);
  return spec;
 };
 auto managerFor=[](const Json& spec){return std::make_unique<Manager>(fs::path(wide(str(spec,"directory"))));};
 auto preserved=[&](const Json& spec){check(fileHash(fs::path(wide(str(spec,"partialFile"))))==str(spec,"partialHash"),"Review decision preserves original partial bytes");};
 {
  auto spec=fixture("refresh","address");auto m=managerFor(spec);auto token=str(spec,"token");auto view=m->browserCaptureReview(token);auto old=m->jobs.front()->data;
  check(m->pendingBrowserCaptureReviews()==Json::array({token})&&yes(view,"canRefresh"),"Pending review and eligible current target survive reopening");
  check(str(commitPreparedCapture(*m,token),"status")=="review"&&m->jobs.size()==1,"Repeated commit of a review never replays the request");
  auto rows=captureStore(*m);rows[token]["created"]=epoch()-172800000;captureSpace(rows,epoch());check(rows.contains(token),"Receipt aging retains an undecided protected link");
  const auto journal=captureStore(*m).dump();check(journal.find("https:")==std::string::npos&&journal.find("fixture=atomic")==std::string::npos,"Review keeps URLs and credentials protected at rest");
  auto result=m->resolveBrowserCaptureReview(view,"refresh");auto offer=m->addressRefreshCandidate(m->jobs.front());
  check(str(result,"status")=="accepted"&&str(result,"id")==str(old,"Id"),"Reviewed replacement keeps its original download identity");
  check(str(m->jobs.front()->data,"Url")==str(old,"Url")&&m->jobs.front()->data["Segments"]==old["Segments"],"Review replacement only stages the address; it does not replace current URL or parts");
  check(str(m->pendingBrowserPresentations().at(0),"kind")=="refresh","Reviewed replacement opens the ordinary address dialog");
  m->refreshAddress(m->jobs.front(),str(offer,"url"),offer["headers"].get<Headers>(),str(offer,"page"));
  check(str(m->jobs.front()->data,"Url")=="https://example.invalid/fixture.zip?renewed=1"&&yes(m->jobs.front()->data,"RefreshPendingValidation"),"Reviewed Save still requires server-validator checks before reusing parts");preserved(spec);
 }
 for(const auto& change:{"address","removed","complete"}){
  auto spec=fixture(std::string("new-")+change,change);auto m=managerFor(spec);auto token=str(spec,"token");auto before=m->snapshot()["Downloads"];
  auto settings=m->state["Settings"];settings["DuplicatePolicy"]="Existing";settings["SkipBrowserFileInfo"]=true;m->setSettings(settings);
  auto result=m->resolveBrowserCaptureReview(m->browserCaptureReview(token),"new");auto created=m->jobs.back();
  check(str(created->data,"Status")=="Awaiting confirmation"&&!yes(created->data,"QueueMember")&&std::none_of(m->jobs.begin(),m->jobs.end(),[&](JobPtr j){return m->isActive(j);}),"New-file review requires File Info even with Skip File Info enabled");
  check(m->jobs.size()==before.size()+1&&created->id()!=str(spec,"originalId"),"Explicit new-file decision creates its own record: "+std::string(change));
  for(size_t i=0;i<before.size();++i)check(m->jobs[i]->data==before[i],"New-file review leaves previous downloads unchanged");
  check(str(captureTransactionStatus(*m,token),"status")=="accepted"&&str(result,"id")==created->id(),"New file and accepted receipt publish together");
  rejects([&]{m->resolveBrowserCaptureReview(m->browserCaptureReview(token),"new");},"Repeating an already-handled review cannot create another file");preserved(spec);
 }
 {
  auto spec=fixture("stale","address");auto m=managerFor(spec);auto token=str(spec,"token");auto view=m->browserCaptureReview(token);
  m->jobs.front()->data["FileName"]="changed-again.zip";m->save();auto before=m->snapshot();
  for(const auto& choice:{"new","refresh","discard"})rejects([&]{m->resolveBrowserCaptureReview(view,choice);},"Stale review cannot apply a decision: "+std::string(choice));
  check(m->snapshot()==before,"Stale review rejection preserves job and protected request");
  view=m->browserCaptureReview(token);m->remove(m->jobs.front());rejects([&]{m->resolveBrowserCaptureReview(view,"refresh");},"Removal after display cannot redirect the captured link");
  check(!yes(m->browserCaptureReview(token),"canRefresh"),"Removed target disables replacement review");preserved(spec);
 }
 {
  auto spec=fixture("discard","complete");auto m=managerFor(spec);auto token=str(spec,"token");auto view=m->browserCaptureReview(token);auto before=m->snapshot()["Downloads"];
  check(!yes(view,"canRefresh"),"A completed target cannot be changed by link review");
  rejects([&]{m->resolveBrowserCaptureReview(view,"refresh");},"Completed target rejects a forced replacement choice");
  check(str(m->resolveBrowserCaptureReview(view,"discard"),"status")=="discarded","Discard resolves only the retained captured link");
  check(m->snapshot()["Downloads"]==before&&m->pendingBrowserCaptureReviews().empty()&&!captureStore(*m)[token].contains("request"),"Discard retains original jobs and removes protected request material");
  check(str(releasePreparedCapture(*m,token),"status")=="discarded"&&str(commitPreparedCapture(*m,token),"status")=="discarded","Discard is terminal and can never request browser resume or native replay");preserved(spec);
 }
 for(const auto& choice:{"new","refresh","discard"}){
  auto spec=fixture(std::string("rollback-")+choice,"address");auto m=managerFor(spec);auto token=str(spec,"token");auto view=m->browserCaptureReview(token);auto before=m->snapshot();auto original=readText(m->root/L"state.json");
  Handle reader(CreateFileW((m->root/L"state.json").c_str(),GENERIC_READ,FILE_SHARE_READ,nullptr,OPEN_EXISTING,0,nullptr));check(bool(reader),"Review rollback fixture locks the catalog");
  rejects([&]{m->resolveBrowserCaptureReview(view,choice);},"Review reports a failed durable save: "+std::string(choice));
  check(m->snapshot()==before&&readText(m->root/L"state.json")==original,"Failed review save restores jobs, protected request and receipt: "+std::string(choice));
  CloseHandle(reader.h);reader.h=INVALID_HANDLE_VALUE;
  check(yes(m->resolveBrowserCaptureReview(view,choice),"ok"),"Review can be retried after storage recovery: "+std::string(choice));preserved(spec);
 }
}
