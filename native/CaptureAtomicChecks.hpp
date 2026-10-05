#pragma once
#include "CaptureTransactions.hpp"
static std::string captureAtomicToken(){auto value=guid();for(int at:{20,16,12,8})value.insert(at,"-");return std::to_string(epoch())+"-"+value;}
static Json captureAtomicFixture(const fs::path& folder,const std::string& scenario){
 Manager m(folder/L"state");auto settings=m.state["Settings"];
 settings["DownloadFolder"]=utf8((folder/L"files").wstring());settings["CategoryFolders"]=false;settings["PrefetchFileInfo"]=false;settings["SkipBrowserFileInfo"]=true;
 settings["DuplicatePolicy"]=scenario=="existing"?"Existing":scenario=="ask"?"Ask":scenario=="replace-partial"||scenario=="replace-complete"?"Replace":"Numbered";
 m.setSettings(settings);
 Json request={{"action","add"},{"url","https://example.invalid/fixture.zip"},{"filename","fixture.zip"},{"downloadLater",true},{"cookies","fixture=atomic"}};
 Json spec={{"scenario",scenario},{"token",captureAtomicToken()},{"beforeCount",0},{"afterCount",1}};
 if(scenario!="new"){
  auto old=m.add(str(request,"url"),str(settings,"DownloadFolder"),"fixture.zip","Main queue",true,browserHeaders(request));
  spec["originalId"]=old->id();spec["beforeCount"]=1;spec["afterCount"]=(scenario=="existing"||scenario=="replace-partial"||scenario=="refresh")?1:2;
  if(scenario=="replace-complete"){
   fs::create_directories(old->target().parent_path());writeBytes(old->target(),Bytes(89,0x61));old->data["Status"]="Complete";old->data["Size"]=89;old->data["Received"]=89;
   spec["preservedFile"]=utf8(old->target().wstring());spec["preservedHash"]=fileHash(old->target());
  }else if(scenario=="replace-partial"){
   auto part=m.root/L"parts"/wide(old->id())/L"0000.part";fs::create_directories(part.parent_path());writeBytes(part,Bytes(64,0x5a));
   old->data["Size"]=4096;old->data["Received"]=64;old->data["Segments"]=Json::array({{{"Index",0},{"Start",0},{"End",4095},{"Done",64}}});
   spec["partialFile"]=utf8(part.wstring());spec["partialHash"]=fileHash(part);
  }
  m.save();
  if(scenario=="refresh"){m.beginAddressRefresh(old);request["url"]="https://example.invalid/fixture.zip?renewed=1";}
 }
 prepareCapture(m,{{"captureToken",spec["token"]},{"download",request}});
 atomicText(folder/L"fixture.json",spec.dump());return spec;
}
static int captureAtomicCrashChild(const fs::path& folder,const std::string& phase){
 const auto spec=Json::parse(readText(folder/L"fixture.json"));Manager m(folder/L"state");unsigned deferred=0;
 m.commitBrowserCapture(str(spec,"token"),[&](const char* point){
  std::string name=point;if(name=="deferred-save")name+=":"+std::to_string(++deferred);
  if(name==phase){TerminateProcess(GetCurrentProcess(),197);ExitProcess(198);}
 });
 return 9; // A requested checkpoint that was not reached must fail the parent test.
}
static DWORD captureAtomicChild(const fs::path& folder,const std::string& phase){
 auto exe=appDir()/L"Udm.NativeTests.exe";
 auto command=quote(exe.wstring())+L" --capture-crash-fixture "+quote(folder.wstring())+L" "+quote(wide(phase));
 STARTUPINFOW start{sizeof(start)};PROCESS_INFORMATION process{};
 if(!CreateProcessW(exe.c_str(),command.data(),nullptr,nullptr,FALSE,CREATE_NO_WINDOW,nullptr,appDir().c_str(),&start,&process))throw std::runtime_error("Cannot launch the isolated capture crash fixture.");
 Handle thread(process.hThread),child(process.hProcess);auto waited=WaitForSingleObject(child.h,20000);
 if(waited!=WAIT_OBJECT_0){TerminateProcess(child.h,199);WaitForSingleObject(child.h,5000);throw std::runtime_error("Capture crash fixture timed out.");}
 DWORD code=0;if(!GetExitCodeProcess(child.h,&code))throw std::runtime_error("Cannot read the capture crash fixture exit code.");return code;
}
static void captureAtomicChecks(const fs::path& root){
 const std::vector<std::string> scenarios={"new","numbered","existing","ask","replace-partial","replace-complete","refresh"};
 for(const auto& scenario:scenarios){
  auto phases=std::vector<std::string>{"deferred-save:1","deferred-save:2","before-commit","temporary-flushed","committed"};
  if(scenario!="refresh")phases.insert(phases.begin()+2,"deferred-save:3");
  for(const auto& phase:phases){
   const auto label=scenario+" / "+phase;auto folder=root/L"capture-crash"/wide(scenario)/wide(guid());auto spec=captureAtomicFixture(folder,scenario);auto token=str(spec,"token");
   check(captureAtomicChild(folder,phase)==197,"Child process terminates at "+label);
   const bool committed=phase=="committed";auto disk=Json::parse(readText(folder/L"state/state.json"));
   auto receipt=disk["BrowserCaptures"][token];
   check(str(receipt,"status")== (committed?"accepted":"prepared"),"Receipt has a definite durable state after "+label);
   check(disk["Downloads"].size()==(size_t)num(spec,committed?"afterCount":"beforeCount"),"No orphan admission is saved after "+label);
   if(spec.contains("partialFile"))check(fs::exists(fs::path(wide(str(spec,"partialFile"))))&&fileHash(fs::path(wide(str(spec,"partialFile"))))==str(spec,"partialHash"),"Original parts survive until post-commit cleanup at "+label);
   if(spec.contains("preservedFile"))check(fileHash(fs::path(wide(str(spec,"preservedFile"))))==str(spec,"preservedHash"),"Completed original file is untouched at "+label);
   Manager recovered(folder/L"state");
   if(!committed&&spec.contains("partialFile"))check(fileHash(fs::path(wide(str(spec,"partialFile"))))==str(spec,"partialHash"),"Restart keeps uncommitted replacement parts at "+label);
   auto accepted=commitPreparedCapture(recovered,token);auto id=str(accepted,"id");
   check(str(accepted,"status")=="accepted"&&recovered.jobs.size()==(size_t)num(spec,"afterCount"),"Restart admits exactly the intended result after "+label);
   auto again=commitPreparedCapture(recovered,token);check(again==accepted&&recovered.jobs.size()==(size_t)num(spec,"afterCount"),"Repeated recovery reuses the same receipt after "+label);
   if(scenario=="existing"||scenario=="replace-partial"||scenario=="refresh")check(id==str(spec,"originalId"),"Recovery preserves the existing download identity at "+label);
   if(scenario=="refresh"){
    JobPtr target;for(auto job:recovered.jobs)if(job->id()==id)target=job;
    check(str(target->data,"Url")=="https://example.invalid/fixture.zip"&&str(recovered.addressRefreshCandidate(target),"url")=="https://example.invalid/fixture.zip?renewed=1","Prepared link refresh remains a reviewed offer after "+label);
   }
   if(spec.contains("partialFile"))check(!fs::exists(fs::path(wide(str(spec,"partialFile")))),"Committed replacement retires its old parts after "+label);
   if(spec.contains("preservedFile"))check(fileHash(fs::path(wide(str(spec,"preservedFile"))))==str(spec,"preservedHash"),"Admission does not publish over an existing completed file at "+label);
  }
 }
 for(const auto& scenario:scenarios){
  auto folder=root/L"capture-rollback"/wide(scenario);auto spec=captureAtomicFixture(folder,scenario);Manager m(folder/L"state");
  if(!m.jobs.empty()){m.jobs[0]->workers.push_back({1,0,4095,64,64,"Paused"});m.jobs[0]->speed=123;m.jobs[0]->speedMeter.update(64,1,true);m.jobs[0]->sessionLimit=777;}
  auto before=m.snapshot();auto file=m.root/L"state.json";auto original=readText(file);
  Handle reader(CreateFileW(file.c_str(),GENERIC_READ,FILE_SHARE_READ,nullptr,OPEN_EXISTING,0,nullptr));check((bool)reader,"Rollback fixture locks the catalog for "+scenario);
  rejects([&]{commitPreparedCapture(m,str(spec,"token"));},"Failed final publication reports an error for "+scenario);
  check(m.snapshot()==before&&readText(file)==original,"Failed publication restores catalog and receipt for "+scenario);
  if(!m.jobs.empty())check(m.jobs[0]->workers.size()==1&&m.jobs[0]->speed==123&&m.jobs[0]->sessionLimit==777,"Failed publication restores transient duplicate state for "+scenario);
  if(spec.contains("partialFile"))check(fileHash(fs::path(wide(str(spec,"partialFile"))))==str(spec,"partialHash"),"Failed publication preserves original partial bytes");
  CloseHandle(reader.h);reader.h=INVALID_HANDLE_VALUE;
  check(str(commitPreparedCapture(m,str(spec,"token")),"status")=="accepted"&&m.jobs.size()==(size_t)num(spec,"afterCount"),"Storage recovery admits one intended result for "+scenario);
 }
 {
  auto folder=root/L"capture-refresh-late";auto spec=captureAtomicFixture(folder,"numbered");Manager m(folder/L"state");auto original=m.jobs.front();m.beginAddressRefresh(original);
  auto accepted=commitPreparedCapture(m,str(spec,"token"));
  check(str(accepted,"id")!=original->id()&&m.jobs.size()==2&&m.addressRefreshCandidate(original).is_null(),"A later refresh session cannot hijack an already prepared ordinary download");
  check(m.browserCaptureContext({{"url","https://example.invalid/fixture.zip?later=1"},{"filename","fixture.zip"}}).value("refreshId","")==original->id(),"Committing an unrelated capture preserves the user's current refresh selection");
 }
 for(const auto& change:{"address","filename","removed"}){
  auto folder=root/L"capture-refresh-changed"/wide(change);auto spec=captureAtomicFixture(folder,"refresh");Manager m(folder/L"state");
  if(std::string(change)=="removed")m.remove(m.jobs.front());
  else{m.jobs.front()->data[std::string(change)=="address"?"Url":"FileName"]=std::string(change)=="address"?"https://example.invalid/changed.zip":"changed.zip";m.save();}
  auto before=m.snapshot();check(str(commitPreparedCapture(m,str(spec,"token")),"status")=="review",std::string("A changed refresh target requires review: ")+change);
  check(m.snapshot()["Downloads"]==before["Downloads"]&&str(captureTransactionStatus(m,str(spec,"token")),"status")=="review",std::string("A changed refresh target preserves downloads and retains the protected request for review: ")+change);
 }
 for(const auto& phase:{"deferred-save","before-commit","temporary-flushed"}){
  auto folder=root/L"capture-exception"/wide(phase);auto spec=captureAtomicFixture(folder,"replace-partial");Manager m(folder/L"state");auto before=m.snapshot();
  rejects([&]{m.commitBrowserCapture(str(spec,"token"),[&](const char* point){if(std::string(point)==phase)throw std::runtime_error("Isolated admission failure");});},std::string("Admission exception is returned at ")+phase);
  check(m.snapshot()==before&&fileHash(fs::path(wide(str(spec,"partialFile"))))==str(spec,"partialHash"),std::string("Admission exception restores the catalog and partial bytes at ")+phase);
  check(str(commitPreparedCapture(m,str(spec,"token")),"status")=="accepted"&&m.jobs.size()==1,std::string("Admission exception can be retried once at ")+phase);
 }
 {
  auto folder=root/L"capture-postcommit-error";auto spec=captureAtomicFixture(folder,"new");Manager m(folder/L"state");
  rejects([&]{m.commitBrowserCapture(str(spec,"token"),[](const char* point){if(std::string(point)=="committed")throw std::runtime_error("Reply lost after commit");});},"Failure after commit is reported without rolling the download back");
  check(str(captureTransactionStatus(m,str(spec,"token")),"status")=="accepted"&&m.jobs.size()==1,"Durable accepted receipt survives a post-commit exception");
  Manager restarted(folder/L"state");check(str(commitPreparedCapture(restarted,str(spec,"token")),"status")=="accepted"&&restarted.jobs.size()==1,"Lost post-commit reply remains idempotent after restart");
 }
}
