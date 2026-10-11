#pragma once
#include "../drivers/signed-network/BrokerProtocol.hpp"
#include "../drivers/signed-network/CaptureSession.hpp"
#include <fstream>
#include <future>
#include <ws2tcpip.h>
#include "../drivers/signed-network/DurableResponseSink.hpp"
namespace udm {
inline Json monitorTests(bool live,bool verifyRestart=true){
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
  auto delayedCommand=std::async(std::launch::async,[&]{return udmbroker::receive(server.h,100,nullptr,INFINITE);});
  check(delayedCommand.wait_for(std::chrono::milliseconds(250))==std::future_status::timeout,"Idle command wait survives the normal I/O deadline");
  udmbroker::send(client.h,{{"Action","AfterIdle"}});
  check(delayedCommand.get().value("Action",std::string())=="AfterIdle","Helper accepts commands after prolonged idle");
  udmbroker::Handle exitedPeer(CreateEventW(nullptr,TRUE,TRUE,nullptr));
  rejects([&]{udmbroker::receive(server.h,100,exitedPeer.h,INFINITE);},"Owner exit cancels indefinite idle command wait");
  unsigned char firstLengthByte=2;udmbroker::transfer(client.h,&firstLengthByte,1,true,1000);
  rejects([&]{udmbroker::receive(server.h,100,nullptr,INFINITE);},"Partial command header retains bounded timeout");
  DWORD incompleteBodyLength=2;udmbroker::transfer(client.h,&incompleteBodyLength,4,true,1000);
  rejects([&]{udmbroker::receive(server.h,100,nullptr,INFINITE);},"Partial command body retains bounded timeout");
  Json credits={{"Session",std::string(32,'a')},{"Generation",uint64_t(7)},{"ResponseCreditsSupported",true}};
  check(!udmbroker::responseCreditsAvailable(Json::object())&&!udmbroker::responseCreditsAvailable(Json{{"ResponseCreditsSupported",false}}),"Old and disabled helpers do not advertise credit enforcement");
  for(auto value:std::vector<Json>{0,"true",nullptr,Json::array()})rejects([&]{udmbroker::responseCreditsAvailable(Json{{"ResponseCreditsSupported",value}});},"Malformed helper capability cannot enable enforcement");
  rejects([&]{udmbroker::responseGrantCommand(Json::object(),1,1);},"Credit command requires helper capability negotiation");
  check(udmbroker::responseAllowance(Json(uint64_t(0)))==0&&udmbroker::responseAllowance(Json(uint64_t(INT64_MAX)))==uint64_t(INT64_MAX),"Allowance supports zero and the full signed file-size range");
  for(auto value:std::vector<Json>{-1,true,1.5,"1",nullptr,Json(uint64_t(INT64_MAX)+1)})rejects([&]{udmbroker::responseAllowance(value);},"Invalid or overflowing byte allowance is rejected");
  rejects([&]{udmbroker::responseGrantCommand(credits,0,1);},"Credit command rejects a missing response identifier");
  auto staleCredit=credits;staleCredit["Generation"]=uint64_t(0);rejects([&]{udmbroker::responseGrantCommand(staleCredit,1,1);},"Credit command rejects an inactive capture generation");
  const uint64_t largeAllowance=(uint64_t(1)<<53)+17;auto grant=udmbroker::responseGrantCommand(credits,3,largeAllowance);
  udmbroker::send(client.h,grant);auto receivedGrant=udmbroker::receive(server.h);
  check(receivedGrant==grant&&udmbroker::responseAllowance(receivedGrant.at("TotalBytes"))==largeAllowance,"Real named pipe preserves exact credit count above floating-point integer precision");
  check(grant["Session"]==credits["Session"]&&grant["Generation"]==credits["Generation"]&&grant["Id"]==3,"Credit command binds allowance to capture session, generation and response");
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
  check(!empty.running(),"Idle monitor has no live helper");
  empty.watchExisting({});check(!empty.running(),"Empty polling does not start or elevate a helper");
  rejects([&]{empty.watchExisting({GetCurrentProcessId()});},"Polling a missing helper reports restart without requesting elevation");
  check(!empty.running(),"Rejected polling leaves the helper stopped");
  empty.capture({});check(empty.snapshot()["Flows"].empty(),"Stopping idle capture does not start a broker");
  rejects([&]{empty.capture({80});},"Desktop capture requires a started monitor without elevating automatically");
  rejects([&]{empty.capture({80},true);},"Admission cannot elevate or start an idle monitor");
  rejects([&]{empty.admitResponse(Json::object(),1,INVALID_HANDLE_VALUE,INVALID_HANDLE_VALUE);},"Unscoped response admission is rejected before IPC");
  rejects([&]{empty.cancelResponse(Json::object(),1);},"Unscoped response cancellation is rejected before IPC");
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
   auto approvedBroker=initial["BrokerProcessId"];
   monitor.watchExisting({});auto idle=monitor.snapshot();
   check(monitor.running()&&idle["BrokerProcessId"]==approvedBroker&&num(idle,"Watched")==0,"Closing browser roots keeps the approved helper alive");
   monitor.watchExisting({GetCurrentProcessId()});auto resumed=monitor.snapshot();
   check(monitor.running()&&resumed["BrokerProcessId"]==approvedBroker&&num(resumed,"Watched")==1,"Reopened browser roots reuse the approved helper without elevation");
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
   for(unsigned captureMode:{0u,1u,4u,5u,2u,3u}){const bool creditResponse=captureMode==5,acceptResponse=captureMode!=0,cancelResponse=captureMode==2||captureMode==3,shutdownResponse=captureMode==3,finishAfterRootChange=captureMode==4,slowResponse=acceptResponse;std::atomic_bool releaseOrigin{false};
    struct SocketFile{SOCKET s=INVALID_SOCKET;~SocketFile(){if(s!=INVALID_SOCKET)closesocket(s);}} listener;
    listener.s=socket(AF_INET,SOCK_STREAM,IPPROTO_TCP);sockaddr_in address{};address.sin_family=AF_INET;address.sin_addr.s_addr=htonl(INADDR_LOOPBACK);
    if(bind(listener.s,reinterpret_cast<sockaddr*>(&address),sizeof(address))||listen(listener.s,1))throw std::runtime_error("Admission fixture listener failed.");
    int addressSize=sizeof(address);if(getsockname(listener.s,reinterpret_cast<sockaddr*>(&address),&addressSize))throw std::runtime_error("Admission fixture address failed.");
    monitor.capture({ntohs(address.sin_port)},true);
    std::string browserReply,originRequest,originError,browserError;std::thread originThread,browserThread;
    auto directory=fs::temp_directory_path()/wide("udm-admission-"+guid());fs::create_directories(directory);
    struct AdmissionCleanup{Monitor& monitor;std::thread& a;std::thread& b;fs::path folder;~AdmissionCleanup(){try{monitor.capture({});}catch(...){}if(a.joinable())a.join();if(b.joinable())b.join();std::error_code ec;fs::remove(folder/L"body",ec);fs::remove(folder/L"receipt",ec);fs::remove(folder,ec);}} cleanup{monitor,originThread,browserThread,directory};
    const std::string originalRequest="GET /one-use HTTP/1.1\r\nHost: 127.0.0.1:"+std::to_string(ntohs(address.sin_port))+"\r\n\r\n";
    auto timeout=[](SOCKET s){DWORD wait=6000;setsockopt(s,SOL_SOCKET,SO_RCVTIMEO,reinterpret_cast<char*>(&wait),sizeof(wait));setsockopt(s,SOL_SOCKET,SO_SNDTIMEO,reinterpret_cast<char*>(&wait),sizeof(wait));};
    auto sendAll=[](SOCKET s,const std::string& bytes){size_t at=0;while(at<bytes.size()){int n=::send(s,bytes.data()+at,static_cast<int>(bytes.size()-at),0);if(n<=0)throw std::runtime_error("Admission fixture send failed.");at+=n;}};
    originThread=std::thread([&]{try{fd_set ready;FD_ZERO(&ready);FD_SET(listener.s,&ready);timeval wait{6,0};if(select(0,&ready,nullptr,nullptr,&wait)!=1)throw std::runtime_error("Origin timed out.");SocketFile peer;peer.s=accept(listener.s,nullptr,nullptr);timeout(peer.s);char c=0;while(originRequest.find("\r\n\r\n")==std::string::npos){if(recv(peer.s,&c,1,0)!=1||originRequest.size()>4096)throw std::runtime_error("Missing original request.");originRequest+=c;}sendAll(peer.s,"HTTP/1.1 200 OK\r\nContent-Type: application/octet-stream\r\nContent-Length: 4\r\nConnection: close\r\n\r\n"+std::string(slowResponse?"da":"data"));if(slowResponse){auto end=GetTickCount64()+6000;while(!releaseOrigin&&GetTickCount64()<end)Sleep(5);if(!cancelResponse)sendAll(peer.s,"ta");}shutdown(peer.s,SD_SEND);}catch(const std::exception& e){originError=e.what();}});
    browserThread=std::thread([&]{try{SocketFile peer;peer.s=socket(AF_INET,SOCK_STREAM,IPPROTO_TCP);timeout(peer.s);if(connect(peer.s,reinterpret_cast<sockaddr*>(&address),sizeof(address)))throw std::runtime_error("Browser fixture connect failed.");sendAll(peer.s,originalRequest);char bytes[1024];for(;;){int n=recv(peer.s,bytes,sizeof(bytes),0);if(n<0)throw std::runtime_error("Browser fixture receive failed.");if(!n)break;browserReply.append(bytes,n);}}catch(const std::exception& e){browserError=e.what();}});
    Json offerSnapshot;for(unsigned i=0;i<100;++i){auto current=monitor.snapshot();auto& captureView=current["Capture"];if(captureView.contains("PendingResponses")&&!captureView["PendingResponses"].empty()){offerSnapshot=captureView;break;}Sleep(10);}
    if(offerSnapshot.is_null())throw std::runtime_error("No original response admission offer.");
    auto id=offerSnapshot["PendingResponses"][0]["Id"].get<uint64_t>();
    Manager catalog(directory/L"catalog");auto prefs=catalog.state["Settings"];prefs["DownloadFolder"]=utf8((directory/L"downloads").wstring());prefs["CategoryFolders"]=false;if(creditResponse){prefs["QuotaMb"]=1;prefs["LimitKbps"]=1;}catalog.setSettings(prefs);if(creditResponse){catalog.state["QuotaStart"]=date();catalog.state["QuotaBytes"]=1024*1024-2;catalog.save();}
    auto admitted=catalog.processOriginalResponses(monitor,offerSnapshot,[acceptResponse](const Json&){return acceptResponse;});
    if(!acceptResponse){originThread.join();browserThread.join();monitor.capture({});check(num(admitted,"Rejected")==1&&admitted["Errors"].empty()&&catalog.jobs.empty(),"Rejected capture policy creates no catalog job");check(originError.empty()&&browserError.empty()&&originRequest==originalRequest&&browserReply=="HTTP/1.1 200 OK\r\nContent-Type: application/octet-stream\r\nContent-Length: 4\r\nConnection: close\r\n\r\ndata","Rejected policy leaves exact original response with browser");continue;}
    check(num(admitted,"Accepted")==1&&admitted["Errors"].empty()&&catalog.jobs.size()==1,"Automatic controller reserves one catalog job and admits the original response");
    if(catalog.jobs.size()!=1)throw std::runtime_error("Missing captured catalog job.");auto job=catalog.jobs[0];auto saved=Json::parse(readText(catalog.root/L"state.json"));
    check(saved["Downloads"].size()==1&&saved["Downloads"][0].contains("OriginalResponse"),"Live handoff has durable job identity in download history");
    auto responseFolder=catalog.root/L"responses"/wide(job->id());
    Handle body(CreateFileW((responseFolder/L"body").c_str(),GENERIC_READ,FILE_SHARE_READ|FILE_SHARE_WRITE,nullptr,OPEN_EXISTING,0,nullptr));
    Handle receipt(CreateFileW((responseFolder/L"receipt").c_str(),GENERIC_READ,FILE_SHARE_READ|FILE_SHARE_WRITE,nullptr,OPEN_EXISTING,0,nullptr));
    auto stale=offerSnapshot;stale["Session"]="expired";rejects([&]{monitor.admitResponse(stale,id,body.h,receipt.h);},"Stale desktop admission session cannot transfer files");

    check(body&&receipt,"Response files remain openable before the origin finishes");
    check(monitor.admitResponse(offerSnapshot,id,body.h,receipt.h),"Lost admission acknowledgement can be retried without a second sink");
    if(!cancelResponse&&!finishAfterRootChange)releaseOrigin=true;
    if(creditResponse){
     bool held=false;for(unsigned i=0;i<100&&!held;++i){auto current=monitor.snapshot();auto result=catalog.processOriginalResponses(monitor,current["Capture"],[](const Json&){return true;});admitted["Published"]=num(admitted,"Published")+num(result,"Published");if(!result["Errors"].empty())throw std::runtime_error(result["Errors"].dump());held=num(job->data,"Received")==2;if(!held)Sleep(10);}
     check(held&&yes(job->data["OriginalResponse"],"FlowControlled")&&num(job->data["OriginalResponse"],"GrantedBytes")==2,"Live helper consumes only the two quota-authorized bytes");
     Sleep(100);auto heldSnapshot=monitor.snapshot();bool stillHeld=false;for(auto row:heldSnapshot["Capture"]["Responses"])if(row["Id"]==id)stillHeld=num(row,"Received")==2&&str(row,"State")=="Receiving";
     check(stillHeld&&yes(catalog.quotaStatus(job),"Waiting"),"Live response waits at exhausted quota without abandoning its original socket");
     catalog.state["QuotaStart"]=date(epoch()-2*3600000);catalog.save();bool finished=false;
     for(unsigned i=0;i<100&&!finished;++i){auto current=monitor.snapshot();auto result=catalog.processOriginalResponses(monitor,current["Capture"],[](const Json&){return true;});admitted["Published"]=num(admitted,"Published")+num(result,"Published");if(!result["Errors"].empty())throw std::runtime_error(result["Errors"].dump());for(auto row:current["Capture"]["Responses"])if(row["Id"]==id)finished=str(row,"State")=="Complete";if(!finished)Sleep(10);}
     check(finished&&num(catalog.state,"QuotaBytes")==2,"Live quota rollover resumes and accounts only the remaining original bytes");
    }
    if(finishAfterRootChange){
     bool started=false;for(unsigned i=0;i<100&&!started;++i){auto current=monitor.snapshot();for(const auto& response:current["Capture"]["Responses"])if(response["Id"]==id&&num(response,"Received")==2)started=true;if(!started)Sleep(10);}
     check(started,"Successful transfer is held mid-body before browser roots change");
     monitor.watch({GetCurrentProcessId(),broker});monitor.watch({GetCurrentProcessId()});
     auto current=monitor.snapshot();check(current["Capture"]["Generation"]==offerSnapshot["Generation"],"Successful transfer retains capture generation across root changes");
     releaseOrigin=true;
    }
    if(cancelResponse){bool progressSeen=false;for(unsigned i=0;i<100&&!progressSeen;++i){auto current=monitor.snapshot();for(const auto& row:current["Capture"]["Responses"])if(row["Id"]==id&&num(row,"Received")==2){progressSeen=true;catalog.processOriginalResponses(monitor,current["Capture"],[](const Json&){return true;});}if(!progressSeen)Sleep(10);}check(progressSeen,"Broker reports only the two received body bytes before cancellation");for(bool addRoot:{true,false}){monitor.watch(addRoot?std::vector<DWORD>{GetCurrentProcessId(),broker}:std::vector<DWORD>{GetCurrentProcessId()});auto refreshed=monitor.snapshot();bool receiving=false;for(const auto& response:refreshed["Capture"]["Responses"])if(response["Id"]==id&&str(response,"State")=="Receiving"&&num(response,"Received")==2)receiving=true;check(refreshed["Capture"]["Generation"]==offerSnapshot["Generation"]&&yes(refreshed["Capture"],"ResponseAdmissionEnabled")&&receiving,addRoot?"Adding a process root preserves active original-response transfer":"Removing a process root preserves active original-response transfer");}check(catalog.isActive(job)&&str(job->data,"Status")=="Downloading"&&num(job->data,"Received")==2&&num(job->data,"TransferredBytes")==2,"Catalog exposes live original-response progress");bool removeBlocked=false;try{catalog.remove(job);}catch(...){removeBlocked=true;}check(removeBlocked,"Active original response cannot be removed");if(shutdownResponse){catalog.stopOriginalResponses(monitor);check(!catalog.isActive(job)&&str(job->data,"Status")=="Paused"&&num(job->data,"Received")==2,"Capture shutdown releases writer and reconciles partial catalog data");catalog.stopOriginalResponses(monitor);check(monitor.snapshot()["Flows"].empty(),"Repeated capture shutdown does not restart the helper");}else{catalog.pause(job);check(str(job->data,"Status")=="Pausing"&&yes(job->data["OriginalResponse"],"PauseRequested"),"Pause saves original-response cancellation intent");auto cancellation=catalog.processOriginalResponses(monitor,monitor.snapshot()["Capture"],[](const Json&){return true;});check(cancellation["Errors"].empty()&&monitor.cancelResponse(offerSnapshot,id),"Catalog pause reaches the broker and cancellation remains idempotent");}releaseOrigin=true;}
    originThread.join();browserThread.join();
    for(unsigned i=0;!cancelResponse&&i<100;++i){LARGE_INTEGER size{};if(GetFileSizeEx(receipt.h,&size)&&size.QuadPart==128)break;Sleep(10);}
    if(!cancelResponse)check(!monitor.cancelResponse(offerSnapshot,id),"Completed response cannot be cancelled retroactively");
    if(!cancelResponse){auto completed=catalog.processOriginalResponses(monitor,monitor.snapshot()["Capture"],[](const Json&){return true;});admitted["Published"]=num(admitted,"Published")+num(completed,"Published");check(completed["Errors"].empty(),"Desktop collects completed response statistics before capture teardown");}
    monitor.capture({});auto recovered=udmnet::DurableResponseSink::recover(body.h,receipt.h);
    LARGE_INTEGER zero{};SetFilePointerEx(body.h,zero,nullptr,FILE_BEGIN);char bytes[4]{};DWORD count=0;ReadFile(body.h,bytes,sizeof(bytes),&count,nullptr);
    check(originError.empty()&&browserError.empty()&&originRequest==originalRequest&&browserReply==udmnet::HttpConversation::interceptedReply(),"One original browser request produces only the replacement reply");
    check(recovered.complete==!cancelResponse&&recovered.received==(cancelResponse?2:4)&&count==(cancelResponse?2u:4u)&&std::string(bytes,count)==(cancelResponse?"da":"data"),"Original response receipt and bytes match its completed or cancelled state");
    CloseHandle(body.h);body.h=INVALID_HANDLE_VALUE;CloseHandle(receipt.h);receipt.h=INVALID_HANDLE_VALUE;
    if(cancelResponse){catalog.recoverOriginalResponses();check(str(job->data,"Status")=="Paused"&&str(job->data["OriginalResponse"],"State")=="Interrupted"&&num(job->data,"Received")==2&&!fs::exists(job->target()),"Cancelled original response preserves partial bytes without publishing a complete file");continue;}
    auto finalized=catalog.processOriginalResponses(monitor,offerSnapshot,[](const Json&){return true;});
    check(num(admitted,"Published")+num(finalized,"Published")==1&&finalized["Errors"].empty()&&catalog.jobs.size()==1,"Repeated capture polling publishes once without a second catalog job");
    check(str(job->data,"Status")=="Complete"&&readText(job->target())=="data"&&str(job->data,"Sha256")==fileHash(job->target()),"Live original response becomes a verified completed UDM download");
    check(real(job->data,"TransferSeconds")>0&&num(job->data,"TransferredBytes")==4,"Completed original response retains nonzero duration and transferred bytes for average speed");
    Manager reopened(catalog.root);check(reopened.jobs.size()==1&&str(reopened.jobs[0]->data,"Status")=="Complete"&&readText(reopened.jobs[0]->target())=="data","Completed live capture survives catalog restart");
    fd_set second;FD_ZERO(&second);FD_SET(listener.s,&second);timeval noReplay{0,20000};check(select(0,&second,nullptr,nullptr,&noReplay)==0,"Completed capture did not issue another request to the original source");

   }
   monitor.watch({});check(child&&WaitForSingleObject(child.h,3000)==WAIT_OBJECT_0,"Stopping the desktop monitor closes the helper and filters");
   check(monitor.snapshot()["Flows"].empty(),"Stopped monitor returns an empty snapshot");
   if(verifyRestart){monitor.watch({GetCurrentProcessId()});auto restarted=monitor.snapshot();
   check(restarted.value("BrokerProcessId",0u)!=broker,"Monitor can start a fresh helper after stopping");
   monitor.watch({});
   }
   check(status["TestMode"]==false&&status["CodeIntegrityEnabled"]==true,"Live integration runs with Test Mode off and code integrity on");
  }
 }catch(const std::exception& e){report["Error"]=e.what();check(false,e.what());}
 size_t passed=0;for(auto& row:report["Results"])if(row["Passed"]==true)++passed;report["Passed"]=passed;report["Failed"]=report["Results"].size()-passed;return report;
}
}

