#pragma once
#include "Core.hpp"
#include <future>
#include <condition_variable>
#define UDM_COLUMN_SETTINGS_TASK_AVAILABLE
namespace udm {
inline constexpr int ColumnSettingsCount=12;
struct ColumnSettingsDraft {Json layout;std::vector<int> widths;};
struct ColumnSettingsResult {Json layout,widths;bool saved=false,cancelled=false;std::string error;};
inline void validateColumnSettings(const ColumnSettingsDraft& draft){
 if(draft.widths.size()!=ColumnSettingsCount||!draft.layout.is_object()||draft.layout.size()!=4)throw std::runtime_error("Invalid column layout.");
 for(int i=0;i<ColumnSettingsCount;++i)if(draft.widths[i]<(i==0?40:1)||draft.widths[i]>1200)throw std::runtime_error("Invalid column width.");
 const auto& layout=draft.layout;
 if(!layout.contains("Widths")||!layout["Widths"].is_array()||layout["Widths"].size()!=ColumnSettingsCount||!layout.contains("Order")||!layout["Order"].is_array()||layout["Order"].size()!=ColumnSettingsCount||!layout.contains("SortColumn")||!layout["SortColumn"].is_number_integer()||!layout.contains("Ascending")||!layout["Ascending"].is_boolean())throw std::runtime_error("Invalid column layout.");
 std::set<int> seen;for(int i=0;i<ColumnSettingsCount;++i){const auto& position=layout["Order"][i];const auto& width=layout["Widths"][i];if(!position.is_number_integer()||position<0||position>=ColumnSettingsCount||!seen.insert(position.get<int>()).second||!width.is_number_integer()||(width!=0&&width!=draft.widths[i]))throw std::runtime_error("Invalid column order or visibility.");}
 if(layout["SortColumn"]< -1||layout["SortColumn"]>=ColumnSettingsCount)throw std::runtime_error("Invalid column sort.");
}
// Own a cancellable read or an accepted, joined settings save. Never capture a
// window/control; merge only the column fields into current settings on Save.
class ColumnSettingsTask {
 std::future<ColumnSettingsResult> future;std::thread worker;Manager* guard=nullptr;std::atomic_bool cancelled=false;std::mutex mutex;std::condition_variable wake;
 void releaseGuard()noexcept{if(guard){guard->fileInfoDecisionPending=false;guard=nullptr;}}
public:
 ColumnSettingsTask()=default;ColumnSettingsTask(const ColumnSettingsTask&)=delete;ColumnSettingsTask& operator=(const ColumnSettingsTask&)=delete;~ColumnSettingsTask(){finish();}
 bool valid()const noexcept{return future.valid();}bool ready()const{return valid()&&future.wait_for(std::chrono::seconds(0))==std::future_status::ready;}bool saving()const noexcept{return guard!=nullptr;}
 void start(Manager& manager,std::optional<ColumnSettingsDraft> draft={},std::function<void()> notify={},std::function<void(const char*)> observe={}){
  if(valid()||worker.joinable())throw std::runtime_error("Column settings are already pending.");
  if(draft){validateColumnSettings(*draft);bool expected=false;if(!manager.fileInfoDecisionPending.compare_exchange_strong(expected,true))throw std::runtime_error("Another change is being saved. Try again shortly.");guard=&manager;}
  cancelled=false;try{std::promise<ColumnSettingsResult> promise;future=promise.get_future();worker=std::thread([this,&manager,draft=std::move(draft),notify=std::move(notify),observe=std::move(observe),promise=std::move(promise)]()mutable{
   ColumnSettingsResult result;
   try{for(;;){if(!draft&&cancelled){result.cancelled=true;break;}std::unique_lock lock(manager.mutex,std::try_to_lock);
    if(lock.owns_lock()&&(draft||!manager.fileInfoDecisionPending)){
     if(!draft&&cancelled){result.cancelled=true;break;}if(observe)observe(draft?"before-save":"before-read");
     if(draft){result.widths=draft->widths;result.layout=draft->layout;auto next=manager.state["Settings"];next["ColumnWidths"]=result.widths;next["ListLayout"]=result.layout;manager.setSettings(next);result.saved=true;}
     else{result.widths=manager.state["Settings"].value("ColumnWidths",Json());result.layout=manager.state["Settings"].value("ListLayout",Json());}lock.unlock();if(observe)try{observe("captured");}catch(...){}break;
    }
    if(lock.owns_lock())lock.unlock();if(observe)observe(draft?"waiting-save":"waiting-read");std::unique_lock waiting(mutex);wake.wait_for(waiting,std::chrono::milliseconds(10),[&]{return !draft&&cancelled.load();});
   }}catch(const std::exception& error){result.error=error.what();}catch(...){result.error="Column settings could not be processed.";}
   if(!draft&&cancelled)result.cancelled=true;promise.set_value(std::move(result));if(notify)try{notify();}catch(...){}
  });}catch(...){future={};releaseGuard();throw;}
 }
 void cancelRead()noexcept{cancelled=true;wake.notify_all();}
 ColumnSettingsResult take(){if(!ready())throw std::runtime_error("Column settings are not ready.");auto result=future.get();if(worker.joinable())worker.join();releaseGuard();return result;}
 ColumnSettingsResult finish()noexcept{
  cancelRead();if(worker.joinable())worker.join();ColumnSettingsResult result;if(future.valid())try{result=future.get();}catch(...){/* No accepted result is acknowledged on an unexpected future failure. */}future={};releaseGuard();return result;
 }
};
}
