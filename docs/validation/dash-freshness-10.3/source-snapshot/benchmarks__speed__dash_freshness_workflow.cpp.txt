#include "DashManifest.hpp"
#include <iostream>
using namespace udm;
int wmain(int argc,wchar_t** argv){
 if(argc!=4)return 2;const auto root=fs::absolute(argv[1]);if(fs::exists(root))return 2;fs::create_directories(root);const auto mode=utf8(argv[3]);using Clock=std::chrono::steady_clock;
 WSADATA sockets{};WSAStartup(MAKEWORD(2,2),&sockets);CoInitializeEx(nullptr,COINIT_MULTITHREADED);Json result,events=Json::array();std::string error;bool cancelled=false;const auto begun=Clock::now();
 auto elapsed=[&](){return std::chrono::duration<double,std::milli>(Clock::now()-begun).count();};
 try{auto cancel=std::make_shared<Cancel>();cancel->deadline=GetTickCount64()+15000;Json prefs={{"ProxyMode","Connect directly"}};
  auto snapshot=refreshDashManifest(utf8(argv[2]),{},prefs,cancel);std::vector<size_t> selected;for(size_t n=0;n<snapshot.manifest.nodes.size();++n)if(snapshot.manifest.nodes[n].name=="Representation")selected.push_back(n);auto previous=createDashLiveState(snapshot,selected,*cancel);const auto bodyAnchor=previous.snapshot.bodyReceived;
  // Schedule a refresh deterministically; do not alter receipt/body anchors.
  previous.snapshot.validUntil=Clock::now()-std::chrono::milliseconds(1);const auto before=previous.snapshot.document;result["initialFetchAndParseMs"]=elapsed();size_t callbacks=0;
  try{
   auto next=updateDashLivePlan(previous,{{0,2000}},{},prefs,cancel,[&](size_t bytes,const Cancel& token){const auto start=elapsed();++callbacks;if(mode=="cancel"&&callbacks==2)cancel->stop=true;const bool hold=(mode=="always"||callbacks==1)&&mode!="cached304";if(hold)token.wait(350);token.check();events.push_back({{"callback",callbacks},{"bytes",bytes},{"startMs",start},{"endMs",elapsed()},{"requestedHoldMs",hold?350:0}});});
   const auto now=Clock::now();result["completed"]=true;result["freshAtReturn"]=!next.state.snapshot.validUntil||now<*next.state.snapshot.validUntil;result["originalBodyAnchorPreserved"]=next.state.snapshot.bodyReceived==bodyAnchor;result["segments"]=next.windows.empty()?0:next.windows[0].segments.size();result["transition"]=bool(next.transition);
  }catch(const Cancelled& e){cancelled=true;error=e.what();}catch(const std::exception& e){error=e.what();}
  result["inputSnapshotUnchanged"]=previous.snapshot.document==before&&previous.snapshot.bodyReceived==bodyAnchor;result["metadataCallbacks"]=callbacks;
 }catch(const Cancelled& e){cancelled=true;error=e.what();}catch(const std::exception& e){error=e.what();}
 result["error"]=error;result["cancelled"]=cancelled;result["events"]=events;result["elapsedMs"]=elapsed();atomicText(root/L"results.json",result.dump(2),false);std::cout<<result.dump()<<"\n";CoUninitialize();WSACleanup();return error.empty()?0:cancelled?3:1;
}
