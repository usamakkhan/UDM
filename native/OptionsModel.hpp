#pragma once
#include "Core.hpp"
#include <algorithm>
#include <set>
#include <regex>
#include <string_view>
namespace udm {
inline void collectQuotaDraft(Json& prefs,bool enabled,const std::string& amount,const std::string& hours){
 if(!enabled){if(num(prefs,"QuotaMb")>0)prefs["QuotaLastMb"]=num(prefs,"QuotaMb");prefs["QuotaMb"]=0;return;}
 auto whole=[](const std::string& input,i64 maximum,const char* message){auto text=trim(input);i64 value=0;if(text.empty())throw std::runtime_error(message);for(unsigned char c:text){if(c<'0'||c>'9'||value>(maximum-(c-'0'))/10)throw std::runtime_error(message);value=value*10+(c-'0');if(value>maximum)throw std::runtime_error(message);}if(value<1)throw std::runtime_error(message);return value;};
 const auto size=whole(amount,100000000,"Enter a whole download limit from 1 to 100000000 MBytes.");
 const auto period=whole(hours,168,"Enter a whole quota period from 1 to 168 hours.");
 prefs["QuotaLastMb"]=size;prefs["QuotaMb"]=size;prefs["QuotaHours"]=period;
}
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
inline std::string browserOfferStatus(const Json& prefs,bool explicitlyLater=false){return explicitlyLater?"Paused":!yes(prefs,"SkipBrowserFileInfo")?"Awaiting confirmation":yes(prefs,"BrowserDownloadLater")?"Paused":"Queued";}
inline std::vector<std::string> optionCategories(const Json& prefs){
 std::vector<std::string> values;auto hidden=prefs.value("HiddenBuiltinCategories",Json::array());
 for(const auto& name:{"Archives","Documents","Music","Programs","Video","Images","Other"}){
  if(std::string_view(name)=="Images"&&yes(prefs,"IdmCategoryDefaults"))continue;
  if(std::find(hidden.begin(),hidden.end(),Json(name))==hidden.end())values.push_back(name);
 }
 for(const auto& item:prefs.value("CustomCategories",Json::array()))if(item.is_string())values.push_back(item.get<std::string>());return values;
}
inline bool activeCategory(const Json& prefs,const std::string& name){auto names=optionCategories(prefs);return std::find(names.begin(),names.end(),name)!=names.end();}
inline const std::map<std::string,std::string>& builtinCategoryExtensions(bool idmDefaults=false){
 static const std::map<std::string,std::string> legacy={{"Archives","zip 7z rar gz tar iso"},{"Video","mp4 mkv webm mov avi ts"},{"Music","mp3 flac wav ogg m4a aac"},{"Programs","exe msi msix apk"},{"Images","jpg jpeg png svg webp gif"},{"Documents","pdf doc docx xlsx txt epub pptx csv"}};
 static const std::map<std::string,std::string> reference={
  {"Archives","zip rar r0* r1* arj gz sit sitx sea ace bz2 7z"},
  {"Documents","doc pdf ppt pps docx pptx"},
  {"Music","mp3 wav wma mpa ram ra aac aif m4a tsa"},
  {"Programs","exe msi"},
  {"Video","avi mpg mpe mpeg asf wmv mov qt rm mp4 flv m4v webm ogv ogg mkv ts tsv"}
 };
 return idmDefaults?reference:legacy;
}
inline bool validCategoryExtension(const std::string& token){
 if(token=="*")return true;
 static const std::regex pattern("[a-z0-9_-]{1,30}\\*?");
 return std::regex_match(token,pattern);
}
inline bool matchesCategoryExtension(const std::string& extension,const std::vector<std::string>& tokens){
 return std::any_of(tokens.begin(),tokens.end(),[&](const std::string& token){
  if(token=="*"||token==extension)return true;
  return token.size()>1&&token.back()=='*'&&extension.compare(0,token.size()-1,token,0,token.size()-1)==0;
 });
}
inline bool siteRestrictedCategory(const Json& prefs,const std::string& name){
 bool restricted=false;for(const auto& rule:prefs.value("CategoryRules",Json::array()))if(str(rule,"Category")==name){if(trim(str(rule,"Hosts")).empty())return false;restricted=true;}return restricted;
}
inline std::string categoryExtensions(const Json& prefs,const std::string& name){
 if(siteRestrictedCategory(prefs,name))for(const auto& rule:prefs.value("CategoryRules",Json::array()))if(str(rule,"Category")==name)return str(rule,"Extensions");
 auto overrides=prefs.value("CategoryTypeOverrides",Json::object());if(overrides.contains(name))return str(overrides,name.c_str());
 for(const auto& row:prefs.value("CategoryRules",Json::array()))if(str(row,"Category")==name&&str(row,"Hosts").empty())return str(row,"Extensions");
 const auto& defaults=builtinCategoryExtensions(yes(prefs,"IdmCategoryDefaults"));
 auto found=defaults.find(name);return found==defaults.end()?"":found->second;
}
inline std::string categoryForPreferences(const std::string& filename,const Json& prefs){
 auto ext=lower(utf8(fs::path(wide(filename)).extension().wstring()));if(!ext.empty())ext.erase(0,1);
 auto overrides=prefs.value("CategoryTypeOverrides",Json::object());
 auto enabled=[&](const std::string& name){return activeCategory(prefs,name)&&!siteRestrictedCategory(prefs,name);};
 for(auto it=overrides.begin();it!=overrides.end();++it){
  if(!enabled(it.key()))continue;
  const auto types=words(it->get<std::string>());
  if(std::find(types.begin(),types.end(),"*")==types.end()&&matchesCategoryExtension(ext,types))return it.key();
 }
 for(const auto& row:builtinCategoryExtensions(yes(prefs,"IdmCategoryDefaults")))
  if(!overrides.contains(row.first)&&enabled(row.first)&&matchesCategoryExtension(ext,words(row.second)))return row.first;
 for(auto it=overrides.begin();it!=overrides.end();++it)
  if(enabled(it.key())&&matchesCategoryExtension(ext,words(it->get<std::string>())))return it.key();
 return "Other";
}
inline std::string downloadCategory(const std::string& filename,const std::string& hostName,const Json& prefs){
 auto cat=categoryForPreferences(filename,prefs);auto ext=lower(utf8(fs::path(wide(filename)).extension().wstring()));if(!ext.empty())ext.erase(0,1);
 for(const auto& r:prefs.value("CategoryRules",Json::array())){auto ex=words(str(r,"Extensions")),hs=words(str(r,"Hosts"));bool host=hs.empty();for(auto v:hs){while(!v.empty()&&(v[0]=='*'||v[0]=='.'))v.erase(0,1);host|=hostIs(hostName,v);}if(activeCategory(prefs,str(r,"Category"))&&host&&matchesCategoryExtension(ext,ex)){cat=str(r,"Category",cat);break;}}return cat;
}
inline std::string categoryFolder(const Json& prefs,const std::string& category){
 auto folder=str(dictionary(prefs.value("CategoryPaths",Json::array())),category.c_str());if(!folder.empty())return folder;
 auto path=fs::path(wide(str(prefs,"DownloadFolder")));
 const bool idmDefaults=yes(prefs,"IdmCategoryDefaults");
 if(yes(prefs,"CategoryFolders",true)&&(!idmDefaults||category!="Other"))
  path/=wide(idmDefaults&&category=="Archives"?"Compressed":category);
 return utf8(path.wstring());
}
inline void rememberCategoryDestination(Json& prefs,const std::string& category,const fs::path& folder){
 if(!yes(prefs.value("CategoryRememberLast",Json::object()),category.c_str()))return;
 if(!folder.is_absolute())throw std::runtime_error("Choose an absolute category folder.");
 auto paths=dictionary(prefs.value("CategoryPaths",Json::array()));paths[category]=utf8(folder.wstring());prefs["CategoryPaths"]=legacyDictionary(paths);
}
inline void rememberFileInfoDestination(Json& prefs,const std::string& category,const fs::path& folder,const FileInfoFolderChoice& choice){
 prefs["CategoryRememberLast"][category]=choice.rememberedCategoryFolder.has_value();
 if(choice.rememberedCategoryFolder)rememberCategoryDestination(prefs,category,*choice.rememberedCategoryFolder);
 const auto selected=utf8(folder.wstring()),key=lower(selected);
 auto recent=Json::array({selected});
 const auto history=prefs.find("SaveFolderHistory");
 if(history!=prefs.end()&&history->is_array())for(const auto& entry:*history){
  if(recent.size()==20)break;
  if(entry.is_string()&&lower(entry.get<std::string>())!=key)recent.push_back(entry);
 }
 prefs["SaveFolderHistory"]=std::move(recent);
}
inline void validateOptionsModel(const Json& prefs){
 if(prefs.contains("IdmCategoryDefaults")&&!prefs["IdmCategoryDefaults"].is_boolean())throw std::runtime_error("Invalid category defaults.");
 auto hidden=prefs.value("HiddenBuiltinCategories",Json::array());if(!hidden.is_array()||hidden.size()>6)throw std::runtime_error("Invalid hidden category list.");std::set<std::string> hiddenNames;
 for(const auto& item:hidden)if(!item.is_string()||!builtinCategoryExtensions().count(item.get<std::string>())||!hiddenNames.insert(item.get<std::string>()).second)throw std::runtime_error("Choose a predefined category to hide.");
 for(const char* key:{"BrowserDownloadLater","QueuePromptLater","QueuePromptBatch","UseTls13","UseHttp3","IgnoreLastModified"})if(prefs.contains(key)&&!prefs[key].is_boolean())throw std::runtime_error("Invalid download dialog preference.");
 auto remember=prefs.value("CategoryRememberLast",Json::object());if(!remember.is_object()||remember.size()>107)throw std::runtime_error("Invalid category folder memory.");
 const auto categories=optionCategories(prefs);for(auto it=remember.begin();it!=remember.end();++it)if(!it->is_boolean()||std::find(categories.begin(),categories.end(),it.key())==categories.end())throw std::runtime_error("Choose an existing category for folder memory.");
 auto types=prefs.value("CategoryTypeOverrides",Json::object());if(!types.is_object()||types.size()>107)throw std::runtime_error("Invalid category file types.");
 for(auto it=types.begin();it!=types.end();++it){if(!it->is_string()||std::find(categories.begin(),categories.end(),it.key())==categories.end())throw std::runtime_error("Choose an existing category for file types.");for(const auto& type:words(it->get<std::string>()))if(!validCategoryExtension(type))throw std::runtime_error("Use category extensions or prefixes such as r0*.");}
}
}
