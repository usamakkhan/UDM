#include "CompletionClaimTask.hpp"
#include <iostream>
namespace udm {struct CatalogCheckpointTestAccess {static std::function<void(const char*)>& checkpoint(Manager& m){return m.catalogCheckpoint;}};}
using namespace udm;
template<class F>static bool until(F test){const auto end=GetTickCount64()+5000;while(!test()&&GetTickCount64()<end)Sleep(1);return test();}
struct ClaimWriter {
 std::promise<void> release;std::thread thread;std::atomic_bool held=false;bool opened=false;
 explicit ClaimWriter(Manager& m){auto gate=release.get_future().share();thread=std::thread([this,&m,gate]{Lock lock(m.mutex);held=true;gate.wait();});if(!until([&]{return held.load();}))throw std::runtime_error("Writer deadline");}
 void open(){if(!opened){opened=true;release.set_value();}if(thread.joinable())thread.join();}~ClaimWriter(){open();}
};
int wmain(int argc,wchar_t** argv){
 if(argc!=2)return 2;const auto root=fs::absolute(argv[1]);if(fs::exists(root))return 2;fs::create_directories(root);
 Json checks=Json::array();std::string error;
 auto check=[&](bool value,const char* name){checks.push_back({{"name",name},{"passed",value}});if(!value)throw std::runtime_error(name);};
 try{
  Manager manager(root/L"catalog");CompletionClaimTask task;
  const auto job=manager.add("https://example.invalid/claim","","owned.bin","Main queue",true);
  auto arm=[&]{job->data["Status"]="Paused";manager.setCompletionPlan(job,{"Open downloaded file"},false,15,false);job->data["Status"]="Complete";manager.save();};
  auto run=[&](JobPtr target,std::function<void(const char*)> observe={},std::function<void()> notify={}){task.start(manager,std::move(target),std::move(notify),std::move(observe));if(!until([&]{return task.ready();}))throw std::runtime_error("Claim deadline");return task.take();};
  arm();
  {ClaimWriter writer(manager);std::atomic_bool waiting=false;const auto begun=GetTickCount64();task.start(manager,job,{},[&](const char* stage){if(std::string(stage)=="waiting-model")waiting=true;});
   check(GetTickCount64()-begun<250&&until([&]{return waiting.load();})&&writer.held&&!task.ready(),"Accepting a claim returns while the live catalog is held");
   bool duplicate=false;try{task.start(manager,job);}catch(...){duplicate=true;}check(duplicate&&task.valid(),"A duplicate start cannot overwrite the accepted claim");writer.open();
   check(until([&]{return task.ready();}),"The accepted claim becomes ready after the model owner releases");auto saved=task.take();
   check(saved.saved&&saved.error.empty()&&saved.action["Actions"]==Json::array({"Open downloaded file"}),"Only the durable claim returns the canonical action");
  }
  check(!yes(job->data,"CompletionActionArmed"),"The successful claim disarms the live action");
  {Manager reopened(manager.root);check(!yes(reopened.jobs.at(0)->data,"CompletionActionArmed"),"The disarm is present in the reopened durable catalog");}
  const auto before=readText(manager.root/L"state.json");unsigned writes=0;CatalogCheckpointTestAccess::checkpoint(manager)=[&](const char* phase){if(std::string(phase)=="temporary-flushed")++writes;};
  auto again=run(job);CatalogCheckpointTestAccess::checkpoint(manager)={};check(again.saved&&again.action.empty()&&writes==0&&readText(manager.root/L"state.json")==before,"A repeated claim has no action and adds no redundant checkpoint");
  arm();manager.fileInfoDecisionPending=true;std::atomic_bool waiting=false;task.start(manager,job,{},[&](const char* phase){if(std::string(phase)=="waiting-model")waiting=true;});
  check(until([&]{return waiting.load();})&&!task.ready()&&yes(job->data,"CompletionActionArmed"),"An accepted foreign decision retains priority over the new claim");manager.fileInfoDecisionPending=false;
  check(until([&]{return task.ready();})&&task.take().saved,"The claim resumes when the foreign decision is acknowledged");
  arm();job->data["ScanResult"]={{"Status","Pending"}};manager.save();auto skipped=run(job);check(skipped.saved&&skipped.action.empty()&&yes(job->data,"CompletionActionArmed"),"A newly pending scanner prevents a stale claim without disarming it");job->data.erase("ScanResult");
  arm();const auto snapshot=manager.snapshot();const auto disk=readText(manager.root/L"state.json");
  CatalogCheckpointTestAccess::checkpoint(manager)=[](const char* phase){if(std::string(phase)=="temporary-flushed")throw std::runtime_error("Owned claim checkpoint failure");};
  auto failed=run(job);CatalogCheckpointTestAccess::checkpoint(manager)={};
  check(!failed.saved&&failed.action.empty()&&failed.error=="Owned claim checkpoint failure","A checkpoint failure returns no executable action");
  check(manager.snapshot()==snapshot&&readText(manager.root/L"state.json")==disk,"Checkpoint failure restores the armed live job and exact durable catalog");
  auto retry=run(job);check(retry.saved&&!retry.action.empty()&&!yes(job->data,"CompletionActionArmed"),"The exact failed claim can be explicitly retried once");
  arm();failed=run(job,[](const char* phase){if(std::string(phase)=="before-claim")throw std::runtime_error("Owned claim preparation failure");});check(!failed.saved&&yes(job->data,"CompletionActionArmed"),"Preparation failure preserves the armed action for retry");
  bool unlocked=false;auto notified=run(job,{},[&]{std::thread probe([&]{std::unique_lock lock(manager.mutex,std::try_to_lock);unlocked=lock.owns_lock();});probe.join();throw std::runtime_error("Ignored wake failure");});check(notified.saved&&unlocked&&!task.valid(),"Result publication releases the model and survives a failed notification");
  arm();CompletionClaimTask other;task.start(manager,job);other.start(manager,job);check(until([&]{return task.ready()&&other.ready();}),"Independent competing claim owners both settle");auto a=task.take(),b=other.take();check(a.saved&&b.saved&&int(!a.action.empty())+int(!b.action.empty())==1,"Concurrent claims produce at most one executable action");
  arm();{ClaimWriter writer(manager);task.start(manager,job);std::thread release([&]{Sleep(30);writer.open();});auto final=task.finish();release.join();check(final.saved&&!final.action.empty()&&!task.valid(),"Owner shutdown joins its accepted claim instead of detaching or dropping it");}
  auto empty=run({});check(empty.saved&&empty.action.empty(),"A removed or absent target cannot produce an action");
  task.finish();task.finish();check(!task.valid(),"Repeated owner cleanup is safe");manager.stop();
 }catch(const std::exception& problem){error=problem.what();}
 atomicText(root/L"results.json",Json{{"passed",error.empty()},{"checks",checks},{"error",error}}.dump(2),false);std::cout<<checks.size()<<" checks; "<<(error.empty()?"passed":error)<<"\n";return error.empty()?0:1;
}
