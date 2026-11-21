#pragma once
#include <condition_variable>
#include <cstdint>
#include <functional>
#include <mutex>
#include <stdexcept>
#include <string>
#include <string_view>
#include <thread>
#include <utility>
namespace udm {
// One owned, lazy worker. The UI thread owns request/stop/restart; callbacks
// touch only the model. Notification is an owned integer-only wake-up.
class ModelMaintenance {
public:
 struct Result {
  uint64_t ticks=0,dispatches=0,tickFailures=0,dispatchFailures=0;
  bool active=false,tickPending=false,dispatchPending=false,stopping=false;
  std::string tickError,dispatchError;
 };
private:
 std::mutex mutex;std::condition_variable wake;std::thread worker;
 std::function<void()> tick,dispatch,notify;Result result;
 void perform(bool isTick)noexcept{
  {std::lock_guard<std::mutex> lock(mutex);if(result.stopping)return;result.active=true;}
  bool failed=false;std::string error;
  try{(isTick?tick:dispatch)();}
  catch(const std::exception& e){failed=true;try{error=std::string_view(e.what()).substr(0,4096);}catch(...) {}}
  catch(...){failed=true;try{error="Model maintenance failed.";}catch(...) {}}
  {std::lock_guard<std::mutex> lock(mutex);result.active=false;
   if(isTick){++result.ticks;if(failed){++result.tickFailures;result.tickError=std::move(error);}}
   else{++result.dispatches;if(failed){++result.dispatchFailures;result.dispatchError=std::move(error);}}
  }
  try{if(notify)notify();}catch(...) {/* A failed UI wake-up cannot stop model work. */}
 }
 void run()noexcept{
  for(;;){
   bool tickRequested=false,dispatchRequested=false;
   {std::unique_lock<std::mutex> lock(mutex);wake.wait(lock,[&]{return result.stopping||result.tickPending||result.dispatchPending;});if(result.stopping)return;
    tickRequested=std::exchange(result.tickPending,false);dispatchRequested=std::exchange(result.dispatchPending,false);
   }
   if(dispatchRequested)perform(false);if(tickRequested)perform(true);
  }
 }
 bool request(bool isTick){
  {std::lock_guard<std::mutex> lock(mutex);if(result.stopping)return false;
   if(!worker.joinable())worker=std::thread([this]{run();});
   (isTick?result.tickPending:result.dispatchPending)=true;
  }
  wake.notify_one();return true;
 }
public:
 ModelMaintenance(std::function<void()> onTick,std::function<void()> onDispatch,std::function<void()> onReady={}):tick(std::move(onTick)),dispatch(std::move(onDispatch)),notify(std::move(onReady)){}
 ModelMaintenance(const ModelMaintenance&)=delete;ModelMaintenance& operator=(const ModelMaintenance&)=delete;
 ~ModelMaintenance(){stop();}
 bool requestTick(){return request(true);}
 bool requestDispatch(){return request(false);}
 Result takeResult(){
  std::lock_guard<std::mutex> lock(mutex);Result taken; taken.ticks=result.ticks;taken.dispatches=result.dispatches;taken.tickFailures=result.tickFailures;taken.dispatchFailures=result.dispatchFailures;
  taken.active=result.active;taken.tickPending=result.tickPending;taken.dispatchPending=result.dispatchPending;taken.stopping=result.stopping;
  taken.tickError=std::move(result.tickError);taken.dispatchError=std::move(result.dispatchError);result.tickError.clear();result.dispatchError.clear();return taken;
 }
 void stop(){
  {std::lock_guard<std::mutex> lock(mutex);result.stopping=true;result.tickPending=false;result.dispatchPending=false;}
  wake.notify_one();if(worker.joinable())worker.join();
 }
 // A failed durable owner shutdown may resume the same live Manager. This is
 // serialized by the UI owner, after stop has joined all old callbacks.
 void restart(){std::lock_guard<std::mutex> lock(mutex);if(worker.joinable())throw std::logic_error("Stop model maintenance before restarting it.");result.stopping=false;}
};
}
