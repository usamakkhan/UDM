#pragma once
#include "CompletionCountdown.hpp"
#include "QueueCompletion.hpp"
#include "QueueLabels.hpp"
namespace udm {
struct QueueCompletionCountdownView {
 CompletionCountdownState state=CompletionCountdownState::Busy;
 Json shown=Json::object();
};
inline QueueCompletionCountdownView tryQueueCompletionCountdown(Manager& manager,const Json& completion,bool presentation=true){
 std::unique_lock lock(manager.mutex,std::try_to_lock);
 if(!lock.owns_lock()||manager.fileInfoDecisionPending)return {};
 if(!manager.completionEventReady(completion))return {CompletionCountdownState::Obsolete,{}};
 Json shown=Json::object();if(presentation){shown=completion;shown["Queue"]=queueDisplayLabel(queueDisplayLabels(manager.state["Queues"]),str(completion,"Queue"));}
 return {CompletionCountdownState::Ready,std::move(shown)};
}
// UI-owned progress, not an executable worker. A busy next-step read retains
// this exact cursor; successful earlier steps are never repeated on a wake.
struct QueueCompletionCountdownAction {
 Json completion;std::vector<std::string> steps;size_t nextStep=0;bool confirmed=false;
 explicit QueueCompletionCountdownAction(Json event):completion(std::move(event)),steps(queueEventSteps(completion)){}
};
}
