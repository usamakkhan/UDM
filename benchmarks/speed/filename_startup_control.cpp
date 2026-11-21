#include "Core.hpp"
#include "Version.hpp"
#include <iostream>
using namespace udm;
static unsigned long long cycles(){ULONG64 value=0;if(!QueryThreadCycleTime(GetCurrentThread(),&value))throw std::runtime_error("Cannot query current-thread cycles");return value;}
int wmain(int argc,wchar_t** argv){
 if(argc!=3)return 2;const auto data=fs::absolute(argv[1]),report=fs::absolute(argv[2]);if(!fs::is_regular_file(data/L"state.json")||fs::exists(report))return 2;CoInitializeEx(nullptr,COINIT_MULTITHREADED);
 try{
  const auto began=std::chrono::steady_clock::now();const auto before=cycles();Manager manager(data,Manager::StartupRecovery::Desktop);const auto after=cycles();const auto ready=std::chrono::steady_clock::now();const auto count=manager.jobs.size();
  for(size_t i=0;i<count;++i){const auto& record=manager.jobs[i]->data;if(str(record,"FileName")!="history-"+std::to_string(i)+".bin"||str(record,"Description")!=std::string(2048,'x')||str(record,"Status")!="Complete")throw std::runtime_error("History differs after startup");}
  std::vector<std::string> input;input.reserve(7500);for(int i=0;i<7500;++i)input.push_back(i%101?"history-"+std::to_string(i)+".bin":"folder\\CON.txt");std::vector<std::string> normalized;normalized.reserve(input.size());
  const auto nameBegan=std::chrono::steady_clock::now();const auto nameBefore=cycles();for(const auto& name:input)normalized.push_back(safeName(name));const auto nameAfter=cycles();const auto nameReady=std::chrono::steady_clock::now();
  Json result={{"passed",true},{"compiledVersion",UDM_VERSION_STRING},{"historyRows",count},{"constructorMs",std::chrono::duration<double,std::milli>(ready-began).count()},{"constructorThreadCycles",after-before},{"filenameCalls",input.size()},{"filenameMs",std::chrono::duration<double,std::milli>(nameReady-nameBegan).count()},{"filenameThreadCycles",nameAfter-nameBefore},{"normalized",normalized},{"historyPreserved",true}};
  manager.stop();const auto saved=Json::parse(readText(data/L"state.json"));if(saved["Downloads"].size()!=count)throw std::runtime_error("Normal shutdown lost history");result["historyPersisted"]=true;atomicText(report,result.dump(2),false);std::cout<<count<<" history rows; "<<UDM_VERSION_STRING<<"; constructor "<<result["constructorMs"]<<" ms; filenames "<<result["filenameMs"]<<" ms\n";CoUninitialize();return 0;
 }catch(const std::exception& e){atomicText(report,Json{{"passed",false},{"error",e.what()}}.dump(2),false);std::cerr<<e.what()<<"\n";CoUninitialize();return 1;}
}
