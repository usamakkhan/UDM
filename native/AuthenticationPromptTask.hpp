#pragma once
#include "AutomaticPromptSnapshot.hpp"
#include "SiteLogins.hpp"
#include <future>
namespace udm {
inline constexpr const char* AuthenticationPromptFields[]={"Id","Url","Status","LastAttempt","AuthenticationOrigin","AuthenticationScheme","LastHttpStatus","LastFtpStatus","SyncLastHttpStatus","SyncLastFtpStatus","SyncRetryFailed","ProtectedHeaders","ProtectedBrowserSession","SourceUrl","ProtectedAdaptive"};
inline Json authenticationPromptKey(const Json& data){
 Json result=Json::object();
 for(const auto* field:AuthenticationPromptFields){const auto found=data.find(field);if(found!=data.end())result[field]=*found;}
 return result;
}
inline bool authenticationPromptKeyMatches(const Json& data,const Json& key){
 if(!key.is_object())return false;size_t fields=0;
 for(const auto* field:AuthenticationPromptFields){const auto left=data.find(field),right=key.find(field);if(left==data.end()){if(right!=key.end())return false;}else{++fields;if(right==key.end()||*left!=*right)return false;}}
 return fields==key.size();
}
inline Json authenticationLoginSources(const Json& prefs){const auto found=prefs.find("SiteLogins");return found==prefs.end()?Json::object():Json{{"SiteLogins",*found}};}
inline bool authenticationLoginSourcesMatch(const Json& prefs,const Json& sources){
 if(!sources.is_object())return false;const auto current=prefs.find("SiteLogins"),captured=sources.find("SiteLogins");
 return current==prefs.end()?sources.empty():sources.size()==1&&captured!=sources.end()&&*current==*captured;
}
struct AuthenticationPromptView {JobPtr job;Json key,loginSources;std::pair<std::string,std::string> initial;bool expectedPending=false,queueOrigin=false;};
using AuthenticationPromptViewPtr=std::shared_ptr<const AuthenticationPromptView>;
using AuthenticationPromptObserver=std::function<void(const char*)>;
inline bool automaticAuthenticationEligible(Manager& manager,const JobPtr& job){
 return job&&std::find(manager.jobs.begin(),manager.jobs.end(),job)!=manager.jobs.end()&&!yes(manager.state["Settings"],"SuppressAuthenticationDialog")&&!manager.isActive(job)&&!yes(job->data,"QueueOrigin")&&authenticationDialogReady(job->data);
}
inline AuthenticationPromptViewPtr prepareAuthenticationPrompt(Manager& manager,const JobPtr& job,const AuthenticationPromptObserver& observe={}){
 auto view=std::make_shared<AuthenticationPromptView>();view->job=job;
 {Lock lock(manager.mutex);if(manager.fileInfoDecisionPending||!automaticAuthenticationEligible(manager,job))return {};
  view->key=authenticationPromptKey(job->data);view->loginSources=authenticationLoginSources(manager.state["Settings"]);
 }
 if(observe)observe("captured");
 view->initial=basicLogin(siteRequestHeaders(str(view->key,"Url"),readHeaders(view->key),view->loginSources));
 {Lock lock(manager.mutex);
  if(manager.fileInfoDecisionPending||!automaticAuthenticationEligible(manager,job)||!authenticationPromptKeyMatches(job->data,view->key)||!authenticationLoginSourcesMatch(manager.state["Settings"],view->loginSources))return {};
  if(observe)observe("before-claim");
  if(tryClaimAutomaticAuthentication(manager,job)!=AutomaticPromptReadiness::Ready)return {};
 }
 // No operation capable of throwing is allowed after the durable claim.
 return view;
}
inline AutomaticPromptReadiness tryAuthenticationPromptCurrent(Manager& manager,const AuthenticationPromptView& view){
 std::unique_lock lock(manager.mutex,std::try_to_lock);if(!lock.owns_lock()||manager.fileInfoDecisionPending)return AutomaticPromptReadiness::Busy;
 const auto& job=view.job;
 if(!job||std::find(manager.jobs.begin(),manager.jobs.end(),job)==manager.jobs.end()||!authenticationPromptKeyMatches(job->data,view.key)||yes(job->data,"AuthenticationPromptPending")||manager.isActive(job)||yes(job->data,"QueueOrigin")||yes(manager.state["Settings"],"SuppressAuthenticationDialog")||!authenticationLoginSourcesMatch(manager.state["Settings"],view.loginSources))return AutomaticPromptReadiness::Obsolete;
 return AutomaticPromptReadiness::Ready;
}
// Restoration is also a durable command. Keep true in memory if its save
// fails; the owner retains the view and retries the unfinished barrier.
inline bool rearmAuthenticationPrompt(Manager& manager,const AuthenticationPromptView& view){
 Lock lock(manager.mutex);const auto& job=view.job;
 if(!job||std::find(manager.jobs.begin(),manager.jobs.end(),job)==manager.jobs.end()||!authenticationPromptKeyMatches(job->data,view.key)||manager.isActive(job))return true;
 if(manager.fileInfoDecisionPending)return false;
 job->data["AuthenticationPromptPending"]=true;manager.save();return true;
}
inline Json saveAuthenticationLogin(Manager& manager,const AuthenticationPromptView& view,const std::string& user,const std::string& password,bool remember,bool expectedPending=false,const AuthenticationPromptObserver& observe={}){
 Lock lock(manager.mutex);const auto& job=view.job;
 if(!job||std::find(manager.jobs.begin(),manager.jobs.end(),job)==manager.jobs.end()||manager.isActive(job)||yes(job->data,"QueueOrigin")!=view.queueOrigin||yes(job->data,"AuthenticationPromptPending")!=expectedPending||!authenticationPromptKeyMatches(job->data,view.key))throw std::runtime_error("The download or login challenge changed. Close this dialog and review its current request.");
 if(observe)observe("before-save");manager.setDownloadLogin(job,user,password,remember);return authenticationPromptKey(job->data);
}
inline void applyPreparedAuthenticationLogin(Manager& manager,const AuthenticationPromptView& view,const std::string& user,const std::string& password,bool remember){
 const auto expected=saveAuthenticationLogin(manager,view,user,password,remember);
 if(!manager.resumeIf(view.job,[&]{return std::find(manager.jobs.begin(),manager.jobs.end(),view.job)!=manager.jobs.end()&&authenticationPromptKeyMatches(view.job->data,expected);}))throw std::runtime_error("The login was saved, but the download changed before retrying it.");
}
// All methods except the action itself belong to the UI owner. A result is
// retained until presentation acknowledges it or a joined rearm completes.
class AuthenticationPromptTask {
 enum class Kind {None,Prepare,Rearm};
 struct Outcome {AuthenticationPromptViewPtr view;bool settled=true;};
 Manager& manager;AuthenticationPromptObserver observer;std::future<Outcome> result;std::thread worker;Kind kind=Kind::None;
 AuthenticationPromptViewPtr view;bool needsRearm=false;
 void start(Kind selected,std::function<Outcome()> action,std::function<void()> notify){
  if(active())throw std::runtime_error("An authentication command is already running.");
  std::promise<Outcome> promise;auto future=promise.get_future();
  worker=std::thread([promise=std::move(promise),action=std::move(action),notify=std::move(notify)]()mutable{try{promise.set_value(action());}catch(...){promise.set_exception(std::current_exception());}if(notify)try{notify();}catch(...) {}});
  result=std::move(future);kind=selected;
 }
public:
 explicit AuthenticationPromptTask(Manager& owner,AuthenticationPromptObserver observe={}):manager(owner),observer(std::move(observe)){}
 AuthenticationPromptTask(const AuthenticationPromptTask&)=delete;AuthenticationPromptTask& operator=(const AuthenticationPromptTask&)=delete;
 ~AuthenticationPromptTask(){try{finish();}catch(...) {/* Rearm failures retain pending=true for the Manager's final save. */}}
 bool active()const noexcept{return kind!=Kind::None;}
 bool ownsPrompt()const noexcept{return active()||bool(view);}
 bool rearmPending()const noexcept{return needsRearm;}
 bool ready(){return active()&&result.valid()&&result.wait_for(std::chrono::seconds(0))==std::future_status::ready;}
 AuthenticationPromptViewPtr prepared()const{return !active()&&!needsRearm?view:AuthenticationPromptViewPtr{};}
 void prepare(const JobPtr& job,std::function<void()> notify={}){
  if(ownsPrompt())throw std::runtime_error("Finish the current authentication presentation first.");
  start(Kind::Prepare,[this,job]{return Outcome{prepareAuthenticationPrompt(manager,job,observer),true};},std::move(notify));
 }
 bool poll(){
  if(!ready())return false;if(worker.joinable())worker.join();const auto completed=kind;kind=Kind::None;
  try{auto outcome=result.get();if(completed==Kind::Prepare)view=std::move(outcome.view);else if(outcome.settled){view.reset();needsRearm=false;}return true;}
  catch(...){if(completed==Kind::Rearm)needsRearm=true;throw;}
 }
 void acknowledge(){if(active()||needsRearm)throw std::runtime_error("Authentication work is not ready for acknowledgement.");view.reset();}
 void rearm(std::function<void()> notify={}){
  if(!view||active())return;needsRearm=true;const auto pending=view;
  start(Kind::Rearm,[this,pending]{return Outcome{{},rearmAuthenticationPrompt(manager,*pending)};},std::move(notify));
 }
 void finish(){
  if(worker.joinable())worker.join();if(active())poll();
  if(view){needsRearm=true;if(!rearmAuthenticationPrompt(manager,*view))throw std::runtime_error("Finish the pending download decision before closing UDM.");view.reset();needsRearm=false;}
 }
};
}
