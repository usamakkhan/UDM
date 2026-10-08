#include "PacProxy.hpp"
#include "SocksProxy.hpp"
#include <condition_variable>
namespace udm {
void validatePacSettings(const Json& prefs){
 if(prefs.contains("ProxyAutoConfigUrl")&&!prefs["ProxyAutoConfigUrl"].is_string())throw std::runtime_error("Enter a proxy script address.");
 const auto address=str(prefs,"ProxyAutoConfigUrl");if(address.empty()&&!usesPacScript(prefs))return;
 if(address.empty()&&yes(prefs,"ProxyAutoDetect"))return;
 if(address.empty()||address.size()>8192||address.find_first_of("\r\n\t ")!=std::string::npos||address.find('\0')!=std::string::npos)throw std::runtime_error("Enter an HTTP or HTTPS proxy script address.");
 Url url(address);if(url.scheme!="http"&&url.scheme!="https")throw std::runtime_error("Proxy scripts require an HTTP or HTTPS address.");
}
bool retryPacConnection(DWORD error){return error==ERROR_WINHTTP_CANNOT_CONNECT||error==ERROR_WINHTTP_CONNECTION_ERROR||error==ERROR_WINHTTP_NAME_NOT_RESOLVED||error==ERROR_WINHTTP_TIMEOUT;}
namespace {
struct PacState {
 std::mutex mutex;std::condition_variable changed;bool complete=false,closed=false;DWORD error=0;
 static void CALLBACK callback(HINTERNET,DWORD_PTR context,DWORD status,void* data,DWORD length){
  if(!context)return;auto* state=reinterpret_cast<PacState*>(context);std::lock_guard<std::mutex> lock(state->mutex);
  if(status==WINHTTP_CALLBACK_STATUS_HANDLE_CLOSING)state->closed=true;
  else if(status==WINHTTP_CALLBACK_STATUS_GETPROXYFORURL_COMPLETE)state->complete=true;
  else if(status==WINHTTP_CALLBACK_STATUS_REQUEST_ERROR){state->complete=true;state->error=data&&length>=sizeof(WINHTTP_ASYNC_RESULT)?static_cast<WINHTTP_ASYNC_RESULT*>(data)->dwError:ERROR_WINHTTP_INTERNAL_ERROR;}
  else return;state->changed.notify_all();
 }
};
class Lookup {
 HINTERNET session=nullptr,resolver=nullptr;std::unique_ptr<PacState> state=std::make_unique<PacState>();bool contextSet=false;
 void close()noexcept{
  if(resolver){auto h=resolver;resolver=nullptr;if(WinHttpCloseHandle(h)){if(contextSet){std::unique_lock<std::mutex> lock(state->mutex);state->changed.wait(lock,[&]{return state->closed;});}}else if(contextSet)state.release();}
  if(session){WinHttpCloseHandle(session);session=nullptr;}
 }
public:
 Lookup(){try{
  session=WinHttpOpen(L"UDM proxy configuration",WINHTTP_ACCESS_TYPE_NO_PROXY,WINHTTP_NO_PROXY_NAME,WINHTTP_NO_PROXY_BYPASS,WINHTTP_FLAG_ASYNC);if(!session)throw std::runtime_error("Cannot initialize proxy script lookup.");
  if(!WinHttpSetTimeouts(session,15000,15000,15000,15000))throw std::runtime_error("Cannot set proxy script timeout.");
  if(WinHttpSetStatusCallback(session,PacState::callback,WINHTTP_CALLBACK_FLAG_REQUEST_ERROR|WINHTTP_CALLBACK_FLAG_GETPROXYFORURL_COMPLETE|WINHTTP_CALLBACK_FLAG_HANDLES,0)==WINHTTP_INVALID_STATUS_CALLBACK)throw std::runtime_error("Cannot initialize proxy script callbacks.");
  auto error=WinHttpCreateProxyResolver(session,&resolver);if(error)throw std::runtime_error("Cannot create proxy script resolver (Windows error "+std::to_string(error)+").");
  DWORD_PTR context=reinterpret_cast<DWORD_PTR>(state.get());if(!WinHttpSetOption(resolver,WINHTTP_OPTION_CONTEXT_VALUE,&context,sizeof(context)))throw std::runtime_error("Cannot initialize proxy script context.");contextSet=true;
 }catch(...){close();throw;}}
 ~Lookup(){close();}
 std::vector<Json> run(const Json& prefs,const Url& url,const Cancel& cancel){
  cancel.check();auto script=wide(str(prefs,"ProxyAutoConfigUrl")),address=wide(url.full);WINHTTP_AUTOPROXY_OPTIONS options{};if(yes(prefs,"ProxyAutoDetect")&&script.empty()){options.dwFlags=WINHTTP_AUTOPROXY_AUTO_DETECT;options.dwAutoDetectFlags=WINHTTP_AUTO_DETECT_TYPE_DHCP|WINHTTP_AUTO_DETECT_TYPE_DNS_A;}else{options.dwFlags=WINHTTP_AUTOPROXY_CONFIG_URL;options.lpszAutoConfigUrl=script.c_str();}options.fAutoLogonIfChallenged=FALSE;
  auto error=WinHttpGetProxyForUrlEx(resolver,address.c_str(),&options,reinterpret_cast<DWORD_PTR>(state.get()));if(error!=ERROR_IO_PENDING)throw std::runtime_error("Proxy script lookup failed (Windows error "+std::to_string(error)+").");
  const auto deadline=GetTickCount64()+30000;{std::unique_lock<std::mutex> lock(state->mutex);while(!state->complete){if(cancel.cancelled()){lock.unlock();close();throw Cancelled();}if(GetTickCount64()>=deadline){lock.unlock();close();throw std::runtime_error("Proxy script lookup timed out.");}state->changed.wait_for(lock,std::chrono::milliseconds(25));}error=state->error;}
  cancel.check();if(error)throw std::runtime_error("Proxy script failed (Windows error "+std::to_string(error)+"). No direct fallback was used.");
  WINHTTP_PROXY_RESULT result{};error=WinHttpGetProxyResult(resolver,&result);if(error)throw std::runtime_error("Cannot read proxy script result (Windows error "+std::to_string(error)+").");
  struct Release{WINHTTP_PROXY_RESULT* result;~Release(){WinHttpFreeProxyResult(result);}} release{&result};
  if(!result.cEntries||result.cEntries>32)throw std::runtime_error("Proxy script returned no usable route or too many alternatives.");
  std::vector<Json> routes;for(DWORD i=0;i<result.cEntries;++i){const auto& entry=result.pEntries[i];Json route=prefs;route.erase("ProtocolProxies");route.erase("ProxyAutoConfigUrl");route["ProxyBypass"]="";
   if(!entry.fProxy||entry.fBypass){route["ProxyMode"]="Connect directly";route["Proxy"]="";route["ProxyUser"]="";route["ProxySecret"]="";}
   else{
    auto host=entry.pwszProxy?utf8(entry.pwszProxy):"";if(host.empty()||!entry.ProxyPort)throw std::runtime_error("Proxy script returned an invalid server.");if(host.find(':')!=std::string::npos&&host.front()!='[')host="["+host+"]";route["Proxy"]=host+":"+std::to_string(entry.ProxyPort);
    if(entry.ProxyScheme==INTERNET_SCHEME_HTTP||entry.ProxyScheme==INTERNET_SCHEME_HTTPS){route["ProxyMode"]="Use a proxy server";route["ResolvedPacProxyScheme"]=entry.ProxyScheme==INTERNET_SCHEME_HTTPS?"https":"http";validateConnectProxy(route);}
    else if(entry.ProxyScheme==INTERNET_SCHEME_SOCKS){route["ProxyMode"]="Use a SOCKS4 / 4a proxy";validateSocksSettings(route);}
    else throw std::runtime_error("Proxy script returned an unsupported proxy protocol.");
   }routes.push_back(std::move(route));
  }return routes;
 }
};
}
std::vector<Json> resolvePacRoutes(const Json& prefs,const Url& url,const Cancel& cancel){
 validatePacSettings(prefs);if(!usesPacScript(prefs))return {prefs};cancel.check();
 if(socksBypass(url,str(prefs,"ProxyBypass"))){auto direct=prefs;direct.erase("ProtocolProxies");direct.erase("ProxyAutoConfigUrl");direct["ProxyMode"]="Connect directly";direct["Proxy"]="";direct["ProxyUser"]="";direct["ProxySecret"]="";return {direct};}
 Lookup lookup;return lookup.run(prefs,url,cancel);
}
}
