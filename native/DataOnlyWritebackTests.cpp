#include "Core.hpp"
#include "DataOnlyWriteback.hpp"
#include <future>
#include <iostream>
using namespace udm;
namespace udm {struct DataOnlyWritebackTestAccess {static std::unique_lock<std::mutex> hold(DataOnlyWriteback& w){return std::unique_lock<std::mutex>(w.mutex);}};}
int wmain(int argc,wchar_t** argv){
 if(argc!=2)return 2;const auto root=fs::absolute(argv[1]);if(fs::exists(root))return 2;
 fs::create_directories(root);Json checks=Json::array();std::string error;
 auto check=[&](bool ok,const char* name){checks.push_back({{"name",name},{"passed",ok}});if(!ok)throw std::runtime_error(name);};
 try{
  {std::promise<void> entered,release,second;auto ready=entered.get_future(),next=second.get_future();auto released=release.get_future().share();std::atomic_uint calls{0},active{0},peak{0};std::future<void> progress;
   DataOnlyWriteback writeback([&]{const auto now=++active;peak.store(std::max(peak.load(),now));if(++calls==1){entered.set_value();released.wait();}else second.set_value();--active;return true;});
   struct Release{std::promise<void>& p;~Release(){try{p.set_value();}catch(...){}}} cleanup{release};
   writeback.progress(DataOnlyWriteback::triggerBytes-1);check(ready.wait_for(std::chrono::milliseconds(20))==std::future_status::timeout,"No request before successful written-byte trigger");
   writeback.progress(DataOnlyWriteback::triggerBytes);check(ready.wait_for(std::chrono::seconds(3))==std::future_status::ready,"Written bytes start cache drainage");
   progress=std::async(std::launch::async,[&]{writeback.progress(2*DataOnlyWriteback::triggerBytes);});check(progress.wait_for(std::chrono::seconds(1))==std::future_status::ready,"Reception does not wait for held writeback");progress.get();release.set_value();
   check(next.wait_for(std::chrono::seconds(3))==std::future_status::ready,"Bytes credited during an operation trigger another drain");const auto stats=writeback.finish();
   check(calls==2&&peak==1&&stats.flushes==2&&stats.errors==0,"Drainage is serialized and successful requests are counted");
   check(stats.notifications==1,"Progress during active IO needs no additional worker wakeup");
   writeback.progress(64*1048576LL);check(writeback.finish().flushes==2,"Finished worker cannot restart");}
  {std::promise<void> entered,release;auto ready=entered.get_future();auto released=release.get_future().share();std::future<PrefinishWriteback::Stats> finish;
   DataOnlyWriteback writeback([&]{entered.set_value();released.wait();return false;});
   struct Release{std::promise<void>& p;~Release(){try{p.set_value();}catch(...){}}} cleanup{release};
   writeback.progress(DataOnlyWriteback::triggerBytes);check(ready.wait_for(std::chrono::seconds(3))==std::future_status::ready,"Optional failure enters its operation");
   writeback.progress(64*1048576LL);finish=std::async(std::launch::async,[&]{return writeback.finish();});check(finish.wait_for(std::chrono::milliseconds(20))==std::future_status::timeout,"Finish joins in-flight IO before handle closure");release.set_value();auto stats=finish.get();
   check(stats.flushes==1&&stats.errors==1&&stats.inFlightAtFinish&&stats.discardedPending,"Failure/shutdown drops pending work without a retry storm");}
  {std::promise<void> entered;auto ready=entered.get_future();DataOnlyWriteback writeback([&]()->bool{entered.set_value();throw std::runtime_error("optional failure");});writeback.progress(DataOnlyWriteback::triggerBytes);check(ready.wait_for(std::chrono::seconds(3))==std::future_status::ready,"Operation exceptions are contained");check(writeback.finish().errors==1,"Exception is an optional writeback error");}
  {DataOnlyWriteback writeback([]{return true;});std::future<void> progress;bool ready;
   {auto held=DataOnlyWritebackTestAccess::hold(writeback);progress=std::async(std::launch::async,[&]{writeback.progress(1048576);});ready=progress.wait_for(std::chrono::seconds(1))==std::future_status::ready;}
   progress.get();check(ready,"Sub-trigger reception does not acquire the worker bookkeeping lock");check(writeback.finish().flushes==0,"Sub-trigger updates never create optional IO");}
  const auto file=root/L"body.data";Handle output(CreateFileW(file.c_str(),GENERIC_READ|GENERIC_WRITE,0,nullptr,CREATE_NEW,FILE_ATTRIBUTE_NORMAL,nullptr));check(bool(output),"Create owned synchronous body handle");
  std::vector<char> bytes(8192,'D');DWORD written=0;check(WriteFile(output.h,bytes.data(),static_cast<DWORD>(bytes.size()),&written,nullptr)&&written==bytes.size(),"Write exact owned fixture bytes");
  const auto call=dataOnlyFlush();check(call&&flushBodyData(call,output.h),"Real data-only operation succeeds on the synchronous file handle");
  check(!flushBodyData(nullptr,output.h),"Unavailable native export cannot pretend success");
  check(FlushFileBuffers(output.h)!=FALSE,"Mandatory full durability flush still succeeds after optional drainage");CloseHandle(output.h);output.h=INVALID_HANDLE_VALUE;
  check(readText(file)==std::string(8192,'D'),"Data remains exact after both writeback stages");
 }catch(const std::exception& e){error=e.what();}
 const bool passed=error.empty();atomicText(root/L"results.json",Json{{"passed",passed},{"checks",checks},{"error",error}}.dump(2));std::cout<<checks.size()<<" checks; "<<(passed?"passed":error)<<"\n";return passed?0:1;
}
