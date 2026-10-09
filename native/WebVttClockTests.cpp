#include "Core.hpp"
#include "WebVttClockChecks.hpp"
#include <iostream>
using namespace udm;
int wmain(int argc,wchar_t** argv){if(argc!=2)return 2;Json checks=Json::array();bool passed=true;try{webVttClockChecks([&](bool ok,const char* name){checks.push_back({{"name",name},{"passed",ok}});passed&=ok;});}catch(const std::exception& e){checks.push_back({{"error",e.what()},{"passed",false}});passed=false;}auto result=Json{{"passed",passed},{"checks",checks}};atomicText(argv[1],result.dump(2),false);std::cout<<result.dump(2)<<std::endl;return passed?0:1;}