#include "Core.hpp"
#include "PublicationCatalogProof.hpp"
#include "PublicationOwnership.hpp"
#include <future>
#include <iostream>
namespace udm {struct CatalogCheckpointTestAccess {
 static void observe(Manager& m,std::function<void(const char*)> fn){Lock lock(m.mutex);m.catalogCheckpoint=std::move(fn);}
 static void batch(Manager& m,bool value){Lock lock(m.mutex);m.catalogTransaction=value;}
 static bool closing(Manager& m){Lock lock(m.mutex);return m.stopping;}
};}
using namespace udm;
struct PruningGate {
 std::string phase;std::promise<void> entered,released;std::shared_future<void> ready=entered.get_future().share(),wait=released.get_future().share();std::atomic_bool first=false,done=false,expired=false;
 explicit PruningGate(std::string point):phase(std::move(point)){}
 bool observe(const char* point){if(phase==point&&!first.exchange(true)){entered.set_value();if(wait.wait_for(std::chrono::seconds(10))!=std::future_status::ready){expired=true;throw std::runtime_error("Pruning gate expired");}return true;}return false;}
 void release(){if(!done.exchange(true))released.set_value();}~PruningGate(){release();}
};
static Json privateReceipt(const fs::path& files){return PublicationCatalogProof::binding(Json{{"Id",guid()},{"PublicationCommitId",guid()},{"Folder",utf8(files.wstring())},{"FileName","output-"+guid()+".bin"},{"Status","Complete"},{"Size",14},{"Received",14},{"Sha256",std::string(64,'a')},{"Finished",date()}});}
int wmain(int argc,wchar_t** argv){
 if(argc!=2)return 2;const auto root=fs::absolute(argv[1]);if(fs::exists(root))return 2;fs::create_directories(root);
 Json checks=Json::array(),cases=Json::array();std::string error,active;
 const auto check=[&](bool value,const char* label){checks.push_back({{"name",label},{"passed",value}});if(!value)throw std::runtime_error(label);};
 try{
  const std::vector<std::pair<std::string,std::string>> scenarios={
   {"receipt-prune-inventory","none"},{"receipt-prune-inventory","journal-owner"},{"receipt-prune-inventory","unrelated-publisher"},{"receipt-prune-inventory","multiple"},{"receipt-prune-inventory","inventory-failure"},{"receipt-prune-inventory","known-description"},{"receipt-prune-inventory","known-destination"},{"receipt-prune-inventory","new-receipt"},{"receipt-prune-inventory","successor"},{"receipt-prune-inventory","removed"},{"receipt-prune-inventory","journal"},{"receipt-prune-inventory","batch"},{"receipt-prune-inventory","shutdown"},
   {"receipt-prune-catalog-prepared","none"},{"receipt-prune-catalog-prepared","shutdown"},{"receipt-prune-catalog-prepared","new-receipt"},{"receipt-prune-catalog-prepared","successor"},{"receipt-prune-catalog-prepared","deferred"},
   {"temporary-flushed","none"},{"temporary-flushed","multiple"},{"temporary-flushed","new-receipt"},{"temporary-flushed","failure"},{"temporary-flushed","failure-successor"},{"temporary-flushed","shutdown"},
   {"snapshot-prepared","later-containing"},{"publication-catalog-written","lost-ack"}
  };
  for(size_t index=0;index<scenarios.size();++index){
   const auto& [phase,fault]=scenarios[index];active=phase+"/"+fault;const auto folder=root/std::to_wstring(index);Manager m(folder/L"catalog");const auto files=folder/L"files";fs::create_directories(files);
   auto receipt=privateReceipt(files);const auto target=files/wide(str(receipt,"FileName"));atomicText(target,"verified bytes",false);receipt["Sha256"]=fileHash(target);const auto originalHash=str(receipt,"Sha256"),id=str(receipt,"PublicationCommitId");
   std::vector<std::pair<fs::path,std::string>> completed{{target,originalHash}};JobPtr known;
   {Lock lock(m.mutex);PublicationCatalogProof::installReceipt(m,receipt);
    if(fault=="multiple")for(unsigned i=1;i<16;++i){auto row=privateReceipt(files);const auto file=files/wide(str(row,"FileName"));atomicText(file,"verified bytes",false);row["Sha256"]=fileHash(file);completed.push_back({file,str(row,"Sha256")});PublicationCatalogProof::installReceipt(m,row);}
    if(fault=="known-description"||fault=="known-destination"){known=std::make_shared<Job>(receipt);m.jobs.push_back(known);}
   }m.save();
   auto gate=std::make_shared<PruningGate>(phase);
   CatalogCheckpointTestAccess::observe(m,[&](const char* point){const bool held=gate->observe(point);if(((fault=="failure"||fault=="failure-successor")&&std::string_view(point)=="temporary-flushed")||(fault=="lost-ack"&&std::string_view(point)=="publication-catalog-written")||(fault=="later-containing"&&held)||(fault=="inventory-failure"&&held))throw std::runtime_error("Owned pruning save failure");});
   auto work=std::async(std::launch::async,[&]{m.recoverReplacements();});struct Release {std::shared_ptr<PruningGate> gate;~Release(){gate->release();}}release{gate};
   check(gate->ready.wait_for(std::chrono::seconds(5))==std::future_status::ready,"Pruning reaches its actual inventory or persistence boundary");
   Json extra,successor;fs::path journal;std::future<void> stopper;
   {std::unique_lock<std::recursive_mutex> lock(m.mutex,std::try_to_lock);check(lock.owns_lock(),"Receipt inventory and catalog IO leave Model available");m.snapshot();check(m.downloadDestinationReserved(target),"The private target stays reserved without a visible job");
    bool refused=false;try{auto other=PublicationOwnership::receiptRetirement(m,{}, {target});}catch(...){refused=true;}check(refused,"A second publisher cannot take the retained private destination");
    if(fault=="new-receipt"){extra=privateReceipt(files);PublicationCatalogProof::installReceipt(m,extra);}
    if(fault=="successor"||fault=="failure-successor"){successor=receipt;successor["Sha256"]=std::string(64,'b');m.state["PublicationReceipts"][id]=successor;}
    if(fault=="removed")m.state.erase("PublicationReceipts");
    if(fault=="known-description")known->data["Description"]="independent current metadata";
    if(fault=="known-destination")known->data["FileName"]="changed.bin";
   }
   if(fault=="unrelated-publisher"){
    const auto otherFolder=folder/L"unrelated";fs::create_directories(otherFolder);auto data=privateReceipt(otherFolder);auto other=std::make_shared<Job>(data);const auto stage=otherFolder/L"stage.part";atomicText(stage,"verified bytes",false);
    m.publishFile(other,stage,originalHash);check(fileHash(other->target())==originalHash&&!fs::exists(stage),"An unrelated real publication can complete while private receipt inventory is held");completed.push_back({other->target(),originalHash});
   }
   if(fault=="journal-owner"){
    const auto otherFolder=folder/L"other";fs::create_directories(otherFolder);auto data=receipt;data["Folder"]=utf8(otherFolder.wstring());auto competing=std::make_shared<Job>(data);const auto stage=otherFolder/L"stage.part";atomicText(stage,"verified bytes",false);
    bool rejected=false;try{m.publishFile(competing,stage,originalHash);}catch(...){rejected=true;}
    check(rejected&&fs::exists(stage)&&!fs::exists(competing->target())&&!fs::exists(m.root/L"publications"/(wide(str(receipt,"Id"))+L".json")),"The real publisher cannot reuse a reserved journal ID through another destination");
   }
   if(fault=="journal"){journal=m.root/L"publications"/(wide(str(receipt,"Id"))+L".json");atomicText(journal,"{}",false);}
   if(fault=="batch"||fault=="deferred")CatalogCheckpointTestAccess::batch(m,true);
   if(fault=="new-receipt"&&phase!="temporary-flushed")m.save();
   if(fault=="later-containing"){
    // The first save has captured the exact absence but has not acquired the
    // writer mutex. A newer containing snapshot publishes before it fails.
    {Lock lock(m.mutex);m.state["Settings"]["PruningLaterWriter"]="accepted";}m.save();
   }
   if(fault=="shutdown"){stopper=std::async(std::launch::async,[&]{m.stop();});const auto until=GetTickCount64()+3000;while(!CatalogCheckpointTestAccess::closing(m)&&GetTickCount64()<until)Sleep(1);check(CatalogCheckpointTestAccess::closing(m)&&stopper.wait_for(std::chrono::milliseconds(10))!=std::future_status::ready,"Shutdown waits for owned pruning IO and acceptance");}
   gate->release();std::string failure;try{work.get();}catch(const std::exception& e){failure=e.what();}CatalogCheckpointTestAccess::observe(m,{});CatalogCheckpointTestAccess::batch(m,false);if(stopper.valid())stopper.get();
   const bool refused=fault=="inventory-failure"||fault=="known-destination"||fault=="failure"||fault=="failure-successor"||fault=="lost-ack"||fault=="deferred"||fault=="batch"||fault=="removed"||(fault=="successor"&&phase!="temporary-flushed")||(fault=="shutdown"&&phase=="receipt-prune-inventory");
   check(refused?!failure.empty():(failure.empty()||(fault=="shutdown"&&failure=="UDM is closing.")),"Failed, stale, deferred and accepted pruning outcomes are distinguished");check(!gate->expired,"The owned IO hold was released explicitly");check(!PublicationOwnership::reserved(m,target),"Pruning releases every reserved destination on success and refusal");bool exact=true;for(const auto& output:completed)exact=exact&&fileHash(output.first)==output.second;check(exact,"Pruning never changes completed file bytes");
   {Lock lock(m.mutex);
    const auto store=m.state.find("PublicationReceipts");const auto current=[&](const std::string& key)->Json{return store!=m.state.end()&&store->is_object()&&store->contains(key)?store->at(key):Json();};
    if(!extra.is_null())check(current(str(extra,"PublicationCommitId"))==extra,"Pruning preserves a concurrently added unrelated receipt");
    if(!successor.is_null())check(current(id)==successor,"Neither rollback nor acceptance overwrites a later receipt");
    else if(fault=="removed")check(current(id).is_null(),"A concurrently removed receipt is not resurrected before mutation");
    else check(current(id).is_null()==(!refused&&fault!="journal"),"Only an accepted journal-free receipt is retired");
   }
   if(fault=="journal"){check(fs::exists(journal),"An observed new journal prevents receipt retirement");fs::remove(journal);}
   const auto saved=Json::parse(readText(m.root/L"state.json"));if(!refused&&fault!="journal")check(!saved.contains("PublicationReceipts")||!saved.at("PublicationReceipts").contains(id),"Actual persisted acceptance contains the exact receipt absence");
   if(fault=="later-containing")check(str(saved["Settings"],"PruningLaterWriter")=="accepted","A newer exact snapshot can prove retirement after the older save fails");
   if(fault=="multiple")check(!saved.contains("PublicationReceipts"),"All sixteen journal-free private receipts are durably retired together");
   if(fault=="known-description")check(str(known->data,"Description")=="independent current metadata","Pruning preserves independent visible-job metadata");
   cases.push_back({{"phase",phase},{"fault",fault},{"modelAvailable",true},{"error",failure}});m.stop();
  }
  {
   active="reserved-target-skip";Manager m(root/L"reserved/catalog");const auto files=root/L"reserved/files";fs::create_directories(files);const auto receipt=privateReceipt(files);const auto target=files/wide(str(receipt,"FileName"));
   {Lock lock(m.mutex);PublicationCatalogProof::installReceipt(m,receipt);}m.save();
   {auto owner=PublicationOwnership::receiptRetirement(m,{}, {target});owner.beginCommit();m.recoverReplacements();check(m.state.at("PublicationReceipts").at(str(receipt,"PublicationCommitId"))==receipt,"An already reserved target keeps its receipt without blocking unrelated recovery");}
   m.recoverReplacements();check(!m.state.contains("PublicationReceipts"),"Released private ownership allows a later explicit retirement");m.stop();
  }
  {
   active="absence-proof-contract";Manager m(root/L"proof/catalog");const auto files=root/L"proof/files";fs::create_directories(files);auto a=privateReceipt(files),b=privateReceipt(files);const Json receipts={{str(a,"PublicationCommitId"),a},{str(b,"PublicationCommitId"),b}};
   {Lock lock(m.mutex);PublicationCatalogProof::installReceipt(m,a);PublicationCatalogProof::installReceipt(m,b);}m.save();
   auto proof=PublicationCatalogProof::prepareReceiptRetirement(m,receipts);bool refused=false;try{m.save();}catch(...){refused=true;}check(refused&&!proof.resolve(),"A snapshot containing any retired receipt cannot acknowledge absence");proof.release();
   {Lock lock(m.mutex);PublicationCatalogProof::eraseReceipt(m,a);PublicationCatalogProof::eraseReceipt(m,b);proof=PublicationCatalogProof::prepareReceiptRetirement(m,receipts);}
   refused=false;try{auto completion=PublicationCatalogProof::prepare(m,b,true);}catch(...){refused=true;}check(refused,"Receipt retirement excludes completion proof for every affected job");m.save();check(proof.resolve(),"An exact persisted absence proves all selected retirements");proof.release();
   auto completion=PublicationCatalogProof::prepare(m,b,true);refused=false;try{auto retirement=PublicationCatalogProof::prepareReceiptRetirement(m,receipts);}catch(...){refused=true;}check(refused,"An existing completion proof excludes overlapping receipt retirement");completion.release();m.stop();
  }
 }catch(const std::exception& failure){error=failure.what();}
 atomicText(root/L"results.json",Json{{"passed",error.empty()},{"error",error},{"case",active},{"checks",checks},{"cases",cases}}.dump(2),false);
 std::cout<<checks.size()<<" checks; "<<(error.empty()?"passed":error)<<"\n";return error.empty()?0:1;
}
