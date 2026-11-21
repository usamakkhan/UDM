#include "Core.hpp"
#include <fstream>
#include <future>
#include <iostream>
namespace udm {struct CatalogCheckpointTestAccess {static void observe(Manager& m,std::function<void(const char*)> f){Lock lock(m.mutex);m.catalogCheckpoint=std::move(f);}};}
using namespace udm;
int wmain(int argc,wchar_t** argv){
 if(argc!=4)return 2;const auto area=fs::absolute(argv[1]);const auto expectation=utf8(argv[3]);
 if(expectation!="ordered"&&expectation!="legacy"&&expectation!="either"&&expectation!="held-change")return 3;
 if(fs::exists(area))return 4;fs::create_directories(area);atomicText(area/L".udm-response-control","owned response-policy qualification",false);
 Json result,checks=Json::array();std::string error;bool passed=false;
 auto check=[&](bool value,const char* name){checks.push_back({{"name",name},{"passed",value}});if(!value)throw std::runtime_error(name);};
 try{
  struct Gate {std::atomic_bool armed{false},held{false},expired{false};std::atomic<DWORD> saver{0};std::mutex mutex;std::condition_variable changed;bool released=false;void release(){std::lock_guard<std::mutex> lock(mutex);released=true;changed.notify_all();}} gate;
  Manager manager(area/L"catalog");{Lock lock(manager.mutex);auto& prefs=manager.state["Settings"];prefs["Connections"]=8;prefs["Parallel"]=1;prefs["SuppressProgressDialog"]=true;prefs["SuppressCompletionDialog"]=true;prefs["Sound"]=false;prefs["ClipboardMonitor"]=false;prefs["PrefetchFileInfo"]=false;prefs["CategoryFolders"]=false;prefs["UseHttp3"]=false;manager.state["Queues"][0]["Retries"]=3;}
  auto job=manager.add(utf8(argv[2]),utf8((area/L"files").wstring()),"response-control.bin","Main queue",true);manager.resume(job);const auto started=GetTickCount64();
  std::future<void> saving;bool saveStarted=false,gateResolved=false;
  try{
   if(expectation=="held-change")CatalogCheckpointTestAccess::observe(manager,[&](const char* stage){if(std::string_view(stage)!="snapshot-prepared"||GetCurrentThreadId()!=gate.saver||!gate.armed.exchange(false))return;atomicText(area/L"old-checkpoint-held.flag","detached old-generation snapshot",false);gate.held=true;std::unique_lock<std::mutex> lock(gate.mutex);if(!gate.changed.wait_for(lock,std::chrono::seconds(15),[&]{return gate.released;}))gate.expired=true;});
   for(;;){
    manager.tick();std::string status;bool active;Json live;{Lock lock(manager.mutex);live=job->snapshot();status=str(live,"Status");active=manager.isActive(job);}
    if(expectation=="held-change"&&!saveStarted&&num(live,"Received")>=524288){saveStarted=true;saving=std::async(std::launch::async,[&]{gate.saver=GetCurrentThreadId();gate.armed=true;manager.save();});}
    if(expectation=="held-change"&&gate.held&&!gateResolved&&yes(live.value("OrderedBodyWrites",Json()),"Paused")){
     check(saving.valid()&&saving.wait_for(std::chrono::milliseconds(0))==std::future_status::timeout&&!gate.expired,"The actual old-generation detached save is still held during representation replacement");
     const auto oldTag=str(live,"ETag");const auto part=manager.root/L"parts"/wide(job->id())/(wide(job->id())+L".data");
     std::ifstream input(part,std::ios::binary);Bytes prefix(64);input.read(reinterpret_cast<char*>(prefix.data()),64);check(input.gcount()==64,"The old parts file remains readable while retirement waits");result["oldPrefixBeforeRelease"]=b64(prefix);
     Sleep(40);auto view=manager.snapshot();check(str(view["Downloads"][0],"ETag")==oldTag&&oldTag=="\"udm-response-v1\""&&fs::exists(part)&&!fs::exists(job->target())&&!manager.completionSaved(job),"Reset and publication cannot overtake a captured old-generation checkpoint");
     gate.release();saving.get();gateResolved=true;CatalogCheckpointTestAccess::observe(manager,{});
    }
    if(!active&&(status=="Complete"||status=="Failed"))break;if(GetTickCount64()-started>90000)throw std::runtime_error("Response-policy fixture deadline exceeded");Sleep(5);
   }
   gate.release();if(saving.valid())saving.get();CatalogCheckpointTestAccess::observe(manager,{});
   if(expectation=="held-change")check(gateResolved&&!gate.expired,"The held checkpoint settled before the new representation completed");
  }catch(...){gate.release();if(saving.valid())try{saving.get();}catch(...){}CatalogCheckpointTestAccess::observe(manager,{});try{manager.stop();}catch(...){}throw;}
  manager.stop();result["job"]=job->snapshot();result["catalog"]=Json::parse(readText(manager.root/L"state.json"));result["normalStopJoined"]=true;
  check(str(job->data,"Status")=="Complete","The actual Manager pipeline completes the selected response policy");
  check(manager.completionSaved(job),"Completion is acknowledged by the actual final catalog checkpoint");
  check(num(job->data,"Received")==num(job->data,"Size"),"Published completion has exact complete byte counters");
  const auto found=job->data.find("OrderedBodyWrites");const bool ordered=found!=job->data.end()&&yes(*found,"Used");
  if(expectation=="ordered")check(ordered&&yes(*found,"Complete"),"The range-enabled private candidate joins ordered writes");
  if(expectation=="legacy")check(!ordered,"The unsupported response retains the legacy synchronous fallback");
  result["target"]=utf8(job->target().wstring());result["fileBytes"]=fs::file_size(job->target());result["fileSha256"]=fileHash(job->target());result["orderedUsed"]=ordered;
  check(result["catalog"]["Downloads"][0]["Sha256"]==job->data["Sha256"],"The actual saved catalog preserves the completed digest");passed=true;
 }catch(const std::exception& e){error=e.what();}
 result["passed"]=passed;result["error"]=error;result["checks"]=checks;result["expectation"]=expectation;result["qualificationFeatureDefine"]="UDM_ORDERED_BODY_QUALIFICATION";result["performanceComparison"]=false;atomicText(area/L"result.json",result.dump(2),false);std::cout<<checks.size()<<" checks; "<<(passed?"passed":error)<<"\n";return passed?0:1;
}
