#include <windows.h>
#include "ordered_range_buffer.hpp"
#include "third_party/json.hpp"
#include <filesystem>
#include <fstream>
#include <future>
#include <iostream>
#include <limits>
#include <memory>
#include <string>
#include <type_traits>
using Buffer=udm::experiment::OrderedRangeBuffer;
using Json=nlohmann::json;namespace fs=std::filesystem;
static_assert(!std::is_copy_constructible_v<Buffer>&&!std::is_move_constructible_v<Buffer>);
struct Release {std::promise<void>& value;~Release(){try{value.set_value();}catch(...){}}};
int main(int argc,char**argv){
 if(argc!=2)return 2;const auto root=fs::absolute(argv[1]);if(fs::exists(root))return 2;fs::create_directories(root);std::ofstream(root/".udm-ordered-range-fixture")<<"owned";
 Json checks=Json::array();std::string error;
 auto check=[&](bool value,const char* label){checks.push_back({{"name",label},{"passed",value}});if(!value)throw std::runtime_error(label);};
 auto rejects=[](auto&& fn){try{fn();return false;}catch(const std::exception&){return true;}};
 const std::vector<Buffer::Byte> body={'a','b','c','d','e','f','g','h'};
 try{
  auto noop=[](std::size_t,const Buffer::Byte*,std::size_t){};
  check(rejects([&]{Buffer b(0,noop);}),"Empty bodies are rejected before worker creation");
  check(rejects([&]{Buffer b(Buffer::maxBytes+1,noop);}),"The 64 MiB allocation cap is enforced before admission");
  check(rejects([&]{Buffer b(8,noop,0);}),"An empty writer window is rejected");
  check(rejects([&]{Buffer b(8,Buffer::Writer{});}),"A missing writer cannot retain a budget lease");
  {
   std::atomic_uint calls{0};Buffer b(8,[&](auto,auto,auto){++calls;});b.put(4,body.data()+4,4);auto s=b.snapshot();
   check(s.acceptedBytes==4&&s.writtenBytes==0&&calls==0,"A received tail waits in memory and earns no disk resume credit across a gap");
   s=b.finish();check(s.settled&&!s.complete&&s.writtenBytes==0&&s.acceptedBytes==4&&calls==0,"Finishing with a gap never writes the tail or claims a complete body");
   check(!b.completedBody()&&s.resumePrefix(4,4)==0,"Buffered-only bytes cannot supply a completion body or saved segment prefix");
   check(rejects([&]{b.put(0,body.data(),4);}),"Closed generations reject further input");
   check(!b.pause().complete&&calls==0,"A second close cannot change an already settled policy or issue new IO");
  }
  {
   std::vector<Buffer::Byte> disk(8,0);std::vector<std::size_t> calls;Buffer b(8,[&](auto at,auto data,auto n){calls.push_back(at);std::copy_n(data,n,disk.data()+at);},2);
   b.put(4,body.data()+4,4);b.put(2,body.data()+2,2);b.put(0,body.data(),2);auto s=b.finish();
   check(s.complete&&s.settled&&!s.failure&&s.acceptedBytes==8&&s.writtenBytes==8,"Out-of-order network input completes only after every ordered write is acknowledged");
   check(calls==std::vector<std::size_t>({0,2,4,6})&&disk==body,"The writer emits contiguous ascending windows with the exact original bytes");
   check(s.resumePrefix(0,8)==8&&s.resumePrefix(2,3)==3&&s.resumePrefix(9,1)==0,"Resume credit is clipped to the requested valid segment");
   check(b.completedBody()&&*b.completedBody()==body,"Only a settled successful finish exposes the full cached body");
  }
  {
   std::vector<Buffer::Byte> disk(8,0);Buffer b(8,[&](auto at,auto data,auto n){std::copy_n(data,n,disk.data()+at);},2);b.put(4,body.data()+4,4);auto s=b.pause();
   check(s.paused&&s.settled&&!s.complete&&s.acceptedBytes==4&&s.writtenBytes==4,"Pause drains accepted noncontiguous spans without declaring completion");
   check(s.resumePrefix(0,8)==0&&s.resumePrefix(4,4)==4,"Pause credit cannot cross a missing segment prefix");
   check(std::equal(disk.begin()+4,disk.end(),body.begin()+4)&&!b.completedBody(),"Paused tail data is preserved while completion exposure remains closed");
  }
  {
   std::atomic_uint calls{0};Buffer b(8,[&](auto,auto,auto){++calls;});b.put(4,body.data()+4,4);auto s=b.abort();
   check(s.aborted&&s.settled&&!s.complete&&s.acceptedBytes==4&&s.writtenBytes==0&&calls==0,"Abort discards buffered-only tails without granting saved progress");
  }
  {
   std::promise<void> entered,release;auto waiting=entered.get_future();auto proceed=release.get_future().share();std::atomic_uint calls{0};std::vector<Buffer::Byte> disk(8,0);
   Buffer* observer=nullptr;std::size_t duringWrite=99;Buffer b(8,[&](auto at,auto data,auto n){duringWrite=observer->snapshot().writtenBytes;if(calls++==0){entered.set_value();proceed.wait();}std::copy_n(data,n,disk.data()+at);},4);observer=&b;Release safe{release};
   b.put(0,body.data(),8);check(waiting.wait_for(std::chrono::seconds(3))==std::future_status::ready,"A held storage operation is observable without locking out state readers");
   for(unsigned i=0;i<32;++i)b.put(1,body.data()+1,6);
   check(b.snapshot().acceptedBytes==8&&b.snapshot().writtenBytes==0,"Identical retries do not overwrite in-flight bytes or credit a held write");
   const Buffer::Byte wrong[]={'x','x'};check(rejects([&]{b.put(2,wrong,2);}),"Conflicting retries are rejected before modifying accepted data");
   release.set_value();auto s=b.finish();check(s.complete&&disk==body&&calls==2&&duringWrite==4,"Identical retries neither duplicate writes nor change the completed representation");
  }
  {
   Buffer b(8,[](auto,auto,auto){throw std::runtime_error("storage failed");});b.put(0,body.data(),8);auto s=b.finish();
   check(bool(s.failure)&&!s.complete&&s.writtenBytes==0&&s.resumePrefix(0,8)==0,"A failed write never earns completion or resume credit");
   check(!b.completedBody()&&rejects([&]{b.put(0,body.data(),1);}),"Storage failure closes completion and propagates to later producers");
  }
  {
   std::vector<Buffer::Byte> disk(8,0);unsigned calls=0;Buffer b(8,[&](auto at,auto data,auto n){if(calls++==1){std::copy_n(data,2,disk.data()+at);throw std::runtime_error("partial write");}std::copy_n(data,n,disk.data()+at);},4);b.put(0,body.data(),8);auto s=b.finish();
   check(s.failure&&s.writtenBytes==4&&s.resumePrefix(0,8)==4&&s.resumePrefix(4,4)==0,"A later partial write preserves earlier credit but grants none for the failed operation");
   check(disk[4]==body[4]&&!s.complete&&!b.completedBody(),"Physically changed but unacknowledged bytes cannot imply completeness");
  }
  {
   std::vector<Buffer::Byte> source(65536),disk(source.size(),0);for(std::size_t i=0;i<source.size();++i)source[i]=static_cast<Buffer::Byte>(i%251);
   Buffer b(source.size(),[&](auto at,auto data,auto n){std::copy_n(data,n,disk.data()+at);},4096);std::vector<std::thread> producers;
   struct Join {std::vector<std::thread>& threads;~Join(){for(auto& t:threads)if(t.joinable())t.join();}} joined{producers};
   for(unsigned i=0;i<8;++i)producers.emplace_back([&,i]{const auto at=(7-i)*8192;b.put(at,source.data()+at,8192);});for(auto& t:producers)t.join();auto s=b.finish();
   check(s.complete&&s.writtenBytes==source.size()&&disk==source,"Eight producers share one stable buffer and preserve exact ordered output");
  }
  {
   Buffer b(8,noop);check(rejects([&]{b.put(9,body.data(),0);})&&rejects([&]{b.put(std::numeric_limits<std::size_t>::max(),body.data(),1);}),"Out-of-range and overflowing offsets cannot alter buffer state");
   check(rejects([&]{b.put(7,body.data(),2);})&&rejects([&]{b.put(0,nullptr,1);}),"Oversized and missing-source writes are rejected before copying");
   b.put(8,nullptr,0);check(b.snapshot().acceptedBytes==0,"A valid zero-length notification accepts no bytes");b.abort();
  }
  {
   Buffer b(8,noop);check(rejects([&]{Buffer contender(8,noop);}),"A second experimental body allocation is refused while the first lease is live");b.abort();
   check(rejects([&]{Buffer contender(8,noop);}),"Closing retains ownership while the body storage remains alive");
  }
  {
   Buffer successor(8,noop);successor.put(0,body.data(),8);check(successor.finish().complete,"A successor acquires the budget after the prior buffer is destroyed");
  }
  {
   Buffer b(16384,noop);const Buffer::Byte value=1;for(std::size_t i=0;i<Buffer::maxSpans;++i)b.put(i*2+1,&value,1);
   check(rejects([&]{b.put(Buffer::maxSpans*2+1,&value,1);}),"Excessive fragmented metadata is rejected atomically at its fixed limit");
   auto s=b.abort();check(s.acceptedBytes==Buffer::maxSpans&&s.writtenBytes==0,"Metadata refusal preserves prior accepted bytes without false credit");
  }
  {
   std::promise<void> entered,release;auto waiting=entered.get_future();auto proceed=release.get_future().share();std::atomic_uint calls{0};Buffer b(8,[&](auto,auto,auto){if(calls++==0){entered.set_value();proceed.wait();}},2);Release safe{release};b.put(0,body.data(),8);check(waiting.wait_for(std::chrono::seconds(3))==std::future_status::ready,"Abort control holds a real in-flight writer callback");
   auto stopped=std::async(std::launch::async,[&]{return b.abort();});Release safeFuture{release};const auto deadline=std::chrono::steady_clock::now()+std::chrono::seconds(3);while(!b.snapshot().closing&&std::chrono::steady_clock::now()<deadline)std::this_thread::yield();
   check(b.snapshot().closing&&rejects([&]{b.put(0,body.data(),1);}),"Abort closes producer admission before joining the active operation");release.set_value();auto s=stopped.get();
   check(s.aborted&&!s.complete&&s.writtenBytes==2&&calls==1,"Abort acknowledges an operation that already succeeded and starts no later writes");
  }
  {
   Buffer* observer=nullptr;Buffer b(8,[&](auto,auto,auto){observer->abort();});observer=&b;b.put(0,body.data(),8);auto s=b.finish();
   check(s.failure&&!s.complete&&s.writtenBytes==0,"A writer callback cannot close and join its own thread");
  }
  {
   const auto path=root/"ordered-file.bin";HANDLE file=CreateFileW(path.c_str(),GENERIC_READ|GENERIC_WRITE,FILE_SHARE_READ,nullptr,CREATE_NEW,FILE_ATTRIBUTE_NORMAL,nullptr);if(file==INVALID_HANDLE_VALUE)throw std::runtime_error("Cannot create owned disk fixture");
   struct File {HANDLE h;~File(){if(h!=INVALID_HANDLE_VALUE)CloseHandle(h);}} held{file};
   constexpr std::size_t size=262144;LARGE_INTEGER eof{};eof.QuadPart=size;if(!SetFilePointerEx(file,eof,nullptr,FILE_BEGIN)||!SetEndOfFile(file))throw std::runtime_error("Cannot reserve owned fixture");
   std::vector<Buffer::Byte> source(size),disk(size);for(std::size_t i=0;i<size;++i)source[i]=static_cast<Buffer::Byte>(i%251);std::vector<std::size_t> offsets;
   Buffer b(size,[&](auto at,auto data,auto n){OVERLAPPED place{};place.Offset=static_cast<DWORD>(at);DWORD done=0;if(!WriteFile(file,data,static_cast<DWORD>(n),&done,&place)||done!=n)throw std::runtime_error("Owned positioned write failed");offsets.push_back(at);},32768);
   b.put(size/2,source.data()+size/2,size/2);check(b.snapshot().writtenBytes==0,"A real regular file receives no tail write across a gap");b.put(0,source.data(),size/2);auto complete=b.finish();
   check(complete.complete&&offsets==std::vector<std::size_t>({0,32768,65536,98304,131072,163840,196608,229376}),"Real positioned writes advance from zero without gaps or reordered IO");
   check(FlushFileBuffers(file)!=FALSE,"A separate explicit final flush remains the caller's responsibility");LARGE_INTEGER zero{};DWORD read=0;if(!SetFilePointerEx(file,zero,nullptr,FILE_BEGIN)||!ReadFile(file,disk.data(),static_cast<DWORD>(disk.size()),&read,nullptr))throw std::runtime_error("Cannot verify owned output");
   check(read==size&&disk==source,"The flushed regular file preserves every original byte after tail-first admission");
  }
  {
   const auto path=root/"paused-file.bin";HANDLE file=CreateFileW(path.c_str(),GENERIC_READ|GENERIC_WRITE,FILE_SHARE_READ,nullptr,CREATE_NEW,FILE_ATTRIBUTE_NORMAL,nullptr);if(file==INVALID_HANDLE_VALUE)throw std::runtime_error("Cannot create owned pause fixture");struct File {HANDLE h;~File(){CloseHandle(h);}} held{file};
   std::vector<Buffer::Byte> tail(4096,123),disk(8192);Buffer b(8192,[&](auto at,auto data,auto n){OVERLAPPED place{};place.Offset=static_cast<DWORD>(at);DWORD done=0;if(!WriteFile(file,data,static_cast<DWORD>(n),&done,&place)||done!=n)throw std::runtime_error("Owned pause write failed");});b.put(4096,tail.data(),tail.size());auto paused=b.pause();
   check(paused.paused&&!paused.complete&&paused.resumePrefix(0,8192)==0&&paused.resumePrefix(4096,4096)==4096,"A real paused file retains only its acknowledged segment-prefix credit");
   if(!FlushFileBuffers(file))throw std::runtime_error("Cannot flush pause fixture");LARGE_INTEGER zero{};DWORD read=0;if(!SetFilePointerEx(file,zero,nullptr,FILE_BEGIN)||!ReadFile(file,disk.data(),static_cast<DWORD>(disk.size()),&read,nullptr))throw std::runtime_error("Cannot read pause fixture");
   check(read==8192&&std::all_of(disk.begin(),disk.begin()+4096,[](auto value){return value==0;})&&std::equal(disk.begin()+4096,disk.end(),tail.begin()),"The paused output contains the exact tail while its unwritten leading gap remains uncredited");
  }
  {
   Buffer b(8,[](auto,auto,auto){});b.put(0,body.data(),8);auto paused=b.pause();
   check(paused.paused&&paused.writtenBytes==8&&!b.takeCompletedStorage(),"A fully written paused body cannot be exported as confirmed completion");
  }
  {
   Buffer b(8,[](auto,auto,auto){});b.put(0,body.data(),8);const auto deadline=std::chrono::steady_clock::now()+std::chrono::seconds(3);while(b.snapshot().writtenBytes<8&&std::chrono::steady_clock::now()<deadline)std::this_thread::yield();
   auto aborted=b.abort();check(aborted.aborted&&aborted.writtenBytes==8&&!b.takeCompletedStorage(),"An abort cannot export completion even when all writes already succeeded");
  }
  {
   Buffer b(8,[](auto,auto,auto){throw std::runtime_error("export failure control");});b.put(0,body.data(),8);auto failed=b.finish();
   check(failed.failure&&!b.takeCompletedStorage(),"A failed writer cannot hand storage to the completion-hash stage");
  }
 }catch(const std::exception& e){error=e.what();}
 Json result={{"passed",error.empty()},{"error",error},{"checks",checks},{"productIntegrated",false},{"durabilityBarrierImplemented",false}};std::ofstream(root/"results.json",std::ios::binary)<<result.dump(2);std::cout<<checks.size()<<" checks; "<<(error.empty()?"passed":error)<<"\n";return error.empty()?0:1;
}
