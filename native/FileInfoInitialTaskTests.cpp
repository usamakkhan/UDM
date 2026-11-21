#include "FileInfoInitialTask.hpp"
#include <iostream>
using namespace udm;
template<class F>static bool until(F f){const auto end=GetTickCount64()+6000;while(!f()&&GetTickCount64()<end)Sleep(1);return f();}
int wmain(int argc,wchar_t** argv){
 if(argc!=2)return 2;const auto root=fs::absolute(argv[1]);if(fs::exists(root))return 2;fs::create_directories(root);Json checks=Json::array();std::string error;
 const auto check=[&](bool ok,const char* name){checks.push_back({{"name",name},{"passed",ok}});if(!ok)throw std::runtime_error(name);};
 try{
  Manager manager(root/L"catalog");auto job=manager.add("http://127.0.0.1:1/file.zip",utf8((root/L"files").native()),"file.zip","Main queue",true,{{"Authorization","Basic dTpw"}});
  job->data["Size"]=1234;const auto prepared=captureFileInfoInitialView(manager,*job);const auto before=manager.snapshot();
  {
   FileInfoInitialTask task;std::atomic_uint notices=0;task.start(manager,job,prepared,[&]{++notices;});check(until([&]{return task.ready();}),"Initial preparation settles on an owned reader");auto result=task.take();
   check(result.view==prepared&&result.address=="http://127.0.0.1:1/file.zip"&&result.basic&&result.login==std::make_pair(std::string("u"),std::string("p")),"A coherent render cache and its private address and credentials are prepared together");
   check(result.view->queues.size()>=2&&!result.view->queueLabels.empty()&&num(result.view->data,"Size")==1234,"The initial view includes exact metadata, queue names and display labels");
   check(manager.snapshot()==before&&!task.valid(),"Preparing the dialog never admits a transfer, mutates history or consumes the offer");
  }
  {
   job->data["Size"]=9876;FileInfoInitialTask task;task.start(manager,job,prepared);check(until([&]{return task.ready();}),"An obsolete render cache is refreshed asynchronously");auto result=task.take();check(result.view!=prepared&&num(result.view->data,"Size")==9876,"A changed size replaces the stale render snapshot before controls become editable");
  }
  {
   std::promise<void> release,entered;auto gate=release.get_future().share();auto ready=entered.get_future();std::atomic_bool held=false,waiting=false;
   std::thread writer([&]{Lock lock(manager.mutex);held=true;entered.set_value();gate.wait();held=false;});
   struct Join{std::promise<void>& release;std::thread& writer;~Join(){try{release.set_value();}catch(...){}if(writer.joinable())writer.join();}} join{release,writer};FileInfoInitialTask task;
   check(ready.wait_for(std::chrono::seconds(3))==std::future_status::ready,"A real catalog writer holds Manager before cold preparation starts");task.start(manager,job,{}, {},[&](const char* stage){if(std::string(stage)=="waiting-model")waiting=true;});
   check(until([&]{return waiting.load();})&&held&&!task.ready(),"Requesting cold File Info returns while its reader waits for the real model");
   bool duplicateRejected=false;try{task.start(manager,job);}catch(const std::exception&){duplicateRejected=true;}check(duplicateRejected,"One initial result cannot be overwritten by another request");
   auto stopped=std::async(std::launch::async,[&]{task.finish();});const auto result=stopped.wait_for(std::chrono::seconds(1));release.set_value();writer.join();check(result==std::future_status::ready,"Cancelling and joining a model-waiting reader completes before that external writer must be released");stopped.get();check(!task.valid(),"Cancelled initial preparation retains no worker or result");
  }
  {
   FileInfoInitialTask task;manager.fileInfoDecisionPending=true;std::atomic_bool waiting=false;task.start(manager,job,{}, {},[&](const char* stage){if(std::string(stage)=="waiting-model")waiting=true;});check(until([&]{return waiting.load();})&&!task.ready(),"An outstanding decision defers the initial view without spinning under Manager");manager.fileInfoDecisionPending=false;check(until([&]{return task.ready();})&&task.take().view,"The same owned reader resumes after decision acknowledgement");
  }
  {
   const auto url="http://127.0.0.1:1/form";auto form=manager.add(url,utf8((root/L"files").native()),"form.udmform","Main queue",true,{},"",Json{{"method","POST"},{"url",url},{"contentType","application/octet-stream"},{"body","eA=="}});
   FileInfoInitialTask task;task.start(manager,form);check(until([&]{return task.ready();}),"A real protected POST request is prepared on the same reader");auto result=task.take();check(result.view->formRequest&&!result.view->media&&!result.view->data.contains("ProtectedRequest"),"The form layout is preserved without exposing its protected request body in render data");
  }
  {
   auto removed=manager.add("http://127.0.0.1:1/removed",utf8((root/L"files").native()),"removed.bin","Main queue",true);manager.jobs.erase(std::remove(manager.jobs.begin(),manager.jobs.end(),removed),manager.jobs.end());FileInfoInitialTask task;task.start(manager,removed);check(until([&]{return task.ready();}),"A removed download settles its initial read");bool rejected=false;try{task.take();}catch(const std::exception&){rejected=true;}check(rejected&&!task.valid(),"An absent history member cannot become an editable initial dialog");
  }
  {
   FileInfoInitialTask task;task.start(manager,job,{}, {},[](const char* stage){if(std::string(stage)=="before-read")throw std::runtime_error("Owned initial read failure");});check(until([&]{return task.ready();}),"An initial-reader exception settles on its owner");bool failed=false;try{task.take();}catch(const std::exception& e){failed=std::string(e.what())=="Owned initial read failure";}check(failed&&!task.valid(),"Initial errors are consumed without admitting or cancelling the user's download");
   task.start(manager,job);check(until([&]{return task.ready();})&&task.take().view,"The same owner can Retry after a consumed initial-read failure");
  }
  manager.stop();
 }catch(const std::exception& e){error=e.what();}
 atomicText(root/L"results.json",Json{{"passed",error.empty()},{"checks",checks},{"error",error}}.dump(2),false);std::cout<<checks.size()<<" checks; "<<(error.empty()?"passed":error)<<"\n";return error.empty()?0:1;
}
