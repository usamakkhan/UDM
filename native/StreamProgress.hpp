#pragma once
#include "Core.hpp"
#include <algorithm>
#include <cmath>
namespace udm {
inline std::string downloadStatusText(const Json& d,const std::string& state,int permille){
  if(state=="Complete")return "Complete";if(state=="Failed")return "Error";
  if(permille<0||!(state=="Downloading"||state=="Paused"||state=="Pausing"||state=="Queued"))return state;
  double percent=permille/10.0;const auto total=num(d,"Size",-1);const auto done=num(d,"Received",-1);if(total>0&&done>=0&&done<=total&&state!="Complete")percent=std::min(99.99,100.0*done/total);
  char text[32];sprintf_s(text,"%.2f%%",percent);return text;
 }
inline std::string downloadTimeLeftText(i64 seconds){if(seconds<0)return "";if(seconds<60)return std::to_string(seconds)+" sec";if(seconds<3600)return std::to_string(seconds/60)+" min "+std::to_string(seconds%60)+" sec";return std::to_string(seconds/3600)+" hour(s) "+std::to_string(seconds%3600/60)+" min";}
inline int downloadPermille(const Json& data){
 if(str(data,"Status")=="Complete")return 1000;
 if(yes(data,"LiveRecording"))return -1;
 auto duration=num(data,"StreamDurationMs");
 if(duration>0)return (int)std::clamp(1000.0*num(data,"StreamCompletedMs")/duration,0.0,999.0);
 auto segments=num(data,"AdaptiveTotalSegments");
 if(segments>0)return (int)std::clamp(1000.0*num(data,"AdaptiveCompletedSegments")/segments,0.0,999.0);
 auto size=num(data,"Size",-1);return size>0?(int)std::clamp(1000.0*num(data,"Received")/size,0.0,1000.0):-1;
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
inline i64 downloadSecondsLeft(const Json& data,double speed){
 if(yes(data,"LiveRecording"))return -1;
 if(str(data,"Status")!="Downloading"||!std::isfinite(speed)||speed<=0)return -1;
 auto duration=num(data,"StreamDurationMs"),done=num(data,"StreamCompletedMs");
 if(duration>0){auto elapsed=real(data,"StreamElapsedSeconds");const auto baseline=num(data,"StreamResumedMs");if(done<=baseline||done>=duration||!std::isfinite(elapsed)||elapsed<=0)return -1;
  return (i64)std::clamp(elapsed*(duration-done)/(done-baseline),0.0,86400.0);
 }
 auto size=num(data,"Size",-1),received=num(data,"Received");return size>received?(i64)((size-received)/speed):-1;
}
}
