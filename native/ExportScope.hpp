#pragma once
#include "Catalog.hpp"
namespace udm {
enum class ExportScope { All, Selected, Queue };
inline std::vector<JobPtr> exportScopeRows(const std::vector<JobPtr>& all,const std::set<std::string>& selected,ExportScope scope,const std::string& queue={}){
 std::vector<JobPtr> result;
 for(auto job:all){
  if(scope==ExportScope::Selected&&!selected.count(job->id()))continue;
  if(scope==ExportScope::Queue&&(!yes(job->data,"QueueMember",str(job->data,"Status")!="Complete")||str(job->data,"Status")=="Complete"||!str(job->data,"RecycledAt").empty()||(!queue.empty()&&str(job->data,"Queue")!=queue)))continue;
  result.push_back(job);
 }
 return result;
}
inline void writeDownloadExport(Manager& manager,const std::vector<JobPtr>& chosen,const fs::path& path,bool catalog,bool credentials){
 Lock lock(manager.mutex);if(chosen.empty())throw std::runtime_error("Select at least one download.");
 std::set<std::string> ids;for(auto job:chosen){if(!job||std::find(manager.jobs.begin(),manager.jobs.end(),job)==manager.jobs.end()||!ids.insert(job->id()).second)throw std::runtime_error("The download list changed. Reopen Export downloads and review the selection.");}
 std::string value;if(catalog)value=exportCatalog(manager,chosen,credentials).dump(2);else for(auto job:chosen)value+=str(job->data,"Url")+"\r\n";
 atomicText(path,value,false);
}
}
