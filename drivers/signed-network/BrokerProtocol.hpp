#pragma once
#include <windows.h>
#include <sddl.h>
#include <string>
#include <stdexcept>
#include <vector>
#include <algorithm>
#include "third_party/json.hpp"
namespace udmbroker {
using Json=nlohmann::json;
constexpr DWORD Version=1,MaxFrame=512*1024;
inline bool responseCreditsAvailable(const Json& capture){
    if(!capture.is_object())throw std::runtime_error("Invalid response capability snapshot.");
    auto it=capture.find("ResponseCreditsSupported");if(it==capture.end())return false;
    if(!it->is_boolean())throw std::runtime_error("Invalid response credit capability.");return it->get<bool>();
}
inline bool snapshotResponseCredits(const Json& snapshot,const std::string& session){
    if(!snapshot.is_object()||!snapshot.contains("Flows")||!snapshot["Flows"].is_array()||snapshot["Flows"].size()>512)throw std::runtime_error("Invalid network snapshot.");
    if(!snapshot.contains("Capture"))return false;
    const auto& capture=snapshot["Capture"];
    if(!capture.is_object()||capture.value("Session",std::string())!=session||!capture.contains("Observations")||!capture["Observations"].is_object()||!capture["Observations"].contains("Items")||!capture["Observations"]["Items"].is_array()||capture["Observations"]["Items"].size()>64)throw std::runtime_error("Invalid network capture snapshot.");
    if(capture.contains("Responses")&&(!capture["Responses"].is_array()||capture["Responses"].size()>64))throw std::runtime_error("Invalid response progress snapshot.");
    if(capture.contains("PendingResponses")&&(!capture["PendingResponses"].is_array()||capture["PendingResponses"].size()>16))throw std::runtime_error("Invalid pending response offers.");
    return responseCreditsAvailable(capture);
}
inline uint64_t responseAllowance(const Json& value){
    if(!value.is_number_unsigned()||value.get<uint64_t>()>static_cast<uint64_t>(INT64_MAX))throw std::runtime_error("Invalid response allowance.");
    return value.get<uint64_t>();
}
inline Json responseGrantCommand(const Json& capture,uint64_t id,uint64_t total){
    if(!responseCreditsAvailable(capture))throw std::runtime_error("Update the network helper before enabling captured-download limits.");
    if(!id||!capture.contains("Session")||!capture["Session"].is_string()||capture["Session"].get<std::string>().empty()||!capture.contains("Generation")||!capture["Generation"].is_number_unsigned()||capture["Generation"].get<uint64_t>()==0)throw std::runtime_error("Invalid response credit session.");
    return {{"Action","GrantResponse"},{"Session",capture["Session"]},{"Generation",capture["Generation"]},{"Id",id},{"TotalBytes",responseAllowance(Json(total))}};
}
inline DWORD processIdText(const std::string& text){
    if(text.empty()||text.size()>10||text.find_first_not_of("0123456789")!=std::string::npos)throw std::runtime_error("Invalid process identifier.");
    const auto value=std::stoull(text);if(value<=4||value>MAXDWORD)throw std::runtime_error("Invalid process identifier.");return static_cast<DWORD>(value);
}
inline std::vector<unsigned> capturePorts(const Json& ports) {
    if(!ports.is_array()||ports.size()>32)throw std::runtime_error("Select up to 32 TCP destination ports.");
    std::vector<unsigned> result;
    for(const auto& value:ports){
        if(!value.is_number_integer()||value.is_boolean()||(value.is_number_integer()&&!value.is_number_unsigned()&&value.get<int64_t>()<1))throw std::runtime_error("Invalid TCP destination port.");
        const auto port=value.get<uint64_t>();if(!port||port>65535)throw std::runtime_error("Invalid TCP destination port.");
        result.push_back(static_cast<unsigned>(port));
    }
    std::sort(result.begin(),result.end());result.erase(std::unique(result.begin(),result.end()),result.end());return result;
}
struct Handle {
    HANDLE h=INVALID_HANDLE_VALUE;
    explicit Handle(HANDLE v=INVALID_HANDLE_VALUE):h(v){}
    ~Handle(){if(h&&h!=INVALID_HANDLE_VALUE)CloseHandle(h);}
    Handle(const Handle&)=delete;Handle& operator=(const Handle&)=delete;
};
struct WindowsError:std::runtime_error{DWORD code;WindowsError(const char* message,DWORD value):std::runtime_error(std::string(message)+" (Windows error "+std::to_string(value)+")."),code(value){}};
inline WindowsError failure(const char* message){return WindowsError(message,GetLastError());}
struct TransportError:std::runtime_error{using std::runtime_error::runtime_error;};
inline void awaitIo(HANDLE pipe,OVERLAPPED& operation,BOOL immediate,DWORD& done,DWORD timeout,HANDLE peer=nullptr) {
    if(immediate)return;
    if(GetLastError()!=ERROR_IO_PENDING)throw TransportError(failure("Network broker I/O failed").what());
    HANDLE waits[]={operation.hEvent,peer};
    DWORD result=WaitForMultipleObjects(peer?2:1,waits,FALSE,timeout);
    if(result!=WAIT_OBJECT_0){CancelIoEx(pipe,&operation);GetOverlappedResult(pipe,&operation,&done,TRUE);throw TransportError(result==WAIT_OBJECT_0+1?"Network broker peer exited.":"Network broker timed out.");}
    if(!GetOverlappedResult(pipe,&operation,&done,FALSE))throw TransportError(failure("Network broker disconnected").what());
}
// Bound the whole frame, not each fragment supplied by the peer.
struct Deadline {
    ULONGLONG started=GetTickCount64();DWORD timeout;
    explicit Deadline(DWORD value):timeout(value){}
    DWORD remaining()const {
        if(timeout==INFINITE)return INFINITE;
        const auto elapsed=GetTickCount64()-started;
        if(elapsed>=timeout)throw TransportError("Network broker timed out.");
        return timeout-static_cast<DWORD>(elapsed);
    }
};
inline void transfer(HANDLE pipe,void* buffer,DWORD size,bool writing,DWORD timeout,HANDLE peer=nullptr) {
    Deadline deadline(timeout);auto p=static_cast<unsigned char*>(buffer);
    while(size){
        Handle event(CreateEventW(nullptr,TRUE,FALSE,nullptr));if(!event.h)throw failure("Cannot allocate broker event");
        OVERLAPPED op{};op.hEvent=event.h;DWORD done=0;
        const auto remaining=deadline.remaining();BOOL ok=writing?WriteFile(pipe,p,size,&done,&op):ReadFile(pipe,p,size,&done,&op);
        awaitIo(pipe,op,ok,done,remaining,peer);
        if(!done||done>size)throw std::runtime_error("Invalid network broker frame.");
        p+=done;size-=done;
    }
}
inline void send(HANDLE pipe,const Json& value,DWORD timeout=3000,HANDLE peer=nullptr) {
    auto text=value.dump();if(text.empty()||text.size()>MaxFrame)throw std::runtime_error("Network broker frame exceeds its limit.");
    Deadline deadline(timeout);DWORD size=static_cast<DWORD>(text.size());transfer(pipe,&size,sizeof(size),true,deadline.remaining(),peer);transfer(pipe,text.data(),size,true,deadline.remaining(),peer);
}
inline Json receive(HANDLE pipe,DWORD timeout=3000,HANDLE peer=nullptr,DWORD idleTimeout=0) {
    DWORD size=0;
    // Idle command waits may outlive normal I/O deadlines; partial frames may not.
    if(idleTimeout)transfer(pipe,&size,1,false,idleTimeout,peer);
    Deadline deadline(timeout);
    if(idleTimeout){transfer(pipe,reinterpret_cast<char*>(&size)+1,sizeof(size)-1,false,deadline.remaining(),peer);}
    else transfer(pipe,&size,sizeof(size),false,deadline.remaining(),peer);
    if(!size||size>MaxFrame)throw std::runtime_error("Invalid network broker frame length.");
    std::string text(size,'\0');transfer(pipe,text.data(),size,false,deadline.remaining(),peer);
    // Reject excessive JSON nesting before the parser can consume stack.
    unsigned depth=0;bool quoted=false,escaped=false;
    for(char c:text){if(quoted){if(escaped)escaped=false;else if(c=='\\')escaped=true;else if(c=='"')quoted=false;}
        else if(c=='"')quoted=true;else if(c=='['||c=='{'){if(++depth>16)throw std::runtime_error("Network broker JSON nesting limit.");}
        else if(c==']'||c=='}'){if(!depth)throw std::runtime_error("Invalid network broker JSON.");--depth;}}
    auto value=Json::parse(text);if(!value.is_object())throw std::runtime_error("Network broker expected an object.");return value;
}
// A partial or invalid reply loses framing. Never reuse that command channel.
inline Json exchange(Handle& pipe,Json request,DWORD timeout=3000,HANDLE peer=nullptr){
    if(!pipe.h||pipe.h==INVALID_HANDLE_VALUE)throw std::runtime_error("Network broker connection is closed.");
    try{
        request["Version"]=Version;send(pipe.h,request,timeout,peer);auto response=receive(pipe.h,timeout,peer);
        if(response.value("Version",0u)!=Version||!response.contains("Ok")||!response["Ok"].is_boolean())throw std::runtime_error("Invalid network broker response.");
        return response;
    }catch(...){CloseHandle(pipe.h);pipe.h=INVALID_HANDLE_VALUE;throw;}
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
    const auto error=GetLastError();LocalFree(sd);if(pipe==INVALID_HANDLE_VALUE)throw WindowsError("Cannot create network broker",error);return pipe;
}
inline void accept(HANDLE pipe,DWORD timeout,HANDLE peer) {
    Handle event(CreateEventW(nullptr,TRUE,FALSE,nullptr));if(!event.h)throw failure("Cannot allocate broker connection event");
    OVERLAPPED op{};op.hEvent=event.h;DWORD ignored=0;BOOL ok=ConnectNamedPipe(pipe,&op);
    if(!ok&&GetLastError()==ERROR_PIPE_CONNECTED)return;
    awaitIo(pipe,op,ok,ignored,timeout,peer);
}
// Reconnect only to the original, still-live process held open by the caller.
inline HANDLE connectOwner(const std::wstring& nonce,HANDLE owner,DWORD timeout){
    Deadline deadline(timeout);const auto name=pipeName(nonce);const auto parent=GetProcessId(owner);
    if(!parent)throw std::runtime_error("Network broker owner is unavailable.");
    for(;;){
        if(WaitForSingleObject(owner,0)!=WAIT_TIMEOUT)throw TransportError("Network broker peer exited.");
        deadline.remaining();
        Handle pipe(CreateFileW(name.c_str(),FILE_READ_DATA|FILE_WRITE_DATA|FILE_READ_ATTRIBUTES|FILE_WRITE_ATTRIBUTES|SYNCHRONIZE,0,nullptr,OPEN_EXISTING,FILE_FLAG_OVERLAPPED|SECURITY_SQOS_PRESENT|SECURITY_IDENTIFICATION,nullptr));
        if(pipe.h!=INVALID_HANDLE_VALUE){
            ULONG server=0;if(!GetNamedPipeServerProcessId(pipe.h,&server)||server!=parent)throw std::runtime_error("Network broker server identity mismatch.");
            auto result=pipe.h;pipe.h=INVALID_HANDLE_VALUE;return result;
        }
        const auto error=GetLastError();if(error!=ERROR_FILE_NOT_FOUND&&error!=ERROR_PIPE_BUSY)throw failure("Cannot reconnect to the desktop network broker");
        if(WaitForSingleObject(owner,std::min<DWORD>(50,deadline.remaining()))!=WAIT_TIMEOUT)throw TransportError("Network broker peer exited.");
    }
}
inline std::string nonceString(const std::wstring& nonce){pipeName(nonce);std::string text;for(auto c:nonce)text.push_back(static_cast<char>(c));return text;}
inline void helperHello(HANDLE pipe,const std::wstring& nonce,HANDLE owner){
    send(pipe,{{"Version",Version},{"Nonce",nonceString(nonce)},{"ProcessId",GetCurrentProcessId()},{"ReconnectSupported",true}},3000,owner);
}
inline bool authenticateHelper(HANDLE pipe,const std::wstring& nonce,HANDLE child,DWORD timeout){
    accept(pipe,timeout,child);ULONG pid=0;
    if(!GetNamedPipeClientProcessId(pipe,&pid)||pid!=GetProcessId(child))throw std::runtime_error("Network helper process identity mismatch.");
    auto hello=receive(pipe,3000,child);
    if(hello.value("Version",0u)!=Version||hello.value("Nonce",std::string())!=nonceString(nonce)||hello.value("ProcessId",0u)!=pid)throw std::runtime_error("Network helper authentication failed.");
    return hello.value("ReconnectSupported",false);
}
inline HANDLE reconnectServer(const std::wstring& nonce,HANDLE child,DWORD timeout){
    Deadline deadline(timeout);
    for(;;){
        if(WaitForSingleObject(child,0)!=WAIT_TIMEOUT)throw TransportError("Network broker peer exited.");
        deadline.remaining();
        try{return createServer(nonce);}catch(const WindowsError& failure){
            // The disconnected client's old handle can briefly retain the name.
            if(failure.code!=ERROR_ACCESS_DENIED&&failure.code!=ERROR_PIPE_BUSY)throw;
            if(WaitForSingleObject(child,std::min<DWORD>(50,deadline.remaining()))!=WAIT_TIMEOUT)throw TransportError("Network broker peer exited.");
        }
    }
}
inline void reconnectHelper(Handle& pipe,const std::wstring& nonce,HANDLE owner){
    if(pipe.h&&pipe.h!=INVALID_HANDLE_VALUE)CloseHandle(pipe.h);pipe.h=INVALID_HANDLE_VALUE;
    pipe.h=connectOwner(nonce,owner,10000);helperHello(pipe.h,nonce,owner);
}
}
