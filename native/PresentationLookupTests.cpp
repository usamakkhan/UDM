#include "Core.hpp"
#include <iostream>
#include <algorithm>
using namespace udm;
int wmain(int argc,wchar_t** argv){
 if(argc!=2||fs::exists(argv[1]))return 2;
 Json checks=Json::array(),timings=Json::array();
 try{
  Manager manager(argv[1]);
  auto check=[&](bool ok,const char* name){checks.push_back({{"name",name},{"passed",ok}});if(!ok)throw std::runtime_error(name);};
  for(unsigned i=0;i<10000;++i)manager.jobs.push_back(std::make_shared<Job>(Json{{"Id",guid()},{"Status","Complete"}}));
  for(bool receipts:{false,true}){
   manager.state["BrowserCaptures"]=Json::object();
   if(receipts)for(unsigned i=0;i<100;++i)manager.state["BrowserCaptures"][std::to_string(i)]={{"status","accepted"},{"id",std::to_string(i)}};
   std::vector<double> runs;for(unsigned i=0;i<7;++i){auto start=std::chrono::steady_clock::now();auto offers=manager.pendingBrowserPresentations();runs.push_back(std::chrono::duration<double,std::milli>(std::chrono::steady_clock::now()-start).count());check(offers.empty(),"History without pending presentations produces no dialogs");}
   std::sort(runs.begin(),runs.end());timings.push_back({{"case",receipts?"consumed-receipts":"empty-journal"},{"medianMilliseconds",runs[3]},{"jobs",10000}});
  }
  manager.state["BrowserCaptures"]={{"a",{{"status","accepted"},{"presentation",{{"id",manager.jobs[3]->id()},{"kind","download"}}}}},{"b",{{"status","accepted"},{"presentation",{{"id",manager.jobs[3]->id()},{"kind","refresh"}}}}},{"c",{{"status","accepted"},{"presentation",{{"id","missing"},{"kind","existing"}}}}}};
  auto offers=manager.pendingBrowserPresentations();check(offers.size()==1&&str(offers[0],"id")==manager.jobs[3]->id()&&str(offers[0],"kind")=="refresh"&&offers[0]["tokens"].size()==2,"Pending presentations group by existing job and preserve refresh precedence");
  manager.state["BrowserCaptures"]["a"]["presentation"]["kind"]="invalid";bool rejected=false;try{manager.pendingBrowserPresentations();}catch(...){rejected=true;}check(rejected,"Malformed pending presentation is still rejected");
  atomicText(fs::path(argv[1])/L"results.json",Json{{"checks",checks},{"benchmark",timings},{"passed",true}}.dump(2),false);
  std::cout<<checks.size()<<" checks passed\n";return 0;
 }catch(const std::exception& e){std::cerr<<e.what()<<"\n";return 1;}
}
