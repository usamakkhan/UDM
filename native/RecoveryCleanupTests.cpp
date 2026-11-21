#include "Core.hpp"
#include "PublicationOwnership.hpp"
#include "PublicationCatalogProof.hpp"
#include <future>
#include <iostream>
namespace udm {struct CatalogCheckpointTestAccess {
 static void observe(Manager& m,std::function<void(const char*)> f){Lock lock(m.mutex);m.catalogCheckpoint=std::move(f);}
 static void batch(Manager& m,bool v){Lock lock(m.mutex);m.catalogTransaction=v;}
 static bool closing(Manager& m){Lock lock(m.mutex);return m.stopping;}
};}
using namespace udm;
struct CleanupGate {
 std::string point;std::promise<void> entered,released;std::shared_future<void> ready=entered.get_future().share(),wait=released.get_future().share();std::atomic_bool held=false,done=false,expired=false;
 explicit CleanupGate(std::string p):point(std::move(p)){}
 void observe(const char* phase){if(point==phase&&!held.exchange(true)){entered.set_value();if(wait.wait_for(std::chrono::seconds(10))!=std::future_status::ready){expired=true;throw std::runtime_error("Owned cleanup gate expired");}}}
 void release(){if(!done.exchange(true))released.set_value();}~CleanupGate(){release();}
};
struct ClearCleanup {Manager& m;~ClearCleanup(){CatalogCheckpointTestAccess::observe(m,{});}};
static const Json* savedCleanupRow(const Json& catalog,const std::string& id){std::vector<const Json*> rows;for(const auto& row:catalog.at("Downloads"))rows.push_back(&row);while(!rows.empty()){const auto& row=*rows.back();rows.pop_back();if(str(row,"Id")==id)return &row;for(const char* key:{"Video","Audio"})if(row.contains(key)&&row.at(key).is_object())rows.push_back(&row.at(key));}return nullptr;}
int wmain(int argc,wchar_t** argv){
 if(argc!=2)return 2;const auto root=fs::absolute(argv[1]);if(fs::exists(root))return 2;fs::create_directories(root);Json checks=Json::array(),cases=Json::array();std::string error,activeCase,workflowError;
 const auto check=[&](bool ok,const char* name){checks.push_back({{"name",name},{"passed",ok}});if(!ok)throw std::runtime_error(name);};
 try{
  for(const std::string kind:{"ordinary","paused","private","child","replacement","original-scope","paired-paused","legacy"})for(const std::string phase:{"recovery-cleanup-begin","recovery-cleanup-metadata","recovery-journal-retire","recovery-journal-retired","recovery-cleanup-complete"})for(const std::string fault:{"none","metadata-save","destination","membership","graph","batch","failure","shutdown","receipt-replaced","retained-lock","journal-readonly","retained-alias"}){
   if(kind=="legacy"&&phase!="recovery-cleanup-begin"&&phase!="recovery-cleanup-complete")continue;if(fault=="receipt-replaced"&&kind!="private")continue;if(fault=="retained-lock"&&(kind!="ordinary"||phase!="recovery-cleanup-begin"))continue;if(fault=="journal-readonly"&&((kind!="ordinary"&&kind!="private"&&kind!="replacement")||phase!="recovery-cleanup-begin"))continue;if(fault=="retained-alias"&&(kind!="ordinary"||phase!="recovery-cleanup-begin"))continue;
   activeCase=kind+"-"+phase+"-"+fault;workflowError.clear();const auto folder=root/wide(activeCase);Manager m(folder/L"catalog");const auto files=folder/L"files";fs::create_directories(files);auto prefs=m.state["Settings"];prefs["DownloadFolder"]=utf8(files.wstring());prefs["CategoryFolders"]=false;m.setSettings(prefs);const bool paired=kind=="replacement"||kind=="original-scope"||kind=="paired-paused",paused=kind=="paused"||kind=="paired-paused",legacy=kind=="legacy";
   JobPtr original,parent;std::string oldHash;
   if(paired){original=m.add("https://fixture.example/old","","result.bin","Main queue",true);atomicText(original->target(),"old bytes",false);oldHash=fileHash(original->target());original->data["Status"]="Complete";original->data["Size"]=9;original->data["Received"]=9;original->data["Sha256"]=oldHash;original->data["Finished"]=date();}
   auto j=m.add("https://fixture.example/new","",paired?"next.bin":"result.bin","Main queue",true);if(paired){j->data["FileName"]="result.bin";j->data["ReplacementOf"]=original->id();j->data["ReplacementHash"]=oldHash;j->data["PreviousPath"]=utf8((files/L"previous.bin").wstring());}
   const auto target=j->target(),stage=files/L"stage.part",retained=files/(L".udm-"+wide(j->id())+L".adaptive.mp4");
   if(!legacy){atomicText(retained,"retained bytes",false);j->data["RetainedMediaOutput"]={{"Version",1},{"Path",utf8(retained.wstring())},{"Container","mp4"}};}
   if(kind=="private"||kind=="child"){m.jobs.erase(std::find(m.jobs.begin(),m.jobs.end(),j));if(kind=="child"){parent=m.add("https://fixture.example/parent","","parent.bin","Main queue",true);parent->video=j;}}
   auto selected=kind=="original-scope"?original:parent?parent:j;const auto oldJobs=m.jobs;const auto video=selected->video,audio=selected->audio;m.save();const auto journal=m.root/(paired?L"replacements":L"publications")/(wide(j->id())+L".json");std::string digest;
   if(legacy){atomicText(target,"verified bytes",false);digest=fileHash(target);j->data["Status"]="Failed";j->data["Error"]="Could not replace the saved state (Windows error 1175). Retry after closing programs that hold the state file.";j->data["Size"]=14;j->data["Received"]=14;j->data["Sha256"]=digest;j->data["Finished"]=date();m.save();}
   else{atomicText(stage,"verified bytes",false);digest=fileHash(stage);if(fault=="retained-alias"){fs::remove(retained);check(CreateHardLinkW(retained.c_str(),stage.c_str(),nullptr)!=FALSE,"The retained source is an actual hard-link alias of published bytes");}CatalogCheckpointTestAccess::observe(m,[&](const char* point){if(std::string_view(point)==(paired?"replacement-file-moved":"publication-file-moved"))throw std::runtime_error("Cleanup setup interruption");});try{m.publishFile(j,stage,digest);}catch(...){}CatalogCheckpointTestAccess::observe(m,{});check(fs::exists(journal),"Cleanup starts from a real interrupted file publication");if(paused){fs::remove(target);if(!paired)atomicText(stage,"unpublished bytes",false);}}
   auto gate=std::make_shared<CleanupGate>(phase);CatalogCheckpointTestAccess::observe(m,[&](const char* point){gate->observe(point);if(fault=="failure"&&phase==point)throw std::runtime_error("Owned cleanup observation failure");});ClearCleanup clear{m};auto work=std::async(std::launch::async,[&]{m.recoverReplacements(selected);});struct Release {std::shared_ptr<CleanupGate> gate;~Release(){gate->release();}}release{gate};std::future<void> stopper;std::unique_ptr<Handle> locked;
   check(gate->ready.wait_for(std::chrono::seconds(5))==std::future_status::ready,"Recovery reaches the held post-acceptance cleanup boundary");Json accepted,acceptedOriginal,scopeAccepted,receipt,successor;const bool alreadyRetired=legacy||phase=="recovery-journal-retired"||phase=="recovery-cleanup-complete";
   {std::unique_lock<std::recursive_mutex> ui(m.mutex,std::try_to_lock);check(ui.owns_lock(),"Real journal retirement and post-acceptance cleanup leave Model available");check(m.isActive(selected)&&m.downloadDestinationReserved(target),"The operation graph and target remain reserved throughout cleanup");if(!legacy)check(m.downloadDestinationReserved(retained),"The separate retained media source is reserved until cleanup retires");check(str(j->data,"Status")== (paused?"Paused":"Complete"),"Cleanup begins only after the correct completed or paused outcome is applied");accepted=j->snapshot();acceptedOriginal=original?original->snapshot():Json();scopeAccepted=selected->data;const auto saved=Json::parse(readText(m.root/L"state.json"));const auto row=savedCleanupRow(saved,j->id());
    if(kind=="private"){receipt=PublicationCatalogProof::binding(j->data);check(!row&&saved.at("PublicationReceipts").at(str(receipt,"PublicationCommitId"))==receipt,"Private cleanup has an exact durable receipt without a visible row");}
    else check(row&&str(*row,"Status")==str(j->data,"Status")&&num(*row,"Size",-2)==num(j->data,"Size",-2),"The accepted history is already durable while cleanup is held");
    m.snapshot();if(fault=="metadata-save"){selected->data["Description"]="independent cleanup metadata";m.state["Settings"]["CleanupMarker"]=activeCase;}
    if(fault=="destination")selected->data["FileName"]="changed.bin";
    if(fault=="membership"){auto found=std::find(m.jobs.begin(),m.jobs.end(),selected);if(found==m.jobs.end())m.jobs.push_back(selected);else m.jobs.erase(found);}
    if(fault=="graph")selected->video=std::make_shared<Job>(Json{{"Id",guid()},{"Folder",utf8(files.wstring())},{"FileName","other.bin"},{"Status","Paused"}});
    if(fault=="batch")CatalogCheckpointTestAccess::batch(m,true);
    if(fault=="receipt-replaced"){successor=receipt;successor["Sha256"]=std::string(64,'1');m.state["PublicationReceipts"][str(receipt,"PublicationCommitId")]=successor;}
   }
   if(fault=="metadata-save"){m.save();check(str(Json::parse(readText(m.root/L"state.json"))["Settings"],"CleanupMarker")==activeCase,"An independent durable save completes while cleanup IO is held");}
   if(fault=="retained-lock"){locked=std::make_unique<Handle>(CreateFileW(retained.c_str(),GENERIC_READ,FILE_SHARE_READ,nullptr,OPEN_EXISTING,0,nullptr));check(bool(*locked),"A real retained-source reader can refuse best-effort deletion");}
   if(fault=="journal-readonly")check(SetFileAttributesW(journal.c_str(),FILE_ATTRIBUTE_READONLY)!=FALSE,"A real read-only journal will refuse accepted retirement");
   if(!legacy){check(fs::exists(journal)!=alreadyRetired,"Only the actual retirement boundary consumes its exact journal");if(!alreadyRetired){const auto moved=folder/L"refused.json";check(!MoveFileExW(journal.c_str(),moved.c_str(),0),"The exclusive journal claim remains pinned until accepted retirement");}}
   if(fault=="shutdown"){stopper=std::async(std::launch::async,[&]{m.stop();});const auto deadline=GetTickCount64()+5000;while(!CatalogCheckpointTestAccess::closing(m)&&GetTickCount64()<deadline)Sleep(1);check(CatalogCheckpointTestAccess::closing(m)&&stopper.wait_for(std::chrono::milliseconds(20))!=std::future_status::ready,"Shutdown waits for accepted cleanup and all held ownership");}
   gate->release();try{work.get();}catch(const std::exception& e){workflowError=e.what();}CatalogCheckpointTestAccess::observe(m,{});if(stopper.valid())stopper.get();CatalogCheckpointTestAccess::batch(m,false);locked.reset();check(!gate->expired,"The owned cleanup gate is explicitly released");const bool refused=fault!="none"&&fault!="metadata-save"&&fault!="retained-lock"&&fault!="retained-alias"&&fault!="shutdown";check(fault=="shutdown"?(workflowError.empty()||workflowError=="UDM is closing."):refused?!workflowError.empty():workflowError.empty(),"Changed scope, receipt, batch, failure and shutdown refuse stale cleanup continuation");check(!m.isActive(selected),"Accepted and refused cleanup release every operation reservation");
   if(!paused)check(fileHash(target)==digest,"Cleanup preserves the exact verified completed file");else if(paired)check(fileHash(target)==oldHash,"Paused replacement cleanup preserves its exact restored original");else check(!fs::exists(target),"Paused missing publication never invents a completed file");
   if(fault=="receipt-replaced")check(m.state.at("PublicationReceipts").at(str(receipt,"PublicationCommitId"))==successor,"Cleanup cannot erase an unrelated successor receipt");
   if(fault=="journal-readonly")check(SetFileAttributesW(journal.c_str(),FILE_ATTRIBUTE_NORMAL)!=FALSE,"The fixture restores only its owned read-only journal for explicit retry");
   if(refused){check(legacy||fs::exists(journal)!=alreadyRetired,"A cleanup failure preserves an unretired journal without undoing completed retirement");{Lock lock(m.mutex);m.jobs=oldJobs;j->data=accepted;if(original)original->data=acceptedOriginal;selected->data=scopeAccepted;selected->video=video;selected->audio=audio;if(!receipt.is_null())m.state["PublicationReceipts"][str(receipt,"PublicationCommitId")]=receipt;}
    if(fault=="shutdown"){Manager reopened(m.root);const auto saved=Json::parse(readText(m.root/L"state.json"));(void)saved;check(!fs::exists(journal),"Restart finishes or acknowledges only already-durable accepted cleanup");reopened.stop();}
    else m.recoverReplacements(selected);
   }
   if(fault=="shutdown"){Manager reopened(m.root);check(!fs::exists(journal),"Shutdown drains accepted cleanup before restart rather than abandoning its journal");reopened.stop();}
   if(!legacy)check(!fs::exists(journal),"Successful cleanup or explicit retry retires its exact journal");
   if(fault=="metadata-save")check(str(selected->data,"Description")=="independent cleanup metadata","Cleanup preserves independent current metadata");
   if(fault=="retained-lock")check(fs::exists(retained),"Best-effort retained-output failure preserves the locked source and accepted final file");
   if((fault=="none"||fault=="retained-alias")&&!legacy&&!paused)check(!fs::exists(retained),"Successful metadata cleanup removes the separate retained source");
   cases.push_back({{"kind",kind},{"phase",phase},{"fault",fault},{"error",workflowError},{"acceptedBeforeCleanup",true},{"modelAvailable",true}});m.stop();
  }
 }catch(const std::exception& e){error=e.what();}
 atomicText(root/L"results.json",Json{{"passed",error.empty()},{"error",error},{"case",activeCase},{"workflowError",workflowError},{"checks",checks},{"cases",cases}}.dump(2),false);std::cout<<checks.size()<<" checks; "<<(error.empty()?"passed":error)<<"\n";return error.empty()?0:1;
}
