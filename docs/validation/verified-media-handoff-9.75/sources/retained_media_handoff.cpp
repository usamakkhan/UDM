#include "Core.hpp"
#ifdef UDM_BASELINE_MEDIA_HANDOFF
#include "BaselineMediaPublication.hpp"
#else
#include "MediaPublication.hpp"
#endif
#include <iostream>
using namespace udm;
int wmain(int argc,wchar_t** argv){
 if(argc!=2)return 2;const auto root=fs::absolute(argv[1]);if(fs::exists(root))return 2;fs::create_directories(root);Json checks=Json::array();std::string error;
 const auto check=[&](bool ok,const char* name){checks.push_back({{"passed",ok},{"name",name}});};
 try{for(bool substitute:{false,true}){
  const auto area=root/(substitute?L"replace-name":L"healthy");Manager manager(area/L"catalog");auto settings=manager.state["Settings"];settings["DownloadFolder"]=utf8((area/L"files").wstring());settings["CategoryFolders"]=false;manager.setSettings(settings);auto job=manager.add("https://example.invalid/cached-video.mp4","","cached.mp4","Main queue",true);
  const auto source=job->target().parent_path()/(L".udm-"+wide(job->id())+L".muxing.mp4");const fs::path moved=source.wstring()+L".retained";atomicText(source,"verified cached video",false);const auto digest=fileHash(source);job->data["RetainedMediaOutput"]={{"Version",1},{"Path",utf8(source.wstring())},{"Size",(i64)fs::file_size(source)},{"Sha256",digest},{"Binding","owned-handoff-fixture"}};
  int bindings=0;std::string failure;bool published=false;
  try{published=publishRetainedMediaOutput(manager,job,std::make_shared<Cancel>(),[&](const Job&){if(++bindings==2&&substitute){if(!MoveFileExW(source.c_str(),moved.c_str(),0))throw std::runtime_error("Fixture cannot move the held source");atomicText(source,"substituted new video",false);}return std::string("owned-handoff-fixture");});}catch(const std::exception& e){failure=e.what();}
  check(bindings==2,"Retained workflow reaches the post-verification selection check");
  check(substitute?(!published&&!failure.empty()&&str(job->data,"Status")!="Complete"&&!fs::exists(job->target())):(published&&failure.empty()&&str(job->data,"Status")=="Complete"&&fileHash(job->target())==digest),"Retained publication accepts original bytes and rejects a replaced source name");
  check(substitute?(fs::exists(moved)&&fileHash(moved)==digest&&fs::exists(source)&&readText(source)=="substituted new video"):!fs::exists(source),"Retained workflow preserves both originals and replacements on rejection");manager.stop();
 }}catch(const std::exception& e){error=e.what();}
 const bool passed=error.empty()&&checks.size()==6&&std::all_of(checks.begin(),checks.end(),[](const Json& row){return yes(row,"passed");});atomicText(root/L"results.json",Json{{"passed",passed},{"error",error},{"checks",checks}}.dump(2),false);std::cout<<checks.size()<<" checks; "<<(passed?"passed":"failed")<<"\n";return passed?0:1;
}
