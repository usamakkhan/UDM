#include "DashMetadataTransport.hpp"
#include "DashManifest.hpp"
#include <iostream>
using namespace udm;
int wmain(int argc,wchar_t** argv){
 if(argc!=4||fs::exists(argv[1]))return 2;const auto root=fs::absolute(argv[1]);fs::create_directories(root);const auto a=utf8(argv[2]),b=utf8(argv[3]);Json checks=Json::array();std::string error;WSADATA sockets{};WSAStartup(MAKEWORD(2,2),&sockets);CoInitializeEx(nullptr,COINIT_MULTITHREADED);
 auto check=[&](bool ok,const char* name){checks.push_back({{"name",name},{"passed",ok}});if(!ok)throw std::runtime_error(name);};
 try{
  Json prefs={{"ProxyMode","Connect directly"},{"SiteLogins",Json::array()},{"WindowsLoginIntranet",false},{"ActiveBrowserSession",{{"Origin",a},{"UserAgent","Owned browser"},{"LogoutPages",""},{"Cookies",Json::array()}}}};
  auto cancel=std::make_shared<Cancel>();cancel->deadline=GetTickCount64()+15000;std::weak_ptr<HttpSession> lease;
  {
   DashMetadataTransportScope scope;Headers auth={{"Authorization","Bearer owned-token"}};
   auto first=refreshDashManifest(a+"/manifest.mpd",auth,prefs,cancel);lease=dashMetadataTransport(a,prefs);check(!lease.expired(),"Actual manifest uses the recording-owned transport");
   auto second=refreshDashManifest(a+"/manifest.mpd",auth,prefs,cancel,&first);check(second.document==first.document,"A repeated manifest remains bound to its source document");
   auto clock=synchronizeDashClock(second.manifest,second.url,prefs,second.bodyReceived,cancel);check(!clock.scheme.empty(),"Actual HTTP clock synchronization succeeds through sanitized preferences");
   auto sanitized=prefs;sanitized.erase("ActiveBrowserSession");
   auto clockPool=dashMetadataTransport(a,sanitized,DashMetadataTransportRole::Clock);
   auto discoveryPool=dashMetadataTransport(a,sanitized,DashMetadataTransportRole::ResourceDiscovery);
   check(clockPool!=discoveryPool,"Identical sanitized settings still isolate clock and discovery roles");
   DashResourceHeaders resources={{a,{{"Authorization","Bearer resource-token"}}}};
   auto version=discoverDashResourceVersion({a+"/resource.bin"},resources,prefs,cancel);
   auto same=discoverDashResourceVersion({a+"/resource.bin"},resources,prefs,cancel);
   check(version==same&&str(version,"etag")=="\"owned-resource\""&&num(version,"total")==8,"HEAD fallback preserves the same strong version and full resource length");
   check(discoveryPool==dashMetadataTransport(a,sanitized,DashMetadataTransportRole::ResourceDiscovery),"Repeated version discovery retains its role-owned session");
   synchronizeDashClock(second.manifest,second.url,prefs,second.bodyReceived,cancel);
   refreshDashManifest(b+"/manifest.mpd",{},prefs,cancel);
   auto redirected=refreshDashManifest(a+"/redirect",auth,prefs,cancel);check(redirected.url==b+"/manifest.mpd","Cross-origin redirect completes with its independently scoped transport");
   refreshDashManifest(a+"/manifest.mpd",{{"Authorization","Bearer changed-token"}},prefs,cancel);
   refreshDashManifest(a+"/manifest.mpd",{},prefs,cancel);
  }
  check(lease.expired(),"Fully closed requests do not retain the recording's pool after scope exit");
  {DashMetadataTransportScope later;refreshDashManifest(a+"/manifest.mpd",{},prefs,cancel);}
 }catch(const std::exception& e){error=e.what();}
 atomicText(root/L"results.json",Json{{"passed",error.empty()},{"error",error},{"checks",checks}}.dump(2),false);std::cout<<checks.size()<<" checks; "<<error<<"\n";CoUninitialize();WSACleanup();return error.empty()?0:1;
}
