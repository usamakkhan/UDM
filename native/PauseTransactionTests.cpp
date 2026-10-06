#include "Core.hpp"
#include "TransferFixture.hpp"
#include <iostream>
using namespace udm;

class CatalogLease {
 HANDLE handle=INVALID_HANDLE_VALUE;
public:
 explicit CatalogLease(const fs::path& file){
  handle=CreateFileW(file.c_str(),GENERIC_READ,FILE_SHARE_READ,nullptr,OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL,nullptr);
  if(handle==INVALID_HANDLE_VALUE)throw std::runtime_error("Cannot lock fixture catalog.");
 }
 ~CatalogLease(){CloseHandle(handle);}
 CatalogLease(const CatalogLease&)=delete;
 CatalogLease& operator=(const CatalogLease&)=delete;
};

int wmain(int argc,wchar_t** argv){
 if(argc!=2)return 2;const auto root=fs::absolute(argv[1]);if(fs::exists(root))return 2;
 fs::create_directories(root);Json checks=Json::array();int failed=0;WSADATA wsa{};
 if(WSAStartup(MAKEWORD(2,2),&wsa))return 2;
 auto check=[&](bool ok,const char* name){checks.push_back({{"name",name},{"passed",ok}});if(!ok)++failed;};
 try{
  {
   Manager m(root/L"queued");auto job=m.add("https://example.invalid/queued.bin");m.resume(job);
   m.state["BrowserCaptures"]["fixture"]={{"presentation",{{"id",job->id()},{"kind","progress"}}}};m.save();
   const auto before=m.snapshot(),disk=Json::parse(readText(m.root/L"state.json"));bool rejected=false;
   {CatalogLease lease(m.root/L"state.json");try{m.pause(job);}catch(const std::exception&){rejected=true;}}
   check(rejected,"Queued Pause reports blocked catalog replacement");
   check(m.snapshot()==before,"Failed queued Pause restores all job and capture metadata");
   check(Json::parse(readText(m.root/L"state.json"))==disk,"Failed queued Pause preserves the disk catalog");
   m.pause(job);
   check(str(job->data,"Status")=="Paused"&&!yes(job->data,"IndividualStart"),"Queued Pause can be retried after the lock is released");
   check(!m.state["BrowserCaptures"]["fixture"].contains("presentation"),"Successful Pause consumes its capture presentation");
   const auto id=job->id();m.stop();Manager reopened(root/L"queued");
   check(reopened.jobs.size()==1&&reopened.jobs[0]->id()==id&&str(reopened.jobs[0]->data,"Status")=="Paused","Successful Pause survives reopening the catalog");
  }
  {
   Manager m(root/L"completed");auto job=m.add("https://example.invalid/completed.bin");
   job->data["Status"]="Complete";job->data["QueueMember"]=false;m.save();
   const auto before=m.snapshot();auto file=m.root/L"state.json";
   const auto stamp=fs::last_write_time(file)-std::chrono::hours(24);fs::last_write_time(file,stamp);bool accepted=true;
   {CatalogLease lease(file);try{m.pause(job);}catch(const std::exception&){accepted=false;}}
   check(accepted,"Idle completed Stop does not require a writable catalog");
   check(m.snapshot()==before&&fs::last_write_time(file)==stamp,"Idle completed Stop preserves metadata and write time");
   job->data["SyncPending"]=true;job->data["IndividualStart"]=true;m.save();const auto pending=m.snapshot();bool rejected=false;
   {CatalogLease lease(file);try{m.pause(job);}catch(const std::exception&){rejected=true;}}
   check(rejected&&m.snapshot()==pending,"Failed synchronization Stop preserves pending explicit work");
   m.pause(job);check(!yes(job->data,"SyncPending")&&!job->data.contains("IndividualStart"),"Successful synchronization Stop clears pending explicit work");
  }
  TransferFixture server(2*1024*1024,10,10);server.expected(root/L"expected.bin");
  auto configure=[&](Manager& m){auto prefs=m.state["Settings"];prefs["DownloadFolder"]=utf8((m.root/L"files").wstring());prefs["CategoryFolders"]=false;prefs["Connections"]=1;prefs["Parallel"]=1;prefs["Retries"]=0;prefs["ProxyMode"]="Connect directly";m.setSettings(prefs);};
  auto pump=[&](Manager& m,auto ready,int seconds){auto until=GetTickCount64()+seconds*1000;while(GetTickCount64()<until){m.tick();{Lock lock(m.mutex);if(ready())return;}Sleep(20);}throw std::runtime_error("Transfer fixture timed out.");};
  {
   Manager m(root/L"active");configure(m);auto job=m.add(server.url("/steady"));job->data["LimitKbps"]=128;m.resume(job);
   pump(m,[&]{return m.isActive(job)&&num(job->data,"Received")>=65536;},10);
   i64 received=0;
   {Lock lock(m.mutex);m.save();const auto before=job->data;const auto disk=readText(m.root/L"state.json");bool rejected=false;
    {CatalogLease lease(m.root/L"state.json");try{m.pause(job);}catch(const std::exception&){rejected=true;}}
    check(rejected,"Active Pause reports blocked catalog replacement");
    check(job->data==before,"Failed active Pause restores its complete job state");
    check(readText(m.root/L"state.json")==disk,"Failed active Pause leaves saved state unchanged");received=num(job->data,"Received");
   }
   auto until=GetTickCount64()+2500;bool continued=false;
   while(GetTickCount64()<until){Sleep(20);Lock lock(m.mutex);if(m.isActive(job)&&num(job->data,"Received")>received){continued=true;break;}if(!m.isActive(job))break;}
   check(continued,"Failed active Pause does not cancel the worker and bytes continue arriving");
   m.pause(job);pump(m,[&]{return !m.isActive(job);},5);
   check(str(job->data,"Status")=="Paused","A successful retry actually stops the active transfer");
   job->data["LimitKbps"]=0;m.resume(job);pump(m,[&]{return !m.isActive(job)&&str(job->data,"Status")=="Complete";},15);
   check(fileHash(job->target())==fileHash(root/L"expected.bin"),"Resumed transfer publishes the exact original bytes");
  }
  {
   Manager m(root/L"cycle");configure(m);auto q=defaultQueue();q["FinishAction"]="Exit UDM";m.setQueue(q);
   auto job=m.add(server.url("/steady"));job->data["LimitKbps"]=128;m.queueRun("Main queue",true);
   pump(m,[&]{return m.isActive(job)&&num(job->data,"Received")>=65536;},10);
   {Lock lock(m.mutex);CatalogLease lease(m.root/L"state.json");bool rejected=false;try{m.pause(job);}catch(const std::exception&){rejected=true;}check(rejected,"Queue-origin Pause exposes the save error");}
   {Lock lock(m.mutex);job->data["LimitKbps"]=0;}
   auto until=GetTickCount64()+15000;
   while(GetTickCount64()<until){m.tick();{Lock lock(m.mutex);if(!m.isActive(job))break;}Sleep(20);}
   m.tick();auto events=m.takeQueueCompletions();
   check(str(job->data,"Status")=="Complete"&&events.size()==1,"Failed Pause does not poison a successful queue completion cycle");
   check(events.size()==1&&m.completionEventReady(events[0]),"Completion remains ready after a failed Pause without executing its action");
  }
 }catch(const std::exception& e){checks.push_back({{"name",std::string("Unexpected: ")+e.what()},{"passed",false}});++failed;}
 WSACleanup();Json result={{"passed",checks.size()-failed},{"failed",failed},{"checks",checks}};
 atomicText(root/L"results.json",result.dump(2),false);std::cout<<result.dump(2)<<std::endl;return failed?1:0;
}
