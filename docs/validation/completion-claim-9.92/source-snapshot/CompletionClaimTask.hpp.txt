#pragma once
#include "Core.hpp"
#include <future>
#define UDM_COMPLETION_CLAIM_TASK_AVAILABLE
namespace udm {
struct CompletionClaimResult {bool saved=false;Json action=Json::object();std::string error;};
// A durable at-most-once claim, never a shell/power action. Once accepted,
// the save outlives its dialog and must be joined before Manager destruction.
class CompletionClaimTask {
 std::future<CompletionClaimResult> future;std::thread worker;
public:
 CompletionClaimTask()=default;CompletionClaimTask(const CompletionClaimTask&)=delete;
 CompletionClaimTask& operator=(const CompletionClaimTask&)=delete;~CompletionClaimTask(){finish();}
 bool valid()const noexcept{return future.valid();}
 bool ready()const{return valid()&&future.wait_for(std::chrono::seconds(0))==std::future_status::ready;}
 void start(Manager& manager,JobPtr job,std::function<void()> notify={},std::function<void(const char*)> observe={}){
  if(valid()||worker.joinable())throw std::runtime_error("A completion action is already being prepared.");
  std::promise<CompletionClaimResult> promise;future=promise.get_future();
  try{worker=std::thread([&manager,job=std::move(job),notify=std::move(notify),observe=std::move(observe),promise=std::move(promise)]()mutable{
   CompletionClaimResult result;
   try{if(observe)observe("accepted");for(;;){
    std::unique_lock lock(manager.mutex,std::try_to_lock);
    if(lock.owns_lock()&&!manager.fileInfoDecisionPending){if(observe)observe("before-claim");result.action=manager.takeDownloadCompletion(job);result.saved=true;break;}
    if(lock.owns_lock())lock.unlock();if(observe)observe("waiting-model");Sleep(10);
   }}
   catch(const std::exception& error){result.error=error.what();}catch(...){result.error="The completion action could not be prepared.";}
   // takeDownloadCompletion has released the catalog before result publication.
   promise.set_value(std::move(result));if(notify)try{notify();}catch(...){}
  });}catch(...){future={};throw;}
 }
 CompletionClaimResult take(){if(!ready())throw std::runtime_error("The completion action is not ready.");auto result=future.get();if(worker.joinable())worker.join();return result;}
 CompletionClaimResult finish()noexcept{if(worker.joinable())worker.join();CompletionClaimResult result;if(future.valid())try{result=future.get();}catch(...){}future={};return result;}
};
}
