#pragma once
#include "ProxyPolicy.hpp"
#include "FtpProxyPolicy.hpp"
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
 auto pac=defaultSettings();pac["ProxyMode"]="Use automatic configuration script";pac["ProxyAutoConfigUrl"]="https://proxy.example.test/config.pac";validatePacSettings(pac);m.setSettings(pac);m.save();Manager pacReloaded(m.root);
 check(pacReloaded.state["Settings"]["ProxyAutoConfigUrl"]==pac["ProxyAutoConfigUrl"],"Custom PAC URL survives saving and reopening settings");
 check(!yes(pac,"IgnoreLastModified")&&yes(pac,"UseTls13"),"Default transport settings keep date validation and enable supported TLS 1.3");
 for(const auto& value:std::vector<Json>{"","file:///C:/proxy.pac","ftp://proxy.test/a",123,"http://proxy.test/a b","http://proxy.test/a\r\n"}){auto invalid=pac;invalid["ProxyAutoConfigUrl"]=value;rejects([&]{m.setSettings(invalid);},"Unsafe or missing PAC addresses cannot be saved");}
 auto per=pac;per["ProtocolProxies"]={{"https",{{"ProxyMode","Connect directly"}}}};check(str(protocolProxySettings(per,Url("https://example.test/")),"ProxyAutoConfigUrl").empty(),"An explicit protocol route clears the default PAC script");
 per["ProtocolProxies"]["https"]={{"ProxyMode","Use automatic configuration script"},{"ProxyAutoConfigUrl","http://proxy.test/secure.pac"}};validateProtocolProxies(per);check(str(protocolProxySettings(per,Url("https://example.test/")),"ProxyAutoConfigUrl")=="http://proxy.test/secure.pac","A protocol-specific PAC script overrides the default script");
 per["ProtocolProxies"]["https"]["ProxyAutoConfigUrl"]="";rejects([&]{m.setSettings(per);},"Missing protocol PAC URLs are rejected before transfers");
 for(const char* key:{"UseTls13","IgnoreLastModified"}){auto invalid=pac;invalid[key]="yes";rejects([&]{m.setSettings(invalid);},"Transport checkboxes require boolean settings");}
 // SECURE_PROTOCOLS is set-only in WinHTTP; real ClientHello coverage is in transport-options.native-live.cjs.
 check(retryPacConnection(ERROR_WINHTTP_CANNOT_CONNECT)&&retryPacConnection(ERROR_WINHTTP_TIMEOUT)&&!retryPacConnection(ERROR_WINHTTP_SECURE_FAILURE)&&!retryPacConnection(ERROR_WINHTTP_LOGIN_FAILURE),"PAC failover distinguishes connection failures from TLS and authentication errors");
 Cancel cancelled;cancelled.stop=true;rejects([&]{resolvePacRoutes(pac,Url("https://example.test/"),cancelled);},"Cancelled PAC requests exit before starting script retrieval");
 pac["ProxyBypass"]="*.example.test";Cancel live;auto direct=resolvePacRoutes(pac,Url("https://cdn.example.test/file"),live);check(direct.size()==1&&str(direct[0],"ProxyMode")=="Connect directly"&&str(direct[0],"ProxySecret").empty(),"An explicit PAC bypass uses a direct route without proxy credentials");
 auto ftpUrl=Url("ftp://cdn.example.test/archive.zip");
 check(windowsFtpDirect(ftpUrl,false,false,true,"*.example.test")&&windowsFtpDirect(ftpUrl,false,false,false,""),"Windows static FTP bypass and unconfigured proxy use direct routes");
 check(windowsFtpDirect(ftpUrl,false,false,true,"other.test  *.example.test")&&windowsFtpDirect(ftpUrl,false,false,true,"other.test\t*.example.test"),"Windows FTP bypass accepts whitespace-separated host rules");
 check(!windowsFtpDirect(ftpUrl,false,false,true,"other.test *.invalid.test"),"Windows FTP bypass does not match unrelated whitespace-separated rules");
 check(!windowsFtpDirect(ftpUrl,false,false,true,"other.test")&&!windowsFtpDirect(ftpUrl,true,false,true,"*.example.test")&&!windowsFtpDirect(ftpUrl,false,true,true,"*.example.test"),"FTP never guesses a direct route through a configured proxy or automatic PAC/WPAD");
 auto windows=defaultSettings();windows["ProxyMode"]="Use Windows proxy / PAC settings";
 auto scripted=windowsFtpPreferences(ftpUrl,windows,true,"http://proxy.example.test/ftp.pac","static.example.test:8080","*.example.test");
 check(str(scripted,"ProxyMode")=="Use automatic configuration script"&&str(scripted,"ProxyAutoConfigUrl")=="http://proxy.example.test/ftp.pac"&&str(scripted,"ProxyBypass").empty(),"Windows-configured FTP PAC URL takes precedence over static bypass without guessing direct access");
 auto bypassed=windowsFtpPreferences(ftpUrl,windows,false,"","static.example.test:8080","*.example.test");check(str(bypassed,"ProxyMode")=="Connect directly"&&str(bypassed,"Proxy").empty(),"Windows static FTP bypass resolves to credential-free direct settings");
 auto staticHttp=windowsFtpPreferences(ftpUrl,windows,false,"","http=other.test:8080;ftp=http://ftp-proxy.test:3128","other.test");check(str(staticHttp,"ProxyMode")=="Use a proxy server"&&str(staticHttp,"Proxy")=="ftp-proxy.test:3128"&&str(staticHttp,"ProxyBypass").empty(),"Protocol-labeled Windows HTTP FTP proxy selects the CONNECT route");
 rejects([&]{windowsFtpPreferences(ftpUrl,windows,false,"","ftp=ftp://legacy-gateway.test:21","");},"Windows FTP gateway is not misidentified as an HTTP CONNECT proxy");
 rejects([&]{windowsFtpPreferences(ftpUrl,windows,false,"","http=other.test:8080","");},"Unmatched Windows HTTP-only proxy entry does not imply direct FTP");
 auto detected=windowsFtpPreferences(ftpUrl,windows,true,"","static.example.test:8080","*.example.test");check(str(detected,"ProxyMode")=="Use automatic configuration script"&&yes(detected,"ProxyAutoDetect")&&str(detected,"ProxyAutoConfigUrl").empty()&&str(detected,"ProxyBypass").empty(),"Windows FTP WPAD uses the asynchronous resolver without guessing a static bypass");validatePacSettings(detected);
 Cancel stopped;stopped.stop=true;rejects([&]{resolvePacRoutes(detected,ftpUrl,stopped);},"Cancelled FTP WPAD exits before network discovery");

}
