#include "GrabberRenderedStore.hpp"
#include <iostream>
using namespace udm;
namespace udm {struct RenderedGrabberTestAccess {static void transaction(Manager& m,bool value){m.catalogTransaction=value;}static void checkpoint(Manager& m,std::function<void(const char*)> value){m.catalogCheckpoint=std::move(value);}};}
int main(){int passed=0;try{
 Json page={{"version",1},{"url","https://example.test/page"},{"baseUrl","https://example.test/assets/"},{"title","Page"},{"documentId","document"},{"tabId",1},{"frameId",0},{"visited",12},{"truncated",false},{"references",Json::array({Json{{"url","https://example.test/file.zip"},{"kind","link"},{"description","File"}}})}};
 auto check=[&](bool ok){if(!ok)throw std::runtime_error("Rendered validation assertion failed");++passed;};
 check(validateRenderedGrabberSnapshot(page,str(page,"url"))==page);
 auto reject=[&](Json value){try{validateRenderedGrabberSnapshot(value,str(page,"url"));}catch(const std::exception&){++passed;return;}throw std::runtime_error("Invalid page was accepted");};
 for(auto change:std::vector<Json>{{{"version",2}},{{"url","https://other.test/page"}},{{"documentId",""}},{{"tabId",-1}},{{"frameId",1}},{{"visited",10001}},{{"visited",0}},{{"truncated",1}},{{"cookies","unexpected"}},{{"baseUrl","file:///C:/secret"}}}){auto copy=page;copy.update(change);reject(copy);}
 for(const auto& address:{"file:///C:/secret","javascript:alert(1)","https://user:password@example.test/file"}){auto copy=page;copy["references"][0]["url"]=address;reject(copy);}
 auto copy=page;copy["references"][0]["kind"]="unknown";reject(copy);copy=page;copy["references"][0]["description"]=std::string(4097,'x');reject(copy);
 copy=page;for(int i=0;i<2000;++i)copy["references"].push_back(page["references"][0]);reject(copy);
 copy=page;copy["references"].push_back(page["references"][0]);check(validateRenderedGrabberSnapshot(copy,str(page,"url"))["references"].size()==1);
 copy=page;copy["truncated"]=true;check(yes(validateRenderedGrabberSnapshot(copy,str(page,"url")),"truncated"));

 {
  auto htmlPage=page;htmlPage["html"]="<!doctype html><p>Runtime-created content</p>";htmlPage["htmlUnavailable"]=false;
  check(validateRenderedGrabberSnapshot(htmlPage,str(page,"url"))==htmlPage);
  auto missing=htmlPage;missing.erase("htmlUnavailable");reject(missing);
  missing=htmlPage;missing.erase("html");reject(missing);
  missing=htmlPage;missing["htmlUnavailable"]=true;reject(missing);
  missing=htmlPage;missing["html"]=std::string(512*1024+1,'x');reject(missing);
  missing=htmlPage;missing["html"]=std::string("a\0b",3);reject(missing);
  missing=htmlPage;missing["html"]="";reject(missing);
  missing=htmlPage;missing["htmlUnavailable"]=1;reject(missing);
  missing=page;missing["htmlUnavailable"]=true;check(validateRenderedGrabberSnapshot(missing,str(page,"url"))==missing);
  htmlPage["html"]=std::string(512*1024,'x');check(validateRenderedGrabberSnapshot(htmlPage,str(page,"url"))==htmlPage);
  page["html"]="<!doctype html><p>Runtime-created content</p>";page["htmlUnavailable"]=false;
 }
 Json pending={{"projectId","project-a"},{"ticket","unique-request"},{"generation",2},{"expires",2000},{"url",str(page,"url")}};
 Json message={{"projectId","project-a"},{"ticket","unique-request"},{"generation",2},{"snapshot",page}};
 check(validateRenderedGrabberReply(message,pending,1000)==page);
 auto rejectReply=[&](const Json& reply,const Json& request,i64 now){try{validateRenderedGrabberReply(reply,request,now);}catch(const std::exception&){++passed;return;}throw std::runtime_error("Unbound reply accepted");};
 for(auto change:std::vector<Json>{{{"projectId","project-b"}},{{"ticket","old-request"}},{{"generation",1}},{{"generation","2"}}}){auto reply=message;reply.update(change);rejectReply(reply,pending,1000);}
 rejectReply(message,pending,2000);rejectReply(message,pending,-1);
 auto request=pending;request["expires"]=1000000;rejectReply(message,request,1000);
 request=pending;request["url"]="https://example.test/changed";rejectReply(message,request,1000);
 request=pending;request["ticket"]="replacement-request";rejectReply(message,request,1000);
 rejectReply(message,Json::object(),1000);

 auto root=fs::temp_directory_path()/wide("udm-rendered-state-"+guid());Manager manager(root);
 Json project={{"Id",guid()},{"Name","Rendered fixture"},{"StartUrl",str(page,"url")},{"FileInclude","*.zip"},{"Depth",2},{"MaxPages",20},{"Links",Json::array()}};manager.saveProject(project);auto id=str(project,"Id");
 auto offlineProject=project;offlineProject["Id"]=guid();offlineProject["Name"]="Offline HTML capability";offlineProject["Template"]="Offline website (ZIP)";manager.saveProject(offlineProject);
 auto offlineRequest=beginRenderedGrabberRequest(manager,str(offlineProject,"Id"));check(offlineRequest["captureHtml"]==true);manager.state["Projects"].erase(manager.state["Projects"].begin()+1);manager.state["RenderedGrabber"].erase(str(offlineProject,"Id"));manager.save();
 auto catalogBefore=manager.state;auto fileBefore=readText(root/L"state.json");
 check(renderedGrabberProjects(manager,str(page,"url"))==Json::array({Json{{"projectId",id},{"name","Rendered fixture"}}}));
 check(renderedGrabberProjects(manager,str(page,"url")+"#section").size()==1);
 for(const auto& other:{"https://example.test/page?other=1","https://other.test/page","http://example.test/page","https://example.test/else"})check(renderedGrabberProjects(manager,other).empty());
 for(const auto& invalid:{"", "file:///C:/secret", "https://user:password@example.test/page", "https://example.test/page\n"}){bool failed=false;try{renderedGrabberProjects(manager,invalid);}catch(...){failed=true;}check(failed);}
 check(manager.state==catalogBefore&&readText(root/L"state.json")==fileBefore);
 manager.state["Projects"][0]["ExplorationRunning"]=true;check(renderedGrabberProjects(manager,str(page,"url")).empty());manager.state=catalogBefore;
 auto ticket=beginRenderedGrabberRequest(manager,id);check(ticket["captureHtml"]==false);Json reply=ticket;reply["snapshot"]=page;
 Manager restored(root);check(restored.state["RenderedGrabber"][id]["pending"]==ticket);
 check(completeRenderedGrabberRequest(restored,reply)==page);Manager saved(root);check(!saved.state["RenderedGrabber"][id].contains("pending"));check(Json::parse(reveal(str(saved.state["RenderedGrabber"][id],"protectedSnapshot")))==page);
 bool denied=false;try{completeRenderedGrabberRequest(saved,reply);}catch(...){denied=true;}check(denied);
 auto lastGood=loadRenderedGrabberSnapshot(saved,project);
 auto interrupted=beginRenderedGrabberRequest(saved,id);check(loadRenderedGrabberSnapshot(saved,project)==lastGood);
 check(cancelRenderedGrabberRequest(saved,id,str(interrupted,"ticket")));check(loadRenderedGrabberSnapshot(saved,project)==lastGood);
 Manager afterCancel(root);check(loadRenderedGrabberSnapshot(afterCancel,project)==lastGood);
 auto failedReplacement=beginRenderedGrabberRequest(saved,id);auto invalidReplacement=failedReplacement;invalidReplacement["snapshot"]=page;invalidReplacement["snapshot"]["documentId"]="";
 denied=false;try{completeRenderedGrabberRequest(saved,invalidReplacement);}catch(...){denied=true;}check(denied&&loadRenderedGrabberSnapshot(saved,project)==lastGood);
 auto successfulReplacement=failedReplacement;successfulReplacement["snapshot"]=page;successfulReplacement["snapshot"]["title"]="New capture";completeRenderedGrabberRequest(saved,successfulReplacement);
 check(str(loadRenderedGrabberSnapshot(saved,project)["snapshot"],"title")=="New capture");
 auto replacement=beginRenderedGrabberRequest(saved,id);check(num(replacement,"generation")>num(ticket,"generation"));check(!cancelRenderedGrabberRequest(saved,id,str(ticket,"ticket")));check(saved.state["RenderedGrabber"][id]["pending"]==replacement);check(cancelRenderedGrabberRequest(saved,id,str(replacement,"ticket")));check(!cancelRenderedGrabberRequest(saved,id,str(replacement,"ticket")));reply=replacement;reply["snapshot"]=page;denied=false;try{completeRenderedGrabberRequest(saved,reply);}catch(...){denied=true;}check(denied);
 replacement=beginRenderedGrabberRequest(saved,id);project["FileInclude"]="*.pdf";saved.saveProject(project);reply=replacement;reply["snapshot"]=page;denied=false;try{completeRenderedGrabberRequest(saved,reply);}catch(...){denied=true;}check(denied);

 auto stable=saved.state;auto disk=readText(root/L"state.json");RenderedGrabberTestAccess::transaction(saved,true);denied=false;try{beginRenderedGrabberRequest(saved,id);}catch(...){denied=true;}RenderedGrabberTestAccess::transaction(saved,false);check(denied&&saved.state==stable&&readText(root/L"state.json")==disk);
 RenderedGrabberTestAccess::checkpoint(saved,[](const char* stage){if(std::string(stage)=="temporary-flushed")throw std::runtime_error("Injected durable-save failure");});denied=false;try{beginRenderedGrabberRequest(saved,id);}catch(...){denied=true;}RenderedGrabberTestAccess::checkpoint(saved,{});check(denied&&saved.state==stable&&readText(root/L"state.json")==disk);
 auto current=beginRenderedGrabberRequest(saved,id);check(loadRenderedGrabberSnapshot(saved,project).empty());reply=current;reply["snapshot"]=page;stable=saved.state;disk=readText(root/L"state.json");RenderedGrabberTestAccess::checkpoint(saved,[](const char*){throw std::runtime_error("Injected completion save failure");});denied=false;try{completeRenderedGrabberRequest(saved,reply);}catch(...){denied=true;}RenderedGrabberTestAccess::checkpoint(saved,{});check(denied&&saved.state==stable&&readText(root/L"state.json")==disk);check(completeRenderedGrabberRequest(saved,reply)==page);
 saved.state["RenderedGrabber"]["deleted-project"]={{"generation",1},{"protectedSnapshot","obsolete"}};saved.save();beginRenderedGrabberRequest(saved,id);check(!saved.state["RenderedGrabber"].contains("deleted-project"));Manager cleaned(root);check(!cleaned.state["RenderedGrabber"].contains("deleted-project"));
 std::cout<<"PASS "<<passed<<" native rendered-page validation checks\n";return 0;
 }catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
