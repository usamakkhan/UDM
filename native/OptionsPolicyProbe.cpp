#include "Core.hpp"
#include <iostream>
using namespace udm;
static int passed=0;static void check(bool ok,const std::string& name){if(!ok)throw std::runtime_error(name);++passed;}
template<class F>void rejects(F f,const std::string& name){bool rejected=false;try{f();}catch(...){rejected=true;}check(rejected,name);}
#include "OptionsChecks.hpp"
int wmain(int argc,wchar_t** argv){if(argc!=2)return 2;fs::path root=argv[1];if(fs::exists(root))return 3;CoInitializeEx(nullptr,COINIT_MULTITHREADED);try{fs::create_directories(root);optionsChecks(root);atomicText(root/L"results.json",Json{{"passed",true},{"checks",passed}}.dump(2),false);std::cout<<passed<<" checks passed\n";CoUninitialize();return 0;}catch(const std::exception& e){std::cerr<<e.what();atomicText(root/L"results.json",Json{{"passed",false},{"checks",passed},{"error",e.what()}}.dump(2),false);return 1;}}