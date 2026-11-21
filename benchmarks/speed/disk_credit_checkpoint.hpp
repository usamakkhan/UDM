#pragma once
// Research adapter. Production Job/Manager snapshots do not use this yet.
#include "Core.hpp"
#include "BodyBufferStorage.hpp"
#include <limits>
namespace udm::experiment {
class DiskCreditCheckpoint {
 static_assert(std::is_nothrow_move_constructible_v<Json>,"Snapshot lease construction must not throw after acquiring a pin");
 using Ranges=std::map<i64,i64>; // half-open, acknowledged writes only
 struct State {
  std::mutex mutex;
  const std::shared_ptr<const Cancel> cancel;
  const std::shared_ptr<Handle> file;
  const Json representation;
  const i64 size;
  Ranges written;
  size_t captures=0,writes=0;
  bool retired=false;
  State(std::shared_ptr<const Cancel> c,std::shared_ptr<Handle> f,Json key,i64 total):cancel(std::move(c)),file(std::move(f)),representation(std::move(key)),size(total){}
 };
 std::shared_ptr<State> state;
 static i64 integer(const Json& object,const char* name){
  const auto it=object.find(name);
  if(it==object.end()||!it->is_number_integer()||(it->is_number_unsigned()&&it->get<unsigned long long>()>static_cast<unsigned long long>(std::numeric_limits<i64>::max())))throw std::invalid_argument("Invalid disk-credit integer");
  return it->get<i64>();
 }
 static Json identity(const Json& row){
  if(!row.is_object()||str(row,"Id").empty()||str(row,"Url").empty())throw std::invalid_argument("Missing disk-credit representation");
  Json key=Json::object();
  for(auto name:{"Id","Url","Size","Folder","FileName","ETag","Modified","HttpContentEncoding","SparseDataFile","HttpPartsResetPending","RefreshPendingValidation"})if(row.contains(name))key[name]=row[name];
  return key;
 }
 template<class T>static bool same(const std::shared_ptr<T>& a,const std::shared_ptr<T>& b)noexcept{return a.get()==b.get()&&!a.owner_before(b)&&!b.owner_before(a);}
 static void current(const State& s,const std::shared_ptr<const Cancel>& cancel,const std::shared_ptr<Handle>& file){
  if(s.retired||!same(s.cancel,cancel)||!same(s.file,file))throw std::runtime_error("Stale disk-credit generation");
 }
 static void include(Ranges& spans,i64 low,i64 high){
  auto at=spans.lower_bound(low);if(at!=spans.begin()){auto before=std::prev(at);if(before->second>=low)at=before;}
  while(at!=spans.end()&&at->first<=high){low=std::min(low,at->first);high=std::max(high,at->second);at=spans.erase(at);}spans.emplace(low,high);
 }
 static i64 prefix(const Ranges& spans,i64 low,i64 length)noexcept{
  auto at=spans.upper_bound(low);if(at==spans.begin())return 0;--at;
  return at->first<=low&&at->second>low?std::min(length,at->second-low):0;
 }
public:
 class Write {
  friend class DiskCreditCheckpoint;
  std::shared_ptr<State> state;i64 offset=0,length=0;
  Write(std::shared_ptr<State> s,i64 at,i64 n)noexcept:state(std::move(s)),offset(at),length(n){}
 public:
  Write(const Write&)=delete;Write& operator=(const Write&)=delete;Write& operator=(Write&&)=delete;
  Write(Write&&)=default;
  ~Write(){if(state){std::lock_guard<std::mutex> lock(state->mutex);--state->writes;}}
  // Call only after the exact requested write succeeds. A failed or partial
  // write leaves no new credit. Failure to allocate metadata also adds none.
  void acknowledge(){
   if(!state)throw std::logic_error("Write already acknowledged");
   auto held=state;std::lock_guard<std::mutex> lock(held->mutex);
   auto next=held->written;include(next,offset,offset+length);
   if(next.size()>4096)throw std::length_error("Disk-credit interval limit");
   held->written.swap(next);--held->writes;state.reset();
  }
 };
 class Snapshot {
  friend class DiskCreditCheckpoint;
  std::shared_ptr<State> state;
  Json row;
  Snapshot(std::shared_ptr<State> s,Json value)noexcept:state(std::move(s)),row(std::move(value)){}
 public:
  Snapshot(const Snapshot&)=delete;Snapshot& operator=(const Snapshot&)=delete;Snapshot& operator=(Snapshot&&)=delete;
  Snapshot(Snapshot&& other)noexcept:state(std::move(other.state)),row(std::move(other.row)){}
  ~Snapshot(){if(state){std::lock_guard<std::mutex> lock(state->mutex);--state->captures;}}
  const Json& value()const noexcept{return row;}
 };
 DiskCreditCheckpoint(const Json& row,std::shared_ptr<const Cancel> cancel,std::shared_ptr<Handle> file){
  const auto size=integer(row,"Size");
  if(size<=0||static_cast<unsigned long long>(size)>BodyBufferStorage::maxBytes||!cancel||!file||!static_cast<bool>(*file)||yes(row,"HttpPartsResetPending")||yes(row,"RefreshPendingValidation"))throw std::invalid_argument("Invalid disk-credit binding");
  BY_HANDLE_FILE_INFORMATION info{};
  if(!GetFileInformationByHandle(file->h,&info)||(info.dwFileAttributes&(FILE_ATTRIBUTE_DIRECTORY|FILE_ATTRIBUTE_REPARSE_POINT)))throw std::invalid_argument("Disk credit requires an ordinary held file");
  state=std::make_shared<State>(std::move(cancel),std::move(file),identity(row),size);
 }
 DiskCreditCheckpoint(const DiskCreditCheckpoint&)=delete;DiskCreditCheckpoint& operator=(const DiskCreditCheckpoint&)=delete;
 Write beginWrite(const std::shared_ptr<const Cancel>& cancel,const std::shared_ptr<Handle>& file,i64 offset,i64 length){
  std::lock_guard<std::mutex> lock(state->mutex);current(*state,cancel,file);cancel->check();
  if(offset<0||length<=0||offset>state->size||length>state->size-offset)throw std::out_of_range("Invalid disk-credit write");
  ++state->writes;return Write(state,offset,length);
 }
 Snapshot capture(const Json& live,const std::shared_ptr<const Cancel>& cancel,const std::shared_ptr<Handle>& file){
  std::lock_guard<std::mutex> lock(state->mutex);current(*state,cancel,file);
  if(identity(live)!=state->representation)throw std::runtime_error("Disk-credit representation changed");
  const auto segments=live.find("Segments");if(segments==live.end()||!segments->is_array()||segments->size()>4096)throw std::invalid_argument("Invalid disk-credit segments");
  auto row=live;auto& projected=row["Segments"];Ranges claimed;std::set<i64> indices;i64 received=0;
  for(auto& segment:projected){
   const auto index=integer(segment,"Index"),start=integer(segment,"Start"),end=integer(segment,"End"),done=integer(segment,"Done");
   if(index<0||index>100000||!indices.insert(index).second||start<0||end<start||end>=state->size||done<0||done>end-start+1)throw std::invalid_argument("Invalid disk-credit segment bounds");
   const auto next=claimed.lower_bound(start);
   if((next!=claimed.end()&&next->first<=end)||(next!=claimed.begin()&&std::prev(next)->second>start))throw std::invalid_argument("Overlapping disk-credit segments");
   claimed.emplace(start,end+1);
   const auto saved=std::min(done,prefix(state->written,start,end-start+1));segment["Done"]=saved;received+=saved;
  }
  row["Received"]=received;
  if(str(row,"Status")=="Complete"&&received!=state->size)throw std::runtime_error("Unwritten bytes cannot be checkpointed complete");
  ++state->captures;return Snapshot(state,std::move(row));
 }
 // Nonblocking: the controller retries outside Manager::mutex after pending
 // writes/checkpoints settle. No truncation, reset, detach or file reuse may
 // happen until this succeeds. This class never calls into Manager while locked.
 bool tryRetire(){std::lock_guard<std::mutex> lock(state->mutex);if(state->writes||state->captures)return false;state->retired=true;return true;}
};
}
