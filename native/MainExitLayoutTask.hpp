#pragma once
#include "QueueChoiceTask.hpp"
#include "ManagedModelWork.hpp"
#define UDM_MAIN_EXIT_LAYOUT_AVAILABLE
namespace udm {
// Caller owns Manager. Merge just the UI-owned layout, preserving concurrent
// settings and leaving persistence to the existing final Manager checkpoint.
inline void mergeMainExitLayout(Manager& manager,Json layout){
 if(!layout.is_object())throw std::invalid_argument("The download layout is invalid.");
 manager.state.at("Settings")["ListLayout"]=std::move(layout);
}
struct MainExitLayoutResult {bool applied=false,cancelled=false;std::string error;};
class MainExitLayoutTask {
 std::future<MainExitLayoutResult> future;
 std::shared_ptr<ManagedModelWork> operation;std::shared_ptr<std::atomic_bool> accepted;Manager* owner=nullptr;
public:
 MainExitLayoutTask()=default;MainExitLayoutTask(const MainExitLayoutTask&)=delete;MainExitLayoutTask& operator=(const MainExitLayoutTask&)=delete;~MainExitLayoutTask(){finish();}
 bool valid()const noexcept{return future.valid();}
 bool ready()const{return valid()&&future.wait_for(std::chrono::seconds(0))==std::future_status::ready;}
 void start(Manager& manager,Json layout,std::function<void()> notify={},std::function<void(const char*)> observe={}){
  if(valid()||std::atomic_load(&operation))throw std::runtime_error("Exit layout preparation is already pending.");
  auto work=std::make_shared<ManagedModelWork>();auto applied=std::make_shared<std::atomic_bool>(false);std::promise<MainExitLayoutResult> promise;future=promise.get_future();owner=&manager;accepted=applied;std::atomic_store(&operation,work);
  try{manager.retainModelWork(work);work->start([&manager,work,applied,layout=std::move(layout),notify=std::move(notify),observe=std::move(observe),promise=std::move(promise)]()mutable{
   MainExitLayoutResult result;
   try{for(;;){
    if(work->isCancelled()){result.cancelled=true;break;}
    if(observe)observe("waiting-model");
    // The Manager owns this wait, including after its window is gone. Normal
    // mutex contention adds no retry timer; cancellation is checked on entry.
    std::unique_lock lock(manager.mutex);
    if(work->isCancelled()){result.cancelled=true;break;}
    if(manager.fileInfoDecisionPending){lock.unlock();work->waitForDecision();continue;}
    if(observe)observe("before-merge");if(work->isCancelled()){result.cancelled=true;break;}
    mergeMainExitLayout(manager,std::move(layout));result.applied=true;*applied=true;lock.unlock();if(observe)observe("merged");break;
   }}catch(const std::exception& error){result.error=error.what();}catch(...){result.error="The download layout could not be prepared for Exit.";}
   result.cancelled=!result.applied&&(result.cancelled||work->isCancelled());promise.set_value(std::move(result));if(notify)try{notify();}catch(...){}
  });}catch(...){work->cancel();work->join();manager.releaseModelWork(work);owner=nullptr;accepted.reset();std::atomic_store(&operation,std::shared_ptr<ManagedModelWork>{});future={};throw;}
 }
 void cancelRead()noexcept{if(auto work=std::atomic_load(&operation))work->cancel();}
 MainExitLayoutResult take(){
  if(!ready())throw std::runtime_error("The Exit layout is not ready.");auto work=std::atomic_load(&operation);MainExitLayoutResult result;
  try{result=future.get();}catch(const std::exception& error){result.applied=accepted&&accepted->load();result.error=error.what();}
  work->join();owner->releaseModelWork(work);owner=nullptr;accepted.reset();std::atomic_store(&operation,std::shared_ptr<ManagedModelWork>{});return result;
 }
 void finish()noexcept{
  auto work=std::atomic_exchange(&operation,std::shared_ptr<ManagedModelWork>{});if(work){work->cancel();if(work->isComplete())try{work->join();if(owner)owner->releaseModelWork(work);}catch(...){/* Manager still owns any unreleased work. */}}
  // A blocked acquisition has no window references. Manager retains it and
  // joins it before checkpoint/destruction, rather than detaching the thread.
  owner=nullptr;accepted.reset();future={};
 }
};
}
