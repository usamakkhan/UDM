#pragma once
#include "GrabberProject.hpp"
namespace udm {
inline Json beginGrabberBrowserLogin(Manager& manager,Json project){
 Lock lock(manager.mutex);if(!yes(project,"BrowserLogin"))throw std::runtime_error("Enable manual browser sign-in for this project first.");
 project.erase("ProtectedBrowserSession");project.erase("LastBrowserLoginTicket");validateGrabberBrowserLogin(project);if(yes(project,"ExplorationRunning"))throw std::runtime_error("Stop exploration before replacing its website session.");
 auto login=str(project,"LoginPage");if(login.empty())login=str(project,"StartUrl");
 project["PendingBrowserLogin"]={{"Token",guid()},{"Expires",epoch()+15*60*1000},{"StartUrl",grabberCanonical(str(project,"StartUrl"))},{"LoginPage",Url(login).full}};
 manager.saveProject(project);return project;
}
inline Json pendingGrabberBrowserLogins(const Manager& manager,const std::string& address){
 Url current(address);if(current.scheme!="http"&&current.scheme!="https")throw std::runtime_error("Open the project's website in the browser.");
 Lock lock(manager.mutex);Json items=Json::array();
 for(const auto& project:manager.state["Projects"]){
  if(!yes(project,"BrowserLogin")||!project.contains("PendingBrowserLogin"))continue;
  const auto& pending=project["PendingBrowserLogin"];if(!pending.is_object()||num(pending,"Expires")<epoch()||num(pending,"Expires")>epoch()+15*60*1000||str(pending,"Token").empty())continue;
  try{auto source=grabberCanonical(str(project,"StartUrl"));if(source!=str(pending,"StartUrl")||Url(source).origin!=current.origin)continue;
   auto domain=PublicSuffix::installed().domain(current.host);if(domain.empty())domain=current.host;
   items.push_back({{"id",str(project,"Id")},{"name",str(project,"Name")},{"ticket",str(pending,"Token")},{"url",source},{"origin",current.origin},{"cookieDomain",domain},{"expires",num(pending,"Expires")}});
  }catch(...){}if(items.size()>=50)break;
 }return {{"ok",true},{"projects",items}};
}
inline Json completeGrabberBrowserLogin(Manager& manager,const Json& message){
 Lock lock(manager.mutex);auto project=grabberProject(manager,str(message,"projectId"));
 if(!yes(project,"BrowserLogin")||yes(project,"ExplorationRunning"))throw std::runtime_error("This project is not waiting for browser sign-in.");
 auto source=grabberCanonical(str(project,"StartUrl"));Url page(str(message,"url"));if(page.origin!=Url(source).origin)throw std::runtime_error("Return to the project's website before sharing its session.");
 if(!message.contains("session"))throw std::runtime_error("No browser session was supplied.");auto supplied=message["session"];supplied["LogoutPages"]=str(project,"LogoutPages");auto session=validateBrowserSession(supplied);
 if(str(session,"Origin")!=page.origin)throw std::runtime_error("The browser session belongs to a different website.");
 auto ticket=str(message,"ticket");if(ticket.empty()||ticket.size()>64)throw std::runtime_error("Invalid browser sign-in request.");
 auto pending=project.value("PendingBrowserLogin",Json::object());
 if(pending.empty()&&str(project,"LastBrowserLoginTicket")==ticket&&readBrowserSession(project)==session)return {{"ok",true},{"projectId",str(project,"Id")},{"alreadySaved",true}};
 if(str(pending,"Token")!=ticket||num(pending,"Expires")<epoch()||num(pending,"Expires")>epoch()+15*60*1000||str(pending,"StartUrl")!=source)throw std::runtime_error("This sign-in request expired or changed. Start browser sign-in again in UDM.");
 project["ProtectedBrowserSession"]=protect(session.dump());project["LastBrowserLoginTicket"]=ticket;project["BrowserLoginSaved"]=date();project.erase("PendingBrowserLogin");project.erase("ExploreState");
 manager.saveProject(project);return {{"ok",true},{"projectId",str(project,"Id")}};
}
inline void cancelGrabberBrowserLogin(Manager& manager,const std::string& id){Lock lock(manager.mutex);auto project=grabberProject(manager,id);project.erase("PendingBrowserLogin");manager.saveProject(project);}
}
