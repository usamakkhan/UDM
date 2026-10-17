#pragma once
// Loopback FTP server and HTTP CONNECT proxy, enough of RFC 959 to qualify
// Ftp.cpp: passive (EPSV/PASV) and active (EPRT/PORT) data connections, REST
// resume, SIZE/MDTM metadata, logins, a one-time data cut, and FTP gateway
// conventions (USER user@host, SITE host, OPEN host).
#include "Ftp.hpp"
#include "SiteLogins.hpp"
class FtpFixture {
 SOCKET listener=INVALID_SOCKET;std::thread server;std::vector<std::thread> clients;std::atomic_bool stopping{false};std::mutex logMutex;std::vector<std::string> entries;
 static void sendAll(SOCKET s,const std::string& text){for(size_t at=0;at<text.size();){int n=send(s,text.data()+at,(int)(text.size()-at),0);if(n<=0)return;at+=n;}}
 static SOCKET listen4(sockaddr_in& bound){SOCKET s=socket(AF_INET,SOCK_STREAM,IPPROTO_TCP);bound={};bound.sin_family=AF_INET;bound.sin_addr.s_addr=htonl(INADDR_LOOPBACK);if(bind(s,(sockaddr*)&bound,sizeof(bound))||listen(s,8)){closesocket(s);return INVALID_SOCKET;}int size=sizeof(bound);getsockname(s,(sockaddr*)&bound,&size);return s;}
 static SOCKET acceptOne(SOCKET l){fd_set set;FD_ZERO(&set);FD_SET(l,&set);timeval wait{10,0};if(select(0,&set,nullptr,nullptr,&wait)<=0)return INVALID_SOCKET;return accept(l,nullptr,nullptr);}
 void note(const std::string& line){std::lock_guard<std::mutex> guard(logMutex);entries.push_back(line);}
 void serve(SOCKET s){
  DWORD timeout=10000;setsockopt(s,SOL_SOCKET,SO_RCVTIMEO,(const char*)&timeout,sizeof(timeout));setsockopt(s,SOL_SOCKET,SO_SNDTIMEO,(const char*)&timeout,sizeof(timeout));
  sendAll(s,"220 UDM FTP fixture ready\r\n");std::string pending,currentUser;bool loggedIn=false;long long restOffset=0;SOCKET passive=INVALID_SOCKET;sockaddr_in active{};bool haveActive=false;
  try{char buffer[4096];
   while(!stopping){
    while(pending.find("\r\n")==std::string::npos){int n=recv(s,buffer,sizeof(buffer),0);if(n<=0)throw std::runtime_error("closed");pending.append(buffer,n);if(pending.size()>16384)throw std::runtime_error("oversized");}
    auto end=pending.find("\r\n");auto line=pending.substr(0,end);pending.erase(0,end+2);note(line);
    auto space=line.find(' ');auto verb=line.substr(0,space);auto argument=space==std::string::npos?std::string():line.substr(space+1);for(auto& ch:verb)ch=(char)toupper((unsigned char)ch);
    auto reply=[&](const std::string& text){sendAll(s,text+"\r\n");};
    if(verb=="USER"){currentUser=argument;auto at=argument.find('@');if(gateway&&at!=std::string::npos){++gatewayUserLogins;gatewayTargets.push_back(argument.substr(at+1));currentUser=argument.substr(0,at);}reply("331 Password required");}
    else if(verb=="PASS"){if(requireLogin&&(currentUser!=user||argument!=password)){++rejectedLogins;reply("530 Login incorrect");}else{loggedIn=true;++logins;reply("230 Logged in");}}
    else if(verb=="SITE"){if(!gateway){reply("502 SITE not supported");continue;}++gatewaySiteLogins;gatewayTargets.push_back(argument);loggedIn=false;reply("220 Connected to "+argument);}
    else if(verb=="OPEN"){if(!gateway){reply("502 OPEN not supported");continue;}++gatewayOpenLogins;gatewayTargets.push_back(argument);loggedIn=false;reply("220 Connected to "+argument);}
    else if(!loggedIn)reply("530 Please log in");
    else if(verb=="TYPE")reply("200 Type set");
    else if(verb=="SIZE"){++sizes;if(noSize)reply("502 SIZE not supported");else{auto file=files.find(argument);if(file==files.end())reply("550 No such file");else reply("213 "+std::to_string(file->second.size()));}}
    else if(verb=="MDTM"){if(noMdtm)reply("502 MDTM not supported");else if(!files.count(argument))reply("550 No such file");else reply("213 "+modified);}
    else if(verb=="REST"){if(noRest){reply("502 REST not supported");continue;}restOffset=std::stoll(argument);++rests;if(restOffset)++restResumes;reply("350 Restarting at "+argument);}
    else if(verb=="EPSV"){++epsvs;if(noEpsv){reply("502 EPSV not supported");continue;}sockaddr_in bound;if(passive!=INVALID_SOCKET)closesocket(passive);passive=listen4(bound);reply("229 Entering Extended Passive Mode (|||"+std::to_string(ntohs(bound.sin_port))+"|)");}
    else if(verb=="PASV"){sockaddr_in bound;if(passive!=INVALID_SOCKET)closesocket(passive);passive=listen4(bound);++pasvs;auto p=ntohs(bound.sin_port);reply("227 Entering Passive Mode (127,0,0,1,"+std::to_string(p>>8)+","+std::to_string(p&255)+")");}
    else if(verb=="EPRT"){++eprts;if(noEprt){reply("502 EPRT not supported");continue;}std::smatch m;if(!std::regex_match(argument,m,std::regex("\\|1\\|([0-9.]+)\\|([0-9]+)\\|"))){reply("501 Bad EPRT");continue;}active={};active.sin_family=AF_INET;inet_pton(AF_INET,m[1].str().c_str(),&active.sin_addr);active.sin_port=htons((u_short)std::stoi(m[2]));haveActive=true;reply("200 EPRT ok");}
    else if(verb=="PORT"){std::smatch m;if(!std::regex_match(argument,m,std::regex("([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+)"))){reply("501 Bad PORT");continue;}active={};active.sin_family=AF_INET;auto host=m[1].str()+"."+m[2].str()+"."+m[3].str()+"."+m[4].str();inet_pton(AF_INET,host.c_str(),&active.sin_addr);active.sin_port=htons((u_short)(std::stoi(m[5])*256+std::stoi(m[6])));haveActive=true;++ports;reply("200 PORT ok");}
    else if(verb=="RETR"){
     auto file=files.find(argument);if(file==files.end()){reply("550 No such file");continue;}
     if(passive==INVALID_SOCKET&&!haveActive){reply("425 Use PASV or PORT first");continue;}
     ++retrs;reply("150 Opening data connection");SOCKET data=INVALID_SOCKET;
     if(passive!=INVALID_SOCKET){data=acceptOne(passive);closesocket(passive);passive=INVALID_SOCKET;}
     else{data=socket(AF_INET,SOCK_STREAM,IPPROTO_TCP);if(connect(data,(sockaddr*)&active,sizeof(active))){closesocket(data);data=INVALID_SOCKET;}else ++activeConnections;haveActive=false;}
     if(data==INVALID_SOCKET){reply("425 Cannot open data connection");continue;}
     const auto& bytes=file->second;auto from=(size_t)std::min<long long>(restOffset,(long long)bytes.size());restOffset=0;std::string body=bytes.substr(from);
     bool cut=false;if(cutOnce.exchange(false)){body=body.substr(0,(size_t)std::min<long long>(cutAfterBytes,(long long)body.size()));cut=true;}
     sendAll(data,body);shutdown(data,SD_SEND);closesocket(data);
     if(cut){++cuts;reply("426 Connection closed; transfer aborted");}else reply("226 Transfer complete");
    }
    else if(verb=="QUIT"){reply("221 Goodbye");break;}
    else reply("502 Command not implemented");
   }
  }catch(...){}
  if(passive!=INVALID_SOCKET)closesocket(passive);shutdown(s,SD_BOTH);closesocket(s);
 }
public:
 unsigned short port=0;std::map<std::string,std::string> files;std::string user="fixture-user",password="fixture-password",modified="20260116101500";
 std::atomic_bool noEpsv{false},noEprt{false},noRest{false},noSize{false},noMdtm{false},gateway{false},requireLogin{false},cutOnce{false};std::atomic<long long> cutAfterBytes{0};
 std::atomic_int logins{0},rejectedLogins{0},sizes{0},rests{0},restResumes{0},epsvs{0},pasvs{0},eprts{0},ports{0},retrs{0},activeConnections{0},cuts{0},gatewayUserLogins{0},gatewaySiteLogins{0},gatewayOpenLogins{0};
 std::vector<std::string> gatewayTargets;
 FtpFixture(){
  std::string payload(3*1024*1024+123,'\0');for(size_t i=0;i<payload.size();++i)payload[i]=(char)((i*19+7)%251);files["/pub/archive.bin"]=payload;files["/pub/small.bin"]=payload.substr(0,70000);
  sockaddr_in bound;listener=listen4(bound);if(listener==INVALID_SOCKET)throw std::runtime_error("FTP fixture failed to listen.");port=ntohs(bound.sin_port);
  server=std::thread([this]{while(!stopping){fd_set set;FD_ZERO(&set);FD_SET(listener,&set);timeval wait{0,100000};if(select(0,&set,nullptr,nullptr,&wait)<=0)continue;auto client=accept(listener,nullptr,nullptr);if(client!=INVALID_SOCKET)clients.emplace_back([this,client]{serve(client);});}});
 }
 ~FtpFixture(){stopping=true;if(server.joinable())server.join();closesocket(listener);for(auto& t:clients)if(t.joinable())t.join();}
 std::string url(const std::string& path="/pub/archive.bin")const{return "ftp://127.0.0.1:"+std::to_string(port)+path;}
 std::string history(){std::lock_guard<std::mutex> guard(logMutex);std::string out;for(const auto& e:entries)out+=e+" | ";return out;}
};
// Minimal HTTP CONNECT proxy: tunnels to the requested host and port.
class ConnectProxyFixture {
 SOCKET listener=INVALID_SOCKET;std::thread server;std::vector<std::thread> clients;std::atomic_bool stopping{false};
 static void relay(SOCKET from,SOCKET to){char buffer[16384];for(;;){int n=recv(from,buffer,sizeof(buffer),0);if(n<=0)break;for(int at=0;at<n;){int sent=send(to,buffer+at,n-at,0);if(sent<=0)return;at+=sent;}}shutdown(to,SD_SEND);}
 void serve(SOCKET s){
  DWORD timeout=10000;setsockopt(s,SOL_SOCKET,SO_RCVTIMEO,(const char*)&timeout,sizeof(timeout));std::string request;char buffer[4096];
  while(request.find("\r\n\r\n")==std::string::npos&&request.size()<16384){int n=recv(s,buffer,sizeof(buffer),0);if(n<=0){closesocket(s);return;}request.append(buffer,n);}
  std::smatch m;if(!std::regex_search(request,m,std::regex("^CONNECT ([^:\\s]+):([0-9]+) HTTP/1\\.[01]"))){static const char* bad="HTTP/1.1 400 Bad Request\r\nContent-Length: 0\r\n\r\n";send(s,bad,(int)strlen(bad),0);closesocket(s);return;}
  ++tunnels;sockaddr_in target{};target.sin_family=AF_INET;inet_pton(AF_INET,m[1].str().c_str(),&target.sin_addr);target.sin_port=htons((u_short)std::stoi(m[2]));
  SOCKET upstream=socket(AF_INET,SOCK_STREAM,IPPROTO_TCP);if(connect(upstream,(sockaddr*)&target,sizeof(target))){static const char* fail="HTTP/1.1 502 Bad Gateway\r\nContent-Length: 0\r\n\r\n";send(s,fail,(int)strlen(fail),0);closesocket(upstream);closesocket(s);return;}
  static const char* ok="HTTP/1.1 200 Connection established\r\n\r\n";send(s,ok,(int)strlen(ok),0);timeout=0;setsockopt(s,SOL_SOCKET,SO_RCVTIMEO,(const char*)&timeout,sizeof(timeout));
  std::thread back([=]{relay(upstream,s);});relay(s,upstream);back.join();closesocket(upstream);closesocket(s);
 }
public:
 unsigned short port=0;std::atomic_int tunnels{0};
 ConnectProxyFixture(){
  listener=socket(AF_INET,SOCK_STREAM,IPPROTO_TCP);sockaddr_in bound{};bound.sin_family=AF_INET;bound.sin_addr.s_addr=htonl(INADDR_LOOPBACK);if(bind(listener,(sockaddr*)&bound,sizeof(bound))||listen(listener,8))throw std::runtime_error("CONNECT fixture failed to listen.");int size=sizeof(bound);getsockname(listener,(sockaddr*)&bound,&size);port=ntohs(bound.sin_port);
  server=std::thread([this]{while(!stopping){fd_set set;FD_ZERO(&set);FD_SET(listener,&set);timeval wait{0,100000};if(select(0,&set,nullptr,nullptr,&wait)<=0)continue;auto client=accept(listener,nullptr,nullptr);if(client!=INVALID_SOCKET)clients.emplace_back([this,client]{serve(client);});}});
 }
 ~ConnectProxyFixture(){stopping=true;if(server.joinable())server.join();closesocket(listener);for(auto& t:clients)if(t.joinable())t.join();}
 std::string address()const{return "127.0.0.1:"+std::to_string(port);}
};
static void ftpChecks(const udm::fs::path& root){
 using namespace udm;int scenario=0;
 auto manager=[&](std::function<void(Json&)> configure={}){auto m=std::make_unique<Manager>(root/wide("ftp-"+std::to_string(++scenario)));auto settings=m->state["Settings"];settings["DownloadFolder"]=utf8((root/wide("ftp-files-"+std::to_string(scenario))).wstring());settings["CategoryFolders"]=false;settings["PrefetchFileInfo"]=false;settings["ProxyMode"]="Connect directly";settings["Connections"]=3;settings["Retries"]=2;if(configure)configure(settings);m->setSettings(settings);return m;};
 auto finish=[&](Manager& m,JobPtr job){for(int i=0;i<3000;++i){m.tick();if(!m.isActive(job)&&(str(job->data,"Status")=="Complete"||str(job->data,"Status")=="Failed"))return;Sleep(10);}throw std::runtime_error("FTP download timed out.");};
 auto hashOf=[&](const std::string& bytes){auto p=root/wide("ftp-expected-"+std::to_string(++scenario)+".bin");writeBytes(p,Bytes(bytes.begin(),bytes.end()));return fileHash(p);};
 auto run=[&](FtpFixture& server,Manager& m,const std::string& path,const std::string& name){auto job=m.add(server.url(path),"",name,"Main queue",false);finish(m,job);return job;};
 {
  FtpFixture server;auto m=manager();auto job=run(server,*m,"/pub/archive.bin","passive.bin");
  check(str(job->data,"Status")=="Complete"&&fileHash(job->target())==hashOf(server.files["/pub/archive.bin"]),"Passive FTP download through EPSV publishes the exact file");
  check(server.epsvs>=3&&server.pasvs==0&&server.restResumes>=2&&yes(job->data,"RangeSupported")&&job->data["Segments"].size()==3,"Resumable FTP files are split into segments that restart at their offsets");
  check(str(job->data,"FtpModified")==server.modified&&num(job->data,"Size")==(i64)server.files["/pub/archive.bin"].size(),"FTP metadata is recorded with the download");
 }
 {
  FtpFixture server;server.noEpsv=true;auto m=manager();auto job=run(server,*m,"/pub/archive.bin","pasv.bin");
  check(str(job->data,"Status")=="Complete"&&server.pasvs>=3&&fileHash(job->target())==hashOf(server.files["/pub/archive.bin"]),"PASV is used when the server does not support EPSV");
 }
 {
  FtpFixture server;auto m=manager([](Json& s){s["FtpPassive"]=false;});auto job=run(server,*m,"/pub/archive.bin","active.bin");
  check(str(job->data,"Status")=="Complete"&&server.eprts>=3&&server.activeConnections>=3&&server.epsvs==0&&fileHash(job->target())==hashOf(server.files["/pub/archive.bin"]),"Active FTP through EPRT accepts the server's data connections");
 }
 {
  FtpFixture server;server.noEprt=true;auto m=manager([](Json& s){s["FtpPassive"]=false;});auto job=run(server,*m,"/pub/archive.bin","port.bin");
  check(str(job->data,"Status")=="Complete"&&server.ports>=3&&server.eprts>=1&&fileHash(job->target())==hashOf(server.files["/pub/archive.bin"]),"PORT is used when the server does not support EPRT");
 }
 {
  FtpFixture server;server.noRest=true;auto m=manager();auto job=run(server,*m,"/pub/archive.bin","norest.bin");
  check(str(job->data,"Status")=="Complete"&&!yes(job->data,"RangeSupported")&&job->data["Segments"].size()==1&&fileHash(job->target())==hashOf(server.files["/pub/archive.bin"]),"Servers without REST download in one segment");
 }
 {
  FtpFixture server;server.noSize=true;auto m=manager();auto job=run(server,*m,"/pub/small.bin","nosize.bin");
  check(str(job->data,"Status")=="Complete"&&num(job->data,"Size")==70000&&fileHash(job->target())==hashOf(server.files["/pub/small.bin"]),"Servers without SIZE still complete with the received length");
 }
 {
  FtpFixture server;server.cutOnce=true;server.cutAfterBytes=400000;auto m=manager([](Json& s){s["Connections"]=1;});auto job=run(server,*m,"/pub/archive.bin","resume.bin");
  check(str(job->data,"Status")=="Complete"&&server.cuts==1&&server.restResumes>=1&&server.retrs>=2&&fileHash(job->target())==hashOf(server.files["/pub/archive.bin"]),"A cut data connection resumes from the saved offset and the file stays exact");
 }
 {
  FtpFixture server;server.requireLogin=true;auto m=manager([&](Json& s){s["SiteLogins"]=Json::array({makeSiteLogin(server.url("/"),"fixture-user","fixture-password")});});auto job=run(server,*m,"/pub/small.bin","login.bin");
  check(str(job->data,"Status")=="Complete"&&server.logins>=1&&server.rejectedLogins==0&&fileHash(job->target())==hashOf(server.files["/pub/small.bin"]),"A saved site login is used for FTP");
  auto wrong=manager([&](Json& s){s["SiteLogins"]=Json::array({makeSiteLogin(server.url("/"),"fixture-user","wrong")});});auto denied=run(server,*wrong,"/pub/small.bin","denied.bin");
  check(str(denied->data,"Status")=="Failed"&&str(denied->data,"AuthenticationScheme")=="FTP"&&num(denied->data,"LastFtpStatus")==530&&server.rejectedLogins>=1,"A rejected FTP login asks for a new login instead of retrying");
 }
 {
  FtpFixture server;ConnectProxyFixture proxy;auto m=manager([&](Json& s){s["ProxyMode"]="Use a proxy server";s["Proxy"]=proxy.address();});auto job=run(server,*m,"/pub/archive.bin","connect.bin");
  check(str(job->data,"Status")=="Complete"&&proxy.tunnels>=4&&server.epsvs>=3&&fileHash(job->target())==hashOf(server.files["/pub/archive.bin"]),"FTP through an HTTP CONNECT proxy tunnels control and passive data connections");
  auto active=manager([&](Json& s){s["ProxyMode"]="Use a proxy server";s["Proxy"]=proxy.address();s["FtpPassive"]=false;});auto rejected=run(server,*active,"/pub/small.bin","connect-active.bin");
  check(str(rejected->data,"Status")=="Failed"&&str(rejected->data,"Error").find("passive")!=std::string::npos,"Active FTP through a CONNECT proxy is refused with an explanation");
 }

 // FTP gateways: the proxy is an FTP server that is told the destination at login.
 for(const std::string type:{"USER user@host","SITE host","OPEN host"}){
  FtpFixture gw;gw.gateway=true;auto proxy="127.0.0.1:"+std::to_string(gw.port);
  auto m=manager([&](Json& s){s["ProxyMode"]="Use a proxy server";s["Proxy"]=proxy;s["FtpProxyType"]=type;});
  auto job=m->add("ftp://files.example.test/pub/archive.bin","","gateway.bin","Main queue",false);finish(*m,job);
  const int used=type=="USER user@host"?gw.gatewayUserLogins.load():type=="SITE host"?gw.gatewaySiteLogins.load():gw.gatewayOpenLogins.load();
  check(str(job->data,"Status")=="Complete"&&used>=1&&std::find(gw.gatewayTargets.begin(),gw.gatewayTargets.end(),"files.example.test")!=gw.gatewayTargets.end()&&fileHash(job->target())==hashOf(gw.files["/pub/archive.bin"]),"FTP gateway type "+type+" names the destination at login and downloads through the gateway");
  auto active=manager([&](Json& s){s["ProxyMode"]="Use a proxy server";s["Proxy"]=proxy;s["FtpProxyType"]=type;s["FtpPassive"]=false;});
  auto activeJob=active->add("ftp://files.example.test/pub/small.bin","","gateway-active.bin","Main queue",false);finish(*active,activeJob);
  check(str(activeJob->data,"Status")=="Complete"&&gw.activeConnections>=1&&fileHash(activeJob->target())==hashOf(gw.files["/pub/small.bin"]),"Active FTP works through gateway type "+type);
 }
 {
  FtpFixture gw;gw.gateway=true;gw.requireLogin=true;auto proxy="127.0.0.1:"+std::to_string(gw.port);
  auto m=manager([&](Json& s){s["ProxyMode"]="Use a proxy server";s["Proxy"]=proxy;s["ProxyUser"]="fixture-user";s["ProxySecret"]=protect("fixture-password");s["FtpProxyType"]="SITE host";s["SiteLogins"]=Json::array({makeSiteLogin("ftp://files.example.test/","fixture-user","fixture-password")});});
  auto job=m->add("ftp://files.example.test/pub/small.bin","","gateway-login.bin","Main queue",false);finish(*m,job);
  check(str(job->data,"Status")=="Complete"&&gw.logins>=2&&gw.gatewaySiteLogins>=1,"A gateway login precedes SITE and the destination login follows it");
  rejects([&]{auto bad=manager([&](Json& s){s["FtpProxyType"]="TUNNEL";});},"Unknown FTP proxy types are rejected");
 }
}
