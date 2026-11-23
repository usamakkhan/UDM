// Owned-origin integration driver. This is an engine check, not GUI/IDM timing acceptance.
#include "Core.hpp"
#include <iostream>
using namespace udm;
int wmain(int argc,wchar_t** argv){
 if(argc!=6&&argc!=7)return 2;const auto root=fs::absolute(argv[1]);if(fs::exists(root))return 2;
 fs::create_directories(root);Json checks=Json::array(),record;std::string failure;
 auto check=[&](bool ok,const char* name){checks.push_back({{"name",name},{"passed",ok}});if(!ok)throw std::runtime_error(name);};
 try{
  WSADATA wsa{};if(WSAStartup(MAKEWORD(2,2),&wsa))throw std::runtime_error("WSA startup");
  const auto url=utf8(argv[2]),expected=utf8(argv[3]),mode=utf8(argv[5]);const i64 expectedSize=std::stoll(argv[4]);
  const int configuredConnections=argc==7?std::stoi(argv[6]):mode=="single"?1:8;
  if(configuredConnections<1||configuredConnections>32)throw std::runtime_error("Invalid test connection count.");
  if(expectedSize<=0||(mode!="single"&&mode!="split"&&mode!="resume"))throw std::runtime_error("Unknown owned transfer case.");
  Manager manager(root/L"catalog");auto prefs=manager.state["Settings"];
  prefs["DownloadFolder"]=utf8((root/L"files").wstring());prefs["CategoryFolders"]=false;
  prefs["ProxyMode"]="Connect directly";prefs["Retries"]=1;prefs["StopOnWebPage"]=false;
  prefs["CorrectFileExtensions"]=false;prefs["PrefetchFileInfo"]=false;manager.setSettings(prefs);
  auto job=manager.add(url,"","owned.bin","Main queue",true);job->data["Connections"]=configuredConnections;
  const auto begun=std::chrono::steady_clock::now();manager.resume(job);
  const auto deadline=GetTickCount64()+60000;
  const auto waitIdle=[&]{while(GetTickCount64()<deadline){manager.tick();if(!manager.isActive(job))return;Sleep(2);}throw std::runtime_error("Owned transfer exceeded its deadline.");};
  if(mode=="resume"){
   for(;;){manager.tick();{Lock lock(manager.mutex);if(num(job->data,"Received")>=expectedSize/8)break;}if(!manager.isActive(job)||GetTickCount64()>deadline)throw std::runtime_error("Partial transfer was not observed.");Sleep(2);}
   manager.pause(job);waitIdle();
   {Lock lock(manager.mutex);check(str(job->data,"Status")=="Paused"&&num(job->data,"Received")>0&&num(job->data,"Received")<expectedSize,"Pause retains a genuine incomplete range plan");}
   check(!fs::exists(job->target()),"Pause cannot publish the partial response");
   manager.resume(job);
  }
  waitIdle();manager.stop();
  const auto settled=std::chrono::steady_clock::now();
  record=job->data;
  check(str(record,"Status")=="Complete","The production worker and owned shutdown report completion");
  check(fs::file_size(job->target())==static_cast<uintmax_t>(expectedSize),"The saved output has the independent origin's exact length");
  check(fileHash(job->target())==expected&&str(record,"Sha256")==expected,"Output and completion digest match the independent source");
  check(num(record,"Received")==expectedSize,"Range credits cover the complete representation");
  const auto catalog=Json::parse(readText(manager.root/L"state.json"));bool durable=false;
  for(const auto& item:catalog["Downloads"])if(str(item,"Id")==job->id())durable=str(item,"Status")=="Complete"&&str(item,"Sha256")==expected;
  check(durable,"Complete and the exact checksum survive in the saved catalog");
  record={{"mode",mode},{"configuredConnections",configuredConnections},{"size",expectedSize},{"received",num(record,"Received")},{"transferredBytes",num(record,"TransferredBytes")},
   {"dynamicSplits",num(record,"DynamicSplits")},{"segments",record["Segments"]},{"finishPhases",record.value("FinishPhases",Json::object())},
   {"engineThroughJoinedStopMs",std::chrono::duration<double,std::milli>(settled-begun).count()},
   {"proofHashAfterTimedEngineStop",true},{"savedHash",expected}};
 }catch(const std::exception& error){failure=error.what();}
 atomicText(root/L"results.json",Json{{"passed",failure.empty()},{"error",failure},{"checks",checks},{"transfer",record}}.dump(2),false);
 std::cout<<checks.size()<<" checks; "<<(failure.empty()?"passed":failure)<<"\n";return failure.empty()?0:1;
}
