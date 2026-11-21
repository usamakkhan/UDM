#pragma once
#include "FileInfoInitialView.hpp"
#include "SiteLogins.hpp"
#include <future>
#include <condition_variable>
namespace udm {
struct FileInfoInitialResult {
 std::shared_ptr<const FileInfoInitialView> view;std::string address;
 std::pair<std::string,std::string> login;bool basic=false,cancelled=false;
};
using FileInfoInitialObserver=std::function<void(const char*)>;
class FileInfoInitialTask {
 std::atomic_bool stopping=false;std::mutex mutex;std::condition_variable wake;
 std::future<FileInfoInitialResult> future;std::thread worker;
 FileInfoInitialResult read(Manager& manager,const JobPtr& job,const std::shared_ptr<const FileInfoInitialView>& prepared,const FileInfoInitialObserver& observe){
  FileInfoInitialResult result;
  for(;;){
   if(stopping){result.cancelled=true;return result;}
   std::unique_lock lock(manager.mutex,std::try_to_lock);
   if(lock.owns_lock()&&!manager.fileInfoDecisionPending){
    if(observe)observe("before-read");
    if(std::find(manager.jobs.begin(),manager.jobs.end(),job)==manager.jobs.end())throw std::runtime_error("This download is no longer in the list.");
    const auto current=captureFileInfoInitialView(manager,*job);
    result.view=prepared&&sameFileInfoInitialView(*prepared,*current)?prepared:current;
    break;
   }
   if(lock.owns_lock())lock.unlock();
   if(observe)observe("waiting-model");
   std::unique_lock waiting(mutex);wake.wait_for(waiting,std::chrono::milliseconds(10),[&]{return stopping.load();});
  }
  if(stopping){result.view.reset();result.cancelled=true;return result;}
  // Address and remembered credentials are private owned values, resolved after
  // releasing Manager. None of this work reads controls or serializes secrets.
  result.address=downloadAddress(result.view->addressInputs);
  if(!result.view->media){const auto headers=siteRequestHeaders(str(result.view->data,"Url"),readHeaders(result.view->data),result.view->preferences);result.login=basicLogin(headers);result.basic=lower(headerValue(headers,"Authorization")).rfind("basic ",0)==0;}
  if(observe)observe("captured");return result;
 }
public:
 FileInfoInitialTask()=default;FileInfoInitialTask(const FileInfoInitialTask&)=delete;FileInfoInitialTask& operator=(const FileInfoInitialTask&)=delete;
 ~FileInfoInitialTask(){finish();}
 bool valid()const noexcept{return future.valid();}
 bool ready()const{return future.valid()&&future.wait_for(std::chrono::seconds(0))==std::future_status::ready;}
 void start(Manager& manager,JobPtr job,std::shared_ptr<const FileInfoInitialView> prepared={},std::function<void()> notify={},FileInfoInitialObserver observe={}){
  if(valid())throw std::runtime_error("Initial download details are already being read.");if(!job)throw std::invalid_argument("Missing download for File Info.");
  stopping=false;std::promise<FileInfoInitialResult> result;future=result.get_future();
  try{worker=std::thread([this,&manager,job=std::move(job),prepared=std::move(prepared),notify=std::move(notify),observe=std::move(observe),result=std::move(result)]()mutable{try{result.set_value(read(manager,job,prepared,observe));}catch(...){result.set_exception(std::current_exception());}if(notify)try{notify();}catch(...) {}});}catch(...){future={};throw;}
 }
 void cancel()noexcept{stopping=true;wake.notify_all();}
 FileInfoInitialResult take(){if(!ready())throw std::runtime_error("Initial download details are still being read.");if(worker.joinable())worker.join();return future.get();}
 void finish()noexcept{cancel();if(worker.joinable())worker.join();future={};}
};
#ifdef UDM_TOOLBAR_COMPONENT_TEST
inline FileInfoInitialObserver fileInfoInitialTestObserver;
#endif
}
