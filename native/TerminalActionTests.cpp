#include "Core.hpp"
#include "TransferFixture.hpp"
#include <iostream>
using namespace udm;
int wmain(int argc,wchar_t** argv){
 if(argc==3&&std::wstring(argv[1])==L"--scanner-fixture"){auto file=fs::path(argv[2]);writeBytes(fs::path(file.wstring()+L".scanner-started"),Bytes{'s'});Sleep(2500);writeBytes(fs::path(file.wstring()+L".scanner-finished"),Bytes{'f'});return 0;}
 if(argc!=2)return 2;auto root=fs::absolute(argv[1]);if(fs::exists(root))return 2;fs::create_directories(root);Json checks=Json::array();int failed=0;
 auto check=[&](bool ok,const std::string& name){checks.push_back({{"name",name},{"passed",ok}});if(!ok)++failed;};
 try{
 WSADATA wsa{};if(WSAStartup(MAKEWORD(2,2),&wsa))throw std::runtime_error("WSA");
 TransferFixture server(1024*1024,20,20);Manager manager(root/L"data");auto prefs=manager.state["Settings"];prefs["DownloadFolder"]=utf8((root/L"files").wstring());prefs["CategoryFolders"]=false;prefs["ProxyMode"]="Connect directly";manager.setSettings(prefs);
 auto completed=manager.add("https://example.invalid/completed.bin","","completed.bin","Main queue",true);completed->data["Status"]="Complete";fs::create_directories(completed->target().parent_path());writeBytes(completed->target(),Bytes{'d','o','n','e'});manager.save();
 for(auto key:{"RequiresRequestCapture","RequiresMediaCapture","DuplicateOf","ProtectedBrowserSession"}){
  auto original=completed->data;
  completed->data[key]=std::string(key)=="DuplicateOf"||std::string(key)=="ProtectedBrowserSession"?Json("fixture-invalid"):Json(true);
  manager.save();auto snapshot=manager.snapshot();auto hash=fileHash(manager.root/L"state.json");bool accepted=true;try{manager.resume(completed);}catch(...){accepted=false;}
  check(accepted&&manager.snapshot()==snapshot&&fileHash(manager.root/L"state.json")==hash,std::string("Completed Resume ignores ")+key);
  completed->data=original;
 }
 manager.save();auto before=manager.snapshot();auto hash=fileHash(manager.root/L"state.json");manager.pause(completed);
 check(manager.snapshot()==before,"Completed Stop preserves catalog metadata");check(fileHash(manager.root/L"state.json")==hash,"Completed Stop performs no catalog write");check(readText(completed->target())=="done","Completed Stop preserves file bytes");
 auto running=manager.add(server.url("/steady"),"","running.bin","Main queue",true);running->data["Connections"]=1;running->data["LimitKbps"]=64;manager.resume(running);manager.tick();
 check(manager.isActive(running),"Actual transfer started for active Resume check");
 {Lock lock(manager.mutex);running->data["RequiresMediaCapture"]=true;bool accepted=true;try{manager.resume(running);}catch(...){accepted=false;}check(accepted&&manager.isActive(running),"Resume ignores already active download before capture validation");running->data.erase("RequiresMediaCapture");}
 manager.pause(running);auto end=GetTickCount64()+5000;while(manager.isActive(running)&&GetTickCount64()<end)Sleep(20);
 check(!manager.isActive(running),"Stop still pauses an actual active transfer");
 running->data["RequiresMediaCapture"]=true;bool refused=false;try{manager.resume(running);}catch(...){refused=true;}check(refused,"Unfinished media still requires fresh capture");
 running->data.erase("RequiresMediaCapture");running->data["LimitKbps"]=0;manager.resume(running);
 end=GetTickCount64()+15000;while(GetTickCount64()<end){manager.tick();bool done;{Lock lock(manager.mutex);auto s=str(running->data,"Status");done=s=="Complete"||s=="Failed";}if(done)break;Sleep(20);}
 server.expected(root/L"expected.bin");check(str(running->data,"Status")=="Complete"&&fileHash(running->target())==fileHash(root/L"expected.bin"),"Eligible resumed download completes with exact bytes");

 end=GetTickCount64()+5000;while(manager.isActive(running)&&GetTickCount64()<end)Sleep(20);
 wchar_t executable[32768]{};GetModuleFileNameW(nullptr,executable,32768);prefs=manager.state["Settings"];prefs["ScanProgram"]=utf8(executable);prefs["ScanArguments"]="--scanner-fixture \"{file}\"";prefs["ScanTimeoutSeconds"]=10;manager.setSettings(prefs);
 auto scanning=manager.add(server.url("/steady"),"","scanning.bin","Main queue",true);manager.resume(scanning);
 auto started=fs::path(scanning->target().wstring()+L".scanner-started"),finished=fs::path(scanning->target().wstring()+L".scanner-finished");
 end=GetTickCount64()+10000;while(!fs::exists(started)&&GetTickCount64()<end){manager.tick();Sleep(20);}
 check(fs::exists(started)&&manager.isActive(scanning)&&str(scanning->data,"Status")=="Complete","Completed download has a real scanner process still running");
 auto scannerHash=fileHash(scanning->target());auto began=GetTickCount64();manager.pause(scanning);
 end=began+1000;while(manager.isActive(scanning)&&GetTickCount64()<end)Sleep(20);
 check(!manager.isActive(scanning)&&str(scanning->data["ScanResult"],"Status")=="Interrupted","Stop ends scanner wait for a completed active download");
 check(str(scanning->data,"Status")=="Complete"&&fileHash(scanning->target())==scannerHash,"Stopping scanner wait preserves completed file");
 end=GetTickCount64()+5000;while(!fs::exists(finished)&&GetTickCount64()<end)Sleep(20);
 check(fs::exists(finished),"Stopping wait does not terminate the scanner process");
 manager.stop();
 }catch(const std::exception& e){check(false,std::string("Unexpected: ")+e.what());}
 auto result=Json{{"passed",checks.size()-failed},{"failed",failed},{"checks",checks}};atomicText(root/L"results.json",result.dump(2),false);std::cout<<result.dump(2)<<"\n";return failed?1:0;
}
