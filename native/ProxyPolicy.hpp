#pragma once
#include "Core.hpp"
#include "SocksProxy.hpp"
#include "PacProxy.hpp"
namespace udm {
// FTP proxy conventions. CONNECT (the default) tunnels through an HTTP or SOCKS
// proxy; the others address an FTP gateway during login.
inline std::string ftpProxyType(const Json& prefs){auto type=str(prefs,"FtpProxyType");return type.empty()?"CONNECT":type;}
inline void validateFtpProxyType(const Json& prefs){auto type=ftpProxyType(prefs);if(type!="CONNECT"&&type!="USER user@host"&&type!="SITE host"&&type!="OPEN host")throw std::runtime_error("Choose a valid FTP proxy type.");}
inline Json protocolProxySettings(const Json& prefs,const Url& url){
 auto effective=prefs;effective.erase("ProtocolProxies");
 auto found=prefs.find("ProtocolProxies");if(found==prefs.end())return effective;
 if(!found->is_object())throw std::runtime_error("Invalid per-protocol proxy settings.");
 auto row=found->find(url.scheme);if(row==found->end())return effective;
 if(!row->is_object())throw std::runtime_error("Invalid protocol proxy entry.");
 for(const char* key:{"ProxyMode","Proxy","ProxyBypass","ProxyUser","ProxySecret","ProxyAutoConfigUrl"})effective[key]=str(*row,key);
 if(row->contains("FtpProxyType"))effective["FtpProxyType"]=str(*row,"FtpProxyType");
 return effective;
}
inline void validateProtocolProxies(const Json& prefs){
 validateFtpProxyType(prefs);
 auto found=prefs.find("ProtocolProxies");if(found==prefs.end())return;
 if(!found->is_object()||found->size()>3)throw std::runtime_error("Configure proxies only for HTTP, HTTPS and FTP.");
 for(auto it=found->begin();it!=found->end();++it){
  if((it.key()!="http"&&it.key()!="https"&&it.key()!="ftp")||!it->is_object())throw std::runtime_error("Invalid proxy protocol.");
  for(auto field=it->begin();field!=it->end();++field){
   if(field.key()!="ProxyMode"&&field.key()!="Proxy"&&field.key()!="ProxyBypass"&&field.key()!="ProxyUser"&&field.key()!="ProxySecret"&&field.key()!="ProxyAutoConfigUrl"&&field.key()!="FtpProxyType")throw std::runtime_error("Unknown protocol proxy setting.");
   if(!field->is_string()||field->get_ref<const std::string&>().size()>8192)throw std::runtime_error("Invalid protocol proxy value.");
   const auto& value=field->get_ref<const std::string&>();if(value.find('\0')!=std::string::npos||value.find_first_of("\r\n")!=std::string::npos)throw std::runtime_error("Proxy values must be single lines.");
  }
  auto p=protocolProxySettings(prefs,Url(it.key()+"://example.test/file"));auto mode=str(p,"ProxyMode");
  if(mode!="Connect directly"&&mode!="Use Windows proxy / PAC settings"&&mode!="Use a proxy server"&&!usesPacScript(p)&&!isSocksProxy(p))throw std::runtime_error("Choose a valid protocol proxy mode.");
  validatePacSettings(p);if(isSocksProxy(p))validateSocksSettings(p);else if(mode=="Use a proxy server")validateConnectProxy(p);validateFtpProxyType(p);
 }
}
}
