#pragma once
#include "../drivers/signed-network/BrokerProtocol.hpp"
#include "../drivers/signed-network/CaptureSession.hpp"
#include <fstream>
#include <ws2tcpip.h>
#include "../drivers/signed-network/DurableResponseSink.hpp"
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
  rejects([&]{empty.capture({80},true);},"Admission cannot elevate or start an idle monitor");
  rejects([&]{empty.admitResponse(Json::object(),1,INVALID_HANDLE_VALUE,INVALID_HANDLE_VALUE);},"Unscoped response admission is rejected before IPC");
  rejects([&]{empty.rejectResponse(Json::object(),1);},"Unscoped response rejection is rejected before IPC");
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
   {
    struct SocketFile{SOCKET s=INVALID_SOCKET;~SocketFile(){if(s!=INVALID_SOCKET)closesocket(s);}} listener;
    listener.s=socket(AF_INET,SOCK_STREAM,IPPROTO_TCP);sockaddr_in address{};address.sin_family=AF_INET;address.sin_addr.s_addr=htonl(INADDR_LOOPBACK);
    if(bind(listener.s,reinterpret_cast<sockaddr*>(&address),sizeof(address))||listen(listener.s,1))throw std::runtime_error("Admission fixture listener failed.");
    int addressSize=sizeof(address);if(getsockname(listener.s,reinterpret_cast<sockaddr*>(&address),&addressSize))throw std::runtime_error("Admission fixture address failed.");
    monitor.capture({ntohs(address.sin_port)},true);
    std::string browserReply,originRequest,originError,browserError;std::thread originThread,browserThread;
    auto directory=fs::temp_directory_path()/wide("udm-admission-"+guid());fs::create_directories(directory);
    struct AdmissionCleanup{Monitor& monitor;std::thread& a;std::thread& b;fs::path folder;~AdmissionCleanup(){try{monitor.capture({});}catch(...){}if(a.joinable())a.join();if(b.joinable())b.join();std::error_code ec;fs::remove(folder/L"body",ec);fs::remove(folder/L"receipt",ec);fs::remove(folder,ec);}} cleanup{monitor,originThread,browserThread,directory};
    auto timeout=[](SOCKET s){DWORD wait=6000;setsockopt(s,SOL_SOCKET,SO_RCVTIMEO,reinterpret_cast<char*>(&wait),sizeof(wait));setsockopt(s,SOL_SOCKET,SO_SNDTIMEO,reinterpret_cast<char*>(&wait),sizeof(wait));};
    auto sendAll=[](SOCKET s,const std::string& bytes){size_t at=0;while(at<bytes.size()){int n=::send(s,bytes.data()+at,static_cast<int>(bytes.size()-at),0);if(n<=0)throw std::runtime_error("Admission fixture send failed.");at+=n;}};
    originThread=std::thread([&]{try{fd_set ready;FD_ZERO(&ready);FD_SET(listener.s,&ready);timeval wait{6,0};if(select(0,&ready,nullptr,nullptr,&wait)!=1)throw std::runtime_error("Origin timed out.");SocketFile peer;peer.s=accept(listener.s,nullptr,nullptr);timeout(peer.s);char c=0;while(originRequest.find("\r\n\r\n")==std::string::npos){if(recv(peer.s,&c,1,0)!=1||originRequest.size()>4096)throw std::runtime_error("Missing original request.");originRequest+=c;}sendAll(peer.s,"HTTP/1.1 200 OK\r\nContent-Type: application/octet-stream\r\nContent-Length: 4\r\nConnection: close\r\n\r\ndata");shutdown(peer.s,SD_SEND);}catch(const std::exception& e){originError=e.what();}});
    browserThread=std::thread([&]{try{SocketFile peer;peer.s=socket(AF_INET,SOCK_STREAM,IPPROTO_TCP);timeout(peer.s);if(connect(peer.s,reinterpret_cast<sockaddr*>(&address),sizeof(address)))throw std::runtime_error("Browser fixture connect failed.");sendAll(peer.s,"GET /one-use HTTP/1.1\r\nHost: fixture.invalid\r\n\r\n");char bytes[1024];for(;;){int n=recv(peer.s,bytes,sizeof(bytes),0);if(n<0)throw std::runtime_error("Browser fixture receive failed.");if(!n)break;browserReply.append(bytes,n);}}catch(const std::exception& e){browserError=e.what();}});
    Json offerSnapshot;for(unsigned i=0;i<100;++i){auto current=monitor.snapshot();auto& captureView=current["Capture"];if(captureView.contains("PendingResponses")&&!captureView["PendingResponses"].empty()){offerSnapshot=captureView;break;}Sleep(10);}
    if(offerSnapshot.is_null())throw std::runtime_error("No original response admission offer.");
    auto id=offerSnapshot["PendingResponses"][0]["Id"].get<uint64_t>();
    Handle body(CreateFileW((directory/L"body").c_str(),GENERIC_READ|GENERIC_WRITE,FILE_SHARE_READ|FILE_SHARE_WRITE,nullptr,CREATE_NEW,0,nullptr));
    Handle receipt(CreateFileW((directory/L"receipt").c_str(),GENERIC_READ|GENERIC_WRITE,FILE_SHARE_READ|FILE_SHARE_WRITE,nullptr,CREATE_NEW,0,nullptr));
    auto stale=offerSnapshot;stale["Session"]="expired";rejects([&]{monitor.admitResponse(stale,id,body.h,receipt.h);},"Stale desktop admission session cannot transfer files");
    check(monitor.admitResponse(offerSnapshot,id,body.h,receipt.h),"Desktop broker accepts reserved body and receipt handles");
    check(monitor.admitResponse(offerSnapshot,id,body.h,receipt.h),"Lost admission acknowledgement can be retried without a second sink");
    originThread.join();browserThread.join();
    for(unsigned i=0;i<100;++i){LARGE_INTEGER size{};if(GetFileSizeEx(receipt.h,&size)&&size.QuadPart==128)break;Sleep(10);}
    monitor.capture({});auto recovered=udmnet::DurableResponseSink::recover(body.h,receipt.h);
    LARGE_INTEGER zero{};SetFilePointerEx(body.h,zero,nullptr,FILE_BEGIN);char bytes[4]{};DWORD count=0;ReadFile(body.h,bytes,sizeof(bytes),&count,nullptr);
    check(originError.empty()&&browserError.empty()&&originRequest=="GET /one-use HTTP/1.1\r\nHost: fixture.invalid\r\n\r\n"&&browserReply==udmnet::HttpConversation::interceptedReply(),"One original browser request produces only the replacement reply");
    check(recovered.complete&&recovered.received==4&&count==4&&std::string(bytes,count)=="data","Original response persists through driver, broker and recipient file handles");
   }
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

