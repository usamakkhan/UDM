#pragma once
#include "Core.hpp"
namespace udm {
// A pure projection of the existing scheduler: no new queue is started here.
i64 nextQueueWake(const Json&,i64 now);
bool queueHasWakeWork(const Json&,const std::vector<JobPtr>&);
class QueueWakeTimer {
 HANDLE timer=nullptr;
 i64 due=0,lastCheck=0;
 std::string signature;
 Json status={{"Status","Off"},{"Message","Wake computer is off."}};
public:
 ~QueueWakeTimer();
 void update(const Json& queues,const std::vector<JobPtr>& jobs,i64 now);
 void stop();
 Json snapshot()const{return status;}
 bool signaled()const{return timer&&WaitForSingleObject(timer,0)==WAIT_OBJECT_0;}
};
}
