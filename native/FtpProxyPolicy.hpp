#pragma once
#include "SocksProxy.hpp"
#include <cctype>
#include <sstream>
namespace udm {
// A static Windows bypass is a known direct route. Automatic PAC/WPAD must be
// resolved independently; its presence never authorizes a guessed direct FTP route.
inline bool windowsFtpBypass(const Url& url,std::string bypass){
 for(char& ch:bypass)if(std::isspace(static_cast<unsigned char>(ch)))ch=';';
 return socksBypass(url,bypass);
}
inline bool windowsFtpDirect(const Url& url,bool autoDetect,bool autoConfig,bool staticProxy,const std::string& bypass){
 return !autoDetect&&!autoConfig&&(!staticProxy||windowsFtpBypass(url,bypass));
}
inline Json windowsFtpPreferences(const Url& url,Json prefs,bool autoDetect,const std::string& autoConfigUrl,const std::string& staticProxy,const std::string& bypass){
 if(!autoConfigUrl.empty()){
  prefs["ProxyMode"]="Use automatic configuration script";prefs["ProxyAutoConfigUrl"]=autoConfigUrl;prefs["ProxyAutoDetect"]=false;
  prefs["Proxy"]="";prefs["ProxyBypass"]="";prefs["ProxyUser"]="";prefs["ProxySecret"]="";
  return prefs;
 }
 if(autoDetect){
  prefs["ProxyMode"]="Use automatic configuration script";prefs["ProxyAutoConfigUrl"]="";prefs["ProxyAutoDetect"]=true;
  prefs["Proxy"]="";prefs["ProxyBypass"]="";prefs["ProxyUser"]="";prefs["ProxySecret"]="";
  return prefs;
 }
 if(windowsFtpDirect(url,autoDetect,false,!staticProxy.empty(),bypass)){
  prefs["ProxyMode"]="Connect directly";prefs["Proxy"]="";prefs["ProxyBypass"]="";
  prefs["ProxyUser"]="";prefs["ProxySecret"]="";return prefs;
 }
 std::string entries=staticProxy;for(char& ch:entries)if(ch==';')ch=' ';
 std::istringstream input(entries);std::string entry,ftpRoute;bool seenFtp=false;
 while(input>>entry)if(lower(entry).rfind("ftp=",0)==0){
  if(seenFtp)throw std::runtime_error("Windows specifies multiple FTP proxy routes. Choose an explicit UDM proxy route.");
  seenFtp=true;
  ftpRoute=entry.substr(4);
 }
 if(lower(ftpRoute).rfind("http://",0)==0){
  prefs["ProxyMode"]="Use a proxy server";prefs["Proxy"]=ftpRoute.substr(7);prefs["ProxyBypass"]="";
  prefs["ProxyUser"]="";prefs["ProxySecret"]="";validateConnectProxy(prefs);return prefs;
 }
 throw std::runtime_error("FTP through this Windows proxy route is not supported. Configure an explicit proxy or bypass for this server.");
}
}
