#include "Core.hpp"
#include <chrono>
#include <iomanip>
#include <iostream>
#include <sstream>

using namespace udm;

int wmain(int argc,wchar_t** argv){
 if(argc<2||argc>3)return 2;
 const auto output=fs::absolute(argv[1]);
 if(fs::exists(output))return 2;
 fs::create_directories(output);
 try{
  constexpr int count=5000;
  Json downloads=Json::array();
  for(int i=0;i<count;++i){
   std::ostringstream id;id<<std::hex<<std::setw(32)<<std::setfill('0')<<i;
   downloads.push_back({{"Id",id.str()},{"FileName","item-"+std::to_string(i)+".bin"},
    {"Folder",utf8(output.wstring())},{"Status","Complete"},{"Queue","Main queue"}});
  }
  Json state={{"Schema",1},{"Settings",defaultSettings()},{"Downloads",std::move(downloads)},
   {"Queues",Json::array({defaultQueue()})},{"Projects",Json::array()},
   {"QuotaStart",date()},{"QuotaBytes",0}};
  const auto data=output/L"catalog";
  fs::create_directories(data);
  const auto serialized=state.dump();
  atomicText(data/L"state.json",serialized,false);
  atomicText(data/L"state.before-native.json",serialized,false);
  Json samples=Json::array();
  for(int repeat=0;repeat<3;++repeat){
   auto start=std::chrono::steady_clock::now();
   Manager manager(data);
   const auto elapsed=std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now()-start).count();
   if(manager.jobs.size()!=count)throw std::runtime_error("Saved history count changed during startup.");
   samples.push_back(elapsed);
  }
  Json tickSamples=Json::array();
  {
   Manager manager(data);
   for(int repeat=0;repeat<6;++repeat){
    auto start=std::chrono::steady_clock::now();
    manager.tick();
    tickSamples.push_back(std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now()-start).count());
   }
  }
  auto valid=Json{{"Id",std::string(32,'A')},{"FileName","valid.bin"}};
  Job accepted(valid);
  valid["Id"]=std::string(31,'A')+"G";
  bool rejected=false;
  try{Job invalid(valid);}catch(const std::runtime_error&){rejected=true;}
  if(!rejected)throw std::runtime_error("Non-hexadecimal saved ID was accepted.");
  valid["Id"]="A";
  rejected=false;
  try{Job invalid(valid);}catch(const std::runtime_error&){rejected=true;}
  if(!rejected)throw std::runtime_error("Short saved ID was accepted.");
  Json result={{"passed",true},{"variant",argc==3?utf8(argv[2]):"runner"},{"downloads",count},{"startupMs",samples},{"tickMs",tickSamples},
   {"idValidationChecks",3}};
  atomicText(output/L"results.json",result.dump(2),false);
  std::cout<<result.dump()<<'\n';
  return 0;
 }catch(const std::exception& error){
  atomicText(output/L"results.json",Json{{"passed",false},{"error",error.what()}}.dump(2),false);
  std::cerr<<error.what()<<'\n';return 1;
 }
}
