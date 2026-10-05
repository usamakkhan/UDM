#pragma once
#include "CaptureTransactions.hpp"
static void capturePresentationChecks(const fs::path& root){
 auto ordinary=[&](Manager& m,bool later=false,bool skip=false){
  auto settings=m.state["Settings"];settings["DownloadFolder"]=utf8((m.root/L"files").wstring());settings["CategoryFolders"]=false;settings["PrefetchFileInfo"]=false;settings["DuplicatePolicy"]="Numbered";settings["SkipBrowserFileInfo"]=skip;m.setSettings(settings);
  auto token=captureAtomicToken();prepareCapture(m,{{"captureToken",token},{"download",{{"action","add"},{"url","https://example.invalid/prompt.zip"},{"downloadLater",later}}}});commitPreparedCapture(m,token);return token;
 };
 auto folder=root/L"capture-present-information";std::string token,id;
 {Manager m(folder);token=ordinary(m);id=m.jobs.front()->id();auto pending=m.pendingBrowserPresentations();check(pending.size()==1&&str(pending[0],"kind")=="download","New confirmation is recorded with accepted browser admission");check(m.state["BrowserCaptures"][token]["presentation"].dump().find("https:")==std::string::npos,"Saved presentation contains no download URL or request credentials");}
 {Manager m(folder);auto offer=m.pendingBrowserPresentations().at(0);auto job=m.restoreBrowserPresentation(offer);check(job&&job->id()==id&&str(job->data,"Status")=="Paused"&&m.presentOffer(job)==OfferPresentation::Information,"Restart restores the paused download's File Info presentation");m.pause(job);check(m.pendingBrowserPresentations().empty(),"Cancel or manual pause consumes the saved presentation with its catalog write");}
 {Manager m(folder);check(m.pendingBrowserPresentations().empty()&&m.jobs.size()==1,"Canceled File Info does not reopen or create another download after restart");}
 for(bool skip:{false,true}){Manager m(root/(skip?L"capture-present-skip":L"capture-present-later"));ordinary(m,!skip,skip);check(m.pendingBrowserPresentations().empty(),skip?"Skip File Info keeps ordinary queued downloads free of a forced prompt":"Download later does not introduce an unwanted File Info prompt");}
 for(const auto& scenario:{"existing","replace-complete"}){
  auto dir=root/L"capture-present-existing"/wide(scenario);auto spec=captureAtomicFixture(dir,scenario);
  {Manager m(dir/L"state");if(std::string(scenario)=="replace-complete"){auto p=m.state["Settings"];p["DuplicatePolicy"]="Existing";m.setSettings(p);}commitPreparedCapture(m,str(spec,"token"));check(m.pendingBrowserPresentations().size()==1&&str(m.pendingBrowserPresentations()[0],"kind")=="existing","An existing-download offer is durable");}
  {Manager m(dir/L"state");auto offer=m.pendingBrowserPresentations().at(0);auto job=m.restoreBrowserPresentation(offer);auto shown=m.presentOffer(job);check(shown==(std::string(scenario)=="existing"?OfferPresentation::Progress:OfferPresentation::Complete),"Restored existing offer retains Resume versus Download complete behavior");m.finishBrowserPresentation(offer);check(m.pendingBrowserPresentations().empty()&&m.jobs.size()==1,"Existing presentation completes without creating another history row");}
 }
 for(const auto& choice:{"Existing","Replace","Numbered","Cancel"}){
  auto dir=root/L"capture-present-choice"/wide(choice);auto spec=captureAtomicFixture(dir,"ask");std::string resultId;
  {Manager m(dir/L"state");auto admitted=commitPreparedCapture(m,str(spec,"token"));JobPtr candidate;for(auto j:m.jobs)if(j->id()==str(admitted,"id"))candidate=j;auto result=m.resolveDuplicate(candidate,choice);if(result)resultId=result->id();check(m.pendingBrowserPresentations().size()==(std::string(choice)=="Cancel"?0:1),"Duplicate choice retains exactly its pending presentation");}
  {Manager m(dir/L"state");auto pending=m.pendingBrowserPresentations();if(std::string(choice)=="Cancel"){check(pending.empty()&&m.jobs.size()==1,"Canceled duplicate has no orphan presentation after restart");continue;}auto offer=pending.at(0);check(str(offer,"id")==resultId,"Duplicate decision retargets the durable presentation to its actual record");auto job=m.restoreBrowserPresentation(offer);check(m.presentOffer(job)==(std::string(choice)=="Existing"?OfferPresentation::Progress:OfferPresentation::Information),"Duplicate decision resumes with the correct dialog after restart");m.finishBrowserPresentation(offer);}
 }
 {
  auto dir=root/L"capture-present-ack";auto spec=captureAtomicFixture(dir,"existing");Manager m(dir/L"state");commitPreparedCapture(m,str(spec,"token"));auto first=m.pendingBrowserPresentations().at(0);
  auto secondToken=captureAtomicToken();auto original=Json::parse(readText(dir/L"fixture.json"));
  prepareCapture(m,{{"captureToken",secondToken},{"download",{{"action","add"},{"url","https://example.invalid/fixture.zip"},{"filename","fixture.zip"},{"cookies","fixture=atomic"}}}});commitPreparedCapture(m,secondToken);
  check(m.pendingBrowserPresentations().size()==1&&m.pendingBrowserPresentations()[0]["tokens"].size()==2,"Repeated offers for one record are combined into one pending dialog");
  check(!m.restoreBrowserPresentation(first),"A stale UI snapshot cannot dispatch over a newer capture");
  m.finishBrowserPresentation(first);auto pending=m.pendingBrowserPresentations();check(pending.size()==1&&pending[0]["tokens"]==Json::array({secondToken}),"An older dialog acknowledgement leaves a later capture pending");
  auto before=m.snapshot();Handle reader(CreateFileW((m.root/L"state.json").c_str(),GENERIC_READ,FILE_SHARE_READ,nullptr,OPEN_EXISTING,0,nullptr));
  rejects([&]{m.finishBrowserPresentation(pending[0]);},"Presentation acknowledgement requires a durable catalog write");check(m.snapshot()==before,"Failed acknowledgement preserves the exact pending presentation");
  auto job=m.jobs.front();rejects([&]{m.resume(job);},"A storage failure rejects resume and presentation consumption together");check(m.snapshot()==before,"Failed resume restores job state and its presentation");
  CloseHandle(reader.h);reader.h=INVALID_HANDLE_VALUE;m.remove(job);check(m.pendingBrowserPresentations().empty(),"Removing a record removes its presentation without revoking the accepted receipt");check(str(captureTransactionStatus(m,secondToken),"status")=="accepted","Removing a presented download cannot permit automatic replay");
 }
 {
  auto dir=root/L"capture-present-aging";Manager m(dir);auto t=ordinary(m);m.state["BrowserCaptures"][t]["created"]=epoch()-172800000;auto rows=captureStore(m);captureSpace(rows,epoch());check(rows.contains(t),"Receipt aging preserves an unhandled presentation");m.finishBrowserPresentation(m.pendingBrowserPresentations().at(0));rows=captureStore(m);captureSpace(rows,epoch());check(!rows.contains(t),"Acknowledged presentations no longer prevent receipt aging");
 }
 {
  auto dir=root/L"capture-present-refresh";auto spec=captureAtomicFixture(dir,"refresh");
  {Manager m(dir/L"state");commitPreparedCapture(m,str(spec,"token"));check(str(m.pendingBrowserPresentations().at(0),"kind")=="refresh","Captured replacement address saves its refresh presentation");}
  {Manager m(dir/L"state");auto offer=m.pendingBrowserPresentations().at(0);auto job=m.restoreBrowserPresentation(offer);auto fresh=m.addressRefreshCandidate(job);m.refreshAddress(job,str(fresh,"url"),fresh["headers"].get<Headers>(),str(fresh,"page"));check(m.pendingBrowserPresentations().empty()&&str(job->data,"Url")=="https://example.invalid/fixture.zip?renewed=1","Saving a recovered refresh address consumes its prompt atomically");}
 }
 {
  auto dir=root/L"capture-present-retarget-fail";auto spec=captureAtomicFixture(dir,"ask");Manager m(dir/L"state");auto accepted=commitPreparedCapture(m,str(spec,"token"));JobPtr candidate;for(auto j:m.jobs)if(j->id()==str(accepted,"id"))candidate=j;auto before=m.snapshot();Handle reader(CreateFileW((m.root/L"state.json").c_str(),GENERIC_READ,FILE_SHARE_READ,nullptr,OPEN_EXISTING,0,nullptr));
  rejects([&]{m.resolveDuplicate(candidate,"Existing");},"A failed duplicate choice cannot consume or retarget a prompt");check(m.snapshot()==before,"Failed duplicate retarget restores both history and presentation");
 }
}
