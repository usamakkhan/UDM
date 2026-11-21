#include "HistoryReadSnapshot.hpp"
#include <iostream>
using namespace udm;
int wmain(int argc,wchar_t** argv){
 if(argc!=2)return 2;const auto root=fs::absolute(argv[1]);if(fs::exists(root))return 2;fs::create_directories(root);Json checks=Json::array(),diagnostics=Json::object();std::string error;
 const auto check=[&](bool value,const char* name){checks.push_back({{"name",name},{"passed",value}});if(!value)throw std::runtime_error(name);};
 try{
  Manager manager(root/L"catalog");auto member=std::make_shared<Job>(Json{{"Id",guid()},{"FileName","member.bin"},{"Category","Video"},{"Status","Paused"},{"Queue","Main queue"},{"QueueMember",true}});
  auto complete=std::make_shared<Job>(Json{{"Id",guid()},{"FileName","complete.bin"},{"Category","Music"},{"Status","Complete"},{"Queue","Main queue"},{"QueueMember",false}});
  {Lock lock(manager.mutex);manager.jobs={member,complete};}
  std::map<JobPtr,std::shared_ptr<const DownloadListRow>> rows;DownloadFilterCache cache;HistoryReadRequest request;
  auto initial=tryHistoryReadSnapshot(manager,request,rows,cache);check(initial&&initial->rows==std::vector<JobPtr>{member,complete},"Initial All Downloads contains both owned records");
  auto queue=defaultQueue("Navigation audit");queue["Enabled"]=false;manager.setQueue(queue);
  {Lock lock(manager.mutex);member->data["Queue"]="Navigation audit";complete->data["Queue"]="Navigation audit";}
  request.previousTree=&initial->navigation;request.filter="queue:Navigation audit";
  auto selected=tryHistoryReadSnapshot(manager,request,rows,cache);
  diagnostics["newQueue"]={{"requested",request.filter},{"actual",selected?selected->filter:"busy"},{"rows",selected?selected->rows.size():0}};
  check(selected&&selected->filter==request.filter,"A newly created queue remains the requested filter during structural change");
  check(selected->rows==std::vector<JobPtr>{member},"The first new-queue snapshot excludes a completed nonmember");
  auto current=selected->navigation;
  check(historyEffectiveFilter(current,nullptr,request.filter,{})==request.filter,"A valid first-view filter works without an older navigation snapshot");
  auto changed=current;changed.projects.emplace_back("new-project","New project");
  check(historyEffectiveFilter(changed,&current,"project:new-project",{})=="project:new-project","A newly added project can be requested in its first structural refresh");
  check(historyEffectiveFilter(changed,&current,"queue:Navigation audit",{"all"})=="queue:Navigation audit","An explicit valid request takes priority over an older caret ancestry");
  changed.categories.push_back("New category");
  check(historyEffectiveFilter(changed,&current,"unfinished:New category",{})=="unfinished:New category","A newly added category keeps its unfinished subview");
  check(historyEffectiveFilter(changed,&current,"finished",{})=="finished","An unrelated structural change preserves the Finished view");
  check(historyEffectiveFilter(changed,&current,"queue:removed",{"queue:removed","root:queues"})=="all","A removed queue still falls back to the surviving queues root");
  check(historyEffectiveFilter(changed,&current,"unfinished:removed",{"unfinished:removed","unfinished"})=="unfinished","A removed category still falls back to Unfinished");
  check(historyEffectiveFilter(changed,&current,"project:removed",{"project:removed","grabber"})=="grabber","A removed project still falls back to Grabber");
  check(historyEffectiveFilter(current,&current,"invalid",{})=="all","An invalid request in unchanged navigation remains All Downloads");
  request.previousTree=&current;auto repeated=tryHistoryReadSnapshot(manager,request,selected->records,cache);
  check(repeated&&repeated->filter==request.filter&&repeated->rows==selected->rows,"The following snapshot keeps the same filtered cohort");
  manager.stop();
 }catch(const std::exception& failure){error=failure.what();}
 atomicText(root/L"results.json",Json{{"passed",error.empty()},{"checks",checks},{"error",error},{"diagnostics",diagnostics}}.dump(2),false);std::cout<<checks.size()<<" checks; "<<(error.empty()?"passed":error)<<"\n";return error.empty()?0:1;
}
