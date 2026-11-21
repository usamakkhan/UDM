#pragma once
#include "FileInfoDecision.hpp"
namespace udm {
inline constexpr const char* DuplicateContextFields[]={"Id","Url","Status","Folder","FileName","DuplicateOf","DuplicateQueue","Queue","QueueMember","QueueOrigin","ReplacementOf","ReplacementHash","PreviousPath","Sha256","Size","LastAttempt","ProtectedHeaders","ProtectedRequest","ProtectedBrowserSession","ProtectedAdaptive","SourceUrl"};
inline Json duplicateContextKey(const Json& data){Json key=Json::object();for(const auto* field:DuplicateContextFields){const auto found=data.find(field);if(found!=data.end())key[field]=*found;}return key;}
inline bool duplicateContextMatches(const Json& data,const Json& key,bool activeProgress=false){
 if(!key.is_object())return false;size_t count=0;
 for(const auto* field:DuplicateContextFields){if(activeProgress&&(std::string_view(field)=="Size"||std::string_view(field)=="Sha256"))continue;const auto left=data.find(field),right=key.find(field);if(left==data.end()){if(right!=key.end())return false;}else{++count;if(right==key.end()||*left!=*right)return false;}}
 return count==key.size();
}
struct DuplicatePromptView {JobPtr candidate,original;Json candidateKey,originalKey;std::string address;bool canReplace=false,originalActive=false;};
using DuplicatePromptViewPtr=std::shared_ptr<const DuplicatePromptView>;
inline JobPtr duplicateOriginalLocked(Manager& manager,const JobPtr& candidate){const auto id=str(candidate->data,"DuplicateOf");for(const auto& job:manager.jobs)if(job->id()==id)return job;return {};}
inline DuplicatePromptViewPtr captureDuplicatePrompt(Manager& manager,const JobPtr& candidate){
 Lock lock(manager.mutex);if(!candidate||std::find(manager.jobs.begin(),manager.jobs.end(),candidate)==manager.jobs.end())throw std::runtime_error("This duplicate download was removed.");
 if(str(candidate->data,"DuplicateOf").empty())return {};
 auto view=std::make_shared<DuplicatePromptView>();view->candidate=candidate;view->candidateKey=duplicateContextKey(candidate->data);view->address=str(view->candidateKey,"Url");view->original=duplicateOriginalLocked(manager,candidate);
 if(view->original){view->originalKey=duplicateContextKey(view->original->data);view->originalActive=manager.isActive(view->original);if(view->originalActive){view->originalKey.erase("Size");view->originalKey.erase("Sha256");}const auto status=str(view->originalKey,"Status");view->canReplace=!view->originalActive&&(status=="Complete"||status=="Paused"||status=="Failed")&&str(view->originalKey,"ReplacementOf").empty();}
 return view;
}
inline bool duplicateCandidateMatchesLocked(Manager& manager,const DuplicatePromptView& view){return view.candidate&&std::find(manager.jobs.begin(),manager.jobs.end(),view.candidate)!=manager.jobs.end()&&!manager.isActive(view.candidate)&&duplicateContextMatches(view.candidate->data,view.candidateKey)&&!str(view.candidateKey,"DuplicateOf").empty();}
inline bool duplicateOriginalIdentityMatchesLocked(Manager& manager,const DuplicatePromptView& view){
 const auto current=duplicateOriginalLocked(manager,view.candidate);if(!current||current!=view.original)return false;
 for(const auto* field:{"Id","Url","Folder","FileName","ProtectedHeaders","ProtectedRequest","ProtectedBrowserSession","ProtectedAdaptive","SourceUrl"}){const auto left=current->data.find(field);const auto right=view.originalKey.find(field);if(left==current->data.end()){if(right!=view.originalKey.end())return false;}else if(right==view.originalKey.end()||*left!=*right)return false;}return true;
}
inline bool duplicatePromptMatchesLocked(Manager& manager,const DuplicatePromptView& view){
 if(!duplicateCandidateMatchesLocked(manager,view))return false;
 const auto original=duplicateOriginalLocked(manager,view.candidate);return original==view.original&&(!original||(duplicateContextMatches(original->data,view.originalKey,view.originalActive)&&manager.isActive(original)==view.originalActive));
}
enum class DuplicateViewReadiness {Busy,Ready,Obsolete};
inline DuplicateViewReadiness tryDuplicatePromptCurrent(Manager& manager,const DuplicatePromptView& view){
 std::unique_lock lock(manager.mutex,std::try_to_lock);if(!lock.owns_lock()||manager.fileInfoDecisionPending)return DuplicateViewReadiness::Busy;
 return duplicatePromptMatchesLocked(manager,view)?DuplicateViewReadiness::Ready:DuplicateViewReadiness::Obsolete;
}
struct DuplicateChoiceResult {JobPtr job;bool chosen=false;std::optional<OfferPresentation> presentation;std::string warning;};
using DuplicateDecisionObserver=std::function<void(const char*)>;
inline DuplicateChoiceResult applyDuplicateDecision(Manager& manager,const DuplicatePromptView& view,const std::string& mode,bool remember,const DuplicateDecisionObserver& observe={}){
 DuplicateChoiceResult result;
 {Lock lock(manager.mutex);
  if(!duplicateCandidateMatchesLocked(manager,view)){
   if(mode=="Cancel"){result.chosen=true;return result;}
   throw std::runtime_error("The duplicate or existing download changed. Close this dialog and review the current request.");
  }
  if(mode!="Cancel"&&mode!="Existing"&&mode!="Numbered"&&mode!="Replace")throw std::runtime_error("Choose an existing download, numbered copy or replacement.");
  if(mode=="Existing"&&!duplicateOriginalIdentityMatchesLocked(manager,view))throw std::runtime_error("The existing download changed. Choose a numbered copy or review the current request.");
  if(mode=="Replace"&&(!view.canReplace||!duplicatePromptMatchesLocked(manager,view)))throw std::runtime_error("Pause the existing download before overwriting it.");
  if(observe)observe("before-resolve");result.job=manager.resolveDuplicate(view.candidate,mode,mode!="Cancel"&&remember);result.chosen=true;
 }
 if(!result.job)return result;
 try{
  if(observe)observe("choice-saved");result.presentation=manager.presentOffer(result.job);
  if(*result.presentation==OfferPresentation::Progress)try{manager.dispatchQueueWork();}catch(const std::exception& e){result.warning="The duplicate choice was saved and retry was queued, but immediate scheduling failed. "+std::string(e.what());}
 }catch(const std::exception& e){result.warning="The duplicate choice was saved, but its next action could not start. "+std::string(e.what());}
 return result;
}
class DuplicateDecisionTask {
 FileInfoDecisionTask task;std::shared_ptr<DuplicateChoiceResult> outcome;
public:
 bool active()const noexcept{return task.valid();}
 bool ready(){return task.valid()&&task.ready();}
 void start(Manager& manager,DuplicatePromptViewPtr view,std::string mode,bool remember,std::function<void()> notify={},DuplicateDecisionObserver observe={}){
  if(active())throw std::runtime_error("The duplicate choice is already being saved.");if(!view)throw std::runtime_error("Missing duplicate request view.");auto next=std::make_shared<DuplicateChoiceResult>();
  task.startAction(manager,[&manager,view=std::move(view),mode=std::move(mode),remember,next,observe=std::move(observe)]{*next=applyDuplicateDecision(manager,*view,mode,remember,observe);return FileInfoDecisionResult{};},std::move(notify));outcome=std::move(next);
 }
 DuplicateChoiceResult take(){if(!ready())throw std::runtime_error("The duplicate choice has not finished.");try{task.get();task.release();auto value=std::move(*outcome);outcome.reset();return value;}catch(...){task.release();outcome.reset();throw;}}
 void finish()noexcept{task.release();outcome.reset();}
};
inline std::atomic_uint duplicateDialogOwners=0;
inline bool duplicateDialogShowing()noexcept{return duplicateDialogOwners.load()!=0;}
struct DuplicateDialogOwner {DuplicateDialogOwner(){++duplicateDialogOwners;}~DuplicateDialogOwner(){--duplicateDialogOwners;}DuplicateDialogOwner(const DuplicateDialogOwner&)=delete;};
}
