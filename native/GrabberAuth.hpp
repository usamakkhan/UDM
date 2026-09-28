#pragma once
#include "GrabberFilters.hpp"
#include "GuiModels.hpp"
#include "BrowserSession.hpp"
namespace udm {
inline void validateGrabberBrowserLogin(const Json& project){
 if(project.contains("BrowserLogin")&&!project["BrowserLogin"].is_boolean())throw std::runtime_error("Invalid browser sign-in setting.");
 for(const char* key:{"LoginPage","LogoutPages"})if(project.contains(key)&&!project[key].is_string())throw std::runtime_error("Invalid browser sign-in page.");
 grabberPatterns(str(project,"LogoutPages"));if(!str(project,"LoginPage").empty())grabberCanonical(str(project,"LoginPage"));
 if(!yes(project,"BrowserLogin"))return;
 if(!str(project,"ProtectedLogin").empty())throw std::runtime_error("Choose HTTP login or browser sign-in for this project.");
 auto session=readBrowserSession(project);if(!session.empty()&&str(session,"Origin")!=Url(str(project,"StartUrl")).origin)throw std::runtime_error("The starting website changed. Clear its browser session and sign in again.");
}
inline Json grabberBrowserSession(const Json& project,bool required=true){
 validateGrabberBrowserLogin(project);if(!yes(project,"BrowserLogin"))return Json::object();auto session=readBrowserSession(project);if(!session.empty())session["LogoutPages"]=str(project,"LogoutPages");
 if(required&&(session.empty()||project.contains("PendingBrowserLogin")))throw std::runtime_error("Finish browser sign-in from the UDM extension before exploring this website.");return session;
}
inline Json grabberLogin(const Json& project){
 if(!project.contains("ProtectedLogin"))return Json::object();
 if(!project["ProtectedLogin"].is_string()||str(project,"ProtectedLogin").size()>32768)throw std::runtime_error("Invalid saved project login.");
 if(str(project,"ProtectedLogin").empty())return Json::object();
 Json login;try{login=Json::parse(reveal(str(project,"ProtectedLogin")));}catch(...){throw std::runtime_error("The project login cannot be read. Enter it again.");}
 if(!login.is_object()||!login.contains("Origin")||!login["Origin"].is_string()||!login.contains("UserName")||!login["UserName"].is_string()||!login.contains("Password")||!login["Password"].is_string())throw std::runtime_error("Invalid saved project login.");
 Url origin(str(login,"Origin"));if((origin.scheme!="http"&&origin.scheme!="https")||origin.origin!=str(login,"Origin"))throw std::runtime_error("Invalid project login website.");
 if(Url(str(project,"StartUrl")).origin!=origin.origin)throw std::runtime_error("The starting website changed. Clear the saved project login or enter the login for the new website.");
 Headers checked;setBasicLogin(checked,str(login,"UserName"),str(login,"Password"));return login;
}
inline void setGrabberLogin(Json& project,const std::string& user,const std::string& password,bool enabled=true){
 if(!enabled){project.erase("ProtectedLogin");return;}
 Url url(str(project,"StartUrl"));if(url.scheme!="http"&&url.scheme!="https")throw std::runtime_error("Project logins require an HTTP or HTTPS starting page.");
 Headers checked;setBasicLogin(checked,user,password);
 if(project.contains("ProtectedLogin")&&!str(project,"ProtectedLogin").empty())try{auto previous=grabberLogin(project);if(str(previous,"UserName")==user&&str(previous,"Password")==password)return;}catch(...){}
 auto secret=protect(Json{{"Origin",url.origin},{"UserName",user},{"Password",password}}.dump());
 if(secret.size()>32768)throw std::runtime_error("The project login is too large.");
 project["ProtectedLogin"]=secret;
}
inline Headers grabberRequestHeaders(const Json& project,const std::string& referrer,const std::string& address){
 auto headers=grabberHeaders(referrer,address);auto login=grabberLogin(project);
 if(!login.empty()&&Url(address).origin==str(login,"Origin"))setBasicLogin(headers,str(login,"UserName"),str(login,"Password"));
 return browserSessionHeaders(grabberBrowserSession(project),address,std::move(headers));
}
}
