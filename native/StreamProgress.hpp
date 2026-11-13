#pragma once
#include "Core.hpp"
#include "HttpRepresentation.hpp"
#include <algorithm>
#include <cmath>
#include <limits>
namespace udm {
inline std::atomic<unsigned long long>& downloadStatusLocaleGeneration(){static std::atomic<unsigned long long> generation{1};return generation;}
inline void invalidateDownloadStatusLocale(){downloadStatusLocaleGeneration().fetch_add(1,std::memory_order_relaxed);}
inline const std::string& downloadStatusDecimalSeparator(){
 struct Cache {unsigned long long generation=0;std::string value;};
 thread_local Cache cache;
 const auto generation=downloadStatusLocaleGeneration().load(std::memory_order_relaxed);
 if(cache.generation!=generation){
  wchar_t separator[4]{};
  const int count=GetLocaleInfoW(LOCALE_USER_DEFAULT,LOCALE_SDECIMAL,separator,4);
  cache.value=count>1?utf8(std::wstring(separator,static_cast<size_t>(count-1))):",";
  cache.generation=generation;
 }
 return cache.value;
}
inline std::string downloadStatusPercentage(i64 completed,i64 total){
 if(completed<=0||total<=0)return {};
 // Reference list rendering stores the percentage as float, then truncates its
 // whole part and fractional hundredths separately instead of rounding printf.
 const float percent=static_cast<float>(100.0*static_cast<double>(completed)/static_cast<double>(total));
 if(!std::isfinite(percent)||percent<0||percent>=105.0f)return {};
 const int whole=static_cast<int>(percent);
 const int tail=static_cast<int>((static_cast<double>(percent)-whole)*100.0);
 return std::to_string(whole)+downloadStatusDecimalSeparator()+(tail<10?"0":"")+std::to_string(tail)+"%";
}
inline i64 adaptiveSourceBytes(const Json& data){
 return yes(data,"AdaptiveRefreshPendingValidation")?0:num(data,"AdaptiveSourceBytes");
}
inline std::string downloadStatusText(const Json& d,const std::string& state,int /*permille*/){
 if(state=="Complete")return "Complete";
 if(!(state=="Downloading"||state=="Paused"||state=="Pausing"||state=="Queued"||state=="Failed"))return state;
 const auto failure=num(d,"LastHttpStatus")==404?"Not Found":"Error";
 if(yes(d,"LiveRecording"))return state=="Failed"?failure:state;
 const auto duration=num(d,"StreamDurationMs"),segments=num(d,"AdaptiveTotalSegments"),source=adaptiveSourceBytes(d);
 const auto total=duration>0?duration:source>0?source:segments>0?segments:httpOutputSize(d);
 const auto completed=duration>0?num(d,"StreamCompletedMs"):source<=0&&segments>0?num(d,"AdaptiveCompletedSegments"):httpOutputReceived(d);
 const auto percent=downloadStatusPercentage(completed,total);
 if(!percent.empty())return percent;
 // Positive retained progress takes precedence over textual failure labels in
 // the reference. Unknown/invalid coverage cannot supply a percentage.
 if(state=="Failed"&&completed<=0&&num(d,"Received")<=0)return failure;
 return {};
 }
inline std::string downloadTimeLeftText(i64 seconds){if(seconds<0)return "";if(seconds/3600>10000)return " ";if(seconds<60)return std::to_string(seconds)+" sec";if(seconds<3600)return std::to_string(seconds/60)+" min "+std::to_string(seconds%60)+" sec";return std::to_string(seconds/3600)+" hour(s) "+std::to_string(seconds%3600/60)+" min";}
inline int downloadPermille(const Json& data){
 if(str(data,"Status")=="Complete")return 1000;
 if(yes(data,"LiveRecording"))return -1;
 auto duration=num(data,"StreamDurationMs");
 if(duration>0)return (int)std::clamp(1000.0*num(data,"StreamCompletedMs")/duration,0.0,999.0);
 if(const auto source=adaptiveSourceBytes(data);source>0)
  return (int)std::clamp(1000.0*num(data,"Received")/source,0.0,999.0);
 auto segments=num(data,"AdaptiveTotalSegments");
 if(segments>0)return (int)std::clamp(1000.0*num(data,"AdaptiveCompletedSegments")/segments,0.0,999.0);
 auto size=httpOutputSize(data);return size>0?(int)std::clamp(1000.0*httpOutputReceived(data)/size,0.0,1000.0):-1;
}
inline std::string mediaClock(i64 milliseconds){
 auto seconds=std::max<i64>(0,milliseconds)/1000;auto tail=std::to_string(seconds%60);
 return std::to_string(seconds/60)+":"+(tail.size()<2?"0":"")+tail;
}
// IDM shows a media download's stream length in the Size column while its byte size is
// unknown: "2 hour(s) 3 min", "3 min 20 sec" or "20 sec". MediaLengthMs is recorded for
// HLS/DASH plans whose segments all carry durations; SABR records carry StreamDurationMs.
inline std::string streamLengthText(i64 milliseconds){
 auto seconds=std::max<i64>(0,milliseconds+500)/1000;const auto hours=seconds/3600,minutes=(seconds%3600)/60;seconds%=60;
 if(hours>0)return std::to_string(hours)+" hour(s) "+std::to_string(minutes)+" min";
 if(minutes>0)return std::to_string(minutes)+" min "+std::to_string(seconds)+" sec";
 return std::to_string(seconds)+" sec";
}
inline std::string sizeCellText(const Json& data,i64 size){
 if(size>=0)return bytes((double)size);if(yes(data,"LiveRecording"))return "";
 auto length=num(data,"MediaLengthMs");if(length<=0)length=num(data,"StreamDurationMs");return length>0?streamLengthText(length):"";
}
inline std::string streamProgressText(const Json& data){
 if(yes(data,"LiveRecording"))return "   Recorded: "+mediaClock((i64)(std::max(0.0,real(data,"LiveCapturedSeconds"))*1000));
 auto duration=num(data,"StreamDurationMs");if(duration<=0)return "";
 return "   Media: "+mediaClock(std::clamp<i64>(num(data,"StreamCompletedMs"),0,duration))+" / "+mediaClock(duration);
}
inline i64 boundedDownloadEta(double seconds){
 if(std::isnan(seconds)||seconds<0)return -1;
 // double cannot represent INT64_MAX exactly. Guard before casting, including
 // positive overflow from an otherwise valid finite rate/elapsed calculation.
 if(!std::isfinite(seconds)||seconds>=static_cast<double>(std::numeric_limits<i64>::max()))return std::numeric_limits<i64>::max();
 return static_cast<i64>(seconds);
}
inline i64 downloadSecondsLeftForState(const Json& data,double speed,bool includeStopped){
 if(yes(data,"LiveRecording")||pendingHttpGzip(data))return -1;
 const auto state=str(data,"Status");const bool eligible=state=="Downloading"||(includeStopped&&(state=="Paused"||state=="Failed"||state=="Queued"));
 if(!eligible||!std::isfinite(speed)||speed<=0)return -1;
 auto duration=num(data,"StreamDurationMs"),done=num(data,"StreamCompletedMs");
 if(duration>0){const auto baseline=num(data,"StreamResumedMs");
  if(done<0||done>duration||baseline<0||baseline>duration)return -1;
  if(done==duration)return 0;
  const auto elapsed=real(data,"StreamElapsedSeconds");if(done<=baseline||!std::isfinite(elapsed)||elapsed<=0)return -1;
  return boundedDownloadEta((static_cast<double>(duration-done)/static_cast<double>(done-baseline))*elapsed);
 }
 const auto source=adaptiveSourceBytes(data);
 const auto size=source>0?source:num(data,"Size",-1),received=num(data,"Received");
 if(size<=0||received<0||received>size)return -1;
 return boundedDownloadEta(static_cast<double>(size-received)/speed);
}
inline i64 downloadSecondsLeft(const Json& data,double speed){return downloadSecondsLeftForState(data,speed,false);}
inline i64 downloadListIntegerRate(double speed){
 if(!std::isfinite(speed)||speed<1)return 0;
 if(speed>=static_cast<double>(std::numeric_limits<i64>::max()))return std::numeric_limits<i64>::max();
 return static_cast<i64>(speed);
}
inline i64 downloadListEtaRate(const Json& data,double speed){
 // The reference Time Left comparator starts with the record's cached rate
 // and replaces it only when the active session supplies a nonzero rate.
 const auto live=str(data,"Status")=="Downloading"?downloadListIntegerRate(speed):0;
 return live>0?live:std::max<i64>(0,num(data,"LastTransferRate"));
}
inline i64 downloadListSecondsLeft(const Json& data,double speed){
 // The reference list uses an integer rate and suppresses ETA at 10 B/s or less.
 // Progress dialogs keep the valid fractional-rate estimate above.
 const auto rate=downloadListEtaRate(data,speed);if(rate<=10)return -1;
 return downloadSecondsLeftForState(data,static_cast<double>(rate),true);
}
}
