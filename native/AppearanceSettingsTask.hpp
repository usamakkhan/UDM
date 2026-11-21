#pragma once
#include "ToolbarModel.hpp"
#include <future>
#define UDM_APPEARANCE_COMMAND_AVAILABLE
namespace udm {
enum class AppearanceChange {ToggleDark,ToggleCategories,Font,ResetFont,Tray,ToolbarSize,ToolbarStyle,ToolbarLook,ToggleToolbar,ToolbarSkin,ToolbarLayout,ToolbarReload};
struct AppearanceDraft {AppearanceChange change=AppearanceChange::ToggleDark;std::string face;int height=11,weight=400;std::string tray;std::string toolbarValue;std::vector<int> toolbarButtons;};
struct AppearanceResult {Json settings;AppearanceChange change=AppearanceChange::ToggleDark;bool saved=false,refreshed=false;std::string error;std::shared_ptr<const ToolbarSkin> toolbarSkin;std::string toolbarSkinError;std::optional<std::vector<int>> toolbarRollback;};
inline void validateAppearanceDraft(const AppearanceDraft& draft){
 switch(draft.change){
  case AppearanceChange::ToggleDark:case AppearanceChange::ToggleCategories:case AppearanceChange::ResetFont:case AppearanceChange::ToggleToolbar:case AppearanceChange::ToolbarReload:return;
  case AppearanceChange::Font:if(!draft.face.empty()&&draft.face.size()<=31&&draft.face.find_first_of("\r\n")==std::string::npos&&draft.height>=8&&draft.height<=24&&draft.weight>=0&&draft.weight<=1000)return;break;
  case AppearanceChange::ToolbarSize:validateToolbarSettings(Json{{"ToolbarSize",draft.toolbarValue}});return;
  case AppearanceChange::ToolbarStyle:validateToolbarSettings(Json{{"ToolbarStyle",draft.toolbarValue}});return;
  case AppearanceChange::ToolbarLook:if(draft.toolbarValue=="Neon"||draft.toolbarValue=="3D Style")return;break;
  case AppearanceChange::ToolbarSkin:validateToolbarSettings(Json{{"ToolbarSkin",draft.toolbarValue}});return;
  case AppearanceChange::ToolbarLayout:if(validToolbarLayout(Json(draft.toolbarButtons)))return;break;
  case AppearanceChange::Tray:if(draft.tray=="Color"||draft.tray=="Classic"||draft.tray=="Hidden")return;break;
 }
 throw std::runtime_error("Invalid appearance choice.");
}
// Accepted work is joined, not cancelled. It captures values, never Windows controls.
class AppearanceSettingsTask {
 std::future<AppearanceResult> future;std::thread worker;Manager* owner=nullptr;
 void release()noexcept{if(owner){owner->fileInfoDecisionPending=false;owner=nullptr;}}
public:
 AppearanceSettingsTask()=default;AppearanceSettingsTask(const AppearanceSettingsTask&)=delete;AppearanceSettingsTask& operator=(const AppearanceSettingsTask&)=delete;~AppearanceSettingsTask(){finish();}
 bool valid()const noexcept{return future.valid();}bool ready()const{return valid()&&future.wait_for(std::chrono::seconds(0))==std::future_status::ready;}
 void start(Manager& manager,AppearanceDraft draft,std::function<void()> notify={},std::function<void(const char*)> observe={}){
  if(valid()||worker.joinable())throw std::runtime_error("An appearance change is already pending.");validateAppearanceDraft(draft);
  bool expected=false;if(!manager.fileInfoDecisionPending.compare_exchange_strong(expected,true))throw std::runtime_error("Another change is being saved. Try again shortly.");owner=&manager;
  try{std::promise<AppearanceResult> promise;future=promise.get_future();worker=std::thread([&manager,draft=std::move(draft),notify=std::move(notify),observe=std::move(observe),promise=std::move(promise)]()mutable{
   AppearanceResult result;result.change=draft.change;
   try{if(draft.change==AppearanceChange::ToolbarSkin&&!draft.toolbarValue.empty()){if(observe)observe("before-skin-read");result.toolbarSkin=std::make_shared<const ToolbarSkin>(loadToolbarSkin(fs::path(wide(draft.toolbarValue))));}
    if(observe)observe("waiting-save");{Lock lock(manager.mutex);auto next=manager.state["Settings"];if(draft.change==AppearanceChange::ToolbarLayout)result.toolbarRollback=toolbarLayout(next);if(observe)observe("before-save");
    switch(draft.change){
     case AppearanceChange::ToggleDark:next["DarkMode"]=!yes(next,"DarkMode");break;
     case AppearanceChange::ToggleCategories:next["HideCategories"]=!yes(next,"HideCategories");break;
     case AppearanceChange::Font:next["FontName"]=draft.face;next["FontHeight"]=draft.height;next["FontWeight"]=draft.weight;break;
     case AppearanceChange::ResetFont:{const auto defaults=defaultSettings();for(auto key:{"FontName","FontHeight","FontWeight"})next[key]=defaults[key];break;}
     case AppearanceChange::Tray:next["TrayIcon"]=draft.tray;break;
     case AppearanceChange::ToolbarSize:next["ToolbarSize"]=draft.toolbarValue;break;
     case AppearanceChange::ToolbarStyle:next["ToolbarStyle"]=draft.toolbarValue;break;
     case AppearanceChange::ToolbarLook:next["ToolbarLook"]=draft.toolbarValue;next["ToolbarSkin"]="";break;
     case AppearanceChange::ToggleToolbar:next["HideToolbar"]=!yes(next,"HideToolbar");break;
     case AppearanceChange::ToolbarSkin:next["ToolbarSkin"]=draft.toolbarValue;break;
     case AppearanceChange::ToolbarLayout:saveToolbarLayout(next,draft.toolbarButtons);break;
     case AppearanceChange::ToolbarReload:break;
    }
    result.settings=next;if(draft.change!=AppearanceChange::ToolbarReload&&num(next,"LimitKbps")>0)result.settings["GlobalLimitRememberedKbps"]=num(next,"LimitKbps");if(draft.change==AppearanceChange::ToolbarReload)result.refreshed=true;else{manager.setSettings(next);result.saved=true;}
   }
    if(draft.change!=AppearanceChange::Tray&&!str(result.settings,"ToolbarSkin").empty()&&!result.toolbarSkin)try{if(observe)observe("before-skin-read");result.toolbarSkin=std::make_shared<const ToolbarSkin>(loadToolbarSkin(fs::path(wide(str(result.settings,"ToolbarSkin")))));}catch(const std::exception& error){result.toolbarSkinError=error.what();}
    if(observe)try{observe("captured");}catch(...){}}
   catch(const std::exception& error){result.error=error.what();}catch(...){result.error="Appearance settings could not be saved.";}
   promise.set_value(std::move(result));if(notify)try{notify();}catch(...){}
  });}catch(...){future={};release();throw;}
 }
 AppearanceResult take(){if(!ready())throw std::runtime_error("Appearance settings are not ready.");auto result=future.get();if(worker.joinable())worker.join();release();return result;}
 AppearanceResult finish()noexcept{if(worker.joinable())worker.join();AppearanceResult result;if(future.valid())try{result=future.get();}catch(...){}future={};release();return result;}
};
}
