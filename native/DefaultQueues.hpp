#pragma once
#include "Core.hpp"
#include <set>
namespace udm {
// One-time initialization preserves later deliberate deletion and custom queues.
inline void initializeDefaultQueues(Json& state){
 if(yes(state,"DefaultQueuesInitialized"))return;
 auto& queues=state["Queues"];std::set<std::string> names;Json* existing=nullptr;
 for(auto& queue:queues){
  auto name=lower(str(queue,"Name"));names.insert(name);
  if(name=="main queue"&&str(queue,"DefaultQueueRole").empty())queue["DefaultQueueRole"]="Download";
  if(yes(queue,"Synchronize")&&(str(queue,"DefaultQueueRole")=="Synchronization"||name=="synchronization queue"))existing=&queue;
 }
 if(existing)(*existing)["DefaultQueueRole"]="Synchronization";
 else{
  std::string name="Synchronization queue";for(unsigned suffix=2;names.count(lower(name));++suffix)name="Synchronization queue ("+std::to_string(suffix)+")";
  auto queue=defaultQueue(name);queue["Synchronize"]=true;queue["Enabled"]=false;queue["DefaultQueueRole"]="Synchronization";queues.push_back(std::move(queue));
 }
 state["DefaultQueuesInitialized"]=true;
}
}
