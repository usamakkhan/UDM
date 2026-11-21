#pragma once
#include "FileInfoInitialView.hpp"
#include "DownloadPreview.hpp"
#include "QueueRetry.hpp"
#include <future>
#include <condition_variable>
namespace udm {
struct FileInfoPreviewRequest {
 JobPtr job;uint64_t generation=0;std::string expectedUrl,user,password;
 bool automatic=false,draftLogin=false,loginEnabled=false;int timeoutMs=10000;
};
struct FileInfoPreviewResult {
 JobPtr job;uint64_t generation=0;bool cancelled=false,skipped=false;
 std::string address,video,audio,message;std::shared_ptr<DownloadPreview> preview;
};
using FileInfoPreviewObserver=std::function<void(const char*)>;
using FileInfoPreviewFactory=std::function<std::shared_ptr<DownloadPreview>(Json,Json,int,Manager*,JobPtr)>;
class FileInfoPreviewTask {
 std::atomic_bool stopping=false;std::mutex mutex;std::condition_variable wake;
 std::future<FileInfoPreviewResult> future;std::thread worker;std::shared_ptr<DownloadPreview> created;
 bool eligible(Manager& manager,const FileInfoPreviewRequest& request,const Json& record)const{
  if(manager.isActive(request.job)&&!request.draftLogin)return false;
  const bool direct=!hasNonemptyString(record,"SourceUrl")&&!hasNonemptyString(record,"ProtectedAdaptive")&&!record.contains("OfflineProject");
  if(direct&&yes(record,"RefreshPendingValidation")&&Url(str(record,"Url")).scheme!="ftp")return false;
  if(request.automatic&&(!serverRetryReady(record,epoch())||yes(record,"AuthenticationPromptPending")||yes(record,"BrowserVerificationRequired")||num(record,"LastHttpStatus")>=400||num(record,"LastFtpStatus")>=400||yes(record,"WebPageReceived")||yes(record,"ContentPrefixPending")))return false;
  if(request.automatic&&direct&&!request.draftLogin&&reportedDirectResponseSize(record)>0&&hasNonemptyString(record,"BrowserReportedFileName"))return false;
  return canPreviewDownload(*request.job,request.draftLogin);
 }
 std::unique_lock<std::recursive_mutex> acquire(Manager& manager,const FileInfoPreviewObserver& observe){
  for(;;){
   if(stopping)return {};
   std::unique_lock lock(manager.mutex,std::try_to_lock);if(lock.owns_lock()&&!manager.fileInfoDecisionPending)return lock;
   if(lock.owns_lock())lock.unlock();if(observe)observe("waiting-model");
   std::unique_lock waiting(mutex);wake.wait_for(waiting,std::chrono::milliseconds(10),[&]{return stopping.load();});
  }
 }
 FileInfoPreviewResult prepare(Manager& manager,const FileInfoPreviewRequest& request,const FileInfoPreviewObserver& observe,const FileInfoPreviewFactory& factory,const std::function<void()>& complete){
  FileInfoPreviewResult result;result.job=request.job;result.generation=request.generation;Json record,settings,selection;
  const auto current=[&]{return std::find(manager.jobs.begin(),manager.jobs.end(),request.job)!=manager.jobs.end();};
  {
   auto lock=acquire(manager,observe);if(!lock.owns_lock()){result.cancelled=true;return result;}
   if(!current()||str(request.job->data,"Url")!=request.expectedUrl){result.skipped=true;result.message="The download address changed. Refresh its address before checking details.";return result;}
   if(!eligible(manager,request,request.job->data)){result.skipped=true;return result;}
   record=request.job->snapshot();settings=manager.state["Settings"];selection=fileInfoSelection(*request.job);
  }
  if(request.draftLogin){
   record["Status"]="Awaiting confirmation";record["Received"]=0;record["Segments"]=Json::array();record.erase("ContentType");
   auto headers=readHeaders(record);setBasicLogin(headers,request.user,request.password,request.loginEnabled);record["ProtectedHeaders"]=headers.empty()?"":protect(legacyDictionary(Json(headers)).dump());
  }
  if(!canPreviewDownload(record)){result.skipped=true;return result;}
  result.address=str(record,"Url");result.video=record.contains("Video")&&record["Video"].is_object()?str(record["Video"],"Url"):"";result.audio=record.contains("Audio")&&record["Audio"].is_object()?str(record["Audio"],"Url"):"";
  if(observe)observe("captured");
  {
   auto lock=acquire(manager,observe);if(!lock.owns_lock()){result.cancelled=true;return result;}
   if(!current()||!fileInfoSelectionUnchanged(*request.job,selection)){result.skipped=true;result.message="The download request changed while preparing details. Refresh details again.";return result;}
   if(!eligible(manager,request,request.job->data)||stopping){result.cancelled=stopping;result.skipped=!stopping;return result;}
   // Authorize launching under the model lock. The factory only starts the
   // existing bounded probe; it does not wait for its HTTP result.
   auto value=factory?factory(std::move(record),std::move(settings),request.timeoutMs,request.draftLogin?nullptr:&manager,request.job):std::make_shared<DownloadPreview>(std::move(record),std::move(settings),request.timeoutMs,request.draftLogin?nullptr:&manager,request.job,[complete,observe]{if(observe)try{observe("probe-ready");}catch(...){}if(complete)complete();});
   {std::lock_guard<std::mutex> guard(mutex);created=value;if(stopping&&value)value->stop();}
   if(stopping){result.cancelled=true;return result;}result.preview=std::move(value);
  }
  if(observe)observe("launched");return result;
 }
public:
 FileInfoPreviewTask()=default;FileInfoPreviewTask(const FileInfoPreviewTask&)=delete;FileInfoPreviewTask& operator=(const FileInfoPreviewTask&)=delete;
 ~FileInfoPreviewTask(){finish();}
 bool valid()const noexcept{return future.valid();}
 bool ready()const{return future.valid()&&future.wait_for(std::chrono::seconds(0))==std::future_status::ready;}
 bool start(Manager& manager,FileInfoPreviewRequest request,std::function<void()> notify={},FileInfoPreviewObserver observe={},FileInfoPreviewFactory factory={}){
  if(valid())return false;if(!request.job||request.expectedUrl.empty())throw std::invalid_argument("Missing download address for file details.");stopping=false;std::promise<FileInfoPreviewResult> result;future=result.get_future();
  try{worker=std::thread([this,&manager,request=std::move(request),notify=std::move(notify),observe=std::move(observe),factory=std::move(factory),result=std::move(result)]()mutable{try{result.set_value(prepare(manager,request,observe,factory,notify));}catch(...){result.set_exception(std::current_exception());}if(notify)try{notify();}catch(...) {}});}catch(...){future={};throw;}return true;
 }
 void cancel()noexcept{stopping=true;wake.notify_all();std::shared_ptr<DownloadPreview> value;{std::lock_guard<std::mutex> lock(mutex);value=created;}if(value)value->stop();}
 FileInfoPreviewResult take(){if(!ready())throw std::runtime_error("File details are still being prepared.");if(worker.joinable())worker.join();struct Clear{FileInfoPreviewTask& owner;~Clear(){std::lock_guard<std::mutex> lock(owner.mutex);owner.created.reset();}} clear{*this};return future.get();}
 void finish()noexcept{cancel();if(worker.joinable())worker.join();future={};std::lock_guard<std::mutex> lock(mutex);created.reset();}
};
#ifdef UDM_TOOLBAR_COMPONENT_TEST
inline FileInfoPreviewObserver fileInfoPreviewTestObserver;
inline std::function<void(CWnd&,const FileInfoPreviewResult&)> fileInfoPreviewTestPublished;
#endif
}
