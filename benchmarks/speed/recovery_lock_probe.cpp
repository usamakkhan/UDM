// Baseline diagnostic: observing a blocked lock is NOT a successful recovery fix.
#include "Core.hpp"
#include <future>
#include <iostream>
namespace udm {struct CatalogCheckpointTestAccess {
 static void observe(Manager& m,std::function<void(const char*)> f){Lock lock(m.mutex);m.catalogCheckpoint=std::move(f);}
};}
using namespace udm;
int wmain(int argc,wchar_t** argv){
 if(argc!=2)return 2;auto root=fs::absolute(argv[1]);if(fs::exists(root))return 2;fs::create_directories(root);
 Json cases=Json::array();bool valid=true;std::string error;
 try{for(const std::string mode:{"direct","resume","file-info-start","direct:journal","resume:journal","file-info-start:journal"}){
  const auto entry=mode.substr(0,mode.find(':'));const bool journal=mode.find(':')!=std::string::npos;
  const auto folder=root/wide(entry+(journal?"-journal":"-legacy"));Manager manager(folder/L"catalog");
  auto prefs=manager.state["Settings"];prefs["DownloadFolder"]=utf8((folder/L"files").wstring());prefs["CategoryFolders"]=false;manager.setSettings(prefs);
  auto job=manager.add("https://example.invalid/recovery.bin");fs::create_directories(job->target().parent_path());atomicText(job->target(),"verified bytes",false);
  job->data["Status"]="Failed";job->data["Error"]="Could not replace the saved state (Windows error 1175). Retry after closing programs that hold the state file.";
  job->data["Size"]=14;job->data["Received"]=14;job->data["Sha256"]=fileHash(job->target());job->data["Finished"]=date();job->data["QueueMember"]=false;manager.save();
  if(journal){
   auto stage=folder/L"staging.part";fs::rename(job->target(),stage);job->data["Status"]="Paused";job->data["Error"]="";manager.save();
   CatalogCheckpointTestAccess::observe(manager,[](const char* point){if(std::string_view(point)=="publication-file-moved")throw std::runtime_error("Owned interruption after rename");});
   bool interrupted=false;try{manager.publishFile(job,stage,fileHash(stage));}catch(const std::exception&){interrupted=true;}
   CatalogCheckpointTestAccess::observe(manager,{});if(!interrupted||!fs::exists(manager.root/L"publications"/(wide(job->id())+L".json")))throw std::runtime_error("Interrupted publication journal was not preserved");
  }
  std::promise<void> reached,released;auto ready=reached.get_future();auto releaseWait=released.get_future().share();std::atomic_bool held=false;
  CatalogCheckpointTestAccess::observe(manager,[&](const char* stage){if(std::string_view(stage)=="temporary-flushed"&&!held.exchange(true)){reached.set_value();if(releaseWait.wait_for(std::chrono::seconds(10))!=std::future_status::ready)throw std::runtime_error("Recovery diagnostic hold expired");}});
  std::future<void> work;struct Release {std::promise<void>& p;bool done=false;void now(){if(!done){done=true;p.set_value();}}~Release(){now();}} unblock{released};
  work=std::async(std::launch::async,[&]{if(entry=="resume")manager.resume(job);else if(entry=="file-info-start")manager.finishStoppedFileInfo(job,Json::object(),FileInfoFolderChoice{},std::nullopt,false);else manager.recoverReplacements(job);});
  if(ready.wait_for(std::chrono::seconds(5))!=std::future_status::ready){unblock.now();work.get();throw std::runtime_error("Recovery did not reach its actual flushed catalog boundary");}
  std::unique_lock<std::recursive_mutex> ui(manager.mutex,std::try_to_lock);const bool available=ui.owns_lock();if(available)ui.unlock();unblock.now();std::string workflowError;try{work.get();}catch(const std::exception& e){workflowError=e.what();}
  if(entry=="file-info-start"){if(workflowError!="The saved file was recovered at its previous destination. Use Move in File Properties to change its location.")throw std::runtime_error("Unexpected recovered File Info outcome");}else if(!workflowError.empty())throw std::runtime_error(workflowError);
  CatalogCheckpointTestAccess::observe(manager,{});const bool recovered=str(job->data,"Status")=="Complete"&&readText(job->target())=="verified bytes";
  cases.push_back({{"entry",entry},{"journalBacked",journal},{"workflowError",workflowError},{"actualCatalogFlushReached",true},{"modelLockAvailableDuringHeldBoundary",available},{"recoveredExactBytes",recovered}});valid=valid&&recovered;manager.stop();
 }}catch(const std::exception& e){valid=false;error=e.what();}
 atomicText(root/L"results.json",Json{{"diagnosticValid",valid},{"error",error},{"cases",cases},{"recoveryResponsivenessFixed",false}}.dump(2),false);
 std::cout<<cases.size()<<" recovery observations; diagnostic "<<(valid?"valid":error)<<"\n";return valid?0:1;
}
