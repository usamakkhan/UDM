#include "Core.hpp"
#include "ModelMaintenance.hpp"
#include "CliCompletion.hpp"
#include <future>
#include <iostream>
namespace udm {struct CatalogCheckpointTestAccess {static int ticks(Manager& manager){Lock lock(manager.mutex);return manager.ticks;}};}
using namespace udm;
template<class Predicate>static bool until(Predicate ready){const auto end=GetTickCount64()+3000;while(!ready()&&GetTickCount64()<end)Sleep(1);return ready();}
struct Release {std::promise<void>& promise;~Release(){try{promise.set_value();}catch(...) {}}};
int wmain(int argc,wchar_t** argv){
 if(argc!=2)return 2;const auto root=fs::absolute(argv[1]);if(fs::exists(root))return 2;fs::create_directories(root);Json checks=Json::array();std::string error;
 auto check=[&](bool ok,const char* name){checks.push_back({{"name",name},{"passed",ok}});if(!ok)throw std::runtime_error(name);};
 try{
  {
   std::promise<void> entered,release;auto ready=entered.get_future();auto released=release.get_future().share();std::atomic_uint ticks=0,dispatches=0,notifications=0,active=0,peak=0;std::set<std::thread::id> threads;const auto owner=std::this_thread::get_id();
   const auto enter=[&]{const auto n=++active;peak=std::max(peak.load(),n);threads.insert(std::this_thread::get_id());};
   ModelMaintenance worker([&]{enter();if(++ticks==1){entered.set_value();released.wait();}--active;},[&]{enter();++dispatches;--active;},[&]{++notifications;});Release cleanup{release};
   check(worker.requestTick()&&ready.wait_for(std::chrono::seconds(3))==std::future_status::ready,"The first request starts one owned background callback");
   for(int i=0;i<1000;++i){worker.requestTick();worker.requestDispatch();}
   const auto held=worker.takeResult();check(held.active&&held.tickPending&&held.dispatchPending&&held.ticks==0,"Request storms coalesce into two pending kinds while work is held");
   check(ticks==1&&dispatches==0,"Additional requests never run concurrently with held work");bool refusedRestart=false;try{worker.restart();}catch(const std::logic_error&){refusedRestart=true;}check(refusedRestart,"Restart cannot replace an unjoined active worker");
   release.set_value();check(until([&]{return notifications==3;}),"One follow-up tick and one dispatch finish after the held callback");worker.stop();
   const auto done=worker.takeResult();check(done.ticks==2&&done.dispatches==1&&ticks==2&&dispatches==1,"Coalesced work executes exactly once per pending kind");
   check(peak==1&&threads.size()==1&&!threads.count(owner),"Ticks and dispatches share one worker distinct from the owner thread");
   check(!worker.requestTick()&&!worker.requestDispatch(),"Stopped maintenance refuses new work");
  }
  {
   std::promise<void> entered,release;auto ready=entered.get_future();auto released=release.get_future().share();std::atomic_uint dispatches=0;
   ModelMaintenance worker([&]{entered.set_value();released.wait();},[&]{++dispatches;});std::future<void> stopped;Release cleanup{release};
   worker.requestTick();check(ready.wait_for(std::chrono::seconds(3))==std::future_status::ready,"Shutdown fixture holds an actual running callback");worker.requestTick();worker.requestDispatch();
   stopped=std::async(std::launch::async,[&]{worker.stop();});check(until([&]{return worker.takeResult().stopping;})&&stopped.wait_for(std::chrono::milliseconds(20))==std::future_status::timeout,"Stop enters closing and joins before its held callback can return");
   check(!worker.requestDispatch(),"Closing rejects requests before the join has finished");release.set_value();stopped.get();
   const auto joined=worker.takeResult();check(joined.ticks==1&&joined.dispatches==0&&!joined.active&&!joined.tickPending&&!joined.dispatchPending,"Shutdown discards unstarted maintenance while joining all active work");
   worker.restart();check(worker.requestDispatch()&&until([&]{return dispatches==1;}),"A failed owner shutdown can restart maintenance on the same live model");worker.stop();
   check(worker.takeResult().dispatches==1,"Restart retains completion accounting without replaying discarded requests");
  }
  {
   std::atomic_uint notifications=0;ModelMaintenance worker([]{throw std::runtime_error(std::string(5000,'x'));},[]{throw 7;},[&]{++notifications;throw std::runtime_error("owned wake failure");});
   worker.requestTick();worker.requestDispatch();check(until([&]{return notifications>=2;}),"Action and wake-up exceptions do not terminate the owned worker");
   auto failed=worker.takeResult();check(failed.tickFailures==1&&failed.dispatchFailures==1&&failed.tickError.size()==4096&&failed.dispatchError=="Model maintenance failed.","Typed and unknown failures are counted and retain bounded UI messages");
   auto consumed=worker.takeResult();check(consumed.tickError.empty()&&consumed.dispatchError.empty()&&consumed.tickFailures==1,"Taking error messages does not erase cumulative failure accounting");
   worker.requestTick();check(until([&]{return notifications>=3;}),"A later request still runs after action and notification failure");worker.stop();check(worker.takeResult().ticks==2,"The owner can join a worker that has reported failures");
  }
  {
   Manager manager(root/L"catalog");auto pending=std::make_shared<Job>(Json{{"Id",guid()},{"FileName","owned-pending.bin"},{"Folder",utf8(root.wstring())},{"Status","Paused"},{"QueueMember",false}});manager.jobs.push_back(pending);manager.cliHangups.push_back(pending);const auto before=CatalogCheckpointTestAccess::ticks(manager);std::promise<void> entered;auto ready=entered.get_future();std::atomic_bool called=false;
   ModelMaintenance worker([&]{called=true;entered.set_value();manager.tick();},[&]{manager.dispatchQueueWork();});std::future<JobPtr> hangup;
   {std::unique_lock lock(manager.mutex);check(worker.requestTick()&&ready.wait_for(std::chrono::seconds(3))==std::future_status::ready,"Real Manager maintenance is scheduled while another thread owns its catalog");
    check(called&&worker.takeResult().active&&worker.takeResult().ticks==0,"The callback can wait for Manager without blocking its request owner");
    worker.requestDispatch();check(worker.takeResult().dispatchPending,"A queue wake is retained while the real Manager is busy");
    hangup=std::async(std::launch::async,[&]{return takeCliHangup(manager,false);});check(hangup.wait_for(std::chrono::seconds(1))==std::future_status::ready&&!hangup.get(),"Advisory CLI hangup observation returns without acquiring a busy catalog");check(manager.cliHangups==std::vector<JobPtr>{pending},"Busy hangup observation leaves the pending CLI intent in place");
   }
   check(until([&]{auto result=worker.takeResult();return result.ticks==1&&result.dispatches==1&&!result.active;}),"After catalog release both real maintenance operations complete");worker.stop();
   check(CatalogCheckpointTestAccess::ticks(manager)==before+1,"The background tick advances the real Manager exactly once");manager.stop();
   check(!worker.requestTick(),"No callback can access the stopped Manager after owner teardown");
  }
 }catch(const std::exception& e){error=e.what();}
 atomicText(root/L"results.json",Json{{"passed",error.empty()},{"checks",checks},{"error",error}}.dump(2),false);std::cout<<checks.size()<<" checks; "<<(error.empty()?"passed":error)<<"\n";return error.empty()?0:1;
}
