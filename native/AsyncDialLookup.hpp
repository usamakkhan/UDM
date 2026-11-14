#pragma once
#include "DialCredentials.hpp"
#include <condition_variable>
#include <deque>
#include <limits>
#include <variant>
namespace udm {
// Advisory reads only. Two process-wide workers and at most one queued action
// per receiver. Workers own their inputs; closing receivers never join OS calls.
class AsyncDialLookup {
public:
 using Value=std::variant<std::vector<DialEntry>,DialCredentialInfo,bool>;
 struct Result {std::optional<Value> value;std::string error;};
private:
 struct Slot {std::string feature="Windows connection",unknown="Cannot read Windows connection information.";uint64_t generation=0;bool alive=true,queued=false,pending=false;std::function<Value()> action;std::function<void()> notify;std::optional<Result> result;};
 struct Pool {std::mutex mutex;std::condition_variable wake;bool closing=false,disabled=false;std::deque<std::weak_ptr<Slot>> queue;};
 static void run(std::shared_ptr<Pool> pool)noexcept{
  try{for(;;){std::shared_ptr<Slot> slot;std::function<Value()> action;std::function<void()> notify;uint64_t generation=0;std::string unknown;
   {std::unique_lock<std::mutex> lock(pool->mutex);pool->wake.wait(lock,[&]{return pool->closing||!pool->queue.empty();});if(pool->closing)return;
    slot=pool->queue.front().lock();pool->queue.pop_front();if(!slot)continue;slot->queued=false;if(!slot->alive||!slot->pending||!slot->action)continue;
    generation=slot->generation;action=std::move(slot->action);notify=slot->notify;unknown=slot->unknown;
   }
   Result result;try{result.value=action();}catch(const std::exception& e){result.error=e.what();}catch(...){result.error=std::move(unknown);}
   bool published=false;
   {std::lock_guard<std::mutex> lock(pool->mutex);if(!pool->closing&&!pool->disabled&&slot->alive&&slot->generation==generation){slot->result=std::move(result);slot->pending=false;published=true;}}
   if(published&&notify)try{notify();}catch(...){}
  }}catch(...){std::lock_guard<std::mutex> lock(pool->mutex);pool->disabled=true;}
 }
 struct Service {
  std::shared_ptr<Pool> pool=std::make_shared<Pool>();
  Service(){unsigned launched=0;for(unsigned i=0;i<2;++i)try{std::thread([p=pool]{run(std::move(p));}).detach();++launched;}catch(...){}if(!launched)pool->disabled=true;}
  ~Service(){std::lock_guard<std::mutex> lock(pool->mutex);pool->closing=true;pool->queue.clear();pool->wake.notify_all();}
 };
 static std::shared_ptr<Pool> sharedPool(){static Service service;return service.pool;}
 std::shared_ptr<Pool> pool=sharedPool();std::shared_ptr<Slot> slot=std::make_shared<Slot>();
 void advance(){if(slot->generation==std::numeric_limits<uint64_t>::max())throw std::runtime_error("Connection lookup generations exhausted.");++slot->generation;}
public:
 explicit AsyncDialLookup(std::string feature="Windows connection",std::string unknown="Cannot read Windows connection information."){slot->feature=std::move(feature);slot->unknown=std::move(unknown);}AsyncDialLookup(const AsyncDialLookup&)=delete;AsyncDialLookup& operator=(const AsyncDialLookup&)=delete;
 ~AsyncDialLookup(){stop();}
 void request(std::function<Value()> action,std::function<void()> notify={}){
  if(!action)throw std::runtime_error("Missing connection lookup.");
  {std::lock_guard<std::mutex> lock(pool->mutex);if(!slot->alive)throw std::runtime_error("Connection lookup has closed.");advance();slot->result.reset();slot->pending=false;slot->action={};slot->notify={};
   if(pool->closing||pool->disabled)throw std::runtime_error(slot->feature+" lookup is unavailable.");
   if(!slot->queued&&pool->queue.size()>=32)throw std::runtime_error(slot->feature+" lookup is busy. Try again shortly.");
   slot->action=std::move(action);slot->notify=std::move(notify);slot->pending=true;
   if(!slot->queued)try{pool->queue.push_back(slot);slot->queued=true;}catch(...){slot->pending=false;slot->action={};throw;}
  }pool->wake.notify_one();
 }
 std::optional<Result> take(){std::lock_guard<std::mutex> lock(pool->mutex);if(pool->disabled&&slot->pending){slot->pending=false;slot->action={};return Result{{},slot->feature+" lookup is unavailable."};}auto result=std::move(slot->result);slot->result.reset();return result;}
 // Publication is not UI delivery: remain pending until take() consumes it.
 bool pending()const{std::lock_guard<std::mutex> lock(pool->mutex);return slot->pending||slot->result.has_value();}
 void invalidate(){std::lock_guard<std::mutex> lock(pool->mutex);advance();slot->pending=false;slot->result.reset();slot->action={};slot->notify={};}
 void stop()noexcept{if(!slot)return;std::lock_guard<std::mutex> lock(pool->mutex);slot->alive=false;slot->pending=false;slot->result.reset();slot->action={};slot->notify={};}
};
}
