#include "SetupMigration.hpp"
#include "Version.hpp"
#include <iostream>
using namespace udm;
int wmain(int argc,wchar_t** argv){
 try{
  Json result;
  if(argc==3&&std::wstring(argv[1])==L"--inspect-data")result=inspectSetupData(Json::parse(readText(argv[2],524288)));
  else if(argc==4&&std::wstring(argv[1])==L"--prepare-data")result=prepareSetupData(Json::parse(readText(argv[2],524288)),setupPath(utf8(argv[3])));
  else if(argc==3&&std::wstring(argv[1])==L"--rollback-data")result=rollbackSetupData(setupPath(utf8(argv[2])));
  else throw std::runtime_error("Use --inspect-data REQUEST, --prepare-data REQUEST RECEIPT, or --rollback-data RECEIPT.");
  result["version"]=UDM_VERSION_STRING;std::cout<<result.dump()<<std::endl;return 0;
 }catch(const std::exception& error){std::cerr<<error.what()<<std::endl;std::cout<<Json{{"ok",false},{"error",error.what()}}.dump()<<std::endl;return 1;}
}
