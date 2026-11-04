#pragma once
#include "Core.hpp"
namespace udm {
inline int queueIndicatorKind(const Json& queue){
 return (str(queue,"Name")=="Main queue"||str(queue,"DefaultQueueRole")=="Download"||str(queue,"DefaultQueueRole")=="Synchronization"?0:1)+(yes(queue,"Synchronize")?2:0)+((yes(queue,"Scheduled")||yes(queue,"RunOnce"))?4:0);
}
inline std::string queueIndicatorDescription(const Json& queue){
 return str(queue,"Name")+" — "+(yes(queue,"Synchronize")?"Synchronization queue":"Download queue")+((yes(queue,"Scheduled")||yes(queue,"RunOnce"))?"; scheduled start":"")+(!yes(queue,"Enabled",true)?"; stopped":"");
}
}
