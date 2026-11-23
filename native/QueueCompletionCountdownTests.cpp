#include "QueueCompletionCountdown.hpp"
#include <future>
#include <iostream>
using namespace udm;
int wmain(int argc,wchar_t** argv){
 if(argc!=2)return 2;const auto root=fs::absolute(argv[1]);if(fs::exists(root))return 2;fs::create_directories(root);Json checks=Json::array();std::string error;
 auto check=[&](bool ok,const char* label){checks.push_back({{"name",label},{"passed",ok}});if(!ok)throw std::runtime_error(label);};
 try{Manager manager(root/L"catalog");auto& q=manager.state["Queues"][0];setQueueCompletionSteps(q,{"Open file","Exit UDM"},false);q["CompletionCycle"]=guid();q["CompletionCanceled"]=false;manager.state["CompletionWorkEpoch"]=guid();auto event=queueCompletionEvent(q);event["WorkEpoch"]=str(manager.state,"CompletionWorkEpoch");manager.save();
  auto current=[&](bool presentation=true){return tryQueueCompletionCountdown(manager,event,presentation);};
  check(current().state==CompletionCountdownState::Ready,"A current completed queue is eligible");
  check(str(current().shown,"Queue")==queueDisplayLabel(queueDisplayLabels(manager),str(event,"Queue")),"Presentation snapshots the current display label without changing the stored event name");
  check(current(false).shown.empty()&&str(event,"Queue")==str(q,"Name"),"Step checks avoid rebuilding labels and preserve the event identity");
  std::promise<void> held,release;auto gate=release.get_future();std::thread writer([&]{Lock lock(manager.mutex);held.set_value();gate.wait();});held.get_future().wait();const auto begun=GetTickCount64();auto busy=current();const auto elapsed=GetTickCount64()-begun;release.set_value();writer.join();check(busy.state==CompletionCountdownState::Busy&&elapsed<200,"A foreign model writer never blocks queue countdown admission");
  manager.fileInfoDecisionPending=true;check(current().state==CompletionCountdownState::Busy&&manager.fileInfoDecisionPending,"An accepted File Info decision defers queue actions without being borrowed");manager.fileInfoDecisionPending=false;
  auto old=event;event["Cycle"]=guid();check(current().state==CompletionCountdownState::Obsolete,"A stale queue cycle cannot execute");event=old;event["WorkEpoch"]=guid();check(current().state==CompletionCountdownState::Obsolete,"A previous work epoch cannot execute");event=old;event.erase("Cycle");check(current().state==CompletionCountdownState::Obsolete,"Missing cycle identity is rejected");event=old;
  q["CompletionCanceled"]=true;check(current().state==CompletionCountdownState::Obsolete,"Queue cancellation invalidates the countdown");q["CompletionCanceled"]=false;
  auto job=manager.add("https://example.invalid/queue-countdown","","owned.bin",str(event,"Queue"),true);event=queueCompletionEvent(manager.state["Queues"][0]);event["WorkEpoch"]=str(manager.state,"CompletionWorkEpoch");job->data["Status"]="Queued";check(current().state==CompletionCountdownState::Obsolete,"New queued work invalidates completion");job->data["Status"]="Paused";job->data["SyncPending"]=true;check(current().state==CompletionCountdownState::Obsolete,"Pending synchronization invalidates completion");job->data["SyncPending"]=false;job->data["Status"]="Complete";
  check(current().state==CompletionCountdownState::Ready,"Eligibility returns only after all queue work is finished");
  QueueCompletionCountdownAction action(event);check(!action.confirmed&&action.nextStep==0&&action.steps==std::vector<std::string>({"Open file","Exit UDM"}),"An owned action starts with a canonical ordered cursor");
  const auto model=manager.snapshot();const auto disk=readText(manager.root/L"state.json");for(int i=0;i<20;++i)current(false);check(manager.snapshot()==model&&readText(manager.root/L"state.json")==disk,"Queue readiness reads never consume events, tick or checkpoint the model");
  auto changed=manager.state["Queues"][0];setQueueCompletionSteps(changed,{"Exit UDM"},false);manager.setQueue(changed);check(current().state==CompletionCountdownState::Obsolete,"The real queue-settings API invalidates an edited completion plan");
  auto missing=manager.state["Queues"];manager.state["Queues"]=Json::array();check(current().state==CompletionCountdownState::Obsolete,"Removing the queue rejects its pending action");manager.state["Queues"]=missing;manager.stop();
 }catch(const std::exception& e){error=e.what();}atomicText(root/L"results.json",Json{{"passed",error.empty()},{"checks",checks},{"error",error}}.dump(2),false);std::cout<<checks.size()<<" checks; "<<(error.empty()?"passed":error)<<"\n";return error.empty()?0:1;
}
