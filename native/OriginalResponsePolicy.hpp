#pragma once
#include "BrowserSettings.hpp"
#include "ServerConnections.hpp"
#include "ContentDispositionName.hpp"
namespace udm {
inline std::string originalResponseProcess(const Json& offer){
 try{
  if(!offer.contains("ProcessId")||!offer["ProcessId"].is_number_unsigned()||!offer.contains("ProcessCreated100ns")||!offer["ProcessCreated100ns"].is_number_unsigned())return {};
  auto id=offer["ProcessId"].get<uint64_t>(),created=offer["ProcessCreated100ns"].get<uint64_t>();if(id<=4||id>MAXDWORD||!created)return {};
  Handle process(OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION|SYNCHRONIZE,FALSE,static_cast<DWORD>(id)));if(!process||WaitForSingleObject(process.h,0)!=WAIT_TIMEOUT)return {};
  FILETIME start{},end{},kernel{},user{};if(!GetProcessTimes(process.h,&start,&end,&kernel,&user)||((uint64_t(start.dwHighDateTime)<<32)|start.dwLowDateTime)!=created)return {};
  DWORD currentSession=0,targetSession=0;if(!ProcessIdToSessionId(GetCurrentProcessId(),&currentSession)||!ProcessIdToSessionId(static_cast<DWORD>(id),&targetSession)||currentSession!=targetSession)return {};
  wchar_t path[32768];DWORD length=32768;if(!QueryFullProcessImageNameW(process.h,0,path,&length)||WaitForSingleObject(process.h,0)!=WAIT_TIMEOUT)return {};
  return lower(utf8(fs::path(std::wstring(path,length)).filename().wstring()));
 }catch(...){return {};}
}
inline bool originalResponseBrowser(const std::string& name){return name=="chrome.exe"||name=="msedge.exe"||name=="firefox.exe"||name=="brave.exe"||name=="opera.exe"||name=="vivaldi.exe";}
inline std::string originalResponseName(const Json& offer){
 auto fallback=[&]{Url url("http://"+str(offer,"Host")+str(offer,"Target"));return safeName(unescape(url.path));};
 auto field=offer.find("ContentDisposition");if(field==offer.end()||!field->is_string())return fallback();
 return contentDispositionName(field->get<std::string>(),fallback());
}
inline bool originalResponseAllowed(const Json& prefs,const std::string& browser,const Json& offer){
 try{
  if(browser.empty()||!browserCaptureAllowed(prefs,browser)||!yes(prefs,"BrowserCaptureEnabled",true))return false;
  if(!yes(offer,"WasInterceptable")||str(offer,"Method")!="GET"||num(offer,"Status")!=200||yes(offer,"RequestBodyFramed")||!str(offer,"Range").empty())return false;
  auto host=str(offer,"Host"),target=str(offer,"Target");
  if(host.empty()||host.find_first_of("/\\?#@ \r\n\t")!=std::string::npos||target.empty()||target[0]!='/')return false;
  auto address=exactCaptureAddress("http://"+host+target);Url url(address);
  for(const auto& excluded:words(str(prefs,"CaptureExcludedHosts")))if(hostIs(url.host,excluded))return false;
  for(auto pattern:addressExceptions(str(prefs,"CaptureExcludedUrls"))){
   if(pattern.front()=='='){if(pattern.substr(1)==address)return false;continue;}
   auto schemeEnd=pattern.find("://"),pathStart=pattern.find('/',schemeEnd+3);
   auto authority=pattern.substr(schemeEnd+3,pathStart-schemeEnd-3);bool subdomains=authority.rfind("*.",0)==0;if(subdomains)authority.erase(0,2);
   Url excluded(pattern.substr(0,schemeEnd+3)+authority+"/");
   if(url.scheme==excluded.scheme&&url.port==excluded.port&&(url.host==excluded.host||(subdomains&&hostIs(url.host,excluded.host)))&&connectionHostGlob(url.path+url.query,pattern.substr(pathStart)))return false;
  }
  // Playback resources require an explicit media-panel selection. A network
  // response alone does not establish the user's intent to save a video.
  auto type=lower(str(offer,"ContentType"));
  if(type.rfind("video/",0)==0||type.rfind("audio/",0)==0||type.find("mpegurl")!=std::string::npos||type.find("dash+xml")!=std::string::npos)return false;
  auto path=originalResponseName(offer);auto slash=path.find_last_of('/'),dot=path.find_last_of('.');
  if(dot==std::string::npos||(slash!=std::string::npos&&dot<slash))return false;
  auto extension=lower(path.substr(dot+1));auto allowed=words(str(prefs,"CaptureExtensions"));
  return std::find(allowed.begin(),allowed.end(),extension)!=allowed.end();
 }catch(...){return false;}
}
}
