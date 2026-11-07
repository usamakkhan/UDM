#include "BrowserProxy.hpp"
#include "BrowserSession.hpp"
#include "DownloadPreview.hpp"
#include "ContentDispositionName.hpp"
#include "PreviewFilename.hpp"
#include "MediaPreview.hpp"
#include "Ftp.hpp"
#include <algorithm>
#include <deque>
#include <regex>
namespace udm {
namespace {
bool canPreviewDownloadRequest(const Json& data,const Json* video,const Json* audio,bool draftLogin){
 if(data.contains("OfflineProject")||hasNonemptyString(data,"ProtectedRequest")||yes(data,"RequiresRequestCapture")||
    hasNonemptyString(data,"ProtectedAdaptive")||yes(data,"RequiresMediaCapture")||hasNonemptyString(data,"DuplicateOf"))return false;
 if(!draftLogin){
  const auto status=str(data,"Status");if(!status.empty()&&status!="Paused"&&status!="Failed"&&status!="Awaiting confirmation")return false;
  // Existing segment sizes belong to their saved transfer generation, not this lookup.
  const auto segments=data.find("Segments");
  if(num(data,"Received")>0||(segments!=data.end()&&!segments->empty()))return false;
 }
 if(hasNonemptyString(data,"SourceUrl")&&!canPreviewMedia(data,video,audio))return false;
 try{auto scheme=Url(str(data,"Url")).scheme;return scheme=="http"||scheme=="https"||scheme=="ftp";}catch(...){return false;}
}
}
bool canPreviewDownload(const Json& data){
 const auto video=data.find("Video"),audio=data.find("Audio");
 return canPreviewDownloadRequest(data,video==data.end()?nullptr:&*video,audio==data.end()?nullptr:&*audio,false);
}
bool canPreviewDownload(const Job& job,bool draftLogin){
 return canPreviewDownloadRequest(job.data,job.video?&job.video->data:nullptr,job.audio?&job.audio->data:nullptr,draftLogin);
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
Json probeFileDetails(const std::string& address,const Headers& headers,const Json& prefs,const Cancel& cancel,std::function<void(const Json&)> saveBrowserSession={}){
 Json metadata;
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
Json probeBatchRequest(const BatchPreviewRequest& request,const Json& preferences,const Cancel& cancel){
 cancel.check();
 auto prefs=yes(preferences,"UseBrowserProxy",true)&&!request.proxy.empty()
  ?browserProxyRoutePreferences(preferences,request.address,validateBrowserProxy(request.proxy,request.address))
  :preferences;
 prefs=withBrowserSession(std::move(prefs),request.session.empty()?Json::object():validateBrowserSession(request.session));
 auto headers=batchRequestHeaders(request.headers,request.protectedOverrides);
 validateHeaders(headers);
 return probeFileDetails(request.address,headers,prefs,cancel);
}
}
Json probeDownload(const Json& data,const Json& originalPrefs,const Cancel& cancel,std::function<void(const Json&)> saveBrowserSession){
 auto prefs=browserSessionPreferences(browserProxyPreferences(originalPrefs,data),data);
 cancel.check();if(!canPreviewDownload(data))return {{"Status","Unavailable"}};
 if(canPreviewMedia(data))return probeMediaSize(data,prefs,cancel);
 return probeFileDetails(str(data,"Url"),readHeaders(data),prefs,cancel,std::move(saveBrowserSession));
}
Headers batchRequestHeaders(Headers headers,const std::string& protectedOverrides){
 if(protectedOverrides.empty())return headers;
 const auto overrides=readHeaders(Json{{"ProtectedHeaders",protectedOverrides}});
 for(const auto& [name,value]:overrides){
  setHeader(headers,name,value);
  if(value.empty()&&lower(name)=="authorization")headers["Authorization"]="";
 }
 return headers;
}
namespace {
constexpr unsigned DetailWorkerLimit=4;
std::atomic_uint detailWorkers{0};

struct DetailWorkerSlot {
 const bool held=detailWorkers.fetch_add(1)<DetailWorkerLimit;
 DetailWorkerSlot(){if(!held)detailWorkers.fetch_sub(1);}
 ~DetailWorkerSlot(){if(held)detailWorkers.fetch_sub(1);}
};

Json stoppedDetails(const Cancel& cancel){
 if(cancel.stop)return {{"Status","Cancelled"},{"Message",""}};
 return {{"Status","Error"},{"Message","File details lookup timed out."}};
}
}

struct DownloadPreview::State {
 Cancel cancel;
 std::mutex mutex;
 Json result={{"Status","Checking"}};
};

DownloadPreview::DownloadPreview(Json download,Json preferences,int timeoutMs,Manager* sessionManager,JobPtr sessionJob)
 :state(std::make_shared<State>()){
 if(!canPreviewDownload(download)){
  state->result={{"Status","Unavailable"}};
  return;
 }
 state->cancel.deadline=GetTickCount64()+static_cast<ULONGLONG>(
  std::clamp(timeoutMs,1,canPreviewMedia(download)?240000:60000));
 auto slot=std::make_shared<DetailWorkerSlot>();
 if(!slot->held){
  state->result={{"Status","Error"},{"Message","Previous file details lookups are still running. Try Refresh details again shortly."}};
  return;
 }
 std::function<void(const Json&)> saveSession;
 if(sessionManager){
  saveSession=sessionManager->browserSessionPersistence(
   std::move(sessionJob),readBrowserSession(download),std::shared_ptr<const Cancel>(state,&state->cancel));
 }
 try{
  std::thread([work=state,slot,download=std::move(download),preferences=std::move(preferences),saveSession=std::move(saveSession)]()mutable{
   Json outcome;
   try{
    outcome=probeDownload(download,preferences,work->cancel,std::move(saveSession));
   }catch(const Cancelled&){
    outcome=stoppedDetails(work->cancel);
   }catch(const HttpRejected& e){
    auto auth=dynamic_cast<const AuthenticationRequired*>(&e);
    const bool localLogin=auth&&auth->origin==Url(str(download,"Url")).origin;
    auto message="File details unavailable (HTTP "+std::to_string(e.status)+"). You can still start the download.";
    if(e.status==401){
     message=localLogin?"Login is required. Enter it under More, then refresh details.":
      "The server needs browser authorization. Capture a fresh link from its download page.";
    }
    outcome={{"Status","Error"},{"Message",std::move(message)},{"HttpStatus",e.status}};
   }catch(...){
    outcome={{"Status","Error"},{"Message","File details unavailable. Check the connection or login, then refresh details."}};
   }
   std::lock_guard<std::mutex> lock(work->mutex);
   work->result=work->cancel.cancelled()?stoppedDetails(work->cancel):std::move(outcome);
  }).detach();
 }catch(const std::system_error&){
  state->result={{"Status","Error"},{"Message","Could not start a file details lookup. Try Refresh details again."}};
 }
}
DownloadPreview::~DownloadPreview(){stop();}
void DownloadPreview::stop()noexcept{state->cancel.stop=true;}
Json DownloadPreview::snapshot()const{
 std::lock_guard<std::mutex> lock(state->mutex);
 if(state->cancel.stop||(str(state->result,"Status")=="Checking"&&state->cancel.cancelled()))
  return stoppedDetails(state->cancel);
 return state->result;
}
struct BatchDownloadPreview::State {
 struct Work {size_t index;BatchPreviewRequest request;std::shared_ptr<Cancel> cancel;};
 std::shared_ptr<Cancel> cancel=std::make_shared<Cancel>();
 Json preferences;
 std::mutex mutex;
 std::deque<Work> pending;
 std::map<size_t,std::shared_ptr<Cancel>> current;
 std::vector<std::pair<size_t,Json>> completed;
 size_t workers=0;
 void enqueue(size_t index,BatchPreviewRequest request,bool urgent){
  auto token=std::make_shared<Cancel>();token->parent=cancel;
  current[index]=token;
  if(urgent)pending.push_front({index,std::move(request),std::move(token)});
  else pending.push_back({index,std::move(request),std::move(token)});
 }
 void run(){
  for(;;){
   std::unique_lock<std::mutex> lock(mutex);
   if(pending.empty()||cancel->cancelled()){--workers;return;}
   auto item=std::move(pending.front());pending.pop_front();
   lock.unlock();
   item.cancel->deadline=GetTickCount64()+8000;
   Json outcome;
   try{outcome=probeBatchRequest(item.request,preferences,*item.cancel);}
   catch(const Cancelled&){outcome={{"Status","Error"},{"Message","File details lookup timed out."}};}
   catch(const std::exception&){outcome={{"Status","Error"},{"Message","File details unavailable. Check the connection or authorization."}};}
   lock.lock();
   const auto latest=current.find(item.index);
   if(latest==current.end()||latest->second!=item.cancel||cancel->cancelled())continue;
   completed.emplace_back(item.index,std::move(outcome));
   current.erase(latest);
  }
 }
 // The caller holds the mutex until all started workers have been counted.
 void startWorkers(const std::shared_ptr<State>& work){
  const auto slots=std::min(size_t{4}-workers,pending.size());
  try{
   for(size_t i=0;i<slots;++i){
    std::thread([work]{work->run();}).detach();
    ++workers;
   }
  }catch(const std::system_error&){
   if(workers)return;
   for(const auto& item:pending)completed.emplace_back(item.index,Json{{"Status","Error"},{"Message","Could not start a file details lookup."}});
   pending.clear();current.clear();
  }
 }
};
BatchDownloadPreview::BatchDownloadPreview(std::vector<BatchPreviewRequest> requests,Json preferences):state(std::make_shared<State>()){
 state->preferences=std::move(preferences);
 for(size_t index=0;index<requests.size();++index)
  if(!requests[index].address.empty())state->enqueue(index,std::move(requests[index]),false);
 std::lock_guard<std::mutex> lock(state->mutex);
 state->startWorkers(state);
}
BatchDownloadPreview::~BatchDownloadPreview(){state->cancel->stop=true;}
void BatchDownloadPreview::replace(size_t index,BatchPreviewRequest request){
 std::lock_guard<std::mutex> lock(state->mutex);
 const auto previous=state->current.find(index);
 if(previous!=state->current.end()){
  previous->second->stop=true;
  state->current.erase(previous);
 }
 auto& pending=state->pending;
 pending.erase(std::remove_if(pending.begin(),pending.end(),[index](const State::Work& item){return item.index==index;}),pending.end());
 auto& completed=state->completed;
 completed.erase(std::remove_if(completed.begin(),completed.end(),[index](const auto& item){return item.first==index;}),completed.end());
 if(request.address.empty())return;
 state->enqueue(index,std::move(request),true);
 state->startWorkers(state);
}
std::vector<std::pair<size_t,Json>> BatchDownloadPreview::take(){std::lock_guard<std::mutex> lock(state->mutex);std::vector<std::pair<size_t,Json>> result;result.swap(state->completed);return result;}
}
