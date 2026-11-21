#include "QueueChoiceTask.hpp"
#include <iostream>
using namespace udm;
template<class F>static bool until(F f){const auto end=GetTickCount64()+5000;while(!f()&&GetTickCount64()<end)Sleep(1);return f();}
int wmain(int argc,wchar_t** argv){
 if(argc!=2)return 2;const auto root=fs::absolute(argv[1]);if(fs::exists(root))return 2;fs::create_directories(root);Json checks=Json::array();std::string error;
 const auto check=[&](bool ok,const char* name){checks.push_back({{"name",name},{"passed",ok}});if(!ok)throw std::runtime_error(name);};
 try{
  Manager manager(root/L"catalog");manager.state["Queues"]=Json::array({Json{{"Name","Stored default"},{"DefaultQueueRole","Download"}},Json{{"Name","Main download queue"}},Json{{"Name","Other"}}});manager.save();
  const auto state=manager.snapshot();const auto disk=readText(manager.root/L"state.json");QueueChoiceTask task;
  const auto read=[&](QueueChoiceObserver observer={}){task.start(manager,{},std::move(observer));if(!until([&]{return task.ready();}))throw std::runtime_error("Queue read deadline");return task.take();};
  auto first=read();check(!first.cancelled&&first.error.empty(),"Queue preparation publishes a successful owned result");
  check(first.view.names==std::vector<std::string>{"Stored default","Main download queue","Other"},"Stored queue identities retain catalog order");
  check(first.view.labels==queueDisplayLabels(state["Queues"])&&first.view.labels.at("Main download queue")=="Main download queue (custom)","One cohort preserves localized-role labels and custom-name collision resolution");
  check(manager.snapshot()==state&&readText(manager.root/L"state.json")==disk&&!manager.fileInfoDecisionPending,"Preparing choices never writes catalog or acquires accepted-command ownership");
  manager.state["Queues"][2]["Name"]="New name";auto fresh=read();check(fresh.view.names.back()=="New name"&&first.view.names.back()=="Other","Each read sees current queues without mutating an earlier snapshot");
  {
   std::promise<void> release;auto gate=release.get_future().share();std::atomic_bool held=false,waiting=false;std::thread writer([&]{Lock lock(manager.mutex);held=true;gate.wait();});struct Join{std::promise<void>& release;std::thread& worker;~Join(){try{release.set_value();}catch(...){}if(worker.joinable())worker.join();}} joined{release,writer};
   check(until([&]{return held.load();}),"A real catalog writer precedes selector preparation");task.start(manager,{},[&](const char* stage){if(std::string(stage)=="waiting-model")waiting=true;});check(until([&]{return waiting.load();})&&!task.ready()&&held,"Preparation returns while the catalog remains locked");
   bool duplicate=false;try{task.start(manager);}catch(...){duplicate=true;}check(duplicate,"One selector cannot start duplicate queue readers");task.cancelRead();check(until([&]{return task.ready();})&&task.take().cancelled&&held,"Cancellation joins without waiting for the catalog writer");release.set_value();writer.join();
  }
  manager.fileInfoDecisionPending=true;task.start(manager);Sleep(25);check(!task.ready(),"Another accepted command defers the advisory reader");task.finish();check(!task.valid()&&manager.fileInfoDecisionPending,"Teardown cancels acquisition without clearing another command guard");manager.fileInfoDecisionPending=false;
  auto failed=read([](const char* stage){if(std::string(stage)=="before-read")throw std::runtime_error("Owned queue read failure");});check(failed.error=="Owned queue read failure"&&!task.valid(),"Read failure is reported and its worker is consumed");check(read().error.empty(),"The same selector owner can retry after a read failure");
  std::atomic_bool notified=false;task.start(manager,[&]{notified=true;throw std::runtime_error("Owned wake failure");});check(until([&]{return notified.load();})&&task.ready()&&task.take().error.empty(),"A throwing notification cannot invalidate the published queue cohort");
  manager.state["Queues"]=Json::array();auto empty=read();check(empty.error.empty()&&empty.view.names.empty()&&empty.view.labels.empty(),"An empty catalog produces an empty selectable cohort without fabrication");task.finish();task.finish();check(!task.valid()&&!manager.fileInfoDecisionPending,"Repeated cleanup leaves no reader or accepted-command ownership");manager.stop();
 }catch(const std::exception& failure){error=failure.what();}
 atomicText(root/L"results.json",Json{{"passed",error.empty()},{"checks",checks},{"error",error}}.dump(2),false);std::cout<<checks.size()<<" checks; "<<(error.empty()?"passed":error)<<"\n";return error.empty()?0:1;
}
