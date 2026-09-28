#include "Ftp.hpp"
#include "DialUp.hpp"
#include "SocksProxy.hpp"
#include "ProxyPolicy.hpp"
#include <ws2tcpip.h>
#include <algorithm>
#include <regex>
#include <limits>

namespace udm { namespace {
struct Retryable : std::runtime_error {using runtime_error::runtime_error;};
struct Socket {
 SOCKET value=INVALID_SOCKET;
 Socket()=default;explicit Socket(SOCKET s):value(s){}
 ~Socket(){close();}Socket(const Socket&)=delete;Socket& operator=(const Socket&)=delete;
 Socket(Socket&& s)noexcept:value(s.value){s.value=INVALID_SOCKET;}
 Socket& operator=(Socket&& s)noexcept{if(this!=&s){close();value=s.value;s.value=INVALID_SOCKET;}return *this;}
 void close(){if(value!=INVALID_SOCKET){closesocket(value);value=INVALID_SOCKET;}}
 void nonblocking(){u_long mode=1;if(ioctlsocket(value,FIONBIO,&mode))throw Retryable("Cannot initialize FTP socket.");}
};
struct Winsock {Winsock(){WSADATA data{};if(WSAStartup(MAKEWORD(2,2),&data))throw Retryable("Cannot initialize FTP networking.");}~Winsock(){WSACleanup();}};
struct Address {sockaddr_storage value{};int length=0;int proxyIndex=-1;};
void ready(SOCKET s,bool write,const Cancel& cancel,ULONGLONG until){
 for(;;){cancel.check();if(GetTickCount64()>=until)throw Retryable("FTP server timed out.");
  fd_set set,errors;FD_ZERO(&set);FD_SET(s,&set);FD_ZERO(&errors);FD_SET(s,&errors);timeval delay{0,50000};
  int result=select(0,write?nullptr:&set,write?&set:nullptr,&errors,&delay);
  if(result<0||FD_ISSET(s,&errors))throw Retryable("FTP socket failed.");if(result>0)return;
 }
}
std::vector<Address> resolve(const Url& url,const Cancel& cancel){
 ADDRINFOEXW hints{};hints.ai_family=AF_UNSPEC;hints.ai_socktype=SOCK_STREAM;hints.ai_protocol=IPPROTO_TCP;
 PADDRINFOEXW result=nullptr;Handle event(CreateEventW(nullptr,TRUE,FALSE,nullptr));if(!event)throw Retryable("Cannot initialize FTP name lookup.");
 OVERLAPPED operation{};operation.hEvent=event.h;HANDLE query=nullptr;timeval timeout{15,0};auto host=wide(url.host),port=std::to_wstring(url.port);
 auto code=GetAddrInfoExW(host.c_str(),port.c_str(),NS_DNS,nullptr,&hints,&result,&timeout,&operation,nullptr,&query);
 if(code==WSA_IO_PENDING){bool cancelled=false;auto until=GetTickCount64()+15000;
  while(WaitForSingleObject(event.h,25)!=WAIT_OBJECT_0){if(!cancelled&&(cancel.cancelled()||GetTickCount64()>=until)){GetAddrInfoExCancel(&query);cancelled=true;}}
  code=GetAddrInfoExOverlappedResult(&operation);
 }
 std::unique_ptr<ADDRINFOEXW,decltype(&FreeAddrInfoExW)> owner(result,FreeAddrInfoExW);cancel.check();
 if(code)throw Retryable("Cannot resolve FTP server.");std::vector<Address> addresses;
 for(auto item=result;item;item=item->ai_next)if(item->ai_addrlen<=sizeof(sockaddr_storage)){Address a;a.length=(int)item->ai_addrlen;memcpy(&a.value,item->ai_addr,item->ai_addrlen);addresses.push_back(a);}
 if(addresses.empty())throw Retryable("FTP server has no usable address.");return addresses;
}
Socket connect(const Address& address,const Cancel& cancel){
 Socket socket(::socket(address.value.ss_family,SOCK_STREAM,IPPROTO_TCP));if(socket.value==INVALID_SOCKET)throw Retryable("Cannot create FTP connection.");socket.nonblocking();
 int code=::connect(socket.value,(const sockaddr*)&address.value,address.length);
 if(code&&WSAGetLastError()!=WSAEWOULDBLOCK)throw Retryable("Cannot connect to FTP server.");
 ready(socket.value,true,cancel,GetTickCount64()+15000);int error=0,length=sizeof(error);
 if(getsockopt(socket.value,SOL_SOCKET,SO_ERROR,(char*)&error,&length)||error)throw Retryable("FTP connection rejected.");return socket;
}
size_t receive(Socket& socket,void* data,size_t size,const Cancel& cancel,ULONGLONG until){
 for(;;){ready(socket.value,false,cancel,until);int n=recv(socket.value,(char*)data,(int)size,0);if(n==SOCKET_ERROR&&WSAGetLastError()==WSAEWOULDBLOCK)continue;if(n<0)throw Retryable("FTP data connection interrupted.");return (size_t)n;}
}
void safeArgument(const std::string& value){if(value.size()>8192||std::any_of(value.begin(),value.end(),[](unsigned char ch){return ch<32||ch==127||ch==255;}))throw std::runtime_error("FTP address or login contains unsupported control characters.");}
struct Reply {int code;std::string text;};
void rejected(const Reply& reply,const char* action){auto message=std::string("FTP ")+action+" rejected ("+std::to_string(reply.code)+").";if(reply.code>=400&&reply.code<500)throw Retryable(message);throw std::runtime_error(message);}
bool unsupported(const Reply& reply){return reply.code==500||reply.code==501||reply.code==502||reply.code==504;}
struct Metadata {
 i64 size=-1;std::string modified;
 bool usable()const{return size>=0&&!modified.empty();}
 bool operator==(const Metadata& other)const{return size==other.size&&modified==other.modified;}
 bool matches(const Metadata& other,bool ignoreDate)const{return size==other.size&&(ignoreDate||modified==other.modified);}
};
bool directPolicy(const Url&,const Json&);
struct Route {
 struct Choice {Json prefs;std::unique_ptr<SocksConnector> proxy;};
 Url destination;std::vector<Choice> choices;
 Route(const Url& url,const Json& original,const Cancel& cancel,bool dataRequired):destination(url){
  validateProtocolProxies(original);auto prefs=protocolProxySettings(original,url);
  for(auto& resolved:resolvePacRoutes(prefs,url,cancel)){
   Choice choice;choice.prefs=resolved;
   if(!directPolicy(url,resolved)){
    if(str(resolved,"ResolvedPacProxyScheme")=="https")throw std::runtime_error("FTP over a TLS-encrypted proxy is not supported. No unencrypted proxy connection was made.");
    if(dataRequired&&!yes(prefs,"FtpPassive",true))throw std::runtime_error("FTP through a proxy requires passive mode. Enable Use FTP in PASV mode in Options.");
    choice.proxy=std::make_unique<SocksConnector>(resolved);
   }choices.push_back(std::move(choice));
  }
 }
 Socket tunnel(int index,unsigned short port,const Cancel& cancel)const{try{return Socket(choices.at(index).proxy->connect(destination.host,port,cancel));}catch(const SocksConnectionError& e){throw Retryable(e.what());}}
 Socket control(Address& peer,const Cancel& cancel)const{
  std::exception_ptr failure;
  for(size_t index=0;index<choices.size();++index)try{
   if(choices[index].proxy){auto socket=tunnel((int)index,destination.port,cancel);peer.proxyIndex=(int)index;return socket;}
   for(const auto& address:resolve(destination,cancel))try{auto socket=connect(address,cancel);peer=address;return socket;}catch(const Retryable&){failure=std::current_exception();}
  }catch(const Retryable&){failure=std::current_exception();}
  cancel.check();if(failure)std::rethrow_exception(failure);throw Retryable("Cannot connect to FTP server through the configured routes.");
 }
 Socket data(int port,Address peer,const Cancel& cancel)const{
  if(peer.proxyIndex>=0)return tunnel(peer.proxyIndex,(unsigned short)port,cancel);
  if(peer.value.ss_family==AF_INET)((sockaddr_in*)&peer.value)->sin_port=htons((u_short)port);else ((sockaddr_in6*)&peer.value)->sin6_port=htons((u_short)port);
  return connect(peer,cancel);
 }
};
struct Session {
 Socket control;Address peer;const Route& route;const Cancel& cancel;std::string buffered;
 Session(const Route& r,const std::string& user,const std::string& password,const Cancel& c):route(r),cancel(c){
  control=route.control(peer,c);
  auto reply=read();if(reply.code==120)reply=read();if(reply.code!=220)rejected(reply,"greeting");
  reply=command("USER "+user);if(reply.code==331)reply=command("PASS "+password);if(reply.code!=230)rejected(reply,"login");
  reply=command("TYPE I");if(reply.code!=200)rejected(reply,"binary mode");
 }
 std::string line(ULONGLONG until){for(;;){auto end=buffered.find("\r\n");if(end!=std::string::npos){auto line=buffered.substr(0,end);buffered.erase(0,end+2);return line;}if(buffered.size()>16384)throw std::runtime_error("FTP reply is too large.");char bytes[1024];auto n=receive(control,bytes,sizeof(bytes),cancel,until);if(!n)throw Retryable("FTP control connection closed.");buffered.append(bytes,n);}}
 Reply read(){auto until=GetTickCount64()+30000;auto first=line(until);
  if(first.size()<4||first.substr(0,3).find_first_not_of("0123456789")!=std::string::npos||(first[3]!=' '&&first[3]!='-'))throw std::runtime_error("Malformed FTP reply.");
  Reply reply{std::stoi(first.substr(0,3)),first.substr(4)};
  if(first[3]=='-'){size_t total=first.size();for(int n=0;;++n){auto next=line(until);total+=next.size();if(total>65536||n>=256)throw std::runtime_error("FTP multiline reply is too large.");reply.text+="\n"+next;if(next.rfind(first.substr(0,3)+" ",0)==0)break;}}
  return reply;
 }
 Reply command(const std::string& text){safeArgument(text);auto bytes=text+"\r\n";size_t at=0;auto until=GetTickCount64()+15000;
  while(at<bytes.size()){ready(control.value,true,cancel,until);int n=::send(control.value,bytes.data()+at,(int)(bytes.size()-at),0);if(n==SOCKET_ERROR&&WSAGetLastError()==WSAEWOULDBLOCK)continue;if(n<=0)throw Retryable("Cannot send FTP command.");at+=n;}return read();
 }
 Metadata metadata(const std::string& path){Metadata m;auto reply=command("SIZE "+path);
  if(reply.code==213){if(reply.text.empty()||reply.text.find_first_not_of("0123456789")!=std::string::npos)throw std::runtime_error("Malformed FTP file size.");try{m.size=std::stoll(reply.text);}catch(...){throw std::runtime_error("FTP file is too large.");}}
  else if(!unsupported(reply))rejected(reply,"file size");
  reply=command("MDTM "+path);if(reply.code==213){if(!std::regex_match(reply.text,std::regex("[0-9]{14}(\\.[0-9]{1,9})?")))throw std::runtime_error("Malformed FTP modification time.");m.modified=reply.text;}
  else if(!unsupported(reply))rejected(reply,"modification time");return m;
 }
 bool restart(){auto reply=command("REST 0");if(reply.code==350)return true;if(unsupported(reply))return false;rejected(reply,"restart probe");return false;}
 Socket data(const std::string& path,i64 offset,bool passive){
  Socket data,listener;
  if(passive){auto reply=command("EPSV");int port=0;
   if(reply.code==229){auto begin=reply.text.find('('),end=reply.text.find(')',begin);if(begin==std::string::npos||end==std::string::npos)throw std::runtime_error("Malformed FTP EPSV reply.");auto value=reply.text.substr(begin+1,end-begin-1);if(value.size()<5||value[0]!=value[1]||value[0]!=value[2]||value.back()!=value[0])throw std::runtime_error("Malformed FTP EPSV reply.");auto number=value.substr(3,value.size()-4);if(number.empty()||number.size()>5||number.find_first_not_of("0123456789")!=std::string::npos)throw std::runtime_error("Malformed FTP data port.");port=std::stoi(number);}
   else {if(!unsupported(reply))rejected(reply,"passive mode");if(peer.proxyIndex<0&&peer.value.ss_family!=AF_INET)throw std::runtime_error("This IPv6 FTP server does not support EPSV.");reply=command("PASV");if(reply.code!=227)rejected(reply,"passive mode");std::smatch match;if(!std::regex_search(reply.text,match,std::regex("\\(([0-9]{1,3}),([0-9]{1,3}),([0-9]{1,3}),([0-9]{1,3}),([0-9]{1,3}),([0-9]{1,3})\\)")))throw std::runtime_error("Malformed FTP PASV reply.");for(int i=1;i<=6;++i)if(std::stoi(match[i])>255)throw std::runtime_error("Malformed FTP PASV reply.");port=std::stoi(match[5])*256+std::stoi(match[6]);}
   // Never follow a server-supplied IP to another host (FTP bounce / NAT).
   if(port<1024||port>65535)throw std::runtime_error("FTP server offered an unsafe data port.");data=route.data(port,peer,cancel);
  }else{
   Address local;local.length=sizeof(local.value);if(getsockname(control.value,(sockaddr*)&local.value,&local.length))throw Retryable("Cannot find FTP local address.");
   if(local.value.ss_family==AF_INET)((sockaddr_in*)&local.value)->sin_port=0;else ((sockaddr_in6*)&local.value)->sin6_port=0;
   listener=Socket(::socket(local.value.ss_family,SOCK_STREAM,IPPROTO_TCP));if(listener.value==INVALID_SOCKET)throw Retryable("Cannot create active FTP listener.");BOOL exclusive=TRUE;setsockopt(listener.value,SOL_SOCKET,SO_EXCLUSIVEADDRUSE,(const char*)&exclusive,sizeof(exclusive));listener.nonblocking();
   if(bind(listener.value,(sockaddr*)&local.value,local.length)||listen(listener.value,8)||getsockname(listener.value,(sockaddr*)&local.value,&local.length))throw Retryable("Cannot listen for active FTP data.");
   char host[INET6_ADDRSTRLEN]{};bool v4=local.value.ss_family==AF_INET;auto raw=v4?(void*)&((sockaddr_in*)&local.value)->sin_addr:(void*)&((sockaddr_in6*)&local.value)->sin6_addr;InetNtopA(local.value.ss_family,raw,host,sizeof(host));auto port=ntohs(v4?((sockaddr_in*)&local.value)->sin_port:((sockaddr_in6*)&local.value)->sin6_port);
   auto reply=command("EPRT |"+std::string(v4?"1":"2")+"|"+host+"|"+std::to_string(port)+"|");
   if(reply.code!=200){if(!v4||!unsupported(reply))rejected(reply,"active mode");std::string address=host;std::replace(address.begin(),address.end(),'.',',');reply=command("PORT "+address+","+std::to_string(port>>8)+","+std::to_string(port&255));if(reply.code!=200)rejected(reply,"active mode");}
  }
  if(offset){auto reply=command("REST "+std::to_string(offset));if(reply.code!=350)rejected(reply,"resume offset");}
  auto reply=command("RETR "+path);if(reply.code!=150&&reply.code!=125)rejected(reply,"download");
  if(!passive){auto until=GetTickCount64()+15000;for(;;){ready(listener.value,false,cancel,until);Address incoming;incoming.length=sizeof(incoming.value);Socket accepted(accept(listener.value,(sockaddr*)&incoming.value,&incoming.length));if(accepted.value==INVALID_SOCKET)continue;
    bool same=incoming.value.ss_family==peer.value.ss_family;
    if(same&&peer.value.ss_family==AF_INET)same=((sockaddr_in*)&incoming.value)->sin_addr.s_addr==((sockaddr_in*)&peer.value)->sin_addr.s_addr;
    else if(same)same=!memcmp(&((sockaddr_in6*)&incoming.value)->sin6_addr,&((sockaddr_in6*)&peer.value)->sin6_addr,sizeof(IN6_ADDR));
    if(same){accepted.nonblocking();data=std::move(accepted);break;}
   }}return data;
 }
 void finished(){auto reply=read();if(reply.code!=226&&reply.code!=250)rejected(reply,"transfer completion");}
};
fs::path partPath(const fs::path& folder,i64 index){wchar_t name[32];swprintf_s(name,L"%04lld.part",index);return folder/name;}
bool validPlan(Json segments,i64 size){if(!segments.is_array()||segments.empty()||segments.size()>32||size<0)return false;std::sort(segments.begin(),segments.end(),[](const Json& a,const Json& b){return num(a,"Start")<num(b,"Start");});i64 next=0;std::set<i64> ids;for(const auto& s:segments){auto id=num(s,"Index",-1),end=num(s,"End",-2);if(id<0||id>100000||!ids.insert(id).second||num(s,"Start",-1)!=next||end>=size||(end<next&&size!=0))return false;next=end+1;}return next==size;}
bool directPolicy(const Url& url,const Json& prefs){auto mode=str(prefs,"ProxyMode",str(prefs,"Proxy").empty()?"Use Windows proxy / PAC settings":"Use a proxy server");if(mode=="Connect directly")return true;
 if((isSocksProxy(prefs)||mode=="Use a proxy server")&&socksBypass(url,str(prefs,"ProxyBypass")))return true;
 if(isSocksProxy(prefs)||mode=="Use a proxy server")return false;
 if(mode=="Use Windows proxy / PAC settings"){WINHTTP_CURRENT_USER_IE_PROXY_CONFIG config{};if(!WinHttpGetIEProxyConfigForCurrentUser(&config))throw std::runtime_error("Cannot check Windows FTP proxy settings. Choose an explicit connection mode.");bool configured=config.fAutoDetect||(config.lpszAutoConfigUrl&&*config.lpszAutoConfigUrl)||(config.lpszProxy&&*config.lpszProxy);if(config.lpszAutoConfigUrl)GlobalFree(config.lpszAutoConfigUrl);if(config.lpszProxy)GlobalFree(config.lpszProxy);if(config.lpszProxyBypass)GlobalFree(config.lpszProxyBypass);if(!configured)return true;}
 throw std::runtime_error("FTP through this proxy mode is not supported. No direct connection was made. Configure a direct connection or an explicit bypass for this server.");
}
std::string httpDate(const std::string& value){if(value.size()<14)return {};SYSTEMTIME time{};time.wYear=(WORD)std::stoi(value.substr(0,4));time.wMonth=(WORD)std::stoi(value.substr(4,2));time.wDay=(WORD)std::stoi(value.substr(6,2));time.wHour=(WORD)std::stoi(value.substr(8,2));time.wMinute=(WORD)std::stoi(value.substr(10,2));time.wSecond=(WORD)std::stoi(value.substr(12,2));FILETIME stamp{};if(!SystemTimeToFileTime(&time,&stamp)||!FileTimeToSystemTime(&stamp,&time))return {};wchar_t text[WINHTTP_TIME_FORMAT_BUFSIZE]{};return WinHttpTimeFromSystemTime(&time,text)?utf8(text):"";}
} // namespace

Json previewFtp(const std::string& address,const Headers& headers,const Json& prefs,const Cancel& cancel){
 Url url(address);auto path=unescape(url.path);safeArgument(path);std::string user="anonymous",password="udm@example.invalid";
 for(const auto& entry:headers)if(lower(entry.first)=="authorization"&&entry.second.rfind("Basic ",0)==0){auto decoded=unb64(entry.second.substr(6));std::string value(decoded.begin(),decoded.end());auto colon=value.find(':');user=value.substr(0,colon);password=colon==std::string::npos?"":value.substr(colon+1);}
 safeArgument(user);safeArgument(password);ensureDialConnection(prefs,cancel);Winsock winsock;Route route(url,prefs,cancel,false);Session session(route,user,password,cancel);auto metadata=session.metadata(path);
 return {{"Size",metadata.size},{"ContentType",""}};
}

void transferFtp(Manager& manager,JobPtr job,const std::shared_ptr<Cancel>& cancel,const Json& prefs,const Headers& headers,const std::string& address,const fs::path& parts,JobPtr owner,const std::shared_ptr<Rate>& rate){
 Url url(address);auto path=unescape(url.path);safeArgument(path);std::string user="anonymous",password="udm@example.invalid";
 for(const auto& entry:headers)if(lower(entry.first)=="authorization"&&entry.second.rfind("Basic ",0)==0){auto decoded=unb64(entry.second.substr(6));std::string value(decoded.begin(),decoded.end());auto colon=value.find(':');user=value.substr(0,colon);password=colon==std::string::npos?"":value.substr(colon+1);}
 safeArgument(user);safeArgument(password);ensureDialConnection(prefs,*cancel);Winsock winsock;Route route(url,prefs,*cancel,true);Metadata remote;bool resumable=false;
 {Session probe(route,user,password,*cancel);remote=probe.metadata(path);resumable=remote.usable()&&probe.restart();}
 int connections,retries;Json segments;bool existing=false;
 {Lock lock(manager.mutex);auto& data=job->data;segments=data.value("Segments",Json::array());
  // Detect actual saved bytes, including a process exit before counters were saved.
  for(const auto& file:fs::directory_iterator(parts))if(file.is_regular_file()&&file.path().extension()==L".part"&&file.file_size())existing=true;
  if(yes(data,"FtpInvalidated"))throw Changed("The FTP file changed during transfer. Use File > Redownload to start a new copy; saved parts were preserved.");
  bool same=resumable&&num(data,"Size",-1)==remote.size&&(str(data,"FtpModified")==remote.modified||(yes(prefs,"IgnoreLastModified")&&!yes(data,"RefreshPendingValidation")))&&reveal(str(data,"ProtectedFtpSource"))==address+"\n"+user&&validPlan(segments,remote.size);
  if(existing&&!same)throw Changed("Cannot validate the saved FTP parts against this server file. Use File > Redownload to start a new copy; saved parts were preserved.");
  connections=(int)std::clamp<i64>(num(data,"Connections",8),1,32);retries=manager.retries(str(data,"Queue"));
  if(!same){for(const auto& file:fs::directory_iterator(parts))if(file.is_regular_file()&&file.path().extension()==L".part")fs::remove(file.path());segments=Json::array();
   int count=resumable?(int)std::min<i64>(connections,std::max<i64>(1,remote.size/(1024*1024))):1;auto chunk=remote.size>=0?(remote.size/count+(remote.size%count?1:0)):0;
   for(int i=0;i<count;++i){auto start=i*chunk,end=remote.size>=0?start+std::min(chunk,remote.size-start)-1:-2;segments.push_back({{"Index",i},{"Start",start},{"End",end},{"Done",0},{"FtpComplete",false}});}
  }
  i64 total=0;for(auto& s:segments){auto file=partPath(parts,num(s,"Index"));auto have=fs::exists(file)?(i64)fs::file_size(file):0;auto need=num(s,"End")-num(s,"Start")+1;if(have<0||(need>=0&&have>need))throw std::runtime_error("FTP partial file exceeds its assigned range.");if(!fs::exists(file)||have!=num(s,"Done"))s["FtpComplete"]=false;s["Done"]=have;total+=have;}
  data["Segments"]=segments;data["Received"]=total;data["Size"]=remote.size;data["RangeSupported"]=resumable;data["FtpModified"]=remote.modified;data["ProtectedFtpSource"]=protect(address+"\n"+user);data["Modified"]=httpDate(remote.modified);data["ETag"]=nullptr;data["RefreshPendingValidation"]=false;
  connections=std::min(connections,(int)segments.size());job->workers.assign(connections,Worker{});for(int i=0;i<connections;++i)job->workers[i].number=i+1;manager.save();
 }
 auto group=std::make_shared<Cancel>();group->parent=cancel;std::atomic_size_t next{0};std::mutex errorMutex;std::exception_ptr error;bool changed=false;
 auto work=[&](int number){auto& activity=job->workers[number];try{for(;;){auto index=next.fetch_add(1);if(index>=segments.size())break;const auto segment=segments[index];auto start=num(segment,"Start"),end=num(segment,"End"),need=remote.size>=0?end-start+1:-1;auto file=partPath(parts,num(segment,"Index"));
   for(int attempt=0;;++attempt){group->check();try{
    i64 have=fs::exists(file)?(i64)fs::file_size(file):0;bool completed;{Lock lock(manager.mutex);completed=yes(job->data["Segments"][index],"FtpComplete");}if(completed&&need>=0&&have==need)break;
    // Re-read the tail if bytes arrived but the final FTP completion reply did not.
    if(!resumable)have=0;else if(need>=0&&have==need)have=std::max<i64>(0,have-65536);
    {Lock lock(manager.mutex);auto& actual=job->data["Segments"][index];job->data["Received"]=num(job->data,"Received")+have-num(actual,"Done");actual["Done"]=have;actual["FtpComplete"]=false;activity.start=start;activity.end=end;activity.position=start+have;activity.state="Connecting";}
    Session session(route,user,password,*group);if(!session.metadata(path).matches(remote,yes(prefs,"IgnoreLastModified")))throw Changed("FTP server file changed during transfer.");
    auto data=session.data(path,start+have,yes(prefs,"FtpPassive",true));Handle output(CreateFileW(file.c_str(),GENERIC_WRITE,FILE_SHARE_READ,nullptr,OPEN_ALWAYS,FILE_ATTRIBUTE_NORMAL,nullptr));if(!output)throw std::runtime_error("Cannot write FTP partial file.");LARGE_INTEGER offset{};offset.QuadPart=have;if(!SetFilePointerEx(output.h,offset,nullptr,FILE_BEGIN)||!SetEndOfFile(output.h))throw std::runtime_error("Cannot seek FTP partial file.");
    BYTE buffer[65536];i64 done=have;bool fullFile=remote.size<0||end==remote.size-1;
    for(;;){if(!fullFile&&need>=0&&done==need)break;size_t count=sizeof(buffer);if(need>=0)count=(size_t)std::min<i64>(count,std::max<i64>(1,need-done));auto n=receive(data,buffer,count,*group,GetTickCount64()+30000);if(!n){if(need>=0&&done!=need)throw Retryable("FTP data ended before the expected bytes arrived.");break;}if(need>=0&&(i64)n>need-done)throw Changed("FTP server sent more bytes than its declared size.");manager.charge(n,*group,*rate,owner);DWORD written=0;if(!WriteFile(output.h,buffer,(DWORD)n,&written,nullptr)||written!=n)throw std::runtime_error("Cannot write FTP partial file.");done+=(i64)n;manager.progress(job,n,index,&activity);}
    data.close();if(fullFile)session.finished();if(!FlushFileBuffers(output.h))throw std::runtime_error("Cannot flush FTP partial file.");
    {Lock lock(manager.mutex);job->data["Segments"][index]["FtpComplete"]=true;if(remote.size<0){job->data["Size"]=done;job->data["Segments"][index]["End"]=done-1;}activity.state="Finished";manager.save();}break;
   }catch(const Retryable&){if(attempt>=retries)throw;{Lock lock(manager.mutex);activity.state="Retrying";}group->wait(std::min(8000,500*(1<<std::min(attempt,4))));}}
  }}catch(const Changed&){std::lock_guard<std::mutex> lock(errorMutex);error=std::current_exception();changed=true;group->stop=true;}catch(...){std::lock_guard<std::mutex> lock(errorMutex);if(!error)error=std::current_exception();group->stop=true;}
  {Lock lock(manager.mutex);activity.state=group->cancelled()?"Stopped":"Finished";}
 };
 std::vector<std::thread> workers;try{for(int i=0;i<connections;++i)workers.emplace_back(work,i);}catch(...){group->stop=true;for(auto& worker:workers)worker.join();throw;}for(auto& worker:workers)worker.join();
 if(changed){Lock lock(manager.mutex);job->data["FtpInvalidated"]=true;manager.save();}cancel->check();if(error)std::rethrow_exception(error);
 Session finalProbe(route,user,password,*cancel);if(!finalProbe.metadata(path).matches(remote,yes(prefs,"IgnoreLastModified"))){Lock lock(manager.mutex);job->data["FtpInvalidated"]=true;manager.save();throw Changed("FTP file changed before verification. Use File > Redownload; no mixed file was published.");}
}
}
