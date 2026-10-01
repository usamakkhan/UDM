#pragma once
#include "StreamRelay.hpp"
#include <iphlpapi.h>
#include <tuple>
namespace udmnet {
using IpBytes=std::array<unsigned char,16>;
struct FlowKey {
    bool ipv6=false;IpBytes local{},remote{};uint16_t localPort=0,remotePort=0;
    bool operator<(const FlowKey& b)const{return std::tie(ipv6,local,remote,localPort,remotePort)<std::tie(b.ipv6,b.local,b.remote,b.localPort,b.remotePort);}
};
inline DWORD tcpOwner(const FlowKey& k) {
    DWORD bytes=0;ULONG family=k.ipv6?AF_INET6:AF_INET;
    if(GetExtendedTcpTable(nullptr,&bytes,FALSE,family,TCP_TABLE_OWNER_PID_ALL,0)!=ERROR_INSUFFICIENT_BUFFER||bytes>16*1024*1024)return 0;
    std::vector<unsigned char> data(bytes);
    if(GetExtendedTcpTable(data.data(),&bytes,FALSE,family,TCP_TABLE_OWNER_PID_ALL,0)!=NO_ERROR)return 0;
    if(k.ipv6) {
        auto table=reinterpret_cast<PMIB_TCP6TABLE_OWNER_PID>(data.data());
        for(DWORD i=0;i<table->dwNumEntries;i++){const auto& row=table->table[i];
            if(ntohs((u_short)row.dwLocalPort)==k.localPort&&ntohs((u_short)row.dwRemotePort)==k.remotePort&&
                !memcmp(row.ucLocalAddr,k.local.data(),16)&&!memcmp(row.ucRemoteAddr,k.remote.data(),16))return row.dwOwningPid;}
    }else {
        auto table=reinterpret_cast<PMIB_TCPTABLE_OWNER_PID>(data.data());
        for(DWORD i=0;i<table->dwNumEntries;i++){const auto& row=table->table[i];
            if(ntohs((u_short)row.dwLocalPort)==k.localPort&&ntohs((u_short)row.dwRemotePort)==k.remotePort&&
                !memcmp(&row.dwLocalAddr,k.local.data(),4)&&!memcmp(&row.dwRemoteAddr,k.remote.data(),4))return row.dwOwningPid;}
    }
    return 0;
}
struct GatewayStats {
    uint64_t routed=0,bypassed=0,rewritten=0,completed=0,connectFailures=0,relayFailures=0,candidates=0,intercepted=0,redirects=0;
    uint64_t clientBytes=0,serverBytes=0,clientResets=0;DWORD error=0;int lastSocketError=0;size_t active=0;
};
// TCP-only redirection selected by explicit process tree and destination ports.
// Unknown owners and full capacity bypass unchanged. Existing redirected
// connections end if this process exits; we do not claim crash-transparent recovery.
class TcpGateway {
    struct Route {
        FlowKey key;DWORD process=0,ifIndex=0,subIfIndex=0;
        UINT32 synSequence=0;uint16_t alias=0;bool loopback=false,accepted=false;
        std::atomic_bool done{false};std::thread worker;
        std::chrono::steady_clock::time_point created=std::chrono::steady_clock::now();
        std::chrono::steady_clock::time_point finished{};
    };
    Library& api;ProcessScope scope;std::set<unsigned> ports;
    std::unique_ptr<NetSocket> listen4,listen6;uint16_t proxy4=0,proxy6=0;
    HANDLE handle=INVALID_HANDLE_VALUE;std::atomic_bool stopping{false};
    std::thread capture,acceptor;std::mutex mutex;
    std::map<FlowKey,std::shared_ptr<Route>> routes;
    std::map<uint16_t,std::shared_ptr<Route>> aliases;
    uint16_t nextAlias=20000;GatewayStats counters;
    std::function<bool(const DownloadCandidate&)> decide;
    static uint16_t listener(NetSocket& s,bool v6) {
        s.value=socket(v6?AF_INET6:AF_INET,SOCK_STREAM,IPPROTO_TCP);if(s.value==INVALID_SOCKET)throw error("Cannot create TCP gateway listener",WSAGetLastError());
        int yes=1;if(setsockopt(s.value,SOL_SOCKET,SO_EXCLUSIVEADDRUSE,(char*)&yes,sizeof(yes)))throw error("Cannot reserve TCP gateway listener",WSAGetLastError());
        if(v6&&setsockopt(s.value,IPPROTO_IPV6,IPV6_V6ONLY,(char*)&yes,sizeof(yes)))throw error("Cannot configure IPv6 listener",WSAGetLastError());
        sockaddr_storage a{};int size=v6?sizeof(sockaddr_in6):sizeof(sockaddr_in);
        a.ss_family=v6?AF_INET6:AF_INET;
        if(bind(s.value,(sockaddr*)&a,size)||getsockname(s.value,(sockaddr*)&a,&size)||listen(s.value,64))throw error("Cannot bind TCP gateway listener",WSAGetLastError());
        nonblocking(s.value);return v6?ntohs(((sockaddr_in6*)&a)->sin6_port):ntohs(((sockaddr_in*)&a)->sin_port);
    }
    void serve(SOCKET accepted,const std::shared_ptr<Route>& route) noexcept {
        NetSocket client(accepted),server(socket(route->key.ipv6?AF_INET6:AF_INET,SOCK_STREAM,IPPROTO_TCP));
        try {
            if(server.value==INVALID_SOCKET)throw error("Cannot create upstream socket",WSAGetLastError());
            sockaddr_storage address{};int size=route->key.ipv6?sizeof(sockaddr_in6):sizeof(sockaddr_in);
            if(route->key.ipv6){auto a=(sockaddr_in6*)&address;a->sin6_family=AF_INET6;a->sin6_port=htons(route->key.remotePort);memcpy(&a->sin6_addr,route->key.remote.data(),16);
                if(IN6_IS_ADDR_LINKLOCAL(&a->sin6_addr))a->sin6_scope_id=route->ifIndex;
            }else{auto a=(sockaddr_in*)&address;a->sin_family=AF_INET;a->sin_port=htons(route->key.remotePort);memcpy(&a->sin_addr,route->key.remote.data(),4);}
            if(!connectBounded(server.value,(sockaddr*)&address,size,stopping)) {
                std::lock_guard<std::mutex> lock(mutex);++counters.connectFailures;
            }else {
                auto result=relayStream(client.value,server.value,stopping,decide);
                std::lock_guard<std::mutex> lock(mutex);++counters.completed;
                counters.clientBytes+=result.clientBytes;counters.serverBytes+=result.serverBytes;
                counters.candidates+=result.candidates;counters.redirects+=result.redirects;counters.intercepted+=result.intercepted?1:0;
                bool clientReset=result.errorAtClient&&(result.socketError==WSAECONNRESET||result.socketError==WSAECONNABORTED);
                if(clientReset)++counters.clientResets;
                if(result.socketError)counters.lastSocketError=result.socketError;
                if((result.socketError&&!clientReset)||result.timedOut)++counters.relayFailures;
            }
        }catch(...){std::lock_guard<std::mutex> lock(mutex);++counters.relayFailures;}
        {std::lock_guard<std::mutex> lock(mutex);route->finished=std::chrono::steady_clock::now();route->done=true;}
    }
    void accepts() noexcept {
        while(!stopping) {
            fd_set set;FD_ZERO(&set);FD_SET(listen4->value,&set);FD_SET(listen6->value,&set);timeval wait{0,100000};
            int n=select(0,&set,nullptr,nullptr,&wait);if(n<=0)continue;
            for(bool v6:{false,true}) {
                SOCKET listenerSocket=v6?listen6->value:listen4->value;if(!FD_ISSET(listenerSocket,&set))continue;
                sockaddr_storage peer{};int len=sizeof(peer);SOCKET s=accept(listenerSocket,(sockaddr*)&peer,&len);
                if(s==INVALID_SOCKET)continue;
                uint16_t alias=v6?ntohs(((sockaddr_in6*)&peer)->sin6_port):ntohs(((sockaddr_in*)&peer)->sin_port);
                IpBytes remote{};memcpy(remote.data(),v6?(void*)&((sockaddr_in6*)&peer)->sin6_addr:(void*)&((sockaddr_in*)&peer)->sin_addr,v6?16:4);
                std::shared_ptr<Route> route;
                {
                    std::lock_guard<std::mutex> lock(mutex);auto it=aliases.find(alias);
                    if(it!=aliases.end()&&!it->second->accepted&&it->second->key.ipv6==v6&&remote==(it->second->loopback?it->second->key.local:it->second->key.remote)){
                        route=it->second;route->accepted=true;
                    }
                }
                if(!route){closesocket(s);continue;}
                try{route->worker=std::thread([this,s,route]{serve(s,route);});}
                catch(...){closesocket(s);std::lock_guard<std::mutex> lock(mutex);route->finished=std::chrono::steady_clock::now();route->done=true;++counters.relayFailures;}
            }
        }
    }
    void cleanupLocked() {
        auto now=std::chrono::steady_clock::now();
        for(auto it=routes.begin();it!=routes.end();) {
            auto route=it->second;
            if((route->done&&now-route->finished>std::chrono::seconds(30))||(!route->accepted&&now-route->created>std::chrono::seconds(10))) {
                if(route->worker.joinable())route->worker.join();aliases.erase(route->alias);it=routes.erase(it);
            }else ++it;
        }
    }
    bool transform(unsigned char* packet,UINT count,WINDIVERT_ADDRESS& address) {
        PWINDIVERT_IPHDR ip=nullptr;PWINDIVERT_IPV6HDR ip6=nullptr;PWINDIVERT_TCPHDR tcp=nullptr;
        if(!api.parse(packet,count,&ip,&ip6,nullptr,nullptr,nullptr,&tcp,nullptr,nullptr,nullptr,nullptr,nullptr)||!tcp||(!ip&&!ip6))return true;
        FlowKey key;key.ipv6=ip6!=nullptr;size_t width=key.ipv6?16:4;
        void* src=key.ipv6?(void*)ip6->SrcAddr:(void*)&ip->SrcAddr;void* dst=key.ipv6?(void*)ip6->DstAddr:(void*)&ip->DstAddr;
        memcpy(key.local.data(),src,width);memcpy(key.remote.data(),dst,width);
        key.localPort=ntohs(tcp->SrcPort);key.remotePort=ntohs(tcp->DstPort);
        uint16_t proxy=key.ipv6?proxy6:proxy4;std::shared_ptr<Route> route;
        if(key.localPort==proxy) {
            std::lock_guard<std::mutex> lock(mutex);auto it=aliases.find(key.remotePort);
            if(it==aliases.end())return false;
            route=it->second;
            if(route->key.ipv6!=key.ipv6||key.remote!=(route->loopback?route->key.local:route->key.remote))return false;
            memcpy(src,route->key.remote.data(),width);memcpy(dst,route->key.local.data(),width);
            tcp->SrcPort=htons(route->key.remotePort);tcp->DstPort=htons(route->key.localPort);
        }else {
            if(!ports.count(key.remotePort))return true;
            {
                std::lock_guard<std::mutex> lock(mutex);cleanupLocked();
                auto it=routes.find(key);if(it!=routes.end())route=it->second;
                if(route&&tcp->Syn&&!tcp->Ack&&tcp->SeqNum!=route->synSequence&&route->done) {
                    if(route->worker.joinable())route->worker.join();aliases.erase(route->alias);routes.erase(key);route.reset();
                }
            }
            if(!route) {
                if(!tcp->Syn||tcp->Ack)return true;
                DWORD pid=tcpOwner(key);
                if(!pid||pid==GetCurrentProcessId()||!scope.matches(pid)){std::lock_guard<std::mutex> lock(mutex);++counters.bypassed;return true;}
                std::lock_guard<std::mutex> lock(mutex);
                if(routes.size()>=64){++counters.bypassed;return true;}
                route=std::make_shared<Route>();route->key=key;route->process=pid;route->synSequence=tcp->SeqNum;
                route->loopback=address.Loopback!=0;route->ifIndex=address.Network.IfIdx;route->subIfIndex=address.Network.SubIfIdx;
                while(aliases.count(nextAlias)||nextAlias==proxy4||nextAlias==proxy6){if(++nextAlias>=60000)nextAlias=20000;}
                route->alias=nextAlias++;routes.emplace(key,route);aliases.emplace(route->alias,route);++counters.routed;
            }
            // Never reinterpret a tuple reused by a new TCP connection.
            if(tcp->Syn&&!tcp->Ack&&tcp->SeqNum!=route->synSequence)return false;
            tcp->SrcPort=htons(route->alias);tcp->DstPort=htons(proxy);
            if(!route->loopback){memcpy(src,route->key.remote.data(),width);memcpy(dst,route->key.local.data(),width);}
        }
        address.Outbound=route->loopback?1u:0u;
        address.Network.IfIdx=route->ifIndex;address.Network.SubIfIdx=route->subIfIndex;
        if(!api.checksums(packet,count,&address,0))throw error("Cannot repair gateway packet",ERROR_INVALID_DATA);
        {std::lock_guard<std::mutex> lock(mutex);++counters.rewritten;}return true;
    }
    void packets() noexcept {
        std::vector<unsigned char> data(65575);
        try {
            while(!stopping) {
                WINDIVERT_ADDRESS address{};UINT count=0;
                if(!api.recv(handle,data.data(),(UINT)data.size(),&count,&address)) {
                    DWORD e=GetLastError();if(!stopping&&e!=ERROR_NO_DATA&&e!=ERROR_OPERATION_ABORTED)throw error("Gateway receive",e);break;
                }
                if(transform(data.data(),count,address)&&!api.send(handle,data.data(),count,nullptr,&address))throw error("Gateway send");
            }
        }catch(...){std::lock_guard<std::mutex> lock(mutex);counters.error=ERROR_GEN_FAILURE;stopping=true;}
    }
public:
    TcpGateway(Library& library,const std::vector<DWORD>& pids,const std::vector<unsigned>& destinationPorts,bool children,
        std::function<bool(const DownloadCandidate&)> decision={}):api(library),scope(pids,children),decide(std::move(decision)) {
        if(destinationPorts.empty()||destinationPorts.size()>32)throw std::runtime_error("Select 1-32 TCP destination ports.");
        for(auto port:destinationPorts)if(!port||port>65535||!ports.insert(port).second)throw std::runtime_error("Invalid or duplicate destination port.");
        listen4=std::make_unique<NetSocket>();listen6=std::make_unique<NetSocket>();
        proxy4=listener(*listen4,false);proxy6=listener(*listen6,true);
        if(ports.count(proxy4)||ports.count(proxy6))throw std::runtime_error("Listener port conflicts with destination scope.");
        std::string filter="outbound and !impostor and tcp and (tcp.SrcPort == "+std::to_string(proxy4)+" or tcp.SrcPort == "+std::to_string(proxy6);
        for(auto p:ports)filter+=" or tcp.DstPort == "+std::to_string(p);filter+=")";
        handle=api.open(filter.c_str(),WINDIVERT_LAYER_NETWORK,50,0);
        if(handle==INVALID_HANDLE_VALUE)throw error("Cannot open scoped TCP gateway");
        try{capture=std::thread([this]{packets();});acceptor=std::thread([this]{accepts();});}
        catch(...){stop();throw;}
    }
    ~TcpGateway(){stop();}
    void stop()noexcept {
        stopping=true;
        if(handle!=INVALID_HANDLE_VALUE)api.shutdown(handle,WINDIVERT_SHUTDOWN_RECV);
        if(capture.joinable())capture.join();
        if(acceptor.joinable())acceptor.join();
        // Closing the listeners first prevents an unfiltered open proxy.
        listen4.reset();listen6.reset();
        for(auto& item:routes)if(item.second->worker.joinable())item.second->worker.join();
        if(handle!=INVALID_HANDLE_VALUE){api.close(handle);handle=INVALID_HANDLE_VALUE;}
    }
    GatewayStats snapshot(){std::lock_guard<std::mutex> lock(mutex);auto value=counters;for(const auto& r:routes)if(!r.second->done)++value.active;return value;}
};
}
