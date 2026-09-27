#include "Core.hpp"
#include <ws2tcpip.h>
#include <iphlpapi.h>
#include <tlhelp32.h>
#include <shellapi.h>
#include "../drivers/signed-network/BrokerProtocol.hpp"
namespace udm {
static std::string address(int family,const void* p,unsigned port){char out[128]{};if(!InetNtopA(family,(void*)p,out,sizeof(out)))return "Unknown";return family==AF_INET6?"["+std::string(out)+"]:"+std::to_string(port):std::string(out)+":"+std::to_string(port);}
struct Monitor::Impl {
 std::unique_ptr<udmbroker::Handle> pipe,child;
 std::vector<DWORD> roots;
 ~Impl(){stop();}
 void stop() noexcept {
  if(pipe&&child){try{rpc({{"Action","Stop"}});}catch(...){}}
  pipe.reset();
  if(child&&WaitForSingleObject(child->h,3000)==WAIT_TIMEOUT){TerminateProcess(child->h,1);WaitForSingleObject(child->h,3000);}
  child.reset();roots.clear();
 }
 Json rpc(Json request) {
  if(!pipe||!child)throw std::runtime_error("Network monitoring has not started.");
  request["Version"]=udmbroker::Version;
  udmbroker::send(pipe->h,request,3000,child->h);auto response=udmbroker::receive(pipe->h,3000,child->h);
  if(response.value("Version",0u)!=udmbroker::Version||!response.value("Ok",false))throw std::runtime_error(response.value("Error",std::string("Invalid network broker response.")));
  return response;
 }
 void start() {
  auto helper=appDir()/L"network"/L"Udm.Network.exe";
  if(!fs::is_regular_file(helper))throw std::runtime_error("The signed network helper is missing. Repair or reinstall UDM.");
  auto nonce=wide(guid());pipe=std::make_unique<udmbroker::Handle>(udmbroker::createServer(nonce));
  auto params=L"--desktop-broker "+std::to_wstring(GetCurrentProcessId())+L" "+nonce;
  SHELLEXECUTEINFOW launch{sizeof(launch)};launch.fMask=SEE_MASK_NOCLOSEPROCESS|SEE_MASK_NOASYNC|SEE_MASK_FLAG_NO_UI;
  launch.lpVerb=L"runas";launch.lpFile=helper.c_str();launch.lpParameters=params.c_str();launch.nShow=SW_HIDE;
  if(!ShellExecuteExW(&launch)){auto code=GetLastError();pipe.reset();throw std::runtime_error(code==ERROR_CANCELLED?"Network monitoring was cancelled at the administrator prompt.":"Cannot start the signed network helper (Windows error "+std::to_string(code)+").");}
  child=std::make_unique<udmbroker::Handle>(launch.hProcess);
  try {
   udmbroker::accept(pipe->h,15000,child->h);
   ULONG pid=0;if(!GetNamedPipeClientProcessId(pipe->h,&pid)||pid!=GetProcessId(child->h))throw std::runtime_error("Network helper process identity mismatch.");
   auto hello=udmbroker::receive(pipe->h,3000,child->h);
   if(hello.value("Version",0u)!=udmbroker::Version||hello.value("Nonce",std::string())!=utf8(nonce)||hello.value("ProcessId",0u)!=pid)throw std::runtime_error("Network helper authentication failed.");
  }catch(...){stop();throw;}
 }
};
Monitor::Monitor():impl(std::make_unique<Impl>()){}
Monitor::~Monitor()=default;
void Monitor::watch(const std::vector<DWORD>& pids) {
 std::set<DWORD> unique(pids.begin(),pids.end());
 if(unique.size()>32||(!unique.empty()&&*unique.begin()<=4))throw std::runtime_error("Select up to 32 live process identifiers.");
 std::vector<DWORD> requested(unique.begin(),unique.end());
 if(requested.empty()){impl->stop();return;}
 if(impl->child&&WaitForSingleObject(impl->child->h,0)!=WAIT_TIMEOUT)impl->stop();
 if(!impl->child)impl->start();
 if(requested!=impl->roots){impl->rpc({{"Action","Watch"},{"ProcessIds",requested}});impl->roots=requested;}
}
Json Monitor::snapshot(){
 if(!impl->child)return {{"Watched",0},{"Dropped",0},{"Connections",0},{"Flows",Json::array()},{"Received",nullptr},{"Sent",nullptr}};
 auto response=impl->rpc({{"Action","Snapshot"}});
 if(!response.contains("Snapshot")||!response["Snapshot"].is_object()||!response["Snapshot"].contains("Flows")||!response["Snapshot"]["Flows"].is_array()||response["Snapshot"]["Flows"].size()>512)throw std::runtime_error("Invalid network snapshot.");
 return response["Snapshot"];
}
Json diagnostics(){
 Json result={{"DriverStatus","Unavailable"},{"DriverLoadedByStatus",false},{"DesktopIntegrated",true}};
 try{Cancel cancel;auto status=Json::parse(execute(appDir()/L"network"/L"Udm.Network.exe",{L"--status"},10,cancel));
  result.update(status);result["DesktopIntegrated"]=true;result["TestSignedDriversAllowed"]=status.value("TestMode",Json(nullptr));result["MemoryIntegrityEnabled"]=status.value("MemoryIntegrity",Json(nullptr));
  result["DriverStatus"]=status.value("RuntimeVerified",false)?"Signed runtime verified; start monitoring to load the driver.":"Signed runtime verification failed.";
 }catch(const std::exception& e){result["DriverStatus"]=e.what();}
 return result;
}

Json endpoints(){std::map<DWORD,std::string> names;DWORD currentSession=0;ProcessIdToSessionId(GetCurrentProcessId(),&currentSession);Handle processes(CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS,0));PROCESSENTRY32W entry{sizeof(entry)};if(processes&&Process32FirstW(processes.h,&entry))do{auto name=lower(utf8(entry.szExeFile));DWORD session=0;if((name=="udm.exe"||name=="chrome.exe"||name=="msedge.exe"||name=="firefox.exe")&&ProcessIdToSessionId(entry.th32ProcessID,&session)&&session==currentSession)names[entry.th32ProcessID]=name;}while(Process32NextW(processes.h,&entry));Json rows=Json::array();auto add=[&](DWORD pid,std::string local,std::string remote,const char* protocol,std::string state){if(names.count(pid))rows.push_back({{"ProcessId",pid},{"Process",names[pid]},{"Local",local},{"Remote",remote},{"Transport",protocol},{"State",state},{"Received",nullptr},{"Sent",nullptr}});};for(ULONG family:{AF_INET,AF_INET6}){ULONG size=0;auto result=GetExtendedTcpTable(nullptr,&size,FALSE,family,TCP_TABLE_OWNER_PID_ALL,0);if(result!=ERROR_INSUFFICIENT_BUFFER)throw std::runtime_error("Cannot read Windows TCP endpoints.");Bytes table(size);result=GetExtendedTcpTable(table.data(),&size,FALSE,family,TCP_TABLE_OWNER_PID_ALL,0);if(result==NO_ERROR){if(family==AF_INET){auto t=(MIB_TCPTABLE_OWNER_PID*)table.data();for(DWORD i=0;i<t->dwNumEntries;++i){auto& row=t->table[i];add(row.dwOwningPid,address(AF_INET,&row.dwLocalAddr,ntohs((u_short)row.dwLocalPort)),address(AF_INET,&row.dwRemoteAddr,ntohs((u_short)row.dwRemotePort)),"TCP",row.dwState==MIB_TCP_STATE_ESTAB?"Established":row.dwState==MIB_TCP_STATE_LISTEN?"Listening":std::to_string(row.dwState));}}else{auto t=(MIB_TCP6TABLE_OWNER_PID*)table.data();for(DWORD i=0;i<t->dwNumEntries;++i){auto& row=t->table[i];add(row.dwOwningPid,address(AF_INET6,row.ucLocalAddr,ntohs((u_short)row.dwLocalPort)),address(AF_INET6,row.ucRemoteAddr,ntohs((u_short)row.dwRemotePort)),"TCP",row.dwState==MIB_TCP_STATE_ESTAB?"Established":std::to_string(row.dwState));}}}size=0;GetExtendedUdpTable(nullptr,&size,FALSE,family,UDP_TABLE_OWNER_PID,0);table.resize(size);if(GetExtendedUdpTable(table.data(),&size,FALSE,family,UDP_TABLE_OWNER_PID,0)==NO_ERROR){if(family==AF_INET){auto t=(MIB_UDPTABLE_OWNER_PID*)table.data();for(DWORD i=0;i<t->dwNumEntries;++i){auto& row=t->table[i];add(row.dwOwningPid,address(AF_INET,&row.dwLocalAddr,ntohs((u_short)row.dwLocalPort)),"","UDP","Bound");}}else{auto t=(MIB_UDP6TABLE_OWNER_PID*)table.data();for(DWORD i=0;i<t->dwNumEntries;++i){auto& row=t->table[i];add(row.dwOwningPid,address(AF_INET6,row.ucLocalAddr,ntohs((u_short)row.dwLocalPort)),"","UDP","Bound");}}}}return rows;}
}
