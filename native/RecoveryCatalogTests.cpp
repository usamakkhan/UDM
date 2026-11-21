#include "Core.hpp"
#include "PublicationCatalogProof.hpp"
#include <future>
#include <iostream>
namespace udm {struct CatalogCheckpointTestAccess {
 static void observe(Manager& m,std::function<void(const char*)> callback){Lock lock(m.mutex);m.catalogCheckpoint=std::move(callback);}
 static void batch(Manager& m,bool value){Lock lock(m.mutex);m.catalogTransaction=value;}
 static bool closing(Manager& m){Lock lock(m.mutex);return m.stopping;}
};}
using namespace udm;
struct RecoveryObservation {Manager& manager;~RecoveryObservation(){CatalogCheckpointTestAccess::observe(manager,{});}};
struct RecoveryGate {
 std::promise<void> entered,released;std::shared_future<void> ready=entered.get_future().share(),wait=released.get_future().share();std::atomic_bool held=false,done=false,expired=false;
 void hold(){if(!held.exchange(true)){entered.set_value();if(wait.wait_for(std::chrono::seconds(10))!=std::future_status::ready){expired=true;throw std::runtime_error("Recovery fixture deadline");}}}
 void release(){if(!done.exchange(true))released.set_value();}~RecoveryGate(){release();}
};
static void settings(Manager& m,const fs::path& files){fs::create_directories(files);auto p=m.state["Settings"];p["DownloadFolder"]=utf8(files.wstring());p["CategoryFolders"]=false;m.setSettings(p);}
static const Json* savedRow(const Json& catalog,const std::string& id){std::vector<const Json*> pending;for(const auto& row:catalog.at("Downloads"))pending.push_back(&row);const Json* found=nullptr;while(!pending.empty()){const auto& row=*pending.back();pending.pop_back();if(!row.is_object())continue;if(str(row,"Id")==id){if(found)throw std::runtime_error("Duplicate persisted recovery row");found=&row;}for(const char* key:{"Video","Audio"})if(row.contains(key)&&row.at(key).is_object())pending.push_back(&row.at(key));}return found;}
static bool sameRecoveryCore(const Json& current,const Json& saved){for(const char* key:{"Id","Folder","FileName","Status","Size","Received","Sha256","Finished","PublicationCommitId","ReplacementOf","ReplacementHash","PreviousPath"})if(current.contains(key)!=saved.contains(key)||(current.contains(key)&&current.at(key)!=saved.at(key)))return false;return true;}
int wmain(int argc,wchar_t** argv){
 if(argc!=2)return 2;const auto root=fs::absolute(argv[1]);if(fs::exists(root))return 2;fs::create_directories(root);Json checks=Json::array(),cases=Json::array();std::string error,activeCase;bool passed=false;
 auto check=[&](bool value,const char* label){checks.push_back({{"name",label},{"passed",value}});if(!value)throw std::runtime_error(label);};
 try{
  for(const std::string entry:{"direct","resume","file-info","schedule","project"})for(const bool journalBacked:{false,true}){
   if(entry=="schedule"&&!journalBacked)continue;
   activeCase=entry+(journalBacked?"-journal":"-legacy");const auto folder=root/wide(activeCase);Manager m(folder/L"catalog");settings(m,folder/L"files");auto job=m.add("https://example.invalid/recovery.bin");atomicText(job->target(),"verified bytes",false);const auto hash=fileHash(job->target());
   Json project={{"Id",guid()},{"Name","Recovery project"},{"StartUrl","https://example.invalid/index.html"},{"Depth",1},{"MaxPages",5},{"Links",Json::array({Json{{"Url",str(job->data,"Url")},{"Selected",true}}})}};if(entry=="project")job->data["ProjectId"]=str(project,"Id");
   job->data["Status"]="Failed";job->data["Error"]="Could not replace the saved state (Windows error 1175). Retry after closing programs that hold the state file.";job->data["Size"]=14;job->data["Received"]=14;job->data["Sha256"]=hash;job->data["Finished"]=date();job->data["QueueMember"]=false;m.save();
   if(journalBacked){const auto stage=folder/L"stage.part";fs::rename(job->target(),stage);job->data["Status"]="Paused";job->data["Error"]="";m.save();CatalogCheckpointTestAccess::observe(m,[](const char* point){if(std::string_view(point)=="publication-file-moved")throw std::runtime_error("Owned interruption");});try{m.publishFile(job,stage,hash);}catch(...){}CatalogCheckpointTestAccess::observe(m,{});}
   const auto originalDraft=job->data;
   auto gate=std::make_shared<RecoveryGate>();CatalogCheckpointTestAccess::observe(m,[&](const char* point){if(std::string_view(point)=="temporary-flushed")gate->hold();});
   RecoveryObservation clear{m};
   auto work=std::async(std::launch::async,[&]{if(entry=="resume")m.resume(job);else if(entry=="file-info")m.finishStoppedFileInfo(job,Json::object(),FileInfoFolderChoice{},std::nullopt,false);else if(entry=="schedule")m.scheduleExistingDownload(job,originalDraft,DownloadQueueChoice{},Json::object(),FileInfoFolderChoice{});else if(entry=="project")m.addProject(project,"Main queue",true,true,true);else m.recoverReplacements(job);});struct Release {std::shared_ptr<RecoveryGate> gate;~Release(){gate->release();}} release{gate};
   check(gate->ready.wait_for(std::chrono::seconds(5))==std::future_status::ready,"The public recovery entry reaches its actual held catalog flush");
   {std::unique_lock<std::recursive_mutex> ui(m.mutex,std::try_to_lock);check(ui.owns_lock(),"Direct, Resume and File Info recovery leave the model available during catalog IO");check(m.isActive(job)&&m.downloadDestinationReserved(job->target()),"Recovery owns its job tree and destination until catalog acceptance");auto view=m.snapshot();check(view["Downloads"].size()==1,"An independent UI snapshot remains available during the held recovery write");job->data["Description"]="independent metadata";m.state["Settings"]["RecoveryUiMarker"]=activeCase;}
   m.resume(job);bool refused=false;try{m.remove(job);}catch(...){refused=true;}check(refused,"Removal cannot take over a recovering job");refused=false;try{m.configure(job,{{"FileName","changed.bin"}});}catch(...){refused=true;}check(refused,"Configuration cannot redirect a recovering file");
   gate->release();std::string workflowError;try{work.get();}catch(const std::exception& e){workflowError=e.what();}CatalogCheckpointTestAccess::observe(m,{});
   check((entry=="file-info"||entry=="schedule")?workflowError=="The saved file was recovered at its previous destination. Use Move in File Properties to change its location.":workflowError.empty(),"Public recovery preserves its Complete versus recovered-File-Info outcome");
   check(!gate->expired&&str(job->data,"Status")=="Complete"&&fileHash(job->target())==hash&&str(job->data,"Description")=="independent metadata"&&!m.isActive(job),"Explicit release accepts exact bytes, preserves independent metadata and releases ownership");m.save();cases.push_back({{"entry",entry},{"journalBacked",journalBacked},{"modelAvailable",true},{"exactBytes",true}});m.stop();
  }
  for(const std::string outcome:{"ordinary-complete","ordinary-paused","replacement-complete","replacement-paused","legacy-complete","private-complete","private-paused"})for(const std::string fault:{"none","catalog-failure","noncontaining","later-containing","later-retry","lost-ack","deferred","receipt-removed","removed-field"}){
   if(fault=="receipt-removed"&&outcome!="private-complete")continue;if(fault=="removed-field"&&outcome!="replacement-complete")continue;
   activeCase=outcome+"-"+fault;const auto folder=root/wide(activeCase);Manager m(folder/L"catalog");const auto files=folder/L"files";settings(m,files);const bool paired=outcome.find("replacement")==0,privateRow=outcome.find("private")==0,legacy=outcome=="legacy-complete",completed=outcome.find("complete")!=std::string::npos;
   JobPtr original;std::string oldHash;if(paired){original=m.add("https://example.invalid/original","","output.bin","Main queue",true);atomicText(original->target(),"old bytes",false);oldHash=fileHash(original->target());original->data["Status"]="Complete";original->data["Size"]=9;original->data["Received"]=9;original->data["Sha256"]=oldHash;original->data["Finished"]=date();}
   auto job=m.add("https://example.invalid/recovered","",paired?"next.bin":"output.bin","Main queue",true);if(paired){job->data["FileName"]=str(original->data,"FileName");job->data["ReplacementOf"]=original->id();job->data["ReplacementHash"]=oldHash;job->data["PreviousPath"]=utf8((files/L"previous.bin").wstring());}if(privateRow)m.jobs.erase(std::find(m.jobs.begin(),m.jobs.end(),job));m.save();
   const auto stage=files/((completed||paired)?L".udm-"+wide(job->id())+L".assembling":L"unrecognized.stage"),target=job->target(),journal=m.root/(paired?L"replacements":L"publications")/(wide(job->id())+L".json");atomicText(stage,"verified bytes",false);const auto hash=fileHash(stage);
   if(legacy){fs::rename(stage,target);job->data["Status"]="Failed";job->data["Error"]="Could not replace the saved state (Windows error 1175). Retry after closing programs that hold the state file.";job->data["Size"]=14;job->data["Received"]=14;job->data["Sha256"]=hash;job->data["Finished"]=date();m.save();}
   else{CatalogCheckpointTestAccess::observe(m,[&](const char* point){if(std::string_view(point)==(paired?(completed?"replacement-file-moved":"replacement-intent-written"):"publication-intent-written"))throw std::runtime_error("Owned interrupted intent");});try{m.publishFile(job,stage,hash);}catch(...){}CatalogCheckpointTestAccess::observe(m,{});check(fs::exists(journal),"The checkpoint scenario begins with a real preserved publication intent");}
   const auto before=job->data;auto gate=std::make_shared<RecoveryGate>();std::atomic<DWORD> workerId=0;std::atomic_bool first=false,second=false;std::promise<void> otherCaptured;auto otherReady=otherCaptured.get_future();bool rejected=false;
   CatalogCheckpointTestAccess::observe(m,[&](const char* point){
    const std::string_view phase(point);if(GetCurrentThreadId()!=workerId.load()){if(phase=="snapshot-prepared"&&!second.exchange(true))otherCaptured.set_value();return;}
    if(phase=="recovery-catalog-prepared"&&fault=="deferred"){CatalogCheckpointTestAccess::batch(m,true);return;}
    if(phase=="snapshot-prepared"&&!first.exchange(true)){
     {Lock lock(m.mutex);job->data["Description"]="independent checkpoint metadata";if(fault=="noncontaining"){if(privateRow)m.jobs.push_back(job);else job->data["Status"]="Failed";}if(fault=="removed-field")job->data["ReplacementOf"]=original->id();if(fault=="receipt-removed")m.state.erase("PublicationReceipts");}
     if(fault=="noncontaining"||fault=="removed-field"||fault=="receipt-removed"){try{m.save();}catch(...){rejected=true;throw;}}
     if(fault=="later-containing")gate->hold();
    }
    if(phase=="temporary-flushed"){if(fault=="catalog-failure")throw std::runtime_error("Owned recovery flush failure");if(fault=="later-retry"){gate->hold();throw std::runtime_error("Owned failed earlier snapshot");}}
    if(phase=="publication-catalog-written"&&fault=="lost-ack")throw std::runtime_error("Owned acknowledgement loss");
   });
   std::future<void> later;RecoveryObservation clear{m};
   auto work=std::async(std::launch::async,[&]{workerId=GetCurrentThreadId();m.recoverReplacements(job);});struct Release {std::shared_ptr<RecoveryGate> gate;~Release(){gate->release();}} release{gate};
   if(fault=="later-containing"||fault=="later-retry"){
    check(gate->ready.wait_for(std::chrono::seconds(5))==std::future_status::ready,"Recovery exposes the actual snapshot or flush boundary for a later writer");
    {Lock lock(m.mutex);job->data["Description"]="independent checkpoint metadata";m.state["Settings"]["ContainingRecoveryMarker"]=activeCase;}
    later=std::async(std::launch::async,[&]{m.save();});check(otherReady.wait_for(std::chrono::seconds(5))==std::future_status::ready,"The later writer captures an actual containing snapshot");if(fault=="later-containing")later.get();gate->release();
   }
   std::string failure;try{work.get();}catch(const std::exception& e){failure=e.what();}if(later.valid())later.get();CatalogCheckpointTestAccess::observe(m,{});CatalogCheckpointTestAccess::batch(m,false);
   const bool failed=fault=="catalog-failure"||fault=="noncontaining"||fault=="lost-ack"||fault=="deferred"||fault=="receipt-removed"||fault=="removed-field";
   check(failed?!failure.empty():failure.empty(),"Only a snapshot-bound durable outcome acknowledges recovery");
   if(fault=="noncontaining"||fault=="receipt-removed"||fault=="removed-field")check(rejected,"A later noncontaining row, removed-field regression or missing private receipt is rejected before writing");
   if(privateRow&&fault=="noncontaining"){Lock lock(m.mutex);m.jobs.erase(std::find(m.jobs.begin(),m.jobs.end(),job));}
   if(failed){check(legacy||fs::exists(journal),"Unproved recovery preserves its original journal");check(!m.isActive(job),"Every failed checkpoint releases its runtime job/path reservation");m.recoverReplacements(job);}
   check(!gate->expired&&str(job->data,"Status")== (completed?"Complete":"Paused")&&(legacy||!fs::exists(journal)),"Accepted recovery or explicit retry reaches the correct complete or paused state");
   check(completed?fs::exists(target)&&fileHash(target)==hash:fs::exists(stage)&&fileHash(stage)==hash,"Every outcome preserves the exact downloaded bytes");
   if(paired)check(fileHash(completed?files/L"previous.bin":target)==oldHash,"Paired recovery preserves the original file in the correct location");
   if(fault!="deferred")check(str(job->data,"Description")=="independent checkpoint metadata","Acceptance and conditional rollback preserve independent current metadata");
   check(privateRow?m.jobs.empty():std::find(m.jobs.begin(),m.jobs.end(),job)!=m.jobs.end(),"Private recovery never inserts a visible catalog row");
   const auto disk=Json::parse(readText(m.root/L"state.json"));const auto persisted=savedRow(disk,job->id());check(privateRow?!persisted:persisted&&sameRecoveryCore(job->data,*persisted),"Before another save, the accepted catalog contains the exact recovery core or private-row absence");
   if(privateRow&&completed){const auto& receipts=disk.at("PublicationReceipts");bool recorded=false;for(const auto& receipt:receipts)if(str(receipt,"Id")==job->id()&&str(receipt,"Status")=="Complete"&&num(receipt,"Size")==14&&str(receipt,"Sha256")==hash)recorded=true;check(recorded,"A private completed outcome has its exact durable receipt before cleanup checkpoints");}
   if(paired){const auto persistedOriginal=savedRow(disk,original->id());check(persistedOriginal&&sameRecoveryCore(original->data,*persistedOriginal),"Both replacement roles are acknowledged in the same actual persisted snapshot");}
   m.save();cases.push_back({{"outcome",outcome},{"fault",fault},{"expectedFailure",failed},{"rejectedNoncontainingWriter",rejected},{"finalStatus",str(job->data,"Status")}});m.stop();
  }
  {
   activeCase="shutdown-checkpoint";Manager m(root/L"shutdown-checkpoint/catalog");const auto files=root/L"shutdown-checkpoint/files";settings(m,files);auto job=m.add("https://example.invalid/shutdown","","output.bin","Main queue",true);const auto stage=files/(L".udm-"+wide(job->id())+L".assembling");atomicText(stage,"verified bytes",false);const auto hash=fileHash(stage),id=job->id();
   CatalogCheckpointTestAccess::observe(m,[](const char* point){if(std::string_view(point)=="publication-intent-written")throw std::runtime_error("Owned shutdown intent");});try{m.publishFile(job,stage,hash);}catch(...){}CatalogCheckpointTestAccess::observe(m,{});
   auto gate=std::make_shared<RecoveryGate>();CatalogCheckpointTestAccess::observe(m,[&](const char* point){if(std::string_view(point)=="temporary-flushed")gate->hold();});RecoveryObservation clear{m};std::future<void> stop;auto work=std::async(std::launch::async,[&]{m.recoverReplacements(job);});struct Release {std::shared_ptr<RecoveryGate> gate;~Release(){gate->release();}} release{gate};
   check(gate->ready.wait_for(std::chrono::seconds(5))==std::future_status::ready,"Shutdown scenario reaches the held recovery checkpoint");stop=std::async(std::launch::async,[&]{m.stop();});const auto deadline=GetTickCount64()+5000;while(!CatalogCheckpointTestAccess::closing(m)&&GetTickCount64()<deadline)Sleep(5);
   check(CatalogCheckpointTestAccess::closing(m)&&stop.wait_for(std::chrono::milliseconds(0))!=std::future_status::ready,"Shutdown waits for the reserved recovery transaction to settle");gate->release();std::string failure;try{work.get();}catch(const std::exception& e){failure=e.what();}stop.get();CatalogCheckpointTestAccess::observe(m,{});
   check((failure.empty()||failure=="UDM is closing.")&&fileHash(job->target())==hash&&str(job->data,"Status")=="Complete"&&!fs::exists(m.root/L"publications"/(wide(id)+L".json")),"Shutdown retains the acknowledged completion and exact bytes without leaving its old journal");bool refused=false;try{m.recoverReplacements(job);}catch(...){refused=true;}check(refused,"A stopped Manager cannot acquire a new recovery owner after shutdown finishes");
  }
  {
   activeCase="projection-proof";Manager m(root/L"projection-proof");settings(m,root/L"projection-proof-files");auto job=m.add("https://example.invalid/proof","","proof.bin","Main queue",true);auto before=job->data,after=before;after["Status"]="Paused";
   PublicationCatalogProof::Lease proof;{Lock lock(m.mutex);job->data=after;proof=PublicationCatalogProof::prepareRecovery(m,{{before,after,true}});}check(!proof.resolve(),"A prior checkpoint sequence alone cannot acknowledge a newly prepared recovery");proof.release();
   before=after;before["RecoveryRemovedField"]=true;{Lock lock(m.mutex);job->data=before;}m.save();after=before;after.erase("RecoveryRemovedField");{Lock lock(m.mutex);job->data=after;proof=PublicationCatalogProof::prepareRecovery(m,{{before,after,true}});}
   {Lock lock(m.mutex);job->data["RecoveryRemovedField"]=true;}bool refused=false;try{m.save();}catch(...){refused=true;}check(refused,"Removed fields are part of the recovery snapshot proof");{Lock lock(m.mutex);job->data.erase("RecoveryRemovedField");}m.save();check(proof.resolve(),"An exact later snapshot can prove a recovery with removed fields");proof.release();
   {Lock lock(m.mutex);proof=PublicationCatalogProof::prepareRecovery(m,{{after,after,false}});}refused=false;try{m.save();}catch(...){refused=true;}check(refused&&!proof.resolve(),"A visible row cannot satisfy a private absence binding");proof.release();m.stop();
  }
  passed=true;
 }catch(const std::exception& e){error=e.what();}
 atomicText(root/L"results.json",Json{{"passed",passed},{"error",error},{"case",activeCase},{"checks",checks},{"cases",cases}}.dump(2),false);std::cout<<checks.size()<<" checks; "<<(passed?"passed":error)<<"\n";return passed?0:1;
}
