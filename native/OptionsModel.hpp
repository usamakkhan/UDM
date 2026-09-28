#pragma once
#include "Core.hpp"
#include <algorithm>
#include <set>
#include <regex>
namespace udm {
inline Json mergeOptionsDraft(const Json& original,const Json& draft,const Json& current){
 if(!original.is_object()||!draft.is_object()||!current.is_object())throw std::runtime_error("Invalid settings draft.");
 Json result=current;std::set<std::string> keys;for(auto it=original.begin();it!=original.end();++it)keys.insert(it.key());for(auto it=draft.begin();it!=draft.end();++it)keys.insert(it.key());
 for(const auto& key:keys){
  auto old=original.find(key),next=draft.find(key);if(old==original.end()){auto live=current.find(key);if(next->is_object()&&live!=current.end()&&live->is_object())result[key]=mergeOptionsDraft(Json::object(),*next,*live);else result[key]=*next;continue;}
  if(next==draft.end()){result.erase(key);continue;}if(*old==*next)continue;
  auto live=current.find(key);
  if(key=="CategoryPaths"&&live!=current.end())result[key]=legacyDictionary(mergeOptionsDraft(dictionary(*old),dictionary(*next),dictionary(*live)));
  else if(old->is_object()&&next->is_object()&&live!=current.end()&&live->is_object())result[key]=mergeOptionsDraft(*old,*next,*live);else result[key]=*next;
 }return result;
}
inline std::string browserOfferStatus(const Json& prefs,bool explicitlyLater=false){return explicitlyLater||yes(prefs,"BrowserDownloadLater")?"Paused":yes(prefs,"SkipBrowserFileInfo")?"Queued":"Awaiting confirmation";}
inline std::vector<std::string> optionCategories(const Json& prefs){std::vector<std::string> values={"Archives","Documents","Music","Programs","Video","Images","Other"};for(const auto& item:prefs.value("CustomCategories",Json::array()))if(item.is_string())values.push_back(item.get<std::string>());return values;}
inline const std::map<std::string,std::string>& builtinCategoryExtensions(){
 static const std::map<std::string,std::string> types={{"Archives","zip 7z rar gz tar iso"},{"Video","mp4 mkv webm mov avi ts"},{"Music","mp3 flac wav ogg m4a aac"},{"Programs","exe msi msix apk"},{"Images","jpg jpeg png svg webp gif"},{"Documents","pdf doc docx xlsx txt epub pptx csv"}};return types;
}
inline std::string categoryExtensions(const Json& prefs,const std::string& name){
 auto overrides=prefs.value("CategoryTypeOverrides",Json::object());if(overrides.contains(name))return str(overrides,name.c_str());
 for(const auto& row:prefs.value("CategoryRules",Json::array()))if(str(row,"Category")==name&&str(row,"Hosts").empty())return str(row,"Extensions");
 auto found=builtinCategoryExtensions().find(name);return found==builtinCategoryExtensions().end()?"":found->second;
}
inline std::string categoryForPreferences(const std::string& filename,const Json& prefs){
 auto ext=lower(utf8(fs::path(wide(filename)).extension().wstring()));if(!ext.empty())ext.erase(0,1);
 auto matches=[&](const std::string& text){const auto types=words(text);return std::find(types.begin(),types.end(),ext)!=types.end()||std::find(types.begin(),types.end(),"*")!=types.end();};
 auto overrides=prefs.value("CategoryTypeOverrides",Json::object());for(auto it=overrides.begin();it!=overrides.end();++it)if(it->get<std::string>().find('*')==std::string::npos&&matches(it->get<std::string>()))return it.key();
 for(auto& row:builtinCategoryExtensions())if(!overrides.contains(row.first)&&matches(row.second))return row.first;for(auto it=overrides.begin();it!=overrides.end();++it)if(matches(it->get<std::string>()))return it.key();return "Other";
}
inline std::string downloadCategory(const std::string& filename,const std::string& hostName,const Json& prefs){
 auto cat=categoryForPreferences(filename,prefs);auto ext=lower(utf8(fs::path(wide(filename)).extension().wstring()));if(!ext.empty())ext.erase(0,1);
 for(const auto& r:prefs.value("CategoryRules",Json::array())){auto ex=words(str(r,"Extensions")),hs=words(str(r,"Hosts"));bool host=hs.empty();for(auto v:hs){while(!v.empty()&&(v[0]=='*'||v[0]=='.'))v.erase(0,1);host|=hostIs(hostName,v);}if(host&&(std::find(ex.begin(),ex.end(),ext)!=ex.end()||std::find(ex.begin(),ex.end(),"*")!=ex.end())){cat=str(r,"Category",cat);break;}}return cat;
}
inline std::string categoryFolder(const Json& prefs,const std::string& category){
 auto folder=str(dictionary(prefs.value("CategoryPaths",Json::array())),category.c_str());if(!folder.empty())return folder;
 auto path=fs::path(wide(str(prefs,"DownloadFolder")));if(yes(prefs,"CategoryFolders",true))path/=wide(category);return utf8(path.wstring());
}
inline void rememberCategoryDestination(Json& prefs,const std::string& category,const fs::path& folder){
 if(!yes(prefs.value("CategoryRememberLast",Json::object()),category.c_str()))return;
 if(!folder.is_absolute())throw std::runtime_error("Choose an absolute category folder.");
 auto paths=dictionary(prefs.value("CategoryPaths",Json::array()));paths[category]=utf8(folder.wstring());prefs["CategoryPaths"]=legacyDictionary(paths);
}
inline void validateOptionsModel(const Json& prefs){
 for(const char* key:{"BrowserDownloadLater","QueuePromptLater","QueuePromptBatch","UseTls13","IgnoreLastModified"})if(prefs.contains(key)&&!prefs[key].is_boolean())throw std::runtime_error("Invalid download dialog preference.");
 auto remember=prefs.value("CategoryRememberLast",Json::object());if(!remember.is_object()||remember.size()>107)throw std::runtime_error("Invalid category folder memory.");
 const auto categories=optionCategories(prefs);for(auto it=remember.begin();it!=remember.end();++it)if(!it->is_boolean()||std::find(categories.begin(),categories.end(),it.key())==categories.end())throw std::runtime_error("Choose an existing category for folder memory.");
 auto types=prefs.value("CategoryTypeOverrides",Json::object());if(!types.is_object()||types.size()>107)throw std::runtime_error("Invalid category file types.");
 for(auto it=types.begin();it!=types.end();++it){if(!it->is_string()||std::find(categories.begin(),categories.end(),it.key())==categories.end())throw std::runtime_error("Choose an existing category for file types.");for(auto type:words(it->get<std::string>()))if(type!="*"&&!std::regex_match(type,std::regex("[a-z0-9_-]{1,30}")))throw std::runtime_error("Use plain category file extensions.");}
}
}
