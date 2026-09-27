#pragma once
#include "Core.hpp"
#include <limits>
namespace udm {
inline bool sameScheduleMinute(const SYSTEMTIME& a,const SYSTEMTIME& b){return a.wYear==b.wYear&&a.wMonth==b.wMonth&&a.wDay==b.wDay&&a.wHour==b.wHour&&a.wMinute==b.wMinute;}
inline SYSTEMTIME localScheduleTime(i64 ms,const DYNAMIC_TIME_ZONE_INFORMATION* zone=nullptr){
 if(!ms)ms=epoch();constexpr i64 origin=116444736000000000LL;
 if(ms<-11644473600000LL||ms>(std::numeric_limits<i64>::max()-origin)/10000)throw std::runtime_error("Scheduled date is outside the supported range.");
 ULARGE_INTEGER value{};value.QuadPart=origin+ms*10000;FILETIME stamp{value.LowPart,value.HighPart};SYSTEMTIME utc{},local{};
 if(!FileTimeToSystemTime(&stamp,&utc)||!SystemTimeToTzSpecificLocalTimeEx(zone,&utc,&local))throw std::runtime_error("Cannot convert the scheduled date to local time.");return local;
}
inline i64 utcScheduleTime(SYSTEMTIME local,const DYNAMIC_TIME_ZONE_INFORMATION* zone=nullptr){
 SYSTEMTIME utc{},roundTrip{};FILETIME stamp{};
 if(!TzSpecificLocalTimeToSystemTimeEx(zone,&local,&utc)||!SystemTimeToTzSpecificLocalTimeEx(zone,&utc,&roundTrip)||!sameScheduleMinute(local,roundTrip)||local.wSecond!=roundTrip.wSecond||local.wMilliseconds!=roundTrip.wMilliseconds||!SystemTimeToFileTime(&utc,&stamp))throw std::runtime_error("This local time does not exist because of a clock change. Choose a valid scheduled time.");
 ULARGE_INTEGER value{};value.LowPart=stamp.dwLowDateTime;value.HighPart=stamp.dwHighDateTime;i64 result=(i64)(value.QuadPart/10000)-11644473600000LL;
 DYNAMIC_TIME_ZONE_INFORMATION current{};if(zone)current=*zone;else if(GetDynamicTimeZoneInformation(&current)==TIME_ZONE_ID_INVALID)return result;
 TIME_ZONE_INFORMATION rules{};
 if(!current.DynamicDaylightTimeDisabled&&GetTimeZoneInformationForYear(local.wYear,&current,&rules)){
  auto delta=((i64)rules.StandardBias-rules.DaylightBias)*60000;if(delta<0)delta=-delta;
  // Choose the first occurrence for a newly entered repeated autumn time.
  // Validate both offsets, including non-hour DST transitions.
  if(delta)for(auto candidate:{result-delta,result+delta}){auto other=localScheduleTime(candidate,zone);if(sameScheduleMinute(local,other)&&local.wSecond==other.wSecond&&local.wMilliseconds==other.wMilliseconds)result=std::min(result,candidate);}
 }
 return result;
}
inline i64 editedScheduleTime(SYSTEMTIME local,const Json& previous,const DYNAMIC_TIME_ZONE_INFORMATION* zone=nullptr){
 auto old=parseDate(previous);
 // Loading and applying an unchanged ambiguous autumn time must preserve the
 // originally saved occurrence, including seconds hidden by the minute picker.
 if(old&&sameScheduleMinute(local,localScheduleTime(old,zone)))return old;
 local.wSecond=0;local.wMilliseconds=0;return utcScheduleTime(local,zone);
}
}
