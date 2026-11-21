#pragma once
#include "GlobalLimiter.hpp"
#include <future>
#include <condition_variable>
#define UDM_GLOBAL_LIMIT_SETTINGS_TASK_AVAILABLE
#define UDM_GLOBAL_LIMIT_TOGGLE_AVAILABLE
namespace udm {
// enabled is used only by On/Off actions; these retain the live rate and startup choice.
struct GlobalLimitSettingsDraft {i64 rate=1000;bool startup=false;std::optional<bool> enabled;};
struct GlobalLimitSettingsResult {Json settings;bool saved=false,cancelled=false;std::string error;};
inline void validateGlobalLimitSettings(const GlobalLimitSettingsDraft& draft){if(draft.rate<1||draft.rate>1000000)throw std::runtime_error("Enter a speed limit from 1 to 1,000,000 KB/s.");}
// Own a cancellable read or an accepted, joined settings save. Never capture a
// window/control; merge only the limiter fields into current settings on Save.
class GlobalLimitSettingsTask {
 std::future<GlobalLimitSettingsResult> future;std::thread worker;Manager* guard=nullptr;std::atomic_bool cancelled=false;std::mutex mutex;std::condition_variable wake;
 void releaseGuard()noexcept{if(guard){guard->fileInfoDecisionPending=false;guard=nullptr;}}
public:
 GlobalLimitSettingsTask()=default;GlobalLimitSettingsTask(const GlobalLimitSettingsTask&)=delete;GlobalLimitSettingsTask& operator=(const GlobalLimitSettingsTask&)=delete;~GlobalLimitSettingsTask(){finish();}
 bool valid()const noexcept{return future.valid();}bool ready()const{return valid()&&future.wait_for(std::chrono::seconds(0))==std::future_status::ready;}bool saving()const noexcept{return guard!=nullptr;}
 void start(Manager& manager,std::optional<GlobalLimitSettingsDraft> draft={},std::function<void()> notify={},std::function<void(const char*)> observe={}){
  if(valid()||worker.joinable())throw std::runtime_error("Speed Limiter settings are already pending.");
  if(draft){validateGlobalLimitSettings(*draft);bool expected=false;if(!manager.fileInfoDecisionPending.compare_exchange_strong(expected,true))throw std::runtime_error("Another change is being saved. Try again shortly.");guard=&manager;}
  cancelled=false;try{std::promise<GlobalLimitSettingsResult> promise;future=promise.get_future();worker=std::thread([this,&manager,draft=std::move(draft),notify=std::move(notify),observe=std::move(observe),promise=std::move(promise)]()mutable{
   GlobalLimitSettingsResult result;
   try{for(;;){if(!draft&&cancelled){result.cancelled=true;break;}std::unique_lock lock(manager.mutex,std::try_to_lock);
    if(lock.owns_lock()&&(draft||!manager.fileInfoDecisionPending)){
     if(!draft&&cancelled){result.cancelled=true;break;}if(observe)observe(draft?"before-save":"before-read");
     if(draft){auto next=manager.state["Settings"];if(draft->enabled)next=globalLimiterSettings(std::move(next),*draft->enabled,rememberedGlobalLimit(manager.state["Settings"]));else{next=globalLimiterSettings(std::move(next),num(manager.state["Settings"],"LimitKbps")>0,draft->rate);next["GlobalLimitOnStartup"]=draft->startup;}result.settings=next;manager.setSettings(next);result.saved=true;}
     else result.settings=manager.state["Settings"];lock.unlock();if(observe)try{observe("captured");}catch(...){}break;
    }
    if(lock.owns_lock())lock.unlock();if(observe)observe(draft?"waiting-save":"waiting-read");std::unique_lock waiting(mutex);wake.wait_for(waiting,std::chrono::milliseconds(10),[&]{return !draft&&cancelled.load();});
   }}catch(const std::exception& error){result.error=error.what();}catch(...){result.error="Speed Limiter settings could not be processed.";}
   if(!draft&&cancelled)result.cancelled=true;promise.set_value(std::move(result));if(notify)try{notify();}catch(...){}
  });}catch(...){future={};releaseGuard();throw;}
 }
 void startToggle(Manager& manager,bool enabled,std::function<void()> notify={},std::function<void(const char*)> observe={}){start(manager,GlobalLimitSettingsDraft{1000,false,enabled},std::move(notify),std::move(observe));}
 void cancelRead()noexcept{cancelled=true;wake.notify_all();}
 GlobalLimitSettingsResult take(){if(!ready())throw std::runtime_error("Speed Limiter settings are not ready.");auto result=future.get();if(worker.joinable())worker.join();releaseGuard();return result;}
 GlobalLimitSettingsResult finish()noexcept{
  cancelRead();if(worker.joinable())worker.join();GlobalLimitSettingsResult result;if(future.valid())try{result=future.get();}catch(...){/* No accepted result is acknowledged on an unexpected future failure. */}future={};releaseGuard();return result;
 }
};
}
