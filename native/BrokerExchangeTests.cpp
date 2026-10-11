#include "../drivers/signed-network/BrokerProtocol.hpp"
#include <future>
#include <iostream>
int main(int argc,char** argv){if(argc==2&&std::string(argv[1])=="--idle-peer"){Sleep(5000);return 0;}unsigned passed=0;try{
 auto check=[&](bool ok,const char* message){if(!ok)throw std::runtime_error(message);++passed;std::cout<<"PASS "<<message<<std::endl;};
 udmbroker::Handle self(OpenProcess(SYNCHRONIZE|PROCESS_QUERY_LIMITED_INFORMATION,FALSE,GetCurrentProcessId()));
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
 {
  wchar_t nonce[33]{};swprintf_s(nonce,L"%016llx%016llx",static_cast<unsigned long long>(GetCurrentProcessId()),900ull);
  udmbroker::Handle server(udmbroker::createServer(nonce));std::promise<void> processed,release;auto released=release.get_future();
  auto helper=std::async(std::launch::async,[&]{try{
   udmbroker::Handle client(udmbroker::connectOwner(nonce,self.h,1000));udmbroker::helperHello(client.h,nonce,self.h);
   auto request=udmbroker::receive(client.h);if(request.value("Action",std::string())!="AdmitFixture")throw std::runtime_error("Unexpected fixture admission");unsigned admissions=1;processed.set_value();released.wait();
   try{udmbroker::send(client.h,{{"Version",udmbroker::Version},{"Ok",true}});}catch(const udmbroker::TransportError&){udmbroker::reconnectHelper(client,nonce,self.h);}
   request=udmbroker::receive(client.h);if(request.value("Action",std::string())!="Snapshot")throw std::runtime_error("Admission was replayed after reconnect");
   udmbroker::send(client.h,{{"Version",udmbroker::Version},{"Ok",true},{"Admissions",admissions},{"Generation",7}});
   }catch(const std::exception& e){std::cerr<<"Helper fixture: "<<e.what()<<std::endl;throw;}
  });
  check(udmbroker::authenticateHelper(server.h,nonce,self.h,1000),"Authenticated helper advertises reconnection capability");
  udmbroker::send(server.h,{{"Action","AdmitFixture"}});processed.get_future().wait();CloseHandle(server.h);server.h=INVALID_HANDLE_VALUE;release.set_value();
  server.h=udmbroker::reconnectServer(nonce,self.h,2000);
  check(udmbroker::authenticateHelper(server.h,nonce,self.h,2000),"New pipe authenticates the same live helper and session");
  auto reply=udmbroker::exchange(server,{{"Action","Snapshot"}});
  check(reply.at("Admissions")==1&&reply.at("Generation")==7,"Reconnect preserves helper-owned state without replaying the lost admission reply");helper.get();
 }
 {
  wchar_t executable[32768]{};GetModuleFileNameW(nullptr,executable,32768);std::wstring command=L"\""+std::wstring(executable)+L"\" --idle-peer";STARTUPINFOW startup{sizeof(startup)};PROCESS_INFORMATION process{};
  if(!CreateProcessW(executable,command.data(),nullptr,nullptr,FALSE,CREATE_NO_WINDOW,nullptr,nullptr,&startup,&process))throw udmbroker::failure("Cannot start identity fixture");
  udmbroker::Handle peer(process.hProcess),thread(process.hThread);
  struct Cleanup{HANDLE process;~Cleanup(){if(WaitForSingleObject(process,0)==WAIT_TIMEOUT){TerminateProcess(process,0);WaitForSingleObject(process,1000);}}} cleanup{peer.h};
  wchar_t nonce[33]{};swprintf_s(nonce,L"%016llx%016llx",static_cast<unsigned long long>(GetCurrentProcessId()),901ull);udmbroker::Handle server(udmbroker::createServer(nonce));
  bool rejected=false;try{udmbroker::Handle wrong(udmbroker::connectOwner(nonce,peer.h,1000));}catch(const std::exception& e){rejected=std::string(e.what()).find("server identity mismatch")!=std::string::npos;}
  check(rejected,"Helper rejects a pipe hosted by a different process even with the correct nonce");
  CloseHandle(server.h);server.h=udmbroker::createServer(nonce);
  udmbroker::Handle client(udmbroker::connectOwner(nonce,self.h,1000));rejected=false;
  try{udmbroker::authenticateHelper(server.h,nonce,peer.h,1000);}catch(const std::exception& e){rejected=std::string(e.what()).find("process identity mismatch")!=std::string::npos;}
  check(rejected,"Desktop rejects a reconnecting client other than the held helper process");
 }
 std::cout<<passed<<" passed, 0 failed"<<std::endl;return 0;
}catch(const std::exception& e){std::cerr<<"FAIL "<<e.what()<<std::endl;return 1;}}
