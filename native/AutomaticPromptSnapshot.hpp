#pragma once
#include "GuiModels.hpp"
#include <unordered_set>
#include <type_traits>
namespace udm {
struct AutomaticConfirmation {JobPtr job;std::string id;bool legacy=false;};
struct AutomaticPromptSnapshot {std::vector<AutomaticConfirmation> confirmations;std::vector<std::string> reviews;JobPtr authentication;};
enum class AutomaticPromptReadiness {Busy,Obsolete,Ready};
inline bool automaticConfirmationStatus(const Json& data){const auto status=str(data,"Status");return status=="Awaiting confirmation"||status=="Awaiting duplicate choice";}
// Transfer ephemeral legacy offers to an owned UI reservation only after all
// snapshot allocations/validation succeed. Unpresented reservations return on
// the next read; discovery never marks a dialog seen or clears authentication.
inline std::optional<AutomaticPromptSnapshot> tryAutomaticPromptSnapshot(Manager& manager,bool captureOnly,
 const std::set<std::string>& captured,const std::set<std::string>& seen,const std::vector<AutomaticConfirmation>& reserved){
 std::unique_lock lock(manager.mutex,std::try_to_lock);if(!lock.owns_lock()||manager.fileInfoDecisionPending)return std::nullopt;
 AutomaticPromptSnapshot result;auto owned=captured;for(const auto& offer:manager.pendingBrowserPresentations())owned.insert(str(offer,"id"));
 const bool indexOffers=reserved.size()>8||manager.pendingOffers.size()>8;std::unordered_set<const Job*> live;if(indexOffers){live.reserve(manager.jobs.size());for(const auto& job:manager.jobs)live.insert(job.get());}
 std::map<JobPtr,size_t> positions;
 const auto add=[&](const JobPtr& job,bool legacy,bool knownLive=false){
  if(!job||(!knownLive&&(indexOffers?!live.count(job.get()):std::find(manager.jobs.begin(),manager.jobs.end(),job)==manager.jobs.end())))return;
  if(!legacy&&!automaticConfirmationStatus(job->data))return;const auto id=job->id();if(owned.count(id)||(!legacy&&seen.count(id)))return;
  const auto inserted=positions.emplace(job,result.confirmations.size());if(inserted.second)result.confirmations.push_back({job,id,legacy});else if(legacy)result.confirmations[inserted.first->second].legacy=true;
 };
 for(const auto& entry:reserved)add(entry.job,entry.legacy);
 for(const auto& job:manager.pendingOffers)add(job,true);
 if(!captureOnly)for(const auto& job:manager.jobs){
  add(job,false,true);
  if(!result.authentication&&!yes(manager.state["Settings"],"SuppressAuthenticationDialog")&&yes(job->data,"AuthenticationPromptPending")&&!manager.isActive(job)&&!yes(job->data,"QueueOrigin")&&authenticationDialogReady(job->data))result.authentication=job;
 }
 for(const auto& token:manager.pendingBrowserCaptureReviews({},true))result.reviews.push_back(token.get<std::string>());
 // From this point the return and caller's reservation move must not allocate.
 static_assert(std::is_nothrow_move_constructible_v<AutomaticPromptSnapshot>);manager.pendingOffers.clear();return std::optional<AutomaticPromptSnapshot>(std::move(result));
}
inline AutomaticPromptReadiness tryAutomaticConfirmationReady(Manager& manager,const AutomaticConfirmation& candidate,const std::set<std::string>& seen){
 std::unique_lock lock(manager.mutex,std::try_to_lock);if(!lock.owns_lock()||manager.fileInfoDecisionPending)return AutomaticPromptReadiness::Busy;
 if(!candidate.job||std::find(manager.jobs.begin(),manager.jobs.end(),candidate.job)==manager.jobs.end()||candidate.job->id()!=candidate.id)return AutomaticPromptReadiness::Obsolete;
 for(const auto& offer:manager.pendingBrowserPresentations(candidate.job))if(str(offer,"id")==candidate.id)return AutomaticPromptReadiness::Obsolete;
 if(!candidate.legacy&&(!automaticConfirmationStatus(candidate.job->data)||seen.count(candidate.id)))return AutomaticPromptReadiness::Obsolete;
 return AutomaticPromptReadiness::Ready;
}
// A durable claim is a command, not discovery. A busy model leaves it pending;
// save failure restores the exact in-memory flag before propagating the error.
inline AutomaticPromptReadiness tryClaimAutomaticAuthentication(Manager& manager,const JobPtr& job){
 std::unique_lock lock(manager.mutex,std::try_to_lock);if(!lock.owns_lock()||manager.fileInfoDecisionPending)return AutomaticPromptReadiness::Busy;
 if(!job||std::find(manager.jobs.begin(),manager.jobs.end(),job)==manager.jobs.end()||yes(manager.state["Settings"],"SuppressAuthenticationDialog")||manager.isActive(job)||yes(job->data,"QueueOrigin")||!authenticationDialogReady(job->data))return AutomaticPromptReadiness::Obsolete;
 auto previous=job->data.at("AuthenticationPromptPending");job->data["AuthenticationPromptPending"]=false;
 try{manager.save();}catch(...){job->data["AuthenticationPromptPending"]=std::move(previous);throw;}
 return AutomaticPromptReadiness::Ready;
}
inline std::optional<std::vector<BrowserBatchRow>> tryBrowserBatchSnapshot(Manager& manager){
 std::unique_lock lock(manager.mutex,std::try_to_lock);if(!lock.owns_lock()||manager.fileInfoDecisionPending)return std::nullopt;
 if(manager.pendingBrowserBatches.empty())return std::vector<BrowserBatchRow>{};
 std::optional<std::vector<BrowserBatchRow>> result(std::move(manager.pendingBrowserBatches.front()));manager.pendingBrowserBatches.erase(manager.pendingBrowserBatches.begin());return result;
}
}
