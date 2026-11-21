#pragma once
#include "AuthenticationPromptTask.hpp"
#include "FileInfoDecision.hpp"
namespace udm {
struct AuthenticationLoginDraft {AuthenticationPromptView view;std::string user,password;bool remember=false,expectedPending=false;};
struct AuthenticationLoginOutcome {Json savedKey;bool queued=false;std::string warning;};
inline AuthenticationLoginOutcome applyAuthenticationLoginCommand(Manager& manager,const AuthenticationLoginDraft& draft,const AuthenticationPromptObserver& observe={}){
 AuthenticationLoginOutcome outcome;outcome.savedKey=saveAuthenticationLogin(manager,draft.view,draft.user,draft.password,draft.remember,draft.expectedPending,observe);
 try{
  if(observe)observe("credentials-saved");
  if(!manager.resumeIf(draft.view.job,[&]{return std::find(manager.jobs.begin(),manager.jobs.end(),draft.view.job)!=manager.jobs.end()&&yes(draft.view.job->data,"QueueOrigin")==draft.view.queueOrigin&&authenticationPromptKeyMatches(draft.view.job->data,outcome.savedKey);})){
   outcome.warning="The login was saved, but the download changed before retrying it. Close this dialog and review its current request.";return outcome;
  }
  outcome.queued=true;
  try{manager.dispatchQueueWork();}catch(const std::exception& e){outcome.warning="The login was saved and retry was queued, but immediate scheduling failed. "+std::string(e.what());}
 }catch(const std::exception& e){outcome.warning="The login was saved, but retry could not start. "+std::string(e.what());}
 return outcome;
}
// Reuse the existing joined decision worker and presentation guard. The UI
// captures a complete draft; no controls or HWNDs enter the command action.
class AuthenticationLoginTask {
 FileInfoDecisionTask task;std::shared_ptr<AuthenticationLoginOutcome> outcome;
public:
 AuthenticationLoginTask()=default;AuthenticationLoginTask(const AuthenticationLoginTask&)=delete;AuthenticationLoginTask& operator=(const AuthenticationLoginTask&)=delete;
 bool active()const noexcept{return task.valid();}
 bool ready(){return task.valid()&&task.ready();}
 void start(Manager& manager,AuthenticationLoginDraft draft,std::function<void()> notify={},AuthenticationPromptObserver observe={}){
  if(active())throw std::runtime_error("The login command is already running.");auto next=std::make_shared<AuthenticationLoginOutcome>();
  task.startAction(manager,[&manager,draft=std::move(draft),next,observe=std::move(observe)]{*next=applyAuthenticationLoginCommand(manager,draft,observe);return FileInfoDecisionResult{next->warning};},std::move(notify));outcome=std::move(next);
 }
 AuthenticationLoginOutcome take(){
  if(!ready())throw std::runtime_error("The login command has not finished.");
  try{task.get();task.release();auto result=std::move(*outcome);outcome.reset();return result;}
  catch(...){task.release();outcome.reset();throw;}
 }
 void finish(){if(!active())return;task.release();outcome.reset();}
};
}
