#pragma once
#include "Core.hpp"
#include "OriginalResponsePolicy.hpp"
#include <future>
#include <tlhelp32.h>
#include <objbase.h>
#include <string_view>
namespace udm {
enum class NetworkCaptureAction {Poll,Start,Capture,Stop};
struct NetworkCaptureSnapshot {Json flows=Json::array();bool loaded=false,endpoints=false,monitorPresent=false,monitoring=false,capturing=false;i64 watched=0,dropped=0;};
using NetworkCaptureSnapshotPtr=std::shared_ptr<const NetworkCaptureSnapshot>;
struct NetworkCaptureOperations {std::function<NetworkCaptureSnapshotPtr(NetworkCaptureAction,bool)> perform;std::function<NetworkCaptureSnapshotPtr()> observe;std::function<void()> release;};
inline std::set<DWORD> networkProcessRoots(){
 std::map<DWORD,DWORD> parents;DWORD currentSession=0;ProcessIdToSessionId(GetCurrentProcessId(),&currentSession);
 Handle snapshot(CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS,0));PROCESSENTRY32W entry{sizeof(entry)};
 if(!snapshot)throw std::runtime_error("Cannot enumerate browser process roots.");
 if(Process32FirstW(snapshot.h,&entry))do{
  auto name=lower(utf8(entry.szExeFile));DWORD session=0;
  if(originalResponseBrowser(name)&&ProcessIdToSessionId(entry.th32ProcessID,&session)&&session==currentSession)parents[entry.th32ProcessID]=entry.th32ParentProcessID;
 }while(Process32NextW(snapshot.h,&entry));
 std::set<DWORD> roots={GetCurrentProcessId()};for(auto& [pid,parent]:parents)if(!parents.count(parent))roots.insert(pid);return roots;
}
class NetworkCaptureBackend {
public:
 Manager* catalog=nullptr;std::function<std::set<DWORD>()> roots;bool capturing=false;std::unique_ptr<Monitor> monitor;std::map<DWORD,FILETIME> watched;
 explicit NetworkCaptureBackend(Manager* manager=nullptr,std::function<std::set<DWORD>()> processes=networkProcessRoots):catalog(manager),roots(std::move(processes)){}
 void stop(){if(monitor){if(catalog)catalog->stopOriginalResponses(*monitor);else monitor->watch({});}monitor.reset();watched.clear();capturing=false;}
 void start(){if(monitor&&monitor->reconnectExisting())return;const bool resumeCapture=capturing;stop();auto driver=std::make_unique<Monitor>();auto pids=roots();std::map<DWORD,FILETIME> next;
  for(auto pid:pids){if(next.size()==32)break;Handle process(OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION,FALSE,pid));FILETIME a{},b{},c{},d{};if(process&&GetProcessTimes(process.h,&a,&b,&c,&d))next[pid]=a;}
  std::vector<DWORD> live;for(auto& item:next)live.push_back(item.first);if(live.empty())throw std::runtime_error("Open a browser before starting the signed monitor.");driver->watch(live);if(resumeCapture)driver->capture({80},true);watched=std::move(next);monitor=std::move(driver);capturing=resumeCapture;
 }
 void capture(){if(!monitor)throw std::runtime_error("Start the signed monitor first.");monitor->capture({80},true);capturing=true;}
 Json poll(){if(!monitor)return Json();try{std::vector<DWORD> live;std::map<DWORD,FILETIME> next;
  for(auto pid:roots()){
   if(next.size()==32)break;
   Handle process(OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION|SYNCHRONIZE,FALSE,pid));FILETIME a{},b{},c{},d{};
   if(process&&WaitForSingleObject(process.h,0)==WAIT_TIMEOUT&&GetProcessTimes(process.h,&a,&b,&c,&d)){next[pid]=a;live.push_back(pid);}
  }
  monitor->watchExisting(live);watched=std::move(next);if(capturing&&!live.empty())monitor->capture({80},true);auto snap=monitor->snapshot();
  if(capturing&&catalog){if(snap.contains("Capture")){auto result=catalog->processOriginalResponses(*monitor,snap["Capture"]);if(!result["Errors"].empty())throw std::runtime_error(str(result["Errors"][0],"Message"));}else catalog->recoverOriginalResponses(true);}
  return snap;
  }catch(...){if(catalog)catalog->recoverOriginalResponses(true);throw;}
 }
 NetworkCaptureSnapshotPtr last=std::make_shared<NetworkCaptureSnapshot>();
 NetworkCaptureSnapshotPtr observe(){auto next=std::make_shared<NetworkCaptureSnapshot>(*last);next->monitorPresent=bool(monitor);next->monitoring=monitor&&monitor->running();next->capturing=capturing;if(!monitor&&!next->endpoints){next->loaded=false;next->flows=Json::array();next->watched=0;next->dropped=0;}return next;}
 NetworkCaptureSnapshotPtr perform(NetworkCaptureAction action,bool fallback){
  HRESULT apartment=E_FAIL;if(action==NetworkCaptureAction::Start){apartment=CoInitializeEx(nullptr,COINIT_APARTMENTTHREADED|COINIT_DISABLE_OLE1DDE);if(FAILED(apartment))throw std::runtime_error("Cannot initialize the monitor launcher apartment.");}struct Apartment{HRESULT value;~Apartment(){if(SUCCEEDED(value))CoUninitialize();}} cleanup{apartment};
  if(action==NetworkCaptureAction::Start)start();else if(action==NetworkCaptureAction::Capture)capture();else if(action==NetworkCaptureAction::Stop)stop();
  auto next=std::make_shared<NetworkCaptureSnapshot>();
  if(monitor){auto raw=poll();next->flows=raw.value("Flows",Json::array());next->watched=num(raw,"Watched");next->dropped=num(raw,"Dropped");next->loaded=true;}
  else if(fallback){next->flows=udm::endpoints();next->loaded=true;next->endpoints=true;}
  next->monitorPresent=bool(monitor);next->monitoring=monitor&&monitor->running();next->capturing=capturing;last=next;return next;
 }
 void release(){monitor.reset();watched.clear();capturing=false;}
};
struct NetworkCaptureView {NetworkCaptureSnapshotPtr snapshot=std::make_shared<NetworkCaptureSnapshot>();std::string error;uint64_t revision=0;std::optional<NetworkCaptureAction> completed;};
// One UI owner, one joined operation. Repeated polls while busy are coalesced;
// a control request is accepted only when idle and is never replayed on error.
class NetworkCaptureSession {
 struct Result {NetworkCaptureSnapshotPtr snapshot;std::string error;};
 Manager* catalog=nullptr;std::shared_ptr<NetworkCaptureBackend> backend;NetworkCaptureOperations operations;std::function<void()> notify;
 std::future<Result> pending;std::thread worker;std::optional<NetworkCaptureAction> active;NetworkCaptureView current;bool used=false;
 bool request(NetworkCaptureAction action,bool fallback,std::function<void()> extra={}){
  if(active)return false;
  std::promise<Result> result;auto future=result.get_future();const auto previous=current.snapshot;auto ops=operations;auto signal=notify;
  worker=std::thread([action,fallback,ops=std::move(ops),previous,result=std::move(result),signal=std::move(signal),extra=std::move(extra)]()mutable{
   try{Result value;try{value.snapshot=ops.perform(action,fallback);}catch(const std::exception& e){value.error=std::string_view(e.what()).substr(0,4096);try{if(ops.observe)value.snapshot=ops.observe();}catch(...){}if(!value.snapshot)value.snapshot=previous;}catch(...){value.snapshot=previous;value.error="The network operation failed.";}result.set_value(std::move(value));}catch(...){result.set_exception(std::current_exception());}
   for(const auto* callback:{&signal,&extra})if(*callback)try{(*callback)();}catch(...){}
  });pending=std::move(future);active=action;used=true;return true;
 }
 bool settle(bool wait){
  if(!active||(!wait&&!ready()))return false;if(worker.joinable())worker.join();const auto action=*active;active.reset();
  try{auto result=pending.get();if(result.snapshot)current.snapshot=std::move(result.snapshot);current.error=std::move(result.error);}catch(const std::exception& e){current.error=e.what();}catch(...){current.error="The network operation could not finish.";}
  current.completed=action;++current.revision;if(action==NetworkCaptureAction::Stop&&current.error.empty())used=false;return true;
 }
public:
 explicit NetworkCaptureSession(Manager* manager=nullptr,std::function<void()> onReady={}):catalog(manager),backend(std::make_shared<NetworkCaptureBackend>(manager)),notify(std::move(onReady)){
  operations.perform=[source=backend](NetworkCaptureAction action,bool fallback){return source->perform(action,fallback);};operations.observe=[source=backend]{return source->observe();};operations.release=[source=backend]{source->release();};
 }
 explicit NetworkCaptureSession(NetworkCaptureOperations source,Manager* manager=nullptr,std::function<void()> onReady={}):catalog(manager),operations(std::move(source)),notify(std::move(onReady)){if(!operations.perform)throw std::runtime_error("Missing network operation source.");}
 NetworkCaptureSession(const NetworkCaptureSession&)=delete;NetworkCaptureSession& operator=(const NetworkCaptureSession&)=delete;
 ~NetworkCaptureSession(){try{stop();}catch(...){try{if(worker.joinable())worker.join();if(operations.release){std::thread cleanup(operations.release);cleanup.join();}}catch(...){}}}
 bool hasCatalog()const noexcept{return catalog!=nullptr;}
 bool busy()const noexcept{return active.has_value();}
 bool ready(){return active&&pending.valid()&&pending.wait_for(std::chrono::seconds(0))==std::future_status::ready;}
 const NetworkCaptureView& view()const noexcept{return current;}
 bool pollReady(){return settle(false);}
 bool requestPoll(bool fallback=false,std::function<void()> extra={}){if(!fallback&&!current.snapshot->monitorPresent)return false;return request(NetworkCaptureAction::Poll,fallback,std::move(extra));}
 bool requestStart(std::function<void()> extra={}){return request(NetworkCaptureAction::Start,true,std::move(extra));}
 bool requestCapture(std::function<void()> extra={}){return request(NetworkCaptureAction::Capture,true,std::move(extra));}
 bool requestStop(std::function<void()> extra={},bool fallback=true){return request(NetworkCaptureAction::Stop,fallback,std::move(extra));}
 // Lifecycle only. Normal UI Stop is asynchronous; shutdown joins the current
 // operation and its final durable stop before the Manager can be destroyed.
 void stop(){settle(true);if(!used)return;if(!request(NetworkCaptureAction::Stop,false))throw std::runtime_error("The network operation is still running.");settle(true);if(!current.error.empty())throw std::runtime_error(current.error);}
};
inline std::atomic_uint networkDialogOwners=0;
inline bool networkDialogShowing()noexcept{return networkDialogOwners.load()!=0;}
struct NetworkDialogOwner {NetworkDialogOwner(){++networkDialogOwners;}~NetworkDialogOwner(){--networkDialogOwners;}NetworkDialogOwner(const NetworkDialogOwner&)=delete;};
}
