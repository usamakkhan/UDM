#pragma once
#include "CategoryEditor.hpp"
#include <future>
#include <condition_variable>
namespace udm {
struct CategoryPropertiesDraft {std::string original,name,extensions,hosts,folder;bool remember=false;};
struct CategoryPropertiesResult {Json settings=Json::object();std::vector<std::string> categories;std::string name,error;bool saved=false,cancelled=false;};
inline Json categoryPropertiesPreferences(const Json& settings){Json result=Json::object();for(const auto* key:{"CategoryPaths","CategoryRules","CategoryRememberLast","CustomCategories","HiddenBuiltinCategories","IdmCategoryDefaults","CategoryFolders","DownloadFolder","CategoryTypeOverrides"})if(auto found=settings.find(key);found!=settings.end())result[key]=*found;return result;}
// One owned read or accepted save; no window pointers cross the worker boundary.
class CategoryPropertiesTask {
 std::future<CategoryPropertiesResult> future;std::thread worker;Manager* guard=nullptr;std::atomic_bool cancelled=false;std::mutex mutex;std::condition_variable wake;
public:
 CategoryPropertiesTask()=default;CategoryPropertiesTask(const CategoryPropertiesTask&)=delete;CategoryPropertiesTask& operator=(const CategoryPropertiesTask&)=delete;
 ~CategoryPropertiesTask(){finish();}
 bool valid()const noexcept{return future.valid();}
 bool ready()const{return valid()&&future.wait_for(std::chrono::seconds(0))==std::future_status::ready;}
 bool saving()const noexcept{return guard!=nullptr;}
 void start(Manager& manager,std::optional<CategoryPropertiesDraft> draft={},std::function<void()> notify={},std::function<void(const char*)> observe={}){
  if(valid()||worker.joinable())throw std::runtime_error("Category work is already pending.");
  if(draft){bool expected=false;if(!manager.fileInfoDecisionPending.compare_exchange_strong(expected,true))throw std::runtime_error("Another change is being saved. Try again shortly.");guard=&manager;}
  cancelled=false;
  try{std::promise<CategoryPropertiesResult> result;future=result.get_future();worker=std::thread([this,&manager,draft=std::move(draft),notify=std::move(notify),observe=std::move(observe),result=std::move(result)]()mutable{
   CategoryPropertiesResult value;
   try{
    for(;;){
     if(!draft&&cancelled){value.cancelled=true;break;}
     std::unique_lock lock(manager.mutex,std::try_to_lock);
     if(lock.owns_lock()&&(draft||!manager.fileInfoDecisionPending)){
      if(!draft&&cancelled){value.cancelled=true;break;}
      if(draft){saveCategoryProperties(manager,draft->original,draft->name,draft->extensions,draft->hosts,draft->folder,draft->remember);value.saved=true;value.name=draft->name;}
      value.settings=categoryPropertiesPreferences(manager.state["Settings"]);value.categories=manager.categories();break;
     }
     if(lock.owns_lock())lock.unlock();if(observe)observe(draft?"waiting-save":"waiting-read");std::unique_lock waiting(mutex);wake.wait_for(waiting,std::chrono::milliseconds(10),[&]{return !draft&&cancelled.load();});
    }
   }catch(const std::exception& error){value.error=error.what();}catch(...){value.error="The category settings could not be processed.";}
   result.set_value(std::move(value));if(notify)try{notify();}catch(...){}
  });}catch(...){future={};if(guard){guard->fileInfoDecisionPending=false;guard=nullptr;}throw;}
 }
 void cancelRead()noexcept{cancelled=true;wake.notify_all();}
 CategoryPropertiesResult take(){if(!ready())throw std::runtime_error("Category settings are still pending.");auto result=future.get();if(worker.joinable())worker.join();if(guard){guard->fileInfoDecisionPending=false;guard=nullptr;}return result;}
 void finish()noexcept{cancelRead();if(worker.joinable())worker.join();future={};if(guard){guard->fileInfoDecisionPending=false;guard=nullptr;}}
};
}
