#pragma once
#include "Core.hpp"
#include <algorithm>
#include "BodyBufferStorage.hpp"
namespace udm {
// One process-wide 64 MiB optional cache. The owner serializes credited writes;
// readers use the body only after its download writers have joined.
// A worker may disable caching, so the budget lease has no thread affinity.
class WrittenBodyCache {
public:
 static constexpr i64 maxBytes=BodyBufferStorage::maxBytes;
 bool valid=false;
private:
 BodyBufferStorage::Ptr storage;
 std::vector<std::pair<i64,i64>> coverage;
 uint64_t revision=0;
 void disable()noexcept{++revision;valid=false;decltype(coverage){}.swap(coverage);storage.reset();}
public:
 WrittenBodyCache()=default;
 WrittenBodyCache(const WrittenBodyCache&)=delete;
 WrittenBodyCache& operator=(const WrittenBodyCache&)=delete;
 WrittenBodyCache(WrittenBodyCache&&)=delete;
 WrittenBodyCache& operator=(WrittenBodyCache&&)=delete;
 ~WrittenBodyCache(){disable();}
 void reset(i64 size)noexcept{
  disable();if(size<=0||size>maxBytes)return;
  storage=BodyBufferStorage::reserve(static_cast<size_t>(size));valid=bool(storage);
 }
 // Serialized handoff invalidates cache credit; storage bytes and budget survive.
 BodyBufferStorage::Ptr releaseStorage()noexcept{valid=false;decltype(coverage){}.swap(coverage);return std::move(storage);}
 // Only a joined writer with complete successful disk coverage may call this.
 // Admission transfers the existing allocation; hashing never reserves a copy.
 bool adoptCompletedStorage(BodyBufferStorage::Ptr completed,i64 size)noexcept{
  disable();if(!completed||size<=0||size>maxBytes||static_cast<size_t>(size)!=completed->size()||completed.use_count()!=1)return false;
  try{coverage.emplace_back(0,size);storage=std::move(completed);valid=true;return true;}catch(...){disable();return false;}
 }
 void wrote(const BYTE* source,size_t count,i64 offset)noexcept{
  if(!valid||!storage||!count)return;
  if(!source||offset<0||static_cast<uint64_t>(offset)>storage->size()||count>storage->size()-static_cast<size_t>(offset)){disable();return;}
  // A retry can replace previously covered bytes. The complete cache remains
  // usable, but a consumer that already hashed a prefix must fall back.
  const auto prior=std::lower_bound(coverage.begin(),coverage.end(),offset,[](const auto& range,i64 start){return range.second<=start;});
  if(prior!=coverage.end()&&prior->first<offset+static_cast<i64>(count))++revision;
  std::copy_n(source,count,storage->data()+static_cast<size_t>(offset));
  try{
   i64 low=offset,high=offset+static_cast<i64>(count);
   auto first=std::lower_bound(coverage.begin(),coverage.end(),low,[](const auto& range,i64 start){return range.second<start;});auto last=first;
   while(last!=coverage.end()&&last->first<=high){low=std::min(low,last->first);high=std::max(high,last->second);++last;}
   if(first==last&&coverage.size()>=4096){disable();return;}
   auto position=coverage.erase(first,last);coverage.insert(position,{low,high});
  }catch(...){disable();} // Optional allocation failure falls back to disk.
 }
 // Caller holds the same model lock as wrote/reset. Return an independent,
 // bounded copy, never an alias that can race a receiver or cache invalidation.
 static constexpr size_t prefixCopyLimit=1024*1024;
 bool matchesPrefixRevision(uint64_t stamp)const noexcept{return valid&&storage&&revision==stamp;}
 bool copyWrittenPrefix(i64 offset,Bytes& output,uint64_t& stamp)const noexcept{
  output.clear();stamp=revision;
  if(!valid||!storage||offset<0||static_cast<uint64_t>(offset)>storage->size())return false;
  if(coverage.empty()||coverage.front().first!=0)return offset==0;
  if(offset>coverage.front().second)return false;
  const auto count=std::min<size_t>(prefixCopyLimit,static_cast<size_t>(coverage.front().second-offset));
  try{output.assign(storage->data()+static_cast<size_t>(offset),storage->data()+static_cast<size_t>(offset)+count);return true;}
  catch(...){output.clear();return false;}
 }
 const Bytes* body(i64 size,i64 complete)const noexcept{
  return valid&&storage&&size>0&&size==complete&&size==static_cast<i64>(storage->size())&&coverage.size()==1&&coverage[0].first==0&&coverage[0].second==size?&storage->bytes():nullptr;
 }
};
}
