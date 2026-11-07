#include <future>
#include "CaptureTransactions.hpp"
#include "OriginalResponsePolicy.hpp"
#include "../drivers/signed-network/DurableResponseSink.hpp"
#include "AdaptiveCapture.hpp"
#include "MediaAdmission.hpp"
#include "Scanner.hpp"
#include "FileIdentity.hpp"
namespace udm {
JobPtr Manager::receiveBrowserMedia(const Json& message,const std::function<void(const char*)>& checkpoint){
 Lock lock(mutex);auto token=str(message,"mediaToken");auto created=captureTime(token),now=epoch();
 const auto action=str(message,"action");
 const bool directAudio=action=="add"&&yes(message,"youtubeDirectAudio");
 if((action!="adaptive"&&action!="media"&&action!="sabr"&&!directAudio)||message.contains("captureToken"))
  throw std::runtime_error("Media receipts require a video or streaming audio download.");
 if(directAudio){
  validateStream(str(message,"url"));
  validateSource(str(message,"youtubePage"));
  const auto extension=fs::path(wide(lower(str(message,"filename")))).extension();
  if(!message.value("request",Json::object()).empty()||extension!=L".m4a")
   throw std::runtime_error("Direct audio receipts require an M4A download.");
 }
 auto request=message;request.erase("mediaToken");request.erase("requestId");auto fingerprint=mediaAdmissionDigest(request);auto rows=captureStore(*this);
 if(rows.contains(token)){
  const auto& row=rows[token];
  if(num(row,"protocol")!=3||str(row,"status")!="accepted")throw std::runtime_error("This media operation was released or belongs to another request. Check its receipt.");
  if(reveal(str(row,"fingerprint"))!=fingerprint)throw std::runtime_error("The accepted media selection changed. Do not reuse its token.");
  for(auto job:jobs)if(job->id()==str(row,"id"))return job;
  throw std::runtime_error("This media download was already accepted and its history record was later removed.");
 }
 if(created<now-600000||created>now+60000)throw std::runtime_error("Media operation token has expired.");
 if(action=="adaptive")validateAdaptiveCapture(request);
 captureSpace(rows,now);
 if(catalogTransaction)throw std::runtime_error("Another catalog operation is in progress.");
 const auto protectedFingerprint=protect(fingerprint);
 auto previousState=state;auto previousJobs=jobs;auto previousOffers=pendingOffers;auto previousExisting=existingOffers;
 std::vector<std::pair<JobPtr,Json>> before;for(auto job:jobs)before.push_back({job,job->data});
 struct DownloadProgress {JobPtr job;std::vector<Worker> workers;double speed;SpeedMeter meter;std::optional<i64> limit;};
 std::vector<DownloadProgress> progress;
 if(directAudio)for(auto job:jobs)progress.push_back({job,job->workers,job->speed,job->speedMeter,job->sessionLimit});
 auto oldRefresh=refreshId;auto oldRefreshUntil=refreshUntil;JobPtr admitted;
 catalogCheckpoint=checkpoint;catalogTransaction=true;
 try{
  admitted=receive(request);
  if(directAudio){rows=captureStore(*this);captureSpace(rows,now);}
  bool refreshed=false;for(const auto& old:before)if(old.first==admitted)refreshed=str(old.second,"ProtectedRefreshOffer")!=str(admitted->data,"ProtectedRefreshOffer");
  Json receipt={{"created",created},{"protocol",3},{"status","accepted"},{"id",admitted->id()},{"fingerprint",protectedFingerprint}};
  const auto status=str(admitted->data,"Status");
  const bool offered=directAudio&&std::find(pendingOffers.begin(),pendingOffers.end(),admitted)!=pendingOffers.end();
  if(refreshed||status=="Awaiting confirmation"||status=="Awaiting duplicate choice"||offered){
   const char* kind=refreshed?"refresh":offered&&existingOffers.count(admitted->id())?"existing":"download";
   receipt["presentation"]={{"id",admitted->id()},{"kind",kind}};
   if(offered)pendingOffers.erase(std::remove(pendingOffers.begin(),pendingOffers.end(),admitted),pendingOffers.end());
  }
  rows[token]=receipt;state["BrowserCaptures"]=rows;
  if(checkpoint)checkpoint("before-commit");
  catalogTransaction=false;save();
 }catch(...){
  catalogTransaction=false;catalogCheckpoint={};state.swap(previousState);jobs.swap(previousJobs);pendingOffers.swap(previousOffers);existingOffers.swap(previousExisting);
  for(auto& old:before)old.first->data.swap(old.second);
  for(auto& old:progress){old.job->workers.swap(old.workers);old.job->speed=old.speed;std::swap(old.job->speedMeter,old.meter);old.job->sessionLimit=old.limit;}
  refreshId=oldRefresh;refreshUntil=oldRefreshUntil;
  if(directAudio)recoverRestarts();
  throw;
 }
 catalogCheckpoint={};
 // Failure here models a lost acknowledgement; the committed job must survive.
 if(checkpoint)checkpoint("committed");
 if(directAudio)recoverRestarts();
 return admitted;
}

static Json browserPresentationOffers(const Json& rows,const std::string* onlyId=nullptr){
 Json offers=Json::array();std::map<std::string,size_t> groups;
 for(auto it=rows.begin();it!=rows.end();++it){
  const auto& row=it.value();if(str(row,"status")!="accepted"||!row.contains("presentation"))continue;
  const auto& presentation=row.at("presentation");auto id=str(presentation,"id"),kind=str(presentation,"kind");
  if(kind!="download"&&kind!="existing"&&kind!="refresh")throw std::runtime_error("Invalid saved browser presentation. The download history was preserved.");
  if(onlyId&&id!=*onlyId)continue;
  auto inserted=groups.emplace(id,offers.size());if(inserted.second)offers.push_back({{"id",id},{"kind",kind},{"tokens",Json::array()}});
  auto combined=&offers.at(inserted.first->second);
  combined->at("tokens").push_back(it.key());
  if(kind=="refresh"||(kind=="existing"&&str(*combined,"kind")!="refresh"))(*combined)["kind"]=kind;
 }
 return offers;
}
Json Manager::pendingBrowserPresentations()const{
 Lock lock(mutex);auto offers=browserPresentationOffers(captureStoreView(*this));
 if(offers.empty())return offers;
 std::set<std::string> missing;for(const auto& offer:offers)missing.insert(str(offer,"id"));
 // Recent captures normally sit at the end of the history.
 for(auto it=jobs.rbegin();it!=jobs.rend()&&!missing.empty();++it)missing.erase((*it)->id());
 if(!missing.empty())offers.erase(std::remove_if(offers.begin(),offers.end(),[&](const Json& offer){return missing.count(str(offer,"id"))!=0;}),offers.end());
 return offers;
}
bool Manager::hasBrowserPresentation(JobPtr job)const{
 Lock lock(mutex);const auto id=job->id();
 for(const auto& row:captureStoreView(*this))if(row.contains("presentation")&&str(row,"status")=="accepted"&&str(row.at("presentation"),"id")==id)return true;
 return false;
}
JobPtr Manager::restoreBrowserPresentation(const Json& requested){
 Lock lock(mutex);const auto id=str(requested,"id");
 const auto current=browserPresentationOffers(captureStoreView(*this),&id);
 if(current.empty()||current.front()!=requested)return {};
 for(auto it=jobs.rbegin();it!=jobs.rend();++it)if((*it)->id()==id){
  if(str(requested,"kind")=="existing")existingOffers.insert(id);
  return *it;
 }
 return {};
}
void Manager::finishBrowserPresentation(const Json& offer){
 Lock lock(mutex);auto rows=captureStore(*this);bool changed=false;
 for(const auto& token:offer.at("tokens"))if(token.is_string()&&rows.contains(token.get<std::string>())){
  auto& row=rows[token.get<std::string>()];if(str(row,"status")=="accepted"&&row.contains("presentation")){row.erase("presentation");changed=true;}
 }
 if(changed)saveCaptureStore(*this,rows);
}
Json Manager::browserCaptureContext(const Json& request)const{
 Lock lock(mutex);if(refreshId.empty()||refreshUntil<epoch()||!request.value("request",Json::object()).empty())return Json::object();
 JobPtr target;for(auto job:jobs)if(job->id()==refreshId)target=job;
 if(!canRefreshAddress(target)||!str(target->data,"SourceUrl").empty()||!str(target->data,"ProtectedAdaptive").empty())return Json::object();
 Url original(str(target->data,"Url")),next(str(request,"url"));auto name=str(request,"filename");
 if(original.scheme=="ftp")return Json::object();
 auto filename=lower(safeName(name.empty()?unescape(next.path):name));
 if(filename!=lower(str(target->data,"FileName"))&&filename!=lower(safeName(unescape(original.path))))return Json::object();
 return {{"refreshId",refreshId},{"url",str(target->data,"Url")},{"filename",str(target->data,"FileName")}};
}
Json Manager::commitBrowserCapture(const std::string& token,const std::function<void(const char*)>& checkpoint){
 Lock lock(mutex);auto status=captureTransactionStatus(*this,token);
 if(str(status,"status")!="prepared")return status;
 if(catalogTransaction)throw std::runtime_error("Another catalog operation is in progress.");
 const auto receipt=captureStore(*this).at(token);
 auto envelope=preparedCaptureEnvelope(receipt);auto request=envelope.at("download"),context=envelope.at("context");
 const auto savedRefreshId=refreshId;const auto savedRefreshUntil=refreshUntil;
 if(!context.empty()){
  JobPtr target;for(auto job:jobs)if(job->id()==str(context,"refreshId"))target=job;
  if(!canRefreshAddress(target)||Url(str(target->data,"Url")).scheme=="ftp"||!str(target->data,"SourceUrl").empty()||!str(target->data,"ProtectedAdaptive").empty()||str(target->data,"Url")!=str(context,"url")||str(target->data,"FileName")!=str(context,"filename")){
   // Commit is sent only after browser cancellation. Keep that ownership and
   // the protected request durably, without replaying or changing another job.
   auto rows=captureStore(*this);rows[token]["status"]="review";saveCaptureStore(*this,rows);
   return {{"ok",true},{"status","review"}};
  }
 }
 // Snapshot the mutable effects of ordinary admission, including a paused
 // duplicate's transient progress state. No download worker starts under this lock.
 auto previousState=state;auto previousJobs=jobs,previousOffers=pendingOffers;auto previousExisting=existingOffers;
 struct Before {JobPtr job;Json data;std::vector<Worker> workers;double speed;SpeedMeter meter;std::optional<i64> limit;};
 std::vector<Before> before;before.reserve(jobs.size());
 for(auto job:jobs)before.push_back({job,job->data,job->workers,job->speed,job->speedMeter,job->sessionLimit});
 JobPtr admitted;
 // Only the intent saved during preparation may route a captured link to a
 // refresh target. An unrelated, newly armed UI refresh cannot hijack it.
 refreshId=str(context,"refreshId");refreshUntil=refreshId.empty()?0:epoch()+60000;
 catalogCheckpoint=checkpoint;catalogTransaction=true;
 try{
  admitted=receive(request);
  if(!context.empty()&&admitted->id()!=str(context,"refreshId"))throw std::runtime_error("The prepared replacement address no longer matches its download.");
  state["BrowserCaptures"][token]={{"created",num(receipt,"created")},{"protocol",2},{"status","accepted"},{"id",admitted->id()}};
  const bool offered=std::find(pendingOffers.begin(),pendingOffers.end(),admitted)!=pendingOffers.end();
  const auto jobStatus=str(admitted->data,"Status");
  if(!context.empty()||offered||jobStatus=="Awaiting confirmation"||jobStatus=="Awaiting duplicate choice"){
   const char* kind=!context.empty()?"refresh":existingOffers.count(admitted->id())?"existing":"download";
   state["BrowserCaptures"][token]["presentation"]={{"id",admitted->id()},{"kind",kind}};
   pendingOffers.erase(std::remove(pendingOffers.begin(),pendingOffers.end(),admitted),pendingOffers.end());
  }
  if(catalogCheckpoint)catalogCheckpoint("before-commit");
  catalogTransaction=false;
  // This one replacement publishes BOTH the final job and its accepted receipt.
  // A crash before replacement leaves the protected prepared request retryable.
  save();
 }catch(...){
  catalogTransaction=false;catalogCheckpoint={};
  refreshId=savedRefreshId;refreshUntil=savedRefreshUntil;
  state.swap(previousState);jobs.swap(previousJobs);pendingOffers.swap(previousOffers);existingOffers.swap(previousExisting);
  for(auto& old:before){old.job->data.swap(old.data);old.job->workers.swap(old.workers);old.job->speed=old.speed;std::swap(old.job->speedMeter,old.meter);old.job->sessionLimit=old.limit;}
  // A restart journal written during staging has no matching committed attempt.
  // Recovery removes that journal without retiring the previous partial files.
  recoverRestarts();throw;
 }
 catalogCheckpoint={};
 refreshId=savedRefreshId;refreshUntil=savedRefreshUntil;
 // This observation point is deliberately outside the rollback path: the catalog
 // is durable even if a caller fails or the process exits before its reply.
 if(checkpoint)checkpoint("committed");
 recoverRestarts();
 try{rememberAutomaticCapture(admitted,token,str(request,"url"));}catch(...){}
 return {{"ok",true},{"status","accepted"},{"id",admitted->id()}};
}
Json Manager::pendingBrowserCaptureReviews()const{
 Lock lock(mutex);Json result=Json::array();auto rows=captureStore(*this);
 for(auto it=rows.begin();it!=rows.end();++it)if(num(it.value(),"protocol")==2&&str(it.value(),"status")=="review")result.push_back(it.key());
 return result;
}
Json Manager::browserCaptureReview(const std::string& token)const{
 Lock lock(mutex);captureTime(token);auto rows=captureStore(*this);
 if(!rows.contains(token)||num(rows[token],"protocol")!=2||str(rows[token],"status")!="review")return Json();
 const auto& receipt=rows[token];auto envelope=preparedCaptureEnvelope(receipt);const auto& request=envelope.at("download");const auto& context=envelope.at("context");
 JobPtr target;for(auto job:jobs)if(job->id()==str(context,"refreshId"))target=job;
 bool eligible=canRefreshAddress(target)&&Url(str(target->data,"Url")).scheme!="ftp"&&str(target->data,"SourceUrl").empty()&&str(target->data,"ProtectedAdaptive").empty()&&request.value("request",Json::object()).empty();
 // These revisions stay inside the native UI. Request headers are never shown.
 return {{"token",token},{"receiptRevision",receipt},{"targetRevision",target?target->data:Json()},
  {"originalId",str(context,"refreshId")},{"originalName",str(context,"filename")},{"previousUrl",str(context,"url")},
  {"newUrl",str(request,"url")},{"filename",str(request,"filename")},{"canRefresh",eligible},
  {"targetPresent",bool(target)},{"targetName",target?str(target->data,"FileName"):""},{"targetUrl",target?str(target->data,"Url"):""},
  {"targetStatus",target?str(target->data,"Status"):"Removed"},{"received",target?num(target->data,"Received"):0}};
}
Json Manager::resolveBrowserCaptureReview(const Json& displayed,const std::string& choice){
 Lock lock(mutex);const auto token=str(displayed,"token");auto latest=browserCaptureReview(token);
 if(latest.is_null()||latest!=displayed)throw std::runtime_error("The original download changed again. Review the updated details before choosing.");
 if(choice!="new"&&choice!="refresh"&&choice!="discard")throw std::runtime_error("Choose a new download, review the replacement address, or discard the link.");
 if(catalogTransaction)throw std::runtime_error("Another catalog operation is in progress.");
 if(choice=="refresh"&&!yes(latest,"canRefresh"))throw std::runtime_error("Pause an eligible unfinished download before using this replacement address.");
 const auto receipt=captureStore(*this).at(token);auto request=preparedCaptureEnvelope(receipt).at("download");
 if(choice=="discard"){
  auto rows=captureStore(*this);rows[token]={{"created",num(receipt,"created")},{"protocol",2},{"status","discarded"}};
  saveCaptureStore(*this,rows);return {{"ok",true},{"status","discarded"}};
 }
 auto headers=udm::browserHeaders(request);auto address=str(request,"url");auto post=validatePostRequest(request.value("request",Json::object()),address);
 auto proxy=validateBrowserProxy(request.value("browserProxy",Json::object()),address);auto session=browserDownloadSession(request.value("browserSession",Json::object()),address,headers);
 const auto previousState=state;const auto previousJobs=jobs;JobPtr target;Json previousTarget;
 for(auto job:jobs)if(job->id()==str(latest,"originalId"))target=job;
 if(target)previousTarget=target->data;
 catalogTransaction=true;JobPtr admitted;
 try{
  if(choice=="new"){
   // Explicitly requesting a new file never reuses an existing/active duplicate
   // and never inherits Skip File Info or automatic queue-start preferences.
   admitted=add(address,"",str(request,"filename"),"Main queue",true,headers,"",post,proxy,session);
   admitted->data["Status"]="Awaiting confirmation";admitted->data["QueueMember"]=false;
   if(!str(request,"referrer").empty()){Url page(str(request,"referrer"));if(page.scheme=="http"||page.scheme=="https")admitted->data["DownloadPage"]=page.full;}
  }else{
   admitted=target;
   Json offer={{"url",address},{"headers",Json(headers)},{"page",str(request,"referrer")},{"received",epoch()},{"browserSession",session}};
   if(request.contains("browserProxy"))offer["browserProxy"]=proxy;
   admitted->data["ProtectedRefreshOffer"]=protect(offer.dump());
  }
  state["BrowserCaptures"][token]={{"created",num(receipt,"created")},{"protocol",2},{"status","accepted"},{"id",admitted->id()},
   {"presentation",{{"id",admitted->id()},{"kind",choice=="new"?"download":"refresh"}}}};
  catalogTransaction=false;save();
 }catch(...){catalogTransaction=false;state=previousState;jobs=previousJobs;if(target)target->data=previousTarget;throw;}
 return {{"ok",true},{"status","accepted"},{"id",admitted->id()}};
}
OriginalResponseReservation Manager::reserveOriginalResponse(const Json& capture,const Json& offer,const std::function<void(const char*)>& checkpoint){
 Lock lock(mutex);
 if(catalogTransaction)throw std::runtime_error("Another catalog operation is in progress.");
 auto session=str(capture,"Session");
 if(!std::regex_match(session,std::regex("[a-fA-F0-9]{32}"))||!capture.contains("Generation")||!capture["Generation"].is_number_unsigned()||!capture["Generation"].get<uint64_t>()||!offer.contains("Id")||!offer["Id"].is_number_unsigned()||!offer["Id"].get<uint64_t>())throw std::runtime_error("Invalid original response identity.");
 if(!yes(capture,"ResponseAdmissionEnabled")||!capture.contains("PendingResponses")||!capture["PendingResponses"].is_array()||std::find(capture["PendingResponses"].begin(),capture["PendingResponses"].end(),offer)==capture["PendingResponses"].end())throw std::runtime_error("The response is not a pending browser offer.");
 if(!yes(offer,"WasInterceptable")||str(offer,"Method")!="GET"||num(offer,"Status")!=200||yes(offer,"RequestBodyFramed")||!str(offer,"Range").empty())throw std::runtime_error("This response cannot be adopted as a complete file.");
 auto host=str(offer,"Host"),target=str(offer,"Target");
 if(host.empty()||host.find_first_of("/\\?#@ \r\n\t")!=std::string::npos||target.empty()||target[0]!='/')throw std::runtime_error("Invalid original HTTP response address.");
 auto address="http://"+host+target;Url parsed(address);
 if(parsed.scheme!="http")throw std::runtime_error("Original response capture requires HTTP.");
 i64 length=-1;if(offer.contains("Length")&&!offer["Length"].is_null()){if(!offer["Length"].is_number_unsigned()||offer["Length"].get<uint64_t>()>static_cast<uint64_t>(INT64_MAX))throw std::runtime_error("Invalid original response length.");length=offer["Length"].get<i64>();}
 for(auto job:jobs)if(job->data.contains("OriginalResponse")){const auto& previous=job->data["OriginalResponse"];if(str(previous,"Session")==session&&previous.value("Generation",Json())==capture["Generation"]&&previous.value("OfferId",Json())==offer["Id"])throw std::runtime_error("This response already has a saved job. Recover its receipt instead of creating another download.");}
 // Keep the original representation identity before admitting its body. A weak
 // tag cannot authorize byte-range continuation; invalid fields cannot become
 // headers on a later request.
 auto tag=str(offer,"ETag"),modified=str(offer,"LastModified");
 for(const auto& value:{tag,modified}){if(value.size()>8192||std::any_of(value.begin(),value.end(),[](unsigned char c){return c<32||c==127;}))throw std::runtime_error("Invalid original response validator.");}
 bool strong=tag.size()>=2&&tag.front()=='"'&&tag.back()=='"'&&tag.substr(1,tag.size()-2).find('"')==std::string::npos;
 SYSTEMTIME modifiedTime{};bool validDate=!modified.empty()&&WinHttpTimeToSystemTime(wide(modified).c_str(),&modifiedTime);
 Headers replayHeaders;
 if(offer.contains("ProtectedRequestHeaders"))try{
  // Allow JSON escaping, DPAPI framing and base64 expansion of the bounded
  // 48 KiB capture metadata; decoded header limits remain enforced below.
  if(!offer["ProtectedRequestHeaders"].is_string()||str(offer,"ProtectedRequestHeaders").empty()||str(offer,"ProtectedRequestHeaders").size()>192*1024)throw std::runtime_error("Invalid envelope");
  auto envelope=Json::parse(reveal(str(offer,"ProtectedRequestHeaders")));
  if(str(envelope,"Host")!=host||str(envelope,"Target")!=target||!envelope.contains("Headers")||!envelope["Headers"].is_object()||envelope["Headers"].size()>7)throw std::runtime_error("Wrong request binding");
  size_t bytes=0;for(auto it=envelope["Headers"].begin();it!=envelope["Headers"].end();++it){if(!it.value().is_string())throw std::runtime_error("Invalid field");auto value=it.value().get<std::string>();bytes+=it.key().size()+value.size();if(bytes>48*1024)throw std::runtime_error("Header limit");replayHeaders.emplace(it.key(),std::move(value));}
  validateHeaders(replayHeaders);
 }catch(...){throw std::runtime_error("Invalid protected request headers for this captured response.");}
 auto oldState=state;auto oldJobs=jobs;JobPtr job;Handle body,receipt;fs::path folder;bool created=false;
 catalogCheckpoint=checkpoint;catalogTransaction=true;
 try{
  job=add(address,"",originalResponseName(offer),"Main queue",false,replayHeaders);job->data["QueueMember"]=false;job->data["Connections"]=1;job->data["Size"]=length;
  job->data["ETag"]=strong?Json(tag):Json();job->data["Modified"]=validDate?Json(modified):Json();
  job->data["OriginalResponse"]={{"Session",session},{"Generation",capture["Generation"]},{"OfferId",offer["Id"]},{"State","Reserved"}};
  folder=root/L"responses"/wide(job->id());fs::create_directories(folder.parent_path());created=fs::create_directory(folder);if(!created)throw std::runtime_error("The response reservation folder already exists.");
  body.h=CreateFileW((folder/L"body").c_str(),GENERIC_READ|GENERIC_WRITE,FILE_SHARE_READ|FILE_SHARE_WRITE,nullptr,CREATE_NEW,0,nullptr);
  receipt.h=CreateFileW((folder/L"receipt").c_str(),GENERIC_READ|GENERIC_WRITE,FILE_SHARE_READ|FILE_SHARE_WRITE,nullptr,CREATE_NEW,0,nullptr);
  if(!body||!receipt)throw std::runtime_error("Cannot reserve original response files.");
  if(checkpoint)checkpoint("before-commit");catalogTransaction=false;save();
 }catch(...){catalogTransaction=false;catalogCheckpoint={};state=oldState;jobs=oldJobs;if(body){CloseHandle(body.h);body.h=INVALID_HANDLE_VALUE;}if(receipt){CloseHandle(receipt.h);receipt.h=INVALID_HANDLE_VALUE;}if(created){std::error_code ec;fs::remove(folder/L"body",ec);fs::remove(folder/L"receipt",ec);fs::remove(folder,ec);}throw;}
 catalogCheckpoint={};if(checkpoint)checkpoint("committed");return {job,std::move(body),std::move(receipt)};
}
void Manager::requestOriginalResponsePause(JobPtr job,bool deferCompletedSave){
 auto before=job->data;
 auto& response=job->data.at("OriginalResponse");
 response["PauseRequested"]=true;
 if(deferCompletedSave)response["DeferCompletedSave"]=true;
 job->data["Status"]="Pausing";
 try{save();}catch(...){job->data.swap(before);throw;}
}
void Manager::stopOriginalResponses(Monitor& monitor){
 Lock lock(mutex);
 // Cancel owned responses while the redirect path still exists, allowing the
 // relay to close the browser side before removing its network filters.
 Json snapshot=Json::object();
 // Snapshot may reconnect an existing helper, but never starts a new one.
 // If it has exited, recover the receipt after its file handles close.
 try{snapshot=monitor.snapshot();}catch(...){if(monitor.running())throw;}
 auto capture=snapshot.value("Capture",Json::object());
 for(auto job:jobs)if(job->data.contains("OriginalResponse")){
  auto& record=job->data["OriginalResponse"];
  if((str(record,"State")=="Receiving"||str(record,"State")=="Reserved")&&record.value("Session",Json())==capture.value("Session",Json())&&record.value("Generation",Json())==capture.value("Generation",Json())){
   for(const auto& row:capture.value("Responses",Json::array()))if(row.value("Id",Json())==record.at("OfferId")){
    auto seconds=real(row,"TransferSeconds");if(std::isfinite(seconds)&&seconds>0){job->data["TransferSeconds"]=seconds;job->data["ElapsedSeconds"]=seconds;}
    if(row.contains("Received")&&row["Received"].is_number_unsigned()&&row["Received"].get<uint64_t>()<=static_cast<uint64_t>(INT64_MAX))job->data["TransferredBytes"]=row["Received"];
   }
   requestOriginalResponsePause(job,false);monitor.cancelResponse(capture,record.at("OfferId").get<uint64_t>());
  }
 }
 auto deadline=GetTickCount64()+3000;
 for(;;){recoverOriginalResponses();bool receiving=false;for(auto job:jobs)receiving|=job->data.contains("OriginalResponse")&&(str(job->data["OriginalResponse"],"State")=="Receiving"||str(job->data["OriginalResponse"],"State")=="Reserved");
  if(!receiving||GetTickCount64()>=deadline)break;Sleep(10);
 }
 monitor.watch({});
 recoverOriginalResponses();
 for(auto job:jobs)if(job->data.contains("OriginalResponse"))job->speed=0;
 save();
}
Json Manager::processOriginalResponses(Monitor& monitor,const Json& capture){
 Lock lock(mutex);
 return processOriginalResponsesImpl(monitor,capture,[&](const Json& offer){auto browser=originalResponseProcess(offer);return originalResponseBrowser(browser)&&originalResponseAllowed(state["Settings"],browser,offer);},true);
}
Json Manager::processOriginalResponses(Monitor& monitor,const Json& capture,const std::string& browser){
 Lock lock(mutex);
 return processOriginalResponses(monitor,capture,[&](const Json& offer){return originalResponseAllowed(state["Settings"],browser,offer);});
}
Json Manager::processOriginalResponses(Monitor& monitor,const Json& capture,const std::function<bool(const Json&)>& policy){
 return processOriginalResponsesImpl(monitor,capture,policy,false);
}
Json Manager::processOriginalResponsesImpl(Monitor& monitor,const Json& capture,const std::function<bool(const Json&)>& policy,bool background){
 Lock lock(mutex);Json result={{"Accepted",0},{"Rejected",0},{"Published",0},{"Errors",Json::array()}};
 if(!policy||!capture.is_object()||!capture.contains("PendingResponses")||!capture["PendingResponses"].is_array()||capture["PendingResponses"].size()>16)throw std::runtime_error("Invalid original-response controller input.");
 for(const auto& offer:capture["PendingResponses"]){JobPtr job;
  try{
   if(!offer.contains("Id")||!offer["Id"].is_number_unsigned())throw std::runtime_error("Invalid pending response identifier.");
   auto id=offer["Id"].get<uint64_t>();bool known=false;
   for(const auto& saved:jobs)if(saved->data.contains("OriginalResponse")){const auto& record=saved->data["OriginalResponse"];if(record.value("Session",Json())==capture.value("Session",Json())&&record.value("Generation",Json())==capture.value("Generation",Json())&&record.value("OfferId",Json())==offer["Id"]){known=true;break;}}
   if(known)continue;
   if(stopping||!policy(offer)){monitor.rejectResponse(capture,id);result["Rejected"]=result["Rejected"].get<unsigned>()+1;continue;}
   const bool controlled=num(state["Settings"],"QuotaMb")>0||num(state["Settings"],"LimitKbps")>0;
   if(controlled&&!yes(capture,"ResponseCreditsSupported")){monitor.rejectResponse(capture,id);throw std::runtime_error("Update the network helper before capturing downloads with a quota or speed limit.");}
   auto reservation=reserveOriginalResponse(capture,offer);job=reservation.job;
   if(controlled){job->data["OriginalResponse"]["FlowControlled"]=true;job->data["OriginalResponse"]["GrantedBytes"]=0;save();}
   const bool accepted=monitor.admitResponse(capture,id,reservation.body.h,reservation.receipt.h,controlled);
   job->data["OriginalResponse"]["State"]=accepted?"Receiving":"Unaccepted";
   job->data["Status"]=accepted?"Downloading":"Paused";job->speedMeter.reset(0);
   job->data["Error"]=accepted?"Receiving the original browser response.":"The response offer expired; the browser retained ownership.";save();
   const char* count=accepted?"Accepted":"Rejected";result[count]=result[count].get<unsigned>()+1;
   if(accepted&&event)event(job,false);
  }catch(const std::exception& e){result["Errors"].push_back({{"Id",job?job->id():""},{"Message",e.what()}});}
 }
 // Progress is accepted only for the exact persisted session, generation and offer.
 // A pause intent is durable before the irreversible broker cancellation.
 for(auto job:jobs){
  if(!job->data.contains("OriginalResponse"))continue;
  auto& record=job->data["OriginalResponse"];
  if((str(record,"State")!="Receiving"&&str(record,"State")!="Reserved")||record.value("Session",Json())!=capture.value("Session",Json())||record.value("Generation",Json())!=capture.value("Generation",Json()))continue;
  try{
   // A lost admission reply leaves the durable reservation intact. A fresh,
   // authenticated progress row can prove that the same helper accepted it.
   if(str(record,"State")=="Reserved"){
    const auto rows=capture.value("Responses",Json::array());if(!rows.is_array()||rows.size()>64)throw std::runtime_error("Invalid response progress snapshot.");
    const Json* owned=nullptr;for(const auto& row:rows)if(row.value("Id",Json())==record.at("OfferId")){if(owned)throw std::runtime_error("Ambiguous response ownership snapshot.");owned=&row;}
    if(!owned)continue;const auto phase=str(*owned,"State");if(phase!="Receiving"&&phase!="Finalizing"&&phase!="Complete")continue;
    if(!owned->contains("Received")||!(*owned)["Received"].is_number_unsigned()||(*owned)["Received"].get<uint64_t>()>static_cast<uint64_t>(INT64_MAX))throw std::runtime_error("Invalid response byte count.");
    const auto received=(*owned)["Received"].get<i64>();if(received<num(job->data,"Received")||(num(job->data,"Size",-1)>=0&&received>num(job->data,"Size")))throw std::runtime_error("Inconsistent response byte count.");
    if(!owned->contains("FlowControlled")||!(*owned)["FlowControlled"].is_boolean()||yes(*owned,"FlowControlled")!=yes(record,"FlowControlled"))throw std::runtime_error("Response ownership credit mode changed.");
    auto before=job->data;record["State"]="Receiving";job->data["Status"]=yes(record,"PauseRequested")?"Paused":"Downloading";job->data["Error"]="Receiving the original browser response.";job->speedMeter.reset(0);
    try{save();}catch(...){job->data=before;throw;}result["Accepted"]=result["Accepted"].get<unsigned>()+1;if(event)event(job,false);
   }
   if(yes(record,"PauseRequested"))monitor.cancelResponse(capture,record.at("OfferId").get<uint64_t>());
   auto rows=capture.value("Responses",Json::array());
   if(!rows.is_array()||rows.size()>64)throw std::runtime_error("Invalid response progress snapshot.");
   for(const auto& row:rows)if(row.value("Id",Json())==record.at("OfferId")){
    if(!row.contains("Received")||!row["Received"].is_number_unsigned()||row["Received"].get<uint64_t>()>static_cast<uint64_t>(INT64_MAX))throw std::runtime_error("Invalid response byte count.");
    auto received=row["Received"].get<i64>();
    if(received<num(job->data,"Received")||(num(job->data,"Size",-1)>=0&&received>num(job->data,"Size")))throw std::runtime_error("Inconsistent response byte count.");
    accountOriginalResponseBytes(job,received);job->data["Received"]=received;job->data["TransferredBytes"]=received;
    auto seconds=real(row,"TransferSeconds");if(std::isfinite(seconds)&&seconds>=0){job->data["TransferSeconds"]=seconds;job->data["ElapsedSeconds"]=seconds;}
    if(str(row,"State")=="Receiving"&&!yes(record,"PauseRequested")){
     if(yes(record,"FlowControlled")){auto total=scheduledOriginalResponseCredits(job,64*1024*1024);monitor.grantResponse(capture,record.at("OfferId").get<uint64_t>(),total);}
     else if(num(state["Settings"],"QuotaMb")>0||num(state["Settings"],"LimitKbps")>0||job->sessionLimit.value_or(num(job->data,"LimitKbps"))>0){pause(job);monitor.cancelResponse(capture,record.at("OfferId").get<uint64_t>());}
    }
   }
  }catch(const std::exception& e){result["Errors"].push_back({{"Id",job->id()},{"Message",e.what()}});}
 }
 // Handles above have closed. Recovery ignores a still-active broker writer.
 recoverOriginalResponses(background);
 for(auto job:jobs)if(!active.count(job->id())&&job->data.contains("OriginalResponse")&&
     str(job->data["OriginalResponse"],"State")=="Ready"&&!yes(job->data,"ResponseSaveDeferred")){
  try{
   // A short response can finish between the caller's snapshot and receipt recovery.
   // Fetch terminal timing before the completion event opens the average-rate dialog.
   auto terminal=monitor.snapshot().value("Capture",Json::object());const auto& ownership=job->data["OriginalResponse"];
   if(terminal.value("Session",Json())==ownership.value("Session",Json())&&terminal.value("Generation",Json())==ownership.value("Generation",Json()))
    for(const auto& row:terminal.value("Responses",Json::array()))if(row.value("Id",Json())==ownership.value("OfferId",Json())&&str(row,"State")=="Complete"){
     auto seconds=real(row,"TransferSeconds");if(std::isfinite(seconds)&&seconds>0){job->data["TransferSeconds"]=seconds;job->data["ElapsedSeconds"]=seconds;job->data["TransferredBytes"]=num(job->data,"Received");}
    }
   if(background)startOriginalResponseSave(job);
   else{publishOriginalResponse(job);result["Published"]=result["Published"].get<unsigned>()+1;}}
  catch(const std::exception& e){result["Errors"].push_back({{"Id",job->id()},{"Message",e.what()}});}
 }
 return result;
}
void Manager::publishOriginalResponse(JobPtr job,const std::function<void(const char*)>& checkpoint){
 publishOriginalResponseImpl(job,checkpoint,nullptr);
}
void Manager::publishOriginalResponseImpl(JobPtr job,const std::function<void(const char*)>& checkpoint,const Cancel* cancel){
 std::unique_lock lock(mutex);
 if(!job||std::find(jobs.begin(),jobs.end(),job)==jobs.end()||!job->data.contains("OriginalResponse"))throw std::runtime_error("Select a received original response.");
 // Reserved ownership may have finished before its acknowledgement arrived.
 // File locks and the complete durable receipt below must still prove publication.
 const auto owner=active.find(job->id());
 if((owner==active.end()?cancel!=nullptr:owner->second.get()!=cancel)||
    str(job->data["OriginalResponse"],"State")=="Receiving")throw std::runtime_error("Select a received original response.");
 auto initialOwnership=job->data["OriginalResponse"];if(str(initialOwnership,"State")=="Published")return;
 const bool recovering=str(initialOwnership,"State")=="Publishing";
 auto folder=root/L"responses"/wide(job->id());auto target=job->target();auto staging=target.parent_path()/(L".udm-response-"+wide(job->id())+L".part");
 const auto savedStage=fs::path(wide(str(initialOwnership,"StagePath")));
 if(!savedStage.empty()&&(!savedStage.is_absolute()||
    !std::regex_match(savedStage.filename().wstring(),std::wregex(L"\\.udm-response-"+wide(job->id())+L"-[0-9a-f]{32}\\.part"))||
    (recovering&&savedStage.parent_path()!=target.parent_path())))
  throw std::runtime_error("The saved response staging path is invalid.");
 if(!savedStage.empty())staging=savedStage;
 const auto expected=lower(str(job->data,"ExpectedSha256"));
 if(cancel)lock.unlock();
 if(!recovering&&fs::exists(target))throw std::runtime_error("The destination already exists. Choose another location in File Properties, then resume the saved response.");
 Handle body(CreateFileW((folder/L"body").c_str(),GENERIC_READ,FILE_SHARE_READ,nullptr,OPEN_EXISTING,0,nullptr));
 Handle receipt(CreateFileW((folder/L"receipt").c_str(),GENERIC_READ,FILE_SHARE_READ,nullptr,OPEN_EXISTING,0,nullptr));
 if(!body||!receipt)throw std::runtime_error("The original response is unavailable or still being written.");
 auto recovered=udmnet::DurableResponseSink::recover(body.h,receipt.h);if(!recovered.complete)throw std::runtime_error("The original response is incomplete.");
 auto checkCancel=[&]{if(cancel)cancel->check();};
 const auto total=static_cast<i64>(recovered.received);
 auto reportSave=[&](ResponseSaveProgress::Phase phase,i64 copied=0){
  if(cancel){Lock progressLock(mutex);job->responseSaveProgress=ResponseSaveProgress{phase,copied,total};}
 };
 reportSave(ResponseSaveProgress::Phase::VerifyingResponse);
 auto hash=fileHash(body.h,checkCancel);if(!expected.empty()&&expected!=hash)throw std::runtime_error("SHA-256 mismatch. Original response was not published.");
 auto verify=[&](const fs::path& path){
  Handle file(CreateFileW(path.c_str(),GENERIC_READ,FILE_SHARE_READ|FILE_SHARE_DELETE,nullptr,
                          OPEN_EXISTING,FILE_FLAG_OPEN_REPARSE_POINT|FILE_FLAG_SEQUENTIAL_SCAN,nullptr));
  BY_HANDLE_FILE_INFORMATION info{};
  if(!file||!GetFileInformationByHandle(file.h,&info)||(info.dwFileAttributes&(FILE_ATTRIBUTE_DIRECTORY|FILE_ATTRIBUTE_REPARSE_POINT))||
     fileHash(file.h,checkCancel)!=hash)throw std::runtime_error("Original response publication file is missing or changed.");
  return file;
 };
 auto stagingIdentity=[](HANDLE file){
  auto identity=stableFileIdentity(file);
  if(identity.is_object()){identity.erase("Size");identity.erase("Written");}
  return identity;
 };
 auto openVerifiedStaging=[&](const Handle& verified,DWORD access){
  Handle current(CreateFileW(staging.c_str(),access,FILE_SHARE_READ|FILE_SHARE_DELETE,nullptr,
                             OPEN_EXISTING,FILE_FLAG_OPEN_REPARSE_POINT,nullptr));
  BY_HANDLE_FILE_INFORMATION before{},now{};
  if(!current||!GetFileInformationByHandle(verified.h,&before)||!GetFileInformationByHandle(current.h,&now)||
     before.dwVolumeSerialNumber!=now.dwVolumeSerialNumber||before.nFileIndexHigh!=now.nFileIndexHigh||
     before.nFileIndexLow!=now.nFileIndexLow)return Handle{};
  return current;
 };
 auto releaseConflictingDestination=[&](const Handle& verified){
  // The verified staging file is still present, so this intent has not
  // consumed it. Keep the private response body as the source for Resume.
  if(!openVerifiedStaging(verified,FILE_READ_ATTRIBUTES))throw std::runtime_error("The saved response needs publication recovery before changing its destination.");
  const char* message="The destination is occupied. Choose another location in File Properties, then resume the saved response.";
  auto before=job->data;
  auto& ownership=job->data["OriginalResponse"];
  ownership["State"]="Ready";ownership.erase("Hash");ownership.erase("Target");
  ownership.erase("StagePath");ownership.erase("StageFile");
  job->data["ResponseSaveDeferred"]=true;job->data["Status"]="Failed";job->data["Error"]=message;
  try{save();}catch(...){job->data.swap(before);throw;}
  auto cleanup=openVerifiedStaging(verified,FILE_READ_ATTRIBUTES|DELETE);
  if(cleanup){FILE_DISPOSITION_INFO disposition{TRUE};SetFileInformationByHandle(cleanup.h,FileDispositionInfo,&disposition,sizeof(disposition));}
  throw std::runtime_error(message);
 };
 auto publishStaging=[&](const Handle& verified){
  if(!openVerifiedStaging(verified,FILE_READ_ATTRIBUTES))throw std::runtime_error("The staged response changed before the final save.");
  if(MoveFileExW(staging.c_str(),target.c_str(),MOVEFILE_WRITE_THROUGH))return;
  std::error_code error;
  if(fs::exists(target,error))releaseConflictingDestination(verified);
  throw std::runtime_error("Cannot publish original response; no destination was overwritten.");
 };
 if(!recovering){
  fs::create_directories(target.parent_path());
  bool created=false;
  bool reserved=false;
  std::optional<Handle> verified;
  try{
   checkCancel();
   {
    Handle staged;
    const auto savedIdentity=initialOwnership.value("StageFile",Json());
    if(!savedStage.empty()&&savedStage.parent_path()==target.parent_path()&&savedIdentity.is_object()){
     staged.h=CreateFileW(savedStage.c_str(),GENERIC_READ|GENERIC_WRITE,0,nullptr,OPEN_EXISTING,
                          FILE_FLAG_OPEN_REPARSE_POINT|FILE_FLAG_SEQUENTIAL_SCAN,nullptr);
     if(staged&&stagingIdentity(staged.h)!=savedIdentity){CloseHandle(staged.h);staged.h=INVALID_HANDLE_VALUE;}
     reserved=bool(staged);
    }
    if(!staged){
     staging=target.parent_path()/(L".udm-response-"+wide(job->id())+L"-"+wide(guid())+L".part");
     staged.h=CreateFileW(staging.c_str(),GENERIC_READ|GENERIC_WRITE,0,nullptr,CREATE_NEW,FILE_FLAG_SEQUENTIAL_SCAN,nullptr);
     if(!staged)throw std::runtime_error("Cannot stage the received original response.");
     created=true;
     const auto identity=stagingIdentity(staged.h);
     if(identity.is_object()){
      if(cancel)lock.lock();
      checkCancel();
      if(job->target()!=target||job->data.at("OriginalResponse")!=initialOwnership)throw std::runtime_error("Original response publication changed before saving.");
      auto before=job->data;
      auto& ownership=job->data["OriginalResponse"];
      ownership["StagePath"]=utf8(staging.wstring());ownership["StageFile"]=identity;
      try{save();}catch(...){job->data.swap(before);throw;}
      reserved=true;
      initialOwnership=ownership;
      if(cancel)lock.unlock();
      if(!savedStage.empty()&&savedStage!=staging&&savedIdentity.is_object()){
       Handle oldStage(CreateFileW(savedStage.c_str(),FILE_READ_ATTRIBUTES|DELETE,FILE_SHARE_READ|FILE_SHARE_DELETE,
                                   nullptr,OPEN_EXISTING,FILE_FLAG_OPEN_REPARSE_POINT,nullptr));
       if(oldStage&&stagingIdentity(oldStage.h)==savedIdentity){
        FILE_DISPOSITION_INFO disposition{TRUE};
        SetFileInformationByHandle(oldStage.h,FileDispositionInfo,&disposition,sizeof(disposition));
       }
      }
     }
    }
    LARGE_INTEGER start{};
    if(!SetFilePointerEx(body.h,start,nullptr,FILE_BEGIN)||!SetFilePointerEx(staged.h,start,nullptr,FILE_BEGIN)||!SetEndOfFile(staged.h))
     throw std::runtime_error("Cannot reset the original response staging file.");
    reportSave(ResponseSaveProgress::Phase::Copying);
    auto lastUpdate=GetTickCount64();
    i64 copied=0;
    std::vector<BYTE> buffer(1024*1024);
    for(;;){
     checkCancel();
     DWORD count=0,written=0;
     if(!ReadFile(body.h,buffer.data(),static_cast<DWORD>(buffer.size()),&count,nullptr))throw std::runtime_error("Cannot read the received original response.");
     if(!count)break;
     if(!WriteFile(staged.h,buffer.data(),count,&written,nullptr)||written!=count)throw std::runtime_error("Cannot write the staged response.");
     copied+=written;
     const auto now=GetTickCount64();
     if(now-lastUpdate>=100){reportSave(ResponseSaveProgress::Phase::Copying,copied);lastUpdate=now;}
    }
    reportSave(ResponseSaveProgress::Phase::Copying,copied);
    if(!FlushFileBuffers(staged.h))throw std::runtime_error("Cannot flush the staged response.");
   }
   reportSave(ResponseSaveProgress::Phase::VerifyingCopy);
   verified.emplace(verify(staging));
   if(cancel)lock.lock();
   checkCancel();
   if(job->target()!=target||job->data.at("OriginalResponse")!=initialOwnership)throw std::runtime_error("Original response publication changed before saving.");
   auto before=job->data;
   auto& ownership=job->data["OriginalResponse"];ownership["State"]="Publishing";ownership["Hash"]=hash;ownership["Target"]=utf8(target.wstring());
   ownership["StagePath"]=utf8(staging.wstring());
   if(!reserved)ownership.erase("StageFile");
   try{save();}catch(...){job->data=before;throw;}
  }catch(...){verified.reset();if(created&&!reserved){std::error_code ec;fs::remove(staging,ec);}throw;}
  if(checkpoint)checkpoint("publication-prepared");
  reportSave(ResponseSaveProgress::Phase::Publishing);
  publishStaging(*verified);
 }else{
  if(str(initialOwnership,"Hash")!=hash||str(initialOwnership,"Target")!=utf8(target.wstring()))throw std::runtime_error("Original response publication identity changed; recovery files were preserved.");
  const bool published=fs::exists(target);
  const bool staged=fs::exists(staging);
  reportSave(ResponseSaveProgress::Phase::VerifyingCopy);
  auto verified=verify(staged?staging:target);
  if(cancel)lock.lock();
  checkCancel();
  if(job->target()!=target||job->data.at("OriginalResponse")!=initialOwnership)throw std::runtime_error("Original response publication changed before recovery.");
  if(checkpoint)checkpoint("publication-prepared");
  reportSave(ResponseSaveProgress::Phase::Publishing);
  if(staged&&published)releaseConflictingDestination(verified);
  if(staged)publishStaging(verified);
 }
 if(checkpoint)checkpoint("file-published");markZone(target);
 auto before=job->data;auto& ownership=job->data["OriginalResponse"];ownership["State"]="Published";ownership.erase("StagePath");ownership.erase("StageFile");job->data.erase("ResponseSaveDeferred");job->data["Sha256"]=hash;job->data["Size"]=recovered.received;job->data["Received"]=recovered.received;job->data["Status"]="Complete";job->data["Finished"]=date();job->data["Error"]="";job->data["QueueMember"]=false;
 prepareCompletionScan(job->data,state["Settings"]);
 if(recovering)job->data["CompletionActionArmed"]=false;
 try{save();}catch(...){job->data=before;throw;}
 job->speed=0;
 // Completion is durable and the destination has been verified. Release only
 // this job's duplicate body; retain the receipt and all failed/partial bodies.
 // A sharing/permission failure is harmless and must not fail the download.
 CloseHandle(body.h);body.h=INVALID_HANDLE_VALUE;
 DeleteFileW((folder/L"body").c_str());
 if(recovering||cancel)return;
 if(job->data.contains("ScanResult")){
  try{launchCompletedScan(job,ScanLaunch::CapturedDownload);}
  catch(const std::system_error&){/* The saved pending check can run on a later tick. */}
 }
 else if(event)event(job,true);
}
// Prepare a verified range seed while the manager lock protects the record.
void Manager::stageOriginalResponseResume(const JobPtr& job,Json& next){
 if(std::find(jobs.begin(),jobs.end(),job)==jobs.end()||isActive(job))throw std::runtime_error("Wait for the captured response to stop before resuming.");
 const auto response=next.find("OriginalResponse");
 if(response==next.end()||str(*response,"State")!="Interrupted")throw std::runtime_error("This captured response is not an interrupted download.");
 auto length=num(next,"Size",-1);
 if(length<=0||(str(next,"ETag").empty()&&str(next,"Modified").empty()))throw std::runtime_error("The captured response has no reliable file identity for resume. Refresh its address or add a new download.");
 auto folder=root/L"responses"/wide(job->id());
 Handle body(CreateFileW((folder/L"body").c_str(),GENERIC_READ,FILE_SHARE_READ,nullptr,OPEN_EXISTING,0,nullptr));
 Handle receipt(CreateFileW((folder/L"receipt").c_str(),GENERIC_READ,FILE_SHARE_READ,nullptr,OPEN_EXISTING,0,nullptr));
 if(!body||!receipt)throw std::runtime_error("The original response is unavailable or still being written.");
 auto received=udmnet::DurableResponseSink::recover(body.h,receipt.h);
 if(received.complete||!received.received||received.received>=static_cast<uint64_t>(length))throw std::runtime_error("The captured partial file cannot be continued as a byte range.");
 // Retain the original body until continuation has been independently verified.
 // Reusing a pre-commit staging file is allowed only when its bytes match.
 auto parts=folder/L"continuation";fs::create_directories(parts);auto part=parts/L"0000.part";
 auto hash=fileHash(folder/L"body");
 if(!fs::exists(part)){
  auto staging=parts/(L".seed-"+wide(guid())+L".tmp");bool created=false;
  try{
   {Handle output(CreateFileW(staging.c_str(),GENERIC_WRITE,0,nullptr,CREATE_NEW,0,nullptr));
    if(!output)throw std::runtime_error("Cannot prepare captured bytes for resume.");created=true;
    LARGE_INTEGER beginning{};if(!SetFilePointerEx(body.h,beginning,nullptr,FILE_BEGIN))throw std::runtime_error("Cannot seek captured resume data.");
    std::vector<char> buffer(65536);DWORD count=0;
    for(;;){if(!ReadFile(body.h,buffer.data(),static_cast<DWORD>(buffer.size()),&count,nullptr))throw std::runtime_error("Cannot read captured resume data.");if(!count)break;
     for(DWORD offset=0;offset<count;){DWORD written=0;if(!WriteFile(output.h,buffer.data()+offset,count-offset,&written,nullptr)||!written)throw std::runtime_error("Cannot write captured resume data.");offset+=written;}}
    if(!FlushFileBuffers(output.h))throw std::runtime_error("Cannot flush captured resume data.");}
   if(fileHash(staging)!=hash)throw std::runtime_error("Captured resume staging failed verification.");
   if(!MoveFileExW(staging.c_str(),part.c_str(),MOVEFILE_WRITE_THROUGH))throw std::runtime_error("Cannot publish captured resume staging; existing data was preserved.");
  }catch(...){if(created){std::error_code error;fs::remove(staging,error);}throw;}
 }
 if(fileHash(part)!=hash)throw std::runtime_error("Resume staging differs from the original response; both files were preserved.");
 {Handle staged(CreateFileW(part.c_str(),GENERIC_READ|GENERIC_WRITE,0,nullptr,OPEN_EXISTING,0,nullptr));if(!staged||!FlushFileBuffers(staged.h))throw std::runtime_error("Cannot flush captured resume data.");}
 next["OriginalResponseHistory"]=*response;next.erase("OriginalResponse");
 next["PartsFolder"]=utf8(parts.wstring());next["Segments"]=Json::array({{{"Index",0},{"Start",0},{"End",length-1},{"Done",received.received}}});
 next["Received"]=received.received;next["PreserveResumeIdentity"]=true;next["RefreshPendingValidation"]=true;
}
void Manager::startOriginalResponseSave(JobPtr job,bool startupRecovery,bool persistStart){
 const auto id=job->id();
 if(stopping||active.count(id)){
  if(persistStart)throw std::runtime_error("Wait until this download can start, then retry.");
  return;
 }
 const bool recovering=startupRecovery||str(job->data.at("OriginalResponse"),"State")=="Publishing";
 const bool deferred=yes(job->data,"ResponseSaveDeferred");
 auto cancel=std::make_shared<Cancel>();
 auto before=job->data;
 std::promise<bool> admission;
 auto admitted=admission.get_future();
 bool launched=false;
 active.emplace(id,cancel);
 try{
  if(deferred){
   job->data.erase("ResponseSaveDeferred");
   job->data["OriginalResponse"].erase("DeferCompletedSave");
  }
  job->data["Status"]="Verifying";job->data["Error"]="";
  if(recovering)job->data["CompletionActionArmed"]=false;
  threads.emplace_back([this,job,cancel,recovering,admitted=std::move(admitted)]() mutable{
   // Wait outside the manager lock so a rejected start can join this worker.
   if(!admitted.get())return;
   bool published=false;
   try{
    publishOriginalResponseImpl(job,{},cancel.get());
    published=true;
    if(!recovering)scanCompleted(job,*cancel);
   }catch(const std::exception& error){
    Lock lock(mutex);
    if(!published){
     if(!cancel->cancelled())job->data["ResponseSaveDeferred"]=true;
     job->data["Status"]=cancel->cancelled()?"Paused":"Failed";
     job->data["Error"]=cancel->cancelled()?"":error.what();
    }else{
     job->data["ScanResult"]={{"Status","Failed"},{"Message",error.what()}};
     job->data["CompletionActionArmed"]=false;
    }
   }
   std::function<void(JobPtr,bool)> notify;
   {Lock lock(mutex);active.erase(job->id());schedulePaused.erase(job->id());job->speed=0;job->responseSaveProgress.reset();
    if(published&&!recovering&&!stopping)notify=event;
    try{save();}catch(...) {}
   }
   if(notify)notify(job,true);
  });
  launched=true;
  if(deferred||persistStart)save();
 }catch(...){
  active.erase(id);job->data.swap(before);
  if(launched){
   admission.set_value(false);
   threads.back().join();
   threads.pop_back();
  }
  throw;
 }
 admission.set_value(true);
}
void Manager::recoverDesktopOriginalResponses(){
 if(!startupResponseRecoveryPending||catalogTransaction)return;
 recoverOriginalResponses(true);
 for(const auto& job:jobs){
  const auto response=job->data.find("OriginalResponse");
  if(response==job->data.end()||yes(job->data,"ResponseSaveDeferred")||active.count(job->id()))continue;
  const auto phase=str(*response,"State");
  if(phase=="Ready"||phase=="Publishing")startOriginalResponseSave(job,true);
 }
 startupResponseRecoveryPending=false;
}
void Manager::recoverOriginalResponses(bool background){
 Lock lock(mutex);bool changed=false;
 for(auto job:jobs){if(active.count(job->id())||!job->data.contains("OriginalResponse")||
     str(job->data["OriginalResponse"],"State")=="Published"||yes(job->data,"ResponseSaveDeferred"))continue;
  if(str(job->data["OriginalResponse"],"State")=="Publishing"){
   try{if(background)startOriginalResponseSave(job);else publishOriginalResponse(job);}catch(const std::exception& e){job->speed=0;job->data["Status"]="Paused";job->data["QueueMember"]=false;job->data["Error"]=e.what();changed=true;}continue;
  }
  auto folder=root/L"responses"/wide(job->id());
  auto before=job->data;
  Handle body(CreateFileW((folder/L"body").c_str(),GENERIC_READ,FILE_SHARE_READ,nullptr,OPEN_EXISTING,0,nullptr));auto bodyError=GetLastError();
  Handle receipt(CreateFileW((folder/L"receipt").c_str(),GENERIC_READ,FILE_SHARE_READ,nullptr,OPEN_EXISTING,0,nullptr));
  // An active writer owns these files. Do not read or reposition its handles.
  auto receiptError=GetLastError();
  if(!body||!receipt){
   if((!body&&bodyError==ERROR_SHARING_VIOLATION)||(!receipt&&receiptError==ERROR_SHARING_VIOLATION))continue;
   LARGE_INTEGER received{};if(body&&GetFileSizeEx(body.h,&received))job->data["Received"]=received.QuadPart;
   job->data["OriginalResponse"]["State"]="Recovery required";job->data["Error"]="Original response body or receipt is unavailable; saved history was preserved.";
   job->speed=0;job->data["Status"]="Paused";job->data["QueueMember"]=false;changed|=before!=job->data;continue;
  }
  try{LARGE_INTEGER received{};if(!GetFileSizeEx(body.h,&received))throw std::runtime_error("Cannot inspect the received body.");job->data["Received"]=received.QuadPart;LARGE_INTEGER size{};if(!GetFileSizeEx(receipt.h,&size))throw std::runtime_error("Cannot inspect the response receipt.");
   if(size.QuadPart==0){if(received.QuadPart)throw std::runtime_error("Received bytes have no ownership receipt; files were preserved.");job->data["OriginalResponse"]["State"]="Unaccepted";job->data["Received"]=0;job->data["Error"]="The browser response was not accepted; its reserved files were preserved.";}
   else{
    auto recovered=udmnet::DurableResponseSink::recover(body.h,receipt.h);
    job->data["Received"]=recovered.received;job->data["OriginalResponse"]["State"]=recovered.complete?"Ready":"Interrupted";
    const bool deferred=recovered.complete&&yes(job->data["OriginalResponse"],"DeferCompletedSave");
    if(deferred)job->data["ResponseSaveDeferred"]=true;
    job->data["Error"]=deferred?"Save paused. Resume to save the received response.":
                       recovered.complete?"Original response received and ready for publication.":"Original response interrupted; received bytes were preserved.";
    accountOriginalResponseBytes(job,static_cast<i64>(recovered.received));
   }
  }catch(const std::exception& e){job->data["OriginalResponse"]["State"]="Recovery required";job->data["Error"]=e.what();}
  job->speed=0;job->data["Status"]="Paused";job->data["QueueMember"]=false;changed|=before!=job->data;
 }
 if(changed)save();
}

}
