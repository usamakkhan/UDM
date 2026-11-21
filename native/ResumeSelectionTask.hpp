#pragma once
#include "RequestedAuthenticationTask.hpp"
#include "FileInfoDecision.hpp"
#include "DuplicateDecisionTask.hpp"
namespace udm {
enum class ResumeRequestKind {Skip,Resume,Duplicate,Refresh,Login,Content};
struct ResumeRequestView {JobPtr job;Json key;ResumeRequestKind kind=ResumeRequestKind::Skip;bool contentPrefix=false,webPage=false;i64 prefixBytes=0;DuplicatePromptViewPtr duplicate;};
struct ResumeContentDecision {ResumeRequestView view;std::optional<bool> stripPrefix,allowWebPage;};
struct ResumeSelectionResult {std::vector<ResumeRequestView> requests;size_t queued=0;bool choicesSaved=false;std::string warning;};
inline constexpr const char* ResumeContextFields[]={"Id","Url","Status","LastAttempt","Folder","FileName","Queue","QueueMember","QueueOrigin","DuplicateOf","SabrSessionRefreshRequired","ContentPrefixPending","ContentPrefixBytes","WebPageReceived","StripContentPrefix","AllowContentPrefix","AllowWebPageContent","AuthenticationPromptPending","AuthenticationOrigin","AuthenticationScheme","LastHttpStatus","LastFtpStatus","ProtectedHeaders","ProtectedBrowserSession","ProtectedAdaptive","ProtectedRequest","SourceUrl"};
inline Json resumeContextKey(const Json& data){Json key=Json::object();for(const auto* field:ResumeContextFields){const auto found=data.find(field);if(found!=data.end())key[field]=*found;}return key;}
inline bool resumeContextMatches(const Json& data,const Json& key){
 if(!key.is_object())return false;size_t count=0;
 for(const auto* field:ResumeContextFields){const auto left=data.find(field),right=key.find(field);if(left==data.end()){if(right!=key.end())return false;}else{++count;if(right==key.end()||*left!=*right)return false;}}
 return count==key.size();
}
// Caller owns Manager. No controls, decryption or disk reads occur here.
inline ResumeRequestKind resumeRequestKind(Manager& manager,const JobPtr& job){
 if(!job||std::find(manager.jobs.begin(),manager.jobs.end(),job)==manager.jobs.end()||manager.isRelocating(job))return ResumeRequestKind::Skip;
 const auto status=str(job->data,"Status");const bool active=manager.isActive(job);
 if(active?!(job->prefetchDeferring&&status=="Paused"):(status=="Complete"||status=="Queued"))return ResumeRequestKind::Skip;
 if(!str(job->data,"DuplicateOf").empty())return ResumeRequestKind::Duplicate;
 if(yes(job->data,"SabrSessionRefreshRequired"))return ResumeRequestKind::Refresh;
 if(!active&&yes(job->data,"QueueOrigin")&&!yes(manager.state["Settings"],"SuppressAuthenticationDialog")&&authenticationDialogReady(job->data))return ResumeRequestKind::Login;
 if(yes(job->data,"ContentPrefixPending")||yes(job->data,"WebPageReceived"))return ResumeRequestKind::Content;
 return ResumeRequestKind::Resume;
}
inline ResumeRequestView resumeRequestView(Manager& manager,const JobPtr& job,ResumeRequestKind kind){return {job,resumeContextKey(job->data),kind,yes(job->data,"ContentPrefixPending"),yes(job->data,"WebPageReceived"),num(job->data,"ContentPrefixBytes"),kind==ResumeRequestKind::Duplicate?captureDuplicatePrompt(manager,job):DuplicatePromptViewPtr{}};}
inline AutomaticPromptReadiness tryResumeRequestCurrent(Manager& manager,const ResumeRequestView& view){
 std::unique_lock lock(manager.mutex,std::try_to_lock);if(!lock.owns_lock()||manager.fileInfoDecisionPending)return AutomaticPromptReadiness::Busy;
 return view.job&&view.kind!=ResumeRequestKind::Skip&&resumeRequestKind(manager,view.job)==view.kind&&resumeContextMatches(view.job->data,view.key)?AutomaticPromptReadiness::Ready:AutomaticPromptReadiness::Obsolete;
}
inline void resumeWarning(ResumeSelectionResult& result,const std::string& message){if(!result.warning.empty())result.warning+='\n';result.warning+=message;}
inline void dispatchSelectedResumes(Manager& manager,ResumeSelectionResult& result){if(result.queued)try{manager.dispatchQueueWork();}catch(const std::exception& e){resumeWarning(result,"The retry was queued, but immediate scheduling failed. "+std::string(e.what()));}}
inline ResumeSelectionResult applyResumeSelection(Manager& manager,const std::vector<JobPtr>& jobs,const AuthenticationPromptObserver& observe={}){
 ResumeSelectionResult result;std::set<JobPtr> seen;
 for(const auto& job:jobs){if(!seen.insert(job).second)continue;
  try{
   ResumeRequestKind kind;{Lock lock(manager.mutex);kind=resumeRequestKind(manager,job);if(kind!=ResumeRequestKind::Skip&&kind!=ResumeRequestKind::Resume)result.requests.push_back(resumeRequestView(manager,job,kind));}
   if(kind!=ResumeRequestKind::Resume)continue;
   if(observe)observe("before-resume");std::optional<ResumeRequestView> changed;
   const bool queued=manager.resumeIf(job,[&]{const auto current=resumeRequestKind(manager,job);if(current==ResumeRequestKind::Resume)return true;if(current!=ResumeRequestKind::Skip)changed=resumeRequestView(manager,job,current);return false;});
   if(queued)++result.queued;else if(changed)result.requests.push_back(std::move(*changed));
  }catch(const std::exception& e){resumeWarning(result,"Could not resume a selected download. "+std::string(e.what()));}
 }
 std::stable_partition(result.requests.begin(),result.requests.end(),[](const ResumeRequestView& view){return view.kind!=ResumeRequestKind::Refresh;});dispatchSelectedResumes(manager,result);return result;
}
inline ResumeSelectionResult applyResumeContentDecision(Manager& manager,const ResumeContentDecision& decision,const AuthenticationPromptObserver& observe={}){
 ResumeSelectionResult result;Json savedKey;
 const auto& view=decision.view;
 try{
  {Lock lock(manager.mutex);
   if(resumeRequestKind(manager,view.job)!=ResumeRequestKind::Content||!resumeContextMatches(view.job->data,view.key))throw std::runtime_error("The download changed. Review its current content warning before retrying.");
   if(decision.stripPrefix.has_value()!=view.contentPrefix||decision.allowWebPage.has_value()!=view.webPage)throw std::runtime_error("The content decision is incomplete.");
   auto before=view.job->data;
   try{
    if(decision.stripPrefix){view.job->data["StripContentPrefix"]=*decision.stripPrefix;view.job->data["AllowContentPrefix"]=!*decision.stripPrefix;}
    if(decision.allowWebPage)view.job->data["AllowWebPageContent"]=*decision.allowWebPage;
    savedKey=resumeContextKey(view.job->data);if(observe)observe("before-choice-save");manager.save();
   }catch(...){view.job->data.swap(before);throw;}
   result.choicesSaved=true;
  }
  if(observe)observe("choices-saved");
  const bool queued=manager.resumeIf(view.job,[&]{return resumeRequestKind(manager,view.job)==ResumeRequestKind::Content&&resumeContextMatches(view.job->data,savedKey);});
  if(queued)result.queued=1;else resumeWarning(result,"The choices were saved, but the download changed before retrying it.");
 }catch(const std::exception& e){resumeWarning(result,(result.choicesSaved?"The choices were saved, but retry could not start. ":"The content choices could not be saved. ")+std::string(e.what()));}
 dispatchSelectedResumes(manager,result);return result;
}
class ResumeSelectionTask {
 FileInfoDecisionTask task;std::shared_ptr<ResumeSelectionResult> outcome;
 void startAction(Manager& manager,std::function<ResumeSelectionResult()> action,std::function<void()> notify){
  if(active())throw std::runtime_error("The selected downloads are already being processed.");auto next=std::make_shared<ResumeSelectionResult>();
  task.startAction(manager,[next,action=std::move(action)]{*next=action();return FileInfoDecisionResult{};},std::move(notify));outcome=std::move(next);
 }
public:
 bool active()const noexcept{return task.valid();}
 bool ready(){return task.valid()&&task.ready();}
 void start(Manager& manager,std::vector<JobPtr> jobs,std::function<void()> notify={},AuthenticationPromptObserver observe={}){startAction(manager,[&manager,jobs=std::move(jobs),observe=std::move(observe)]{return applyResumeSelection(manager,jobs,observe);},std::move(notify));}
 void confirm(Manager& manager,ResumeContentDecision decision,std::function<void()> notify={},AuthenticationPromptObserver observe={}){startAction(manager,[&manager,decision=std::move(decision),observe=std::move(observe)]{return applyResumeContentDecision(manager,decision,observe);},std::move(notify));}
 ResumeSelectionResult take(){if(!ready())throw std::runtime_error("The Resume command has not finished.");try{task.get();task.release();auto value=std::move(*outcome);outcome.reset();return value;}catch(...){task.release();outcome.reset();throw;}}
 void finish()noexcept{task.release();outcome.reset();}
};
}
