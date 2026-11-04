#pragma once
#include "Core.hpp"
#include "QueueIndicatorState.hpp"

namespace udm {
struct NavigationTreeSnapshot {
 std::vector<std::string> categories,queueNames;
 std::vector<int> queueKinds;
 std::vector<std::pair<std::string,std::string>> projects;
 bool sameStructure(const NavigationTreeSnapshot& other)const{return categories==other.categories&&queueNames==other.queueNames&&projects==other.projects;}
 std::string structureKey()const{
  Json projectKey=Json::array();for(const auto& project:projects)projectKey.push_back({project.first,project.second});
  return Json{{"Categories",categories},{"Queues",queueNames},{"Projects",std::move(projectKey)}}.dump();
 }
};
inline NavigationTreeSnapshot navigationTreeSnapshot(Manager& manager){
 Lock lock(manager.mutex);NavigationTreeSnapshot result;result.categories=manager.categories();
 for(const auto& queue:manager.state["Queues"]){result.queueNames.push_back(str(queue,"Name"));result.queueKinds.push_back(queueIndicatorKind(queue));}
 for(const auto& project:manager.state["Projects"])result.projects.emplace_back(str(project,"Id"),str(project,"Name"));
 return result;
}
}
