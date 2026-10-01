#pragma once
#include "Core.hpp"
#include <tlhelp32.h>
namespace udm {
inline std::string nativeBrowserExecutable(){
 Handle snapshot(CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS,0));if(!snapshot)return {};
 PROCESSENTRY32W entry{sizeof(entry)};DWORD parent=0;std::map<DWORD,std::pair<DWORD,std::string>> processes;
 if(Process32FirstW(snapshot.h,&entry))do{processes[entry.th32ProcessID]={entry.th32ParentProcessID,lower(utf8(entry.szExeFile))};if(entry.th32ProcessID==GetCurrentProcessId())parent=entry.th32ParentProcessID;}while(Process32NextW(snapshot.h,&entry));
 auto it=processes.find(parent);return it==processes.end()?std::string{}:it->second.second;
}
inline void restartBrowserHosts(){
 auto expected=fs::absolute(appDir()/L"Udm.NativeHost.exe").lexically_normal();Handle snapshot(CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS,0));if(!snapshot)throw std::runtime_error("Cannot inspect UDM browser connections.");
 PROCESSENTRY32W entry{sizeof(entry)};if(!Process32FirstW(snapshot.h,&entry))return;
 do{if(lower(utf8(entry.szExeFile))!="udm.nativehost.exe")continue;Handle process(OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION|PROCESS_TERMINATE,FALSE,entry.th32ProcessID));if(!process)continue;wchar_t path[32768];DWORD size=32768;if(!QueryFullProcessImageNameW(process.h,0,path,&size))continue;if(lower(utf8(fs::path(std::wstring(path,size)).lexically_normal().wstring()))!=lower(utf8(expected.wstring())))continue;if(!TerminateProcess(process.h,0))throw std::runtime_error("Windows could not restart one UDM browser connection.");}while(Process32NextW(snapshot.h,&entry));
}
}
