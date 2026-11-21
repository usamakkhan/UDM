#pragma once
#include "FileInfoInitialView.hpp"
#include <future>
namespace udm {
using FileInfoPrefetchObserver=std::function<void(const char*)>;
struct FileInfoPrefetchResult {PrefetchAdmission admission=PrefetchAdmission::Unavailable;bool deferred=false;};
// One admission attempt owned by the dialog. No controls, detached threads,
// implicit mutation retries, or Manager lifetime beyond the joined owner.
class FileInfoPrefetchTask {
 std::future<FileInfoPrefetchResult> result;std::thread worker;Json selectionGuard=Json::object();JobPtr selectedJob;
public:
 FileInfoPrefetchTask()=default;FileInfoPrefetchTask(const FileInfoPrefetchTask&)=delete;FileInfoPrefetchTask& operator=(const FileInfoPrefetchTask&)=delete;
 ~FileInfoPrefetchTask(){finish();}
 bool valid()const noexcept{return result.valid();}
 bool ready()const{return result.valid()&&result.wait_for(std::chrono::seconds(0))==std::future_status::ready;}
 bool start(Manager& manager,JobPtr job,std::function<void()> notify={},FileInfoPrefetchObserver observe={},Json expectedSelection={}){
  if(valid())return false;
  if(selectedJob&&selectedJob!=job){selectionGuard=Json::object();selectedJob.reset();}
  if(!expectedSelection.empty()){selectionGuard=std::move(expectedSelection);selectedJob=job;}
  expectedSelection=selectionGuard;
  std::promise<FileInfoPrefetchResult> promise;result=promise.get_future();
  try{worker=std::thread([&manager,job=std::move(job),promise=std::move(promise),notify=std::move(notify),observe=std::move(observe),expectedSelection=std::move(expectedSelection)]()mutable{
   try{
    if(observe)observe("before-admission");FileInfoPrefetchResult value;
    {Lock lock(manager.mutex);
     if(manager.fileInfoDecisionPending)value.deferred=true;
     else if(job&&std::find(manager.jobs.begin(),manager.jobs.end(),job)!=manager.jobs.end()&&(expectedSelection.empty()||fileInfoSelectionUnchanged(*job,expectedSelection)))value.admission=manager.beginPrefetch(job);
    }
    if(observe)observe("admitted");promise.set_value(value);
   }catch(...){promise.set_exception(std::current_exception());}
   if(notify)try{notify();}catch(...){}
  });}catch(...){result={};throw;}
  return true;
 }
 // Retain a selected restoration guard across capacity/decision retries. Only
 // an explicit address change or a different job may discard it.
 void clearSelection(){if(valid())throw std::logic_error("Background preparation is still running.");selectionGuard=Json::object();selectedJob.reset();}
 FileInfoPrefetchResult take(){if(!ready())throw std::runtime_error("Background preparation is still running.");if(worker.joinable())worker.join();return result.get();}
 // Used by a following command or exceptional owner teardown, never by a
 // regular window pulse while the operation is pending. Discarding an error
 // does not undo admission; the following Cancel still cleans current state.
 void finish()noexcept{if(worker.joinable())worker.join();result={};}
};
inline void cancelFileInfoPrefetch(Manager& manager,const JobPtr& job,FileInfoPrefetchTask& admission,bool removeOffer){
 admission.finish();manager.endPrefetch(job,!removeOffer);
 if(removeOffer)manager.cancelUnconfirmedOffer(job,Manager::OfferCancellation::FileInfo);else manager.pause(job);
}
#ifdef UDM_TOOLBAR_COMPONENT_TEST
inline FileInfoPrefetchObserver fileInfoPrefetchTestObserver;
#endif
}
