#pragma once
#include "Core.hpp"
namespace udm {
// Caller holds manager.mutex. Only eligible, explicitly enrolled direct files
// stay in synchronization after publication; ordinary downloads leave queues.
inline bool retainCompletedMembership(const Manager& manager,const Json& data){
 if(!yes(data,"QueueMember")||data.contains("OfflineProject")||!str(data,"ProtectedRequest").empty()||yes(data,"RequiresRequestCapture")||yes(data,"RequiresMediaCapture")||!str(data,"RecycledAt").empty()||!str(data,"PreviousVersionOf").empty()||!str(data,"SourceUrl").empty()||!str(data,"ProtectedAdaptive").empty())return false;
 const Url url(str(data,"Url"));if(url.scheme!="http"&&url.scheme!="https"&&url.scheme!="ftp")return false;
 for(const auto& queue:manager.state["Queues"])if(str(queue,"Name")==str(data,"Queue"))return yes(queue,"Synchronize");
 return false;
}
// Caller holds manager.mutex throughout validation and commit.
inline void validateQueueMembership(Manager& manager,const JobPtr& job,bool member,const std::string& queue){
 if(!job||std::find(manager.jobs.begin(),manager.jobs.end(),job)==manager.jobs.end())throw std::runtime_error("Unknown download.");
 if(manager.isActive(job))throw std::runtime_error("Pause this download before changing its queue membership.");
 const auto destination=queue.empty()?str(job->data,"Queue"):queue;const Json* target=nullptr;
 for(const auto& q:manager.state["Queues"])if(str(q,"Name")==destination){target=&q;break;}
 if((member||!queue.empty())&&!target)throw std::runtime_error("Choose an existing queue.");
 const bool completed=str(job->data,"Status")=="Complete";
 if(member&&completed){
  if(!yes(*target,"Synchronize"))throw std::runtime_error("Choose a synchronization queue for a completed file, or use Redownload to download a new copy.");
  const auto& data=job->data;Url url(str(data,"Url"));
  if((url.scheme!="http"&&url.scheme!="https"&&url.scheme!="ftp")||!str(data,"DuplicateOf").empty()||data.contains("OfflineProject")||!str(data,"ProtectedRequest").empty()||yes(data,"RequiresRequestCapture")||yes(data,"RequiresMediaCapture")||!str(data,"RecycledAt").empty()||!str(data,"PreviousVersionOf").empty()||!str(data,"SourceUrl").empty()||!str(data,"ProtectedAdaptive").empty())throw std::runtime_error("Synchronization requires a saved direct HTTP, HTTPS or FTP file.");
  if(!fs::is_regular_file(job->target()))throw std::runtime_error("The saved file is missing. Use Redownload to restore it.");
 }

}
// Validate and snapshot the entire selection before changing any record.
inline void setQueueMembershipBatch(Manager& manager,const std::vector<JobPtr>& selection,bool member,const std::string& queue={}){
 Lock lock(manager.mutex);
 std::vector<std::pair<JobPtr,Json>> previous;
 for(const auto& job:selection){
  if(std::any_of(previous.begin(),previous.end(),[&](const auto& entry){return entry.first==job;}))continue;
  validateQueueMembership(manager,job,member,queue);
  previous.emplace_back(job,job->data);
 }
 if(previous.empty())return;
 try{
  for(auto& entry:previous){auto& job=entry.first;const bool completed=str(job->data,"Status")=="Complete";
   if(!queue.empty())job->data["Queue"]=queue;
   job->data["QueueMember"]=member;
   if(!member||completed)job->data["SyncPending"]=false;
   if(!member&&str(job->data,"Status")=="Queued")job->data["Status"]="Paused";
  }
  manager.save();
 }catch(...){for(auto& entry:previous)entry.first->data.swap(entry.second);throw;}
}

// Validate the whole selection before performing a single durable order update.
inline std::set<JobPtr> queueOrderSelection(Manager& manager,const std::vector<JobPtr>& selection){
 std::set<JobPtr> chosen;std::string queue;
 for(const auto& job:selection){
  if(!job||std::find(manager.jobs.begin(),manager.jobs.end(),job)==manager.jobs.end())throw std::runtime_error("The selected download no longer exists.");
  if(manager.isActive(job))throw std::runtime_error("Stop selected downloads before reordering them.");
  if(!yes(job->data,"QueueMember",str(job->data,"Status")!="Complete"))throw std::runtime_error("The selected download is no longer in its queue.");
  if(chosen.empty())queue=str(job->data,"Queue");else if(str(job->data,"Queue")!=queue)throw std::runtime_error("Choose files from the same queue.");
  chosen.insert(job);
 }
 return chosen;
}
inline void moveQueueSelection(Manager& manager,const std::vector<JobPtr>& selection,int direction){
 Lock lock(manager.mutex);if(direction!=-1&&direction!=1)throw std::runtime_error("Invalid queue direction.");
 auto chosen=queueOrderSelection(manager,selection);if(chosen.empty())return;
 auto queue=str((*chosen.begin())->data,"Queue");std::vector<size_t> slots;std::vector<JobPtr> order;
 for(size_t i=0;i<manager.jobs.size();++i){auto j=manager.jobs[i];if(str(j->data,"Queue")==queue&&yes(j->data,"QueueMember",str(j->data,"Status")!="Complete")){slots.push_back(i);order.push_back(j);}}
 if(direction<0){for(size_t i=1;i<order.size();++i)if(chosen.count(order[i])&&!chosen.count(order[i-1]))std::swap(order[i],order[i-1]);}
 else {for(size_t i=order.size();i>1;--i)if(chosen.count(order[i-2])&&!chosen.count(order[i-1]))std::swap(order[i-2],order[i-1]);}
 auto next=manager.jobs;for(size_t i=0;i<slots.size();++i)next[slots[i]]=order[i];if(next==manager.jobs)return;
 manager.jobs.swap(next);try{manager.save();}catch(...){manager.jobs.swap(next);throw;}
}
inline void reorderQueueSelection(Manager& manager,const std::vector<JobPtr>& selection,JobPtr before={}){
 Lock lock(manager.mutex);auto chosen=queueOrderSelection(manager,selection);if(chosen.empty()||chosen.count(before))return;
 auto queue=str((*chosen.begin())->data,"Queue");
 if(before&&(std::find(manager.jobs.begin(),manager.jobs.end(),before)==manager.jobs.end()||str(before->data,"Queue")!=queue||!yes(before->data,"QueueMember",str(before->data,"Status")!="Complete")))throw std::runtime_error("The drop destination changed.");
 std::vector<JobPtr> next,ordered;for(auto j:manager.jobs)(chosen.count(j)?ordered:next).push_back(j);
 auto at=before?std::find(next.begin(),next.end(),before):next.end();next.insert(at,ordered.begin(),ordered.end());if(next==manager.jobs)return;
 manager.jobs.swap(next);try{manager.save();}catch(...){manager.jobs.swap(next);throw;}
}

}
