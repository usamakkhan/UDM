#pragma once
#include "GrabberRendered.hpp"
#include "GrabberProject.hpp"
namespace udm {
inline Json renderedGrabberPolicy(const Json& project){auto policy=grabberTemplateSettings(project);for(const char* key:{"StartUrl","ProtectedLogin","ProtectedBrowserSession","LogoutPages"})if(project.contains(key))policy[key]=project[key];return policy;}
inline void saveRenderedGrabberState(Manager& manager,const Json& records){if(manager.catalogTransaction)throw std::runtime_error("Finish the active catalog operation before capturing a rendered page.");auto old=manager.state;manager.state["RenderedGrabber"]=records;try{manager.save();}catch(...){manager.state=std::move(old);throw;}}
inline Json beginRenderedGrabberRequest(Manager& manager,const std::string& id){
 Lock lock(manager.mutex);auto project=grabberProject(manager,id);if(yes(project,"ExplorationRunning"))throw std::runtime_error("Stop exploration before capturing a rendered page.");
 auto records=manager.state.value("RenderedGrabber",Json::object());if(!records.is_object()||records.size()>100)throw std::runtime_error("Invalid rendered Grabber state.");std::set<std::string> existing;for(const auto& saved:manager.state["Projects"])existing.insert(str(saved,"Id"));for(auto it=records.begin();it!=records.end();)if(!existing.count(it.key()))it=records.erase(it);else ++it;if(!records.contains(id)&&records.size()==100)throw std::runtime_error("Too many rendered Grabber projects.");
 auto generation=records.contains(id)?num(records[id],"generation"):0;if(generation<0||generation==INT64_MAX)throw std::runtime_error("Invalid rendered Grabber generation.");
 Json request={{"projectId",id},{"ticket",guid()},{"generation",generation+1},{"url",grabberCanonical(str(project,"StartUrl"))},{"expires",epoch()+15*60*1000}};
 records[id]={{"generation",generation+1},{"pending",request},{"policy",renderedGrabberPolicy(project)}};saveRenderedGrabberState(manager,records);return request;
}
inline void cancelRenderedGrabberRequest(Manager& manager,const std::string& id){Lock lock(manager.mutex);auto records=manager.state.value("RenderedGrabber",Json::object());if(!records.contains(id))return;records[id].erase("pending");saveRenderedGrabberState(manager,records);}
inline Json completeRenderedGrabberRequest(Manager& manager,const Json& message){
 Lock lock(manager.mutex);auto id=str(message,"projectId");auto project=grabberProject(manager,id);auto records=manager.state.value("RenderedGrabber",Json::object());
 if(!records.contains(id)||yes(project,"ExplorationRunning")||records[id].value("policy",Json{})!=renderedGrabberPolicy(project))throw std::runtime_error("Rendered Grabber project changed. Capture it again.");
 auto snapshot=validateRenderedGrabberReply(message,records[id].value("pending",Json::object()),epoch());records[id]["protectedSnapshot"]=protect(snapshot.dump());records[id]["receipt"]={{"ticket",message["ticket"]},{"generation",message["generation"]}};records[id].erase("pending");saveRenderedGrabberState(manager,records);return snapshot;
}
}
