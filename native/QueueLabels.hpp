#pragma once
#include "Core.hpp"

namespace udm {
using QueueLabels=std::map<std::string,std::string>;

inline QueueLabels queueDisplayLabels(const Json& queues){
 QueueLabels labels;
 std::set<std::string> used;
 for(const auto& queue:queues){
  const auto role=str(queue,"DefaultQueueRole");
  const auto label=role=="Download"?"Main download queue":role=="Synchronization"?"Synchronization queue":"";
  if(*label&&used.insert(lower(label)).second)labels.emplace(str(queue,"Name"),label);
 }
 std::vector<std::string> collisions;
 for(const auto& queue:queues){
  const auto name=str(queue,"Name");
  if(labels.count(name))continue;
  if(used.insert(lower(name)).second)labels.emplace(name,name);
  else collisions.push_back(name);
 }
 for(const auto& name:collisions){
  auto label=name+" (custom)";
  while(!used.insert(lower(label)).second)label+=" (custom)";
  labels.emplace(name,label);
 }
 return labels;
}

inline QueueLabels queueDisplayLabels(Manager& manager){
 Lock lock(manager.mutex);
 return queueDisplayLabels(manager.state["Queues"]);
}

inline std::string queueDisplayLabel(const QueueLabels& labels,const std::string& name){
 const auto found=labels.find(name);
 return found==labels.end()?name:found->second;
}
}
