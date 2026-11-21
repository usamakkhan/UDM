#pragma once
#include "OptionsModel.hpp"
#include <future>
#include <condition_variable>
namespace udm {
enum class SchedulePreparationMode {Defaults,Listed,Revise};
struct ScheduleListedContext {
 Json original=Json::object(),queue;fs::path target;std::string category,description,previousQueue;
};
struct SchedulePreparationResult {
 Json settings=Json::object();std::vector<std::string> categories,queues;
 std::optional<i64> connections,speedLimit;std::string error;bool cancelled=false;
 std::optional<ScheduleListedContext> listed;
};
inline Json schedulePreparationPreferences(const Json& settings){
 Json result=Json::object();for(const auto* key:{"DownloadFolder","SaveFolderHistory","CategoryPaths","CategoryFolders","IdmCategoryDefaults","CustomCategories","HiddenBuiltinCategories","CategoryRules","CategoryTypeOverrides"})if(auto found=settings.find(key);found!=settings.end())result[key]=*found;return result;
}
using SchedulePreparationObserver=std::function<void(const char*)>;
// Advisory read ownership only. Accepted schedule writes keep their existing
// FileInfoDecisionTask and revalidate the current catalog at commit time.
class SchedulePreparationTask {
 std::future<SchedulePreparationResult> future;std::thread worker;std::atomic_bool cancelled=false;std::mutex mutex;std::condition_variable wake;
public:
 SchedulePreparationTask()=default;SchedulePreparationTask(const SchedulePreparationTask&)=delete;SchedulePreparationTask& operator=(const SchedulePreparationTask&)=delete;~SchedulePreparationTask(){finish();}
 bool valid()const noexcept{return future.valid();}bool ready()const{return valid()&&future.wait_for(std::chrono::seconds(0))==std::future_status::ready;}
 void start(Manager& manager,JobPtr job={},std::function<void()> notify={},SchedulePreparationObserver observe={},SchedulePreparationMode mode=SchedulePreparationMode::Defaults){
  if(valid()||worker.joinable())throw std::runtime_error("Schedule details are already being prepared.");cancelled=false;std::promise<SchedulePreparationResult> promise;future=promise.get_future();
  try{worker=std::thread([this,&manager,job=std::move(job),notify=std::move(notify),observe=std::move(observe),mode,promise=std::move(promise)]()mutable{
   SchedulePreparationResult result;
   try{
    for(;;){
     if(cancelled){result.cancelled=true;break;}
     std::unique_lock lock(manager.mutex,std::try_to_lock);
     if(lock.owns_lock()&&!manager.fileInfoDecisionPending){
      if(observe)observe("before-read");
      if(job&&std::find(manager.jobs.begin(),manager.jobs.end(),job)==manager.jobs.end())throw std::runtime_error("This download is no longer available. Close the wizard and choose another download.");
      if(mode!=SchedulePreparationMode::Defaults){
       if(!job)throw std::runtime_error("Choose a download to schedule.");
       ScheduleListedContext context;
       if(mode==SchedulePreparationMode::Revise){auto queue=manager.editableDownloadSchedule(job);if(!queue)throw std::runtime_error("This download no longer has an editable schedule.");context.queue=std::move(*queue);}
       else{if(!manager.canScheduleExistingDownload(job))throw std::runtime_error("Pause the download and its queue before adding a schedule.");context.previousQueue=str(job->data,"Queue");}
       for(const auto* field:{"Queue","Folder","FileName","Category","Description","Connections","LimitKbps"})context.original[field]=job->data.value(field,Json());
       context.target=job->target();context.category=str(job->data,"Category");context.description=str(job->data,"Description");result.listed=std::move(context);
      }
      result.settings=schedulePreparationPreferences(manager.state["Settings"]);result.categories=manager.categories();
      for(const auto& queue:manager.state["Queues"])result.queues.push_back(str(queue,"Name"));
      if(job){result.connections=num(job->data,"Connections",8);result.speedLimit=num(job->data,"LimitKbps");}
      break;
     }
     if(lock.owns_lock())lock.unlock();if(observe)observe("waiting-model");std::unique_lock waiting(mutex);wake.wait_for(waiting,std::chrono::milliseconds(10),[&]{return cancelled.load();});
    }
    if(!result.cancelled){
     Json history=Json::array();std::vector<std::string> seen;
     for(const auto& item:result.settings.value("SaveFolderHistory",Json::array())){if(cancelled)break;if(!item.is_string())continue;const auto value=item.get<std::string>();if(fs::path(wide(value)).is_absolute()&&std::find(seen.begin(),seen.end(),value)==seen.end()){seen.push_back(value);history.push_back(value);}if(history.size()==20)break;}
     result.settings["SaveFolderHistory"]=std::move(history);
    }
    result.cancelled=result.cancelled||cancelled.load();if(observe&&!result.cancelled)observe("captured");
   }catch(const std::exception& error){result.error=error.what();result.cancelled=cancelled;}catch(...){result.error="The schedule details could not be prepared. Retry or cancel.";result.cancelled=cancelled;}
   promise.set_value(std::move(result));if(notify)try{notify();}catch(...){}
  });}catch(...){future={};throw;}
 }
 void cancelRead()noexcept{cancelled=true;wake.notify_all();}
 SchedulePreparationResult take(){if(!ready())throw std::runtime_error("Schedule details are still being prepared.");auto result=future.get();if(worker.joinable())worker.join();return result;}
 void finish()noexcept{cancelRead();if(worker.joinable())worker.join();future={};}
};
#ifdef UDM_SCHEDULE_WIZARD_COMPONENT_TEST
inline SchedulePreparationObserver schedulePreparationTestObserver;
#endif
}
