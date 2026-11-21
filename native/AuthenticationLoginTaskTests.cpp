#include "AuthenticationLoginTask.hpp"
#include "AuthenticationFixture.hpp"
#include <iostream>
using namespace udm;
template<class Predicate>static bool until(Predicate ready,unsigned milliseconds=6000){const auto end=GetTickCount64()+milliseconds;while(!ready()&&GetTickCount64()<end)Sleep(1);return ready();}
struct PromiseRelease {std::promise<void>& value;~PromiseRelease(){try{value.set_value();}catch(...) {}}};
int wmain(int argc,wchar_t** argv){
 if(argc!=2)return 2;const auto root=fs::absolute(argv[1]);if(fs::exists(root))return 2;fs::create_directories(root);Json checks=Json::array();std::string error;
 const auto check=[&](bool ok,const char* name){checks.push_back({{"name",name},{"passed",ok}});if(!ok)throw std::runtime_error(name);};
 try{
  WSADATA winsock{};if(WSAStartup(MAKEWORD(2,2),&winsock))throw std::runtime_error("Cannot initialize owned socket fixture.");struct WinsockCleanup{~WinsockCleanup(){WSACleanup();}} cleanup;
  AuthenticationFixture server;Manager manager(root/L"catalog");manager.state["Settings"]["DownloadFolder"]=utf8((root/L"files").wstring());manager.state["Settings"]["CategoryFolders"]=false;manager.state["Settings"]["Connections"]=1;manager.state["Settings"]["Retries"]=0;manager.state["Queues"][0]["Enabled"]=false;
  const auto make=[&](const char* name,const char* path="/basic"){
   auto job=manager.add(server.url(path),"",name,"Main queue",true);Lock lock(manager.mutex);job->data["Status"]="Failed";job->data["QueueMember"]=false;job->data["QueueOrigin"]=false;job->data["AuthenticationOrigin"]=Url(server.url(path)).origin;job->data["AuthenticationScheme"]=std::string(path)=="/digest"?"Digest":"Basic";job->data["LastHttpStatus"]=401;job->data["AuthenticationPromptPending"]=true;return job;
  };
  const auto draftFor=[&](const JobPtr& job,bool manual=false){AuthenticationLoginDraft draft;if(manual){Lock lock(manager.mutex);draft.view.job=job;draft.view.key=authenticationPromptKey(job->data);draft.expectedPending=yes(job->data,"AuthenticationPromptPending");}else{auto view=prepareAuthenticationPrompt(manager,job);if(!view)throw std::runtime_error("Cannot prepare owned login challenge.");draft.view=*view;}draft.user="u";draft.password="p:2";return draft;};
  const auto complete=[&](const JobPtr& job){return until([&]{manager.tick();Lock lock(manager.mutex);return !manager.isActive(job)&&str(job->data,"Status")!="Queued";},15000)&&str(job->data,"Status")=="Complete"&&readText(job->target(),server.payload.size()+1)==server.payload;};
  {
   auto job=make("held.bin");auto draft=draftFor(job);AuthenticationLoginTask task;std::promise<void> entered,release;auto ready=entered.get_future();auto released=release.get_future().share();std::atomic_uint notifications=0;PromiseRelease releaseGuard{release};
   task.start(manager,draft,[&]{++notifications;},[&](const char* stage){if(std::string(stage)=="before-save"){entered.set_value();released.wait();}});
   check(ready.wait_for(std::chrono::seconds(3))==std::future_status::ready&&task.active()&&!task.ready()&&manager.fileInfoDecisionPending,"Starting Sign in returns while its actual save command holds the model on a worker");
   bool duplicateRejected=false;try{task.start(manager,draft);}catch(const std::exception&){duplicateRejected=true;}check(duplicateRejected&&manager.fileInfoDecisionPending,"A duplicate sign-in cannot replace active work or release its decision guard");
   bool earlyRejected=false;try{task.take();}catch(const std::exception&){earlyRejected=true;}check(earlyRejected&&task.active()&&manager.fileInfoDecisionPending,"Taking an unfinished result cannot abandon the command");
   release.set_value();check(until([&]{return task.ready();})&&manager.fileInfoDecisionPending,"Completed persistence retains the guard until the UI consumes its outcome");
   const auto result=task.take();check(result.queued&&!result.savedKey.empty()&&!task.active()&&!manager.fileInfoDecisionPending&&notifications==1,"Result consumption joins the worker and acknowledges the guard exactly once");
   check(complete(job),"The held sign-in command dispatches and publishes the complete owned Basic-auth file");
  }
  {
   auto job=make("save-failure.bin");auto draft=draftFor(job);draft.remember=true;Json settings;{Lock lock(manager.mutex);settings=manager.state["Settings"];}
   Handle lease(CreateFileW((manager.root/L"state.json").c_str(),GENERIC_READ,FILE_SHARE_READ,nullptr,OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL,nullptr));check(bool(lease),"The real owned catalog is pinned for a credential-save failure");AuthenticationLoginTask task;task.start(manager,draft);
   check(until([&]{return task.ready();})&&manager.fileInfoDecisionPending,"Failed persistence still produces a guarded asynchronous outcome");bool failed=false;try{task.take();}catch(const std::exception&){failed=true;}
   check(failed&&!task.active()&&!manager.fileInfoDecisionPending&&authenticationPromptKeyMatches(job->data,draft.view.key)&&manager.state["Settings"]==settings,"A failed credential save rolls back job and remembered settings and releases only its own guard");CloseHandle(lease.h);lease.h=INVALID_HANDLE_VALUE;
   task.start(manager,draft);check(until([&]{return task.ready();})&&task.take().queued,"The unchanged draft can be retried after storage recovery");check(complete(job),"The recovered saved-login command downloads the complete original file");
  }
  {
   auto job=make("retry-failure.bin");auto draft=draftFor(job);AuthenticationLoginTask task;task.start(manager,draft,{},[](const char* stage){if(std::string(stage)=="credentials-saved")throw std::runtime_error("Owned retry preparation failure");});
   check(until([&]{return task.ready();}),"A post-save retry failure is delivered without keeping the UI thread in the command");auto result=task.take();check(!result.queued&&!result.warning.empty()&&authenticationPromptKeyMatches(job->data,result.savedKey)&&!yes(job->data,"AuthenticationPromptPending"),"The outcome distinguishes committed credentials from a retry that did not start");
   draft.view.key=result.savedKey;draft.expectedPending=false;task.start(manager,draft);check(until([&]{return task.ready();})&&task.take().queued,"The returned saved identity supports a fresh validated retry");check(complete(job),"The post-save retry recovery publishes the exact complete file");
  }
  {
   auto job=make("changed.bin");auto draft=draftFor(job);const auto headers=str(job->data,"ProtectedHeaders");job->data["Url"]=server.url("/digest");AuthenticationLoginTask task;task.start(manager,draft);check(until([&]{return task.ready();}),"A changed challenge is resolved asynchronously");bool rejected=false;try{task.take();}catch(const std::exception&){rejected=true;}
   check(rejected&&str(job->data,"Url")==server.url("/digest")&&str(job->data,"ProtectedHeaders")==headers&&!manager.fileInfoDecisionPending,"Changed URL identity cannot receive credentials from an obsolete displayed challenge");
   manager.jobs.erase(std::remove(manager.jobs.begin(),manager.jobs.end(),job),manager.jobs.end());task.start(manager,draft);check(until([&]{return task.ready();}),"A removed job also returns a settled validation result");rejected=false;try{task.take();}catch(const std::exception&){rejected=true;}check(rejected&&!task.active()&&!manager.fileInfoDecisionPending,"A removed job is not resurrected and cannot leak the presentation guard");
  }
  {
   auto job=make("manual-digest.bin","/digest");auto draft=draftFor(job,true);AuthenticationLoginTask task;task.start(manager,draft);check(until([&]{return task.ready();})&&task.take().queued,"An unclaimed manual or queue-origin-style pending challenge uses the same command boundary");check(complete(job),"The manual pending-state contract retries Digest and publishes the complete file");
  }
  {
   auto job=make("pending-changed.bin");auto draft=draftFor(job,true);job->data["AuthenticationPromptPending"]=false;AuthenticationLoginTask task;task.start(manager,draft);check(until([&]{return task.ready();}),"A changed pending-state expectation returns a settled result");bool rejected=false;try{task.take();}catch(const std::exception&){rejected=true;}check(rejected&&!yes(job->data,"AuthenticationPromptPending")&&!manager.fileInfoDecisionPending,"A stale manual pending-state draft cannot overwrite an acknowledged request");
  }
  {
   auto job=make("shutdown.bin");auto draft=draftFor(job);AuthenticationLoginTask task;std::promise<void> entered,release;auto ready=entered.get_future();auto released=release.get_future().share();std::future<void> finishing;PromiseRelease releaseGuard{release};
   task.start(manager,draft,{},[&](const char* stage){if(std::string(stage)=="before-save"){entered.set_value();released.wait();}});check(ready.wait_for(std::chrono::seconds(3))==std::future_status::ready,"Teardown starts with a real in-flight login command");
   finishing=std::async(std::launch::async,[&]{task.finish();});check(finishing.wait_for(std::chrono::milliseconds(20))==std::future_status::timeout&&manager.fileInfoDecisionPending,"Forced owner teardown joins actual work before releasing its guard");release.set_value();finishing.get();check(!task.active()&&!task.ready()&&!manager.fileInfoDecisionPending,"Joined teardown releases the settled command guard without exposing a discarded result");check(complete(job),"A settled retry remains valid after its command owner has joined");
  }
  {
   auto job=make("other-owner.bin");auto draft=draftFor(job);FileInfoDecisionTask other;std::promise<void> entered,release;auto ready=entered.get_future();auto released=release.get_future().share();PromiseRelease releaseGuard{release};other.startAction(manager,[&]{entered.set_value();released.wait();return FileInfoDecisionResult{};});check(ready.wait_for(std::chrono::seconds(3))==std::future_status::ready,"Another decision owns the shared presentation guard");AuthenticationLoginTask task;
   bool rejected=false;try{task.start(manager,draft);}catch(const std::exception&){rejected=true;}check(rejected&&!task.active()&&manager.fileInfoDecisionPending,"A blocked new login cannot clear another decision owner's guard");release.set_value();check(until([&]{return other.ready();}),"The independent decision finishes normally");other.get();other.release();
  }
  manager.stop();check(server.accepted>=5,"Owned authenticated retries reached the local origin without public-site access");
 }catch(const std::exception& e){error=e.what();}
 atomicText(root/L"results.json",Json{{"passed",error.empty()},{"checks",checks},{"error",error}}.dump(2),false);std::cout<<checks.size()<<" checks; "<<(error.empty()?"passed":error)<<"\n";return error.empty()?0:1;
}
