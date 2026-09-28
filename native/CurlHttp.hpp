#pragma once
#include "Core.hpp"
#include "SocksProxy.hpp"
#include <curl/curl.h>
#include <ws2tcpip.h>
#include <sstream>

namespace udm {
// Preserve explicit loopback, encrypted proxy and local-DNS SOCKS routes.
// WinHTTP and its SOCKS bridge cannot represent all three without changing routing.
struct CurlConnectionFailure:std::runtime_error {DWORD code;CurlConnectionFailure(const char* message,DWORD value):runtime_error(message),code(value){}};
inline bool needsExplicitProxyTransport(const Url& url,const Json& prefs){
 const auto mode=str(prefs,"ProxyMode");
 if(mode!="Use a proxy server"&&!isSocksProxy(prefs))return false;
 if(socksBypass(url,str(prefs,"ProxyBypass")))return false;
 if(str(prefs,"ResolvedPacProxyScheme")=="https"||(isSocksProxy(prefs)&&prefs.contains("CapturedProxyDNS")&&!yes(prefs,"CapturedProxyDNS")))return true;
 auto host=lower(url.host);if(host.size()>1&&host.front()=='['&&host.back()==']')host=host.substr(1,host.size()-2);
 if(!host.empty()&&host.back()=='.')host.pop_back();
 if(host=="localhost"||host=="loopback"||hostIs(host,"localhost"))return true;
 IN_ADDR v4{};IN6_ADDR v6{};
 if(InetPtonA(AF_INET,host.c_str(),&v4)==1&&(ntohl(v4.s_addr)>>24)==127)return true;
 const auto legacy=inet_addr(host.c_str());if(legacy!=INADDR_NONE&&(ntohl(legacy)>>24)==127)return true;
 if(InetPtonA(AF_INET6,host.c_str(),&v6)==1){static const IN6_ADDR local=IN6ADDR_LOOPBACK_INIT;return memcmp(&v6,&local,sizeof(v6))==0||(IN6_IS_ADDR_V4MAPPED(&v6)&&v6.u.Byte[12]==127);}
 return false;
}

class CurlHttp {
 struct State {
  CURL* easy=nullptr;CURLM* multi=nullptr;curl_slist* outgoing=nullptr;
  bool attached=false,complete=false,paused=false,ready=false,digest=false;
  CURLcode result=CURLE_OK;DWORD status=0;size_t headerBytes=0,offset=0;
  Bytes buffer;std::vector<std::pair<std::string,std::string>> response;
  std::vector<std::string> cookies;std::exception_ptr callbackError;
  ~State(){if(attached)curl_multi_remove_handle(multi,easy);if(easy)curl_easy_cleanup(easy);if(multi)curl_multi_cleanup(multi);if(outgoing)curl_slist_free_all(outgoing);}
  template<class T>void set(CURLoption option,T value){auto code=curl_easy_setopt(easy,option,value);if(code!=CURLE_OK)throw std::runtime_error("HTTP proxy configuration (option "+std::to_string(option)+"): "+curl_easy_strerror(code));}
  void append(const std::string& header){auto* next=curl_slist_append(outgoing,header.c_str());if(!next)throw std::bad_alloc();outgoing=next;}
  static size_t body(char* data,size_t size,size_t count,void* opaque)noexcept{
   auto& self=*static_cast<State*>(opaque);
   try{
    if(size&&count>SIZE_MAX/size)return 0;const auto bytes=size*count;
    if(self.digest&&self.status==401)return bytes;
    if(!self.buffer.empty()){self.paused=true;return CURL_WRITEFUNC_PAUSE;}
    if(bytes>CURL_MAX_WRITE_SIZE)throw std::runtime_error("HTTP response block exceeds the permitted size.");
    self.buffer.assign(data,data+bytes);self.offset=0;return bytes;
   }catch(...){self.callbackError=std::current_exception();return 0;}
  }
  static size_t header(char* data,size_t size,size_t count,void* opaque)noexcept{
   auto& self=*static_cast<State*>(opaque);
   try{
    if(size&&count>SIZE_MAX/size)return 0;const auto bytes=size*count;
    if(bytes>196608||self.headerBytes>196608-bytes)throw std::runtime_error("HTTP response headers exceed the permitted size.");
    self.headerBytes+=bytes;std::string line(data,bytes);
    if(line.rfind("HTTP/",0)==0){
     std::istringstream input(line);std::string version;unsigned code=0;input>>version>>code;
     if(code<100||code>599)throw std::runtime_error("Invalid HTTP response status.");
     self.status=code;self.ready=false;self.response.clear();
    }else if(line=="\r\n"||line=="\n"){
     // Digest may require a challenge exchange on this connection. Wait for its
     // final response or completion instead of exposing the intermediate 401.
     self.ready=self.status>=200&&!(self.digest&&self.status==401);
    }else{
     auto colon=line.find(':');if(colon==std::string::npos||self.response.size()>=512)throw std::runtime_error("Invalid HTTP response headers.");
     auto name=lower(trim(line.substr(0,colon))),value=trim(line.substr(colon+1));
     self.response.emplace_back(name,value);if(name=="set-cookie")self.cookies.push_back(value);
    }
    return bytes;
   }catch(...){self.callbackError=std::current_exception();return 0;}
  }
  void check(){
   if(callbackError)std::rethrow_exception(callbackError);
   if(complete&&result!=CURLE_OK){
    // Only connection failures before an HTTP response qualify for PAC failover.
    // TLS, authentication, malformed responses and partial bodies remain terminal.
    DWORD code=0;if(!status){
     if(result==CURLE_COULDNT_CONNECT)code=ERROR_WINHTTP_CANNOT_CONNECT;
     else if(result==CURLE_COULDNT_RESOLVE_PROXY||result==CURLE_COULDNT_RESOLVE_HOST)code=ERROR_WINHTTP_NAME_NOT_RESOLVED;
     else if(result==CURLE_OPERATION_TIMEDOUT)code=ERROR_WINHTTP_TIMEOUT;
    }
    if(code)throw CurlConnectionFailure("HTTP proxy connection failed.",code);
    throw std::runtime_error(std::string("HTTP proxy request failed: ")+curl_easy_strerror(result)+".");
   }
  }
  void pump(const Cancel& cancel){
   cancel.check();int running=0;auto code=curl_multi_perform(multi,&running);
   if(code!=CURLM_OK)throw std::runtime_error("HTTP proxy transfer failed.");
   int remaining=0;while(auto* message=curl_multi_info_read(multi,&remaining))if(message->msg==CURLMSG_DONE){complete=true;result=message->data.result;}
   cancel.check();check();
  }
  void wait(const Cancel& cancel,ULONGLONG deadline){
   cancel.check();if(GetTickCount64()>=deadline)throw std::runtime_error("HTTP proxy response timed out.");
   if(curl_multi_poll(multi,nullptr,0,25,nullptr)!=CURLM_OK)throw std::runtime_error("HTTP proxy wait failed.");
   pump(cancel);
  }
 };
 std::unique_ptr<State> state=std::make_unique<State>();
public:
 CurlHttp(const std::string& address,const std::wstring& rawHeaders,const Json& prefs,const Cancel& cancel,const Bytes* body,bool head,const std::pair<std::string,std::string>& digest={} ){
  static const auto initialized=curl_global_init(CURL_GLOBAL_DEFAULT);
  if(initialized!=CURLE_OK)throw std::runtime_error("HTTP proxy transport initialization failed.");
  auto& s=*state;s.easy=curl_easy_init();s.multi=curl_multi_init();if(!s.easy||!s.multi)throw std::bad_alloc();
  s.digest=!digest.first.empty();
  s.set(CURLOPT_URL,address.c_str());s.set(CURLOPT_PROTOCOLS_STR,"http,https");s.set(CURLOPT_FOLLOWLOCATION,0L);
  s.set(CURLOPT_NOSIGNAL,1L); // NETRC support is excluded from this static build.
  s.set(CURLOPT_CONNECTTIMEOUT_MS,15000L);s.set(CURLOPT_HTTP_VERSION,(long)CURL_HTTP_VERSION_1_1);
  s.set(CURLOPT_HTTP_CONTENT_DECODING,0L);s.set(CURLOPT_SUPPRESS_CONNECT_HEADERS,1L);
  s.set(CURLOPT_SSL_VERIFYPEER,1L);s.set(CURLOPT_SSL_VERIFYHOST,2L);
  s.set(CURLOPT_SSLVERSION,(long)(CURL_SSLVERSION_TLSv1_2|(yes(prefs,"UseTls13",true)?CURL_SSLVERSION_MAX_DEFAULT:CURL_SSLVERSION_MAX_TLSv1_2)));
  s.set(CURLOPT_PROXY_SSL_VERIFYPEER,1L);s.set(CURLOPT_PROXY_SSL_VERIFYHOST,2L);
  s.set(CURLOPT_PROXY_SSLVERSION,(long)CURL_SSLVERSION_TLSv1_2);
  s.set(CURLOPT_SSL_OPTIONS,0L); // Do not opt into automatic client-certificate selection.
  s.set(CURLOPT_NOPROXY,""); // The caller already resolved the explicit bypass policy.
  const auto mode=str(prefs,"ProxyMode");
  std::string scheme=mode=="Use a SOCKS5 proxy"?(yes(prefs,"CapturedProxyDNS",true)?"socks5h":"socks5"):mode=="Use a SOCKS4 / 4a proxy"?(yes(prefs,"CapturedProxyDNS",true)?"socks4a":"socks4"):str(prefs,"ResolvedPacProxyScheme","http");
  auto proxy=scheme+"://"+str(prefs,"Proxy");s.set(CURLOPT_PROXY,proxy.c_str());
  auto proxyUser=str(prefs,"ProxyUser"),proxySecret=reveal(str(prefs,"ProxySecret"));
  if(!proxyUser.empty()){s.set(CURLOPT_PROXYUSERNAME,proxyUser.c_str());s.set(CURLOPT_PROXYPASSWORD,proxySecret.c_str());s.set(CURLOPT_PROXYAUTH,(long)CURLAUTH_BASIC);}
  auto userAgent=str(prefs,"UserAgent");if(userAgent.empty())userAgent="UDM/0.67.0";s.set(CURLOPT_USERAGENT,userAgent.c_str());
  std::istringstream input(utf8(rawHeaders));std::string line;
  while(std::getline(input,line)){
   if(!line.empty()&&line.back()=='\r')line.pop_back();if(line.empty())continue;
   if(s.digest&&lower(line).rfind("authorization:",0)==0)continue;
   s.append(line);
  }
  // Headers belonging to the origin must never be placed on HTTPS CONNECT.
  s.set(CURLOPT_HEADEROPT,(long)CURLHEADER_SEPARATE);
  if(body){s.set(CURLOPT_POSTFIELDSIZE_LARGE,(curl_off_t)body->size());s.set(CURLOPT_POSTFIELDS,body->empty()?"":reinterpret_cast<const char*>(body->data()));s.append("Expect:");}
  if(head)s.set(CURLOPT_NOBODY,1L);
  if(s.digest){s.set(CURLOPT_HTTPAUTH,(long)CURLAUTH_DIGEST);s.set(CURLOPT_USERNAME,digest.first.c_str());s.set(CURLOPT_PASSWORD,digest.second.c_str());}
  s.set(CURLOPT_HTTPHEADER,s.outgoing);s.set(CURLOPT_HEADERFUNCTION,State::header);s.set(CURLOPT_HEADERDATA,&s);
  s.set(CURLOPT_WRITEFUNCTION,State::body);s.set(CURLOPT_WRITEDATA,&s);
  if(curl_multi_add_handle(s.multi,s.easy)!=CURLM_OK)throw std::runtime_error("HTTP proxy transfer initialization failed.");s.attached=true;
  const auto deadline=GetTickCount64()+30000;s.pump(cancel);while(!s.ready&&!s.complete)s.wait(cancel,deadline);
  if(s.status<100)throw std::runtime_error("The proxy returned no HTTP response.");
 }
 DWORD status()const{return state->status;}
 std::vector<std::string> headers(const wchar_t* name)const{std::vector<std::string> values;const auto key=lower(utf8(name));for(const auto& entry:state->response)if(entry.first==key)values.push_back(entry.second);return values;}
 std::string header(const wchar_t* name)const{const auto values=headers(name);return values.empty()?"":values.front();}
 const std::vector<std::string>& receivedCookies()const{return state->cookies;}
 size_t read(void* destination,size_t size,const Cancel& cancel){
  cancel.check();if(!size)return 0;auto& s=*state;const auto deadline=GetTickCount64()+30000;
  while(s.buffer.empty()){
   s.check();if(s.complete)return 0;
   if(s.paused){s.paused=false;auto code=curl_easy_pause(s.easy,CURLPAUSE_CONT);if(code!=CURLE_OK)throw std::runtime_error("HTTP proxy resume failed.");s.check();if(!s.buffer.empty())break;}
   s.wait(cancel,deadline);
  }
  const auto count=std::min(size,s.buffer.size()-s.offset);memcpy(destination,s.buffer.data()+s.offset,count);s.offset+=count;
  if(s.offset==s.buffer.size()){s.buffer.clear();s.offset=0;}return count;
 }
};
}
