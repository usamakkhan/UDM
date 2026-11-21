#include "Core.hpp"
#include <iostream>
using namespace udm;
static Json checks=Json::array();
static void check(bool value,const std::string& label){checks.push_back({{"name",label},{"passed",value}});if(!value)throw std::runtime_error(label);}
template<class F>static void rejects(F&& action,const std::string& label){bool refused=false;try{action();}catch(...){refused=true;}check(refused,label);}
#include "CaptureAtomicChecks.hpp"
#include "CapturePresentationChecks.hpp"
int wmain(int argc,wchar_t** argv){
 if(argc!=2)return 2;const auto root=fs::absolute(argv[1]);if(fs::exists(root))return 2;fs::create_directories(root);std::string error;
 WSADATA sockets{};if(WSAStartup(MAKEWORD(2,2),&sockets))return 3;const auto apartment=CoInitializeEx(nullptr,COINIT_MULTITHREADED);
 try{capturePresentationChecks(root);}catch(const std::exception& e){error=e.what();}
 if(SUCCEEDED(apartment))CoUninitialize();WSACleanup();atomicText(root/L"results.json",Json{{"passed",error.empty()},{"error",error},{"checks",checks}}.dump(2),false);
 std::cout<<checks.size()<<" checks; "<<(error.empty()?"passed":error)<<"\n";return error.empty()?0:1;
}
