#include "SocksProxy.hpp"
#include <ws2tcpip.h>
#include <sstream>
#include <algorithm>
namespace udm {
namespace {
struct Endpoint { std::string host; unsigned short port; };
Endpoint endpoint(const std::string& input) {
 auto text=trim(input); std::string host,port;
 if(text.empty()||text.find_first_of("/\\@?#;= \t\r\n")!=std::string::npos)throw std::runtime_error("Enter a SOCKS5 host:port, or [IPv6]:port.");
 if(text.front()=='['){auto end=text.find(']');if(end==std::string::npos||end+1>=text.size()||text[end+1]!=':')throw std::runtime_error("Use [IPv6]:port for a SOCKS5 address.");host=text.substr(1,end-1);port=text.substr(end+2);IN6_ADDR ip{};if(InetPtonA(AF_INET6,host.c_str(),&ip)!=1)throw std::runtime_error("Invalid IPv6 proxy address.");}
 else {auto at=text.rfind(':');if(at==std::string::npos||text.find(':')!=at)throw std::runtime_error("Enter the SOCKS5 port, normally 1080.");host=text.substr(0,at);port=text.substr(at+1);}
 if(host.empty()||host.size()>255||port.empty()||port.size()>5||port.find_first_not_of("0123456789")!=std::string::npos)throw std::runtime_error("Invalid SOCKS5 host or port.");
 auto value=std::stoi(port);if(value<1||value>65535)throw std::runtime_error("Proxy port must be 1-65535.");return {lower(host),(unsigned short)value};
}
std::string hostKey(std::string host,unsigned short port){if(host.size()>1&&host.front()=='['&&host.back()==']')host=host.substr(1,host.size()-2);return lower(host)+":"+std::to_string(port);}
}
bool socksBypass(const Url& url,const std::string& list){std::istringstream input(lower(list));std::string rule,host=lower(url.host);if(host.size()>1&&host.front()=='['&&host.back()==']')host=host.substr(1,host.size()-2);auto match=[](const std::string& text,const std::string& pattern){size_t i=0,j=0,star=std::string::npos,retry=0;while(i<text.size()){if(j<pattern.size()&&(pattern[j]=='?'||pattern[j]==text[i])){++i;++j;}else if(j<pattern.size()&&pattern[j]=='*'){star=j++;retry=i;}else if(star!=std::string::npos){j=star+1;i=++retry;}else return false;}while(j<pattern.size()&&pattern[j]=='*')++j;return j==pattern.size();};while(std::getline(input,rule,';')){rule=trim(rule);if(rule.empty())continue;if(rule=="<local>"&&host.find('.')==std::string::npos)return true;if(match(host,rule)||match(lower(url.host),rule))return true;}return false;}
void validateSocksDestination(const Url& url,const Json& prefs){if(str(prefs,"ProxyMode")!="Use a SOCKS5 proxy"||socksBypass(url,str(prefs,"ProxyBypass")))return;auto host=lower(url.host);IN_ADDR v4{};IN6_ADDR v6{};if(host.size()>1&&host.front()=='['&&host.back()==']')host=host.substr(1,host.size()-2);bool loopback=host=="localhost"||host=="loopback"||hostIs(host,"localhost");if(InetPtonA(AF_INET,host.c_str(),&v4)==1)loopback|=(ntohl(v4.s_addr)>>24)==127;auto legacy=inet_addr(host.c_str());if(legacy!=INADDR_NONE)loopback|=(ntohl(legacy)>>24)==127;if(InetPtonA(AF_INET6,host.c_str(),&v6)==1){static const IN6_ADDR local=IN6ADDR_LOOPBACK_INIT;loopback|=memcmp(&v6,&local,sizeof(v6))==0;loopback|=IN6_IS_ADDR_V4MAPPED(&v6)&&v6.u.Byte[12]==127;}if(loopback)throw std::runtime_error("Windows bypasses the proxy for this loopback URL. Use the server's DNS name, or explicitly add this host to Options > Proxy > Bypass to download it directly.");}
void validateSocksSettings(const Json& settings) {
 endpoint(str(settings,"Proxy"));auto user=str(settings,"ProxyUser"),secret=reveal(str(settings,"ProxySecret"));
 if(user.size()>255||secret.size()>255||user.empty()!=secret.empty())throw std::runtime_error("SOCKS5 authentication needs both a user name and password, each at most 255 UTF-8 bytes, or both blank.");
}
struct SocksProxy::Impl {
 WSADATA wsa{};bool started=false;SOCKET listener=INVALID_SOCKET;std::atomic_bool stop{false};std::thread acceptor;
 std::mutex mutex;std::set<SOCKET> sockets;std::set<std::string> allowed;
 struct Client {std::thread thread;std::shared_ptr<std::atomic_bool> done;};std::vector<Client> clients;
 std::vector<std::pair<sockaddr_storage,int>> addresses;std::string user,password,token;unsigned short port=0;
 struct Socket {Impl& owner;SOCKET value;Socket(Impl& o,SOCKET s):owner(o),value(s){std::lock_guard<std::mutex> lock(owner.mutex);owner.sockets.insert(s);}~Socket(){std::lock_guard<std::mutex> lock(owner.mutex);owner.sockets.erase(value);closesocket(value);}Socket(const Socket&)=delete;};
 void ready(SOCKET s,bool write,ULONGLONG until){for(;;){if(stop)throw Cancelled();if(GetTickCount64()>until)throw std::runtime_error("SOCKS5 connection timed out.");fd_set set;FD_ZERO(&set);FD_SET(s,&set);timeval delay{0,50000};int n=select(0,write?nullptr:&set,write?&set:nullptr,nullptr,&delay);if(n>0)return;if(n<0)throw std::runtime_error("SOCKS5 socket failed.");}}
 void sendBytes(SOCKET s,const void* raw,size_t n,ULONGLONG until){auto data=(const char*)raw;while(n){ready(s,true,until);int sent=::send(s,data,(int)std::min<size_t>(n,65536),0);if(sent==SOCKET_ERROR&&WSAGetLastError()==WSAEWOULDBLOCK)continue;if(sent<=0)throw std::runtime_error("SOCKS5 send failed.");data+=sent;n-=sent;}}
 void sendBytes(SOCKET s,const Bytes& data,ULONGLONG until){sendBytes(s,data.data(),data.size(),until);}
 void sendText(SOCKET s,const std::string& data,ULONGLONG until){sendBytes(s,data.data(),data.size(),until);}
 void receive(SOCKET s,void* raw,size_t n,ULONGLONG until){auto data=(char*)raw;while(n){ready(s,false,until);int got=recv(s,data,(int)n,0);if(got==SOCKET_ERROR&&WSAGetLastError()==WSAEWOULDBLOCK)continue;if(got<=0)throw std::runtime_error("SOCKS5 connection closed.");data+=got;n-=got;}}
 void nonblocking(SOCKET s){u_long value=1;if(ioctlsocket(s,FIONBIO,&value))throw std::runtime_error("Cannot initialize SOCKS5 socket.");}
 void handshake(SOCKET s,const Endpoint& target,ULONGLONG until){
  unsigned char method=user.empty()?0:2;sendBytes(s,Bytes{5,1,method},until);unsigned char reply[4]{};receive(s,reply,2,until);
  if(reply[0]!=5||reply[1]!=method)throw std::runtime_error("SOCKS5 authentication method rejected.");
  if(method==2){Bytes login{1,(unsigned char)user.size()};login.insert(login.end(),user.begin(),user.end());login.push_back((unsigned char)password.size());login.insert(login.end(),password.begin(),password.end());sendBytes(s,login,until);SecureZeroMemory(login.data(),login.size());receive(s,reply,2,until);if(reply[0]!=1||reply[1]!=0)throw std::runtime_error("SOCKS5 authentication failed.");}
  std::string host=target.host;if(host.size()>1&&host.front()=='['&&host.back()==']')host=host.substr(1,host.size()-2);
  Bytes request{5,1,0};IN_ADDR v4{};IN6_ADDR v6{};
  if(InetPtonA(AF_INET,host.c_str(),&v4)==1){request.push_back(1);auto p=(unsigned char*)&v4;request.insert(request.end(),p,p+4);}
  else if(InetPtonA(AF_INET6,host.c_str(),&v6)==1){request.push_back(4);auto p=(unsigned char*)&v6;request.insert(request.end(),p,p+16);}
  else {if(host.empty()||host.size()>255)throw std::runtime_error("SOCKS5 destination host is too long.");request.push_back(3);request.push_back((unsigned char)host.size());request.insert(request.end(),host.begin(),host.end());}
  request.push_back((unsigned char)(target.port>>8));request.push_back((unsigned char)target.port);sendBytes(s,request,until);receive(s,reply,4,until);
  if(reply[0]!=5||reply[1]!=0||reply[2]!=0)throw std::runtime_error("SOCKS5 server rejected the connection.");
  size_t remaining=reply[3]==1?4:reply[3]==4?16:0;if(reply[3]==3){unsigned char count=0;receive(s,&count,1,until);remaining=count;}else if(!remaining)throw std::runtime_error("Invalid SOCKS5 reply.");Bytes bound(remaining+2);receive(s,bound.data(),bound.size(),until);
 }
 void relay(SOCKET a,SOCKET b){bool ar=true,br=true;char buffer[65536];while((ar||br)&&!stop){fd_set read;FD_ZERO(&read);if(ar)FD_SET(a,&read);if(br)FD_SET(b,&read);timeval delay{0,50000};int n=select(0,&read,nullptr,nullptr,&delay);if(n<0)return;if(!n)continue;for(int i=0;i<2;++i){auto source=i?b:a,destination=i?a:b;bool& open=i?br:ar;if(!open||!FD_ISSET(source,&read))continue;int count=recv(source,buffer,sizeof(buffer),0);if(count==SOCKET_ERROR&&WSAGetLastError()==WSAEWOULDBLOCK)continue;if(count<=0){open=false;shutdown(destination,SD_SEND);continue;}sendBytes(destination,buffer,(size_t)count,GetTickCount64()+30000);}}}
 void serve(SOCKET accepted){Socket client(*this,accepted);bool tunneled=false;try{
  nonblocking(accepted);auto until=GetTickCount64()+15000;std::string header;char ch=0;
  while(header.size()<32768&& (header.size()<4||header.compare(header.size()-4,4,"\r\n\r\n"))){receive(accepted,&ch,1,until);header+=ch;}if(header.size()>=32768)throw std::runtime_error("Proxy request header too large.");
  auto first=header.find("\r\n");std::istringstream line(header.substr(0,first));std::string method,address,version;line>>method>>address>>version;
  bool connect=method=="CONNECT";if(!connect&&method!="GET"&&method!="POST")throw std::runtime_error("Unsupported proxy request.");
  Endpoint target=connect?endpoint(address):Endpoint{Url(address).host,Url(address).port};if(!connect&&Url(address).scheme!="http")throw std::runtime_error("Invalid proxy request scheme.");
  bool authenticated=false;std::string outgoing;size_t at=first+2;while(at<header.size()-2){auto end=header.find("\r\n",at);auto entry=header.substr(at,end-at);auto colon=entry.find(':');if(colon==std::string::npos)throw std::runtime_error("Invalid proxy header.");auto key=lower(trim(entry.substr(0,colon))),value=trim(entry.substr(colon+1));if(key=="proxy-authorization")authenticated=value==token;else if(key!="proxy-connection"&&key!="connection"&&key!="host")outgoing+=entry+"\r\n";at=end+2;}
  if(!authenticated){sendText(accepted,"HTTP/1.1 407 Proxy Authentication Required\r\nProxy-Authenticate: Basic realm=\"UDM\"\r\nContent-Length: 0\r\nConnection: close\r\n\r\n",until);return;}
  {std::lock_guard<std::mutex> lock(mutex);if(!allowed.count(hostKey(target.host,target.port)))throw std::runtime_error("Destination is not part of this download session.");}
  std::unique_ptr<Socket> remote;for(const auto& value:addresses){auto s=socket(value.first.ss_family,SOCK_STREAM,IPPROTO_TCP);if(s==INVALID_SOCKET)continue;auto candidate=std::make_unique<Socket>(*this,s);nonblocking(s);int code=::connect(s,(const sockaddr*)&value.first,value.second);if(code&&WSAGetLastError()!=WSAEWOULDBLOCK)continue;try{ready(s,true,until);int error=0,length=sizeof(error);if(getsockopt(s,SOL_SOCKET,SO_ERROR,(char*)&error,&length)||error)continue;remote=std::move(candidate);break;}catch(const Cancelled&){throw;}catch(...){}}
  if(!remote)throw std::runtime_error("Cannot connect to SOCKS5 proxy.");handshake(remote->value,target,until);
  if(connect)sendText(accepted,"HTTP/1.1 200 Connection Established\r\n\r\n",until);
  else {Url url(address);std::string authority=url.host;if(url.port!=80)authority+=":"+std::to_string(url.port);sendText(remote->value,method+" "+url.path+url.query+" HTTP/1.1\r\nHost: "+authority+"\r\n"+outgoing+"Connection: close\r\n\r\n",until);}
  tunneled=true;relay(accepted,remote->value);
 }catch(...){if(!tunneled&&!stop)try{sendText(accepted,"HTTP/1.1 502 Bad Gateway\r\nContent-Length: 0\r\nConnection: close\r\n\r\n",GetTickCount64()+500);}catch(...) {}}shutdown(accepted,SD_BOTH);}
 void run(){while(!stop){for(auto it=clients.begin();it!=clients.end();)if(*it->done){it->thread.join();it=clients.erase(it);}else ++it;fd_set set;FD_ZERO(&set);FD_SET(listener,&set);timeval delay{0,50000};if(select(0,&set,nullptr,nullptr,&delay)<=0)continue;auto s=accept(listener,nullptr,nullptr);if(s==INVALID_SOCKET)continue;if(clients.size()>=64){closesocket(s);continue;}auto done=std::make_shared<std::atomic_bool>(false);try{clients.push_back({std::thread([this,s,done]{serve(s);*done=true;}),done});}catch(...){closesocket(s);}}
 }
 explicit Impl(const Json& settings){
  validateSocksSettings(settings);if(WSAStartup(MAKEWORD(2,2),&wsa))throw std::runtime_error("Cannot initialize SOCKS5 networking.");started=true;
  try {clients.reserve(64);auto proxy=endpoint(str(settings,"Proxy"));user=str(settings,"ProxyUser");password=reveal(str(settings,"ProxySecret"));auto secret="udm:"+guid()+guid();token="Basic "+b64(Bytes(secret.begin(),secret.end()));
   ADDRINFOW hints{};hints.ai_family=AF_UNSPEC;hints.ai_socktype=SOCK_STREAM;PADDRINFOW results=nullptr;auto host=wide(proxy.host),service=std::to_wstring(proxy.port);if(GetAddrInfoW(host.c_str(),service.c_str(),&hints,&results))throw std::runtime_error("Cannot resolve SOCKS5 proxy host.");for(auto item=results;item;item=item->ai_next)if(item->ai_addrlen<=sizeof(sockaddr_storage)){sockaddr_storage address{};memcpy(&address,item->ai_addr,item->ai_addrlen);addresses.push_back({address,(int)item->ai_addrlen});}FreeAddrInfoW(results);
   listener=socket(AF_INET,SOCK_STREAM,IPPROTO_TCP);if(listener==INVALID_SOCKET)throw std::runtime_error("Cannot create SOCKS5 bridge.");BOOL exclusive=TRUE;setsockopt(listener,SOL_SOCKET,SO_EXCLUSIVEADDRUSE,(const char*)&exclusive,sizeof(exclusive));sockaddr_in local{};local.sin_family=AF_INET;local.sin_addr.s_addr=htonl(INADDR_LOOPBACK);if(bind(listener,(sockaddr*)&local,sizeof(local))||listen(listener,32))throw std::runtime_error("Cannot bind SOCKS5 bridge.");int size=sizeof(local);getsockname(listener,(sockaddr*)&local,&size);port=ntohs(local.sin_port);acceptor=std::thread([this]{run();});
  }catch(...){if(listener!=INVALID_SOCKET)closesocket(listener);WSACleanup();started=false;throw;}
 }
 ~Impl(){stop=true;if(acceptor.joinable())acceptor.join();{std::lock_guard<std::mutex> lock(mutex);for(auto s:sockets)shutdown(s,SD_BOTH);}for(auto& client:clients)if(client.thread.joinable())client.thread.join();if(listener!=INVALID_SOCKET)closesocket(listener);if(!password.empty())SecureZeroMemory(password.data(),password.size());if(started)WSACleanup();}
};
SocksProxy::SocksProxy(const Json& p):impl(std::make_unique<Impl>(p)){}
SocksProxy::~SocksProxy()=default;
std::wstring SocksProxy::address()const{return L"127.0.0.1:"+std::to_wstring(impl->port);}
void SocksProxy::allow(const Url& url){std::lock_guard<std::mutex> lock(impl->mutex);impl->allowed.insert(hostKey(url.host,url.port));}
void SocksProxy::credentials(HINTERNET request)const{auto plain=unb64(impl->token.substr(6));std::string secret(plain.begin()+4,plain.end());auto password=wide(secret);if(!WinHttpSetCredentials(request,WINHTTP_AUTH_TARGET_PROXY,WINHTTP_AUTH_SCHEME_BASIC,L"udm",password.c_str(),nullptr))throw std::runtime_error("Cannot authenticate the SOCKS5 bridge.");}
}
