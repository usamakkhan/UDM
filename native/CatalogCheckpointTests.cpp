#include "Core.hpp"
#include "CatalogAdmissionRegistry.hpp"
#include <future>
#include <iostream>
namespace udm {
struct CatalogCheckpointTestAccess {
 static CatalogAdmissionRegistry* registry(Manager& m){return m.admissionRegistry.get();}
 static void automatic(Manager& m){m.checkpointPeriodic();}
 static void staleAutomatic(Manager& m){m.saveCatalog(true);}
 static unsigned long long sequence(Manager& m){Lock lock(m.mutex);return m.catalogSequence;}
 static void stopping(Manager& m,bool value){Lock lock(m.mutex);m.stopping=value;}
 static std::function<void(const char*)>& checkpoint(Manager& m){return m.catalogCheckpoint;}
 static void transaction(Manager& m,bool value){Lock lock(m.mutex);m.catalogTransaction=value;}
 static std::string text(Manager& m){Lock lock(m.mutex);return m.checkpointText;}
 static void forgetText(Manager& m){Lock lock(m.mutex);m.checkpointText.clear();}
};
}
using namespace udm;
int wmain(int argc,wchar_t** argv){
 if(argc!=2)return 2;fs::path root=argv[1];if(fs::exists(root))return 3;
 fs::create_directories(root);Json checks=Json::array();std::string error;bool passed=false;
 auto check=[&](bool value,const char* name){checks.push_back({{"name",name},{"passed",value}});if(!value)throw std::runtime_error(name);};
 try{
  Manager m(root/L"catalog");m.save();auto original=readText(m.root/L"state.json");
  auto seq=CatalogCheckpointTestAccess::sequence(m);unsigned flushed=0;
  struct ResetCallback {Manager& manager;~ResetCallback(){CatalogCheckpointTestAccess::checkpoint(manager)={};}} reset{m};
  CatalogCheckpointTestAccess::checkpoint(m)=[&](const char* point){if(std::string(point)=="temporary-flushed")++flushed;};
  CatalogCheckpointTestAccess::automatic(m);
  check(CatalogCheckpointTestAccess::sequence(m)==seq&&flushed==0,"Unchanged automatic checkpoint performs no durable write");
  m.state["Settings"]["CheckpointTestMarker"]="superseded";m.save();++seq;
  check(flushed==1,"Explicit save durably commits the newer state");
  CatalogCheckpointTestAccess::staleAutomatic(m);
  check(CatalogCheckpointTestAccess::sequence(m)==seq&&flushed==1,"An obsolete automatic dirty decision is rechecked before snapshot and IO");
  check(str(Json::parse(readText(m.root/L"state.json"))["Settings"],"CheckpointTestMarker")=="superseded","Skipping obsolete automatic work retains the explicit commit");
  m.state["Settings"]["CheckpointTestMarker"]="changed";CatalogCheckpointTestAccess::automatic(m);++seq;
  check(flushed==2&&CatalogCheckpointTestAccess::sequence(m)==seq,"Changed automatic checkpoint still commits exactly once");
  check(str(Json::parse(readText(m.root/L"state.json"))["Settings"],"CheckpointTestMarker")=="changed","Changed settings survive a catalog reload");
  auto stable=readText(m.root/L"state.json");m.save();++seq;
  check(flushed==3&&CatalogCheckpointTestAccess::sequence(m)==seq,"Explicit unchanged save retains its forced durable barrier");
  bool injected=false;CatalogCheckpointTestAccess::checkpoint(m)=[&](const char* point){if(std::string(point)=="temporary-flushed"){injected=true;throw std::runtime_error("fixture commit interruption");}};
  try{m.save();check(false,"Injected save must fail");}catch(const std::exception& e){check(std::string(e.what())=="fixture commit interruption","Forced-save failure reports the original error");}++seq;
  check(injected&&!m.storageError.empty()&&readText(m.root/L"state.json")==stable,"Failure preserves the published catalog and records a storage error");
  flushed=0;CatalogCheckpointTestAccess::checkpoint(m)=[&](const char* point){if(std::string(point)=="temporary-flushed")++flushed;};
  CatalogCheckpointTestAccess::automatic(m);++seq;
  check(flushed==1&&m.storageError.empty()&&CatalogCheckpointTestAccess::sequence(m)==seq,"Automatic checkpoint retries a failed save even when data matches its last checkpoint");
  CatalogCheckpointTestAccess::transaction(m,true);m.state["Settings"]["CheckpointTestMarker"]="transaction";CatalogCheckpointTestAccess::automatic(m);
  check(flushed==1&&CatalogCheckpointTestAccess::sequence(m)==seq&&readText(m.root/L"state.json")==stable,"Automatic checkpoint does not publish an unfinished transaction");
  CatalogCheckpointTestAccess::transaction(m,false);CatalogCheckpointTestAccess::automatic(m);++seq;
  check(flushed==2&&str(Json::parse(readText(m.root/L"state.json"))["Settings"],"CheckpointTestMarker")=="transaction","Transaction state is saved after deferral ends");
  CatalogCheckpointTestAccess::stopping(m,true);m.state["Settings"]["CheckpointTestMarker"]="shutdown";CatalogCheckpointTestAccess::automatic(m);
  check(flushed==2&&CatalogCheckpointTestAccess::sequence(m)==seq,"Stopping suppresses automatic checkpoints");
  m.save();++seq;
  check(flushed==3&&str(Json::parse(readText(m.root/L"state.json"))["Settings"],"CheckpointTestMarker")=="shutdown","Forced shutdown save persists final state while stopping");
  CatalogCheckpointTestAccess::stopping(m,false);CatalogCheckpointTestAccess::checkpoint(m)={};
  {
   unsigned writes=0;auto owner=std::make_unique<Manager>(root/L"destructor-clean");CatalogCheckpointTestAccess::checkpoint(*owner)=[&](const char* point){if(std::string_view(point)=="temporary-flushed")++writes;};
   owner->save();owner->stop();const auto before=readText(owner->root/L"state.json");
   check(writes==1,"Shutdown reuses an exact acknowledged checkpoint only after inspecting the pinned catalog");owner.reset();
   check(writes==1&&readText(root/L"destructor-clean/state.json")==before,"Destruction after a successful unchanged shutdown rechecks the real catalog without another durable write");
  }
  {
   unsigned writes=0;auto owner=std::make_unique<Manager>(root/L"destructor-dirty");CatalogCheckpointTestAccess::checkpoint(*owner)=[&](const char* point){if(std::string_view(point)=="temporary-flushed")++writes;};
   owner->save();owner->stop();owner->state["Settings"]["AfterStop"]="latest settings";owner.reset();
   check(writes==2&&str(Json::parse(readText(root/L"destructor-dirty/state.json"))["Settings"],"AfterStop")=="latest settings","A live-state change after explicit shutdown still gets a forced destructor save");
  }
  {
   unsigned writes=0;auto owner=std::make_unique<Manager>(root/L"destructor-missing");CatalogCheckpointTestAccess::checkpoint(*owner)=[&](const char* point){if(std::string_view(point)=="temporary-flushed")++writes;};
   owner->save();owner->stop();const auto before=Json::parse(readText(owner->root/L"state.json"));fs::remove(owner->root/L"state.json");owner.reset();
   check(writes==2&&Json::parse(readText(root/L"destructor-missing/state.json"))==before,"A removed on-disk catalog is recreated rather than trusted from the checkpoint cache");
  }
  for(const std::string kind:{"malformed","changed-valid"}){
   unsigned writes=0;auto folder=root/wide("destructor-"+kind);auto owner=std::make_unique<Manager>(folder);CatalogCheckpointTestAccess::checkpoint(*owner)=[&](const char* point){if(std::string_view(point)=="temporary-flushed")++writes;};
   owner->save();owner->stop();const auto before=Json::parse(readText(owner->root/L"state.json"));atomicText(owner->root/L"state.json",kind=="malformed"?"not JSON":Json{{"other","catalog"}}.dump(),false);owner.reset();
   check(writes==2&&Json::parse(readText(folder/L"state.json"))==before,"A malformed or externally changed catalog forces a repair save even when live state is unchanged");
  }
  {
   unsigned writes=0;bool fail=false;auto owner=std::make_unique<Manager>(root/L"destructor-retry");CatalogCheckpointTestAccess::checkpoint(*owner)=[&](const char* point){if(std::string_view(point)=="temporary-flushed"){++writes;if(fail)throw std::runtime_error("destructor fixture storage failure");}};
   owner->save();owner->stop();fail=true;bool rejected=false;try{owner->save();}catch(const std::exception& e){rejected=std::string(e.what())=="destructor fixture storage failure";}
   check(rejected&&!owner->storageError.empty(),"The destructor retry fixture retains a real failed-write storage error");fail=false;const auto before=Json::parse(readText(owner->root/L"state.json"));owner.reset();
   check(writes==3&&Json::parse(readText(root/L"destructor-retry/state.json"))==before,"A storage error forces a durable destructor retry instead of the unchanged-catalog shortcut");
  }
  {
   unsigned writes=0;auto folder=root/L"destructor-pending-admission";auto owner=std::make_unique<Manager>(folder);CatalogCheckpointTestAccess::checkpoint(*owner)=[&](const char* point){if(std::string_view(point)=="temporary-flushed")++writes;};
   owner->save();owner->stop();auto registry=CatalogCheckpointTestAccess::registry(*owner);const auto id=guid();auto lease=registry->acquire("shutdown-admission","shutdown-binding");
   lease.prepare(std::make_shared<const CatalogAdmissionReservation>("shutdown-admission",Json{{"Id",id},{"Folder",utf8((root/L"reserved").wstring())},{"FileName","reserved.bin"},{"Status","Awaiting confirmation"}},Json{{"protocol",3},{"status","accepted"},{"id",id},{"fingerprint","shutdown-binding"}}));
   owner->save();const auto sequence=registry->lookup("shutdown-admission").durableSequence;
   check(lease.committed()&&sequence&&writes==2,"The admission fixture durably writes an unresolved overlay without installing it in live jobs");owner.reset();
   check(writes==3&&lease.committed(),"An equal catalog with a pending admission still gets an actual destructor write and retains its acknowledged receipt");
   const auto saved=Json::parse(readText(folder/L"state.json"));check(saved["Downloads"].size()==1&&str(saved["Downloads"][0],"Id")==id&&str(saved["BrowserCaptures"]["shutdown-admission"],"id")==id,"Destructor completion preserves the pending admitted job and exact acceptance receipt");
  }
  for(const std::string kind:{"dirty","missing","malformed","changed-valid","failed-save"}){
   Manager owner(root/wide("stop-"+kind));unsigned writes=0;bool fail=false;
   CatalogCheckpointTestAccess::checkpoint(owner)=[&](const char* point){if(std::string_view(point)=="temporary-flushed"){++writes;if(fail)throw std::runtime_error("shutdown injected storage failure");}};
   owner.save();const auto path=owner.root/L"state.json";
   if(kind=="dirty")owner.state["Settings"]["ShutdownMarker"]="must survive";
   if(kind=="missing")fs::remove(path);
   if(kind=="malformed"||kind=="changed-valid")atomicText(path,kind=="malformed"?"not JSON":Json{{"other","catalog"}}.dump(),false);
   if(kind=="failed-save"){fail=true;try{owner.save();}catch(...){}fail=false;}
   const auto before=writes;owner.stop();
   check(writes==before+1&&owner.storageError.empty(),"Shutdown performs a real write for dirty, missing, invalid, changed or previously failed state");
   auto saved=Json::parse(readText(path));check(saved==owner.snapshot(),"Shutdown fallback persists the exact current state");
   owner.stop();check(writes==before+1,"Repeated clean shutdown does not repeat the durable write");
   CatalogCheckpointTestAccess::checkpoint(owner)={};
  }
  {
   unsigned writes=0;bool fail=false;Manager owner(root/L"accepted-text");
   CatalogCheckpointTestAccess::checkpoint(owner)=[&](const char* stage){if(std::string_view(stage)=="temporary-flushed"){++writes;if(fail)throw std::runtime_error("accepted-text injected failure");}};
   owner.save();const auto first=readText(owner.root/L"state.json");
   check(CatalogCheckpointTestAccess::text(owner)==first,"Accepted text belongs to the actual durable snapshot");
   owner.state["Settings"]["AcceptedTextMarker"]="new generation";fail=true;bool rejected=false;
   try{owner.save();}catch(const std::exception&){rejected=true;}fail=false;
   check(rejected&&CatalogCheckpointTestAccess::text(owner)==first&&readText(owner.root/L"state.json")==first,
         "Failed persistence cannot replace accepted bytes with an uncommitted serialization");
   owner.save();const auto second=readText(owner.root/L"state.json");
   check(second!=first&&CatalogCheckpointTestAccess::text(owner)==second,"Successful retry replaces both accepted snapshot and its exact bytes");
   const auto before=writes;CatalogCheckpointTestAccess::forgetText(owner);owner.stop();
   check(writes==before+1,"Absent accepted bytes force a durable stop checkpoint despite matching parsed state");
   check(CatalogCheckpointTestAccess::text(owner)==readText(owner.root/L"state.json"),"Fallback persistence installs a fresh accepted serialization");
   CatalogCheckpointTestAccess::checkpoint(owner)={};
  }
  for(const bool bom:{false,true}){
   unsigned writes=0;Manager owner(root/(bom?L"accepted-bom":L"accepted-whitespace"));
   CatalogCheckpointTestAccess::checkpoint(owner)=[&](const char* stage){if(std::string_view(stage)=="temporary-flushed")++writes;};
   owner.save();const auto text=CatalogCheckpointTestAccess::text(owner);const auto before=writes;
   atomicText(owner.root/L"state.json",bom?std::string("\xEF\xBB\xBF")+text:Json::parse(text).dump(2),false);
   owner.stop();
   check(writes==before+(bom?0:1),"BOM normalization remains accepted while alternate JSON formatting requires a conservative save");
   check(CatalogCheckpointTestAccess::text(owner)==readText(owner.root/L"state.json"),"The real catalog and accepted text agree after formatting validation");
   CatalogCheckpointTestAccess::checkpoint(owner)={};
  }
  {
   Manager owner(root/L"accepted-text-superseded");owner.state["Settings"]["AcceptedTextMarker"]="old";owner.save();
   std::promise<void> entered,release;auto ready=entered.get_future();auto released=release.get_future().share();
   std::atomic<DWORD> worker{0};std::atomic_bool once{false};std::future<void> saving;
   struct Cleanup {Manager& owner;std::promise<void>& release;std::future<void>& saving;~Cleanup(){try{release.set_value();}catch(...){}if(saving.valid())try{saving.get();}catch(...){}CatalogCheckpointTestAccess::checkpoint(owner)={};}} cleanup{owner,release,saving};
   CatalogCheckpointTestAccess::checkpoint(owner)=[&](const char* stage){if(std::string_view(stage)=="snapshot-prepared"&&GetCurrentThreadId()==worker&&!once.exchange(true)){entered.set_value();released.wait();}};
   saving=std::async(std::launch::async,[&]{worker=GetCurrentThreadId();owner.save();});
   check(ready.wait_for(std::chrono::seconds(3))==std::future_status::ready,"An actual detached older save is held before coordinator admission");
   {Lock lock(owner.mutex);owner.state["Settings"]["AcceptedTextMarker"]="new";}owner.save();
   const auto newer=readText(owner.root/L"state.json");
   check(CatalogCheckpointTestAccess::text(owner)==newer&&str(Json::parse(newer)["Settings"],"AcceptedTextMarker")=="new",
         "A newer accepted save owns the cached bytes while an older serialization is held");
   release.set_value();saving.get();
   check(CatalogCheckpointTestAccess::text(owner)==newer&&readText(owner.root/L"state.json")==newer,
         "Skipped stale publication cannot overwrite the newer accepted byte cache");
  }
  {
   unsigned writes=0;auto owner=std::make_unique<Manager>(root/L"destructor-missing-accepted-text");
   CatalogCheckpointTestAccess::checkpoint(*owner)=[&](const char* stage){if(std::string_view(stage)=="temporary-flushed")++writes;};
   owner->save();owner->stop();const auto expected=owner->snapshot();const auto path=owner->root/L"state.json";
   const auto before=writes;CatalogCheckpointTestAccess::forgetText(*owner);owner.reset();
   check(writes==before+1&&Json::parse(readText(path))==expected,"Destructor cannot skip its repair save without accepted-byte ownership");
  }
  passed=true;
 }catch(const std::exception& e){error=e.what();}
 atomicText(root/L"results.json",Json{{"passed",passed},{"error",error},{"checks",checks}}.dump(2),false);
 std::cout<<checks.size()<<" checks; "<<(passed?"passed":error)<<"\n";return passed?0:1;
}
