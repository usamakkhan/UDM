#pragma once
#include "QueueLabels.hpp"
#include <future>
#include <condition_variable>
namespace udm {
struct QueueChoiceView {std::vector<std::string> names;QueueLabels labels;};
struct QueueChoiceResult {QueueChoiceView view;bool cancelled=false;std::string error;};
using QueueChoiceObserver=std::function<void(const char*)>;
// Read-only selector preparation. Names and labels belong to one catalog cohort;
// the eventual command remains responsible for validating and saving its choice.
class QueueChoiceTask {
 std::future<QueueChoiceResult> future;std::thread worker;std::atomic_bool cancelled=false;std::mutex mutex;std::condition_variable wake;
public:
 QueueChoiceTask()=default;QueueChoiceTask(const QueueChoiceTask&)=delete;QueueChoiceTask& operator=(const QueueChoiceTask&)=delete;~QueueChoiceTask(){finish();}
 bool valid()const noexcept{return future.valid();}bool ready()const{return valid()&&future.wait_for(std::chrono::seconds(0))==std::future_status::ready;}
 void start(Manager& manager,std::function<void()> notify={},QueueChoiceObserver observe={}){
  if(valid()||worker.joinable())throw std::runtime_error("The queue list is already being prepared.");cancelled=false;std::promise<QueueChoiceResult> promise;future=promise.get_future();
  try{worker=std::thread([this,&manager,notify=std::move(notify),observe=std::move(observe),promise=std::move(promise)]()mutable{
   QueueChoiceResult result;
   try{for(;;){
    if(cancelled){result.cancelled=true;break;}
    std::unique_lock lock(manager.mutex,std::try_to_lock);
    if(lock.owns_lock()&&!manager.fileInfoDecisionPending){
     if(observe)observe("before-read");const auto& queues=manager.state["Queues"];
     for(const auto& queue:queues)result.view.names.push_back(str(queue,"Name"));result.view.labels=queueDisplayLabels(queues);break;
    }
    if(lock.owns_lock())lock.unlock();if(observe)observe("waiting-model");std::unique_lock waiting(mutex);wake.wait_for(waiting,std::chrono::milliseconds(10),[&]{return cancelled.load();});
   }result.cancelled=result.cancelled||cancelled.load();if(observe&&!result.cancelled)observe("captured");}
   catch(const std::exception& error){result.error=error.what();result.cancelled=cancelled;}catch(...){result.error="The queue list could not be read. Try again.";result.cancelled=cancelled;}
   promise.set_value(std::move(result));if(notify)try{notify();}catch(...){}
  });}catch(...){future={};throw;}
 }
 void cancelRead()noexcept{cancelled=true;wake.notify_all();}
 QueueChoiceResult take(){if(!ready())throw std::runtime_error("The queue list is still being prepared.");auto result=future.get();if(worker.joinable())worker.join();return result;}
 void finish()noexcept{cancelRead();if(worker.joinable())worker.join();future={};}
};
#ifdef UDM_TOOLBAR_COMPONENT_TEST
inline QueueChoiceObserver queueChoiceTestObserver;
#endif
}
