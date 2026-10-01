#include "BrowserProxy.hpp"
#include "BrowserSession.hpp"
#include "Core.hpp"
#include <algorithm>
#include <regex>
namespace udm {
static void validateQueue(const Json& q){
 if(q.contains("StartOnStartup")&&!q["StartOnStartup"].is_boolean())throw std::runtime_error("Start on UDM startup must be on or off.");
 if(q.contains("WakeComputer")&&!q["WakeComputer"].is_boolean())throw std::runtime_error("Wake computer must be on or off.");
 auto name=trim(str(q,"Name"));if(name.empty()||name!=str(q,"Name")||name.size()>80||num(q,"Parallel",2)<1||num(q,"Parallel",2)>16||num(q,"StartMinute")<0||num(q,"StartMinute")>1439||num(q,"StopMinute",1440)<0||num(q,"StopMinute",1440)>1440||num(q,"Days",127)<0||num(q,"Days",127)>127||num(q,"Retries")<0||num(q,"Retries")>10)throw std::runtime_error("Invalid queue settings.");
 if(yes(q,"Scheduled")&&!yes(q,"RunOnce")&&!num(q,"Days"))throw std::runtime_error("Choose at least one day.");
 if(yes(q,"RunOnce")){auto a=parseDate(q.value("StartOnceUtc",Json())),b=parseDate(q.value("StopOnceUtc",Json()));if(!a||(b&&b<=a))throw std::runtime_error("Choose a valid start and stop date.");}
 if(num(q,"FileRetries")<0||num(q,"FileRetries")>20||num(q,"RetryDelaySeconds",30)<1||num(q,"RetryDelaySeconds",30)>86400||num(q,"RepeatMinutes")<0||num(q,"RepeatMinutes")>10080)throw std::runtime_error("Use 0-20 file retries, 1-86400 seconds retry delay and 0-10080 minutes repeat interval.");
 auto action=str(q,"FinishAction","None");if(action!="None"&&action!="Open file"&&action!="Exit UDM"&&action!="Disconnect dial-up / VPN"&&action!="Sleep"&&action!="Hibernate"&&action!="Shut down"&&action!="Restart")throw std::runtime_error("Unknown queue completion action.");
 if(action=="Open file"&&(!fs::path(wide(str(q,"FinishFile"))).is_absolute()||!fs::is_regular_file(fs::path(wide(str(q,"FinishFile"))))))throw std::runtime_error("Choose an existing file to open after this queue completes.");
 if(num(q,"FinishDelaySeconds",30)<15||num(q,"FinishDelaySeconds",30)>3600)throw std::runtime_error("Completion countdown must be 15-3600 seconds.");
 if(num(q,"RepeatMinutes")&&yes(q,"RunOnce"))throw std::runtime_error("Repeating queues require a daily window or no time schedule.");
 if(num(q,"RepeatMinutes")&&action!="None")throw std::runtime_error("Choose either queue repetition or a completion action.");
}
void Manager::setQueues(const Json& input){
 Lock l(mutex);if(!input.is_array()||input.empty())throw std::runtime_error("Keep at least one queue.");auto next=input;std::set<std::string> names;
 for(auto& q:next){validateQueue(q);if(!names.insert(lower(str(q,"Name"))).second)throw std::runtime_error("That queue already exists.");for(auto current:state["Queues"])if(str(current,"Name")==str(q,"Name")){
  bool rearm=yes(q,"RunOnce")&&(!yes(current,"RunOnce")||current.value("StartOnceUtc",Json())!=q.value("StartOnceUtc",Json())||current.value("StopOnceUtc",Json())!=q.value("StopOnceUtc",Json())||(!yes(current,"Enabled",true)&&yes(q,"Enabled",true)));
  q["OnceStarted"]=rearm?false:yes(current,"OnceStarted");
  q["NextRunUtc"]=num(q,"RepeatMinutes")==num(current,"RepeatMinutes")?current.value("NextRunUtc",Json()):Json();
 }}
 for(auto j:jobs)if(!names.count(lower(str(j->data,"Queue"))))throw std::runtime_error("Reassign the queue's downloads before deleting it.");
 auto old=state["Queues"];state["Queues"]=next;try{save();}catch(...){state["Queues"]=old;throw;}updateWakeTimer(epoch());
}
void Manager::setQueue(const Json& q){Lock l(mutex);auto next=state["Queues"];bool found=false;for(auto& r:next)if(str(r,"Name")==str(q,"Name")){r=q;found=true;break;}if(!found)next.push_back(q);setQueues(next);}
void Manager::prepareQueue(const std::string& name){
 bool sync=false;for(auto& q:state["Queues"])if(str(q,"Name")==name){sync=yes(q,"Synchronize");q["NextRunUtc"]=nullptr;}
 cycleFailed[name]=false;
 for(auto j:jobs)if(str(j->data,"Queue")==name&&!isActive(j)&&str(j->data,"DuplicateOf").empty()){
  auto status=str(j->data,"Status");
  if(!yes(j->data,"RequiresRequestCapture")&&(!yes(j->data,"PostAttempted")||!str(j->data,"ProtectedPostGetUrl").empty())&&yes(j->data,"QueueMember",status!="Complete")&&(status=="Paused"||status=="Failed"||status=="Queued")){j->data["Status"]="Queued";j->data["QueueOrigin"]=true;j->data["QueueAttempts"]=0;j->data["NotBefore"]=nullptr;}
  if(sync&&!j->data.contains("OfflineProject")&&str(j->data,"ProtectedRequest").empty()&&!yes(j->data,"RequiresRequestCapture")&&!yes(j->data,"RequiresMediaCapture")&&str(j->data,"RecycledAt").empty()&&status=="Complete"&&str(j->data,"PreviousVersionOf").empty()&&str(j->data,"SourceUrl").empty()&&str(j->data,"ProtectedAdaptive").empty()){
   bool replacing=false;for(auto other:jobs)replacing|=str(other->data,"ReplacementOf")==j->id();if(!replacing)j->data["SyncPending"]=true;
  }
 }
}
void Manager::queueRun(const std::string& name,bool enabled){
 Lock l(mutex);bool found=false;for(auto& q:state["Queues"])if(str(q,"Name")==name){q["Enabled"]=enabled;found=true;}
 if(!found)throw std::runtime_error("Queue no longer exists.");
 if(enabled){manualQueues.insert(name);prepareQueue(name);}else{manualQueues.erase(name);cyclingQueues.erase(name);cycleFailed.erase(name);for(auto j:jobs)if(str(j->data,"Queue")==name){j->data["GrabberImmediate"]=false;j->data["SyncPending"]=false;if(isActive(j)){schedulePaused.insert(j->id());active[j->id()]->stop=true;}}}
 save();updateWakeTimer(epoch());
}
void Manager::startQueuesOnStartup(){
 Lock lock(mutex);if(startupQueuesApplied||stopping)return;
 auto previousQueues=state["Queues"];auto previousManual=manualQueues;auto previousFailures=cycleFailed;
 std::vector<Json> previousJobs;for(auto job:jobs)previousJobs.push_back(job->data);
 bool changed=false;
 try{
  for(auto& q:state["Queues"])if(q.contains("StartOnStartup")&&q["StartOnStartup"].is_boolean()&&yes(q,"StartOnStartup")){
   auto name=str(q,"Name");q["Enabled"]=true;manualQueues.insert(name);prepareQueue(name);changed=true;
  }
  if(changed)save();startupQueuesApplied=true;
 }catch(...){state["Queues"]=previousQueues;manualQueues=previousManual;cycleFailed=previousFailures;for(size_t i=0;i<jobs.size();++i)jobs[i]->data=previousJobs[i];throw;}
 updateWakeTimer(epoch());
}
void Manager::queueTick(i64 now){
 // Explicit Grabber starts are independent of the selected queue's schedule.
 // They retain queue membership, share the global limit, and never start other jobs.
 std::map<std::string,int> projectActive;for(auto job:jobs)if(yes(job->data,"GrabberImmediate")&&isActive(job))++projectActive[str(job->data,"ProjectId")];
 auto immediateJobs=jobs;for(auto job:immediateJobs){if(active.size()>=(size_t)std::clamp<i64>(num(state["Settings"],"Parallel",3),1,16))break;
  auto id=str(job->data,"ProjectId");if(id.empty()||!yes(job->data,"GrabberImmediate")||isActive(job)||str(job->data,"Status")!="Queued"||!str(job->data,"DuplicateOf").empty()||projectActive[id]>=std::clamp<i64>(num(job->data,"GrabberParallel",2),1,16))continue;start(job);++projectActive[id];
 }

 for(auto& q:state["Queues"]){auto name=str(q,"Name");bool automatic=inWindow(q,now,false);bool scheduled=yes(q,"Scheduled")||yes(q,"RunOnce");
  if(!automatic)openWindows.erase(name);
  if(automatic&&scheduled&&!openWindows.count(name)){openWindows.insert(name);if(!yes(q,"RunOnce")||!yes(q,"OnceStarted"))prepareQueue(name);}
  auto due=parseDate(q.value("NextRunUtc",Json()));if(automatic&&due&&due<=now)prepareQueue(name);
  bool work=false;int running=0;
  for(auto j:jobs)if(str(j->data,"Queue")==name&&!yes(j->data,"GrabberImmediate")){
   if(str(j->data,"ProtectedRequest").empty()&&cyclingQueues.count(name)&&yes(j->data,"QueueOrigin")&&yes(j->data,"QueueMember")&&str(j->data,"Status")=="Failed"&&num(j->data,"QueueAttempts")<num(q,"FileRetries")){
    auto code=num(j->data,"LastHttpStatus");bool permanent=code>=400&&code<500&&code!=408&&code!=425&&code!=429;
    if(!permanent){j->data["QueueAttempts"]=num(j->data,"QueueAttempts")+1;j->data["Status"]="Queued";j->data["NotBefore"]=date(now+num(q,"RetryDelaySeconds",30)*1000);}
   }
   work|=isActive(j)||str(j->data,"Status")=="Queued"||yes(j->data,"SyncPending");running+=isActive(j)?1:0;
  }
  if(!work){
   if(cyclingQueues.erase(name)){
    bool success=!cycleFailed[name];for(auto j:jobs)if(str(j->data,"Queue")==name&&yes(j->data,"QueueMember")&&str(j->data,"Status")!="Complete")success=false;
    if(success&&str(q,"FinishAction","None")!="None")finishedQueues.push_back({{"Queue",name},{"Action",str(q,"FinishAction")},{"DelaySeconds",num(q,"FinishDelaySeconds",30)},{"File",str(q,"FinishFile")}});
    if(yes(q,"Enabled",true)&&num(q,"RepeatMinutes")>0)q["NextRunUtc"]=date(now+num(q,"RepeatMinutes")*60000);
    cycleFailed.erase(name);
   }
   manualQueues.erase(name);if(yes(q,"RunOnce")&&yes(q,"OnceStarted"))q["Enabled"]=false;
  }
  if(!inWindow(q,now,manualQueues.count(name)!=0)){
   for(auto j:jobs)if(str(j->data,"Queue")==name&&!yes(j->data,"GrabberImmediate")&&isActive(j)&&!convertingProjects.count(str(j->data,"ProjectId"))&&str(j->data,"Status")!="Pausing"){schedulePaused.insert(j->id());if(str(j->data,"Status")!="Complete")j->data["Status"]="Pausing";active[j->id()]->stop=true;}continue;
  }
  // Snapshot: a synchronization worker may append a replacement after taking the lock.
  auto pending=jobs;for(auto j:pending){if(running>=std::clamp<i64>(num(q,"Parallel",2),1,16)||active.size()>=(size_t)std::clamp<i64>(num(state["Settings"],"Parallel",3),1,16))break;
   if(str(j->data,"Queue")!=name||yes(j->data,"GrabberImmediate")||isActive(j)||!str(j->data,"DuplicateOf").empty())continue;
   if(yes(j->data,"SyncPending")|| (str(j->data,"Status")=="Queued"&&parseDate(j->data.value("NotBefore",Json()))<=now)){
    if(yes(q,"RunOnce"))q["OnceStarted"]=true;cyclingQueues.insert(name);if(yes(j->data,"SyncPending"))startSynchronization(j);else start(j);++running;
   }
  }
 }
}
std::vector<Json> Manager::takeQueueCompletions(){Lock l(mutex);auto result=std::move(finishedQueues);finishedQueues.clear();return result;}
bool Manager::completionReady(const std::string& queue)const{Lock l(mutex);if(!active.empty())return false;for(auto j:jobs)if(str(j->data,"Status")=="Queued"||yes(j->data,"SyncPending")||(str(j->data,"Queue")==queue&&yes(j->data,"QueueMember")&&str(j->data,"Status")!="Complete"))return false;return true;}
void Manager::reorder(JobPtr job,const std::string& queue,JobPtr before){
 Lock l(mutex);if(!job||isActive(job)||str(job->data,"Status")=="Complete")throw std::runtime_error("Stop this unfinished download before moving it.");bool found=false;for(auto q:state["Queues"])found|=str(q,"Name")==queue;if(!found)throw std::runtime_error("Queue no longer exists.");
 if(before&&(before==job||str(before->data,"Queue")!=queue))return;
 auto old=jobs;auto data=job->data;jobs.erase(std::remove(jobs.begin(),jobs.end(),job),jobs.end());auto at=before?std::find(jobs.begin(),jobs.end(),before):jobs.end();jobs.insert(at,job);job->data["Queue"]=queue;job->data["QueueMember"]=true;
 try{save();}catch(...){jobs=old;job->data=data;throw;}
}
void Manager::move(JobPtr j,int direction){Lock l(mutex);if(direction!=-1&&direction!=1)throw std::runtime_error("Invalid queue direction.");auto at=std::find(jobs.begin(),jobs.end(),j);if(at==jobs.end())return;if(isActive(j))throw std::runtime_error("Stop the download before reordering it.");int i=(int)(at-jobs.begin());for(int k=i+direction;k>=0&&k<(int)jobs.size();k+=direction)if(yes(jobs[k]->data,"QueueMember",str(jobs[k]->data,"Status")!="Complete")&&str(jobs[k]->data,"Queue")==str(j->data,"Queue")){auto old=jobs;std::swap(jobs[i],jobs[k]);try{save();}catch(...){jobs=old;throw;}break;}}
void Manager::startSynchronization(JobPtr original){
 auto cancel=std::make_shared<Cancel>();original->data["SyncPending"]=false;original->data["SyncStatus"]="Checking for updates";active[original->id()]=cancel;
 threads.emplace_back([this,original,cancel]{JobPtr incoming;std::string queue;bool downloaded=false;
  try{
   Json source,prefs;{Lock l(mutex);source=original->data;prefs=browserSessionPreferences(browserProxyPreferences(state["Settings"],source),source);queue=str(source,"Queue");}
   if(!fs::is_regular_file(original->target()))throw std::runtime_error("The saved file is missing. Use Redownload to restore it.");
   Url url(str(source,"Url"));if(url.scheme!="https"&&url.scheme!="http")throw std::runtime_error("Synchronization supports direct HTTP and HTTPS files.");
   ensureConnection(original,*cancel);auto pool=std::make_shared<HttpSession>(prefs,browserSessionSaver(*this,original,prefs.value("ActiveBrowserSession",Json::object())));Http probe(str(source,"Url"),readHeaders(source),prefs,*cancel,0,0,"",nullptr,true,pool);
   {Lock lock(mutex);if(original->data.contains("ProtectedBrowserSession"))source["ProtectedBrowserSession"]=original->data["ProtectedBrowserSession"];}
   if(probe.status!=200&&probe.status!=206)throw HttpRejected(probe.status,probe.header(L"Retry-After"));
   auto tag=probe.header(L"ETag"),modified=probe.header(L"Last-Modified");i64 size=-1;
   auto length=probe.header(probe.status==206?L"Content-Range":L"Content-Length");std::smatch match;
   if(probe.status==206&&std::regex_match(length,match,std::regex("bytes 0-0/([0-9]+)")))size=std::stoll(match[1]);else if(probe.status==200&&!length.empty())size=std::stoll(length);
   auto remoteSize=!str(source,"GrabberSourceHash").empty()&&str(source,"GrabberConvertedHash")==str(source,"Sha256")?num(source,"GrabberSourceBytes",-1):num(source,"Size",-1);
   bool same=size==remoteSize&&size>=0&&((!tag.empty()&&tag.rfind("W/",0)!=0&&tag==str(source,"ETag"))||(tag.empty()&&!modified.empty()&&modified==str(source,"Modified")));
   cancel->check();
   {Lock l(mutex);
    if(same){original->data["SyncStatus"]="Up to date";original->data["LastSync"]=date();}
    else{
     // Reserve the existing verified replacement workflow while holding the manager lock.
     active.erase(original->id());
     incoming=add(str(source,"Url"),str(source,"Folder"),str(source,"FileName"),queue,true,readHeaders(source),"",Json::object(),readBrowserProxy(source));
     incoming->data["DuplicateOf"]=original->id();resolveDuplicate(incoming,"Replace");
     for(const char* field:{"Description","Category","DownloadPage","Connections","LimitKbps","ExpectedSha256","ProtectedBrowserProxy","RequiresBrowserProxyCapture","ProtectedBrowserSession","RequiresBrowserSessionCapture","ProjectId","ProjectDestinationRoot","GrabberContentType","GrabberDiscoveredUrl"})if(source.contains(field))incoming->data[field]=source[field];
     incoming->data["QueueOrigin"]=true;incoming->data["SuppressCompletionDialog"]=true;incoming->data["Status"]="Downloading";incoming->data["LastAttempt"]=date();active[incoming->id()]=cancel;save();
    }
   }
   if(incoming){if(event)event(incoming,false);transfer(*this,incoming,cancel);downloaded=true;scanCompleted(incoming,*cancel);Lock l(mutex);incoming->data["SyncStatus"]="Updated; previous version retained";incoming->data["LastSync"]=date();original->data["SyncStatus"]="Previous version";}
  }catch(const std::exception& e){Lock l(mutex);cycleFailed[queue]=true;original->data["SyncStatus"]=cancel->cancelled()?"Check canceled":std::string("Check failed: ")+e.what();original->data["LastSync"]=date();
   if(incoming&&!str(incoming->data,"DuplicateOf").empty()){jobs.erase(std::remove(jobs.begin(),jobs.end(),incoming),jobs.end());incoming.reset();}
   if(incoming){incoming->data["Status"]=cancel->cancelled()?"Paused":"Failed";incoming->data["Error"]=cancel->cancelled()?"":e.what();if(auto http=dynamic_cast<const HttpRejected*>(&e))incoming->data["LastHttpStatus"]=http->status;}
  }
  {Lock l(mutex);active.erase(original->id());original->data.erase("ConnectionStatus");schedulePaused.erase(original->id());if(incoming){active.erase(incoming->id());incoming->speed=0;}try{save();}catch(...) {}}
  if(downloaded&&event)event(incoming,true);
 });
}
}
