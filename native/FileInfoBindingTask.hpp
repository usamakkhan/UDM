#pragma once
#include "PendingFileInfoInbox.hpp"
#include "CaptureReceipts.hpp"
#include <future>
#include <condition_variable>
namespace udm {
struct FileInfoBindingResult {
 std::shared_ptr<const PendingFileInfoInbox::Operation> operation;JobPtr job;Json presentation=Json::object();bool deferred=false,cancelled=false;
};
using FileInfoBindingObserver=std::function<void(const char*)>;
class FileInfoBindingTask {
 std::atomic_bool stopping=false;std::mutex waitMutex;std::condition_variable wake;std::future<FileInfoBindingResult> future;std::thread worker;
 FileInfoBindingResult read(Manager& manager,std::shared_ptr<const PendingFileInfoInbox::Operation> operation,const FileInfoBindingObserver& observe){
  FileInfoBindingResult result;result.operation=operation;
  for(;;){
   if(stopping||operation->cancelRequested()){result.cancelled=true;return result;}
   std::unique_lock lock(manager.mutex,std::try_to_lock);
   if(lock.owns_lock()){
    if(observe)observe("before-read");
    if(manager.fileInfoDecisionPending){result.deferred=true;return result;}
    if(operation->outcome()==PendingFileInfoInbox::Outcome::Pending||operation->retrying()){result.deferred=true;return result;}
    for(const auto& candidate:manager.jobs)if(candidate->id()==operation->view->jobId){result.job=candidate;break;}
    if(!result.job)return result;
    if(str(result.job->data,"Status")!="Awaiting confirmation"||yes(result.job->data,"IndividualStart")){result.job.reset();return result;}
    if(!operation->token.empty()){
     const auto& rows=captureStoreView(manager);const auto found=rows.find(operation->token);
     if(found==rows.end()||num(*found,"protocol")!=3||str(*found,"status")!="accepted"||str(*found,"id")!=operation->view->jobId||reveal(str(*found,"fingerprint"))!=operation->fingerprint())throw std::runtime_error("The accepted media receipt differs from this File Info selection. Check download history.");
     const auto offers=manager.pendingBrowserPresentations(result.job);
     for(const auto& offer:offers){const auto tokens=offer.find("tokens");if(str(offer,"kind")=="download"&&tokens!=offer.end()&&std::find(tokens->begin(),tokens->end(),Json(operation->token))!=tokens->end()){result.presentation=offer;break;}}
     if(result.presentation.empty()||manager.restoreBrowserPresentation(result.presentation)!=result.job)throw std::runtime_error("This browser presentation is no longer available. Check download history.");
    }else if(!operation->fingerprint().empty())throw std::runtime_error("The prepared media receipt is missing.");
    const auto& view=*operation->view;
    const bool unchanged=view.selection.empty()?sameFileInfoInitialView(*captureFileInfoInitialView(manager,*result.job,operation->fingerprint()),view):fileInfoSelectionUnchanged(*result.job,view.selection);
    if(!unchanged)throw std::runtime_error("The selected download request changed while File Info was preparing. Your draft is retained; check download history.");
    if(observe)observe("captured");return result;
   }
   if(observe)observe("waiting-model");
   std::unique_lock waiting(waitMutex);wake.wait_for(waiting,std::chrono::milliseconds(10),[&]{return stopping.load();});
  }
 }
public:
 FileInfoBindingTask()=default;FileInfoBindingTask(const FileInfoBindingTask&)=delete;FileInfoBindingTask& operator=(const FileInfoBindingTask&)=delete;
 ~FileInfoBindingTask(){finish();}
 bool valid()const noexcept{return future.valid();}
 bool ready()const{return future.valid()&&future.wait_for(std::chrono::seconds(0))==std::future_status::ready;}
 bool start(Manager& manager,std::shared_ptr<const PendingFileInfoInbox::Operation> operation,std::function<void()> notify={},FileInfoBindingObserver observe={}){
  if(valid())return false;if(!operation||!operation->view)throw std::runtime_error("Missing prepared File Info operation.");stopping=false;std::promise<FileInfoBindingResult> promise;future=promise.get_future();
  try{worker=std::thread([this,&manager,operation=std::move(operation),notify=std::move(notify),observe=std::move(observe),promise=std::move(promise)]()mutable{try{promise.set_value(read(manager,std::move(operation),observe));}catch(...){promise.set_exception(std::current_exception());}if(notify)try{notify();}catch(...){}});}catch(...){future={};throw;}return true;
 }
 FileInfoBindingResult take(){if(!ready())throw std::runtime_error("File Info acceptance is still being checked.");if(worker.joinable())worker.join();return future.get();}
 void finish()noexcept{stopping=true;wake.notify_all();if(worker.joinable())worker.join();future={};}
};
#ifdef UDM_TOOLBAR_COMPONENT_TEST
inline FileInfoBindingObserver fileInfoBindingTestObserver;
#endif
}
