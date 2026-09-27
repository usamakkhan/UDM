#pragma once
#include <powrprof.h>
#include <ras.h>
#include <raserror.h>
namespace udm {
inline void performSystemAction(const std::string& action){
 if(action=="Disconnect dial-up / VPN"){
  DWORD bytes=sizeof(RASCONNW),count=0;std::vector<unsigned char> buffer(bytes);auto entry=(RASCONNW*)buffer.data();entry->dwSize=sizeof(RASCONNW);DWORD result=RasEnumConnectionsW(entry,&bytes,&count);if(result==ERROR_BUFFER_TOO_SMALL){buffer.resize(bytes);entry=(RASCONNW*)buffer.data();entry->dwSize=sizeof(RASCONNW);result=RasEnumConnectionsW(entry,&bytes,&count);}if(result)throw std::runtime_error("Cannot list dial-up/VPN connections.");for(DWORD i=0;i<count;++i)if(RasHangUpW(entry[i].hrasconn))throw std::runtime_error("Windows could not disconnect a dial-up/VPN connection.");return;
 }
 if(action=="Sleep"||action=="Hibernate"){if(!SetSuspendState(action=="Hibernate",FALSE,FALSE))throw std::runtime_error("Windows could not enter the selected power state.");return;}
 if(action!="Shut down"&&action!="Restart")return;
 Handle token;if(!OpenProcessToken(GetCurrentProcess(),TOKEN_ADJUST_PRIVILEGES|TOKEN_QUERY,&token.h))throw std::runtime_error("Windows denied the power action.");TOKEN_PRIVILEGES privileges{},previous{};privileges.PrivilegeCount=1;if(!LookupPrivilegeValueW(nullptr,SE_SHUTDOWN_NAME,&privileges.Privileges[0].Luid))throw std::runtime_error("Cannot request shutdown privilege.");privileges.Privileges[0].Attributes=SE_PRIVILEGE_ENABLED;DWORD length=sizeof(previous);SetLastError(ERROR_SUCCESS);if(!AdjustTokenPrivileges(token.h,FALSE,&privileges,sizeof(previous),&previous,&length)||GetLastError()!=ERROR_SUCCESS)throw std::runtime_error("Windows denied the shutdown privilege.");BOOL ok=ExitWindowsEx(action=="Restart"?EWX_REBOOT:EWX_POWEROFF,SHTDN_REASON_MAJOR_APPLICATION|SHTDN_REASON_FLAG_PLANNED);AdjustTokenPrivileges(token.h,FALSE,&previous,0,nullptr,nullptr);if(!ok)throw std::runtime_error("Windows could not start the selected power action.");
}
}
