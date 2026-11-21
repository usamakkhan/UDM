#pragma once
#include "AuthenticationPromptTask.hpp"
namespace udm {
// A requested prompt is a read, not an automatic pending-flag claim. Cancel,
// stale results and joined teardown must leave the original request intact.
struct RequestedAuthenticationView {AuthenticationPromptView prompt;bool queueOnly=true;};
using RequestedAuthenticationViewPtr=std::shared_ptr<const RequestedAuthenticationView>;
inline bool requestedAuthenticationEligible(Manager& manager,const JobPtr& job,bool queueOnly){
 return job&&std::find(manager.jobs.begin(),manager.jobs.end(),job)!=manager.jobs.end()&&!manager.isActive(job)&&
  (queueOnly?(yes(job->data,"QueueOrigin")&&!yes(manager.state["Settings"],"SuppressAuthenticationDialog")&&authenticationDialogReady(job->data)):canRequestLogin(job->data));
}
inline bool requestedAuthenticationMatches(Manager& manager,const RequestedAuthenticationView& view){
 const auto& prompt=view.prompt;
 return requestedAuthenticationEligible(manager,prompt.job,view.queueOnly)&&authenticationPromptKeyMatches(prompt.job->data,prompt.key)&&
  yes(prompt.job->data,"AuthenticationPromptPending")==prompt.expectedPending&&yes(prompt.job->data,"QueueOrigin")==prompt.queueOrigin&&authenticationLoginSourcesMatch(manager.state["Settings"],prompt.loginSources);
}
inline RequestedAuthenticationViewPtr prepareRequestedAuthentication(Manager& manager,const JobPtr& job,bool queueOnly=true,const AuthenticationPromptObserver& observe={}){
 auto view=std::make_shared<RequestedAuthenticationView>();view->queueOnly=queueOnly;view->prompt.job=job;
 {Lock lock(manager.mutex);if(!requestedAuthenticationEligible(manager,job,queueOnly))return {};
  view->prompt.key=authenticationPromptKey(job->data);view->prompt.loginSources=authenticationLoginSources(manager.state["Settings"]);
  view->prompt.expectedPending=yes(job->data,"AuthenticationPromptPending");view->prompt.queueOrigin=yes(job->data,"QueueOrigin");}
 if(observe)observe("captured");
 view->prompt.initial=basicLogin(siteRequestHeaders(str(view->prompt.key,"Url"),readHeaders(view->prompt.key),view->prompt.loginSources));
 if(observe)observe("resolved");
 {Lock lock(manager.mutex);if(!requestedAuthenticationMatches(manager,*view))return {};}
 return view;
}
inline AutomaticPromptReadiness tryRequestedAuthenticationCurrent(Manager& manager,const RequestedAuthenticationView& view){
 std::unique_lock lock(manager.mutex,std::try_to_lock);if(!lock.owns_lock()||manager.fileInfoDecisionPending)return AutomaticPromptReadiness::Busy;
 return requestedAuthenticationMatches(manager,view)?AutomaticPromptReadiness::Ready:AutomaticPromptReadiness::Obsolete;
}
class RequestedAuthenticationTask {
 Manager& manager;AuthenticationPromptObserver observer;std::future<RequestedAuthenticationViewPtr> result;std::thread worker;bool running=false;RequestedAuthenticationViewPtr view;
public:
 explicit RequestedAuthenticationTask(Manager& owner,AuthenticationPromptObserver observe={}):manager(owner),observer(std::move(observe)){}
 RequestedAuthenticationTask(const RequestedAuthenticationTask&)=delete;RequestedAuthenticationTask& operator=(const RequestedAuthenticationTask&)=delete;
 ~RequestedAuthenticationTask(){finish();}
 bool active()const noexcept{return running;}
 bool ownsPrompt()const noexcept{return running||bool(view);}
 bool ready(){return running&&result.valid()&&result.wait_for(std::chrono::seconds(0))==std::future_status::ready;}
 RequestedAuthenticationViewPtr prepared()const{return running?RequestedAuthenticationViewPtr{}:view;}
 void prepare(const JobPtr& job,bool queueOnly=true,std::function<void()> notify={}){
  if(ownsPrompt())throw std::runtime_error("Finish the requested login prompt first.");
  std::promise<RequestedAuthenticationViewPtr> promise;auto future=promise.get_future();
  worker=std::thread([this,job,queueOnly,promise=std::move(promise),notify=std::move(notify)]()mutable{try{promise.set_value(prepareRequestedAuthentication(manager,job,queueOnly,observer));}catch(...){promise.set_exception(std::current_exception());}if(notify)try{notify();}catch(...) {}});
  result=std::move(future);running=true;
 }
 bool poll(){if(!ready())return false;if(worker.joinable())worker.join();running=false;view=result.get();return true;}
 void acknowledge(){if(running)throw std::runtime_error("The requested login view is not ready.");view.reset();}
 void finish()noexcept{if(worker.joinable())worker.join();running=false;view.reset();result={};}
};
}
