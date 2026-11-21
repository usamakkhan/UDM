#include "WrittenBodyCache.hpp"
#include "OrderedBodyWriter.hpp"
#include "StreamHash.hpp"
#include <iostream>
#include <future>
#include <thread>
#include <type_traits>
using namespace udm;
static_assert(!std::is_copy_constructible_v<WrittenBodyCache> && !std::is_copy_assignable_v<WrittenBodyCache>);
static_assert(!std::is_move_constructible_v<WrittenBodyCache> && !std::is_move_assignable_v<WrittenBodyCache>);
int wmain(int argc,wchar_t** argv){
 if(argc!=2)return 2;const auto root=fs::absolute(argv[1]);if(fs::exists(root))return 2;fs::create_directories(root);Json checks=Json::array();std::string error;
 auto check=[&](bool ok,const char* name){checks.push_back({{"name",name},{"passed",ok}});if(!ok)throw std::runtime_error(name);};
 try{
  WrittenBodyCache cache;cache.reset(0);check(!cache.valid,"An empty body does not reserve the shared cache");
  cache.reset(WrittenBodyCache::maxBytes+1);check(!cache.valid,"A body above the global byte cap is refused before allocation");
  cache.reset(-1);check(!cache.valid,"Unknown or negative body length cannot create a cache");
  cache.reset(8);check(cache.valid,"A bounded body can acquire the shared cache");
  check(!cache.body(8,8),"A claimed complete count cannot expose bytes that were never credited");
  const auto write=[&](const char* data,size_t count,i64 offset){cache.wrote(reinterpret_cast<const BYTE*>(data),count,offset);};
  write("efgh",4,4);check(!cache.body(8,8),"An out-of-order tail does not prove a complete body");
  write("ab",2,0);check(!cache.body(8,8),"A leading prefix and tail still retain an uncovered middle gap");
  write("cd",2,2);auto body=cache.body(8,8);check(body&&std::string(body->begin(),body->end())=="abcdefgh","Adjacent credited ranges merge into an exact complete body");
  write("XY",2,1);body=cache.body(8,8);check(body&&std::string(body->begin(),body->end())=="aXYdefgh","An overlapping credited rewrite replaces the actual cached bytes");
  check(!cache.body(8,7),"An incomplete transfer count cannot admit the cached body");
  check(!cache.body(7,7),"A changed final size cannot reuse an older body allocation");
  cache.wrote(nullptr,0,-1);check(cache.body(8,8)!=nullptr,"A zero-length notification neither reads memory nor invalidates credited bytes");
  cache.reset(8);check(cache.valid&&!cache.body(8,8),"Reset clears old coverage rather than adopting unwritten zeros");
  write("xx",2,7);check(!cache.valid&&!cache.body(8,8),"An out-of-bounds write disables the optional cache");
  cache.reset(8);write("x",1,-1);check(!cache.valid,"A negative write offset falls back without exposing a partial body");
  cache.reset(8);cache.wrote(nullptr,1,0);check(!cache.valid,"A missing positive-length source disables optional caching");
  cache.reset(4);const BYTE binary[]={0,255,1,0};cache.wrote(binary,4,0);body=cache.body(4,4);check(body&&*body==Bytes({0,255,1,0}),"Binary zero and high-bit values are preserved exactly");
  cache.reset(0);check(!cache.valid,"Releasing the cache clears its valid state");
  {WrittenBodyCache first,second;first.reset(16);second.reset(16);check(first.valid&&!second.valid,"Concurrent transfers cannot each reserve the global body budget");first.reset(0);second.reset(16);check(second.valid,"Another owner can acquire the slot after release");second.reset(0);first.reset(WrittenBodyCache::maxBytes);check(!first.body(WrittenBodyCache::maxBytes,WrittenBodyCache::maxBytes),"The maximum budget never admits uncovered bytes, including on allocation refusal");first.reset(WrittenBodyCache::maxBytes+1);check(!first.valid,"A reset beyond the byte cap also releases an older lease");}
  cache.reset(9000);const BYTE x=1;for(i64 offset=0;offset<=8192;offset+=2)cache.wrote(&x,1,offset);check(!cache.valid,"Excessively fragmented coverage is bounded and falls back to disk");
  cache.reset(8);write("abcdefgh",8,0);body=cache.body(8,8);check(body&&std::string(body->begin(),body->end())=="abcdefgh","A fresh attempt can recover after optional-cache invalidation");
  cache.reset(0);
  {
   WrittenBodyCache owner,contender;owner.reset(16);check(owner.valid,"The transfer thread acquires a cache before worker-side invalidation");
   std::thread worker([&]{owner.wrote(nullptr,1,0);});worker.join();
   check(!owner.valid&&!owner.body(16,16),"A serialized worker-side cache failure discards unverified bytes");
   contender.reset(16);check(contender.valid,"Worker-side invalidation releases the process budget without unlocking another thread's mutex");
  }
  {
   auto owner=std::make_unique<WrittenBodyCache>();owner->reset(16);check(owner->valid,"A cache lease is acquired before destruction on another thread");
   std::thread destroy([owned=std::move(owner)]{});destroy.join();
   WrittenBodyCache next;next.reset(16);check(next.valid,"Serialized cross-thread destruction releases the lease for its successor");
  }
  {
   WrittenBodyCache owner;owner.reset(16);{WrittenBodyCache denied;denied.reset(16);check(!denied.valid,"A denied cache owns no process-budget lease");}
   WrittenBodyCache contender;contender.reset(16);check(owner.valid&&!contender.valid,"Destroying a denied cache cannot release the real owner's budget");
  }
  for(unsigned round=0;round<24;++round){
   std::promise<void> start,release;auto ready=start.get_future().share(),finish=release.get_future().share();
   std::vector<std::thread> workers;std::atomic_uint attempted{0},owners{0};
   struct Join {std::promise<void>& start;std::promise<void>& release;std::vector<std::thread>& workers;~Join(){try{start.set_value();}catch(...){}try{release.set_value();}catch(...){}for(auto& thread:workers)if(thread.joinable())thread.join();}} joined{start,release,workers};
   for(unsigned i=0;i<8;++i)workers.emplace_back([&]{ready.wait();WrittenBodyCache candidate;candidate.reset(16);if(candidate.valid)++owners;++attempted;finish.wait();});
   start.set_value();const auto deadline=std::chrono::steady_clock::now()+std::chrono::seconds(5);
   while(attempted.load()!=8&&std::chrono::steady_clock::now()<deadline)std::this_thread::yield();
   check(attempted==8&&owners==1,"Eight simultaneous cache attempts retain exactly one budget owner until release");
   release.set_value();for(auto& thread:workers)thread.join();
   WrittenBodyCache next;next.reset(16);check(next.valid,"A fresh cache acquires the budget after every contender has finished");
  }
  {
   WrittenBodyCache owner,denied;owner.reset(8);owner.wrote(reinterpret_cast<const BYTE*>("abcdefgh"),8,0);const auto address=owner.body(8,8)->data();auto exported=owner.releaseStorage();
   check(!owner.valid&&!owner.body(8,8)&&exported&&exported->data()==address,"Storage export invalidates cache credit without copying or releasing its byte allocation");
   check(exported->bytes()==Bytes({'a','b','c','d','e','f','g','h'}),"An exported lease preserves the exact buffered representation");
   denied.reset(8);check(!denied.valid,"Exported storage still owns the same global cache budget");
   auto reader=exported;exported.reset();denied.reset(8);check(!denied.valid,"A remaining shared storage reference prevents another body allocation");
   std::thread destroy([last=std::move(reader)]{});destroy.join();denied.reset(8);check(denied.valid,"Last-reference destruction on a worker releases the budget exactly once");
  }
  {
   WrittenBodyCache owner,denied;owner.reset(16);
   check(!BodyBufferStorage::reserve(0)&&!BodyBufferStorage::reserve(BodyBufferStorage::maxBytes+1),"Invalid shared-storage reservations neither allocate nor disturb an existing owner");
   denied.reset(16);check(owner.valid&&!denied.valid,"Invalid storage reservation cannot release the live owner's token");
  }
  {
   auto exported=BodyBufferStorage::reserve(8);check(bool(exported),"The shared storage factory admits a bounded standalone owner");auto alias=exported;
   bool refused=false;try{OrderedBodyWriter writer(8,[](auto,auto,auto){},4,exported);}catch(const std::invalid_argument&){refused=true;}
   check(refused&&exported.use_count()==2,"An ordered writer refuses aliased mutable storage before starting IO");alias.reset();
   refused=false;try{OrderedBodyWriter writer(16,[](auto,auto,auto){},4,std::move(exported));}catch(const std::invalid_argument&){refused=true;}
   check(refused&&!exported,"An ordered writer rejects an incorrectly sized transferred buffer");WrittenBodyCache next;next.reset(8);check(next.valid,"Failed writer admission retains no consumed storage lease");
  }
  {
   WrittenBodyCache cache,denied;cache.reset(8);check(cache.valid,"The existing production cache owns the shared token before writer admission");
   bool refused=false;try{OrderedBodyWriter other(8,[](auto,auto,auto){});}catch(const std::runtime_error&){refused=true;}
   check(refused,"The ordered writer cannot allocate a second body beside the production cache");
   auto storage=cache.releaseStorage();const auto address=storage->data();Bytes disk(8,0);OrderedBodyWriter writer(8,[&](auto at,auto data,auto n){std::copy_n(data,n,disk.data()+at);},4,std::move(storage));
   check(!storage&&!cache.valid,"Writer admission consumes the existing cache buffer rather than a second allocation");
   const BYTE bytes[]={'a','b','c','d','e','f','g','h'};writer.put(4,bytes+4,4);writer.put(0,bytes,4);auto done=writer.finish();
   check(done.complete&&disk==Bytes({'a','b','c','d','e','f','g','h'})&&writer.completedBody()->data()==address,"Ordered completion preserves buffer identity and exact bytes across cache-to-writer handoff");
   auto completed=writer.takeCompletedStorage();check(completed&&completed->data()==address&&!writer.completedBody()&&!writer.takeCompletedStorage(),"A successful joined writer exports its original allocation at most once");
   denied.reset(8);check(!denied.valid,"An exported completed body retains the budget for its hash reader");completed.reset();denied.reset(8);check(denied.valid,"The body budget is reusable after the final completed-buffer reader releases it");
  }
  {
   OrderedBodyWriter writer(8,[](auto,auto,auto){});const BYTE byte=1;writer.put(4,&byte,1);
   check(!writer.takeCompletedStorage(),"An unsettled writer cannot export a body as complete");auto state=writer.pause();check(state.paused&&!writer.takeCompletedStorage(),"A paused incomplete writer cannot export completion storage");
  }
  {
   WrittenBodyCache cache;cache.reset(8);auto storage=cache.releaseStorage();const auto address=storage->data();const BYTE expected[]={1,2,3,4,5,6,7,8};
   OrderedBodyWriter writer(8,[](auto,auto,auto){},4,std::move(storage));writer.put(4,expected+4,4);writer.put(0,expected,4);auto done=writer.finish();
   check(done.complete&&cache.adoptCompletedStorage(writer.takeCompletedStorage(),8),"The joined native writer transfers complete disk-backed storage into advisory hashing");
   const auto body=cache.body(8,8);check(body&&body->data()==address&&*body==Bytes(expected,expected+8),"The final cache reuses the same allocation and preserves every original byte");
   WrittenBodyCache denied;denied.reset(8);check(!denied.valid,"The adopted hash body continues to hold the one-buffer budget");
  }
  {
   WrittenBodyCache cache;auto storage=BodyBufferStorage::reserve(8);auto alias=storage;
   check(!cache.adoptCompletedStorage(std::move(storage),8)&&!cache.valid,"Completed hash admission refuses a shared mutable allocation");alias.reset();cache.reset(8);check(cache.valid,"A refused completed-body alias retains no abandoned budget");
  }
  {
   WrittenBodyCache cache;auto storage=BodyBufferStorage::reserve(8);
   check(!cache.adoptCompletedStorage(std::move(storage),7)&&!cache.valid,"Completed hash admission refuses the wrong exact body size");cache.reset(8);check(cache.valid,"A rejected completion size releases its transferred buffer");
  }
  {
   WrittenBodyCache owner;owner.reset(3);Bytes copy;uint64_t stamp=0;
   const BYTE tail='c';owner.wrote(&tail,1,2);
   check(owner.copyWrittenPrefix(0,copy,stamp)&&copy.empty(),"A cached tail cannot expose an unwritten prefix to reception hashing");
   const BYTE head[]={'a','b'};owner.wrote(head,2,0);
   check(owner.copyWrittenPrefix(0,copy,stamp)&&copy==Bytes({'a','b','c'}),"Closing a gap exposes only successfully written contiguous bytes");
   StreamHash hash;hash.update(copy.data(),copy.size());check(hash.finish()=="ba7816bf8f01cfea414140de5dae2223b00361a396177a9cb410ff61f20015ad","Reception prefix hashing retains the standard SHA-256 known answer");
   check(owner.matchesPrefixRevision(stamp),"Non-overlapping writes retain the active prefix revision");
   const BYTE replacement='A';owner.wrote(&replacement,1,0);
   check(!owner.matchesPrefixRevision(stamp),"A rewrite invalidates an already consumed hash prefix");
   check(copy==Bytes({'a','b','c'}),"The consumer owns an independent copy across rewrites");
   check(owner.body(3,3)&&*owner.body(3,3)==Bytes({'A','b','c'}),"Rewrite invalidation preserves the complete latest body for fallback hashing");
   owner.reset(0);check(!owner.copyWrittenPrefix(0,copy,stamp)&&copy.empty(),"Disabled storage cannot supply a stale prefix");
  }
  {
   WrittenBodyCache owner;const auto size=WrittenBodyCache::prefixCopyLimit*2;owner.reset(size);Bytes data(size,0x5a),copy;uint64_t stamp=0;owner.wrote(data.data(),data.size(),0);
   check(owner.copyWrittenPrefix(0,copy,stamp)&&copy.size()==WrittenBodyCache::prefixCopyLimit,"Reception snapshots are bounded to one MiB");
   check(owner.copyWrittenPrefix(WrittenBodyCache::prefixCopyLimit,copy,stamp)&&copy.size()==WrittenBodyCache::prefixCopyLimit&&std::all_of(copy.begin(),copy.end(),[](auto b){return b==0x5a;}),"The next bounded snapshot retains every original byte");
   check(owner.copyWrittenPrefix(size,copy,stamp)&&copy.empty(),"An exact end cursor produces no additional hash input");
   check(!owner.copyWrittenPrefix(size+1,copy,stamp)&&copy.empty(),"A cursor beyond owned storage is rejected");
   check(!owner.copyWrittenPrefix(-1,copy,stamp),"Negative hash cursors are rejected");
  }
 }catch(const std::exception& e){error=e.what();}
 atomicText(root/L"results.json",Json{{"passed",error.empty()},{"error",error},{"checks",checks}}.dump(2),false);std::cout<<checks.size()<<" checks; "<<(error.empty()?"passed":error)<<"\n";return error.empty()?0:1;
}
