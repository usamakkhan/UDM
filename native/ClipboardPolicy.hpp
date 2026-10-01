#pragma once
#include "Core.hpp"
#include <algorithm>
namespace udm {
inline bool clipboardCandidate(const std::string& value,const Json& prefs){
 if(!yes(prefs,"ClipboardMonitor")||value.empty()||value.size()>32768)return false;
 try{Url url(value);if(url.scheme!="http"&&url.scheme!="https"&&url.scheme!="ftp")return false;
  for(auto host:words(str(prefs,"CaptureExcludedHosts"))){while(!host.empty()&&(host[0]=='*'||host[0]=='.'))host.erase(0,1);if(!host.empty()&&hostIs(url.host,host))return false;}
  if(!yes(prefs,"ClipboardOnlyFileTypes",true))return true;
  auto path=url.path;auto last=path.find_last_of('/'),dot=path.find_last_of('.');if(dot==std::string::npos||(last!=std::string::npos&&dot<last))return false;auto ext=lower(path.substr(dot+1));auto allowed=words(str(prefs,"CaptureExtensions"));return std::find(allowed.begin(),allowed.end(),ext)!=allowed.end();
 }catch(...){return false;}
}
}
