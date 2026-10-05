#pragma once
#include "CompletionPolicy.hpp"
namespace udm {
inline std::vector<std::string> queueCompletionSteps(const Json& q){
 std::vector<std::string> actions;
 if(q.contains("FinishActions"))actions=q.at("FinishActions").get<std::vector<std::string>>();
 else{auto one=str(q,"FinishAction","None");if(one!="None")actions.push_back(one);}
 for(auto& action:actions)if(action=="Open file")action="Open downloaded file";else if(action=="Open downloaded file")throw std::runtime_error("Use Open file for queue completion.");
 actions=validateCompletionSteps(actions);for(auto& action:actions)if(action=="Open downloaded file")action="Open file";return actions;
}
inline void setQueueCompletionSteps(Json& q,const std::vector<std::string>& steps,bool force){
 auto next=q;next["FinishActions"]=steps;auto ordered=queueCompletionSteps(next);next["FinishActions"]=ordered;next["FinishAction"]=ordered.empty()?"None":ordered.front();
 if(force&&std::find(ordered.begin(),ordered.end(),"Shut down")==ordered.end()&&std::find(ordered.begin(),ordered.end(),"Restart")==ordered.end())throw std::runtime_error("Force termination requires shutdown or restart.");
 next["FinishForce"]=force;q=std::move(next);
}
inline Json queueCompletionEvent(const Json& q){auto steps=queueCompletionSteps(q);return {{"Cycle",str(q,"CompletionCycle")},{"Queue",str(q,"Name")},{"Action",steps.empty()?"None":steps.front()},{"Actions",steps},{"Force",yes(q,"FinishForce")},{"DelaySeconds",num(q,"FinishDelaySeconds",30)},{"File",str(q,"FinishFile")}};}
inline std::string queueCompletionMessage(const Json& event){auto name=str(event,"Queue");auto failed=num(event,"FailedFiles");return failed>0?name+" finished processing. "+std::to_string(failed)+(failed==1?" file failed after the retry limit.":" files failed after the retry limit."):name+" completed successfully.";}
inline std::vector<std::string> queueEventSteps(const Json& event){Json q;if(event.contains("Actions"))q["FinishActions"]=event["Actions"];else q["FinishAction"]=str(event,"Action","None");return queueCompletionSteps(q);}
inline void executeQueueCompletion(const Json& event,const std::function<bool()>& ready,const std::function<void(const std::string&,bool,const std::string&)>& perform){
 for(const auto& step:queueEventSteps(event)){if(!ready())return;perform(step,yes(event,"Force"),str(event,"File"));}
}
}