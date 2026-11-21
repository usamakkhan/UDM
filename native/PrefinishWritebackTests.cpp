#include "Core.hpp"
#include "PrefinishWriteback.hpp"
#include <future>
#include <iostream>
namespace udm {
struct PrefinishWritebackTestAccess {
 static bool closing(PrefinishWriteback& writeback) {
  std::lock_guard<std::mutex> lock(writeback.mutex);return writeback.stopping;
 }
};
}
using namespace udm;
int wmain(int argc,wchar_t** argv){if(argc!=2)return 2;auto root=fs::absolute(argv[1]);if(fs::exists(root))return 2;fs::create_directories(root);Json checks=Json::array();std::string error;auto check=[&](bool ok,const char* name){checks.push_back({{"name",name},{"passed",ok}});if(!ok)throw std::runtime_error(name);};try{
 {std::promise<void> entered,release;auto ready=entered.get_future();auto released=release.get_future().share();std::atomic_uint calls=0;PrefinishWriteback writeback(8*1048576LL,1,[&]{if(!calls.fetch_add(1)){entered.set_value();released.wait();}return true;});std::future<void> progress;std::future<PrefinishWriteback::Stats> finish;struct Release {std::promise<void>& p;~Release(){try{p.set_value();}catch(...){}}} cleanup{release};writeback.progress(1048576);check(ready.wait_for(std::chrono::milliseconds(20))==std::future_status::timeout,"Data writeback is not requested before its in-flight threshold");writeback.progress(4*1048576LL);check(ready.wait_for(std::chrono::seconds(3))==std::future_status::ready,"A completed body prefix schedules actual background writeback");progress=std::async(std::launch::async,[&]{writeback.progress(7*1048576LL);});check(progress.wait_for(std::chrono::seconds(1))==std::future_status::ready,"Credited transfer progress does not wait for a held disk flush");progress.get();finish=std::async(std::launch::async,[&]{return writeback.finish();});check(finish.wait_for(std::chrono::milliseconds(20))==std::future_status::timeout,"Shutdown joins an already running writeback before its file handle can close");release.set_value();auto stats=finish.get();check(stats.flushes>=1&&stats.flushes<=2&&stats.errors==0,"Threshold requests are bounded/coalesced and successful flush statistics remain accurate");writeback.progress(8*1048576LL);check(writeback.finish().flushes==stats.flushes,"Finished writeback cannot schedule another operation");}
 {std::promise<void> called;auto ready=called.get_future();PrefinishWriteback writeback(8*1048576LL,1,[&]()->bool{called.set_value();throw std::runtime_error("owned writeback error");});writeback.progress(4*1048576LL);check(ready.wait_for(std::chrono::seconds(3))==std::future_status::ready,"The optional background failure is exercised by the worker");auto stats=writeback.finish();check(stats.errors==1&&stats.flushes==1,"An optional early-flush exception is recorded without replacing the required final flush");}
 {auto path=root/L"writeback.bin";atomicText(path,std::string(1048576,'x'),false);auto before=fileHash(path);Handle file(CreateFileW(path.c_str(),GENERIC_WRITE,FILE_SHARE_READ|FILE_SHARE_WRITE|FILE_SHARE_DELETE,nullptr,OPEN_EXISTING,0,nullptr));check(bool(file),"Owned real-file fixture opens the data writeback handle");std::promise<void> entered;auto ready=entered.get_future();PrefinishWriteback writeback(4*1048576LL,1,[&]{bool ok=FlushFileBuffers(file.h)!=FALSE;entered.set_value();return ok;});writeback.progress(2*1048576LL);check(ready.wait_for(std::chrono::seconds(3))==std::future_status::ready,"Background operation performs real FlushFileBuffers on the owned data file");auto stats=writeback.finish();CloseHandle(file.h);file.h=INVALID_HANDLE_VALUE;check(stats.flushes==1&&stats.errors==0&&fileHash(path)==before,"Successful prefinish writeback preserves exact bytes and joins before releasing the handle");}
 for(const int64_t total:{4*1048576LL,64*1048576LL}){
  std::promise<void> first,second;auto a=first.get_future(),b=second.get_future();std::atomic_uint calls=0;
  PrefinishWriteback writeback(total,1,[&]{const auto n=++calls;if(n==1)first.set_value();else if(n==2)second.set_value();return true;});
  writeback.progress(total/2);check(a.wait_for(std::chrono::seconds(3))==std::future_status::ready,"Sequential writeback performs its half-body flush");
  const auto nearEnd=total-std::min<int64_t>(total/8,2*1048576LL);
  writeback.progress(nearEnd-1);check(b.wait_for(std::chrono::milliseconds(50))==std::future_status::timeout,"The distinct near-end writeback is not consumed by an earlier progress update");
  writeback.progress(nearEnd);check(b.wait_for(std::chrono::seconds(3))==std::future_status::ready,"Small and medium sequential responses each execute a distinct near-end flush before EOF");
  const auto stats=writeback.finish();check(stats.flushes==2&&stats.errors==0,"Both sequential milestones finish without dropping or duplicating their operations");
 }
 {
  std::promise<void> firstStarted,release,secondStarted;auto firstReady=firstStarted.get_future();auto released=release.get_future().share();auto secondReady=secondStarted.get_future();std::atomic_uint calls=0;
  PrefinishWriteback writeback(64*1048576LL,1,[&]{const auto n=++calls;if(n==1){firstStarted.set_value();released.wait();}else if(n==2)secondStarted.set_value();return true;});
  struct Release {std::promise<void>& p;~Release(){try{p.set_value();}catch(...){}}} cleanup{release};
  Sleep(100);writeback.progress(32*1048576LL);check(firstReady.wait_for(std::chrono::seconds(3))==std::future_status::ready,"The adaptive fixture reaches its held first real worker operation");
  Sleep(120);writeback.progress(40*1048576LL);release.set_value();const auto deadline=GetTickCount64()+3000;
  while(secondReady.wait_for(std::chrono::milliseconds(1))!=std::future_status::ready&&GetTickCount64()<deadline)writeback.progress(48*1048576LL);
  check(secondReady.wait_for(std::chrono::milliseconds(0))==std::future_status::ready,"A measured slow first flush advances the second operation before the old final-2-MiB threshold");
  auto stats=writeback.finish();check(stats.flushes==2&&stats.errors==0&&stats.adjustments==1&&stats.leadBytes>=16*1048576LL&&stats.leadBytes<=32*1048576LL,"Adaptive runway is bounded to half-body and retains exactly two successful operations");
  writeback.progress(64*1048576LL);check(writeback.finish().flushes==2,"Adaptive completion cannot admit another late operation");
 }
 for(const int64_t total:{32*1048576LL,64*1048576LL}){
  std::promise<void> first,second,third;auto a=first.get_future(),b=second.get_future(),c=third.get_future();std::atomic_uint calls=0;
  PrefinishWriteback writeback(total,1,[&]{const auto n=++calls;if(n==1)first.set_value();else if(n==2)second.set_value();else if(n==3)third.set_value();return true;},true);
  writeback.progress(total/4);check(a.wait_for(std::chrono::seconds(3))==std::future_status::ready,"Bounded medium sequential pacing starts at quarter body");
  writeback.progress(total/2);check(b.wait_for(std::chrono::seconds(3))==std::future_status::ready,"Bounded medium sequential pacing flushes the half-body prefix");
  writeback.progress(total-4*1048576LL-1);check(c.wait_for(std::chrono::milliseconds(20))==std::future_status::timeout,"The final paced milestone cannot be consumed before its four-MiB runway");
  writeback.progress(total-4*1048576LL);check(c.wait_for(std::chrono::seconds(3))==std::future_status::ready,"The third sequential flush starts with four MiB remaining");
  const auto stats=writeback.finish();writeback.progress(total);check(stats.flushes==3&&stats.errors==0&&stats.leadBytes==4*1048576LL&&writeback.finish().flushes==3,"Medium sequential pacing is bounded to three operations and cannot restart after joining");
 }
 {
  std::promise<void> entered,release;auto ready=entered.get_future();auto released=release.get_future().share();std::atomic_uint calls=0;
  PrefinishWriteback writeback(64*1048576LL,1,[&]{if(++calls==1){entered.set_value();released.wait();}return false;},true);
  std::future<void> progress;std::future<PrefinishWriteback::Stats> stopped;
  struct Release {std::promise<void>& p;~Release(){try{p.set_value();}catch(...){}}} cleanup{release};
  writeback.progress(16*1048576LL);check(ready.wait_for(std::chrono::seconds(3))==std::future_status::ready,"Paced cancellation fixture holds an actual in-flight operation");
  progress=std::async(std::launch::async,[&]{writeback.progress(64*1048576LL);});check(progress.wait_for(std::chrono::seconds(1))==std::future_status::ready,"Paced progress coalesces later milestones without waiting for held IO");progress.get();
  stopped=std::async(std::launch::async,[&]{return writeback.finish();});const auto stopDeadline=GetTickCount64()+3000;while(!PrefinishWritebackTestAccess::closing(writeback)&&GetTickCount64()<stopDeadline)Sleep(1);check(PrefinishWritebackTestAccess::closing(writeback)&&stopped.wait_for(std::chrono::milliseconds(20))==std::future_status::timeout,"Paced teardown waits for in-flight IO before a caller releases its handle");
  release.set_value();const auto stats=stopped.get();check(stats.flushes==1&&stats.errors==1,"Paced stop drops unstarted optional work and records failure without scheduling another flush");
 }
 for(const auto config:{std::pair<int64_t,unsigned>{32*1048576LL,2},{64*1048576LL,8}}){
  std::promise<void> entered;auto ready=entered.get_future();std::atomic_uint calls=0;
  PrefinishWriteback writeback(config.first,config.second,[&]{if(++calls==1)entered.set_value();return true;},true,true);
  writeback.progress(config.first/8-1);check(ready.wait_for(std::chrono::milliseconds(20))==std::future_status::timeout,"Bounded split writeback waits for its one-eighth-body milestone");
  writeback.progress(config.first/8);check(ready.wait_for(std::chrono::seconds(3))==std::future_status::ready,"Two- and eight-connection split transfers each admit one early operation");
  writeback.progress(config.first/2);writeback.progress(config.first);const auto stats=writeback.finish();check(stats.flushes==1&&stats.errors==0&&stats.adjustments==0&&stats.leadBytes==config.first-config.first/8,"Split pacing never admits another flush after later ranges finish");
 }
 {
  std::promise<void> entered,release;auto ready=entered.get_future();auto released=release.get_future().share();std::atomic_uint calls=0;
  PrefinishWriteback writeback(64*1048576LL,8,[&]{if(++calls==1){entered.set_value();released.wait();}return false;},true,true);
  std::future<void> progress;std::future<PrefinishWriteback::Stats> stopped;
  struct Release {std::promise<void>& p;~Release(){try{p.set_value();}catch(...){}}} cleanup{release};
  writeback.progress(8*1048576LL);check(ready.wait_for(std::chrono::seconds(3))==std::future_status::ready,"Held split writeback enters its only operation");
  progress=std::async(std::launch::async,[&]{writeback.progress(64*1048576LL);});check(progress.wait_for(std::chrono::seconds(1))==std::future_status::ready,"Other split progress remains nonblocking while writeback is held");progress.get();
  stopped=std::async(std::launch::async,[&]{return writeback.finish();});check(stopped.wait_for(std::chrono::milliseconds(20))==std::future_status::timeout,"Split teardown joins held writeback before its file handle can close");
  release.set_value();const auto stats=stopped.get();writeback.progress(64*1048576LL);check(stats.flushes==1&&stats.errors==1&&writeback.finish().flushes==1,"Split writeback failure is advisory, counted once and cannot restart");
 }
 for(const auto config:{std::pair<int64_t,unsigned>{31*1048576LL,8},{64*1048576LL,9}}){
  std::promise<void> entered;auto ready=entered.get_future();
  PrefinishWriteback writeback(config.first,config.second,[&]{entered.set_value();return true;},true,true);
  writeback.progress(config.first/8);check(ready.wait_for(std::chrono::milliseconds(20))==std::future_status::timeout,"Out-of-scope size or connection count cannot enable split pacing");
  writeback.progress(config.first/2);const bool enteredLegacy=ready.wait_for(std::chrono::seconds(3))==std::future_status::ready;const auto stats=writeback.finish();check(enteredLegacy&&stats.flushes==1&&stats.errors==0,"Out-of-scope scheduler retains the legacy half-body operation");
 }
 {
  PrefinishWriteback idle(8*1048576LL,1,[]{return true;});
  const auto stats=idle.finish(),again=idle.finish();
  check(stats.flushes==0&&stats.seconds==0&&stats.longestFlushSeconds==0&&!stats.inFlightAtFinish&&!stats.discardedPending,
        "Idle teardown distinguishes no optional IO from a completed flush");
  check(std::isfinite(stats.finishWaitSeconds)&&stats.finishWaitSeconds>=0&&again.finishWaitSeconds==stats.finishWaitSeconds,
        "Repeated finish preserves the original bounded join measurement");
 }
 {
  std::promise<void> entered,release;auto ready=entered.get_future();auto released=release.get_future().share();
  PrefinishWriteback held(8*1048576LL,1,[&]{entered.set_value();released.wait();return true;});
  std::future<PrefinishWriteback::Stats> finishing;
  struct Release {std::promise<void>& p;~Release(){try{p.set_value();}catch(...){}}} cleanup{release};
  held.progress(4*1048576LL);
  check(ready.wait_for(std::chrono::seconds(3))==std::future_status::ready,"Diagnostic fixture holds a real optional operation");
  held.progress(7*1048576LL);finishing=std::async(std::launch::async,[&]{return held.finish();});
  const auto deadline=GetTickCount64()+3000;
  while(!PrefinishWritebackTestAccess::closing(held)&&GetTickCount64()<deadline)Sleep(1);
  check(PrefinishWritebackTestAccess::closing(held)&&finishing.wait_for(std::chrono::milliseconds(20))==std::future_status::timeout,
        "Diagnostic fixture observes entered teardown while its IO remains held");
  release.set_value();const auto stats=finishing.get(),again=held.finish();
  check(stats.inFlightAtFinish&&stats.discardedPending&&stats.flushes==1&&stats.errors==0,
        "Teardown reports in-flight IO separately from a discarded unstarted request");
  check(std::isfinite(stats.finishWaitSeconds)&&stats.finishWaitSeconds>0&&stats.longestFlushSeconds>0&&stats.seconds>=stats.longestFlushSeconds,
        "Joined diagnostics report actual wait and longest completed flush durations");
  check(again.finishWaitSeconds==stats.finishWaitSeconds&&again.longestFlushSeconds==stats.longestFlushSeconds&&again.inFlightAtFinish==stats.inFlightAtFinish,
        "Repeated teardown cannot overwrite the original held-operation evidence");
 }
 }catch(const std::exception& e){error=e.what();}atomicText(root/L"results.json",Json{{"passed",error.empty()},{"error",error},{"checks",checks}}.dump(2),false);std::cout<<checks.size()<<" checks; "<<(error.empty()?"passed":error)<<"\n";return error.empty()?0:1;}
