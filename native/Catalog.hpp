#include "BrowserProxy.hpp"
#pragma once
#include "GuiModels.hpp"
#include "OfflineSite.hpp"
namespace udm {
inline Json exportCatalog(Manager& manager,const std::vector<JobPtr>& selected,bool credentials=false){
 Lock lock(manager.mutex);Json records=Json::array(),queues=Json::array();std::set<std::string> names;
 for(auto j:selected){Json record=Json::object();for(const char* key:{"Url","FileName","Folder","Category","Queue","Description","DownloadPage","Connections","LimitKbps","ExpectedSha256","Status","Size","Sha256","Added","Finished","ETag","Modified"})if(j->data.contains(key))record[key]=j->data[key];if(!str(j->data,"SourceUrl").empty()||!str(j->data,"ProtectedAdaptive").empty())record["RequiresMediaCapture"]=true;if(!str(j->data,"ProtectedRequest").empty()||yes(j->data,"RequiresRequestCapture")){record["RequiresRequestCapture"]=true;if(credentials&&!str(j->data,"ProtectedRequest").empty()){record["ProtectedRequest"]=str(j->data,"ProtectedRequest");record["RequiresRequestCapture"]=false;}}if(j->data.contains("OfflineProject"))record["OfflineProject"]=j->data["OfflineProject"];if(!str(j->data,"ProtectedBrowserProxy").empty()||yes(j->data,"RequiresBrowserProxyCapture")){record["RequiresBrowserProxyCapture"]=true;if(credentials&&!str(j->data,"ProtectedBrowserProxy").empty()){record["ProtectedBrowserProxy"]=str(j->data,"ProtectedBrowserProxy");record["RequiresBrowserProxyCapture"]=false;}}if(credentials)record["ProtectedHeaders"]=str(j->data,"ProtectedHeaders");records.push_back(record);names.insert(str(record,"Queue"));}
 for(auto q:manager.state["Queues"])if(names.count(str(q,"Name")))queues.push_back(q);
 return {{"Format","UDM catalog"},{"Version",1},{"Exported",date()},{"Credentials",credentials?"Windows account encrypted":"Omitted"},{"Downloads",records},{"Queues",queues}};
}
inline size_t importCatalog(Manager& manager,const Json& catalog,const std::string& destination,bool credentials=false,bool attachExisting=false){
 if(str(catalog,"Format")!="UDM catalog"||num(catalog,"Version")!=1||!catalog.contains("Downloads")||!catalog["Downloads"].is_array()||catalog["Downloads"].size()>10000)throw std::runtime_error("Choose a valid UDM catalog with at most 10,000 downloads.");
 if(!destination.empty()&&!fs::path(wide(destination)).is_absolute())throw std::runtime_error("Choose an absolute destination folder.");
 Lock lock(manager.mutex);auto previousJobs=manager.jobs,staged=previousJobs;auto previousQueues=manager.state["Queues"],queues=previousQueues;std::map<std::string,std::string> queueNames;
 for(auto q:queues)queueNames[lower(str(q,"Name"))]=str(q,"Name");auto cats=manager.categories();std::set<std::string> paths;
 for(auto j:staged){paths.insert(lower(utf8(j->target().wstring())));if(!str(j->data,"PreviousPath").empty())paths.insert(lower(str(j->data,"PreviousPath")));}
 for(auto entry:catalog["Downloads"]){
  Url url(str(entry,"Url"));auto name=str(entry,"FileName"),folder=destination.empty()?str(entry,"Folder"):destination,queue=trim(str(entry,"Queue","Imported"));if(name.empty()||safeName(name)!=name||!fs::path(wide(folder)).is_absolute()||queue.empty()||queue.size()>80)throw std::runtime_error("Catalog has an invalid file name, folder or queue.");
  Headers headers;if(credentials&&!str(entry,"ProtectedHeaders").empty()){headers=readHeaders(entry);validateHeaders(headers);}
  bool duplicate=false;for(auto j:staged)duplicate|=str(j->data,"Url")==url.full&&str(j->data,"FileName")==name&&lower(str(j->data,"Folder"))==lower(folder)&&readHeaders(j->data)==headers;if(duplicate)continue;
  if(queueNames.count(lower(queue)))queue=queueNames[lower(queue)];else{auto q=defaultQueue(queue);q["Enabled"]=false;
   if(catalog.contains("Queues")&&catalog["Queues"].is_array())for(auto original:catalog["Queues"])if(str(original,"Name")==queue){q["Parallel"]=std::clamp<i64>(num(original,"Parallel",2),1,16);q["Retries"]=std::clamp<i64>(num(original,"Retries",3),0,10);break;}
   queues.push_back(q);queueNames[lower(queue)]=queue;
  }
  auto path=fs::path(wide(folder))/wide(name);bool complete=attachExisting&&str(entry,"Status")=="Complete"&&std::regex_match(str(entry,"Sha256"),std::regex("[a-fA-F0-9]{64}"))&&!paths.count(lower(utf8(path.wstring())))&&fs::is_regular_file(path)&&fileHash(path)==lower(str(entry,"Sha256"));
  if(!complete){auto original=fs::path(wide(name));for(int i=1;paths.count(lower(utf8(path.wstring())))||fs::exists(path);++i){name=utf8(original.stem().wstring())+" ("+std::to_string(i)+")"+utf8(original.extension().wstring());path=fs::path(wide(folder))/wide(name);}}
  paths.insert(lower(utf8(path.wstring())));Json data={{"Id",guid()},{"Url",url.full},{"FileName",name},{"Folder",folder},{"Queue",queue},{"Category",category(name)},{"Status",complete?"Complete":"Paused"},{"QueueMember",!complete},{"QueueOrigin",false},{"Added",date()},{"Size",complete?(i64)fs::file_size(path):-1},{"Received",complete?(i64)fs::file_size(path):0},{"Segments",Json::array()},{"Error",""},{"ProtectedHeaders",headers.empty()?"":protect(legacyDictionary(Json(headers)).dump())}};
  for(const char* key:{"Description","DownloadPage","Connections","LimitKbps","ExpectedSha256"})if(entry.contains(key))data[key]=entry[key];
  if(std::find(cats.begin(),cats.end(),str(entry,"Category"))!=cats.end())data["Category"]=entry["Category"];
  if(complete)for(const char* key:{"Sha256","Finished","ETag","Modified"})if(entry.contains(key))data[key]=entry[key];
  if(!str(entry,"ProtectedRequest").empty()&&credentials)data["ProtectedRequest"]=protect(readPostRequest(entry).dump());else if((yes(entry,"RequiresRequestCapture")||!str(entry,"ProtectedRequest").empty())&&!complete){data["RequiresRequestCapture"]=true;data["Status"]="Needs browser capture";data["QueueMember"]=false;}if(yes(entry,"RequiresMediaCapture")&&!complete){data["RequiresMediaCapture"]=true;data["Status"]="Needs browser capture";data["QueueMember"]=false;}if(entry.contains("OfflineProject")){validateOfflineProject(entry["OfflineProject"]);if(str(entry["OfflineProject"],"StartUrl")!=url.full)throw std::runtime_error("Offline project address does not match its download.");data["OfflineProject"]=entry["OfflineProject"];}if(credentials&&!str(entry,"ProtectedBrowserProxy").empty())data["ProtectedBrowserProxy"]=protect(readBrowserProxy(entry).dump());else if(yes(entry,"RequiresBrowserProxyCapture")||!str(entry,"ProtectedBrowserProxy").empty()){data["RequiresBrowserProxyCapture"]=true;if(!complete){data["Status"]="Needs browser capture";data["QueueMember"]=false;}}validateFileMetadata(data);staged.push_back(std::make_shared<Job>(data));
 }
 manager.jobs=staged;manager.state["Queues"]=queues;try{manager.save();}catch(...){manager.jobs=previousJobs;manager.state["Queues"]=previousQueues;throw;}return staged.size()-previousJobs.size();
}
}
