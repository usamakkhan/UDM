#pragma once
#include "BrowserSettings.hpp"
#include "ServerConnections.hpp"
namespace udm {
inline std::string originalResponseName(const Json& offer){
 auto fallback=[&]{Url url("http://"+str(offer,"Host")+str(offer,"Target"));return safeName(unescape(url.path));};
 auto field=offer.find("ContentDisposition");if(field==offer.end()||!field->is_string())return fallback();
 auto header=field->get<std::string>();if(header.size()>8192||header.find_first_of("\r\n")!=std::string::npos)return fallback();
 std::map<std::string,std::string> parameters;size_t pos=header.find(';');
 while(pos!=std::string::npos&&pos<header.size()){
  ++pos;while(pos<header.size()&&isspace(static_cast<unsigned char>(header[pos])))++pos;
  auto equals=header.find('=',pos),separator=header.find(';',pos);if(equals==std::string::npos||(separator!=std::string::npos&&separator<equals)){pos=separator;continue;}
  auto key=lower(trim(header.substr(pos,equals-pos)));pos=equals+1;while(pos<header.size()&&isspace(static_cast<unsigned char>(header[pos])))++pos;
  std::string value;
  if(pos<header.size()&&header[pos]=='"'){
   ++pos;bool closed=false;while(pos<header.size()){char c=header[pos++];if(c=='"'){closed=true;break;}if(c=='\\'&&pos<header.size())c=header[pos++];value+=c;}
   if(!closed)return fallback();while(pos<header.size()&&isspace(static_cast<unsigned char>(header[pos])))++pos;if(pos<header.size()&&header[pos]!=';')return fallback();
  }else{auto end=header.find(';',pos);value=trim(header.substr(pos,end==std::string::npos?end:end-pos));pos=end;}
  if((key=="filename"||key=="filename*")&&!parameters.emplace(key,value).second)return fallback();
 }
 auto valid=[](const std::string& value){return !value.empty()&&value.find('\0')==std::string::npos&&MultiByteToWideChar(CP_UTF8,MB_ERR_INVALID_CHARS,value.data(),static_cast<int>(value.size()),nullptr,0)>0;};
 auto extended=parameters.find("filename*");if(extended!=parameters.end()){
  auto value=extended->second;auto first=value.find('\''),second=first==std::string::npos?first:value.find('\'',first+1);
  if(first!=std::string::npos&&second!=std::string::npos&&lower(value.substr(0,first))=="utf-8"){
   auto encoded=value.substr(second+1);bool escapes=true;for(size_t i=0;i<encoded.size();++i)if(encoded[i]=='%'){if(i+2>=encoded.size()||!isxdigit(static_cast<unsigned char>(encoded[i+1]))||!isxdigit(static_cast<unsigned char>(encoded[i+2]))){escapes=false;break;}i+=2;}
   if(escapes){auto decoded=unescape(encoded);if(valid(decoded))return safeName(decoded);}
  }
 }
 auto plain=parameters.find("filename");if(plain!=parameters.end()&&valid(plain->second))return safeName(plain->second);
 return fallback();
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
