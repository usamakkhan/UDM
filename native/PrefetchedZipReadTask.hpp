#pragma once
#include "FileInfoInitialView.hpp"
#include "ZipPreview.hpp"
#include <future>
#include <condition_variable>
namespace udm {
using PrefetchedZipReadObserver=std::function<void(const char*)>;
// This reader never admits, stops or downloads a transfer. The modal owner
// joins it before Manager can disappear; its result contains no credentials.
class PrefetchedZipReadTask {
 struct Candidate {fs::path path; i64 size=-1;};
 std::atomic_bool stopping=false;std::mutex mutex;std::condition_variable wake;
 std::future<fs::path> result;std::thread worker;std::optional<fs::path> terminal;
 bool wait(){std::unique_lock lock(mutex);wake.wait_for(lock,std::chrono::milliseconds(10),[&]{return stopping.load();});return !stopping;}
 std::unique_lock<std::recursive_mutex> acquire(Manager& manager,const PrefetchedZipReadObserver& observe){
  while(!stopping){std::unique_lock lock(manager.mutex,std::try_to_lock);if(lock.owns_lock()&&!manager.fileInfoDecisionPending)return lock;if(lock.owns_lock())lock.unlock();if(observe)observe("waiting-model");if(!wait())break;}return {};
 }
 fs::path read(Manager& manager,const JobPtr& job,const std::string& url,const std::string& headers,const PrefetchedZipReadObserver& observe){
  Json selection;bool selected=false;
  const auto candidate=[&](bool& invalid)->std::optional<Candidate>{
   if(!job||std::find(manager.jobs.begin(),manager.jobs.end(),job)==manager.jobs.end()||!manager.isActive(job)||!yes(job->data,"ConfirmationPending")||str(job->data,"Url")!=url||str(job->data,"ProtectedHeaders")!=headers||!canPreviewZip(job->data)){invalid=true;return {};}
   if(!selected){selection=fileInfoSelection(*job);selected=true;}else if(!fileInfoSelectionUnchanged(*job,selection)){invalid=true;return {};}
   const auto& record=job->data;const auto receipt=record.find("CompletedPrefetch");const auto size=num(record,"Size",-1);
   if(yes(record,"RefreshPendingValidation")||receipt==record.end()||num(*receipt,"Version")!=1||str(*receipt,"Url")!=url||str(*receipt,"Headers")!=headers||size<0||num(*receipt,"Size",-1)!=size||num(record,"Received")!=size)return {};
   const auto custom=str(record,"PartsFolder");const auto folder=custom.empty()?manager.root/L"parts"/wide(job->id()):fs::path(wide(custom));return Candidate{folder/(wide(job->id())+L".data"),size};
  };
  while(!stopping){
   std::optional<Candidate> saved;bool invalid=false;
   {auto lock=acquire(manager,observe);if(!lock.owns_lock())return {};saved=candidate(invalid);}
   if(invalid)return {};
   if(!saved){if(observe)observe("waiting-receipt");if(!wait())return {};continue;}
   if(observe)observe("before-file-check");if(stopping)return {};
   std::error_code error;const auto size=fs::file_size(saved->path,error);if(error||size!=static_cast<uintmax_t>(saved->size))return {};
   // Disk inspection is outside Manager. Revalidate afterward so a newer
   // request, receipt or parts location cannot inherit this file's result.
   {auto lock=acquire(manager,observe);if(!lock.owns_lock())return {};const auto current=candidate(invalid);if(invalid||!current||current->path!=saved->path||current->size!=saved->size)return {};}
   return stopping?fs::path{}:saved->path;
  }return {};
 }
public:
 PrefetchedZipReadTask(Manager& manager,JobPtr job,std::string url,std::string headers,PrefetchedZipReadObserver observe={},std::function<void()> onReady={}){
  std::promise<fs::path> promise;result=promise.get_future();worker=std::thread([this,&manager,job=std::move(job),url=std::move(url),headers=std::move(headers),observe=std::move(observe),onReady=std::move(onReady),promise=std::move(promise)]()mutable{try{promise.set_value(read(manager,job,url,headers,observe));}catch(...){promise.set_exception(std::current_exception());}if(onReady)try{onReady();}catch(...) {}});
 }
 PrefetchedZipReadTask(const PrefetchedZipReadTask&)=delete;PrefetchedZipReadTask& operator=(const PrefetchedZipReadTask&)=delete;
 ~PrefetchedZipReadTask(){stop();}
 std::optional<fs::path> poll(){if(terminal)return terminal;if(!result.valid()||result.wait_for(std::chrono::seconds(0))!=std::future_status::ready)return {};if(worker.joinable())worker.join();terminal=result.get();return terminal;}
 void cancel()noexcept{stopping=true;wake.notify_all();}
 void stop()noexcept{cancel();if(worker.joinable())worker.join();}
};
#ifdef UDM_TOOLBAR_COMPONENT_TEST
inline PrefetchedZipReadObserver prefetchedZipReadTestObserver;
#endif
}
