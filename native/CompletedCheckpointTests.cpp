#include "PublishedWriteback.hpp"
#include "PreparedTransferCommit.hpp"
#include "CatalogAdmissionRegistry.hpp"
#include "FinishSettlement.hpp"
#include "Core.hpp"
#include <iostream>
#include <future>
namespace udm {struct CatalogCheckpointTestAccess {
 static void observe(Manager& m,std::function<void(const char*)> callback){Lock lock(m.mutex);m.catalogCheckpoint=std::move(callback);}
 static void stopPeriodic(Manager& m){m.periodicCheckpoint.stop();}
 static void start(Manager& m,JobPtr job){Lock lock(m.mutex);m.start(job);}
 static std::shared_ptr<Cancel> owner(Manager& m,JobPtr job){Lock lock(m.mutex);auto i=m.active.find(job->id());return i==m.active.end()?nullptr:i->second;}
 static bool stopping(Manager& m){Lock lock(m.mutex);return m.stopping;}
 static void own(Manager& m,JobPtr job,std::shared_ptr<Cancel> c){Lock lock(m.mutex);if(c)m.active[job->id()]=std::move(c);else m.active.erase(job->id());}
 static bool deferred(Manager& m){Lock lock(m.mutex);return PreparedTransferCommit::defersAutomatic(m,m.active);}
 static void automatic(Manager& m){m.checkpointPeriodic();}
 static CatalogAdmissionRegistry* registry(Manager& m){return m.admissionRegistry.get();}
};}
using namespace udm;
// In-memory HTTP input, independent of the filesystem/catalog under test.
class FinishOrigin {
 SOCKET listener=INVALID_SOCKET;std::thread thread;std::atomic_bool stopping=false,bodyReady=true,bodyRequested=false;std::string payload;
 void run(){while(!stopping){auto client=accept(listener,nullptr,nullptr);if(client==INVALID_SOCKET)return;DWORD timeout=3000;setsockopt(client,SOL_SOCKET,SO_RCVTIMEO,reinterpret_cast<const char*>(&timeout),sizeof(timeout));std::string request;char buffer[4096];while(request.find("\r\n\r\n")==std::string::npos&&request.size()<32768){auto n=recv(client,buffer,sizeof(buffer),0);if(n<=0)break;request.append(buffer,n);}if(!request.empty()){const bool head=request.rfind("HEAD ",0)==0;auto response=std::string("HTTP/1.1 200 OK\r\nContent-Length: 131072\r\nContent-Type: application/octet-stream\r\nAccept-Ranges: none\r\nConnection: close\r\n\r\n");auto sendAll=[&](const std::string& bytes){size_t offset=0;while(offset<bytes.size()){auto n=send(client,bytes.data()+offset,static_cast<int>(bytes.size()-offset),0);if(n<=0)return;offset+=n;}};sendAll(response);if(!head){bodyRequested=true;while(!bodyReady&&!stopping)Sleep(1);if(!stopping)sendAll(payload);}}shutdown(client,SD_BOTH);closesocket(client);}}
public:
 std::string address;void pauseBody(){bodyRequested=false;bodyReady=false;}void allowBody(){bodyReady=true;}bool waitingForBody()const{return bodyRequested.load();}
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
   activeCase=mode;auto gate=std::make_shared<FinishGate>(mode);std::atomic_int published=0,finished=0,workerCompleteWrites=0;auto managerOwner=std::make_unique<Manager>(root/wide(mode)/L"state");auto& manager=*managerOwner;
   auto prefs=manager.state["Settings"];prefs["DownloadFolder"]=utf8((root/wide(mode)/L"files").wstring());prefs["CategoryFolders"]=false;prefs["Connections"]=1;prefs["Parallel"]=1;prefs["ProxyMode"]="Connect directly";prefs["StopOnWebPage"]=false;prefs["CorrectFileExtensions"]=false;prefs["Sound"]=false;manager.setSettings(prefs);
   auto job=manager.add(origin+"/"+mode,"",mode+".udmfixture","Main queue",true);manager.resume(job);
   manager.filePublished=[&](JobPtr value){if(value==job)++published;};manager.event=[&](JobPtr value,bool complete){if(value==job&&complete)++finished;};
   std::future<void> stopped;
   struct Release {std::shared_ptr<FinishGate> gate;FinishOrigin& server;~Release(){server.allowBody();gate->release();}} release{gate,server};
   server.pauseBody();CatalogCheckpointTestAccess::start(manager,job);CatalogCheckpointTestAccess::stopPeriodic(manager);
   const auto bodyDeadline=GetTickCount64()+5000;while(!server.waitingForBody()&&GetTickCount64()<bodyDeadline)Sleep(1);
   check(server.waitingForBody()&&fs::is_directory(manager.root/L"publications")&&!fs::exists(manager.root/L"publications"/(wide(job->id())+L".json")),"Actual HTTP worker prepares publication storage before the first body byte without publishing a premature intent");
   CatalogCheckpointTestAccess::observe(manager,[gate,&workerCompleteWrites,&job,&manager](const char* stage){if(std::string_view(stage)=="temporary-flushed"&&GetCurrentThreadId()==gate->worker){Lock lock(manager.mutex);if(str(job->data,"Status")=="Complete")++workerCompleteWrites;}gate->observe(stage);});server.allowBody();
   check(gate->ready.wait_for(std::chrono::seconds(15))==std::future_status::ready,"Actual Manager HTTP worker reaches the held completed-checkpoint phase");
   std::unique_lock<std::recursive_mutex> lease(manager.mutex,std::try_to_lock);check(lease.owns_lock(),"Completed checkpoint serialization boundary and durable flush do not retain the model mutex");
   check(str(job->data,"Status")=="Complete"&&manager.isActive(job)&&finished==0,"Published completion retains active ownership until checkpoint settles and emits no early Finished event");
   auto owner=CatalogCheckpointTestAccess::owner(manager,job);check(bool(owner)&&published==0,"Held first completion checkpoint belongs to its actual worker and emits no early published notification");
   manager.resume(job);check(CatalogCheckpointTestAccess::owner(manager,job)==owner,"Resume during a completed checkpoint cannot start a duplicate worker or replace ownership");
   bool rejected=false;try{manager.remove(job);}catch(const std::exception& e){rejected=std::string(e.what()).find("Pause the download")!=std::string::npos;}check(rejected&&manager.jobs.size()==1,"Removing a published download still waits for completed-checkpoint ownership to retire");
   check(str(job->data["FinishPhases"],"PublishedWriteAccess")=="write-only"&&job->data["FinishPhases"].contains("PublishedFileOpen")&&job->data["FinishPhases"].contains("PublishedFileFlush")&&job->data["FinishPhases"].contains("PublishedFileClose")&&yes(job->data["FinishPhases"],"PublishedWritebackOverlap"),"Actual HTTP worker joins owned write-only body flush and close before its final checkpoint");
   check(!yes(job->data["FinishPhases"],"SettleWriterReopen",true),"An already correct actual output length skips the finish-path writer reopen");check(!yes(job->data,"SparseDataFile",true),"A sequential response does not create a sparse output requiring post-publication normalization");check(str(job->data,"ContentPrefixSource")=="write-buffer","The actual completed worker classifies its credited write prefix without a finish-path file reread");check((GetFileAttributesW(job->target().c_str())&FILE_ATTRIBUTE_SPARSE_FILE)==0,"The published sequential file is a normal allocated file, not a skipped sparse conversion");
   check(str(job->data["FinishPhases"],"CommitStrategy")=="prepared-final-checkpoint","Actual ordinary HTTP worker uses the prepared final completion checkpoint");
   const auto retained=str(job->data,"PartsFolder").empty()?manager.root/L"parts"/wide(job->id()):fs::path(wide(str(job->data,"PartsFolder")));
   check(fs::exists(retained/(wide(job->id())+L".data")),"Downloaded parts remain until the first completion checkpoint is acknowledged");
   check(str(job->data,"Sha256")==expected&&str(job->data["PublicationPhases"],"TimingScope")=="through-rename-before-final-catalog-save","The first completion snapshot already contains the final SHA and truthful bounded publication timing scope");
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
   gate->release();auto end=GetTickCount64()+10000;while((manager.isActive(job)||(mode!="failure"&&finished==0))&&GetTickCount64()<end)Sleep(1);
   check(!manager.isActive(job)&&finished==(mode=="failure"?0:1),"Checkpoint completion or failure retires ownership without a false successful event");
   check(fileHash(job->target())==expected&&str(job->data,"Status")== (mode=="failure"?"Failed":"Complete"),"First-checkpoint failure retains exact published bytes while reporting failed metadata publication");
   if(mode=="failure"){
    check(str(job->data,"Error")=="Original completed checkpoint failure"&&!manager.completionSaved(job)&&published==0,"Failed first completion checkpoint retains its original error and sends no published success");
    check(fs::exists(manager.root/L"publications"/(wide(job->id())+L".json"))&&fs::exists(retained/(wide(job->id())+L".data")),"Failed first completion checkpoint keeps the recovery intent and downloaded parts");
   }else check(manager.completionSaved(job)&&published==1,"Successful or superseding first checkpoint proves durable completion and publishes once");
   check(workerCompleteWrites==(mode=="superseded"?0:1),"Actual worker issues at most one complete-catalog durable write and does not rewrite accepted completion");
   if(mode=="superseded")check(str(Json::parse(readText(manager.root/L"state.json"))["Settings"],"CompletedCheckpointMarker")=="newer durable owner","An older completed snapshot cannot overwrite the newer committed settings");
   if(stopped.valid()){check(stopped.wait_for(std::chrono::seconds(5))==std::future_status::ready,"Shutdown finishes after the owned checkpoint is released");stopped.get();}
   CatalogCheckpointTestAccess::observe(manager,{});manager.stop();const auto savedRoot=manager.root;managerOwner.reset();
   {Manager reopened(savedRoot);check(reopened.jobs.size()==1&&str(reopened.jobs[0]->data,"Status")=="Complete"&&fileHash(reopened.jobs[0]->target())==expected,"Successful or interrupted prepared publication recovers exact output and completion on reopen");if(mode=="failure")check(!fs::exists(savedRoot/L"publications"/(wide(job->id())+L".json")),"Startup recovery acknowledges the interrupted completion and removes its journal");}

  }
  {
   Manager manager(root/L"cancel-prepared/state");auto prefs=manager.state["Settings"];prefs["DownloadFolder"]=utf8((root/L"cancel-prepared/files").wstring());prefs["CategoryFolders"]=false;prefs["Connections"]=1;prefs["ProxyMode"]="Connect directly";prefs["StopOnWebPage"]=false;prefs["CorrectFileExtensions"]=false;manager.setSettings(prefs);auto job=manager.add(origin+"/cancel-prepared","","cancel.udmfixture","Main queue",true);manager.resume(job);std::atomic_bool reached=false;std::atomic_int published=0,finished=0;manager.filePublished=[&](JobPtr){++published;};manager.event=[&](JobPtr,bool ok){if(ok)++finished;};
   server.pauseBody();CatalogCheckpointTestAccess::start(manager,job);CatalogCheckpointTestAccess::stopPeriodic(manager);
   CatalogCheckpointTestAccess::observe(manager,[&](const char* stage){if(std::string_view(stage)=="publication-preflight-begin"){reached=true;auto owner=CatalogCheckpointTestAccess::owner(manager,job);if(owner)owner->stop=true;}});server.allowBody();
   const auto end=GetTickCount64()+10000;while(manager.isActive(job)&&GetTickCount64()<end)Sleep(1);manager.stop();CatalogCheckpointTestAccess::observe(manager,{});
   check(reached&&str(job->data,"Status")=="Paused"&&!manager.isActive(job),"Actual prepared transfer honors cancellation before its durable intent");
   check(published==0&&finished==0&&!fs::exists(job->target())&&!fs::exists(manager.root/L"publications"/(wide(job->id())+L".json")),"Canceled prepared transfer sends no success and publishes neither output nor intent");
   check(!fs::exists(job->target().parent_path()/(L".udm-"+wide(job->id())+L".assembling")),"Canceled prepared handoff releases its staging name after the worker joins");
   const auto parts=str(job->data,"PartsFolder").empty()?manager.root/L"parts"/wide(job->id()):fs::path(wide(str(job->data,"PartsFolder")));check(fs::exists(parts/(wide(job->id())+L".data")),"Canceled prepared transfer preserves downloaded data for resume");
  }
  {
   activeCase="finalization-coalescing";Manager manager(root/L"finalization-coalescing");auto job=std::make_shared<Job>(Json{{"Id",guid()},{"Status","Downloading"},{"Folder",utf8(root.wstring())},{"FileName","coalesced.bin"},{"Size",4},{"Received",4}});auto token=std::make_shared<Cancel>();manager.jobs.push_back(job);CatalogCheckpointTestAccess::own(manager,job,token);manager.save();unsigned writes=0;CatalogCheckpointTestAccess::observe(manager,[&](const char* point){if(std::string_view(point)=="temporary-flushed")++writes;});
   check(!CatalogCheckpointTestAccess::deferred(manager),"An unmarked active transfer never suppresses automatic checkpoints");
   {
    PreparedTransferCommit handoff(manager,job,token,true);handoff.beginFinalization();check(CatalogCheckpointTestAccess::deferred(manager),"The exact full-body finishing owner permits automatic IO coalescing");
    manager.state["Settings"]["AutoMarker"]="deferred";CatalogCheckpointTestAccess::automatic(manager);check(writes==0,"A finishing-only automatic checkpoint performs no temporary-file write or flush");
    manager.save();check(writes==1&&str(Json::parse(readText(manager.root/L"state.json"))["Settings"],"AutoMarker")=="deferred","Explicit saves remain durable while finalization is marked");
    manager.storageError="retry required";CatalogCheckpointTestAccess::automatic(manager);check(writes==2&&manager.storageError.empty(),"Storage-error retries bypass finalization coalescing");
    auto other=std::make_shared<Job>(Json{{"Id",guid()},{"Status","Downloading"},{"Size",100},{"Received",1}});CatalogCheckpointTestAccess::own(manager,other,std::make_shared<Cancel>());check(!CatalogCheckpointTestAccess::deferred(manager),"Another active transfer prevents finishing-only suppression");
    manager.state["Settings"]["AutoMarker"]="other active";CatalogCheckpointTestAccess::automatic(manager);check(writes==3,"Unrelated active work retains its automatic durable checkpoint");CatalogCheckpointTestAccess::own(manager,other,{});
    handoff.endFinalization();check(!CatalogCheckpointTestAccess::deferred(manager),"Leaving finalization immediately releases automatic checkpoint admission");
    manager.state["Settings"]["AutoMarker"]="after finalization";CatalogCheckpointTestAccess::automatic(manager);check(writes==4,"Automatic checkpoint IO resumes after finalization ends");
   }
   {PreparedTransferCommit handoff(manager,job,token,true);handoff.beginFinalization();token->stop=true;check(!CatalogCheckpointTestAccess::deferred(manager),"Cancellation cannot retain a suppressing finalization owner");token->stop=false;}
   {PreparedTransferCommit handoff(manager,job,token,false);handoff.beginFinalization();check(!CatalogCheckpointTestAccess::deferred(manager),"Prefetch scopes cannot defer automatic checkpoints");}
   CatalogCheckpointTestAccess::own(manager,job,{});CatalogCheckpointTestAccess::observe(manager,{});manager.stop();
  }
  {
   activeCase="finalization-pending-admission";Manager manager(root/L"finalization-admission");auto job=std::make_shared<Job>(Json{{"Id",guid()},{"Status","Downloading"},{"Folder",utf8(root.wstring())},{"FileName","owned.bin"},{"Size",4},{"Received",4}});manager.jobs.push_back(job);auto token=std::make_shared<Cancel>();CatalogCheckpointTestAccess::own(manager,job,token);manager.save();unsigned writes=0;CatalogCheckpointTestAccess::observe(manager,[&](const char* point){if(std::string_view(point)=="temporary-flushed")++writes;});
   {PreparedTransferCommit handoff(manager,job,token,true);handoff.beginFinalization();auto registry=CatalogCheckpointTestAccess::registry(manager);auto lease=registry->acquire("finish-admission","finish-binding");const auto id=guid();lease.prepare(std::make_shared<const CatalogAdmissionReservation>("finish-admission",Json{{"Id",id},{"Folder",utf8(root.wstring())},{"FileName","admitted.bin"},{"Status","Awaiting confirmation"}},Json{{"protocol",3},{"status","accepted"},{"id",id},{"fingerprint","finish-binding"}}));manager.state["Settings"]["AdmissionMarker"]="pending receipt";CatalogCheckpointTestAccess::automatic(manager);check(writes==1&&lease.committed(),"Pending admission acknowledgement retains actual automatic durable IO during finalization");}
   CatalogCheckpointTestAccess::own(manager,job,{});CatalogCheckpointTestAccess::observe(manager,{});manager.stop();
  }
  {
   Manager model(root/L"settlement-lock");auto job=std::make_shared<Job>(Json{{"Id",guid()},{"Status","Downloading"},{"Folder","original"},{"PrefetchedFinalSize",131072}});Json phases;
   std::promise<void> entered,released;auto ready=entered.get_future();auto wait=released.get_future().share();
   struct ReleaseValidation {std::promise<void>& release;bool done=false;void now(){if(!done){done=true;release.set_value();}}~ReleaseValidation(){now();}} release{released};
   // Declare the future before its release guard so failure unwinding cannot
   // wait for the validator while its release is still pending.
   std::future<Json> work;
   struct Unblock {ReleaseValidation& release;~Unblock(){release.now();}} unblock{release};
   work=std::async(std::launch::async,[&]{return finishSettlementSnapshot(model,job,1.25,phases,[&](const Json& value){entered.set_value();if(wait.wait_for(std::chrono::seconds(5))!=std::future_status::ready)throw std::runtime_error("Settlement validator deadline");if(str(value,"Folder")!="original")throw std::runtime_error("Validation snapshot changed");});});
   check(ready.wait_for(std::chrono::seconds(5))==std::future_status::ready,"Settlement reaches its held filesystem validator");
   std::unique_lock<std::recursive_mutex> lease(model.mutex,std::try_to_lock);check(lease.owns_lock(),"A blocked destination validation does not retain the shared Manager mutex");
   check(str(job->data,"Status")=="Verifying"&&!job->data.contains("PrefetchedFinalSize")&&real(job->data,"TransferSeconds")==1.25,"Settlement model transition and elapsed time commit before detached validation");
   job->data["Folder"]="newer destination";lease.unlock();Sleep(20);release.now();const auto captured=work.get();
   check(str(captured,"Folder")=="original"&&str(job->data,"Folder")=="newer destination","Filesystem validation owns a stable snapshot without overwriting newer model state");
   check(real(phases,"SettleValidateFolder")>=.015&&real(phases,"SettleLockWait")>=0&&real(phases,"SettleSnapshot")>=0,"Separate settlement counters attribute an actually held validator without mislabelling it lock wait");
  }
  {
   const auto folder=root/L"writeback-operations";fs::create_directories(folder);const auto file=folder/L"actual.bin";atomicText(file,"writeback must preserve exact bytes",false);
   Json ignored;const auto actual=publishedWriteback(file);check(actual.confirmed&&readText(file)=="writeback must preserve exact bytes","Actual write-only publication flush preserves complete output bytes");
   const auto missing=publishedWriteback(folder/L"missing.bin");check(!missing.confirmed&&missing.openError&&missing.flushSeconds==0&&missing.closeSeconds==0,"Missing output reports open failure without attempting flush or close");
   struct Ops {
    std::string held,events;DWORD access=0;bool openFails=false,flushFails=false,normalizeFails=false;
    HANDLE open(const fs::path&,DWORD value){events+="open;";access=value;if(held=="open")Sleep(20);if(openFails){SetLastError(ERROR_FILE_NOT_FOUND);return INVALID_HANDLE_VALUE;}return reinterpret_cast<HANDLE>(static_cast<uintptr_t>(1));}
    bool normalize(HANDLE){events+="normalize;";SetLastError(ERROR_INVALID_FUNCTION);return !normalizeFails;}
    bool flush(HANDLE){events+="flush;";if(held=="flush")Sleep(20);SetLastError(ERROR_WRITE_FAULT);return !flushFails;}
    bool close(HANDLE){events+="close;";return true;}
   };
   for(const std::string held:{"open","flush"}){
    Ops ops;ops.held=held;const auto value=publishedWriteback(file,false,ops);
    check((held=="open"?value.openSeconds:value.flushSeconds)>=.015,"A held writeback operation is attributed to its own open or flush counter");
    check(ops.access==GENERIC_WRITE,"Publication writeback requests no unnecessary read access");
    check(value.confirmed&&ops.events=="open;flush;close;","Writeback runs each valid operation once and closes its handle before confirmation");
   }
   Ops failedFlush;failedFlush.flushFails=true;const auto flush=publishedWriteback(file,false,failedFlush);check(!flush.confirmed&&flush.flushError==ERROR_WRITE_FAULT&&failedFlush.events=="open;flush;close;","A real flush failure cannot confirm writeback and still closes its handle");
   Ops failedOpen;failedOpen.openFails=true;const auto open=publishedWriteback(file,false,failedOpen);check(!open.confirmed&&open.openError==ERROR_FILE_NOT_FOUND&&failedOpen.events=="open;","An invalid handle is never flushed or closed");
   Ops failedNormalize;failedNormalize.normalizeFails=true;const auto normalize=publishedWriteback(file,true,failedNormalize);check(!normalize.confirmed&&normalize.normalizeError==ERROR_INVALID_FUNCTION&&failedNormalize.events=="open;normalize;flush;close;","Failed sparse normalization preserves a failed confirmation while still flushing and closing valid data");
  }
  passed=true;
 }catch(const std::exception& e){error=e.what();}
 atomicText(root/L"results.json",Json{{"passed",passed},{"error",error},{"case",activeCase},{"checks",checks}}.dump(2),false);std::cout<<checks.size()<<" checks; "<<(passed?"passed":error)<<"\n";return passed?0:1;
}
