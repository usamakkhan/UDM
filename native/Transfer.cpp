#include "ContentDispositionName.hpp"
#include "MediaStorage.hpp"
#include "MediaAssemblyVerification.hpp"
#include "MediaPublication.hpp"
#include "StreamCache.hpp"
#include "Core.hpp"
#include "StreamHash.hpp"
#include "PostBodyStore.hpp"
#include "Ftp.hpp"
#include "FileIdentity.hpp"
#include "DownloadPreview.hpp"
#include "SocksProxy.hpp"
#include "ProxyPolicy.hpp"
#include "PacProxy.hpp"
#include "BrowserProxy.hpp"
#include "BrowserCookies.hpp"
#include "CurlHttp.hpp"
#include "SiteLogins.hpp"
#include "DialUp.hpp"
#include "GuiModels.hpp"
#include "GrabberDestinations.hpp"
#include "ContentSniff.hpp"
#include "Id3v2.hpp"
#include "ProtocolFallback.hpp"
#include "QueueRetry.hpp"
#include <wininet.h>
#include <winioctl.h>
#include <shlwapi.h>
#include <xmllite.h>
#include <cwctype>
#include <array>
#include <string_view>
#include <fstream>
#include <sstream>
#include <iomanip>
#include <regex>
#include <future>
#include <algorithm>
#include <condition_variable>
namespace udm {
static void internetError(const char* operation){throw InternetFailure(operation,GetLastError());}
// Worker read size. Each read is one asynchronous WinHTTP round trip plus the
// Manager lock traffic of progress accounting, so 16 KiB reads capped a fast
// link; 256 KiB keeps that overhead below one percent of the transfer.
static constexpr size_t transferReadBuffer=256*1024;
// After a download was receiving, a connection that fails without a byte is a
// network outage rather than a dead server: for this long probe without spending
// the retry budget, so a short blackout ends promptly when the network is back.
static constexpr ULONGLONG outageProbeWindow=20000,outageProbeInterval=500;
static constexpr auto busyRangeProbeInterval=std::chrono::milliseconds(3000);
static constexpr auto recoveryRampInterval=std::chrono::milliseconds(100);
static constexpr auto recoveryRampDuration=std::chrono::seconds(2);
// A connection that delivered at least this much before it broke was a working
// connection, not a failed attempt: it is reopened at once with a fresh retry budget.
static constexpr i64 reconnectProgressReset=1048576;
// Assembly copies parts in this unit while hashing them in the same pass.
static constexpr size_t assemblyBuffer=1024*1024;
// A worker gives its range back when another response can continue the transfer.
// With a delay it can claim again. Without one it retires for this transfer.
struct RangeReleased{std::optional<int> pauseMs;};
struct HttpOverlapMismatch:Changed{
 std::optional<size_t> range;
 HttpOverlapMismatch(const char* message,std::optional<size_t> index={}):Changed(message),range(index){}
};
static constexpr i64 httpOverlapLimit=64;
HttpSession::HttpSession(const Json& prefs,std::function<void(const Json&)> saveBrowserSession):preferences(prefs) {
 if(prefs.contains("ActiveBrowserSession"))browserCookies=std::make_unique<BrowserCookieJar>(prefs["ActiveBrowserSession"],std::move(saveBrowserSession)); validateProtocolProxies(prefs);validatePacSettings(prefs);
 auto proxy=wide(str(prefs,"Proxy")),bypass=wide(str(prefs,"ProxyBypass"));auto mode=str(prefs,"ProxyMode",proxy.empty()?"Use Windows proxy / PAC settings":"Use a proxy server");DWORD access=(mode=="Connect directly"||usesPacScript(prefs))?WINHTTP_ACCESS_TYPE_NO_PROXY:mode=="Use a proxy server"?WINHTTP_ACCESS_TYPE_NAMED_PROXY:WINHTTP_ACCESS_TYPE_AUTOMATIC_PROXY;
 if(isSocksProxy(prefs)){socks=std::make_unique<SocksProxy>(prefs);proxy=socks->address();access=WINHTTP_ACCESS_TYPE_NAMED_PROXY;}
 auto userAgent=wide(str(prefs,"UserAgent").empty()?"UDM/0.35.0":str(prefs,"UserAgent"));session=WinHttpOpen(userAgent.c_str(),access,access==WINHTTP_ACCESS_TYPE_NAMED_PROXY?proxy.c_str():WINHTTP_NO_PROXY_NAME,bypass.empty()?WINHTTP_NO_PROXY_BYPASS:bypass.c_str(),WINHTTP_FLAG_ASYNC);
 if(!session)internetError("HTTP initialization");
 const auto responseTimeout=serverResponseSeconds(prefs)*1000;
 DWORD headerTimeout=(DWORD)responseTimeout;
 // Keep the default cap on the whole header exchange unless the requested receive wait is longer.
 if(!WinHttpSetTimeouts(session,15000,15000,30000,responseTimeout)||
    (headerTimeout>90000&&!WinHttpSetOption(session,WINHTTP_OPTION_RECEIVE_RESPONSE_TIMEOUT,&headerTimeout,sizeof(headerTimeout)))){
  auto error=GetLastError();WinHttpCloseHandle(session);session=nullptr;SetLastError(error);internetError("HTTP timeouts");
 }
 DWORD protocols=WINHTTP_FLAG_SECURE_PROTOCOL_TLS1_2|(yes(prefs,"UseTls13",true)?WINHTTP_FLAG_SECURE_PROTOCOL_TLS1_3:0);
 if(!WinHttpSetOption(session,WINHTTP_OPTION_SECURE_PROTOCOLS,&protocols,sizeof(protocols))){
  auto error=GetLastError();protocols=WINHTTP_FLAG_SECURE_PROTOCOL_TLS1_2;
  if(error!=ERROR_INVALID_PARAMETER||!WinHttpSetOption(session,WINHTTP_OPTION_SECURE_PROTOCOLS,&protocols,sizeof(protocols))){error=GetLastError();WinHttpCloseHandle(session);session=nullptr;SetLastError(error);internetError("TLS configuration");}
 }
 // Negotiate HTTP/2 when the origin supports it, and HTTP/3 where Windows offers
 // it and the server advertises it (Alt-Svc); HTTP/1.1 remains available. Older
 // Windows versions reject the HTTP/3 flag, or the whole optional selector.
 DWORD httpProtocols=WINHTTP_PROTOCOL_FLAG_HTTP2|(yes(prefs,"UseHttp3",true)?WINHTTP_PROTOCOL_FLAG_HTTP3:0);
 if(!WinHttpSetOption(session,WINHTTP_OPTION_ENABLE_HTTP_PROTOCOL,&httpProtocols,sizeof(httpProtocols))){
  auto error=GetLastError();auto optional=[](DWORD e){return e==ERROR_WINHTTP_INVALID_OPTION||e==ERROR_INVALID_PARAMETER;};
  httpProtocols=WINHTTP_PROTOCOL_FLAG_HTTP2;
  if(!optional(error)||(!WinHttpSetOption(session,WINHTTP_OPTION_ENABLE_HTTP_PROTOCOL,&httpProtocols,sizeof(httpProtocols))&&!optional(error=GetLastError()))){WinHttpCloseHandle(session);session=nullptr;SetLastError(error);internetError("HTTP protocol configuration");}
 }
 DWORD maximum=32;
 WinHttpSetOption(session,WINHTTP_OPTION_MAX_CONNS_PER_SERVER,&maximum,sizeof(maximum));
 WinHttpSetOption(session,WINHTTP_OPTION_MAX_CONNS_PER_1_0_SERVER,&maximum,sizeof(maximum));
}
Headers HttpSession::browserHeaders(const std::string& address,Headers headers)const{return browserCookies?browserCookies->headers(address,std::move(headers)):headers;}
void HttpSession::receiveBrowserCookies(const std::string& address,const std::vector<std::string>& values){if(browserCookies)browserCookies->receive(address,values);}
Json HttpSession::browserSessionSnapshot()const{return browserCookies?browserCookies->snapshot():Json::object();}
std::shared_ptr<CurlSession> HttpSession::curlSession(){std::lock_guard<std::mutex> lock(mutex);if(!curl)curl=std::make_shared<CurlSession>();return curl;}
HttpSession::~HttpSession(){for(auto& entry:connections)WinHttpCloseHandle(entry.second);if(session)WinHttpCloseHandle(session);}
std::shared_ptr<HttpSession> HttpSession::forUrl(const Url& url){
 if(!str(preferences,"CapturedProxyUrl").empty()&&Url(str(preferences,"CapturedProxyUrl")).full!=url.full)throw std::runtime_error("This download redirected beyond its captured proxy route. Capture the final address in the browser.");
 auto found=preferences.find("ProtocolProxies");if(found==preferences.end()||!found->contains(url.scheme))return {};
 std::lock_guard<std::mutex> lock(mutex);auto& route=routes[url.scheme];if(!route)route=std::make_shared<HttpSession>(protocolProxySettings(preferences,url));return route;
}
std::vector<std::shared_ptr<HttpSession>> HttpSession::pacRoutes(const Url& url,const Cancel& cancel){
 if(!usesPacScript(preferences))return {};auto resolved=resolvePacRoutes(preferences,url,cancel);std::vector<std::shared_ptr<HttpSession>> result;
 std::lock_guard<std::mutex> lock(mutex);for(auto& settings:resolved){auto key="pac:"+settings.dump();auto& route=routes[key];if(!route)route=std::make_shared<HttpSession>(settings);result.push_back(route);}return result;
}
HINTERNET HttpSession::connect(const Url& url){
 std::lock_guard<std::mutex> lock(mutex);
 if(socks)socks->allow(url);
 auto found=connections.find(url.origin);if(found!=connections.end())return found->second;
 auto connection=WinHttpConnect(session,wide(url.host).c_str(),url.port,0);
 if(!connection)internetError("HTTP connection");
 try{connections.emplace(url.origin,connection);}catch(...){WinHttpCloseHandle(connection);throw;}
 return connection;
}
bool HttpSession::proxyConnectionFailed(HINTERNET request)const{return socks&&socks->connectionFailed(request);}
void HttpSession::proxyCredentials(HINTERNET request)const{const auto& prefs=preferences;
 if(!str(prefs,"ResolvedPacProxyScheme").empty()){
  Url address(str(prefs,"ResolvedPacProxyScheme")+"://"+str(prefs,"Proxy")+"/");auto host=wide(address.host);WINHTTP_PROXY_RESULT_ENTRY entry{};entry.fProxy=TRUE;entry.ProxyScheme=address.scheme=="https"?INTERNET_SCHEME_HTTPS:INTERNET_SCHEME_HTTP;entry.pwszProxy=host.data();entry.ProxyPort=address.port;
  if(!WinHttpSetOption(request,WINHTTP_OPTION_PROXY_RESULT_ENTRY,&entry,sizeof(entry)))internetError("PAC proxy route");
 }
if(socks){socks->credentials(request);return;}auto user=wide(str(prefs,"ProxyUser")),password=wide(reveal(str(prefs,"ProxySecret")));if(!user.empty())WinHttpSetCredentials(request,WINHTTP_AUTH_TARGET_PROXY,WINHTTP_AUTH_SCHEME_BASIC,user.c_str(),password.c_str(),nullptr);}
// The facade remains blocking to its worker, but WinHTTP operations are asynchronous.
// Only that worker closes the request, after the API returned. Callback context and
// caller-owned read/POST buffers remain alive through HANDLE_CLOSING.
struct HttpAsyncState {
 std::mutex mutex;std::condition_variable changed;
 bool completed=false,closed=false,registered=false;DWORD error=0,bytes=0;
 static void CALLBACK callback(HINTERNET,DWORD_PTR context,DWORD status,void* information,DWORD length){
  auto* state=reinterpret_cast<HttpAsyncState*>(context);if(!state)return;
  std::lock_guard<std::mutex> lock(state->mutex);
  if(status==WINHTTP_CALLBACK_STATUS_HANDLE_CLOSING)state->closed=true;
  else if(status==WINHTTP_CALLBACK_STATUS_REQUEST_ERROR){state->error=information&&length>=sizeof(WINHTTP_ASYNC_RESULT)?static_cast<WINHTTP_ASYNC_RESULT*>(information)->dwError:ERROR_WINHTTP_INTERNAL_ERROR;state->completed=true;}
  else if(status==WINHTTP_CALLBACK_STATUS_SENDREQUEST_COMPLETE||status==WINHTTP_CALLBACK_STATUS_HEADERS_AVAILABLE||status==WINHTTP_CALLBACK_STATUS_READ_COMPLETE){state->bytes=status==WINHTTP_CALLBACK_STATUS_READ_COMPLETE?length:0;state->completed=true;}
  else return;
  state->changed.notify_all();
 }
};
void Http::closeRequest() noexcept {
 explicitProxy.reset();
 auto handle=request;request=nullptr;
 if(handle){
  if(WinHttpCloseHandle(handle)){
   if(async&&async->registered){std::unique_lock<std::mutex> lock(async->mutex);async->changed.wait(lock,[&]{return async->closed;});}
  }else if(async&&async->registered){
   // An unexpected close failure cannot justify freeing context still owned by WinHTTP.
   async.release();return;
  }
 }
 async.reset();
}
void Http::prepareOperation(){std::lock_guard<std::mutex> lock(async->mutex);async->completed=false;async->error=0;async->bytes=0;}
DWORD Http::awaitOperation(BOOL started,const Cancel& cancel,const char* operation){
 if(!started){auto error=GetLastError();if(error!=ERROR_IO_PENDING){cancel.check();SetLastError(error);internetError(operation);}}
 std::unique_lock<std::mutex> lock(async->mutex);
 while(!async->completed&&!cancel.cancelled())async->changed.wait_for(lock,std::chrono::milliseconds(25));
 if(cancel.cancelled()){lock.unlock();closeRequest();throw Cancelled();}
 auto error=async->error,count=async->bytes;lock.unlock();
 if(error){SetLastError(error);internetError(operation);}return count;
}
Http::~Http(){closeRequest();}
static Json curlSettingsFor(Json settings,const Json& prefs){if(yes(prefs,"DisableHttp2"))settings["DisableHttp2"]=true;return settings;}
Http::Http(const std::string& address,const Headers& headers,const Json& prefs,const Cancel& c,std::optional<i64> begin,std::optional<i64> end,std::string validator,const Bytes* body,bool redirects,std::shared_ptr<HttpSession> shared,bool head){
 try {
  if(head&&body)throw std::runtime_error("A metadata request cannot submit a body.");
  ensureDialConnection(prefs,c);auto common=shared?std::move(shared):std::make_shared<HttpSession>(prefs);
  std::string current=address;bool sensitive=true;
  for(int redirect=0;redirect<11;++redirect){
   c.check();Url u(current);auto selected=common->forUrl(u);if(!selected)selected=common;
   auto candidates=selected->pacRoutes(u,c);if(candidates.empty())candidates.push_back(selected);
   for(size_t route=0;route<candidates.size();++route){try {
    pool=candidates[route];
    if(u.scheme!="http"&&u.scheme!="https")throw std::runtime_error("HTTP redirect uses an unsupported protocol.");
 auto effective=siteRequestHeaders(current,headers,prefs,sensitive);effective=common->browserHeaders(current,std::move(effective));std::wstring h=L"Accept-Encoding: identity\r\n";if(begin){h+=L"Range: bytes="+std::to_wstring(*begin)+L"-"+(end?std::to_wstring(*end):L"")+L"\r\n";if(!validator.empty())h+=L"If-Range: "+wide(validator)+L"\r\n";}if(body){bool contentType=false,accept=false;for(const auto& item:headers){contentType|=lower(item.first)=="content-type";accept|=lower(item.first)=="accept";}if(!contentType)h+=L"Content-Type: application/x-protobuf\r\n";if(!accept)h+=L"Accept: application/vnd.yt-ump\r\n";}for(const auto& [k,v]:effective){auto key=lower(k);if(key=="authorization"&&v.empty())continue;if(!sensitive&&((key=="cookie"&&!prefs.contains("ActiveBrowserSession"))||key=="authorization"||key=="referer"||key=="origin"))continue;if(k.find_first_of("\r\n")!=std::string::npos||v.find_first_of("\r\n")!=std::string::npos)throw std::runtime_error("Invalid HTTP header.");h+=wide(k)+L": "+wide(v)+L"\r\n";}

    if(needsExplicitProxyTransport(u,pool->settings())){
     explicitProxy=std::make_unique<CurlHttp>(current,h,curlSettingsFor(pool->settings(),prefs),c,body,head,std::pair<std::string,std::string>{},body?nullptr:pool->curlSession());
     status=explicitProxy->status();common->receiveBrowserCookies(current,explicitProxy->receivedCookies());
     const auto login=basicLogin(effective);
     bool digest=false;for(const auto& challenge:this->headers(L"WWW-Authenticate"))digest|=std::regex_search(challenge,std::regex("(^|,)\\s*Digest\\s",std::regex::icase));
     if(status==401&&sensitive&&!login.first.empty()&&digest){
      explicitProxy.reset();
      // Refresh browser cookies from the challenge before retrying this origin.
      if(prefs.contains("ActiveBrowserSession")){
       const auto refreshed=common->browserHeaders(current,effective);std::wistringstream lines(h);std::wstring line,retry;
       while(std::getline(lines,line)){if(lower(utf8(line)).rfind("cookie:",0)!=0){if(!line.empty()&&line.back()==L'\r')line.pop_back();retry+=line+L"\r\n";}}
       retry+=L"Cookie: "+wide(headerValue(refreshed,"Cookie"))+L"\r\n";h=std::move(retry);
      }
      explicitProxy=std::make_unique<CurlHttp>(current,h,curlSettingsFor(pool->settings(),prefs),c,body,head,login,body?nullptr:pool->curlSession());
      status=explicitProxy->status();common->receiveBrowserCookies(current,explicitProxy->receivedCookies());
     }
    }else{
     validateSocksDestination(u,pool->settings());
session=pool->handle();if(u.scheme!="http"&&u.scheme!="https")throw std::runtime_error("HTTP redirect uses an unsupported protocol.");auto host=wide(u.host),path=wide(u.path+u.query);connection=pool->connect(u);if(!connection)internetError("HTTP connection");request=WinHttpOpenRequest(connection,head?L"HEAD":body?L"POST":L"GET",path.c_str(),nullptr,WINHTTP_NO_REFERER,WINHTTP_DEFAULT_ACCEPT_TYPES,u.scheme=="https"?WINHTTP_FLAG_SECURE:0);if(!request)internetError("HTTP request");if(yes(prefs,"DisableHttp2")||yes(prefs,"DisableHttp3")){DWORD allowed=yes(prefs,"DisableHttp2")?0:WINHTTP_PROTOCOL_FLAG_HTTP2;WinHttpSetOption(request,WINHTTP_OPTION_ENABLE_HTTP_PROTOCOL,&allowed,sizeof(allowed));}async=std::make_unique<HttpAsyncState>();DWORD_PTR context=reinterpret_cast<DWORD_PTR>(async.get());if(!WinHttpSetOption(request,WINHTTP_OPTION_CONTEXT_VALUE,&context,sizeof(context)))internetError("HTTP context");if(WinHttpSetStatusCallback(request,HttpAsyncState::callback,WINHTTP_CALLBACK_FLAG_ALL_COMPLETIONS|WINHTTP_CALLBACK_FLAG_HANDLES,0)==WINHTTP_INVALID_STATUS_CALLBACK)internetError("HTTP callback");async->registered=true;DWORD disabled=WINHTTP_DISABLE_REDIRECTS|WINHTTP_DISABLE_COOKIES;WinHttpSetOption(request,WINHTTP_OPTION_DISABLE_FEATURE,&disabled,sizeof(disabled));// Current-user Windows login is offered only to intranet servers (hosts without dots), and only when enabled.
 DWORD autologon=yes(prefs,"WindowsLoginIntranet",true)?WINHTTP_AUTOLOGON_SECURITY_LEVEL_MEDIUM:WINHTTP_AUTOLOGON_SECURITY_LEVEL_HIGH;WinHttpSetOption(request,WINHTTP_OPTION_AUTOLOGON_POLICY,&autologon,sizeof(autologon));
 pool->proxyCredentials(request);
 auto login=basicLogin(effective);int authRetries=0,proxyRetries=0;DWORD lastSupported=0;
 // WinHTTP completes Negotiate/NTLM for intranet servers by itself, whatever the
 // autologon policy says, so "no Windows login" means disabling its automatic
 // authentication. Saved logins and proxy logins need that feature, so a request
 // carrying either keeps it.
 if(!yes(prefs,"WindowsLoginIntranet",true)&&login.first.empty()&&str(prefs,"ProxyUser").empty()){DWORD noAuth=WINHTTP_DISABLE_REDIRECTS|WINHTTP_DISABLE_COOKIES|WINHTTP_DISABLE_AUTHENTICATION;WinHttpSetOption(request,WINHTTP_OPTION_DISABLE_FEATURE,&noAuth,sizeof(noAuth));}
 for(;;){const bool digestRetried=authRetries>0||proxyRetries>0;
  try{prepareOperation();awaitOperation(WinHttpSendRequest(request,digestRetried?WINHTTP_NO_ADDITIONAL_HEADERS:h.c_str(),digestRetried?0:(DWORD)h.size(),body?(void*)body->data():WINHTTP_NO_REQUEST_DATA,body?(DWORD)body->size():0,body?(DWORD)body->size():0,context),c,"HTTP send");
  prepareOperation();awaitOperation(WinHttpReceiveResponse(request,nullptr),c,"HTTP response");DWORD size=sizeof(status);if(!WinHttpQueryHeaders(request,WINHTTP_QUERY_STATUS_CODE|WINHTTP_QUERY_FLAG_NUMBER,nullptr,&status,&size,nullptr))internetError("HTTP status");}
  // Rejected Windows-login credentials surface as a login failure rather than a 401.
  catch(const InternetFailure& failure){if(failure.code!=ERROR_WINHTTP_LOGIN_FAILURE)throw;throw AuthenticationRequired(Url(current).origin,lastSupported?authSchemeName(lastSupported):"Negotiate");}
  common->receiveBrowserCookies(current,this->headers(L"Set-Cookie"));
  // Challenge schemes: Digest, Negotiate and NTLM need the server's challenge
  // before credentials apply. A saved login is used for any of them; without
  // one, an intranet Negotiate/NTLM server may receive the current Windows
  // login. NTLM can take two resends, so the loop allows one more pass.
  if(status==401&&sensitive&&authRetries<2){
   DWORD supported=0,first=0,target=0;
   if(WinHttpQueryAuthSchemes(request,&supported,&first,&target)&&target==WINHTTP_AUTH_TARGET_SERVER){lastSupported=supported;
    DWORD scheme=0;const bool windows=(supported&(WINHTTP_AUTH_SCHEME_NEGOTIATE|WINHTTP_AUTH_SCHEME_NTLM))!=0;
    if(!login.first.empty()){if(supported&WINHTTP_AUTH_SCHEME_DIGEST)scheme=WINHTTP_AUTH_SCHEME_DIGEST;else if(supported&WINHTTP_AUTH_SCHEME_NEGOTIATE)scheme=WINHTTP_AUTH_SCHEME_NEGOTIATE;else if(supported&WINHTTP_AUTH_SCHEME_NTLM)scheme=WINHTTP_AUTH_SCHEME_NTLM;}
    const bool challenge=scheme!=0,currentUser=windows&&login.first.empty()&&autologon!=WINHTTP_AUTOLOGON_SECURITY_LEVEL_HIGH;
    if((challenge||currentUser)&&(authRetries==0||scheme==WINHTTP_AUTH_SCHEME_NTLM||scheme==WINHTTP_AUTH_SCHEME_NEGOTIATE||currentUser)){
     if(challenge&&!authRetries){if(!WinHttpAddRequestHeaders(request,L"Authorization:",(DWORD)-1,WINHTTP_ADDREQ_FLAG_REPLACE))internetError("HTTP authentication header");auto name=wide(login.first),secret=wide(login.second);if(!WinHttpSetCredentials(request,WINHTTP_AUTH_TARGET_SERVER,scheme,name.c_str(),secret.c_str(),nullptr))internetError("HTTP authentication");}
     auto refreshed=common->browserHeaders(current,effective);if(prefs.contains("ActiveBrowserSession")){auto cookie=L"Cookie: "+wide(headerValue(refreshed,"Cookie"));if(!WinHttpAddRequestHeaders(request,cookie.c_str(),(DWORD)-1,WINHTTP_ADDREQ_FLAG_REPLACE|WINHTTP_ADDREQ_FLAG_ADD))internetError("HTTP cookie refresh");}
     ++authRetries;continue;
    }
   }
  }
  // Proxy challenges: the strongest scheme the proxy offers is used with the
  // configured proxy login. Basic was already supplied up front.
  // Proxies with a configured login use the curl transport (needsExplicitProxyTransport),
  // so this path serves a proxy that asks for Windows login while none is
  // configured: the current user's login is offered to intranet proxies.
  if(status==407&&proxyRetries<2&&str(prefs,"ProxyUser").empty()&&autologon!=WINHTTP_AUTOLOGON_SECURITY_LEVEL_HIGH){
   DWORD supported=0,first=0,target=0;
   if(WinHttpQueryAuthSchemes(request,&supported,&first,&target)&&target==WINHTTP_AUTH_TARGET_PROXY&&(supported&(WINHTTP_AUTH_SCHEME_NEGOTIATE|WINHTTP_AUTH_SCHEME_NTLM))){++proxyRetries;continue;}
  }break;
 }

    }
    // Cloudflare marks its verification responses explicitly. Stop before any
    // caller treats the response as file data, metadata, or a transient error.
    if(lower(trim(header(L"cf-mitigated")))=="challenge"){
     auto contentType=header(L"Content-Type");
     if(lower(trim(contentType.substr(0,contentType.find(';'))))=="text/html")
      throw BrowserVerificationRequired(status);
    }
    if(request&&status==502&&pool->proxyConnectionFailed(request))throw InternetFailure("SOCKS connection",ERROR_WINHTTP_CANNOT_CONNECT);
    finalUrl=current;
    if(redirects&&(status==301||status==302||status==303||status==307||status==308)){
     auto location=header(L"Location");if(location.empty())throw std::runtime_error("Redirect response has no Location header.");
     auto target=combineUrl(current,location);Url next(target);
     if(u.scheme=="https"&&next.scheme!="https")throw std::runtime_error("Refusing an HTTPS downgrade redirect.");
     if(u.origin!=next.origin)sensitive=false;
     closeRequest();connection=nullptr;current=target;break;
    }
    return;
   }catch(const CurlConnectionFailure& failure){
    if(body||route+1>=candidates.size()||!retryPacConnection(failure.code))throw;
    closeRequest();continue;
   }catch(const InternetFailure& failure){
    if(body||route+1>=candidates.size()||!retryPacConnection(failure.code))throw;
    closeRequest();connection=nullptr;session=nullptr;c.check();
   }}
  }
  throw std::runtime_error("Too many HTTP redirects.");
 }catch(...){closeRequest();connection=nullptr;session=nullptr;throw;}
}
std::string Http::protocol()const{
 if(explicitProxy)return explicitProxy->protocol();
 if(!request)return {};
 DWORD version=0,size=sizeof(version);if(WinHttpQueryOption(request,WINHTTP_OPTION_HTTP_PROTOCOL_USED,&version,&size)){if(version&WINHTTP_PROTOCOL_FLAG_HTTP3)return "HTTP/3";if(version&WINHTTP_PROTOCOL_FLAG_HTTP2)return "HTTP/2";}
 wchar_t text[32]{};size=sizeof(text);if(WinHttpQueryHeaders(request,WINHTTP_QUERY_VERSION,nullptr,text,&size,nullptr))return utf8(text);return {};
}
std::string Http::header(const wchar_t* name)const{if(explicitProxy)return explicitProxy->header(name);DWORD n=0;WinHttpQueryHeaders(request,WINHTTP_QUERY_CUSTOM,name,nullptr,&n,nullptr);if(GetLastError()!=ERROR_INSUFFICIENT_BUFFER)return {};std::wstring v(n/sizeof(wchar_t),0);if(!WinHttpQueryHeaders(request,WINHTTP_QUERY_CUSTOM,name,v.data(),&n,nullptr))return {};v.resize(n/sizeof(wchar_t));while(!v.empty()&&!v.back())v.pop_back();return utf8(v);}
std::vector<std::string> Http::headers(const wchar_t* name)const{
 if(explicitProxy)return explicitProxy->headers(name);
 std::vector<std::string> values;DWORD index=0;size_t total=0;
 while(values.size()<512){DWORD bytes=0,probe=index;WinHttpQueryHeaders(request,WINHTTP_QUERY_CUSTOM,name,nullptr,&bytes,&probe);if(GetLastError()!=ERROR_INSUFFICIENT_BUFFER)break;if(bytes>196608||total+bytes>196608)break;std::wstring value(bytes/sizeof(wchar_t),0);if(!WinHttpQueryHeaders(request,WINHTTP_QUERY_CUSTOM,name,value.data(),&bytes,&index))break;value.resize(bytes/sizeof(wchar_t));while(!value.empty()&&!value.back())value.pop_back();total+=bytes;values.push_back(utf8(value));}
 return values;
}
size_t Http::read(void* b,size_t n,const Cancel& c){if(explicitProxy)return explicitProxy->read(b,n,c);c.check();prepareOperation();return awaitOperation(WinHttpReadData(request,b,(DWORD)std::min<size_t>(n,MAXDWORD),nullptr),c,"HTTP read");}
void Http::beginRead(void* buffer,size_t count,const Cancel& c){if(reading)throw std::logic_error("A read is already outstanding.");c.check();pendingBuffer=buffer;pendingCount=count;reading=true;if(explicitProxy)return;prepareOperation();pendingStarted=WinHttpReadData(request,buffer,(DWORD)std::min<size_t>(count,MAXDWORD),nullptr);pendingError=pendingStarted?0:GetLastError();}
size_t Http::finishRead(const Cancel& c){if(!reading)throw std::logic_error("No read is outstanding.");reading=false;if(explicitProxy)return explicitProxy->read(pendingBuffer,pendingCount,c);SetLastError(pendingError);return awaitOperation(pendingStarted,c,"HTTP read");}
Bytes Http::all(size_t limit,const Cancel& c){Bytes out,b(65536);for(;;){auto n=read(b.data(),b.size(),c);if(!n)return out;if(out.size()+n>limit)throw std::runtime_error("Response exceeds the permitted size.");out.insert(out.end(),b.begin(),b.begin()+n);}}
static i64 length(const std::string& s){if(s.empty())return -1;if(!std::regex_match(s,std::regex("[0-9]+")))throw std::runtime_error("Invalid Content-Length.");try{return std::stoll(s);}catch(...){throw std::runtime_error("Content-Length overflow.");}}
struct ContentRange{i64 start=-1,end=-1,total=-1;bool valid=false;explicit ContentRange(const std::string& s){std::smatch m;if(std::regex_match(s,m,std::regex("bytes ([0-9]+)-([0-9]+)/([0-9]+)"))){start=length(m[1]);end=length(m[2]);total=length(m[3]);valid=start<=end&&end<total;}else if(s=="bytes */0"){total=0;valid=true;}}};
static void success(const Http& r){if(r.status==401){if(auto schemes=challengeSchemes(r.headers(L"WWW-Authenticate")))throw AuthenticationRequired(Url(r.finalUrl).origin,authSchemeName(schemes));}if(r.status==401){DWORD supported=0,first=0,target=0;if(WinHttpQueryAuthSchemes(r.request,&supported,&first,&target)&&target==WINHTTP_AUTH_TARGET_SERVER&&(supported&(WINHTTP_AUTH_SCHEME_BASIC|WINHTTP_AUTH_SCHEME_DIGEST|WINHTTP_AUTH_SCHEME_NEGOTIATE|WINHTTP_AUTH_SCHEME_NTLM)))throw AuthenticationRequired(Url(r.finalUrl).origin,authSchemeName(supported));}if(r.status<200||r.status>=300)throw HttpRejected(r.status,r.header(L"Retry-After"));auto enc=lower(r.header(L"Content-Encoding"));if(!enc.empty()&&enc!="identity")throw std::runtime_error("The server ignored identity encoding.");}
// Layout written by builds before 1.1: one NNNN.part per range. Still read for resume.
static fs::path partPath(const fs::path& folder,i64 index){wchar_t b[32];swprintf_s(b,L"%04lld.part",index);return folder/b;}
// Current layout: one <job-id>.data file of the final size. Every range writes in
// place at Start+Done, so completion is a hash pass and a rename instead of a copy.
static fs::path dataPath(const fs::path& folder,const std::string& id){return folder/(wide(id)+L".data");}
static void clearParts(const fs::path& folder){if(fs::exists(folder))for(const auto& p:fs::directory_iterator(folder))if(p.is_regular_file()&&(p.path().extension()==L".part"||p.path().extension()==L".data"))fs::remove(p.path());}
static i64 fileLength(HANDLE file){LARGE_INTEGER size{};if(!GetFileSizeEx(file,&size))throw std::runtime_error("Cannot read the download file size.");return size.QuadPart;}
static void setFileLength(HANDLE file,i64 size){FILE_END_OF_FILE_INFO info{};info.EndOfFile.QuadPart=size;if(!SetFileInformationByHandle(file,FileEndOfFileInfo,&info,sizeof(info)))throw std::runtime_error("Cannot reserve space for the download file.");}
static void setSparse(HANDLE file,bool sparse){FILE_SET_SPARSE_BUFFER request{};request.SetSparse=sparse?1:0;DWORD returned=0;DeviceIoControl(file,FSCTL_SET_SPARSE,&request,sizeof(request),nullptr,0,&returned,nullptr);}
// Positioned I/O on a handle that may be one of several open on the same file.
static void writeAt(HANDLE file,const void* data,size_t count,i64 offset){OVERLAPPED at{};at.Offset=(DWORD)(offset&0xffffffffu);at.OffsetHigh=(DWORD)((unsigned long long)offset>>32);DWORD written=0;if(!WriteFile(file,data,(DWORD)count,&written,&at)||written!=count)throw std::runtime_error("Cannot write the partial download.");}
static size_t readAt(HANDLE file,void* data,size_t count,i64 offset){OVERLAPPED at{};at.Offset=(DWORD)(offset&0xffffffffu);at.OffsetHigh=(DWORD)((unsigned long long)offset>>32);DWORD read=0;if(!ReadFile(file,data,(DWORD)count,&read,&at)&&GetLastError()!=ERROR_HANDLE_EOF)return 0;return read;}
static i64 allocatedPrefix(HANDLE file,i64 begin,i64 saved,const Cancel& cancel){
 const i64 end=begin+saved;
 i64 covered=begin;
 while(covered<end){
  cancel.check();
  FILE_ALLOCATED_RANGE_BUFFER query{},ranges[16]{};
  query.FileOffset.QuadPart=covered;
  query.Length.QuadPart=end-covered;
  DWORD bytes=0;
  const BOOL complete=DeviceIoControl(file,FSCTL_QUERY_ALLOCATED_RANGES,&query,sizeof(query),ranges,sizeof(ranges),&bytes,nullptr);
  if(!complete&&GetLastError()!=ERROR_MORE_DATA)return saved;
  if(bytes>sizeof(ranges)||bytes%sizeof(ranges[0]))return saved;
  const auto before=covered;
  for(size_t i=0;i<bytes/sizeof(ranges[0]);++i){
   const i64 start=ranges[i].FileOffset.QuadPart,length=ranges[i].Length.QuadPart;
   if(start<0||length<=0||start>std::numeric_limits<i64>::max()-length)return saved;
   if(start>covered)return covered-begin;
   covered=std::max(covered,start+length);
   if(covered>=end)return saved;
  }
  if(complete)return covered-begin;
  if(covered==before)return saved;
 }
 return saved;
}
static bool hasSparseAllocations(HANDLE file){
 FILE_BASIC_INFO info{};
 return GetFileInformationByHandleEx(file,FileBasicInfo,&info,sizeof(info))&&
  (info.FileAttributes&FILE_ATTRIBUTE_SPARSE_FILE)&&!(info.FileAttributes&FILE_ATTRIBUTE_COMPRESSED);
}
static i64 retainedDataPrefix(HANDLE file,i64 length,bool sparse,i64 start,i64 saved,const Cancel& cancel){
 const auto present=std::min(saved,std::max<i64>(0,length-start));
 return sparse&&present>0?allocatedPrefix(file,start,present,cancel):present;
}
// Opens or creates the download's single output file for a saved or fresh plan and
// settles each range's resume offset. The catalog's Done advances only after
// writes return, but a sparse file can retain its length after losing allocations.
// Use its saved offset only up to the first reported hole. A plan whose NNNN.part files
// still exist was written by an older build; each part is folded into the single
// file at its range's Start and its size becomes that range's Done, as before.
// Without the file, saved progress is lost: the ranges restart at zero.
// The file is sparse while incomplete so that writes far past the valid data
// length are not preceded by zero-filling everything before them.
static std::vector<fs::path> prepareDataFile(const fs::path& parts,const std::string& id,i64 size,Json& segments,const Cancel& cancel){
 const auto path=dataPath(parts,id);const bool existed=fs::exists(path);
 Handle file(CreateFileW(path.c_str(),GENERIC_READ|GENERIC_WRITE,FILE_SHARE_READ|FILE_SHARE_WRITE,nullptr,OPEN_ALWAYS,FILE_ATTRIBUTE_NORMAL,nullptr));
 if(!file)throw std::runtime_error("Cannot create the download file.");
 if(!existed)setSparse(file.h,true);
 const bool sparse=existed&&hasSparseAllocations(file.h);
 const i64 length=fileLength(file.h);std::vector<char> buffer;
 std::vector<fs::path> migrated;
 for(auto& segment:segments){
  const i64 begin=num(segment,"Start"),end=num(segment,"End"),need=end>=begin?end-begin+1:-1;
  i64 have=existed?std::max<i64>(0,num(segment,"Done")):0;
  if(need>=0)have=std::min(have,need);
  have=retainedDataPrefix(file.h,length,sparse,begin,have,cancel);
  const auto part=partPath(parts,num(segment,"Index"));
  if(fs::exists(part)){
   i64 copy=(i64)fs::file_size(part);if(need>=0)copy=std::min(copy,need);
   if(copy>0){
    std::ifstream input(part,std::ios::binary);if(!input)throw std::runtime_error("A required partial file is missing.");
    if(buffer.empty())buffer.resize(assemblyBuffer);
    for(i64 at=0;at<copy;){cancel.check();const auto n=(size_t)std::min<i64>((i64)buffer.size(),copy-at);input.read(buffer.data(),(std::streamsize)n);if(input.gcount()!=(std::streamsize)n)throw std::runtime_error("Could not read a partial file.");writeAt(file.h,buffer.data(),n,begin+at);at+=(i64)n;}
   }
   have=copy;migrated.push_back(part);
  }
  segment["Done"]=have;
 }
 if(size>=0&&fileLength(file.h)!=size)setFileLength(file.h,size);
 if(!migrated.empty()&&!FlushFileBuffers(file.h))throw std::runtime_error("Could not flush the migrated partial download.");
 return migrated;
}
static std::string etag(const Http& r){auto s=r.header(L"ETag");return s.rfind("W/",0)==0?"":s;}
static bool strongModificationDate(const Http& response,const std::string& modified){
 if(!response.header(L"ETag").empty())return false;
 const auto parse=[](const std::string& value)->std::optional<i64>{
  if(value.size()!=29||std::any_of(value.begin(),value.end(),[](unsigned char c){return c<0x20||c>0x7e;}))return {};
  const auto text=wide(value);
  SYSTEMTIME time{};FILETIME file{};wchar_t canonical[WINHTTP_TIME_FORMAT_BUFSIZE]{};
  if(!WinHttpTimeToSystemTime(text.c_str(),&time)||!SystemTimeToFileTime(&time,&file)||
     !WinHttpTimeFromSystemTime(&time,canonical)||text!=canonical)return {};
  ULARGE_INTEGER stamp{};stamp.LowPart=file.dwLowDateTime;stamp.HighPart=file.dwHighDateTime;
  return static_cast<i64>(stamp.QuadPart/10000000);
 };
 const auto stamp=parse(modified),date=parse(response.header(L"Date"));
 // Leave a margin for different clocks on the two response fields.
 return stamp&&date&&*date-*stamp>=60;
}
static std::string responseSha256(const Http& response,const wchar_t* field){
 const bool legacy=std::wstring_view(field)==L"Digest";
 for(const auto& line:response.headers(field)){
  std::istringstream entries(line);
  for(std::string entry;std::getline(entries,entry,',');){
   entry=trim(entry);
   std::string encoded;
   if(legacy){
    const auto equals=entry.find('=');
    if(equals==std::string::npos||lower(trim(entry.substr(0,equals)))!="sha-256")continue;
    encoded=trim(entry.substr(equals+1));
   }else{
    constexpr std::string_view prefix="sha-256=:";
    if(entry.size()!=prefix.size()+45||entry.compare(0,prefix.size(),prefix)!=0||entry.back()!=':')continue;
    encoded=entry.substr(prefix.size(),44);
   }
   if(encoded.size()!=44)continue;
   Bytes digest;
   try{digest=unb64(encoded);}catch(const std::runtime_error&){continue;}
   if(digest.size()!=32||b64(digest)!=encoded)continue;
   std::string result;
   constexpr char hex[]="0123456789abcdef";
   for(auto byte:digest){result+=hex[byte>>4];result+=hex[byte&15];}
   return result;
  }
 }
 return {};
}
static std::string httpOverlapHash(const BYTE* bytes,size_t size){
 StreamHash hash;hash.update(bytes,size);return hash.finish();
}
// Hashes the download while it arrives. Ranges are claimed front to back, so the
// contiguous prefix of the file grows at nearly the download rate; a thread trails
// it with SHA-256 and completion only hashes the ranges still open when the last
// byte came in. Without this the finish phase is a full pass over the file (about
// two seconds per 384 MiB on a CPU without SHA extensions).
struct PrefixHasher {
 struct Reader {
  Handle file;
  std::vector<char> buffer;
 };
 std::unique_ptr<StreamHash> hash;i64 hashed=0;bool valid=false;
 std::string digest;
 void reset(){hash=std::make_unique<StreamHash>();hashed=0;digest.clear();valid=true;}
 const std::string& finish(){if(digest.empty())digest=hash->finish();return digest;}
 // Bytes complete from the start of the file according to the catalog ranges.
 static i64 contiguous(Json segments,i64 from){
  std::sort(segments.begin(),segments.end(),[](const Json& a,const Json& b){return num(a,"Start")<num(b,"Start");});
  i64 position=from;
  for(const auto& s:segments){const i64 begin=num(s,"Start"),end=num(s,"End"),done=num(s,"Done");
   if(begin>position)break;
   position=std::max(position,begin+done);
   if(end<begin||done<end-begin+1)break;
  }
  return position;
 }
 // Hashes [hashed,limit) of the file. Those bytes are complete and never rewritten.
 void advance(Reader& reader,const fs::path& path,i64 limit,const Cancel& cancel,const std::function<void()>& checkpoint={}){
  if(!valid||limit<=hashed)return;
  cancel.check();
  if(!reader.file){
   reader.file.h=CreateFileW(path.c_str(),GENERIC_READ,FILE_SHARE_READ|FILE_SHARE_WRITE,nullptr,OPEN_EXISTING,0,nullptr);
   if(!reader.file)throw std::runtime_error("Cannot read the downloaded file.");
   reader.buffer.resize(assemblyBuffer);
  }
  while(hashed<limit){
   if(checkpoint)checkpoint();else cancel.check();
   const auto n=(size_t)std::min<i64>((i64)reader.buffer.size(),limit-hashed);
   if(readAt(reader.file.h,reader.buffer.data(),n,hashed)!=n)throw std::runtime_error("Cannot read the downloaded file.");
   hash->update(reader.buffer.data(),n);hashed+=(i64)n;
  }
 }
 void advance(const fs::path& path,i64 limit,const Cancel& cancel,const std::function<void()>& checkpoint={}){
  Reader reader;
  advance(reader,path,limit,cancel,checkpoint);
 }
};
// Copy the retained tail and hash exactly the bytes written to staging.
static void copyFileTail(const fs::path& path,HANDLE output,const Cancel& cancel,i64& skip,
                         StreamHash& digest,std::vector<char>& buffer,HANDLE verified=INVALID_HANDLE_VALUE){
 Handle opened(verified==INVALID_HANDLE_VALUE
  ?CreateFileW(path.c_str(),GENERIC_READ,FILE_SHARE_READ|FILE_SHARE_WRITE,nullptr,OPEN_EXISTING,FILE_FLAG_SEQUENTIAL_SCAN,nullptr)
  :INVALID_HANDLE_VALUE);
 const auto input=opened?opened.h:verified;
 if(input==INVALID_HANDLE_VALUE)throw std::runtime_error("A required partial file is missing.");
 const auto length=fileLength(input);
 i64 offset=std::clamp<i64>(skip,0,length);
 skip-=offset;
 while(offset<length){
  cancel.check();
  const auto count=(DWORD)std::min<i64>((i64)buffer.size(),length-offset);
  if(readAt(input,buffer.data(),count,offset)!=count)throw std::runtime_error("Could not read a partial file.");
  DWORD written=0;
  if(!WriteFile(output,buffer.data(),count,&written,nullptr)||written!=count)
   throw std::runtime_error("Could not assemble the download.");
  digest.update(buffer.data(),count);
  offset+=count;
 }
}
static void copyFileTo(const fs::path& path,HANDLE output,const Cancel& c){std::ifstream input(path,std::ios::binary);if(!input)throw std::runtime_error("A required partial file is missing.");std::vector<char> buffer(assemblyBuffer);char* b=buffer.data();while(input){c.check();input.read(b,(std::streamsize)buffer.size());auto n=input.gcount();if(n){DWORD written=0;if(!WriteFile(output,b,(DWORD)n,&written,nullptr)||written!=n)throw std::runtime_error("Could not assemble the download.");}}if(!input.eof())throw std::runtime_error("Could not read a partial download.");}
// A saved range plan must cover the resource exactly, independent of part-file IDs.
static bool completePlan(const Json& segments,i64 size){
 if(!segments.is_array())return false;
 if(segments.empty())return size==0;
 auto ordered=segments;
 std::sort(ordered.begin(),ordered.end(),[](const Json& a,const Json& b){return num(a,"Start")<num(b,"Start");});
 i64 position=0;std::set<i64> ids;
 for(const auto& segment:ordered){
  auto begin=num(segment,"Start",-1),end=num(segment,"End",-1),id=num(segment,"Index",-1);
  if(begin!=position||end<begin||end>=size||id<0||id>100000||!ids.insert(id).second)return false;
  position=end+1;
 }
 return position==size;
}
static i64 freshHttpConnectionLimit(i64 size,i64 connections){
 constexpr i64 rangeSizeTarget=4*1048576;
 return std::min(connections,1+(size-1)/rangeSizeTarget);
}
static std::string webPageMessage(const SniffResult& result,const std::string& expected){
 return "The server sent "+std::string(result.verdict==SniffVerdict::Html?"a web page":"text")+" instead of the expected ."+expected+" file. The received data was kept. Refresh the download address, or clear \"Stop downloads that receive a web page instead of the file\" in Options > File Types and resume.";
}
static i64 checkedDirectOutputSize(const Bytes& head,i64 size){
 if(size<=0||static_cast<i64>(head.size())!=std::min<i64>(size,contentSniffReadLimit))return -1;
 const auto result=sniffContent(head.data(),head.size(),true);
 if(result.verdict!=SniffVerdict::Match&&result.verdict!=SniffVerdict::MatchAfterPrefix)return -1;
 const auto prefix=static_cast<i64>(result.prefixLength);
 return prefix<size?size-prefix:-1;
}
static void transferHttp(Manager& m,JobPtr job,const std::shared_ptr<Cancel>& cancel,
 const Json& prefs,const Headers& headers,const std::string& url,const fs::path& parts,
 JobPtr limitOwner,const std::shared_ptr<Rate>& rate,std::shared_ptr<HttpSession> pool={},PrefixHasher* hasher=nullptr){
 const auto httpStartTick=GetTickCount64();
 // HTTP/3 multiplexes every range of this download over one QUIC connection, which on a shared
 // link gets one connection's share: Alpine over a 4 MB/s link took 143 to 156 s against 64 s on
 // separate connections (docs/speed-benchmark-2026-10-23.md). A download planned with more than
 // one connection therefore negotiates HTTP/2 or HTTP/1.1; single-connection transfers keep HTTP/3.
 Json poolPrefs=prefs;{Lock lock(m.mutex);if(std::clamp<i64>(num(job->data,"Connections",8),1,32)>1)poolPrefs["UseHttp3"]=false;}
 if(!pool)pool=std::make_shared<HttpSession>(poolPrefs,browserSessionSaver(m,job,prefs.value("ActiveBrowserSession",Json::object())));int retryBudget;
 constexpr auto unsafeRefresh="The new address could not be verified as the same resumable file. Saved partial data was kept. Try another link, or add a separate new download.";
 const auto dataFile=dataPath(parts,job->id());
 {Lock lock(m.mutex);retryBudget=m.retries(str(job->data,"Queue"));}
 // A server deadline applies to new requests for this transfer, even when a
 // worker gives up its range or a validator change forces a fresh probe.
 std::atomic<ULONGLONG> retryNotBefore{0};
 auto deferRequests=[&](int delay){
  const auto until=GetTickCount64()+delay;
  auto before=retryNotBefore.load();
  while(before<until&&!retryNotBefore.compare_exchange_weak(before,until)){}
 };
 auto rememberServerDelay=[&](const HttpRejected& rejection){
  if(rejection.retryAfterMs==-1)return;
  Lock lock(m.mutex);
  recordServerRetry(job->data,rejection);
 };
 auto waitForServer=[&](const Cancel& token){
  for(;;){
   token.check();
   const auto now=GetTickCount64(),until=retryNotBefore.load();
   if(now>=until)return;
   token.wait(static_cast<int>(std::min<ULONGLONG>(250,until-now)));
  }
 };
 const auto recoverableConnectionError=[](DWORD code){
  return code==ERROR_WINHTTP_CANNOT_CONNECT||code==ERROR_WINHTTP_CONNECTION_ERROR||
         code==ERROR_WINHTTP_NAME_NOT_RESOLVED||code==ERROR_WINHTTP_TIMEOUT;
 };
 const auto rangeRequestRejected=[](const HttpRejected& rejection){
  return rejection.retryAfterMs==-1&&(rejection.status==403||rejection.status==405||rejection.status==501);
 };
 std::unique_ptr<Http> restartResponse;
 bool restartWithoutRanges=false,restartTried=false,repairTried=false;
 std::optional<size_t> repairNext;
 for(int generation=0;;++generation){
  const auto repairIndex=repairNext;
  repairNext.reset();
  std::unique_ptr<Http> initialResponse,initialHelper,resumedResponse;
  struct ResumeRange{size_t index;i64 partIndex,segmentStart,start,end,done,size;std::string tag,modified;};
  std::optional<ResumeRange> resumeRange;
  std::exception_ptr helperWaitRequiresReview;
  i64 helperBegin=0,helperEnd=0;
  enum class InitialRangeProbe { Unused, Pending, Verified, Unavailable };
  auto initialRangeProbe=InitialRangeProbe::Unused;
  bool verifyRangeOnReconnect=false;
  bool requireStrongDateOnReconnect=false;
  std::string initialRangeTag,initialRangeModified,initialRangeValidator;
  auto openHelper=[&](const Cancel& token,bool parallel,const std::string& tag,const std::string& modified,const std::string& validator,i64 size)->std::unique_ptr<Http>{
   // The full response still owns the file. Only admit a verified partial response.
   try{
    Json probePrefs=prefs;
    if(parallel){probePrefs["DisableHttp2"]=true;probePrefs["DisableHttp3"]=true;}
    waitForServer(token);
    const auto requestStart=helperBegin-std::min(helperBegin,httpOverlapLimit);
    auto response=std::make_unique<Http>(url,headers,probePrefs,token,requestStart,helperEnd,parallel?validator:"",nullptr,true,pool);
    if(response->status!=206){
     if(response->status>=400)throw HttpRejected(response->status,response->header(L"Retry-After"));
     return {};
    }
    const auto encoding=lower(response->header(L"Content-Encoding"));
    if(!encoding.empty()&&encoding!="identity")return {};
    std::optional<ContentRange> range;
    try{range.emplace(response->header(L"Content-Range"));}
    catch(const std::runtime_error&){return {};} // An overflowing offset cannot prove range support.
    if(!range->valid||range->start!=requestStart||range->end!=helperEnd||
       range->total!=size||etag(*response)!=tag||
       (tag.empty()&&response->header(L"Last-Modified")!=modified))return {};
    if(!parallel){
     BYTE byte;
     if(response->read(&byte,1,token)!=1||response->read(&byte,1,token)!=0)return {};
    }
    return response;
   }catch(const HttpRejected& rejection){
    if(rejection.transientStatus()){
     // A server delay governs the next request. The accepted response can keep running.
     rememberServerDelay(rejection);
     if(rejection.retryAfterMs==-2){Lock lock(m.mutex);helperWaitRequiresReview=std::current_exception();}
     else deferRequests(rejection.delay(0));
    }
   }catch(const InternetFailure&){}
   catch(const CurlConnectionFailure&){}
   catch(const CurlResponseDisconnect&){}
   catch(const CurlBodyDisconnect&){}
   token.check();
   return {};
  };
  bool fresh=false,preserveIdentity=false,refreshPending=false,protectSavedRefresh=false;int requestedConnections=1;
  bool resumeWithoutHttp2=false,resumeWithoutHttp3=false,catalogHasSavedBytes=false;
  std::string savedStrongDate;
  {
   Lock lock(m.mutex);const auto& data=job->data;const auto& segments=data["Segments"];
   refreshPending=yes(data,"RefreshPendingValidation");
   if(refreshPending){
    protectSavedRefresh=num(data,"Received")>0;
    if(!protectSavedRefresh){
     const bool consolidated=fs::exists(dataFile);
     for(const auto& segment:segments){
      const auto file=partPath(parts,num(segment,"Index"));
      if((consolidated&&num(segment,"Done")>0)||(fs::exists(file)&&fs::file_size(file)>0)){
       protectSavedRefresh=true;
       break;
      }
     }
    }
   }
   fresh=generation==0&&segments.empty()&&num(data,"Received")==0&&!refreshPending;
   preserveIdentity=yes(data,"PreserveResumeIdentity");
   requestedConnections=(int)std::clamp<i64>(num(data,"Connections",8),1,32);
   const auto size=num(data,"Size",-1);auto tag=str(data,"ETag"),modified=str(data,"Modified");
   const bool strongTag=tag.size()>=2&&tag.front()=='"'&&tag.back()=='"'&&
    std::all_of(tag.begin()+1,tag.end()-1,[](unsigned char c){return c==0x21||(c>=0x23&&c!=0x7f);});
   catalogHasSavedBytes=num(data,"Received")>0;
   if(generation==0&&catalogHasSavedBytes&&tag.empty()&&!modified.empty()&&str(data,"HttpStrongLastModified")==modified&&
      (refreshPending||preserveIdentity||!yes(prefs,"IgnoreLastModified")))savedStrongDate=modified;
   const bool strongIdentity=strongTag||!savedStrongDate.empty();
   const bool mayResume=yes(data,"RangeSupported")||!data.contains("RangeSupported");
   if(generation==0&&mayResume&&strongIdentity&&completePlan(segments,size)){
    for(size_t index=0;index<segments.size();++index){
     const auto& segment=segments[index];const auto start=num(segment,"Start"),end=num(segment,"End"),done=num(segment,"Done");
     if(done<0||done>=end-start+1)continue;
     resumeRange=ResumeRange{index,num(segment,"Index"),start,0,end,done,size,std::move(tag),std::move(modified)};
     resumeWithoutHttp2=requestedConnections>1||yes(data,"DisableHttp2");
     resumeWithoutHttp3=requestedConnections>1||yes(data,"DisableHttp3");
     break;
    }
   }
  }
  // Check the same local source that prepareDataFile will use. Old catalogs
  // can have a numbered part even when their saved Received count is zero.
  if(resumeRange){
   auto& range=*resumeRange;
   const auto part=partPath(parts,range.partIndex);
   if(fs::exists(part)){
    const auto length=fs::file_size(part);
    if(!length||length>=static_cast<uintmax_t>(range.end-range.segmentStart+1))resumeRange.reset();
    else range.done=static_cast<i64>(length);
   }else if(!catalogHasSavedBytes)resumeRange.reset();
   else{
    Handle file(CreateFileW(dataFile.c_str(),GENERIC_READ,FILE_SHARE_READ|FILE_SHARE_WRITE,nullptr,OPEN_EXISTING,0,nullptr));
    if(!file)resumeRange.reset();
    else range.done=retainedDataPrefix(file.h,fileLength(file.h),hasSparseAllocations(file.h),range.segmentStart,range.done,*cancel);
   }
   if(resumeRange){
    const auto overlap=std::min(range.done>0?range.done:range.segmentStart,httpOverlapLimit);
    range.start=range.segmentStart+range.done-overlap;
   }
  }
  std::optional<Json> resumePrefs;
  if(resumeRange){
   resumePrefs=prefs;
   if(resumeWithoutHttp2)(*resumePrefs)["DisableHttp2"]=true;
   if(resumeWithoutHttp3)(*resumePrefs)["DisableHttp3"]=true;
  }
  // A replacement without saved bytes can use its first full response.
  const bool replacementWithoutPartial=refreshPending&&!protectSavedRefresh;
  bool fullProbe=fresh||replacementWithoutPartial||restartWithoutRanges;
  {
   auto probe=std::move(restartResponse);
   auto retryConnection=[&](DWORD code,int attempt){
    if(attempt>=retryBudget||!recoverableConnectionError(code))return false;
    probe.reset();auto delay=std::min(10000,500*(1<<std::min(attempt,4)));
    {Lock lock(m.mutex);job->workers.assign(1,Worker{});job->workers[0].number=1;job->workers[0].state="Retrying connection";}
    cancel->wait(delay);return true;
   };
   if(!probe)for(int attempt=0;;++attempt){
    try{
     waitForServer(*cancel);
     const auto begin=fullProbe?std::nullopt:std::optional<i64>(resumeRange?resumeRange->start:0);
     const auto end=fullProbe?std::nullopt:std::optional<i64>(resumeRange?resumeRange->end:0);
     probe=std::make_unique<Http>(url,headers,resumeRange?*resumePrefs:prefs,*cancel,begin,end,resumeRange?resumeRange->tag:"",nullptr,true,pool);
     if(probe->status!=416)success(*probe);
     if(fullProbe&&probe->status==206)throw std::runtime_error("Unexpected partial response to a full download.");
     if(resumeRange){
      if(probe->status==200)resumeRange.reset();
      else{
       bool matches=false;
       if(probe->status==206)try{
        const ContentRange range(probe->header(L"Content-Range"));
        matches=range.valid&&range.start==resumeRange->start&&range.end==resumeRange->end&&
         range.total==resumeRange->size&&etag(*probe)==resumeRange->tag;
        if(matches&&resumeRange->tag.empty())
         matches=probe->header(L"Last-Modified")==resumeRange->modified&&strongModificationDate(*probe,resumeRange->modified);
       }catch(const std::runtime_error&){} // An overflowing or unreadable offset cannot prove identity.
       if(!matches){
        // A replacement must establish the saved range and identity before touching the existing parts.
        if(refreshPending||(!savedStrongDate.empty()&&preserveIdentity))throw std::runtime_error(unsafeRefresh);
        // A rejected continuation needs a clean body from the beginning.
        fullProbe=true;
        resumeRange.reset();probe.reset();--attempt;continue;
       }
      }
     }else if(probe->status==206){
      ContentRange range(probe->header(L"Content-Range"));
      if(!range.valid||range.start!=0||range.end!=0||range.total<=0)throw std::runtime_error("Server returned an invalid probe range.");
      if(!savedStrongDate.empty()&&(probe->header(L"Last-Modified")!=savedStrongDate||!strongModificationDate(*probe,savedStrongDate))){
       if(refreshPending||preserveIdentity)throw std::runtime_error(unsafeRefresh);
       fullProbe=true;probe.reset();--attempt;continue;
      }
      // A probe is successful only after its one-byte body is received intact.
      if(probe->all(1,*cancel).size()!=1)throw InternetFailure("Incomplete probe range",ERROR_WINHTTP_CONNECTION_ERROR);
     }
     break;
    }catch(const InternetFailure& error){
     if(!retryConnection(error.code,attempt))throw;
    }catch(const CurlConnectionFailure& error){
     if(!retryConnection(error.code,attempt))throw;
    }catch(const CurlResponseDisconnect&){
     if(!retryConnection(ERROR_WINHTTP_CONNECTION_ERROR,attempt))throw;
    }catch(const CurlBodyDisconnect&){
     if(!retryConnection(ERROR_WINHTTP_CONNECTION_ERROR,attempt))throw;
    }catch(const BrowserVerificationRequired&){throw;}
    catch(const HttpRejected& error){
     rememberServerDelay(error);
     probe.reset();
     if(resumeRange&&!refreshPending&&!preserveIdentity&&rangeRequestRejected(error)){
      // A plain range refusal can still allow a full response. Keep the saved
      // plan and bytes until that response succeeds.
      fullProbe=true;
      resumeRange.reset();
      --attempt;
      continue;
     }
     if(!error.retryable()||attempt>=retryBudget)throw;
     auto delay=error.delay(attempt);
     {Lock lock(m.mutex);job->workers.assign(1,Worker{});job->workers[0].number=1;job->workers[0].state="Server busy; retrying in "+std::to_string((delay+999)/1000)+" s";}
     cancel->wait(delay);
    }
   }
   auto& response=*probe;
   ContentRange range(response.header(L"Content-Range"));
   bool empty=response.status==416&&range.total==0;
   if(!empty)success(response);
   bool ranges=response.status==206&&range.valid&&range.total>0&&
    range.start==(resumeRange?resumeRange->start:0)&&range.end==(resumeRange?resumeRange->end:0);
   if(response.status==206&&!ranges)throw std::runtime_error("Server returned an invalid probe range.");
   i64 size=empty?0:ranges?range.total:length(response.header(L"Content-Length"));
   auto tag=etag(response),modified=response.header(L"Last-Modified");
   const bool strongDate=!modified.empty()&&tag.empty()&&strongModificationDate(response,modified);
    // Repr-Digest and legacy Digest cover the full representation, even on
    // a 206. Content-Digest is a whole-file check only on the full GET.
    auto serverSha256=hasher&&(response.status==200||response.status==206)?
     responseSha256(response,L"Repr-Digest"):std::string{};
    if(hasher&&serverSha256.empty()&&response.status==200)
     serverSha256=responseSha256(response,L"Content-Digest");
    if(hasher&&serverSha256.empty()&&(response.status==200||response.status==206))
     serverSha256=responseSha256(response,L"Digest");
   bool validator=!tag.empty()||!modified.empty();
   // Keep the full first response alive while establishing whether helpers may
   // open ranges. A one-use URL may reject that extra request; its original body
   // remains valid and can finish through one worker.
   const auto acceptRanges=lower(trim(response.header(L"Accept-Ranges")));
   if(replacementWithoutPartial&&response.status==200&&size>0&&acceptRanges!="none"&&
      (!tag.empty()||strongDate)){
    // Keep the replacement on one request unless its body is interrupted.
    verifyRangeOnReconnect=true;
    requireStrongDateOnReconnect=tag.empty();
   }else if(fresh&&response.status==200&&size>0&&validator&&acceptRanges!="none"){
    const bool advertised=acceptRanges=="bytes";
    const auto count=freshHttpConnectionLimit(size,requestedConnections);
     if(advertised&&count==1)ranges=true;
     else if(acceptRanges.empty()&&count==1&&
             (!tag.empty()||(strongDate&&(!yes(prefs,"IgnoreLastModified")||preserveIdentity)))){
      // A single worker already has the full response. If it breaks, verify
      // the first resumed range and its overlap before keeping any new bytes.
      verifyRangeOnReconnect=true;
      requireStrongDateOnReconnect=tag.empty();
     }
    else {
     if(count>1){
      // Leave the full response enough bytes for the larger half of an odd
      // worker set. Divide before multiplying to keep large file offsets safe.
      const auto helperWorkers=count/2;
      const auto helperBytes=(size/count)*helperWorkers+(size%count)*helperWorkers/count;
      helperBegin=size-helperBytes;
      helperEnd=size-1;
     }
     auto helperValidator=tag;
     if(helperValidator.empty()&&(!yes(prefs,"IgnoreLastModified")||preserveIdentity))helperValidator=modified;
     const bool mayResumeSingle=advertised&&!helperValidator.empty();
     if(count>1&&!helperValidator.empty()){
      // The full response can write its known span immediately. An
      // unadvertised server still needs a verified 206 before any range is used.
      ranges=mayResumeSingle;initialRangeProbe=InitialRangeProbe::Pending;
      initialRangeTag=tag;initialRangeModified=modified;initialRangeValidator=helperValidator;
     }else{
      auto check=openHelper(*cancel,count>1,tag,modified,helperValidator,size);
      ranges=check!=nullptr;
      // The worker consumes the validated data response through the normal rate and resume path.
      if(count>1)initialHelper=std::move(check);
     }
     cancel->check();
    }
   }
   Json plan;bool resetParts=false,startWithoutCheckpoint=false;
   {
   Lock lock(m.mutex);auto& data=job->data;
   const bool ignoreDate=yes(prefs,"IgnoreLastModified")&&!yes(data,"RefreshPendingValidation")&&!yes(data,"PreserveResumeIdentity");
   bool same=num(data,"Size",-1)==size&&(!tag.empty()?str(data,"ETag")==tag:str(data,"ETag").empty()&&!modified.empty()&&!str(data,"Modified").empty()&&(ignoreDate||str(data,"Modified")==modified));
   auto& segments=data["Segments"];
   const bool savedBytes=num(data,"Received")>0||protectSavedRefresh;
   if((protectSavedRefresh||preserveIdentity)&&savedBytes&&(!same||!ranges||!validator||!completePlan(segments,size)))
    throw std::runtime_error(unsafeRefresh);
   const bool resetPlan=(generation!=0&&!repairIndex)||!same||!ranges||!validator||!completePlan(segments,size);
   resetParts=resetPlan&&(!segments.empty()||savedBytes||!fs::is_empty(parts));
   startWithoutCheckpoint=fresh&&resetPlan&&!resetParts;
   auto before=resetParts?data:Json{};
   try{
   data["ContentType"]=previewMimeType(response.header(L"Content-Type"));data["ProtectedResolvedUrl"]=response.finalUrl!=url?protect(response.finalUrl):"";data["ResponseFileName"]=contentDispositionName(response.header(L"Content-Disposition"),"");data["ResponseFileNameUrl"]=url;
   if(resetPlan){
    segments=Json::array();data["Received"]=0;data["DynamicSplits"]=0;
   }
    if(!serverSha256.empty())data["ServerSha256"]=serverSha256;
    else if(!same)data.erase("ServerSha256");
   data["Size"]=size;data["ETag"]=tag.empty()?Json():Json(tag);data["Modified"]=modified.empty()?Json():Json(modified);
   if(strongDate&&(!savedBytes||resetPlan||str(data,"HttpStrongLastModified")==modified))
    data["HttpStrongLastModified"]=modified;
   else data.erase("HttpStrongLastModified");
   if(verifyRangeOnReconnect)data.erase("RangeSupported");
   else data["RangeSupported"]=ranges&&validator;
   if(segments.empty()&&size!=0){
    // Seed only the responses already open. Additional workers split an owned
    // range when they need work, so their offsets use current progress.
    segments.push_back({{"Index",0},{"Start",0},{"End",initialHelper?helperBegin-1:size-1},{"Done",0}});
    if(initialHelper)segments.push_back({{"Index",1},{"Start",helperBegin},{"End",helperEnd},{"Done",0}});
   }
   plan=segments;
   if(repairIndex&&!resetPlan){
    auto& repaired=plan.at(*repairIndex);
    repaired["Done"]=0;
    repaired.erase("HttpBoundarySha256");
   }
   // A pending reset prevents an interrupted restart from adopting stale
   // legacy parts under the new range map.
   if(resetParts){data["HttpPartsResetPending"]=true;m.save();}
   }catch(...){if(resetParts)data=std::move(before);throw;}
   }
   if(resetParts)clearParts(parts);
   // File work happens outside the catalog lock: folding an older layout into the
   // single file may copy a lot of data. No worker exists yet, so the plan is ours.
   const auto migrated=prepareDataFile(parts,job->id(),size,plan,*cancel);
   cancel->check();
   {
    Lock lock(m.mutex);auto& data=job->data;data["Segments"]=plan;
    i64 received=0;for(const auto& segment:plan)received+=num(segment,"Done");
    data["Received"]=received;
    if(resetParts)data.erase("HttpPartsResetPending");
    // A fresh job with no old parts already has its address and destination
    // on disk. Until the periodic or pause save, recovery starts from zero.
    // Saved plans, resets and migrations must still commit before writing.
    if(!startWithoutCheckpoint||!migrated.empty())
     try{m.save();}catch(...){if(resetParts)data["HttpPartsResetPending"]=true;throw;}
   }
   // Until the new offsets are committed, the legacy files remain the
   // recovery source. Finish cleanup before any worker can write newer bytes.
   for(const auto& part:migrated)fs::remove(part);
   // A full response may be the only usable response for this URL. Preserve
   // that unread body for the single worker rather than issuing another GET.
   if(response.status==200)initialResponse=std::move(probe);
   else if(resumeRange&&!resetParts&&resumeRange->index<plan.size()){
    const auto& segment=plan[resumeRange->index];
    const auto start=num(segment,"Start"),done=num(segment,"Done");
    const auto overlap=std::min(done>0?done:start,httpOverlapLimit);
    // A truncated or migrated part can change the saved offset during disk validation.
    if(done==resumeRange->done&&start+done-overlap==resumeRange->start&&num(segment,"End")==resumeRange->end)
     resumedResponse=std::move(probe);
   }
  }

  auto group=std::make_shared<Cancel>();group->parent=cancel;
  Cancel initialRangeCancel;initialRangeCancel.parent=group;
  // The prefix hash reads only bytes covered by the catalog. Stop it between
  // chunks once the range workers return, preserving the completed prefix.
  Cancel hashCancel;hashCancel.parent=group;
  std::mutex hashIdleMutex;
  std::condition_variable hashIdle;
  std::thread hashing;
  if(hasher){hasher->reset();hashing=std::thread([&]{try{
   PrefixHasher::Reader reader;
   while(!hashCancel.cancelled()){
    Json segments;
    {
     // A checkpoint can hold the manager lock for disk I/O. Hashing is
     // optional during transfer and must remain stoppable during that wait.
     std::unique_lock lock(m.mutex,std::try_to_lock);
     if(lock.owns_lock()&&!hashCancel.cancelled())segments=job->data["Segments"];
    }
    const auto limit=segments.is_array()?PrefixHasher::contiguous(std::move(segments),hasher->hashed):hasher->hashed;
    if(limit>hasher->hashed)
     hasher->advance(reader,dataFile,std::min(limit,hasher->hashed+(i64)(16*assemblyBuffer)),hashCancel);
    else{
     std::unique_lock lock(hashIdleMutex);
     hashIdle.wait_for(lock,std::chrono::milliseconds(20),[&]{return hashCancel.cancelled();});
    }
   }
  }catch(const Cancelled&){}catch(...){hasher->valid=false;}});}
  struct JoinHasher{
   std::thread& thread;
   Cancel& cancel;
   std::mutex& mutex;
   std::condition_variable& idle;
   void stop(){
    {std::lock_guard<std::mutex> lock(mutex);cancel.stop=true;}
    idle.notify_one();
   }
   void join(){if(thread.joinable())thread.join();}
   ~JoinHasher(){stop();join();}
  } joinHasher{hashing,hashCancel,hashIdleMutex,hashIdle};
  // Owners and all boundary/progress changes are protected by Manager::mutex.
  // -1 = unassigned, -2 = complete; nonnegative values identify an active worker.
  std::vector<int> owners;int connections,retries;bool ranges;
  // A range tail written in this attempt can prove the adjacent HTTP overlap
  // without rereading the staging file. Resume and repair start with no proofs.
  std::map<i64,std::array<BYTE,httpOverlapLimit>> writtenTails;
  {
   Lock lock(m.mutex);ranges=verifyRangeOnReconnect||yes(job->data,"RangeSupported")||initialRangeProbe==InitialRangeProbe::Pending;
   connections=ranges&&!verifyRangeOnReconnect?(int)std::clamp<i64>(num(job->data,"Connections",8),1,32):1;
   if(fresh&&ranges)connections=(int)freshHttpConnectionLimit(num(job->data,"Size"),connections);
   retries=m.retries(str(job->data,"Queue"));
   for(const auto& segment:job->data["Segments"]){auto end=num(segment,"End"),begin=num(segment,"Start");
    owners.push_back(end>=begin&&num(segment,"Done")==end-begin+1?-2:-1);
   }
   job->workers.assign(connections,Worker{});for(int i=0;i<connections;++i)job->workers[i].number=i+1;
  }
  std::vector<std::chrono::steady_clock::time_point> assigned(connections);
  std::vector<i64> receivedAtAssignment(connections,0);
  // A peer is healthy only while its current response is making progress.
  // Protected by Manager::mutex like owners.
  std::vector<ULONGLONG> workerProgressTick(connections,0);
  std::condition_variable_any rangeWorkChanged;
  // End of the bytes a worker is writing outside the lock; a split never lands
  // inside them, so no two workers ever own the same byte. Protected by Manager::mutex.
  std::vector<i64> inflightEnd(connections,0);
  std::vector<unsigned char> acceptedResponses(connections,0);
  std::vector<i64> responseEnd(connections,-1);
  std::vector<size_t> splitParent(connections,SIZE_MAX);
  // Failed opens compete for one retry slot per transfer. Successful workers
  // can keep claiming ranges and reusing their connections.
  std::chrono::steady_clock::time_point lastConnectProbe{},recoveryRampUntil{};
  std::chrono::steady_clock::time_point deferRangeExpansionUntil{};
  bool recoveryRampPending=false,rangeExpansionDenied=false;
  struct ResponseActivity{
   Manager& manager;
   ULONGLONG& progressTick;
   i64& writeEnd;
   unsigned char& accepted;
   i64& responseEnd;
   ~ResponseActivity(){Lock lock(manager.mutex);progressTick=0;writeEnd=0;accepted=0;responseEnd=-1;}
  };
  // The caller uses the same lock for claiming and entering a wait so a
  // range becoming available cannot be missed between those operations.
  auto claim=[&](int worker,size_t lastCompleted,bool responseDrained)->size_t{
   group->check();auto& segments=job->data["Segments"];
   if((initialRangeProbe==InitialRangeProbe::Pending||initialRangeProbe==InitialRangeProbe::Unavailable)&&worker!=0)return SIZE_MAX;
   const auto now=std::chrono::steady_clock::now();
   // Once a fresh transfer has an accepted range, try one additional open at
   // a time. A worker that just finished can still reuse its connection, and
   // a transfer with no accepted response keeps the normal recovery pacing.
   if(fresh&&ranges&&!responseDrained&&
      std::any_of(acceptedResponses.begin(),acceptedResponses.end(),[](unsigned char accepted){return accepted!=0;})){
    if(now<deferRangeExpansionUntil||std::any_of(owners.begin(),owners.end(),[&](int owner){return owner>=0&&!acceptedResponses[owner];}))
     return SIZE_MAX;
   }
   auto speed=[&](int owner)->double{
    if(assigned[owner]==std::chrono::steady_clock::time_point{})return 0;
    double seconds=std::chrono::duration<double>(now-assigned[owner]).count();
    i64 bytes=job->workers[owner].received-receivedAtAssignment[owner];
    return seconds>=0.4&&bytes>=65536?bytes/seconds:0;
   };
   auto assignedTo=[&](size_t index){owners[index]=worker;splitParent[worker]=SIZE_MAX;assigned[worker]=now;receivedAtAssignment[worker]=job->workers[worker].received;return index;};
   size_t pending=SIZE_MAX;
   const i64 nextStart=ranges&&lastCompleted!=SIZE_MAX?num(segments[lastCompleted],"End")+1:-1;
   for(size_t i=0;i<owners.size();++i){
    const auto owner=owners[i];
    // A finished connection takes the adjacent unstarted range before splitting.
    // The current owner may already have sent its request while waiting for headers.
    if(nextStart>=0&&owner==-1&&num(segments[i],"Start")==nextStart&&num(segments[i],"Done")==0){
     return assignedTo(i);
    }
    if(owner==-1){
     if(pending==SIZE_MAX)pending=i;
     if(nextStart<0)break;
    }
   }
   if(!ranges||rangeExpansionDenied||segments.size()>=4096)return pending==SIZE_MAX?SIZE_MAX:assignedTo(pending);
   size_t largest=SIZE_MAX;
   i64 boundary=0,largestRemaining=0;
   const auto helperSpeed=speed(worker);
   const i64 workerShare=num(job->data,"Size",0)/connections;
   const i64 minimumHalf=std::clamp<i64>(workerShare/8,262144,2*1048576);
   const bool powerOfTwo=(connections&(connections-1))==0;
   const bool balanceFreshWorkers=fresh&&!powerOfTwo&&segments.size()<static_cast<size_t>(connections);
   // Allow the initial connection assignments and one ordinary tail split.
   // After that, keep the healthy responses that already cover the file.
   // A stalled or demonstrably slower response can still get help.
   const bool healthyBudgetUsed=fresh&&segments.size()>=static_cast<size_t>(connections+1);
   constexpr auto tailStall=std::chrono::milliseconds(750);
   const auto tick=GetTickCount64();
   for(size_t i=0;i<owners.size();++i)if(owners[i]>=0&&acceptedResponses[owners[i]]){
    const auto& s=segments[i];
    const auto owner=owners[i];
    // The reserved write belongs to the original worker. Bisect only the
    // bytes still available to either connection.
    const i64 first=std::max(num(s,"Start")+num(s,"Done"),inflightEnd[owner]);
    const i64 remaining=num(s,"End")-first+1;
    if(remaining<=largestRemaining)continue;
    const double ownerSpeed=speed(owner);
    const bool earlyHelp=ownerSpeed>0&&helperSpeed>=ownerSpeed*2&&remaining/ownerSpeed>=0.75;
    if(pending!=SIZE_MAX&&!earlyHelp)continue;
    if(remaining<2*(earlyHelp?65536:minimumHalf))continue;
    if(healthyBudgetUsed&&!earlyHelp){
     const auto progress=workerProgressTick[owner];
     const bool stalled=progress?tick-progress>=static_cast<ULONGLONG>(tailStall.count()):now-assigned[owner]>=tailStall;
     if(!stalled)continue;
    }
    largestRemaining=remaining;
    largest=i;
    // While filling the initial worker set, hand out at most one worker's
    // share when the connection count cannot be reached by doubling.
    const auto helperBytes=balanceFreshWorkers&&!earlyHelp
     ?std::min(remaining/2,workerShare):remaining/2;
    boundary=first+remaining-helperBytes;
   }
   if(largest==SIZE_MAX)return pending==SIZE_MAX?SIZE_MAX:assignedTo(pending);
   const auto oldEnd=num(segments[largest],"End");
   i64 partId=0;for(const auto& s:segments)partId=std::max(partId,num(s,"Index")+1);
   if(partId>100000)return SIZE_MAX;
   // The split only divides bytes past the committed and reserved writes.
   // An older checkpoint can still resume the unsplit range at its saved Done.
   // The periodic checkpoint and Pause save the live plan with its progress.
   auto previousSplits=num(job->data,"DynamicSplits");
   owners.push_back(worker);
   try{
    segments.push_back({{"Index",partId},{"Start",boundary},{"End",oldEnd},{"Done",0}});
    segments[largest]["End"]=boundary-1;job->data["DynamicSplits"]=previousSplits+1;
   }catch(...){
    if(segments.size()==owners.size())segments.erase(segments.end()-1);
    owners.pop_back();segments[largest]["End"]=oldEnd;job->data["DynamicSplits"]=previousSplits;throw;
   }
   job->workers[owners[largest]].end=boundary-1;
   const auto index=assignedTo(owners.size()-1);
   splitParent[worker]=largest;
   return index;
  };

  // A refused extra connection may return its untouched tail to the response
  // already carrying those bytes. Only remove the last slot so no live index
  // can shift. A checkpoint that still contains the untouched tail can resume
  // it from zero. The caller holds the manager lock through the change.
  const auto restoreRefusedSplit=[&](size_t index,int worker){
   const auto parent=splitParent[worker];
   if(parent==SIZE_MAX||index!=owners.size()-1||owners[parent]<0)return false;
   auto& data=job->data;
   auto& segments=data["Segments"];
   const auto donor=owners[parent];
   const auto end=num(segments[index],"End");
   if(!acceptedResponses[donor]||responseEnd[donor]<end||num(segments[index],"Done")!=0||
      num(segments[parent],"End")+1!=num(segments[index],"Start"))return false;
   const auto splits=num(data,"DynamicSplits");
   segments[parent]["End"]=end;
   segments.erase(segments.end()-1);
   data["DynamicSplits"]=splits-1;
   owners.pop_back();
   job->workers[donor].end=end;
   return true;
  };

  std::mutex errorsMutex;std::exception_ptr error;bool changed=false,overlapMismatch=false,repairable=true;
  std::optional<size_t> repairCandidate;
  auto recordError=[&](bool resourceChanged,bool overlap=false,std::optional<size_t> range={}){std::lock_guard<std::mutex> lock(errorsMutex);
   if(overlap||!error||(resourceChanged&&!overlapMismatch))error=std::current_exception();
   if(resourceChanged){
    if(!range||(repairCandidate&&repairCandidate!=range))repairable=false;
    else repairCandidate=range;
   }
   overlapMismatch|=overlap;changed|=resourceChanged;group->stop=true;rangeWorkChanged.notify_all();
  };
  auto admitInitialRange=[&]{
   std::unique_ptr<Http> response;
   try{response=openHelper(initialRangeCancel,true,initialRangeTag,initialRangeModified,initialRangeValidator,helperEnd+1);}
   catch(const Cancelled&){group->check();}
   Lock lock(m.mutex);group->check();
   auto& data=job->data;auto& segments=data["Segments"];
   const auto owner=owners[0];
   if(!response||owner==-2){
    initialRangeProbe=InitialRangeProbe::Unavailable;
    rangeWorkChanged.notify_all();
    return;
   }
   if(num(segments[0],"Done")>helperBegin||(owner>=0&&inflightEnd[owner]>helperBegin)){
    // The response proves ranges work, but its original start is behind the
    // primary worker. Let the workers split the current remainder normally.
    data["RangeSupported"]=true;
    initialRangeProbe=InitialRangeProbe::Verified;
    rangeWorkChanged.notify_all();
    return;
   }
   // Like a live split, this only divides unwritten bytes. The existing
   // checkpoint can still recover the unsplit range. Let the periodic or
   // pause checkpoint persist the new plan without blocking active workers.
   owners.push_back(-1);
   try{
    segments.push_back({{"Index",1},{"Start",helperBegin},{"End",helperEnd},{"Done",0}});
    data["RangeSupported"]=true;
    segments[0]["End"]=helperBegin-1;
   }catch(...){
    if(segments.size()==owners.size())segments.erase(segments.end()-1);
    owners.pop_back();throw;
   }
   if(owner>=0)job->workers[owner].end=helperBegin-1;
   initialHelper=std::move(response);
   initialRangeProbe=InitialRangeProbe::Verified;
   rangeWorkChanged.notify_all();
  };
  std::vector<std::thread> workers;
  // Tick of the last byte any worker received on this download; 0 before the first.
  std::atomic<ULONGLONG> lastProgressTick{0},firstProgressTick{0};
  // Call under the manager lock, which also protects ownership transitions.
  const auto unfinished=[&]{return std::any_of(owners.begin(),owners.end(),[](int owner){return owner!=-2;});};
  const auto checkInterval=std::chrono::milliseconds(50);
  auto workerBody=[&](int worker){
   auto& activity=job->workers[worker];
   // Consecutive failures without a byte, kept across ranges; a megabyte of progress restores the budget.
   int failures=0,releases=0,probeReleases=0;
   // A kept-alive connection the server closed after its previous response answers the next
   // request with nothing; like a browser, retry that once at once on a fresh connection
   // before treating empty replies as an outage. previousRequestBytes>0 marks such a connection.
   i64 previousRequestBytes=0;bool staleRetried=false,responseDrained=false;
   // Two read buffers, owned by the worker so they outlive every request handle: a read
   // still outstanding when a request closes completes into memory that is still ours.
   std::vector<BYTE> bufferStorage(2*transferReadBuffer);
   auto waitForConnectProbe=false;
   auto othersHealthy=[&]{
    Lock lock(m.mutex);
    const auto now=GetTickCount64();
    for(int w=0;w<connections;++w)if(w!=worker&&workerProgressTick[w]&&now-workerProgressTick[w]<outageProbeWindow)return true;
    return false;
   };
   size_t lastCompleted=SIZE_MAX;
   try{
    bool probeInitialRange;
    {Lock lock(m.mutex);probeInitialRange=worker==1&&initialRangeProbe==InitialRangeProbe::Pending;}
    if(probeInitialRange)admitInitialRange();
    for(;;){
     size_t index;
     {
      std::unique_lock lock(m.mutex);
      if(waitForConnectProbe){
       const bool peerAccepted=std::any_of(acceptedResponses.begin(),acceptedResponses.end(),[](unsigned char accepted){return accepted!=0;});
       const auto now=std::chrono::steady_clock::now();
       const auto lastByte=lastProgressTick.load();
       // After a genuine stall, the first recovered response should restore
       // parallelism quickly. A brief gap between healthy ranges does not qualify.
       if(!peerAccepted&&lastByte&&GetTickCount64()-lastByte>=outageProbeInterval){
        recoveryRampPending=true;
        deferRangeExpansionUntil={};
       }
       if(peerAccepted&&recoveryRampPending){recoveryRampPending=false;recoveryRampUntil=now+recoveryRampDuration;}
       auto interval=std::chrono::milliseconds(outageProbeInterval);
       if(peerAccepted)interval=now<recoveryRampUntil?recoveryRampInterval:busyRangeProbeInterval;
       const auto probeAt=lastConnectProbe+interval;
       if(now<probeAt&&unfinished()){
        group->check();
        rangeWorkChanged.wait_until(lock,std::min(probeAt,now+checkInterval));
        continue;
       }
      }
      index=claim(worker,lastCompleted,responseDrained);
      if(index==SIZE_MAX){
       // A failed connection can return its range to the pool. The timeout
       // also covers external cancellation and time-based split eligibility.
       if((initialRangeProbe==InitialRangeProbe::Unavailable&&worker!=0)||!unfinished())break;
       activity.state="Waiting";
       rangeWorkChanged.wait_for(lock,checkInterval);
       continue;
      }
      if(waitForConnectProbe){lastConnectProbe=std::chrono::steady_clock::now();waitForConnectProbe=false;}
     }
     lastCompleted=SIZE_MAX;
     std::string lastProtocol;
     // A connection that broke after delivering bytes is reopened at once; after a
     // megabyte it also restores the retry budget, so a server that resets every
     // connection after N MB finishes instead of failing after N*retries MB.
     // Retries=0 still means no reconnection. A failure without any byte while
     // another worker is receiving points at a server-side connection limit: the
     // range goes back to the pool for the working connections and this worker
     // backs off, for longer once its budget is spent, instead of failing the download.
     auto release=[&](std::optional<int> pauseMs,bool coordinateConnect=false){
      {
       Lock lock(m.mutex);
       if(!coordinateConnect||!fresh||!restoreRefusedSplit(index,worker))owners[index]=-1;
       activity.state="Waiting for a free connection";
      }
      waitForConnectProbe=coordinateConnect;
      rangeWorkChanged.notify_all();
      throw RangeReleased{pauseMs};
     };
     auto paceReconnect=[&](i64 progressed,bool failedConnect=false)->bool{
      if(progressed==0&&previousRequestBytes>0&&responseDrained&&!staleRetried){staleRetried=true;responseDrained=false;previousRequestBytes=0;{Lock lock(m.mutex);activity.state="Reconnecting";}return true;}
      responseDrained=false;
      if(fresh&&progressed==0&&failedConnect){
       Lock lock(m.mutex);
       // Prevent other idle workers from splitting or claiming extra ranges
       // just as this failed open returns its range to the accepted workers.
       if(std::any_of(acceptedResponses.begin(),acceptedResponses.end(),[](unsigned char accepted){return accepted!=0;}))
        deferRangeExpansionUntil=std::chrono::steady_clock::now()+busyRangeProbeInterval;
      }
      if(progressed>0){previousRequestBytes=progressed;staleRetried=false;}else previousRequestBytes=0;
      const auto recent=lastProgressTick.load();
      const bool probing=progressed==0&&retries>0&&recent&&GetTickCount64()-recent<outageProbeWindow;
      if(progressed>=reconnectProgressReset&&retries>0)failures=0;else if(!probing)++failures;
      // Only a connection that never delivered a byte is handed to the other workers,
      // and never with Retries=0, which disables reconnection altogether.
      if(failures>retries){if(progressed>0||retries<=0||!othersHealthy())return false;release(std::min(30000,2000*(1<<std::min(releases++,4))),failedConnect);}
      if(progressed>0){probeReleases=0;Lock lock(m.mutex);activity.state="Reconnecting";return true;}
      // 1, 2, 4, 8 then 15 s: the default three retries ride out a short outage.
      auto delay=std::min(15000,1000*(1<<std::min(std::max(failures,1)-1,4)));
      // Early in an outage the other workers still look healthy (their reads have not
      // failed yet), so the first three releases pause one second; after them a server
      // that refuses extra connections gets the 1, 2, 4, 8 then 15 s backoff as before.
      if(probing){if(othersHealthy()){const int pause=probeReleases<3?1000:std::min(15000,1000*(1<<std::min(probeReleases-3,4)));++probeReleases;release(pause,failedConnect);}if(failedConnect)release((int)outageProbeInterval,true);{Lock lock(m.mutex);activity.state="Reconnecting";}group->wait((int)outageProbeInterval);return true;}
      if(othersHealthy()||failedConnect)release(delay,failedConnect);
      {Lock lock(m.mutex);activity.state="Retrying";}
      group->wait(delay);return true;
     };
     try{
     for(int attempt=0;;++attempt){
      group->check();lastProtocol.clear();i64 readBytes=0;bool openingFailed=false;
      try{
       Json segment,data;i64 have,need;
       {
        Lock lock(m.mutex);data=job->data;segment=data["Segments"][index];
        if(!ranges&&attempt>0)job->data.erase("PrefetchedFinalSize");
        // The catalog's Done is the resume offset; a full download restarts at zero.
        have=ranges?std::max<i64>(0,num(segment,"Done")):0;
        need=num(segment,"End")>=num(segment,"Start")?num(segment,"End")-num(segment,"Start")+1:-1;
        if(need>=0&&have>need)throw std::runtime_error("Saved progress exceeds its assigned range.");
        auto& actual=job->data["Segments"][index];job->data["Received"]=num(job->data,"Received")+have-num(actual,"Done");actual["Done"]=have;
        activity.start=num(segment,"Start");activity.end=num(segment,"End");activity.position=activity.start+have;
       }
       if(need>=0&&have==need)break;
       const bool adjacentOverlap=ranges&&have==0&&num(segment,"Start")>0;
       const auto overlap=static_cast<size_t>(std::min(adjacentOverlap?num(segment,"Start"):have,httpOverlapLimit));
       const auto requestStart=num(segment,"Start")+have-(i64)overlap;
       const bool dateResume=requireStrongDateOnReconnect||!savedStrongDate.empty();
       auto validator=str(data,"ETag");
       if(!dateResume&&!protectSavedRefresh&&validator.empty()&&(!yes(prefs,"IgnoreLastModified")||preserveIdentity))
        validator=str(data,"Modified");
       const bool retainedInitial=index==0&&have==0&&initialResponse!=nullptr;
       std::unique_ptr<Http> ownedResponse;
       if(retainedInitial)ownedResponse=std::move(initialResponse);
       else if(resumeRange&&index==resumeRange->index&&resumedResponse){
        if(requestStart==resumeRange->start&&num(segment,"End")==resumeRange->end)ownedResponse=std::move(resumedResponse);
        else resumedResponse.reset();
       }
       else if(index==1&&initialHelper){
        if(have==0&&num(segment,"Start")==helperBegin&&num(segment,"End")<=helperEnd){
         ownedResponse=std::move(initialHelper);
         // Validate the open response against the requested end. The read
         // loop still observes any shortened ownership in the live map.
         segment["End"]=helperEnd;
         need=helperEnd-helperBegin+1;
        }
        else initialHelper.reset();
       }
       // Parallel ranges each get their own HTTP/1.1 connection. Over HTTP/2 WinHTTP
       // multiplexes every range onto one TCP connection, so a per-connection limit
       // at the origin or one congestion window caps the whole download (measured
       // at 1.6 MB/s against 4.1 MB/s on the same CDN, docs/speed-benchmark-2026-10-18.md).
       Json requestPrefs=prefs;if(yes(data,"DisableHttp2")||connections>1)requestPrefs["DisableHttp2"]=true;if(yes(data,"DisableHttp3")||connections>1)requestPrefs["DisableHttp3"]=true;
       if(!ownedResponse){
        {
         std::unique_lock lock(m.mutex);
         while(initialRangeProbe==InitialRangeProbe::Pending){
          group->check();
          rangeWorkChanged.wait_for(lock,checkInterval);
         }
         if(helperWaitRequiresReview)std::rethrow_exception(helperWaitRequiresReview);
         if(ranges&&!verifyRangeOnReconnect&&!yes(job->data,"RangeSupported"))
          throw Changed("The initial response ended before range support could be verified.");
        }
        waitForServer(*group);
       }
       {Lock lock(m.mutex);activity.state="Connecting";}
       if(!ownedResponse){
        try{
         ownedResponse=std::make_unique<Http>(url,headers,requestPrefs,*group,ranges?std::optional<i64>(requestStart):std::nullopt,
           ranges?std::optional<i64>(num(segment,"End")):std::nullopt,ranges?validator:"",nullptr,true,pool);
        }catch(...){
         group->check();
         openingFailed=true;
         throw;
        }
        group->check();
       }
       ResponseActivity responseActivity{m,workerProgressTick[worker],inflightEnd[worker],acceptedResponses[worker],responseEnd[worker]};
       auto& response=*ownedResponse;lastProtocol=response.protocol();{Lock lock(m.mutex);job->data["Protocol"]=lastProtocol;} // the negotiated protocol, for the speed notes
       if(ranges&&!retainedInitial){
        if(response.status==200||response.status==416){
         if(response.status==200&&!restartTried&&!preserveIdentity&&!protectSavedRefresh){
          const auto encoding=lower(response.header(L"Content-Encoding"));
          if((encoding.empty()||encoding=="identity")&&response.header(L"Content-Range").empty()){
           std::lock_guard<std::mutex> lock(errorsMutex);
           if(!restartResponse)restartResponse=std::move(ownedResponse);
          }
         }
         throw Changed("The server changed the file or stopped honoring byte ranges.");
        }
        success(response);ContentRange cr(response.header(L"Content-Range"));
        if(response.status!=206||!cr.valid||cr.start!=requestStart||cr.end!=num(segment,"End")||cr.total!=num(data,"Size"))
         throw Changed("The server returned a mismatched byte range.");
        auto remote=etag(response),modified=response.header(L"Last-Modified");
        if((dateResume&&(modified!=str(data,"Modified")||!strongModificationDate(response,modified)))||
           (!str(data,"ETag").empty()&&remote!=str(data,"ETag"))||
           (str(data,"ETag").empty()&&(!yes(prefs,"IgnoreLastModified")||preserveIdentity||protectSavedRefresh)&&modified!=str(data,"Modified")))throw Changed("The remote file changed during transfer.");
       }else if(!ranges){
        success(response);if(response.status==206)throw std::runtime_error("Unexpected partial response to a full download.");
        need=length(response.header(L"Content-Length"));Lock lock(m.mutex);job->data["Size"]=need;job->data["Segments"][index]["End"]=need-1;activity.end=need-1;
       }
       if(retainedInitial&&ranges)need=num(data,"Size");
       {Lock lock(m.mutex);activity.receiveStart=num(segment,"Start")+have;}
       if(overlap){
        BYTE incoming[httpOverlapLimit];
        for(size_t read=0;read<overlap;){
         const auto n=response.read(incoming+read,overlap-read,*group);
         if(!n)throw std::runtime_error("HTTP data ended before the resume boundary could be verified.");
         m.charge(n,*group,*rate,limitOwner);
         {Lock lock(m.mutex);job->data["TransferredBytes"]=num(job->data,"TransferredBytes")+n;}
         read+=n;
        }
        if(adjacentOverlap){
         const auto digest=httpOverlapHash(incoming,overlap);
         Lock lock(m.mutex);job->data["Segments"][index]["HttpBoundarySha256"]=digest;
        }else{
         BYTE saved[httpOverlapLimit];
         Handle input(CreateFileW(dataFile.c_str(),GENERIC_READ,FILE_SHARE_READ|FILE_SHARE_WRITE,nullptr,OPEN_EXISTING,0,nullptr));
         if(!input||readAt(input.h,saved,overlap,requestStart)!=overlap)
          throw std::runtime_error("Cannot read the saved HTTP resume boundary.");
         if(memcmp(saved,incoming,overlap)!=0)
          throw HttpOverlapMismatch("The HTTP resume bytes do not match the saved file. The old partial download was kept.",index);
        }
       }
       if(verifyRangeOnReconnect&&!retainedInitial){
        Lock lock(m.mutex);
        job->data["RangeSupported"]=true;
        verifyRangeOnReconnect=false;
       }
       // One handle per worker on the shared output file; every write names its offset.
       Handle output(CreateFileW(dataFile.c_str(),GENERIC_WRITE,FILE_SHARE_READ|FILE_SHARE_WRITE,nullptr,OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL,nullptr));
       if(!output)throw std::runtime_error("Cannot write the partial download.");
       if(ranges){
        {Lock lock(m.mutex);responseEnd[worker]=retainedInitial?num(data,"Size")-1:num(segment,"End");acceptedResponses[worker]=1;}
        rangeWorkChanged.notify_all();
       }
       // The next read is issued before the previous buffer is written, so a single
       // connection no longer alternates network and disk (about 100 MB/s on loopback).
       BYTE* const buffers[2]={bufferStorage.data(),bufferStorage.data()+transferReadBuffer};int current=0;bool pendingRead=false;
       const bool canSplitRead=ranges&&connections>1;
       size_t readWindow=canSplitRead?32*1024:transferReadBuffer;
       std::chrono::steady_clock::time_point readStarted;
       // The first bytes of the file decide early whether the server sent a
       // web page instead of the file, before the whole body is downloaded.
       Bytes head;bool headChecked=false,sizeChecked=false;
       const bool firstBytes=num(segment,"Start")==0&&have==0&&str(data,"ProjectId").empty()&&!yes(data,"GrabberImmediate");
       const bool checkWebPage=firstBytes&&yes(prefs,"StopOnWebPage",true)&&!yes(data,"AllowWebPageContent");
       const auto responseSize=ranges?num(data,"Size",-1):need;
       // A replacement starting at byte zero can prove the same prefix
       // as a new download. Saved replacement parts still need validation.
       const bool checkOutputSize=firstBytes&&responseSize>0&&!protectSavedRefresh&&
        yes(data,"ConfirmationPending")&&mayStripResponsePrefix(data);
       const auto outputHeaderSize=checkOutputSize?static_cast<size_t>(std::min<i64>(responseSize,contentSniffReadLimit)):0;
       const auto headLimit=std::max(checkWebPage?contentSniffWindow:0,outputHeaderSize);
       auto checkHead=[&](bool complete){
        const auto count=std::min(head.size(),contentSniffWindow);
        if(!checkWebPage||headChecked||!count)return;
        const auto result=sniffContent(head.data(),count,complete);
        if(result.verdict==SniffVerdict::NeedMore)return;headChecked=true;
        const auto expected=fileExtension(str(data,"FileName"));if(!webPageInsteadOfFile(result,expected))return;
        {Lock lock(m.mutex);job->data["ContentCheck"]=sniffVerdictName(result.verdict);job->data["SniffedType"]=result.mime;job->data["SniffedExtension"]=result.extension;}
        throw WebPageReceived(webPageMessage(result,expected));
       };
       auto wantSize=[&](i64 ownedBytes){
        size_t want=readWindow;
        // Use a quarter second of the speed limit with a 64 KiB minimum.
        if(const auto limit=m.effectiveLimitKbps(limitOwner);limit>0)
         want=std::min(want,std::max<size_t>(65536,(size_t)std::min<i64>(limit*256,(i64)want)));
        // A complete wire response still needs an EOF read. One byte is
        // enough to detect excess data without reading another whole buffer.
        if(ownedBytes>=0)want=(size_t)std::clamp<i64>(ownedBytes,1,(i64)want);
        return want;
       };
       auto beginBodyRead=[&](BYTE* buffer,i64 ownedBytes){
        if(canSplitRead)readStarted=std::chrono::steady_clock::now();
        response.beginRead(buffer,wantSize(ownedBytes),*group);
        pendingRead=true;
       };
       for(;;){
        i64 ownedBytes=-1;
        if(ranges){
         Lock lock(m.mutex);const auto& actual=job->data["Segments"][index];
         ownedBytes=num(actual,"End")-num(actual,"Start")+1-num(actual,"Done");
         const bool unreadBody=need<0||readBytes<need-have;
         if(unreadBody&&(retainedInitial||num(actual,"End")<num(segment,"End"))&&ownedBytes==0){
          acceptedResponses[worker]=0;
          break;
         }
        }
        if(!pendingRead)beginBodyRead(buffers[current],ownedBytes);
        BYTE* const buffer=buffers[current];auto n=response.finishRead(*group);pendingRead=false;if(!n)break;
        if(canSplitRead){
         // A split can shorten this worker's ownership while a read is pending.
         // Keep slow reads short enough to observe the new boundary promptly.
         const auto milliseconds=std::chrono::duration_cast<std::chrono::milliseconds>(
          std::chrono::steady_clock::now()-readStarted).count();
         readWindow=std::clamp<size_t>(n*125/std::max<i64>(1,milliseconds),16*1024,transferReadBuffer);
        }
        if(need>=0&&(i64)n>need-have-readBytes)throw std::runtime_error("Server sent more bytes than the declared range.");
        readBytes+=(i64)n;m.charge(n,*group,*rate,limitOwner);
        size_t keep;i64 at,ownedAfterWrite=-1;
        {
         Lock lock(m.mutex);const auto& actual=job->data["Segments"][index];
         const auto tick=GetTickCount64();ULONGLONG none=0;
         firstProgressTick.compare_exchange_strong(none,tick);lastProgressTick=tick;workerProgressTick[worker]=tick;
         at=num(actual,"Start")+num(actual,"Done");
         keep=n;
         if(ranges){
          const auto remaining=num(actual,"End")-at+1;
          keep=(size_t)std::min<i64>((i64)n,remaining);
          ownedAfterWrite=remaining-(i64)keep;
          // Bytes already read past a split boundary cannot be restored by
          // enlarging this response's assignment.
          if(keep<n)responseEnd[worker]=-1;
         }
         inflightEnd[worker]=at+(i64)keep;
        }
        // The next read is in flight while this buffer is written; the disk write runs outside
        // the catalog lock so the other workers and the window never wait on it, and a split
        // meanwhile stays past these bytes.
        // Bound the next read to the observed ownership. If another worker
        // splits it in the meantime, the write still uses the live boundary.
        if(ownedAfterWrite!=0&&(need<0||readBytes<need-have)){
         beginBodyRead(buffers[1-current],ownedAfterWrite);
        }
        if(keep)writeAt(output.h,buffer,keep,at);
        {
         Lock lock(m.mutex);const auto& actual=job->data["Segments"][index];inflightEnd[worker]=0;
         const size_t credit=ranges?(size_t)std::min<i64>((i64)keep,num(actual,"End")-num(actual,"Start")+1-num(actual,"Done")):keep;
         m.progress(job,credit,index,&activity);
         const auto end=at+(i64)credit;
         if(ranges&&credit&&end==num(actual,"End")+1&&end<num(data,"Size")){
          const auto tailSize=(size_t)std::min(end,httpOverlapLimit);
          if(credit>=tailSize)
           std::copy_n(buffer+credit-tailSize,tailSize,writtenTails[end].begin());
         }
         job->data["TransferredBytes"]=num(job->data,"TransferredBytes")+(n-credit);
         if((!headChecked||checkOutputSize)&&credit&&head.size()<headLimit)
          head.insert(head.end(),buffer,buffer+std::min<size_t>(credit,headLimit-head.size()));
        }
        if(head.size()>=contentSniffWindow)checkHead(false);
        if(checkOutputSize&&!sizeChecked&&head.size()>=outputHeaderSize){
         sizeChecked=true;
         const auto finalSize=checkedDirectOutputSize(head,responseSize);
         if(finalSize>=0){
          Lock lock(m.mutex);
          auto& current=job->data;
          if(!group->cancelled()&&yes(current,"ConfirmationPending")&&num(current,"Size",-1)==responseSize)
           current["PrefetchedFinalSize"]=finalSize;
         }
        }
        current=1-current;
       }
       if(checkWebPage)checkHead(need<0||have+readBytes>=need);
       {
        Lock lock(m.mutex);const auto& actual=job->data["Segments"][index];
        i64 expected=ranges?num(actual,"End")-num(actual,"Start")+1:need;
        if(expected>=0&&num(actual,"Done")!=expected)throw std::runtime_error("Connection ended before the expected bytes arrived.");
       }
       previousRequestBytes=readBytes;staleRetried=false;
       // A dynamic split can end ownership before the wire response ends.
       // Only a fully read response can supply a reusable connection.
       responseDrained=need<0||readBytes==need-have;
       // The shared file is flushed once, before publication; a per-range flush
       // would write back every worker's dirty pages each time one range ended.
       break;
      }catch(const Changed&){throw;}catch(const Cancelled&){throw;}catch(const WebPageReceived&){throw;}catch(const BrowserVerificationRequired&){throw;}catch(const HttpRejected& rejection){
       rememberServerDelay(rejection);
       if(verifyRangeOnReconnect&&rangeRequestRejected(rejection))
        throw Changed("The server did not accept a resume range.");
       if(ranges&&retries>0&&readBytes==0&&rejection.status==403&&rejection.retryAfterMs==-1){
        // An origin may forbid another connection while an accepted response is open.
        Lock lock(m.mutex);
        if(std::any_of(acceptedResponses.begin(),acceptedResponses.end(),[](unsigned char accepted){return accepted!=0;})){
         // An explicit denial ends speculative splitting for this transfer.
         // Existing uncovered ranges remain eligible through the paced path.
         if(fresh){
          rangeExpansionDenied=true;
          deferRangeExpansionUntil=std::chrono::steady_clock::now()+busyRangeProbeInterval;
         }
         release(std::nullopt,fresh);
        }
       }
       if(!rejection.retryable())throw;
       ++failures;
       if(failures>retries){
        if(rejection.retryAfterMs>=0)deferRequests(rejection.retryAfterMs);
        if(retries<=0||!othersHealthy())throw;
        release(std::min(30000,2000*(1<<std::min(releases++,4))));
       }
       const auto delay=rejection.delay(failures-1);
       deferRequests(delay);
       {Lock lock(m.mutex);activity.state="Server busy; retrying in "+std::to_string((delay+999)/1000)+" s";}
      }catch(const std::exception& e){
       // One retry a protocol level down after a failed HTTP/3 or HTTP/2
       // exchange, ahead of the ordinary retry budget. The choice stays with the download.
       bool alreadyDisabled,http3Disabled;{Lock lock(m.mutex);alreadyDisabled=yes(job->data,"DisableHttp2");http3Disabled=yes(job->data,"DisableHttp3");}
       if(shouldDisableHttp3(lastProtocol,http3Disabled,e)){Lock lock(m.mutex);job->data["DisableHttp3"]=true;job->data["Http3FallbackError"]=e.what();activity.state="Retrying with HTTP/2";continue;}
       if(shouldDisableHttp2(lastProtocol,alreadyDisabled,e)){Lock lock(m.mutex);job->data["DisableHttp2"]=true;job->data["Http2FallbackError"]=e.what();activity.state="Retrying with HTTP/1.1";continue;}
       bool failedConnect=false;
       if(openingFailed&&ranges&&connections>1&&retries>0){
        auto resetOrRefused=[](DWORD code){return code==ERROR_WINHTTP_CANNOT_CONNECT||code==ERROR_WINHTTP_CONNECTION_ERROR;};
        if(auto windowsFailure=dynamic_cast<const InternetFailure*>(&e))failedConnect=resetOrRefused(windowsFailure->code);
        else if(auto curlFailure=dynamic_cast<const CurlConnectionFailure*>(&e))failedConnect=resetOrRefused(curlFailure->code);
        else failedConnect=dynamic_cast<const CurlResponseDisconnect*>(&e)!=nullptr;
       }
       if(!paceReconnect(readBytes,failedConnect))throw;
      }catch(...){
       if(!paceReconnect(readBytes))throw;
      }
     }
     }catch(const RangeReleased& released){
      if(!released.pauseMs)break;
      // Pause before asking the server for a connection again, but never outlive
      // the download: the working connections may finish the released range meanwhile.
      const auto retryAt=std::chrono::steady_clock::now()+std::chrono::milliseconds(*released.pauseMs);
      std::unique_lock lock(m.mutex);
      while(unfinished()){
       group->check();
       const auto now=std::chrono::steady_clock::now();
       if(now>=retryAt)break;
       rangeWorkChanged.wait_until(lock,std::min(retryAt,now+checkInterval));
      }
      continue;
     }
     {Lock lock(m.mutex);owners[index]=-2;activity.state="Waiting";
      if(index==0&&initialRangeProbe==InitialRangeProbe::Pending)initialRangeCancel.stop=true;
     }
     rangeWorkChanged.notify_all();
     lastCompleted=index;
    }
   }catch(const HttpOverlapMismatch& mismatch){recordError(true,true,mismatch.range);}catch(const Changed&){recordError(true);}catch(...){recordError(false);}
   {Lock lock(m.mutex);activity.state=group->cancelled()?"Stopped":"Finished";}
  };
  try{for(int w=0;w<connections;++w)workers.emplace_back(workerBody,w);}catch(...){
   group->stop=true;rangeWorkChanged.notify_all();for(auto& worker:workers)worker.join();throw;
  }
  for(auto& worker:workers)worker.join();
  const auto workersJoined=GetTickCount64();
  joinHasher.stop();
  cancel->check();
  i64 bufferedChecks=0,diskChecks=0;
  if(ranges&&!error)try{
   Json segments;{Lock lock(m.mutex);segments=job->data["Segments"];}
   Handle input;
   for(const auto& segment:segments){
    const auto digest=str(segment,"HttpBoundarySha256");
    if(digest.empty())continue;
    cancel->check();
    const auto start=num(segment,"Start");
    if(start<=0)throw std::runtime_error("Invalid completed HTTP range boundary.");
    const auto overlap=(size_t)std::min(start,httpOverlapLimit);
    BYTE saved[httpOverlapLimit];
    const auto tail=writtenTails.find(start);
    const BYTE* previous=saved;
    if(tail!=writtenTails.end()){
     previous=tail->second.data();
     ++bufferedChecks;
    }else{
     if(!input)input.h=CreateFileW(dataFile.c_str(),GENERIC_READ,FILE_SHARE_READ|FILE_SHARE_WRITE,nullptr,OPEN_EXISTING,0,nullptr);
     if(!input||readAt(input.h,saved,overlap,start-(i64)overlap)!=overlap)
      throw std::runtime_error("Cannot read the completed HTTP range boundary.");
     ++diskChecks;
    }
    if(httpOverlapHash(previous,overlap)!=digest)
     throw HttpOverlapMismatch("Adjacent HTTP ranges do not match. The mixed file was not published.");
   }
  }catch(const HttpOverlapMismatch&){recordError(true,true);}
  const auto boundariesVerified=GetTickCount64();
  joinHasher.join();
  const auto prefixHashStopped=GetTickCount64();
  {
   Lock lock(m.mutex);
   const auto lastByte=lastProgressTick.load();
   job->data["HttpBoundaryChecks"]={{"FromWriteBuffer",bufferedChecks},{"FromDisk",diskChecks}};
   job->data["NetworkTailSeconds"]=lastByte?(boundariesVerified-lastByte)/1000.0:0.0;
   job->data["NetworkTicks"]={{"HttpStart",(i64)httpStartTick},{"FirstByte",(i64)firstProgressTick.load()},
    {"LastByte",(i64)lastByte},{"WorkersJoined",(i64)workersJoined},{"Joined",(i64)boundariesVerified},
    {"PrefixHashStopped",(i64)prefixHashStopped}};
  }
  if(error){
   {Lock lock(m.mutex);job->data.erase("PrefetchedFinalSize");}
   if(changed&&!restartTried&&!protectSavedRefresh){
    // The prefix hasher's read handle is closed before repairing any parts.
    Lock lock(m.mutex);
    if(!preserveIdentity){
     if(repairable&&repairCandidate&&!repairTried){
      const auto& segments=job->data["Segments"];
      const auto retained=num(job->data,"Received")-num(segments.at(*repairCandidate),"Done");
      if(retained>0){repairTried=true;repairNext=repairCandidate;continue;}
     }
     restartTried=true;
     restartWithoutRanges=overlapMismatch||verifyRangeOnReconnect||requireStrongDateOnReconnect||!savedStrongDate.empty();
     continue;
    }
   }
   std::rethrow_exception(error);
  }
  {
   Lock lock(m.mutex);
   auto& data=job->data;
   // Keep the saved-data policy through worker validation and interruption.
   data["RefreshPendingValidation"]=false;
   if(helperWaitRequiresReview)data.erase("ServerRetryRequiresReview");
   // A full response without a length becomes measurable at clean EOF,
   // before prefetch starts waiting for the user's decision.
   if(!ranges&&num(data,"Size",-1)<0&&data["Segments"].size()==1){
    auto& segment=data["Segments"][0];
    const auto size=num(segment,"Done");
    data["Size"]=size;
    if(size==0)data["Segments"].clear();
    else segment["End"]=size-1;
    job->workers[0].end=size-1;
   }
  }
  return;
 }
}

static void transferPost(Manager& m,JobPtr job,const std::shared_ptr<Cancel>& cancel,const Json& prefs,
 Headers headers,const std::string& url,const fs::path& parts,JobPtr limitOwner,const std::shared_ptr<Rate>& rate){
 Json request;std::string redirected;
 {Lock lock(m.mutex);request=readPostRequest(job->data);redirected=reveal(str(job->data,"ProtectedPostGetUrl"));}
 // A staged body is read and verified first: a missing or damaged file stops here, before the submission counts as attempted.
 const Bytes body=request.contains("bodyFile")?readStagedPostBody(m,request):str(request,"body").empty()?Bytes{}:unb64(str(request,"body"));
 {Lock lock(m.mutex);
  if(!redirected.empty()){headers=readHeaders(Json{{"ProtectedHeaders",str(job->data,"ProtectedPostGetHeaders")}});}
  else if(yes(job->data,"PostAttempted"))throw std::runtime_error("This form was already submitted. Start this download explicitly to submit it again.");
  else {job->data["PostAttempted"]=true;job->data["RangeSupported"]=false;m.save();}
 }
 auto pool=std::make_shared<HttpSession>(prefs,browserSessionSaver(m,job,prefs.value("ActiveBrowserSession",Json::object())));
 if(!redirected.empty()){transferHttp(m,job,cancel,prefs,headers,redirected,parts,limitOwner,rate,pool);return;}
 headers["Content-Type"]=str(request,"contentType");
 if(headerValue(headers,"Accept").empty())headers["Accept"]="*/*";
 std::unique_ptr<Http> response;auto current=url;
 for(int hop=0;hop<11;++hop){
  cancel->check();response=std::make_unique<Http>(current,headers,prefs,*cancel,std::nullopt,std::nullopt,"",&body,false,pool);
  const auto status=response->status;
  if(status!=301&&status!=302&&status!=303&&status!=307&&status!=308)break;
  auto location=response->header(L"Location");if(location.empty())throw std::runtime_error("Redirect response has no Location header.");
  auto next=combineUrl(current,location);Url from(current),to(next);
  if((to.scheme!="http"&&to.scheme!="https")||(from.scheme=="https"&&to.scheme!="https"))throw std::runtime_error("Refusing an unsafe form-download redirect.");
  if(status==307||status==308){if(from.origin!=to.origin)throw std::runtime_error("The site redirected the form to another origin. Finish this download in the browser.");if(hop==10)throw std::runtime_error("Too many form-download redirects.");current=next;response.reset();continue;}
  headers.erase("Content-Type");if(from.origin!=to.origin)for(auto it=headers.begin();it!=headers.end();){auto key=lower(it->first);if(key=="authorization"||key=="cookie"||key=="origin"||key=="referer")it=headers.erase(it);else ++it;}
  {Lock lock(m.mutex);job->data["ProtectedPostGetUrl"]=protect(next);job->data["ProtectedPostGetHeaders"]=headers.empty()?"":protect(legacyDictionary(Json(headers)).dump());m.save();}
  response.reset();transferHttp(m,job,cancel,prefs,headers,next,parts,limitOwner,rate,pool);return;
 }
 success(*response);if(response->status==206)throw std::runtime_error("A form download returned an unexpected partial response.");
 auto expected=length(response->header(L"Content-Length"));
 {Lock lock(m.mutex);clearParts(parts);job->data["Received"]=0;job->data["Size"]=expected;job->data["Modified"]=response->header(L"Last-Modified");job->data["ETag"]=etag(*response);job->data["Segments"]=Json::array({{{"Index",0},{"Start",0},{"End",expected-1},{"Done",0}}});job->workers.assign(1,Worker{1});job->workers[0].state="Receiving form download";job->workers[0].end=expected-1;m.save();}
 Handle output(CreateFileW(partPath(parts,0).c_str(),GENERIC_WRITE,FILE_SHARE_READ,nullptr,CREATE_ALWAYS,FILE_ATTRIBUTE_NORMAL,nullptr));if(!output)throw std::runtime_error("Cannot write the form download.");
 BYTE buffer[65536];i64 received=0;
 for(;;){auto count=response->read(buffer,sizeof(buffer),*cancel);if(!count)break;if(expected>=0&&(i64)count>expected-received)throw std::runtime_error("Server sent more bytes than declared.");m.charge(count,*cancel,*rate,limitOwner);DWORD written=0;if(!WriteFile(output.h,buffer,(DWORD)count,&written,nullptr)||written!=count)throw std::runtime_error("Cannot write the form download.");received+=(i64)count;m.progress(job,count,0,&job->workers[0]);}
 if(expected>=0&&received!=expected)throw std::runtime_error("The form response ended early. Start again to resubmit, or recapture it in the browser.");
 if(!FlushFileBuffers(output.h))throw std::runtime_error("Cannot flush the form download.");
 {Lock lock(m.mutex);job->data["Size"]=received;job->data["Segments"][0]["End"]=received-1;job->workers[0].state="Finished";m.save();}
}
// Reads the leading downloaded bytes of the contiguous ranges, in file order,
// from the single output file or from an older layout's parts.
static Bytes leadingBytes(const fs::path& parts,const std::string& id,bool layoutSingle,Json segments,size_t limit){
 std::sort(segments.begin(),segments.end(),[](const Json& a,const Json& b){return num(a,"Start")<num(b,"Start");});
 Bytes out;i64 position=0;
 const auto data=dataPath(parts,id);
 Handle single(layoutSingle?CreateFileW(data.c_str(),GENERIC_READ,FILE_SHARE_READ|FILE_SHARE_WRITE,nullptr,OPEN_EXISTING,0,nullptr):INVALID_HANDLE_VALUE);
 if(layoutSingle&&!single)return out;
 for(const auto& s:segments){
  if(out.size()>=limit||num(s,"Start")!=position)break;
  const auto wanted=(size_t)std::min<i64>(num(s,"Done"),(i64)(limit-out.size()));if(!wanted)break;
  const auto before=out.size();out.resize(before+wanted);size_t got=0;
  if(single)got=readAt(single.h,out.data()+before,wanted,num(s,"Start"));
  else{std::ifstream input(partPath(parts,num(s,"Index")),std::ios::binary);if(!input){out.resize(before);break;}input.read(reinterpret_cast<char*>(out.data()+before),(std::streamsize)wanted);got=(size_t)input.gcount();}
  out.resize(before+got);position+=(i64)got;if(got<wanted||num(s,"Done")!=num(s,"End")-num(s,"Start")+1)break;
 }
 return out;
}
static bool youtubeTtmlRequest(const Url& address){
 if(address.scheme!="https"||address.port!=443||address.path!="/api/timedtext"||
    (address.host!="www.youtube.com"&&address.host!="m.youtube.com"&&address.host!="www.youtube-nocookie.com"))return false;
 const auto parameters=query(address.full);
 const auto format=parameters.find("fmt");
 return format!=parameters.end()&&format->second=="ttml";
}
static bool hasTtmlCaptionText(const Bytes& bytes,const Cancel& cancel){
 const auto release=[](auto* value){value->Release();};
 std::unique_ptr<IStream,decltype(release)> stream(SHCreateMemStream(bytes.data(),static_cast<UINT>(bytes.size())),release);
 IXmlReader* raw=nullptr;
 if(!stream||FAILED(CreateXmlReader(__uuidof(IXmlReader),reinterpret_cast<void**>(&raw),nullptr)))
  throw std::runtime_error("Cannot initialize subtitle verification.");
 std::unique_ptr<IXmlReader,decltype(release)> reader(raw,release);
 if(FAILED(reader->SetProperty(XmlReaderProperty_DtdProcessing,DtdProcessing_Prohibit))||
    FAILED(reader->SetProperty(XmlReaderProperty_XmlResolver,0))||
    FAILED(reader->SetProperty(XmlReaderProperty_ConformanceLevel,XmlConformanceLevel_Document))||
    FAILED(reader->SetProperty(XmlReaderProperty_MaxElementDepth,32))||FAILED(reader->SetInput(stream.get())))
  throw std::runtime_error("Cannot configure subtitle verification.");
 std::wstring ttmlNamespace;
 bool inBody=false,hasText=false;
 UINT paragraph=0;
 XmlNodeType type=XmlNodeType_None;
 HRESULT status;
 while((status=reader->Read(&type))==S_OK){
  cancel.check();
  if(type==XmlNodeType_Element||type==XmlNodeType_EndElement){
   UINT depth=0,length=0,namespaceLength=0;
   const WCHAR *name=nullptr,*ns=nullptr;
   if(FAILED(reader->GetDepth(&depth))||FAILED(reader->GetLocalName(&name,&length))||
      FAILED(reader->GetNamespaceUri(&ns,&namespaceLength)))return false;
   const auto local=std::wstring_view(name,length);
   const auto uri=std::wstring_view(ns,namespaceLength);
   if(type==XmlNodeType_EndElement){
    if(depth==paragraph)paragraph=0;
    if(depth==1)inBody=false;
   }else if(depth==0){
    if(local!=L"tt"||(uri!=L"http://www.w3.org/ns/ttml"&&uri!=L"http://www.w3.org/2006/10/ttaf1"))return false;
    ttmlNamespace=uri;
   }else if(uri==ttmlNamespace&&!reader->IsEmptyElement()){
    if(depth==1&&local==L"body")inBody=true;
    else if(inBody&&!paragraph&&local==L"p")paragraph=depth;
   }
  }else if(paragraph&&!hasText&&(type==XmlNodeType_Text||type==XmlNodeType_CDATA)){
   const WCHAR* value=nullptr;UINT length=0;
   if(FAILED(reader->GetValue(&value,&length)))return false;
   hasText=std::any_of(value,value+length,[](wchar_t ch){return !std::iswspace(ch);});
  }
 }
 return status==S_FALSE&&hasText;
}
static void verifyYouTubeTtml(Manager& m,JobPtr job,const fs::path& parts,bool single,const Cancel& cancel){
 constexpr i64 maxBytes=16*1024*1024;
 i64 size;Json segments;
 {Lock lock(m.mutex);size=num(job->data,"Size",-1);segments=job->data.value("Segments",Json::array());}
 bool valid=false;
 if(size>0&&size<=maxBytes){
  const auto bytes=leadingBytes(parts,job->id(),single,std::move(segments),static_cast<size_t>(size));
  if(bytes.size()!=static_cast<size_t>(size))throw std::runtime_error("Cannot read the complete subtitle response. The received data was kept.");
  valid=hasTtmlCaptionText(bytes,cancel);
 }
 if(valid)return;
 cancel.check();
 {
  Lock lock(m.mutex);
  auto before=job->data;
  job->data["HttpPartsResetPending"]=true;
  job->data["Segments"]=Json::array();
  job->data["Size"]=-1;
  job->data["Received"]=0;
  job->data["RangeSupported"]=false;
  job->data.erase("CompletedPrefetch");
  job->data.erase("ReusedCompletedPrefetch");
  job->data.erase("BrowserReportedDirectSize");
  try{m.save();}catch(...){job->data=std::move(before);throw;}
 }
 if(size>maxBytes)throw std::runtime_error("The subtitle response exceeds the 16 MB verification limit.");
 throw std::runtime_error("YouTube returned no usable TTML captions. Play the selected subtitle track in the browser and try again.");
}
// Classifies the downloaded content before publication. A web page or text
// served in place of a file format stops the download with its parts kept;
// a name without a usable extension gains the extension of the content.
static void checkDownloadedContent(Manager& m,JobPtr job,const fs::path& parts,bool layoutSingle){
 Json j,settings;{Lock l(m.mutex);j=job->data;settings=m.state["Settings"];}
 if(!str(j,"ProjectId").empty()||yes(j,"GrabberImmediate"))return;
 const auto head=leadingBytes(parts,job->id(),layoutSingle,j.value("Segments",Json::array()),contentSniffReadLimit);
 const auto result=sniffContent(head.data(),head.size(),true);
 const auto name=str(j,"FileName"),expected=fileExtension(name);
 {Lock l(m.mutex);auto& data=job->data;data["ContentCheck"]=sniffVerdictName(result.verdict);data["SniffedType"]=result.mime;data["SniffedExtension"]=result.extension;
  if(result.prefixLength)data["ContentPrefixBytes"]=(i64)result.prefixLength;else data.erase("ContentPrefixBytes");}
 // Inserted web page text before the file: media captured from the browser's
 // video panel drops it without asking, as IDM does for its video downloads;
 // any other file asks the user.
 if(result.verdict==SniffVerdict::MatchAfterPrefix&&!yes(j,"StripContentPrefix")&&mayStripResponsePrefix(j)){Lock l(m.mutex);job->data["StripContentPrefix"]=true;job->data["StrippedPrefixReason"]="media";j["StripContentPrefix"]=true;}
 if(result.verdict==SniffVerdict::MatchAfterPrefix&&!yes(j,"StripContentPrefix")&&!yes(j,"AllowContentPrefix"))throw ContentPrefixDetected("The file starts with "+std::to_string(result.prefixLength)+" bytes of web page text before the "+result.description+". Resume the download to choose whether to remove that text or keep the data as received.");
 if(yes(settings,"StopOnWebPage",true)&&!yes(j,"AllowWebPageContent")&&webPageInsteadOfFile(result,expected))
  throw WebPageReceived(webPageMessage(result,expected));
 if(!yes(settings,"CorrectFileExtensions",true)||!str(j,"ReplacementOf").empty())return;
 const auto next=correctedFileName(name,result);if(next.empty())return;
 Lock l(m.mutex);if(str(job->data,"FileName")!=name)return;
 const auto folder=fs::path(wide(str(job->data,"Folder"))),candidate=folder/wide(next);std::error_code error;
 if(fs::exists(candidate,error)||error||m.relocationReserves(candidate))return;
 const auto key=lower(utf8(candidate.wstring()));for(auto item:m.jobs)if(item!=job&&lower(utf8(item->target().wstring()))==key)return;
 // An automatically assigned category follows the corrected name; a category
 // that no longer matches the old name was chosen by the user and stays.
 const auto host=Url(str(job->data,"Url")).host,previous=downloadCategory(name,host,settings),updated=downloadCategory(next,host,settings);
 if(!yes(job->data,"CategoryExplicit")&&str(job->data,"Category")==previous&&updated!=previous)job->data["Category"]=updated;
 job->data["OriginalFileName"]=name;job->data["FileName"]=next;m.save();
}
static void checkPrefetchedDirectSize(Manager& m,const JobPtr& job,const fs::path& parts,bool numbered,const Cancel& cancel){
 i64 size;
 Json segments;
 {
  Lock lock(m.mutex);
  const auto& data=job->data;
  if(!yes(data,"ConfirmationPending")||!mayStripResponsePrefix(data)||hasNonemptyString(data,"ProjectId")||yes(data,"GrabberImmediate"))return;
  size=num(data,"Size",-1);
  if(size<=0||num(data,"Received")!=size)return;
  segments=data.value("Segments",Json::array());
 }
 cancel.check();
 Bytes head;
 try{
  const bool single=!numbered&&fs::exists(dataPath(parts,job->id()));
  head=leadingBytes(parts,job->id(),single,std::move(segments),contentSniffReadLimit);
 }catch(const std::runtime_error&){
  return; // Publication still performs its normal content check after Start.
 }
 const auto finalSize=checkedDirectOutputSize(head,size);
 if(finalSize<0)return;
 Lock lock(m.mutex);
 cancel.check();
 if(yes(job->data,"ConfirmationPending"))job->data["PrefetchedFinalSize"]=finalSize;
}
// A completed response is reusable after Download Later without spending its URL
// again. Bind the receipt to the request and exact saved plan, then verify every
// part before bypassing transport. A refresh or restart invalidates that binding.
struct PrefetchConfirmed {};
struct VerifiedPrefetch {
 Json identity;
 std::string sha256;
 Handle singleFile;
 Json numberedPlan;
 std::map<i64,Handle> numberedParts;
};
static bool completedPrefetch(Manager& m,JobPtr job,const fs::path& parts,const Cancel& cancel,bool remember,bool ftp,PrefixHasher* prefix,VerifiedPrefetch& verified)try{
 Json data;
 {
  Lock lock(m.mutex);
  if(remember&&!yes(job->data,"ConfirmationPending")&&!job->prefetchDeferring)return false;
  data=job->snapshot();
 }
 if(!str(data,"ProtectedRequest").empty()||yes(data,"RefreshPendingValidation"))return false;
 const auto size=num(data,"Size",-1);auto plan=data.value("Segments",Json::array());
 const bool emptyFtp=ftp&&size==0&&plan.is_array()&&plan.size()==1&&num(plan[0],"Start",-1)==0&&
  num(plan[0],"End")==-1&&num(plan[0],"Index",-1)>=0&&num(plan[0],"Index")<=100000;
 if((!emptyFtp&&!completePlan(plan,size))||num(data,"Received")!=size)return false;
 if(ftp&&(plan.empty()||yes(data,"FtpInvalidated")||
    std::any_of(plan.begin(),plan.end(),[](const Json& part){return !yes(part,"FtpComplete");})))return false;
 auto receipt=data.value("CompletedPrefetch",Json::object());
 if(!remember&&(!receipt.is_object()||num(receipt,"Version")!=1||str(receipt,"Url")!=str(data,"Url")||str(receipt,"Headers")!=str(data,"ProtectedHeaders")||num(receipt,"Size",-1)!=size||receipt.value("Plan",Json::array())!=plan||!receipt.contains("Hashes")||!receipt["Hashes"].is_array()||receipt["Hashes"].size()!=plan.size()))return false;
 // Start makes a new receipt unnecessary. A saved receipt must still be
 // verified independently, and Download Later cancels under the same lock.
 const auto checkReceipt=[&]{
  if(remember){
   Lock lock(m.mutex);cancel.check();
   if(!yes(job->data,"ConfirmationPending")&&!job->prefetchDeferring)throw PrefetchConfirmed{};
  }else cancel.check();
 };
 const auto dataFile=dataPath(parts,job->id());const bool single=!ftp&&fs::exists(dataFile);
 const DWORD share=remember?FILE_SHARE_READ|FILE_SHARE_WRITE:FILE_SHARE_READ;
 Handle dataHandle(single?CreateFileW(dataFile.c_str(),GENERIC_READ,share,nullptr,OPEN_EXISTING,0,nullptr):INVALID_HANDLE_VALUE);
 if(single&&!dataHandle){
  if(remember)return false;
  throw std::runtime_error("Cannot open the saved prefetch for verification.");
 }
 // An empty plan has no hashes to check. Require the actual saved empty file
 // so a missing or replaced temporary file cannot stand in for the response.
 if(size==0&&!ftp&&(!dataHandle||fileLength(dataHandle.h)!=0))return false;
 const auto identity=!remember&&dataHandle&&!plan.empty()&&fileLength(dataHandle.h)==size?stableFileIdentity(dataHandle.h):Json();
 std::vector<size_t> order;
 order.reserve(plan.size());
 for(size_t index=0;index<plan.size();++index)order.push_back(index);
 std::optional<StreamHash> whole;
 if(!identity.is_null()&&plan.size()>1){
  // Read in file order for the publication hash, retaining the original
  // indices so each range is still checked against its own saved digest.
  std::sort(order.begin(),order.end(),[&](size_t a,size_t b){return num(plan[a],"Start")<num(plan[b],"Start");});
  whole.emplace();
 }
 ScopedSavedDataCheck checking(m,*job,remember?0:size);
 auto reportChecked=[&](i64 bytes){
  if(!remember){Lock lock(m.mutex);job->savedDataCheck.checked+=bytes;}
 };
 Json hashes=Json::array();
 std::vector<char> buffer(assemblyBuffer);
 std::map<i64,Handle> numberedParts;
 std::set<i64> damaged;
 const i64 available=single?fileLength(dataHandle.h):0;
 for(auto index:order){
  checkReceipt();
  const auto& segment=plan[index];
  const auto start=num(segment,"Start"),length=num(segment,"End")-start+1,id=num(segment,"Index");
  std::string hash;
  i64 checked=0;
  Handle part;
  if(num(segment,"Done")==length){
   if(!single){
    const auto file=partPath(parts,id);
    if(fs::is_regular_file(file)){
     // Keep verified parts open without write or delete sharing through assembly.
     part.h=CreateFileW(file.c_str(),GENERIC_READ,FILE_SHARE_READ,nullptr,OPEN_EXISTING,FILE_FLAG_SEQUENTIAL_SCAN,nullptr);
     if(!part)throw std::runtime_error("Cannot read the saved prefetch part.");
    }
   }
   if((single&&available>=start+length)||(part&&fileLength(part.h)==length)){
    if(single&&prefix&&prefix->valid&&plan.size()==1&&prefix->hashed<=length){
     prefix->advance(dataFile,length,cancel,checkReceipt);
     hash=prefix->finish();
    }else{
     const HANDLE input=single?dataHandle.h:part.h;
     const i64 offset=single?start:0;
     StreamHash digest;
     while(checked<length){
      checkReceipt();
      const auto n=(size_t)std::min<i64>((i64)buffer.size(),length-checked);
      if(readAt(input,buffer.data(),n,offset+checked)!=n){
       if(!single)throw std::runtime_error("Cannot read the saved prefetch part.");
       break;
      }
      digest.update(buffer.data(),n);
      if(whole)whole->update(buffer.data(),n);
      checked+=(i64)n;reportChecked((i64)n);
     }
     if(checked==length)hash=digest.finish();
    }
   }
  }
  // Missing or damaged bytes are examined when their range is rejected.
  reportChecked(length-checked);
  if(hash.empty()||(!remember&&receipt["Hashes"][index]!=hash)){
   if(remember)return false;
   damaged.insert(id);
  }else if(part)numberedParts.emplace(id,std::move(part));
  hashes[index]=std::move(hash);
 }
 Lock lock(m.mutex);cancel.check();
 if(job->data.value("Segments",Json::array())!=plan||str(job->data,"Url")!=str(data,"Url")||str(job->data,"ProtectedHeaders")!=str(data,"ProtectedHeaders")||yes(job->data,"RefreshPendingValidation"))return false;
 if(!damaged.empty()){
  // Repair can migrate old numbered parts. Remove any damaged part, and
  // remove shadow parts when the verified layout was the consolidated file.
  for(const auto& segment:plan){
   const auto id=num(segment,"Index");
   if(!single&&!damaged.count(id))continue;
   const auto file=partPath(parts,id);
   if(fs::exists(file)){
    if(!fs::is_regular_file(file))throw std::runtime_error("Cannot remove a damaged prefetch part.");
    fs::remove(file);
   }
  }
  auto before=job->data;
  for(auto& segment:job->data["Segments"]){
   if(!damaged.count(num(segment,"Index")))continue;
   job->data["Received"]=num(job->data,"Received")-num(segment,"Done");
   segment["Done"]=0;
   if(ftp){segment["FtpComplete"]=false;segment.erase("FtpBoundarySha256");}
  }
  job->data.erase("CompletedPrefetch");
  job->data.erase("ReusedCompletedPrefetch");
  try{m.save();}catch(...){job->data=std::move(before);throw;}
  return false;
 }
 if(remember){
  if(!yes(job->data,"ConfirmationPending")&&!job->prefetchDeferring)return false;
  auto before=job->data;job->data["CompletedPrefetch"]={{"Version",1},{"Url",str(data,"Url")},{"Headers",str(data,"ProtectedHeaders")},{"Size",size},{"Plan",plan},{"Hashes",hashes}};
  try{m.save();}catch(...){job->data=std::move(before);throw;}
 }else{
  if(!identity.is_null()&&stableFileIdentity(dataHandle.h)==identity){
   verified.identity=identity;
   verified.sha256=whole?whole->finish():hashes[0].get<std::string>();
   std::swap(verified.singleFile.h,dataHandle.h);
  }
  job->data["ReusedCompletedPrefetch"]=true;
 }
 if(!numberedParts.empty()){
  verified.numberedPlan=std::move(plan);
  verified.numberedParts.swap(numberedParts);
 }
 return true;
}catch(const PrefetchConfirmed&){
 cancel.check();
 return false;
}
void transfer(Manager& m,JobPtr job,const std::shared_ptr<Cancel>& cancel,JobPtr limitOwner,std::shared_ptr<Rate> sharedRate){requirePublicationRecovered(m,job);const bool ownsConnection=!limitOwner;if(!limitOwner)limitOwner=job;if(!sharedRate)sharedRate=std::make_shared<Rate>();auto started=std::chrono::steady_clock::now();const auto transferStartTick=GetTickCount64();auto networkEnd=started;bool networkFinished=false,rememberPrefetch=false;double prior,confirmationSeconds=0,ftpConfirmationWait=0;Json prefs;Headers headers;fs::path folder,parts;std::string url;PrefixHasher hasher;VerifiedPrefetch verifiedPrefetch;{Lock l(m.mutex);validateGrabberFolder(job->data);prior=real(job->data,"TransferSeconds");prefs=browserSessionPreferences(browserProxyPreferences(m.state["Settings"],job->data),job->data);headers=readHeaders(job->data);url=str(job->data,"Url");folder=fs::path(wide(str(job->data,"Folder")));parts=m.root/L"parts"/wide(job->id());if(!str(job->data,"PartsFolder").empty())parts=fs::path(wide(str(job->data,"PartsFolder")));else if(!fs::exists(parts)&&!str(prefs,"TemporaryFolder").empty()){parts=fs::path(wide(str(prefs,"TemporaryFolder")))/L"UDM-parts"/wide(job->id());job->data["PartsFolder"]=utf8(parts.wstring());m.save();}}fs::create_directories(folder);fs::create_directories(parts);try{
 bool pendingReset;
 {Lock lock(m.mutex);pendingReset=yes(job->data,"HttpPartsResetPending");}
 if(pendingReset){
  cancel->check();clearParts(parts);
  Lock lock(m.mutex);job->data.erase("HttpPartsResetPending");
  try{m.save();}catch(...){job->data["HttpPartsResetPending"]=true;throw;}
 }
 const Url address(url);
 const bool ftp=address.scheme=="ftp";
 bool post;{Lock lock(m.mutex);post=!ftp&&!str(job->data,"ProtectedRequest").empty();}
 if(post){
  if(ownsConnection)m.ensureConnection(job,*cancel);
  transferPost(m,job,cancel,prefs,headers,url,parts,limitOwner,sharedRate);
 }else if(!completedPrefetch(m,job,parts,*cancel,false,ftp,nullptr,verifiedPrefetch)){
   {Lock lock(m.mutex);job->data.erase("CompletedPrefetch");job->data.erase("ReusedCompletedPrefetch");}
   if(ownsConnection)m.ensureConnection(job,*cancel);
   if(ftp)transferFtp(m,job,cancel,prefs,headers,url,parts,limitOwner,sharedRate,ftpConfirmationWait);
   else transferHttp(m,job,cancel,prefs,headers,url,parts,limitOwner,sharedRate,{},&hasher);
   rememberPrefetch=true;
  }
 networkEnd=std::chrono::steady_clock::now();
 networkFinished=true;
 {
  Lock lock(m.mutex);
  job->samplePrefetchRate(networkEnd,true);
  if(rememberPrefetch&&yes(job->data,"ConfirmationPending"))
   job->data["ConnectionStatus"]=SavingPrefetchReceipt;
  if(job->data.contains("NetworkTicks")){
   job->data["NetworkTicks"]["TransferStart"]=(i64)transferStartTick;
   job->data["NetworkTicks"]["NetworkEnd"]=(i64)GetTickCount64();
  }
 }
 // Finish phases are recorded on the record (Content, Hash, Publish seconds and the hash mode) so the
 // time between the last byte and the catalog showing the file complete can be read, not guessed.
 const auto phase=[](std::chrono::steady_clock::time_point& from){const auto now=std::chrono::steady_clock::now();const double seconds=std::chrono::duration<double>(now-from).count();from=now;return seconds;};auto mark=networkEnd;Json phases=Json::object();
 if(!post&&youtubeTtmlRequest(address)){
  const bool single=verifiedPrefetch.numberedParts.empty()&&fs::exists(dataPath(parts,job->id()));
  verifyYouTubeTtml(m,job,parts,single,*cancel);
  phases["CaptionVerification"]=phase(mark);
 }
 // At EOF the length and bounded prefix are enough to determine the output
 // size. Show it while the worker verifies the rest for later reuse.
 if(!post&&!ftp){
  checkPrefetchedDirectSize(m,job,parts,!verifiedPrefetch.numberedParts.empty(),*cancel);
  phases["DirectSize"]=phase(mark);
 }
 if(rememberPrefetch){
  completedPrefetch(m,job,parts,*cancel,true,ftp,ftp?nullptr:&hasher,verifiedPrefetch);
  phases["PrefetchReceipt"]=phase(mark);
  {
   Lock lock(m.mutex);
   if(str(job->data,"ConnectionStatus")==SavingPrefetchReceipt)job->data.erase("ConnectionStatus");
   if(job->prefetchDeferring)cancel->stop=true;
  }
  cancel->check();
 }
 try{m.waitForPrefetchConfirmation(job,*cancel);}catch(...){confirmationSeconds=phase(mark);throw;}
 confirmationSeconds=phase(mark);
 phases["ConfirmationWait"]=confirmationSeconds;
 const auto data=dataPath(parts,job->id());
 // Keep the verified layout even if an unrelated consolidated file appears.
 const bool verifiedNumbered=!verifiedPrefetch.numberedParts.empty();
 const bool single=!verifiedNumbered&&!ftp&&fs::exists(data);
 checkDownloadedContent(m,job,parts,single);phases["Content"]=phase(mark);
 cancel->check();Json j;{Lock l(m.mutex);job->data.erase("PrefetchedFinalSize");job->data["TransferSeconds"]=prior+std::chrono::duration<double>((networkFinished?networkEnd:std::chrono::steady_clock::now())-started).count()-ftpConfirmationWait;job->data["Status"]="Verifying";j=job->data;validateGrabberFolder(j);}
 // Assemble beside the confirmed destination, including a folder selected
 // while prefetch was in flight. Parts remain in their existing working area.
 folder=fs::path(wide(str(j,"Folder")));fs::create_directories(folder);
 requirePublicationRecovered(m,job);
 auto staging=folder/(L".udm-"+wide(job->id())+L".assembling");struct StagingCleanup{Manager& manager;JobPtr job;fs::path path;~StagingCleanup(){try{if(!publicationNeedsRecovery(manager,job)){std::error_code error;fs::remove(path,error);}}catch(...){/* Uncertain recovery state retains staging. */}}} cleanup{m,job,staging};
 i64 skip=yes(j,"StripContentPrefix")?num(j,"ContentPrefixBytes"):0;const i64 stripped=skip;std::string digest;bool deferredHash=false,copiedAcrossVolumes=false;i64 complete=0;
  const auto serverSha256=single&&!post?str(j,"ServerSha256"):std::string{};
 if(single){
  // The output file already is the download. Settle its length (a full response
  // may have declared another size than the probe) and make it a regular file
  // again; nothing is copied. The digest continues the prefix hash that trailed
  // the transfer, so only the ranges still open at the last byte are read now.
  if(verifiedPrefetch.singleFile){
   // Keep writes and deletion blocked until the current path is checked.
   Handle held=std::move(verifiedPrefetch.singleFile);
   Handle named(CreateFileW(data.c_str(),FILE_READ_ATTRIBUTES,FILE_SHARE_READ,nullptr,OPEN_EXISTING,FILE_FLAG_OPEN_REPARSE_POINT,nullptr));
   if(!named||stableFileIdentity(named.h)!=verifiedPrefetch.identity)
    throw std::runtime_error("The verified prefetch changed before publication. Resume to check the saved data again.");
   digest=verifiedPrefetch.sha256;
  }
  if(digest.empty()){
   Handle file(CreateFileW(data.c_str(),GENERIC_READ|GENERIC_WRITE,FILE_SHARE_READ,nullptr,OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL,nullptr));if(!file)throw std::runtime_error("Cannot open the downloaded file.");
   const i64 size=num(j,"Size",-1);
   if(size>=0&&fileLength(file.h)!=size)setFileLength(file.h,size);
  }
  phases["Settle"]=phase(mark);
  // The sparse attribute is cleared after publication: on a nearly full or mechanical disk
  // NTFS can take many seconds over it (12 s measured for 384 MiB), and the file is complete
  // and readable with the attribute still set.
  if(skip>0){StreamHash assembled;{Handle output(CreateFileW(staging.c_str(),GENERIC_WRITE,0,nullptr,CREATE_ALWAYS,0,nullptr));if(!output)throw std::runtime_error("Cannot create the assembled file.");std::vector<char> buffer(assemblyBuffer);copyFileTail(data,output.h,*cancel,skip,assembled,buffer);if(!FlushFileBuffers(output.h))throw std::runtime_error("Could not flush the assembled file.");}digest=assembled.finish();}
  else{
   complete=PrefixHasher::contiguous(j["Segments"],0);
    const bool mustVerify=!serverSha256.empty()||!str(j,"ExpectedSha256").empty()||!str(j,"ReplacementOf").empty();
   if(!digest.empty())phases["HashMode"]="verified-prefetch";
   else if(hasher.valid&&hasher.hashed==complete){phases["HashMode"]="trailing";digest=hasher.finish();}
   else if(!mustVerify){deferredHash=true;phases["HashMode"]=hasher.valid?"deferred":"deferred-full";} // the file is complete and usable now; its SHA-256 is recorded a moment later (IDM publishes without hashing at all)
   else{if(hasher.valid){try{hasher.advance(data,complete,*cancel);}catch(const Cancelled&){throw;}catch(...){hasher.valid=false;}}
    const bool trailing=hasher.valid&&hasher.hashed==complete;phases["HashMode"]=trailing?"trailing":"full";digest=trailing?hasher.finish():fileHash(data,[&]{cancel->check();});}
  }
 }else{
  auto segments=j["Segments"];
  if(verifiedNumbered&&verifiedPrefetch.numberedPlan!=segments)
   throw std::runtime_error("The saved prefetch plan changed before assembly.");
  std::sort(segments.begin(),segments.end(),[](const Json& a,const Json& b){return num(a,"Start")<num(b,"Start");});
  StreamHash assembled;
  {
   Handle output(CreateFileW(staging.c_str(),GENERIC_WRITE,0,nullptr,CREATE_ALWAYS,0,nullptr));
   if(!output)throw std::runtime_error("Cannot create the assembled file.");
   std::vector<char> buffer(assemblyBuffer);
   for(const auto& segment:segments){
    const auto index=num(segment,"Index");
    const auto input=verifiedNumbered?verifiedPrefetch.numberedParts.at(index).h:INVALID_HANDLE_VALUE;
    copyFileTail(partPath(parts,index),output.h,*cancel,skip,assembled,buffer,input);
   }
   if(!FlushFileBuffers(output.h))throw std::runtime_error("Could not flush the assembled file.");
  }
  verifiedPrefetch.numberedParts.clear();
  if(verifiedNumbered)phases["HashMode"]="verified-prefetch-parts";
  digest=assembled.finish();
 }
  if(!serverSha256.empty()){
   // The server hashed the original body, even when the user opted to strip a prefix.
   const auto original=stripped>0?fileHash(data,[&]{cancel->check();}):digest;
   if(original!=serverSha256)throw std::runtime_error("The server's SHA-256 checksum did not match. The file was not published.");
  }
  phases["Hash"]=phase(mark);if(!phases.contains("HashMode"))phases["HashMode"]=single?"assembled":"parts";{Lock lock(m.mutex);job->data["FinishPhases"]=phases;}
 if(stripped>0){Lock lock(m.mutex);job->data["StrippedPrefixBytes"]=stripped;}
 if(yes(j,"StripContentPrefix")&&num(j,"ContentPrefixBytes")>0){Bytes head(contentSniffReadLimit);std::ifstream verify(staging,std::ios::binary);verify.read(reinterpret_cast<char*>(head.data()),(std::streamsize)head.size());head.resize((size_t)verify.gcount());auto verdict=sniffContent(head.data(),head.size(),true);if(verdict.verdict!=SniffVerdict::Match||verdict.prefixLength)throw std::runtime_error("Removing the inserted text did not leave a recognizable file. Nothing was published; the received data is kept.");}
 if(!str(j,"ExpectedSha256").empty()&&lower(str(j,"ExpectedSha256"))!=digest)throw std::runtime_error("SHA-256 mismatch. The file was not published.");cancel->check();
 // Optional tags are advisory metadata, never a reason to rename the output.
 {
  const auto audio=str(j,"SniffedType")=="audio/mpeg"?readAudioMetadata(single&&stripped<=0?data:staging):AudioMetadata{};Lock lock(m.mutex);
  job->data.erase("AudioMetadata");
  if(!audio.title.empty()||!audio.artist.empty()||!audio.album.empty())job->data["AudioMetadata"]={{"Source",audio.source},{"Title",audio.title},{"Artist",audio.artist},{"Album",audio.album}};
 }
 if(single&&stripped<=0){
  // Publish the verified file itself. A second link in the destination folder
  // lets publishFile rename it there while the resumable file stays until the
  // catalog has committed completion; another volume or file system gets a copy.
  std::error_code ec;fs::remove(staging,ec);
  if(!CreateHardLinkW(staging.c_str(),data.c_str(),nullptr)){
   // Another volume: a buffered copy lets the cache take the write and the file is complete at once,
   // as IDM's assembly is; CopyFileW copies large files unbuffered and waits on the destination disk
   // (3.7 to 7 s for 384 MiB onto a hard disk). The copy is flushed after publication.
   Handle output(CreateFileW(staging.c_str(),GENERIC_WRITE,0,nullptr,CREATE_ALWAYS,FILE_ATTRIBUTE_NORMAL|FILE_FLAG_SEQUENTIAL_SCAN,nullptr));if(!output)throw std::runtime_error("Cannot stage the downloaded file for publication.");
   copyFileTo(data,output.h,*cancel);copiedAcrossVolumes=true;
  }
 }
 m.publishFile(job,staging,digest,cancel.get());phases["Publish"]=phase(mark);phases["Total"]=std::chrono::duration<double>(mark-networkEnd).count()-confirmationSeconds;{Lock lock(m.mutex);job->data["FinishPhases"]=phases;}markZone(job->target());if(yes(prefs,"UseServerDate")){SYSTEMTIME system{};FILETIME stamp{};auto modified=wide(str(j,"Modified"));if(!modified.empty()&&WinHttpTimeToSystemTime(modified.c_str(),&system)&&SystemTimeToFileTime(&system,&stamp)){Handle f(CreateFileW(job->target().c_str(),FILE_WRITE_ATTRIBUTES,FILE_SHARE_READ|FILE_SHARE_WRITE,nullptr,OPEN_EXISTING,0,nullptr));if(f)SetFileTime(f.h,&stamp,nullptr,&stamp);}}
 // publishFile already committed completion. Cleanup must not turn it into a failed download.
 // The published bytes are written back now, after the catalog shows the file complete
 // (as IDM does), so a slow disk lengthens this thread, not the visible finish; a
 // queued exit or shutdown still waits for it.
 if(single&&stripped<=0){Handle file(CreateFileW(data.c_str(),GENERIC_READ|GENERIC_WRITE,FILE_SHARE_READ|FILE_SHARE_WRITE|FILE_SHARE_DELETE,nullptr,OPEN_EXISTING,0,nullptr));if(file){setSparse(file.h,false);FlushFileBuffers(file.h);}
  if(copiedAcrossVolumes){Handle published(CreateFileW(job->target().c_str(),GENERIC_WRITE,FILE_SHARE_READ|FILE_SHARE_WRITE|FILE_SHARE_DELETE,nullptr,OPEN_EXISTING,0,nullptr));if(published)FlushFileBuffers(published.h);}
  phases["Unsparse"]=phase(mark);{Lock lock(m.mutex);job->data["FinishPhases"]=phases;}}
 if(deferredHash){ // the resumable file is a hard link of the published one; hash it before the parts go. Failure leaves the digest blank rather than failing a complete download.
  std::string late;try{Cancel never;if(hasher.valid){try{hasher.advance(data,complete,never);}catch(...){hasher.valid=false;}}late=hasher.valid&&hasher.hashed==complete?hasher.finish():fileHash(data,{});}catch(...){late.clear();}
  phases["LateHash"]=phase(mark);
  {
   Lock lock(m.mutex);
   if(!late.empty())job->data["Sha256"]=late;
   job->data["FinishPhases"]=phases;
   // A manager-owned worker saves the final record before notifying the UI.
   // Standalone transfers and media tracks still need their own checkpoint.
   const auto worker=m.active.find(job->id());
   if(worker==m.active.end()||worker->second!=cancel)
    try{m.save();}catch(...){/* The completed download was already committed. */}
  }
 }
 try{clearParts(parts);}catch(const fs::filesystem_error&){}
 }catch(...){Lock l(m.mutex);job->data["TransferSeconds"]=prior+std::chrono::duration<double>((networkFinished?networkEnd:std::chrono::steady_clock::now())-started).count()-ftpConfirmationWait;job->data["ElapsedSeconds"]=real(job->data,"ElapsedSeconds")+std::chrono::duration<double>(std::chrono::steady_clock::now()-started).count()-confirmationSeconds-ftpConfirmationWait;throw;}{Lock l(m.mutex);job->data["ElapsedSeconds"]=real(job->data,"ElapsedSeconds")+std::chrono::duration<double>(std::chrono::steady_clock::now()-started).count()-confirmationSeconds-ftpConfirmationWait;}
}
std::string execute(const fs::path& exe,const std::vector<std::wstring>& args,int seconds,const Cancel& cancel){if(!fs::exists(exe))throw std::runtime_error("Media helper missing. Run setup-media.ps1 in the UDM folder.");SECURITY_ATTRIBUTES sa{sizeof(sa),nullptr,TRUE};HANDLE a=nullptr,b=nullptr;if(!CreatePipe(&a,&b,&sa,0))throw std::runtime_error("Cannot create media output pipe.");Handle input(a),output(b);SetHandleInformation(input.h,HANDLE_FLAG_INHERIT,0);Handle nullInput(CreateFileW(L"NUL",GENERIC_READ,FILE_SHARE_READ|FILE_SHARE_WRITE,&sa,OPEN_EXISTING,0,nullptr));std::wstring command=quote(exe.wstring());for(const auto& arg:args)command+=L" "+quote(arg);STARTUPINFOEXW si{};si.StartupInfo.cb=sizeof(si);si.StartupInfo.dwFlags=STARTF_USESTDHANDLES;si.StartupInfo.hStdInput=nullInput.h;si.StartupInfo.hStdOutput=output.h;si.StartupInfo.hStdError=output.h;SIZE_T size=0;InitializeProcThreadAttributeList(nullptr,1,0,&size);Bytes attributes(size);si.lpAttributeList=(PPROC_THREAD_ATTRIBUTE_LIST)attributes.data();if(!InitializeProcThreadAttributeList(si.lpAttributeList,1,0,&size))throw std::runtime_error("Cannot initialize media process.");HANDLE inherited[]={nullInput.h,output.h};if(!UpdateProcThreadAttribute(si.lpAttributeList,0,PROC_THREAD_ATTRIBUTE_HANDLE_LIST,inherited,sizeof(inherited),nullptr,nullptr)){DeleteProcThreadAttributeList(si.lpAttributeList);throw std::runtime_error("Cannot restrict inherited media handles.");}PROCESS_INFORMATION pi{};BOOL created=CreateProcessW(exe.c_str(),command.data(),nullptr,nullptr,TRUE,CREATE_NO_WINDOW|EXTENDED_STARTUPINFO_PRESENT,nullptr,exe.parent_path().c_str(),&si.StartupInfo,&pi);DeleteProcThreadAttributeList(si.lpAttributeList);if(!created)throw std::runtime_error("Could not start the media helper.");Handle process(pi.hProcess),thread(pi.hThread);CloseHandle(output.h);output.h=INVALID_HANDLE_VALUE;std::string text;auto deadline=epoch()+seconds*1000LL;try{for(;;){cancel.check();if(epoch()>deadline)throw std::runtime_error("Media helper timed out.");DWORD available=0;while(PeekNamedPipe(input.h,nullptr,0,nullptr,&available,nullptr)&&available){char buffer[4096];DWORD n=0;if(!ReadFile(input.h,buffer,std::min<DWORD>(sizeof(buffer),available),&n,nullptr)||!n)break;text.append(buffer,n);if(text.size()>2*1024*1024)text.erase(0,text.size()-2*1024*1024);}if(WaitForSingleObject(process.h,20)==WAIT_OBJECT_0){while(PeekNamedPipe(input.h,nullptr,0,nullptr,&available,nullptr)&&available){char buffer[4096];DWORD n=0;if(!ReadFile(input.h,buffer,std::min<DWORD>(sizeof(buffer),available),&n,nullptr)||!n)break;text.append(buffer,n);if(text.size()>2*1024*1024)text.erase(0,text.size()-2*1024*1024);}break;}}}catch(...){TerminateProcess(process.h,1);WaitForSingleObject(process.h,5000);throw;}DWORD code=1;GetExitCodeProcess(process.h,&code);if(code){text=std::regex_replace(text,std::regex("https?://[^\\s]+"),"[URL]");if(text.size()>1500)text=text.substr(text.size()-1500);throw std::runtime_error(utf8(exe.filename().wstring())+": "+text);}return text;}
void validateSource(const std::string& s){Url u(s);if(u.scheme!="https"||!(hostIs(u.host,"youtube.com")||u.host=="youtu.be"))throw std::runtime_error("Media capture requires an HTTPS YouTube page.");}
void validateStream(const std::string& s){Url u(s);if(u.scheme!="https"||u.host=="googlevideo.com"||!hostIs(u.host,"googlevideo.com")||u.path!="/videoplayback")throw std::runtime_error("Expected a YouTube playback stream.");}
void setStreams(Manager& m,JobPtr job,const std::string& video,const std::string& audio){const bool audioOnly=str(job->data,"MediaOutput")=="audio";if(audioOnly){if(!video.empty()||audio.empty())throw std::runtime_error("Audio-only capture requires only an audio stream.");}else validateStream(video);if(!audio.empty())validateStream(audio);Lock l(m.mutex);auto child=[&](const std::string& url,const std::string& kind,int n){return std::make_shared<Job>(Json{{"Id",guid()},{"Url",url},{"Folder",utf8(mediaWorkingDirectory(m,job).wstring())},{"FileName",kind+".mp4"},{"Queue",str(job->data,"Queue")},{"Status","Queued"},{"Connections",n},{"ProtectedHeaders",str(job->data,"ProtectedHeaders")},{"Size",-1},{"Segments",Json::array()}});};int n=(int)num(job->data,"Connections",8);if(!audioOnly){if(!job->video)job->video=child(video,"video",std::max(1,n-(audio.empty()?0:2)));else job->video->data["Url"]=video;}if(!audio.empty()){if(!job->audio)job->audio=child(audio,"audio",audioOnly?n:std::min(2,std::max(1,n/2)));else job->audio->data["Url"]=audio;}}
// Completed tracks have no range parts left. Validate them before skipping their
// transfer after a session refresh; keep both tracks unchanged on any failure.
static void verifyRefreshedCompleteTracks(Manager& m,JobPtr parent,const std::shared_ptr<Cancel>& cancel){
 struct Verified {JobPtr job;Json data;};std::vector<Verified> verified;Rate rate;
 for(auto child:{parent->video,parent->audio})if(child){
  Json data,prefs;Headers headers;{Lock lock(m.mutex);data=child->data;if(!yes(data,"CompletedMediaRefreshPendingValidation"))continue;prefs=browserSessionPreferences(browserProxyPreferences(m.state["Settings"],data),data);headers=readHeaders(data);}
  if(str(data,"Status")!="Complete")continue;
  const auto size=num(data,"Size",-1);const auto savedHash=str(data,"Sha256");
  if(size<1||!std::regex_match(savedHash,std::regex("[a-fA-F0-9]{64}"))||!fs::is_regular_file(child->target())||fs::file_size(child->target())!=(uintmax_t)size||fileHash(child->target(),[&]{cancel->check();})!=savedHash)throw std::runtime_error("A saved media track is missing or changed. Saved data was kept; start a separate download.");
  {Lock lock(m.mutex);parent->data["MediaPhase"]="Verifying saved tracks";child->workers.assign(1,Worker{});child->workers[0].number=1;child->workers[0].state="Verifying saved track";}
  const auto url=str(data,"Url");auto pool=std::make_shared<HttpSession>(prefs,browserSessionSaver(m,child,prefs.value("ActiveBrowserSession",Json::object())));
  auto response=std::make_unique<Http>(url,headers,prefs,*cancel,0,0,"",nullptr,true,pool);success(*response);ContentRange range(response->header(L"Content-Range"));
  bool ranged=response->status==206&&range.valid&&range.start==0&&range.end==0&&range.total==size;
  if(response->status==206&&!ranged)throw std::runtime_error("The refreshed media track has a different size or invalid byte range. Saved data was kept.");
  const auto tag=etag(*response),modified=response->header(L"Last-Modified");
  bool same=ranged&&(!tag.empty()?str(data,"ETag")==tag:str(data,"ETag").empty()&&!modified.empty()&&str(data,"Modified")==modified);
  if(ranged&&response->all(1,*cancel).size()!=1)throw std::runtime_error("Incomplete refreshed media probe.");
  if(!same){
   // Missing or rotated validators need exact comparison. Stream against the
   // retained file instead of allocating another potentially large disk copy.
   if(ranged)response=std::make_unique<Http>(url,headers,prefs,*cancel,std::nullopt,std::nullopt,"",nullptr,true,pool);
   success(*response);if(response->status!=200||(length(response->header(L"Content-Length"))>=0&&length(response->header(L"Content-Length"))!=size))throw std::runtime_error("The refreshed media track cannot be compared with the saved file. Saved data was kept.");
   std::ifstream local(child->target(),std::ios::binary);if(!local)throw std::runtime_error("Cannot read the saved media track.");i64 read=0;char incoming[65536],saved[65536];
   for(;;){auto n=response->read(incoming,sizeof(incoming),*cancel);if(!n)break;m.charge(n,*cancel,rate,parent);{Lock lock(m.mutex);child->data["TransferredBytes"]=num(child->data,"TransferredBytes")+(i64)n;child->workers[0].received+=(i64)n;child->workers[0].position+=(i64)n;}
    if((i64)n>size-read)throw std::runtime_error("The refreshed media track is larger than the saved file. Saved data was kept.");local.read(saved,n);if(local.gcount()!=(std::streamsize)n||memcmp(saved,incoming,n)!=0)throw std::runtime_error("The refreshed media track differs from the saved file. Saved data was kept; choose the original streams or start a separate download.");read+=(i64)n;
   }
   if(read!=size)throw std::runtime_error("The refreshed media track ended early. Saved data was kept.");
  }
  cancel->check();Json next=data;next["ETag"]=etag(*response).empty()?Json():Json(etag(*response));next["Modified"]=response->header(L"Last-Modified").empty()?Json():Json(response->header(L"Last-Modified"));next["CompletedMediaRefreshPendingValidation"]=false;next["RefreshPendingValidation"]=false;verified.push_back({child,next});
 }
 if(verified.empty())return;cancel->check();Lock lock(m.mutex);std::vector<Json> before;for(auto& item:verified){before.push_back(item.job->data);for(const auto* key:{"ETag","Modified","CompletedMediaRefreshPendingValidation","RefreshPendingValidation"})item.job->data[key]=item.data[key];}
 try{m.save();}catch(...){for(size_t i=0;i<verified.size();++i)verified[i].job->data=before[i];throw;}
}

std::string retainedYouTubeOutputBinding(const Job& job){
 Json selection=Json::object();
 for(const auto* key:{"SourceUrl","ProtectedSabr","MediaOutput","MediaFormatId","MediaHeight","MediaPixelHeight","ExactMediaQuality"})
  if(job.data.contains(key))selection[key]=job.data.at(key);
 for(const auto* key:{"Video","Audio"}){
  const auto& track=key[0]=='V'?job.video:job.audio;
  if(track)selection[key]={{"Url",str(track->data,"Url")},{"Headers",str(track->data,"ProtectedHeaders")}};
 }
 const auto encoded=selection.dump();StreamHash hash;hash.update(encoded.data(),encoded.size());return hash.finish();
}
static void cleanPublishedMediaTracks(const JobPtr& job,const fs::path& media){
 for(const auto& child:{job->video,job->audio})if(child){std::error_code ignored;fs::remove(child->target(),ignored);}
 try{cleanStreamCache(media);}catch(const fs::filesystem_error&){}
}
void mediaTransfer(Manager& m,JobPtr job,const std::shared_ptr<Cancel>& c){requirePublicationRecovered(m,job);double confirmationSeconds=0;auto started=std::chrono::steady_clock::now();Json j;{Lock l(m.mutex);j=job->data;}validateSource(str(j,"SourceUrl"));const auto stagingFolder=fs::path(wide(str(j,"Folder")));const bool audioOnly=str(j,"MediaOutput")=="audio";if(audioOnly?(!job->audio||bool(job->video)):!job->video)throw std::runtime_error("Fresh browser-captured links are required. Choose the quality again in the browser.");try{auto media=mediaWorkingDirectory(m,job);if(publishRetainedMediaOutput(m,job,c,retainedYouTubeOutputBinding)){cleanPublishedMediaTracks(job,media);return;}m.checkpointMediaStart(job);auto tools=appDir()/L"tools";if(!fs::exists(tools/L"ffmpeg.exe"))throw std::runtime_error("Media helper missing. Run setup-media.ps1.");fs::create_directories(media);fs::create_directories(stagingFolder);for(auto child:{job->video,job->audio})if(child){Lock l(m.mutex);child->data["Folder"]=utf8(media.wstring());child->data["FileName"]=child==job->video?"video.mp4":"audio.mp4";if(str(child->data,"Status")=="Complete"&&!fs::exists(child->target())&&!yes(child->data,"CompletedMediaRefreshPendingValidation"))child->data["Status"]="Paused";if(str(j,"ProtectedSabr").empty()&&str(child->data,"Status")!="Complete"){auto q=query(str(child->data,"Url"));if(q.count("expire")&&std::stoll(q["expire"])*1000<epoch()+120000)throw std::runtime_error("Captured links expired. Use Refresh download address, then choose the same quality in the browser.");}}
 auto network=std::chrono::steady_clock::now();
 if(!str(j,"ProtectedSabr").empty()){
  if(verifiedMediaTracks(job,*c,&m)){Lock lock(m.mutex);job->data["ReusedMediaTracks"]=true;job->data["MediaTracksReady"]=true;}
  else sabrTransfer(m,job,c,{},[&]{m.ensureConnection(job,*c);});
 }else{
  bool needsNetwork=false;
  {Lock lock(m.mutex);for(const auto& track:{job->video,job->audio})if(track)
   needsNetwork|=str(track->data,"Status")!="Complete"||yes(track->data,"CompletedMediaRefreshPendingValidation");}
  if(needsNetwork)m.ensureConnection(job,*c);
  verifyRefreshedCompleteTracks(m,job,c);
  const auto connections=(int)std::clamp<i64>(num(j,"Connections",8),1,32);
  std::vector<JobPtr> tracks;
  {
   Lock lock(m.mutex);
   for(const auto& track:{job->video,job->audio})if(track&&str(track->data,"Status")!="Complete"){
    track->data["Connections"]=connections;
    track->data["Queue"]=str(j,"Queue");
    tracks.push_back(track);
   }
   // Both tracks share the parent cap. With a single connection they run
   // serially. A lone unfinished track can use the whole cap.
   if(tracks.size()==2&&connections>1){
    const auto audioConnections=std::min(2,connections/2);
    job->audio->data["Connections"]=audioConnections;
    job->video->data["Connections"]=connections-audioConnections;
   }
  }
  auto group=std::make_shared<Cancel>();group->parent=c;auto rate=std::make_shared<Rate>();std::mutex errorMutex;std::exception_ptr error;std::vector<std::future<void>> tasks;tasks.reserve(2);
 // Record the initiating failure before cancelling its companion. Joining video
 // first must not replace an audio HTTP error with "Download paused".
 auto fail=[&]{std::lock_guard<std::mutex> lock(errorMutex);if(!error)error=std::current_exception();group->stop=true;};
 auto run=[&](const JobPtr& track){
  try{group->check();transfer(m,track,group,job,rate);}catch(...){fail();}
 };
 if(connections==1||tracks.size()==1){
  for(const auto& track:tracks){run(track);if(error)break;}
 }else{
  try{for(const auto& track:tracks)tasks.push_back(std::async(std::launch::async,run,track));}catch(...){fail();}
 }
 for(auto& task:tasks)task.get();c->check();if(error)std::rethrow_exception(error);} {
  Lock l(m.mutex);const auto networkEnd=std::chrono::steady_clock::now();
  if(yes(job->data,"ConfirmationPending")){
   job->syncTrackProgress();
   job->samplePrefetchRate(networkEnd,true);
  }
  job->data["TransferSeconds"]=real(job->data,"TransferSeconds")+std::chrono::duration<double>(networkEnd-network).count();m.save();
 }
 // Assemble on the worker so the dialog stays responsive. Measure the held
 // staging file while its checksum is verified. Publication waits for confirmation.
 requirePublicationRecovered(m,job);{Lock lock(m.mutex);job->data["Status"]="Merging";job->data["MediaPhase"]="Assembling file";m.save();}auto staging=stagingFolder/(L".udm-"+wide(job->id())+(audioOnly?L".muxing.m4a":L".muxing.mp4"));auto merge=std::chrono::steady_clock::now();std::vector<std::wstring> args={L"-hide_banner",L"-loglevel",L"error",L"-nostdin",L"-y",L"-protocol_whitelist",L"file,pipe",L"-i",(audioOnly?job->audio:job->video)->target().wstring()};if(audioOnly)args.insert(args.end(),{L"-map",L"0:a:0",L"-vn"});else if(job->audio){args.insert(args.end(),{L"-protocol_whitelist",L"file,pipe",L"-i",job->audio->target().wstring(),L"-map",L"0:v:0",L"-map",L"1:a:0"});}else args.insert(args.end(),{L"-map",L"0:v:0",L"-map",L"0:a:0"});args.insert(args.end(),{L"-c",L"copy",L"-movflags",L"+faststart",staging.wstring()});try{
 const auto primary=(audioOnly?job->audio:job->video)->target();
 std::string capturedHash;
 if(!audioOnly&&!job->audio&&str(j,"ProtectedSabr").empty()){
  Lock lock(m.mutex);capturedHash=str(job->video->data,"Sha256");
 }
 const auto primaryProbe=capturedHash.empty()?Json():mediaAssemblyProbe(primary,*c);
 const bool preserveMuxed=!capturedHash.empty()&&canPreserveMuxedMp4(primaryProbe);
 // A previous attempt may have left a staging name linked to the source.
 // Unlink it before either path can create or write a replacement.
 fs::remove(staging);
 if(preserveMuxed){
  if(!CreateHardLinkW(staging.c_str(),primary.c_str(),nullptr)){
   Handle output(CreateFileW(staging.c_str(),GENERIC_WRITE,0,nullptr,CREATE_NEW,FILE_ATTRIBUTE_NORMAL,nullptr));
   if(!output)throw std::runtime_error("Cannot prepare the captured MP4.");
   copyFileTo(primary,output.h,*c);
   if(!FlushFileBuffers(output.h))throw std::runtime_error("Cannot flush the captured MP4.");
  }
 }else execute(tools/L"ffmpeg.exe",args,300,*c);
 if(!fs::exists(staging)||!fs::file_size(staging))throw std::runtime_error("Media assembly produced no output.");
 {Lock lock(m.mutex);job->data["MediaPhase"]="Verifying output";}
 // The preserved output still gets its own probe and must match the saved
 // source digest. Reuse the metadata already read to select this path.
 verifyMediaAssembly(staging,primary,!audioOnly&&job->audio?std::optional<fs::path>{job->audio->target()}:std::optional<fs::path>{},audioOnly,j,*c,preserveMuxed?&primaryProbe:nullptr);
 VerifiedMediaFile verified(staging);
 ScopedAssembledPrefetchSize displaySize(m,*job,*c,verified.size);
 auto digest=fileHash(verified.file.h,[&]{c->check();});
 if(preserveMuxed&&digest!=capturedHash)throw std::runtime_error("The captured MP4 failed verification. Saved streams are retained.");
 if(!str(j,"ExpectedSha256").empty()&&lower(str(j,"ExpectedSha256"))!=digest)throw std::runtime_error("Media SHA-256 does not match the expected value. Saved streams are retained.");
 c->check();const auto mergeSeconds=std::chrono::duration<double>(std::chrono::steady_clock::now()-merge).count();
 const auto confirmationStarted=std::chrono::steady_clock::now();
 auto recordConfirmationWait=[&]{confirmationSeconds+=std::chrono::duration<double>(std::chrono::steady_clock::now()-confirmationStarted).count();};
 const auto verifiedSize=verified.size;
 Json prepared;
 {Lock lock(m.mutex);prepared={{"Version",1},{"Path",utf8(staging.wstring())},{"Size",verifiedSize},{"Sha256",digest},{"Binding",retainedYouTubeOutputBinding(*job)},{"MergeSeconds",real(job->data,"MergeSeconds")+mergeSeconds}};}
 try{m.waitForPrefetchConfirmation(job,*c,verifiedSize,prepared);}catch(...){recordConfirmationWait();throw;}recordConfirmationWait();
 {Lock lock(m.mutex);job->data["MergeSeconds"]=real(job->data,"MergeSeconds")+mergeSeconds;}
 verified.releaseForPublication(staging);
 publishMediaOutput(m,job,staging,digest,c);markZone(job->target());
 {Lock lock(m.mutex);job->data["MediaPhase"]="Complete";}
 // Completion is already committed. Cache cleanup cannot invalidate it.
 cleanPublishedMediaTracks(job,media);
}catch(...){bool retained;{Lock lock(m.mutex);retained=retainedMediaOutputPath(job->data)==staging;job->data.erase("PrefetchedFinalSize");}std::error_code ec;if(!retained&&!publicationNeedsRecovery(m,job))fs::remove(staging,ec);throw;}}catch(...){Lock l(m.mutex);job->data["MediaPhase"]=c->cancelled()?"Paused":"Failed";job->data.erase("PrefetchedFinalSize");job->data["ElapsedSeconds"]=real(job->data,"ElapsedSeconds")+std::max(0.0,std::chrono::duration<double>(std::chrono::steady_clock::now()-started).count()-confirmationSeconds);throw;}{Lock l(m.mutex);job->data["ElapsedSeconds"]=real(job->data,"ElapsedSeconds")+std::max(0.0,std::chrono::duration<double>(std::chrono::steady_clock::now()-started).count()-confirmationSeconds);}
}
}
