#pragma once
#include "Core.hpp"
#include <ws2tcpip.h>
#include <regex>
#include <fstream>

// Loopback-only HTTP/1.1 server: persistent sockets and one deliberately slow range.
class TransferFixture {
    SOCKET listener = INVALID_SOCKET;
    std::thread server;
    std::vector<std::thread> clients;
    std::mutex socketsMutex;
    std::mutex rangesMutex;std::vector<udm::i64> starts;
    std::set<SOCKET> sockets;
    std::atomic_bool stopping{false};
    bool sendAll(SOCKET socket, const char* data, size_t count) {
        for(size_t at=0; at<count && !stopping;) {
            int n=::send(socket,data+at,(int)std::min<size_t>(65536,count-at),0);
            if(n<=0)return false;
            at+=n;
        }
        return !stopping;
    }
    void serve(SOCKET socket) {
        DWORD timeout=5000;
        setsockopt(socket,SOL_SOCKET,SO_RCVTIMEO,(const char*)&timeout,sizeof(timeout));
        setsockopt(socket,SOL_SOCKET,SO_SNDTIMEO,(const char*)&timeout,sizeof(timeout));
        BOOL noDelay=TRUE;
        setsockopt(socket,IPPROTO_TCP,TCP_NODELAY,(const char*)&noDelay,sizeof(noDelay));
        Sleep(40); // Per-connection setup latency, identical for the two engine builds.
        try {
            std::string pending;
            char buffer[65536];
            while(!stopping) {
                while(pending.find("\r\n\r\n")==std::string::npos) {
                    int n=recv(socket,buffer,sizeof(buffer),0);
                    if(n<=0)goto finished;
                    pending.append(buffer,n);
                    if(pending.size()>32768)goto finished;
                }
                auto stop=pending.find("\r\n\r\n")+4;
                auto request=pending.substr(0,stop);pending.erase(0,stop);
                ++requests;
                std::smatch match;
                bool range=std::regex_search(request,match,std::regex("Range: bytes=([0-9]+)-([0-9]*)",std::regex::icase));
                udm::i64 begin=range?std::stoll(match[1]):0;
                udm::i64 end=range&&!match[2].str().empty()?std::stoll(match[2]):size-1;
                if(begin<0||end<begin||end>=size)goto finished;
                bool probe=range&&begin==0&&end==0; if(range&&begin==262144)resumedPrefix=true;
                if(!probe){std::lock_guard<std::mutex> lock(rangesMutex);starts.push_back(begin);}
                bool slow=request.find(" /straggler ")!=std::string::npos&&begin==0&&!probe;
                bool missingValidator=request.find(" /missing-validator ")!=std::string::npos&&!probe;
                auto response=std::string(range?"HTTP/1.1 206 Partial Content\r\n":"HTTP/1.1 200 OK\r\n")+
                    (missingValidator?"":"ETag: \"persistent-fixture-v1\"\r\n")+"Content-Length: "+std::to_string(end-begin+1)+"\r\n";
                if(range)response+="Content-Range: bytes "+std::to_string(begin)+"-"+std::to_string(end)+"/"+std::to_string(size)+"\r\n";
                response+="Connection: keep-alive\r\n\r\n";
                if(!sendAll(socket,response.data(),response.size()))goto finished;
                for(auto at=begin;at<=end;) {
                    size_t count=(size_t)std::min<udm::i64>(sizeof(buffer),end-at+1);
                    for(size_t i=0;i<count;++i)buffer[i]=value(at+(udm::i64)i);
                    if(!probe)Sleep(slow?slowDelay:fastDelay);
                    if(!sendAll(socket,buffer,count))goto finished;
                    at+=(udm::i64)count;
                }
            }
        } catch(...) {}
finished:
        {std::lock_guard<std::mutex> lock(socketsMutex);sockets.erase(socket);}
        shutdown(socket,SD_BOTH);closesocket(socket);
    }
public:
    unsigned short port=0;
    udm::i64 size; DWORD slowDelay,fastDelay;
    std::atomic_int connections{0},requests{0}; std::atomic_bool resumedPrefix{false};
    explicit TransferFixture(udm::i64 bytes=32*1024*1024,DWORD delay=120,DWORD fast=1):size(bytes),slowDelay(delay),fastDelay(fast) {
        listener=socket(AF_INET,SOCK_STREAM,IPPROTO_TCP);
        if(listener==INVALID_SOCKET)throw std::runtime_error("Transfer fixture socket failed.");
        sockaddr_in address{};address.sin_family=AF_INET;address.sin_addr.s_addr=htonl(INADDR_LOOPBACK);
        if(bind(listener,(sockaddr*)&address,sizeof(address))||listen(listener,32))throw std::runtime_error("Transfer fixture bind failed.");
        int length=sizeof(address);getsockname(listener,(sockaddr*)&address,&length);port=ntohs(address.sin_port);
        server=std::thread([this]{while(!stopping){fd_set read;FD_ZERO(&read);FD_SET(listener,&read);timeval delay{0,100000};
            if(select(0,&read,nullptr,nullptr,&delay)>0){auto client=accept(listener,nullptr,nullptr);if(client!=INVALID_SOCKET){
                {std::lock_guard<std::mutex> lock(socketsMutex);sockets.insert(client);}
                ++connections;clients.emplace_back([this,client]{serve(client);});
            }}
        }});
    }
    ~TransferFixture(){stopping=true;if(server.joinable())server.join();closesocket(listener);
        {std::lock_guard<std::mutex> lock(socketsMutex);for(auto socket:sockets)shutdown(socket,SD_BOTH);}
        for(auto& client:clients)if(client.joinable())client.join();
    }
    std::vector<udm::i64> rangeStarts(){std::lock_guard<std::mutex> lock(rangesMutex);return starts;}
    static char value(udm::i64 offset){return (char)((offset*31+offset/65536+7)%251);}
    std::string url(const char* path)const{return "http://127.0.0.1:"+std::to_string(port)+path;}
    void expected(const udm::fs::path& path)const {
        std::ofstream output(path,std::ios::binary);char buffer[65536];
        for(udm::i64 at=0;at<size;){auto count=(size_t)std::min<udm::i64>(sizeof(buffer),size-at);for(size_t i=0;i<count;++i)buffer[i]=value(at+(udm::i64)i);output.write(buffer,count);at+=count;}
        if(!output)throw std::runtime_error("Cannot write expected fixture bytes.");
    }
};
