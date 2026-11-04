#pragma once
#include "Core.hpp"
#include <algorithm>

namespace udm {
struct DownloadListSizeKey {i64 bytes=0,durationMs=0;};
inline DownloadListSizeKey downloadListSizeKey(const Json& data,i64 displayedBytes){
 if(displayedBytes>0)return {displayedBytes,0};
 // The reference uses duration only when neither row has a positive byte size.
 // Match the duration source shown by sizeCellText without rounding its key.
 if(yes(data,"LiveRecording"))return {};
 auto duration=num(data,"MediaLengthMs");if(duration<=0)duration=num(data,"StreamDurationMs");
 return {0,std::max<i64>(0,duration)};
}
inline int compareDownloadListSize(const DownloadListSizeKey& left,const DownloadListSizeKey& right){
 if(left.bytes!=right.bytes)return left.bytes<right.bytes?-1:1;
 if(left.bytes>0)return 0;
 return left.durationMs<right.durationMs?-1:left.durationMs>right.durationMs?1:0;
}
}
