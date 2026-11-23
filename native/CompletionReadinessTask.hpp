#pragma once
#include "Core.hpp"
#include <future>
#include <condition_variable>
#define UDM_COMPLETION_ADMISSION_AVAILABLE
namespace udm {
struct CompletionReadinessResult {bool cancelled=false;std::string error;};
// A cancellable availability wake-up only. Presentation data is read afresh
// on the UI thread using try_lock; this worker never consumes an action.
class CompletionReadinessTask {
 std::future<CompletionReadinessResult> future;std::thread worker;std::atomic_bool cancelled=false;std::mutex mutex;std::condition_variable wake;
public:
 CompletionReadinessTask()=default;CompletionReadinessTask(const CompletionReadinessTask&)=delete;CompletionReadinessTask& operator=(const CompletionReadinessTask&)=delete;~CompletionReadinessTask(){finish();}
 bool valid()const noexcept{return future.valid();}bool ready()const{return valid()&&future.wait_for(std::chrono::seconds(0))==std::future_status::ready;}
 void start(Manager& manager,std::function<void()> notify={},std::function<void(const char*)> observe={}){
  if(valid()||worker.joinable())throw std::runtime_error("Completion admission is already waiting.");cancelled=false;std::promise<CompletionReadinessResult> promise;future=promise.get_future();
  try{worker=std::thread([this,&manager,notify=std::move(notify),observe=std::move(observe),promise=std::move(promise)]()mutable{
   CompletionReadinessResult result;
   try{for(;;){if(cancelled){result.cancelled=true;break;}std::unique_lock lock(manager.mutex,std::try_to_lock);
    if(lock.owns_lock()&&!manager.fileInfoDecisionPending){lock.unlock();if(observe)observe("available");break;}
    if(lock.owns_lock())lock.unlock();if(observe)observe("waiting-model");std::unique_lock waiting(mutex);wake.wait_for(waiting,std::chrono::milliseconds(10),[&]{return cancelled.load();});
   }}catch(const std::exception& error){result.error=error.what();}catch(...){result.error="Completion admission could not be prepared.";}
   result.cancelled=result.cancelled||cancelled.load();promise.set_value(std::move(result));if(notify)try{notify();}catch(...){}
  });}catch(...){future={};throw;}
 }
 void cancelRead()noexcept{cancelled=true;wake.notify_all();}
 CompletionReadinessResult take(){if(!ready())throw std::runtime_error("Completion admission is not ready.");auto result=future.get();if(worker.joinable())worker.join();return result;}
 CompletionReadinessResult finish()noexcept{cancelRead();if(worker.joinable())worker.join();CompletionReadinessResult result;if(future.valid())try{result=future.get();}catch(...){}future={};return result;}
};
}
