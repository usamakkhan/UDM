#include "Core.hpp"
#include <iostream>
namespace udm {
struct CatalogCheckpointTestAccess {
 static void automatic(Manager& m){m.checkpointPeriodic();}
 static void staleAutomatic(Manager& m){m.saveCatalog(true);}
 static unsigned long long sequence(Manager& m){Lock lock(m.mutex);return m.catalogSequence;}
 static void stopping(Manager& m,bool value){Lock lock(m.mutex);m.stopping=value;}
 static std::function<void(const char*)>& checkpoint(Manager& m){return m.catalogCheckpoint;}
 static void transaction(Manager& m,bool value){Lock lock(m.mutex);m.catalogTransaction=value;}
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
  CatalogCheckpointTestAccess::stopping(m,false);CatalogCheckpointTestAccess::checkpoint(m)={};passed=true;
 }catch(const std::exception& e){error=e.what();}
 atomicText(root/L"results.json",Json{{"passed",passed},{"error",error},{"checks",checks}}.dump(2),false);
 std::cout<<checks.size()<<" checks; "<<(passed?"passed":error)<<"\n";return passed?0:1;
}
