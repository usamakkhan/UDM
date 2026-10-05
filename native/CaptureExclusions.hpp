#pragma once
#include "BrowserSettings.hpp"
namespace udm {
inline std::string captureExceptionText(const std::vector<std::string>& rows){std::string text;for(const auto& row:rows){if(!text.empty())text+='\n';text+=row;}return text;}
inline Json captureExceptionSettings(const Json& prefs,const Json& offer,bool site,bool address,bool suppress){
 auto next=prefs;auto canonical=exactCaptureAddress(str(offer,"address"));Url url(canonical);
 if(site){auto hosts=words(str(next,"CaptureExcludedHosts"));if(std::find(hosts.begin(),hosts.end(),url.host)==hosts.end())hosts.push_back(url.host);std::string joined;for(const auto& host:hosts){if(!joined.empty())joined+=' ';joined+=host;}next["CaptureExcludedHosts"]=joined;}
 if(address){auto rows=addressExceptions(str(next,"CaptureExcludedUrls"));auto exact="="+canonical;if(std::find(rows.begin(),rows.end(),exact)==rows.end())rows.push_back(exact);next["CaptureExcludedUrls"]=captureExceptionText(rows);}
 if(suppress)next["OfferCaptureExclusions"]=false;
 validateBrowserSettings(next);return next;
}
}
