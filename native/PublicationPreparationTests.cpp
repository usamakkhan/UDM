#include "Core.hpp"
#include "PublicationPreparation.hpp"
#include <future>
#include <iostream>
namespace udm {struct CatalogCheckpointTestAccess {static void observe(Manager& m,std::function<void(const char*)> callback){Lock lock(m.mutex);m.catalogCheckpoint=std::move(callback);}};}
using namespace udm;
struct PreparationGate {
 std::string point;std::promise<void> entered,released;std::shared_future<void> ready=entered.get_future().share(),wait=released.get_future().share();std::atomic_bool unlocked=false,held=false;
 explicit PreparationGate(std::string value):point(std::move(value)){}
 void observe(const char* stage){if(stage==point&&!held.exchange(true)){entered.set_value();if(wait.wait_for(std::chrono::seconds(10))!=std::future_status::ready)throw std::runtime_error("Publication preparation fixture deadline");}}
 void release(){if(!unlocked.exchange(true))released.set_value();}~PreparationGate(){release();}
};
int wmain(int argc,wchar_t** argv){
 if(argc!=2)return 2;const auto root=fs::absolute(argv[1]);if(fs::exists(root))return 2;fs::create_directories(root);Json checks=Json::array();std::string failure,activeCase;bool passed=false;
 auto check=[&](bool value,const char* label){checks.push_back({{"name",label},{"passed",value}});if(!value)throw std::runtime_error(label);};
 try{
  for(const std::string mode:{"begin","ready","unknown-hash","zero","gzip","child","cancel","changed-folder","changed-root","changed-source","project","missing-project","busy-source","missing-source","directory-source"}){
   activeCase=mode;const auto folder=root/wide(mode);Manager manager(folder/L"state");auto files=folder/L"files";const bool project=mode=="project"||mode=="changed-root"||mode=="missing-project";if(project)files/=L"nested";
   if(mode!="missing-project")fs::create_directories(files);else fs::create_directories(folder/L"files");
   auto prefs=manager.state["Settings"];prefs["DownloadFolder"]=utf8(files.wstring());prefs["CategoryFolders"]=false;manager.setSettings(prefs);
   auto job=manager.add("https://example.invalid/owned.bin","",mode=="ready"?utf8(L"output-\u2603.bin"):"output.bin","Main queue",true);
   if(mode=="child"){auto data=job->data;data["Id"]=guid();data["FileName"]="child.bin";job=std::make_shared<Job>(data);}
   if(mode=="missing-project")fs::remove(files);
   if(project)job->data["ProjectDestinationRoot"]=utf8((folder/L"files").wstring());
   if(mode=="gzip"){job->data["HttpContentEncoding"]="gzip";job->data["Size"]=23;job->data["Received"]=21;}
   const auto before=job->data;auto staging=folder/L"staged-\u2603.part";if(mode=="directory-source")fs::create_directories(staging);else if(mode!="missing-source")atomicText(staging,mode=="zero"?"":"verified bytes",false);
   const auto hash=mode=="directory-source"||mode=="missing-source"?std::string("unavailable"):fileHash(staging);const auto journal=manager.root/L"publications"/(wide(job->id())+L".json");auto cancel=std::make_shared<Cancel>();
   std::unique_ptr<Handle> writer;if(mode=="busy-source"){writer=std::make_unique<Handle>(CreateFileW(staging.c_str(),GENERIC_WRITE,FILE_SHARE_READ|FILE_SHARE_WRITE|FILE_SHARE_DELETE,nullptr,OPEN_EXISTING,0,nullptr));check(bool(*writer),"Owned writer opens before publication preparation to test source sharing rejection");}
   const bool hold=mode=="begin"||mode=="ready"||mode=="cancel"||mode=="changed-folder"||mode=="changed-root"||mode=="changed-source"||mode=="project";
   auto gate=std::make_shared<PreparationGate>(mode=="begin"?"publication-preflight-begin":"publication-preflight-ready");if(hold)CatalogCheckpointTestAccess::observe(manager,[gate](const char* stage){gate->observe(stage);});
   std::future<void> work;struct Release {std::shared_ptr<PreparationGate> gate;~Release(){gate->release();}} release{gate};std::string error;
   auto publish=[&]{manager.publishFile(job,staging,mode=="unknown-hash"?"":hash,cancel.get());};
   if(hold){
    work=std::async(std::launch::async,publish);check(gate->ready.wait_for(std::chrono::seconds(10))==std::future_status::ready,"Actual publication reaches the owned preparation boundary off the caller thread");
    std::unique_lock<std::recursive_mutex> lease(manager.mutex,std::try_to_lock);check(lease.owns_lock(),"Publication path checks and measured-file preparation do not retain the catalog mutex");manager.state["Settings"]["PublicationPreparationMarker"]="independent owner";manager.save();
    check(job->data==before&&!fs::exists(journal)&&!fs::exists(job->target()),"Held preparation publishes neither a journal nor tentative completion metadata");
    if(mode=="ready"||mode=="project"){Handle contender(CreateFileW(staging.c_str(),GENERIC_WRITE,FILE_SHARE_READ|FILE_SHARE_WRITE|FILE_SHARE_DELETE,nullptr,OPEN_EXISTING,0,nullptr));check(!contender,"A measured staging file rejects writers while prepared input is held");}
    if(mode=="project"){auto moved=folder/L"moved";check(!MoveFileExW(files.c_str(),moved.c_str(),0)&&fs::exists(files),"Protected website subfolders cannot be renamed while their validated directory handles are held");}
    if(mode=="cancel")cancel->stop=true;
    if(mode=="changed-folder"){auto next=folder/L"different";fs::create_directories(next);job->data["Folder"]=utf8(next.wstring());}
    if(mode=="changed-root")job->data["ProjectDestinationRoot"]=utf8(folder.wstring());
    if(mode=="changed-source"){auto retained=folder/L"retained.part";check(MoveFileExW(staging.c_str(),retained.c_str(),MOVEFILE_WRITE_THROUGH)!=FALSE,"Owned source rename exercises allowed delete sharing without modifying the held measured object");atomicText(staging,"replaced bytes",false);}
    lease.unlock();gate->release();try{work.get();}catch(const std::exception& e){error=e.what();}
   }else try{publish();}catch(const std::exception& e){error=e.what();}
   CatalogCheckpointTestAccess::observe(manager,{});writer.reset();
   const bool accepted=mode=="begin"||mode=="ready"||mode=="unknown-hash"||mode=="zero"||mode=="gzip"||mode=="child"||mode=="project";
   if(accepted){
    check(error.empty()&&str(job->data,"Status")=="Complete"&&fs::exists(job->target())&&!fs::exists(staging)&&!fs::exists(journal),"Accepted prepared files publish through the existing rename and durable catalog transaction");
    check(fs::file_size(job->target())==(mode=="zero"?0:14)&&fileHash(job->target())==hash&&num(job->data,"Size")==num(job->data,"Received")&&num(job->data,"Size")==static_cast<i64>(fs::file_size(job->target())),"Prepared size, final metadata and complete output bytes match for ordinary, empty and decoded output");
    if(mode=="gzip")check(num(job->data,"HttpWireSize")==23&&num(job->data,"HttpWireReceived")==21&&num(job->data,"HttpDecodedSize")==14&&job->data["Segments"].size()==1&&num(job->data["Segments"][0],"Done")==14,"Decoded completion uses one measured output length while retaining original wire-byte accounting");
    if(mode=="child")check(manager.jobs.size()==1&&manager.jobs[0]!=job,"Preparing an internal track does not insert its child job into the parent catalog");
    else{manager.stop();Manager reopened(manager.root);check(str(reopened.jobs[0]->data,"Status")=="Complete"&&num(reopened.jobs[0]->data,"Size")==num(job->data,"Size"),"Prepared completion and exact output size survive catalog reopening");}
    if(hold)check(str(manager.state["Settings"],"PublicationPreparationMarker")=="independent owner","Publication preserves an independently committed setting accepted during preparation");
   }else{
    check(!error.empty()&&str(job->data,"Status")!="Complete"&&!fs::exists(journal)&&!fs::exists(job->target()),"Canceled, stale or unavailable preparation cannot publish files, completion metadata or intent");
    if(mode=="cancel")check(error=="Download paused.","Cancellation after preparation retains the original cancellation message");
    if(mode=="changed-folder"||mode=="changed-root")check(error=="The download changed before publication. Its file was not replaced.","Changing only the folder or its saved project-root policy invalidates prepared publication inputs");
    if(mode=="changed-source")check(readText(staging)=="replaced bytes"&&readText(folder/L"retained.part")=="verified bytes"&&error.find("completed file changed")!=std::string::npos,"A replaced staging name is rejected without overwriting either the measured file or replacement bytes");
    else if(mode!="missing-source"&&mode!="directory-source")check(readText(staging)=="verified bytes","Rejected preparation retains the complete staged source for review or retry");
   }
   if(mode=="project"){auto moved=folder/L"moved";check(MoveFileExW(files.c_str(),moved.c_str(),0)!=FALSE,"Protected directory handles release after publication so later folder operations remain available");}
   manager.stop();
  }
  passed=true;
 }catch(const std::exception& e){failure=e.what();}
 atomicText(root/L"results.json",Json{{"passed",passed},{"error",failure},{"case",activeCase},{"checks",checks}}.dump(2),false);std::cout<<checks.size()<<" checks; "<<(passed?"passed":failure)<<"\n";return passed?0:1;
}
