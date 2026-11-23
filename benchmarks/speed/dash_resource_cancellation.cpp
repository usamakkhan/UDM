#include "DashManifest.hpp"
#include "DashResourceTransport.hpp"
#include <iostream>
using namespace udm;
int wmain(int argc,wchar_t** argv){
 if(argc!=3||fs::exists(argv[1]))return 2;auto root=fs::absolute(argv[1]);fs::create_directories(root);Json checks=Json::array();std::string error;double unwindMs=0,releasedMs=0;bool cancelled=false;std::weak_ptr<HttpSession> transport;
 WSADATA sockets{};WSAStartup(MAKEWORD(2,2),&sockets);CoInitializeEx(nullptr,COINIT_MULTITHREADED);
 auto check=[&](bool ok,const char* label){checks.push_back({{"name",label},{"passed",ok}});if(!ok)throw std::runtime_error(label);};
 try{
  using Clock=std::chrono::steady_clock;auto cancel=std::make_shared<Cancel>();cancel->deadline=GetTickCount64()+15000;std::atomic_size_t bytes{0};Clock::time_point stopped;
  {
   DashResourceTransportScope scope;const Json prefs={{"ProxyMode","Connect directly"}};auto sanitized=prefs;sanitized["SiteLogins"]=Json::array();sanitized["WindowsLoginIntranet"]=false;
   auto pinned=dashResourceTransport(utf8(argv[2]),sanitized,{});transport=pinned;pinned.reset();
   auto snapshot=refreshDashManifest(utf8(argv[2]),{},prefs,cancel);DashLiveUpdate update;update.state.snapshot=snapshot;update.staticPresentation=true;
   for(size_t n=0;n<snapshot.manifest.nodes.size();++n)if(snapshot.manifest.nodes[n].name=="Representation"){auto track=selectDashTrack(snapshot,n,*cancel);update.windows.push_back(planDashTemplateWindow(track.addressing,0,1000,*cancel));update.state.tracks.push_back(std::move(track));}
   check(update.state.tracks.size()==3,"Held-response test selects all three capture workers");
   std::atomic_bool finished{false},signalled{false};
   std::thread monitor([&]{while(!finished){if(fs::exists(root/L"cancel.signal")){stopped=Clock::now();signalled=true;cancel->stop=true;break;}Sleep(5);}});
   struct Join {std::atomic_bool& done;std::thread& thread;~Join(){done=true;if(thread.joinable())thread.join();}} join{finished,monitor};
   try{captureDashWindow(root,update,{},prefs,cancel,[&](size_t,size_t count,const Cancel&){bytes+=count;},DashCaptureInterval{0,1},3);}catch(const Cancelled&){cancelled=true;}
   finished=true;monitor.join();if(signalled)unwindMs=std::chrono::duration<double,std::milli>(Clock::now()-stopped).count();
   check(signalled&&cancelled,"Parent cancellation interrupts the held shared-session requests");
   check(bytes==0,"No body bytes are consumed before held headers are released");
   check(unwindMs<3000,"All joined capture workers unwind within three seconds");
  }
  releasedMs=std::chrono::duration<double,std::milli>(Clock::now()-stopped).count();check(releasedMs<3000,"Shared-session teardown also completes within the cancellation bound");
  check(transport.expired()&&!dashResourceTransportContext(),"Final scope teardown releases the shared body session and restores context");
  size_t receipts=0;for(const auto& file:fs::directory_iterator(root))if(file.path().filename().wstring().find(L".receipt.json")!=std::wstring::npos)++receipts;
  check(receipts==0,"Cancellation before admission publishes no resource receipts");
 }catch(const std::exception& e){error=e.what();}
 Json result={{"passed",error.empty()},{"error",error},{"checks",checks},{"cancelled",cancelled},{"cancelToUnwindMs",unwindMs},{"cancelToReleaseMs",releasedMs},{"transportReleased",transport.expired()}};atomicText(root/L"results.json",result.dump(2),false);std::cout<<result.dump()<<"\n";CoUninitialize();WSACleanup();return error.empty()?0:1;
}
