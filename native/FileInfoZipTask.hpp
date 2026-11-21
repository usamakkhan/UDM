#pragma once
#include "FileInfoInitialView.hpp"
#include "DownloadPreview.hpp"
#include "BrowserSession.hpp"
#include "ZipPreview.hpp"
#include "PrefetchedZipReadTask.hpp"
#include <future>
#include <condition_variable>
namespace udm {
struct FileInfoZipRequest {JobPtr job;std::string expectedUrl,mime,user,password;bool draftLogin=false,loginEnabled=false;};
struct FileInfoZipOutcome {bool resumePrefetch=false;Json selection=Json::object();};
struct FileInfoZipResult {
 FileInfoZipOutcome outcome;bool cancelled=false,prefetched=false;std::string name,url,headers,message;
 std::shared_ptr<ZipPreviewTask> remote;std::shared_ptr<PrefetchedZipReadTask> reader;
};
using FileInfoZipObserver=std::function<void(const char*)>;
class FileInfoZipTask {
 Manager& manager;FileInfoZipRequest request;std::atomic_bool stopping=false;
 std::mutex mutex;std::condition_variable wake;std::thread worker;std::future<FileInfoZipResult> future;
 std::shared_ptr<ZipPreviewTask> created;std::shared_ptr<PrefetchedZipReadTask> createdReader;
 std::unique_lock<std::recursive_mutex> acquire(const FileInfoZipObserver& observe){
  while(!stopping){std::unique_lock lock(manager.mutex,std::try_to_lock);if(lock.owns_lock()&&!manager.fileInfoDecisionPending)return lock;if(lock.owns_lock())lock.unlock();if(observe)observe("waiting-model");std::unique_lock waiting(mutex);wake.wait_for(waiting,std::chrono::milliseconds(10),[&]{return stopping.load();});}return {};
 }
 FileInfoZipResult prepare(const FileInfoZipObserver& observe,const std::function<void()>& notify){
  FileInfoZipResult result;
  try{
   Json record,settings;bool needsStop=false;
   const auto valid=[&]{if(!request.job||std::find(manager.jobs.begin(),manager.jobs.end(),request.job)==manager.jobs.end()||str(request.job->data,"Url")!=request.expectedUrl)throw std::runtime_error("The download address changed. Close Preview and try again.");auto candidate=request.job->data;if(!request.mime.empty())candidate["ContentType"]=request.mime;if(!canPreviewZip(candidate)||(yes(candidate,"RefreshPendingValidation")&&Url(str(candidate,"Url")).scheme!="ftp"))throw std::runtime_error("ZIP preview is unavailable for the current request. Refresh its details and try again.");};
   const auto selected=[&]{valid();if(!fileInfoSelectionUnchanged(*request.job,result.outcome.selection))throw std::runtime_error("The selected request changed. Close Preview and try again.");};
   {
    auto lock=acquire(observe);if(!lock.owns_lock()){result.cancelled=true;return result;}valid();record=request.job->snapshot();settings=manager.state["Settings"];result.outcome.selection=fileInfoSelection(*request.job);result.name=str(record,"FileName");
    if(!request.draftLogin&&manager.isActive(request.job)&&yes(record,"ConfirmationPending")&&Url(str(record,"Url")).scheme!="ftp"){
     result.prefetched=true;result.url=str(record,"Url");result.headers=str(record,"ProtectedHeaders");result.reader=std::make_shared<PrefetchedZipReadTask>(manager,request.job,result.url,result.headers,PrefetchedZipReadObserver{},notify);{std::lock_guard guard(mutex);createdReader=result.reader;if(stopping)createdReader->cancel();}return result;
    }
    needsStop=yes(record,"ConfirmationPending");
   }
   if(observe)observe("captured");
   if(stopping){result.cancelled=true;return result;}
   if(needsStop){
    if(observe)observe("before-stop");if(stopping){result.cancelled=true;return result;}
    // Manager validates the same selection under its own stop lock, including
    // after waiting for the transfer. Never hold an outer recursive lock here.
    result.outcome.resumePrefetch=!request.draftLogin;
    manager.endPrefetch(request.job,false,&result.outcome.selection);
    if(observe)observe("stopped-prefetch");
    auto lock=acquire(observe);if(!lock.owns_lock()){result.cancelled=true;return result;}selected();record=request.job->snapshot();
   }
   if(!request.mime.empty())record["ContentType"]=request.mime;
   if(request.draftLogin){auto headers=readHeaders(record);setBasicLogin(headers,request.user,request.password,request.loginEnabled);record["ProtectedHeaders"]=headers.empty()?"":protect(legacyDictionary(Json(headers)).dump());}
   auto persist=request.draftLogin?std::function<void(const Json&)>{}:browserSessionSaver(manager,request.job,readBrowserSession(record));
   if(observe)observe("before-launch");
   {
    auto lock=acquire(observe);if(!lock.owns_lock()){result.cancelled=true;return result;}selected();
    if(manager.isActive(request.job))throw std::runtime_error("The download started while preparing Preview. Close Preview and try again.");
    auto value=std::make_shared<ZipPreviewTask>(std::move(record),std::move(settings),std::move(persist),30000,notify);
    {std::lock_guard guard(mutex);created=value;if(stopping)value->stop();}result.remote=std::move(value);
   }
  }catch(const std::exception& failure){result.message=failure.what();}
  catch(...){result.message="Cannot prepare ZIP preview. Close Preview and try again.";}
  result.cancelled=stopping;if(result.cancelled&&result.remote)result.remote->stop();return result;
 }
public:
 FileInfoZipTask(Manager& owner,FileInfoZipRequest value):manager(owner),request(std::move(value)){}
 FileInfoZipTask(const FileInfoZipTask&)=delete;FileInfoZipTask& operator=(const FileInfoZipTask&)=delete;
 ~FileInfoZipTask(){finish();}
 bool valid()const noexcept{return future.valid();}
 bool ready()const{return future.valid()&&future.wait_for(std::chrono::seconds(0))==std::future_status::ready;}
 void start(std::function<void()> notify={},FileInfoZipObserver observe={}){
  if(valid())throw std::runtime_error("ZIP preparation is already running.");stopping=false;std::promise<FileInfoZipResult> result;future=result.get_future();
  try{worker=std::thread([this,result=std::move(result),notify=std::move(notify),observe=std::move(observe)]()mutable{try{result.set_value(prepare(observe,notify));}catch(...){result.set_exception(std::current_exception());}if(notify)try{notify();}catch(...) {}});}catch(...){future={};throw;}
 }
 void cancel()noexcept{stopping=true;wake.notify_all();std::shared_ptr<ZipPreviewTask> value;{std::lock_guard guard(mutex);value=created;}if(value)value->stop();std::shared_ptr<PrefetchedZipReadTask> reader;{std::lock_guard guard(mutex);reader=createdReader;}if(reader)reader->cancel();}
 FileInfoZipResult take(){if(!ready())throw std::runtime_error("ZIP preparation is still running.");if(worker.joinable())worker.join();struct Clear{FileInfoZipTask& owner;~Clear(){std::lock_guard guard(owner.mutex);owner.created.reset();owner.createdReader.reset();}} clear{*this};return future.get();}
 void finish()noexcept{cancel();if(worker.joinable())worker.join();future={};std::lock_guard guard(mutex);created.reset();createdReader.reset();}
};
#ifdef UDM_TOOLBAR_COMPONENT_TEST
inline FileInfoZipObserver fileInfoZipTestObserver;
#endif
}
