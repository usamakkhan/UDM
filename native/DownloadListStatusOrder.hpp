#pragma once
#include "Core.hpp"
#include "IdmTextSort.hpp"
#include <algorithm>

namespace udm {
struct DownloadListStatusKey {bool complete=false;double progress=0;std::string label;};
inline DownloadListStatusKey downloadListStatusKey(const Json& data){
 DownloadListStatusKey key;
 const auto state=str(data,"Status");key.complete=state=="Complete";
 if(key.complete)return key;
 // Compare underlying coverage, not the rounded percentage displayed in the UI.
 // UDM's media coverage fields are the equivalents of the reference stream clock.
 if(!yes(data,"LiveRecording")){
  const auto duration=num(data,"StreamDurationMs"),segments=num(data,"AdaptiveTotalSegments"),size=num(data,"Size",-1);
  // The traced media branch uses a percent score (capped at 100.1), whereas
  // ordinary byte downloads use a ratio. Preserve that reference distinction.
  if(duration>0)key.progress=std::clamp(100.0*static_cast<double>(num(data,"StreamCompletedMs"))/duration,0.0,100.1);
  else if(segments>0)key.progress=std::clamp(100.0*static_cast<double>(num(data,"AdaptiveCompletedSegments"))/segments,0.0,100.1);
  else if(size>0)key.progress=std::max(0.0,static_cast<double>(num(data,"Received"))/size);
 }
 if(key.progress==0&&num(data,"Received")<=0&&state=="Failed")key.label=num(data,"LastHttpStatus")==404?"Not Found":"Error";
 return key;
}
inline int compareDownloadListStatus(const DownloadListStatusKey& left,const DownloadListStatusKey& right,bool byteFallback){
 // Ascending is the negation of the reference's primary (descending) callback:
 // unfinished rows first, ascending coverage, then Complete rows as a stable group.
 if(left.complete!=right.complete)return left.complete?1:-1;
 if(left.complete)return 0;
 if(left.progress!=right.progress)return left.progress<right.progress?-1:1;
 if(left.progress>0)return 0;
 // The descending reference callback uses ascending optional-text comparison
 // only when both progress scores are zero; its wrapper reverses that too.
 const int label=compareOptionalAnsiText(left.label,right.label,byteFallback);
 return label<0?1:label>0?-1:0;
}
}
