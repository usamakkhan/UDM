#pragma once
// Loopback HTTP forward proxy that demands a login (Basic, Digest or Negotiate
// through the Windows security API) before serving any absolute-URI request
// itself, so proxy login can be checked without an external proxy.
#include "WindowsLoginChecks.hpp"
#include "AuthenticationChecks.hpp"
class AuthProxyFixture {
 SOCKET listener=INVALID_SOCKET;std::thread server;std::vector<std::thread> clients;std::atomic_bool stopping{false};CredHandle credentials{};bool haveCredentials=false;
 static void sendAll(SOCKET s,const std::string& text){for(size_t at=0;at<text.size();){int n=send(s,text.data()+at,(int)(text.size()-at),0);if(n<=0)return;at+=n;}}
 void serve(SOCKET s){
  CtxtHandle context{};bool haveContext=false;DWORD timeout=5000;setsockopt(s,SOL_SOCKET,SO_RCVTIMEO,(const char*)&timeout,sizeof(timeout));setsockopt(s,SOL_SOCKET,SO_SNDTIMEO,(const char*)&timeout,sizeof(timeout));
  try{std::string pending;char buffer[8192];
   while(!stopping){
    while(pending.find("\r\n\r\n")==std::string::npos){int n=recv(s,buffer,sizeof(buffer),0);if(n<=0)throw std::runtime_error("closed");pending.append(buffer,n);if(pending.size()>65536)throw std::runtime_error("oversized");}
    auto end=pending.find("\r\n\r\n");auto request=pending.substr(0,end+4);pending.erase(0,end+4);++requests;
    std::smatch line;if(!std::regex_search(request,line,std::regex("^([A-Z]+) ([^ ]+) HTTP/1\\.[01]")))throw std::runtime_error("bad request");
    const bool head=line[1]=="HEAD";std::string target=line[2];
    auto respond=[&](const std::string& code,const std::string& headers,const std::string& body){sendAll(s,"HTTP/1.1 "+code+"\r\n"+headers+"Content-Length: "+std::to_string(body.size())+"\r\nProxy-Connection: keep-alive\r\nConnection: keep-alive\r\n\r\n"+(head?std::string():body));};
    std::smatch auth;std::string scheme,value;if(std::regex_search(request,auth,std::regex("Proxy-Authorization: *([A-Za-z]+) +([^\\r\\n]*)",std::regex::icase))){scheme=auth[1];value=auth[2];}
    auto challenge=[&]{++challenges;std::string header;if(mode=="basic")header="Proxy-Authenticate: Basic realm=\"udm-proxy\"\r\n";else if(mode=="digest")header="Proxy-Authenticate: Digest realm=\"udm-proxy\", nonce=\"proxy-nonce\", algorithm=MD5, qop=\"auth\"\r\n";else header="Proxy-Authenticate: Negotiate\r\nProxy-Authenticate: NTLM\r\n";respond("407 Proxy Authentication Required",header,"");};
    bool ok=false;
    if(scheme.empty()){challenge();continue;}
    if(mode=="basic")ok=lower(scheme)=="basic"&&value==udm::b64(udm::Bytes(login.begin(),login.end()));
    else if(mode=="digest"&&lower(scheme)=="digest"){std::map<std::string,std::string> fields;std::regex pairs("([a-zA-Z0-9_-]+)=(?:\"([^\"]*)\"|([^, ]+))");for(auto it=std::sregex_iterator(value.begin(),value.end(),pairs);it!=std::sregex_iterator();++it)fields[(*it)[1]]=(*it)[2].matched?(*it)[2].str():(*it)[3].str();
     auto colon=login.find(':');auto ha1=authMd5(login.substr(0,colon)+":udm-proxy:"+login.substr(colon+1)),ha2=authMd5(line[1].str()+":"+fields["uri"]);auto expected=authMd5(ha1+":proxy-nonce:"+fields["nc"]+":"+fields["cnonce"]+":auth:"+ha2);ok=fields["username"]==login.substr(0,colon)&&fields["response"]==expected;if(ok)++digests;}
    else if(mode=="negotiate"&&(lower(scheme)=="negotiate"||lower(scheme)=="ntlm")){
     auto incoming=udm::unb64(value);SecBuffer in{(unsigned long)incoming.size(),SECBUFFER_TOKEN,incoming.data()},out{0,SECBUFFER_TOKEN,nullptr};SecBufferDesc inDesc{SECBUFFER_VERSION,1,&in},outDesc{SECBUFFER_VERSION,1,&out};ULONG attributes=0;TimeStamp expiry{};CtxtHandle next{};
     auto status=AcceptSecurityContext(&credentials,haveContext?&context:nullptr,&inDesc,ASC_REQ_CONNECTION|ASC_REQ_ALLOCATE_MEMORY,SECURITY_NATIVE_DREP,&next,&outDesc,&attributes,&expiry);
     std::string reply;if(out.pvBuffer){reply=udm::b64(udm::Bytes((unsigned char*)out.pvBuffer,(unsigned char*)out.pvBuffer+out.cbBuffer));FreeContextBuffer(out.pvBuffer);}
     if(status==SEC_I_CONTINUE_NEEDED||status==SEC_I_COMPLETE_AND_CONTINUE){context=next;haveContext=true;++challenges;respond("407 Proxy Authentication Required","Proxy-Authenticate: "+scheme+" "+reply+"\r\n","");continue;}
     if(haveContext)DeleteSecurityContext(&context);haveContext=false;if(status==SEC_E_OK){DeleteSecurityContext(&next);ok=true;++accepted;}
    }
    if(!ok){++rejected;challenge();continue;}
    std::smatch match;bool ranged=std::regex_search(request,match,std::regex("Range: bytes=([0-9]+)-([0-9]*)",std::regex::icase));size_t from=ranged?std::stoull(match[1]):0,to=ranged&&!match[2].str().empty()?std::stoull(match[2]):payload.size()-1;to=std::min(to,payload.size()-1);
    std::string extra="ETag: \"proxy-fixture\"\r\nAccept-Ranges: bytes\r\nContent-Type: application/octet-stream\r\n";if(ranged)extra+="Content-Range: bytes "+std::to_string(from)+"-"+std::to_string(to)+"/"+std::to_string(payload.size())+"\r\n";
    ++served;respond(ranged?"206 Partial Content":"200 OK",extra,payload.substr(from,to-from+1));
   }
  }catch(...){}
  if(haveContext)DeleteSecurityContext(&context);shutdown(s,SD_BOTH);closesocket(s);
 }
public:
 unsigned short port=0;std::string mode,login="proxy-user:proxy-secret",payload;std::atomic_int requests{0},challenges{0},digests{0},accepted{0},rejected{0},served{0};
 explicit AuthProxyFixture(std::string authMode):mode(std::move(authMode)){
  if(mode=="negotiate"){TimeStamp expiry{};if(AcquireCredentialsHandleW(nullptr,const_cast<wchar_t*>(L"Negotiate"),SECPKG_CRED_INBOUND,nullptr,nullptr,nullptr,nullptr,&credentials,&expiry)!=SEC_E_OK)throw std::runtime_error("Proxy fixture could not acquire server credentials.");haveCredentials=true;}
  payload.resize(2*65536+99);for(size_t i=0;i<payload.size();++i)payload[i]=(char)((i*23+11)%251);
  listener=socket(AF_INET,SOCK_STREAM,IPPROTO_TCP);sockaddr_in address{};address.sin_family=AF_INET;address.sin_addr.s_addr=htonl(INADDR_LOOPBACK);
  if(bind(listener,(sockaddr*)&address,sizeof(address))||listen(listener,16))throw std::runtime_error("Proxy fixture failed to listen.");int size=sizeof(address);getsockname(listener,(sockaddr*)&address,&size);port=ntohs(address.sin_port);
  server=std::thread([this]{while(!stopping){fd_set set;FD_ZERO(&set);FD_SET(listener,&set);timeval wait{0,100000};if(select(0,&set,nullptr,nullptr,&wait)<=0)continue;auto client=accept(listener,nullptr,nullptr);if(client!=INVALID_SOCKET)clients.emplace_back([this,client]{serve(client);});}});
 }
 ~AuthProxyFixture(){stopping=true;if(server.joinable())server.join();closesocket(listener);for(auto& t:clients)if(t.joinable())t.join();if(haveCredentials)FreeCredentialsHandle(&credentials);}
 std::string address()const{return "127.0.0.1:"+std::to_string(port);}
};
static void proxyLoginChecks(const udm::fs::path& root){
 using namespace udm;int scenario=0;
 auto manager=[&](const AuthProxyFixture& proxy,const std::string& user,const std::string& secret){auto m=std::make_unique<Manager>(root/wide("proxy-login-"+std::to_string(++scenario)));auto settings=m->state["Settings"];settings["DownloadFolder"]=utf8((root/wide("proxy-login-files-"+std::to_string(scenario))).wstring());settings["CategoryFolders"]=false;settings["PrefetchFileInfo"]=false;settings["Connections"]=2;settings["Retries"]=0;settings["ProxyMode"]="Use a proxy server";settings["Proxy"]=proxy.address();settings["ProxyUser"]=user;settings["ProxySecret"]=secret.empty()?"":protect(secret);m->setSettings(settings);return m;};
 auto finish=[&](Manager& m,JobPtr job){for(int i=0;i<1500;++i){m.tick();if(!m.isActive(job)&&(str(job->data,"Status")=="Complete"||str(job->data,"Status")=="Failed"))return;Sleep(10);}throw std::runtime_error("Proxy login download timed out.");};
 auto expected=[&](const AuthProxyFixture& proxy){auto p=root/wide("proxy-expected-"+std::to_string(++scenario)+".bin");writeBytes(p,Bytes(proxy.payload.begin(),proxy.payload.end()));return fileHash(p);};
 const std::string origin="http://files.example.test/pub/archive.bin";
 {
  AuthProxyFixture proxy("basic");auto m=manager(proxy,"proxy-user","proxy-secret");auto job=m->add(origin,"","basic.bin","Main queue",false);finish(*m,job);
  check(str(job->data,"Status")=="Complete"&&proxy.served>=1&&proxy.rejected==0&&fileHash(job->target())==expected(proxy),"A Basic proxy login downloads through the proxy");
  auto wrong=manager(proxy,"proxy-user","wrong");auto denied=wrong->add(origin,"","basic-wrong.bin","Main queue",false);finish(*wrong,denied);
  check(str(denied->data,"Status")=="Failed"&&num(denied->data,"LastHttpStatus")==407&&proxy.rejected>=1,"A rejected proxy login reports the 407 instead of retrying");
 }
 {
  AuthProxyFixture proxy("digest");auto m=manager(proxy,"proxy-user","proxy-secret");auto job=m->add(origin,"","digest.bin","Main queue",false);finish(*m,job);
  check(str(job->data,"Status")=="Complete"&&proxy.digests>=1&&proxy.challenges>=1&&fileHash(job->target())==expected(proxy),"A Digest proxy challenge is answered with the configured login");
 }
 {
  AuthProxyFixture proxy("negotiate");auto m=manager(proxy,"proxy-user","proxy-secret");auto job=m->add(origin,"","negotiate.bin","Main queue",false);finish(*m,job);
  std::cout<<"Proxy login (negotiate, configured login): "<<Json{{"status",str(job->data,"Status")},{"error",str(job->data,"Error")},{"accepted",proxy.accepted.load()},{"challenges",proxy.challenges.load()}}.dump()<<std::endl;
  // A configured login answers NTLM only when the vendored curl carries SSPI.
  if(proxyAuthSchemes()&CURLAUTH_NTLM)check(str(job->data,"Status")=="Complete"&&proxy.accepted>=1&&fileHash(job->target())==expected(proxy),"A Negotiate/NTLM proxy challenge completes the Windows login handshake with the configured login");
  else check(str(job->data,"Status")=="Failed"&&num(job->data,"LastHttpStatus")==407&&proxy.accepted==0,"Without SSPI in the curl build, a configured login cannot answer a Negotiate/NTLM proxy and the 407 is reported");
 }
 {
  AuthProxyFixture proxy("negotiate");auto m=manager(proxy,"","");auto job=m->add(origin,"","negotiate-user.bin","Main queue",false);finish(*m,job);
  std::cout<<"Proxy login (negotiate, current user): "<<Json{{"status",str(job->data,"Status")},{"error",str(job->data,"Error")},{"accepted",proxy.accepted.load()},{"challenges",proxy.challenges.load()},{"requests",proxy.requests.load()}}.dump()<<std::endl;
  check(str(job->data,"Status")=="Complete"&&proxy.accepted>=1&&fileHash(job->target())==expected(proxy),"An intranet proxy that asks for Windows login receives the current user's login when no proxy login is configured");
 }
}
