#include "FileInfoPreviewTask.hpp"
#include "AuthenticationFixture.hpp"
#include <iostream>
using namespace udm;
template<class F>static bool until(F f){const auto end=GetTickCount64()+8000;while(!f()&&GetTickCount64()<end)Sleep(1);return f();}
int wmain(int argc,wchar_t** argv){
 if(argc!=2)return 2;const auto root=fs::absolute(argv[1]);if(fs::exists(root))return 2;fs::create_directories(root);Json checks=Json::array();std::string error;
 const auto check=[&](bool ok,const char* name){checks.push_back({{"name",name},{"passed",ok}});if(!ok)throw std::runtime_error(name);};
 try{
  WSADATA sockets{};if(WSAStartup(MAKEWORD(2,2),&sockets))throw std::runtime_error("Cannot initialize owned preview origin.");struct Wsa{~Wsa(){WSACleanup();}} wsa;
  AuthenticationFixture origin;Manager manager(root/L"catalog");manager.state["Settings"]["Retries"]=0;
  auto job=manager.add(origin.url("/basic"),utf8((root/L"files").native()),"preview.bin","Main queue",true,{{"Authorization","Basic dTpwOjI="}});
  const auto request=[&]{FileInfoPreviewRequest r;r.job=job;r.generation=41;r.expectedUrl=origin.url("/basic");return r;};
  {
   const auto before=manager.snapshot();std::atomic_int notices=0;FileInfoPreviewTask task;check(task.start(manager,request(),[&]{++notices;}),"An owned preview preparation accepts one identified request");check(!task.start(manager,request()),"An unconsumed preparation cannot be overwritten");check(until([&]{return task.ready();}),"Actual loopback preview launch settles asynchronously");auto result=task.take();
   check(result.job==job&&result.generation==41&&result.preview&&!result.cancelled&&!result.skipped,"The result identifies its exact job and request generation");
   check(until([&]{return notices.load()>=2&&str(result.preview->snapshot(),"Status")!="Checking";}),"Both launch and actual HTTP completion deliver result-only wake-ups");auto details=result.preview->snapshot();
   check(str(details,"Status")=="Ready"&&num(details,"Size",-1)==static_cast<i64>(origin.payload.size())&&origin.requests==1,"One actual HEAD returns the exact owned-origin metadata length");check(manager.snapshot()==before&&!manager.isActive(job),"Metadata probing does not admit or change the saved download");result.preview->stop();
  }
  {
   job->data["Received"]=99;job->data["Segments"]=Json::array({Json{{"Index",0},{"Start",0},{"Done",99}}});const auto before=manager.snapshot();auto r=request();r.draftLogin=true;r.loginEnabled=true;r.user="edited";r.password="private";Json captured;Manager* persistence=&manager;FileInfoPreviewTask task;
   task.start(manager,r,{}, {},[&](Json record,Json,int,Manager* owner,JobPtr){captured=std::move(record);persistence=owner;return std::shared_ptr<DownloadPreview>{};});check(until([&]{return task.ready();})&&!task.take().skipped,"Edited credentials prepare a distinct metadata request for a partially saved job");
   check(num(captured,"Received")==0&&captured["Segments"].empty()&&str(captured,"Status")=="Awaiting confirmation"&&basicLogin(readHeaders(captured))==std::make_pair(std::string("edited"),std::string("private")),"Only the owned probe copy receives edited credentials and reset progress");check(!persistence&&manager.snapshot()==before,"An edited-login preview cannot persist cookies or credentials into its original job");job->data["Received"]=0;job->data["Segments"]=Json::array();
  }
  for(const std::string change:{"url","headers","proxy","progress","completed","capture-required","removed","retry-blocked"}){
   auto r=request();r.automatic=change=="retry-blocked";const auto before=job->data;bool called=false;FileInfoPreviewTask task;
   task.start(manager,r,{},[&](const char* stage){if(std::string(stage)=="captured"){Lock lock(manager.mutex);if(change=="url")job->data["Url"]="http://127.0.0.1:1/different";if(change=="headers")job->data["ProtectedHeaders"]=protect("{}");if(change=="proxy")job->data["ProtectedBrowserProxy"]="changed";if(change=="progress")job->data["Received"]=1;if(change=="completed")job->data["Status"]="Complete";if(change=="capture-required")job->data["RequiresRequestCapture"]=true;if(change=="removed")manager.jobs.erase(std::remove(manager.jobs.begin(),manager.jobs.end(),job),manager.jobs.end());if(change=="retry-blocked")job->data["BrowserVerificationRequired"]=true;}},[&](Json,Json,int,Manager*,JobPtr){called=true;return std::shared_ptr<DownloadPreview>{};});
   check(until([&]{return task.ready();})&&task.take().skipped&&!called,"Changed request identity, membership or retry policy prevents a late probe launch");{Lock lock(manager.mutex);job->data=before;if(change=="removed")manager.jobs.push_back(job);}
  }
  {
   std::promise<void> entered,release;auto ready=entered.get_future();auto gate=release.get_future().share();std::atomic_bool held=false,waiting=false;std::atomic_int launches=0;
   std::thread writer([&]{Lock lock(manager.mutex);held=true;entered.set_value();gate.wait();held=false;});struct Join{std::promise<void>& release;std::thread& writer;~Join(){try{release.set_value();}catch(...){}if(writer.joinable())writer.join();}} join{release,writer};FileInfoPreviewTask task;
   check(ready.wait_for(std::chrono::seconds(3))==std::future_status::ready,"A real catalog writer holds Manager before metadata preparation");task.start(manager,request(),{},[&](const char* stage){if(std::string(stage)=="waiting-model")waiting=true;},[&](Json,Json,int,Manager*,JobPtr){++launches;return std::shared_ptr<DownloadPreview>{};});
   check(until([&]{return waiting.load();})&&held&&!task.ready(),"Requesting file details returns while its reader waits for the model");auto stopped=std::async(std::launch::async,[&]{task.finish();});auto status=stopped.wait_for(std::chrono::seconds(1));release.set_value();writer.join();stopped.get();check(status==std::future_status::ready&&launches==0&&!task.valid(),"Cancelling a model-waiting request joins it before release and never launches a later probe");
  }
  for(const std::string blocked:{"verification","authentication","http-error","replacement","reported-hint","post"}){
   const auto before=job->data;auto r=request();r.automatic=true;bool called=false;
   if(blocked=="verification")job->data["BrowserVerificationRequired"]=true;if(blocked=="authentication")job->data["AuthenticationPromptPending"]=true;if(blocked=="http-error")job->data["LastHttpStatus"]=403;if(blocked=="replacement")job->data["RefreshPendingValidation"]=true;if(blocked=="reported-hint"){job->data["BrowserReportedDirectSize"]=1234;job->data["BrowserReportedFileName"]="hint.bin";}if(blocked=="post")job->data["ProtectedRequest"]="present";
   FileInfoPreviewTask task;task.start(manager,r,{}, {},[&](Json,Json,int,Manager*,JobPtr){called=true;return std::shared_ptr<DownloadPreview>{};});check(until([&]{return task.ready();})&&task.take().skipped&&!called,"Automatic preview preserves verification, authorization, replacement, browser-hint and POST safeguards");job->data=before;
  }
  {
   FileInfoPreviewTask task;task.start(manager,request(),{},[](const char* stage){if(std::string(stage)=="captured")throw std::runtime_error("Owned preparation failure");});check(until([&]{return task.ready();}),"An owned preparation error settles without a UI model callback");bool failed=false;try{task.take();}catch(const std::exception& e){failed=std::string(e.what())=="Owned preparation failure";}check(failed&&!task.valid(),"The error is consumed and leaves the owner reusable");
  }
  manager.stop();
 }catch(const std::exception& e){error=e.what();}
 atomicText(root/L"results.json",Json{{"passed",error.empty()},{"checks",checks},{"error",error}}.dump(2),false);std::cout<<checks.size()<<" checks; "<<(error.empty()?"passed":error)<<"\n";return error.empty()?0:1;
}
