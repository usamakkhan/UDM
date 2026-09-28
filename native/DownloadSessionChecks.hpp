#pragma once
#include "BrowserRequest.hpp"
static void downloadSessionChecks(const fs::path& root){
 const std::string address="http://127.0.0.1:9191/private/report.bin";
 Json session={{"Origin","http://127.0.0.1:9191"},{"UserAgent","Fixture"},{"LogoutPages",""},{"Cookies",Json::array({sessionCookie("session","secret","/private")})}};
 Headers headers={{"Cookie","session=secret"},{"User-Agent","Fixture"}};
 auto canonical=browserDownloadSession(session,address,headers);check(!canonical.empty(),"Ordinary browser session validates against its observed request");
 auto bad=session;bad["Origin"]="http://other.test";rejects([&]{browserDownloadSession(bad,address,headers);},"Ordinary capture rejects a session from another origin");
 bad=session;bad["UserAgent"]="Other";rejects([&]{browserDownloadSession(bad,address,headers);},"Session user agent must match the captured request");
 bad=session;bad["Cookies"][0]["value"]="other-account";rejects([&]{browserDownloadSession(bad,address,headers);},"Session cookies must match the captured request");
 bad=session;bad["LogoutPages"]="*/logout";rejects([&]{browserDownloadSession(bad,address,headers);},"Ordinary captures cannot add unrelated crawler settings");
 auto boundary=session;boundary["Cookies"]=Json::array({sessionCookie("a",std::string(8192,'a')),sessionCookie("b",std::string(8186,'b'))});check(headerValue(browserSessionHeaders(validateBrowserSession(boundary),address,{}),"Cookie").size()==16384,"Exact native Cookie wire-byte limit accepts 16384 bytes");boundary["Cookies"][1]["value"]=std::string(8187,'b');rejects([&]{validateBrowserSession(boundary);},"Native cookie limits include equals and separator bytes");
 Manager manager(root/L"ordinary-session");manager.state["Settings"]["DownloadFolder"]=utf8((root/L"ordinary-files").wstring());manager.state["Settings"]["CategoryFolders"]=false;
 Json message={{"action","add"},{"url",address},{"filename","report.bin"},{"headers",Json(headers)},{"browserSession",session},{"downloadLater",true}};
 auto job=manager.receive(message);check(readBrowserSession(job->data)==canonical,"Browser add saves a managed session before returning its job");check(readText(manager.root/L"state.json").find("secret")==std::string::npos,"Ordinary browser session is encrypted in the catalog");
 {Manager reopened(manager.root);check(readBrowserSession(reopened.jobs.at(0)->data)==canonical,"Ordinary session survives reopening the native catalog");}
 auto choice=manager.receive(message);check(choice!=job&&str(choice->data,"DuplicateOf")==job->id()&&readBrowserSession(choice->data)==canonical,"Duplicate choice retains the new capture's encrypted session");check(manager.resolveDuplicate(choice,"Existing")==job,"Choosing the existing duplicate retains its original record");manager.state["Settings"]["DuplicatePolicy"]="Existing";check(manager.receive(message)==job,"The existing-download policy reuses an identical captured session");
 job->data["Status"]="Paused";manager.state["Settings"]["DuplicatePolicy"]="Existing";auto changed=message;changed["browserSession"]["Cookies"][0]["path"]="/";rejects([&]{manager.receive(changed);},"Existing downloads cannot silently acquire a different session scope");check(readBrowserSession(job->data)==canonical,"Rejected duplicate preserves the existing encrypted session");
 manager.state["Settings"]["DuplicatePolicy"]="Replace";auto replacement=manager.receive(changed);check(replacement==job&&str(readBrowserSession(job->data)["Cookies"][0],"path")=="/","Unfinished duplicate replacement commits its new session atomically");
 job->data["Status"]="Paused";manager.beginAddressRefresh(job);auto refreshed=message;refreshed["url"]=address+"?fresh=1";auto offered=manager.receive(refreshed);check(offered==job&&!manager.addressRefreshCandidate(job).empty(),"Browser refresh captures the session alongside the replacement URL");manager.refreshAddress(job,str(refreshed,"url"),headers);check(readBrowserSession(job->data)==canonical&&str(job->data,"Url")==str(refreshed,"url"),"Reviewed link refresh adopts its captured managed session");
 manager.refreshAddress(job,address,headers);check(readBrowserSession(job->data).empty(),"Manual header replacement clears the previously managed session");
 auto count=manager.jobs.size();bad=message;bad["browserSession"]["Cookies"][0]["value"]="wrong";rejects([&]{manager.receive(bad);},"Invalid capture is rejected before any native download is created");check(manager.jobs.size()==count,"Invalid capture leaves the download catalog unchanged");
}
