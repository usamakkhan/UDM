#pragma once
#include <windows.h>
#include <sddl.h>
#include <string>
#include <stdexcept>
#include <vector>
#include "third_party/json.hpp"
namespace udmbroker {
using Json=nlohmann::json;
constexpr DWORD Version=1,MaxFrame=512*1024;
struct Handle {
    HANDLE h=INVALID_HANDLE_VALUE;
    explicit Handle(HANDLE v=INVALID_HANDLE_VALUE):h(v){}
    ~Handle(){if(h&&h!=INVALID_HANDLE_VALUE)CloseHandle(h);}
    Handle(const Handle&)=delete;Handle& operator=(const Handle&)=delete;
};
inline std::runtime_error failure(const char* message){return std::runtime_error(std::string(message)+" (Windows error "+std::to_string(GetLastError())+").");}
inline void awaitIo(HANDLE pipe,OVERLAPPED& operation,BOOL immediate,DWORD& done,DWORD timeout,HANDLE peer=nullptr) {
    if(immediate)return;
    if(GetLastError()!=ERROR_IO_PENDING)throw failure("Network broker I/O failed");
    HANDLE waits[]={operation.hEvent,peer};
    DWORD result=WaitForMultipleObjects(peer?2:1,waits,FALSE,timeout);
    if(result!=WAIT_OBJECT_0){CancelIoEx(pipe,&operation);GetOverlappedResult(pipe,&operation,&done,TRUE);throw std::runtime_error(result==WAIT_OBJECT_0+1?"Network broker peer exited.":"Network broker timed out.");}
    if(!GetOverlappedResult(pipe,&operation,&done,FALSE))throw failure("Network broker disconnected");
}
inline void transfer(HANDLE pipe,void* buffer,DWORD size,bool writing,DWORD timeout,HANDLE peer=nullptr) {
    auto p=static_cast<unsigned char*>(buffer);
    while(size){
        Handle event(CreateEventW(nullptr,TRUE,FALSE,nullptr));if(!event.h)throw failure("Cannot allocate broker event");
        OVERLAPPED op{};op.hEvent=event.h;DWORD done=0;
        BOOL ok=writing?WriteFile(pipe,p,size,&done,&op):ReadFile(pipe,p,size,&done,&op);
        awaitIo(pipe,op,ok,done,timeout,peer);
        if(!done||done>size)throw std::runtime_error("Invalid network broker frame.");
        p+=done;size-=done;
    }
}
inline void send(HANDLE pipe,const Json& value,DWORD timeout=3000,HANDLE peer=nullptr) {
    auto text=value.dump();if(text.empty()||text.size()>MaxFrame)throw std::runtime_error("Network broker frame exceeds its limit.");
    DWORD size=static_cast<DWORD>(text.size());transfer(pipe,&size,sizeof(size),true,timeout,peer);transfer(pipe,text.data(),size,true,timeout,peer);
}
inline Json receive(HANDLE pipe,DWORD timeout=3000,HANDLE peer=nullptr) {
    DWORD size=0;transfer(pipe,&size,sizeof(size),false,timeout,peer);
    if(!size||size>MaxFrame)throw std::runtime_error("Invalid network broker frame length.");
    std::string text(size,'\0');transfer(pipe,text.data(),size,false,timeout,peer);
    // Reject excessive JSON nesting before the parser can consume stack.
    unsigned depth=0;bool quoted=false,escaped=false;
    for(char c:text){if(quoted){if(escaped)escaped=false;else if(c=='\\')escaped=true;else if(c=='"')quoted=false;}
        else if(c=='"')quoted=true;else if(c=='['||c=='{'){if(++depth>16)throw std::runtime_error("Network broker JSON nesting limit.");}
        else if(c==']'||c=='}'){if(!depth)throw std::runtime_error("Invalid network broker JSON.");--depth;}}
    auto value=Json::parse(text);if(!value.is_object())throw std::runtime_error("Network broker expected an object.");return value;
}
inline std::wstring pipeName(const std::wstring& nonce) {
    if(nonce.size()!=32||nonce.find_first_not_of(L"0123456789abcdefABCDEF")!=std::wstring::npos)throw std::runtime_error("Invalid network broker nonce.");
    return L"\\\\.\\pipe\\UDM.Network."+nonce;
}
inline HANDLE createServer(const std::wstring& nonce) {
    Handle token; if(!OpenProcessToken(GetCurrentProcess(),TOKEN_QUERY,&token.h))throw failure("Cannot read broker identity");
    DWORD size=0;GetTokenInformation(token.h,TokenUser,nullptr,0,&size);std::vector<unsigned char> bytes(size);
    if(!GetTokenInformation(token.h,TokenUser,bytes.data(),size,&size))throw failure("Cannot read broker identity");
    LPWSTR sid=nullptr;if(!ConvertSidToStringSidW(reinterpret_cast<TOKEN_USER*>(bytes.data())->User.Sid,&sid))throw failure("Cannot encode broker identity");
    // Explicit read/write rights exclude FILE_CREATE_PIPE_INSTANCE. No Everyone/default DACL.
    std::wstring acl=L"D:P(A;;GA;;;SY)(A;;0x00120183;;;"+std::wstring(sid)+L")";LocalFree(sid);
    PSECURITY_DESCRIPTOR sd=nullptr;if(!ConvertStringSecurityDescriptorToSecurityDescriptorW(acl.c_str(),SDDL_REVISION_1,&sd,nullptr))throw failure("Cannot secure network broker");
    SECURITY_ATTRIBUTES sa{sizeof(sa),sd,FALSE};
    HANDLE pipe=CreateNamedPipeW(pipeName(nonce).c_str(),PIPE_ACCESS_DUPLEX|FILE_FLAG_FIRST_PIPE_INSTANCE|FILE_FLAG_OVERLAPPED,
        PIPE_TYPE_BYTE|PIPE_READMODE_BYTE|PIPE_WAIT|PIPE_REJECT_REMOTE_CLIENTS,1,MaxFrame,MaxFrame,0,&sa);
    LocalFree(sd);if(pipe==INVALID_HANDLE_VALUE)throw failure("Cannot create network broker");return pipe;
}
inline void accept(HANDLE pipe,DWORD timeout,HANDLE peer) {
    Handle event(CreateEventW(nullptr,TRUE,FALSE,nullptr));if(!event.h)throw failure("Cannot allocate broker connection event");
    OVERLAPPED op{};op.hEvent=event.h;DWORD ignored=0;BOOL ok=ConnectNamedPipe(pipe,&op);
    if(!ok&&GetLastError()==ERROR_PIPE_CONNECTED)return;
    awaitIo(pipe,op,ok,ignored,timeout,peer);
}
}

