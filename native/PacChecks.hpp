#pragma once
// Loopback proxy auto-configuration: a script served over HTTP is fetched and
// evaluated by the Windows resolver, so PAC route order, bypass and failover
// can be checked without a real proxy deployment. WPAD discovery (DHCP/DNS)
// cannot be simulated on a loopback and stays unqualified.
#include "ProxyLoginChecks.hpp"
#include "BrowserProxy.hpp"
#include "DownloadPreview.hpp"
#include "InlinePacSource.hpp"
class PacScriptFixture {
 SOCKET listener=INVALID_SOCKET;std::thread server;std::vector<std::thread> clients;std::atomic_bool stopping{false};std::string script;
 void serve(SOCKET s){
  try{DWORD timeout=5000;setsockopt(s,SOL_SOCKET,SO_RCVTIMEO,(const char*)&timeout,sizeof(timeout));std::string request;char buffer[4096];
   while(request.find("\r\n\r\n")==std::string::npos&&request.size()<16384){int n=recv(s,buffer,sizeof(buffer),0);if(n<=0)break;request.append(buffer,n);}
   ++requests;const bool found=request.rfind("GET /proxy.pac ",0)==0;if(found)++served;
   std::string response=found?"HTTP/1.1 200 OK\r\nContent-Type: application/x-ns-proxy-autoconfig\r\nContent-Length: "+std::to_string(script.size())+"\r\nConnection: close\r\n\r\n"+script:"HTTP/1.1 404 Not Found\r\nContent-Length: 0\r\nConnection: close\r\n\r\n";
   for(size_t at=0;at<response.size();){int n=send(s,response.data()+at,(int)(response.size()-at),0);if(n<=0)break;at+=n;}
  }catch(...){}
  shutdown(s,SD_BOTH);closesocket(s);
 }
public:
 unsigned short port=0;std::atomic_int requests{0},served{0};
 explicit PacScriptFixture(std::string text):script(std::move(text)){
  listener=socket(AF_INET,SOCK_STREAM,IPPROTO_TCP);sockaddr_in address{};address.sin_family=AF_INET;address.sin_addr.s_addr=htonl(INADDR_LOOPBACK);
  if(bind(listener,(sockaddr*)&address,sizeof(address))||listen(listener,16))throw std::runtime_error("PAC fixture failed to listen.");int size=sizeof(address);getsockname(listener,(sockaddr*)&address,&size);port=ntohs(address.sin_port);
  server=std::thread([this]{while(!stopping){fd_set set;FD_ZERO(&set);FD_SET(listener,&set);timeval wait{0,100000};if(select(0,&set,nullptr,nullptr,&wait)<=0)continue;auto client=accept(listener,nullptr,nullptr);if(client!=INVALID_SOCKET)clients.emplace_back([this,client]{serve(client);});}});
 }
 ~PacScriptFixture(){stopping=true;if(server.joinable())server.join();closesocket(listener);for(auto& t:clients)if(t.joinable())t.join();}
 std::string url()const{return "http://127.0.0.1:"+std::to_string(port)+"/proxy.pac";}
};
static void pacChecks(const udm::fs::path& root){
 using namespace udm;int scenario=0;
 // The authenticating proxy serves the payload itself, so a route through it is
 // observable; a closed port stands in for an unreachable proxy.
 AuthProxyFixture proxy("basic");
 unsigned short closedPort=0;{SOCKET probe=socket(AF_INET,SOCK_STREAM,IPPROTO_TCP);sockaddr_in a{};a.sin_family=AF_INET;a.sin_addr.s_addr=htonl(INADDR_LOOPBACK);bind(probe,(sockaddr*)&a,sizeof(a));int size=sizeof(a);getsockname(probe,(sockaddr*)&a,&size);closedPort=ntohs(a.sin_port);closesocket(probe);}
 const std::string closed="127.0.0.1:"+std::to_string(closedPort);
 PacScriptFixture pac("function FindProxyForURL(url,host){\n if(shExpMatch(host,\"*.direct.example.test\"))return \"DIRECT\";\n if(host==\"failover.example.test\")return \"PROXY "+closed+"; PROXY "+proxy.address()+"\";\n if(host==\"dead.example.test\")return \"PROXY "+closed+"\";\n if(host==\"socks.example.test\")return \"SOCKS "+proxy.address()+"\";\n return \"PROXY "+proxy.address()+"; DIRECT\";\n}\n");
 auto prefs=[&]{auto p=defaultSettings();p["ProxyMode"]="Use automatic configuration script";p["ProxyAutoConfigUrl"]=pac.url();p["ProxyUser"]="proxy-user";p["ProxySecret"]=protect("proxy-secret");return p;};
 Cancel live;
 {
  const std::string text="function FindProxyForURL(){return 'DIRECT';}";
  auto source=std::make_unique<InlinePacSource>(text);auto settings=defaultSettings();settings["ProxyMode"]="Connect directly";
  Http valid(source->url(),Headers{},settings,live);auto bytes=valid.all(65536,live);
  check(valid.status==200&&std::string(bytes.begin(),bytes.end())==text,"Inline PAC source serves exact bytes at its private endpoint");
  Http missing(combineUrl(source->url(),"/wrong-token.pac"),Headers{},settings,live);
  check(missing.status==404&&missing.all(65536,live).empty(),"Inline PAC source rejects requests without its random path");
  Url endpoint(source->url());SOCKET slow=socket(AF_INET,SOCK_STREAM,IPPROTO_TCP);sockaddr_in local{};local.sin_family=AF_INET;local.sin_addr.s_addr=htonl(INADDR_LOOPBACK);local.sin_port=htons(endpoint.port);
  if(connect(slow,(sockaddr*)&local,sizeof(local))){closesocket(slow);throw std::runtime_error("Cannot connect inline PAC slow-client test.");}
  ::send(slow,"GET ",4,0);Sleep(50);auto started=GetTickCount64();source.reset();auto elapsed=GetTickCount64()-started;closesocket(slow);
  check(elapsed<2000,"Inline PAC source shutdown does not wait for a stalled client request");
 }
 for(bool inlineScript:{false,true}){
  const std::string script="function FindProxyForURL(url,host){return 'DIRECT';}";
  Fixture origin;PacScriptFixture browserPac(script);
  const auto address=origin.url("/range");Json config={{"url",address},{"type","pac"},{"pacUrl",browserPac.url()}};
  if(inlineScript){config.erase("pacUrl");config["pacScript"]=script;}
  Manager m(root/(inlineScript?L"browser-pac-inline":L"browser-pac-capture"));m.state["Settings"]["DownloadFolder"]=utf8((root/(inlineScript?L"browser-pac-inline-files":L"browser-pac-files")).wstring());m.state["Settings"]["CategoryFolders"]=false;
  m.state["Settings"]["ProxyMode"]="Use a proxy server";m.state["Settings"]["Proxy"]="127.0.0.1:1";m.state["Settings"]["PrefetchFileInfo"]=false;
  auto job=m.receive({{"action","add"},{"url",address},{"filename","browser-pac.bin"},{"downloadLater",true},{"browserProxy",config}});
  check(readBrowserProxy(job->data)==config,"Browser PAC configuration survives capture admission");
  check(readText(m.root/L"state.json").find(inlineScript?script:browserPac.url())==std::string::npos,"Browser PAC configuration is encrypted in the catalog");
  Manager reopened(m.root);check(readBrowserProxy(reopened.jobs[0]->data)==config,"Browser PAC configuration survives desktop restart");
  auto preview=probeDownload(job->data,m.state["Settings"],Cancel{});
  check(str(preview,"Status")=="Ready"&&num(preview,"Size")==origin.payload.size(),"Captured PAC configuration resolves File Info before downloading");
  transfer(m,job,std::make_shared<Cancel>());
  check(str(job->data,"Status")=="Complete"&&readText(job->target())==origin.payload&&(inlineScript||browserPac.served>0),"Captured PAC script overrides unrelated desktop proxy and downloads exact bytes");
  auto copy=m.redownload(job);check(readBrowserProxy(copy->data)==config,"Redownload retains the captured PAC configuration");
 }
 {
  auto routes=resolvePacRoutes(prefs(),Url("http://files.example.test/pub/archive.bin"),live);
  check(routes.size()==2&&str(routes[0],"ProxyMode")=="Use a proxy server"&&str(routes[0],"Proxy")==proxy.address()&&str(routes[0],"ResolvedPacProxyScheme")=="http"&&str(routes[1],"ProxyMode")=="Connect directly"&&str(routes[1],"Proxy").empty(),"A fetched PAC script yields its PROXY and DIRECT alternatives in script order");
  check(pac.served>=1&&str(routes[0],"ProxyUser")=="proxy-user"&&!str(routes[0],"ProxySecret").empty()&&str(routes[1],"ProxyUser").empty()&&!routes[0].contains("ProxyAutoConfigUrl"),"The script is fetched from the configured address and the proxy login applies only to proxy routes");
  auto direct=resolvePacRoutes(prefs(),Url("https://cdn.direct.example.test/file"),live);
  check(direct.size()==1&&str(direct[0],"ProxyMode")=="Connect directly","A host the script sends DIRECT resolves to a single direct route");
  auto plain=prefs();plain["ProxyUser"]="";plain["ProxySecret"]="";
  auto socks=resolvePacRoutes(plain,Url("http://socks.example.test/file"),live);
  check(socks.size()==1&&str(socks[0],"ProxyMode")=="Use a SOCKS4 / 4a proxy"&&str(socks[0],"Proxy")==proxy.address(),"A SOCKS directive from the script becomes a SOCKS route");
  // The configured proxy login carries into script routes, and SOCKS4 takes no password.
  rejects([&]{resolvePacRoutes(prefs(),Url("http://socks.example.test/file"),live);},"A SOCKS directive with a configured proxy password is rejected as SOCKS4 takes none");
  auto missing=prefs();missing["ProxyAutoConfigUrl"]="http://127.0.0.1:"+std::to_string(pac.port)+"/missing.pac";
  rejects([&]{resolvePacRoutes(missing,Url("http://files.example.test/a"),live);},"A script address that returns 404 fails the lookup without a direct fallback");
  auto unreachable=prefs();unreachable["ProxyAutoConfigUrl"]="http://"+closed+"/proxy.pac";
  rejects([&]{resolvePacRoutes(unreachable,Url("http://files.example.test/a"),live);},"An unreachable script server fails the lookup without a direct fallback");
 }
 auto manager=[&](const std::string& label){auto m=std::make_unique<Manager>(root/wide("pac-"+label+"-"+std::to_string(++scenario)));auto settings=m->state["Settings"];settings["DownloadFolder"]=utf8((root/wide("pac-files-"+std::to_string(scenario))).wstring());settings["CategoryFolders"]=false;settings["PrefetchFileInfo"]=false;settings["Connections"]=2;settings["Retries"]=0;auto p=prefs();for(auto it=p.begin();it!=p.end();++it)if(it.key().rfind("Proxy",0)==0)settings[it.key()]=it.value();m->setSettings(settings);return m;};
 auto finish=[&](Manager& m,JobPtr job){for(int i=0;i<1500;++i){m.tick();if(!m.isActive(job)&&(str(job->data,"Status")=="Complete"||str(job->data,"Status")=="Failed"))return;Sleep(10);}throw std::runtime_error("PAC download timed out.");};
 auto expected=[&]{auto p=root/wide("pac-expected.bin");writeBytes(p,Bytes(proxy.payload.begin(),proxy.payload.end()));return fileHash(p);}();
 {
  auto m=manager("proxy");auto job=m->add("http://files.example.test/pub/archive.bin","","via-proxy.bin","Main queue",false);finish(*m,job);
  check(str(job->data,"Status")=="Complete"&&proxy.served>=1&&fileHash(job->target())==expected,"A download routed by the script goes through the selected proxy with the configured login");
 }
 {
  auto served=proxy.served.load();auto m=manager("failover");auto job=m->add("http://failover.example.test/pub/archive.bin","","failover.bin","Main queue",false);finish(*m,job);
  check(str(job->data,"Status")=="Complete"&&proxy.served>served&&fileHash(job->target())==expected,"An unreachable first proxy fails over to the script's next alternative");
 }
 {
  auto served=proxy.served.load();auto m=manager("dead");auto job=m->add("http://dead.example.test/pub/archive.bin","","dead.bin","Main queue",false);finish(*m,job);
  check(str(job->data,"Status")=="Failed"&&proxy.served==served,"A script whose only proxy is unreachable fails the download rather than connecting directly");
 }
}
