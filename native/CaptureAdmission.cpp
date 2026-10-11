#include "CaptureTransactions.hpp"
#include "OriginalResponsePolicy.hpp"
#include "../drivers/signed-network/DurableResponseSink.hpp"
#include "AdaptiveCapture.hpp"
#include "MediaAdmission.hpp"
namespace udm {
JobPtr Manager::receiveBrowserMedia(const Json& message,const std::function<void(const char*)>& checkpoint){
 Lock lock(mutex);auto token=str(message,"mediaToken");auto created=captureTime(token),now=epoch();
 if(str(message,"action")!="adaptive"||message.contains("captureToken"))throw std::runtime_error("Media receipts require an adaptive download.");
 auto request=message;request.erase("mediaToken");request.erase("requestId");auto fingerprint=mediaAdmissionDigest(request);auto rows=captureStore(*this);
 if(rows.contains(token)){
  const auto& row=rows[token];
  if(num(row,"protocol")!=3||str(row,"status")!="accepted")throw std::runtime_error("This media operation was released or belongs to another request. Check its receipt.");
  if(reveal(str(row,"fingerprint"))!=fingerprint)throw std::runtime_error("The accepted media selection changed. Do not reuse its token.");
  for(auto job:jobs)if(job->id()==str(row,"id"))return job;
  throw std::runtime_error("This media download was already accepted and its history record was later removed.");
 }
 if(created<now-600000||created>now+60000)throw std::runtime_error("Media operation token has expired.");
 validateAdaptiveCapture(request);captureSpace(rows,now);
 if(catalogTransaction)throw std::runtime_error("Another catalog operation is in progress.");
 const auto protectedFingerprint=protect(fingerprint);
 auto previousState=state;auto previousJobs=jobs;auto previousOffers=pendingOffers;auto previousExisting=existingOffers;
 std::vector<std::pair<JobPtr,Json>> before;for(auto job:jobs)before.push_back({job,job->data});
 auto oldRefresh=refreshId;auto oldRefreshUntil=refreshUntil;JobPtr admitted;
 catalogCheckpoint=checkpoint;catalogTransaction=true;
 try{
  admitted=receive(request);
  bool refreshed=false;for(const auto& old:before)if(old.first==admitted)refreshed=str(old.second,"ProtectedRefreshOffer")!=str(admitted->data,"ProtectedRefreshOffer");
  Json receipt={{"created",created},{"protocol",3},{"status","accepted"},{"id",admitted->id()},{"fingerprint",protectedFingerprint}};
  if(refreshed||str(admitted->data,"Status")=="Awaiting confirmation")receipt["presentation"]={{"id",admitted->id()},{"kind",refreshed?"refresh":"download"}};
  rows[token]=receipt;state["BrowserCaptures"]=rows;
  if(checkpoint)checkpoint("before-commit");
  catalogTransaction=false;save();
 }catch(...){
  catalogTransaction=false;catalogCheckpoint={};state.swap(previousState);jobs.swap(previousJobs);pendingOffers.swap(previousOffers);existingOffers.swap(previousExisting);
  for(auto& old:before)old.first->data.swap(old.second);
  refreshId=oldRefresh;refreshUntil=oldRefreshUntil;throw;
 }
 catalogCheckpoint={};
 // Failure here models a lost acknowledgement; the committed job must survive.
 if(checkpoint)checkpoint("committed");
 return admitted;
}

Json Manager::pendingBrowserPresentations()const{
 Lock lock(mutex);auto rows=captureStore(*this);Json offers=Json::array();std::set<std::string> known;std::map<std::string,size_t> groups;
 for(auto job:jobs)known.insert(job->id());
 for(auto it=rows.begin();it!=rows.end();++it){
  const auto& row=it.value();if(str(row,"status")!="accepted"||!row.contains("presentation"))continue;
  const auto& presentation=row.at("presentation");auto id=str(presentation,"id"),kind=str(presentation,"kind");
  if(kind!="download"&&kind!="existing"&&kind!="refresh")throw std::runtime_error("Invalid saved browser presentation. The download history was preserved.");
  if(!known.count(id))continue;
  auto inserted=groups.emplace(id,offers.size());if(inserted.second)offers.push_back({{"id",id},{"kind",kind},{"tokens",Json::array()}});
  auto combined=&offers.at(inserted.first->second);
  combined->at("tokens").push_back(it.key());
  if(kind=="refresh"||(kind=="existing"&&str(*combined,"kind")!="refresh"))(*combined)["kind"]=kind;
 }
 return offers;
}
bool Manager::hasBrowserPresentation(JobPtr job)const{
 Lock lock(mutex);for(const auto& row:captureStore(*this))if(str(row,"status")=="accepted"&&str(row.value("presentation",Json::object()),"id")==job->id())return true;return false;
}
JobPtr Manager::restoreBrowserPresentation(const Json& requested){
 Lock lock(mutex);auto current=pendingBrowserPresentations();
 for(const auto& offer:current)if(offer==requested){for(auto job:jobs)if(job->id()==str(offer,"id")){
  if(str(offer,"kind")=="existing")existingOffers.insert(job->id());return job;
 }}return {};
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
  if(!canRefreshAddress(target)||!str(target->data,"SourceUrl").empty()||!str(target->data,"ProtectedAdaptive").empty()||str(target->data,"Url")!=str(context,"url")||str(target->data,"FileName")!=str(context,"filename")){
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
 bool eligible=canRefreshAddress(target)&&str(target->data,"SourceUrl").empty()&&str(target->data,"ProtectedAdaptive").empty()&&request.value("request",Json::object()).empty();
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
void Manager::stopOriginalResponses(Monitor& monitor){
 Lock lock(mutex);
 // Cancel owned responses while the redirect path still exists, allowing the
 // relay to close the browser side before removing its network filters.
 Json snapshot=Json::object();
 // A helper that has already exited cannot acknowledge cancellation. Its file
 // handles are closed, so recover the durable receipt without requiring IPC.
 if(monitor.running())try{snapshot=monitor.snapshot();}catch(...){if(monitor.running())throw;}
 auto capture=snapshot.value("Capture",Json::object());
 for(auto job:jobs)if(job->data.contains("OriginalResponse")){
  auto& record=job->data["OriginalResponse"];
  if(str(record,"State")=="Receiving"&&record.value("Session",Json())==capture.value("Session",Json())&&record.value("Generation",Json())==capture.value("Generation",Json())){
   for(const auto& row:capture.value("Responses",Json::array()))if(row.value("Id",Json())==record.at("OfferId")){
    auto seconds=real(row,"TransferSeconds");if(std::isfinite(seconds)&&seconds>0){job->data["TransferSeconds"]=seconds;job->data["ElapsedSeconds"]=seconds;}
    if(row.contains("Received")&&row["Received"].is_number_unsigned()&&row["Received"].get<uint64_t>()<=static_cast<uint64_t>(INT64_MAX))job->data["TransferredBytes"]=row["Received"];
   }
   pause(job);monitor.cancelResponse(capture,record.at("OfferId").get<uint64_t>());
  }
 }
 auto deadline=GetTickCount64()+3000;
 for(;;){recoverOriginalResponses();bool receiving=false;for(auto job:jobs)receiving|=job->data.contains("OriginalResponse")&&str(job->data["OriginalResponse"],"State")=="Receiving";
  if(!receiving||GetTickCount64()>=deadline)break;Sleep(10);
 }
 monitor.watch({});
 recoverOriginalResponses();
 for(auto job:jobs)if(job->data.contains("OriginalResponse"))job->speed=0;
 save();
}
Json Manager::processOriginalResponses(Monitor& monitor,const Json& capture){
 Lock lock(mutex);
 return processOriginalResponses(monitor,capture,[&](const Json& offer){auto browser=originalResponseProcess(offer);return originalResponseBrowser(browser)&&originalResponseAllowed(state["Settings"],browser,offer);});
}
Json Manager::processOriginalResponses(Monitor& monitor,const Json& capture,const std::string& browser){
 Lock lock(mutex);
 return processOriginalResponses(monitor,capture,[&](const Json& offer){return originalResponseAllowed(state["Settings"],browser,offer);});
}
Json Manager::processOriginalResponses(Monitor& monitor,const Json& capture,const std::function<bool(const Json&)>& policy){
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
  if(str(record,"State")!="Receiving"||record.value("Session",Json())!=capture.value("Session",Json())||record.value("Generation",Json())!=capture.value("Generation",Json()))continue;
  try{
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
 recoverOriginalResponses();
 for(auto job:jobs)if(job->data.contains("OriginalResponse")&&str(job->data["OriginalResponse"],"State")=="Ready"){
  try{
   // A short response can finish between the caller's snapshot and receipt recovery.
   // Fetch terminal timing before the completion event opens the average-rate dialog.
   auto terminal=monitor.snapshot().value("Capture",Json::object());const auto& ownership=job->data["OriginalResponse"];
   if(terminal.value("Session",Json())==ownership.value("Session",Json())&&terminal.value("Generation",Json())==ownership.value("Generation",Json()))
    for(const auto& row:terminal.value("Responses",Json::array()))if(row.value("Id",Json())==ownership.value("OfferId",Json())&&str(row,"State")=="Complete"){
     auto seconds=real(row,"TransferSeconds");if(std::isfinite(seconds)&&seconds>0){job->data["TransferSeconds"]=seconds;job->data["ElapsedSeconds"]=seconds;job->data["TransferredBytes"]=num(job->data,"Received");}
    }
   publishOriginalResponse(job);result["Published"]=result["Published"].get<unsigned>()+1;}
  catch(const std::exception& e){result["Errors"].push_back({{"Id",job->id()},{"Message",e.what()}});}
 }
 return result;
}
void Manager::publishOriginalResponse(JobPtr job,const std::function<void(const char*)>& checkpoint){
 Lock lock(mutex);
 if(!job||std::find(jobs.begin(),jobs.end(),job)==jobs.end()||isActive(job)||!job->data.contains("OriginalResponse"))throw std::runtime_error("Select a received original response.");
 auto& ownership=job->data["OriginalResponse"];if(str(ownership,"State")=="Published")return;
 auto folder=root/L"responses"/wide(job->id());auto target=job->target();auto staging=target.parent_path()/(L".udm-response-"+wide(job->id())+L".part");
 Handle body(CreateFileW((folder/L"body").c_str(),GENERIC_READ,FILE_SHARE_READ,nullptr,OPEN_EXISTING,0,nullptr));
 Handle receipt(CreateFileW((folder/L"receipt").c_str(),GENERIC_READ,FILE_SHARE_READ,nullptr,OPEN_EXISTING,0,nullptr));
 if(!body||!receipt)throw std::runtime_error("The original response is unavailable or still being written.");
 auto recovered=udmnet::DurableResponseSink::recover(body.h,receipt.h);if(!recovered.complete)throw std::runtime_error("The original response is incomplete.");
 auto hash=fileHash(folder/L"body");auto expected=lower(str(job->data,"ExpectedSha256"));if(!expected.empty()&&expected!=hash)throw std::runtime_error("SHA-256 mismatch. Original response was not published.");
 if(str(ownership,"State")!="Publishing"){
  if(fs::exists(target))throw std::runtime_error("The destination already exists; no file was overwritten.");
  fs::create_directories(target.parent_path());
  if(!CopyFileW((folder/L"body").c_str(),staging.c_str(),TRUE))throw std::runtime_error("Cannot stage the received original response.");
  try{Handle staged(CreateFileW(staging.c_str(),GENERIC_READ|GENERIC_WRITE,0,nullptr,OPEN_EXISTING,0,nullptr));if(!staged||!FlushFileBuffers(staged.h))throw std::runtime_error("Cannot flush the staged response.");}catch(...){std::error_code ec;fs::remove(staging,ec);throw;}
  if(fileHash(staging)!=hash)throw std::runtime_error("Staged original response failed verification.");
  auto before=job->data;ownership["State"]="Publishing";ownership["Hash"]=hash;ownership["Target"]=utf8(target.wstring());
  try{save();}catch(...){job->data=before;std::error_code ec;fs::remove(staging,ec);throw;}
 }
 if(str(ownership,"Hash")!=hash||str(ownership,"Target")!=utf8(target.wstring()))throw std::runtime_error("Original response publication identity changed; recovery files were preserved.");
 if(checkpoint)checkpoint("publication-prepared");
 if(fs::exists(target)){
  if(fs::exists(staging)||fileHash(target)!=hash)throw std::runtime_error("Publication destination is occupied or changed; recovery files were preserved.");
 }else{
  if(!fs::is_regular_file(staging)||fileHash(staging)!=hash)throw std::runtime_error("Original response publication staging is missing or changed.");
  if(!MoveFileExW(staging.c_str(),target.c_str(),MOVEFILE_WRITE_THROUGH))throw std::runtime_error("Cannot publish original response; no destination was overwritten.");
 }
 if(checkpoint)checkpoint("file-published");markZone(target);
 auto before=job->data;ownership["State"]="Published";job->data["Sha256"]=hash;job->data["Size"]=recovered.received;job->data["Received"]=recovered.received;job->data["Status"]="Complete";job->data["Finished"]=date();job->data["Error"]="";job->data["QueueMember"]=false;
 if(!str(state["Settings"],"ScanProgram").empty())job->data["ScanResult"]={{"Status","Running"},{"Message","Original response awaits scanning."}};
 try{save();}catch(...){job->data=before;throw;}
 job->speed=0;
 // Completion is durable and the destination has been verified. Release only
 // this job's duplicate body; retain the receipt and all failed/partial bodies.
 // A sharing/permission failure is harmless and must not fail the download.
 CloseHandle(body.h);body.h=INVALID_HANDLE_VALUE;
 DeleteFileW((folder/L"body").c_str());
 Cancel cancel;scanCompleted(job,cancel);
 if(event)event(job,true);
}
void Manager::prepareOriginalResponseResume(JobPtr job){
 Lock lock(mutex);
 if(!job||std::find(jobs.begin(),jobs.end(),job)==jobs.end()||isActive(job))throw std::runtime_error("Wait for the captured response to stop before resuming.");
 recoverOriginalResponses();
 if(!job->data.contains("OriginalResponse")||str(job->data["OriginalResponse"],"State")!="Interrupted")throw std::runtime_error("This captured response is not an interrupted download.");
 auto length=num(job->data,"Size",-1);
 if(length<=0||(str(job->data,"ETag").empty()&&str(job->data,"Modified").empty()))throw std::runtime_error("The captured response has no reliable file identity for resume. Refresh its address or add a new download.");
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
 auto before=job->data;
 job->data["OriginalResponseHistory"]=job->data["OriginalResponse"];job->data.erase("OriginalResponse");
 job->data["PartsFolder"]=utf8(parts.wstring());job->data["Segments"]=Json::array({{{"Index",0},{"Start",0},{"End",length-1},{"Done",received.received}}});
 job->data["Received"]=received.received;job->data["PreserveResumeIdentity"]=true;job->data["RefreshPendingValidation"]=true;
 try{save();}catch(...){job->data=before;throw;}
}
void Manager::recoverOriginalResponses(){
 Lock lock(mutex);bool changed=false;
 for(auto job:jobs){if(!job->data.contains("OriginalResponse")||str(job->data["OriginalResponse"],"State")=="Published")continue;
  if(str(job->data["OriginalResponse"],"State")=="Publishing"){
   try{publishOriginalResponse(job);}catch(const std::exception& e){job->speed=0;job->data["Status"]="Paused";job->data["QueueMember"]=false;job->data["Error"]=e.what();changed=true;}continue;
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
   else{auto recovered=udmnet::DurableResponseSink::recover(body.h,receipt.h);job->data["Received"]=recovered.received;job->data["OriginalResponse"]["State"]=recovered.complete?"Ready":"Interrupted";job->data["Error"]=recovered.complete?"Original response received and ready for publication.":"Original response interrupted; received bytes were preserved.";accountOriginalResponseBytes(job,static_cast<i64>(recovered.received));}
  }catch(const std::exception& e){job->data["OriginalResponse"]["State"]="Recovery required";job->data["Error"]=e.what();}
  job->speed=0;job->data["Status"]="Paused";job->data["QueueMember"]=false;changed|=before!=job->data;
 }
 if(changed)save();
}

}
