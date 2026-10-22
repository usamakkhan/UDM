#include "GrabberRenderedStore.hpp"
#include <iostream>
using namespace udm;
int main(int argc,char** argv){try{if(argc!=2&&argc!=3)return 2;std::string url=argv[1];auto root=fs::temp_directory_path()/wide("udm-rendered-crawler-"+guid());Manager manager(root);manager.state["Settings"]["ProxyMode"]="Connect directly";manager.state["Settings"]["Retries"]=0;manager.save();Json project={{"Id",guid()},{"Name","Rendered crawler"},{"StartUrl",url},{"FileInclude","*.zip"},{"FileExclude","blocked*"},{"Depth",1},{"MaxPages",10},{"Links",Json::array()}};manager.saveProject(project);Cancel cancel;int passed=0;auto check=[&](bool ok){if(!ok)throw std::runtime_error("Rendered crawler assertion failed at "+std::to_string(passed));++passed;};
 auto baseline=runGrabber(manager,project,cancel);check(baseline["Links"].empty());project=baseline;
 const bool pauseSession=argc==3&&std::string(argv[2])=="--cookie-pause";
 const bool cookieSession=pauseSession||(argc==3&&std::string(argv[2])=="--cookie-session");
 if(cookieSession){project["BrowserLogin"]=true;project["ProtectedBrowserSession"]=protect(Json{{"Origin",Url(url).origin},{"UserAgent","UDM fixture"},{"LogoutPages",""},{"Cookies",Json::array()}}.dump());manager.saveProject(project);}
 auto request=beginRenderedGrabberRequest(manager,str(project,"Id"));auto origin=Url(url).origin;Json snapshot={{"version",1},{"url",url},{"baseUrl",url},{"title","Fixture"},{"documentId","document"},{"tabId",1},{"frameId",0},{"visited",4},{"truncated",false},{"references",Json::array()}};for(const char* name:{"runtime.zip","blocked.zip","wrong.pdf"})snapshot["references"].push_back({{"url",origin+"/"+name},{"kind","link"},{"description","Browser-created file"}});if(pauseSession)snapshot["references"].push_back({{"url",origin+"/child.html"},{"kind","link"},{"description","Next page"}});request["snapshot"]=snapshot;completeRenderedGrabberRequest(manager,request);
 if(cookieSession&&!pauseSession){
  auto before=manager.state["RenderedGrabber"];bool failed=false;
  {Handle held;try{runGrabber(manager,project,cancel,[&](std::string){if(!held)held.h=CreateFileW((root/L"state.json").c_str(),GENERIC_READ,FILE_SHARE_READ,nullptr,OPEN_EXISTING,0,nullptr);});}catch(...){failed=true;}check(!!held);}
  check(failed);check(manager.state["RenderedGrabber"]==before);check(Json::parse(readText(root/L"state.json"))["RenderedGrabber"]==before);
 }
 Cancel paused;auto found=pauseSession?runGrabber(manager,project,paused,[&](std::string progress){if(progress.find("Exploring page 2 /")==0)paused.stop=true;}):runGrabber(manager,project,cancel);
 if(pauseSession){check(yes(found,"ExplorationStopped"));check(num(found,"PagesVisited")==1);check(found["ExploreState"]["Pending"].size()==1);}if(argc==3&&std::string(argv[2])=="--expect-empty"){check(found["Links"].empty());check(!found["Errors"].empty());std::cout<<"PASS "<<passed<<" rendered file-access checks\n";return 0;}check(found["Links"].size()==1);check(str(found["Links"][0],"Url")==origin+"/runtime.zip");check(num(found["Links"][0],"Size")==4);check(str(found["Links"][0],"Description")=="Browser-created file");check(!found.contains("snapshot")&&!found.contains("protectedSnapshot"));check(str(found["ExploreState"],"Signature").find("|rendered:")!=std::string::npos);
 if(cookieSession){
  auto session=readBrowserSession(found);check(!session["Cookies"].empty());
  const auto suffix="|browser:"+str(found,"ProtectedBrowserSession")+"|rendered:"+str(request,"ticket");
  const auto signature=str(found["ExploreState"],"Signature");check(signature.size()>=suffix.size()&&signature.substr(signature.size()-suffix.size())==suffix);
  check(str(found,"ProtectedBrowserSession")!=str(project,"ProtectedBrowserSession"));
  Manager reopened(root);auto loaded=loadRenderedGrabberSnapshot(reopened,found);check(str(loaded,"ticket")==str(request,"ticket"));
  auto again=runGrabber(reopened,found,cancel);check(again["Links"].size()==(pauseSession?2:1));
  if(pauseSession){check(!yes(again,"ExplorationStopped"));check(num(again,"PagesVisited")==2);check(again["ExploreState"]["Pending"].empty());check(str(again["Links"][1],"Url")==origin+"/child.zip");}
  auto changed=again;changed["FileInclude"]="*.pdf";bool denied=false;try{runGrabber(reopened,changed,cancel);}catch(...){denied=true;}check(denied);
  auto differentSession=again;differentSession["ProtectedBrowserSession"]=protect(Json{{"Origin",Url(url).origin},{"UserAgent","Another browser"},{"LogoutPages",""},{"Cookies",Json::array()}}.dump());denied=false;try{runGrabber(reopened,differentSession,cancel);}catch(...){denied=true;}check(denied);
  std::cout<<"PASS "<<passed<<" rendered cookie-checkpoint checks\n";return 0;
 }
 auto changed=found;changed["FileInclude"]="*.pdf";bool rejected=false;try{runGrabber(manager,changed,cancel);}catch(...){rejected=true;}check(rejected);Manager restarted(root);check(!loadRenderedGrabberSnapshot(restarted,found).empty());std::cout<<"PASS "<<passed<<" rendered crawler checks\n";return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
