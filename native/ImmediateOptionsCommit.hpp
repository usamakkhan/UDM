#pragma once
#include "Core.hpp"
namespace udm {
enum class ImmediateOptionsKind {CategoryFolder,CategoryRemember,ServerDate,TemporaryFolder};
struct ImmediateOptionsDraft {ImmediateOptionsKind kind;std::string category,folder;bool enabled=false;};
struct ImmediateOptionsResult {Json settings=Json::object();std::vector<std::string> fields;};
inline ImmediateOptionsResult applyImmediateOptions(Manager& manager,const ImmediateOptionsDraft& draft){
 Lock lock(manager.mutex);auto next=manager.state["Settings"];ImmediateOptionsResult result;
 switch(draft.kind){
  case ImmediateOptionsKind::CategoryFolder:case ImmediateOptionsKind::CategoryRemember:{
   const auto categories=manager.categories();if(std::find(categories.begin(),categories.end(),draft.category)==categories.end())throw std::runtime_error("That category no longer exists. Reopen its properties or choose another category.");
   if(draft.kind==ImmediateOptionsKind::CategoryFolder){if(!fs::path(wide(draft.folder)).is_absolute())throw std::runtime_error("Choose an absolute category folder.");auto paths=dictionary(next.value("CategoryPaths",Json::object()));paths[draft.category]=draft.folder;next["CategoryPaths"]=legacyDictionary(paths);}
   else next["CategoryRememberLast"][draft.category]=draft.enabled;
   result.fields={"CategoryPaths","CategoryRememberLast"};break;
  }
  case ImmediateOptionsKind::ServerDate:next["UseServerDate"]=draft.enabled;result.fields={"UseServerDate"};break;
  case ImmediateOptionsKind::TemporaryFolder:if(!draft.folder.empty()&&!fs::path(wide(draft.folder)).is_absolute())throw std::runtime_error("Choose an absolute temporary-files folder.");next["TemporaryFolder"]=draft.folder;result.fields={"TemporaryFolder"};break;
  default:throw std::runtime_error("Unknown immediate settings command.");
 }
 manager.setSettings(next);const auto& saved=manager.state["Settings"];for(const auto& field:result.fields)if(saved.contains(field))result.settings[field]=saved[field];return result;
}
inline void acknowledgeImmediateOptions(Json& draft,Json& baseline,const ImmediateOptionsResult& result){
 for(const auto& field:result.fields){if(result.settings.contains(field)){draft[field]=result.settings[field];baseline[field]=result.settings[field];}else{draft.erase(field);baseline.erase(field);}}
}
}
