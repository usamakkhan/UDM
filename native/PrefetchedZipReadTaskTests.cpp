#include "PrefetchedZipReadTask.hpp"
#include <iostream>
#include <fstream>
using namespace udm;
template<class F>static bool until(F f){const auto end=GetTickCount64()+4000;while(!f()&&GetTickCount64()<end)Sleep(1);return f();}
int wmain(int argc,wchar_t** argv){
 if(argc!=2)return 2;const auto root=fs::absolute(argv[1]);if(fs::exists(root))return 2;fs::create_directories(root);Json checks=Json::array();std::string error;
 const auto check=[&](bool ok,const char* name){checks.push_back({{"name",name},{"passed",ok}});if(!ok)throw std::runtime_error(name);};
 try{
  Manager manager(root/L"catalog");manager.state["Settings"]["PrefetchFileInfo"]=false;auto job=manager.add("http://127.0.0.1:1/archive.zip",utf8((root/L"files").native()),"archive.zip","Main queue",true);const auto url=str(job->data,"Url"),headers=str(job->data,"ProtectedHeaders");
  struct ClearReservation{Manager& manager;JobPtr job;~ClearReservation(){Lock lock(manager.mutex);job->data.erase("OriginalResponse");}} clear{manager,job};
  const auto folder=manager.root/L"parts"/wide(job->id());fs::create_directories(folder);const auto path=folder/(wide(job->id())+L".data");{std::ofstream file(path,std::ios::binary);file<<"owned ZIP bytes";}const auto size=static_cast<i64>(fs::file_size(path));
  job->data["ConfirmationPending"]=true;job->data["Size"]=size;job->data["Received"]=size;job->data["CompletedPrefetch"]={{"Version",1},{"Url",url},{"Headers",headers},{"Size",size}};job->data["OriginalResponse"]={{"State","Reserved"}};const auto original=job->data;
  {
   const auto before=manager.snapshot();auto notices=std::make_shared<std::atomic_int>(0);PrefetchedZipReadTask task(manager,job,url,headers,{},[notices]{++*notices;});std::optional<fs::path> ready;check(until([&]{ready=task.poll();return ready.has_value();}),"A complete prefetched receipt settles on the owned reader");check(*notices==1,"A terminal prefetched file result wakes its owner exactly once");check(ready==path&&task.poll()==ready,"The exact existing file is returned and terminal polling is stable");check(manager.snapshot()==before,"ZIP readiness inspection never changes the model or starts another request");
  }
  {
   job->data.erase("CompletedPrefetch");std::atomic_bool waiting=false;PrefetchedZipReadTask task(manager,job,url,headers,[&](const char* stage){if(std::string(stage)=="waiting-receipt")waiting=true;});check(until([&]{return waiting.load();})&&!task.poll(),"An unfinished prefetch remains pending rather than starting a replacement request");{Lock lock(manager.mutex);job->data=original;}std::optional<fs::path> ready;check(until([&]{ready=task.poll();return ready.has_value();})&&ready==path,"The same reader observes a later complete receipt");
  }
  for(const std::string change:{"url","headers","proxy","removed","inactive","confirmation","capture-required","file-size","receipt-size","parts-location"}){
   job->data=original;job->data["OriginalResponse"]={{"State","Reserved"}};std::atomic_bool changed=false;
   PrefetchedZipReadTask task(manager,job,url,headers,[&](const char* stage){if(std::string(stage)!="before-file-check"||changed.exchange(true))return;Lock lock(manager.mutex);
    if(change=="url")job->data["Url"]="http://127.0.0.1:1/new.zip";if(change=="headers")job->data["ProtectedHeaders"]="changed";if(change=="proxy")job->data["ProtectedBrowserProxy"]="changed";if(change=="removed")manager.jobs.clear();if(change=="inactive")job->data.erase("OriginalResponse");if(change=="confirmation")job->data["ConfirmationPending"]=false;if(change=="capture-required")job->data["RequiresRequestCapture"]=true;if(change=="file-size"){std::ofstream file(path,std::ios::binary|std::ios::app);file<<"x";}if(change=="receipt-size")job->data["CompletedPrefetch"]["Size"]=size+1;if(change=="parts-location")job->data["PartsFolder"]=utf8((root/L"different-parts").native());
   });std::optional<fs::path> ready;check(until([&]{ready=task.poll();return ready.has_value();})&&ready->empty(),"A changed request, ownership, receipt or file is rejected after disk inspection");task.stop();if(manager.jobs.empty())manager.jobs.push_back(job);if(change=="file-size")fs::resize_file(path,size);
  }
  job->data=original;job->data["OriginalResponse"]={{"State","Reserved"}};
  {
   std::promise<void> entered,release;auto ready=entered.get_future();auto gate=release.get_future().share();std::atomic_bool held=false,waiting=false;
   std::thread writer([&]{Lock lock(manager.mutex);held=true;entered.set_value();gate.wait();held=false;});struct Release{std::promise<void>& promise;std::thread& writer;~Release(){try{promise.set_value();}catch(...){}if(writer.joinable())writer.join();}} cleanup{release,writer};
   check(ready.wait_for(std::chrono::seconds(3))==std::future_status::ready,"A real writer owns Manager before ZIP readiness begins");PrefetchedZipReadTask task(manager,job,url,headers,[&](const char* stage){if(std::string(stage)=="waiting-model")waiting=true;});check(until([&]{return waiting.load();})&&held&&!task.poll(),"Starting and polling ZIP readiness return while its reader waits for Manager");
   auto stopped=std::async(std::launch::async,[&]{task.stop();});check(stopped.wait_for(std::chrono::seconds(2))==std::future_status::ready&&held,"Cancellation joins the lock-waiting reader without needing the writer to release");release.set_value();writer.join();stopped.get();check(task.poll().has_value()&&task.poll()->empty(),"Cancelled readiness cannot publish a file after Manager becomes available");
  }
  {
   job->data.erase("CompletedPrefetch");std::atomic_bool waiting=false;PrefetchedZipReadTask task(manager,job,url,headers,[&](const char* stage){if(std::string(stage)=="waiting-receipt")waiting=true;});check(until([&]{return waiting.load();}),"Receipt-wait cancellation begins from an observed pending state");task.stop();check(task.poll().has_value()&&task.poll()->empty(),"Stopping receipt polling joins the reader and settles without a file");
  }
  job->data.erase("OriginalResponse");manager.stop();
 }catch(const std::exception& e){error=e.what();}
 atomicText(root/L"results.json",Json{{"passed",error.empty()},{"checks",checks},{"error",error}}.dump(2),false);std::cout<<checks.size()<<" checks; "<<(error.empty()?"passed":error)<<"\n";return error.empty()?0:1;
}
