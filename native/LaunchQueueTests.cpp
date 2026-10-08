#include "Launch.hpp"
#include "CliCompletion.hpp"
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
  auto quit=parseLaunch({L"/q",L"/n",L"/d",L"https://example.test/file"});
  check(quit.quitAfterDownload&&quit.silent,"Quit-after-download switch is accepted with a download");
  check(!quit.request().contains("quitAfterDownload"),"First-instance quit intent is never forwarded to a running app");
  check(parseLaunch({L"--quit-after-download"}).address.empty(),"Quit option without a download does not create a request");
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
  auto hang=parseLaunch({L"/d",L"https://example.test/hang.bin",L"/a",L"/h",L"/q"});
  check(hang.hangupAfterDownload&&hang.quitAfterDownload&&yes(hang.request(),"hangup"),"Hang-up intent is forwarded while quit remains first-instance only");
  check(parseLaunch({L"--hangup-after-download"}).hangupAfterDownload,"Long hang-up alias is accepted");
  Manager hmanager(root/L"hangup-data");hmanager.state["Settings"]["DuplicatePolicy"]="Existing";
  auto hj=hmanager.receive(hang.request());
  check(hmanager.cliHangups.size()==1&&!takeCliHangup(hmanager),"Paused CLI hang-up request is armed without executing");
  hmanager.receive(hang.request());
  check(hmanager.cliHangups.size()==1,"Repeated request for the same download does not duplicate hang-up actions");
  for(auto state:{"Queued","Downloading","Paused","Failed","Awaiting confirmation"}){hj->data["Status"]=state;check(!takeCliHangup(hmanager),"Unfinished or failed download cannot trigger hang-up");}
  hj->data["Status"]="Complete";
  for(auto state:{"Running","Failed","Attention","Timed out","Interrupted"}){hj->data["ScanResult"]={{"Status",state},{"ExitCode",0}};check(!takeCliHangup(hmanager),"Incomplete or unsuccessful scanner blocks CLI hang-up");}
  hj->data["ScanResult"]={{"Status","Finished"},{"ExitCode",5}};
  check(!takeCliHangup(hmanager),"Nonzero scanner exit blocks CLI hang-up");
  hj->data["ScanResult"]={{"Status","Finished"},{"ExitCode",0}};
  check(!cliCompletionReady(hmanager,hj)&&!takeCliHangup(hmanager),"An unsaved in-memory completion cannot deliver a CLI action");
  hmanager.save();
  check(takeCliHangup(hmanager)==hj&&!takeCliHangup(hmanager),"Successful scanned download delivers exactly one hang-up action");
  armCliHangup(hmanager,hj);check(hmanager.cliHangups.empty(),"Previously complete record does not arm a new hang-up");
  hj->data["Status"]="Paused";armCliHangup(hmanager,hj);hmanager.save();
  {Manager reopened(root/L"hangup-data");check(reopened.cliHangups.empty(),"CLI hang-up intent is not restored from a saved catalog");}
  hmanager.jobs.clear();check(!takeCliHangup(hmanager)&&hmanager.cliHangups.empty(),"Removing a download cancels its pending hang-up");
  auto ordinary=hmanager.receive({{"action","add"},{"url","https://example.test/browser.bin"},{"hangup",true}});
  check(hmanager.cliHangups.empty(),"Ordinary browser add cannot schedule CLI hang-up");
  Manager blocked(root/L"blocked-completion");auto pending=blocked.add("https://example.test/blocked.bin");armCliHangup(blocked,pending);
  pending->data["Status"]="Complete";auto stateFile=blocked.root/L"state.json";
  HANDLE lease=CreateFileW(stateFile.c_str(),GENERIC_READ,FILE_SHARE_READ,nullptr,OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL,nullptr);
  if(lease==INVALID_HANDLE_VALUE)throw std::runtime_error("Could not lock the fixture catalog.");
  bool saveFailed=false;try{blocked.save();}catch(const std::exception&){saveFailed=true;}CloseHandle(lease);
  check(saveFailed&&str(Json::parse(readText(stateFile))["Downloads"][0],"Status")!="Complete","Catalog replacement failure leaves completion uncommitted");
  check(!cliCompletionReady(blocked,pending)&&!takeCliHangup(blocked)&&blocked.cliHangups.size()==1,"Failed completion save retains /q and /h intent without executing it");
  blocked.save();
  check(cliCompletionReady(blocked,pending)&&takeCliHangup(blocked)==pending&&!takeCliHangup(blocked),"Both CLI actions become eligible after completion is saved");
 }catch(const std::exception& e){error=e.what();}
 atomicText(root/L"results.json",Json{{"passed",error.empty()},{"error",error},{"checks",checks}}.dump(2),false);
 return error.empty()?0:1;
}
