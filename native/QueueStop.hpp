#pragma once
#include "Core.hpp"
#include <ctime>
namespace udm {
inline bool independentStopEnabled(const Json& q){return yes(q,"StopWithoutStartEnabled")&&!yes(q,"Scheduled")&&!yes(q,"RunOnce");}
inline i64 nextIndependentStop(int minute,i64 now){
 if(minute<0||minute>1439)throw std::runtime_error("Stop time must be between 00:00 and 23:59.");
 auto local=[](i64 value){time_t seconds=value/1000;tm t{};if(localtime_s(&t,&seconds))throw std::runtime_error("Cannot calculate the local stop time.");return t;};
 auto previous=local(now);auto base=(now/60000)*60000;
 for(int i=1;i<=49*60;++i){auto candidate=base+(i64)i*60000;auto current=local(candidate);int before=previous.tm_hour*60+previous.tm_min,after=current.tm_hour*60+current.tm_min;
  bool sameDay=current.tm_year==previous.tm_year&&current.tm_yday==previous.tm_yday;
  if(after==minute||(sameDay&&before<minute&&after>minute&&after>before+1))return candidate;
  previous=current;
 }
 throw std::runtime_error("Cannot find the next local stop time.");
}
inline bool independentStopExpired(const Json& q,i64 now){
 if(!independentStopEnabled(q))return false;
 auto deadline=parseDate(q.value("StopWithoutStartUtc",Json()));return !deadline||now>=deadline;
}
inline void armIndependentStop(Json& q,const Json& previous,i64 now,bool force=false){
 if(!independentStopEnabled(q)){q["StopWithoutStartUtc"]=nullptr;return;}
 auto old=parseDate(previous.value("StopWithoutStartUtc",Json()));
 bool preserve=!force&&independentStopEnabled(previous)&&old&&num(q,"StopWithoutStartMinute",1020)==num(previous,"StopWithoutStartMinute",1020)&&!(yes(q,"Enabled",true)&&!yes(previous,"Enabled",true));
 q["StopWithoutStartUtc"]=preserve?previous["StopWithoutStartUtc"]:Json(date(nextIndependentStop((int)num(q,"StopWithoutStartMinute",1020),now)));
}
inline bool dailyRunsToCompletion(const Json& q){return yes(q,"Scheduled")&&!yes(q,"RunOnce")&&!yes(q,"DailyStopEnabled",true)&&num(q,"RepeatMinutes")==0;}
inline bool dailyManualStopExpired(const Json& q,i64 now){
 if(!yes(q,"Scheduled")||yes(q,"RunOnce")||dailyRunsToCompletion(q))return false;
 auto stop=parseDate(q.value("ManualDailyStopUtc",Json()));return stop&&now>=stop;
}
inline void armDailyManualStop(Json& q,const Json& previous,i64 now,bool active,bool force=false){
 if(!active||!yes(q,"Scheduled")||yes(q,"RunOnce")||dailyRunsToCompletion(q)){q["ManualDailyStopUtc"]=nullptr;return;}
 auto old=parseDate(previous.value("ManualDailyStopUtc",Json()));
 bool preserve=!force&&yes(previous,"Scheduled")&&!yes(previous,"RunOnce")&&old&&yes(previous,"DailyStopEnabled",true)==yes(q,"DailyStopEnabled",true)&&num(previous,"RepeatMinutes")==num(q,"RepeatMinutes")&&num(previous,"StopMinute",1440)==num(q,"StopMinute",1440);
 q["ManualDailyStopUtc"]=preserve?previous["ManualDailyStopUtc"]:Json(date(nextIndependentStop(yes(q,"DailyStopEnabled",true)?(int)num(q,"StopMinute",1440)%1440:0,now)));
}
inline bool nextDailyStartAfterManualStop(const Json& q,i64 now){
 if(!dailyManualStopExpired(q,now))return false;
 auto stop=parseDate(q.value("ManualDailyStopUtc",Json()));
 return now>=nextIndependentStop((int)num(q,"StartMinute"),stop);
}}
