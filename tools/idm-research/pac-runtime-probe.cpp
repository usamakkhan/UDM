// Prototype only: embeds the engine without its OS/module library.
#define WIN32_LEAN_AND_MEAN
#include <winsock2.h>
#include <ws2tcpip.h>
#include <windows.h>
#pragma comment(lib,"ws2_32.lib")
#include "quickjs.h"
#include "../../native/PacFunctions.hpp"
#include <cstdio>
#include <fstream>
#include <iterator>
#include <string>
#include <vector>
static std::string quote(const std::string& text){std::string out="\"";for(unsigned char c:text){if(c=='"'||c=='\\'){out+='\\';out+=c;}else if(c<32){char escaped[7];sprintf_s(escaped,"\\u%04x",c);out+=escaped;}else out+=c;}return out+'"';}
struct Budget{ULONGLONG deadline;bool interrupted=false;unsigned dnsCalls=0;};
static int interrupt(JSRuntime*,void* opaque){auto& budget=*static_cast<Budget*>(opaque);if(GetTickCount64()>=budget.deadline){budget.interrupted=true;return 1;}return 0;}
static JSValue dnsResolve(JSContext* context,JSValueConst,int argc,JSValueConst* argv){
 auto& budget=*static_cast<Budget*>(JS_GetContextOpaque(context));
 if(interrupt(nullptr,&budget))return JS_ThrowInternalError(context,"PAC execution deadline reached");
 if(++budget.dnsCalls>32)return JS_ThrowInternalError(context,"PAC DNS lookup limit reached");
 if(argc<1)return JS_NULL;
 size_t length=0;const char* value=JS_ToCStringLen(context,&length,argv[0]);
 if(!value)return JS_EXCEPTION;
 std::string host(value,length);JS_FreeCString(context,value);
 if(host.empty()||host.size()>255||host.find('\0')!=std::string::npos)return JS_NULL;
 const auto count=MultiByteToWideChar(CP_UTF8,MB_ERR_INVALID_CHARS,host.data(),static_cast<int>(host.size()),nullptr,0);
 if(!count)return JS_NULL;
 std::wstring name(count,L'\0');MultiByteToWideChar(CP_UTF8,MB_ERR_INVALID_CHARS,host.data(),static_cast<int>(host.size()),name.data(),count);
 ADDRINFOW hints{};hints.ai_family=AF_INET;hints.ai_socktype=SOCK_STREAM;
 ADDRINFOW* addresses=nullptr;
 const auto status=GetAddrInfoW(name.c_str(),nullptr,&hints,&addresses);
 std::string result;
 if(!status){
  for(auto* address=addresses;address;address=address->ai_next){
   if(address->ai_family!=AF_INET||address->ai_addrlen<sizeof(sockaddr_in))continue;
   char buffer[INET_ADDRSTRLEN]{};
   if(InetNtopA(AF_INET,&reinterpret_cast<sockaddr_in*>(address->ai_addr)->sin_addr,buffer,sizeof(buffer))){result=buffer;break;}
  }
 }
 if(addresses)FreeAddrInfoW(addresses);
 if(interrupt(nullptr,&budget))return JS_ThrowInternalError(context,"PAC execution deadline reached");
 return result.empty()?JS_NULL:JS_NewString(context,result.c_str());
}
// Exercise parent-owned lifetime independently of the JavaScript interrupt.
// The child starts suspended so no script executes before job assignment.
static int supervise(const wchar_t* script,const wchar_t* timeoutText){
 wchar_t* end=nullptr;const auto timeout=wcstoul(timeoutText,&end,10);
 if(*end||timeout<1||timeout>5000)return 2;
 std::vector<wchar_t> executable(32768);const auto length=GetModuleFileNameW(nullptr,executable.data(),static_cast<DWORD>(executable.size()));
 if(!length||length>=executable.size())return 3;
 // Script paths come from the owned fixture and cannot contain a quote on Windows.
 std::wstring command=L"\""+std::wstring(executable.data())+L"\" \""+script+L"\" 2000";
 HANDLE job=CreateJobObjectW(nullptr,nullptr);if(!job)return 4;
 JOBOBJECT_EXTENDED_LIMIT_INFORMATION limits{};
 limits.BasicLimitInformation.LimitFlags=JOB_OBJECT_LIMIT_KILL_ON_JOB_CLOSE;
 if(!SetInformationJobObject(job,JobObjectExtendedLimitInformation,&limits,sizeof(limits))){CloseHandle(job);return 4;}
 STARTUPINFOW startup{};startup.cb=sizeof(startup);PROCESS_INFORMATION child{};
 if(!CreateProcessW(executable.data(),command.data(),nullptr,nullptr,FALSE,CREATE_SUSPENDED|CREATE_NO_WINDOW,nullptr,nullptr,&startup,&child)){CloseHandle(job);return 4;}
 if(!AssignProcessToJobObject(job,child.hProcess)||ResumeThread(child.hThread)==static_cast<DWORD>(-1)){
  TerminateProcess(child.hProcess,1);WaitForSingleObject(child.hProcess,5000);
  CloseHandle(child.hThread);CloseHandle(child.hProcess);CloseHandle(job);return 4;
 }
 CloseHandle(child.hThread);
 printf("{\"workerPid\":%lu,\"assignedBeforeExecution\":true}\n",child.dwProcessId);fflush(stdout);
 const auto began=GetTickCount64();const auto waited=WaitForSingleObject(child.hProcess,timeout);
 // Closing the only job handle must end even a worker blocked outside JS.
 CloseHandle(job);
 const bool exited=WaitForSingleObject(child.hProcess,5000)==WAIT_OBJECT_0;
 DWORD code=STILL_ACTIVE;GetExitCodeProcess(child.hProcess,&code);CloseHandle(child.hProcess);
 printf("{\"workerExited\":%s,\"deadlineReached\":%s,\"exitCode\":%lu,\"wallMs\":%llu}\n",exited?"true":"false",waited==WAIT_TIMEOUT?"true":"false",code,GetTickCount64()-began);
 return exited&&waited!=WAIT_FAILED?0:1;
}
int wmain(int argc,wchar_t** argv){
 if(argc==4&&std::wstring(argv[1])==L"--supervise")return supervise(argv[2],argv[3]);
 if(argc!=3)return 2;wchar_t* end=nullptr;auto limit=wcstoul(argv[2],&end,10);if(*end||limit<1||limit>2000)return 2;
 std::ifstream file(argv[1],std::ios::binary);if(!file)return 3;
 std::string script(65537,'\0');file.read(script.data(),static_cast<std::streamsize>(script.size()));
 script.resize(static_cast<size_t>(file.gcount()));if(file.bad()||script.size()>65536)return 3;
 WSADATA sockets{};if(WSAStartup(MAKEWORD(2,2),&sockets))return 4;
 struct SocketCleanup{~SocketCleanup(){WSACleanup();}} socketCleanup;
 const auto started=GetTickCount64();Budget budget{started+limit};JSRuntime* runtime=JS_NewRuntime();if(!runtime)return 4;
 JS_SetMemoryLimit(runtime,32*1024*1024);JS_SetMaxStackSize(runtime,256*1024);JS_SetInterruptHandler(runtime,interrupt,&budget);
 JSContext* context=JS_NewContext(runtime);if(!context){JS_FreeRuntime(runtime);return 4;}
 JS_SetContextOpaque(context,&budget);
 auto globals=JS_GetGlobalObject(context);
 const auto registered=JS_SetPropertyStr(context,globals,"dnsResolve",JS_NewCFunction(context,dnsResolve,"dnsResolve",1));
 JS_FreeValue(context,globals);
 if(registered<0){JS_FreeContext(context);JS_FreeRuntime(runtime);return 4;}
 bool ok=false;std::string result,error;
 auto evaluated=JS_Eval(context,udm::pacStringFunctions,sizeof(udm::pacStringFunctions)-1,"pac-helpers.js",JS_EVAL_TYPE_GLOBAL);
 if(!JS_IsException(evaluated)){
  JS_FreeValue(context,evaluated);
  evaluated=JS_Eval(context,script.data(),script.size(),"captured.pac",JS_EVAL_TYPE_GLOBAL);
 }
 if(!JS_IsException(evaluated)){
  auto global=JS_GetGlobalObject(context),function=JS_GetPropertyStr(context,global,"FindProxyForURL");
  if(JS_IsFunction(context,function)){
   JSValue args[]={JS_NewString(context,"http://udm-pac-runtime.invalid/file"),JS_NewString(context,"udm-pac-runtime.invalid")};
   auto value=JS_Call(context,function,global,2,args);
   if(JS_IsString(value)){size_t size=0;const char* text=JS_ToCStringLen(context,&size,value);if(text&&size<=8192){result.assign(text,size);ok=true;}else error="Invalid or oversized PAC result";if(text)JS_FreeCString(context,text);}
   else if(!JS_IsException(value))error="PAC result must be a string";
   JS_FreeValue(context,value);for(auto arg:args)JS_FreeValue(context,arg);
  }else error="FindProxyForURL is missing";
  JS_FreeValue(context,function);JS_FreeValue(context,global);
 }
 JS_FreeValue(context,evaluated);
 if(!ok&&error.empty()){
  auto exception=JS_GetException(context);const char* text=JS_ToCString(context,exception);error=text?text:"Script evaluation failed";if(text)JS_FreeCString(context,text);JS_FreeValue(context,exception);
 }
 JS_FreeContext(context);JS_FreeRuntime(runtime);
 printf("{\"ok\":%s,\"interrupted\":%s,\"wallMs\":%llu,\"result\":%s,\"error\":%s}\n",ok?"true":"false",budget.interrupted?"true":"false",GetTickCount64()-started,quote(result).c_str(),quote(error).c_str());
 return ok?0:1;
}
