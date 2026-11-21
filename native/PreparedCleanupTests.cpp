#include "PreparedTransferCommit.hpp"
#include <iostream>
using namespace udm;
int wmain(int argc,wchar_t** argv){if(argc!=2)return 2;const auto root=fs::absolute(argv[1]);if(fs::exists(root))return 2;fs::create_directories(root);Json checks=Json::array();std::string error;const auto check=[&](bool ok,const char* name){checks.push_back({{"passed",ok},{"name",name}});};
 try{for(bool async:{false,true})for(const std::string action:{"unchanged","replaced","missing","directory"}){
  auto folder=root/wide(std::string(async?"async-":"sync-")+action);Manager manager(folder/L"catalog");auto prefs=manager.state["Settings"];prefs["DownloadFolder"]=utf8((folder/L"files").wstring());prefs["CategoryFolders"]=false;manager.setSettings(prefs);auto job=manager.add("https://example.invalid/owned.bin","","output.bin","Main queue",true);fs::create_directories(job->target().parent_path());auto staging=job->target().parent_path()/(L".udm-"+wide(job->id())+L".assembling"),moved=staging; moved+=L".moved";atomicText(staging,"owned content",false);const auto hash=fileHash(staging);auto token=std::make_shared<Cancel>();
  {PreparedTransferCommit owner(manager,job,token,true);if(async&&!owner.startWriteback(staging,false))throw std::runtime_error("Cannot start real owned flush");owner.prepare(staging,folder/L"parts",hash,false);
   if(action!="unchanged"){if(!MoveFileExW(staging.c_str(),moved.c_str(),0))throw std::runtime_error("Cannot move owned source");if(action=="replaced")atomicText(staging,"owned content",false);if(action=="directory")fs::create_directory(staging);}
  }
  if(action=="unchanged")check(!fs::exists(staging),"Uncommitted unchanged owned stage is removed after its writeback joins");
  else{check(fs::exists(moved)&&fileHash(moved)==hash,"Renamed owned source remains available for review");if(action=="replaced")check(fs::exists(staging)&&fs::is_regular_file(staging)&&fileHash(staging)==hash,"Equal-byte replacement at the old name is preserved");if(action=="missing")check(!fs::exists(staging),"Missing staging name is not recreated");if(action=="directory")check(fs::is_directory(staging),"Foreign directory at the staging name is preserved");}
  manager.stop();
 }}catch(const std::exception& e){error=e.what();}bool passed=error.empty()&&std::all_of(checks.begin(),checks.end(),[](const Json& row){return yes(row,"passed");});atomicText(root/L"results.json",Json{{"passed",passed},{"error",error},{"checks",checks}}.dump(2),false);std::cout<<checks.size()<<" checks; "<<(passed?"passed":"failed")<<"\n";return passed?0:1;}
