#pragma once
#include "FileInfoInitialView.hpp"
#include "SiteLogins.hpp"
#include <future>
#include <condition_variable>
namespace udm {
struct FileInfoRefreshResult {
 Json data=Json::object(),preferences=Json::object(),selection=Json::object();
 std::pair<std::string,std::string> login;std::string address,page,error;
 bool basic=false,media=false,cancelled=false;
};
using FileInfoRefreshObserver=std::function<void(const char*)>;
// One continuation after an accepted recovery. Only advisory acquisition is
// cancellable: an already accepted endPrefetch must drain before owner release.
class FileInfoRefreshTask {
 std::atomic_bool stopping=false;std::mutex mutex;std::condition_variable wake;
 std::future<FileInfoRefreshResult> future;std::thread worker;
 std::unique_lock<std::recursive_mutex> acquire(Manager& manager,const FileInfoRefreshObserver& observe){
  while(!stopping){std::unique_lock lock(manager.mutex,std::try_to_lock);if(lock.owns_lock()&&!manager.fileInfoDecisionPending)return lock;if(lock.owns_lock())lock.unlock();if(observe)observe("waiting-model");std::unique_lock waiting(mutex);wake.wait_for(waiting,std::chrono::milliseconds(10),[&]{return stopping.load();});}return {};
 }
 FileInfoRefreshResult prepare(Manager& manager,const JobPtr& job,const Json& expected,const FileInfoRefreshObserver& observe){
  FileInfoRefreshResult result;
  try{
   const auto selected=[&]{if(!job||expected.empty()||std::find(manager.jobs.begin(),manager.jobs.end(),job)==manager.jobs.end()||!fileInfoSelectionUnchanged(*job,expected))throw std::runtime_error("The saved download request changed. Refresh it again before continuing.");};
   {auto lock=acquire(manager,observe);if(!lock.owns_lock()){result.cancelled=true;return result;}selected();result.media=hasNonemptyString(job->data,"SourceUrl")||hasNonemptyString(job->data,"ProtectedAdaptive");}
   if(!result.media){
    if(observe)observe("before-stop");if(stopping){result.cancelled=true;return result;}
    // Do not hold an outer recursive lock while endPrefetch joins the transfer.
    manager.endPrefetch(job,false,&expected);if(observe)observe("stopped-prefetch");
   }
   {auto lock=acquire(manager,observe);if(!lock.owns_lock()){result.cancelled=true;return result;}selected();
    for(const auto* key:{"Url","ProtectedHeaders","DownloadPage"})result.data[key]=str(job->data,key);
    if(!result.media)if(auto found=manager.state["Settings"].find("SiteLogins");found!=manager.state["Settings"].end())result.preferences["SiteLogins"]=*found;
    result.selection=fileInfoSelection(*job);
   }
   if(stopping){result.cancelled=true;return result;}
   result.address=str(result.data,"Url");result.page=recoveryPage(result.data);
   if(!result.media){const auto headers=siteRequestHeaders(result.address,readHeaders(result.data),result.preferences);result.login=basicLogin(headers);result.basic=lower(headerValue(headers,"Authorization")).rfind("basic ",0)==0;}
   if(observe)observe("captured");result.cancelled=stopping;
  }catch(const std::exception& failure){result.error=failure.what();}
  catch(...){result.error="The refreshed download details could not be prepared. Refresh again or close this dialog.";}
  return result;
 }
public:
 FileInfoRefreshTask()=default;FileInfoRefreshTask(const FileInfoRefreshTask&)=delete;FileInfoRefreshTask& operator=(const FileInfoRefreshTask&)=delete;
 ~FileInfoRefreshTask(){finish();}
 bool valid()const noexcept{return future.valid();}
 bool ready()const{return future.valid()&&future.wait_for(std::chrono::seconds(0))==std::future_status::ready;}
 bool start(Manager& manager,JobPtr job,Json expected,std::function<void()> notify={},FileInfoRefreshObserver observe={}){
  if(valid())return false;stopping=false;std::promise<FileInfoRefreshResult> result;future=result.get_future();
  try{worker=std::thread([this,&manager,job=std::move(job),expected=std::move(expected),notify=std::move(notify),observe=std::move(observe),result=std::move(result)]()mutable{try{result.set_value(prepare(manager,job,expected,observe));}catch(...){result.set_exception(std::current_exception());}if(notify)try{notify();}catch(...) {}});}catch(...){future={};throw;}return true;
 }
 void cancel()noexcept{stopping=true;wake.notify_all();}
 FileInfoRefreshResult take(){if(!ready())throw std::runtime_error("Refreshed download details are still being prepared.");if(worker.joinable())worker.join();return future.get();}
 void finish()noexcept{cancel();if(worker.joinable())worker.join();future={};}
};
#ifdef UDM_TOOLBAR_COMPONENT_TEST
inline FileInfoRefreshObserver fileInfoRefreshTestObserver;
// Form is intentionally not referenced here; UI declares its render observer.
#endif
}
