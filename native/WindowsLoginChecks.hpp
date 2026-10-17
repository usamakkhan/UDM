#pragma once
// Loopback server that requires Windows login (Negotiate/NTLM) through the
// Windows security API, so the current-user and saved-login paths can be
// checked without a domain or an external server.
#define SECURITY_WIN32
#include <security.h>
#include "SiteLogins.hpp"
class WindowsLoginFixture {
 SOCKET listener=INVALID_SOCKET;std::thread server;std::vector<std::thread> clients;std::atomic_bool stopping{false};
 CredHandle credentials{};bool haveCredentials=false;
 static void sendAll(SOCKET socket,const std::string& value){for(size_t at=0;at<value.size();){auto n=send(socket,value.data()+at,(int)(value.size()-at),0);if(n<=0)break;at+=n;}}
 void serve(SOCKET socket){
  CtxtHandle context{};bool haveContext=false;DWORD timeout=5000;setsockopt(socket,SOL_SOCKET,SO_RCVTIMEO,(const char*)&timeout,sizeof(timeout));
  try{std::string pending;char buffer[8192];
   while(!stopping){
    while(pending.find("\r\n\r\n")==std::string::npos){int n=recv(socket,buffer,sizeof(buffer),0);if(n<=0)throw std::runtime_error("closed");pending.append(buffer,n);if(pending.size()>65536)throw std::runtime_error("oversized");}
    auto end=pending.find("\r\n\r\n");auto request=pending.substr(0,end+4);pending.erase(0,end+4);++requests;
    auto begin=request.find(' '),stop=request.find(' ',begin+1);auto path=request.substr(begin+1,stop-begin-1);const bool head=request.rfind("HEAD ",0)==0;
    std::smatch match;std::string scheme,token;
    if(std::regex_search(request,match,std::regex("Authorization: *(Negotiate|NTLM) +([A-Za-z0-9+/=]+)",std::regex::icase))){scheme=match[1];token=match[2];}
    auto respond=[&](const std::string& code,const std::string& headers,const std::string& body){sendAll(socket,"HTTP/1.1 "+code+"\r\n"+headers+"Content-Length: "+std::to_string(body.size())+"\r\nConnection: keep-alive\r\n\r\n"+(head?std::string():body));};
    if(token.empty()){++challenges;respond("401 Unauthorized","WWW-Authenticate: Negotiate\r\nWWW-Authenticate: NTLM\r\n","");continue;}
    auto incoming=udm::unb64(token);SecBuffer in{(unsigned long)incoming.size(),SECBUFFER_TOKEN,incoming.data()},out{0,SECBUFFER_TOKEN,nullptr};
    SecBufferDesc inDesc{SECBUFFER_VERSION,1,&in},outDesc{SECBUFFER_VERSION,1,&out};ULONG attributes=0;TimeStamp expiry{};CtxtHandle next{};
    auto status=AcceptSecurityContext(&credentials,haveContext?&context:nullptr,&inDesc,ASC_REQ_CONNECTION|ASC_REQ_ALLOCATE_MEMORY,SECURITY_NATIVE_DREP,&next,&outDesc,&attributes,&expiry);
    std::string reply;if(out.pvBuffer){reply=udm::b64(udm::Bytes((unsigned char*)out.pvBuffer,(unsigned char*)out.pvBuffer+out.cbBuffer));FreeContextBuffer(out.pvBuffer);}
    if(status==SEC_I_CONTINUE_NEEDED||status==SEC_I_COMPLETE_AND_CONTINUE){context=next;haveContext=true;respond("401 Unauthorized","WWW-Authenticate: "+scheme+" "+reply+"\r\n","");continue;}
    if(haveContext)DeleteSecurityContext(&context);haveContext=false;
    if(status!=SEC_E_OK){++rejected;respond("401 Unauthorized","WWW-Authenticate: Negotiate\r\nWWW-Authenticate: NTLM\r\n","");continue;}
    DeleteSecurityContext(&next);++accepted;
    bool ranged=std::regex_search(request,match,std::regex("Range: bytes=([0-9]+)-([0-9]*)",std::regex::icase));size_t from=ranged?std::stoull(match[1]):0,to=ranged&&!match[2].str().empty()?std::stoull(match[2]):payload.size()-1;to=std::min(to,payload.size()-1);
    std::string extra="ETag: \"windows-login-fixture\"\r\nAccept-Ranges: bytes\r\n";if(ranged)extra+="Content-Range: bytes "+std::to_string(from)+"-"+std::to_string(to)+"/"+std::to_string(payload.size())+"\r\n";
    respond(ranged?"206 Partial Content":"200 OK",extra,payload.substr(from,to-from+1));
   }
  }catch(...){}
  if(haveContext)DeleteSecurityContext(&context);shutdown(socket,SD_BOTH);closesocket(socket);
 }
public:
 unsigned short port=0;std::string payload;std::atomic_int requests{0},challenges{0},accepted{0},rejected{0};
 WindowsLoginFixture(){
  TimeStamp expiry{};if(AcquireCredentialsHandleW(nullptr,const_cast<wchar_t*>(L"Negotiate"),SECPKG_CRED_INBOUND,nullptr,nullptr,nullptr,nullptr,&credentials,&expiry)!=SEC_E_OK)throw std::runtime_error("Windows login fixture could not acquire server credentials.");haveCredentials=true;
  payload.resize(3*65536+17);for(size_t i=0;i<payload.size();++i)payload[i]=(char)((i*13+3)%251);
  listener=socket(AF_INET,SOCK_STREAM,IPPROTO_TCP);sockaddr_in address{};address.sin_family=AF_INET;address.sin_addr.s_addr=htonl(INADDR_LOOPBACK);
  if(bind(listener,(sockaddr*)&address,sizeof(address))||listen(listener,16))throw std::runtime_error("Windows login fixture failed to listen.");int size=sizeof(address);getsockname(listener,(sockaddr*)&address,&size);port=ntohs(address.sin_port);
  server=std::thread([this]{while(!stopping){fd_set set;FD_ZERO(&set);FD_SET(listener,&set);timeval wait{0,100000};if(select(0,&set,nullptr,nullptr,&wait)<=0)continue;auto client=accept(listener,nullptr,nullptr);if(client!=INVALID_SOCKET)clients.emplace_back([this,client]{serve(client);});}});
 }
 ~WindowsLoginFixture(){stopping=true;if(server.joinable())server.join();closesocket(listener);for(auto& t:clients)if(t.joinable())t.join();if(haveCredentials)FreeCredentialsHandle(&credentials);}
 // "localhost" has no dots, so WinHTTP treats it as an intranet host.
 std::string url(const std::string& path="/file.bin")const{return "http://localhost:"+std::to_string(port)+path;}
};
static void windowsLoginChecks(const udm::fs::path& root){
 using namespace udm;WindowsLoginFixture server;
 Manager m(root/L"windows-login");auto settings=m.state["Settings"];settings["DownloadFolder"]=utf8((root/L"windows-login-files").wstring());settings["CategoryFolders"]=false;settings["PrefetchFileInfo"]=false;settings["ProxyMode"]="Connect directly";settings["Connections"]=2;settings["Retries"]=0;m.setSettings(settings);
 auto finish=[&](JobPtr job){for(int i=0;i<1500;++i){m.tick();if(!m.isActive(job)&&(str(job->data,"Status")=="Complete"||str(job->data,"Status")=="Failed"))return;Sleep(10);}throw std::runtime_error("Windows login download timed out.");};
 auto expected=root/L"windows-login-expected.bin";writeBytes(expected,Bytes(server.payload.begin(),server.payload.end()));
 auto job=m.add(server.url(),"","intranet.bin","Main queue",false);finish(job);
 check(str(job->data,"Status")=="Complete"&&fileHash(job->target())==fileHash(expected)&&server.accepted>=1&&server.challenges>=1,"An intranet server that requires Windows login receives the current user's login and the file arrives intact");
 settings["WindowsLoginIntranet"]=false;m.setSettings(settings);
 auto denied=m.add(server.url("/second.bin"),"","second.bin","Main queue",false);finish(denied);
 check(str(denied->data,"Status")=="Failed"&&str(denied->data,"AuthenticationScheme")=="Negotiate"&&str(denied->data,"AuthenticationOrigin")=="http://localhost:"+std::to_string(server.port),"With the Windows login option off, the challenge is reported for a manual login");
 settings["WindowsLoginIntranet"]=true;settings["SiteLogins"]=Json::array({makeSiteLogin(server.url("/"),"fixture-wrong-user","fixture-wrong-password")});m.setSettings(settings);
 const auto rejectedBefore=server.rejected.load();auto wrong=m.add(server.url("/third.bin"),"","third.bin","Main queue",false);finish(wrong);
 check(str(wrong->data,"Status")=="Failed"&&str(wrong->data,"AuthenticationScheme")=="Negotiate"&&server.rejected>rejectedBefore,"A saved login that the server rejects is reported with the Windows scheme instead of falling back to the current user");
}
