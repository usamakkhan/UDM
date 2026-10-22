#pragma once
#include "GrabberRendered.hpp"
#include "GrabberProject.hpp"
namespace udm {
// Only disclose projects eligible for an explicit capture of this page.
inline Json renderedGrabberProjects(Manager& manager,const std::string& address){
 if(address.empty()||address.size()>16000||std::any_of(address.begin(),address.end(),[](unsigned char c){return c<32||c==127;}))throw std::runtime_error("Open the project website first.");
 auto url=grabberCanonical(address);Lock lock(manager.mutex);Json result=Json::array();
 for(const auto& project:manager.state["Projects"]){
  if(yes(project,"ExplorationRunning"))continue;
  try{if(grabberCanonical(str(project,"StartUrl"))!=url)continue;}catch(const std::exception&){continue;}
  result.push_back({{"projectId",str(project,"Id")},{"name",str(project,"Name")}});
  if(result.size()==100)break;
 }
 return result;
}
inline Json renderedGrabberPolicy(const Json& project){auto policy=grabberTemplateSettings(project);for(const char* key:{"StartUrl","ProtectedLogin","ProtectedBrowserSession","LogoutPages"})if(project.contains(key))policy[key]=project[key];return policy;}
inline void saveRenderedGrabberState(Manager& manager,const Json& records){if(manager.catalogTransaction)throw std::runtime_error("Finish the active catalog operation before capturing a rendered page.");auto old=manager.state;manager.state["RenderedGrabber"]=records;try{manager.save();}catch(...){manager.state=std::move(old);throw;}}
inline Json beginRenderedGrabberRequest(Manager& manager,const std::string& id){
 Lock lock(manager.mutex);auto project=grabberProject(manager,id);if(yes(project,"ExplorationRunning"))throw std::runtime_error("Stop exploration before capturing a rendered page.");
 auto records=manager.state.value("RenderedGrabber",Json::object());if(!records.is_object()||records.size()>100)throw std::runtime_error("Invalid rendered Grabber state.");std::set<std::string> existing;for(const auto& saved:manager.state["Projects"])existing.insert(str(saved,"Id"));for(auto it=records.begin();it!=records.end();)if(!existing.count(it.key()))it=records.erase(it);else ++it;if(!records.contains(id)&&records.size()==100)throw std::runtime_error("Too many rendered Grabber projects.");
 auto generation=records.contains(id)?num(records[id],"generation"):0;if(generation<0||generation==INT64_MAX)throw std::runtime_error("Invalid rendered Grabber generation.");
 Json request={{"projectId",id},{"ticket",guid()},{"generation",generation+1},{"url",grabberCanonical(str(project,"StartUrl"))},{"expires",epoch()+15*60*1000}};
 // Negotiate with the running desktop, not the independently updated native host.
 request["captureHtml"]=str(project,"Template")=="Offline website (ZIP)";
 // A failed refresh must not discard the last successful capture. Preserve it
 // only while the project policy still matches; completion replaces it atomically.
 auto policy=renderedGrabberPolicy(project);auto previous=records.value(id,Json::object());
 records[id]={{"generation",generation+1},{"pending",request},{"policy",policy}};
 if(previous.value("policy",Json{})==policy&&previous.contains("protectedSnapshot")&&previous.contains("receipt")){
  records[id]["protectedSnapshot"]=previous["protectedSnapshot"];records[id]["receipt"]=previous["receipt"];
 }
 saveRenderedGrabberState(manager,records);return request;
}
inline bool cancelRenderedGrabberRequest(Manager& manager,const std::string& id,const std::string& ticket){
 Lock lock(manager.mutex);if(ticket.empty()||ticket.size()>128)throw std::runtime_error("Choose the rendered request to cancel.");auto records=manager.state.value("RenderedGrabber",Json::object());
 if(!records.contains(id)||!records[id].contains("pending")||str(records[id]["pending"],"ticket")!=ticket)return false;
 records[id].erase("pending");saveRenderedGrabberState(manager,records);return true;
}
inline Json completeRenderedGrabberRequest(Manager& manager,const Json& message){
 Lock lock(manager.mutex);auto id=str(message,"projectId");auto project=grabberProject(manager,id);auto records=manager.state.value("RenderedGrabber",Json::object());
 if(!records.contains(id)||yes(project,"ExplorationRunning")||records[id].value("policy",Json{})!=renderedGrabberPolicy(project))throw std::runtime_error("Rendered Grabber project changed. Capture it again.");
 auto snapshot=validateRenderedGrabberReply(message,records[id].value("pending",Json::object()),epoch());records[id]["protectedSnapshot"]=protect(snapshot.dump());records[id]["receipt"]={{"ticket",message["ticket"]},{"generation",message["generation"]}};records[id].erase("pending");saveRenderedGrabberState(manager,records);return snapshot;
}
inline Json loadRenderedGrabberSnapshot(Manager& manager,const Json& project){
 Lock lock(manager.mutex);auto records=manager.state.value("RenderedGrabber",Json::object());auto id=str(project,"Id");
 if(!records.contains(id)||!records[id].contains("protectedSnapshot"))return Json::object();
 const auto& record=records[id];if(record.value("policy",Json{})!=renderedGrabberPolicy(project))throw std::runtime_error("Project settings changed. Capture the browser page again before exploring.");
 auto snapshot=validateRenderedGrabberSnapshot(Json::parse(reveal(str(record,"protectedSnapshot"))),grabberCanonical(str(project,"StartUrl")));
 return {{"snapshot",snapshot},{"ticket",str(record.value("receipt",Json::object()),"ticket")}};
}

}
