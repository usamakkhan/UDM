#include "ZipPreview.hpp"
#include "BrowserProxy.hpp"
#include "BrowserSession.hpp"
#include "DownloadPreview.hpp"
#include "Ftp.hpp"
#include "SiteLogins.hpp"
#include <regex>
namespace udm {
bool canPreviewZip(const Json& data,const std::string& mime){
 if(data.contains("OfflineProject")||!str(data,"ProtectedRequest").empty()||yes(data,"RequiresRequestCapture")||yes(data,"RequiresMediaCapture")||yes(data,"RequiresBrowserSessionCapture")||!str(data,"ProtectedAdaptive").empty()||!str(data,"SourceUrl").empty()||!str(data,"DuplicateOf").empty())return false;
 try{Url url(str(data,"Url"));if(url.scheme!="http"&&url.scheme!="https"&&url.scheme!="ftp")return false;auto type=previewMimeType(mime.empty()?str(data,"ContentType"):mime);return lower(utf8(fs::path(wide(str(data,"FileName"))).extension().wstring()))==".zip"||lower(utf8(fs::path(wide(url.path)).extension().wstring()))==".zip"||type=="application/zip"||type=="application/x-zip-compressed";}catch(...){return false;}
}
namespace {
uint64_t number(const std::string& text){if(text.empty()||text.size()>19||text.find_first_not_of("0123456789")!=std::string::npos)throw std::runtime_error("Invalid ZIP response size.");try{auto value=std::stoull(text);if(value>INT64_MAX)throw std::runtime_error("too large");return value;}catch(...){throw std::runtime_error("ZIP response size is too large.");}}
void encoding(const Http& response){auto value=lower(trim(response.header(L"Content-Encoding")));if(!value.empty()&&value!="identity")throw std::runtime_error("ZIP preview requires an uncompressed HTTP response.");}
uint64_t rangeSize(const Http& response,uint64_t start,size_t count){
 if(response.status!=206)throw HttpRejected(response.status);std::smatch match;auto value=response.header(L"Content-Range");if(!std::regex_match(value,match,std::regex("bytes ([0-9]+)-([0-9]+)/([0-9]+)")))throw std::runtime_error("Invalid ZIP response byte range.");
 auto first=number(match[1]),last=number(match[2]),size=number(match[3]);if(first!=start||last<first||last-first+1!=count||last>=size)throw std::runtime_error("ZIP server returned a different byte range.");auto length=response.header(L"Content-Length");if(!length.empty()&&number(length)!=count)throw std::runtime_error("ZIP response length does not match its byte range.");encoding(response);return size;
}
struct HttpZip {
 Json prefs;Headers headers;std::string address,finalUrl,etag,modified;const Cancel& cancel;std::shared_ptr<HttpSession> pool;
 uint64_t length=0,received=0;Bytes complete;std::vector<std::pair<uint64_t,Bytes>> cache;
 HttpZip(const Json& data,Json settings,const Cancel& c,std::function<void(const Json&)> save):prefs(std::move(settings)),headers(readHeaders(data)),address(str(data,"Url")),cancel(c),pool(std::make_shared<HttpSession>(prefs,std::move(save))){
  for(auto it=headers.begin();it!=headers.end();){auto name=lower(it->first);if(name=="range"||name=="if-range"||name=="if-match"||name=="if-none-match"||name=="if-modified-since"||name=="if-unmodified-since"||name=="accept-encoding")it=headers.erase(it);else ++it;}
  Http response(address,headers,prefs,cancel,0,0,"",nullptr,true,pool);encoding(response);finalUrl=response.finalUrl;
  if(response.status==200){auto size=response.header(L"Content-Length");if(!size.empty()&&number(size)>ZipFallbackLimit)throw std::runtime_error("This server does not support byte ranges. Preview requires downloading more than 8 MB; preview the completed ZIP instead.");complete=response.all((size_t)ZipFallbackLimit,cancel);received=complete.size();length=complete.size();if(!size.empty()&&number(size)!=length)throw std::runtime_error("Truncated ZIP response.");return;}
  length=rangeSize(response,0,1);auto first=response.all(1,cancel);if(first.size()!=1)throw std::runtime_error("Truncated ZIP response.");received=1;cache.emplace_back(0,std::move(first));
  auto tag=response.header(L"ETag");if(tag.size()<=1024&&std::regex_match(tag,std::regex("\"[^\"\\x00-\\x20\\x7f]*\"")))etag=tag;
  auto date=response.header(L"Last-Modified");SYSTEMTIME stamp{};if(!date.empty()&&date.size()<128&&WinHttpTimeToSystemTime(wide(date).c_str(),&stamp))modified=date;
 }
 Bytes fetch(uint64_t start,size_t count){
  cancel.check();if(start>length||count>length-start||count>ZipPreviewBudget-received)throw std::runtime_error("ZIP preview exceeded its bounded read budget.");auto request=headers;
  if(!etag.empty())setHeader(request,"If-Match",etag);else if(!modified.empty())setHeader(request,"If-Unmodified-Since",modified);
  Http response(address,request,prefs,cancel,(i64)start,(i64)(start+count-1),"",nullptr,true,pool);
  if(response.status==200||response.status==412||response.status==416)throw Changed("The ZIP changed or the server stopped supporting byte ranges. Close Preview and try again.");
  if(rangeSize(response,start,count)!=length||response.finalUrl!=finalUrl||(!etag.empty()&&response.header(L"ETag")!=etag)||(!modified.empty()&&response.header(L"Last-Modified")!=modified))throw Changed("The ZIP changed while its directory was being read. Close Preview and try again.");
  auto bytes=response.all(count,cancel);received+=bytes.size();if(bytes.size()!=count)throw std::runtime_error("The ZIP directory response was truncated.");return bytes;
 }
 Bytes read(uint64_t start,size_t count){
  cancel.check();if(start>length||count>length-start)throw std::runtime_error("ZIP directory points outside this archive.");if(!count)return {};
  if(!complete.empty())return Bytes(complete.begin()+(size_t)start,complete.begin()+(size_t)start+count);
  for(const auto& part:cache)if(start>=part.first&&start-part.first<=part.second.size()&&count<=part.second.size()-(start-part.first))return Bytes(part.second.begin()+(size_t)(start-part.first),part.second.begin()+(size_t)(start-part.first)+count);
  auto value=fetch(start,count);cache.emplace_back(start,value);return value;
 }
 void verify(){if(!complete.empty())return;auto count=(size_t)std::min<uint64_t>(65557,length),start=length-count;auto saved=read(start,count);if(fetch(start,count)!=saved)throw Changed("The ZIP directory changed while Preview was open. Try again.");}
};
}
ZipListing previewLocalZip(const fs::path& path,const Cancel& cancel){
 cancel.check();Handle file(CreateFileW(path.c_str(),GENERIC_READ,FILE_SHARE_READ,nullptr,OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL,nullptr));
 if(!file)throw std::runtime_error("Cannot open the ZIP file. It may be unavailable or in use.");
 BY_HANDLE_FILE_INFORMATION initial{};LARGE_INTEGER length{};
 if(GetFileType(file.h)!=FILE_TYPE_DISK||!GetFileInformationByHandle(file.h,&initial)||(initial.dwFileAttributes&FILE_ATTRIBUTE_DIRECTORY)||!GetFileSizeEx(file.h,&length)||length.QuadPart<0)throw std::runtime_error("Cannot read a regular ZIP file.");
 ZipListing result;ZipSource source;source.length=static_cast<uint64_t>(length.QuadPart);source.validated=true;
 source.read=[&](uint64_t at,size_t count){
  cancel.check();if(at>source.length||count>source.length-at||count>ZipPreviewBudget-result.received)throw std::runtime_error("ZIP preview exceeded its bounded read budget.");
  Bytes bytes(count);LARGE_INTEGER position{};position.QuadPart=static_cast<LONGLONG>(at);if(!SetFilePointerEx(file.h,position,nullptr,FILE_BEGIN))throw std::runtime_error("Cannot seek ZIP directory.");
  for(size_t offset=0;offset<count;){cancel.check();DWORD received=0;const auto wanted=static_cast<DWORD>(std::min<size_t>(count-offset,65536));if(!ReadFile(file.h,bytes.data()+offset,wanted,&received,nullptr)||!received)throw std::runtime_error("Cannot read ZIP directory; the archive may have changed.");offset+=received;result.received+=received;}
  cancel.check();return bytes;
 };
 source.verify=[&]{
  cancel.check();BY_HANDLE_FILE_INFORMATION current{};
  if(!GetFileInformationByHandle(file.h,&current)||initial.dwVolumeSerialNumber!=current.dwVolumeSerialNumber||initial.nFileIndexHigh!=current.nFileIndexHigh||initial.nFileIndexLow!=current.nFileIndexLow||initial.nFileSizeHigh!=current.nFileSizeHigh||initial.nFileSizeLow!=current.nFileSizeLow||CompareFileTime(&initial.ftLastWriteTime,&current.ftLastWriteTime)!=0)throw std::runtime_error("The ZIP changed while its directory was being read. Close Preview and try again.");
 };
 result.entries=zipContents(source,cancel);result.size=source.length;result.validated=source.validated;return result;
}
namespace {
struct LocalZipReaderSlot {
 static std::atomic_uint active;bool held=false;
 LocalZipReaderSlot(){held=active.fetch_add(1)<4;if(!held)active.fetch_sub(1);}
 ~LocalZipReaderSlot(){if(held)active.fetch_sub(1);}
};
std::atomic_uint LocalZipReaderSlot::active{0};
struct RemoteZipReaderSlot {
 static std::atomic_uint active;bool held=false;
 RemoteZipReaderSlot(){held=active.fetch_add(1)<4;if(!held)active.fetch_sub(1);}
 ~RemoteZipReaderSlot(){if(held)active.fetch_sub(1);}
};
std::atomic_uint RemoteZipReaderSlot::active{0};
}
struct LocalZipPreviewTask::Shared {Cancel cancel;mutable std::mutex mutex;ZipPreviewState result;};
LocalZipPreviewTask::LocalZipPreviewTask(fs::path path):state(std::make_shared<Shared>()){
 auto slot=std::make_shared<LocalZipReaderSlot>();if(!slot->held){state->result.status="Error";state->result.message="Previous ZIP previews are still closing. Wait for them to finish and try again.";return;}
 auto shared=state;try{std::thread([shared,slot,path=std::move(path)]{
  ZipPreviewState outcome;
  try{outcome.listing=std::make_shared<ZipListing>(previewLocalZip(path,shared->cancel));outcome.status="Ready";}
  catch(const Cancelled&){outcome.status="Cancelled";}
  catch(const std::exception& e){outcome.status="Error";outcome.message=e.what();}
  catch(...){outcome.status="Error";outcome.message="Cannot read ZIP directory.";}
  if(shared->cancel.cancelled()){outcome.listing.reset();outcome.status="Cancelled";outcome.message.clear();}
  std::lock_guard<std::mutex> lock(shared->mutex);shared->result=std::move(outcome);
 }).detach();}catch(const std::exception& e){state->result.status="Error";state->result.message=e.what();}
}
LocalZipPreviewTask::~LocalZipPreviewTask(){stop();}
void LocalZipPreviewTask::stop()noexcept{state->cancel.stop=true;}
ZipPreviewState LocalZipPreviewTask::snapshot()const{std::lock_guard<std::mutex> lock(state->mutex);return state->result;}
ZipListing previewRemoteZip(const Json& data,const Json& preferences,const Cancel& cancel,std::function<void(const Json&)> save){
 if(!canPreviewZip(data))throw std::runtime_error("ZIP preview is available for ordinary HTTP, HTTPS and FTP ZIP downloads.");cancel.check();auto prefs=browserSessionPreferences(browserProxyPreferences(preferences,data),data);ZipListing result;ZipSource source;
 if(Url(str(data,"Url")).scheme=="ftp"){
  source=ftpZipSource(str(data,"Url"),siteRequestHeaders(str(data,"Url"),readHeaders(data),prefs),prefs,cancel,result.received);
  result.entries=zipContents(source,cancel);result.size=source.length;result.validated=source.validated;
 }else{
  auto remote=std::make_shared<HttpZip>(data,std::move(prefs),cancel,std::move(save));source.length=remote->length;source.validated=!remote->etag.empty()||!remote->complete.empty();source.read=[remote](uint64_t start,size_t count){return remote->read(start,count);};source.verify=[remote]{remote->verify();};
  result.entries=zipContents(source,cancel);result.size=source.length;result.received=remote->received;result.validated=source.validated;
 }return result;
}
struct ZipPreviewTask::Shared {Cancel cancel;mutable std::mutex mutex;ZipPreviewState result;};
ZipPreviewTask::ZipPreviewTask(Json data,Json prefs,std::function<void(const Json&)> save,int timeoutMs):state(std::make_shared<Shared>()){
 state->cancel.deadline=GetTickCount64()+static_cast<ULONGLONG>(std::clamp(timeoutMs,1,120000));
 auto slot=std::make_shared<RemoteZipReaderSlot>();if(!slot->held){state->result.status="Error";state->result.message="Previous remote ZIP previews are still closing. Wait for them to finish and try again.";return;}
 auto shared=state;try{std::thread([shared,slot,data=std::move(data),prefs=std::move(prefs),save=std::move(save)]{
  ZipPreviewState outcome;
  auto persist=[shared,save](const Json& session){shared->cancel.check();if(save)save(session);};
  try{outcome.listing=std::make_shared<ZipListing>(previewRemoteZip(data,prefs,shared->cancel,std::move(persist)));outcome.status="Ready";}
  catch(const Cancelled&){outcome.status=shared->cancel.stop?"Cancelled":"Error";outcome.message=shared->cancel.stop?"":"ZIP preview timed out. Close it and try again.";}
  catch(const HttpRejected& e){outcome.status="Error";outcome.message=e.status==401?"Login is required. Close Preview, enter it under More, then try Preview again.":"ZIP preview is unavailable (HTTP "+std::to_string(e.status)+"). Refresh the link or try downloading the file.";}
  catch(const std::exception& e){outcome.status="Error";outcome.message=e.what();}
  catch(...){outcome.status="Error";outcome.message="Cannot read the remote ZIP directory.";}
  if(shared->cancel.cancelled()){outcome.listing.reset();outcome.status=shared->cancel.stop?"Cancelled":"Error";outcome.message=shared->cancel.stop?"":"ZIP preview timed out. Close it and try again.";}
  std::lock_guard<std::mutex> lock(shared->mutex);shared->result=std::move(outcome);
 }).detach();}catch(const std::exception& e){state->result.status="Error";state->result.message=e.what();}
}
ZipPreviewTask::~ZipPreviewTask(){stop();}
void ZipPreviewTask::stop()noexcept{state->cancel.stop=true;}
ZipPreviewState ZipPreviewTask::snapshot()const{
 if(state->cancel.stop)return {"Cancelled",{},{}};
 std::lock_guard<std::mutex> lock(state->mutex);
 if(state->cancel.stop)return {"Cancelled",{},{}};
 if(state->result.status=="Checking"&&state->cancel.cancelled())return {"Error","ZIP preview timed out. Close it and try again.",{}};
 return state->result;
}
}
