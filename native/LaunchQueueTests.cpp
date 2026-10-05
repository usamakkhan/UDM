#include "Launch.hpp"
#include <iostream>
using namespace udm;
int wmain(int argc,wchar_t** argv){
 if(argc!=2)return 2;
 auto root=fs::absolute(argv[1]);if(fs::exists(root))return 2;
 fs::create_directories(root);Json checks=Json::array();std::string error;
 auto check=[&](bool ok,const char* label){checks.push_back({{"name",label},{"passed",ok}});if(!ok)throw std::runtime_error(label);};
 try{
  auto first=parseLaunch({L"/a",L"/d",L"https://example.test/queued.bin",L"/p",(root/L"files").wstring(),L"/f",L"chosen.bin"});
  auto last=parseLaunch({L"/d",L"https://example.test/queued.bin",L"/n",L"/a"});
  check(first.paused&&last.paused&&last.silent,"Queue-only flag works before or after URL and with silent mode");
  check(yes(first.request(),"paused")&&yes(last.request(),"paused"),"Forwarded requests retain queue-only intent");
  Manager manager(root/L"data");manager.state["Settings"]["CategoryFolders"]=false;
  auto job=manager.receive(first.request());
  check(str(job->data,"Status")=="Paused"&&str(job->data,"Queue")=="Main queue"&&yes(job->data,"QueueMember"),"Queue-only request creates a paused Main queue member");
  check(str(job->data,"Folder")==first.folder&&str(job->data,"FileName")=="chosen.bin","Queue-only request preserves destination and filename");
  for(int i=0;i<12;++i)manager.tick();
  check(!manager.isActive(job)&&str(job->data,"Status")=="Paused"&&!fs::exists(job->target()),"Scheduler ticks do not start queue-only download");
  manager.state["Settings"]["DuplicatePolicy"]="Existing";
  auto before=job->data;check(manager.receive(first.request())==job&&job->data==before,"Repeated queue-only request honors reuse policy and preserves existing record");
  manager.save();Manager restored(root/L"data");
  check(restored.jobs.size()==1&&str(restored.jobs[0]->data,"Status")=="Paused"&&yes(restored.jobs[0]->data,"QueueMember"),"Queue-only membership persists across restart");
  check(!parseLaunch({L"/d",L"https://example.test/normal"}).paused,"Ordinary CLI download retains confirmation behavior");
  check(str(parseLaunch({L"/s"}).request(),"action")=="cli-start-queue","Scheduler switch produces a queue-start request without a URL");
  check(yes(parseLaunch({L"--background",L"--start-queue"}).request(),"background"),"Long queue-start alias preserves background forwarding");
  for(auto args:{std::vector<std::wstring>{L"/s",L"/d",L"https://example.test/file"},std::vector<std::wstring>{L"/s",L"/a"},std::vector<std::wstring>{L"--recovery",L"/s"}}){bool rejected=false;try{parseLaunch(args);}catch(const std::exception&){rejected=true;}check(rejected,"Conflicting queue-start operations are rejected before changing state");}
 }catch(const std::exception& e){error=e.what();}
 atomicText(root/L"results.json",Json{{"passed",error.empty()},{"error",error},{"checks",checks}}.dump(2),false);
 return error.empty()?0:1;
}
