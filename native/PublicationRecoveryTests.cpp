#include "Core.hpp"
#include <iostream>
#include <future>
namespace udm {struct CatalogCheckpointTestAccess {static void observe(Manager& m,std::function<void(const char*)> f){Lock lock(m.mutex);m.catalogCheckpoint=std::move(f);}};}
using namespace udm;
int wmain(int argc,wchar_t** argv){
 if(argc!=2)return 2;auto root=fs::absolute(argv[1]);if(fs::exists(root))return 2;fs::create_directories(root);
 Json checks=Json::array();std::string error;
 auto check=[&](bool ok,const char* name){checks.push_back({{"name",name},{"passed",ok}});if(!ok)throw std::runtime_error(name);};
 try{
  for(int mode=0;mode<10;++mode){auto data=root/(L"legacy-"+std::to_wstring(mode));auto owner=std::make_unique<Manager>(data);auto& m=*owner;auto prefs=m.state["Settings"];prefs["DownloadFolder"]=utf8((data/L"files").wstring());prefs["CategoryFolders"]=false;m.setSettings(prefs);auto job=m.add("https://example.invalid/legacy.bin");fs::create_directories(job->target().parent_path());atomicText(job->target(),"verified bytes",false);auto hash=fileHash(job->target());
   job->data["Status"]="Failed";job->data["Error"]="Could not replace the saved state (Windows error 1175). Retry after closing programs that hold the state file.";job->data["Size"]=14;job->data["Received"]=14;job->data["Sha256"]=hash;job->data["Finished"]=date();job->data["QueueMember"]=false;
   if(mode==1)atomicText(job->target(),"modified bytes",false);if(mode==2)job->data["Size"]=15;if(mode==3)job->data["ExpectedSha256"]=std::string(64,'0');if(mode==4)job->data.erase("Finished");if(mode==5)job->data["Error"]="Server rejected the download.";if(mode==6)job->data.erase("Sha256");if(mode==8)fs::remove(job->target());m.save();auto before=job->data;
   if(mode==7){Handle reader(CreateFileW((data/L"state.json.bak").c_str(),GENERIC_READ,FILE_SHARE_READ|FILE_SHARE_WRITE,nullptr,OPEN_EXISTING,0,nullptr));bool failed=false;try{m.recoverReplacements();}catch(...){failed=true;}check(bool(reader)&&failed&&job->data==before,"Legacy repair rolls back in-memory status when its save fails");}
   if(mode==9){Handle writer(CreateFileW(job->target().c_str(),GENERIC_WRITE,FILE_SHARE_READ|FILE_SHARE_WRITE,nullptr,OPEN_EXISTING,0,nullptr));m.recoverReplacements();check(bool(writer)&&job->data==before,"A file open for writing cannot be reconciled from an unstable hash");}
   owner.reset();{Manager reopened(data);auto restored=reopened.jobs.at(0);if(mode==0||mode==7||mode==9)check(str(restored->data,"Status")=="Complete"&&str(restored->data,"Error").empty()&&str(restored->data,"Sha256")==hash&&restored->data["Finished"]==before["Finished"]&&!yes(restored->data,"QueueMember"),"Verified legacy publication recovers without another transfer or changing finish time");else check(str(restored->data,"Status")=="Failed"&&!restored->data.contains("PublicationRecovered"),"Incomplete or changed legacy evidence cannot claim completion");}
   Manager again(data);check(str(again.jobs.at(0)->data,"Status")==((mode==0||mode==7||mode==9)?"Complete":"Failed"),"Legacy recovery decision survives another catalog reopen");
  }
  for(int mode=0;mode<5;++mode){
  auto data=root/std::to_wstring(mode);auto owner=std::make_unique<Manager>(data);auto& m=*owner;auto prefs=m.state["Settings"];prefs["DownloadFolder"]=utf8((data/L"files").wstring());prefs["CategoryFolders"]=false;m.setSettings(prefs);
  auto job=m.add("https://example.invalid/file.bin");auto staging=data/L"assembled.part";atomicText(staging,"verified bytes",false);auto hash=fileHash(staging);auto journal=data/L"publications"/(wide(job->id())+L".json");
  fs::create_directories(job->target().parent_path());
  if(mode==3){fs::remove(job->target().parent_path());bool rejected=false;try{m.publishFile(job,staging,hash);}catch(const std::exception&){rejected=true;}
   check(rejected&&fs::exists(journal)&&readText(staging)=="verified bytes","Failed rename preserves staging and publication intent");owner.reset();Manager reopened(data);check(str(reopened.jobs.at(0)->data,"Status")=="Paused"&&readText(staging)=="verified bytes"&&!fs::exists(journal),"Unpublished intent recovers paused without publishing or deleting bytes");continue;
  }
  {Handle reader(CreateFileW((data/L"state.json.bak").c_str(),GENERIC_READ,FILE_SHARE_READ|FILE_SHARE_WRITE,nullptr,OPEN_EXISTING,0,nullptr));check(bool(reader),"Backup state is held by another reader");bool rejected=false;try{m.publishFile(job,staging,hash);}catch(const std::exception&){rejected=true;}
   check(rejected&&readText(job->target())=="verified bytes"&&!fs::exists(staging)&&fs::exists(journal),"Save failure retains exact published file and durable intent");
  }
  if(mode==0){owner.reset();{Manager reopened(data);check(str(reopened.jobs.at(0)->data,"Status")=="Complete"&&str(reopened.jobs.at(0)->data,"Sha256")==hash&&!fs::exists(journal),"Restart reconciles published bytes to completed history");}Manager reopenedAgain(data);check(str(reopenedAgain.jobs.at(0)->data,"Status")=="Complete","Recovered completion survives closing and opening another manager");}
  if(mode==1){m.resume(job);check(str(job->data,"Status")=="Complete"&&!yes(job->data,"IndividualStart")&&!fs::exists(journal),"Resume reconciles locally without scheduling another request");owner.reset();Manager reopened(data);check(str(reopened.jobs.at(0)->data,"Status")=="Complete","Resume reconciliation is durable");}
  if(mode==4){m.remove(job);owner.reset();Manager reopened(data);check(reopened.jobs.empty()&&!fs::exists(journal)&&readText(job->target())=="verified bytes","Removing a recovered record preserves its file and leaves no orphan journal");}
  if(mode==2){atomicText(job->target(),"changed by user",false);owner.reset();bool rejected=false;try{Manager reopened(data);}catch(const std::exception&){rejected=true;}
   check(rejected&&readText(job->target())=="changed by user"&&fs::exists(journal),"Changed destination is preserved and never falsely marked complete");}
 }
 for(const bool staged:{false,true}){
  const auto folder=root/(staged?L"recovery-staged-pin":L"recovery-file-pin");Manager manager(folder/L"catalog");auto prefs=manager.state["Settings"];prefs["DownloadFolder"]=utf8((folder/L"files").wstring());prefs["CategoryFolders"]=false;manager.setSettings(prefs);auto job=manager.add("https://example.invalid/pinned.bin");fs::create_directories(job->target().parent_path());auto stage=staged?job->target().parent_path()/(L".udm-"+wide(job->id())+L".assembling"):folder/L"stage.part";atomicText(stage,"verified bytes",false);
  CatalogCheckpointTestAccess::observe(manager,[staged](const char* point){if(std::string_view(point)==(staged?"publication-intent-written":"publication-file-moved"))throw std::runtime_error("Owned interruption");});bool interrupted=false;try{manager.publishFile(job,stage,fileHash(stage));}catch(...){interrupted=true;}CatalogCheckpointTestAccess::observe(manager,{});
  const auto journal=manager.root/L"publications"/(wide(job->id())+L".json");check(interrupted&&fs::exists(journal),"Real interrupted publication retains the recovery journal before pin testing");
  std::promise<void> reached,released;auto ready=reached.get_future();auto wait=released.get_future().share();std::atomic_bool held=false;std::future<void> recovery;
  struct Release {std::promise<void>& value;bool done=false;void now(){if(!done){done=true;value.set_value();}}~Release(){now();}} release{released};
  CatalogCheckpointTestAccess::observe(manager,[&](const char* point){if(std::string_view(point)=="temporary-flushed"&&!held.exchange(true)){reached.set_value();if(wait.wait_for(std::chrono::seconds(10))!=std::future_status::ready)throw std::runtime_error("Recovery pin gate expired");}});
  recovery=std::async(std::launch::async,[&]{manager.recoverReplacements(job);});check(ready.wait_for(std::chrono::seconds(5))==std::future_status::ready,"Recovery reaches its actual catalog flush while retaining verified file ownership");
  {Handle writer(CreateFileW(job->target().c_str(),GENERIC_WRITE,FILE_SHARE_READ|FILE_SHARE_WRITE|FILE_SHARE_DELETE,nullptr,OPEN_EXISTING,0,nullptr));check(!writer,"Verified recovered bytes reject a concurrent writer until catalog acceptance");}
  const auto moved=folder/L"moved.bin";check(!MoveFileExW(job->target().c_str(),moved.c_str(),0),"Verified recovered destination rejects rename until catalog acceptance");
  release.now();recovery.get();CatalogCheckpointTestAccess::observe(manager,{});check(str(job->data,"Status")=="Complete"&&readText(job->target())=="verified bytes"&&!fs::exists(journal),"Pinned recovery accepts exact bytes before retiring its journal");
  {Handle writer(CreateFileW(job->target().c_str(),GENERIC_WRITE,FILE_SHARE_READ|FILE_SHARE_WRITE|FILE_SHARE_DELETE,nullptr,OPEN_EXISTING,0,nullptr));check(bool(writer),"Recovery releases its write exclusion after accepted catalog persistence");}
  check(MoveFileExW(job->target().c_str(),moved.c_str(),0)!=FALSE,"Recovery releases its rename exclusion after accepted catalog persistence");manager.stop();
 }
 for(const bool restoreOriginal:{false,true}){
  const auto folder=root/(restoreOriginal?L"replacement-restore-pin":L"replacement-pair-pin");Manager manager(folder/L"catalog");auto prefs=manager.state["Settings"];prefs["DownloadFolder"]=utf8((folder/L"files").wstring());prefs["CategoryFolders"]=false;manager.setSettings(prefs);fs::create_directories(folder/L"files");
  auto original=manager.add("https://example.invalid/old","","old.bin","Main queue",true);atomicText(original->target(),"old bytes",false);original->data["Status"]="Complete";original->data["Sha256"]=fileHash(original->target());original->data["Size"]=9;original->data["Received"]=9;original->data["Finished"]=date();
  auto job=manager.add("https://example.invalid/new","","new.bin","Main queue",true);job->data["FileName"]=str(original->data,"FileName");job->data["ReplacementOf"]=original->id();job->data["ReplacementHash"]=str(original->data,"Sha256");const auto previous=folder/L"files/old.previous.bin";job->data["PreviousPath"]=utf8(previous.wstring());manager.save();auto stage=folder/L"files/stage.part";atomicText(stage,"new bytes",false);
  CatalogCheckpointTestAccess::observe(manager,[restoreOriginal](const char* point){if(std::string_view(point)==(restoreOriginal?"replacement-intent-written":"replacement-file-moved"))throw std::runtime_error("Owned replacement interruption");});bool interrupted=false;try{manager.publishFile(job,stage,fileHash(stage));}catch(...){interrupted=true;}CatalogCheckpointTestAccess::observe(manager,{});const auto journal=manager.root/L"replacements"/(wide(job->id())+L".json");check(interrupted&&fs::exists(journal),"Real replacement interruption preserves its paired recovery journal");
  if(restoreOriginal){check(MoveFileExW(job->target().c_str(),previous.c_str(),0)!=FALSE,"Restore fixture preserves original bytes at the journal previous path");}
  std::promise<void> reached,released;auto ready=reached.get_future();auto wait=released.get_future().share();std::atomic_bool held=false;std::future<void> recovery;
  struct Release {std::promise<void>& value;bool done=false;void now(){if(!done){done=true;value.set_value();}}~Release(){now();}} release{released};
  CatalogCheckpointTestAccess::observe(manager,[&](const char* point){if(std::string_view(point)=="temporary-flushed"&&!held.exchange(true)){reached.set_value();if(wait.wait_for(std::chrono::seconds(10))!=std::future_status::ready)throw std::runtime_error("Replacement pin gate expired");}});
  recovery=std::async(std::launch::async,[&]{manager.recoverReplacements(job);});check(ready.wait_for(std::chrono::seconds(5))==std::future_status::ready,"Replacement recovery reaches the actual catalog flush with its verified bytes retained");
  const auto paths=restoreOriginal?std::vector<fs::path>{job->target()}:std::vector<fs::path>{job->target(),previous};
  for(const auto& path:paths){
   {Handle writer(CreateFileW(path.c_str(),GENERIC_WRITE,FILE_SHARE_READ|FILE_SHARE_WRITE|FILE_SHARE_DELETE,nullptr,OPEN_EXISTING,0,nullptr));check(!writer,"Verified replacement recovery rejects concurrent writes until catalog acceptance");}
   check(!MoveFileExW(path.c_str(),(path.wstring()+L".moved").c_str(),0),"Verified replacement recovery rejects concurrent rename until catalog acceptance");
  }
  release.now();recovery.get();CatalogCheckpointTestAccess::observe(manager,{});
  check(!fs::exists(journal)&&readText(job->target())==(restoreOriginal?"old bytes":"new bytes")&&str(job->data,"Status")== (restoreOriginal?"Paused":"Complete"),"Replacement recovery accepts exact output before retiring its journal");
  check(restoreOriginal?original->target()==job->target():readText(original->target())=="old bytes","Replacement recovery retains the original history at its verified location");
  for(const auto& path:paths){
   {Handle writer(CreateFileW(path.c_str(),GENERIC_WRITE,FILE_SHARE_READ|FILE_SHARE_WRITE|FILE_SHARE_DELETE,nullptr,OPEN_EXISTING,0,nullptr));check(bool(writer),"Replacement recovery releases write exclusion after accepted persistence");}
   check(MoveFileExW(path.c_str(),(path.wstring()+L".moved").c_str(),0)!=FALSE,"Replacement recovery releases rename exclusion after accepted persistence");
  }
  manager.stop();
 }
 }catch(const std::exception& e){error=e.what();}
 auto result=Json{{"passed",error.empty()},{"error",error},{"checks",checks}};atomicText(root/L"results.json",result.dump(2),false);std::cout<<result.dump(2)<<std::endl;return error.empty()?0:1;
}