#pragma once
#include "GrabberFilters.hpp"
#include "GuiModels.hpp"
#include <cmath>
namespace udm {
inline bool sessionDomainMatches(std::string host,std::string domain,bool hostOnly){
 host=lower(host);domain=lower(domain);if(!domain.empty()&&domain.front()=='.')domain.erase(0,1);
 return host==domain||(!hostOnly&&host.size()>domain.size()&&host.compare(host.size()-domain.size(),domain.size(),domain)==0&&host[host.size()-domain.size()-1]=='.');
}
inline Json validateBrowserSession(const Json& input){
 if(!input.is_object()||input.dump().size()>98304)throw std::runtime_error("Invalid website session.");
 for(const char* key:{"Origin","UserAgent","LogoutPages"})if(!input.contains(key)||!input[key].is_string())throw std::runtime_error("Invalid website session fields.");
 Url origin(str(input,"Origin"));if((origin.scheme!="http"&&origin.scheme!="https")||origin.origin!=str(input,"Origin"))throw std::runtime_error("Invalid website session origin.");
 auto agent=str(input,"UserAgent");if(agent.empty()||agent.size()>2048||std::any_of(agent.begin(),agent.end(),[](unsigned char c){return c<32||c==127;}))throw std::runtime_error("Invalid browser identity.");
 grabberPatterns(str(input,"LogoutPages"));
 if(!input.contains("Cookies")||!input["Cookies"].is_array()||input["Cookies"].size()>512)throw std::runtime_error("Too many website cookies.");
 Json cookies=Json::array();size_t total=0;std::set<std::string> identities;
 for(const auto& cookie:input["Cookies"]){
  if(!cookie.is_object())throw std::runtime_error("Invalid website cookie.");
  for(const char* key:{"name","value","domain","path"})if(!cookie.contains(key)||!cookie[key].is_string())throw std::runtime_error("Invalid website cookie text.");
  for(const char* key:{"secure","hostOnly"})if(!cookie.contains(key)||!cookie[key].is_boolean())throw std::runtime_error("Invalid website cookie scope.");
  auto name=str(cookie,"name"),value=str(cookie,"value"),domain=lower(str(cookie,"domain")),path=str(cookie,"path");if(!domain.empty()&&domain.front()=='.')domain.erase(0,1);
  total+=name.size()+value.size()+1+(cookies.empty()?0:2);if(total>16384||name.size()>4096||value.size()>8192||domain.size()>255||domain.empty()||path.empty()||path.front()!='/'||path.size()>8192)throw std::runtime_error("Website cookies exceed the size limit.");
  auto invalid=[](const std::string& s,bool name){return std::any_of(s.begin(),s.end(),[&](unsigned char c){return c<32||c==127||c==';'||(name&&(c=='='||c==' '||c=='\t'));});};
  if(invalid(name,true)||invalid(value,false)||invalid(domain,false)||invalid(path,false)||!sessionDomainMatches(origin.host,domain,yes(cookie,"hostOnly")))throw std::runtime_error("Website cookie does not match the selected website.");
  if(cookie.contains("expirationDate")&&(!cookie["expirationDate"].is_number()||!std::isfinite(real(cookie,"expirationDate"))||real(cookie,"expirationDate")<0||real(cookie,"expirationDate")>253402300799.0))throw std::runtime_error("Invalid cookie expiry.");
  if(cookie.contains("partitioned")&&!cookie["partitioned"].is_boolean())throw std::runtime_error("Invalid cookie partition scope.");
  auto identity=domain+"\n"+path+"\n"+name+(yes(cookie,"partitioned")?"\npartitioned":"\nordinary");if(!identities.insert(identity).second)throw std::runtime_error("Ambiguous website cookies.");
  Json clean={{"name",name},{"value",value},{"domain",domain},{"path",path},{"secure",yes(cookie,"secure")},{"hostOnly",yes(cookie,"hostOnly")}};
  if(cookie.contains("expirationDate"))clean["expirationDate"]=cookie["expirationDate"];if(yes(cookie,"partitioned"))clean["partitioned"]=true;cookies.push_back(std::move(clean));
 }
 return {{"Origin",origin.origin},{"UserAgent",agent},{"LogoutPages",str(input,"LogoutPages")},{"Cookies",cookies}};
}
inline Json readBrowserSession(const Json& data){
 if(yes(data,"RequiresBrowserSessionCapture"))throw std::runtime_error("Refresh this download from its signed-in browser page, or create it again through Site Grabber.");
 if(!data.contains("ProtectedBrowserSession"))return Json::object();
 if(!data["ProtectedBrowserSession"].is_string()||str(data,"ProtectedBrowserSession").size()>196608)throw std::runtime_error("Invalid protected website session.");
 if(str(data,"ProtectedBrowserSession").empty())return Json::object();
 try{return validateBrowserSession(Json::parse(reveal(str(data,"ProtectedBrowserSession"))));}catch(const std::exception&){throw std::runtime_error("The saved website session is unavailable. Refresh this download from its browser page, or create it again through Site Grabber.");}
}
inline bool sessionLogout(const Json& session,const std::string& address){
 if(session.empty())return false;GrabberFilters filters({{"StartUrl",str(session,"Origin")+"/"},{"FileInclude","*"}});return filters.locationMatch(grabberPatterns(str(session,"LogoutPages")),Url(address));
}
inline Headers browserSessionHeaders(const Json& session,const std::string& address,Headers headers){
 if(session.empty())return headers;if(sessionLogout(session,address))throw std::runtime_error("This address is excluded as a website logout page.");
 Url target(address);setHeader(headers,"Cookie","");if(target.origin!=str(session,"Origin"))return headers;
 headers["User-Agent"]=str(session,"UserAgent");std::vector<Json> selected;
 for(const auto& cookie:session["Cookies"]){
  auto path=str(cookie,"path");if(yes(cookie,"secure")&&target.scheme!="https")continue;if(cookie.contains("expirationDate")&&real(cookie,"expirationDate")<=epoch()/1000.0)continue;
  if(!sessionDomainMatches(target.host,str(cookie,"domain"),yes(cookie,"hostOnly")))continue;
  if(target.path!=path&&(target.path.rfind(path,0)!=0||(path.back()!='/'&&(target.path.size()==path.size()||target.path[path.size()]!='/'))))continue;selected.push_back(cookie);
 }
 std::stable_sort(selected.begin(),selected.end(),[](const Json& a,const Json& b){return str(a,"path").size()>str(b,"path").size();});
 std::string value;for(const auto& cookie:selected){if(!value.empty())value+="; ";value+=str(cookie,"name")+"="+str(cookie,"value");}if(!value.empty())headers["Cookie"]=value;return headers;
}
inline Json browserSessionPreferences(Json prefs,const Json& data){
 prefs.erase("ActiveBrowserSession");auto session=readBrowserSession(data);if(!session.empty()){if(yes(data,"BrowserLogin"))session["LogoutPages"]=str(data,"LogoutPages");prefs["ActiveBrowserSession"]=std::move(session);}return prefs;
}
// Called serially by a request pool before it commits a rotated session. A newer
// browser recapture wins; failed disk persistence leaves the prior snapshot intact.
inline std::function<void(const Json&)> browserSessionSaver(Manager& manager,JobPtr job,Json expected){
 if(expected.empty())return {};
 return [&manager,job,expected=std::move(expected)](const Json& session)mutable{
  Lock lock(manager.mutex);if(std::find(manager.jobs.begin(),manager.jobs.end(),job)==manager.jobs.end()||readBrowserSession(job->data)!=expected)throw std::runtime_error("The website session changed during this request. Retry using the current session.");
  auto previous=job->data["ProtectedBrowserSession"];job->data["ProtectedBrowserSession"]=protect(session.dump());
  try{manager.save();}catch(...){job->data["ProtectedBrowserSession"]=previous;throw;}expected=session;
 };
}
}
