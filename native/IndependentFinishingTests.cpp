#include "Core.hpp"
#include "PreparedTransferCommit.hpp"
#include "AuthenticationFixture.hpp"
#include "SiteLogins.hpp"
#include <future>
#include <iostream>
namespace udm {struct CatalogCheckpointTestAccess {
 static void observe(Manager& m,std::function<void(const char*)> callback){Lock lock(m.mutex);m.catalogCheckpoint=std::move(callback);}
 static std::shared_ptr<Cancel> worker(Manager& m,const JobPtr& job){Lock lock(m.mutex);auto found=m.active.find(job->id());return found==m.active.end()?nullptr:found->second;}
 static bool independent(Manager& m,const JobPtr& job){Lock lock(m.mutex);return PreparedTransferCommit::independentlyFinishing(m,job,m.active);}
};}
using namespace udm;
template<class F>static bool until(F ready){const auto end=GetTickCount64()+10000;while(!ready()&&GetTickCount64()<end)Sleep(1);return ready();}
struct FinishGate {
 std::promise<void> entered,release;std::shared_future<void> ready=entered.get_future().share(),released=release.get_future().share();std::atomic_bool used=false,unlocked=false,expired=false;
 void observe(const char* point){if(std::string(point)=="publication-preflight-ready"&&!used.exchange(true)){entered.set_value();if(released.wait_for(std::chrono::seconds(8))!=std::future_status::ready)expired=true;}}
 void open(){if(!unlocked.exchange(true))release.set_value();}~FinishGate(){open();}
};
int wmain(int argc,wchar_t** argv){
 if(argc!=2)return 2;const auto root=fs::absolute(argv[1]);if(fs::exists(root))return 2;fs::create_directories(root);Json checks=Json::array();std::string error;
 const auto check=[&](bool ok,const char* name){checks.push_back({{"name",name},{"passed",ok}});if(!ok)throw std::runtime_error(name);};
 try{
  WSADATA winsock{};if(WSAStartup(MAKEWORD(2,2),&winsock))throw std::runtime_error("Cannot initialize owned publication origin");struct Sockets{~Sockets(){WSACleanup();}} sockets;AuthenticationFixture origin;Manager manager(root/L"catalog");
  manager.state["Settings"]["CategoryFolders"]=false;manager.state["Settings"]["Connections"]=1;manager.state["Settings"]["Retries"]=0;manager.state["Settings"]["DownloadFolder"]=utf8((root/L"files").wstring());Headers headers;setBasicLogin(headers,"u","p:2");
  for(int variant=0;variant<3;++variant){
   {Lock lock(manager.mutex);auto& queue=manager.state["Queues"][0];queue["Enabled"]=variant!=0;queue["Scheduled"]=variant==1;queue["Days"]=variant==1?0:127;}
   auto job=manager.add(origin.url("/basic"),"","finishing-"+std::to_string(variant)+".bin","Main queue",true,headers);FinishGate gate;
   struct Clear{Manager& manager;FinishGate& gate;~Clear(){gate.open();if(std::uncaught_exceptions())try{manager.stop();}catch(...){}CatalogCheckpointTestAccess::observe(manager,{});}} clear{manager,gate};CatalogCheckpointTestAccess::observe(manager,[&](const char* point){gate.observe(point);});
   manager.resume(job);manager.dispatchQueueWork();check(gate.ready.wait_for(std::chrono::seconds(4))==std::future_status::ready,"A real ordinary HTTP transfer reaches held publication preflight");
   auto worker=CatalogCheckpointTestAccess::worker(manager,job);check(worker&&!worker->cancelled()&&CatalogCheckpointTestAccess::independent(manager,job),"The exact active worker owns independent finishing before publication");
   {Lock lock(manager.mutex);check(!yes(job->data,"IndividualStart")&&str(job->data,"Status")=="Verifying"&&num(job->data,"Received")==static_cast<i64>(origin.payload.size()),"The durable flag is retired while the complete body is still unpublished");}
   check(!fs::exists(job->target()),"Held preflight has not published the destination prematurely");
   for(int i=0;i<8;++i)manager.tick();
   check(!gate.expired&&!worker->cancelled()&&CatalogCheckpointTestAccess::independent(manager,job),"Actual scheduler ticks cannot pause a manual finisher because its queue is stopped or outside its window");
   {Lock lock(manager.mutex);check(str(job->data,"Status")=="Verifying","Stopped-queue scheduling retains the finishing state instead of writing Pausing");}
   if(variant==2){manager.pause(job);check(worker->cancelled(),"An explicit user Pause still cancels the protected finisher");}
   gate.open();check(until([&]{manager.tick();return !manager.isActive(job);}),"The publication worker settles and releases its active generation");
   if(variant==2){check(str(job->data,"Status")=="Paused"&&!fs::exists(job->target()),"Explicit Pause keeps the unconfirmed publication unpublished and recoverable");}
   else{check(str(job->data,"Status")=="Complete"&&readText(job->target(),origin.payload.size()+1)==origin.payload,"After gate release the exact complete body is published despite stopped queue policy");}
   check(!CatalogCheckpointTestAccess::independent(manager,job),"An inactive generation cannot retain independent finishing authority");
   check(!job->data.contains("IndependentFinishing")&&!manager.snapshot().contains("IndependentFinishing"),"Runtime finishing ownership adds no catalog field or persistence write");
  }
  manager.stop();
 }catch(const std::exception& e){error=e.what();}
 atomicText(root/L"results.json",Json{{"passed",error.empty()},{"checks",checks},{"error",error}}.dump(2),false);std::cout<<checks.size()<<" checks; "<<(error.empty()?"passed":error)<<"\n";return error.empty()?0:1;
}
