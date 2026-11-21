#include "Core.hpp"
#include "OptionsModel.hpp"
#include "Catalog.hpp"
#include "QueueWake.hpp"
#include <ws2tcpip.h>
#include <iostream>
namespace udm {struct CatalogCheckpointTestAccess {
 static void observe(Manager& m,std::function<void(const char*)> fn){Lock lock(m.mutex);m.catalogCheckpoint=std::move(fn);}
 static std::set<std::string> paused(Manager& m){Lock lock(m.mutex);return m.schedulePaused;}
 static void seedPause(Manager& m,const std::string& id){Lock lock(m.mutex);m.schedulePaused.insert(id);}
 static void dropWakeTimer(Manager& m){Lock lock(m.mutex);m.wakeTimer.reset();}
 static bool applied(Manager& m){Lock lock(m.mutex);return m.startupQueuesApplied;}
 static void active(Manager& m,const JobPtr& job,const std::shared_ptr<Cancel>& token){Lock lock(m.mutex);if(token)m.active[job->id()]=token;else m.active.erase(job->id());}
};}
using namespace udm;
static Json checks=Json::array();
static void check(bool value,const std::string& name){checks.push_back({{"name",name},{"passed",value}});if(!value)throw std::runtime_error(name);}
template<class F>static void rejects(F work,const std::string& name){bool refused=false;try{work();}catch(...){refused=true;}check(refused,name);}
// Adapter for the existing startup/export checks: a bounded owned HTTP origin.
class Fixture {
 SOCKET listener=INVALID_SOCKET;std::thread worker;std::vector<std::thread> clients;std::atomic_bool stopping=false;unsigned short port=0;
public:
 const std::string payload="owned startup queue payload";std::atomic_uint requests=0;
 Fixture(){listener=socket(AF_INET,SOCK_STREAM,IPPROTO_TCP);if(listener==INVALID_SOCKET)throw std::runtime_error("Cannot create fixture socket");sockaddr_in address{};address.sin_family=AF_INET;address.sin_addr.s_addr=htonl(INADDR_LOOPBACK);if(bind(listener,reinterpret_cast<sockaddr*>(&address),sizeof(address))||listen(listener,16))throw std::runtime_error("Cannot bind fixture");int length=sizeof(address);getsockname(listener,reinterpret_cast<sockaddr*>(&address),&length);port=ntohs(address.sin_port);
  worker=std::thread([this]{while(!stopping){auto client=accept(listener,nullptr,nullptr);if(client==INVALID_SOCKET)break;clients.emplace_back([this,client]{DWORD timeout=2000;setsockopt(client,SOL_SOCKET,SO_RCVTIMEO,reinterpret_cast<const char*>(&timeout),sizeof(timeout));std::string request;char buffer[4096];while(request.find("\r\n\r\n")==std::string::npos&&request.size()<32768){const auto got=recv(client,buffer,sizeof(buffer),0);if(got<=0)break;request.append(buffer,got);}++requests;const auto reply="HTTP/1.1 200 OK\r\nContent-Length: "+std::to_string(payload.size())+"\r\nContent-Type: application/octet-stream\r\nAccept-Ranges: none\r\nConnection: close\r\n\r\n"+payload;for(size_t at=0;at<reply.size();){const auto sent=send(client,reply.data()+at,static_cast<int>(reply.size()-at),0);if(sent<=0)break;at+=sent;}shutdown(client,SD_BOTH);closesocket(client);});}});
 }
 ~Fixture(){stopping=true;shutdown(listener,SD_BOTH);closesocket(listener);if(worker.joinable())worker.join();for(auto& client:clients)if(client.joinable())client.join();}
 std::string url(const std::string& path)const{return "http://127.0.0.1:"+std::to_string(port)+path;}
};
#include "StartupExportChecks.hpp"
#include "WakeChecks.hpp"
static JobPtr row(const fs::path& folder,const std::string& queue,const std::string& name){return std::make_shared<Job>(Json{{"Id",guid()},{"Url","http://127.0.0.1:9/not-started"},{"Folder",utf8(folder.wstring())},{"FileName",name},{"Queue",queue},{"Status","Paused"},{"QueueMember",true},{"Size",-1},{"Received",0},{"Segments",Json::array()}});}
int wmain(int argc,wchar_t** argv){
 if(argc!=2)return 2;const auto root=fs::absolute(argv[1]);if(fs::exists(root))return 2;fs::create_directories(root);WSADATA sockets{};if(WSAStartup(MAKEWORD(2,2),&sockets))return 3;CoInitializeEx(nullptr,COINIT_MULTITHREADED);
 Json cases=Json::array();std::string error,active;
 try{
  for(const std::string mode:{"startup","start","stop"})for(const std::string fault:{"none","actual-write-failure","flushed-failure","unrelated-current"}){
   active=mode+"/"+fault;const auto folder=root/wide(mode+"-"+fault);Manager m(folder/L"catalog");const auto files=folder/L"files";fs::create_directories(files);
   auto queue=defaultQueue();queue["Enabled"]=false;queue["StartOnStartup"]=true;m.setQueue(queue);auto archive=defaultQueue("Archive");archive["Enabled"]=false;m.setQueue(archive);
   auto a=row(files,"Main queue","a.bin"),b=row(files,"Main queue","b.bin"),other=row(files,"Archive","other.bin");other->data["Description"]="original unrelated metadata";
   auto passive=row(files,"Main queue","history.bin");passive->data["Status"]="Complete";passive->data["QueueMember"]=false;passive->data["Description"]="original passive metadata";const auto passiveBefore=passive->data;
   m.jobs={a,other,passive,b};if(mode!="stop")a->prefetchDeferring=true;CatalogCheckpointTestAccess::seedPause(m,other->id());m.save();
   const auto beforeA=a->data,beforeB=b->data,beforeOther=other->data,beforeQueues=m.state["Queues"];const auto beforePause=CatalogCheckpointTestAccess::paused(m);
   auto token=std::make_shared<Cancel>();if(mode=="stop")CatalogCheckpointTestAccess::active(m,b,token);
   const auto temp=m.root/L"state.json.tmp";if(fault=="actual-write-failure")fs::create_directory(temp);
   CatalogCheckpointTestAccess::observe(m,[&](const char* point){if(fault!="none"&&fault!="actual-write-failure"&&std::string_view(point)=="temporary-flushed"){if(fault=="unrelated-current"){other->data["Description"]="new unrelated metadata";passive->data["Description"]="new passive metadata";m.state["Settings"]["QueueSnapshotMarker"]="current";}throw std::runtime_error("Owned queue checkpoint failure");}});
   bool refused=false;try{if(mode=="startup")m.startQueuesOnStartup();else m.queueRun("Main queue",mode=="start");}catch(...){refused=true;}CatalogCheckpointTestAccess::observe(m,{});if(fault=="actual-write-failure")fs::remove(temp);
   check(refused==(fault!="none"),"Queue preparation reports actual and observed persistence refusal");
   if(refused){
    check(a->data==beforeA&&b->data==beforeB&&m.state["Queues"]==beforeQueues,"Failed queue preparation restores only its affected job and queue state");
    check(CatalogCheckpointTestAccess::paused(m)==beforePause,"Failed startup/start/stop restores the exact transient pause set");
    check(!token->stop,"Failed Stop never cancels a live worker before persistence");
    check(fault=="unrelated-current"?str(other->data,"Description")=="new unrelated metadata":other->data==beforeOther,"Rollback preserves independent unrelated current metadata");
    if(fault=="unrelated-current")check(str(m.state["Settings"],"QueueSnapshotMarker")=="current","Rollback preserves unrelated current settings");
    if(mode=="startup"){check(!CatalogCheckpointTestAccess::applied(m),"Failed startup remains eligible for explicit retry");m.startQueuesOnStartup();check(CatalogCheckpointTestAccess::applied(m)&&str(a->data,"Status")=="Queued","Healthy retry prepares startup members after exact rollback");}
   }else{
    check(other->data==beforeOther,"Successful queue preparation leaves unrelated history untouched");
    check(mode=="stop"?token->stop.load():(str(a->data,"Status")=="Queued"&&str(b->data,"Status")=="Queued"),"Only accepted start/stop changes admission or worker cancellation");
    const auto saved=Json::parse(readText(m.root/L"state.json"));check(saved["Queues"]==m.state["Queues"],"Accepted queue settings are actually persisted");
   }
   check(fault=="unrelated-current"?str(passive->data,"Description")=="new passive metadata":passive->data==passiveBefore,"Queue rollback preserves passive same-label history and its independent metadata");
   a->prefetchDeferring=false;CatalogCheckpointTestAccess::active(m,b,{});m.stop();cases.push_back({{"mode",mode},{"fault",fault},{"refused",refused}});
  }
  for(const bool fail:{false,true}){
   active=fail?"passive-review-refusal":"passive-review-acceptance";Manager m(root/wide(active));auto job=row(root/L"passive-review/files","Main queue","history.bin");job->data["Status"]="Complete";job->data["QueueMember"]=false;job->data["ServerRetryRequiresReview"]=true;m.jobs.push_back(job);m.save();const auto before=job->data;const auto blocked=m.root/L"state.json.tmp";if(fail)fs::create_directory(blocked);bool refused=false;try{m.queueRun("Main queue",true);}catch(...){refused=true;}if(fail)fs::remove(blocked);
   check(refused==fail,"Review clearing reports an actual catalog persistence refusal");check(fail?job->data==before:!job->data.contains("ServerRetryRequiresReview"),"Start snapshots a passive row when it really clears its retry-review field");m.stop();
  }
  {
   active="no-opt-in";Manager m(root/L"no-opt-in/catalog");auto queue=m.state["Queues"][0];queue["Enabled"]=false;queue["StartOnStartup"]=false;m.setQueue(queue);auto job=row(root/L"no-opt-in/files","Main queue","pending.bin");m.jobs.push_back(job);m.save();const auto before=m.snapshot();const auto beforeHash=fileHash(m.root/L"state.json");const auto blocked=m.root/L"state.json.tmp";fs::create_directory(blocked);m.startQueuesOnStartup();check(m.snapshot()==before&&fileHash(m.root/L"state.json")==beforeHash,"No opted-in queue performs no catalog write or record mutation");check(CatalogCheckpointTestAccess::applied(m),"No-op startup still consumes the session's once-only trigger");fs::remove(blocked);
   queue["StartOnStartup"]=true;m.setQueue(queue);const auto later=m.snapshot();m.startQueuesOnStartup();check(m.snapshot()==later&&str(job->data,"Status")=="Paused","Enabling startup preference later does not run it in the current session");m.stop();
  }
  {
   active="wake-without-startup-opt-in";Manager m(root/L"wake-only/catalog");auto job=row(root/L"wake-only/files","Main queue","waiting.bin");m.jobs.push_back(job);auto queue=defaultQueue();queue["WakeComputer"]=true;queue["RunOnce"]=true;queue["StartOnceUtc"]=date(epoch()+120000);queue["StartOnStartup"]=false;m.setQueue(queue);CatalogCheckpointTestAccess::dropWakeTimer(m);m.startQueuesOnStartup();
   check(str(m.queueWakeStatus(),"Status")=="Armed"||str(m.queueWakeStatus(),"Status")=="Unsupported","No-op queue startup still initializes an eligible scheduled wake timer");m.stop();check(str(m.queueWakeStatus(),"Status")=="Off","The owned startup wake timer is canceled on shutdown");
  }
  {active="existing-wake";wakeChecks(root/L"existing-wake");}
  {active="existing-startup-export";Fixture fixture;startupExportChecks(root/L"existing",fixture);cases.push_back({{"mode","existing-startup-export"},{"requests",fixture.requests.load()}});}
 }catch(const std::exception& failure){error=failure.what();}
 atomicText(root/L"results.json",Json{{"passed",error.empty()},{"error",error},{"case",active},{"checks",checks},{"cases",cases},{"systemActionsExecuted",false}}.dump(2),false);CoUninitialize();WSACleanup();std::cout<<checks.size()<<" checks; "<<(error.empty()?"passed":error)<<"\n";return error.empty()?0:1;
}
