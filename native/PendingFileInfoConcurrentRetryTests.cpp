#include "Core.hpp"
#include "PendingFileInfoInbox.hpp"
#include "MediaAdmission.hpp"
#include <future>
#include <iostream>
using namespace udm;
static Json checks=Json::array();
static void check(bool ok,const std::string& name){checks.push_back({{"name",name},{"passed",ok}});if(!ok)throw std::runtime_error(name);}
template<class Action>static void rejects(Action&& action,const std::string& name){bool rejected=false;try{action();}catch(const std::exception&){rejected=true;}check(rejected,name);}
#include "MediaAdmissionChecks.hpp"
template<class Predicate>static bool until(Predicate predicate){const auto end=GetTickCount64()+6000;while(!predicate()&&GetTickCount64()<end)Sleep(5);return predicate();}
// Release gates before joining a worker even when an assertion fails.
struct ReleaseGate {std::promise<void>& promise;~ReleaseGate(){try{promise.set_value();}catch(...){}}};
struct StopInbox {PendingFileInfoInbox& inbox;~StopInbox(){inbox.close();inbox.stopWorker();}};
int wmain(int argc,wchar_t** argv){
 if(argc!=2)return 2;const auto root=fs::absolute(argv[1]);if(fs::exists(root))return 3;fs::create_directories(root);std::string error;Json observations=Json::array();
 try{for(bool notified:{false,true})for(bool browserFails:{false,true})for(bool cancel:{false,true}){
  const auto folder=root/(std::wstring(browserFails?L"browser-rejected":L"browser-accepted")+(cancel?L"-cancel":L"-bind")+(notified?L"-notified":L"-plain"));Manager manager(folder/L"state");mediaSettings(manager,folder);
  const auto request=mediaRequest(mediaToken());const auto token=str(request,"mediaToken");std::shared_ptr<const PendingFileInfoInbox::Operation> cancelledOperation;std::atomic_int notifications{0};PendingFileInfoInbox inbox;inbox.startWorker([&](const auto& operation){cancelledOperation=operation.shared_from_this();cancelPendingFileInfoOffer(manager,operation);});StopInbox stop{inbox};
  std::promise<void> retryEntered;auto retryReady=retryEntered.get_future();
  auto retry=std::make_shared<const PendingFileInfoInbox::RetryHandler>([&](const PreparedFileInfoSink& prepared,const RejectedFileInfoSink& rejected){BrowserAdmissionContext context(true,&prepared,&rejected,true);auto job=manager.receiveBrowserMedia(request,[&](const char* phase){if(std::string(phase)=="join-waiting")retryEntered.set_value();});Lock lock(manager.mutex);return captureFileInfoInitialView(manager,*job);});
  std::shared_ptr<const PendingFileInfoInbox::Operation> parent;
  {PendingFileInfoInbox::Ticket ticket;const PreparedFileInfoSink prepared=[&](auto view){PendingFileInfoInbox::Key key=0;ticket=inbox.publish(std::move(view),[&](auto id){++notifications;key=id;return true;},token,retry);parent=inbox.take(key);};const RejectedFileInfoSink rejected=[&](const auto& id){ticket.rejected(id);};BrowserAdmissionContext context(true,&prepared,&rejected);
   std::string failure;try{manager.receiveBrowserMedia(request,[](const char* phase){if(std::string(phase)=="temporary-flushed")throw std::runtime_error("Owned initial rejection");});}catch(const std::exception& e){failure=e.what();}check(failure=="Owned initial rejection","Initial admission reaches its owned flush failure: "+failure);}
  check(parent&&parent->canRetry()&&manager.jobs.empty(),"The original dialog can retry without a tentative history row");
  std::promise<void> entered,release,preparedEntered,publishRelease;auto ready=entered.get_future();auto gate=release.get_future().share();auto preparedReady=preparedEntered.get_future();auto publishGate=publishRelease.get_future().share();std::shared_ptr<const PendingFileInfoInbox::Operation> notifiedGeneration;
  auto browser=std::async(std::launch::async,[&]{PendingFileInfoInbox::Ticket ticket;const PreparedFileInfoSink prepared=[&](auto view){preparedEntered.set_value();if(publishGate.wait_for(std::chrono::seconds(10))!=std::future_status::ready)throw std::runtime_error("Owned publication gate expired");ticket=inbox.publish(std::move(view),[&](auto){++notifications;return true;},token,retry);notifiedGeneration=parent->latest();};const RejectedFileInfoSink rejected=[&](const auto& id){ticket.rejected(id);};BrowserAdmissionContext context(true,notified?&prepared:nullptr,notified?&rejected:nullptr);try{auto job=manager.receiveBrowserMedia(request,[&](const char* phase){if(std::string(phase)=="temporary-flushed"){entered.set_value();if(gate.wait_for(std::chrono::seconds(10))!=std::future_status::ready)throw std::runtime_error("Owned browser gate expired");if(browserFails)throw std::runtime_error("Owned browser rejection");}});ticket.accepted(job->id());return job;}catch(const std::exception& e){if(browserFails&&std::string(e.what())=="Owned browser rejection")return JobPtr{};throw;}});
  ReleaseGate releaseOnExit{release},publishOnExit{publishRelease};
  check((notified?preparedReady:ready).wait_for(std::chrono::seconds(6))==std::future_status::ready,"Browser resubmission holds the same receipt in flight outside the Manager lock");
  {BrowserAdmissionContext ordinary(true);rejects([&]{manager.receive(request);},"Ordinary browser resubmission keeps its non-waiting busy behavior");}
  auto changed=request;changed["filename"]="Different selection";{BrowserAdmissionContext joined(true,nullptr,nullptr,true);rejects([&]{manager.receive(changed);},"A changed fingerprint cannot join a pending token");}
  check(parent->requestRetry(),"The retained dialog explicitly requests Retry while browser admission is pending");
  check(retryReady.wait_for(std::chrono::seconds(6))==std::future_status::ready&&parent->retrying()&&!parent->requestRetry(),"The worker owns one Retry while waiting for the competing admission");
  check(parent->latest()==parent&&manager.snapshot()["Downloads"].empty(),"Joining does not expose a tentative job or replace the presentation early");
  if(notified){publishRelease.set_value();check(ready.wait_for(std::chrono::seconds(6))==std::future_status::ready,"The second browser producer publishes its own prepared view before its flush gate");check(notifications==1&&notifiedGeneration&&notifiedGeneration!=parent&&inbox.ownsRetryPresentation(notifiedGeneration->view->jobId),"A second producer joins the original private presentation without another window notification");check(parent->retrying()&&notifiedGeneration->retrying()&&!notifiedGeneration->canRetry(),"Retry activity belongs to the shared session across producer generations");}
  if(cancel)check(parent->requestCancel(),"Cancel remains actionable through the original generation while joining");
  release.set_value();const auto accepted=browser.get();
  std::shared_ptr<const PendingFileInfoInbox::Operation> child;
  if(cancel){check(until([&]{return parent->cancelFinished.load();})&&!parent->cancelFailed,"Original-generation Cancel completes against the joined accepted identity");child=cancelledOperation;}
  else{check(until([&]{return !parent->retrying()&&parent->latest()!=parent&&parent->latest()->finished();}),"The joined retry resolves when the complete session outcome becomes terminal");child=parent->latest();}
  check(child&&child->outcome()==PendingFileInfoInbox::Outcome::Accepted&&!child->canRetry(),"The retry follows a durable accepted identity instead of remaining uncertain");
  {Lock lock(manager.mutex);check(manager.jobs.size()==(cancel?0:1),"The race leaves exactly one committed download, or none after explicit Cancel");}
  check(browserFails||child->view->jobId==accepted->id(),"Successful browser admission and dialog Retry share exactly the same job identity");
  auto status=mediaAdmissionStatus(manager,token);
  check(str(status,"status")=="accepted"&&str(status,"id")==child->view->jobId&&yes(status,"present")==!cancel,"Recovery retains one accepted receipt with the correct live-history presence");
  if(!cancel)check(child->attach()&&!parent->requestRetry(),"The same presentation can attach after the join without another admission");
  if(notified){check(notifications==1&&notifiedGeneration->finished(),"The second producer retains responsibility for finishing its ticket without another notification");check(browserFails||child==notifiedGeneration,"A matching accepted successor is reused rather than replaced by another retry generation");}
  observations.push_back({{"secondProducerNotified",notified},{"browserAccepted",!browserFails},{"cancelled",cancel},{"dialogStillUncertain",false},{"backendRecoveryVerified",true},{"windowNotifications",notifications.load()}});
  inbox.close();inbox.stopWorker();manager.stop();
 }
 {
  using Inbox=PendingFileInfoInbox;auto view=[&](std::string id){return std::make_shared<const FileInfoInitialView>(std::move(id),Json::object(),Json::object(),Json::object(),root/L"private.mp4",-1,true,false,false,false,std::vector<std::string>{});};
  Inbox inbox;Inbox::Key originalKey=0;int notifications=0;
  {auto ticket=inbox.publish(view("first"),[&](auto key){originalKey=key;++notifications;return true;},"owned-token");ticket.rejected("first");}
  auto ticket=inbox.publish(view("second"),[&](auto){++notifications;return true;},"owned-token");auto successor=inbox.take(originalKey);
  check(notifications==1&&successor&&successor->view->jobId=="second"&&inbox.ownsRetryPresentation("second"),"An undispatched original message claims its successor instead of leaving a suppressed dialog orphaned");
  check(!successor->finished(),"Coalescing does not take over another producer's ticket completion");ticket.accepted("second");ticket=Inbox::Ticket{};check(successor->finished()&&successor->outcome()==Inbox::Outcome::Accepted,"Only the second producer's ticket finishes its successor");rejects([&]{inbox.publish(view("unproven"),[](auto){return true;},"owned-token");},"A settled accepted receipt cannot be replaced by another prepared identity");
  successor->attach();rejects([&]{inbox.publish(view("third"),[](auto){return true;},"owned-token");},"An attached presentation cannot be replaced by a new prepared producer");inbox.close();inbox.stopWorker();
  Inbox independent;int notices=0;auto a=independent.publish(view("a"),[&](auto){++notices;return true;});auto b=independent.publish(view("b"),[&](auto){++notices;return true;});check(notices==2,"Untagged File Info deliveries retain independent notifications");
  Inbox cancelled;Inbox::Key cancelKey=0;int cancelNotices=0;{auto seed=cancelled.publish(view("cancel-first"),[&](auto key){cancelKey=key;++cancelNotices;return true;},"cancel-token");seed.rejected("cancel-first");}auto cancelledParent=cancelled.take(cancelKey);check(cancelledParent->requestCancel(),"A retained rejected presentation can record Cancel before successor publication");auto cancelledTicket=cancelled.publish(view("cancel-second"),[&](auto){++cancelNotices;return true;},"cancel-token");auto cancelledChild=cancelledParent->latest();check(cancelNotices==1&&cancelledChild!=cancelledParent&&cancelledChild->cancelRequested()&&cancelledParent->requestCancel(),"A successor inherits an already recorded Cancel without another notification");
  Inbox capacity;Inbox::Key capacityKey=0;int capacityNotices=0;{auto seed=capacity.publish(view("capacity-first"),[&](auto key){capacityKey=key;++capacityNotices;return true;},"capacity-token");seed.rejected("capacity-first");}for(int index=0;index<63;++index){auto entry=capacity.publish(view("other-"+std::to_string(index)),[&](auto){++capacityNotices;return true;},"token-"+std::to_string(index));}auto replacement=capacity.publish(view("capacity-second"),[&](auto){++capacityNotices;return true;},"capacity-token");check(capacityNotices==64&&capacity.take(capacityKey)->view->jobId=="capacity-second","A same-token replacement succeeds at the 64-presentation bound without adding a slot");rejects([&]{capacity.publish(view("overflow"),[](auto){return true;},"overflow-token");},"A genuinely new presentation still respects the active bound after replacement");
 }
 }catch(const std::exception& e){error=e.what();}
 atomicText(root/L"results.json",Json{{"passed",error.empty()},{"error",error},{"checks",checks},{"observations",observations},{"renderedDialogVerified",false}}.dump(2),false);std::cout<<checks.size()<<" checks; "<<(error.empty()?"passed":error)<<"\n";return error.empty()?0:1;
}
