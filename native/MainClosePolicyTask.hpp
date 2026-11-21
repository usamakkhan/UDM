#pragma once
#include "QueueChoiceTask.hpp"
namespace udm {
struct MainClosePolicyResult {bool closeToTray=false,cancelled=false;std::string error;};
class MainClosePolicyTask {
 std::future<MainClosePolicyResult> future;std::thread worker;std::atomic_bool cancelled=false;std::mutex mutex;std::condition_variable wake;
public:
 MainClosePolicyTask()=default;MainClosePolicyTask(const MainClosePolicyTask&)=delete;MainClosePolicyTask& operator=(const MainClosePolicyTask&)=delete;~MainClosePolicyTask(){finish();}
 bool valid()const noexcept{return future.valid();}bool ready()const{return valid()&&future.wait_for(std::chrono::seconds(0))==std::future_status::ready;}
 void start(Manager& manager,std::function<void()> notify={},std::function<void(const char*)> observe={}){
  if(valid()||worker.joinable())throw std::runtime_error("A close decision is already pending.");cancelled=false;std::promise<MainClosePolicyResult> promise;future=promise.get_future();
  try{worker=std::thread([this,&manager,notify=std::move(notify),observe=std::move(observe),promise=std::move(promise)]()mutable{
   MainClosePolicyResult result;
   try{for(;;){if(cancelled){result.cancelled=true;break;}std::unique_lock lock(manager.mutex,std::try_to_lock);
    if(lock.owns_lock()&&!manager.fileInfoDecisionPending){if(observe)observe("before-read");result.closeToTray=yes(manager.state["Settings"],"CloseToTray");lock.unlock();if(observe)observe("captured");break;}
    if(lock.owns_lock())lock.unlock();if(observe)observe("waiting-model");std::unique_lock waiting(mutex);wake.wait_for(waiting,std::chrono::milliseconds(10),[&]{return cancelled.load();});
   }}catch(const std::exception& error){result.error=error.what();}catch(...){result.error="Download settings could not be read. Try closing again.";}
   result.cancelled=result.cancelled||cancelled.load();promise.set_value(std::move(result));if(notify)try{notify();}catch(...){}
  });}catch(...){future={};throw;}
 }
 void cancelRead()noexcept{cancelled=true;wake.notify_all();}
 MainClosePolicyResult take(){if(!ready())throw std::runtime_error("The close decision is not ready.");auto result=future.get();if(worker.joinable())worker.join();return result;}
 void finish()noexcept{cancelRead();if(worker.joinable())worker.join();future={};}
};
}
