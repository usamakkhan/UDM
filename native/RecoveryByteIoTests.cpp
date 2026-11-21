#include "Core.hpp"
#include <future>
#include <iostream>
namespace udm {struct CatalogCheckpointTestAccess {
 static void observe(Manager& m,std::function<void(const char*)> f){Lock lock(m.mutex);m.catalogCheckpoint=std::move(f);}
 static void batch(Manager& m,bool value){Lock lock(m.mutex);m.catalogTransaction=value;}
 static bool closing(Manager& m){Lock lock(m.mutex);return m.stopping;}
};}
using namespace udm;
struct ByteIoGate {
 std::promise<void> entered,released;std::shared_future<void> ready=entered.get_future().share(),wait=released.get_future().share();std::atomic_bool held=false,done=false,expired=false;std::atomic_uint reads=0,hashes=0,flushes=0;
 void observe(const char* point){const std::string_view phase(point);if(phase=="recovery-hash-complete")++hashes;if(phase=="recovery-body-flush-end")++flushes;if(phase=="recovery-hash-read"){++reads;if(!held.exchange(true)){entered.set_value();if(wait.wait_for(std::chrono::seconds(10))!=std::future_status::ready){expired=true;throw std::runtime_error("Owned byte IO gate expired");}}}}
 void release(){if(!done.exchange(true))released.set_value();}~ByteIoGate(){release();}
};
struct ByteIoObservation {Manager& m;~ByteIoObservation(){CatalogCheckpointTestAccess::observe(m,{});}};
int wmain(int argc,wchar_t** argv){
 if(argc!=2)return 2;const auto root=fs::absolute(argv[1]);if(fs::exists(root))return 2;fs::create_directories(root);Json checks=Json::array(),cases=Json::array();std::string error,activeCase,workflowError;
 auto check=[&](bool good,const char* name){checks.push_back({{"name",name},{"passed",good}});if(!good)throw std::runtime_error(name);};
#ifdef UDM_RECOVERY_IO_BASELINE
 const bool baseline=true;
#else
 const bool baseline=false;
#endif
 try{
  for(const std::string kind:{"ordinary-staged","ordinary-published","ordinary-early","ordinary-legacy-journal","private-published","child-published","legacy","replacement-complete","replacement-original","replacement-restore"})for(const std::string fault:{"none","metadata-save","destination","membership","newer-commit","graph","batch","read-failure","shutdown","occupied"}){
   if(baseline&&fault!="none")continue;if(fault=="occupied"&&kind!="ordinary-staged"&&kind!="ordinary-early")continue;
   activeCase=kind+"-"+fault;workflowError.clear();const auto folder=root/wide(activeCase);Manager m(folder/L"catalog");const auto files=folder/L"files";fs::create_directories(files);auto prefs=m.state["Settings"];prefs["DownloadFolder"]=utf8(files.wstring());prefs["CategoryFolders"]=false;m.setSettings(prefs);
   const bool paired=kind.find("replacement-")==0,legacy=kind=="legacy",child=kind=="child-published",privateRow=kind=="private-published",early=kind=="ordinary-early";
   JobPtr original,parent;std::string oldHash;
   if(paired){original=m.add("https://fixture.example/original","","result.bin","Main queue",true);atomicText(original->target(),"old bytes",false);oldHash=fileHash(original->target());original->data["Status"]="Complete";original->data["Size"]=9;original->data["Received"]=9;original->data["Sha256"]=oldHash;original->data["Finished"]=date();}
   auto j=m.add("https://fixture.example/recovered","",paired?"next.bin":"result.bin","Main queue",true);
   if(paired){j->data["FileName"]=str(original->data,"FileName");j->data["ReplacementOf"]=original->id();j->data["ReplacementHash"]=oldHash;j->data["PreviousPath"]=utf8((files/L"previous.bin").wstring());}
   if(child||privateRow){m.jobs.erase(std::find(m.jobs.begin(),m.jobs.end(),j));if(child){parent=m.add("https://fixture.example/parent","","parent.bin","Main queue",true);parent->video=j;}}
   m.save();const auto target=j->target(),previous=files/L"previous.bin",stage=files/(L".udm-"+wide(j->id())+L".assembling"),journal=m.root/(paired?L"replacements":L"publications")/(wide(j->id())+L".json");atomicText(stage,"verified bytes",false);const auto hash=fileHash(stage);
   if(legacy){fs::rename(stage,target);j->data["Status"]="Failed";j->data["Error"]="Could not replace the saved state (Windows error 1175). Retry after closing programs that hold the state file.";j->data["Size"]=14;j->data["Received"]=14;j->data["Sha256"]=hash;j->data["Finished"]=date();m.save();}
   else{const auto interrupt=paired?(kind=="replacement-complete"?"replacement-file-moved":"replacement-intent-written"):(kind=="ordinary-staged"||early?"publication-intent-written":"publication-file-moved");CatalogCheckpointTestAccess::observe(m,[&](const char* point){if(std::string_view(point)==interrupt)throw std::runtime_error("Owned interrupted byte IO setup");});try{m.publishFile(j,stage,hash);}catch(...){}CatalogCheckpointTestAccess::observe(m,{});check(fs::exists(journal),"A real interrupted publication supplies the byte recovery journal");if(early){auto op=Json::parse(readText(journal));op["FlushBeforePublish"]=true;atomicText(journal,op.dump(),false);}if(kind=="ordinary-legacy-journal"){auto op=Json::parse(readText(journal));op.erase("CommitId");op["Before"].erase("PublicationCommitId");op["After"].erase("PublicationCommitId");atomicText(journal,op.dump(),false);}if(kind=="replacement-restore")fs::rename(target,previous);}
   const auto originalJobs=m.jobs;const auto before=j->data;const auto beforeOriginal=original?original->data:Json();const auto beforeParent=parent?parent->data:Json();const auto beforeVideo=j->video,beforeAudio=j->audio;auto gate=std::make_shared<ByteIoGate>();CatalogCheckpointTestAccess::observe(m,[&](const char* point){gate->observe(point);if(fault=="read-failure"&&std::string_view(point)=="recovery-hash-read")throw std::runtime_error("Owned byte read checkpoint failure");});ByteIoObservation clear{m};std::future<void> stopper;
   auto work=std::async(std::launch::async,[&]{m.recoverReplacements(parent?parent:j);});struct Release {std::shared_ptr<ByteIoGate> gate;~Release(){gate->release();}} release{gate};
   check(gate->ready.wait_for(std::chrono::seconds(5))==std::future_status::ready,"Recovery reaches its actual held open-file SHA-256 read loop");
   {std::unique_lock<std::recursive_mutex> lock(m.mutex,std::try_to_lock);check(lock.owns_lock()!=baseline,"The read-loop control is blocked in 8.48 and model availability is required after detachment");if(lock.owns_lock()){
    check(m.isActive(j)&&m.downloadDestinationReserved(target),"Byte verification retains the affected job and destination reservation");check(m.snapshot()["Downloads"].size()==m.jobs.size(),"A UI-facing snapshot is available while verification is held");
    if(fault=="metadata-save"){j->data["Description"]="independent verified metadata";if(original)original->data["Description"]="independent original metadata";m.state["Settings"]["ByteIoMarker"]=activeCase;}
    if(fault=="destination")j->data["FileName"]="changed.bin";
    if(fault=="membership"){if(privateRow)m.jobs.push_back(j);else if(child)parent->video.reset();else m.jobs.erase(std::find(m.jobs.begin(),m.jobs.end(),j));}
    if(fault=="newer-commit")j->data["PublicationCommitId"]=guid();
    if(fault=="graph")j->video=std::make_shared<Job>(Json{{"Id",guid()},{"Folder",utf8(files.wstring())},{"FileName","other.bin"},{"Status","Paused"}});
    if(fault=="batch")CatalogCheckpointTestAccess::batch(m,true);
   }}
   const auto heldFile=legacy||kind=="ordinary-published"||kind=="ordinary-legacy-journal"||privateRow||child||kind=="replacement-complete"||kind=="replacement-original"?target:kind=="replacement-restore"?previous:stage;
   Handle writer(CreateFileW(heldFile.c_str(),GENERIC_WRITE,FILE_SHARE_READ|FILE_SHARE_WRITE|FILE_SHARE_DELETE,nullptr,OPEN_EXISTING,0,nullptr));check(!writer,"The actual open verification source refuses a concurrent writer");
   if(!legacy){Handle journalWriter(CreateFileW(journal.c_str(),GENERIC_WRITE,FILE_SHARE_READ|FILE_SHARE_WRITE|FILE_SHARE_DELETE,nullptr,OPEN_EXISTING,0,nullptr));check(!journalWriter,"The held immutable journal refuses writes during detached hashing");}
   if(fault=="metadata-save"){m.save();check(Json::parse(readText(m.root/L"state.json"))["Settings"]["ByteIoMarker"]==activeCase,"An independent durable model save completes while the recovery read loop is held");}
   if(fault=="occupied")atomicText(target,"foreign target",false);
   if(fault=="shutdown"){stopper=std::async(std::launch::async,[&]{m.stop();});const auto deadline=GetTickCount64()+5000;while(!CatalogCheckpointTestAccess::closing(m)&&GetTickCount64()<deadline)Sleep(1);check(CatalogCheckpointTestAccess::closing(m)&&stopper.wait_for(std::chrono::milliseconds(20))!=std::future_status::ready,"Shutdown marks closing but waits for the owned byte IO transaction");}
   gate->release();try{work.get();}catch(const std::exception& e){workflowError=e.what();}CatalogCheckpointTestAccess::observe(m,{});if(stopper.valid())stopper.get();CatalogCheckpointTestAccess::batch(m,false);check(!gate->expired,"Each actual read-loop hold is explicitly released before its deadline");
   const bool expectedFailure=fault!="none"&&fault!="metadata-save"&&!(fault=="occupied"&&!early);
   check(expectedFailure?!workflowError.empty():workflowError.empty(),"Changed inputs, lost membership, closing and read failures cannot apply an unproved recovery result");
   const bool paused=kind=="replacement-original"||kind=="replacement-restore"||(fault=="occupied"&&!early);
   if(expectedFailure){check(legacy||fs::exists(journal),"Rejected byte IO preserves the original journal for retry");check(!m.isActive(j),"Failed byte verification releases runtime job/path ownership");{Lock lock(m.mutex);m.jobs=originalJobs;j->data=before;j->video=beforeVideo;j->audio=beforeAudio;if(parent){parent->data=beforeParent;parent->video=j;}if(original)original->data=beforeOriginal;}if(fault=="occupied")fs::remove(target);
    if(fault=="shutdown"){Manager reopened(m.root);JobPtr restored;for(auto item:reopened.jobs)if(item->id()==(parent?parent:j)->id())restored=item;check(privateRow?reopened.jobs.empty()&&fileHash(target)==hash:restored&&str((parent?restored->video:restored)->data,"Status")== (paused?"Paused":"Complete"),"Reopening after shutdown recovers the retained intent without another transfer");reopened.stop();cases.push_back({{"kind",kind},{"fault",fault},{"modelAvailable",true},{"failure",workflowError}});continue;}
    m.recoverReplacements(parent?parent:j);
   }
   if(fault=="occupied"&&!early)check(readText(target)=="foreign target"&&readText(stage)=="verified bytes"&&str(j->data,"Status")=="Paused","An occupied ordinary target is never overwritten and unpublished bytes remain paused");
   else if(paused)check(readText(target)=="old bytes"&&readText(stage)=="verified bytes"&&str(j->data,"Status")=="Paused","Original retention and restoration preserve both old and unpublished bytes");
   else check(fs::exists(target)&&fileHash(target)==hash&&str(j->data,"Status")=="Complete","Accepted byte recovery records the exact complete file");
   if(paired&&kind=="replacement-complete")check(fileHash(previous)==oldHash,"Paired completion preserves the verified previous version");
   if(fault=="metadata-save")check(str(j->data,"Description")=="independent verified metadata"&&(!original||str(original->data,"Description")=="independent original metadata"),"Recovery application preserves independently saved metadata for both roles");
   check(!m.isActive(j)&&(legacy||!fs::exists(journal)),"Accepted verification and catalog proof retire the journal before releasing ownership");
   if(early&&fault=="none"&&!baseline)check(gate->flushes==1,"Early-intent recovery still performs its required body flush");
   if(kind=="replacement-original"&&fault=="none")check(gate->reads==(baseline?4u:2u),"An intact old replacement target is measured once after detachment instead of twice in the control");
   cases.push_back({{"kind",kind},{"fault",fault},{"modelAvailable",!baseline},{"readCheckpoints",gate->reads.load()},{"hashes",gate->hashes.load()},{"failure",workflowError}});m.stop();
  }
 }catch(const std::exception& e){error=e.what();}
 atomicText(root/L"results.json",Json{{"passed",error.empty()},{"error",error},{"case",activeCase},{"workflowError",workflowError},{"checks",checks},{"cases",cases},{"instrumented848Control",baseline}}.dump(2),false);std::cout<<checks.size()<<" checks; "<<(error.empty()?"passed":error)<<"\n";return error.empty()?0:1;
}
