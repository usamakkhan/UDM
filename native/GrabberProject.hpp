#pragma once
#include "GrabberDestinations.hpp"
namespace udm {
inline const std::vector<std::string>& grabberTemplateKeys(){static const std::vector<std::string> keys={"Template","SaveMode","Folder","SaveCategory","OriginalSubfolders","Extensions","FileInclude","FileExclude","PageIncludePaths","PageExcludePaths","FileIncludePaths","FileExcludePaths","FilesSameSite","ExploreSameSite","ExploreSubdomains","NoParentDirectories","HideDuplicates","Depth","ExternalDepth","MaxPages","MinFileBytes","MaxFileBytes","ExternalAssets","MaxMiB","ConvertLinks","DownloadWhileExploring","DownloadParallel","DownloadQueue","MetadataParallel","UseLinkDescriptions"};return keys;}
inline Json grabberTemplateSettings(const Json& project){Json result=Json::object();for(const auto& key:grabberTemplateKeys())if(project.contains(key))result[key]=project[key];return result;}
inline void validateGrabberActions(const Json& project){
 for(const char* key:{"DownloadWhileExploring","ConvertLinks","UseLinkDescriptions"})if(project.contains(key)&&!project[key].is_boolean())throw std::runtime_error("Invalid Grabber download setting.");
 for(const char* key:{"DownloadParallel","MetadataParallel"})if(project.contains(key)&&(!project[key].is_number_integer()||num(project,key)<1||num(project,key)>16))throw std::runtime_error("Grabber concurrency must be from 1 to 16.");
 if(project.contains("DownloadQueue")&&!project["DownloadQueue"].is_string())throw std::runtime_error("Invalid Grabber queue setting.");
}
inline void validateGrabberTemplateOptions(const Json& options,const Json& settings){auto draft=grabberTemplateSettings(options);draft["Id"]=guid();draft["Name"]="Template";draft["StartUrl"]="https://template.invalid/";draft["Links"]=Json::array();validateGrabberProject(draft,settings);validateGrabberActions(draft);}
inline Json grabberTemplates(const Manager& manager){Lock lock(manager.mutex);auto templates=manager.state.value("GrabberTemplates",Json::array());if(!templates.is_array()||templates.size()>100)throw std::runtime_error("Invalid saved Grabber templates.");return templates;}
inline Json saveGrabberTemplate(Manager& manager,const Json& project,const std::string& label,const std::string& replaceId=""){
 Lock lock(manager.mutex);auto name=trim(label);if(name.empty()||name.size()>160||std::any_of(name.begin(),name.end(),[](unsigned char c){return c<32||c==127;}))throw std::runtime_error("Enter a template name up to 160 bytes without control characters.");validateGrabberTemplateOptions(project,manager.state["Settings"]);
 auto templates=grabberTemplates(manager);size_t found=templates.size();for(size_t i=0;i<templates.size();++i){if(str(templates[i],"Id")==replaceId&&!replaceId.empty())found=i;else if(lower(str(templates[i],"Name"))==lower(name))throw std::runtime_error("A template already has that name. Choose it and replace its settings, or use another name.");}
 if(!replaceId.empty()&&found==templates.size())throw std::runtime_error("The template no longer exists.");if(found==templates.size()&&templates.size()>=100)throw std::runtime_error("Keep at most 100 Grabber templates.");
 Json item={{"Id",replaceId.empty()?guid():replaceId},{"Name",name},{"Version",1},{"Settings",grabberTemplateSettings(project)}};if(found==templates.size())templates.push_back(item);else templates[found]=item;
 bool existed=manager.state.contains("GrabberTemplates");auto old=manager.state.value("GrabberTemplates",Json::array());manager.state["GrabberTemplates"]=templates;try{manager.save();}catch(...){if(existed)manager.state["GrabberTemplates"]=old;else manager.state.erase("GrabberTemplates");throw;}return item;
}
inline void remapGrabberTemplates(Json& state,const std::string& original,const std::string& replacement){if(!state.contains("GrabberTemplates")||!state["GrabberTemplates"].is_array())return;for(auto& item:state["GrabberTemplates"])if(item.is_object()&&item.contains("Settings")&&item["Settings"].is_object()&&str(item["Settings"],"SaveCategory")==original)item["Settings"]["SaveCategory"]=replacement;}
inline void deleteGrabberTemplate(Manager& manager,const std::string& id){Lock lock(manager.mutex);auto before=grabberTemplates(manager),next=before;next.erase(std::remove_if(next.begin(),next.end(),[&](const Json& item){return str(item,"Id")==id;}),next.end());if(next.size()==before.size())throw std::runtime_error("The template no longer exists.");manager.state["GrabberTemplates"]=next;try{manager.save();}catch(...){manager.state["GrabberTemplates"]=before;throw;}}
inline Json applyGrabberTemplate(const Json& project,const Json& item,const Json& settings){
 if(!item.is_object()||num(item,"Version")!=1||!item.contains("Settings")||!item["Settings"].is_object())throw std::runtime_error("Unsupported Grabber template.");auto draft=project;for(const auto& key:grabberTemplateKeys())draft.erase(key);auto options=grabberTemplateSettings(item["Settings"]);for(auto it=options.begin();it!=options.end();++it)draft[it.key()]=it.value();draft["AppliedTemplateId"]=str(item,"Id");draft.erase("ExploreState");
 // Category deletion may predate this template. Keep the template usable without
 // sending files to an obsolete category or changing an existing job's path.
 auto categories=optionCategories(settings);if(str(draft,"SaveMode")=="Category"&&std::find(categories.begin(),categories.end(),str(draft,"SaveCategory"))==categories.end())draft["SaveCategory"]="Other";
 validateGrabberTemplateOptions(draft,settings);return draft;
}
inline Json grabberProject(const Manager& manager,const std::string& id){Lock lock(manager.mutex);for(const auto& project:manager.state["Projects"])if(str(project,"Id")==id)return project;throw std::runtime_error("The Grabber project no longer exists.");}
inline JobPtr grabberJob(const Manager& manager,const std::string& projectId,const std::string& address){Lock lock(manager.mutex);for(auto job:manager.jobs)if(str(job->data,"ProjectId")==projectId&&str(job->data,"Url")==address)return job;return {};}
inline void setGrabberSelection(Manager& manager,const std::string& id,const std::map<std::string,bool>& selected){Lock lock(manager.mutex);auto project=grabberProject(manager,id);for(auto& link:project["Links"]){auto found=selected.find(str(link,"Url"));if(found!=selected.end())link["Selected"]=found->second;}manager.saveProject(project);}
inline void renameGrabberFile(Manager& manager,const std::string& id,const std::string& address,const std::string& name){
 Lock lock(manager.mutex);if(name.empty()||safeName(name)!=name)throw std::runtime_error("Enter a valid file name without a folder path.");if(grabberJob(manager,id,address))throw std::runtime_error("This file is already in UDM. Change its name through Download Properties.");auto project=grabberProject(manager,id);bool found=false;for(auto& link:project["Links"])if(str(link,"Url")==address){link["SaveName"]=name;found=true;}if(!found)throw std::runtime_error("The collected file no longer exists.");manager.saveProject(project);
}
inline std::vector<size_t> grabberRows(const Json& project,const std::string& mode,const std::string& value){std::vector<size_t> rows;for(size_t i=0;i<project["Links"].size();++i){const auto& link=project["Links"][i];bool match=mode=="all";if(mode=="page")match=str(link,"Referrer")==value;else if(mode=="folder"){Url url(str(link,"Url"));match=(url.origin+url.path).rfind(value,0)==0;}if(match)rows.push_back(i);}return rows;}
inline i64 grabberByteSum(i64 left,i64 right){left=std::max<i64>(0,left);right=std::max<i64>(0,right);return right>INT64_MAX-left?INT64_MAX:left+right;}
inline std::string grabberCollectedName(const Json& project,const Json& link){
 auto name=str(link,"SaveName",str(link,"FileName",grabberFileName(str(link,"Url"))));if(!yes(project,"ConvertLinks")||!str(link,"SaveName").empty())return name;auto type=lower(str(link,"ContentType")),extension=lower(utf8(fs::path(wide(name)).extension().wstring()));if(type.find("html")!=std::string::npos&&extension!=".html"&&extension!=".htm")name+=".html";else if(type.find("text/css")!=std::string::npos&&extension!=".css")name+=".css";return name;
}
inline Json grabberStatistics(const Manager& manager,const Json& project){
 Lock lock(manager.mutex);std::map<std::string,JobPtr> jobs;for(auto job:manager.jobs)if(str(job->data,"ProjectId")==str(project,"Id"))jobs[str(job->data,"Url")]=job;Json stats={{"Files",project["Links"].size()},{"Selected",0},{"Complete",0},{"Active",0},{"Failed",0},{"Queued",0},{"Paused",0},{"Received",0},{"TotalBytes",0},{"UnknownSizes",0},{"Pages",num(project,"PagesVisited")},{"Errors",project.value("Errors",Json::array()).size()},{"Speed",0.0}};
 for(const auto& link:project["Links"]){if(yes(link,"Selected",true))stats["Selected"]=num(stats,"Selected")+1;auto found=jobs.find(str(link,"Url"));auto job=found==jobs.end()?JobPtr{}:found->second;auto size=num(link,"Size",-1);if(job&&num(job->data,"Size",-1)>=0)size=num(job->data,"Size");if(size<0)stats["UnknownSizes"]=num(stats,"UnknownSizes")+1;else stats["TotalBytes"]=grabberByteSum(num(stats,"TotalBytes"),size);
  if(job){auto status=str(job->data,"Status");const char* key=status=="Complete"?"Complete":manager.isActive(job)?"Active":status=="Failed"?"Failed":status=="Queued"?"Queued":"Paused";stats[key]=num(stats,key)+1;stats["Received"]=grabberByteSum(num(stats,"Received"),num(job->data,"Received"));if(status!="Complete")stats["Speed"]=real(stats,"Speed")+job->speed;}
 }return stats;
}
using GrabberCheckpoint=std::function<void(Json&)>;
inline Json grabberTree(const Json& project){
 Json nodes=Json::array({{{"Id","all"},{"Label","All files"},{"Mode","all"},{"Value",""},{"Parent",""}},{{"Id","folders"},{"Label","By folder"},{"Mode","all"},{"Value",""},{"Parent",""}},{{"Id","pages"},{"Label","By referring page"},{"Mode","all"},{"Value",""},{"Parent",""}}});
 std::set<std::string> folders;std::map<std::string,std::string> parents;
 auto pageParents=project.value("PageParents",Json::object());if(pageParents.is_object()&&pageParents.size()<=10000)for(auto it=pageParents.begin();it!=pageParents.end();++it)if(it->is_string())parents[it.key()]=it->get<std::string>();
 for(const auto& link:project["Links"]){Url url(str(link,"Url"));std::string prefix=url.origin+"/",parent="folders";if(folders.insert(prefix).second)nodes.push_back({{"Id","folder:"+prefix},{"Label",url.host},{"Mode","folder"},{"Value",prefix},{"Parent",parent}});parent="folder:"+prefix;
  for(size_t begin=1;;){auto slash=url.path.find('/',begin);if(slash==std::string::npos)break;prefix=url.origin+url.path.substr(0,slash+1);if(folders.insert(prefix).second)nodes.push_back({{"Id","folder:"+prefix},{"Label",unescape(url.path.substr(begin,slash-begin))},{"Mode","folder"},{"Value",prefix},{"Parent",parent}});parent="folder:"+prefix;begin=slash+1;}
  auto referrer=str(link,"Referrer");if(!parents.count(referrer))parents[referrer]="";
 }
 std::set<std::string> added,visiting;std::function<std::string(const std::string&,int)> addPage=[&](const std::string& address,int depth){auto id="page:"+address;if(added.count(address))return id;if(depth>64||!visiting.insert(address).second)return std::string("pages");std::string parent="pages";auto found=parents.find(address);if(found!=parents.end()&&!found->second.empty()&&parents.count(found->second))parent=addPage(found->second,depth+1);nodes.push_back({{"Id",id},{"Label",address.empty()?"Direct files":address},{"Mode","page"},{"Value",address},{"Parent",parent}});visiting.erase(address);added.insert(address);return id;};
 for(const auto& item:parents)addPage(item.first,0);return nodes;
}
Json exploreWithUpdates(const Json&,const Json&,const Cancel&,std::function<void(std::string)>,GrabberCheckpoint);
Json runGrabber(Manager&,const Json&,const Cancel&,std::function<void(std::string)> report={},std::function<void(const Json&)> update={});
}
