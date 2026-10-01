#include "CaptureTransactions.hpp"
namespace udm {
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
}
