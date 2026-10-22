#include "Core.hpp"
#include <ws2tcpip.h>
#include <iphlpapi.h>
#include <tlhelp32.h>
#include <shellapi.h>
#include "../drivers/signed-network/BrokerProtocol.hpp"
#include "NetworkRecord.hpp"
namespace udm {
fs::path& monitorRecordPath(){static fs::path path=defaultData()/L"network-helper.json";return path;}
static uint64_t processCreated(HANDLE process){FILETIME c{},e{},k{},u{};if(!GetProcessTimes(process,&c,&e,&k,&u))return 0;ULARGE_INTEGER v{};v.LowPart=c.dwLowDateTime;v.HighPart=c.dwHighDateTime;return v.QuadPart;}
static std::string processImage(HANDLE process){wchar_t path[32768]{};DWORD size=32767;if(!QueryFullProcessImageNameW(process,0,path,&size))return{};return lower(utf8(std::wstring(path,size)));}
static std::string address(int family,const void* p,unsigned port){char out[128]{};if(!InetNtopA(family,(void*)p,out,sizeof(out)))return "Unknown";return family==AF_INET6?"["+std::string(out)+"]:"+std::to_string(port):std::string(out)+":"+std::to_string(port);}
struct Monitor::Impl {
 std::unique_ptr<udmbroker::Handle> pipe,child;
 std::vector<DWORD> roots;std::string session;uint64_t captureGeneration=0;bool responseAdmission=false,responseCredits=false,reconnectSupported=false,reconnectPermitted=false;
 ~Impl(){stop();}
 // The live helper is recorded so a later desktop process can adopt it
 // instead of launching (and elevating) another one. The nonce is the pipe
 // secret; it is protected at rest and the pipe itself admits only this user.
 void saveRecord()const{
  if(!child)return;
  Json record={{"Schema",1},{"ProtectedNonce",protect(session)},{"HelperProcessId",GetProcessId(child->h)},{"HelperCreated",processCreated(child->h)},{"HelperImage",processImage(child->h)},{"DesktopProcessId",GetCurrentProcessId()}};
  try{fs::create_directories(monitorRecordPath().parent_path());auto text=record.dump();writeBytes(monitorRecordPath(),Bytes(text.begin(),text.end()));}catch(...){}
 }
 static void removeRecord()noexcept{std::error_code ignored;fs::remove(monitorRecordPath(),ignored);}
 // Adopt the helper named by the record when it is still the same process and
 // greets this desktop as a resumed session. Anything else leaves the monitor
 // stopped; a stale record is removed, an unreadable one is left for repair.
 std::string adoptError;
 bool adopt(){
  adoptError.clear();
  if(child)return false;std::error_code ignored;if(!fs::is_regular_file(monitorRecordPath(),ignored)){adoptError="No helper record.";return false;}
  Json record;std::string nonce;uint64_t created=0;DWORD pid=0;std::string image;
  try{record=Json::parse(readText(monitorRecordPath(),65536));nonce=reveal(str(record,"ProtectedNonce"));image=str(record,"HelperImage");
   // Creation times exceed double precision, so the integers are read as stored.
   for(const char* key:{"HelperProcessId","HelperCreated","DesktopProcessId"})if(!record.contains(key)||!record[key].is_number_unsigned())throw std::runtime_error("Invalid network helper record.");
   if(record["HelperProcessId"].get<uint64_t>()>MAXDWORD)throw std::runtime_error("Invalid network helper record.");pid=record["HelperProcessId"].get<DWORD>();created=record["HelperCreated"].get<uint64_t>();
   if(num(record,"Schema")!=1||nonce.empty()||pid<=4||!created)throw std::runtime_error("Invalid network helper record.");udmbroker::pipeName(wide(nonce));}
  catch(const std::exception& e){adoptError=e.what();removeRecord();return false;}
  if(record["DesktopProcessId"].get<uint64_t>()==GetCurrentProcessId()){adoptError="The record belongs to this process.";return false;}
  auto helper=std::make_unique<udmbroker::Handle>(OpenProcess(SYNCHRONIZE|PROCESS_QUERY_LIMITED_INFORMATION,FALSE,pid));
  if(!helper->h||helper->h==INVALID_HANDLE_VALUE){adoptError="The recorded helper process cannot be opened (Windows error "+std::to_string(GetLastError())+").";helper->h=INVALID_HANDLE_VALUE;removeRecord();return false;}
  if(WaitForSingleObject(helper->h,0)!=WAIT_TIMEOUT){adoptError="The recorded helper process has exited.";removeRecord();return false;}
  if(processCreated(helper->h)!=created||processImage(helper->h)!=image){adoptError="The recorded helper identity does not match the running process (created "+std::to_string(processCreated(helper->h))+" vs "+std::to_string(created)+", image "+processImage(helper->h)+" vs "+image+").";removeRecord();return false;}
  try{
   auto wideNonce=wide(nonce);auto server=std::make_unique<udmbroker::Handle>(udmbroker::reconnectServer(wideNonce,helper->h,6000));
   auto hello=udmbroker::authenticateHelperHello(server->h,wideNonce,helper->h,15000);
   if(!hello.value("Resumed",false)||!hello.contains("ProcessIds")||!hello["ProcessIds"].is_array()||hello["ProcessIds"].size()>32||!hello.contains("CaptureGeneration")||!hello["CaptureGeneration"].is_number_unsigned())throw std::runtime_error("The network helper did not resume the session.");
   std::vector<DWORD> resumed;for(const auto& p:hello["ProcessIds"]){if(!p.is_number_unsigned()||p.get<uint64_t>()<=4||p.get<uint64_t>()>MAXDWORD)throw std::runtime_error("The network helper resumed invalid roots.");resumed.push_back(p.get<DWORD>());}
   std::sort(resumed.begin(),resumed.end());
   session=nonce;child=std::move(helper);pipe=std::move(server);roots=resumed;captureGeneration=hello["CaptureGeneration"].get<uint64_t>();responseAdmission=hello.value("ResponseAdmissionEnabled",false);
   reconnectSupported=hello.value("ReconnectSupported",false);reconnectPermitted=false;responseCredits=false;
   saveRecord();return true;
  }catch(const std::exception& e){adoptError=e.what();return false;}
 }
 void stop() noexcept {
  if(pipe&&child){try{rpc({{"Action","Stop"}});}catch(...){}}
  removeRecord();pipe.reset();
  if(child&&WaitForSingleObject(child->h,3000)==WAIT_TIMEOUT){TerminateProcess(child->h,1);WaitForSingleObject(child->h,3000);}
  child.reset();roots.clear();session.clear();captureGeneration=0;responseAdmission=false;responseCredits=false;reconnectSupported=false;reconnectPermitted=false;
 }
 void reconnect(){
  if(!reconnectPermitted||!child||session.empty()||WaitForSingleObject(child->h,0)!=WAIT_TIMEOUT)throw std::runtime_error("The network helper connection cannot be resumed.");
  reconnectPermitted=false;responseCredits=false;pipe.reset();
  pipe=std::make_unique<udmbroker::Handle>(udmbroker::reconnectServer(wide(session),child->h,6000));
  try{if(!udmbroker::authenticateHelper(pipe->h,wide(session),child->h,6000))throw std::runtime_error("The network helper no longer supports reconnection.");}
  catch(...){pipe.reset();throw;}
 }
 Json rpc(Json request) {
  if(!child)throw std::runtime_error("Network monitoring has not started.");
  if(!pipe||pipe->h==INVALID_HANDLE_VALUE)reconnect();
  request["Version"]=udmbroker::Version;Json response;
  try{response=udmbroker::exchange(*pipe,request,3000,child->h);}
  catch(const udmbroker::TransportError&){
   reconnectPermitted=reconnectSupported;responseCredits=false;
   // Only read-only snapshots may be repeated after an unknown outcome.
   if(request.value("Action",std::string())!="Snapshot"||!reconnectPermitted)throw;
   reconnect();response=udmbroker::exchange(*pipe,request,3000,child->h);
  }catch(...){reconnectPermitted=false;throw;}
  if(response.value("Version",0u)!=udmbroker::Version||!response.value("Ok",false))throw std::runtime_error(response.value("Error",std::string("Invalid network broker response.")));
  return response;
 }
 void start() {
  auto helper=appDir()/L"network"/L"Udm.Network.exe";
  if(!fs::is_regular_file(helper))throw std::runtime_error("The network helper is missing. Repair or reinstall UDM.");
  auto nonce=wide(guid());session=utf8(nonce);pipe=std::make_unique<udmbroker::Handle>(udmbroker::createServer(nonce));
  auto params=L"--desktop-broker "+std::to_wstring(GetCurrentProcessId())+L" "+nonce;
  SHELLEXECUTEINFOW launch{sizeof(launch)};launch.fMask=SEE_MASK_NOCLOSEPROCESS|SEE_MASK_NOASYNC|SEE_MASK_FLAG_NO_UI;
  launch.lpVerb=L"runas";launch.lpFile=helper.c_str();launch.lpParameters=params.c_str();launch.nShow=SW_HIDE;
  if(!ShellExecuteExW(&launch)){auto code=GetLastError();pipe.reset();throw std::runtime_error(code==ERROR_CANCELLED?"Network monitoring was cancelled at the administrator prompt.":"Cannot start the network helper (Windows error "+std::to_string(code)+").");}
  child=std::make_unique<udmbroker::Handle>(launch.hProcess);
  try {
   reconnectSupported=udmbroker::authenticateHelper(pipe->h,nonce,child->h,15000);
  }catch(...){stop();throw;}
  saveRecord();
 }
};
Monitor::Monitor():impl(std::make_unique<Impl>()){}
Monitor::~Monitor()=default;
bool Monitor::running()const noexcept{return impl->pipe&&impl->pipe->h!=INVALID_HANDLE_VALUE&&impl->child&&WaitForSingleObject(impl->child->h,0)==WAIT_TIMEOUT;}
bool Monitor::reconnectExisting(){
 if(running())return true;
 // A desktop with no helper of its own may adopt one left by a previous process.
 if(!impl->child)return impl->adopt()&&running();
 if(!impl->reconnectPermitted||WaitForSingleObject(impl->child->h,0)!=WAIT_TIMEOUT)return false;
 // Preserve the live helper, its response writers, and capture generation.
 // A failed reconnect is reported; it must not silently launch another helper.
 impl->reconnect();return running();
}
void Monitor::watch(const std::vector<DWORD>& pids){watchImpl(pids,true);}
void Monitor::watchExisting(const std::vector<DWORD>& pids){watchImpl(pids,false);}
void Monitor::watchImpl(const std::vector<DWORD>& pids,bool allowStart) {
 std::set<DWORD> unique(pids.begin(),pids.end());
 if(unique.size()>32||(!unique.empty()&&*unique.begin()<=4))throw std::runtime_error("Select up to 32 live process identifiers.");
 std::vector<DWORD> requested(unique.begin(),unique.end());
 if(requested.empty()&&allowStart){impl->stop();return;}
 if(impl->child&&WaitForSingleObject(impl->child->h,0)!=WAIT_TIMEOUT)impl->stop();
 if(impl->child&&(!impl->pipe||impl->pipe->h==INVALID_HANDLE_VALUE)){
  if(impl->reconnectPermitted)impl->reconnect();else impl->stop();
 }
 // Before reporting a stopped helper or launching one, adopt a helper that
 // outlived an earlier desktop process.
 if(!impl->child&&!requested.empty())impl->adopt();
 if(!impl->child){if(!allowStart){if(requested.empty())return;throw std::runtime_error("The network helper stopped. Choose Start signed monitor to reconnect.");}impl->start();}
 if(requested!=impl->roots){auto result=impl->rpc({{"Action","Watch"},{"ProcessIds",requested}});impl->roots=requested;impl->captureGeneration=result.value("CaptureGeneration",uint64_t(0));impl->responseAdmission=result.value("ResponseAdmissionEnabled",false);}
}
void Monitor::capture(const std::vector<unsigned>& ports){capture(ports,false);}
void Monitor::capture(const std::vector<unsigned>& ports,bool responseAdmission) {
 auto selected=udmbroker::capturePorts(Json(ports));
 if(!impl->child){if(selected.empty())return;throw std::runtime_error("Start process monitoring before HTTP capture.");}
 auto result=impl->rpc({{"Action","Capture"},{"Ports",selected},{"ResponseAdmission",responseAdmission}});
 impl->captureGeneration=result.at("CaptureGeneration").get<uint64_t>();
 impl->responseAdmission=result.value("ResponseAdmissionEnabled",false);
 if(responseAdmission&&!selected.empty()&&!impl->responseAdmission)throw std::runtime_error("The installed network helper does not support response admission. Update it before enabling handoff.");
}
static void validateAdmissionContext(const Json& capture,const std::string& session,uint64_t generation,bool enabled,uint64_t id){
 if(!enabled||!id||session.empty()||!capture.is_object()||capture.value("Session",std::string())!=session||!capture.contains("Generation")||!capture["Generation"].is_number_unsigned()||capture["Generation"].get<uint64_t>()!=generation)throw std::runtime_error("The response offer belongs to an inactive capture session.");
}
bool Monitor::admitResponse(const Json& capture,uint64_t id,HANDLE body,HANDLE receipt){return admitResponse(capture,id,body,receipt,false);}
bool Monitor::admitResponse(const Json& capture,uint64_t id,HANDLE body,HANDLE receipt,bool flowControlled){
 validateAdmissionContext(capture,impl->session,impl->captureGeneration,impl->responseAdmission,id);
 if(flowControlled&&(!impl->responseCredits||!udmbroker::responseCreditsAvailable(capture)))throw std::runtime_error("Update the network helper before enabling captured-download limits.");
 if(!body||body==INVALID_HANDLE_VALUE||!receipt||receipt==INVALID_HANDLE_VALUE||body==receipt||GetFileType(body)!=FILE_TYPE_DISK||GetFileType(receipt)!=FILE_TYPE_DISK)throw std::runtime_error("Reserve separate response body and receipt files before admission.");
 auto result=impl->rpc({{"Action","AdmitResponse"},{"Session",impl->session},{"Generation",impl->captureGeneration},{"Id",id},{"BodyHandle",static_cast<uint64_t>(reinterpret_cast<uintptr_t>(body))},{"ReceiptHandle",static_cast<uint64_t>(reinterpret_cast<uintptr_t>(receipt))},{"FlowControlled",flowControlled}});
 if(!result.contains("Accepted")||!result["Accepted"].is_boolean())throw std::runtime_error("Invalid response admission acknowledgement.");return result["Accepted"].get<bool>();
}
bool Monitor::grantResponse(const Json& capture,uint64_t id,uint64_t totalBytes){
 validateAdmissionContext(capture,impl->session,impl->captureGeneration,impl->responseAdmission,id);
 if(!impl->responseCredits)throw std::runtime_error("The connected helper has not advertised response credit support.");
 auto result=impl->rpc(udmbroker::responseGrantCommand(capture,id,totalBytes));
 if(!result.contains("Granted")||!result["Granted"].is_boolean())throw std::runtime_error("Invalid response allowance acknowledgement.");
 return result["Granted"].get<bool>();
}
bool Monitor::rejectResponse(const Json& capture,uint64_t id){
 validateAdmissionContext(capture,impl->session,impl->captureGeneration,impl->responseAdmission,id);
 auto result=impl->rpc({{"Action","RejectResponse"},{"Session",impl->session},{"Generation",impl->captureGeneration},{"Id",id}});
 if(!result.contains("Rejected")||!result["Rejected"].is_boolean())throw std::runtime_error("Invalid response rejection acknowledgement.");return result["Rejected"].get<bool>();
}
bool Monitor::cancelResponse(const Json& capture,uint64_t id){
 validateAdmissionContext(capture,impl->session,impl->captureGeneration,impl->responseAdmission,id);
 auto result=impl->rpc({{"Action","CancelResponse"},{"Session",impl->session},{"Generation",impl->captureGeneration},{"Id",id}});
 if(!result.contains("Cancelled")||!result["Cancelled"].is_boolean())throw std::runtime_error("Invalid response cancellation acknowledgement.");return result["Cancelled"].get<bool>();
}
Json Monitor::snapshot(){
 if(!impl->child)return {{"Watched",0},{"Dropped",0},{"Connections",0},{"Flows",Json::array()},{"Received",nullptr},{"Sent",nullptr}};
 impl->responseCredits=false;
 auto response=impl->rpc({{"Action","Snapshot"}});
 if(!response.contains("Snapshot"))throw std::runtime_error("Invalid network snapshot.");
 impl->responseCredits=udmbroker::snapshotResponseCredits(response["Snapshot"],impl->session);
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
