#pragma once
#include "MainClosePolicyTask.hpp"
#include "CommandAvailability.hpp"
#include "QueueLabels.hpp"
namespace udm {
struct DownloadMenuView {
 std::vector<JobPtr> selection;
 std::map<UINT,CommandAvailability> commands;
 QueueLabels queueLabels;std::vector<std::string> queueNames;
 Json queues;std::vector<std::pair<bool,bool>> membership;
 std::string doubleClick;bool adding=false,obsolete=false;
};
struct DownloadMenuResult {DownloadMenuView view;bool cancelled=false;std::string error;};
// The reader owns no HWND. Capture only model data under Manager, then publish
// an owned value. Cancellation never waits to acquire the catalog mutex.
class DownloadMenuTask {
 std::future<DownloadMenuResult> future;std::thread worker;std::atomic_bool cancelled=false;
 std::mutex mutex;std::condition_variable wake;
public:
 DownloadMenuTask()=default;DownloadMenuTask(const DownloadMenuTask&)=delete;DownloadMenuTask& operator=(const DownloadMenuTask&)=delete;~DownloadMenuTask(){finish();}
 bool valid()const noexcept{return future.valid();}
 bool ready()const{return valid()&&future.wait_for(std::chrono::seconds(0))==std::future_status::ready;}
 void start(Manager& manager,std::function<DownloadMenuView()> read,std::function<void()> notify={}){
  if(valid()||worker.joinable())throw std::runtime_error("A download menu is already being prepared.");
  cancelled=false;std::promise<DownloadMenuResult> promise;future=promise.get_future();
  try{worker=std::thread([this,&manager,read=std::move(read),notify=std::move(notify),promise=std::move(promise)]()mutable{
   DownloadMenuResult result;
   try{for(;;){
    if(cancelled){result.cancelled=true;break;}
    std::unique_lock lock(manager.mutex,std::try_to_lock);
    if(lock.owns_lock()&&!manager.fileInfoDecisionPending){if(!cancelled)result.view=read();else result.cancelled=true;break;}
    if(lock.owns_lock())lock.unlock();
    std::unique_lock waiting(mutex);wake.wait_for(waiting,std::chrono::milliseconds(10),[&]{return cancelled.load();});
   }}catch(const std::exception& error){result.error=error.what();}catch(...){result.error="The download menu could not be prepared. Try again.";}
   result.cancelled=result.cancelled||cancelled.load();promise.set_value(std::move(result));if(notify)try{notify();}catch(...){}
  });}catch(...){future={};throw;}
 }
 void cancelRead()noexcept{cancelled=true;wake.notify_all();}
 DownloadMenuResult take(){if(!ready())throw std::runtime_error("The download menu is not ready.");auto result=future.get();if(worker.joinable())worker.join();return result;}
 void finish()noexcept{cancelRead();if(worker.joinable())worker.join();future={};}
};
}
