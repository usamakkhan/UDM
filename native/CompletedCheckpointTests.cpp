#include "Core.hpp"
#include <iostream>
#include <future>
namespace udm {struct CatalogCheckpointTestAccess {
 static void observe(Manager& m,std::function<void(const char*)> callback){Lock lock(m.mutex);m.catalogCheckpoint=std::move(callback);}
 static void stopPeriodic(Manager& m){m.periodicCheckpoint.stop();}
 static void start(Manager& m,JobPtr job){Lock lock(m.mutex);m.start(job);}
 static std::shared_ptr<Cancel> owner(Manager& m,JobPtr job){Lock lock(m.mutex);auto i=m.active.find(job->id());return i==m.active.end()?nullptr:i->second;}
 static bool stopping(Manager& m){Lock lock(m.mutex);return m.stopping;}
};}
using namespace udm;
// In-memory HTTP input, independent of the filesystem/catalog under test.
class FinishOrigin {
 SOCKET listener=INVALID_SOCKET;std::thread thread;std::atomic_bool stopping=false,bodyReady=true;std::string payload;
 void run(){while(!stopping){auto client=accept(listener,nullptr,nullptr);if(client==INVALID_SOCKET)return;DWORD timeout=3000;setsockopt(client,SOL_SOCKET,SO_RCVTIMEO,reinterpret_cast<const char*>(&timeout),sizeof(timeout));std::string request;char buffer[4096];while(request.find("\r\n\r\n")==std::string::npos&&request.size()<32768){auto n=recv(client,buffer,sizeof(buffer),0);if(n<=0)break;request.append(buffer,n);}if(!request.empty()){const bool head=request.rfind("HEAD ",0)==0;auto response=std::string("HTTP/1.1 200 OK\r\nContent-Length: 131072\r\nContent-Type: application/octet-stream\r\nAccept-Ranges: none\r\nConnection: close\r\n\r\n");auto sendAll=[&](const std::string& bytes){size_t offset=0;while(offset<bytes.size()){auto n=send(client,bytes.data()+offset,static_cast<int>(bytes.size()-offset),0);if(n<=0)return;offset+=n;}};sendAll(response);if(!head){while(!bodyReady&&!stopping)Sleep(1);if(!stopping)sendAll(payload);}}shutdown(client,SD_BOTH);closesocket(client);}}
public:
 std::string address;void pauseBody(){bodyReady=false;}void allowBody(){bodyReady=true;}
 FinishOrigin(){try{payload.resize(131072);for(size_t i=0;i<payload.size();++i)payload[i]=static_cast<char>(i%256);listener=socket(AF_INET,SOCK_STREAM,IPPROTO_TCP);if(listener==INVALID_SOCKET)throw std::runtime_error("Cannot create owned HTTP listener");sockaddr_in endpoint{};endpoint.sin_family=AF_INET;endpoint.sin_addr.s_addr=htonl(INADDR_LOOPBACK);if(bind(listener,reinterpret_cast<sockaddr*>(&endpoint),sizeof(endpoint))||listen(listener,8))throw std::runtime_error("Cannot bind owned loopback listener");int size=sizeof(endpoint);if(getsockname(listener,reinterpret_cast<sockaddr*>(&endpoint),&size))throw std::runtime_error("Cannot read owned listener port");address="http://127.0.0.1:"+std::to_string(ntohs(endpoint.sin_port));thread=std::thread([this]{run();});}catch(...){if(listener!=INVALID_SOCKET)closesocket(listener);throw;}}
 ~FinishOrigin(){stopping=true;shutdown(listener,SD_BOTH);closesocket(listener);if(thread.joinable())thread.join();}
};
struct FinishGate {
 std::string mode;std::atomic<DWORD> worker=0;std::atomic_bool unlocked=false,held=false;
 std::promise<void> entered,released;std::shared_future<void> ready=entered.get_future().share(),wait=released.get_future().share();
 explicit FinishGate(std::string value):mode(std::move(value)){}
 void observe(const char* point){
  const std::string_view stage(point);const auto thread=GetCurrentThreadId();
  if(stage=="completed-transfer-checkpoint")worker=thread;
  const bool chosen=(mode=="snapshot"||mode=="superseded")?stage=="completed-transfer-checkpoint":stage=="temporary-flushed";
  if(thread==worker&&chosen&&!held.exchange(true)){entered.set_value();if(wait.wait_for(std::chrono::seconds(15))!=std::future_status::ready)throw std::runtime_error("Completed checkpoint fixture deadline");if(mode=="failure")throw std::runtime_error("Original completed checkpoint failure");}
 }
 void release(){if(!unlocked.exchange(true))released.set_value();}
 ~FinishGate(){release();}
};
int wmain(int argc,wchar_t** argv){
 if(argc!=2)return 2;const fs::path root=fs::absolute(argv[1]);if(fs::exists(root))return 2;fs::create_directories(root);Json checks=Json::array();std::string error,activeCase;bool passed=false;
 auto check=[&](bool value,const char* label){checks.push_back({{"name",label},{"passed",value}});if(!value)throw std::runtime_error(label);};
 try{WSADATA wsa{};if(WSAStartup(MAKEWORD(2,2),&wsa))throw std::runtime_error("Cannot initialize owned HTTP fixture");
  FinishOrigin server;const std::string origin=server.address,expected="59f410ae5e17962412e2aed4f815918f634932f2abf084f00bb638c4db017850";Url url(origin);check(url.host=="127.0.0.1"&&url.scheme=="http","Completed checkpoint fixture accepts only its owned loopback HTTP origin");
  for(const std::string mode:{"snapshot","flush","failure","superseded","stop"}){
   activeCase=mode;auto gate=std::make_shared<FinishGate>(mode);std::atomic_int published=0,finished=0;Manager manager(root/wide(mode)/L"state");
   auto prefs=manager.state["Settings"];prefs["DownloadFolder"]=utf8((root/wide(mode)/L"files").wstring());prefs["CategoryFolders"]=false;prefs["Connections"]=1;prefs["Parallel"]=1;prefs["ProxyMode"]="Connect directly";prefs["StopOnWebPage"]=false;prefs["CorrectFileExtensions"]=false;prefs["Sound"]=false;manager.setSettings(prefs);
   auto job=manager.add(origin+"/"+mode,"",mode+".udmfixture","Main queue",true);manager.resume(job);
   manager.filePublished=[&](JobPtr value){if(value==job)++published;};manager.event=[&](JobPtr value,bool complete){if(value==job&&complete)++finished;};
   std::future<void> stopped;
   struct Release {std::shared_ptr<FinishGate> gate;FinishOrigin& server;~Release(){server.allowBody();gate->release();}} release{gate,server};
   server.pauseBody();CatalogCheckpointTestAccess::start(manager,job);CatalogCheckpointTestAccess::stopPeriodic(manager);
   CatalogCheckpointTestAccess::observe(manager,[gate](const char* stage){gate->observe(stage);});server.allowBody();
   check(gate->ready.wait_for(std::chrono::seconds(15))==std::future_status::ready,"Actual Manager HTTP worker reaches the held completed-checkpoint phase");
   std::unique_lock<std::recursive_mutex> lease(manager.mutex,std::try_to_lock);check(lease.owns_lock(),"Completed checkpoint serialization boundary and durable flush do not retain the model mutex");
   check(str(job->data,"Status")=="Complete"&&manager.isActive(job)&&finished==0,"Published completion retains active ownership until checkpoint settles and emits no early Finished event");
   auto owner=CatalogCheckpointTestAccess::owner(manager,job);check(bool(owner)&&published==1,"Held checkpoint belongs to the actual published HTTP worker");
   manager.resume(job);check(CatalogCheckpointTestAccess::owner(manager,job)==owner,"Resume during a completed checkpoint cannot start a duplicate worker or replace ownership");
   bool rejected=false;try{manager.remove(job);}catch(const std::exception& e){rejected=std::string(e.what()).find("Pause the download")!=std::string::npos;}check(rejected&&manager.jobs.size()==1,"Removing a published download still waits for completed-checkpoint ownership to retire");
   check(fs::file_size(job->target())==131072&&fileHash(job->target())==expected,"The complete published HTTP file matches the independent byte count and SHA-256 before checkpoint retirement");lease.unlock();
   if(mode=="superseded"){
    {Lock lock(manager.mutex);manager.state["Settings"]["CompletedCheckpointMarker"]="newer durable owner";}manager.save();
    check(str(Json::parse(readText(manager.root/L"state.json"))["Settings"],"CompletedCheckpointMarker")=="newer durable owner","A newer forced commit can publish while an older completed snapshot is held before its writer lease");
   }
   if(mode=="stop"){
    stopped=std::async(std::launch::async,[&]{manager.stop();});auto end=GetTickCount64()+3000;while(!CatalogCheckpointTestAccess::stopping(manager)&&GetTickCount64()<end)Sleep(1);
    check(CatalogCheckpointTestAccess::stopping(manager)&&stopped.wait_for(std::chrono::seconds(0))!=std::future_status::ready,"Shutdown waits for the owned completed checkpoint instead of abandoning its worker");
    std::unique_lock<std::recursive_mutex> stopLease(manager.mutex,std::try_to_lock);check(stopLease.owns_lock(),"Shutdown join does not retain the model mutex while the completed checkpoint is held");
   }
   gate->release();auto end=GetTickCount64()+10000;while(finished==0&&GetTickCount64()<end)Sleep(1);
   check(finished==1&&!manager.isActive(job),"Checkpoint completion retires active ownership and sends exactly one final event");
   check(str(job->data,"Status")=="Complete"&&fileHash(job->target())==expected,"Checkpoint settlement or failure cannot invalidate already published file bytes");
   if(mode=="failure"){
    {Lock lock(manager.mutex);check(manager.storageError=="Original completed checkpoint failure"&&!manager.completionSaved(job),"Failed final checkpoint keeps its original storage error and cannot prove a durable current completion snapshot");}
    manager.save();check(manager.completionSaved(job),"Explicit retry durably checkpoints the completed record and clears the original storage failure");
   }else check(manager.completionSaved(job),"Successful or superseding checkpoint retains durable proof of the current completed record");
   if(mode=="superseded")check(str(Json::parse(readText(manager.root/L"state.json"))["Settings"],"CompletedCheckpointMarker")=="newer durable owner","An older completed snapshot cannot overwrite the newer committed settings");
   if(stopped.valid()){check(stopped.wait_for(std::chrono::seconds(5))==std::future_status::ready,"Shutdown finishes after the owned checkpoint is released");stopped.get();}
   CatalogCheckpointTestAccess::observe(manager,{});manager.stop();{Manager reopened(manager.root);check(reopened.jobs.size()==1&&str(reopened.jobs[0]->data,"Status")=="Complete"&&fileHash(reopened.jobs[0]->target())==expected,"Completed output and catalog survive reopening after worker retirement");}
  }
  passed=true;
 }catch(const std::exception& e){error=e.what();}
 atomicText(root/L"results.json",Json{{"passed",passed},{"error",error},{"case",activeCase},{"checks",checks}}.dump(2),false);std::cout<<checks.size()<<" checks; "<<(passed?"passed":error)<<"\n";return passed?0:1;
}
