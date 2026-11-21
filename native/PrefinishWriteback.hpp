#pragma once
#include <algorithm>
#include <atomic>
#include <chrono>
#include <cmath>
#include <condition_variable>
#include <cstdint>
#include <functional>
#include <limits>
#include <mutex>
#include <thread>
namespace udm {
// Opportunistic writeback only; the final publication flush remains required.
class PrefinishWriteback {
public:
 struct Stats {
  unsigned flushes=0,errors=0,adjustments=0,notifications=0;
  double seconds=0,longestFlushSeconds=0,finishWaitSeconds=0;
  int64_t leadBytes=0;
  bool inFlightAtFinish=false,discardedPending=false;
 };
private:
 friend struct PrefinishWritebackTestAccess;
 std::mutex mutex;std::condition_variable changed;
 bool stopping=false,pending=false,inFlight=false,firstQueued=false,secondQueued=false,thirdQueued=false;
 const bool singleFlush,paced;
 int64_t total,first,minimumLead,second,third;std::atomic<int64_t> next,latest{0};
 const std::chrono::steady_clock::time_point started=std::chrono::steady_clock::now();
 std::function<bool()> flush;Stats stats;std::thread worker;
 void run()noexcept{
  for(;;){
   std::unique_lock<std::mutex> lock(mutex);changed.wait(lock,[&]{return stopping||pending;});if(stopping)return;
   pending=false;inFlight=true;lock.unlock();const auto began=std::chrono::steady_clock::now();bool ok=false;try{ok=flush();}catch(...){}
   const auto now=std::chrono::steady_clock::now();const auto elapsed=std::chrono::duration<double>(now-began).count();
   lock.lock();inFlight=false;++stats.flushes;if(!ok)++stats.errors;stats.seconds+=elapsed;stats.longestFlushSeconds=std::max(stats.longestFlushSeconds,elapsed);
   // A slow first flush needs more runway than a fixed final 2 MiB. Estimate
   // from credited progress and measured IO; move the last request earlier,
   // never before half-body and never add another operation.
   if(stats.flushes==1&&!(paced?thirdQueued:secondQueued)&&elapsed>=0.05&&total>0){
    const auto age=std::chrono::duration<double>(now-started).count();
    const auto received=std::clamp<int64_t>(latest.load(std::memory_order_relaxed),0,total);
    if(age>0&&received>0){
     const long double needed=std::min<long double>(total/2,std::max<long double>(minimumLead,static_cast<long double>(received)/age*elapsed*2+minimumLead));
     const auto earlier=std::max(paced?second:first,total-static_cast<int64_t>(std::ceil(needed)));
     auto& last=paced?third:second;
     if(earlier<last){last=earlier;++stats.adjustments;stats.leadBytes=total-last;next.store(!firstQueued?first:!secondQueued?second:paced&&!thirdQueued?third:std::numeric_limits<int64_t>::max(),std::memory_order_relaxed);}
    }
   }
  }
 }
public:
 PrefinishWriteback(int64_t bodySize,unsigned connections,std::function<bool()> operation,bool paceSequential=false,bool paceSplit=false):singleFlush(paceSplit&&connections>1&&connections<=8&&bodySize>=32*1048576LL&&bodySize<=64*1048576LL),paced(paceSequential&&connections==1&&bodySize>=32*1048576LL&&bodySize<=64*1048576LL),total(bodySize),first(bodySize/(singleFlush?8:paced?4:2)),minimumLead(paced?std::min<int64_t>(bodySize/4,4*1048576LL):std::min<int64_t>(bodySize/8,2*1048576LL*std::max(1u,connections))),second(paced?bodySize/2:std::max(first,bodySize-minimumLead)),third(std::max(second,bodySize-minimumLead)),next(first),flush(std::move(operation)){
  // Split pacing has exactly one optional request, with most of the body
  // still in flight. It cannot grow into repeated concurrent range flushes.
  secondQueued=singleFlush;stats.leadBytes=total-(singleFlush?first:paced?third:second);worker=std::thread([this]{run();});
 }
 PrefinishWriteback(const PrefinishWriteback&)=delete;PrefinishWriteback& operator=(const PrefinishWriteback&)=delete;
 ~PrefinishWriteback(){finish();}
 void progress(int64_t received){
  latest.store(received,std::memory_order_relaxed);if(received<next.load(std::memory_order_relaxed))return;
  {std::lock_guard<std::mutex> lock(mutex);if(stopping)return;if(!firstQueued&&received>=first){firstQueued=true;pending=true;}if(!secondQueued&&received>=second){secondQueued=true;pending=true;}if(paced&&!thirdQueued&&received>=third){thirdQueued=true;pending=true;}next.store(!firstQueued?first:!secondQueued?second:paced&&!thirdQueued?third:std::numeric_limits<int64_t>::max(),std::memory_order_relaxed);}
  changed.notify_one();
 }
 Stats finish(){
  const auto began=std::chrono::steady_clock::now();const bool joining=worker.joinable();
  {std::lock_guard<std::mutex> lock(mutex);if(joining){stats.inFlightAtFinish=inFlight;stats.discardedPending=pending;}stopping=true;}
  changed.notify_one();if(joining)worker.join();
  std::lock_guard<std::mutex> lock(mutex);
  if(joining)stats.finishWaitSeconds=std::chrono::duration<double>(std::chrono::steady_clock::now()-began).count();
  return stats;
 }
};
}
