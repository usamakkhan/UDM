#pragma once
#include <chrono>
#include <condition_variable>
#include <functional>
#include <mutex>
#include <thread>
#include <utility>

namespace udm {
// Owned worker: one checkpoint at a time, independently of the desktop timer.
// The owner serializes start/stop and keeps the callback's state alive until stop.
class PeriodicCheckpoint {
 std::mutex mutex;
 std::condition_variable wake;
 std::function<void()> action;
 std::thread worker;
 bool stopping=false;
 void run(){
  for(;;){
   {std::unique_lock<std::mutex> lock(mutex);
    if(wake.wait_for(lock,std::chrono::milliseconds(1500),[&]{return stopping;}))return;}
   // The callback reports storage failures through the owner's existing channel.
   try{action();}catch(...){}
  }
 }
public:
 explicit PeriodicCheckpoint(std::function<void()> task):action(std::move(task)){}
 PeriodicCheckpoint(const PeriodicCheckpoint&)=delete;
 PeriodicCheckpoint& operator=(const PeriodicCheckpoint&)=delete;
 ~PeriodicCheckpoint(){stop();}
 void start(){
  std::lock_guard<std::mutex> lock(mutex);
  if(stopping)return;
  if(!worker.joinable())worker=std::thread([this]{run();});
 }
 void stop(){
  {std::lock_guard<std::mutex> lock(mutex);stopping=true;}
  wake.notify_one();
  if(worker.joinable())worker.join();
  // A failed owner shutdown may resume normal operation and start again.
  {std::lock_guard<std::mutex> lock(mutex);stopping=false;}
 }
};
}
