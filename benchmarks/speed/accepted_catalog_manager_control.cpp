#include "Core.hpp"
#include <iostream>
namespace udm {
struct CatalogCheckpointTestAccess {
 static void observe(Manager& manager,std::function<void(const char*)> callback){
  Lock lock(manager.mutex);manager.catalogCheckpoint=std::move(callback);
 }
};
}
using namespace udm;

int wmain(int argc,wchar_t** argv){
 if(argc!=2)return 2;const auto root=fs::absolute(argv[1]);if(fs::exists(root))return 3;
 fs::create_directories(root);atomicText(root/L".udm-accepted-manager-control","owned actual Manager stop comparison",false);
 Json result,measurements=Json::array();std::string error;
 try{
  unsigned writes=0;
  auto manager=std::make_unique<Manager>(root/L"catalog");
  for(int index=0;index<7500;++index){
   const auto digits=std::to_string(index);const auto id=std::string(32-digits.size(),'0')+digits;
   manager->jobs.push_back(std::make_shared<Job>(Json{
    {"Id",id},{"Url","http://127.0.0.1/fixture/"+digits},{"FileName","record-"+digits+".bin"},
    {"Folder",utf8((root/L"files").wstring())},{"Status","Paused"},{"Size",67108864},
    {"Received",0},{"Queue","Main queue"},{"QueueMember",false},{"Segments",Json::array()}}));
  }
  CatalogCheckpointTestAccess::observe(*manager,[&](const char* stage){if(std::string_view(stage)=="temporary-flushed")++writes;});
  manager->save();const auto state=manager->root/L"state.json";
  const auto digest=fileHash(state);const auto bytes=fs::file_size(state);const auto initialWrites=writes;
  manager->stop(); // Common warmup, including the first stopping transition.
  for(int trial=0;trial<10;++trial){
   ULONG64 before=0,after=0;if(!QueryThreadCycleTime(GetCurrentThread(),&before))throw std::runtime_error("Cannot read thread cycles");
   const auto began=std::chrono::steady_clock::now();manager->stop();
   const auto elapsed=std::chrono::duration<double>(std::chrono::steady_clock::now()-began).count();
   if(!QueryThreadCycleTime(GetCurrentThread(),&after))throw std::runtime_error("Cannot read thread cycles");
   measurements.push_back({{"trial",trial+1},{"seconds",elapsed},{"threadCycles",after-before}});
  }
  result["stopWrites"]=writes-initialWrites;
  if(writes!=initialWrites||fileHash(state)!=digest)throw std::runtime_error("Stop failed to reuse the exact accepted checkpoint");
  const auto began=std::chrono::steady_clock::now();manager.reset();
  result["destructorSeconds"]=std::chrono::duration<double>(std::chrono::steady_clock::now()-began).count();
  if(writes!=initialWrites||fileHash(state)!=digest)throw std::runtime_error("Destructor rewrote or changed an accepted catalog");
  result["records"]=7500;result["catalogBytes"]=bytes;result["catalogSha256"]=digest;
  result["destructorWrites"]=writes-initialWrites;
 }catch(const std::exception& exception){error=exception.what();}
 result["passed"]=error.empty();result["error"]=error;result["measurements"]=measurements;
 result["scope"]="Actual Manager stop with 7,500 paused records; warm accepted catalog; includes live snapshot and proof; no active transfers";
 result["completeApplicationTiming"]=false;result["pairedIdmComparison"]=false;
 atomicText(root/L"result.json",result.dump(2),false);
 std::cout<<measurements.size()<<" stop calls; "<<(error.empty()?"passed":error)<<"\n";
 return error.empty()?0:1;
}
