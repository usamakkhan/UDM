#include "BrowserProxy.hpp"
#include "BrowserSession.hpp"
#include "DownloadPreview.hpp"
#include "ContentDispositionName.hpp"
#include "PreviewFilename.hpp"
#include "MediaPreview.hpp"
#include "Ftp.hpp"
#include <algorithm>
#include <regex>
namespace udm {
bool canPreviewDownload(const Json& data){
 if(data.contains("OfflineProject")||!str(data,"ProtectedRequest").empty()||yes(data,"RequiresRequestCapture")||
    (!str(data,"SourceUrl").empty()&&!canPreviewMedia(data))||!str(data,"ProtectedAdaptive").empty()||yes(data,"RequiresMediaCapture")||!str(data,"DuplicateOf").empty())return false;
 auto status=str(data,"Status");if(!status.empty()&&status!="Paused"&&status!="Failed"&&status!="Awaiting confirmation")return false;
 // Existing segment sizes belong to their saved transfer generation, not this lookup.
 if(num(data,"Received")>0||!data.value("Segments",Json::array()).empty())return false;
 try{auto scheme=Url(str(data,"Url")).scheme;return scheme=="http"||scheme=="https"||scheme=="ftp";}catch(...){return false;}
}
std::string previewMimeType(const std::string& raw){
 if(raw.size()>1024)return {};auto value=lower(trim(raw.substr(0,raw.find(';'))));
 static const std::regex valid("[a-z0-9!#$&^_.+*-]{1,64}/[a-z0-9!#$&^_.+*-]{1,64}");
 return std::regex_match(value,valid)?value:"";
}
namespace {
i64 previewLength(const std::string& value){if(value.empty())return -1;if(value.size()>19||value.find_first_not_of("0123456789")!=std::string::npos)throw std::runtime_error("Server returned an invalid file size.");try{return std::stoll(value);}catch(...){throw std::runtime_error("Server returned an unsupported file size.");}}
void previewSuccess(const Http& response){
 if(response.status==401){if(auto schemes=challengeSchemes(response.headers(L"WWW-Authenticate")))throw AuthenticationRequired(Url(response.finalUrl).origin,authSchemeName(schemes));}
 if(response.status==401){DWORD supported=0,first=0,target=0;if(WinHttpQueryAuthSchemes(response.request,&supported,&first,&target)&&target==WINHTTP_AUTH_TARGET_SERVER&&(supported&(WINHTTP_AUTH_SCHEME_BASIC|WINHTTP_AUTH_SCHEME_DIGEST|WINHTTP_AUTH_SCHEME_NEGOTIATE|WINHTTP_AUTH_SCHEME_NTLM)))throw AuthenticationRequired(Url(response.finalUrl).origin,authSchemeName(supported));}
 if(response.status<200||response.status>=300)throw HttpRejected(response.status);
 auto encoding=lower(trim(response.header(L"Content-Encoding")));if(!encoding.empty()&&encoding!="identity")throw std::runtime_error("Server did not provide metadata for the uncompressed download.");
}
Json describe(const Http& response,bool ranged){
 auto type=previewMimeType(response.header(L"Content-Type"));i64 size=-1;
 if(ranged&&response.status==416&&response.header(L"Content-Range")=="bytes */0")return {{"Size",0},{"ContentType",type}};
 previewSuccess(response);
 if(response.status==206){if(!ranged)throw std::runtime_error("Unexpected partial response to a file-details request.");std::smatch match;auto range=response.header(L"Content-Range");
  if(!std::regex_match(range,match,std::regex("bytes 0-0/([0-9]+)")))throw std::runtime_error("Server returned an invalid file-details range.");
  size=previewLength(match[1]);auto count=previewLength(response.header(L"Content-Length"));if(size<1||(count>=0&&count!=1))throw std::runtime_error("Server returned inconsistent file-details headers.");
 }else if(response.status==200)size=previewLength(response.header(L"Content-Length"));
 else if(response.status==204)size=0;
 else throw std::runtime_error("Server did not return download metadata.");
 return {{"Size",size},{"ContentType",type}};
}
}
Json probeDownload(const Json& data,const Json& originalPrefs,const Cancel& cancel,std::function<void(const Json&)> saveBrowserSession){
 auto prefs=browserSessionPreferences(browserProxyPreferences(originalPrefs,data),data);
 cancel.check();if(!canPreviewDownload(data))return {{"Status","Unavailable"}};
 if(canPreviewMedia(data))return probeMediaSize(data,prefs,cancel);
 auto address=str(data,"Url");auto headers=readHeaders(data);Json metadata;
 if(Url(address).scheme=="ftp")metadata=previewFtp(address,headers,prefs,cancel);
 else {
  auto pool=std::make_shared<HttpSession>(prefs,std::move(saveBrowserSession));
  Http head(address,headers,prefs,cancel,{},{},"",nullptr,true,pool,true);
  // Metadata lookup must never spend a single-use GET. The actual transfer owns
  // its response; a disposable range probe here cannot hand that response over.
  if(head.status==403||head.status==405||head.status==501)
   return {{"Status","Error"},{"Message","The server does not allow a file-details lookup. Start the download to get its size."},{"HttpStatus",head.status}};
  metadata=describe(head,false);
  metadata["ResponseHost"]=Url(head.finalUrl).host;
  // A suggestion from the final response only. Preview must not rename the
  // catalog entry or overwrite a destination already selected by the user.
  const bool unambiguous=head.headers(L"Content-Type").size()<=1&&head.headers(L"Content-Disposition").size()<=1;auto name=previewResponseFilename(head.finalUrl,contentDispositionName(head.header(L"Content-Disposition"),""),unambiguous?str(metadata,"ContentType"):"",yes(prefs,"CorrectFileExtensions",true));
  if(!name.empty())metadata["SuggestedFileName"]=std::move(name);
 }
 cancel.check();metadata["Status"]="Ready";return metadata;
}
DownloadPreview::DownloadPreview(Json download,Json preferences,int timeoutMs,std::function<void(const Json&)> saveBrowserSession):result({{"Status","Checking"}}){
 if(!canPreviewDownload(download)){result={{"Status","Unavailable"}};return;}
 cancel->deadline=GetTickCount64()+(ULONGLONG)std::clamp(timeoutMs,1,canPreviewMedia(download)?240000:60000);
 worker=std::thread([this,download=std::move(download),preferences=std::move(preferences),saveBrowserSession=std::move(saveBrowserSession)]{
  Json outcome;
  try{outcome=probeDownload(download,preferences,*cancel,saveBrowserSession);}
  catch(const Cancelled&){outcome={{"Status",cancel->stop?"Cancelled":"Error"},{"Message",cancel->stop?"":"File details lookup timed out."}};}
  catch(const HttpRejected& e){auto auth=dynamic_cast<const AuthenticationRequired*>(&e);bool localLogin=auth&&auth->origin==Url(str(download,"Url")).origin;outcome={{"Status","Error"},{"Message",e.status==401?(localLogin?"Login is required. Enter it under More, then refresh details.":"The server needs browser authorization. Capture a fresh link from its download page."):"File details unavailable (HTTP "+std::to_string(e.status)+"). You can still start the download."},{"HttpStatus",e.status}};}
  catch(const std::exception&){outcome={{"Status","Error"},{"Message","File details unavailable. Check the connection or login, then refresh details."}};}
  std::lock_guard<std::mutex> lock(mutex);result=std::move(outcome);
 });
}
DownloadPreview::~DownloadPreview(){stop();}
void DownloadPreview::stop()noexcept{cancel->stop=true;if(worker.joinable())worker.join();}
Json DownloadPreview::snapshot()const{std::lock_guard<std::mutex> lock(mutex);return result;}
struct BatchDownloadPreview::State {
 std::shared_ptr<Cancel> cancel=std::make_shared<Cancel>();
 std::atomic_size_t next{0};std::vector<std::string> addresses,protectedRowHeaders;Json preferences;std::string protectedHeaders;
 std::mutex mutex;std::vector<std::pair<size_t,Json>> completed;
};
BatchDownloadPreview::BatchDownloadPreview(std::vector<std::string> addresses,Headers headers,Json preferences,std::vector<std::string> protectedRowHeaders):state(std::make_shared<State>()){
 if(!protectedRowHeaders.empty()&&protectedRowHeaders.size()!=addresses.size())throw std::runtime_error("Batch request details do not match the links.");state->protectedRowHeaders=std::move(protectedRowHeaders);
 state->addresses=std::move(addresses);state->preferences=std::move(preferences);
 state->protectedHeaders=headers.empty()?"":protect(legacyDictionary(Json(headers)).dump());
 try{for(size_t worker=0;worker<std::min<size_t>(4,state->addresses.size());++worker)std::thread([work=state]{
  while(!work->cancel->cancelled()){
   const auto index=work->next.fetch_add(1);if(index>=work->addresses.size())break;
   if(work->addresses[index].empty())continue;
   Cancel cancel;cancel.parent=work->cancel;cancel.deadline=GetTickCount64()+8000;Json outcome;
   try{outcome=probeDownload({{"Url",work->addresses[index]},{"ProtectedHeaders",work->protectedRowHeaders.empty()||work->protectedRowHeaders[index].empty()?work->protectedHeaders:work->protectedRowHeaders[index]}},work->preferences,cancel);}
   catch(const Cancelled&){if(work->cancel->cancelled())break;outcome={{"Status","Error"},{"Message","File details lookup timed out."}};}
   catch(const std::exception&){outcome={{"Status","Error"},{"Message","File details unavailable. Check the connection or authorization."}};}
   if(work->cancel->cancelled())break;
   std::lock_guard<std::mutex> lock(work->mutex);work->completed.emplace_back(index,std::move(outcome));
  }
 }).detach();}catch(...){state->cancel->stop=true;throw;}
}
BatchDownloadPreview::~BatchDownloadPreview(){state->cancel->stop=true;}
std::vector<std::pair<size_t,Json>> BatchDownloadPreview::take(){std::lock_guard<std::mutex> lock(state->mutex);std::vector<std::pair<size_t,Json>> result;result.swap(state->completed);return result;}
}
