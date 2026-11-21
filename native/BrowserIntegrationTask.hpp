#pragma once
#include "BrowserSettings.hpp"
#include <future>
#include <condition_variable>
#define UDM_BROWSER_INTEGRATION_TASK_AVAILABLE
namespace udm {
inline const Json& browserIntegrationDefaults(){static const Json fields={{"BrowserCaptureEnabled",true},{"CaptureExcludedUrls",""},{"CaptureBypassKey","Alt"},{"CaptureForceKey","Ctrl"},{"CaptureForceClick",true},{"CaptureForceSkipWeb",true},{"CaptureWebPlayers",false},{"VideoPanelShowProtected",true},{"SelectedLinkHosts",""},{"SelectedLinksCompact",false},{"SelectedLinksMode","all"},{"VideoPanelCompact",false},{"VideoPanelEnabled",true},{"VideoPanelHover",false},{"VideoPanelMenuWidth",490},{"VideoPanelPosition","Top right"},{"VideoPanelReset",0},{"VideoPanelExcludedHosts",""},{"VideoPanelMinKb",Json::object()},{"VideoPanelTypes",defaultVideoPanelTypes()},{"BrowserContextMenus",Json::object()}};return fields;}
inline Json browserIntegrationSnapshot(const Json& settings){auto result=settings;for(auto it=browserIntegrationDefaults().begin();it!=browserIntegrationDefaults().end();++it)if(!result.contains(it.key()))result[it.key()]=*it;result["VideoPanelTypes"]=videoPanelTypes(settings);for(const std::string family:{"chromium","firefox"}){auto row=browserContextMenu(settings,family);if(!row.contains("Link"))row["Link"]=true;if(!row.contains("All"))row["All"]=true;result["BrowserContextMenus"][family]=std::move(row);}return result;}
struct BrowserIntegrationDraft {Json base,edited;};
struct BrowserIntegrationEdit {std::vector<std::string> path;std::optional<Json> value;};
inline void browserIntegrationDiff(const Json& base,const Json& edited,std::vector<std::string>& path,std::vector<BrowserIntegrationEdit>& changes){
 std::set<std::string> keys;for(auto it=base.begin();it!=base.end();++it)keys.insert(it.key());for(auto it=edited.begin();it!=edited.end();++it)keys.insert(it.key());
 for(const auto& key:keys){path.push_back(key);auto old=base.find(key),next=edited.find(key);if(old==base.end())changes.push_back({path,*next});else if(next==edited.end())changes.push_back({path,{}});else if(*old!=*next){if(old->is_object()&&next->is_object())browserIntegrationDiff(*old,*next,path,changes);else changes.push_back({path,*next});}path.pop_back();}
}
inline std::vector<BrowserIntegrationEdit> browserIntegrationChanges(const BrowserIntegrationDraft& draft){
 if(!draft.base.is_object()||!draft.edited.is_object())throw std::runtime_error("Invalid browser preferences draft.");
 for(auto it=draft.edited.begin();it!=draft.edited.end();++it)if(!browserIntegrationDefaults().contains(it.key())&&(!draft.base.contains(it.key())||draft.base[it.key()]!=*it))throw std::runtime_error("Browser integration cannot change other preferences.");
 for(auto it=draft.base.begin();it!=draft.base.end();++it)if(!browserIntegrationDefaults().contains(it.key())&&!draft.edited.contains(it.key()))throw std::runtime_error("Browser integration cannot remove other preferences.");
 for(auto it=browserIntegrationDefaults().begin();it!=browserIntegrationDefaults().end();++it)if(it->is_string()&&draft.edited.contains(it.key())&&!draft.edited[it.key()].is_string())throw std::runtime_error("Invalid browser text preference.");
 if(draft.edited.contains("VideoPanelReset")){const auto& value=draft.edited["VideoPanelReset"];if(!value.is_number_integer()||value.get<double>()<0||value.get<double>()>9007199254740991.0)throw std::runtime_error("Invalid video panel reset time.");}
 for(const char* key:{"BrowserCaptureEnabled","CaptureForceClick","CaptureForceSkipWeb","CaptureWebPlayers","VideoPanelShowProtected","SelectedLinksCompact","VideoPanelCompact","VideoPanelEnabled","VideoPanelHover"})if(draft.edited.contains(key)&&!draft.edited[key].is_boolean())throw std::runtime_error("Invalid browser checkbox choice.");
 if(draft.edited.contains("VideoPanelMenuWidth")&&!draft.edited["VideoPanelMenuWidth"].is_number_integer())throw std::runtime_error("Invalid video panel menu width.");
 validateBrowserSettings(draft.edited);Json base=Json::object(),edited=Json::object();for(auto it=browserIntegrationDefaults().begin();it!=browserIntegrationDefaults().end();++it){if(draft.base.contains(it.key()))base[it.key()]=draft.base[it.key()];if(draft.edited.contains(it.key()))edited[it.key()]=draft.edited[it.key()];}std::vector<BrowserIntegrationEdit> changes;std::vector<std::string> path;browserIntegrationDiff(base,edited,path,changes);return changes;
}
inline void mergeBrowserIntegration(Json& settings,const std::vector<BrowserIntegrationEdit>& changes){
 for(const auto& change:changes){if(change.path.empty()||!browserIntegrationDefaults().contains(change.path.front()))throw std::runtime_error("Invalid browser preference field.");auto* node=&settings;bool missing=false;for(size_t i=0;i+1<change.path.size();++i){auto at=node->find(change.path[i]);if(at==node->end()){if(!change.value){missing=true;break;}(*node)[change.path[i]]=Json::object();at=node->find(change.path[i]);}if(!at->is_object())throw std::runtime_error("Browser preferences changed. Reopen this dialog and try again.");node=&*at;}if(missing)continue;if(change.value)(*node)[change.path.back()]=*change.value;else node->erase(change.path.back());}
 validateBrowserSettings(settings);
}
struct BrowserIntegrationResult {Json settings;bool saved=false,unchanged=false,cancelled=false;std::string error;};
class BrowserIntegrationTask {
 std::future<BrowserIntegrationResult> future;std::thread worker;Manager* owner=nullptr;std::atomic_bool cancelled=false;std::mutex mutex;std::condition_variable wake;
 void release()noexcept{if(owner){owner->fileInfoDecisionPending=false;owner=nullptr;}}
public:
 BrowserIntegrationTask()=default;BrowserIntegrationTask(const BrowserIntegrationTask&)=delete;BrowserIntegrationTask& operator=(const BrowserIntegrationTask&)=delete;~BrowserIntegrationTask(){finish();}
 bool valid()const noexcept{return future.valid();}bool ready()const{return valid()&&future.wait_for(std::chrono::seconds(0))==std::future_status::ready;}bool saving()const noexcept{return owner!=nullptr;}
 void start(Manager& manager,std::optional<BrowserIntegrationDraft> draft={},std::function<void()> notify={},std::function<void(const char*)> observe={}){
  if(valid()||worker.joinable())throw std::runtime_error("Browser preferences are already pending.");if(draft){bool expected=false;if(!manager.fileInfoDecisionPending.compare_exchange_strong(expected,true))throw std::runtime_error("Another change is being saved. Try again shortly.");owner=&manager;}cancelled=false;
  try{std::promise<BrowserIntegrationResult> promise;future=promise.get_future();worker=std::thread([this,&manager,draft=std::move(draft),notify=std::move(notify),observe=std::move(observe),promise=std::move(promise)]()mutable{
   BrowserIntegrationResult result;try{
    if(draft){const auto changes=browserIntegrationChanges(*draft);if(changes.empty()){result.unchanged=true;}else{if(observe)observe("waiting-save");{Lock lock(manager.mutex);if(observe)observe("before-save");auto next=manager.state["Settings"];mergeBrowserIntegration(next,changes);result.settings=next;if(num(next,"LimitKbps")>0)result.settings["GlobalLimitRememberedKbps"]=num(next,"LimitKbps");manager.setSettings(next);result.saved=true;}if(observe)try{observe("captured");}catch(...){}}}
    else for(;;){if(cancelled){result.cancelled=true;break;}std::unique_lock lock(manager.mutex,std::try_to_lock);if(lock.owns_lock()&&!manager.fileInfoDecisionPending){if(cancelled){result.cancelled=true;break;}if(observe)observe("before-read");result.settings=browserIntegrationSnapshot(manager.state["Settings"]);lock.unlock();if(observe)try{observe("captured");}catch(...){}break;}if(lock.owns_lock())lock.unlock();if(observe)observe("waiting-read");std::unique_lock wait(mutex);wake.wait_for(wait,std::chrono::milliseconds(10),[&]{return cancelled.load();});}
   }catch(const std::exception& error){result.error=error.what();}catch(...){result.error="Browser preferences could not be processed.";}if(!draft&&cancelled)result.cancelled=true;promise.set_value(std::move(result));if(notify)try{notify();}catch(...){}
  });}catch(...){future={};release();throw;}
 }
 void cancelRead()noexcept{cancelled=true;wake.notify_all();}
 BrowserIntegrationResult take(){if(!ready())throw std::runtime_error("Browser preferences are not ready.");auto result=future.get();if(worker.joinable())worker.join();release();return result;}
 BrowserIntegrationResult finish()noexcept{cancelRead();if(worker.joinable())worker.join();BrowserIntegrationResult result;if(future.valid())try{result=future.get();}catch(...){}future={};release();return result;}
};
}
