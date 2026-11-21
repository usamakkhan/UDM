#include "FileInfoBindingTask.hpp"
#include <iostream>
using namespace udm;
template<class F>static bool until(F predicate){const auto end=GetTickCount64()+4000;while(!predicate()&&GetTickCount64()<end)Sleep(1);return predicate();}
int wmain(int argc,wchar_t** argv){
 if(argc!=2)return 2;const auto root=fs::absolute(argv[1]);if(fs::exists(root))return 2;fs::create_directories(root);Json checks=Json::array();std::string error;
 const auto check=[&](bool ok,const char* name){checks.push_back({{"name",name},{"passed",ok}});if(!ok)throw std::runtime_error(name);};
 try{
  Manager manager(root/L"catalog");PendingFileInfoInbox inbox;const std::string fingerprint(64,'c');
  struct Fixture {JobPtr job;std::shared_ptr<const PendingFileInfoInbox::Operation> operation;std::string token;};
  const auto make=[&](const char* name){Fixture f;f.job=manager.add("http://127.0.0.1:1/media.mp4",utf8((root/L"files").wstring()),name,"Main queue",true);f.job->data["Status"]="Awaiting confirmation";f.job->data["SourceUrl"]="http://127.0.0.1:1/watch";f.token=std::to_string(epoch())+"-"+guid();manager.state["BrowserCaptures"][f.token]={{"protocol",3},{"status","accepted"},{"id",f.job->id()},{"created",epoch()},{"fingerprint",protect(fingerprint)},{"presentation",{{"id",f.job->id()},{"kind","download"}}}};auto view=captureFileInfoInitialView(manager,*f.job,fingerprint);auto ticket=inbox.publish(view,[](auto){return true;},f.token);ticket.accepted(f.job->id());f.operation=inbox.takeForJob(f.job->id());return f;};
  {
   auto f=make("accepted.mp4");FileInfoBindingTask task;const auto before=manager.snapshot();check(task.start(manager,f.operation)&&until([&]{return task.ready();}),"An already accepted prepared view resolves on an owned reader");check(!task.start(manager,f.operation),"An unconsumed binding request cannot be replayed");auto result=task.take();
   check(result.job==f.job&&result.operation==f.operation&&!result.deferred&&!result.presentation.empty(),"Binding returns the exact job and captured grouped presentation");check(manager.snapshot()==before&&!task.valid(),"Reading and acknowledgement do not mutate admission, presentation or the catalog");
   const auto later=std::to_string(epoch())+"-"+guid();manager.state["BrowserCaptures"][later]=manager.state["BrowserCaptures"][f.token];manager.finishBrowserPresentation(result.presentation);
   check(!manager.state["BrowserCaptures"][f.token].contains("presentation")&&manager.state["BrowserCaptures"][later].contains("presentation"),"Acknowledging only the captured tokens preserves a later presentation");
   check(f.operation->requestCancel()&&!f.operation->attach(),"A Cancel that wins after the read prevents UI attachment");
  }
  {
   auto f=make("held.mp4");std::promise<void> entered,release;auto ready=entered.get_future();auto released=release.get_future().share();std::atomic_bool held=false,waiting=false;
   std::thread writer([&]{Lock lock(manager.mutex);held=true;entered.set_value();released.wait();held=false;});struct Join{std::promise<void>& release;std::thread& writer;~Join(){try{release.set_value();}catch(...){}if(writer.joinable())writer.join();}} join{release,writer};FileInfoBindingTask task;
   check(ready.wait_for(std::chrono::seconds(3))==std::future_status::ready,"The real model is held before accepted binding is requested");task.start(manager,f.operation,{},[&](const char* stage){if(std::string(stage)=="waiting-model")waiting=true;});
   check(until([&]{return waiting.load();})&&held&&!task.ready(),"Request and observation return while the reader waits for the held model");
   auto stopped=std::async(std::launch::async,[&]{task.finish();});check(stopped.wait_for(std::chrono::seconds(1))==std::future_status::ready&&held,"Teardown cancels and joins a model-waiting reader before the external model hold is released");stopped.get();check(!task.valid(),"Joined cancellation leaves no pending result or model callback");release.set_value();writer.join();
  }
  {
   auto f=make("cancelled.mp4");f.operation->requestCancel();FileInfoBindingTask task;task.start(manager,f.operation);check(until([&]{return task.ready();})&&task.take().cancelled,"An already cancelled session never binds an accepted job");
  }
  {
   auto f=make("deferred.mp4");manager.fileInfoDecisionPending=true;FileInfoBindingTask task;task.start(manager,f.operation);check(until([&]{return task.ready();})&&task.take().deferred,"Another accepted command defers binding without changing its session");manager.fileInfoDecisionPending=false;task.start(manager,f.operation);check(until([&]{return task.ready();})&&task.take().job==f.job,"A later cadence read binds once the command guard clears");
  }
  {
   auto f=make("metadata.mp4");
   f.job->data["Size"]=123456;f.job->data["FormatDescription"]="1080p MP4";f.job->data["Description"]="Refreshed title";
   f.job->data["ContentType"]="video/mp4";manager.state["Settings"]["SaveFolderHistory"]=Json::array({"C:/new-folder"});
   const auto before=manager.snapshot();FileInfoBindingTask task;task.start(manager,f.operation);
   check(until([&]{return task.ready();}),"Metadata and preference refreshes settle asynchronously");
   auto result=task.take();check(result.job==f.job&&!result.presentation.empty()&&manager.snapshot()==before,"Harmless display refreshes preserve binding without rewriting the draft or catalog");
   check(!sameFileInfoInitialView(*captureFileInfoInitialView(manager,*f.job,fingerprint),*f.operation->view),"Binding freshness is distinct from rendering-cache freshness");
  }
  for(const std::string change:{"fingerprint","receipt-id","receipt-status","presentation","job-removed","view-changed","headers","proxy","request","adaptive","sabr","output","track","track-presence","already-started"}){
   auto f=make((change+".mp4").c_str());auto& row=manager.state["BrowserCaptures"][f.token];
   if(change=="fingerprint")row["fingerprint"]=protect(std::string(64,'d'));if(change=="receipt-id")row["id"]="different";if(change=="receipt-status")row["status"]="released";if(change=="presentation")row.erase("presentation");if(change=="job-removed")manager.jobs.erase(std::remove(manager.jobs.begin(),manager.jobs.end(),f.job),manager.jobs.end());if(change=="view-changed")f.job->data["SourceUrl"]="http://127.0.0.1:1/another-video";if(change=="headers")f.job->data["ProtectedHeaders"]="changed";if(change=="proxy")f.job->data["ProtectedBrowserProxy"]="changed";if(change=="request")f.job->data["ProtectedRequest"]="changed";if(change=="adaptive")f.job->data["ProtectedAdaptive"]="changed";if(change=="sabr")f.job->data["ProtectedSabr"]="changed";if(change=="output")f.job->data["MediaOutput"]="audio";if(change=="track"||change=="track-presence"){f.job->video=std::make_shared<Job>(Json{{"Id",guid()}});if(change=="track")f.job->video->data["Url"]="http://127.0.0.1:1/track";}if(change=="already-started")f.job->data["IndividualStart"]=true;
   const auto before=manager.snapshot();FileInfoBindingTask task;task.start(manager,f.operation);check(until([&]{return task.ready();}),"An obsolete or invalid prepared binding settles asynchronously");bool rejected=false;try{rejected=!task.take().job;}catch(const std::exception&){rejected=true;}
   check(rejected&&manager.snapshot()==before,"Receipt, membership, presentation, view and individual-start changes cannot attach or mutate a different download");
  }
  {
   auto f=make("exception.mp4");FileInfoBindingTask task;task.start(manager,f.operation,{},[](const char* stage){if(std::string(stage)=="before-read")throw std::runtime_error("Owned binding failure");});check(until([&]{return task.ready();}),"A reader exception settles without a UI callback into the model");bool failed=false;try{task.take();}catch(const std::exception& e){failed=std::string(e.what())=="Owned binding failure";}check(failed&&!task.valid()&&!f.operation->cancelRequested(),"The failure is consumed without cancelling or acknowledging the user's offer");
  }
  inbox.close();inbox.stopWorker();manager.stop();
 }catch(const std::exception& e){error=e.what();}
 atomicText(root/L"results.json",Json{{"passed",error.empty()},{"checks",checks},{"error",error}}.dump(2),false);std::cout<<checks.size()<<" checks; "<<(error.empty()?"passed":error)<<"\n";return error.empty()?0:1;
}
