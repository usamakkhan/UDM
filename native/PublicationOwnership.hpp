#pragma once
#include "Core.hpp"
#include "WindowsPath.hpp"
#include <limits>
#include <unordered_set>
namespace udm {
// Runtime only. Callers acquire/advance while holding Manager::mutex; queries
// use the same model->registry order. Destruction takes only the registry lock.
class PublicationOwnership {
public:
 using PathLess=windowsPath::Less;using Paths=windowsPath::Keys;
 static std::wstring pathKey(const fs::path& path){return windowsPath::key(path);}
private:
 struct Entry {std::map<const Job*,uint64_t> jobs;std::map<std::wstring,uint64_t,PathLess> paths;std::set<uint64_t> committing;};
 struct Registry {std::mutex mutex;std::condition_variable changed;uint64_t next=0;std::map<const Manager*,Entry> managers;};
 // Process lifetime avoids static-destructor ordering against still-joining
 // application workers. Every lease removes its own rows during normal exit.
 static Registry& registry(){static auto* value=new Registry;return *value;}
 static bool treeState(const Manager& manager,const JobPtr& job,bool commitOnly){
  if(!job)return false;auto& state=registry();std::lock_guard<std::mutex> lock(state.mutex);auto found=state.managers.find(&manager);if(found==state.managers.end())return false;
  const auto& entry=found->second;const auto matches=[&](const Job* value){auto owner=entry.jobs.find(value);return owner!=entry.jobs.end()&&(!commitOnly||entry.committing.count(owner->second));};
  if(matches(job.get()))return true;if(!job->video&&!job->audio)return false;
  std::vector<const Job*> pending{job.get()};std::unordered_set<const Job*> seen;
  while(!pending.empty()){auto value=pending.back();pending.pop_back();if(!value||!seen.insert(value).second)continue;if(matches(value))return true;if(value->video)pending.push_back(value->video.get());if(value->audio)pending.push_back(value->audio.get());}return false;
 }
 const Manager* manager;uint64_t token=0;std::vector<JobPtr> jobs;Paths paths;
public:
 PublicationOwnership(const Manager& owner,std::vector<JobPtr> ownedJobs,const std::vector<fs::path>& destinations):manager(&owner),jobs(std::move(ownedJobs)){
  if(jobs.empty()||std::any_of(jobs.begin(),jobs.end(),[](const JobPtr& j){return !j;}))throw std::runtime_error("Missing publication owner.");
  // Retain and exclude the captured descendant graph. Sibling-only leases do
  // not overlap, while a parent and any child cannot publish independently.
  {auto pending=std::move(jobs);jobs.clear();std::unordered_set<const Job*> seen;while(!pending.empty()){auto value=pending.back();pending.pop_back();if(!value||!seen.insert(value.get()).second)continue;jobs.push_back(value);if(value->video)pending.push_back(value->video);if(value->audio)pending.push_back(value->audio);}}
  for(const auto& path:destinations){if(!path.is_absolute())throw std::runtime_error("Publication ownership requires absolute destinations.");paths.insert(pathKey(path));}if(paths.empty())throw std::runtime_error("Missing publication destination.");
  for(const auto& path:destinations)if(owner.relocationReserves(path))throw std::runtime_error("A file move owns this publication destination. Try again after it finishes.");
  for(const auto& path:destinations)if(owner.pendingMediaDestination(path))throw std::runtime_error("A pending browser acceptance owns this publication destination. Try again after it finishes.");
  auto& state=registry();std::lock_guard<std::mutex> lock(state.mutex);auto found=state.managers.find(manager);Entry next=found==state.managers.end()?Entry{}:found->second;
  for(const auto& job:jobs)if(next.jobs.count(job.get()))throw std::runtime_error("Another file publication owns this download or destination. Try again after it finishes.");
  for(const auto& path:paths)if(next.paths.count(path))throw std::runtime_error("Another file publication owns this download or destination. Try again after it finishes.");
  if(state.next==std::numeric_limits<uint64_t>::max())throw std::runtime_error("Publication ownership identifiers exhausted.");const auto id=++state.next;
  for(const auto& job:jobs)next.jobs[job.get()]=id;for(const auto& path:paths)next.paths[path]=id;
  if(found==state.managers.end())state.managers.emplace(manager,std::move(next));else found->second=std::move(next);token=id;
 }
 PublicationOwnership(const PublicationOwnership&)=delete;PublicationOwnership& operator=(const PublicationOwnership&)=delete;
 ~PublicationOwnership(){release();}
 void release()noexcept{
  if(!token)return;auto& state=registry();std::lock_guard<std::mutex> lock(state.mutex);auto found=state.managers.find(manager);if(found!=state.managers.end()){
   auto& entry=found->second;for(const auto& job:jobs){auto i=entry.jobs.find(job.get());if(i!=entry.jobs.end()&&i->second==token)entry.jobs.erase(i);}for(const auto& path:paths){auto i=entry.paths.find(path);if(i!=entry.paths.end()&&i->second==token)entry.paths.erase(i);}entry.committing.erase(token);if(entry.jobs.empty()&&entry.paths.empty())state.managers.erase(found);
  }token=0;state.changed.notify_all();
 }
 // Call without any Manager::mutex ownership. Existing publishers need that
 // mutex to finish; the condition-variable wait releases the registry lock.
 static void waitUntilIdle(const Manager& manager){auto& state=registry();std::unique_lock<std::mutex> lock(state.mutex);state.changed.wait(lock,[&]{return state.managers.find(&manager)==state.managers.end();});}
 bool owns(const JobPtr& job)const noexcept{return token&&std::any_of(jobs.begin(),jobs.end(),[&](const JobPtr& value){return value==job;});}
 void beginCommit(){auto& state=registry();std::lock_guard<std::mutex> lock(state.mutex);auto found=state.managers.find(manager);if(!token||found==state.managers.end())throw std::runtime_error("Publication ownership has ended.");found->second.committing.insert(token);}
 // Caller holds Manager::mutex while reading the live parent/track graph.
 static bool treeOwned(const Manager& manager,const JobPtr& job){return treeState(manager,job,false);}
 static bool treeCommitting(const Manager& manager,const JobPtr& job){return treeState(manager,job,true);}
 static bool owned(const Manager& manager,const JobPtr& job){if(!job)return false;auto& state=registry();std::lock_guard<std::mutex> lock(state.mutex);auto found=state.managers.find(&manager);return found!=state.managers.end()&&found->second.jobs.count(job.get());}
 static bool committing(const Manager& manager,const JobPtr& job){if(!job)return false;auto& state=registry();std::lock_guard<std::mutex> lock(state.mutex);auto found=state.managers.find(&manager);if(found==state.managers.end())return false;auto owner=found->second.jobs.find(job.get());return owner!=found->second.jobs.end()&&found->second.committing.count(owner->second);}
 static bool reserved(const Manager& manager,const fs::path& path,const JobPtr& owner={}){auto key=pathKey(path);auto& state=registry();std::lock_guard<std::mutex> lock(state.mutex);auto found=state.managers.find(&manager);if(found==state.managers.end())return false;auto held=found->second.paths.find(key);if(held==found->second.paths.end())return false;auto own=owner?found->second.jobs.find(owner.get()):found->second.jobs.end();return own==found->second.jobs.end()||own->second!=held->second;}
 static bool requiresDirectory(const Manager& manager,const fs::path& path){auto prefix=pathKey(path);auto& state=registry();std::lock_guard<std::mutex> lock(state.mutex);auto found=state.managers.find(&manager);if(found==state.managers.end())return false;for(const auto& item:found->second.paths)if(windowsPath::descendant(item.first,prefix))return true;return false;}
 static Paths destinations(const Manager& manager){auto& state=registry();std::lock_guard<std::mutex> lock(state.mutex);Paths result;auto found=state.managers.find(&manager);if(found!=state.managers.end())for(const auto& path:found->second.paths)result.insert(path.first);return result;}
};
}
