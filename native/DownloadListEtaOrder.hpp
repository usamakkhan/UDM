#pragma once
#include "StreamProgress.hpp"

namespace udm {
struct DownloadListEtaKey {i64 seconds=0;bool complete=false;};
inline DownloadListEtaKey downloadListEtaKey(const Json& data,double speed){
 const auto rate=downloadListEtaRate(data,speed);
 const auto seconds=downloadSecondsLeftForState(data,static_cast<double>(rate),true);
 // The reference comparator defaults unavailable estimates to zero; its
 // separate cell-display rate threshold does not suppress the sort estimate.
 return {std::max<i64>(0,seconds),str(data,"Status")=="Complete"};
}
inline int compareDownloadListEta(const DownloadListEtaKey& left,const DownloadListEtaKey& right){
 if(left.seconds!=right.seconds)return left.seconds<right.seconds?-1:1;
 if(left.complete!=right.complete)return left.complete?-1:1;
 return 0;
}
}
