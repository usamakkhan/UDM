#pragma once
#include "Core.hpp"
#include "GuiModels.hpp"
#include "DownloadPreview.hpp"
#include "ModelMaintenance.hpp"
namespace udm {
using FileInfoDisplayObserver=std::function<void(const char*)>;
struct FileInfoDisplaySnapshot {
 JobPtr job;bool member=false,draftLogin=false,active=false,canCheck=false,replacementPending=false,canRefreshAddress=false;
 Json data=Json::object(),zipInputs=Json::object();i64 savedOutputSize=-1,assembledOutputSize=-1;
};
using FileInfoDisplaySnapshotPtr=std::shared_ptr<const FileInfoDisplaySnapshot>;
#ifdef UDM_TOOLBAR_COMPONENT_TEST
inline FileInfoDisplayObserver fileInfoDisplayTestObserver;
inline std::function<void(CWnd&,FileInfoDisplaySnapshotPtr)> fileInfoDisplayTestRendered;
#endif
inline FileInfoDisplaySnapshotPtr captureFileInfoDisplay(Manager& manager,const JobPtr& job,bool draftLogin,const FileInfoDisplayObserver& observe={}){
 if(observe)observe("before-read");auto result=std::make_shared<FileInfoDisplaySnapshot>();result->job=job;result->draftLogin=draftLogin;
 {Lock lock(manager.mutex);if(manager.fileInfoDecisionPending)return {};if(!job||std::find(manager.jobs.begin(),manager.jobs.end(),job)==manager.jobs.end())return result;
  result->member=true;const auto& source=job->data;
  for(const char* key:{"Url","SourceUrl","Status","ConfirmationPending","Size","HttpContentEncoding","HttpDecodedSize","HttpDecodedReceived","ContentType","MediaPhase","PrefetchedFinalSize","Received","Error","LastHttpStatus","RefreshPendingValidation","BrowserVerificationRequired","SabrSessionRefreshRequired","SabrRetainedBytes","StreamCompletedMs","EstimatedMediaSize","ReportedMediaSize","BrowserReportedDirectSize","ReplacementOf","ResponseFileNameUrl","ResponseFileName","BrowserReportedFileName","ConnectionStatus"}){
   const auto found=source.find(key);if(found!=source.end())result->data[key]=*found;
  }
  if(job->video)result->data["Video"]={{"Url",str(job->video->data,"Url")},{"Size",num(job->video->data,"Size",-1)}};
  if(job->audio)result->data["Audio"]={{"Url",str(job->audio->data,"Url")},{"Size",num(job->audio->data,"Size",-1)}};
  const bool direct=!hasNonemptyString(source,"ProtectedAdaptive")&&!hasNonemptyString(source,"SourceUrl")&&!source.contains("OfflineProject");
  result->active=manager.isActive(job);result->replacementPending=direct&&yes(source,"RefreshPendingValidation")&&Url(str(source,"Url")).scheme!="ftp";
  result->canCheck=!result->replacementPending&&(!result->active||draftLogin)&&canPreviewDownload(*job,draftLogin);
  result->savedOutputSize=savedMediaOutputSize(source);result->assembledOutputSize=job->assembledPrefetchSize;result->canRefreshAddress=manager.canRefreshAddress(job);
  for(const char* key:{"Url","FileName","ContentType","RequiresRequestCapture","RequiresMediaCapture","RequiresBrowserSessionCapture"}){const auto found=source.find(key);if(found!=source.end())result->zipInputs[key]=*found;}
  if(source.contains("OfflineProject"))result->zipInputs["OfflineProject"]=true;
  // Preview visibility needs only presence, never protected request bodies or
  // capture plans. Explicit Preview keeps its own authoritative snapshot.
  for(const char* key:{"ProtectedRequest","ProtectedAdaptive","SourceUrl","DuplicateOf"})if(hasNonemptyString(source,key))result->zipInputs[key]="present";
 }
 if(observe)observe("captured");return result;
}
class FileInfoDisplayTask {
 struct Request {Manager* manager=nullptr;JobPtr job;bool draftLogin=false;};
 std::mutex mutex;Request requested;FileInfoDisplaySnapshotPtr latest;FileInfoDisplayObserver observer;
 ModelMaintenance maintenance;
 void capture(){Request request;{std::lock_guard<std::mutex> lock(mutex);request=requested;}if(!request.manager)return;auto value=captureFileInfoDisplay(*request.manager,request.job,request.draftLogin,observer);if(value){std::lock_guard<std::mutex> lock(mutex);latest=std::move(value);}}
public:
 struct Result {FileInfoDisplaySnapshotPtr snapshot;std::string error;bool active=false,pending=false;};
 explicit FileInfoDisplayTask(std::function<void()> notify={},FileInfoDisplayObserver observe={}):observer(std::move(observe)),maintenance([this]{capture();},[]{},std::move(notify)){}
 FileInfoDisplayTask(const FileInfoDisplayTask&)=delete;FileInfoDisplayTask& operator=(const FileInfoDisplayTask&)=delete;
 ~FileInfoDisplayTask(){stop();}
 bool request(Manager& manager,const JobPtr& job,bool draftLogin){std::lock_guard<std::mutex> lock(mutex);requested={&manager,job,draftLogin};return maintenance.requestTick();}
 Result take(){const auto status=maintenance.takeResult();std::lock_guard<std::mutex> lock(mutex);return {latest,status.tickError,status.active,status.tickPending};}
 void stop(){maintenance.stop();}
};
inline std::atomic_uint fileInfoDialogOwners=0;
inline bool fileInfoDialogShowing()noexcept{return fileInfoDialogOwners.load()!=0;}
struct FileInfoDialogOwner {FileInfoDialogOwner(){++fileInfoDialogOwners;}~FileInfoDialogOwner(){--fileInfoDialogOwners;}FileInfoDialogOwner(const FileInfoDialogOwner&)=delete;};
}
