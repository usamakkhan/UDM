#include "QueueWake.hpp"
#include <algorithm>
#include <limits>
namespace udm {
i64 nextQueueWake(const Json& q,i64 now){
 if(!yes(q,"Enabled",true)||!yes(q,"WakeComputer"))return 0;
 // A completed run-once queue must never arm a second start timer.
 if(yes(q,"RunOnce")){
  auto start=parseDate(q.value("StartOnceUtc",Json()));
  return !yes(q,"OnceStarted")&&start>now?start:0;
 }
 auto repeat=parseDate(q.value("NextRunUtc",Json()));
 i64 next=repeat>now&&inWindow(q,repeat,false)?repeat:0;
 if(!yes(q,"Scheduled"))return next;
 if(!num(q,"Days",127))return 0;
 if(num(q,"Days",127)==127&&(num(q,"StartMinute")==num(q,"StopMinute",1440)||(num(q,"StartMinute")==0&&num(q,"StopMinute",1440)==1440)))return next;
 // Walk UTC minute boundaries through the same predicate used by queueTick.
 // This handles overnight weekdays, skipped local minutes, repeated local
 // hours, and offset changes without guessing a local-to-UTC conversion.
 auto minute=(now/60000)*60000;
 bool previous=inWindow(q,now,false);
 for(int i=1;i<=8*24*60;++i){
  auto candidate=minute+(i64)i*60000;
  if(next&&candidate>=next)break;
  bool current=inWindow(q,candidate,false);
  if(current&&!previous){next=candidate;break;}
  previous=current;
 }
 return next;
}
bool queueHasWakeWork(const Json& q,const std::vector<JobPtr>& jobs){
 for(const auto& j:jobs){const auto& d=j->data;
  if(str(d,"Queue")!=str(q,"Name")||!str(d,"DuplicateOf").empty()||!str(d,"RecycledAt").empty()||yes(d,"ConfirmationPending"))continue;
  auto status=str(d,"Status");
  if(!yes(d,"RequiresRequestCapture")&&!yes(d,"RequiresMediaCapture")&&(!yes(d,"PostAttempted")||!str(d,"ProtectedPostGetUrl").empty())&&yes(d,"QueueMember",status!="Complete")&&(status=="Paused"||status=="Failed"||status=="Queued"||status=="Downloading"||status=="Pausing"||status=="Verifying"||status=="Merging"))return true;
  if(yes(q,"Synchronize")&&yes(d,"QueueMember")&&status=="Complete"&&!d.contains("OfflineProject")&&str(d,"ProtectedRequest").empty()&&!yes(d,"RequiresRequestCapture")&&!yes(d,"RequiresMediaCapture")&&str(d,"PreviousVersionOf").empty()&&str(d,"SourceUrl").empty()&&str(d,"ProtectedAdaptive").empty()){
   try{auto scheme=Url(str(d,"Url")).scheme;if(scheme=="http"||scheme=="https"||scheme=="ftp")return true;}catch(...){}
  }
 }
 return false;
}
QueueWakeTimer::~QueueWakeTimer(){stop();}
void QueueWakeTimer::stop(){if(timer){CancelWaitableTimer(timer);CloseHandle(timer);timer=nullptr;}due=0;lastCheck=0;signature.clear();status={{"Status","Off"},{"Message","Wake computer is off."}};}
void QueueWakeTimer::update(const Json& queues,const std::vector<JobPtr>& jobs,i64 now){
 Json eligible=Json::array();bool enabled=false;
 for(const auto& q:queues)if(yes(q,"Enabled",true)&&yes(q,"WakeComputer")){
  enabled=true;if(queueHasWakeWork(q,jobs))eligible.push_back(q);
 }
 auto key=eligible.dump()+std::to_string(enabled);
 if(key==signature&&now>=lastCheck&&now-lastCheck<60000&&(!due||due>now))return;
 signature=key;lastCheck=now;i64 next=0;std::string name;
 for(const auto& q:eligible){auto candidate=nextQueueWake(q,now);if(candidate&&(!next||candidate<next)){next=candidate;name=str(q,"Name");}}
 if(!next){if(timer){CancelWaitableTimer(timer);CloseHandle(timer);timer=nullptr;}due=0;status={{"Status",enabled?"Waiting":"Off"},{"Message",enabled?"No future wake is needed for the current queue work.":"Wake computer is off."}};return;}
 if(next==due&&timer){status["Queue"]=name;return;}
 if(!timer)timer=CreateWaitableTimerW(nullptr,TRUE,nullptr);
 auto fail=[&](DWORD error){if(timer){CancelWaitableTimer(timer);CloseHandle(timer);timer=nullptr;}due=0;status={{"Status","Unavailable"},{"Message","Windows could not register a wake timer (error "+std::to_string(error)+"). Scheduled downloads can still run while Windows is awake."},{"ErrorCode",error}};};
 if(!timer){fail(GetLastError());return;}
 constexpr i64 origin=116444736000000000LL;
 if(next<0||next>(std::numeric_limits<i64>::max()-origin)/10000){fail(ERROR_INVALID_PARAMETER);return;}
 LARGE_INTEGER target{};target.QuadPart=origin+next*10000;SetLastError(ERROR_SUCCESS);
 if(!SetWaitableTimer(timer,&target,0,nullptr,nullptr,TRUE)){fail(GetLastError());return;}
 auto error=GetLastError();due=next;
 FILETIME stamp{target.LowPart,(DWORD)target.HighPart};SYSTEMTIME time{};FileTimeToSystemTime(&stamp,&time);char formatted[48];sprintf_s(formatted,"%04u-%02u-%02uT%02u:%02u:%02u.%03uZ",time.wYear,time.wMonth,time.wDay,time.wHour,time.wMinute,time.wSecond,time.wMilliseconds);
 status={{"Status",error==ERROR_NOT_SUPPORTED?"Unsupported":"Armed"},{"NextWakeUtc",formatted},{"Queue",name},{"ErrorCode",error==ERROR_NOT_SUPPORTED?error:0},{"Message",error==ERROR_NOT_SUPPORTED?"Windows reports that wake from sleep is unsupported. The timer can run while Windows is awake.":"Wake timer requested. UDM must stay running; Windows power settings and hardware determine whether the computer wakes."}};
}
Json Manager::queueWakeStatus()const{Lock lock(mutex);return wakeTimer?wakeTimer->snapshot():Json{{"Status","Off"},{"Message","Wake computer is off."}};}
void Manager::updateWakeTimer(i64 now){if(stopping)return;if(!wakeTimer)wakeTimer=std::make_unique<QueueWakeTimer>();wakeTimer->update(state["Queues"],jobs,now);}
}
