#pragma once
#include <atomic>
#include <chrono>
#include <condition_variable>
#include <exception>
#include <memory>
#include <mutex>
#include <stdexcept>
#include <thread>
#include <utility>
namespace udm {
inline thread_local const void* currentModelWorkOwner=nullptr;
inline thread_local const void* currentModelWorkIdentity=nullptr;
// Manager retains this owner before launch and joins it before its final
// checkpoint/destruction. Releasing a window never detaches the thread.
class ManagedModelWork:public std::enable_shared_from_this<ManagedModelWork> {
 std::thread worker;std::mutex lifecycle,waiting;std::condition_variable wake;
 std::atomic_bool cancelled=false,completed=false,wasJoined=false;
 bool retired=false,started=false;const void* owner=nullptr;
public:
 ManagedModelWork()=default;ManagedModelWork(const ManagedModelWork&)=delete;ManagedModelWork& operator=(const ManagedModelWork&)=delete;
 ~ManagedModelWork(){if(worker.joinable())std::terminate();}
 void bindOwner(const void* value){std::lock_guard<std::mutex> lock(lifecycle);if(started||retired||!value||(owner&&owner!=value))throw std::logic_error("The model worker cannot change owners.");owner=value;}
 template<class Action>void start(Action action){
  std::lock_guard<std::mutex> lock(lifecycle);if(started||retired||!owner)throw std::logic_error("The model worker cannot be started.");started=true;
  try{auto self=shared_from_this();worker=std::thread([self,action=std::move(action)]()mutable{
   currentModelWorkOwner=self->owner;currentModelWorkIdentity=self.get();
   try{action();}catch(...){/* A captured promise reports an unexpected failure. */}
   // This dedicated thread keeps its identity through capture destruction.
   self->completed=true;
  });}catch(...){retired=true;wasJoined=true;completed=true;throw;}
 }
 void cancel()noexcept{{std::lock_guard<std::mutex> lock(waiting);cancelled=true;}wake.notify_all();}
 bool isCancelled()const noexcept{return cancelled.load();}
 bool isComplete()const noexcept{return completed.load();}
 bool joined()const noexcept{return wasJoined.load();}
 void waitForDecision(){std::unique_lock<std::mutex> lock(waiting);wake.wait_for(lock,std::chrono::milliseconds(10),[&]{return cancelled.load();});}
 void join(){
  if(currentModelWorkIdentity==this)throw std::logic_error("A model worker cannot join itself.");
  std::lock_guard<std::mutex> lock(lifecycle);retired=true;if(worker.joinable())worker.join();wasJoined=true;completed=true;
 }
 static bool executingFor(const void* value)noexcept{return currentModelWorkOwner==value;}
};
}
