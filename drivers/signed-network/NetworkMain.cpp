#include "NetworkBackend.hpp"
#include "TcpGateway.hpp"
#include "CaptureObservationsJson.hpp"
#include <winhttp.h>
#include "third_party/json.hpp"
#include <fstream>
#include <iostream>
#include <functional>
#include <algorithm>
using Json=nlohmann::json;
using namespace udmnet;
using Clock=std::chrono::steady_clock;
struct Socket {
    SOCKET s=INVALID_SOCKET;
    explicit Socket(SOCKET value=INVALID_SOCKET):s(value){}
    ~Socket(){if(s!=INVALID_SOCKET)closesocket(s);}
    Socket(const Socket&)=delete;
};
static bool admin() {
    SID_IDENTIFIER_AUTHORITY a=SECURITY_NT_AUTHORITY;PSID group=nullptr;BOOL yes=FALSE;
    if(AllocateAndInitializeSid(&a,2,SECURITY_BUILTIN_DOMAIN_RID,DOMAIN_ALIAS_RID_ADMINS,0,0,0,0,0,0,&group)){CheckTokenMembership(nullptr,group,&yes);FreeSid(group);}return yes!=FALSE;
}
static Json policy() {
    struct Info{ULONG size,flags;} info{8,0};
    using Query=LONG(NTAPI*)(ULONG,PVOID,ULONG,PULONG);
    auto q=reinterpret_cast<Query>(GetProcAddress(GetModuleHandleW(L"ntdll.dll"),"NtQuerySystemInformation"));
    ULONG returned=0;
    Json r={{"Administrator",admin()},{"CodeIntegrityEnabled",nullptr},{"TestMode",nullptr},{"MemoryIntegrity",nullptr}};
    if(q&&q(103,&info,sizeof(info),&returned)==0&&returned==sizeof(info)){r["CodeIntegrityEnabled"]=(info.flags&1)!=0;r["TestMode"]=(info.flags&2)!=0;r["MemoryIntegrity"]=(info.flags&0x400)!=0;}
    return r;
}
static void timeout(SOCKET s) {
    DWORD t=5000;if(setsockopt(s,SOL_SOCKET,SO_RCVTIMEO,(char*)&t,sizeof(t))||setsockopt(s,SOL_SOCKET,SO_SNDTIMEO,(char*)&t,sizeof(t)))throw error("Cannot configure fixture timeout",WSAGetLastError());
}
static sockaddr_storage loopback(int family,unsigned port) {
    sockaddr_storage storage{};
    if(family==AF_INET){auto p=(sockaddr_in*)&storage;p->sin_family=AF_INET;p->sin_port=htons((u_short)port);p->sin_addr.s_addr=htonl(INADDR_LOOPBACK);}
    else{auto p=(sockaddr_in6*)&storage;p->sin6_family=AF_INET6;p->sin6_port=htons((u_short)port);p->sin6_addr=in6addr_loopback;}
    return storage;
}
static int addressSize(int family){return family==AF_INET?sizeof(sockaddr_in):sizeof(sockaddr_in6);}
static unsigned bindFixture(SOCKET s,int family) {
    auto a=loopback(family,0);if(bind(s,(sockaddr*)&a,addressSize(family)))throw error("Cannot bind loopback fixture",WSAGetLastError());
    int n=sizeof(a);if(getsockname(s,(sockaddr*)&a,&n))throw error("Cannot inspect loopback port",WSAGetLastError());
    return family==AF_INET?ntohs(((sockaddr_in*)&a)->sin_port):ntohs(((sockaddr_in6*)&a)->sin6_port);
}
static std::vector<char> payload(size_t n) {std::vector<char> p(n);for(size_t i=0;i<n;i++)p[i]=(char)((i*131+17)%251);return p;}
static void writeAll(SOCKET s,const std::vector<char>& p) {
    size_t offset=0;while(offset<p.size()){int n=send(s,p.data()+offset,(int)(p.size()-offset),0);if(n<=0)throw error("Fixture send failed",WSAGetLastError());offset+=n;}
}
static std::vector<char> readAll(SOCKET s,size_t count) {
    std::vector<char> p(count);size_t offset=0;while(offset<count){int n=recv(s,p.data()+offset,(int)(count-offset),0);if(n<=0)throw error("Fixture receive failed",WSAGetLastError());offset+=n;}return p;
}
static void udpClient(int family,unsigned port) {
    Socket client(socket(family,SOCK_DGRAM,IPPROTO_UDP));if(client.s==INVALID_SOCKET)throw error("Cannot create UDP client",WSAGetLastError());timeout(client.s);
    auto a=loopback(family,port);if(connect(client.s,(sockaddr*)&a,addressSize(family)))throw error("Cannot connect UDP fixture",WSAGetLastError());
    auto p=payload(1024);for(int i=0;i<32;i++){p[0]=(char)i;if(send(client.s,p.data(),(int)p.size(),0)!=(int)p.size())throw error("UDP send failed",WSAGetLastError());
        std::vector<char> echoed(2048);int n=recv(client.s,echoed.data(),(int)echoed.size(),0);if(n!=(int)p.size()||!std::equal(p.begin(),p.end(),echoed.begin()))throw std::runtime_error("UDP echo content changed.");}
}
static void childUdp(unsigned port) {
    auto exe=executableDirectory()/L"Udm.Network.exe";
    auto command=L"\""+exe.wstring()+L"\" --fixture-child "+std::to_wstring(port);
    STARTUPINFOW si{sizeof(si)};PROCESS_INFORMATION pi{};
    if(!CreateProcessW(exe.c_str(),command.data(),nullptr,nullptr,FALSE,CREATE_NO_WINDOW,nullptr,executableDirectory().c_str(),&si,&pi))throw error("Cannot launch excluded fixture process");
    CloseHandle(pi.hThread);DWORD result=WaitForSingleObject(pi.hProcess,15000),code=1;
    if(result!=WAIT_OBJECT_0){TerminateProcess(pi.hProcess,1);WaitForSingleObject(pi.hProcess,5000);}
    else GetExitCodeProcess(pi.hProcess,&code);
    CloseHandle(pi.hProcess);if(result!=WAIT_OBJECT_0||code)throw std::runtime_error("Excluded-process fixture failed.");
}
static Json fixture(Library& api,int family,bool udp,int mode,bool excluded=false) {
    Socket server(socket(family,udp?SOCK_DGRAM:SOCK_STREAM,udp?IPPROTO_UDP:IPPROTO_TCP));
    if(server.s==INVALID_SOCKET)throw error("Cannot create fixture server",WSAGetLastError());timeout(server.s);
    unsigned port=bindFixture(server.s,family);
    if(!udp&&listen(server.s,2))throw error("Cannot listen on loopback fixture",WSAGetLastError());
    std::unique_ptr<Socket> reserved;unsigned clientPort=port;
    if(mode==3){reserved=std::make_unique<Socket>(socket(family,udp?SOCK_DGRAM:SOCK_STREAM,udp?IPPROTO_UDP:IPPROTO_TCP));if(reserved->s==INVALID_SOCKET)throw error("Cannot reserve original fixture endpoint",WSAGetLastError());clientPort=bindFixture(reserved->s,family);}
    std::unique_ptr<LoopbackTransport> transport;
    if(mode)transport=std::make_unique<LoopbackTransport>(api,clientPort,mode>=2,mode==3?port:0);
    std::exception_ptr serverError;
    std::thread responder([&] {try {
        if(udp) {
            auto expected=payload(1024);
            for(int i=0;i<32;i++) {
                std::vector<char> received(2048);sockaddr_storage from{};int size=sizeof(from);
                int n=recvfrom(server.s,received.data(),(int)received.size(),0,(sockaddr*)&from,&size);
                expected[0]=(char)i;if(n!=(int)expected.size()||!std::equal(expected.begin(),expected.end(),received.begin()))throw std::runtime_error("UDP server received changed content.");
                if(sendto(server.s,received.data(),n,0,(sockaddr*)&from,size)!=n)throw error("UDP echo failed",WSAGetLastError());
            }
        } else {
            fd_set ready;FD_ZERO(&ready);FD_SET(server.s,&ready);timeval wait{5,0};
            if(select(0,&ready,nullptr,nullptr,&wait)!=1)throw std::runtime_error("TCP accept timed out.");
            Socket peer(accept(server.s,nullptr,nullptr));if(peer.s==INVALID_SOCKET)throw error("TCP accept failed",WSAGetLastError());timeout(peer.s);
            auto p=readAll(peer.s,256*1024);if(p!=payload(p.size()))throw std::runtime_error("TCP server received changed content.");writeAll(peer.s,p);
        }
    }catch(...){serverError=std::current_exception();}});
    auto start=Clock::now();std::exception_ptr clientError;
    try {
        if(udp){if(excluded)childUdp(clientPort);else udpClient(family,clientPort);}
        else {
            Socket client(socket(family,SOCK_STREAM,IPPROTO_TCP));if(client.s==INVALID_SOCKET)throw error("Cannot create TCP client",WSAGetLastError());timeout(client.s);
            auto a=loopback(family,clientPort);if(connect(client.s,(sockaddr*)&a,addressSize(family)))throw error("TCP fixture connection failed",WSAGetLastError());
            auto p=payload(256*1024);writeAll(client.s,p);if(readAll(client.s,p.size())!=p)throw std::runtime_error("TCP echoed content changed.");
        }
    }catch(...){clientError=std::current_exception();}
    responder.join();
    if(serverError)std::rethrow_exception(serverError);if(clientError)std::rethrow_exception(clientError);
    Json r={{"Family",family==AF_INET?"IPv4":"IPv6"},{"Protocol",udp?"UDP":"TCP"},{"Mode",mode==0?"direct":mode==1?"passive-copy":mode==2?"unchanged-reinjection":"loopback-port-redirection"},
        {"PayloadBytesEachDirection",udp?32*1024:256*1024},{"ExactContent",true},{"ElapsedMilliseconds",std::chrono::duration<double,std::milli>(Clock::now()-start).count()}};
    if(transport) {
        Sleep(30);transport->stop();
        r["ObservedPackets"]=transport->packetCount();r["ObservedPacketBytes"]=transport->packetBytes();r["TransportError"]=transport->lastError();
        if(!transport->packetCount()||transport->lastError())throw std::runtime_error("Packet transport did not capture cleanly.");
    }
    return r;
}
static std::string endpoint(const Event& e,bool remote) {
    auto p=remote?e.remote:e.local;char text[INET6_ADDRSTRLEN]{};
    if(e.ipv6) {
        UINT32 network[4];for(int i=0;i<4;i++)network[i]=htonl(p[3-i]);InetNtopA(AF_INET6,network,text,sizeof(text));
        return "["+std::string(text)+"]:"+std::to_string(remote?e.remotePort:e.localPort);
    }
    UINT32 v4=htonl(p[0]);InetNtopA(AF_INET,&v4,text,sizeof(text));
    return std::string(text)+":"+std::to_string(remote?e.remotePort:e.localPort);
}
static Json eventJson(const Event& e) {
    static const char* types[]={"packet","flow-open","flow-close","bind","connect","listen","accept","socket-close","observer-open","observer-close"};
    return {{"ProcessId",e.process},{"EndpointId",e.endpoint},{"Event",e.type<10?types[e.type]:"unknown"},{"Protocol",e.protocol==6?"TCP":e.protocol==17?"UDP":std::to_string(e.protocol)},
        {"Local",endpoint(e,false)},{"Remote",endpoint(e,true)},{"TimestampQpc",e.timestamp},{"ProcessCreated100ns",e.processCreated},
        {"ParentProcessId",e.parentProcess},{"ParentEndpointId",e.parentEndpoint},{"Outbound",e.outbound},{"Loopback",e.loopback}};
}
static Json selfTest(const fs::path& runtime) {
    Json report={{"Component","UDM signed network backend 0.2"},{"PolicyBefore",policy()},{"Results",Json::array()},{"FixtureRuns",Json::array()},
        {"IdmParityPercentage",nullptr},{"KernelVerifierRun",false},{"ProductionRedirectionImplemented",false}};
    auto check=[&](bool passed,const std::string& name){report["Results"].push_back({{"Name",name},{"Passed",passed}});};
    try {
        if(!admin())throw std::runtime_error("Run the signed-network self-test as administrator.");
        if(report["PolicyBefore"]["CodeIntegrityEnabled"]!=true||report["PolicyBefore"]["TestMode"]!=false)throw std::runtime_error("Normal code integrity with Test Mode off is required for this acceptance run.");
        check(true,"Windows code integrity is on and Test Mode is off");
        for(auto p:std::vector<std::vector<DWORD>>{{},{0},{4},{123,123},std::vector<DWORD>(33,123)}) {
            bool rejected=false;try{pidFilter(p);}catch(...){rejected=true;}check(rejected,"Reject invalid or unbounded watch scope");
        }
        Library api(runtime);check(true,"Official DLL and driver match pinned SHA-256 hashes");
        ProcessWatch watch(api,{GetCurrentProcessId()});check(true,"Signed driver opens scoped flow and socket handles");
        std::vector<Event> events;
        auto collect=[&]{Sleep(100);auto next=watch.take();events.insert(events.end(),next.begin(),next.end());};
        for(int family:{AF_INET,AF_INET6})for(bool udp:{false,true})for(int mode:{0,1,2,3}) {
            auto row=fixture(api,family,udp,mode);report["FixtureRuns"].push_back(row);
            check(true,std::string(family==AF_INET?"IPv4 ":"IPv6 ")+(udp?"UDP ":"TCP ")+(mode==0?"direct":mode==1?"passive copy":mode==2?"unchanged reinjection":"loopback port redirection")+" preserves exact bytes");collect();
        }
        report["FixtureRuns"].push_back(fixture(api,AF_INET,true,0,true));collect();
        check(!events.empty()&&std::all_of(events.begin(),events.end(),[](const auto& e){return e.process==GetCurrentProcessId();}),"Excluded child transfers successfully without attributing its events to the watched process");
        for(bool ipv6:{false,true})for(UINT8 protocol:{(UINT8)6,(UINT8)17}) {
            check(std::any_of(events.begin(),events.end(),[&](const auto& e){return e.ipv6==ipv6&&e.protocol==protocol&&e.layer==WINDIVERT_LAYER_FLOW;}),std::string(ipv6?"IPv6 ":"IPv4 ")+(protocol==6?"TCP":"UDP")+" flow attribution observed");
        }
        check(std::any_of(events.begin(),events.end(),[](const auto& e){return e.type==WINDIVERT_EVENT_SOCKET_CONNECT;}),"Socket connection lifecycle is observed");
        check(std::any_of(events.begin(),events.end(),[](const auto& e){return e.type==WINDIVERT_EVENT_FLOW_DELETED||e.type==WINDIVERT_EVENT_SOCKET_CLOSE;}),"Closed connections are observed");
        check(std::all_of(events.begin(),events.end(),[](const auto& e){if(e.layer!=WINDIVERT_LAYER_FLOW)return true;auto prefix=e.ipv6?"[::1]:":"127.0.0.1:";return endpoint(e,false).rfind(prefix,0)==0&&endpoint(e,true).rfind(prefix,0)==0&&e.localPort&&e.remotePort;}),"Observed endpoints preserve the IPv4/IPv6 loopback addresses and ports");
        check(watch.lastError()==0&&watch.lost()==0,"Bounded event queue reports no read errors or overflow");
        report["EventCount"]=events.size();
        auto stopStart=Clock::now();watch.stop();
        check(std::chrono::duration<double>(Clock::now()-stopStart).count()<2,"Idle receive cancellation and handle cleanup complete promptly");
        for(int i=0;i<8;i++){ProcessWatch repeat(api,{GetCurrentProcessId()});repeat.stop();}
        check(true,"Eight repeated start/stop cycles complete");
        {
            ProcessWatch pressure(api,{GetCurrentProcessId()});
            for(int i=0;i<700;i++) {
                Socket s(socket(AF_INET,SOCK_DGRAM,IPPROTO_UDP));if(s.s==INVALID_SOCKET)throw error("Cannot create pressure fixture socket",WSAGetLastError());
                auto address=loopback(AF_INET,9);if(connect(s.s,(sockaddr*)&address,sizeof(sockaddr_in)))throw error("Cannot bind pressure fixture socket",WSAGetLastError());
            }
            Sleep(300);pressure.stop();auto bounded=pressure.take();
            check(bounded.size()<=512&&pressure.lost()>0,"Overflow remains bounded and is reported explicitly");
            check(pressure.lastError()==0,"Event-pressure shutdown completes without a receive failure");
        }
        report["PolicyAfter"]=policy();
        check(report["PolicyAfter"]["TestMode"]==false&&report["PolicyAfter"]["CodeIntegrityEnabled"]==true,"Test Mode remains off and code integrity remains on");
    }catch(const std::exception& e){report["Error"]=e.what();check(false,e.what());}
    size_t passed=0;for(const auto& row:report["Results"])if(row["Passed"]==true)passed++;
    report["Passed"]=passed;report["Failed"]=report["Results"].size()-passed;return report;
}
#include "CoreTests.hpp"
#include "InternetTest.hpp"
static std::vector<DWORD> numericList(const wchar_t* input) {
    std::wstring value(input);if(value.empty()||value.back()==L',')throw std::runtime_error("Empty numeric list item.");
    std::vector<DWORD> result;std::wstringstream list(value);std::wstring item;
    while(std::getline(list,item,L',')) {
        if(item.empty()||item.find_first_not_of(L"0123456789")!=std::wstring::npos)throw std::runtime_error("Invalid numeric list item.");
        auto n=std::stoull(item);if(n>MAXDWORD)throw std::runtime_error("Numeric list item is too large.");result.push_back((DWORD)n);
    }
    return result;
}
static unsigned durationArgument(const wchar_t* input) {
    auto values=numericList(input);if(values.size()!=1||!values[0]||values[0]>600)throw std::runtime_error("Choose 1-600 seconds.");return values[0];
}
static Json gatewayJson(const GatewayStats& s) {
    return {{"Routed",s.routed},{"Bypassed",s.bypassed},{"RewrittenPackets",s.rewritten},{"Completed",s.completed},
        {"ConnectFailures",s.connectFailures},{"RelayFailures",s.relayFailures},{"ClientResets",s.clientResets},{"LastSocketError",s.lastSocketError},{"Candidates",s.candidates},{"Intercepted",s.intercepted},{"DecisionFailures",s.decisionFailures},
        {"ClientStreamBytes",s.clientBytes},{"ServerStreamBytes",s.serverBytes},{"Redirects",s.redirects},{"Active",s.active},{"Error",s.error}};
}
#include "DesktopBroker.hpp"
int wmain(int argc,wchar_t** argv) {
    try {
        WSADATA data{};if(WSAStartup(MAKEWORD(2,2),&data))throw std::runtime_error("Winsock startup failed.");
        struct Cleanup{~Cleanup(){WSACleanup();}}cleanup;
        const auto runtime=executableDirectory()/L"runtime";
        if(argc==4&&std::wstring(argv[1])==L"--desktop-broker"){auto pids=numericList(argv[2]);if(pids.size()!=1)throw std::runtime_error("Invalid broker parent.");return desktopBroker(runtime,pids[0],argv[3]);}
        if(argc==2&&std::wstring(argv[1])==L"--https-child"){httpsFixtureChild();return 0;}
        if(argc==3&&std::wstring(argv[1])==L"--internet-test") {
            auto report=internetTest(runtime);std::ofstream out(fs::path(argv[2]),std::ios::binary|std::ios::trunc);
            if(!out)throw std::runtime_error("Cannot open Internet test report.");out<<report.dump(2);out.close();
            if(!out)throw std::runtime_error("Cannot write Internet test report.");std::cout<<report.dump(2)<<std::endl;return report["Failed"]==0?0:1;
        }
        if(argc==5&&std::wstring(argv[1])==L"--stream-child") {
            int family=std::stoi(argv[2]);unsigned port=(unsigned)std::stoul(argv[3]),mode=(unsigned)std::stoul(argv[4]);
            if((family!=AF_INET&&family!=AF_INET6)||port<1024||port>65535||mode>2)throw std::runtime_error("Invalid stream fixture.");
            streamChild(family,port,mode);return 0;
        }
        if(argc==3&&(std::wstring(argv[1])==L"--core-test"||std::wstring(argv[1])==L"--gateway-test")) {
            bool live=std::wstring(argv[1])==L"--gateway-test";
            auto report=coreTests(live,runtime);std::ofstream out(fs::path(argv[2]),std::ios::binary|std::ios::trunc);
            if(!out)throw std::runtime_error("Cannot open core report.");out<<report.dump(2);out.close();
            if(!out)throw std::runtime_error("Cannot write core report.");
            std::cout<<report.dump(2)<<std::endl;return report["Failed"]==0?0:1;
        }
        if(argc==2&&std::wstring(argv[1])==L"--status") {
            Json r=policy();r["Backend"]="WinDivert 2.2.2-A";r["Version"]="0.3";r["DriverLoadedByStatus"]=false;
            LARGE_INTEGER frequency{};QueryPerformanceFrequency(&frequency);r["TimestampQpcFrequency"]=frequency.QuadPart;
            r["Capabilities"]={"process-tree metadata","IPv4/IPv6 TCP gateway","ordered socket streams","bounded HTTP/1 inspection"};
            r["DesktopBrokerProtocol"]=udmbroker::Version;r["HttpsDecryption"]=false;
            try{Library api(runtime);r["RuntimeVerified"]=true;}catch(const std::exception& e){r["RuntimeVerified"]=false;r["Error"]=e.what();}
            std::cout<<r.dump(2)<<std::endl;return r["RuntimeVerified"]==true?0:1;
        }
        if(argc==3&&std::wstring(argv[1])==L"--fixture-child") {
            size_t used=0;auto p=std::stoul(argv[2],&used);if(used!=wcslen(argv[2])||p<1024||p>65535)throw std::runtime_error("Invalid fixture port.");
            udpClient(AF_INET,(unsigned)p);return 0;
        }
        if(argc==3&&std::wstring(argv[1])==L"--self-test") {
            auto report=selfTest(runtime);std::ofstream out(fs::path(argv[2]),std::ios::binary|std::ios::trunc);
            if(!out)throw std::runtime_error("Cannot open self-test report.");out<<report.dump(2);out.close();
            if(!out)throw std::runtime_error("Cannot write self-test report.");
            std::cout<<report.dump(2)<<std::endl;return report["Failed"]==0?0:1;
        }
        if(argc==5&&std::wstring(argv[1])==L"--redirect-watch") {
            auto pids=numericList(argv[2]);pidFilter(pids);auto rawPorts=numericList(argv[3]);auto seconds=durationArgument(argv[4]);
            std::vector<unsigned> ports(rawPorts.begin(),rawPorts.end());
            if(!admin())throw std::runtime_error("The TCP gateway requires administrator privileges.");
            Library api(runtime);TcpGateway gateway(api,pids,ports,true);
            std::cout<<Json({{"Status","Redirecting selected process trees"},{"ProcessIds",pids},{"TcpPorts",ports},{"MaximumSeconds",seconds},{"AutomaticDownloadTakeover",false}}).dump()<<std::endl;
            auto end=Clock::now()+std::chrono::seconds(seconds);
            while(Clock::now()<end){auto s=gateway.snapshot();if(s.error)throw error("TCP gateway stopped",s.error);Sleep(100);}
            gateway.stop();auto result=gatewayJson(gateway.snapshot());result["Status"]="Stopped";result["CaptureObservations"]=captureObservationsJson(gateway.observations());std::cout<<result.dump()<<std::endl;return 0;
        }
        if(argc==4&&(std::wstring(argv[1])==L"--watch"||std::wstring(argv[1])==L"--watch-tree")) {
            auto seconds=durationArgument(argv[3]);auto pids=numericList(argv[2]);bool children=std::wstring(argv[1])==L"--watch-tree";
            pidFilter(pids);if(!admin())throw std::runtime_error("The signed monitor requires administrator privileges.");
            Library api(runtime);ProcessWatch watch(api,pids,children);auto end=Clock::now()+std::chrono::seconds(seconds);
            std::cout<<Json({{"Status","Monitoring"},{"Mode","Passive flow/socket metadata"},{"ProcessIds",pids},{"IncludeChildren",children},{"MaximumSeconds",seconds}}).dump()<<std::endl;
            while(Clock::now()<end) {
                for(const auto& event:watch.take())std::cout<<eventJson(event).dump()<<std::endl;
                if(watch.lastError())throw error("Monitoring stopped",watch.lastError());Sleep(100);
            }
            watch.stop();for(const auto& event:watch.take())std::cout<<eventJson(event).dump()<<std::endl;
            std::cout<<Json({{"Status","Stopped"},{"DroppedEvents",watch.lost()},{"ScopeLookupFailures",watch.scopeLookupFailures()}}).dump()<<std::endl;return 0;
        }
        std::cerr<<"Usage: Udm.Network --status | --watch/--watch-tree PID[,PID...] SECONDS | --redirect-watch PID[,PID...] PORT[,PORT...] SECONDS | --core-test/--gateway-test/--self-test REPORT.json\n";return 2;
    }catch(const std::exception& e){std::cerr<<e.what()<<std::endl;return 1;}
}
