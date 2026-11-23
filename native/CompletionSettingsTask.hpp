#pragma once
#include "Core.hpp"
#include <future>
#define UDM_COMPLETION_SUPPRESSION_TASK_AVAILABLE
namespace udm {
struct CompletionSettingsResult {bool saved=false;std::optional<bool> persisted;std::string error;};
// Accepted saves outlive their originating dialog. Only the requested field
// is merged under the live model lock; no window or control enters the worker.
class CompletionSettingsTask {
 std::future<CompletionSettingsResult> future;std::thread worker;Manager* guard=nullptr;
 void release()noexcept{if(guard){guard->fileInfoDecisionPending=false;guard=nullptr;}}
public:
 CompletionSettingsTask()=default;CompletionSettingsTask(const CompletionSettingsTask&)=delete;CompletionSettingsTask& operator=(const CompletionSettingsTask&)=delete;~CompletionSettingsTask(){finish();}
 bool valid()const noexcept{return future.valid();}bool ready()const{return valid()&&future.wait_for(std::chrono::seconds(0))==std::future_status::ready;}
 void start(Manager& manager,bool suppressed,std::function<void()> notify={},std::function<void(const char*)> observe={}){
  if(valid()||worker.joinable())throw std::runtime_error("A completion preference is already being saved.");bool expected=false;if(!manager.fileInfoDecisionPending.compare_exchange_strong(expected,true))throw std::runtime_error("Another change is being saved. Try again shortly.");guard=&manager;
  try{std::promise<CompletionSettingsResult> promise;future=promise.get_future();worker=std::thread([&manager,suppressed,notify=std::move(notify),observe=std::move(observe),promise=std::move(promise)]()mutable{
   CompletionSettingsResult result;
   try{if(observe)observe("waiting-save");{Lock lock(manager.mutex);auto next=manager.state["Settings"];result.persisted=yes(next,"SuppressCompletionDialog");if(observe)observe("before-save");if(*result.persisted!=suppressed){next["SuppressCompletionDialog"]=suppressed;manager.setSettings(next);}result.persisted=suppressed;result.saved=true;}if(observe)try{observe("captured");}catch(...){}}
   catch(const std::exception& error){result.error=error.what();}catch(...){result.error="The completion preference could not be saved.";}
   promise.set_value(std::move(result));if(notify)try{notify();}catch(...){}
  });}catch(...){future={};release();throw;}
 }
 CompletionSettingsResult take(){if(!ready())throw std::runtime_error("The completion preference is not ready.");auto result=future.get();if(worker.joinable())worker.join();release();return result;}
 CompletionSettingsResult finish()noexcept{if(worker.joinable())worker.join();CompletionSettingsResult result;if(future.valid())try{result=future.get();}catch(...){}future={};release();return result;}
};
}
