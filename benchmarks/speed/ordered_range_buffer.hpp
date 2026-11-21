#pragma once
// Standalone research component. Not included by the production downloader.
#include <algorithm>
#include "BodyBufferStorage.hpp"
#include <condition_variable>
#include <cstdint>
#include <cstring>
#include <exception>
#include <functional>
#include <map>
#include <mutex>
#include <stdexcept>
#include <thread>
#include <utility>
#include <vector>
namespace udm::experiment {
class OrderedRangeBuffer {
public:
 using Byte=std::uint8_t;
 using Span=std::pair<std::size_t,std::size_t>;
 using Writer=std::function<void(std::size_t,const Byte*,std::size_t)>;
 static constexpr std::size_t maxBytes=BodyBufferStorage::maxBytes;
 static constexpr std::size_t maxSpans=4096;
 struct State {
  std::size_t total=0,acceptedBytes=0,writtenBytes=0;
  bool closing=false,paused=false,aborted=false,settled=false,complete=false;
  std::exception_ptr failure;
  std::vector<Span> accepted,written;
  // Segment Done is a contiguous prefix, never a sum across a hole.
  std::size_t resumePrefix(std::size_t offset,std::size_t length)const noexcept{
   if(offset>total||length>total-offset)return 0;
   auto at=offset;const auto end=offset+length;
   for(const auto& span:written){if(span.second<=at)continue;if(span.first>at)break;at=std::min(end,span.second);if(at==end)break;}
   return at-offset;
  }
 };
private:
 using Ranges=std::map<std::size_t,std::size_t>;
 const std::size_t total,chunk;
 const Writer write;
 BodyBufferStorage::Ptr storage;
 mutable std::mutex mutex;
 std::condition_variable changed;
 Ranges accepted,written;
 bool closing=false,drainGaps=false,aborted=false,settled=false;
 std::exception_ptr failure;
 std::thread worker;
 std::thread::id writerId{};
 static BodyBufferStorage::Ptr checkedStorage(std::size_t size,BodyBufferStorage::Ptr backing){
  if(!backing)backing=BodyBufferStorage::reserve(size);
  if(!backing)throw std::runtime_error("Shared body buffer is occupied or unavailable");
  if(backing->size()!=size||backing.use_count()!=1)throw std::invalid_argument("Writer requires unique, exactly sized storage");
  return backing;
 }
 static Writer checkedWriter(Writer callback){if(!callback)throw std::invalid_argument("Missing storage writer");return callback;}
 static std::size_t checkedSize(std::size_t value){if(!value||value>maxBytes)throw std::invalid_argument("Unsupported body size");return value;}
 static std::size_t checkedChunk(std::size_t value,std::size_t size){if(!value)throw std::invalid_argument("Empty writer window");return std::min(value,size);}
 static void include(Ranges& ranges,std::size_t low,std::size_t high){
  auto at=ranges.lower_bound(low);
  if(at!=ranges.begin()){auto prior=std::prev(at);if(prior->second>=low)at=prior;}
  while(at!=ranges.end()&&at->first<=high){low=std::min(low,at->first);high=std::max(high,at->second);at=ranges.erase(at);}
  ranges.emplace(low,high);
 }
 static std::size_t count(const Ranges& ranges)noexcept{std::size_t n=0;for(const auto& span:ranges)n+=span.second-span.first;return n;}
 std::size_t prefix()const noexcept{return !written.empty()&&written.begin()->first==0?written.begin()->second:0;}
 bool next(Span& result)const noexcept{
  for(const auto& available:accepted){
   auto at=available.first;
   for(const auto& saved:written){
    if(saved.second<=at)continue;if(saved.first>=available.second)break;
    if(saved.first>at)break;at=std::max(at,saved.second);if(at>=available.second)break;
   }
   if(at>=available.second)continue;
   if(!drainGaps&&at!=prefix())return false;
   auto end=std::min(available.second,at+std::min(chunk,total-at));
   // Do not rewrite a later committed span after partial drain.
   auto later=written.lower_bound(at);if(later!=written.end())end=std::min(end,later->first);
   if(end>at){result={at,end};return true;}
  }
  return false;
 }
 void run()noexcept{
  try{
   std::unique_lock<std::mutex> lock(mutex);writerId=std::this_thread::get_id();
   for(;;){
    Span span{};
    changed.wait(lock,[&]{return aborted||closing||next(span);});
    if(aborted)return;
    if(!next(span)){if(closing)return;continue;}
    // Prepare all credit bookkeeping before issuing IO. Failure cannot erase
    // an earlier acknowledgment, and no allocation follows a successful write.
    auto credit=written;include(credit,span.first,span.second);
    lock.unlock();write(span.first,storage->data()+span.first,span.second-span.first);lock.lock();
    written.swap(credit);changed.notify_all();
   }
  }catch(...){std::lock_guard<std::mutex> lock(mutex);failure=std::current_exception();closing=true;changed.notify_all();}
 }
 State stateLocked()const{
  State value;value.total=total;value.acceptedBytes=count(accepted);value.writtenBytes=count(written);value.closing=closing;value.paused=closing&&drainGaps;value.aborted=aborted;value.settled=settled;value.failure=failure;
  value.complete=settled&&!aborted&&!drainGaps&&!failure&&value.writtenBytes==total&&prefix()==total;
  for(const auto& span:accepted)value.accepted.push_back(span);
  for(const auto& span:written)value.written.push_back(span);
  return value;
 }
 void settle(bool aborting,bool preserveGaps){
  {std::lock_guard<std::mutex> lock(mutex);if(writerId==std::this_thread::get_id())throw std::logic_error("Writer cannot close itself");if(!closing){closing=true;aborted=aborting;drainGaps=preserveGaps;}}
  changed.notify_all();if(worker.joinable())worker.join();
  std::lock_guard<std::mutex> lock(mutex);settled=true;
 }
public:
 explicit OrderedRangeBuffer(std::size_t size,Writer callback,std::size_t window=256*1024,BodyBufferStorage::Ptr backing={}):total(checkedSize(size)),chunk(checkedChunk(window,total)),write(checkedWriter(std::move(callback))),storage(checkedStorage(total,std::move(backing))){
  worker=std::thread([this]{run();});
 }
 OrderedRangeBuffer(const OrderedRangeBuffer&)=delete;OrderedRangeBuffer& operator=(const OrderedRangeBuffer&)=delete;
 OrderedRangeBuffer(OrderedRangeBuffer&&)=delete;OrderedRangeBuffer& operator=(OrderedRangeBuffer&&)=delete;
 ~OrderedRangeBuffer(){if(worker.joinable())settle(true,false);}
 void put(std::size_t offset,const Byte* source,std::size_t length){
  std::unique_lock<std::mutex> lock(mutex);
  if(failure)std::rethrow_exception(failure);
  if(closing)throw std::runtime_error("Buffer has closed");
  if(offset>total||length>total-offset||(!source&&length))throw std::out_of_range("Invalid buffered span");
  if(!length)return;
  const auto end=offset+length;
  for(const auto& span:accepted){const auto low=std::max(offset,span.first),high=std::min(end,span.second);if(low<high&&!std::equal(source+(low-offset),source+(high-offset),storage->data()+low))throw std::runtime_error("Conflicting buffered bytes");}
  auto nextAccepted=accepted;include(nextAccepted,offset,end);if(nextAccepted.size()>maxSpans)throw std::length_error("Buffered span limit");
  // Copy only holes. The IO thread may be reading previously accepted bytes;
  // even rewriting identical bytes there would be a C++ data race.
  auto at=offset;
  for(const auto& span:accepted){
   if(span.second<=at)continue;if(span.first>=end)break;
   if(at<span.first){const auto high=std::min(end,span.first);std::memcpy(storage->data()+at,source+(at-offset),high-at);at=high;}
   at=std::max(at,std::min(end,span.second));if(at==end)break;
  }
  if(at<end)std::memcpy(storage->data()+at,source+(at-offset),end-at);
  accepted.swap(nextAccepted);lock.unlock();changed.notify_all();
 }
 State snapshot()const{std::lock_guard<std::mutex> lock(mutex);return stateLocked();}
 // Only one controller closes the buffer. Snapshot/put can run concurrently.
 State finish(){settle(false,false);return snapshot();}
 State pause(){settle(false,true);return snapshot();}
 State abort(){settle(true,false);return snapshot();}
 BodyBufferStorage::Ptr takeCompletedStorage(){
  std::lock_guard<std::mutex> lock(mutex);
  if(!settled||aborted||drainGaps||failure||prefix()!=total||count(written)!=total)return {};
  return std::move(storage);
 }
 const std::vector<Byte>* completedBody()const{
  std::lock_guard<std::mutex> lock(mutex);return storage&&settled&&!aborted&&!drainGaps&&!failure&&prefix()==total&&count(written)==total?&storage->bytes():nullptr;
 }
};
}
