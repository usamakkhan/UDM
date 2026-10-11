#include "../drivers/signed-network/BrokerProtocol.hpp"
#include <future>
#include <iostream>
int main(){unsigned passed=0;try{
 auto check=[&](bool ok,const char* message){if(!ok)throw std::runtime_error(message);++passed;std::cout<<"PASS "<<message<<std::endl;};
 check(udmbroker::processIdText("5")==5&&udmbroker::processIdText("4294967295")==MAXDWORD,"PID parser preserves full DWORD bounds without narrowing overflow");
 for(const auto& text:{"","4","-1","+5","5junk","4294967296","000000000005"}){bool rejected=false;try{udmbroker::processIdText(text);}catch(...){rejected=true;}check(rejected,"Malformed or overflowing PID cannot select a different process");}
 udmbroker::Json snapshot={{"Flows",udmbroker::Json::array()},{"Capture",{{"Session","test"},{"ResponseCreditsSupported",true},{"Observations",{{"Items",udmbroker::Json::array()}}}}}};
 check(udmbroker::snapshotResponseCredits(snapshot,"test"),"Complete valid snapshot enables response credits");
 for(unsigned mode=0;mode<4;++mode){auto invalid=snapshot;if(mode==0)invalid["Capture"]["Session"]="stale";if(mode==1)invalid["Capture"]["Responses"]=42;if(mode==2)invalid["Capture"]["PendingResponses"]=udmbroker::Json::array();if(mode==3)invalid["Flows"]=42;
  if(mode==2){invalid.erase("Capture");check(!udmbroker::snapshotResponseCredits(invalid,"test"),"Snapshot without capture clears prior credit capability");continue;}
  bool rejected=false;try{udmbroker::snapshotResponseCredits(invalid,"test");}catch(...){rejected=true;}check(rejected,"Invalid snapshot cannot advertise response credit capability");
 }
 for(unsigned mode=0;mode<5;++mode){wchar_t nonce[33]{};swprintf_s(nonce,L"%016llx%016llx",static_cast<unsigned long long>(GetCurrentProcessId()),static_cast<unsigned long long>(mode+1));
  udmbroker::Handle server(udmbroker::createServer(nonce)),client(CreateFileW(udmbroker::pipeName(nonce).c_str(),FILE_READ_DATA|FILE_WRITE_DATA|FILE_READ_ATTRIBUTES|FILE_WRITE_ATTRIBUTES|SYNCHRONIZE,0,nullptr,OPEN_EXISTING,FILE_FLAG_OVERLAPPED,nullptr));
  if(client.h==INVALID_HANDLE_VALUE)throw udmbroker::failure("Fixture connect failed");udmbroker::accept(server.h,1000,nullptr);
  auto responder=std::async(std::launch::async,[&]{auto request=udmbroker::receive(client.h);if(request.value("Action",std::string())!="Snapshot")throw std::runtime_error("Unexpected request");
   if(mode==0){udmbroker::send(client.h,{{"Version",udmbroker::Version},{"Ok",false},{"Error","Rejected fixture command"}});udmbroker::receive(client.h);udmbroker::send(client.h,{{"Version",udmbroker::Version},{"Ok",true}});}
   else if(mode==2){unsigned char byte=8;udmbroker::transfer(client.h,&byte,1,true,1000);}
   else if(mode==3)udmbroker::send(client.h,{{"Version",udmbroker::Version+1},{"Ok",true}});
   else if(mode==4)udmbroker::send(client.h,{{"Version",udmbroker::Version},{"Ok","true"}});
  });
  if(mode==0){auto result=udmbroker::exchange(server,{{"Action","Snapshot"}},1000);check(!result.at("Ok").get<bool>()&&server.h!=INVALID_HANDLE_VALUE,"Command rejection keeps a correctly framed channel");check(udmbroker::exchange(server,{{"Action","Snapshot"}},1000).at("Ok").get<bool>(),"Healthy channel accepts its next command");}
  else{bool rejected=false;try{udmbroker::exchange(server,{{"Action","Snapshot"}},200);}catch(...){rejected=true;}check(rejected&&server.h==INVALID_HANDLE_VALUE,"Timeout, partial frame or malformed response closes the channel");rejected=false;try{udmbroker::exchange(server,{{"Action","Snapshot"}},200);}catch(...){rejected=true;}check(rejected,"Closed channel rejects another command without replay");}
  responder.get();
 }
 for(unsigned idle:{0u,INFINITE}){
  wchar_t nonce[33]{};swprintf_s(nonce,L"%016llx%016llx",static_cast<unsigned long long>(GetCurrentProcessId()),static_cast<unsigned long long>(100+idle));
  udmbroker::Handle server(udmbroker::createServer(nonce)),client(CreateFileW(udmbroker::pipeName(nonce).c_str(),FILE_READ_DATA|FILE_WRITE_DATA|FILE_READ_ATTRIBUTES|FILE_WRITE_ATTRIBUTES|SYNCHRONIZE,0,nullptr,OPEN_EXISTING,FILE_FLAG_OVERLAPPED,nullptr));
  if(client.h==INVALID_HANDLE_VALUE)throw udmbroker::failure("Deadline fixture connect failed");udmbroker::accept(server.h,1000,nullptr);
  auto responder=std::async(std::launch::async,[&]{try{
   if(idle)Sleep(200);
   std::string text="{\"Ok\":true}";DWORD size=static_cast<DWORD>(text.size());
   std::vector<unsigned char> frame(reinterpret_cast<unsigned char*>(&size),reinterpret_cast<unsigned char*>(&size)+sizeof(size));frame.insert(frame.end(),text.begin(),text.end());
   for(auto byte:frame){udmbroker::transfer(client.h,&byte,1,true,1000);Sleep(30);}
  }catch(...){} });
  bool rejected=false;auto started=GetTickCount64();try{udmbroker::receive(server.h,120,nullptr,idle);}catch(...){rejected=true;}
  auto elapsed=GetTickCount64()-started;CloseHandle(server.h);server.h=INVALID_HANDLE_VALUE;responder.get();
  check(rejected&&elapsed<(idle?550u:350u),"Fragmented frame has one deadline even when every byte arrives before the per-read timeout");
  if(idle)check(elapsed>=200,"Unlimited idle wait does not consume the subsequent partial-frame deadline");
 }
 std::cout<<passed<<" passed, 0 failed"<<std::endl;return 0;
}catch(const std::exception& e){std::cerr<<"FAIL "<<e.what()<<std::endl;return 1;}}
