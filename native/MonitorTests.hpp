#pragma once
#include "../drivers/signed-network/BrokerProtocol.hpp"
#include "../drivers/signed-network/CaptureSession.hpp"
#include <fstream>
#include <ws2tcpip.h>
namespace udm {
inline Json monitorTests(bool live){
 Json report={{"Component","Signed desktop monitor integration 0.36.0"},{"Results",Json::array()}};
 auto check=[&](bool ok,const char* name){report["Results"].push_back({{"Name",name},{"Passed",ok}});};
 auto rejects=[&](auto fn,const char* name){try{fn();check(false,name);}catch(const std::exception&){check(true,name);}};
 try {
  for(const auto& nonce:{L"",L"../escape",L"12345678"})rejects([&]{udmbroker::pipeName(nonce);},"Malformed pipe identifier rejected");
  auto nonce=wide(guid());udmbroker::Handle server(udmbroker::createServer(nonce));
  rejects([&]{udmbroker::Handle duplicate(udmbroker::createServer(nonce));},"Second pipe instance rejected");
  udmbroker::Handle client(CreateFileW(udmbroker::pipeName(nonce).c_str(),FILE_READ_DATA|FILE_WRITE_DATA|FILE_READ_ATTRIBUTES|FILE_WRITE_ATTRIBUTES|SYNCHRONIZE,0,nullptr,OPEN_EXISTING,FILE_FLAG_OVERLAPPED,nullptr));
  if(client.h==INVALID_HANDLE_VALUE)throw udmbroker::failure("Fixture cannot connect");
  udmbroker::accept(server.h,1000,nullptr);ULONG clientPid=0,serverPid=0;
  check(GetNamedPipeClientProcessId(server.h,&clientPid)&&clientPid==GetCurrentProcessId()&&GetNamedPipeServerProcessId(client.h,&serverPid)&&serverPid==GetCurrentProcessId(),"Both pipe peers expose their actual process IDs");
  udmbroker::send(client.h,{{"Version",udmbroker::Version},{"Action","Snapshot"}});
  check(udmbroker::receive(server.h).value("Action",std::string())=="Snapshot","Framed JSON survives real named-pipe transport");
  DWORD tooLarge=udmbroker::MaxFrame+1;udmbroker::transfer(client.h,&tooLarge,4,true,1000);
  rejects([&]{udmbroker::receive(server.h,1000);},"Oversized input rejected before allocation");
  rejects([&]{udmbroker::receive(server.h,20);},"Silent peer timeout cancels pending I/O");
  udmbroker::send(client.h,{{"Action","AfterTimeout"}});
  check(udmbroker::receive(server.h).value("Action",std::string())=="AfterTimeout","Timeout leaves no pending operation holding stack memory");
  auto raw=[&](std::string input){DWORD n=static_cast<DWORD>(input.size());udmbroker::transfer(client.h,&n,4,true,1000);udmbroker::transfer(client.h,input.data(),n,true,1000);};
  raw("[]");rejects([&]{udmbroker::receive(server.h);},"Non-object command rejected");
  raw("{\"x\":"+std::string(20,'[')+"0"+std::string(20,']')+"}");rejects([&]{udmbroker::receive(server.h);},"Excessive nesting rejected");
  check(udmbroker::capturePorts(Json::array({443,80,80}))==std::vector<unsigned>({80,443}),"Capture ports normalize without widening destination scope");
  for(const auto& ports:std::vector<Json>{Json::array({0}),Json::array({65536}),Json::array({-1}),Json::array({true}),Json::array({80.5}),Json::array({"80"}),Json::object()})
   rejects([&]{udmbroker::capturePorts(ports);},"Invalid capture port representation rejected");
  rejects([&]{udmbroker::capturePorts(Json(std::vector<unsigned>(33,80)));},"Oversized capture port list rejected");
  struct FakeGateway {unsigned* stopped;explicit FakeGateway(unsigned& value):stopped(&value){}~FakeGateway(){++*stopped;}};
  unsigned starts=0,stops=0;udmbroker::CaptureSession<FakeGateway> capture;
  auto create=[&](const auto&){++starts;return std::make_unique<FakeGateway>(stops);};
  rejects([&]{capture.configure({80},false,create);},"Capture requires an existing watched process scope");
  check(starts==0&&capture.currentGeneration()==0&&!capture.get(),"Rejected unscoped capture has no gateway side effects");
  capture.configure({80},true,create);auto firstGeneration=capture.currentGeneration();auto firstGateway=capture.get();
  capture.configure({80},true,create);
  check(starts==1&&stops==0&&capture.get()==firstGateway&&capture.currentGeneration()==firstGeneration,"Repeated capture request preserves gateway and generation");
  capture.configure({8080},true,create);
  check(starts==2&&stops==1&&capture.currentGeneration()>firstGeneration&&capture.destinationPorts()==std::vector<unsigned>({8080}),"Changing capture ports closes the old gateway and changes generation");
  rejects([&]{capture.configure({8081},true,[](const auto&)->std::unique_ptr<FakeGateway>{throw std::runtime_error("Fixture startup failure");});},"Gateway startup failure is reported");
  check(stops==2&&!capture.get()&&capture.destinationPorts().empty(),"Failed replacement leaves capture disabled without stale port scope");
  capture.configure({80},true,create);capture.stop();auto stoppedGeneration=capture.currentGeneration();capture.stop();
  check(starts==3&&stops==3&&!capture.get()&&capture.currentGeneration()==stoppedGeneration,"Stop closes the gateway exactly once and remains idempotent");
  rejects([&]{capture.configure({80},true,[](const auto&){return std::unique_ptr<FakeGateway>{};});},"Null gateway factory cannot report enabled capture");
  check(!capture.get()&&capture.destinationPorts().empty(),"Failed initial gateway leaves capture off");
  udmbroker::send(client.h,{{"Version",udmbroker::Version},{"Action","Capture"},{"Ports",Json::array({80,8080})}});
  auto capturedCommand=udmbroker::receive(server.h);
  check(capturedCommand["Action"]=="Capture"&&udmbroker::capturePorts(capturedCommand["Ports"])==std::vector<unsigned>({80,8080}),"Capture command crosses real framed pipe without changing its port scope");
  Monitor empty;check(empty.snapshot()["Flows"].empty(),"Idle monitor does not elevate or open a driver");
  empty.capture({});check(empty.snapshot()["Flows"].empty(),"Stopping idle capture does not start a broker");
  rejects([&]{empty.capture({80});},"Desktop capture requires a started monitor without elevating automatically");
  rejects([&]{empty.capture({0});},"Desktop validates capture ports before any broker call");
  rejects([&]{empty.watch({0});},"Invalid process ID rejected before elevation");
  std::vector<DWORD> many;for(DWORD i=10;i<43;++i)many.push_back(i);
  rejects([&]{empty.watch(many);},"Oversized process scope rejected before elevation");
  auto status=diagnostics();check(status.value("RuntimeVerified",false)&&status.value("DesktopBrokerProtocol",0u)==udmbroker::Version&&!status.value("DriverLoadedByStatus",true),"Desktop diagnostics verify the signed runtime without loading the driver");
  if(live){
   WSADATA data{};if(WSAStartup(MAKEWORD(2,2),&data))throw std::runtime_error("Fixture Winsock initialization failed.");
   struct WinsockCleanup{~WinsockCleanup(){WSACleanup();}}winsockCleanup;
   Monitor monitor;monitor.watch({GetCurrentProcessId()});auto initial=monitor.snapshot();
   check(initial.value("Watched",0u)==1&&initial.value("IncludeChildren",false),"Desktop starts authenticated elevated process-tree monitor");
   monitor.watch({GetCurrentProcessId(),GetCurrentProcessId()});
   check(monitor.snapshot()["Generation"]==initial["Generation"],"Unchanged and duplicate roots preserve the active observation session");
   for(auto family:{AF_INET,AF_INET6}){
    SOCKET sock=socket(family,SOCK_DGRAM,IPPROTO_UDP);if(sock==INVALID_SOCKET)throw std::runtime_error("Fixture socket failed.");
    sockaddr_storage a{};int n=0;
    if(family==AF_INET){auto v=reinterpret_cast<sockaddr_in*>(&a);v->sin_family=AF_INET;v->sin_addr.s_addr=htonl(INADDR_LOOPBACK);v->sin_port=htons(9);n=sizeof(*v);}
    else {auto v=reinterpret_cast<sockaddr_in6*>(&a);v->sin6_family=AF_INET6;v->sin6_addr=in6addr_loopback;v->sin6_port=htons(9);n=sizeof(*v);}
    int result=connect(sock,reinterpret_cast<sockaddr*>(&a),n);closesocket(sock);if(result)throw std::runtime_error("Fixture UDP connect failed.");
   }
   Json snap;bool v4=false,v6=false,unknown=true;
   for(int i=0;i<30&&(!v4||!v6);++i){Sleep(50);snap=monitor.snapshot();for(auto& row:snap["Flows"]){if(row.value("ProcessId",0u)==GetCurrentProcessId()){auto local=str(row,"Local");v4|=local.find('[')==std::string::npos;v6|=local.find('[')!=std::string::npos;unknown&=row["Received"].is_null()&&row["Sent"].is_null();}}}
   check(v4,"IPv4 socket events reach the desktop");
   check(v6,"IPv6 socket events reach the desktop");
   check(unknown&&!snap["Flows"].empty(),"Unavailable byte counts remain null");
   DWORD broker=snap.value("BrokerProcessId",0u);Handle child(OpenProcess(SYNCHRONIZE,FALSE,broker));
   monitor.watch({});check(child&&WaitForSingleObject(child.h,3000)==WAIT_OBJECT_0,"Stopping the desktop monitor closes the helper and filters");
   check(monitor.snapshot()["Flows"].empty(),"Stopped monitor returns an empty snapshot");
   monitor.watch({GetCurrentProcessId()});auto restarted=monitor.snapshot();
   check(restarted.value("BrokerProcessId",0u)!=broker,"Monitor can start a fresh helper after stopping");
   monitor.watch({});
   check(status["TestMode"]==false&&status["CodeIntegrityEnabled"]==true,"Live integration runs with Test Mode off and code integrity on");
  }
 }catch(const std::exception& e){report["Error"]=e.what();check(false,e.what());}
 size_t passed=0;for(auto& row:report["Results"])if(row["Passed"]==true)++passed;report["Passed"]=passed;report["Failed"]=report["Results"].size()-passed;return report;
}
}

