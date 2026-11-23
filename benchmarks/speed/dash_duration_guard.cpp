#include "DashPeriodAssembly.hpp"
#include <iostream>
int wmain(int argc,wchar_t** argv){using namespace udm;
 if(argc!=3||fs::exists(argv[2]))return 2;
 const auto input=Json::parse(readText(argv[1],1024*1024));Json results=Json::array();
 for(const auto& row:input){
  bool accepted=true;std::string failure;
  try{dashAssemblyDuration(row.at("probe"),row.at("expectedSeconds").get<double>());}
  catch(const std::exception& e){accepted=false;failure=e.what();}
  results.push_back({{"case",row.at("case")},{"guardAccepted",accepted},
   {"decodedOracleExact",row.at("decodedOracleExact")},{"error",failure}});
 }
 atomicText(argv[2],Json{{"investigationCompleted",true},{"actualNativeDurationGuard",true},
  {"fullRecordingWorkflow",false},{"cases",results}}.dump(2),false);
 std::cout<<results.dump()<<"\n";return 0;
}
