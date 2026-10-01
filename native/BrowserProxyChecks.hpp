#pragma once
#include "BrowserProxy.hpp"
#include "DownloadPreview.hpp"
static void browserProxyChecks(const fs::path& root,Fixture& fixture){
 auto address=fixture.url("/range");Json direct={{"url",address},{"type","direct"}},route={{"url",address},{"type","http"},{"host","proxy.invalid"},{"port",3128}};
 check(validateBrowserProxy(direct,address)==direct,"Browser direct routes bind to an exact download URL");
 check(validateBrowserProxy(route,address)==route,"Browser HTTP routes validate a separate proxy endpoint");
 for(const auto& change:std::vector<Json>{{{"type","quic"}},{{"host","bad\r\nhost"}},{{"host","user@proxy"}},{{"port",0}},{{"port",65536}},{{"port","3128"}},{{"password","secret"}},{{"url",fixture.url("/other")}}}){
  auto invalid=route;invalid.update(change);rejects([&]{validateBrowserProxy(invalid,address);},"Unsupported, injected and mismatched browser routes are rejected");
 }
 auto socks=route;socks["type"]="socks5";rejects([&]{validateBrowserProxy(socks,address);},"SOCKS handoff cannot guess the browser DNS policy");socks["proxyDNS"]=true;check(str(validateBrowserProxy(socks,address),"type")=="socks5","Proxy-DNS SOCKS5 route accepted");
 auto tls=route;tls["type"]="https";check(validateBrowserProxy(tls,address)==tls,"Encrypted HTTPS proxy is retained as a distinct protocol");
 for(const auto& type:{"socks4","socks5"})for(bool remote:{false,true}){auto r=route;r["type"]=type;r["proxyDNS"]=remote;check(validateBrowserProxy(r,address)==r,"SOCKS handoff preserves explicit DNS ownership");}
 auto extra=tls;extra["proxyDNS"]=false;rejects([&]{validateBrowserProxy(extra,address);},"HTTPS proxy cannot carry SOCKS DNS options");
 socks["proxyDNS"]="false";rejects([&]{validateBrowserProxy(socks,address);},"SOCKS DNS policy must be boolean");
 auto prefs=defaultSettings();prefs["ProxyMode"]="Use a proxy server";prefs["Proxy"]="other.invalid:3128";prefs["ProxyUser"]="other-user";prefs["ProxySecret"]=protect("other-secret");prefs["ProxyBypass"]="*";prefs["ProtocolProxies"]={{"http",{{"ProxyMode","Connect directly"}}}};
 Json data={{"Url",address},{"ProtectedBrowserProxy",protect(route.dump())}};auto applied=browserProxyPreferences(prefs,data);
 check(str(applied,"Proxy")=="proxy.invalid:3128"&&str(applied,"ProxyUser").empty()&&str(applied,"ProxySecret").empty()&&str(applied,"ProxyBypass").empty()&&!applied.contains("ProtocolProxies"),"Browser route replaces unrelated proxy overrides, bypass lists and logins");
 prefs.erase("ProtocolProxies");prefs["Proxy"]="proxy.invalid:3128";applied=browserProxyPreferences(prefs,data);
 check(reveal(str(applied,"ProxySecret"))=="other-secret","Only the same configured proxy can supply saved credentials");
 data["ProtectedBrowserProxy"]=protect(tls.dump());applied=browserProxyPreferences(prefs,data);
 check(str(applied,"ResolvedPacProxyScheme")=="https"&&str(applied,"ProxyUser").empty(),"An HTTPS proxy cannot inherit credentials from HTTP at the same endpoint");
 prefs["ResolvedPacProxyScheme"]="https";applied=browserProxyPreferences(prefs,data);check(reveal(str(applied,"ProxySecret"))=="other-secret","An exact configured encrypted proxy can supply explicit credentials");
 data["ProtectedBrowserProxy"]=protect(route.dump());applied=browserProxyPreferences(prefs,data);check(!applied.contains("ResolvedPacProxyScheme")&&str(applied,"ProxyUser").empty(),"HTTP capture clears an earlier encrypted route and its credentials");
 socks["proxyDNS"]=false;data["ProtectedBrowserProxy"]=protect(socks.dump());applied=browserProxyPreferences(prefs,data);check(!yes(applied,"CapturedProxyDNS",true)&&!applied.contains("ResolvedPacProxyScheme"),"Captured local DNS replaces stale encrypted proxy transport metadata");
 prefs["CapturedProxyDNS"]=false;
 data["ProtectedBrowserProxy"]=protect(direct.dump());applied=browserProxyPreferences(prefs,data);
 check(str(applied,"ProxyMode")=="Connect directly"&&str(applied,"ProxySecret").empty(),"Observed direct requests override the default desktop proxy without credentials");
 check(!applied.contains("CapturedProxyDNS")&&!applied.contains("ResolvedPacProxyScheme"),"Direct capture clears all inherited transport metadata");
 HttpSession session(applied);check(!session.forUrl(Url(address)),"Captured route can be reused for ranges on the exact URL");rejects([&]{session.forUrl(Url(fixture.url("/redirect")));},"Redirects cannot silently change the captured browser route");
 auto stale=data;stale["Url"]=fixture.url("/changed");rejects([&]{browserProxyPreferences(prefs,stale);},"A manually changed URL cannot silently reuse an earlier captured route");
 prefs["UseBrowserProxy"]=false;check(browserProxyPreferences(prefs,stale)==prefs,"Explicitly disabling browser route inheritance restores desktop preferences");prefs.erase("UseBrowserProxy");
 Manager m(root/L"browser-proxy");m.state["Settings"]["ProxyMode"]="Use a proxy server";m.state["Settings"]["Proxy"]="127.0.0.1:1";m.state["Settings"]["DuplicatePolicy"]="Numbered";m.state["Settings"]["SkipBrowserFileInfo"]=true;
 auto j=m.receive({{"action","add"},{"url",address},{"filename","captured.bin"},{"downloadLater",true},{"browserProxy",direct}});
 check(readBrowserProxy(j->data)==direct&&str(j->data,"Status")=="Paused","Browser handoff saves its protected proxy with the first download record");
 auto stored=readText(m.root/L"state.json");check(stored.find("\"type\":\"direct\"")==std::string::npos,"Browser routing metadata is encrypted in saved history");
 Manager reopened(m.root);check(readBrowserProxy(reopened.jobs[0]->data)==direct,"Captured routes survive a desktop restart");
 auto preview=probeDownload(j->data,m.state["Settings"],Cancel{});check(str(preview,"Status")=="Ready"&&num(preview,"Size")==fixture.payload.size(),"File Info metadata uses the browser route despite a blocked desktop default");
 transfer(m,j,std::make_shared<Cancel>());check(str(j->data,"Status")=="Complete"&&readText(j->target())==fixture.payload,"Native segmented download uses the captured route and publishes exact bytes");
 auto copy=m.redownload(j);check(readBrowserProxy(copy->data)==direct,"Redownload preserves the browser route before its initial save");
 auto exported=exportCatalog(m,{copy});check(yes(exported["Downloads"][0],"RequiresBrowserProxyCapture")&&!exported["Downloads"][0].contains("ProtectedBrowserProxy"),"Ordinary catalog export preserves route recapture requirements without proxy data");
 Manager imported(root/L"browser-proxy-import");importCatalog(imported,exported,utf8((root/L"browser-proxy-files").wstring()));rejects([&]{browserProxyPreferences(defaultSettings(),imported.jobs[0]->data);},"Imported downloads cannot silently discard an omitted proxy route");
 auto encrypted=exportCatalog(m,{copy},true);Manager privateImport(root/L"browser-proxy-private");importCatalog(privateImport,encrypted,utf8((root/L"browser-proxy-private-files").wstring()),true);check(readBrowserProxy(privateImport.jobs[0]->data)==direct,"Explicit encrypted catalog transfer retains the browser route");
 m.beginAddressRefresh(copy);auto next=fixture.url("/fresh");auto nextRoute=direct;nextRoute["url"]=next;
 auto refreshed=m.receive({{"action","add"},{"url",next},{"filename",str(copy->data,"FileName")},{"browserProxy",nextRoute}});
 check(refreshed==copy&&readBrowserProxy(copy->data)==direct,"Pending refresh does not alter the active saved proxy before review");
 auto offer=m.addressRefreshCandidate(copy);m.refreshAddress(copy,next,Headers{},str(offer,"page"));check(readBrowserProxy(copy->data)==nextRoute,"Accepted address refresh installs the new request-bound proxy");
}
