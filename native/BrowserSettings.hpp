#pragma once
#include "Core.hpp"
#include <sstream>
#include <regex>
namespace udm {
inline std::set<std::string> browserKeyParts(const std::string& value,bool force){
 if(value=="None")return {};if(value.empty()||value.size()>40)throw std::runtime_error("Choose a key combination.");
 std::set<std::string> parts;std::istringstream input(value);std::string part;
 while(std::getline(input,part,'+'))if((part!="Alt"&&part!="Ctrl"&&part!="Shift"&&part!=(force?"Ins":"Del"))||!parts.insert(part).second)throw std::runtime_error("Choose Alt, Shift, Ctrl and "+std::string(force?"Ins":"Del")+" without repeated keys.");
 if(value.back()=='+')throw std::runtime_error("Invalid key combination.");return parts;
}
inline Json defaultVideoPanelTypes(){Json types=Json::object();for(const char* type:{"aac","avi","flv","m4a","m4v","mkv","mov","mp3","mp4","mpeg","mpg","oga","ogg","ogv","ts","wav","webm","wma","wmv"})types[type]=true;return types;}
inline void validatePanelHosts(const std::string& value){
 if(value.size()>8192)throw std::runtime_error("Use at most 8 KB of panel exceptions.");
 auto hosts=words(value);if(hosts.size()>100)throw std::runtime_error("Use at most 100 panel exceptions.");
 for(const auto& host:hosts)if(!std::regex_match(host,std::regex("[a-z0-9*]([a-z0-9.*-]{0,251}[a-z0-9*])?")))throw std::runtime_error("Enter host names or wildcard patterns, without a scheme or path.");
}
inline Json videoPanelTypes(const Json& p){return p.value("VideoPanelTypes",defaultVideoPanelTypes());}
inline Json browserContextMenu(const Json& p,const std::string& family){auto all=p.value("BrowserContextMenus",Json::object());return all.value(family,Json{{"Link",true},{"All",true}});}
inline std::vector<std::string> addressExceptions(const std::string& text){
 if(text.size()>32768)throw std::runtime_error("Use at most 32 KB of address exceptions.");
 std::vector<std::string> result;std::istringstream input(text);std::string line;
 while(std::getline(input,line)){line=trim(line);if(line.empty())continue;
  if(line.size()>2048||!std::regex_match(line,std::regex(R"(https?://(\*\.)?[A-Za-z0-9.-]+(:[0-9]{1,5})?/[^\s#\\]*)")))throw std::runtime_error("Use one HTTP/HTTPS address pattern per line, such as https://example.com/private/*. A leading *. matches subdomains.");
  result.push_back(line);if(result.size()>100)throw std::runtime_error("Use at most 100 address exceptions.");
 }return result;
}
inline bool browserCaptureAllowed(const Json& p,const std::string& executable){
 auto rows=p.value("BrowserCaptureTargets",Json::object());if(!rows.is_object())return true;
 auto row=rows.find(lower(executable));return row==rows.end()||!row->is_object()||yes(*row,"Enabled",true);
}
inline void validateBrowserSettings(const Json& p){
 auto targets=p.value("BrowserCaptureTargets",Json::object());if(!targets.is_object()||targets.size()>64)throw std::runtime_error("Use at most 64 browser entries.");
 for(auto it=targets.begin();it!=targets.end();++it)if(!std::regex_match(it.key(),std::regex("[a-z0-9_. -]{1,120}\\.exe"))||!it->is_object()||!it->contains("Enabled")||!it->at("Enabled").is_boolean()||str(*it,"Name").empty()||str(*it,"Name").size()>120)throw std::runtime_error("Choose a browser executable and a short display name.");
 addressExceptions(str(p,"CaptureExcludedUrls"));
 auto force=browserKeyParts(str(p,"CaptureForceKey","Ctrl"),true),bypass=browserKeyParts(str(p,"CaptureBypassKey","Alt"),false);
 if(!force.empty()&&force==bypass)throw std::runtime_error("Choose different force and bypass keys.");
 for(const char* key:{"CaptureForceClick","CaptureForceSkipWeb","CaptureWebPlayers","VideoPanelShowProtected"})if(p.contains(key)&&!p[key].is_boolean())throw std::runtime_error("Invalid browser checkbox setting.");
 auto types=videoPanelTypes(p);if(!types.is_object()||types.size()>64)throw std::runtime_error("Use at most 64 video panel file types.");
 for(auto it=types.begin();it!=types.end();++it)if(!std::regex_match(it.key(),std::regex("[a-z0-9]{1,16}"))||!it->is_boolean())throw std::runtime_error("Enter plain file extensions for video panels.");
 auto sizes=p.value("VideoPanelMinKb",Json::object());if(!sizes.is_object()||sizes.size()>64)throw std::runtime_error("Invalid panel minimum sizes.");
 for(auto it=sizes.begin();it!=sizes.end();++it)if(!types.contains(it.key())||!it->is_number_integer()||it->get<i64>()<0||it->get<i64>()>1073741824)throw std::runtime_error("Enter a panel minimum size from 0 to 1073741824 KB.");
 validatePanelHosts(str(p,"VideoPanelExcludedHosts"));
 auto menus=p.value("BrowserContextMenus",Json::object());if(!menus.is_object()||menus.size()>2)throw std::runtime_error("Invalid browser menu settings.");
 for(auto it=menus.begin();it!=menus.end();++it){if((it.key()!="chromium"&&it.key()!="firefox")||!it->is_object())throw std::runtime_error("Choose a supported browser menu group.");for(auto field=it->begin();field!=it->end();++field)if((field.key()!="Link"&&field.key()!="All")||!field->is_boolean())throw std::runtime_error("Invalid browser menu entry.");}
 const std::set<std::string> positions={"Top right","Top left","Bottom right","Bottom left"};
 if(!positions.count(str(p,"VideoPanelPosition","Top right"))||num(p,"VideoPanelMenuWidth",420)<260||num(p,"VideoPanelMenuWidth",420)>640)throw std::runtime_error("Choose a panel corner and a menu width from 260 to 640 pixels.");
 auto selected=str(p,"SelectedLinksMode","all");if(selected!="all"&&selected!="sites"&&selected!="off")throw std::runtime_error("Choose a valid selected-link panel mode.");
 for(auto h:words(str(p,"CaptureExcludedHosts")+" "+str(p,"SelectedLinkHosts"))){if(Url("https://"+h+"/").host!=h||h.find_first_of("/:@*")!=std::string::npos)throw std::runtime_error("Use plain host names, without a scheme or path.");}
}
inline Json browserPreferences(const Json& p,const std::string& browser=""){
 const bool enabled=browserCaptureAllowed(p,browser);
 return {{"ok",true},{"browserSession",1},{"captureRecovery",1},{"browserProxy",1},{"adaptiveResources",1},{"postDownloads",true},{"postBodyLimit",MaxBrowserPostBytes},{"extensions",words(str(p,"CaptureExtensions"))},{"excluded",words(str(p,"CaptureExcludedHosts"))},
 {"excludedUrls",addressExceptions(str(p,"CaptureExcludedUrls"))},{"captureAllowed",enabled&&yes(p,"BrowserCaptureEnabled",true)},
 {"panelEnabled",enabled&&yes(p,"VideoPanelEnabled",true)},{"panelCompact",yes(p,"VideoPanelCompact")},{"panelHover",yes(p,"VideoPanelHover")},
 {"panelPosition",str(p,"VideoPanelPosition","Top right")},{"panelMenuWidth",num(p,"VideoPanelMenuWidth",420)},
 {"forceKey",str(p,"CaptureForceKey","Ctrl")},{"bypassKey",str(p,"CaptureBypassKey","Alt")},
 {"forceClick",yes(p,"CaptureForceClick",true)},{"forceSkipWeb",yes(p,"CaptureForceSkipWeb",true)},{"captureWebPlayers",yes(p,"CaptureWebPlayers")},
 {"panelTypes",videoPanelTypes(p)},{"panelMinKb",p.value("VideoPanelMinKb",Json::object())},{"panelExcluded",words(str(p,"VideoPanelExcludedHosts"))},{"panelShowProtected",yes(p,"VideoPanelShowProtected",true)},
 {"contextMenu",browserContextMenu(p,lower(browser)=="firefox.exe"?"firefox":"chromium")},
 {"selectedLinks",str(p,"SelectedLinksMode","all")},{"selectedLinkHosts",words(str(p,"SelectedLinkHosts"))},
 {"selectedLinksMini",yes(p,"SelectedLinksCompact")},{"panelReset",num(p,"VideoPanelReset")}};
}
}
