#include "FileInfoPrefetchTask.hpp"
#include "FileInfoDecision.hpp"
#include "AuthenticationFixture.hpp"
#include <iostream>
using namespace udm;
template<class F>static bool until(F ready,unsigned ms=8000){const auto end=GetTickCount64()+ms;while(!ready()&&GetTickCount64()<end)Sleep(1);return ready();}
struct AdmissionRelease{std::promise<void>& value;~AdmissionRelease(){try{value.set_value();}catch(...){}}};
int wmain(int argc,wchar_t** argv){
 if(argc!=2)return 2;const auto root=fs::absolute(argv[1]);if(fs::exists(root))return 2;fs::create_directories(root);Json checks=Json::array();std::string error;
 const auto check=[&](bool value,const char* name){checks.push_back({{"name",name},{"passed",value}});if(!value)throw std::runtime_error(name);};
 try{
  WSADATA winsock{};if(WSAStartup(MAKEWORD(2,2),&winsock))throw std::runtime_error("Owned prefetch origin unavailable");struct Sockets{~Sockets(){WSACleanup();}} sockets;AuthenticationFixture origin;
  Manager manager(root/L"catalog");manager.state["Settings"]["PrefetchFileInfo"]=true;manager.state["Settings"]["CategoryFolders"]=false;manager.state["Settings"]["SuppressCompletionDialog"]=true;manager.state["Settings"]["SuppressProgressDialog"]=true;manager.save();
  const auto make=[&](const char* name){auto job=manager.add(origin.url("/basic"),utf8((root/L"files").wstring()),name,"Main queue",true);job->data["ProtectedHeaders"]=protect(Json{{"Authorization","Basic dTpwOjI="}}.dump());job->data["Connections"]=1;manager.save();return job;};
  const auto exactPrefetch=[&](const JobPtr& job){return until([&]{Lock lock(manager.mutex);const auto found=job->data.find("CompletedPrefetch");return manager.isActive(job)&&found!=job->data.end()&&num(*found,"Version")==1&&str(*found,"Url")==str(job->data,"Url")&&str(*found,"Headers")==str(job->data,"ProtectedHeaders")&&num(*found,"Size",-1)==static_cast<i64>(origin.payload.size())&&num(job->data,"Size",-1)==static_cast<i64>(origin.payload.size())&&num(job->data,"Received")==static_cast<i64>(origin.payload.size());});};
  const auto complete=[&](const JobPtr& job){return until([&]{manager.tick();Lock lock(manager.mutex);return !manager.isActive(job)&&str(job->data,"Status")=="Complete";},15000)&&readText(job->target(),origin.payload.size()+1)==origin.payload;};
  const auto draft=[&](const JobPtr& job,bool start){FileInfoDecisionDraft value;value.start=start;value.loginUnchanged=true;value.edit={{"Folder",str(job->data,"Folder")},{"FileName",str(job->data,"FileName")},{"Category",str(job->data,"Category")},{"Queue","Main queue"},{"Description","Owned asynchronous admission"}};return value;};
  {
   auto job=make("start.bin");std::atomic_uint wakes=0;const auto owner=std::this_thread::get_id();std::atomic_bool ownerRead=false;FileInfoPrefetchTask task;const auto requests=origin.requests.load();
   check(task.start(manager,job,[&]{++wakes;},[&](const char*){if(std::this_thread::get_id()==owner)ownerRead=true;})&&until([&]{return task.ready();}),"Prefetch admission runs to readiness on an owned worker");
   check(!task.start(manager,job),"A second request cannot replay an unacknowledged admission");check(task.take().admission==PrefetchAdmission::Started&&!task.valid()&&!ownerRead,"Acknowledgement joins the admission worker and reports Started");
   check(exactPrefetch(job),"The owned HTTP file downloads completely before Start is accepted");check(!fs::exists(job->target())&&yes(job->data,"ConfirmationPending"),"Complete prefetch waits for confirmation without publishing the destination");
   check(origin.requests==requests+1&&origin.accepted>=1,"Prefetch uses one authorized GET without a second metadata request");
   FileInfoDecisionTask decision;decision.start(manager,job,draft(job,true));check(until([&]{return decision.ready();}),"Start applies after admission on the existing owned decision worker");decision.get();decision.release();check(complete(job),"Start publishes the complete exact prefetched file");check(origin.requests==requests+1,"Confirmation reuses the complete response without another GET");
  }
  {
   auto job=make("later.bin");FileInfoPrefetchTask task;task.start(manager,job);check(until([&]{return task.ready();})&&task.take().admission==PrefetchAdmission::Started&&exactPrefetch(job),"A second complete prefetch is available for Download Later");
   applyFileInfoDecision(manager,job,draft(job,false));check(until([&]{manager.tick();Lock lock(manager.mutex);return !manager.isActive(job);}),"Download Later joins the stopped prefetched transfer");
   check(str(job->data,"Status")=="Paused"&&!yes(job->data,"ConfirmationPending")&&!fs::exists(job->target()),"Download Later preserves the paused unpublished download");
   applyFileInfoDecision(manager,job,draft(job,true));check(complete(job),"A later Start publishes the retained complete body correctly");
  }
  {
   auto job=make("cancel-before.bin");const auto requests=origin.requests.load();std::promise<void> entered,release;auto ready=entered.get_future();auto released=release.get_future().share();FileInfoPrefetchTask admission;FileInfoDecisionTask cancellation;AdmissionRelease cleanup{release};
   admission.start(manager,job,{},[&](const char* stage){if(std::string(stage)=="before-admission"){entered.set_value();released.wait();}});check(ready.wait_for(std::chrono::seconds(3))==std::future_status::ready,"Cancel fixture holds actual pending admission before its model transaction");
   cancellation.startAction(manager,[&]{cancelFileInfoPrefetch(manager,job,admission,false);return FileInfoDecisionResult{};});check(!cancellation.ready()&&manager.fileInfoDecisionPending,"Cancellation owns a command and joins pending admission without blocking its requester");release.set_value();
   check(until([&]{return cancellation.ready();}),"The joined Cancel settles after admission is released");cancellation.get();cancellation.release();
   check(origin.requests==requests&&!manager.isActive(job)&&!yes(job->data,"ConfirmationPending")&&!admission.valid(),"A queued cancellation prevents a late admission from starting a request");
  }
  {
   auto job=make("cancel-after.bin");std::promise<void> entered,release;auto ready=entered.get_future();auto released=release.get_future().share();FileInfoPrefetchTask admission;FileInfoDecisionTask cancellation;AdmissionRelease cleanup{release};
   admission.start(manager,job,{},[&](const char* stage){if(std::string(stage)=="admitted"){entered.set_value();released.wait();}});check(ready.wait_for(std::chrono::seconds(3))==std::future_status::ready&&exactPrefetch(job),"A real admitted transfer can finish while its admission acknowledgement is delayed");
   cancellation.startAction(manager,[&]{cancelFileInfoPrefetch(manager,job,admission,false);return FileInfoDecisionResult{};});check(!cancellation.ready(),"Cancel does not race a still-owned admitted transfer");release.set_value();check(until([&]{return cancellation.ready();}),"Cancel joins admission and the running transfer before acknowledging");cancellation.get();cancellation.release();
   check(!manager.isActive(job)&&!yes(job->data,"ConfirmationPending")&&str(job->data,"Status")=="Paused"&&!fs::exists(job->target()),"Acknowledged cancellation leaves no active or published background download");
  }
  {
   auto job=make("busy.bin");FileInfoPrefetchTask task;std::promise<void> entered,release;auto ready=entered.get_future();auto released=release.get_future().share();std::atomic_bool held=false;std::thread writer([&]{Lock lock(manager.mutex);held=true;entered.set_value();released.wait();held=false;});
   struct Join{std::promise<void>& release;std::thread& writer;~Join(){try{release.set_value();}catch(...){}if(writer.joinable())writer.join();}} joined{release,writer};
   check(ready.wait_for(std::chrono::seconds(3))==std::future_status::ready,"The real model is held before admission is requested");task.start(manager,job);check(held&&!task.ready()&&!task.start(manager,job),"Request and readiness checks return while admission waits for a held catalog");release.set_value();writer.join();check(until([&]{return task.ready();})&&task.take().admission==PrefetchAdmission::Started,"Admission proceeds exactly once after model release");cancelFileInfoPrefetch(manager,job,task,false);
  }
  {
   auto first=make("capacity-owner.bin"),second=make("capacity-waiter.bin");manager.state["Settings"]["Parallel"]=1;FileInfoPrefetchTask owner,waiter;owner.start(manager,first);
   check(until([&]{return owner.ready();})&&owner.take().admission==PrefetchAdmission::Started&&exactPrefetch(first),"One real prefetched response occupies the configured transfer slot");
   const auto requests=origin.requests.load();waiter.start(manager,second);check(until([&]{return waiter.ready();})&&waiter.take().admission==PrefetchAdmission::AtCapacity,"An occupied slot returns capacity without a second transfer");check(origin.requests==requests&&!manager.isActive(second),"Capacity acknowledgement cannot create or replay a request");
   cancelFileInfoPrefetch(manager,first,owner,false);waiter.start(manager,second);check(until([&]{return waiter.ready();})&&waiter.take().admission==PrefetchAdmission::Started,"An explicit cadence-style retry can use a newly available slot");cancelFileInfoPrefetch(manager,second,waiter,false);manager.state["Settings"]["Parallel"]=3;
  }
  {
   auto job=make("lost-ack.bin");FileInfoPrefetchTask task;task.start(manager,job,{},[](const char* stage){if(std::string(stage)=="admitted")throw std::runtime_error("Owned admission acknowledgement lost");});check(until([&]{return task.ready();}),"An uncertain admission acknowledgement settles its future");bool failed=false;try{task.take();}catch(const std::exception&){failed=true;}
   {Lock lock(manager.mutex);check(failed&&manager.isActive(job)&&yes(job->data,"ConfirmationPending"),"An acknowledgement failure does not pretend an already admitted transfer was undone");}cancelFileInfoPrefetch(manager,job,task,false);check(!manager.isActive(job)&&!yes(job->data,"ConfirmationPending"),"Cancel observes current state and stops an uncertain admitted transfer without replay");
  }
  {
   auto job=make("deferred.bin");manager.fileInfoDecisionPending=true;FileInfoPrefetchTask task;task.start(manager,job);check(until([&]{return task.ready();})&&task.take().deferred,"Another accepted decision defers admission rather than reporting unsupported prefetch");manager.fileInfoDecisionPending=false;
   {Lock lock(manager.mutex);manager.jobs.erase(std::remove(manager.jobs.begin(),manager.jobs.end(),job),manager.jobs.end());}task.start(manager,job);check(until([&]{return task.ready();})&&task.take().admission==PrefetchAdmission::Unavailable,"A removed job cannot be admitted or resurrected");
   task.start(manager,{});check(until([&]{return task.ready();})&&task.take().admission==PrefetchAdmission::Unavailable,"A missing job is rejected before dereferencing live state");
  }
  {
   auto job=make("storage-failure.bin");const auto before=job->data;const auto requests=origin.requests.load();Handle lease(CreateFileW((manager.root/L"state.json").c_str(),GENERIC_READ,FILE_SHARE_READ,nullptr,OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL,nullptr));check(bool(lease),"Pin the real catalog to reject the admission checkpoint");FileInfoPrefetchTask task;task.start(manager,job);check(until([&]{return task.ready();}),"A failed durable admission settles asynchronously");bool failed=false;try{task.take();}catch(const std::exception&){failed=true;}
   check(failed&&!task.valid()&&!manager.isActive(job)&&job->data==before&&origin.requests==requests,"Admission save failure preserves the draft and sends no HTTP request");CloseHandle(lease.h);lease.h=INVALID_HANDLE_VALUE;Sleep(20);check(origin.requests==requests,"A failed mutation is not implicitly retried");
   task.start(manager,job);check(until([&]{return task.ready();})&&task.take().admission==PrefetchAdmission::Started,"An explicit retry remains possible after the storage lock is released");cancelFileInfoPrefetch(manager,job,task,false);
  }
  {
   auto job=make("joined.bin");std::promise<void> entered,release;auto ready=entered.get_future();auto released=release.get_future().share();FileInfoPrefetchTask task;std::future<void> joined;AdmissionRelease cleanup{release};task.start(manager,job,{},[&](const char* stage){if(std::string(stage)=="before-admission"){entered.set_value();released.wait();}});check(ready.wait_for(std::chrono::seconds(3))==std::future_status::ready,"Teardown owns a real in-flight admission callback");joined=std::async(std::launch::async,[&]{task.finish();});check(joined.wait_for(std::chrono::milliseconds(20))==std::future_status::timeout,"Teardown joins admission before allowing Manager lifetime to end");release.set_value();joined.get();check(!task.valid(),"Joined teardown consumes the outstanding acknowledgement without detaching work");cancelFileInfoPrefetch(manager,job,task,false);
  }
  manager.stop();
 }catch(const std::exception& e){error=e.what();}
 atomicText(root/L"results.json",Json{{"passed",error.empty()},{"checks",checks},{"error",error}}.dump(2),false);std::cout<<checks.size()<<" checks; "<<(error.empty()?"passed":error)<<"\n";return error.empty()?0:1;
}
