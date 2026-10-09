#include "BrowserProxy.hpp"
#include "BrowserSession.hpp"
#include "DownloadPreview.hpp"
#include "Ftp.hpp"
#include <algorithm>
#include <regex>
namespace udm {
bool canPreviewDownload(const Json& data){
 if(data.contains("OfflineProject")||!str(data,"ProtectedRequest").empty()||yes(data,"RequiresRequestCapture")||
    !str(data,"SourceUrl").empty()||!str(data,"ProtectedAdaptive").empty()||yes(data,"RequiresMediaCapture")||!str(data,"DuplicateOf").empty())return false;
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
 if(response.status==401){DWORD supported=0,first=0,target=0;if(WinHttpQueryAuthSchemes(response.request,&supported,&first,&target)&&target==WINHTTP_AUTH_TARGET_SERVER&&(supported&(WINHTTP_AUTH_SCHEME_BASIC|WINHTTP_AUTH_SCHEME_DIGEST)))throw AuthenticationRequired(Url(response.finalUrl).origin,(supported&WINHTTP_AUTH_SCHEME_DIGEST)?"Digest":"Basic");}
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
 }
 cancel.check();metadata["Status"]="Ready";return metadata;
}
DownloadPreview::DownloadPreview(Json download,Json preferences,int timeoutMs,std::function<void(const Json&)> saveBrowserSession):result({{"Status","Checking"}}){
 if(!canPreviewDownload(download)){result={{"Status","Unavailable"}};return;}
 cancel->deadline=GetTickCount64()+(ULONGLONG)std::clamp(timeoutMs,1,60000);
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
}
