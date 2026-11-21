#include "AuthenticationPromptTask.hpp"
#include "AuthenticationFixture.hpp"
#include <iostream>
using namespace udm;
template<class Predicate>static bool until(Predicate ready){const auto end=GetTickCount64()+5000;while(!ready()&&GetTickCount64()<end)Sleep(1);return ready();}
struct PromiseRelease {std::promise<void>& promise;~PromiseRelease(){try{promise.set_value();}catch(...) {}}};
int wmain(int argc,wchar_t** argv){
 if(argc!=2)return 2;const auto root=fs::absolute(argv[1]);if(fs::exists(root))return 2;fs::create_directories(root);Json checks=Json::array();std::string error;
 const auto check=[&](bool ok,const char* name){checks.push_back({{"name",name},{"passed",ok}});if(!ok)throw std::runtime_error(name);};
 try{
  Manager manager(root/L"catalog");auto job=manager.add("https://auth.example.invalid/file",utf8((root/L"files").wstring()),"owned.bin","Main queue",true);
  const auto arm=[&]{Lock lock(manager.mutex);job->data["Status"]="Failed";job->data["AuthenticationOrigin"]=Url(str(job->data,"Url")).origin;job->data["AuthenticationScheme"]="Basic";job->data["LastHttpStatus"]=401;job->data["AuthenticationPromptPending"]=true;job->data["QueueOrigin"]=false;};arm();
  Headers headers;setBasicLogin(headers,"owned-user","owned-password");job->data["ProtectedHeaders"]=protect(legacyDictionary(Json(headers)).dump());
  auto view=prepareAuthenticationPrompt(manager,job);check(view&&view->job==job&&view->initial==std::make_pair(std::string("owned-user"),std::string("owned-password")),"Preparation resolves only the owned challenge's initial credentials");
  check(!yes(job->data,"AuthenticationPromptPending")&&tryAuthenticationPromptCurrent(manager,*view)==AutomaticPromptReadiness::Ready,"A prepared view follows a durable claim and is eligible for presentation");
  auto disk=Json::parse(readText(manager.root/L"state.json"));check(!yes(disk["Downloads"][0],"AuthenticationPromptPending"),"The claim is committed in the real saved catalog before view publication");
  auto malformed=view->key;malformed["Extra"]=true;check(authenticationPromptKeyMatches(job->data,view->key)&&!authenticationPromptKeyMatches(job->data,malformed),"Challenge validation rejects extra keys without cloning the source");
  job->data["Received"]=7;job->data["FileName"]="renamed.bin";check(tryAuthenticationPromptCurrent(manager,*view)==AutomaticPromptReadiness::Ready,"Unrelated progress and filename changes do not invalidate the challenge");
  manager.state["Settings"]["SuppressAuthenticationDialog"]=true;check(tryAuthenticationPromptCurrent(manager,*view)==AutomaticPromptReadiness::Obsolete,"Changed suppression policy prevents an already-prepared automatic dialog");
  check(rearmAuthenticationPrompt(manager,*view)&&yes(job->data,"AuthenticationPromptPending"),"An unpresented suppressed challenge is durably rearmed");manager.state["Settings"]["SuppressAuthenticationDialog"]=false;
  view=prepareAuthenticationPrompt(manager,job);job->data["QueueOrigin"]=true;check(tryAuthenticationPromptCurrent(manager,*view)==AutomaticPromptReadiness::Obsolete&&rearmAuthenticationPrompt(manager,*view)&&yes(job->data,"AuthenticationPromptPending"),"A challenge moved to queue ownership retains its pending intent");job->data["QueueOrigin"]=false;
  view=prepareAuthenticationPrompt(manager,job);manager.state["Settings"]["SiteLogins"]=Json::array({makeSiteLogin("https://auth.example.invalid/","changed-user","changed-password")});check(tryAuthenticationPromptCurrent(manager,*view)==AutomaticPromptReadiness::Obsolete,"Changed saved-login sources invalidate stale initial credentials");rearmAuthenticationPrompt(manager,*view);
  view=prepareAuthenticationPrompt(manager,job);const auto oldUrl=str(job->data,"Url");job->data["Url"]="https://changed.example.invalid/file";arm();
  check(tryAuthenticationPromptCurrent(manager,*view)==AutomaticPromptReadiness::Obsolete&&rearmAuthenticationPrompt(manager,*view)&&yes(job->data,"AuthenticationPromptPending")&&str(job->data,"Url")!="https://auth.example.invalid/file","Rearming an obsolete view never overwrites a newer challenge");
  job->data["Url"]=oldUrl;arm();
  {
   std::promise<void> entered,release;auto ready=entered.get_future();auto released=release.get_future().share();std::atomic_uint notifications=0;const auto owner=std::this_thread::get_id();std::thread::id actionThread;
   AuthenticationPromptTask task(manager,[&](const char* stage){if(std::string(stage)=="before-claim"){actionThread=std::this_thread::get_id();entered.set_value();released.wait();}});PromiseRelease cleanup{release};
   task.prepare(job,[&]{++notifications;});check(ready.wait_for(std::chrono::seconds(3))==std::future_status::ready&&task.active()&&!task.ready(),"Starting preparation returns while its real model claim remains held on a worker");
   check(task.ownsPrompt()&&!task.prepared()&&actionThread!=owner,"The UI owner retains one in-flight request without exposing an uncommitted view");
   bool duplicateRejected=false;try{task.prepare(job);}catch(const std::exception&){duplicateRejected=true;}check(duplicateRejected,"Another automatic prompt cannot replace owned in-flight work");
   release.set_value();check(until([&]{return task.ready();}),"The task publishes completion only after its claim can finish");task.poll();check(task.prepared()&&!task.active()&&!yes(job->data,"AuthenticationPromptPending"),"Polling a ready task transfers the claimed immutable view to its owner");
   task.acknowledge();check(!task.ownsPrompt()&&!yes(job->data,"AuthenticationPromptPending"),"Normal dialog dismissal acknowledges the existing claim without a second save");task.finish();check(until([&]{return notifications==1;}),"Preparation emits the completion wake exactly once");
  }
  arm();
  {
   AuthenticationPromptTask task(manager);task.prepare(job);check(until([&]{return task.ready();}),"An unpresented shutdown fixture reaches a claimed result");task.finish();
   check(!task.ownsPrompt()&&yes(job->data,"AuthenticationPromptPending"),"Shutdown joins unconsumed preparation and rearms its unpresented claim");
   disk=Json::parse(readText(manager.root/L"state.json"));check(yes(disk["Downloads"][0],"AuthenticationPromptPending"),"Unpresented shutdown restoration is durably saved");
  }
  {
   std::promise<void> entered,release;auto ready=entered.get_future();auto released=release.get_future().share();std::future<void> closing;
   AuthenticationPromptTask task(manager,[&](const char* stage){if(std::string(stage)=="before-claim"){entered.set_value();released.wait();}});PromiseRelease cleanup{release};
   task.prepare(job);check(ready.wait_for(std::chrono::seconds(3))==std::future_status::ready,"Joined shutdown starts with a real held authentication command");closing=std::async(std::launch::async,[&]{task.finish();});check(closing.wait_for(std::chrono::milliseconds(20))==std::future_status::timeout,"Shutdown cannot return before the held command and restoration finish");release.set_value();closing.get();check(!task.ownsPrompt()&&yes(job->data,"AuthenticationPromptPending"),"Joined shutdown includes the result handoff and durable rearm");
  }
  {
   manager.save();Handle lease(CreateFileW((manager.root/L"state.json").c_str(),GENERIC_READ,FILE_SHARE_READ,nullptr,OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL,nullptr));check(bool(lease),"The owned catalog can be pinned to exercise an actual claim-save failure");
   AuthenticationPromptTask task(manager);task.prepare(job);check(until([&]{return task.ready();}),"Failed persistence returns an observable asynchronous result");bool failed=false;try{task.poll();}catch(const std::exception&){failed=true;}CloseHandle(lease.h);lease.h=INVALID_HANDLE_VALUE;
   check(failed&&!task.ownsPrompt()&&yes(job->data,"AuthenticationPromptPending"),"Claim failure restores pending intent and releases the task for a later retry");
  }
  {
   AuthenticationPromptTask task(manager);task.prepare(job);check(until([&]{return task.ready();}),"Rearm failure fixture prepares its real claimed view");task.poll();Handle lease(CreateFileW((manager.root/L"state.json").c_str(),GENERIC_READ,FILE_SHARE_READ,nullptr,OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL,nullptr));check(bool(lease),"The saved claim can be pinned before restoring it");
   task.rearm();check(until([&]{return task.ready();}),"Failed rearm persistence completes off the request thread");bool failed=false;try{task.poll();}catch(const std::exception&){failed=true;}
   check(failed&&task.ownsPrompt()&&task.rearmPending()&&yes(job->data,"AuthenticationPromptPending"),"Rearm failure retains both the view and true in-memory intent for an unfinished durability barrier");CloseHandle(lease.h);lease.h=INVALID_HANDLE_VALUE;
   task.rearm();check(until([&]{return task.ready();}),"A restored filesystem accepts the asynchronous rearm retry");task.poll();check(!task.ownsPrompt()&&yes(job->data,"AuthenticationPromptPending"),"Only successful rearm releases ownership of the unpresented view");
  }
  view=prepareAuthenticationPrompt(manager,job);manager.fileInfoDecisionPending=true;check(tryAuthenticationPromptCurrent(manager,*view)==AutomaticPromptReadiness::Busy&&!rearmAuthenticationPrompt(manager,*view),"File Info acknowledgement defers presentation and restoration without consuming ownership");manager.fileInfoDecisionPending=false;rearmAuthenticationPrompt(manager,*view);
  auto removed=job;view=prepareAuthenticationPrompt(manager,job);manager.jobs.clear();check(tryAuthenticationPromptCurrent(manager,*view)==AutomaticPromptReadiness::Obsolete&&rearmAuthenticationPrompt(manager,*view),"Removed jobs cannot be presented or resurrected by task cleanup");manager.jobs.push_back(removed);arm();
  {const auto previous=job->data;job->data["Url"]="ftp://ftp.example.invalid/file";job->data["AuthenticationOrigin"]="ftp://ftp.example.invalid";job->data["AuthenticationScheme"]="FTP";job->data["LastFtpStatus"]=530;job->data["AuthenticationPromptPending"]=true;
   auto ftp=prepareAuthenticationPrompt(manager,job);check(ftp&&tryAuthenticationPromptCurrent(manager,*ftp)==AutomaticPromptReadiness::Ready,"FTP login challenges use the same prepared-view contract without HTTP status assumptions");check(rearmAuthenticationPrompt(manager,*ftp)&&yes(job->data,"AuthenticationPromptPending"),"An unpresented FTP challenge is restored without starting a network transfer");job->data=previous;}
  manager.stop();
  {
   WSADATA winsock{};if(WSAStartup(MAKEWORD(2,2),&winsock))throw std::runtime_error("Owned HTTP fixture cannot initialize Winsock.");struct Cleanup{~Cleanup(){WSACleanup();}} cleanup;
   AuthenticationFixture server;Manager downloads(root/L"downloads");downloads.state["Settings"]["DownloadFolder"]=utf8((root/L"downloaded-files").wstring());downloads.state["Settings"]["CategoryFolders"]=false;downloads.state["Settings"]["Connections"]=1;downloads.state["Settings"]["Retries"]=0;
   for(const auto* path:{"/basic","/digest"}){
    auto candidate=downloads.add(server.url(path),"",std::string(path+1)+".bin");downloads.resume(candidate);check(until([&]{downloads.tick();Lock lock(downloads.mutex);return !downloads.isActive(candidate)&&str(candidate->data,"Status")!="Queued";})&&authenticationDialogReady(candidate->data),"The owned origin produces a real supported authentication challenge");
    auto prepared=prepareAuthenticationPrompt(downloads,candidate);check(bool(prepared),"A real HTTP challenge can be prepared and durably claimed");applyPreparedAuthenticationLogin(downloads,*prepared,"u","p:2",false);
    check(until([&]{downloads.tick();Lock lock(downloads.mutex);return !downloads.isActive(candidate)&&str(candidate->data,"Status")!="Queued";})&&str(candidate->data,"Status")=="Complete"&&readText(candidate->target(),server.payload.size()+1)==server.payload,"Prepared Basic and Digest logins each retry and publish the complete exact fixture file");
   }
   downloads.stop();check(server.accepted>=2,"Both authenticated retries reach the owned origin without browser or public-site access");
  }
 }catch(const std::exception& e){error=e.what();}
 atomicText(root/L"results.json",Json{{"passed",error.empty()},{"checks",checks},{"error",error}}.dump(2),false);std::cout<<checks.size()<<" checks; "<<(error.empty()?"passed":error)<<"\n";return error.empty()?0:1;
}
