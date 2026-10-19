// Finite-script diagnostic. Do not run an infinite PAC script through this probe.
// Establish whether the synchronous RUN_INPROCESS option really owns evaluation.
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <winhttp.h>
#include <cstdio>
#include <string>
static unsigned long long ticks(FILETIME value){ULARGE_INTEGER n{};n.LowPart=value.dwLowDateTime;n.HighPart=value.dwHighDateTime;return n.QuadPart;}
static unsigned long long cpu(){FILETIME created{},ended{},kernel{},user{};GetProcessTimes(GetCurrentProcess(),&created,&ended,&kernel,&user);return ticks(kernel)+ticks(user);}
static std::string json(const wchar_t* text){
 if(!text)return "\"\"";int length=WideCharToMultiByte(CP_UTF8,0,text,-1,nullptr,0,nullptr,nullptr);
 std::string utf8(length,'\0');WideCharToMultiByte(CP_UTF8,0,text,-1,utf8.data(),length,nullptr,nullptr);utf8.pop_back();
 std::string out="\"";for(unsigned char c:utf8){if(c=='"'||c=='\\'){out+='\\';out+=c;}else if(c<32){char escaped[7];sprintf_s(escaped,"\\u%04x",c);out+=escaped;}else out+=c;}return out+'"';
}
int wmain(int argc,wchar_t** argv){
 if(argc!=3)return 2;
 HINTERNET session=WinHttpOpen(L"UDM finite PAC ownership probe",WINHTTP_ACCESS_TYPE_NO_PROXY,WINHTTP_NO_PROXY_NAME,WINHTTP_NO_PROXY_BYPASS,0);
 if(!session)return 3;WinHttpSetTimeouts(session,5000,5000,5000,5000);
 WINHTTP_AUTOPROXY_OPTIONS options{};options.dwFlags=WINHTTP_AUTOPROXY_CONFIG_URL|WINHTTP_AUTOPROXY_RUN_INPROCESS;options.lpszAutoConfigUrl=argv[1];
 WINHTTP_PROXY_INFO result{};const auto before=cpu(),started=GetTickCount64();
 BOOL ok=WinHttpGetProxyForUrl(session,argv[2],&options,&result);DWORD error=ok?0:GetLastError();
 printf("{\"ok\":%s,\"error\":%lu,\"wallMs\":%llu,\"cpuMs\":%llu,\"accessType\":%lu,\"proxy\":%s,\"bypass\":%s,\"jsproxyLoaded\":%s,\"jscript9Loaded\":%s}\n",
  ok?"true":"false",error,GetTickCount64()-started,(cpu()-before)/10000,result.dwAccessType,json(result.lpszProxy).c_str(),json(result.lpszProxyBypass).c_str(),GetModuleHandleW(L"jsproxy.dll")?"true":"false",GetModuleHandleW(L"jscript9.dll")?"true":"false");
 if(result.lpszProxy)GlobalFree(result.lpszProxy);if(result.lpszProxyBypass)GlobalFree(result.lpszProxyBypass);WinHttpCloseHandle(session);return ok?0:1;
}
