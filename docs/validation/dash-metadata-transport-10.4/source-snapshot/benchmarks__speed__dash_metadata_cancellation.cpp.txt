#include "DashMetadataTransport.hpp"
#include "DashManifest.hpp"
#include <iostream>
using namespace udm;
int wmain(int argc,wchar_t** argv){
 if(argc!=3||fs::exists(argv[1]))return 2;const auto root=fs::absolute(argv[1]);fs::create_directories(root);WSADATA sockets{};WSAStartup(MAKEWORD(2,2),&sockets);CoInitializeEx(nullptr,COINIT_MULTITHREADED);
 auto cancel=std::make_shared<Cancel>();cancel->deadline=GetTickCount64()+10000;std::atomic<ULONGLONG> stoppedAt{0};std::weak_ptr<HttpSession> lease;bool cancelled=false,completed=false;std::string error;
 std::thread stop([&]{const auto deadline=GetTickCount64()+5000;while(!fs::exists(root/L"hold-entered")&&GetTickCount64()<deadline)Sleep(5);stoppedAt=GetTickCount64();cancel->stop=true;});
 try{DashMetadataTransportScope scope;Json prefs={{"ProxyMode","Connect directly"}};lease=dashMetadataTransport(utf8(argv[2]),prefs);refreshDashManifest(utf8(argv[2]),{},prefs,cancel);completed=true;}catch(const Cancelled&){cancelled=true;}catch(const std::exception& e){error=e.what();}
 const auto returnedAt=GetTickCount64();stop.join();const auto latency=stoppedAt.load()&&returnedAt>=stoppedAt?returnedAt-stoppedAt.load():10000;
 const bool released=lease.expired()&&!dashMetadataTransport(utf8(argv[2]),Json::object());const bool passed=cancelled&&!completed&&error.empty()&&released&&latency<3000;
 atomicText(root/L"results.json",Json{{"passed",passed},{"cancelled",cancelled},{"unexpectedCompletion",completed},{"error",error},{"transportReleased",released},{"cancelToUnwindMs",latency}}.dump(2),false);CoUninitialize();WSACleanup();return passed?0:1;
}
