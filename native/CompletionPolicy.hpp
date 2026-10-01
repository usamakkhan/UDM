#pragma once
#include "Core.hpp"
#include <algorithm>
#include <set>
namespace udm {
inline bool completionPower(const std::string& action){return action=="Sleep"||action=="Hibernate"||action=="Shut down"||action=="Restart";}
inline std::vector<std::string> validateCompletionSteps(const std::vector<std::string>& requested){
 const std::vector<std::string> order={"Open downloaded file","Disconnect dial-up / VPN","Sleep","Hibernate","Shut down","Restart","Exit UDM"};
 std::set<std::string> unique;int power=0;
 for(const auto& action:requested){if(std::find(order.begin(),order.end(),action)==order.end()||!unique.insert(action).second)throw std::runtime_error("Choose distinct supported completion actions.");if(completionPower(action)&&++power>1)throw std::runtime_error("Choose only one computer power action.");}
 std::vector<std::string> result;for(const auto& action:order)if(unique.count(action))result.push_back(action);return result;
}
inline std::vector<std::string> completionSteps(const Json& record){
 if(record.contains("CompletionActions"))return validateCompletionSteps(record.at("CompletionActions").get<std::vector<std::string>>());
 auto action=str(record,"CompletionAction","None");return validateCompletionSteps(action=="None"?std::vector<std::string>{}:std::vector<std::string>{action});
}
inline bool completionCanForce(const std::vector<std::string>& steps){return std::find(steps.begin(),steps.end(),"Shut down")!=steps.end()||std::find(steps.begin(),steps.end(),"Restart")!=steps.end();}
inline std::string completionSummary(const std::vector<std::string>& steps){std::string text;for(auto& action:steps){if(!text.empty())text+="; ";text+=action;}return text.empty()?"None":text;}
inline UINT completionShutdownFlags(const std::string& action,bool force){
 if(action!="Shut down"&&action!="Restart")throw std::runtime_error("This action does not use Windows shutdown.");
 return (action=="Restart"?EWX_REBOOT:EWX_POWEROFF)|(force?EWX_FORCE:0);
}
}
