#pragma once
#include "Core.hpp"
#include <algorithm>
#include <cmath>
namespace udm {
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
