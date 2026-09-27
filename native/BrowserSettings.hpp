#pragma once
#include "Core.hpp"
#include <sstream>
#include <regex>
namespace udm {
inline std::vector<std::string> addressExceptions(const std::string& text){
 if(text.size()>32768)throw std::runtime_error("Use at most 32 KB of address exceptions.");
 std::vector<std::string> result;std::istringstream input(text);std::string line;
 while(std::getline(input,line)){line=trim(line);if(line.empty())continue;
  if(line.size()>2048||!std::regex_match(line,std::regex(R"(https?://(\*\.)?[A-Za-z0-9.-]+(:[0-9]{1,5})?/[^\s#\\]*)")))throw std::runtime_error("Use one HTTP/HTTPS address pattern per line, such as https://example.com/private/*. A leading *. matches subdomains.");
  result.push_back(line);if(result.size()>100)throw std::runtime_error("Use at most 100 address exceptions.");
 }return result;
}
inline void validateBrowserSettings(const Json& p){
 addressExceptions(str(p,"CaptureExcludedUrls"));
 const std::set<std::string> keys={"None","Alt","Ctrl","Shift","Ctrl+Shift","Alt+Shift"};
 auto force=str(p,"CaptureForceKey","Alt"),bypass=str(p,"CaptureBypassKey","Ctrl");
 if(!keys.count(force)||!keys.count(bypass)||(force!="None"&&force==bypass))throw std::runtime_error("Choose different force and bypass modifier keys.");
 const std::set<std::string> positions={"Top right","Top left","Bottom right","Bottom left"};
 if(!positions.count(str(p,"VideoPanelPosition","Top right"))||num(p,"VideoPanelMenuWidth",420)<260||num(p,"VideoPanelMenuWidth",420)>640)throw std::runtime_error("Choose a panel corner and a menu width from 260 to 640 pixels.");
 auto selected=str(p,"SelectedLinksMode","all");if(selected!="all"&&selected!="sites"&&selected!="off")throw std::runtime_error("Choose a valid selected-link panel mode.");
 for(auto h:words(str(p,"CaptureExcludedHosts")+" "+str(p,"SelectedLinkHosts"))){if(Url("https://"+h+"/").host!=h||h.find_first_of("/:@*")!=std::string::npos)throw std::runtime_error("Use plain host names, without a scheme or path.");}
}
inline Json browserPreferences(const Json& p){
 return {{"ok",true},{"postDownloads",true},{"extensions",words(str(p,"CaptureExtensions"))},{"excluded",words(str(p,"CaptureExcludedHosts"))},
 {"excludedUrls",addressExceptions(str(p,"CaptureExcludedUrls"))},{"captureAllowed",yes(p,"BrowserCaptureEnabled",true)},
 {"panelEnabled",yes(p,"VideoPanelEnabled",true)},{"panelCompact",yes(p,"VideoPanelCompact")},{"panelHover",yes(p,"VideoPanelHover")},
 {"panelPosition",str(p,"VideoPanelPosition","Top right")},{"panelMenuWidth",num(p,"VideoPanelMenuWidth",420)},
 {"forceKey",str(p,"CaptureForceKey","Alt")},{"bypassKey",str(p,"CaptureBypassKey","Ctrl")},
 {"selectedLinks",str(p,"SelectedLinksMode","all")},{"selectedLinkHosts",words(str(p,"SelectedLinkHosts"))},
 {"selectedLinksMini",yes(p,"SelectedLinksCompact")},{"panelReset",num(p,"VideoPanelReset")}};
}
}
