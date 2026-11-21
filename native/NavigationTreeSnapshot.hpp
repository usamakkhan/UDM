#pragma once
#include "Core.hpp"
#include "QueueLabels.hpp"
#include "QueueIndicatorState.hpp"

namespace udm {
struct NavigationTreeSnapshot {
 std::vector<std::string> categories,queueNames;
 QueueLabels queueLabels;
 std::vector<int> queueKinds;
 std::map<std::string,std::string> queueDescriptions;
 std::vector<std::pair<std::string,std::string>> projects;
 bool sameStructure(const NavigationTreeSnapshot& other)const{return categories==other.categories&&queueNames==other.queueNames&&queueLabels==other.queueLabels&&projects==other.projects;}
 std::string structureKey()const{
  Json projectKey=Json::array();for(const auto& project:projects)projectKey.push_back({project.first,project.second});
  return Json{{"Categories",categories},{"Queues",queueNames},{"QueueLabels",queueLabels},{"Projects",std::move(projectKey)}}.dump();
 }
};
inline NavigationTreeSnapshot navigationTreeSnapshot(Manager& manager){
 Lock lock(manager.mutex);NavigationTreeSnapshot result;result.categories=manager.categories();result.queueLabels=queueDisplayLabels(manager.state["Queues"]);
 for(const auto& queue:manager.state["Queues"]){const auto name=str(queue,"Name");result.queueNames.push_back(name);result.queueKinds.push_back(queueIndicatorKind(queue));result.queueDescriptions.emplace(name,queueIndicatorDescription(queue,queueDisplayLabel(result.queueLabels,name)));}
 for(const auto& project:manager.state["Projects"])result.projects.emplace_back(str(project,"Id"),str(project,"Name"));
 return result;
}
}
