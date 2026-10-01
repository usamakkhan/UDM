#pragma once
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <winsock2.h>
#include <ws2tcpip.h>
#include <windows.h>
#include <bcrypt.h>
#include <filesystem>
#include <vector>
#include <set>
#include <string>
#include <sstream>
#include <iomanip>
#include <stdexcept>
#include <mutex>
#include <thread>
#include <atomic>
#include <chrono>
#include <memory>
#include "windivert.h"

namespace udmnet {
namespace fs=std::filesystem;
inline std::runtime_error error(const char* action,DWORD code=GetLastError()) {
    return std::runtime_error(std::string(action)+" (Windows error "+std::to_string(code)+").");
}
struct FileLock {
    HANDLE value=INVALID_HANDLE_VALUE;
    explicit FileLock(const fs::path& path) {
        value=CreateFileW(path.c_str(),GENERIC_READ,FILE_SHARE_READ,nullptr,OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL,nullptr);
        if(value==INVALID_HANDLE_VALUE)throw error("Cannot lock network runtime file");
    }
    ~FileLock(){if(value!=INVALID_HANDLE_VALUE)CloseHandle(value);}
    FileLock(const FileLock&)=delete;
};
inline std::string digest(HANDLE file) {
    LARGE_INTEGER zero{};if(!SetFilePointerEx(file,zero,nullptr,FILE_BEGIN))throw error("Cannot seek runtime file");
    BCRYPT_ALG_HANDLE algorithm=nullptr;BCRYPT_HASH_HANDLE hash=nullptr;std::vector<UCHAR> object;
    auto cleanup=[&]{if(hash)BCryptDestroyHash(hash);if(algorithm)BCryptCloseAlgorithmProvider(algorithm,0);};
    try {
        if(BCryptOpenAlgorithmProvider(&algorithm,BCRYPT_SHA256_ALGORITHM,nullptr,0)<0)throw error("Cannot open SHA-256");
        DWORD size=0,used=0;if(BCryptGetProperty(algorithm,BCRYPT_OBJECT_LENGTH,(PUCHAR)&size,sizeof(size),&used,0)<0)throw error("Cannot size SHA-256");
        object.resize(size);if(BCryptCreateHash(algorithm,&hash,object.data(),size,nullptr,0,0)<0)throw error("Cannot create SHA-256");
        UCHAR buffer[65536];DWORD count=0;
        do {if(!ReadFile(file,buffer,sizeof(buffer),&count,nullptr))throw error("Cannot read runtime file");
            if(count&&BCryptHashData(hash,buffer,count,0)<0)throw error("Cannot hash runtime file");
        }while(count);
        UCHAR result[32];if(BCryptFinishHash(hash,result,sizeof(result),0)<0)throw error("Cannot finish SHA-256");
        std::ostringstream out;out<<std::hex<<std::setfill('0');for(auto b:result)out<<std::setw(2)<<unsigned(b);
        cleanup();return out.str();
    }catch(...){cleanup();throw;}
}
inline fs::path executableDirectory() {
    wchar_t path[32768];DWORD n=GetModuleFileNameW(nullptr,path,32768);
    if(!n||n>=32768)throw error("Cannot locate network helper");
    return fs::path(std::wstring(path,n)).parent_path();
}
inline std::string pidFilter(const std::vector<DWORD>& pids) {
    if(pids.empty()||pids.size()>32)throw std::runtime_error("Select 1-32 process IDs.");
    std::set<DWORD> unique;
    std::string filter;
    for(DWORD pid:pids) {
        if(pid<=4||!unique.insert(pid).second)throw std::runtime_error("Process IDs must be unique and greater than 4.");
        if(!filter.empty())filter+=" or ";
        filter+="processId == "+std::to_string(pid);
    }
    return "("+filter+")";
}
class Library {
    FileLock dll,driver;
    HMODULE module=nullptr;
    template<class T>T proc(const char* name) {
        auto p=GetProcAddress(module,name);if(!p)throw error("Missing WinDivert API");
        return reinterpret_cast<T>(p);
    }
public:
    decltype(&WinDivertOpen) open=nullptr;
    decltype(&WinDivertRecv) recv=nullptr;
    decltype(&WinDivertSend) send=nullptr;
    decltype(&WinDivertShutdown) shutdown=nullptr;
    decltype(&WinDivertClose) close=nullptr;
    decltype(&WinDivertHelperParsePacket) parse=nullptr;
    decltype(&WinDivertHelperCalcChecksums) checksums=nullptr;
    explicit Library(const fs::path& directory):dll(directory/L"WinDivert.dll"),driver(directory/L"WinDivert64.sys") {
        if(digest(dll.value)!="c1e060ee19444a259b2162f8af0f3fe8c4428a1c6f694dce20de194ac8d7d9a2"||
           digest(driver.value)!="8da085332782708d8767bcace5327a6ec7283c17cfb85e40b03cd2323a90ddc2")
            throw std::runtime_error("Network runtime does not match the verified WinDivert 2.2.2-A package.");
        module=LoadLibraryExW((directory/L"WinDivert.dll").c_str(),nullptr,LOAD_LIBRARY_SEARCH_DLL_LOAD_DIR|LOAD_LIBRARY_SEARCH_SYSTEM32);
        if(!module)throw error("Cannot load verified network runtime");
        try {open=proc<decltype(open)>("WinDivertOpen");recv=proc<decltype(recv)>("WinDivertRecv");
            send=proc<decltype(send)>("WinDivertSend");shutdown=proc<decltype(shutdown)>("WinDivertShutdown");close=proc<decltype(close)>("WinDivertClose");
            parse=proc<decltype(parse)>("WinDivertHelperParsePacket");checksums=proc<decltype(checksums)>("WinDivertHelperCalcChecksums");
        }catch(...){FreeLibrary(module);module=nullptr;throw;}
    }
    ~Library(){if(module)FreeLibrary(module);}
    Library(const Library&)=delete;
};
}
#include "ProcessScope.hpp"
namespace udmnet {
struct Event {
    DWORD process=0,parentProcess=0;UINT64 endpoint=0,parentEndpoint=0,processCreated=0;
    INT64 timestamp=0;bool outbound=false,loopback=false;
    UINT8 protocol=0;UINT16 localPort=0,remotePort=0;
    UINT32 local[4]{},remote[4]{};bool ipv6=false;unsigned type=0,layer=0;
};
class ProcessWatch {
    Library& api;std::vector<HANDLE> handles;std::vector<std::thread> threads;
    ProcessScope scope;
    std::mutex mutex;std::vector<Event> pending;
    std::atomic_bool stopping{false};std::atomic<DWORD> failure{0};std::atomic<UINT64> dropped{0};
    void receive(HANDLE h) {
        while(!stopping) {
            WINDIVERT_ADDRESS a{};UINT size=0;
            if(!api.recv(h,nullptr,0,&size,&a)) {
                DWORD e=GetLastError();if(!stopping&&e!=ERROR_NO_DATA&&e!=ERROR_OPERATION_ABORTED)failure=e;
                break;
            }
            Event event{};event.layer=(unsigned)a.Layer;event.type=(unsigned)a.Event;event.ipv6=a.IPv6!=0;
            event.timestamp=a.Timestamp;event.outbound=a.Outbound!=0;event.loopback=a.Loopback!=0;
            if(a.Layer==WINDIVERT_LAYER_FLOW) {
                event.process=a.Flow.ProcessId;event.endpoint=a.Flow.EndpointId;event.parentEndpoint=a.Flow.ParentEndpointId;event.protocol=a.Flow.Protocol;
                event.localPort=a.Flow.LocalPort;event.remotePort=a.Flow.RemotePort;
                memcpy(event.local,a.Flow.LocalAddr,sizeof(event.local));memcpy(event.remote,a.Flow.RemoteAddr,sizeof(event.remote));
            }else if(a.Layer==WINDIVERT_LAYER_SOCKET) {
                event.process=a.Socket.ProcessId;event.endpoint=a.Socket.EndpointId;event.parentEndpoint=a.Socket.ParentEndpointId;event.protocol=a.Socket.Protocol;
                event.localPort=a.Socket.LocalPort;event.remotePort=a.Socket.RemotePort;
                memcpy(event.local,a.Socket.LocalAddr,sizeof(event.local));memcpy(event.remote,a.Socket.RemoteAddr,sizeof(event.remote));
            }else continue;
            if(!scope.matches(event.process))continue;
            event.processCreated=scope.creation(event.process);event.parentProcess=scope.parent(event.process);
            std::lock_guard<std::mutex> lock(mutex);
            if(pending.size()>=512){++dropped;continue;}pending.push_back(event);
        }
    }
public:
    ProcessWatch(Library& runtime,const std::vector<DWORD>& pids,bool children=false):api(runtime),scope(pids,children) {
        auto filter=children?std::string("true"):pidFilter(pids);
        try {
            for(auto layer:{WINDIVERT_LAYER_FLOW,WINDIVERT_LAYER_SOCKET}) {
                HANDLE h=api.open(filter.c_str(),layer,0,WINDIVERT_FLAG_SNIFF|WINDIVERT_FLAG_RECV_ONLY);
                if(h==INVALID_HANDLE_VALUE)throw error("Cannot start signed process monitor");
                handles.push_back(h);threads.emplace_back([this,h]{receive(h);});
            }
        }catch(...){stop();throw;}
    }
    ~ProcessWatch(){stop();}
    void stop() noexcept {
        stopping=true;
        for(auto h:handles)api.shutdown(h,WINDIVERT_SHUTDOWN_RECV);
        for(auto& t:threads)if(t.joinable())t.join();
        for(auto h:handles)api.close(h);
        handles.clear();threads.clear();
    }
    std::vector<Event> take() {std::lock_guard<std::mutex> lock(mutex);std::vector<Event> out;out.swap(pending);return out;}
    DWORD lastError() const{return failure.load();}
    UINT64 lost() const{return dropped.load();}
    UINT64 scopeLookupFailures() const{return scope.missed();}
};
// This is deliberately limited to loopback test ports. It is not a production redirector.
class LoopbackTransport {
    Library& api;HANDLE handle=INVALID_HANDLE_VALUE;std::thread worker;std::atomic_bool stopping{false};bool pass;
    std::atomic<UINT64> packets{0},bytes{0};std::atomic<DWORD> failure{0};
    unsigned original=0,target=0;
public:
    LoopbackTransport(Library& runtime,unsigned port,bool reinject,unsigned redirectPort=0):api(runtime),pass(reinject),original(port),target(redirectPort) {
        if(port<1024||port>65535)throw std::runtime_error("Loopback fixture port is outside the permitted range.");
        if(target&&(!pass||target<1024||target>65535||target==original))throw std::runtime_error("Invalid loopback redirection target.");
        auto p=std::to_string(port);
        auto filter="loopback and !impostor and (tcp or udp) and (tcp.SrcPort == "+p+" or tcp.DstPort == "+p+" or udp.SrcPort == "+p+" or udp.DstPort == "+p+")";
        if(target) {auto q=std::to_string(target);filter="loopback and !impostor and (tcp or udp) and (tcp.DstPort == "+p+" or udp.DstPort == "+p+" or tcp.SrcPort == "+q+" or udp.SrcPort == "+q+")";}
        handle=api.open(filter.c_str(),WINDIVERT_LAYER_NETWORK,0,reinject?0:WINDIVERT_FLAG_SNIFF|WINDIVERT_FLAG_RECV_ONLY);
        if(handle==INVALID_HANDLE_VALUE)throw error("Cannot start scoped loopback transport");
        try {worker=std::thread([this] {
            std::vector<unsigned char> packet(65575);
            while(!stopping) {
                WINDIVERT_ADDRESS a{};UINT n=0;
                if(!api.recv(handle,packet.data(),(UINT)packet.size(),&n,&a)) {
                    DWORD e=GetLastError();if(!stopping&&e!=ERROR_NO_DATA&&e!=ERROR_OPERATION_ABORTED)failure=e;
                    break;
                }
                ++packets;bytes+=n;
                if(target) {
                    PWINDIVERT_TCPHDR tcp=nullptr;PWINDIVERT_UDPHDR udp=nullptr;
                    if(!api.parse(packet.data(),n,nullptr,nullptr,nullptr,nullptr,nullptr,&tcp,&udp,nullptr,nullptr,nullptr,nullptr)||(!tcp&&!udp)){failure=ERROR_INVALID_DATA;break;}
                    auto source=tcp?&tcp->SrcPort:&udp->SrcPort;auto destination=tcp?&tcp->DstPort:&udp->DstPort;
                    if(ntohs(*destination)==original)*destination=htons((u_short)target);
                    else if(ntohs(*source)==target)*source=htons((u_short)original);
                    else {failure=ERROR_INVALID_DATA;break;}
                    if(!api.checksums(packet.data(),n,&a,0)){failure=ERROR_INVALID_DATA;break;}
                }
                if(pass&&!api.send(handle,packet.data(),n,nullptr,&a)){failure=GetLastError();break;}
            }
        });}catch(...){api.close(handle);handle=INVALID_HANDLE_VALUE;throw;}
    }
    ~LoopbackTransport(){stop();}
    void stop() noexcept {
        stopping=true;
        if(handle!=INVALID_HANDLE_VALUE)api.shutdown(handle,WINDIVERT_SHUTDOWN_RECV);
        if(worker.joinable())worker.join();
        if(handle!=INVALID_HANDLE_VALUE){api.close(handle);handle=INVALID_HANDLE_VALUE;}
    }
    UINT64 packetCount()const{return packets.load();}
    UINT64 packetBytes()const{return bytes.load();}
    DWORD lastError()const{return failure.load();}
};
}
