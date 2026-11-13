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
struct StopRetryFixture {std::function<void()> stop;~StopRetryFixture(){try{stop();}catch(...){}}};
static std::shared_ptr<const PendingFileInfoInbox::Operation> rejectFirst(Manager& manager,PendingFileInfoInbox& inbox,const Json& request,std::shared_ptr<const PendingFileInfoInbox::RetryHandler> retry){
 std::shared_ptr<const PendingFileInfoInbox::Operation> operation;
 {PendingFileInfoInbox::Ticket ticket;const PreparedFileInfoSink prepared=[&](auto view){PendingFileInfoInbox::Key key=0;ticket=inbox.publish(std::move(view),[&](auto id){key=id;return true;},str(request,"mediaToken"),retry);operation=inbox.take(key);};
  const RejectedFileInfoSink rejected=[&](const std::string& id){ticket.rejected(id);check(operation&&!operation->finished()&&!operation->canRetry()&&!operation->requestRetry(),"Rejection notification cannot retry while its producer still owns the lease");};
  BrowserAdmissionContext context(true,&prepared,&rejected);bool failed=false;
  try{manager.receiveBrowserMedia(request,[](const char* stage){if(std::string(stage)=="temporary-flushed")throw std::runtime_error("First write failed");});}catch(const std::exception& e){failed=std::string(e.what())=="First write failed";}
  check(failed&&operation&&operation->outcome()==PendingFileInfoInbox::Outcome::Rejected&&!operation->canRetry(),"A rejected result remains unavailable until the producer ticket finishes");}
 check(operation->finished()&&operation->canRetry(),"Unwinding the rejected producer enables an explicit retry");return operation;
}
int wmain(int argc,wchar_t** argv){
 if(argc!=2)return 2;auto root=fs::absolute(argv[1]);if(fs::exists(root))return 3;fs::create_directories(root);std::string error;
 try{
  {
   Manager manager(root/L"cancel/state");mediaSettings(manager,root/L"cancel");auto keep=manager.add("https://files.example.test/keep",utf8((root/L"cancel/files").native()),"keep.bin","Main queue",true);
   PendingFileInfoInbox inbox;inbox.startWorker([&](const auto& operation){cancelPendingFileInfoOffer(manager,operation);});auto request=mediaRequest(mediaToken());
   std::promise<void> entered,release;auto ready=entered.get_future();auto gate=release.get_future().share();std::atomic_int attempts{0};
   auto retry=std::make_shared<const PendingFileInfoInbox::RetryHandler>([&](const PreparedFileInfoSink& prepared,const RejectedFileInfoSink& rejected){++attempts;BrowserAdmissionContext context(true,&prepared,&rejected);auto job=manager.receiveBrowserMedia(request,[&](const char* stage){if(std::string(stage)=="temporary-flushed"){entered.set_value();if(gate.wait_for(std::chrono::seconds(8))!=std::future_status::ready)throw std::runtime_error("Retry gate expired");}});Lock lock(manager.mutex);return captureFileInfoInitialView(manager,*job);});
   StopRetryFixture cleanup{[&]{try{release.set_value();}catch(...){}inbox.close();inbox.stopWorker();}};
   auto parent=rejectFirst(manager,inbox,request,retry);check(parent->requestRetry()&&!parent->requestRetry(),"Only one explicit retry can claim a rejected generation");
   const bool enteredGate=ready.wait_for(std::chrono::seconds(6))==std::future_status::ready;auto child=parent->latest();
   const bool fresh=child!=parent&&child->key!=parent->key&&child->view->jobId!=parent->view->jobId&&child->token==parent->token&&inbox.ownsRetryPresentation(child->view->jobId);
   parent->requestCancel();release.set_value();check(enteredGate&&fresh,"Retry prepares a new job identity while retaining the original token");
   check(until([&]{return child->cancelFinished.load();})&&!child->cancelFailed&&attempts==1,"Cancel through the old generation follows the in-flight retry and resolves once");
   check(parent->cancelFinished&&until([&]{return !inbox.ownsRetryPresentation(child->view->jobId);}),"All generations share cancellation completion and release the claimed retry presentation");
   {Lock lock(manager.mutex);check(manager.jobs.size()==1&&manager.jobs.front()==keep,"Cancelling an accepted retry preserves unrelated history");}
   inbox.close();inbox.stopWorker();manager.stop();
  }
  {
   Manager manager(root/L"reconcile/state");mediaSettings(manager,root/L"reconcile");PendingFileInfoInbox inbox;inbox.startWorker([&](const auto& operation){cancelPendingFileInfoOffer(manager,operation);});auto request=mediaRequest(mediaToken());
   auto retry=std::make_shared<const PendingFileInfoInbox::RetryHandler>([&](const PreparedFileInfoSink& prepared,const RejectedFileInfoSink& rejected){BrowserAdmissionContext context(true,&prepared,&rejected);auto job=manager.receive(request);Lock lock(manager.mutex);return captureFileInfoInitialView(manager,*job);});
   StopRetryFixture cleanup{[&]{inbox.close();inbox.stopWorker();}};
   auto parent=rejectFirst(manager,inbox,request,retry);JobPtr accepted;{BrowserAdmissionContext context;accepted=manager.receive(request);}
   check(parent->requestRetry(),"A rejected generation can request reconciliation after another caller accepts its token");
   check(until([&]{return parent->latest()!=parent&&parent->latest()->finished();}),"Reconciled retry reaches a terminal prepared generation");auto child=parent->latest();
   check(child->outcome()==PendingFileInfoInbox::Outcome::Accepted&&child->view->jobId==accepted->id()&&manager.jobs.size()==1,"An already accepted receipt binds the existing job without a second download");
   check(child->attach()&&!child->canRetry()&&!parent->requestRetry(),"Bound or superseded generations cannot retry again");inbox.close();inbox.stopWorker();manager.stop();
  }
  {
   Manager manager(root/L"repeat/state");mediaSettings(manager,root/L"repeat");PendingFileInfoInbox inbox;inbox.startWorker([&](const auto& operation){cancelPendingFileInfoOffer(manager,operation);});auto request=mediaRequest(mediaToken());std::atomic_int attempts{0};
   auto retry=std::make_shared<const PendingFileInfoInbox::RetryHandler>([&](const PreparedFileInfoSink& prepared,const RejectedFileInfoSink& rejected){const auto attempt=++attempts;BrowserAdmissionContext context(true,&prepared,&rejected);auto job=manager.receiveBrowserMedia(request,[&](const char* stage){if(attempt==1&&std::string(stage)=="temporary-flushed")throw std::runtime_error("Retry write failed");});Lock lock(manager.mutex);return captureFileInfoInitialView(manager,*job);});
   StopRetryFixture cleanup{[&]{inbox.close();inbox.stopWorker();}};
   auto parent=rejectFirst(manager,inbox,request,retry);parent->requestRetry();check(until([&]{return parent->latest()!=parent&&parent->latest()->canRetry();}),"A definitely rejected retry can finish and enable another explicit attempt");
   auto first=parent->latest();const auto firstKey=first->key;check(first->requestRetry()&&!first->requestRetry(),"A retry generation cannot queue its replacement twice");first.reset();
   check(until([&]{return parent->latest()->key!=firstKey&&parent->latest()->outcome()==PendingFileInfoInbox::Outcome::Accepted;}),"The stable presentation follows repeated attempts without retaining a chain of old generations");
   auto current=parent->latest();check(attempts==2&&manager.jobs.size()==1&&current->token==parent->token,"Repeated write failures still produce exactly one accepted job and receipt token");
   check(parent->requestCancel()&&until([&]{return parent->cancelFinished.load();})&&manager.jobs.empty(),"An original-generation Cancel reaches the newest accepted retry");inbox.close();inbox.stopWorker();manager.stop();
  }
  {
   Manager manager(root/L"uncertain/state");mediaSettings(manager,root/L"uncertain");auto job=manager.receive(mediaRequest(mediaToken()));auto view=captureFileInfoInitialView(manager,*job);PendingFileInfoInbox inbox;inbox.startWorker([](const auto&){});
   auto retry=std::make_shared<const PendingFileInfoInbox::RetryHandler>([](const PreparedFileInfoSink&,const RejectedFileInfoSink&)->std::shared_ptr<const FileInfoInitialView>{throw std::runtime_error("Receipt could not be read");});PendingFileInfoInbox::Key key=0;
   {auto ticket=inbox.publish(view,[&](auto id){key=id;return true;},{},retry);ticket.rejected(view->jobId);}auto parent=inbox.take(key);parent->requestRetry();
   check(until([&]{return parent->latest()!=parent&&parent->latest()->finished();}),"Failure before a new prepared identity still terminates its retry generation");
   check(parent->latest()->outcome()==PendingFileInfoInbox::Outcome::Uncertain&&!parent->latest()->canRetry(),"An uncertain retry never grants permission for another admission");inbox.close();inbox.stopWorker();manager.stop();
  }
  {
   Manager manager(root/L"worker/state");mediaSettings(manager,root/L"worker");auto job=manager.receive(mediaRequest(mediaToken()));auto view=captureFileInfoInitialView(manager,*job);
   PendingFileInfoInbox inbox;std::atomic_int cancelled{0};inbox.startWorker([&](const auto&){++cancelled;});
   std::promise<void> entered,release;auto ready=entered.get_future();auto gate=release.get_future().share();
   auto retry=std::make_shared<const PendingFileInfoInbox::RetryHandler>([&](const auto&,const auto&){entered.set_value();if(gate.wait_for(std::chrono::seconds(8))!=std::future_status::ready)throw std::runtime_error("Retry gate expired");return view;});
   StopRetryFixture cleanup{[&]{try{release.set_value();}catch(...){}inbox.close();inbox.stopWorker();}};
   PendingFileInfoInbox::Key key=0;{auto ticket=inbox.publish(view,[&](auto id){key=id;return true;},{},retry);ticket.rejected(view->jobId);}auto parent=inbox.take(key);parent->requestRetry();const bool running=ready.wait_for(std::chrono::seconds(6))==std::future_status::ready;
   PendingFileInfoInbox::Key otherKey=0;{auto ticket=inbox.publish(view,[&](auto id){otherKey=id;return true;});ticket.accepted(view->jobId);}auto other=inbox.take(otherKey);other->requestCancel();const bool responsive=until([&]{return other->cancelFinished.load();});
   inbox.close();release.set_value();inbox.stopWorker();check(running&&responsive,"A blocked retry does not occupy the independent cancellation worker");
   check(cancelled==2&&!parent->canRetry(),"Closing integration cancels the late retry offer before stopping its cancellation worker");manager.stop();
  }
 }catch(const std::exception& e){error=e.what();}
 if(error.empty())for(const auto& result:checks)if(!yes(result,"passed")){error="An observer check failed";break;}
 atomicText(root/L"results.json",Json{{"passed",error.empty()},{"error",error},{"checks",checks}}.dump(2),false);std::cout<<checks.size()<<" checks; "<<(error.empty()?"passed":error)<<"\n";return error.empty()?0:1;
}
