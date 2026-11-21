#include "DownloadMenuTask.hpp"
#include <iostream>
using namespace udm;
template<class F>static bool until(F f){const auto end=GetTickCount64()+5000;while(!f()&&GetTickCount64()<end)Sleep(1);return f();}
int wmain(int argc,wchar_t** argv){
 if(argc!=2)return 2;const auto root=fs::absolute(argv[1]);if(fs::exists(root))return 2;fs::create_directories(root);Json checks=Json::array();std::string error;
 const auto check=[&](bool value,const char* name){checks.push_back({{"name",name},{"passed",value}});if(!value)throw std::runtime_error(name);};
 try{Manager manager(root/L"catalog");manager.save();const auto before=manager.snapshot();const auto disk=readText(manager.root/L"state.json");DownloadMenuTask task;const auto caller=std::this_thread::get_id();std::atomic_bool readOffThread=false;
  auto reader=[&]{readOffThread=std::this_thread::get_id()!=caller;DownloadMenuView view;view.queues=manager.state["Queues"];view.doubleClick=str(manager.state["Settings"],"CompletedDoubleClick","Properties");return view;};
  task.start(manager,reader);check(until([&]{return task.ready();}),"The owned menu reader publishes its model cohort");auto first=task.take();check(!first.cancelled&&first.error.empty()&&readOffThread,"Menu preparation reads on its owned worker rather than the caller");
  {Lock lock(manager.mutex);manager.state["Settings"]["CompletedDoubleClick"]="Open";}
  task.start(manager,reader);check(until([&]{return task.ready();}),"The same owner can prepare another menu");auto second=task.take();check(second.view.doubleClick=="Open"&&first.view.doubleClick=="Properties","New settings do not mutate a previously captured menu cohort");
  {Lock lock(manager.mutex);manager.state=before;}
  std::promise<void> release;auto gate=release.get_future().share();std::atomic_bool held=false;std::thread writer([&]{Lock lock(manager.mutex);held=true;gate.wait();});struct Join{std::promise<void>& release;std::thread& worker;~Join(){try{release.set_value();}catch(...){}if(worker.joinable())worker.join();}} joined{release,writer};check(until([&]{return held.load();}),"A separate catalog writer owns the model before menu preparation");
  task.start(manager,reader);Sleep(20);check(task.valid()&&!task.ready()&&held,"Menu start returns while its catalog acquisition is pending");bool duplicate=false;try{task.start(manager,reader);}catch(...){duplicate=true;}check(duplicate,"A pending menu owner rejects a duplicate start");
  task.cancelRead();check(until([&]{return task.ready();})&&task.take().cancelled&&held,"Menu cancellation settles without releasing another thread's catalog lock");
  task.start(manager,reader);task.finish();check(!task.valid()&&held,"Shutdown joins a waiting menu reader without acquiring Manager");release.set_value();writer.join();
  manager.fileInfoDecisionPending=true;task.start(manager,reader);Sleep(20);check(!task.ready(),"Accepted File Info work defers menu capture");task.finish();check(manager.fileInfoDecisionPending,"Menu cleanup never clears another command's decision ownership");manager.fileInfoDecisionPending=false;
  task.start(manager,[]{throw std::runtime_error("Owned menu capture failure");return DownloadMenuView{};});check(until([&]{return task.ready();})&&task.take().error=="Owned menu capture failure","Menu read failure is an owned consumable result");
  bool notifiedUnlocked=false;task.start(manager,reader,[&]{std::thread probe([&]{std::unique_lock lock(manager.mutex,std::try_to_lock);notifiedUnlocked=lock.owns_lock();});probe.join();throw std::runtime_error("Owned notification failure");});check(until([&]{return task.ready();}),"The menu owner can retry after a capture failure");auto retry=task.take();check(retry.error.empty()&&notifiedUnlocked,"Notification runs after releasing Manager and cannot undo its result");
  task.finish();task.finish();check(!task.valid(),"Repeated menu cleanup is safe");check(manager.snapshot()==before&&readText(manager.root/L"state.json")==disk,"Menu reads do not change settings jobs queues or catalog files");manager.stop();
 }catch(const std::exception& failed){error=failed.what();}
 atomicText(root/L"results.json",Json{{"passed",error.empty()},{"checks",checks},{"error",error}}.dump(2),false);std::cout<<checks.size()<<" checks; "<<(error.empty()?"passed":error)<<"\n";return error.empty()?0:1;
}
