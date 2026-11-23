// Compile with all native core objects except DashManifest.obj: this fixture
// includes the actual assembly checkpoint/reuse implementation to test it.
#ifdef UDM_DASH_BASELINE_SOURCE
#include UDM_DASH_BASELINE_SOURCE
#else
#include "DashManifest.cpp"
#endif
#include "MediaPublication.hpp"
#include <iostream>
using namespace udm;
int wmain(int argc,wchar_t** argv){
 if(argc!=2&&argc!=3)return 2;const auto root=fs::absolute(argv[1]);if(fs::exists(root))return 2;fs::create_directories(root);Json checks=Json::array();std::string error;
 const auto check=[&](bool ok,const char* name){checks.push_back({{"passed",ok},{"name",name}});};
 try{for(bool restore:{false,true}){
  const auto area=root/(restore?L"restored":L"fresh");fs::create_directories(area);DashRecordingOutput output;output.staging=area/(L"assembled-"+wide(guid())+L".mp4");atomicText(output.staging,"verified DASH output",false);output.bytes=(i64)fs::file_size(output.staging);output.start=0;output.duration=1;Cancel cancel;Json index={{"sealed",true},{"captureStart",0},{"captureEnd",1},{"tracks",Json::array()}};
  checkpointDashAssembly(area,index,output,cancel);
  if(restore){
#ifndef UDM_DASH_BASELINE_SOURCE
   output.verifiedSource.reset();
#endif
   auto cached=restoreDashAssembly(area,index,cancel);if(!cached)throw std::runtime_error("Cannot restore the actual assembly checkpoint");output=std::move(*cached);
  }
  check(output.bytes==20&&output.sha256==fileHash(output.staging),"Assembly receipt measures the exact held bytes");
  Handle intruder(CreateFileW(output.staging.c_str(),GENERIC_WRITE,FILE_SHARE_READ|FILE_SHARE_WRITE|FILE_SHARE_DELETE,nullptr,OPEN_EXISTING,0,nullptr));check(!intruder,"Fresh and restored assembly outputs exclude writers after return");
  if(intruder){const char changed[]="substituted DASH out!";DWORD written=0;WriteFile(intruder.h,changed,20,&written,nullptr);CloseHandle(intruder.h);intruder.h=INVALID_HANDLE_VALUE;}
  Manager manager(area/L"catalog");auto settings=manager.state["Settings"];const auto destination=argc==3?fs::absolute(argv[2])/(restore?L"restored":L"fresh"):area/L"files";settings["DownloadFolder"]=utf8(destination.wstring());settings["CategoryFolders"]=false;manager.setSettings(settings);auto job=manager.add("https://example.invalid/live-dash.mp4","","recording.mp4","Main queue",true);
#ifdef UDM_DASH_BASELINE_SOURCE
  publishMediaOutput(manager,job,output.staging,output.sha256,std::make_shared<Cancel>());
#else
  check(bool(output.verifiedSource),"Assembly returns ownership of its hashed file object");
  auto pin=output.verifiedSource->clone();output.verifiedSource.reset();if(!pin)throw std::runtime_error("Cannot transfer DASH source ownership");
  publishMediaOutput(manager,job,output.staging,output.sha256,std::make_shared<Cancel>(),nullptr,std::move(pin));
#endif
  check(str(job->data,"Status")=="Complete"&&fileHash(job->target())==output.sha256,"Publication uses the verified bytes even after hashing reaches EOF");manager.stop();
 }}catch(const std::exception& e){error=e.what();}
 const bool passed=error.empty()&&std::all_of(checks.begin(),checks.end(),[](const Json& r){return yes(r,"passed");});atomicText(root/L"results.json",Json{{"passed",passed},{"error",error},{"checks",checks}}.dump(2),false);std::cout<<checks.size()<<" checks; "<<(passed?"passed":"failed")<<"\n";return passed?0:1;
}
