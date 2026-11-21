#pragma once
#include "PrefinishWriteback.hpp"
#include <windows.h>
#include <winternl.h>
namespace udm {
// Optional cache drainage, never a durability proof or source of range credit.
// The mandatory final FlushFileBuffers still writes metadata and synchronizes
// storage. Resolve dynamically so unsupported Windows/filesystems can decline.
using DataOnlyFlush=NTSTATUS (NTAPI*)(HANDLE,ULONG,PVOID,ULONG,PIO_STATUS_BLOCK);
inline DataOnlyFlush dataOnlyFlush()noexcept{
 static const auto call=reinterpret_cast<DataOnlyFlush>(GetProcAddress(GetModuleHandleW(L"ntdll.dll"),"NtFlushBuffersFileEx"));
 return call;
}
inline bool flushBodyData(DataOnlyFlush call,HANDLE file)noexcept{
 if(!call)return false;
 IO_STATUS_BLOCK status{};
 const auto result=call(file,1u,nullptr,0,&status); // FLUSH_FLAGS_FILE_DATA_ONLY
 return result>=0&&result!=0x103&&status.Status>=0&&status.Status!=0x103;
}
class DataOnlyWriteback {
public:
 using Stats=PrefinishWriteback::Stats;
 static constexpr int64_t triggerBytes=8*1048576LL;
private:
 friend struct DataOnlyWritebackTestAccess;
 std::mutex mutex;std::condition_variable changed;
 bool stopping=false,pending=false,inFlight=false,disabled=false;
 std::atomic<int64_t> latest{0},next{triggerBytes};int64_t drained=0,activeSnapshot=0;
 std::function<bool()> flush;Stats stats;std::thread worker;
 void run()noexcept{
  std::unique_lock<std::mutex> lock(mutex);
  for(;;){
   changed.wait(lock,[&]{return stopping||pending;});if(stopping)return;
   // Suppress progress-side locking while IO is active. Completion publishes
   // the next threshold before rechecking the atomic high-water mark, so a
   // concurrent final progress update cannot lose its wakeup.
   next.store(std::numeric_limits<int64_t>::max());
   const auto snapshot=latest.load();activeSnapshot=snapshot;pending=false;inFlight=true;
   lock.unlock();const auto began=std::chrono::steady_clock::now();bool ok=false;
   try{ok=flush();}catch(...){}
   const auto elapsed=std::chrono::duration<double>(std::chrono::steady_clock::now()-began).count();
   lock.lock();inFlight=false;++stats.flushes;stats.seconds+=elapsed;
   stats.longestFlushSeconds=std::max(stats.longestFlushSeconds,elapsed);
   if(ok)drained=std::max(drained,snapshot);else{++stats.errors;disabled=true;}
   // Bytes credited during an operation are not assumed drained by it. A
   // failed optional request disables further requests instead of retrying.
   next.store(stopping||disabled?std::numeric_limits<int64_t>::max():drained+triggerBytes);
   pending=!stopping&&!disabled&&latest.load()>=next.load();
  }
 }
public:
 explicit DataOnlyWriteback(std::function<bool()> operation):flush(std::move(operation)),worker([this]{run();}){}
 DataOnlyWriteback(const DataOnlyWriteback&)=delete;DataOnlyWriteback& operator=(const DataOnlyWriteback&)=delete;
 ~DataOnlyWriteback(){finish();}
 void progress(int64_t written){
  auto seen=latest.load();while(seen<written&&!latest.compare_exchange_weak(seen,written)){}
  if(std::max(seen,written)<next.load())return;
  bool wake=false;
  {std::lock_guard<std::mutex> lock(mutex);if(stopping||disabled)return;
   if(latest.load()>=drained+triggerBytes){wake=!pending&&!inFlight;pending=true;if(wake)++stats.notifications;}}
  if(wake)changed.notify_one();
 }
 Stats finish(){
  const auto began=std::chrono::steady_clock::now();const bool joining=worker.joinable();
  {std::lock_guard<std::mutex> lock(mutex);if(joining){stats.inFlightAtFinish=inFlight;stats.discardedPending=pending||(inFlight&&latest.load()-activeSnapshot>=triggerBytes);}stopping=true;next.store(std::numeric_limits<int64_t>::max());}
  changed.notify_one();if(joining)worker.join();
  std::lock_guard<std::mutex> lock(mutex);
  if(joining)stats.finishWaitSeconds=std::chrono::duration<double>(std::chrono::steady_clock::now()-began).count();
  return stats;
 }
};
}
