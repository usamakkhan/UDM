#pragma once
#include "ProxyPolicy.hpp"
#include <algorithm>
namespace udm {
inline Json validateBrowserProxy(const Json& input,const std::string& address){
 if(input.is_object()&&input.empty())return Json::object();
 if(!input.is_object()||input.size()>5)throw std::runtime_error("Invalid captured browser proxy.");
 for(auto it=input.begin();it!=input.end();++it)if(it.key()!="url"&&it.key()!="type"&&it.key()!="host"&&it.key()!="port"&&it.key()!="proxyDNS")throw std::runtime_error("Unknown browser proxy field.");
 for(const char* key:{"url","type"})if(!input.contains(key)||!input[key].is_string())throw std::runtime_error("Incomplete browser proxy.");
 auto raw=str(input,"url"),type=str(input,"type");
 if(raw.size()>16000||raw.find('\0')!=std::string::npos||raw.find_first_of("\r\n")!=std::string::npos)throw std::runtime_error("Invalid captured proxy URL.");
 Url url(raw),target(address);
 if((url.scheme!="http"&&url.scheme!="https")||url.full!=target.full)throw std::runtime_error("The captured browser proxy belongs to another URL. Capture this download again.");
 if(type=="direct"){if(input.size()!=2)throw std::runtime_error("A direct browser route cannot contain proxy fields.");return {{"url",url.full},{"type",type}};}
 if(type!="http"&&type!="https"&&type!="socks5"&&type!="socks4")throw std::runtime_error("This browser proxy type is not supported. Keep the download in the browser.");
 if(!input.contains("host")||!input["host"].is_string()||!input.contains("port")||!input["port"].is_number_integer())throw std::runtime_error("Incomplete browser proxy endpoint.");
 auto host=str(input,"host");auto port=num(input,"port");
 if(host.empty()||host.size()>255||port<1||port>65535||std::any_of(host.begin(),host.end(),[](unsigned char c){return c<=32||c>=127;})||host.find_first_of("/\\@?#;=[]")!=std::string::npos)throw std::runtime_error("Invalid browser proxy endpoint.");
 auto endpoint=(host.find(':')!=std::string::npos?"["+host+"]":host)+":"+std::to_string(port);
 validateConnectProxy({{"Proxy",endpoint}});
 if(type=="http"||type=="https"){if(input.contains("proxyDNS"))throw std::runtime_error("Unexpected DNS option for an HTTP proxy.");}
 else if(!input.contains("proxyDNS")||!input["proxyDNS"].is_boolean())throw std::runtime_error("The captured SOCKS DNS policy is missing. Capture this download again.");
 Json result={{"url",url.full},{"type",type},{"host",lower(host)},{"port",port}};if(type=="socks4"||type=="socks5")result["proxyDNS"]=input["proxyDNS"];return result;
}
inline Json readBrowserProxy(const Json& data){
 if(yes(data,"RequiresBrowserProxyCapture"))throw std::runtime_error("Capture this download again in the browser to restore its proxy route.");
 auto encoded=str(data,"ProtectedBrowserProxy");if(encoded.empty())return Json::object();
 auto plain=reveal(encoded);if(plain.empty())throw std::runtime_error("Cannot read the saved browser proxy. Capture this download again.");
 return validateBrowserProxy(Json::parse(plain),str(data,"Url"));
}
inline Json browserProxyPreferences(const Json& prefs,const Json& data){
 if(!yes(prefs,"UseBrowserProxy",true))return prefs;
 auto route=readBrowserProxy(data);if(route.empty())return prefs;
 auto effective=prefs;auto configured=protocolProxySettings(prefs,Url(str(data,"Url")));
 effective.erase("ProtocolProxies");effective.erase("ResolvedPacProxyScheme");effective.erase("CapturedProxyDNS");
 for(const char* key:{"Proxy","ProxyBypass","ProxyUser","ProxySecret","ProxyAutoConfigUrl"})effective[key]="";
 auto type=str(route,"type");auto mode=type=="direct"?"Connect directly":(type=="http"||type=="https")?"Use a proxy server":type=="socks5"?"Use a SOCKS5 proxy":"Use a SOCKS4 / 4a proxy";
 effective["ProxyMode"]=mode;effective["CapturedProxyUrl"]=str(route,"url");
 if(type=="https")effective["ResolvedPacProxyScheme"]="https";
 if(type=="socks4"||type=="socks5")effective["CapturedProxyDNS"]=route["proxyDNS"];
 if(type!="direct"){
  auto host=str(route,"host");auto endpoint=(host.find(':')!=std::string::npos?"["+host+"]":host)+":"+std::to_string(num(route,"port"));effective["Proxy"]=endpoint;
  // Only explicit credentials for this exact proxy and protocol may be reused.
  const bool sameScheme=(type=="https")== (str(configured,"ResolvedPacProxyScheme")=="https");
  if(sameScheme&&str(configured,"ProxyMode")==mode&&lower(trim(str(configured,"Proxy")))==lower(endpoint)){
   effective["ProxyUser"]=str(configured,"ProxyUser");effective["ProxySecret"]=str(configured,"ProxySecret");
  }
 }
 return effective;
}
}
