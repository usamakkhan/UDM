// Compiles the real monitor implementation; only initial process launch is injected.
#include "Network.cpp"
#include <iostream>
namespace udm {
struct MonitorReconnectFixture {
 static DWORD attach(Monitor& monitor,const std::string& mode){
  auto& state=*monitor.impl;state.session=guid();auto nonce=wide(state.session);
  state.pipe=std::make_unique<udmbroker::Handle>(udmbroker::createServer(nonce));
  wchar_t executable[32768]{};GetModuleFileNameW(nullptr,executable,32768);
  std::wstring command=L"\""+std::wstring(executable)+L"\" --peer "+std::to_wstring(GetCurrentProcessId())+L" "+nonce+L" "+wide(mode);
  STARTUPINFOW startup{sizeof(startup)};PROCESS_INFORMATION process{};
  if(!CreateProcessW(executable,command.data(),nullptr,nullptr,FALSE,CREATE_NO_WINDOW,nullptr,nullptr,&startup,&process))throw udmbroker::failure("Cannot start monitor fixture");
  CloseHandle(process.hThread);state.child=std::make_unique<udmbroker::Handle>(process.hProcess);
  state.reconnectSupported=udmbroker::authenticateHelper(state.pipe->h,nonce,state.child->h,3000);
  state.roots={GetCurrentProcessId()};state.captureGeneration=7;state.responseAdmission=true;
  return process.dwProcessId;
 }
 static Json capture(const Monitor& monitor){return {{"Session",monitor.impl->session},{"Generation",uint64_t(7)}};}
};
}
static int peer(DWORD parent,const std::wstring& nonce,const std::string& mode){
 udmbroker::Handle owner(OpenProcess(SYNCHRONIZE|PROCESS_QUERY_LIMITED_INFORMATION,FALSE,parent));
 udmbroker::Handle pipe(udmbroker::connectOwner(nonce,owner.h,3000));
 if(mode=="legacy")udmbroker::send(pipe.h,{{"Version",udmbroker::Version},{"Nonce",udmbroker::nonceString(nonce)},{"ProcessId",GetCurrentProcessId()}});
 else udmbroker::helperHello(pipe.h,nonce,owner.h);
 unsigned snapshots=0,admissions=0;
 for(;;){auto request=udmbroker::receive(pipe.h,3000,owner.h,INFINITE);auto action=request.value("Action",std::string());
  if(action=="Stop"){udmbroker::send(pipe.h,{{"Version",udmbroker::Version},{"Ok",true}},3000,owner.h);return 0;}
  if(action=="Snapshot")++snapshots;else if(action=="AdmitResponse")++admissions;else throw std::runtime_error("Unexpected monitor command");
  if(mode=="badreply"){udmbroker::send(pipe.h,{{"Version",udmbroker::Version+1},{"Ok",true}});return 0;}
  if(((mode=="snapshot"||mode=="partial")&&snapshots==1)||(mode=="admit"&&admissions==1&&action=="AdmitResponse")||mode=="legacy"){
   if(mode=="partial"){unsigned char first=8;udmbroker::transfer(pipe.h,&first,1,true,1000,owner.h);Sleep(3500);}
   CloseHandle(pipe.h);pipe.h=INVALID_HANDLE_VALUE;if(mode=="legacy")return 0;udmbroker::reconnectHelper(pipe,nonce,owner.h);continue;
  }
  udmbroker::Json capture={{"Session",udmbroker::nonceString(nonce)},{"Generation",uint64_t(7)},{"Observations",{{"Items",udmbroker::Json::array()}}},{"ResponseCreditsSupported",true}};
  udmbroker::send(pipe.h,{{"Version",udmbroker::Version},{"Ok",true},{"Snapshot",{{"Flows",udmbroker::Json::array()},{"Capture",capture},{"BrokerProcessId",GetCurrentProcessId()},{"SnapshotRequests",snapshots},{"AdmissionRequests",admissions}}}},3000,owner.h);
 }
}
int main(int argc,char** argv){using namespace udm;unsigned passed=0;try{
 if(argc==5&&std::string(argv[1])=="--peer")return peer(udmbroker::processIdText(argv[2]),wide(argv[3]),argv[4]);
 auto check=[&](bool ok,const char* label){if(!ok)throw std::runtime_error(label);++passed;std::cout<<"PASS "<<label<<std::endl;};
 for(const auto& mode:{"snapshot","partial"}){Monitor monitor;auto pid=MonitorReconnectFixture::attach(monitor,mode);auto started=GetTickCount64();auto snapshot=monitor.snapshot();
  if(std::string(mode)=="partial")check(GetTickCount64()-started>=3000&&GetTickCount64()-started<10000,"Partial reply times out and reconnects after the old client releases its pipe handle");
  check(snapshot.at("BrokerProcessId")==pid&&monitor.running(),"Actual Monitor snapshot reconnects to the same helper process after a lost reply");
  check(snapshot.at("SnapshotRequests")==2,"Read-only Snapshot is retried exactly once");
  monitor.watchExisting({GetCurrentProcessId()});check(monitor.snapshot().at("BrokerProcessId")==pid,"Subsequent polling reuses the helper without another launch");
  monitor.watch({});check(!monitor.running(),"Explicit stop closes the recovered monitor session");}
 {Monitor monitor;auto pid=MonitorReconnectFixture::attach(monitor,"admit");auto root=fs::temp_directory_path()/wide("udm-monitor-reconnect-"+guid());fs::create_directories(root);
  {Handle body(CreateFileW((root/L"body").c_str(),GENERIC_READ|GENERIC_WRITE,0,nullptr,CREATE_NEW,FILE_ATTRIBUTE_TEMPORARY|FILE_FLAG_DELETE_ON_CLOSE,nullptr));
   Handle receipt(CreateFileW((root/L"receipt").c_str(),GENERIC_READ|GENERIC_WRITE,0,nullptr,CREATE_NEW,FILE_ATTRIBUTE_TEMPORARY|FILE_FLAG_DELETE_ON_CLOSE,nullptr));
   bool rejected=false;try{monitor.admitResponse(MonitorReconnectFixture::capture(monitor),1,body.h,receipt.h);}catch(const udmbroker::TransportError&){rejected=true;}
   check(rejected,"Lost admission acknowledgement reports an unknown outcome instead of replaying the admission");
   monitor.watchExisting({GetCurrentProcessId()});auto snapshot=monitor.snapshot();
   check(snapshot.at("BrokerProcessId")==pid&&snapshot.at("AdmissionRequests")==1,"Next poll reconnects while preserving a single admission in the original helper");
   check(snapshot["Capture"]["Generation"]==7,"Capture generation survives monitor reconnection");monitor.watch({});}
  fs::remove(root);}
 for(const auto& mode:{"legacy","badreply"}){Monitor monitor;MonitorReconnectFixture::attach(monitor,mode);bool rejected=false;try{monitor.snapshot();}catch(...){rejected=true;}check(rejected&&!monitor.running(),"Legacy disconnect or invalid reply fails closed without automatic reconnection");monitor.watch({});}
 std::cout<<passed<<" passed, 0 failed"<<std::endl;return 0;
 }catch(const std::exception& e){std::cerr<<"FAIL "<<e.what()<<std::endl;return 1;}}
