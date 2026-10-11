// Compiles the real monitor implementation; only initial process launch is injected.
#include "Network.cpp"
#include <iostream>
#include "../drivers/signed-network/DurableResponseSink.hpp"
static std::string fixtureBody(){std::string body(256*1024,'x');for(size_t i=0;i<body.size();++i)body[i]=static_cast<char>((i*17+11)%251);return body;}
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
 udmbroker::Handle owner(OpenProcess(SYNCHRONIZE|PROCESS_QUERY_LIMITED_INFORMATION|PROCESS_DUP_HANDLE,FALSE,parent));
 udmbroker::Handle pipe(udmbroker::connectOwner(nonce,owner.h,3000));
 if(mode=="legacy")udmbroker::send(pipe.h,{{"Version",udmbroker::Version},{"Nonce",udmbroker::nonceString(nonce)},{"ProcessId",GetCurrentProcessId()}});
 else udmbroker::helperHello(pipe.h,nonce,owner.h);
 unsigned snapshots=0,admissions=0;uint64_t written=0;bool completed=false;std::unique_ptr<udmnet::DurableResponseSink> sink;
 for(;;){auto request=udmbroker::receive(pipe.h,3000,owner.h,INFINITE);auto action=request.value("Action",std::string());
  if(action=="Stop"){udmbroker::send(pipe.h,{{"Version",udmbroker::Version},{"Ok",true}},3000,owner.h);return 0;}
  if(action=="GrantResponse"&&mode=="body-controlled"){
   const auto total=request.at("TotalBytes").get<uint64_t>();auto bytes=fixtureBody();auto allowed=std::min<uint64_t>(total,bytes.size());
   if(allowed>written){sink->write(std::string_view(bytes).substr(static_cast<size_t>(written),static_cast<size_t>(allowed-written)));written=allowed;}
   if(written==bytes.size()){sink->complete();sink.reset();completed=true;}
   udmbroker::send(pipe.h,{{"Version",udmbroker::Version},{"Ok",true},{"Granted",true}},3000,owner.h);continue;
  }
  if(action=="Snapshot")++snapshots;else if(action=="AdmitResponse")++admissions;else throw std::runtime_error("Unexpected monitor command");
  if(action=="AdmitResponse"&&mode.rfind("body-",0)==0){
   auto duplicate=[&](const char* key){HANDLE result=INVALID_HANDLE_VALUE;if(!DuplicateHandle(owner.h,reinterpret_cast<HANDLE>(static_cast<uintptr_t>(request.at(key).get<uint64_t>())),GetCurrentProcess(),&result,0,FALSE,DUPLICATE_SAME_ACCESS))throw udmbroker::failure("Fixture cannot receive response file");return result;};
   udmbroker::Handle body(duplicate("BodyHandle")),receipt(duplicate("ReceiptHandle"));auto b=body.h,r=receipt.h;body.h=receipt.h=INVALID_HANDLE_VALUE;
   sink=std::make_unique<udmnet::DurableResponseSink>(b,r,true,fixtureBody().size());if(mode!="body-controlled")sink->write(fixtureBody().substr(0,65536));
  }
  if(action=="Snapshot"&&sink&&mode!="body-controlled"){if(mode=="body-complete"){sink->write(fixtureBody().substr(65536));sink->complete();}sink.reset();}
  if(mode=="badreply"){udmbroker::send(pipe.h,{{"Version",udmbroker::Version+1},{"Ok",true}});return 0;}
  if(((mode=="snapshot"||mode=="partial")&&snapshots==1)||((mode=="admit"||mode.rfind("body-",0)==0)&&admissions==1&&action=="AdmitResponse")||mode=="legacy"){
   if(mode=="partial"){unsigned char first=8;udmbroker::transfer(pipe.h,&first,1,true,1000,owner.h);Sleep(3500);}
   CloseHandle(pipe.h);pipe.h=INVALID_HANDLE_VALUE;if(mode=="legacy")return 0;udmbroker::reconnectHelper(pipe,nonce,owner.h);continue;
  }
  udmbroker::Json capture={{"Session",udmbroker::nonceString(nonce)},{"Generation",uint64_t(7)},{"Observations",{{"Items",udmbroker::Json::array()}}},{"ResponseCreditsSupported",true}};
  if(mode=="body-controlled"){
   capture["ResponseAdmissionEnabled"]=true;capture["PendingResponses"]=udmbroker::Json::array();capture["Responses"]=udmbroker::Json::array();
   if(admissions)capture["Responses"].push_back({{"Id",uint64_t(1)},{"Received",written},{"State",completed?"Complete":"Receiving"},{"FlowControlled",true},{"GrantedBytes",written},{"TransferSeconds",0.1}});
  }
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
 for(const auto& mode:{"body-complete","body-interrupted"}){
  Monitor monitor;auto pid=MonitorReconnectFixture::attach(monitor,mode);auto root=fs::temp_directory_path()/wide("udm-monitor-body-"+guid());Manager manager(root);
  manager.state["Settings"]["DownloadFolder"]=utf8((root/L"downloads").wstring());manager.state["Settings"]["CategoryFolders"]=false;
  auto capture=MonitorReconnectFixture::capture(monitor);capture["ResponseAdmissionEnabled"]=true;
  Json offer={{"Id",uint64_t(1)},{"Method","GET"},{"Status",200},{"WasInterceptable",true},{"Host","example.invalid"},{"Target","/original.bin"},{"Length",static_cast<uint64_t>(fixtureBody().size())}};
  capture["PendingResponses"]=Json::array({offer});auto reservation=manager.reserveOriginalResponse(capture,offer);auto job=reservation.job;
  bool unknown=false;try{monitor.admitResponse(capture,1,reservation.body.h,reservation.receipt.h);}catch(const udmbroker::TransportError&){unknown=true;}
  check(unknown,"Lost acknowledgement leaves the real file admission outcome unknown");
  CloseHandle(reservation.body.h);reservation.body.h=INVALID_HANDLE_VALUE;CloseHandle(reservation.receipt.h);reservation.receipt.h=INVALID_HANDLE_VALUE;
  monitor.watchExisting({GetCurrentProcessId()});manager.recoverOriginalResponses();
  check(str(job->data["OriginalResponse"],"State")=="Reserved","Catalog recovery does not steal files held by the reconnected writer");
  auto snapshot=monitor.snapshot();check(snapshot.at("BrokerProcessId")==pid&&snapshot.at("AdmissionRequests")==1,"Durable response continues in the same helper with exactly one admission");
  manager.recoverOriginalResponses();const bool complete=std::string(mode)=="body-complete";const auto expected=fixtureBody().substr(0,complete?fixtureBody().size():65536);
  check(str(job->data["OriginalResponse"],"State")== (complete?"Ready":"Interrupted")&&num(job->data,"Received")==static_cast<i64>(expected.size()),"Durable receipt reports exact completion or interruption after reconnect");
  check(readText(root/L"responses"/wide(job->id())/L"body")==expected,"Response file retains exact bytes across the pipe break");
  if(complete){manager.publishOriginalResponse(job);check(str(job->data,"Status")=="Complete"&&readText(job->target())==expected&&manager.jobs.size()==1,"Recovered complete response publishes one byte-identical download");}
  else check(str(job->data,"Status")=="Paused"&&!fs::exists(job->target()),"Incomplete response stays paused with no published partial output");
  monitor.watch({});
 }
 {Monitor monitor;auto pid=MonitorReconnectFixture::attach(monitor,"body-controlled");auto root=fs::temp_directory_path()/wide("udm-monitor-controlled-"+guid());Manager manager(root);
  manager.state["Settings"]["DownloadFolder"]=utf8((root/L"downloads").wstring());manager.state["Settings"]["CategoryFolders"]=false;manager.state["Settings"]["QuotaMb"]=1;
  auto capture=monitor.snapshot().at("Capture");Json offer={{"Id",uint64_t(1)},{"Method","GET"},{"Status",200},{"WasInterceptable",true},{"Host","example.invalid"},{"Target","/controlled.bin"},{"Length",static_cast<uint64_t>(fixtureBody().size())}};capture["PendingResponses"]=Json::array({offer});
  auto first=manager.processOriginalResponses(monitor,capture,[](const Json&){return true;});check(!first.at("Errors").empty()&&manager.jobs.size()==1,"Automatic controller preserves one reserved job after a lost admission reply");
  monitor.watchExisting({GetCurrentProcessId()});auto recovered=monitor.snapshot();check(recovered.at("BrokerProcessId")==pid&&recovered["Capture"]["Responses"][0]["Received"]==0,"Controlled writer receives no bytes before a recovered credit grant");
  for(unsigned invalid=0;invalid<6;++invalid){auto wrong=recovered.at("Capture");if(invalid==0)wrong["Session"]="old-session";if(invalid==1)wrong["Generation"]=uint64_t(6);if(invalid==2)wrong["Responses"][0]["FlowControlled"]=false;if(invalid==3)wrong["Responses"][0]["Received"]=uint64_t(fixtureBody().size()+1);if(invalid==4)wrong["Responses"].push_back(wrong["Responses"][0]);if(invalid==5)wrong["Responses"][0]["Id"]=uint64_t(2);
   manager.processOriginalResponses(monitor,wrong,[](const Json&){return true;});check(str(manager.jobs.at(0)->data["OriginalResponse"],"State")=="Reserved","Stale, mismatched or ambiguous progress cannot adopt the reserved response");}
  auto result=manager.processOriginalResponses(monitor,recovered.at("Capture"),[](const Json&){return true;});auto job=manager.jobs.at(0);
  check(result.at("Errors").empty()&&result.at("Accepted")==1&&result.at("Published")==1&&str(job->data,"Status")=="Complete"&&readText(job->target())==fixtureBody(),"Automatic controller reconciles unknown admission and completes the credit-controlled response");
  check(monitor.snapshot().at("AdmissionRequests")==1&&num(manager.state,"QuotaBytes")==static_cast<i64>(fixtureBody().size()),"Reconciled response has one admission and exact quota accounting");monitor.watch({});
 }
 for(const auto& mode:{"legacy","badreply"}){Monitor monitor;MonitorReconnectFixture::attach(monitor,mode);bool rejected=false;try{monitor.snapshot();}catch(...){rejected=true;}check(rejected&&!monitor.running(),"Legacy disconnect or invalid reply fails closed without automatic reconnection");monitor.watch({});}
 std::cout<<passed<<" passed, 0 failed"<<std::endl;return 0;
 }catch(const std::exception& e){std::cerr<<"FAIL "<<e.what()<<std::endl;return 1;}}
