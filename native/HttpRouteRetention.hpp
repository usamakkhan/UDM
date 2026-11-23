#pragma once
#include <map>
#include <memory>
#include <string>
namespace udm {
// Called while the containing session's mutex is held. Bounded resource owners
// retain existing routes at capacity; new routes remain request-owned.
template<class T,class Factory>
std::shared_ptr<T> retainHttpRoute(std::map<std::string,std::shared_ptr<T>>& routes,const std::string& key,bool bounded,Factory&& create){
 const auto found=routes.find(key);if(found!=routes.end())return found->second;
 if(bounded&&(routes.size()>=16||key.size()>1024*1024))return create();
 auto route=create();routes.emplace(key,route);return route;
}
}
