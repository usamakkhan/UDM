#pragma once
#include "HttpStream.hpp"
#include <array>
#include <optional>
namespace udmnet {
struct NetSocket {
    SOCKET value=INVALID_SOCKET;
    explicit NetSocket(SOCKET s=INVALID_SOCKET):value(s){}
    ~NetSocket(){if(value!=INVALID_SOCKET)closesocket(value);}
    NetSocket(const NetSocket&)=delete;
};
inline void nonblocking(SOCKET s){u_long on=1;if(ioctlsocket(s,FIONBIO,&on))throw error("Cannot set nonblocking transport",WSAGetLastError());}
inline bool pendingSocketError(int e){return e==WSAEWOULDBLOCK||e==WSAEINPROGRESS||e==WSAEALREADY;}
struct RelayResult {uint64_t clientBytes=0,serverBytes=0;size_t candidates=0,redirects=0,decisionFailures=0;bool responseOwned=false,responseComplete=false,responseFailed=false;bool intercepted=false,opaque=false,cancelled=false,timedOut=false;int socketError=0;bool errorAtClient=false;};
inline RelayResult relayStream(SOCKET client,SOCKET upstream,const std::atomic_bool& stop,
        std::function<bool(const DownloadCandidate&)> decide={},unsigned idleSeconds=60,ResponseSinkFactory accept={},bool retainHeaders=false) {
    nonblocking(client);nonblocking(upstream);
    HttpConversation http(std::move(decide),std::move(accept),retainHeaders);RelayResult result;
    std::string toServer,toClient,held;uint64_t interimForwarded=0;
    bool clientEof=false,serverEof=false,clientShut=false,serverShut=false,firstResponse=true;
    auto active=std::chrono::steady_clock::now();
    std::optional<std::chrono::steady_clock::time_point> cancelDrain;
    while(!stop) {
        if(http.responseCancelled()){
            if(!cancelDrain)cancelDrain=std::chrono::steady_clock::now()+std::chrono::milliseconds(250);
            if(toClient.empty()||std::chrono::steady_clock::now()>=*cancelDrain)break;
        }else if(http.responseFailed())break;
        if(http.responseComplete()&&toClient.empty())break;
        if(!http.responseOwned()&&clientEof&&toServer.empty()&&!serverShut){shutdown(upstream,SD_SEND);serverShut=true;}
        if(serverEof&&toClient.empty()&&!clientShut){shutdown(client,SD_SEND);clientShut=true;}
        if(clientEof&&serverEof&&toServer.empty()&&toClient.empty())break;
        if(std::chrono::steady_clock::now()-active>std::chrono::seconds(idleSeconds)){result.timedOut=true;break;}
        fd_set reads,writes;FD_ZERO(&reads);FD_ZERO(&writes);
        if(!http.responseOwned()&&!clientEof&&toServer.size()<65536)FD_SET(client,&reads);
        if(!http.responseCancelled()&&!serverEof&&toClient.size()+held.size()<65536)FD_SET(upstream,&reads);
        if(!toServer.empty())FD_SET(upstream,&writes);
        if(!toClient.empty())FD_SET(client,&writes);
        timeval wait{0,100000};int ready=select(0,&reads,&writes,nullptr,&wait);
        if(ready==SOCKET_ERROR){result.socketError=WSAGetLastError();break;}
        if(!ready)continue;
        auto flush=[&](SOCKET s,std::string& bytes) {
            if(bytes.empty()||!FD_ISSET(s,&writes))return true;
            int n=send(s,bytes.data(),(int)bytes.size(),0);
            if(n>0){bytes.erase(0,size_t(n));active=std::chrono::steady_clock::now();return true;}
            int e=WSAGetLastError();if(n<0&&pendingSocketError(e))return true;result.socketError=e?e:WSAECONNRESET;result.errorAtClient=s==client;return false;
        };
        if(!flush(upstream,toServer))break;
        if(!flush(client,toClient)){
            if(!http.responseOwned())break;
            // A browser closing its replaced response must not cancel UDM's body.
            toClient.clear();result.socketError=0;result.errorAtClient=false;clientShut=true;
        }
        std::array<char,16384> buffer{};
        for(bool fromClient:{true,false}) {
            SOCKET s=fromClient?client:upstream;
            if(!FD_ISSET(s,&reads)||(fromClient&&http.responseOwned()))continue;
            auto used=fromClient?toServer.size():toClient.size()+held.size();
            int n=recv(s,buffer.data(),(int)std::min(buffer.size(),size_t(65536)-used),0);
            if(n<0){int e=WSAGetLastError();if(pendingSocketError(e))continue;result.socketError=e;result.errorAtClient=fromClient;break;}
            if(!n) {
                if(fromClient)clientEof=true;else{serverEof=true;toClient+=held;held.clear();}
                http.finish(fromClient);continue;
            }
            active=std::chrono::steady_clock::now();
            if(fromClient)result.clientBytes+=n;else result.serverBytes+=n;
            http.feed(fromClient,std::string_view(buffer.data(),size_t(n)));
            if(fromClient)toServer.append(buffer.data(),size_t(n));
            else {
                if(firstResponse) {
                    held.append(buffer.data(),size_t(n));
                    if(http.interimEnd>interimForwarded) {
                        auto length=std::min<uint64_t>(http.interimEnd-interimForwarded,held.size());
                        toClient.append(held,0,size_t(length));held.erase(0,size_t(length));interimForwarded+=length;
                    }
                    if(http.intercepted) {
                        toClient+=HttpConversation::interceptedReply();held.clear();toServer.clear();
                        if(!http.responseOwned()){shutdown(upstream,SD_BOTH);serverEof=true;}
                        clientEof=true;firstResponse=false;
                    }else if(http.responses||http.opaque()||held.size()>=65536) {
                        if(held.size()>=65536)firstResponse=false;
                        toClient+=held;held.clear();firstResponse=false;
                    }
                }else if(!http.responseOwned())toClient.append(buffer.data(),size_t(n));
            }
        }
        if(result.socketError)break;
    }
    result.responseOwned=http.responseOwned();result.responseComplete=http.responseComplete();result.responseFailed=http.responseFailed()||(http.responseOwned()&&!http.responseComplete());
    result.cancelled=stop||http.responseCancelled();result.decisionFailures=http.decisionFailures;result.candidates=http.candidates;result.redirects=http.redirectCount;result.intercepted=http.intercepted;result.opaque=http.opaque();
    return result;
}
inline bool connectBounded(SOCKET s,const sockaddr* target,int size,const std::atomic_bool& stop,unsigned seconds=5) {
    nonblocking(s);if(connect(s,target,size)==0)return true;
    if(!pendingSocketError(WSAGetLastError()))return false;
    auto end=std::chrono::steady_clock::now()+std::chrono::seconds(seconds);
    while(!stop&&std::chrono::steady_clock::now()<end) {
        fd_set writes,errors;FD_ZERO(&writes);FD_ZERO(&errors);FD_SET(s,&writes);FD_SET(s,&errors);
        timeval wait{0,100000};int n=select(0,nullptr,&writes,&errors,&wait);
        if(n<0)return false;if(!n)continue;
        int e=0,len=sizeof(e);return getsockopt(s,SOL_SOCKET,SO_ERROR,(char*)&e,&len)==0&&e==0;
    }
    return false;
}
}
