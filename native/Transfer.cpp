#include "MediaStorage.hpp"
#include "StreamCache.hpp"
#include "Core.hpp"
#include "Ftp.hpp"
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
#include <wininet.h>
#include <fstream>
#include <sstream>
#include <regex>
#include <future>
#include <algorithm>
#include <condition_variable>
namespace udm {
struct InternetFailure:std::runtime_error {DWORD code;InternetFailure(const char* operation,DWORD error):runtime_error(std::string(operation)+" failed (Windows error "+std::to_string(error)+")."),code(error){}};
static void internetError(const char* operation){throw InternetFailure(operation,GetLastError());}
HttpSession::HttpSession(const Json& prefs,std::function<void(const Json&)> saveBrowserSession):preferences(prefs) {
 if(prefs.contains("ActiveBrowserSession"))browserCookies=std::make_unique<BrowserCookieJar>(prefs["ActiveBrowserSession"],std::move(saveBrowserSession)); validateProtocolProxies(prefs);validatePacSettings(prefs);
 auto proxy=wide(str(prefs,"Proxy")),bypass=wide(str(prefs,"ProxyBypass"));auto mode=str(prefs,"ProxyMode",proxy.empty()?"Use Windows proxy / PAC settings":"Use a proxy server");DWORD access=(mode=="Connect directly"||usesPacScript(prefs))?WINHTTP_ACCESS_TYPE_NO_PROXY:mode=="Use a proxy server"?WINHTTP_ACCESS_TYPE_NAMED_PROXY:WINHTTP_ACCESS_TYPE_AUTOMATIC_PROXY;
 if(isSocksProxy(prefs)){socks=std::make_unique<SocksProxy>(prefs);proxy=socks->address();access=WINHTTP_ACCESS_TYPE_NAMED_PROXY;}
 auto userAgent=wide(str(prefs,"UserAgent").empty()?"UDM/0.35.0":str(prefs,"UserAgent"));session=WinHttpOpen(userAgent.c_str(),access,access==WINHTTP_ACCESS_TYPE_NAMED_PROXY?proxy.c_str():WINHTTP_NO_PROXY_NAME,bypass.empty()?WINHTTP_NO_PROXY_BYPASS:bypass.c_str(),WINHTTP_FLAG_ASYNC);
 if(!session)internetError("HTTP initialization");
 if(!WinHttpSetTimeouts(session,15000,15000,30000,30000)){
  auto error=GetLastError();WinHttpCloseHandle(session);session=nullptr;SetLastError(error);internetError("HTTP timeouts");
 }
 DWORD protocols=WINHTTP_FLAG_SECURE_PROTOCOL_TLS1_2|(yes(prefs,"UseTls13",true)?WINHTTP_FLAG_SECURE_PROTOCOL_TLS1_3:0);
 if(!WinHttpSetOption(session,WINHTTP_OPTION_SECURE_PROTOCOLS,&protocols,sizeof(protocols))){
  auto error=GetLastError();protocols=WINHTTP_FLAG_SECURE_PROTOCOL_TLS1_2;
  if(error!=ERROR_INVALID_PARAMETER||!WinHttpSetOption(session,WINHTTP_OPTION_SECURE_PROTOCOLS,&protocols,sizeof(protocols))){error=GetLastError();WinHttpCloseHandle(session);session=nullptr;SetLastError(error);internetError("TLS configuration");}
 }
 // Negotiate HTTP/2 when the origin supports it; HTTP/1.1 remains available.
 // Older Windows versions may not expose this optional protocol selector.
 DWORD httpProtocols=WINHTTP_PROTOCOL_FLAG_HTTP2;
 if(!WinHttpSetOption(session,WINHTTP_OPTION_ENABLE_HTTP_PROTOCOL,&httpProtocols,sizeof(httpProtocols))){
  auto error=GetLastError();if(error!=ERROR_WINHTTP_INVALID_OPTION&&error!=ERROR_INVALID_PARAMETER){WinHttpCloseHandle(session);session=nullptr;SetLastError(error);internetError("HTTP protocol configuration");}
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
     explicitProxy=std::make_unique<CurlHttp>(current,h,pool->settings(),c,body,head,std::pair<std::string,std::string>{},body?nullptr:pool->curlSession());
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
      explicitProxy=std::make_unique<CurlHttp>(current,h,pool->settings(),c,body,head,login,body?nullptr:pool->curlSession());
      status=explicitProxy->status();common->receiveBrowserCookies(current,explicitProxy->receivedCookies());
     }
    }else{
     validateSocksDestination(u,pool->settings());
session=pool->handle();if(u.scheme!="http"&&u.scheme!="https")throw std::runtime_error("HTTP redirect uses an unsupported protocol.");auto host=wide(u.host),path=wide(u.path+u.query);connection=pool->connect(u);if(!connection)internetError("HTTP connection");request=WinHttpOpenRequest(connection,head?L"HEAD":body?L"POST":L"GET",path.c_str(),nullptr,WINHTTP_NO_REFERER,WINHTTP_DEFAULT_ACCEPT_TYPES,u.scheme=="https"?WINHTTP_FLAG_SECURE:0);if(!request)internetError("HTTP request");async=std::make_unique<HttpAsyncState>();DWORD_PTR context=reinterpret_cast<DWORD_PTR>(async.get());if(!WinHttpSetOption(request,WINHTTP_OPTION_CONTEXT_VALUE,&context,sizeof(context)))internetError("HTTP context");if(WinHttpSetStatusCallback(request,HttpAsyncState::callback,WINHTTP_CALLBACK_FLAG_ALL_COMPLETIONS|WINHTTP_CALLBACK_FLAG_HANDLES,0)==WINHTTP_INVALID_STATUS_CALLBACK)internetError("HTTP callback");async->registered=true;DWORD disabled=WINHTTP_DISABLE_REDIRECTS|WINHTTP_DISABLE_COOKIES;WinHttpSetOption(request,WINHTTP_OPTION_DISABLE_FEATURE,&disabled,sizeof(disabled));DWORD autologon=WINHTTP_AUTOLOGON_SECURITY_LEVEL_HIGH;WinHttpSetOption(request,WINHTTP_OPTION_AUTOLOGON_POLICY,&autologon,sizeof(autologon));
 pool->proxyCredentials(request);
 auto login=basicLogin(effective);bool digestRetried=false;
 for(;;){
  prepareOperation();awaitOperation(WinHttpSendRequest(request,digestRetried?WINHTTP_NO_ADDITIONAL_HEADERS:h.c_str(),digestRetried?0:(DWORD)h.size(),body?(void*)body->data():WINHTTP_NO_REQUEST_DATA,body?(DWORD)body->size():0,body?(DWORD)body->size():0,context),c,"HTTP send");
  prepareOperation();awaitOperation(WinHttpReceiveResponse(request,nullptr),c,"HTTP response");DWORD size=sizeof(status);if(!WinHttpQueryHeaders(request,WINHTTP_QUERY_STATUS_CODE|WINHTTP_QUERY_FLAG_NUMBER,nullptr,&status,&size,nullptr))internetError("HTTP status");
  common->receiveBrowserCookies(current,this->headers(L"Set-Cookie"));
  if(status==401&&sensitive&&!digestRetried&&!login.first.empty()){
   DWORD supported=0,first=0,target=0;
   if(WinHttpQueryAuthSchemes(request,&supported,&first,&target)&&target==WINHTTP_AUTH_TARGET_SERVER&&(supported&WINHTTP_AUTH_SCHEME_DIGEST)){
    if(!WinHttpAddRequestHeaders(request,L"Authorization:",(DWORD)-1,WINHTTP_ADDREQ_FLAG_REPLACE))internetError("HTTP authentication header");
    auto name=wide(login.first),secret=wide(login.second);if(!WinHttpSetCredentials(request,WINHTTP_AUTH_TARGET_SERVER,WINHTTP_AUTH_SCHEME_DIGEST,name.c_str(),secret.c_str(),nullptr))internetError("HTTP authentication");
    auto refreshed=common->browserHeaders(current,effective);if(prefs.contains("ActiveBrowserSession")){auto cookie=L"Cookie: "+wide(headerValue(refreshed,"Cookie"));if(!WinHttpAddRequestHeaders(request,cookie.c_str(),(DWORD)-1,WINHTTP_ADDREQ_FLAG_REPLACE|WINHTTP_ADDREQ_FLAG_ADD))internetError("HTTP cookie refresh");}
    digestRetried=true;continue;
   }
  }break;
 }

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
Bytes Http::all(size_t limit,const Cancel& c){Bytes out,b(65536);for(;;){auto n=read(b.data(),b.size(),c);if(!n)return out;if(out.size()+n>limit)throw std::runtime_error("Response exceeds the permitted size.");out.insert(out.end(),b.begin(),b.begin()+n);}}
static i64 length(const std::string& s){if(s.empty())return -1;if(!std::regex_match(s,std::regex("[0-9]+")))throw std::runtime_error("Invalid Content-Length.");try{return std::stoll(s);}catch(...){throw std::runtime_error("Content-Length overflow.");}}
struct ContentRange{i64 start=-1,end=-1,total=-1;bool valid=false;explicit ContentRange(const std::string& s){std::smatch m;if(std::regex_match(s,m,std::regex("bytes ([0-9]+)-([0-9]+)/([0-9]+)"))){start=length(m[1]);end=length(m[2]);total=length(m[3]);valid=start<=end&&end<total;}else if(s=="bytes */0"){total=0;valid=true;}}};
static void success(const Http& r){if(r.status==401&&!r.request){for(const auto& challenge:r.headers(L"WWW-Authenticate")){std::smatch method;if(std::regex_search(challenge,method,std::regex("(^|,)\\s*(Digest|Basic)\\s",std::regex::icase)))throw AuthenticationRequired(Url(r.finalUrl).origin,lower(method[2])=="digest"?"Digest":"Basic");}}if(r.status==401){DWORD supported=0,first=0,target=0;if(WinHttpQueryAuthSchemes(r.request,&supported,&first,&target)&&target==WINHTTP_AUTH_TARGET_SERVER&&(supported&(WINHTTP_AUTH_SCHEME_BASIC|WINHTTP_AUTH_SCHEME_DIGEST)))throw AuthenticationRequired(Url(r.finalUrl).origin,(supported&WINHTTP_AUTH_SCHEME_DIGEST)?"Digest":"Basic");}if(r.status<200||r.status>=300)throw HttpRejected(r.status,r.header(L"Retry-After"));auto enc=lower(r.header(L"Content-Encoding"));if(!enc.empty()&&enc!="identity")throw std::runtime_error("The server ignored identity encoding.");}
static fs::path partPath(const fs::path& folder,i64 index){wchar_t b[32];swprintf_s(b,L"%04lld.part",index);return folder/b;}
static void clearParts(const fs::path& folder){if(fs::exists(folder))for(const auto& p:fs::directory_iterator(folder))if(p.is_regular_file()&&p.path().extension()==L".part")fs::remove(p.path());}
static std::string etag(const Http& r){auto s=r.header(L"ETag");return s.rfind("W/",0)==0?"":s;}
static void copyFileTo(const fs::path& path,HANDLE output,const Cancel& c){std::ifstream input(path,std::ios::binary);if(!input)throw std::runtime_error("A required partial file is missing.");char b[65536];while(input){c.check();input.read(b,sizeof(b));auto n=input.gcount();if(n){DWORD written=0;if(!WriteFile(output,b,(DWORD)n,&written,nullptr)||written!=n)throw std::runtime_error("Could not assemble the download.");}}if(!input.eof())throw std::runtime_error("Could not read a partial download.");}
// A saved range plan must cover the resource exactly, independent of part-file IDs.
static bool completePlan(const Json& segments,i64 size){
 if(!segments.is_array()||segments.empty())return size==0;
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

static void transferHttp(Manager& m,JobPtr job,const std::shared_ptr<Cancel>& cancel,
 const Json& prefs,const Headers& headers,const std::string& url,const fs::path& parts,
 JobPtr limitOwner,const std::shared_ptr<Rate>& rate,std::shared_ptr<HttpSession> pool={}){
 if(!pool)pool=std::make_shared<HttpSession>(prefs,browserSessionSaver(m,job,prefs.value("ActiveBrowserSession",Json::object())));int retryBudget;
 {Lock lock(m.mutex);retryBudget=m.retries(str(job->data,"Queue"));}
 for(int generation=0;;++generation){
  std::unique_ptr<Http> initialResponse;
  {
   std::unique_ptr<Http> probe;
   auto retryConnection=[&](DWORD code,int attempt){
    if(attempt>=retryBudget||(code!=ERROR_WINHTTP_CANNOT_CONNECT&&code!=ERROR_WINHTTP_CONNECTION_ERROR&&code!=ERROR_WINHTTP_NAME_NOT_RESOLVED&&code!=ERROR_WINHTTP_TIMEOUT))return false;
    probe.reset();auto delay=std::min(10000,500*(1<<std::min(attempt,4)));
    {Lock lock(m.mutex);job->workers.assign(1,Worker{});job->workers[0].number=1;job->workers[0].state="Retrying connection";}
    cancel->wait(delay);return true;
   };
   for(int attempt=0;;++attempt){
    try{
     probe=std::make_unique<Http>(url,headers,prefs,*cancel,0,0,"",nullptr,true,pool);
     if(probe->status!=416)success(*probe);
     if(probe->status==206){
      ContentRange range(probe->header(L"Content-Range"));
      if(!range.valid||range.start!=0||range.end!=0||range.total<=0)throw std::runtime_error("Server returned an invalid probe range.");
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
    }catch(const HttpRejected& error){
     probe.reset();if(!error.retryable()||attempt>=retryBudget)throw;
     auto delay=error.delay(attempt);
     {Lock lock(m.mutex);job->workers.assign(1,Worker{});job->workers[0].number=1;job->workers[0].state="Server busy; retrying in "+std::to_string((delay+999)/1000)+" s";}
     cancel->wait(delay);
    }
   }
   auto& response=*probe;
   ContentRange range(response.header(L"Content-Range"));
   bool empty=response.status==416&&range.total==0;
   if(!empty)success(response);
   bool ranges=response.status==206&&range.valid&&range.start==0&&range.end==0&&range.total>0;
   if(response.status==206&&!ranges)throw std::runtime_error("Server returned an invalid probe range.");
   i64 size=empty?0:ranges?range.total:length(response.header(L"Content-Length"));
   auto tag=etag(response),modified=response.header(L"Last-Modified");
   bool validator=!tag.empty()||!modified.empty();
   Lock lock(m.mutex);auto& data=job->data;data["ContentType"]=previewMimeType(response.header(L"Content-Type"));data["ProtectedResolvedUrl"]=response.finalUrl!=url?protect(response.finalUrl):"";
   const bool ignoreDate=yes(prefs,"IgnoreLastModified")&&!yes(data,"RefreshPendingValidation");
   bool same=num(data,"Size",-1)==size&&(!tag.empty()?str(data,"ETag")==tag:str(data,"ETag").empty()&&!modified.empty()&&!str(data,"Modified").empty()&&(ignoreDate||str(data,"Modified")==modified));
   auto& segments=data["Segments"];
   bool savedBytes=num(data,"Received")>0;
   if(yes(data,"RefreshPendingValidation"))for(const auto& segment:segments){auto file=partPath(parts,num(segment,"Index"));if(fs::exists(file)&&fs::file_size(file)>0)savedBytes=true;}
   if(yes(data,"RefreshPendingValidation")&&savedBytes&&(!same||!ranges||!validator||!completePlan(segments,size)))
    throw std::runtime_error("The new address could not be verified as the same resumable file. Saved partial data was kept. Try another link, or add a separate new download.");
   data["RefreshPendingValidation"]=false;
   if(!same||!ranges||!validator||!completePlan(segments,size)){
    clearParts(parts);segments=Json::array();data["Received"]=0;data["DynamicSplits"]=0;
   }
   data["Size"]=size;data["ETag"]=tag.empty()?Json():Json(tag);data["Modified"]=modified.empty()?Json():Json(modified);
   data["RangeSupported"]=ranges&&validator;
   if(segments.empty()&&size!=0){
    if(ranges&&validator){
     i64 connections=std::clamp<i64>(num(data,"Connections",8),1,32);
     // Start one contiguous range per worker. Idle workers can subdivide slow tails.
     // Avoid paying request/response startup latency for four rounds of fixed chunks.
     i64 partitions=connections;
     i64 chunk=std::max<i64>(1048576,size/partitions+(size%partitions!=0));
     int index=0;
     for(i64 begin=0;begin<size;){auto count=std::min(chunk,size-begin);segments.push_back({{"Index",index++},{"Start",begin},{"End",begin+count-1},{"Done",0}});begin+=count;}
    }else segments.push_back({{"Index",0},{"Start",0},{"End",size-1},{"Done",0}});
   }
   i64 received=0;
   for(auto& segment:segments){
    auto file=partPath(parts,num(segment,"Index"));i64 have=fs::exists(file)?(i64)fs::file_size(file):0;
    if(num(segment,"End")>=num(segment,"Start")&&have>num(segment,"End")-num(segment,"Start")+1){fs::remove(file);have=0;}
    segment["Done"]=have;received+=have;
   }
   data["Received"]=received;m.save();
   // A server may ignore Range and return the entire one-use download. Preserve
   // that unread body for the single worker rather than issuing another GET.
   if(response.status==200)initialResponse=std::move(probe);
  }

  auto group=std::make_shared<Cancel>();group->parent=cancel;
  // Owners and all boundary/progress changes are protected by Manager::mutex.
  // -1 = unassigned, -2 = complete; nonnegative values identify an active worker.
  std::vector<int> owners;int connections,retries;bool ranges;
  {
   Lock lock(m.mutex);ranges=yes(job->data,"RangeSupported");
   connections=ranges?(int)std::clamp<i64>(num(job->data,"Connections",8),1,32):1;
   retries=m.retries(str(job->data,"Queue"));
   for(const auto& segment:job->data["Segments"]){auto end=num(segment,"End"),begin=num(segment,"Start");
    owners.push_back(end>=begin&&num(segment,"Done")==end-begin+1?-2:-1);
   }
   job->workers.assign(connections,Worker{});for(int i=0;i<connections;++i)job->workers[i].number=i+1;
  }
  std::vector<std::chrono::steady_clock::time_point> assigned(connections);
  std::vector<i64> receivedAtAssignment(connections,0);
  auto claim=[&](int worker)->size_t{
   Lock lock(m.mutex);group->check();auto& segments=job->data["Segments"];
   const auto now=std::chrono::steady_clock::now();
   auto speed=[&](int owner)->double{
    if(assigned[owner]==std::chrono::steady_clock::time_point{})return 0;
    double seconds=std::chrono::duration<double>(now-assigned[owner]).count();
    i64 bytes=job->workers[owner].received-receivedAtAssignment[owner];
    return seconds>=0.4&&bytes>=65536?bytes/seconds:0;
   };
   auto assignedTo=[&](size_t index){owners[index]=worker;assigned[worker]=now;receivedAtAssignment[worker]=job->workers[worker].received;return index;};
   size_t pending=SIZE_MAX;
   for(size_t i=0;i<owners.size();++i)if(owners[i]==-1){pending=i;break;}
   if(!ranges||segments.size()>=4096)return pending==SIZE_MAX?SIZE_MAX:assignedTo(pending);
   size_t largest=SIZE_MAX;i64 remaining=0;double worstSeconds=0,helperSpeed=speed(worker);
   // A proven fast worker can help a straggler before taking another queued chunk.
   // Require sustained observations and a material delay to avoid reacting to startup jitter.
   for(size_t i=0;i<owners.size();++i)if(owners[i]>=0){const auto& s=segments[i];
    i64 left=num(s,"End")-num(s,"Start")+1-num(s,"Done");
    double ownerSpeed=speed(owners[i]);
    if(left>=131072&&ownerSpeed>0&&helperSpeed>=ownerSpeed*2){
     double seconds=left/ownerSpeed;
     if(seconds>=0.75&&seconds>worstSeconds){worstSeconds=seconds;remaining=left;largest=i;}
    }
   }
   if(largest==SIZE_MAX){
    if(pending!=SIZE_MAX)return assignedTo(pending);
    // Without a reliable speed difference, split only larger tails (256 KiB per half).
    for(size_t i=0;i<owners.size();++i)if(owners[i]>=0){const auto& s=segments[i];
     i64 left=num(s,"End")-num(s,"Start")+1-num(s,"Done");
     if(left>=524288&&left>remaining){remaining=left;largest=i;}
    }
   }
   if(largest==SIZE_MAX)return SIZE_MAX;
   auto oldEnd=num(segments[largest],"End");
   auto boundary=num(segments[largest],"Start")+num(segments[largest],"Done")+remaining/2+remaining%2;
   i64 partId=0;for(const auto& s:segments)partId=std::max(partId,num(s,"Index")+1);
   if(partId>100000)return SIZE_MAX;
   // Discard only an orphan at the newly allocated ID (e.g. an older state backup).
   auto newFile=partPath(parts,partId);if(fs::exists(newFile))fs::remove(newFile);
   auto previousSplits=num(job->data,"DynamicSplits");
   owners.push_back(worker);
   try{
    segments.push_back({{"Index",partId},{"Start",boundary},{"End",oldEnd},{"Done",0}});
    segments[largest]["End"]=boundary-1;job->data["DynamicSplits"]=previousSplits+1;
    // Persist the new ownership map before either worker can write under it.
    m.save();
   }catch(...){
    if(segments.size()==owners.size())segments.erase(segments.end()-1);
    owners.pop_back();segments[largest]["End"]=oldEnd;job->data["DynamicSplits"]=previousSplits;throw;
   }
   job->workers[owners[largest]].end=boundary-1;
   return assignedTo(owners.size()-1);
  };

  std::mutex errorsMutex;std::exception_ptr error;bool changed=false;
  auto recordError=[&](bool resourceChanged){std::lock_guard<std::mutex> lock(errorsMutex);
   if(resourceChanged||!error)error=std::current_exception();changed|=resourceChanged;group->stop=true;
  };
  std::vector<std::thread> workers;
  // A throttle response delays new requests from every worker in this download.
  std::atomic<ULONGLONG> retryNotBefore{0};
  auto waitForServer=[&]{for(;;){auto now=GetTickCount64(),until=retryNotBefore.load();if(now>=until)return;group->wait((int)std::min<ULONGLONG>(250,until-now));}};
  auto workerBody=[&](int worker){
   auto& activity=job->workers[worker];
   try{
    for(;;){
     auto index=claim(worker);if(index==SIZE_MAX)break;
     for(int attempt=0;;++attempt){
      group->check();waitForServer();
      try{
       Json segment,data;i64 have,need;fs::path file;
       {
        Lock lock(m.mutex);data=job->data;segment=data["Segments"][index];
        file=partPath(parts,num(segment,"Index"));have=ranges&&fs::exists(file)?(i64)fs::file_size(file):0;
        need=num(segment,"End")>=num(segment,"Start")?num(segment,"End")-num(segment,"Start")+1:-1;
        if(need>=0&&have>need)throw std::runtime_error("Partial file exceeds its assigned range.");
        auto& actual=job->data["Segments"][index];job->data["Received"]=num(job->data,"Received")+have-num(actual,"Done");actual["Done"]=have;
        activity.start=num(segment,"Start");activity.end=num(segment,"End");activity.position=activity.start+have;activity.state="Connecting";
       }
       if(need>=0&&have==need)break;
       auto validator=str(data,"ETag");if(validator.empty()&&!yes(prefs,"IgnoreLastModified"))validator=str(data,"Modified");
       auto ownedResponse=(!ranges&&worker==0&&initialResponse)?std::move(initialResponse):
        std::make_unique<Http>(url,headers,prefs,*group,ranges?std::optional<i64>(num(segment,"Start")+have):std::nullopt,
         ranges?std::optional<i64>(num(segment,"End")):std::nullopt,ranges?validator:"",nullptr,true,pool);
       auto& response=*ownedResponse;
       if(ranges){
        if(response.status==200||response.status==416)throw Changed("The server changed the file or stopped honoring byte ranges.");
        success(response);ContentRange cr(response.header(L"Content-Range"));
        if(response.status!=206||!cr.valid||cr.start!=num(segment,"Start")+have||cr.end!=num(segment,"End")||cr.total!=num(data,"Size"))
         throw Changed("The server returned a mismatched byte range.");
        auto remote=etag(response),modified=response.header(L"Last-Modified");
        if((!str(data,"ETag").empty()&&remote!=str(data,"ETag"))||
           (str(data,"ETag").empty()&&!yes(prefs,"IgnoreLastModified")&&modified!=str(data,"Modified")))throw Changed("The remote file changed during transfer.");
       }else{
        success(response);if(response.status==206)throw std::runtime_error("Unexpected partial response to a full download.");
        need=length(response.header(L"Content-Length"));Lock lock(m.mutex);job->data["Size"]=need;job->data["Segments"][index]["End"]=need-1;activity.end=need-1;
       }
       Handle output(CreateFileW(file.c_str(),GENERIC_WRITE,FILE_SHARE_READ,nullptr,have?OPEN_EXISTING:CREATE_ALWAYS,FILE_ATTRIBUTE_NORMAL,nullptr));
       if(!output)throw std::runtime_error("Cannot write the partial download.");
       if(have){LARGE_INTEGER distance{};distance.QuadPart=have;if(!SetFilePointerEx(output.h,distance,nullptr,FILE_BEGIN))throw std::runtime_error("Cannot seek the partial download.");}
       BYTE buffer[16384];i64 readBytes=0;
       for(;;){
        {
         Lock lock(m.mutex);const auto& actual=job->data["Segments"][index];
         if(ranges&&num(actual,"End")<num(segment,"End")&&num(actual,"Done")==num(actual,"End")-num(actual,"Start")+1)break;
        }
        auto n=response.read(buffer,sizeof(buffer),*group);if(!n)break;
        if(need>=0&&(i64)n>need-have-readBytes)throw std::runtime_error("Server sent more bytes than the declared range.");
        readBytes+=(i64)n;m.charge(n,*group,*rate,limitOwner);
        {
         // Serialize the write with splitting. In-flight bytes past a new boundary
         // are discarded, never appended to a part belonging to a different range.
         Lock lock(m.mutex);const auto& actual=job->data["Segments"][index];
         size_t keep=ranges?(size_t)std::min<i64>((i64)n,num(actual,"End")-num(actual,"Start")+1-num(actual,"Done")):n;
         DWORD written=0;
         if(keep&&(!WriteFile(output.h,buffer,(DWORD)keep,&written,nullptr)||written!=keep))throw std::runtime_error("Cannot write the partial download.");
         m.progress(job,keep,index,&activity);
         job->data["TransferredBytes"]=num(job->data,"TransferredBytes")+(n-keep);
        }
       }
       {
        Lock lock(m.mutex);const auto& actual=job->data["Segments"][index];
        i64 expected=ranges?num(actual,"End")-num(actual,"Start")+1:need;
        if(expected>=0&&num(actual,"Done")!=expected)throw std::runtime_error("Connection ended before the expected bytes arrived.");
       }
       if(!FlushFileBuffers(output.h))throw std::runtime_error("Cannot flush the partial download.");
       break;
      }catch(const Changed&){throw;}catch(const Cancelled&){throw;}catch(const HttpRejected& rejection){
       if(!rejection.retryable()||attempt>=retries)throw;
       auto delay=rejection.delay(attempt);auto before=retryNotBefore.load();auto until=GetTickCount64()+delay;
       while(before<until&&!retryNotBefore.compare_exchange_weak(before,until)){}
       {Lock lock(m.mutex);activity.state="Server busy; retrying in "+std::to_string((delay+999)/1000)+" s";}
      }catch(...){
       if(attempt>=retries)throw;
       {Lock lock(m.mutex);activity.state="Retrying";}
       group->wait(std::min(10000,500*(1<<std::min(attempt,4))));
      }
     }
     {Lock lock(m.mutex);owners[index]=-2;activity.state="Waiting";}
    }
   }catch(const Changed&){recordError(true);}catch(...){recordError(false);}
   {Lock lock(m.mutex);activity.state=group->cancelled()?"Stopped":"Finished";}
  };
  try{for(int w=0;w<connections;++w)workers.emplace_back(workerBody,w);}catch(...){
   group->stop=true;for(auto& worker:workers)worker.join();throw;
  }
  for(auto& worker:workers)worker.join();cancel->check();
  if(error){
   if(changed&&generation==0){Lock lock(m.mutex);clearParts(parts);job->data["Segments"]=Json::array();job->data["Received"]=0;job->data["ETag"]=nullptr;job->data["Modified"]=nullptr;continue;}
   std::rethrow_exception(error);
  }
  return;
 }
}

static void transferPost(Manager& m,JobPtr job,const std::shared_ptr<Cancel>& cancel,const Json& prefs,
 Headers headers,const std::string& url,const fs::path& parts,JobPtr limitOwner,const std::shared_ptr<Rate>& rate){
 Json request;std::string redirected;
 {Lock lock(m.mutex);request=readPostRequest(job->data);redirected=reveal(str(job->data,"ProtectedPostGetUrl"));
  if(!redirected.empty()){headers=readHeaders(Json{{"ProtectedHeaders",str(job->data,"ProtectedPostGetHeaders")}});}
  else if(yes(job->data,"PostAttempted"))throw std::runtime_error("This form was already submitted. Start this download explicitly to submit it again.");
  else {job->data["PostAttempted"]=true;job->data["RangeSupported"]=false;m.save();}
 }
 auto pool=std::make_shared<HttpSession>(prefs,browserSessionSaver(m,job,prefs.value("ActiveBrowserSession",Json::object())));
 if(!redirected.empty()){transferHttp(m,job,cancel,prefs,headers,redirected,parts,limitOwner,rate,pool);return;}
 const auto body=str(request,"body").empty()?Bytes{}:unb64(str(request,"body"));headers["Content-Type"]=str(request,"contentType");
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
void transfer(Manager& m,JobPtr job,const std::shared_ptr<Cancel>& cancel,JobPtr limitOwner,std::shared_ptr<Rate> sharedRate){if(!limitOwner)limitOwner=job;if(!sharedRate)sharedRate=std::make_shared<Rate>();auto started=std::chrono::steady_clock::now();auto networkEnd=started;bool networkFinished=false;double prior;Json prefs;Headers headers;fs::path folder,parts;std::string url;{Lock l(m.mutex);validateGrabberFolder(job->data);prior=real(job->data,"TransferSeconds");prefs=browserSessionPreferences(browserProxyPreferences(m.state["Settings"],job->data),job->data);headers=readHeaders(job->data);url=str(job->data,"Url");folder=fs::path(wide(str(job->data,"Folder")));parts=m.root/L"parts"/wide(job->id());if(!str(job->data,"PartsFolder").empty())parts=fs::path(wide(str(job->data,"PartsFolder")));else if(!fs::exists(parts)&&!str(prefs,"TemporaryFolder").empty()){parts=fs::path(wide(str(prefs,"TemporaryFolder")))/L"UDM-parts"/wide(job->id());job->data["PartsFolder"]=utf8(parts.wstring());m.save();}}fs::create_directories(folder);fs::create_directories(parts);try{
 if(Url(url).scheme=="ftp"){
  transferFtp(m,job,cancel,prefs,headers,url,parts,limitOwner,sharedRate);
 }else {bool post;{Lock lock(m.mutex);post=!str(job->data,"ProtectedRequest").empty();}if(post)transferPost(m,job,cancel,prefs,headers,url,parts,limitOwner,sharedRate);else transferHttp(m,job,cancel,prefs,headers,url,parts,limitOwner,sharedRate);}
 networkEnd=std::chrono::steady_clock::now();networkFinished=true;
 for(;;){cancel->check();bool pending;{Lock lock(m.mutex);pending=yes(job->data,"ConfirmationPending");}if(!pending)break;cancel->wait(40);}
 cancel->check();Json j;{Lock l(m.mutex);job->data["TransferSeconds"]=prior+std::chrono::duration<double>((networkFinished?networkEnd:std::chrono::steady_clock::now())-started).count();job->data["Status"]="Verifying";j=job->data;validateGrabberFolder(j);}auto staging=folder/(L".udm-"+wide(job->id())+L".assembling");{Handle output(CreateFileW(staging.c_str(),GENERIC_WRITE,0,nullptr,CREATE_ALWAYS,0,nullptr));if(!output)throw std::runtime_error("Cannot create the assembled file.");auto segments=j["Segments"];std::sort(segments.begin(),segments.end(),[](const Json& a,const Json& b){return num(a,"Start")<num(b,"Start");});for(auto s:segments)copyFileTo(partPath(parts,num(s,"Index")),output.h,*cancel);if(!FlushFileBuffers(output.h))throw std::runtime_error("Could not flush the assembled file.");}auto digest=fileHash(staging);if(!str(j,"ExpectedSha256").empty()&&lower(str(j,"ExpectedSha256"))!=digest){fs::remove(staging);throw std::runtime_error("SHA-256 mismatch. The file was not published.");}cancel->check();m.publishFile(job,staging,digest);markZone(job->target());if(yes(prefs,"UseServerDate")){SYSTEMTIME system{};FILETIME stamp{};auto modified=wide(str(j,"Modified"));if(!modified.empty()&&WinHttpTimeToSystemTime(modified.c_str(),&system)&&SystemTimeToFileTime(&system,&stamp)){Handle f(CreateFileW(job->target().c_str(),FILE_WRITE_ATTRIBUTES,FILE_SHARE_READ|FILE_SHARE_WRITE,nullptr,OPEN_EXISTING,0,nullptr));if(f)SetFileTime(f.h,&stamp,nullptr,&stamp);}}
 {Lock l(m.mutex);job->data["Sha256"]=digest;job->data["Size"]=fs::file_size(job->target());job->data["Received"]=job->data["Size"];job->data["Status"]="Complete";job->data["MediaPhase"]="Complete";job->data["MediaTracksReady"]=false;job->data["SabrRetainedBytes"]=0;job->data["Finished"]=date();job->data["Error"]="";m.save();}clearParts(parts);
 }catch(...){Lock l(m.mutex);job->data["TransferSeconds"]=prior+std::chrono::duration<double>((networkFinished?networkEnd:std::chrono::steady_clock::now())-started).count();job->data["ElapsedSeconds"]=real(job->data,"ElapsedSeconds")+std::chrono::duration<double>(std::chrono::steady_clock::now()-started).count();throw;}{Lock l(m.mutex);job->data["ElapsedSeconds"]=real(job->data,"ElapsedSeconds")+std::chrono::duration<double>(std::chrono::steady_clock::now()-started).count();}
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
  if(size<1||!std::regex_match(savedHash,std::regex("[a-fA-F0-9]{64}"))||!fs::is_regular_file(child->target())||fs::file_size(child->target())!=(uintmax_t)size||fileHash(child->target())!=savedHash)throw std::runtime_error("A saved media track is missing or changed. Saved data was kept; start a separate download.");
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

void mediaTransfer(Manager& m,JobPtr job,const std::shared_ptr<Cancel>& c){auto started=std::chrono::steady_clock::now();Json j;{Lock l(m.mutex);j=job->data;}validateSource(str(j,"SourceUrl"));const bool audioOnly=str(j,"MediaOutput")=="audio";if(audioOnly?(!job->audio||bool(job->video)):!job->video)throw std::runtime_error("Fresh browser-captured links are required. Choose the quality again in the browser.");auto media=mediaWorkingDirectory(m,job);{Lock lock(m.mutex);m.save();}auto tools=appDir()/L"tools";if(!fs::exists(tools/L"ffmpeg.exe"))throw std::runtime_error("Media helper missing. Run setup-media.ps1.");fs::create_directories(media);fs::create_directories(job->target().parent_path());for(auto child:{job->video,job->audio})if(child){Lock l(m.mutex);child->data["Folder"]=utf8(media.wstring());child->data["FileName"]=child==job->video?"video.mp4":"audio.mp4";if(str(child->data,"Status")=="Complete"&&!fs::exists(child->target())&&!yes(child->data,"CompletedMediaRefreshPendingValidation"))child->data["Status"]="Paused";if(str(j,"ProtectedSabr").empty()&&str(child->data,"Status")!="Complete"){auto q=query(str(child->data,"Url"));if(q.count("expire")&&std::stoll(q["expire"])*1000<epoch()+120000)throw std::runtime_error("Captured links expired. Use Refresh download address, then choose the same quality in the browser.");}}
 try{auto network=std::chrono::steady_clock::now();if(str(j,"ProtectedSabr").empty())verifyRefreshedCompleteTracks(m,job,c);if(!str(j,"ProtectedSabr").empty()){if(verifiedMediaTracks(job)){Lock lock(m.mutex);job->data["ReusedMediaTracks"]=true;job->data["MediaTracksReady"]=true;}else sabrTransfer(m,job,c);}else{auto group=std::make_shared<Cancel>();group->parent=c;auto rate=std::make_shared<Rate>();std::vector<std::future<void>> tasks;for(auto child:{job->video,job->audio})if(child)tasks.push_back(std::async(std::launch::async,[&,child]{try{if(str(child->data,"Status")!="Complete")transfer(m,child,group,job,rate);}catch(...){group->stop=true;throw;}}));std::exception_ptr error;for(auto& task:tasks)try{task.get();}catch(...){if(!error)error=std::current_exception();}c->check();if(error)std::rethrow_exception(error);} {Lock l(m.mutex);job->data["TransferSeconds"]=real(job->data,"TransferSeconds")+std::chrono::duration<double>(std::chrono::steady_clock::now()-network).count();job->data["Status"]="Merging";job->data["MediaPhase"]="Assembling file";m.save();}auto staging=job->target().parent_path()/(L".udm-"+wide(job->id())+(audioOnly?L".muxing.m4a":L".muxing.mp4"));auto merge=std::chrono::steady_clock::now();std::vector<std::wstring> args={L"-hide_banner",L"-loglevel",L"error",L"-nostdin",L"-y",L"-protocol_whitelist",L"file,pipe",L"-i",(audioOnly?job->audio:job->video)->target().wstring()};if(audioOnly)args.insert(args.end(),{L"-map",L"0:a:0",L"-vn"});else if(job->audio){args.insert(args.end(),{L"-protocol_whitelist",L"file,pipe",L"-i",job->audio->target().wstring(),L"-map",L"0:v:0",L"-map",L"1:a:0"});}else args.insert(args.end(),{L"-map",L"0:v:0",L"-map",L"0:a:0"});args.insert(args.end(),{L"-c",L"copy",L"-movflags",L"+faststart",staging.wstring()});try{execute(tools/L"ffmpeg.exe",args,300,*c);if(!fs::exists(staging)||!fs::file_size(staging))throw std::runtime_error("Media assembly produced no output.");if(audioOnly){auto probe=Json::parse(execute(tools/L"ffprobe.exe",{L"-v",L"error",L"-show_entries",L"stream=codec_type,codec_name",L"-of",L"json",staging.wstring()},20,*c));auto streams=probe.value("streams",Json::array());if(streams.size()!=1||str(streams[0],"codec_type")!="audio"||str(streams[0],"codec_name")!="aac")throw std::runtime_error("Audio assembly did not produce one AAC track. Output was not published.");}else if(yes(j,"ExactMediaQuality")&&num(j,"MediaPixelHeight")>0){auto actual=execute(tools/L"ffprobe.exe",{L"-v",L"error",L"-select_streams",L"v:0",L"-show_entries",L"stream=height",L"-of",L"default=noprint_wrappers=1:nokey=1",staging.wstring()},20,*c);if(std::stoll(trim(actual))!=num(j,"MediaPixelHeight"))throw std::runtime_error("Captured stream dimensions do not match the selected quality. Output was not published.");}{Lock lock(m.mutex);job->data["MediaPhase"]="Verifying output";}auto digest=fileHash(staging);if(!str(j,"ExpectedSha256").empty()&&lower(str(j,"ExpectedSha256"))!=digest)throw std::runtime_error("Media SHA-256 does not match the expected value. Saved streams are retained.");c->check();if(!MoveFileExW(staging.c_str(),job->target().c_str(),MOVEFILE_WRITE_THROUGH))throw std::runtime_error("Cannot publish media; destination may already exist.");markZone(job->target());{Lock l(m.mutex);job->data["MergeSeconds"]=real(job->data,"MergeSeconds")+std::chrono::duration<double>(std::chrono::steady_clock::now()-merge).count();job->data["Sha256"]=digest;job->data["Size"]=fs::file_size(job->target());job->data["Received"]=job->data["Size"];job->data["Status"]="Complete";job->data["MediaPhase"]="Complete";job->data["MediaTracksReady"]=false;job->data["SabrRetainedBytes"]=0;job->data["Finished"]=date();job->data["Error"]="";m.save();}for(auto child:{job->video,job->audio})if(child){std::error_code ec;fs::remove(child->target(),ec);}cleanStreamCache(media);}catch(...){std::error_code ec;fs::remove(staging,ec);throw;}}catch(...){Lock l(m.mutex);job->data["ElapsedSeconds"]=real(job->data,"ElapsedSeconds")+std::chrono::duration<double>(std::chrono::steady_clock::now()-started).count();throw;}{Lock l(m.mutex);job->data["ElapsedSeconds"]=real(job->data,"ElapsedSeconds")+std::chrono::duration<double>(std::chrono::steady_clock::now()-started).count();}
}
}
