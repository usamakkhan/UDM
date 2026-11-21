#include "FileInfoActionTask.hpp"
#include <iostream>
using namespace udm;
template<class F>static bool until(F f){const auto end=GetTickCount64()+5000;while(!f()&&GetTickCount64()<end)Sleep(1);return f();}
int wmain(int argc,wchar_t** argv){
 if(argc!=2)return 2;const auto root=fs::absolute(argv[1]);if(fs::exists(root))return 2;fs::create_directories(root);Json checks=Json::array();std::string error;
 const auto check=[&](bool ok,const char* name){checks.push_back({{"name",name},{"passed",ok}});if(!ok)throw std::runtime_error(name);};
 try{
  Manager manager(root/L"catalog");auto job=manager.add("https://files.example.test/file.bin",utf8((root/L"files").wstring()),"file.bin","Main queue",true);manager.state["Settings"]["QueuePromptLater"]=true;manager.save();
  FileInfoActionTask task;const auto read=[&](JobPtr target,FileInfoActionKind kind=FileInfoActionKind::Later,FileInfoActionObserver observe={}){task.start(manager,std::move(target),{},std::move(observe),kind);if(!until([&]{return task.ready();}))throw std::runtime_error("Action preparation did not settle");return task.take();};
  const auto before=manager.snapshot();const auto disk=readText(manager.root/L"state.json");auto initial=read(job);
  check(initial.error.empty()&&!initial.cancelled&&initial.promptLater&&!initial.refreshSession,"Current queue policy and session state are prepared together");
  check(initial.queues.names.size()==manager.state["Queues"].size()&&initial.queues.labels==queueDisplayLabels(manager.state["Queues"]),"Queue names and display labels come from the same current cohort");
  check(manager.snapshot()==before&&readText(manager.root/L"state.json")==disk&&!manager.fileInfoDecisionPending,"Advisory preparation changes no catalog, transfer or accepted-command guard");
  auto start=read(job,FileInfoActionKind::Start);check(start.queues.names.empty()&&start.queues.labels.empty(),"Start does not prepare an unused queue selector");manager.state["Settings"]["QueuePromptLater"]=false;auto quiet=read(job);check(!quiet.promptLater&&quiet.queues.names.empty()&&quiet.queues.labels.empty(),"A suppressed queue prompt does not prepare unused names and labels");
  job->data["SabrSessionRefreshRequired"]=true;auto rejected=read(job,FileInfoActionKind::Start);check(rejected.refreshSession&&!start.refreshSession,"A fresh Start preparation observes session rejection without changing an earlier result");job->data.erase("SabrSessionRefreshRequired");
  {
   std::promise<void> release;auto gate=release.get_future().share();std::atomic_bool held=false,waiting=false;std::thread writer([&]{Lock lock(manager.mutex);held=true;gate.wait();});struct Release{std::promise<void>& gate;std::thread& worker;~Release(){try{gate.set_value();}catch(...){}if(worker.joinable())worker.join();}} cleanup{release,writer};check(until([&]{return held.load();}),"A real writer precedes Start/Later preparation");task.start(manager,job,{},[&](const char* stage){if(std::string(stage)=="waiting-model")waiting=true;});check(until([&]{return waiting.load();})&&!task.ready()&&held&&!manager.fileInfoDecisionPending,"Preparing an action returns while Manager is held and claims no accepted guard");bool duplicate=false;try{task.start(manager,job);}catch(...){duplicate=true;}check(duplicate,"One File Info owner cannot duplicate its outstanding action reader");task.cancelRead();check(until([&]{return task.ready();})&&task.take().cancelled&&held,"Cancelling advisory acquisition settles before the external writer releases Manager");release.set_value();writer.join();
  }
  manager.fileInfoDecisionPending=true;task.start(manager,job);Sleep(25);check(!task.ready(),"An unrelated accepted command defers the action reader");task.cancelRead();check(until([&]{return task.ready();})&&task.take().cancelled&&manager.fileInfoDecisionPending,"Advisory cancellation never clears another command's guard");manager.fileInfoDecisionPending=false;
  auto failed=read(job,FileInfoActionKind::Start,[](const char* stage){if(std::string(stage)=="before-read")throw std::runtime_error("Owned action read failure");});check(failed.error=="Owned action read failure"&&!task.valid(),"A read failure is returned without retaining a worker or accepted command");check(read(job).error.empty(),"The same owner can retry after consuming a read failure");
  std::atomic_bool notified=false;task.start(manager,job,[&]{notified=true;throw std::runtime_error("wake failure");});check(until([&]{return notified.load();})&&task.ready()&&task.take().error.empty(),"A throwing wake cannot undo a published action result");
  check(read({}).error.find("no longer available")!=std::string::npos,"A missing download cannot authorize an action");manager.jobs.erase(std::remove(manager.jobs.begin(),manager.jobs.end(),job),manager.jobs.end());check(read(job).error.find("no longer available")!=std::string::npos,"A stale strong selection is checked against current catalog membership");
  manager.stop();
 }catch(const std::exception& failure){error=failure.what();}
 atomicText(root/L"results.json",Json{{"passed",error.empty()},{"checks",checks},{"error",error}}.dump(2),false);std::cout<<checks.size()<<" checks; "<<(error.empty()?"passed":error)<<"\n";return error.empty()?0:1;
}
