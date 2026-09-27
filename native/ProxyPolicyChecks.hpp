#pragma once
#include "ProxyPolicy.hpp"
static void proxyPolicyChecks(const fs::path& root){
 auto prefs=defaultSettings();prefs["ProxyMode"]="Use a SOCKS5 proxy";prefs["Proxy"]="default.invalid:1080";prefs["ProxyUser"]="default";prefs["ProxySecret"]=protect("global-secret");
 auto original=prefs;check(protocolProxySettings(prefs,Url("https://example.test/file"))==prefs,"Existing global proxy settings retain their behavior");
 prefs["ProtocolProxies"]={{"http",{{"ProxyMode","Connect directly"}}},{"https",{{"ProxyMode","Use a proxy server"},{"Proxy","secure.invalid:3128"},{"ProxyUser","secure"},{"ProxySecret",protect("secure-secret")}}},{"ftp",{{"ProxyMode","Use a SOCKS4 / 4a proxy"},{"Proxy","ftp.invalid:1080"},{"ProxyUser","ftp-id"}}}};
 validateProtocolProxies(prefs);
 auto http=protocolProxySettings(prefs,Url("http://example.test/file")),https=protocolProxySettings(prefs,Url("https://example.test/file")),ftp=protocolProxySettings(prefs,Url("ftp://example.test/file"));
 check(str(http,"ProxyMode")=="Connect directly"&&str(http,"ProxyUser").empty()&&str(http,"ProxySecret").empty(),"A direct protocol override does not inherit the global proxy login");
 check(str(https,"Proxy")=="secure.invalid:3128"&&reveal(str(https,"ProxySecret"))=="secure-secret","HTTPS receives only its configured proxy and login");
 check(str(ftp,"ProxyMode")=="Use a SOCKS4 / 4a proxy"&&str(ftp,"ProxyUser")=="ftp-id"&&str(ftp,"ProxySecret").empty(),"FTP selects its independently configured SOCKS route");
 check(!https.contains("ProtocolProxies")&&str(https,"DownloadFolder")==str(prefs,"DownloadFolder"),"Resolved proxy preferences preserve ordinary transfer options without recursive routing");
 check(prefs["ProtocolProxies"].size()==3&&str(prefs,"Proxy")=="default.invalid:1080","Route selection does not mutate stored settings");
 auto inherited=prefs;inherited["ProtocolProxies"].erase("http");check(protocolProxySettings(inherited,Url("http://example.test/file"))==original,"Removing an override restores the default route");
 for(const char* key:{"gopher","HTTP","https://example.test"}){auto bad=prefs;bad["ProtocolProxies"][key]={{"ProxyMode","Connect directly"}};rejects([&]{validateProtocolProxies(bad);},"Unknown proxy protocols are rejected");}
 for(auto value:{Json::array(),Json("bad"),Json(123)}){auto bad=prefs;bad["ProtocolProxies"]=value;rejects([&]{validateProtocolProxies(bad);},"Malformed proxy tables are rejected");}
 for(const auto& row:std::vector<Json>{{{"ProxyMode","unknown"}},{{"ProxyMode","Use a proxy server"},{"Proxy","host:0"}},{{"ProxyMode","Connect directly"},{"Other","value"}},{{"ProxyMode","Use a SOCKS5 proxy"},{"Proxy","host:1080"},{"ProxyUser","missing-password"}},{{"ProxyMode","Use a proxy server"},{"Proxy","host:3128"},{"ProxyUser","a:b"}},{{"ProxyMode","Use a proxy server"},{"Proxy","host:3128"},{"ProxyBypass","host\r\nInjected"}}}){
  auto bad=prefs;bad["ProtocolProxies"]["https"]=row;rejects([&]{validateProtocolProxies(bad);},"Invalid endpoint, login or injected proxy values cannot be saved");
 }
 {auto corrupt=prefs;corrupt["ProtocolProxies"]=Json::array();rejects([&]{HttpSession session(corrupt);},"Corrupt saved protocol settings cannot fall back to a default HTTP route");}
 Manager m(root/L"protocol-proxies");m.setSettings(prefs);m.save();Manager reopened(m.root);auto stored=readText(m.root/L"state.json");
 check(!stored.empty()&&stored.find("global-secret")==std::string::npos&&stored.find("secure-secret")==std::string::npos,"Both default and per-protocol passwords are protected at rest");
 check(reveal(str(protocolProxySettings(reopened.state["Settings"],Url("https://example.test/file")),"ProxySecret"))=="secure-secret","Per-protocol credentials survive application restart");
 validateConnectProxy({{"Proxy","[::1]:3128"},{"ProxyUser","user"},{"ProxySecret",protect("password")}});check(true,"HTTP CONNECT accepts an IPv6 endpoint and independent login");
 auto bad=prefs;bad["ProtocolProxies"]["https"]["Proxy"]=std::string("host\0:3128",10);rejects([&]{m.setSettings(bad);},"Settings validation rejects an embedded null in the proxy endpoint");
}
