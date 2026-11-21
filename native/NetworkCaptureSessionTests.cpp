#include "NetworkCaptureSession.hpp"
#include <iostream>
#include <array>
using namespace udm;
template<class Predicate>static bool until(Predicate ready,unsigned ms=6000){const auto end=GetTickCount64()+ms;while(!ready()&&GetTickCount64()<end)Sleep(1);return ready();}
struct PromiseRelease {std::promise<void>& value;~PromiseRelease(){try{value.set_value();}catch(...) {}}};
int wmain(int argc,wchar_t** argv){
 if(argc!=2)return 2;const auto root=fs::absolute(argv[1]);if(fs::exists(root))return 2;fs::create_directories(root);Json checks=Json::array();std::string error;
 const auto check=[&](bool ok,const char* name){checks.push_back({{"name",name},{"passed",ok}});if(!ok)throw std::runtime_error(name);};
 try{
  Manager manager(root/L"catalog");manager.save();NetworkCaptureSnapshot state;std::array<std::atomic_uint,4> calls{};std::atomic_uint ownerWakes=0,dialogWakes=0;std::atomic_bool failCapture=false,holdPoll=false;std::promise<void> entered,release;auto ready=entered.get_future();auto released=release.get_future().share();const auto owner=std::this_thread::get_id();std::thread::id operationThread;
  NetworkCaptureOperations operations;
  operations.observe=[&]{return std::make_shared<const NetworkCaptureSnapshot>(state);};
  operations.perform=[&](NetworkCaptureAction action,bool fallback)->NetworkCaptureSnapshotPtr{
   ++calls[static_cast<size_t>(action)];operationThread=std::this_thread::get_id();
   if(action==NetworkCaptureAction::Poll){if(holdPoll){Lock lock(manager.mutex);entered.set_value();released.wait();}state.loaded=true;state.endpoints=!state.monitorPresent&&fallback;state.flows=Json::array({{{"Process","owned-fixture"},{"ProcessId",GetCurrentProcessId()},{"Local","127.0.0.1:1"},{"Remote","127.0.0.1:2"},{"Transport","TCP"},{"State","Established"},{"Received",42},{"Sent",7}}});}
   else if(action==NetworkCaptureAction::Start){state.monitorPresent=true;state.monitoring=true;state.loaded=true;state.endpoints=false;}
   else if(action==NetworkCaptureAction::Capture){state.capturing=true;if(failCapture)throw std::runtime_error("Owned capture acknowledgement lost");}
   else{Lock lock(manager.mutex);auto before=manager.state;manager.state["OwnedNetworkStopped"]=true;try{manager.save();}catch(...){manager.state.swap(before);throw;}state.monitorPresent=false;state.monitoring=false;state.capturing=false;}
   return std::make_shared<const NetworkCaptureSnapshot>(state);
  };
  NetworkCaptureSession session(operations,&manager,[&]{++ownerWakes;});PromiseRelease cleanup{release};
  check(!session.requestPoll()&&!session.busy()&&calls[0]==0,"Idle background polling does not enumerate endpoints or start a worker without a monitor");
  holdPoll=true;check(session.requestPoll(true,[&]{++dialogWakes;}),"An explicit endpoint view accepts one owned poll");check(ready.wait_for(std::chrono::seconds(3))==std::future_status::ready&&session.busy()&&!session.ready(),"Poll returns while its actual model work is held on a different thread");
  const auto initial=session.view().snapshot;check(!session.pollReady()&&!initial->loaded&&operationThread!=owner,"UI observation retains its prior immutable view without waiting for Manager");
  bool coalesced=true;for(int i=0;i<100;++i)coalesced=coalesced&&!session.requestPoll(true);check(coalesced&&calls[0]==1,"Repeated busy polling cannot accumulate another hundred operations");check(!session.requestStart()&&!session.requestCapture()&&!session.requestStop(),"An overlapping control request is rejected rather than queued or replayed");
  release.set_value();holdPoll=false;check(until([&]{return session.ready();}),"The owned poll settles after its held work is released");check(session.pollReady()&&!session.busy()&&session.view().snapshot->flows.size()==1&&session.view().snapshot->endpoints,"A completed endpoint snapshot is published on acknowledgement");check(ownerWakes==1&&dialogWakes==1,"The owner and requesting view each receive exactly one completion wake");check(!initial->loaded&&initial->flows.empty(),"Publishing a newer result cannot mutate a retained earlier snapshot");
  check(session.requestStart()&&until([&]{return session.ready();}),"Start is accepted after the preceding result is acknowledged");session.pollReady();check(session.view().snapshot->monitorPresent&&session.view().snapshot->monitoring&&calls[1]==1,"Start publishes observed monitor state after one accepted mutation");
  failCapture=true;check(session.requestCapture()&&until([&]{return session.ready();}),"An uncertain capture mutation settles asynchronously");session.pollReady();check(calls[2]==1&&!session.view().error.empty()&&session.view().snapshot->capturing,"A lost acknowledgement reports the observed changed state without replaying Capture");for(int i=0;i<10;++i)session.pollReady();check(calls[2]==1,"Repeated result observation cannot replay an uncertain mutation");
  check(session.requestPoll()&&until([&]{return session.ready();}),"The next explicit periodic poll remains available after a control error");session.pollReady();check(session.view().error.empty()&&session.view().snapshot->capturing&&calls[2]==1,"A successful poll refreshes state without resubmitting the failed control");failCapture=false;
  check(session.requestCapture()&&until([&]{return session.ready();}),"A new explicit Capture can be accepted after recovery");session.pollReady();check(calls[2]==2&&session.view().error.empty(),"Only the second explicit request produces the second capture mutation");
  Handle lease(CreateFileW((manager.root/L"state.json").c_str(),GENERIC_READ,FILE_SHARE_READ,nullptr,OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL,nullptr));check(bool(lease),"Pin the real owned catalog before a durable network-stop failure");check(session.requestStop()&&until([&]{return session.ready();}),"UI Stop returns a settled asynchronous failure");session.pollReady();check(!session.view().error.empty()&&session.view().snapshot->monitorPresent,"Failed durable Stop retains the live session for a later explicit recovery");CloseHandle(lease.h);lease.h=INVALID_HANDLE_VALUE;
  session.stop();check(!session.busy()&&!session.view().snapshot->monitorPresent&&session.view().error.empty(),"Lifecycle Stop retries explicitly, joins its operation and commits the real owned stop");const auto stops=calls[3].load();session.stop();check(calls[3]==stops,"A second settled lifecycle Stop does not repeat backend work");
  {
   NetworkCaptureSnapshot live;live.monitorPresent=true;std::promise<void> heldEntered,heldRelease;auto heldReady=heldEntered.get_future();auto heldReleased=heldRelease.get_future().share();std::atomic_uint stopped=0;NetworkCaptureOperations held;
   held.observe=[&]{return std::make_shared<const NetworkCaptureSnapshot>(live);};held.perform=[&](NetworkCaptureAction action,bool){if(action==NetworkCaptureAction::Poll){heldEntered.set_value();heldReleased.wait();}else if(action==NetworkCaptureAction::Stop){++stopped;live.monitorPresent=false;}return std::make_shared<const NetworkCaptureSnapshot>(live);};NetworkCaptureSession owned(held);std::future<void> closing;PromiseRelease releaseGuard{heldRelease};owned.requestPoll(true);check(heldReady.wait_for(std::chrono::seconds(3))==std::future_status::ready,"Teardown starts with a real in-flight owned operation");closing=std::async(std::launch::async,[&]{owned.stop();});check(closing.wait_for(std::chrono::milliseconds(20))==std::future_status::timeout,"Shutdown cannot detach its held network operation");heldRelease.set_value();closing.get();check(stopped==1&&!owned.busy()&&!owned.view().snapshot->monitorPresent,"Shutdown joins the operation and its final stop before returning");
  }
  manager.stop();
 }catch(const std::exception& e){error=e.what();}
 atomicText(root/L"results.json",Json{{"passed",error.empty()},{"checks",checks},{"error",error}}.dump(2),false);std::cout<<checks.size()<<" checks; "<<(error.empty()?"passed":error)<<"\n";return error.empty()?0:1;
}
