#pragma once
#include "GuiModels.hpp"
#include "OptionsModel.hpp"
#include <future>
#include <condition_variable>
namespace udm {
inline Json fileInfoCategoryPreferences(const Json& settings){Json result=Json::object();for(const auto* key:{"DownloadFolder","CategoryPaths","CategoryFolders","IdmCategoryDefaults","CategoryRememberLast"})if(auto found=settings.find(key);found!=settings.end())result[key]=*found;return result;}
struct FileInfoCategoryRequest {uint64_t revision=0;std::string category;};
struct FileInfoCategoryResult {FileInfoCategoryRequest request;std::string folder,error;bool remember=false,cancelled=false;};
struct FileInfoCategoryDraft {fs::path destination;std::string rememberFolder;bool remember=false;};
inline std::optional<FileInfoCategoryDraft> mergeFileInfoCategory(const FileInfoCategoryResult& value,uint64_t revision,const std::string& category,const FileInfoCategoryDraft& captured,const FileInfoCategoryDraft& current){
 if(value.cancelled||!value.error.empty()||value.request.revision!=revision||value.request.category!=category)return {};
 auto next=current;const bool sameFolder=current.destination.parent_path()==captured.destination.parent_path();
 if(sameFolder&&!current.destination.filename().empty())next.destination=fs::path(wide(value.folder))/current.destination.filename();
 if(current.remember==captured.remember&&current.rememberFolder==captured.rememberFolder){next.remember=value.remember;next.rememberFolder=sameFolder?value.folder:utf8(current.destination.parent_path().wstring());}
 return next;
}
using FileInfoCategoryObserver=std::function<void(const char*)>;
// One persistent reader, one outstanding result. The UI coalesces newer choices;
// cancellation interrupts advisory lock acquisition and never writes settings.
class FileInfoCategoryTask {
 struct Work {Manager* manager;Json settings;std::vector<std::string> categories;FileInfoCategoryRequest request;std::promise<FileInfoCategoryResult> result;std::function<void()> notify;FileInfoCategoryObserver observe;};
 std::mutex mutex;std::condition_variable wake;std::thread worker;std::optional<Work> queued;std::future<FileInfoCategoryResult> future;std::atomic_bool cancelWaiting=false;bool stopThread=false;
 FileInfoCategoryResult read(Work& work){
  FileInfoCategoryResult result;result.request=work.request;
  try{
   if(!result.request.revision)throw std::runtime_error("Choose the category again.");
   if(work.manager){
    for(;;){
     if(cancelWaiting){result.cancelled=true;return result;}
     std::unique_lock lock(work.manager->mutex,std::try_to_lock);
     if(lock.owns_lock()&&!work.manager->fileInfoDecisionPending){work.settings=fileInfoCategoryPreferences(work.manager->state["Settings"]);work.categories=work.manager->categories();break;}
     if(lock.owns_lock())lock.unlock();if(work.observe)work.observe("waiting-model");std::unique_lock waiting(mutex);wake.wait_for(waiting,std::chrono::milliseconds(10),[&]{return cancelWaiting.load();});
    }
   }
   if(cancelWaiting){result.cancelled=true;return result;}
   if(std::find(work.categories.begin(),work.categories.end(),result.request.category)==work.categories.end())throw std::runtime_error("That category is no longer available. Choose another category.");
   if(work.observe)work.observe("captured");result.folder=categoryFolder(work.settings,result.request.category);
   if(!fs::path(wide(result.folder)).is_absolute())throw std::runtime_error("Choose an absolute download folder for this category.");
   result.remember=yes(work.settings.value("CategoryRememberLast",Json::object()),result.request.category.c_str());result.cancelled=cancelWaiting;
  }catch(const std::exception& failure){result.error=failure.what();}catch(...){result.error="The category folder could not be read. Choose the category again.";}
  return result;
 }
 void run()noexcept{
  for(;;){std::optional<Work> work;{std::unique_lock lock(mutex);wake.wait(lock,[&]{return stopThread||queued.has_value();});if(!queued)return;work=std::move(queued);queued.reset();}
   try{work->result.set_value(read(*work));}catch(...){work->result.set_exception(std::current_exception());}if(work->notify)try{work->notify();}catch(...) {}
  }
 }
public:
 FileInfoCategoryTask()=default;FileInfoCategoryTask(const FileInfoCategoryTask&)=delete;FileInfoCategoryTask& operator=(const FileInfoCategoryTask&)=delete;
 ~FileInfoCategoryTask(){finish();}
 bool valid()const noexcept{return future.valid();}
 bool ready()const{return future.valid()&&future.wait_for(std::chrono::seconds(0))==std::future_status::ready;}
 bool start(Manager* manager,Json settings,std::vector<std::string> categories,FileInfoCategoryRequest request,std::function<void()> notify={},FileInfoCategoryObserver observe={}){
  if(valid())return false;std::lock_guard lock(mutex);cancelWaiting=false;stopThread=false;std::promise<FileInfoCategoryResult> result;future=result.get_future();
  try{if(!worker.joinable())worker=std::thread([this]{run();});queued.emplace(Work{manager,std::move(settings),std::move(categories),std::move(request),std::move(result),std::move(notify),std::move(observe)});}catch(...){future={};throw;}wake.notify_one();return true;
 }
 void cancelRead()noexcept{cancelWaiting=true;wake.notify_all();}
 FileInfoCategoryResult take(){if(!ready())throw std::runtime_error("The category folder is still being read.");return future.get();}
 // Called only after this owner stops reading results; reusable after an
 // exceptional File Info cancellation leaves its dialog open for recovery.
 void finish()noexcept{cancelRead();{std::lock_guard lock(mutex);stopThread=true;}wake.notify_all();if(worker.joinable())worker.join();future={};{std::lock_guard lock(mutex);stopThread=false;}}
};
#ifdef UDM_TOOLBAR_COMPONENT_TEST
inline FileInfoCategoryObserver fileInfoCategoryTestObserver;
#endif
}
