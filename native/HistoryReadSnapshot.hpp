#pragma once
#include "DownloadListSnapshot.hpp"
#include "DownloadListTooltip.hpp"
#include "GuiModels.hpp"
namespace udm {
inline std::map<std::string,std::string> historyNavigationKeys(const NavigationTreeSnapshot& tree){
 std::map<std::string,std::string> keys={{"all","all"},{"unfinished","unfinished"},{"finished","finished"},{"grabber","grabber"},{"root:queues","all"}};
 for(const auto& category:tree.categories)if(category!="Other")for(const auto* prefix:{"category:","unfinished:","finished:"}){const auto key=prefix+category;keys.emplace(key,key);}
 for(const auto& name:tree.queueNames)keys.emplace("queue:"+name,"queue:"+name);
 for(const auto& project:tree.projects)keys.emplace("project:"+project.first,"project:"+project.first);
 return keys;
}
inline std::string historyEffectiveFilter(const NavigationTreeSnapshot& tree,const NavigationTreeSnapshot* previous,const std::string& requested,const std::vector<std::string>& selectedAncestors){
 const auto keys=historyNavigationKeys(tree);
 // A valid requested view is authoritative even when nodes were added,
 // renamed or reordered. Ancestors are fallback for a removed view only.
 if(const auto found=keys.find(requested);found!=keys.end())return found->second;
 if(previous&&previous->sameStructure(tree))return "all";
 for(const auto& key:selectedAncestors){const auto found=keys.find(key);if(found!=keys.end())return found->second;}
 return "all";
}
inline bool historyFilterMatches(const Json& data,const std::string& filter){
 const auto category=str(data,"Category"),state=str(data,"Status");
 return filter=="all"||(filter=="unfinished"&&state!="Complete")||(filter=="finished"&&state=="Complete")||(filter=="grabber"&&!str(data,"ProjectId").empty())||(filter.rfind("category:",0)==0&&category==filter.substr(9))||(filter.rfind("unfinished:",0)==0&&category==filter.substr(11)&&state!="Complete")||(filter.rfind("finished:",0)==0&&category==filter.substr(9)&&state=="Complete")||(filter.rfind("queue:",0)==0&&downloadListQueueMember(data)&&str(data,"Queue")==filter.substr(6))||(filter.rfind("project:",0)==0&&str(data,"ProjectId")==filter.substr(8));
}
struct HistoryReadRequest {
 bool visible=true,includeHeaders=false,searchActive=false;
 std::string filter="all";std::vector<std::string> selectedAncestors;
 const NavigationTreeSnapshot* previousTree=nullptr;
 std::vector<JobPtr> selected;JobPtr focused;std::array<int,8> queueImages{};
 Json searchQuery=Json::object(),filterQuery=Json::object();
};
struct HistoryReadSnapshot {
 struct Observation {JobPtr job;std::string id,state;bool httpFailure=false;};
 Json preferences;std::string storageWarning,filter,focusedId;NavigationTreeSnapshot navigation;
 std::vector<Observation> observations;std::set<std::string> selection;std::set<JobPtr> quotaWaiting;
 std::vector<JobPtr> rows;std::map<JobPtr,std::shared_ptr<const DownloadListRow>> records;
 std::map<JobPtr,Json> searchRecords;std::map<JobPtr,bool> searchMatches;
};
// The caller provides only UI-owned inputs. No controls, decryption, sorting or
// disk reads run here. Failure to acquire the model changes neither cache.
inline std::optional<HistoryReadSnapshot> tryHistoryReadSnapshot(Manager& manager,const HistoryReadRequest& request,
 const std::map<JobPtr,std::shared_ptr<const DownloadListRow>>& previousRows,DownloadFilterCache& searchCache){
 std::unique_lock lock(manager.mutex,std::try_to_lock);if(!lock.owns_lock())return std::nullopt;
 HistoryReadSnapshot result;result.preferences=manager.state["Settings"];result.storageWarning=manager.storageError;
 if(request.visible){
  result.navigation=navigationTreeSnapshot(manager);result.filter=historyEffectiveFilter(result.navigation,request.previousTree,request.filter,request.selectedAncestors);
  for(const auto& job:request.selected)result.selection.insert(job->id());if(request.focused)result.focusedId=request.focused->id();
  if(num(result.preferences,"QuotaMb")>0)result.quotaWaiting=manager.quotaWaitingJobs();
  searchCache.begin(request.searchQuery);result.rows.reserve(manager.jobs.size());
 }
 const DownloadSearch matcher(request.filterQuery);std::map<std::string,int> queueIcons;
 if(request.visible)for(size_t i=0;i<result.navigation.queueNames.size();++i)queueIcons.emplace(result.navigation.queueNames[i],request.queueImages.at(result.navigation.queueKinds.at(i)));
 for(const auto& job:manager.jobs){
  const auto& data=job->data;result.observations.push_back({job,job->id(),str(data,"Status"),job->httpFailureNoticePending&&!manager.isActive(job)});
  if(!request.visible||!historyFilterMatches(data,result.filter))continue;
  const auto icon=queueIcons.find(str(data,"Queue"));const int image=downloadListQueueMember(data)&&icon!=queueIcons.end()?icon->second:-2;
  const auto cached=previousRows.find(job);result.records.emplace(job,downloadListRow(data,job->speed,request.includeHeaders,image,cached==previousRows.end()?std::shared_ptr<const DownloadListRow>{}:cached->second,manager.isActive(job),job->prefetchDeferring));
  if(request.searchActive){const auto match=searchCache.cached(job,data,matcher);if(match.has_value())result.searchMatches.emplace(job,*match);else result.searchRecords.emplace(job,matcher.snapshot(data));}
  else searchCache.keep(job);
  result.rows.push_back(job);
 }
 return result;
}
inline std::optional<Json> tryBrowserPresentationSnapshot(Manager& manager){
 std::unique_lock lock(manager.mutex,std::try_to_lock);if(!lock.owns_lock())return std::nullopt;
 return manager.pendingBrowserPresentations();
}
}
