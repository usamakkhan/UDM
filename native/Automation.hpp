#pragma once
// Windows download automation: validation and catalog operations shared by
// the out-of-process COM server (AutomationMain.cpp), the desktop's local
// pipe and the batch review dialog. Single links follow the published
// single-link workflow (flags 1 = silent, 2 = queue only); link arrays are
// stored encrypted until the user reviews them.
#include "Core.hpp"
#include "BrowserRequest.hpp"
#include "GuiModels.hpp"
#include <algorithm>
#include <set>
namespace udm {
inline std::string automationText(const Json& input,const char* key,size_t limit){
 if(!input.contains(key))return {};
 if(!input[key].is_string())throw std::invalid_argument("Automation text has an invalid type.");
 auto value=input[key].get<std::string>();if(value.size()>limit||value.find('\0')!=std::string::npos)throw std::invalid_argument("Automation text exceeds its limit or contains a NUL.");return value;
}
inline Json validateAutomationLink(const Json& input){
 if(!input.is_object())throw std::invalid_argument("Expected an automation link.");
 for(auto it=input.begin();it!=input.end();++it)if(!std::set<std::string>{"url","referrer","cookies","data","user","password","folder","filename","flags","userAgent","description"}.count(it.key()))throw std::invalid_argument("Unknown automation link field.");
 Json value=Json::object();for(auto key:{"url","referrer","cookies","user","password","folder","filename","userAgent","description"})value[key]=automationText(input,key,16384);
 value["data"]=automationText(input,"data",MaxBrowserPostBytes);
 if(input.contains("flags")&&!input["flags"].is_number_integer())throw std::invalid_argument("Automation flags must be an integer.");
 auto flags=num(input,"flags");if(flags<0||flags>3)throw std::invalid_argument("Only automation flags 1 (silent) and 2 (queue only) are supported.");value["flags"]=flags;
 Url url(str(value,"url"));if(!str(value,"referrer").empty()){Url ref(str(value,"referrer"));if(ref.scheme!="http"&&ref.scheme!="https")throw std::invalid_argument("The referring page must use HTTP or HTTPS.");}
 auto folder=str(value,"folder"),name=str(value,"filename");if(!folder.empty()&&!fs::path(wide(folder)).is_absolute())throw std::invalid_argument("Use an absolute download folder.");
 if(!name.empty()&&(name=="."||name==".."||name.find_first_of("/\\:")!=std::string::npos||safeName(name)!=name))throw std::invalid_argument("Use a valid file name without a path.");
 auto headers=browserHeaders(value);if(!str(value,"user").empty()||!str(value,"password").empty())setBasicLogin(headers,str(value,"user"),str(value,"password"));validateHeaders(headers);
 if(url.scheme=="ftp"&&(!str(value,"cookies").empty()||!str(value,"data").empty()))throw std::invalid_argument("FTP links cannot use cookies or POST data.");
 return value;
}
inline Headers automationHeaders(const Json& link){auto headers=browserHeaders(link);if(!str(link,"user").empty()||!str(link,"password").empty())setBasicLogin(headers,str(link,"user"),str(link,"password"));return headers;}
inline Json automationPost(const Json& link){auto data=str(link,"data");if(data.empty())return Json::object();return validatePostRequest({{"method","POST"},{"url",str(link,"url")},{"contentType","application/x-www-form-urlencoded"},{"body",b64(Bytes(data.begin(),data.end()))}},str(link,"url"));}
// Adds one automation link. origin carries batch attribution (AutomationBatchId,
// AutomationIndex) so a reviewed row is never added twice.
inline JobPtr receiveAutomationLink(Manager& manager,const Json& input,const std::string& queue="Main queue",const Json& origin=Json::object()){
 auto link=validateAutomationLink(input);auto headers=automationHeaders(link);auto post=automationPost(link);auto flags=num(link,"flags");
 const std::string status=(flags&2)?"Paused":(flags&1)?"Queued":"Awaiting confirmation";
 Lock lock(manager.mutex);auto previous=manager.jobs;
 auto apply=[&](JobPtr job){for(auto it=origin.begin();it!=origin.end();++it)job->data[it.key()]=it.value();job->data["Description"]=str(link,"description");if(!str(link,"referrer").empty())job->data["DownloadPage"]=str(link,"referrer");job->data["SuppressProgressDialog"]=(flags&1)!=0;job->data["SuppressCompletionDialog"]=(flags&1)!=0;};
 // Silent calls select a fresh numbered destination instead of requiring a duplicate prompt.
 // Existing active work is retained; an API call must never reset it.
 if(flags&1){auto old=manager.findDuplicate(str(link,"url"),headers,{},post);if(old&&(manager.isActive(old)||str(old->data,"Status")=="Queued"))return old;
  auto job=manager.add(str(link,"url"),str(link,"folder"),str(link,"filename"),queue,true,headers,"",post);apply(job);job->data["Status"]=status;manager.save();return job;
 }
 auto job=manager.offerDownload(str(link,"url"),str(link,"folder"),str(link,"filename"),queue,true,headers,post);
 if(std::find(previous.begin(),previous.end(),job)!=previous.end()){manager.pendingOffers.push_back(job);return job;}
 // offerDownload preserves duplicate-review states; only ordinary new offers change status.
 apply(job);if(str(job->data,"DuplicateOf").empty())job->data["Status"]=status;manager.save();return job;
}
inline Json validateAutomationBatch(const Json& message){
 if(!message.is_object()||!message.contains("links")||!message["links"].is_array()||message["links"].empty()||message["links"].size()>1000)throw std::invalid_argument("Supply between 1 and 1000 links.");
 auto ref=automationText(message,"referrer",16384),agent=automationText(message,"userAgent",16384);Json links=Json::array();
 for(auto row:message["links"]){if(!row.is_object())throw std::invalid_argument("Invalid batch row.");row["referrer"]=ref;row["userAgent"]=agent;row["flags"]=3;links.push_back(validateAutomationLink(row));}
 if(links.dump().size()>512*1024)throw std::invalid_argument("The automation batch exceeds 512 KiB.");return links;
}
inline std::string queueAutomationBatch(Manager& manager,const Json& message){
 auto links=validateAutomationBatch(message);auto payload=protect(links.dump());Lock lock(manager.mutex);auto before=manager.state.value("AutomationOffers",Json::array());
 if(!before.is_array()||before.size()>=16)throw std::runtime_error("Review existing automation batches before adding more.");
 size_t total=payload.size();for(auto offer:before)total+=str(offer,"ProtectedLinks").size();if(total>4*1024*1024)throw std::runtime_error("Pending automation batches exceed the storage limit.");
 auto id=guid();auto next=before;next.push_back({{"Id",id},{"Created",date()},{"ProtectedLinks",payload}});manager.state["AutomationOffers"]=next;
 try{manager.save();}catch(...){manager.state["AutomationOffers"]=before;throw;}return id;
}
inline Json nextAutomationBatch(Manager& manager){Lock lock(manager.mutex);auto offers=manager.state.value("AutomationOffers",Json::array());if(!offers.is_array()||offers.empty())return Json::object();auto result=offers.front();result["links"]=Json::parse(reveal(str(result,"ProtectedLinks")));return result;}
inline bool automationRowAdded(Manager& manager,const std::string& id,int index){Lock lock(manager.mutex);for(auto job:manager.jobs)if(str(job->data,"AutomationBatchId")==id&&num(job->data,"AutomationIndex",-1)==index)return true;for(auto offer:manager.state.value("AutomationOffers",Json::array()))if(str(offer,"Id")==id)for(auto accepted:offer.value("Accepted",Json::array()))if(accepted==index)return true;return false;}
inline void acceptAutomationRow(Manager& manager,const Json& offer,int index,const std::string& folder,const std::string& queue,bool later){
 Lock lock(manager.mutex);auto id=str(offer,"Id");if(id.empty()||!offer.contains("links")||index<0||index>=(int)offer["links"].size())throw std::invalid_argument("Invalid automation selection.");
 if(automationRowAdded(manager,id,index))return;bool exists=false;for(auto item:manager.state.value("AutomationOffers",Json::array()))exists|=str(item,"Id")==id;if(!exists)throw std::runtime_error("This automation offer is no longer pending.");
 auto link=offer["links"][index];link["folder"]=folder;link["flags"]=later?3:1;
 // The attribution is written with the new job, and the accepted index is
 // recorded in the same catalog file, so a restarted review skips the row.
 receiveAutomationLink(manager,link,queue,{{"AutomationBatchId",id},{"AutomationIndex",index}});
 auto before=manager.state["AutomationOffers"];for(auto& pending:manager.state["AutomationOffers"])if(str(pending,"Id")==id){if(!pending.contains("Accepted"))pending["Accepted"]=Json::array();pending["Accepted"].push_back(index);}
 try{manager.save();}catch(...){manager.state["AutomationOffers"]=before;throw;}
}
inline void dismissAutomationBatch(Manager& manager,const std::string& id){
 Lock lock(manager.mutex);auto before=manager.state.value("AutomationOffers",Json::array());auto next=Json::array();for(auto offer:before)if(str(offer,"Id")!=id)next.push_back(offer);manager.state["AutomationOffers"]=next;
 try{manager.save();}catch(...){manager.state["AutomationOffers"]=before;throw;}
}
// Sends an automation request to the running desktop over the local pipe,
// starting the desktop in the background when it is not running.
Json automationCommand(const Json&,const fs::path& data=fs::path());
}
