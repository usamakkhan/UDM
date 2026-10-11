#include "Core.hpp"
#include <iostream>
#include <thread>
using namespace udm;
int wmain(int argc,wchar_t** argv){
 if(argc!=2)return 2;
 const auto root=fs::absolute(argv[1]);if(fs::exists(root))return 2;
 fs::create_directories(root);Json checks=Json::array();
 auto check=[&](bool ok,const char* name){checks.push_back({{"name",name},{"passed",ok}});};
 for(bool backupLock:{false,true}){
  auto folder=root/(backupLock?L"backup-reader":L"catalog-reader");auto file=folder/L"state.json";
  atomicText(file,"old");atomicText(file,"current");
  auto held=backupLock?folder/L"state.json.bak":file;
  Handle reader(CreateFileW(held.c_str(),GENERIC_READ,FILE_SHARE_READ|FILE_SHARE_WRITE,nullptr,OPEN_EXISTING,0,nullptr));
  check(bool(reader),"Reader opens without delete sharing");
  HANDLE raw=reader.h;reader.h=INVALID_HANDLE_VALUE;
  std::thread release([raw]{Sleep(150);CloseHandle(raw);});
  std::string error;try{atomicText(file,"new");}catch(const std::exception& e){error=e.what();}
  release.join();
  checks.push_back({{"name",backupLock?"Transient backup lock retries publication":"Transient catalog lock retries publication"},{"passed",error.empty()},{"error",error}});
  check(readText(file)=="new"&&readText(folder/L"state.json.bak")=="current","Successful publication retains previous catalog as backup");
 }
 auto file=root/L"permanent"/L"state.json";atomicText(file,"old");atomicText(file,"current");
 {
  Handle reader(CreateFileW((root/L"permanent"/L"state.json.bak").c_str(),GENERIC_READ,FILE_SHARE_READ|FILE_SHARE_WRITE,nullptr,OPEN_EXISTING,0,nullptr));
  check(bool(reader),"Permanent backup reader opens");bool rejected=false;
  try{atomicText(file,"new");}catch(const std::exception&){rejected=true;}
  check(rejected,"Persistent lock reports failure instead of claiming completion");
  check(readText(file)=="current"&&readText(root/L"permanent"/L"state.json.tmp")=="new","Persistent lock preserves catalog and flushed replacement");
 }
 atomicText(file,"new");check(readText(file)=="new","Publication recovers after permanent reader closes");
 bool passed=true;for(const auto& item:checks)passed=passed&&yes(item,"passed");
 auto result=Json{{"passed",passed},{"checks",checks}};atomicText(root/L"results.json",result.dump(2),false);std::cout<<result.dump(2)<<std::endl;return passed?0:1;
}