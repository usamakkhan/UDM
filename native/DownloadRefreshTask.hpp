#pragma once
#include "FileInfoInitialView.hpp"
#include <future>
#include <condition_variable>
namespace udm {
enum class DownloadRefreshAction {Begin,Read,Apply,Page,Close};
struct DownloadRefreshView {
 Json record=Json::object(),presentations=Json::array();std::shared_ptr<const Json> candidate;uint64_t offerRevision=0;
 const Json& offer()const{static const Json empty;return candidate?*candidate:empty;}
 bool media=false,ftp=false,sabr=false,newReview=false;
 std::string page;
};
using DownloadRefreshViewPtr=std::shared_ptr<const DownloadRefreshView>;
struct DownloadRefreshCommand {
 DownloadRefreshAction action=DownloadRefreshAction::Read;DownloadRefreshViewPtr shown;
 std::string address,page;bool resume=false;
};
struct DownloadRefreshResult {
 DownloadRefreshAction action=DownloadRefreshAction::Read;DownloadRefreshViewPtr view;
 bool cancelled=false,stale=false,applied=false,closed=false;std::string page,error;Json appliedSelection=Json::object();
};
using DownloadRefreshObserver=std::function<void(const char*)>;
// One lazy, joined worker serializes this modal session. Advisory waits may be
// cancelled; accepted saves and exact displayed-token cleanup must settle.
class DownloadRefreshTask {
 struct Work {DownloadRefreshCommand command;std::promise<DownloadRefreshResult> result;std::function<void()> notify;DownloadRefreshObserver observe;};
 Manager& manager;JobPtr job;bool preserveCaptured,fileInfoMode;bool begun=false,applied=false;
 Json selection,previousChoices=Json::array();std::string encryptedOffer;std::shared_ptr<const Json> cachedOffer;uint64_t offerRevision=0;
 std::mutex mutex;std::condition_variable wake;std::thread worker;std::optional<Work> queued;
 std::future<DownloadRefreshResult> future;std::atomic_bool cancelWaiting=false;bool stopping=false;
 std::unique_lock<std::recursive_mutex> acquire(bool cancellable,const DownloadRefreshObserver& observe){
  for(;;){
   if(cancellable&&cancelWaiting)return {};
   std::unique_lock lock(manager.mutex,std::try_to_lock);if(lock.owns_lock()&&!manager.fileInfoDecisionPending)return lock;if(lock.owns_lock())lock.unlock();
   if(observe)observe("waiting-model");std::unique_lock waiting(mutex);wake.wait_for(waiting,std::chrono::milliseconds(10),[&]{return cancellable&&cancelWaiting.load();});
  }
 }
 void current()const{if(!job||std::find(manager.jobs.begin(),manager.jobs.end(),job)==manager.jobs.end())throw std::runtime_error("This download is no longer in the list.");}
 void selected()const{current();if(!fileInfoSelectionUnchanged(*job,selection))throw std::runtime_error("The download request changed. Close recovery and open it again.");}
 Json presentations()const{auto result=manager.pendingBrowserPresentations(job);if(!result.empty()&&str(result.front(),"kind")!="refresh")result.clear();return result;}
 DownloadRefreshViewPtr snapshot(){
  selected();auto view=std::make_shared<DownloadRefreshView>();const auto& data=job->data;
  for(const char* key:{"FileName","Url","SourceUrl","MediaOutput","FormatDescription","Received","SabrRetainedBytes","StreamCompletedMs","DownloadPage","ProtectedHeaders"})if(auto found=data.find(key);found!=data.end())view->record[key]=*found;
  view->media=hasNonemptyString(data,"SourceUrl")||hasNonemptyString(data,"ProtectedAdaptive");view->ftp=Url(str(data,"Url")).scheme=="ftp";view->sabr=hasNonemptyString(data,"ProtectedSabr");
  const std::string empty;const auto stored=data.find("ProtectedRefreshOffer");const auto& encrypted=stored!=data.end()&&stored->is_string()?stored->get_ref<const std::string&>():empty;
  if(!cachedOffer||encrypted!=encryptedOffer||(cachedOffer->is_object()&&!addressRefreshCandidateCurrent(*cachedOffer,epoch()))){
   if(offerRevision==std::numeric_limits<uint64_t>::max())throw std::runtime_error("Recovery identities exhausted. Close and reopen recovery.");
   auto next=std::make_shared<const Json>(manager.addressRefreshCandidate(job));encryptedOffer=encrypted;cachedOffer=std::move(next);++offerRevision;
  }
  view->candidate=cachedOffer;view->offerRevision=offerRevision;view->presentations=presentations();if(!view->media&&!cachedOffer->is_object())view->presentations=Json::array();const auto reviews=manager.pendingBrowserCaptureReviews(job);
  view->newReview=std::any_of(reviews.begin(),reviews.end(),[&](const auto& value){return std::find(previousChoices.begin(),previousChoices.end(),value)==previousChoices.end();});
  view->page=view->media?str(data,"SourceUrl",str(data,"Url")):recoveryPage(view->record);return view;
 }
 DownloadRefreshResult execute(const DownloadRefreshCommand& command,const DownloadRefreshObserver& observe){
  DownloadRefreshResult result;result.action=command.action;
  try{
   if(command.action==DownloadRefreshAction::Close&&!begun){result.closed=true;return result;}
   const bool cancellable=command.action==DownloadRefreshAction::Begin||command.action==DownloadRefreshAction::Read||command.action==DownloadRefreshAction::Page;
   auto lock=acquire(cancellable,observe);if(!lock.owns_lock()){result.cancelled=true;return result;}
   if(command.action==DownloadRefreshAction::Close){
    if(observe)observe("before-close");manager.cancelAddressRefresh(job);
    // Never replace the submitted, actually displayed cohort with a fresh read.
    if(command.shown)for(const auto& offer:command.shown->presentations)manager.finishBrowserPresentation(offer);
    begun=false;result.closed=true;if(observe)observe("closed");return result;
   }
   current();
   if(command.action==DownloadRefreshAction::Begin){
    if(!begun){if(observe)observe("before-begin");previousChoices=manager.pendingBrowserCaptureReviews(job);manager.beginAddressRefresh(job,preserveCaptured);begun=true;selection=fileInfoSelection(*job);if(observe)observe("begun");}
    result.view=snapshot();return result;
   }
   if(!begun)throw std::runtime_error("Open download recovery again before using this command.");selected();
   if(command.action==DownloadRefreshAction::Read){if(observe)observe("before-read");result.view=snapshot();return result;}
   if(command.action==DownloadRefreshAction::Page){if(observe)observe("before-page");result.page=manager.mediaRefreshPage(job);return result;}
   if(applied)throw std::runtime_error("This address was already saved. Close recovery to continue.");
   if(fileInfoMode&&command.resume)throw std::runtime_error("Save the address and start it from Download File Info.");
   if(!command.shown)throw std::runtime_error("Wait for the recovery details before saving.");
   if(observe)observe("before-apply");const auto latest=manager.addressRefreshCandidate(job);const auto pending=presentations();
   if(latest!=command.shown->offer()||pending!=command.shown->presentations){result.stale=true;result.view=snapshot();return result;}
   if(command.shown->media)manager.applyMediaRefresh(job);
   else{std::optional<Headers> headers;if(latest.is_object()&&command.address==str(latest,"url"))headers=latest.at("headers").get<Headers>();manager.refreshAddress(job,trim(command.address),headers,trim(command.page));}
   applied=result.applied=true;selection=fileInfoSelection(*job);result.appliedSelection=selection;if(observe)observe("saved");if(command.resume)manager.resume(job);
  }catch(const std::exception& failure){result.error=failure.what();}
  catch(...){result.error="Download recovery could not finish. Retry or close this window.";}
  return result;
 }
 void run()noexcept{
  for(;;){std::optional<Work> work;
   {std::unique_lock lock(mutex);wake.wait(lock,[&]{return stopping||queued.has_value();});if(!queued)return;work=std::move(queued);queued.reset();}
   try{work->result.set_value(execute(work->command,work->observe));}catch(...){work->result.set_exception(std::current_exception());}
   if(work->notify)try{work->notify();}catch(...) {}
  }
 }
public:
 DownloadRefreshTask(Manager& owner,JobPtr value,bool preserve,bool fileInfo=false):manager(owner),job(std::move(value)),preserveCaptured(preserve),fileInfoMode(fileInfo){}
 DownloadRefreshTask(const DownloadRefreshTask&)=delete;DownloadRefreshTask& operator=(const DownloadRefreshTask&)=delete;
 ~DownloadRefreshTask(){finish();}
 bool valid()const noexcept{return future.valid();}
 bool ready()const{return future.valid()&&future.wait_for(std::chrono::seconds(0))==std::future_status::ready;}
 bool start(DownloadRefreshCommand command,std::function<void()> notify={},DownloadRefreshObserver observe={}){
  if(valid())return false;std::lock_guard lock(mutex);if(stopping)return false;cancelWaiting=false;std::promise<DownloadRefreshResult> result;future=result.get_future();
  try{if(!worker.joinable())worker=std::thread([this]{run();});queued.emplace(Work{std::move(command),std::move(result),std::move(notify),std::move(observe)});}catch(...){future={};throw;}wake.notify_one();return true;
 }
 void cancelRead()noexcept{cancelWaiting=true;wake.notify_all();}
 DownloadRefreshResult take(){if(!ready())throw std::runtime_error("Download recovery is still working.");return future.get();}
 void finish()noexcept{
  cancelRead();{std::lock_guard lock(mutex);stopping=true;}wake.notify_all();if(worker.joinable())worker.join();future={};
  if(begun)try{manager.cancelAddressRefresh(job);begun=false;}catch(...) {}
 }
};
#ifdef UDM_TOOLBAR_COMPONENT_TEST
inline DownloadRefreshObserver downloadRefreshTestObserver;
#endif
}
