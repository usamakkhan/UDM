#include "BrowserProxy.hpp"
#include "BrowserSession.hpp"
#include "Core.hpp"
#include "QueueMembership.hpp"
#include "QueueCompletion.hpp"
#include "QueueStop.hpp"
#include "QueueRetry.hpp"
#include "GuiModels.hpp"
#include "Ftp.hpp"
#include <algorithm>
#include <regex>
namespace udm {
static void validateQueue(const Json& q){if(q.contains("FileRetryLimitEnabled")&&!q["FileRetryLimitEnabled"].is_boolean())throw std::runtime_error("Retry limit must be enabled or disabled.");if(q.contains("DailyStopEnabled")&&!q["DailyStopEnabled"].is_boolean())throw std::runtime_error("Daily stop must be enabled or disabled.");
 if(q.contains("StopWithoutStartEnabled")&&!q["StopWithoutStartEnabled"].is_boolean())throw std::runtime_error("Stop time must be enabled or disabled.");
 if(q.contains("StopWithoutStartMinute")&&(!q["StopWithoutStartMinute"].is_number_integer()||num(q,"StopWithoutStartMinute")<0||num(q,"StopWithoutStartMinute")>1439))throw std::runtime_error("Stop time must be between 00:00 and 23:59.");
 if(q.contains("StartOnStartup")&&!q["StartOnStartup"].is_boolean())throw std::runtime_error("Start on UDM startup must be on or off.");
 if(q.contains("WakeComputer")&&!q["WakeComputer"].is_boolean())throw std::runtime_error("Wake computer must be on or off.");
 auto name=trim(str(q,"Name"));if(name.empty()||name!=str(q,"Name")||name.size()>80||num(q,"Parallel",2)<1||num(q,"Parallel",2)>16||num(q,"StartMinute")<0||num(q,"StartMinute")>1439||num(q,"StopMinute",1440)<0||num(q,"StopMinute",1440)>1440||num(q,"Days",127)<0||num(q,"Days",127)>127||num(q,"Retries")<0||num(q,"Retries")>10)throw std::runtime_error("Invalid queue settings.");
 if(yes(q,"Scheduled")&&!yes(q,"RunOnce")&&!num(q,"Days"))throw std::runtime_error("Choose at least one day.");
 if(yes(q,"RunOnce")){auto a=parseDate(q.value("StartOnceUtc",Json())),b=parseDate(q.value("StopOnceUtc",Json()));if(!a||(b&&b<=a))throw std::runtime_error("Choose a valid start and stop date.");}
 if(num(q,"FileRetries")<0||num(q,"FileRetries")>20||num(q,"RetryDelaySeconds",30)<1||num(q,"RetryDelaySeconds",30)>86400||num(q,"RepeatMinutes")<0||num(q,"RepeatMinutes")>10080)throw std::runtime_error("Use 0-20 file retries, 1-86400 seconds retry delay and 0-10080 minutes repeat interval.");
 auto steps=queueCompletionSteps(q);if(q.contains("FinishForce")&&!q["FinishForce"].is_boolean())throw std::runtime_error("Force termination must be on or off.");auto normalized=q;setQueueCompletionSteps(normalized,steps,yes(q,"FinishForce"));
 if(std::find(steps.begin(),steps.end(),"Open file")!=steps.end()&&(!fs::path(wide(str(q,"FinishFile"))).is_absolute()||!fs::is_regular_file(fs::path(wide(str(q,"FinishFile"))))))throw std::runtime_error("Choose an existing file to open after this queue completes.");
 if(num(q,"FinishDelaySeconds",30)<15||num(q,"FinishDelaySeconds",30)>3600)throw std::runtime_error("Completion countdown must be 15-3600 seconds.");
 if(num(q,"RepeatMinutes")&&yes(q,"RunOnce"))throw std::runtime_error("Repeating queues require a daily window or no time schedule.");
 if(num(q,"RepeatMinutes")&&!steps.empty())throw std::runtime_error("Choose either queue repetition or a completion action.");
}
void Manager::setQueues(const Json& input){
 Lock l(mutex);if(!input.is_array()||input.empty())throw std::runtime_error("Keep at least one queue.");auto next=input;std::set<std::string> names;
 for(auto& q:next){validateQueue(q);if(!names.insert(lower(str(q,"Name"))).second)throw std::runtime_error("That queue already exists.");for(auto current:state["Queues"])if(str(current,"Name")==str(q,"Name")){
  if(yes(current,"Synchronize")&&!yes(q,"Synchronize"))for(auto job:jobs)if(str(job->data,"Queue")==str(q,"Name")&&str(job->data,"Status")=="Complete"&&yes(job->data,"QueueMember"))throw std::runtime_error("Remove completed files from this queue before changing it to download mode.");
  bool rearm=yes(q,"RunOnce")&&(!yes(current,"RunOnce")||current.value("StartOnceUtc",Json())!=q.value("StartOnceUtc",Json())||current.value("StopOnceUtc",Json())!=q.value("StopOnceUtc",Json())||(!yes(current,"Enabled",true)&&yes(q,"Enabled",true)));
  q["OnceStarted"]=rearm?false:yes(current,"OnceStarted");
  q["NextRunUtc"]=num(q,"RepeatMinutes")==num(current,"RepeatMinutes")?current.value("NextRunUtc",Json()):Json();
 }}
 for(auto& q:next){Json previous=Json::object();for(const auto& old:state["Queues"])if(str(old,"Name")==str(q,"Name"))previous=old;auto beforeEvent=queueCompletionEvent(previous),afterEvent=queueCompletionEvent(q);beforeEvent.erase("Cycle");afterEvent.erase("Cycle");q["CompletionCycle"]=beforeEvent==afterEvent?str(previous,"CompletionCycle"):guid();q["CompletionCanceled"]=yes(previous,"CompletionCanceled")||(yes(previous,"Enabled",true)&&!yes(q,"Enabled",true));armIndependentStop(q,previous,epoch());armDailyManualStop(q,previous,epoch(),manualQueues.count(str(q,"Name"))!=0);q["DailyRunUntilComplete"]=yes(q,"Enabled",true)&&dailyRunsToCompletion(q)&&(yes(previous,"DailyRunUntilComplete")||manualQueues.count(str(q,"Name"))||cyclingQueues.count(str(q,"Name")));}
 for(auto j:jobs)if(!names.count(lower(str(j->data,"Queue"))))throw std::runtime_error("Reassign the queue's downloads before deleting it.");
 auto old=state["Queues"];state["Queues"]=next;try{save();}catch(...){state["Queues"]=old;throw;}updateWakeTimer(epoch());
}
void Manager::setQueue(const Json& q){Lock l(mutex);auto next=state["Queues"];bool found=false;for(auto& r:next)if(str(r,"Name")==str(q,"Name")){r=q;found=true;break;}if(!found)next.push_back(q);setQueues(next);}
void Manager::prepareQueue(const std::string& name){
 state["CompletionWorkEpoch"]=guid();bool sync=false;for(auto& q:state["Queues"])if(str(q,"Name")==name){sync=yes(q,"Synchronize");q["NextRunUtc"]=nullptr;q["DailyRunUntilComplete"]=dailyRunsToCompletion(q);q["CompletionCanceled"]=false;q["CompletionCycle"]=guid();}
 cycleFailed[name]=false;
 for(auto j:jobs)if(str(j->data,"Queue")==name&&!isActive(j)&&str(j->data,"DuplicateOf").empty()){
  if(yes(j->data,"ServerRetryRequiresReview"))continue;
  auto status=str(j->data,"Status");
  if(!yes(j->data,"RequiresRequestCapture")&&(!yes(j->data,"PostAttempted")||!str(j->data,"ProtectedPostGetUrl").empty())&&yes(j->data,"QueueMember",status!="Complete")&&(status=="Paused"||status=="Failed"||status=="Queued")){j->data["Status"]="Queued";j->data["QueueOrigin"]=true;j->data.erase("IndividualStart");j->data["QueueAttempts"]=0;j->data["NotBefore"]=j->data.value("ServerRetryAfterUtc",Json());}
  if(sync&&yes(j->data,"QueueMember",status!="Complete")&&!j->data.contains("OfflineProject")&&str(j->data,"ProtectedRequest").empty()&&!yes(j->data,"RequiresRequestCapture")&&!yes(j->data,"RequiresMediaCapture")&&str(j->data,"RecycledAt").empty()&&status=="Complete"&&str(j->data,"PreviousVersionOf").empty()&&str(j->data,"SourceUrl").empty()&&str(j->data,"ProtectedAdaptive").empty()){
   bool replacing=false;for(auto other:jobs)replacing|=str(other->data,"ReplacementOf")==j->id();if(!replacing){j->data["SyncPending"]=true;j->data["SyncRetryFailed"]=false;j->data["QueueOrigin"]=true;j->data["QueueAttempts"]=0;j->data["NotBefore"]=j->data.value("ServerRetryAfterUtc",Json());}
  }
 }
}
void Manager::queueRun(const std::string& name,bool enabled){
 Lock l(mutex);
 if(std::none_of(state["Queues"].begin(),state["Queues"].end(),[&](const Json& q){return str(q,"Name")==name;}))throw std::runtime_error("Queue no longer exists.");
 bool hadEpoch=state.contains("CompletionWorkEpoch");auto previousEpoch=state.value("CompletionWorkEpoch",Json());auto previousQueues=state["Queues"];auto previousManual=manualQueues;auto previousCycling=cyclingQueues;auto previousFailures=cycleFailed;auto previousPaused=schedulePaused;
 std::vector<Json> previousJobs;for(auto job:jobs)previousJobs.push_back(job->data);
 std::vector<std::shared_ptr<Cancel>> pendingStops;
 try{
  for(auto& q:state["Queues"])if(str(q,"Name")==name){q["Enabled"]=enabled;if(!enabled){q["DailyRunUntilComplete"]=false;q["CompletionCanceled"]=true;}if(enabled)armIndependentStop(q,q,epoch(),true);armDailyManualStop(q,q,epoch(),enabled,true);}
  if(enabled){for(auto job:jobs)if(str(job->data,"Queue")==name)job->data.erase("ServerRetryRequiresReview");manualQueues.insert(name);prepareQueue(name);}
  else{
   manualQueues.erase(name);cyclingQueues.erase(name);cycleFailed.erase(name);
   for(auto j:jobs)if(str(j->data,"Queue")==name){j->data["GrabberImmediate"]=false;j->data.erase("IndividualStart");j->data["SyncPending"]=false;if(isActive(j)){schedulePaused.insert(j->id());pendingStops.push_back(active[j->id()]);}}
  }
  save();
 }catch(...){
  if(hadEpoch)state["CompletionWorkEpoch"]=previousEpoch;else state.erase("CompletionWorkEpoch");state["Queues"]=previousQueues;manualQueues=previousManual;cyclingQueues=previousCycling;cycleFailed=previousFailures;schedulePaused=previousPaused;
  for(size_t i=0;i<jobs.size();++i)jobs[i]->data=previousJobs[i];
  throw;
 }
 for(auto& stop:pendingStops)stop->stop=true;
 updateWakeTimer(epoch());
}
void Manager::startQueuesOnStartup(){
 Lock lock(mutex);if(startupQueuesApplied||stopping)return;
 bool hadEpoch=state.contains("CompletionWorkEpoch");auto previousEpoch=state.value("CompletionWorkEpoch",Json());auto previousQueues=state["Queues"];auto previousManual=manualQueues;auto previousFailures=cycleFailed;
 std::vector<Json> previousJobs;for(auto job:jobs)previousJobs.push_back(job->data);
 bool changed=false;
 try{
  for(auto& q:state["Queues"])if(q.contains("StartOnStartup")&&q["StartOnStartup"].is_boolean()&&yes(q,"StartOnStartup")){
   auto name=str(q,"Name");q["Enabled"]=true;armIndependentStop(q,q,epoch(),true);armDailyManualStop(q,q,epoch(),true,true);manualQueues.insert(name);prepareQueue(name);changed=true;
  }
  if(changed)save();startupQueuesApplied=true;
 }catch(...){if(hadEpoch)state["CompletionWorkEpoch"]=previousEpoch;else state.erase("CompletionWorkEpoch");state["Queues"]=previousQueues;manualQueues=previousManual;cycleFailed=previousFailures;for(size_t i=0;i<jobs.size();++i)jobs[i]->data=previousJobs[i];throw;}
 updateWakeTimer(epoch());
}
void Manager::queueTick(i64 now){
 // An explicit Resume starts only that file, even when its queue is stopped or
 // outside its scheduled window. It still shares the global transfer limit.
 auto individuallyRequested=jobs;for(auto job:individuallyRequested){
  if(active.size()>=(size_t)std::clamp<i64>(num(state["Settings"],"Parallel",3),1,16))break;
  if(yes(job->data,"IndividualStart")&&!yes(job->data,"GrabberImmediate")&&!isActive(job)&&str(job->data,"DuplicateOf").empty()){
   if(str(job->data,"Status")=="Complete"&&yes(job->data,"SyncPending"))startSynchronization(job);
   else if(str(job->data,"Status")=="Queued")start(job);
  }
 }

 // Explicit Grabber starts are independent of the selected queue's schedule.
 // They retain queue membership, share the global limit, and never start other jobs.
 std::map<std::string,int> projectActive;for(auto job:jobs)if(yes(job->data,"GrabberImmediate")&&isActive(job))++projectActive[str(job->data,"ProjectId")];
 auto immediateJobs=jobs;for(auto job:immediateJobs){if(active.size()>=(size_t)std::clamp<i64>(num(state["Settings"],"Parallel",3),1,16))break;
  auto id=str(job->data,"ProjectId");if(id.empty()||!yes(job->data,"GrabberImmediate")||isActive(job)||str(job->data,"Status")!="Queued"||!str(job->data,"DuplicateOf").empty()||projectActive[id]>=std::clamp<i64>(num(job->data,"GrabberParallel",2),1,16))continue;start(job);++projectActive[id];
 }

 for(auto& q:state["Queues"]){auto name=str(q,"Name");if(manualQueues.count(name)&&nextDailyStartAfterManualStop(q,now))manualQueues.erase(name);if(independentStopExpired(q,now))q["Enabled"]=false;bool automatic=inWindow(q,now,false);bool scheduled=yes(q,"Scheduled")||yes(q,"RunOnce");
  if(!automatic)openWindows.erase(name);
  if(automatic&&scheduled&&!openWindows.count(name)){openWindows.insert(name);if(!yes(q,"RunOnce")||!yes(q,"OnceStarted"))prepareQueue(name);}
  auto due=parseDate(q.value("NextRunUtc",Json()));if(automatic&&due&&due<=now)prepareQueue(name);
  bool work=false;int running=0;std::vector<JobPtr> retried;
  for(auto j:jobs)if(str(j->data,"Queue")==name&&!yes(j->data,"GrabberImmediate")&&!yes(j->data,"IndividualStart")){
   if(!isActive(j)&&queueFailureCanRetry(q,j->data,cyclingQueues.count(name)!=0)){
    j->data["QueueAttempts"]=num(j->data,"QueueAttempts")+1;j->data["Status"]="Queued";j->data["NotBefore"]=date(queueRetryNotBefore(q,j->data,now));retried.push_back(j);
   }
   if(!isActive(j)&&yes(q,"Synchronize")&&str(j->data,"Status")=="Complete"&&yes(j->data,"SyncRetryFailed")){
    auto failure=j->data;failure["Status"]="Failed";failure["LastHttpStatus"]=num(j->data,"SyncLastHttpStatus");failure["LastFtpStatus"]=num(j->data,"SyncLastFtpStatus");
    if(num(failure,"LastHttpStatus")!=401&&num(failure,"LastHttpStatus")!=407&&queueFailureCanRetry(q,failure,cyclingQueues.count(name)!=0)&&fs::is_regular_file(j->target())){
     j->data["QueueAttempts"]=num(j->data,"QueueAttempts")+1;j->data["SyncPending"]=true;j->data["SyncRetryFailed"]=false;j->data["SyncStatus"]="Waiting to retry check";j->data["NotBefore"]=date(queueRetryNotBefore(q,j->data,now));retried.push_back(j);
    }else if(cyclingQueues.count(name)&&!queueFailureExhausted(q,j->data))cycleFailed[name]=true;
   }
   work|=isActive(j)||str(j->data,"Status")=="Queued"||yes(j->data,"SyncPending");running+=isActive(j)?1:0;
  }
  // Failures go behind the other files without invalidating the traversal above.
  for(auto j:retried){auto found=std::find(jobs.begin(),jobs.end(),j);if(found!=jobs.end()){jobs.erase(found);jobs.push_back(j);}}
  if(!work){if(yes(q,"DailyRunUntilComplete"))q["DailyRunUntilComplete"]=false;
   if(cyclingQueues.erase(name)){
    bool success=!cycleFailed[name];int failedFiles=0;for(auto j:jobs)if(str(j->data,"Queue")==name&&yes(j->data,"QueueMember")){if(!queueItemFinished(q,j->data))success=false;if(queueFailureExhausted(q,j->data))++failedFiles;}
    if(success&&!queueCompletionSteps(q).empty()){auto completion=queueCompletionEvent(q);completion["FailedFiles"]=failedFiles;completion["WorkEpoch"]=str(state,"CompletionWorkEpoch");finishedQueues.push_back(completion);}
    if(yes(q,"Enabled",true)&&num(q,"RepeatMinutes")>0)q["NextRunUtc"]=date(now+num(q,"RepeatMinutes")*60000);
    cycleFailed.erase(name);
   }
   manualQueues.erase(name);if(yes(q,"RunOnce")&&yes(q,"OnceStarted"))q["Enabled"]=false;
  }
  if(!inWindow(q,now,manualQueues.count(name)!=0)){
   for(auto j:jobs)if(str(j->data,"Queue")==name&&!yes(j->data,"GrabberImmediate")&&!yes(j->data,"IndividualStart")&&isActive(j)&&!convertingProjects.count(str(j->data,"ProjectId"))&&str(j->data,"Status")!="Pausing"){schedulePaused.insert(j->id());if(str(j->data,"Status")!="Complete")j->data["Status"]="Pausing";active[j->id()]->stop=true;}continue;
  }
  // Snapshot: a synchronization worker may append a replacement after taking the lock.
  auto pending=jobs;for(auto j:pending){if(running>=std::clamp<i64>(num(q,"Parallel",2),1,16)||active.size()>=(size_t)std::clamp<i64>(num(state["Settings"],"Parallel",3),1,16))break;
   if(str(j->data,"Queue")!=name||yes(j->data,"GrabberImmediate")||yes(j->data,"IndividualStart")||isActive(j)||!str(j->data,"DuplicateOf").empty())continue;
   if((yes(j->data,"SyncPending")||str(j->data,"Status")=="Queued")&&parseDate(j->data.value("NotBefore",Json()))<=now&&serverRetryReady(j->data,now)){
    if(yes(q,"RunOnce"))q["OnceStarted"]=true;if(!cyclingQueues.count(name))q["CompletionCycle"]=guid();cyclingQueues.insert(name);if(yes(j->data,"SyncPending"))startSynchronization(j);else start(j);++running;
   }
  }
 }
}
std::vector<Json> Manager::takeQueueCompletions(){Lock l(mutex);auto result=std::move(finishedQueues);finishedQueues.clear();return result;}
bool Manager::completionReady(const std::string& queue)const{Lock l(mutex);if(!active.empty())return false;const Json* policy=nullptr;for(const auto& q:state["Queues"])if(str(q,"Name")==queue)policy=&q;if(!policy||yes(*policy,"CompletionCanceled"))return false;for(auto j:jobs)if(str(j->data,"Status")=="Queued"||yes(j->data,"SyncPending")||(str(j->data,"Queue")==queue&&yes(j->data,"QueueMember")&&!queueItemFinished(*policy,j->data)))return false;return true;}
bool Manager::completionEventReady(const Json& event)const{Lock lock(mutex);if(str(event,"Cycle").empty()||str(event,"WorkEpoch").empty()||str(event,"WorkEpoch")!=str(state,"CompletionWorkEpoch"))return false;for(const auto& q:state["Queues"])if(str(q,"Name")==str(event,"Queue"))return str(q,"CompletionCycle")==str(event,"Cycle")&&completionReady(str(event,"Queue"));return false;}
void Manager::reorder(JobPtr job,const std::string& queue,JobPtr before){
 Lock lock(mutex);validateQueueMembership(*this,job,true,queue);
 if(before==job)return;
 if(before&&(std::find(jobs.begin(),jobs.end(),before)==jobs.end()||str(before->data,"Queue")!=queue||!yes(before->data,"QueueMember",str(before->data,"Status")!="Complete")))throw std::runtime_error("The destination row changed. Select a current queue item and try again.");
 auto previous=jobs;auto data=job->data;
 try{
  jobs.erase(std::remove(jobs.begin(),jobs.end(),job),jobs.end());auto at=before?std::find(jobs.begin(),jobs.end(),before):jobs.end();jobs.insert(at,job);
  if(str(data,"Queue")!=queue||!yes(data,"QueueMember",str(data,"Status")!="Complete"))job->data["SyncPending"]=false;
  job->data["Queue"]=queue;job->data["QueueMember"]=true;save();
 }catch(...){jobs.swap(previous);job->data.swap(data);throw;}
}

void Manager::move(JobPtr j,int direction){Lock l(mutex);if(direction!=-1&&direction!=1)throw std::runtime_error("Invalid queue direction.");auto at=std::find(jobs.begin(),jobs.end(),j);if(at==jobs.end())return;if(isActive(j))throw std::runtime_error("Stop the download before reordering it.");int i=(int)(at-jobs.begin());for(int k=i+direction;k>=0&&k<(int)jobs.size();k+=direction)if(yes(jobs[k]->data,"QueueMember",str(jobs[k]->data,"Status")!="Complete")&&str(jobs[k]->data,"Queue")==str(j->data,"Queue")){auto old=jobs;std::swap(jobs[i],jobs[k]);try{save();}catch(...){jobs=old;throw;}break;}}
static i64 synchronizationProbeSize(Http& probe,const Cancel& cancel){
 // A range request against an empty file legitimately has no satisfiable range.
 if(probe.status==416&&probe.header(L"Content-Range")=="bytes */0")return 0;
 if(probe.status!=200&&probe.status!=206)throw HttpRejected(probe.status,probe.header(L"Retry-After"));
 auto encoding=lower(probe.header(L"Content-Encoding"));
 if(!encoding.empty()&&encoding!="identity")throw std::runtime_error("The server ignored identity encoding during synchronization.");
 auto sizeValue=[](const std::string& value){
  if(value.empty()||value.find_first_not_of("0123456789")!=std::string::npos)throw std::runtime_error("Invalid synchronization response size.");
  try{return std::stoll(value);}catch(...){throw std::runtime_error("Synchronization response size overflow.");}
 };
 auto length=probe.header(L"Content-Length");
 if(probe.status==200)return length.empty()?-1:sizeValue(length);
 std::smatch match;auto range=probe.header(L"Content-Range");
 if(!std::regex_match(range,match,std::regex("bytes 0-0/([0-9]+)")))throw std::runtime_error("Server returned an invalid synchronization probe range.");
 auto size=sizeValue(match[1]);
 if(size<=0||(!length.empty()&&sizeValue(length)!=1))throw std::runtime_error("Server returned an invalid synchronization probe size.");
 // Headers alone do not establish a successful probe. Validate its bounded body
 // before declaring the saved file current or reserving a replacement download.
 if(probe.all(1,cancel).size()!=1)throw std::runtime_error("Incomplete synchronization probe range.");
 return size;
}
void Manager::startSynchronization(JobPtr original){
 state["CompletionWorkEpoch"]=guid();
 auto cancel=std::make_shared<Cancel>();original->data["SyncPending"]=false;original->data["SyncRetryFailed"]=false;original->data["SyncLastHttpStatus"]=0;original->data["SyncLastFtpStatus"]=0;clearServerRetry(original->data);original->data["AuthenticationPromptPending"]=false;original->data.erase("AuthenticationOrigin");original->data.erase("AuthenticationScheme");original->data["SyncStatus"]="Checking for updates";active[original->id()]=cancel;
 threads.emplace_back([this,original,cancel]{JobPtr incoming;std::string queue;bool downloaded=false;
  try{
   Json source,prefs;{Lock l(mutex);source=original->data;prefs=browserSessionPreferences(browserProxyPreferences(state["Settings"],source),source);queue=str(source,"Queue");}
   if(!fs::is_regular_file(original->target()))throw std::runtime_error("The saved file is missing. Use Redownload to restore it.");
   Url url(str(source,"Url"));if(url.scheme!="https"&&url.scheme!="http"&&url.scheme!="ftp")throw std::runtime_error("Synchronization supports direct HTTP, HTTPS and FTP files.");
   ensureConnection(original,*cancel);std::string tag,modified;i64 size=-1;
   if(url.scheme=="ftp"){auto metadata=previewFtp(str(source,"Url"),readHeaders(source),prefs,*cancel);size=num(metadata,"Size",-1);modified=str(metadata,"FtpModified");}
   else {auto pool=std::make_shared<HttpSession>(prefs,browserSessionSaver(*this,original,prefs.value("ActiveBrowserSession",Json::object())));Http probe(str(source,"Url"),readHeaders(source),prefs,*cancel,0,0,"",nullptr,true,pool);
   {Lock lock(mutex);if(original->data.contains("ProtectedBrowserSession"))source["ProtectedBrowserSession"]=original->data["ProtectedBrowserSession"];}
   if(probe.status==401)for(const auto& challenge:probe.headers(L"WWW-Authenticate")){std::smatch method;if(std::regex_search(challenge,method,std::regex("(^|,)\\s*(Digest|Basic)\\s",std::regex::icase)))throw AuthenticationRequired(Url(probe.finalUrl).origin,lower(method[2])=="digest"?"Digest":"Basic");}
   size=synchronizationProbeSize(probe,*cancel);tag=probe.header(L"ETag");modified=probe.header(L"Last-Modified");}
   auto remoteSize=!str(source,"GrabberSourceHash").empty()&&str(source,"GrabberConvertedHash")==str(source,"Sha256")?num(source,"GrabberSourceBytes",-1):num(source,"Size",-1);
   bool same=size==remoteSize&&size>=0&&((!tag.empty()&&tag.rfind("W/",0)!=0&&tag==str(source,"ETag"))||(tag.empty()&&!modified.empty()&&modified==str(source,url.scheme=="ftp"?"FtpModified":"Modified")));
   cancel->check();
   {Lock l(mutex);
    if(same){original->data["SyncStatus"]="Up to date";original->data["LastSync"]=date();}
    else{
     // Reserve the existing verified replacement workflow while holding the manager lock.
     active.erase(original->id());
     incoming=add(str(source,"Url"),str(source,"Folder"),str(source,"FileName"),queue,true,readHeaders(source),"",Json::object(),readBrowserProxy(source));
     incoming->data["DuplicateOf"]=original->id();resolveDuplicate(incoming,"Replace");
     for(const char* field:{"Description","Category","DownloadPage","Connections","LimitKbps","ExpectedSha256","ProtectedBrowserProxy","RequiresBrowserProxyCapture","ProtectedBrowserSession","RequiresBrowserSessionCapture","ProjectId","ProjectDestinationRoot","GrabberContentType","GrabberDiscoveredUrl"})if(source.contains(field))incoming->data[field]=source[field];
     incoming->data["IndividualStart"]=yes(source,"IndividualStart");incoming->data["QueueOrigin"]=true;incoming->data["QueueAttempts"]=num(source,"QueueAttempts");incoming->data["SynchronizationReplacement"]=true;incoming->data["SuppressCompletionDialog"]=true;incoming->data["Status"]="Downloading";incoming->data["LastAttempt"]=date();active[incoming->id()]=cancel;save();
    }
   }
   if(incoming){if(event)event(incoming,false);transfer(*this,incoming,cancel);downloaded=true;scanCompleted(incoming,*cancel);Lock l(mutex);incoming->data["SyncStatus"]="Updated; previous version retained";incoming->data["LastSync"]=date();original->data["SyncStatus"]="Previous version";}
  }catch(const std::exception& e){Lock l(mutex);if(cancel->cancelled()||(incoming&&!str(incoming->data,"DuplicateOf").empty()))cycleFailed[queue]=true;original->data["SyncRetryFailed"]=!incoming&&!cancel->cancelled();if(auto http=dynamic_cast<const HttpRejected*>(&e)){original->data["SyncLastHttpStatus"]=http->status;if(!incoming)recordServerRetry(original->data,*http);}if(!incoming&&!cancel->cancelled())if(auto auth=dynamic_cast<const AuthenticationRequired*>(&e)){original->data["AuthenticationOrigin"]=auth->origin;original->data["AuthenticationScheme"]=auth->scheme;original->data["AuthenticationPromptPending"]=canRequestLogin(original->data);}if(!incoming&&!cancel->cancelled())if(auto auth=dynamic_cast<const FtpAuthenticationRequired*>(&e)){original->data["SyncLastFtpStatus"]=530;original->data["AuthenticationOrigin"]=auth->origin;original->data["AuthenticationScheme"]="FTP";original->data["AuthenticationPromptPending"]=canRequestLogin(original->data);}original->data["SyncStatus"]=cancel->cancelled()?"Check canceled":std::string("Check failed: ")+e.what();original->data["LastSync"]=date();
   if(incoming&&!str(incoming->data,"DuplicateOf").empty()){jobs.erase(std::remove(jobs.begin(),jobs.end(),incoming),jobs.end());incoming.reset();}
   if(incoming){incoming->data["Status"]=cancel->cancelled()?"Paused":"Failed";incoming->data["Error"]=cancel->cancelled()?"":e.what();if(auto http=dynamic_cast<const HttpRejected*>(&e)){incoming->data["LastHttpStatus"]=http->status;recordServerRetry(incoming->data,*http);}if(!cancel->cancelled())if(auto auth=dynamic_cast<const AuthenticationRequired*>(&e)){incoming->data["AuthenticationOrigin"]=auth->origin;incoming->data["AuthenticationScheme"]=auth->scheme;incoming->data["AuthenticationPromptPending"]=canRequestLogin(incoming->data);}if(!cancel->cancelled())if(auto auth=dynamic_cast<const FtpAuthenticationRequired*>(&e)){incoming->data["LastFtpStatus"]=530;incoming->data["AuthenticationOrigin"]=auth->origin;incoming->data["AuthenticationScheme"]="FTP";incoming->data["AuthenticationPromptPending"]=canRequestLogin(incoming->data);}}
  }
  {Lock l(mutex);active.erase(original->id());original->data.erase("IndividualStart");original->data.erase("ConnectionStatus");schedulePaused.erase(original->id());if(incoming){active.erase(incoming->id());incoming->data.erase("IndividualStart");incoming->speed=0;}try{save();}catch(...) {}}
  if(downloaded&&event)event(incoming,true);
 });
}
}
