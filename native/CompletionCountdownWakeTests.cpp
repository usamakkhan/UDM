#include "CompletionCountdown.hpp"
#include <iostream>
using namespace udm;
template<class F>static bool until(F ready){const auto end=GetTickCount64()+5000;while(!ready()&&GetTickCount64()<end)Sleep(1);return ready();}
int wmain(int argc,wchar_t** argv){
 if(argc!=2)return 2;const auto root=fs::absolute(argv[1]);if(fs::exists(root))return 2;fs::create_directories(root);Json checks=Json::array();std::string error;
 auto check=[&](bool ok,const char* label){checks.push_back({{"name",label},{"passed",ok}});if(!ok)throw std::runtime_error(label);};
 try{
  std::function<void()> slot;unsigned wakes=0;
  {CompletionCountdownWakeGuard guard(slot);guard.bind([&]{++wakes;});slot();check(wakes==1,"The active countdown owns its wake route");
   bool rejected=false;try{CompletionCountdownWakeGuard foreign(slot);}catch(...){rejected=true;}slot();check(rejected&&wakes==2,"A competing owner cannot replace or clear the active route");
   rejected=false;try{guard.bind([]{});}catch(...){rejected=true;}slot();check(rejected&&wakes==3,"Duplicate binding preserves the original notifier");guard.reset();check(!slot,"Explicit modal completion clears the route");guard.bind([&]{++wakes;});}
  check(!slot,"Leaving the countdown scope clears its notifier");
  try{CompletionCountdownWakeGuard guard(slot);guard.bind([]{});throw std::runtime_error("Owned unwind");}catch(...){}
  check(!slot,"Exception unwinding cannot retain a closed countdown route");
  std::atomic_uint callbacks=0,notifications=0;ModelMaintenance worker([&]{++callbacks;},[]{},[&]{++notifications;});CompletionCountdownDeadline deadline;std::string tickError;
  check(deadline.poll(worker,false,tickError,false)==CompletionCountdownTick::Ready,"A wake can refresh eligibility without scheduling a tick");
  auto state=worker.takeResult();check(state.ticks==0&&!state.active&&!state.tickPending,"A passive wake leaves idle maintenance idle");
  check(deadline.poll(worker,false,tickError)==CompletionCountdownTick::Ready&&until([&]{return notifications.load()==1;}),"A real timer pulse still starts and acknowledges scheduler work");
  bool passiveReady=true;for(int i=0;i<100;++i)passiveReady&=deadline.poll(worker,false,tickError,false)==CompletionCountdownTick::Ready;check(passiveReady,"Repeated completion notifications remain read-only wakes");
  state=worker.takeResult();check(callbacks==1&&notifications==1&&state.ticks==1&&!state.active&&!state.tickPending,"A hundred completion wakes cannot create a self-sustaining tick loop");
  check(deadline.poll(worker,false,tickError)==CompletionCountdownTick::Ready&&until([&]{return notifications.load()==2;}),"Subsequent actual timer pulses continue scheduling normally");
  check(deadline.poll(worker,true,tickError,false)==CompletionCountdownTick::Pending,"An expired wake requests the one required fresh final tick");
  check(until([&]{return notifications.load()==3;})&&deadline.poll(worker,true,tickError,false)==CompletionCountdownTick::Ready,"The final scheduler acknowledgement is consumable without another timer pulse");
  for(int i=0;i<100;++i)if(deadline.poll(worker,true,tickError,false)!=CompletionCountdownTick::Ready)throw std::runtime_error("Acknowledged deadline regressed");
  check(callbacks==3&&notifications==3,"Repeated final wakes cannot repeat accepted scheduler work");worker.stop();
  check(deadline.poll(worker,true,tickError,false)==CompletionCountdownTick::Failed,"A stopped worker cannot authorize a countdown from a wake");
  ModelMaintenance failed([]{throw std::runtime_error("Owned final tick failure");},[]{});CompletionCountdownDeadline failure;tickError.clear();
  check(failure.poll(failed,true,tickError,false)==CompletionCountdownTick::Pending,"A passive expired wake still requires an acknowledgement from a failing worker");
  check(until([&]{return failure.poll(failed,true,tickError,false)==CompletionCountdownTick::Failed;})&&tickError=="Owned final tick failure","A failed final acknowledgement rejects execution and preserves the error without a timer");failed.stop();
 }catch(const std::exception& e){error=e.what();}atomicText(root/L"results.json",Json{{"passed",error.empty()},{"checks",checks},{"error",error}}.dump(2),false);std::cout<<checks.size()<<" checks; "<<(error.empty()?"passed":error)<<"\n";return error.empty()?0:1;
}
