#pragma once
#include "GuiModels.hpp"
#include "OptionsModel.hpp"
#include "DownloadPreview.hpp"
#include <future>
#include <condition_variable>
namespace udm {
struct FileInfoNameDraft {
 fs::path destination;std::string category,ruleFolder;bool remember=false,categoryTouched=false,draftLogin=false;uint64_t categoryRevision=0,previewGeneration=0;
};
inline bool sameFileInfoNameDraft(const FileInfoNameDraft& a,const FileInfoNameDraft& b){return a.destination==b.destination&&a.category==b.category&&a.ruleFolder==b.ruleFolder&&a.remember==b.remember&&a.categoryTouched==b.categoryTouched&&a.draftLogin==b.draftLogin&&a.categoryRevision==b.categoryRevision&&a.previewGeneration==b.previewGeneration;}
struct FileInfoNameRequest {FileInfoNameDraft draft;std::string expectedName,previewAddress;bool directResponse=false;std::vector<std::string> categories;std::shared_ptr<DownloadPreview> preview;};
struct FileInfoNameResult {FileInfoNameRequest request;fs::path destination;std::string category,error;bool categoryChanged=false,remember=false,stale=false,cancelled=false;};
inline std::string fileInfoServerSuggestion(const Json& current,bool draftLogin,bool canCheck,const std::string& previewAddress,const Json& metadata){
 if(!str(current,"ReplacementOf").empty())return {};
 if(!draftLogin&&yes(current,"ConfirmationPending")&&str(current,"ResponseFileNameUrl")==str(current,"Url"))return str(current,"ResponseFileName");
 if((draftLogin||canCheck)&&str(current,"Url")==previewAddress&&str(metadata,"Status")=="Ready")return str(metadata,"SuggestedFileName");
 return draftLogin||yes(current,"RefreshPendingValidation")?std::string{}:str(current,"BrowserReportedFileName");
}
inline Json fileInfoNamePreferences(const Json& settings){Json result=Json::object();for(const auto* key:{"DownloadFolder","CategoryPaths","CategoryFolders","IdmCategoryDefaults","CategoryRememberLast","CustomCategories","HiddenBuiltinCategories","CategoryRules","CategoryTypeOverrides"})if(auto found=settings.find(key);found!=settings.end())result[key]=*found;return result;}
inline FileInfoNameResult planFileInfoServerName(const FileInfoNameRequest& request,const Json& current,const Json& settings,const Json& metadata,bool canCheck,bool active){
 FileInfoNameResult result;result.request=request;auto name=fileInfoServerSuggestion(current,request.draft.draftLogin,canCheck,request.previewAddress,metadata);
 if(name.empty()||safeName(name)!=name||name!=request.expectedName||utf8(request.draft.destination.filename().wstring())==name){result.stale=true;return result;}
 auto folder=request.draft.destination.parent_path();const auto& prior=request.draft.category;auto host=Url(str(current,"Url")).host;
 if(!request.draft.categoryTouched&&!yes(current,"CategoryExplicit")&&prior==downloadCategory(str(current,"FileName"),host,settings)){
  if(!request.draft.draftLogin&&yes(current,"ConfirmationPending")&&str(current,"ResponseFileNameUrl")==str(current,"Url")){auto resolved=reveal(str(current,"ProtectedResolvedUrl"));if(!resolved.empty())host=Url(resolved).host;}
  else if((request.draft.draftLogin||!active)&&str(current,"Url")==request.previewAddress&&str(metadata,"Status")=="Ready"&&!str(metadata,"ResponseHost").empty())host=str(metadata,"ResponseHost");
  const auto proposed=downloadCategory(name,host,settings);
  if(proposed!=prior&&std::find(request.categories.begin(),request.categories.end(),proposed)!=request.categories.end()){
   result.category=proposed;result.categoryChanged=true;result.remember=yes(settings.value("CategoryRememberLast",Json::object()),proposed.c_str());
   const auto automatic=fs::path(wide(categoryFolder(settings,prior)));if(lower(utf8(folder.lexically_normal().wstring()))==lower(utf8(automatic.lexically_normal().wstring())))folder=fs::path(wide(categoryFolder(settings,proposed)));
  }
 }
 result.destination=folder/wide(name);if(!result.destination.is_absolute())throw std::runtime_error("Choose an absolute destination folder first.");return result;
}
using FileInfoNameObserver=std::function<void(const char*)>;
class FileInfoNameTask {
 std::future<FileInfoNameResult> future;std::thread worker;std::atomic_bool cancelled=false;std::mutex mutex;std::condition_variable wake;
public:
 FileInfoNameTask()=default;FileInfoNameTask(const FileInfoNameTask&)=delete;FileInfoNameTask& operator=(const FileInfoNameTask&)=delete;~FileInfoNameTask(){finish();}
 bool valid()const noexcept{return future.valid();}bool ready()const{return valid()&&future.wait_for(std::chrono::seconds(0))==std::future_status::ready;}
 void start(Manager& manager,JobPtr job,FileInfoNameRequest request,std::function<void()> notify={},FileInfoNameObserver observe={}){
  if(valid()||worker.joinable())throw std::runtime_error("A server filename is already being checked.");cancelled=false;std::promise<FileInfoNameResult> promise;future=promise.get_future();
  try{worker=std::thread([this,&manager,job=std::move(job),request=std::move(request),notify=std::move(notify),observe=std::move(observe),promise=std::move(promise)]()mutable{
   FileInfoNameResult result;result.request=request;
   try{
    // A preview owns a different lock; never retain Manager while reading it.
    const auto metadata=request.preview?request.preview->snapshot():Json::object();fs::path savedTarget;
    for(;;){
     if(cancelled){result.cancelled=true;break;}
     std::unique_lock lock(manager.mutex,std::try_to_lock);
     if(lock.owns_lock()&&!manager.fileInfoDecisionPending){
      if(!job||std::find(manager.jobs.begin(),manager.jobs.end(),job)==manager.jobs.end())throw std::runtime_error("This download is no longer available.");
      const auto categories=manager.categories();if(std::find(categories.begin(),categories.end(),request.draft.category)==categories.end())throw std::runtime_error("That category is no longer available. Choose another category.");
      const auto current=job->snapshot();const bool active=manager.isActive(job);const bool replacement=request.directResponse&&yes(current,"RefreshPendingValidation")&&Url(str(current,"Url")).scheme!="ftp";
      result=planFileInfoServerName(request,current,fileInfoNamePreferences(manager.state["Settings"]),metadata,!replacement&&(!active||request.draft.draftLogin)&&canPreviewDownload(*job,request.draft.draftLogin),active);savedTarget=job->target();
      if(!result.stale){if(manager.relocationReserves(result.destination))throw std::runtime_error("That destination is reserved by a file move.");if(manager.downloadDestinationReserved(result.destination,job))throw std::runtime_error("That destination belongs to another download.");}break;
     }
     if(lock.owns_lock())lock.unlock();if(observe)observe("waiting-model");std::unique_lock waiting(mutex);wake.wait_for(waiting,std::chrono::milliseconds(10),[&]{return cancelled.load();});
    }
    if(!result.cancelled&&!result.stale){
     if(observe)observe("checking-destination");if(cancelled)result.cancelled=true;
     else if(result.destination!=savedTarget){std::error_code error;const bool exists=fs::exists(result.destination,error);if(error)throw std::runtime_error("The destination could not be checked. Choose a folder and try again.");if(exists)throw std::runtime_error("Destination already exists. Keep your current name or choose another one.");}
    }
    result.cancelled=result.cancelled||cancelled.load();
   }catch(const std::exception& error){result.error=error.what();result.cancelled=cancelled;}catch(...){result.error="The server filename could not be checked. Try again.";result.cancelled=cancelled;}
   promise.set_value(std::move(result));if(notify)try{notify();}catch(...){}
  });}catch(...){future={};throw;}
 }
 void cancelRead()noexcept{cancelled=true;wake.notify_all();}
 FileInfoNameResult take(){if(!ready())throw std::runtime_error("The server filename is still being checked.");auto result=future.get();if(worker.joinable())worker.join();return result;}
 void finish()noexcept{cancelRead();if(worker.joinable())worker.join();future={};}
};
#ifdef UDM_TOOLBAR_COMPONENT_TEST
inline FileInfoNameObserver fileInfoNameTestObserver;
#endif
}
