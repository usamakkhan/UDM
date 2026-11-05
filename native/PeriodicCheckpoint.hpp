#pragma once
#include <condition_variable>
#include <functional>
#include <mutex>
#include <thread>
#include <utility>

namespace udm {
// Owned worker: one running checkpoint and at most one pending request.
// The owner serializes request/stop and keeps the callback's state alive until stop.
class PeriodicCheckpoint {
 std::mutex mutex;
 std::condition_variable wake;
 std::function<void()> action;
 std::thread worker;
 bool stopping=false,pending=false;
 void run(){
  for(;;){
   {std::unique_lock<std::mutex> lock(mutex);
    wake.wait(lock,[&]{return stopping||pending;});
    if(stopping)return;
    pending=false;}
   // The callback reports storage failures through the owner's existing channel.
   try{action();}catch(...){}
  }
 }
public:
 explicit PeriodicCheckpoint(std::function<void()> task):action(std::move(task)){}
 PeriodicCheckpoint(const PeriodicCheckpoint&)=delete;
 PeriodicCheckpoint& operator=(const PeriodicCheckpoint&)=delete;
 ~PeriodicCheckpoint(){stop();}
 void request(){
  std::lock_guard<std::mutex> lock(mutex);
  if(stopping)return;
  pending=true;
  if(!worker.joinable())worker=std::thread([this]{run();});
  wake.notify_one();
 }
 void stop(){
  {std::lock_guard<std::mutex> lock(mutex);stopping=true;pending=false;}
  wake.notify_one();
  if(worker.joinable())worker.join();
  // A failed owner shutdown may resume normal operation and request again.
  {std::lock_guard<std::mutex> lock(mutex);stopping=false;}
 }
};
}
