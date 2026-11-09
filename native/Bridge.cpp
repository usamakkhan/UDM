#include "Automation.hpp"
#include "MediaSize.hpp"
#include "MediaPublication.hpp"
#include "Version.hpp"
#include "RecoveryAvailability.hpp"
#include "BrowserProxy.hpp"
#include "Core.hpp"
#include "PostBodyStore.hpp"
#include "CliCompletion.hpp"
#include "AdaptiveCapture.hpp"
#include "GuiModels.hpp"
#include "DirectMediaRefresh.hpp"
#include "Streaming.hpp"
#include "StreamCache.hpp"
#include "YouTubePlayer.hpp"
#include "DashManifest.hpp"
#include "BrowserSettings.hpp"
#include "BrowserIdentity.hpp"
#include "OptionsModel.hpp"
#include "BrowserRequest.hpp"
#include "DownloadPreview.hpp"
#include "CaptureReceipts.hpp"
#include "CaptureTransactions.hpp"
#include "MediaAdmission.hpp"
#include "CapturePresentation.hpp"
#include "GrabberBrowserSession.hpp"
#include "GrabberRenderedStore.hpp"
#include <sddl.h>
#include <iostream>
#include <algorithm>
#include <regex>
namespace udm {
class BridgeUnavailable:public std::runtime_error{public:using std::runtime_error::runtime_error;};
class DesktopNotRunning:public BridgeUnavailable{public:using BridgeUnavailable::BridgeUnavailable;};
static Json browserImportProperties(const Json& message){
 Json result=Json::object();if(!message.contains("importProperties"))return result;
 const auto& properties=message["importProperties"];
 if(!properties.is_object()||properties.size()>3)throw std::runtime_error("Invalid browser import properties.");
 for(auto it=properties.begin();it!=properties.end();++it){
  if((it.key()!="folder"&&it.key()!="description"&&it.key()!="referer")||!it.value().is_string())throw std::runtime_error("Invalid browser import properties.");
  auto value=it.value().get<std::string>();if(value.size()>4096||value.find('\0')!=std::string::npos)throw std::runtime_error("Browser import properties are too long or invalid.");
  if(it.key()=="folder"&&!value.empty()){
   auto path=fs::path(wide(value));if(!path.is_absolute()||value.rfind("\\\\?\\",0)==0||value.rfind("\\\\.\\",0)==0)throw std::runtime_error("Choose a complete download folder.");
   for(const auto& part:path.relative_path()){auto name=utf8(part.wstring());if(!name.empty()&&(name=="."||name==".."||safeName(name)!=name))throw std::runtime_error("Choose a valid download folder without relative components.");}
  }
  if(it.key()=="referer"&&!value.empty()){Url page(value);if((page.scheme!="http"&&page.scheme!="https")||value.find_first_of("\r\n")!=std::string::npos)throw std::runtime_error("Enter an HTTP or HTTPS Referer, or leave it blank.");}
  result[it.key()]=value;
 }
 return result;
}
void validateYouTubeVideoCapture(const Json& message){
 const auto action=str(message,"action"),address=str(message,"url"),format=str(message,"formatId");
 const auto height=num(message,"height"),pixel=num(message,"pixelHeight");
 if(action!="media"&&action!="sabr")throw std::runtime_error("Invalid YouTube video offer.");
 validateSource(address);
 if(height<144||height>4320||pixel<0||pixel>4320||(!format.empty()&&!std::regex_match(format,std::regex("[0-9]{1,6}"))))
  throw std::runtime_error("Invalid captured stream dimensions or format.");
 if(action=="sabr"){
  if(!message.contains("sabr"))throw std::runtime_error("Browser streaming capture is missing.");
  validateSabr(message["sabr"],address);
  if(pixel<1)throw std::runtime_error("Captured stream dimensions are missing.");
 }else{
  const auto video=str(message,"videoUrl"),audio=str(message,"audioUrl");
  if(video.empty())throw std::runtime_error("UDM needs a captured playback link. Choose the quality in the browser panel.");
  validateStream(video);
  if(!audio.empty())validateStream(audio);
 }
 if(!fs::exists(appDir()/L"tools"/L"ffmpeg.exe"))throw std::runtime_error("Media helper missing. Run setup-media.ps1.");
}
std::string youTubeVideoCaptureName(std::string name){
 if(name.empty())name="YouTube video";
 std::replace(name.begin(),name.end(),'/','_');
 std::replace(name.begin(),name.end(),'\\','_');
 return std::regex_replace(name,std::regex("\\.(mp4|mkv|webm)$",std::regex::icase),"")+".mp4";
}
void applyYouTubeVideoCapture(Manager& manager,const JobPtr& job,const Json& message){
 auto& data=job->data;
 data["SourceUrl"]=str(message,"url");
 saveMediaSizeHint(data,message);
 data["MediaHeight"]=num(message,"height");
 data["MediaPixelHeight"]=num(message,"pixelHeight");
 const bool sabr=str(message,"action")=="sabr";
 data["ExactMediaQuality"]=sabr||yes(message,"exactQuality");
 data["MediaFormatId"]=str(message,"formatId");
 if(sabr){
  const auto& offer=message.at("sabr");
  data["ProtectedSabr"]=protect(offer.dump());
  data["MediaFormatId"]=str(offer.at("video"),"id");
  const auto url=str(offer,"url");
  setStreams(manager,job,url,url);
  data["SabrTrackAddressBound"]=true;
 }else setStreams(manager,job,str(message,"videoUrl"),str(message,"audioUrl"));
 data["FormatDescription"]="Browser capture - "+std::to_string(num(message,"height"))+"p";
}
static size_t queueBrowserBatchReview(Manager& manager,const Json& message){
 const auto& links=message.at("links");
 if(!links.is_array()||links.empty())throw std::runtime_error("Select at least one link.");
 const bool mediaBatch=std::any_of(links.begin(),links.end(),[](const Json& link){return link.contains("adaptive")||link.contains("media");});
 if(mediaBatch?(links.size()>80||links.dump().size()>BrowserRequestMessageLimit):(links.size()>500||links.dump().size()>512*1024))
  throw std::runtime_error(mediaBatch?"The video batch exceeds its review limit.":"Select up to 500 links and 512 KiB of request details.");
 std::vector<BrowserBatchRow> rows;
 std::set<std::string> seen;
 for(const auto& link:links){
  if(!link.is_object())throw std::runtime_error("Invalid browser batch link.");
  Url address(automationText(link,"url",16000)),page(automationText(link,"page",16000));
  if(address.scheme=="ftp"||page.scheme=="ftp")throw std::runtime_error("Browser batch review requires HTTP or HTTPS links.");
  const bool media=link.contains("adaptive")||link.contains("media");
  if(!media&&!seen.insert(address.full).second)throw std::runtime_error("The browser batch contains a repeated link.");
  auto headers=browserHeaders(link);
  auto proxy=validateBrowserProxy(link.value("browserProxy",Json::object()),address.full);
  auto session=browserDownloadSession(link.value("browserSession",Json::object()),address.full,headers);
  if(link.contains("imageOnPage")&&!link["imageOnPage"].is_boolean())throw std::runtime_error("Invalid page image flag.");
  BrowserBatchRow row{std::move(address.full),automationText(link,"linkText",4096),std::move(page.full),
    std::move(headers),std::move(proxy),std::move(session),yes(link,"imageOnPage")};
  const auto suggestedName=automationText(link,"suggestedName",1024);
  if(!suggestedName.empty())row.suggestedName=safeName(suggestedName);
  if(media){
   const auto& request=link.at(link.contains("adaptive")?"adaptive":"media");
   if(!request.is_object()||str(request,"url")!=row.address)
    throw std::runtime_error("Invalid browser streaming offer.");
   if(str(request,"action")=="adaptive"){
    if(row.address!=row.downloadPage)throw std::runtime_error("The streaming source cannot be edited.");
    requireAdaptiveTools();
    const auto capture=validateAdaptiveCapture(request);
    row.suggestedName=adaptiveCaptureName(row.suggestedName.empty()?"Video":row.suggestedName,capture.plan);
   }else{
    if(!str(request,"output").empty()||request.contains("streamRefreshToken"))throw std::runtime_error("Invalid batch video selection.");
    validateYouTubeVideoCapture(request);
    (void)browserHeaders(request);
    row.suggestedName=youTubeVideoCaptureName(row.suggestedName);
   }
   row.media=request;
  }
  if(link.contains("responsePreview")){
   const auto& preview=link.at("responsePreview");
   if(!preview.is_object()||!preview.contains("size")||!preview.at("size").is_number_integer())
    throw std::runtime_error("Invalid browser response details.");
   const auto length=num(preview,"size");
   row.observedType=previewMimeType(automationText(preview,"contentType",255));
   if(length<=0||length>9007199254740991LL||row.observedType.empty())
    throw std::runtime_error("Invalid browser response details.");
   row.observedSize=length;
   const auto name=automationText(preview,"serverName",1024);
   if(!name.empty())row.observedName=safeName(name);
  }
  rows.push_back(std::move(row));
 }
 const auto count=rows.size();
 Lock lock(manager.mutex);
 if(manager.pendingBrowserBatches.size()>=8)
  throw std::runtime_error("Finish the pending browser batch reviews before opening another.");
 manager.pendingBrowserBatches.push_back(std::move(rows));
 return count;
}
std::string pipeName(){HANDLE token=nullptr;if(!OpenProcessToken(GetCurrentProcess(),TOKEN_QUERY,&token))throw std::runtime_error("Cannot read Windows identity.");Handle t(token);DWORD n=0;GetTokenInformation(t.h,TokenUser,nullptr,0,&n);Bytes data(n);if(!GetTokenInformation(t.h,TokenUser,data.data(),n,&n))throw std::runtime_error("Cannot read Windows identity.");LPWSTR value=nullptr;if(!ConvertSidToStringSidW(((TOKEN_USER*)data.data())->User.Sid,&value))throw std::runtime_error("Cannot read Windows SID.");auto sid=utf8(value);LocalFree(value);std::replace(sid.begin(),sid.end(),'-','_');std::string name="udm-"+sid;wchar_t suffix[40];DWORD size=GetEnvironmentVariableW(L"UDM_INSTANCE_TAG",suffix,40);if(size>=40)throw std::runtime_error("Invalid instance tag.");if(size>0&&size<40){auto tag=utf8(std::wstring(suffix,size));if(!std::regex_match(tag,std::regex("[A-Za-z0-9_-]{1,32}")))throw std::runtime_error("Invalid instance tag.");name+="-"+tag;}return name;}
static void pipeIo(HANDLE pipe,void* buffer,DWORD count,bool write,DWORD timeout,HANDLE stop=nullptr){auto bytes=(BYTE*)buffer;while(count){OVERLAPPED ov{};Handle event(CreateEventW(nullptr,TRUE,FALSE,nullptr));ov.hEvent=event.h;DWORD n=0;BOOL ok=write?WriteFile(pipe,bytes,count,&n,&ov):ReadFile(pipe,bytes,count,&n,&ov);if(!ok){if(GetLastError()!=ERROR_IO_PENDING)throw std::runtime_error("Browser connection closed.");HANDLE waits[]={event.h,stop};DWORD state=WaitForMultipleObjects(stop?2:1,waits,FALSE,timeout);if(state!=WAIT_OBJECT_0){CancelIoEx(pipe,&ov);GetOverlappedResult(pipe,&ov,&n,TRUE);throw std::runtime_error("Browser connection timed out.");}if(!GetOverlappedResult(pipe,&ov,&n,FALSE))throw std::runtime_error("Browser connection closed.");}if(!n)throw std::runtime_error("Browser connection closed.");count-=n;bytes+=n;}}
static Json readMessage(HANDLE pipe,DWORD timeout,HANDLE stop=nullptr,DWORD limit=BrowserRequestMessageLimit){DWORD n=0;pipeIo(pipe,&n,4,false,timeout,stop);if(n<1||n>limit)throw std::runtime_error("Browser message exceeds the size limit.");std::string s(n,0);pipeIo(pipe,s.data(),n,false,timeout,stop);return Json::parse(s);}
static void writeMessage(HANDLE pipe,const Json& j,DWORD timeout,HANDLE stop=nullptr,DWORD limit=BrowserReplyMessageLimit){auto s=j.dump();DWORD n=(DWORD)s.size();if(s.size()>limit)throw std::runtime_error("Browser message exceeds the size limit.");pipeIo(pipe,&n,4,true,timeout,stop);pipeIo(pipe,s.data(),n,true,timeout,stop);}
static Json sendDesktop(const Json& request,int timeout,bool stopIfAbsent){
 const auto instance=wide(pipeName());
 const auto name=L"\\\\.\\pipe\\"+instance;
 const auto desktopMutex=L"Local\\"+instance;
 const auto deadline=GetTickCount64()+timeout;
 for(;;){
  Handle pipe(CreateFileW(name.c_str(),GENERIC_READ|GENERIC_WRITE,0,nullptr,OPEN_EXISTING,FILE_FLAG_OVERLAPPED,nullptr));
  if(pipe){
   writeMessage(pipe.h,request,10000,nullptr,BrowserRequestMessageLimit);
   return readMessage(pipe.h,10000,nullptr,BrowserReplyMessageLimit);
  }
  const auto error=GetLastError();
  if(error!=ERROR_FILE_NOT_FOUND&&error!=ERROR_PIPE_BUSY)
   throw BridgeUnavailable("Cannot open UDM browser connection (Windows error "+std::to_string(error)+").");
  if(stopIfAbsent&&error==ERROR_FILE_NOT_FOUND){
   Handle desktop(OpenMutexW(SYNCHRONIZE,FALSE,desktopMutex.c_str()));
   if(!desktop&&GetLastError()==ERROR_FILE_NOT_FOUND)throw DesktopNotRunning("UDM desktop is not running.");
  }
  if(GetTickCount64()>=deadline)throw BridgeUnavailable("UDM browser connection is not ready.");
  if(error==ERROR_PIPE_BUSY)WaitNamedPipeW(name.c_str(),100);
  Sleep(30);
 }
}
Json send(const Json& request,int timeout){return sendDesktop(request,timeout,false);}
static Json startDesktopAndSend(const Json& request,const fs::path& data={}){
 // The launcher lock is separate from the desktop's lifetime lock.
 const auto name=L"Local\\udm-launch-"+wide(pipeName());
 Handle launch(CreateMutexW(nullptr,FALSE,name.c_str()));
 if(!launch)throw BridgeUnavailable("Cannot coordinate UDM startup.");
 const auto wait=WaitForSingleObject(launch.h,12000);
 if(wait!=WAIT_OBJECT_0&&wait!=WAIT_ABANDONED)throw BridgeUnavailable("UDM startup is already in progress.");
 const auto release=[](void* mutex){ReleaseMutex(mutex);};
 std::unique_ptr<void,decltype(release)> launchGuard(launch.h,release);
 try{return sendDesktop(request,12000,true);}catch(const DesktopNotRunning&){}
 const auto exe=appDir()/L"UDM.exe";
 auto command=quote(exe.wstring())+L" --background";
 if(!data.empty())command+=L" --data-dir "+quote(data.wstring());
 STARTUPINFOW startup{sizeof(startup)};
 PROCESS_INFORMATION process{};
 if(!CreateProcessW(exe.c_str(),command.data(),nullptr,nullptr,FALSE,CREATE_NO_WINDOW,nullptr,appDir().c_str(),&startup,&process))
  throw std::runtime_error("Could not start UDM.");
 Handle thread(process.hThread),desktop(process.hProcess);
 return send(request,12000);
}
// Session refresh is explicitly armed by the desktop. Browser capture offers a
// candidate; user review applies it to the existing job without replacing data.
static void requireFreshSabrCapture(const Json& data,const Json& offer,const Json& old){
 if(yes(data,"SabrSessionRefreshRequired")&&real(offer,"capturedAt")<=real(old,"capturedAt"))
  throw std::runtime_error("Play the video again and select the same quality to capture a fresh session.");
}
JobPtr Manager::captureMediaRefresh(const Json& message){
 const auto action=str(message,"action");
 const bool marked=message.contains("streamRefreshToken");
 if(!marked&&action!="sabr"&&action!="adaptive"&&action!="media")return {};
 Lock lock(mutex);
 if(marked&&(action!="sabr"||streamRefreshToken.empty()||str(message,"streamRefreshToken")!=streamRefreshToken||
             refreshId.empty()||refreshUntil<epoch()))
  throw std::runtime_error("This streaming recovery page expired. Reopen it from UDM.");
 if(refreshId.empty()||refreshUntil<epoch())return {};
 JobPtr job;for(auto candidate:jobs)if(candidate->id()==refreshId)job=candidate;
 if(!canRefreshAddress(job)||(marked&&str(job->data,"ProtectedSabr").empty())){
  if(marked)throw std::runtime_error("The download is no longer waiting for a replacement session.");
  return {};
 }
 if(str(message,"action")=="media"){
  if(!directMediaJob(job)||directMediaPageId(str(message,"url"))!=directMediaPageId(str(job->data,"SourceUrl")))return {};
  validateDirectMediaRefresh(job,message);auto before=job->data;job->data["ProtectedRefreshOffer"]=protect(Json{{"kind","direct-media"},{"message",message},{"received",epoch()}}.dump());try{save();}catch(...){job->data=before;throw;}return job;
 }
 if(str(message,"action")=="adaptive"){
  if(str(job->data,"ProtectedAdaptive").empty()||str(message,"url")!=str(job->data,"Url"))return {};
  auto capture=validateAdaptiveCapture(message);validateAdaptiveRefresh(Json::parse(reveal(str(job->data,"ProtectedAdaptive"))),capture.plan);
  auto before=job->data;job->data["ProtectedRefreshOffer"]=protect(Json{{"kind","adaptive"},{"message",message},{"received",epoch()}}.dump());
  try{save();}catch(...){job->data=before;throw;}return job;
 }
 if(str(job->data,"ProtectedSabr").empty())return {};
 const auto old=Json::parse(reveal(str(job->data,"ProtectedSabr")));
 if(!message.contains("sabr")||str(message["sabr"],"videoId")!=str(old,"videoId")){
  if(marked)throw std::runtime_error("Open the original video to refresh this download.");
  return {};
 }
 const bool audioOnly=str(job->data,"MediaOutput")=="audio";
 if((str(message,"output")=="audio")!=audioOnly)throw std::runtime_error("Refresh the original audio/video selection, or cancel session recovery before starting another download.");
 validateSabr(message["sabr"],str(message,"url"),audioOnly);
 requireFreshSabrCapture(job->data,message["sabr"],old);
 if(streamSelection(message["sabr"],audioOnly)!=streamSelection(old,audioOnly)||num(message,"height")!=num(job->data,"MediaHeight")||num(message,"pixelHeight")!=num(job->data,"MediaPixelHeight"))throw std::runtime_error("The captured quality, language or stream revision changed. Saved data was left unchanged; choose the original selection or start a new download.");
 browserHeaders(message);
 auto before=job->data;job->data["ProtectedRefreshOffer"]=protect(Json{{"kind","sabr"},{"message",message},{"received",epoch()}}.dump());
 try{save();}catch(...){job->data=before;throw;}return job;
}
void Manager::applyMediaRefresh(JobPtr job){
 Lock lock(mutex);
 if(!canRefreshAddress(job)||refreshId!=job->id()||refreshUntil<epoch())throw std::runtime_error("Open media session recovery again before applying captured streams.");
 auto candidate=addressRefreshCandidate(job);
 // Applying a session and consuming its review prompt are one catalog commit.
 const auto previousReceipts=state.value("BrowserCaptures",Json());
 auto commitRefresh=[&]{
  if(state.contains("BrowserCaptures")&&state["BrowserCaptures"].is_object())for(auto& row:state["BrowserCaptures"]){
   const auto presentation=row.value("presentation",Json::object());
   if(str(presentation,"id")==job->id()&&str(presentation,"kind")=="refresh")row.erase("presentation");
  }
  try{save();}catch(...){restoreCaptureRows(state,previousReceipts);throw;}
 };
 if(str(candidate,"kind")=="direct-media"){
  const auto& message=candidate.at("message");auto headers=validateDirectMediaRefresh(job,message);auto before=job->data,videoBefore=job->video->data,audioBefore=job->audio?job->audio->data:Json();
  try{saveMediaSizeHint(job->data,message);job->data["ProtectedHeaders"]=headers.empty()?"":protect(legacyDictionary(Json(headers)).dump());
   for(auto child:{job->video,job->audio})if(child){child->data["Url"]=str(message,child==job->video?"videoUrl":"audioUrl");child->data["ProtectedHeaders"]=job->data["ProtectedHeaders"];child->data["RefreshPendingValidation"]=true;child->data["CompletedMediaRefreshPendingValidation"]=str(child->data,"Status")=="Complete";child->data.erase("ProtectedResolvedUrl");child->data.erase("AuthenticationOrigin");child->data.erase("AuthenticationScheme");child->data["AuthenticationPromptPending"]=false;}
   job->data["Status"]="Paused";job->data["Error"]="";job->data["LastHttpStatus"]=0;job->data["AddressRefreshed"]=date();job->data.erase("ProtectedRefreshOffer");commitRefresh();
  }catch(...){job->data=before;job->video->data=videoBefore;if(job->audio)job->audio->data=audioBefore;throw;}cancelAddressRefresh(job);return;
 }
 if(str(candidate,"kind")=="adaptive"&&!str(job->data,"ProtectedAdaptive").empty()){
  const auto& message=candidate.at("message");if(str(message,"url")!=str(job->data,"Url"))throw std::runtime_error("The replacement video page changed.");
  auto capture=validateAdaptiveCapture(message);validateAdaptiveRefresh(Json::parse(reveal(str(job->data,"ProtectedAdaptive"))),capture.plan);
  auto before=job->data;
  try{job->data["ProtectedAdaptive"]=protect(capture.plan.dump());job->data["ProtectedMediaCookies"]=protect(capture.cookies.dump());job->data["ProtectedMediaHeaders"]=protect(capture.originHeaders.dump());job->data["ProtectedHeaders"]=capture.headers.empty()?"":protect(legacyDictionary(Json(capture.headers)).dump());
   saveAdaptiveDisplayAddress(job->data,capture.plan);
   if(str(capture.plan,"type")=="dash"){
    job->data.erase("MediaLengthMs");
    if(num(capture.plan,"durationMs")>0)job->data["MediaLengthMs"]=num(capture.plan,"durationMs");
   }
   job->data["AdaptiveRefreshPendingValidation"]=true;job->data["Status"]="Paused";job->data["Error"]="";job->data["LastHttpStatus"]=0;job->data["AddressRefreshed"]=date();job->data.erase("ProtectedRefreshOffer");commitRefresh();
  }catch(...){job->data=before;throw;}cancelAddressRefresh(job);return;
 }
 if(str(candidate,"kind")!="sabr"||str(job->data,"ProtectedSabr").empty())throw std::runtime_error("Play the original video and send the same quality or audio track to UDM first.");
 auto message=candidate.at("message"),offer=message.at("sabr"),old=Json::parse(reveal(str(job->data,"ProtectedSabr")));const bool audioOnly=str(job->data,"MediaOutput")=="audio";
 validateSabr(offer,str(message,"url"),audioOnly);
 requireFreshSabrCapture(job->data,offer,old);
 if(streamSelection(old,audioOnly)!=streamSelection(offer,audioOnly))throw std::runtime_error("The replacement stream identity no longer matches.");
 const auto receipt=job->data.find("RetainedMediaOutput");
 // A matching refresh changes transport credentials, while the saved tracks
 // still identify the same content. Only carry forward a current receipt.
 const bool keepAssembly=receipt!=job->data.end()&&num(*receipt,"Version")==1&&
  str(*receipt,"Binding")==retainedYouTubeOutputBinding(*job);
 auto before=job->data;Json videoBefore=job->video?job->video->data:Json(),audioBefore=job->audio?job->audio->data:Json();auto headers=browserHeaders(message);
 try{saveMediaSizeHint(job->data,message);job->data["ProtectedSabr"]=protect(offer.dump());job->data["ProtectedHeaders"]=headers.empty()?"":protect(legacyDictionary(Json(headers)).dump());
  for(auto child:{job->video,job->audio})if(child){child->data["Url"]=str(offer,"url");child->data["ProtectedHeaders"]=job->data["ProtectedHeaders"];}
  job->data["SabrTrackAddressBound"]=true;
  if(keepAssembly)job->data["RetainedMediaOutput"]["Binding"]=retainedYouTubeOutputBinding(*job);
  const auto challengedWindow=num(job->data.value("SabrSessionRejection",Json()),"window");
  if(yes(job->data,"SabrSessionRefreshRequired")&&challengedWindow>1&&challengedWindow<=num(job->data,"SabrConnections"))
   job->data["SabrSerialRecovery"]=true;
  job->data.erase("SabrSessionRefreshRequired");
  job->data.erase("SabrSessionRejection");
  job->data["Status"]="Paused";job->data["Error"]="";job->data["LastHttpStatus"]=0;job->data["AddressRefreshed"]=date();job->data.erase("ProtectedRefreshOffer");commitRefresh();
 }catch(...){job->data=before;if(job->video)job->video->data=videoBefore;if(job->audio)job->audio->data=audioBefore;throw;}
 cancelAddressRefresh(job);
}
// Audio streaming uses the captured session but requests no video format.
static JobPtr receiveStreamingAudio(Manager& m,const Json& message){
 const auto source=str(message,"url");
 if(num(message,"height")!=0||num(message,"pixelHeight")!=0||!str(message,"videoUrl").empty()||!message.contains("sabr"))throw std::runtime_error("Invalid audio-only streaming selection.");
 const auto& offer=message["sabr"];validateSabr(offer,source,true);
 const auto selected=formatIdentity(offer["audio"]);
 if(str(message,"formatId")!=str(offer["audio"],"id"))throw std::runtime_error("The selected audio identity changed.");
 if(!fs::exists(appDir()/L"tools"/L"ffmpeg.exe")||!fs::exists(appDir()/L"tools"/L"ffprobe.exe"))throw std::runtime_error("Media helpers missing. Run setup-media.ps1.");
 auto headers=browserHeaders(message);Lock lock(m.mutex);
 for(auto job:m.jobs){const auto status=str(job->data,"Status");
  if(str(job->data,"SourceUrl")!=source||str(job->data,"MediaOutput")!="audio"||!(status=="Queued"||status=="Awaiting confirmation"||m.isActive(job)))continue;
  try{auto previous=Json::parse(reveal(str(job->data,"ProtectedSabr")));if(formatIdentity(previous.at("audio"))==selected)return job;}catch(const std::exception&){}
 }
 auto name=str(message,"filename","YouTube audio");std::replace(name.begin(),name.end(),'/','_');std::replace(name.begin(),name.end(),'\\','_');
 name=std::regex_replace(name,std::regex("\\.(m4a|mp4|mkv|webm)$",std::regex::icase),"")+".m4a";
 auto job=m.add(source,"",name,"Main queue",true,headers);
 try{job->data["SourceUrl"]=source;saveMediaSizeHint(job->data,message);job->data["MediaOutput"]="audio";job->data["MediaHeight"]=0;job->data["MediaPixelHeight"]=0;
  job->data["ExactMediaQuality"]=true;job->data["MediaFormatId"]=str(offer["audio"],"id");job->data["ProtectedSabr"]=protect(offer.dump());
  setStreams(m,job,"",str(offer,"url"));job->data["FormatDescription"]="Browser capture - AAC audio (M4A)";
  job->data["SabrTrackAddressBound"]=true;
  job->data["Status"]=browserOfferStatus(m.state["Settings"]);m.save();return job;
 }catch(...){m.jobs.erase(std::remove(m.jobs.begin(),m.jobs.end(),job),m.jobs.end());m.save();throw;}
}
JobPtr Manager::receive(const Json& message){
 if(message.contains("mediaToken"))return receiveBrowserMedia(message);
 if(auto replacement=captureMediaRefresh(message))return replacement;
 if(str(message,"action")=="sabr"&&str(message,"output")=="audio")return receiveStreamingAudio(*this,message);
 if(message.contains("captureToken")){if(str(message,"action")!="add")throw std::runtime_error("Capture tokens apply only to ordinary downloads.");return receiveCapture(*this,message);}

 if(str(message,"action")=="automation-add")return receiveAutomationLink(*this,message.at("link"));
 if(str(message,"action")=="cli-add"){
  Lock lock(mutex);
  auto previous=jobs;
  // Automatic overwrite can replace an existing record and retire its parts.
  // Its restart journal still owns that transition.
  const bool canOverwrite=str(state["Settings"],"DuplicatePolicy")=="Replace";
  const bool stage=!catalogTransaction&&!canOverwrite;
  std::map<std::string,std::string> attempts;
  if(canOverwrite)for(const auto& old:previous)attempts.emplace(old->id(),str(old->data,"RestartAttempt"));
  const auto priorOfferCount=pendingOffers.size();
  auto priorExisting=stage?existingOffers:decltype(existingOffers){};
  if(stage)catalogTransaction=true;
  JobPtr job;
  try{
   job=offerDownload(str(message,"url"),str(message,"folder"),str(message,"filename"),"Main queue",true);
   const bool existing=std::find(previous.begin(),previous.end(),job)!=previous.end()&&
    (!canOverwrite||attempts.at(job->id())==str(job->data,"RestartAttempt"));
   if(str(job->data,"DuplicateOf").empty()){
    if(existing)pendingOffers.push_back(job); // Never reset an active or completed record.
    else job->data["Status"]=yes(message,"paused")?"Paused":yes(message,"silent")?"Queued":"Awaiting confirmation";
   }
   if(stage)catalogTransaction=false;
   save();
  }catch(...){
   if(stage){
    catalogTransaction=false;
    jobs.swap(previous);
    pendingOffers.resize(priorOfferCount);
    existingOffers.swap(priorExisting);
   }
   throw;
  }
  if(yes(message,"hangup"))armCliHangup(*this,job);
  return job;
 }
if(str(message,"action")=="adaptive")return receiveAdaptive(*this,message);auto action=str(message,"action"),address=str(message,"url");if(action!="add"&&action!="media"&&action!="sabr"&&action!="capture-reconcile"&&action!="capture-prepare"&&action!="capture-status"&&action!="capture-release"&&action!="capture-commit"&&action!="capture-review-open")throw std::runtime_error("Unknown UDM command.");auto h=browserHeaders(message);if(action=="add"){const auto importProperties=browserImportProperties(message);if(importProperties.contains("referer")){for(auto it=h.begin();it!=h.end();)if(lower(it->first)=="referer")it=h.erase(it);else ++it;auto referer=str(importProperties,"referer");if(!referer.empty())h["Referer"]=referer;validateHeaders(h);}auto session=browserDownloadSession(message.value("browserSession",Json::object()),address,h);auto proxy=validateBrowserProxy(message.value("browserProxy",Json::object()),address);Lock routeLock(mutex);auto request=validatePostRequest(message.value("request",Json::object()),address);const auto sourcePage=browserSourcePage(message);auto replacement=request.empty()?captureAddressRefresh(address,h,str(message,"filename"),sourcePage.empty()?str(message,"referrer"):sourcePage,str(message,"reportedServerName"),!sourcePage.empty()):JobPtr{};if(replacement){auto before=replacement->data;try{auto offer=addressRefreshCandidate(replacement);if(message.contains("browserProxy"))offer["browserProxy"]=proxy;offer["browserSession"]=session;replacement->data["ProtectedRefreshOffer"]=protect(offer.dump());save();return replacement;}catch(...){replacement->data=before;throw;}}
auto prior=findDuplicate(address,h,{},request);auto previousAttempt=prior?str(prior->data,"RestartAttempt"):"";if(prior&&(readBrowserProxy(prior->data)!=proxy||readBrowserSession(prior->data)!=session)&&(isActive(prior)||str(prior->data,"Status")=="Queued"||str(prior->data,"Status")=="Awaiting confirmation"||!str(prior->data,"DuplicateOf").empty()||str(state["Settings"],"DuplicatePolicy")=="Existing"))throw std::runtime_error("The existing download uses another browser proxy route or website session. Pause it and capture a numbered copy.");
// The route lock covers the duplicate lookup, route check, and admission.
auto job=offerDownload(address,str(importProperties,"folder"),str(message,"filename"),"Main queue",true,h,request,proxy,session,&prior);
const bool existing=job==prior&&str(job->data,"RestartAttempt")==previousAttempt;
if(!existing&&request.empty()){
 const auto reported=message.find("reportedDirectSize");
 if(reported!=message.end()){
  const auto size=positiveMediaSize(*reported);
  if(size>0)job->data["BrowserReportedDirectSize"]=size;
 }
 const auto serverName=str(message,"reportedServerName");
 if(!serverName.empty()&&serverName.size()<=1024&&serverName.find('\0')==std::string::npos)
  job->data["BrowserReportedFileName"]=safeName(serverName);
}
if(!existing&&importProperties.contains("description"))job->data["Description"]=importProperties["description"];if(!existing){const auto downloadPage=sourcePage.empty()?str(message,"referrer"):sourcePage;if(!downloadPage.empty()){Url page(downloadPage);if(page.scheme=="http"||page.scheme=="https")job->data["DownloadPage"]=page.full;}}if(str(job->data,"DuplicateOf").empty()){if(existing)pendingOffers.push_back(job);else job->data["Status"]=browserOfferStatus(state["Settings"],!existing&&yes(message,"downloadLater"));}save();return job;}
 validateYouTubeVideoCapture(message);
 const auto height=num(message,"height");
 const auto format=str(message,"formatId");
 Lock lock(mutex);
 for(const auto& existing:jobs){
  const auto status=str(existing->data,"Status");
  if(str(existing->data,"Url")==address&&(status=="Queued"||status=="Awaiting confirmation"||isActive(existing))&&
     !str(existing->data,"SourceUrl").empty()&&num(existing->data,"MediaHeight")==height&&
     yes(existing->data,"ExactMediaQuality")==yes(message,"exactQuality")&&str(existing->data,"MediaFormatId")==format)
   return existing;
 }
 auto job=add(address,"",youTubeVideoCaptureName(str(message,"filename")),"Main queue",true,h);
 try{
  applyYouTubeVideoCapture(*this,job,message);
  job->data["Status"]=browserOfferStatus(state["Settings"]);
  save();
  return job;
 }catch(...){jobs.erase(std::remove(jobs.begin(),jobs.end(),job),jobs.end());save();throw;}
}
PipeServer::PipeServer(Manager& m,std::function<void()> s,std::function<void()> p,std::function<void()> queue):manager(m),show(std::move(s)),present(std::move(p)),dispatchQueue(std::move(queue)){stopEvent=CreateEventW(nullptr,TRUE,FALSE,nullptr);if(!stopEvent)throw std::runtime_error("Cannot initialize browser integration.");thread=std::thread([this]{listen();});}
PipeServer::~PipeServer(){stopping=true;SetEvent(stopEvent);if(thread.joinable())thread.join();CloseHandle(stopEvent);}
void PipeServer::listen(){try{HANDLE token=nullptr;if(!OpenProcessToken(GetCurrentProcess(),TOKEN_QUERY,&token))throw std::runtime_error("Cannot read Windows identity.");Handle t(token);DWORD n=0;GetTokenInformation(t.h,TokenUser,nullptr,0,&n);Bytes data(n);if(!GetTokenInformation(t.h,TokenUser,data.data(),n,&n))throw std::runtime_error("Cannot read Windows identity.");LPWSTR sid=nullptr;ConvertSidToStringSidW(((TOKEN_USER*)data.data())->User.Sid,&sid);if(!sid)throw std::runtime_error("Cannot read Windows identity.");std::wstring acl=L"D:P(A;;GA;;;"+std::wstring(sid)+L")";LocalFree(sid);PSECURITY_DESCRIPTOR descriptor=nullptr;if(!ConvertStringSecurityDescriptorToSecurityDescriptorW(acl.c_str(),SDDL_REVISION_1,&descriptor,nullptr))throw std::runtime_error("Cannot secure browser connection.");std::unique_ptr<void,decltype(&LocalFree)> security(descriptor,LocalFree);SECURITY_ATTRIBUTES sa{sizeof(sa),descriptor,FALSE};auto name=L"\\\\.\\pipe\\"+wide(pipeName());while(!stopping){Handle pipe(CreateNamedPipeW(name.c_str(),PIPE_ACCESS_DUPLEX|FILE_FLAG_OVERLAPPED|FILE_FLAG_FIRST_PIPE_INSTANCE,PIPE_TYPE_BYTE|PIPE_READMODE_BYTE|PIPE_WAIT|PIPE_REJECT_REMOTE_CLIENTS,1,4096,4096,0,&sa));if(!pipe)throw std::runtime_error("Cannot create UDM browser connection. Another UDM instance may be running.");Handle ready(CreateEventW(nullptr,TRUE,FALSE,nullptr));OVERLAPPED ov{};ov.hEvent=ready.h;BOOL connected=ConnectNamedPipe(pipe.h,&ov);DWORD error=connected?ERROR_SUCCESS:GetLastError();if(!connected&&error==ERROR_IO_PENDING){HANDLE waits[]={ready.h,stopEvent};if(WaitForMultipleObjects(2,waits,FALSE,INFINITE)!=WAIT_OBJECT_0){CancelIoEx(pipe.h,&ov);DWORD ignored;GetOverlappedResult(pipe.h,&ov,&ignored,TRUE);break;}DWORD ignored;if(!GetOverlappedResult(pipe.h,&ov,&ignored,FALSE))continue;}else if(!connected&&error!=ERROR_PIPE_CONNECTED)continue;try{Json reply;try{auto message=readMessage(pipe.h,10000,stopEvent);auto action=str(message,"action");if(action=="grabber-rendered-projects")reply={{"ok",true},{"projects",renderedGrabberProjects(manager,str(message,"url"))}};
else if(action=="post-body-begin")reply=beginStagedPostBody(manager,message);else if(action=="post-body-chunk")reply=appendStagedPostBody(manager,message);else if(action=="post-body-finish")reply=finishStagedPostBody(manager,message);
 else if(action=="browser-batch-review"){if(!present)throw std::runtime_error("UDM cannot present the browser batch.");reply={{"ok",true},{"count",queueBrowserBatchReview(manager,message)}};present();}
 else if(action=="browser-quality-help"){
  if(!present)throw std::runtime_error("UDM cannot present video guidance.");
  if(!manager.browserQualityHelpRequested.exchange(true))present();
  reply={{"ok",true}};
 }
 else if(action=="grabber-rendered-begin")reply={{"ok",true},{"request",beginRenderedGrabberRequest(manager,str(message,"projectId"))}};
 else if(action=="grabber-rendered-complete"){auto snapshot=completeRenderedGrabberRequest(manager,message);reply={{"ok",true},{"references",snapshot["references"].size()},{"truncated",snapshot["truncated"]},{"htmlSaved",snapshot.contains("html")},{"htmlUnavailable",yes(snapshot,"htmlUnavailable")}};}
 else if(action=="grabber-rendered-cancel"){auto cancelled=cancelRenderedGrabberRequest(manager,str(message,"projectId"),str(message,"ticket"));reply={{"ok",true},{"cancelled",cancelled}};}
 else if(action=="grabber-rendered-status"){Lock lock(manager.mutex);auto id=str(message,"projectId");grabberProject(manager,id);auto records=manager.state.value("RenderedGrabber",Json::object());auto record=records.value(id,Json::object());auto ticket=str(message,"ticket");if(ticket.empty())throw std::runtime_error("Missing rendered request ticket.");auto receipt=record.value("receipt",Json::object());auto pending=record.value("pending",Json::object());reply={{"ok",true},{"status",str(receipt,"ticket")==ticket?"saved":str(pending,"ticket")==ticket&&num(pending,"expires")>epoch()?"pending":"unavailable"}};}
 else if(action=="media-status")reply=mediaAdmissionStatus(manager,str(message,"mediaToken"));else if(action=="grabber-login-pending")reply=pendingGrabberBrowserLogins(manager,str(message,"url"));else if(action=="grabber-login-complete"){reply=completeGrabberBrowserLogin(manager,message);show();}else if(action=="capture-prepare")reply=prepareCapture(manager,message);else if(action=="capture-status")reply=captureTransactionStatus(manager,str(message,"captureToken"),yes(message,"details"));else if(action=="capture-release")reply=releasePreparedCapture(manager,str(message,"captureToken"));
 else if(action=="capture-commit"){
  reply=commitPreparedCapture(manager,str(message,"captureToken"));
  const auto status=str(reply,"status");
  if(status=="accepted"&&dispatchQueue){
   Lock lock(manager.mutex);
   const auto id=str(reply,"id");
   const auto job=std::find_if(manager.jobs.begin(),manager.jobs.end(),[&](const auto& item){return item->id()==id;});
   if(job!=manager.jobs.end()&&str((*job)->data,"Status")=="Queued")dispatchQueue();
  }
  if(status=="review"&&!yes(reply,"linkChoice"))show();
  else if((status=="accepted"||status=="review")&&present)present();
 }else if(action=="capture-review-open"){auto pending=manager.pendingBrowserCaptureReviews();if(!pending.empty()){manager.browserRecoveryRequested=true;show();}reply={{"ok",true},{"reviews",pending.size()}};}else if(action=="capture-legacy-release")reply=releaseLegacyCapture(manager,message);else if(action=="capture-reconcile")reply=reconcileCapture(manager,str(message,"captureToken"));else if(action=="ping")reply={{"ok",true}};else if(action=="diagnostics"){Lock l(manager.mutex);reply={{"ok",true},{"version",UDM_VERSION_STRING},{"dataDirectory",utf8(manager.root.wstring())},{"downloads",manager.jobs.size()},{"wakeTimer",manager.queueWakeStatus()}};}else if(action=="preferences"||action=="preferences-if-running"){Lock l(manager.mutex);reply=browserPreferences(manager.state["Settings"],str(message,"browserExecutable"));reply["browserBatchReview"]=2;reply["browserImportProperties"]=1;reply["browserImportReferer"]=1;reply["mediaReceipts"]=1;reply["youtubeMediaReceipts"]=1;reply["youtubeDirectAudioReceipts"]=1;reply["mediaRequestLimit"]=BrowserRequestMessageLimit;reply["explicitProxyTransport"]=1;reply["browserProxyTypes"]=2;}else if(action=="browser-settings"){manager.browserSettingsRequested=true;show();reply={{"ok",true}};}else if(action=="automation-batch"){auto id=queueAutomationBatch(manager,message);reply={{"ok",true},{"id",id}};show();}else if(action=="show"){show();reply={{"ok",true}};}
 else if(action=="cli-start-queue"){
  manager.queueRun("Main queue",true);
  if(dispatchQueue)dispatchQueue();
  if(!yes(message,"background"))show();
  reply={{"ok",true}};
 }else{
  auto j=manager.receive(message);
  const bool browserDownload=action=="add"||action=="media"||action=="sabr"||action=="adaptive";
  const bool silentCli=action=="cli-add"&&yes(message,"silent")&&!yes(message,"paused");
  if(dispatchQueue&&(browserDownload||silentCli)){
   Lock lock(manager.mutex);
   if(str(j->data,"Status")=="Queued")dispatchQueue();
  }
  reply={{"ok",true},{"id",j->id()}};if(auto kind=str(manager.addressRefreshCandidate(j),"kind");kind=="sabr"||kind=="adaptive"||kind=="direct-media")reply["refreshPending"]=true;if(present&&browserDownload)present();if(!browserDownload&&(action!="cli-add"||!yes(message,"background"))&&(action!="automation-add"||!(num(message.at("link"),"flags")&1)))show();}}catch(const std::exception& e){reply={{"ok",false},{"error",e.what()}};}writeMessage(pipe.h,reply,10000,stopEvent);/* DisconnectNamedPipe discards unread reply bytes. Wait for client close with a cancellable deadline. */BYTE end=0;try{pipeIo(pipe.h,&end,1,false,10000,stopEvent);}catch(...) {}}catch(...){}DisconnectNamedPipe(pipe.h);}}catch(const std::exception& e){Lock l(manager.mutex);manager.storageError=e.what();}}
static Json nativeCommand(const Json& request){
 auto action=str(request,"action");
 if(action=="hello")return {{"ok",true},{"protocol",1},{"postBodyLimit",MaxBrowserPostBytes},{"stagedPostBodyLimit",MaxStagedPostBytes},{"postBodyChunkBytes",PostBodyChunkBytes},{"version",UDM_VERSION_STRING},{"capabilities",Json::array({"persistent","request-headers","post-downloads","post-body-chunks","adaptive-audio","media-receipts","adaptive-subtitles","adaptive-resources","live-hls","live-dash","live-dash-period-selection","live-dash-mp4-webvtt","live-dash-segment-list","live-dash-list-index-metadata","live-dash-segment-base","live-dash-template-index","live-dash-template-initialization","live-dash-ept-offset","live-dash-duration-overlap","live-dash-segment-index-metadata","live-hls-subtitles","live-hls-subtitle-map","hls-subtitle-map","hls-recorded-subtitle-map-changes","hls-recorded-subtitle-discontinuity","hls-recorded-discontinuity","hls-recorded-map-changes","hls-variables","capture-recovery","capture-transaction","capture-review","capture-legacy-review","sabr-audio","media-session-refresh","adaptive-session-refresh","dash-periods","dash-single-period-timing","dash-mp4-webvtt","direct-media-refresh","stream-resume","grabber-browser-login","grabber-rendered-storage"})}};
 if(recoveryInProgress())return recoveryBusyReply();
 if(action=="dash-live-period"){
  const auto address=str(request,"url");const Url source(address);if(source.scheme!="http"&&source.scheme!="https")throw std::runtime_error("DASH Period selection requires HTTP.");
  Json preferences=defaultSettings();const auto stateFile=defaultData()/L"state.json";if(fs::exists(stateFile)){const auto stored=Json::parse(readText(stateFile));if(stored.contains("Settings")&&stored["Settings"].is_object())preferences=stored["Settings"];}
  auto cancel=std::make_shared<Cancel>();cancel->deadline=GetTickCount64()+30000;return selectDashLivePeriod(address,browserHeaders(request),preferences,cancel);
 }
 if(action=="cli-add"||action=="cli-start-queue")throw std::runtime_error("Command-line requests must come from the local UDM executable.");
 if(action=="youtube-player"){
  youtubePlayerRequest(request);Json preferences=defaultSettings();auto stateFile=defaultData()/L"state.json";
  if(fs::exists(stateFile)){auto stored=Json::parse(readText(stateFile));if(stored.contains("Settings")&&stored["Settings"].is_object())preferences=stored["Settings"];}
  try{return retrieveYouTubePlayer(request,preferences);}
  catch(const Cancelled&){return {{"ok",false},{"error","Player retrieval timed out; browser capture remains available."}};}
  catch(const Json::exception&){return {{"ok",false},{"error","Player response format is unsupported."}};}
 }
 // Only failure to connect can trigger startup/retry. A lost reply may follow a saved job.
 try{return sendDesktop(request,12000,true);}catch(const DesktopNotRunning&){
  if(recoveryInProgress())return recoveryBusyReply();
  if(action.rfind("grabber-rendered-",0)==0||action=="preferences-if-running"||action=="grabber-login-pending"||action=="grabber-login-complete")throw;
  if(action!="media-status"&&action!="capture-legacy-release"&&action!="ping"&&action!="show"&&action!="diagnostics"&&action!="preferences"&&action!="browser-settings"&&action!="browser-batch-review"&&action!="browser-quality-help"&&action!="add"&&action!="post-body-begin"&&action!="post-body-chunk"&&action!="post-body-finish"&&action!="media"&&action!="adaptive"&&action!="sabr"&&action!="capture-reconcile"&&action!="capture-prepare"&&action!="capture-status"&&action!="capture-release"&&action!="capture-commit"&&action!="capture-review-open")throw std::runtime_error("Unknown UDM command.");
  return startDesktopAndSend(request);
 }
}
int nativeHost(){
 try{
  auto input=GetStdHandle(STD_INPUT_HANDLE),output=GetStdHandle(STD_OUTPUT_HANDLE);
  auto exact=[](HANDLE handle,void* buffer,DWORD size,bool write,bool allowEof=false){
   BYTE* at=(BYTE*)buffer;DWORD remaining=size;
   while(remaining){DWORD done=0;BOOL ok=write?WriteFile(handle,at,remaining,&done,nullptr):ReadFile(handle,at,remaining,&done,nullptr);
    if(!ok||!done){if(allowEof&&remaining==size&&!write)return false;throw std::runtime_error("Native message stream closed.");}
    remaining-=done;at+=done;
   }return true;
  };
  for(;;){
   DWORD size=0;if(!exact(input,&size,4,false,true))return 0;if(size<1||size>BrowserRequestMessageLimit)return 1;
   std::string text(size,0);exact(input,text.data(),size,false);Json reply;std::optional<i64> requestId;
   try{
    auto request=Json::parse(text);if(!request.is_object())throw std::runtime_error("Invalid native request.");
    if(request.contains("requestId")){
     if(!request["requestId"].is_number_integer())throw std::runtime_error("Invalid native request ID.");
     auto value=request["requestId"].get<i64>();if(value<1||value>2147483647)throw std::runtime_error("Invalid native request ID.");requestId=value;
    }
    if(str(request,"action")=="preferences"||str(request,"action")=="preferences-if-running")request["browserExecutable"]=nativeBrowserExecutable();
    reply=nativeCommand(request);
   }catch(const std::exception& e){reply={{"ok",false},{"error",e.what()}};}
   if(requestId)reply["requestId"]=*requestId;
   text=reply.dump();size=(DWORD)text.size();if(size>BrowserReplyMessageLimit)return 1;
   exact(output,&size,4,true);exact(output,text.data(),size,true);
   if(!requestId)return 0; // Existing sendNativeMessage clients retain one-shot behavior.
  }
 }catch(...){return 1;}
}
}
namespace udm {
Json automationCommand(const Json& request,const fs::path& data){
 auto action=str(request,"action");if(action!="automation-add"&&action!="automation-batch")throw std::invalid_argument("Unknown automation command.");
 if(action=="automation-add")validateAutomationLink(request.at("link"));else validateAutomationBatch(request);
 try{return sendDesktop(request,12000,true);}catch(const DesktopNotRunning&){
  return startDesktopAndSend(request,data);
 }
}
}
