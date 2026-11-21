#include "Core.hpp"
#include <future>
#include <iostream>
namespace udm {
struct CatalogCheckpointTestAccess {
 static void observe(Manager& m,std::function<void(const char*)> f){Lock lock(m.mutex);m.catalogCheckpoint=std::move(f);}
 static bool cancelled(Manager& m,const JobPtr& j){Lock lock(m.mutex);auto it=m.active.find(j->id());return it==m.active.end()||it->second->cancelled();}
 static void stopAutomatic(Manager& m){m.periodicCheckpoint.stop();}
};
}
using namespace udm;
namespace {
using WriteProc=BOOL(WINAPI*)(HANDLE,LPCVOID,DWORD,LPDWORD,LPOVERLAPPED);
struct Operation {uint64_t offset=0;DWORD requested=0,physical=0,reported=0,error=0,thread=0;bool returned=false,injected=false,corrupted=false;};
struct Fault {
 const std::wstring target;const std::string mode;std::mutex mutex;std::condition_variable changed;
 std::vector<Operation> operations;bool held=false,released=false,holdTimedOut=false;std::atomic<unsigned> active{0};
 Fault(fs::path path,std::string kind):target(fs::absolute(path).lexically_normal().wstring()),mode(std::move(kind)){}
 void release(){std::lock_guard<std::mutex> lock(mutex);released=true;changed.notify_all();}
 bool matches(HANDLE file)const noexcept{
  wchar_t path[4096];const auto n=GetFinalPathNameByHandleW(file,path,4096,FILE_NAME_NORMALIZED|VOLUME_NAME_DOS);
  if(!n||n>=4096)return false;const wchar_t* begin=path;if(wcsncmp(begin,L"\\\\?\\",4)==0)begin+=4;
  return CompareStringOrdinal(begin,-1,target.c_str(),-1,TRUE)==CSTR_EQUAL;
 }
 bool waiting(){std::lock_guard<std::mutex> lock(mutex);return held;}
 uint64_t credited(){std::lock_guard<std::mutex> lock(mutex);uint64_t n=0;for(const auto& op:operations)if(op.returned&&op.reported==op.requested)n+=op.requested;return n;}
};
std::atomic<Fault*> activeFault{nullptr};WriteProc originalWrite=nullptr;
BOOL WINAPI controlledWrite(HANDLE file,LPCVOID bytes,DWORD count,LPDWORD reported,LPOVERLAPPED at)noexcept{
 Fault* fault=activeFault.load(std::memory_order_acquire);
 if(!fault||!fault->matches(file))return originalWrite(file,bytes,count,reported,at);
 struct Active {Fault& fault;Active(Fault& f):fault(f){++fault.active;}~Active(){--fault.active;}} active(*fault);
 try{
  if(!at){if(reported)*reported=0;SetLastError(ERROR_INVALID_PARAMETER);return FALSE;}
  Operation op;op.offset=(uint64_t(at->OffsetHigh)<<32)|at->Offset;op.requested=count;op.thread=GetCurrentThreadId();size_t index;
  {std::lock_guard<std::mutex> lock(fault->mutex);index=fault->operations.size();fault->operations.push_back(op);}
  LARGE_INTEGER fileSize{};const bool last=GetFileSizeEx(file,&fileSize)&&op.offset+count==static_cast<uint64_t>(fileSize.QuadPart);
  const bool selected=fault->mode=="hold-eof"?last:(fault->mode.rfind("hold",0)==0?index==0:index==2&&fault->mode!="control");
  op.injected=selected;
  if(selected&&fault->mode.rfind("hold",0)==0){
   std::unique_lock<std::mutex> lock(fault->mutex);fault->held=true;fault->changed.notify_all();
   if(!fault->changed.wait_for(lock,std::chrono::seconds(30),[&]{return fault->released;}))fault->holdTimedOut=true;
  }
  DWORD done=0;BOOL ok=FALSE;
  if(selected&&(fault->mode=="fail"||fault->mode=="partial-fail"||fault->mode=="short-success")){
   op.injected=true;
   if(fault->mode!="fail"){
    const DWORD partial=std::max<DWORD>(1,count/2);std::vector<BYTE> corrupt(partial);const auto* source=static_cast<const BYTE*>(bytes);
    for(DWORD i=0;i<partial;++i)corrupt[i]=source[i]^0x5a;OVERLAPPED location=*at;
    if(!originalWrite(file,corrupt.data(),partial,&done,&location)||done!=partial)throw std::runtime_error("Underlying fault-prefix write failed");
    at->Internal=location.Internal;at->InternalHigh=location.InternalHigh;op.corrupted=true;
   }
   ok=fault->mode=="short-success";op.error=ok?ERROR_SUCCESS:ERROR_DISK_FULL;
  }else{ok=originalWrite(file,bytes,count,&done,at);op.error=ok?ERROR_SUCCESS:GetLastError();}
  op.physical=done;op.reported=done;op.returned=ok!=FALSE;
  {std::lock_guard<std::mutex> lock(fault->mutex);fault->operations[index]=op;}
  if(reported)*reported=done;SetLastError(op.error);return ok;
 }catch(...){if(reported)*reported=0;SetLastError(ERROR_INVALID_DATA);return FALSE;}
}
// Only this fixture executable's import slots are changed. The original 8.62
// core objects, DLLs, other processes and on-disk images remain untouched.
class OwnWriteImport {
 struct Slot {PVOID volatile* address;PVOID original;};std::vector<Slot> slots;bool installed=false;
 static void exchange(Slot slot,PVOID value){DWORD before=0;if(!VirtualProtect(const_cast<PVOID*>(slot.address),sizeof(PVOID),PAGE_READWRITE,&before))throw std::runtime_error("Cannot make owned import writable");InterlockedExchangePointer(slot.address,value);DWORD ignored=0;if(!VirtualProtect(const_cast<PVOID*>(slot.address),sizeof(PVOID),before,&ignored))throw std::runtime_error("Cannot restore owned import protection");}
public:
 explicit OwnWriteImport(Fault& fault){
  const auto image=reinterpret_cast<BYTE*>(GetModuleHandleW(nullptr));auto dos=reinterpret_cast<IMAGE_DOS_HEADER*>(image);if(dos->e_magic!=IMAGE_DOS_SIGNATURE)throw std::runtime_error("Missing owned PE header");
  auto nt=reinterpret_cast<IMAGE_NT_HEADERS64*>(image+dos->e_lfanew);if(nt->Signature!=IMAGE_NT_SIGNATURE||nt->OptionalHeader.Magic!=IMAGE_NT_OPTIONAL_HDR64_MAGIC)throw std::runtime_error("Expected owned PE32+ image");
  const auto size=nt->OptionalHeader.SizeOfImage;const auto imports=nt->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT];if(!imports.VirtualAddress||imports.VirtualAddress>=size)throw std::runtime_error("Missing owned import table");
  auto table=reinterpret_cast<IMAGE_IMPORT_DESCRIPTOR*>(image+imports.VirtualAddress);
  for(size_t i=0;i<imports.Size/sizeof(IMAGE_IMPORT_DESCRIPTOR)&&table[i].Name;++i){
   if(!table[i].OriginalFirstThunk||table[i].OriginalFirstThunk>=size||table[i].FirstThunk>=size)continue;
   auto names=reinterpret_cast<IMAGE_THUNK_DATA64*>(image+table[i].OriginalFirstThunk);auto addresses=reinterpret_cast<IMAGE_THUNK_DATA64*>(image+table[i].FirstThunk);
   for(size_t n=0;names[n].u1.AddressOfData;++n){if(IMAGE_SNAP_BY_ORDINAL64(names[n].u1.Ordinal)||names[n].u1.AddressOfData>=size)continue;auto name=reinterpret_cast<IMAGE_IMPORT_BY_NAME*>(image+names[n].u1.AddressOfData);if(strcmp(reinterpret_cast<const char*>(name->Name),"WriteFile")==0)slots.push_back({reinterpret_cast<PVOID volatile*>(&addresses[n].u1.Function),reinterpret_cast<PVOID>(addresses[n].u1.Function)});}
  }
  if(slots.empty())throw std::runtime_error("Owned WriteFile import not found");for(const auto& slot:slots)if(slot.original!=slots.front().original)throw std::runtime_error("Ambiguous owned WriteFile imports");
  originalWrite=reinterpret_cast<WriteProc>(slots.front().original);activeFault.store(&fault,std::memory_order_release);
  try{for(auto slot:slots)exchange(slot,reinterpret_cast<PVOID>(&controlledWrite));installed=true;}catch(...){for(auto slot:slots)exchange(slot,slot.original);activeFault=nullptr;throw;}
 }
 OwnWriteImport(const OwnWriteImport&)=delete;OwnWriteImport& operator=(const OwnWriteImport&)=delete;
 ~OwnWriteImport(){if(installed)try{restore();}catch(...){std::terminate();}}
 void restore(){if(!installed)return;for(auto slot:slots)exchange(slot,slot.original);activeFault.store(nullptr,std::memory_order_release);installed=false;}
 size_t count()const noexcept{return slots.size();}
};
Json operations(Fault& fault){std::lock_guard<std::mutex> lock(fault.mutex);Json rows=Json::array();for(const auto& op:fault.operations)rows.push_back({{"offset",op.offset},{"requested",op.requested},{"physicalBytes",op.physical},{"reportedBytes",op.reported},{"returnedSuccess",op.returned},{"error",op.error},{"injected",op.injected},{"corrupted",op.corrupted},{"thread",op.thread}});return rows;}
}
int wmain(int argc,wchar_t** argv){
 if(argc!=4)return 2;const auto area=fs::absolute(argv[1]);const auto mode=utf8(argv[3]);
 if(mode=="resume"){
  if(!fs::is_regular_file(area/L".udm-ordered-storage-fault"))return 5;Json result;bool passed=false;std::string error;
  try{Manager manager(area/L"catalog");if(manager.jobs.size()!=1)throw std::runtime_error("Expected one owned recovery job");auto job=manager.jobs[0];manager.resume(job);const auto began=GetTickCount64();
   for(;;){manager.tick();std::string status;bool active;{Lock lock(manager.mutex);status=str(job->data,"Status");active=manager.isActive(job);}if(!active&&(status=="Complete"||status=="Failed"))break;if(GetTickCount64()-began>120000)throw std::runtime_error("Owned recovery deadline exceeded");Sleep(5);}
   manager.stop();result["job"]=job->snapshot();if(str(job->data,"Status")!="Complete")throw std::runtime_error("Owned resume did not complete: "+str(job->data,"Error"));result["target"]=utf8(job->target().wstring());result["fileSha256"]=fileHash(job->target());result["fileBytes"]=fs::file_size(job->target());result["normalStopJoined"]=true;passed=true;
  }catch(const std::exception& e){error=e.what();}result["passed"]=passed;result["error"]=error;result["mode"]=mode;atomicText(area/L"resume-result.json",result.dump(2),false);std::cout<<(passed?"passed":error)<<"\n";return passed?0:1;
 }
 if(mode!="control"&&mode!="fail"&&mode!="partial-fail"&&mode!="short-success"&&mode!="hold-pause"&&mode!="hold-rollback"&&mode!="hold-eof")return 3;
 if(fs::exists(area))return 4;fs::create_directories(area);atomicText(area/L".udm-ordered-storage-fault","owned self-process IO fixture",false);
 Json result,checks=Json::array();bool passed=false;std::string error;
 auto check=[&](bool value,const char* name){checks.push_back({{"name",name},{"passed",value}});if(!value)throw std::runtime_error(name);};
 try{
  Manager manager(area/L"catalog");{Lock lock(manager.mutex);auto& prefs=manager.state["Settings"];prefs["Connections"]=8;prefs["Parallel"]=1;prefs["SuppressProgressDialog"]=true;prefs["SuppressCompletionDialog"]=true;prefs["Sound"]=false;prefs["PrefetchFileInfo"]=false;prefs["ClipboardMonitor"]=false;prefs["CategoryFolders"]=false;manager.state["Queues"][0]["Retries"]=3;}
  auto job=manager.add(utf8(argv[2]),utf8((area/L"files").wstring()),"storage-fault.bin","Main queue",true);Fault fault(manager.root/L"parts"/wide(job->id())/(wide(job->id())+L".data"),mode);OwnWriteImport hook(fault);
  try{
   manager.resume(job);const auto began=GetTickCount64();bool handledHold=false;
   for(;;){
    if(!handledHold)manager.tick();Json live;bool active;
    {Lock lock(manager.mutex);live=job->snapshot();active=manager.isActive(job);}
    if(mode.rfind("hold",0)==0&&fault.waiting()&&!handledHold&&num(live,"Received")>=static_cast<i64>(fault.credited()+(mode=="hold-eof"?1:131072))){
     handledHold=true;CatalogCheckpointTestAccess::stopAutomatic(manager);manager.save();const auto stable=readText(manager.root/L"state.json");auto saved=Json::parse(stable)["Downloads"][0];
     check(num(live,"Received")>num(saved,"Received"),"Live accepted progress runs ahead of the actual saved written prefixes during held IO");
     check(num(saved,"Received")==static_cast<i64>(fault.credited()),"The held operation receives no premature resume credit");
     check(!fs::exists(job->target())&&!manager.completionSaved(job),"Held body IO cannot publish a file or acknowledge completion");
     if(mode=="hold-rollback"){
      CatalogCheckpointTestAccess::observe(manager,[](const char* stage){if(std::string_view(stage)=="temporary-flushed")throw std::runtime_error("injected pause catalog failure");});
      std::string rejected;try{manager.pause(job);}catch(const std::exception& e){rejected=e.what();}CatalogCheckpointTestAccess::observe(manager,{});
      check(rejected=="injected pause catalog failure"&&readText(manager.root/L"state.json")==stable,"A failed pause checkpoint preserves the actual prior catalog");
      {Lock lock(manager.mutex);check(str(job->data,"Status")=="Downloading"&&!CatalogCheckpointTestAccess::cancelled(manager,job),"Failed pause persistence restores live state without canceling its generation");}
     }
     manager.pause(job);auto stopping=std::async(std::launch::async,[&]{manager.stop();});
     const bool waiting=stopping.wait_for(std::chrono::milliseconds(40))==std::future_status::timeout;
     auto view=manager.snapshot();fault.release();stopping.get();
     check(waiting&&view["Downloads"].size()==1,"Stop waits for held IO while model snapshots remain usable");break;
    }
    if(!active&&(str(live,"Status")=="Complete"||str(live,"Status")=="Failed"))break;
    if(GetTickCount64()-began>90000)throw std::runtime_error("Storage fault fixture deadline exceeded");Sleep(5);
   }
   manager.stop();check(fault.active==0,"All intercepted body IO has joined before import restoration");hook.restore();result["importSlotsRestored"]=hook.count();result["holdTimedOut"]=fault.holdTimedOut;result["operations"]=operations(fault);result["job"]=job->snapshot();result["catalog"]=Json::parse(readText(manager.root/L"state.json"));result["partsFile"]=utf8(fault.target);result["normalStopJoined"]=true;
   const auto& state=result["job"];const auto status=str(state,"Status");
   check(yes(state["OrderedBodyWrites"],"Used"),"The actual Manager transfer used the qualified ordered path");
   if(mode=="control")check(status=="Complete"&&yes(state["OrderedBodyWrites"],"Complete"),"A pass-through import control completes through the actual pipeline");
   else if(mode.rfind("hold",0)==0){
    check(handledHold&&!fault.holdTimedOut&&status=="Paused","Releasing held IO finishes the requested parent pause before its deadline");
    const auto& body=state["OrderedBodyWrites"];check(yes(body,"Paused")||(yes(body,"Complete")&&num(body,"WrittenBytes")==num(state,"Size")),"An EOF finish already in flight remains distinct from the canceled publication");
    check(!fs::exists(job->target())&&!manager.completionSaved(job),"Neither early nor EOF pause can publish or acknowledge completion");
    check(num(state,"Received")==static_cast<i64>(fault.credited()),"Paused resume counters equal exactly successful actual writes");
   }
   else{
    check(status=="Failed"&&str(state,"Error")=="Cannot write the partial download.","The original write failure survives HTTP cancellation instead of becoming a user pause");
    check(yes(state["OrderedBodyWrites"],"Failed")&&!yes(state["OrderedBodyWrites"],"Complete"),"Storage failure closes ordered completion admission");
    check(result["operations"].size()==3&&yes(result["operations"][2],"injected"),"No storage operation follows the selected third-write fault");
    check(num(state,"Received")==static_cast<i64>(fault.credited())&&num(state["OrderedBodyWrites"],"WrittenBytes")==num(state,"Received"),"Failed and short physical writes receive no disk or saved progress credit");
    check(!fs::exists(job->target())&&!manager.completionSaved(job),"A failed body cannot publish an output or acknowledged completion");
   }
   check(result["catalog"]["Downloads"][0]["Received"]==state["Received"]&&result["catalog"]["Downloads"][0]["Segments"]==state["Segments"],"The actual final catalog preserves exactly the reconciled resume counters");
   if(mode=="control"){result["target"]=utf8(job->target().wstring());result["fileSha256"]=fileHash(job->target());}
   passed=true;
  }catch(...){fault.release();CatalogCheckpointTestAccess::observe(manager,{});try{manager.stop();}catch(...){}if(fault.active==0)hook.restore();throw;}
 }catch(const std::exception& e){error=e.what();}
 result["passed"]=passed;result["error"]=error;result["mode"]=mode;result["checks"]=checks;result["selfProcessImportOnly"]=true;result["performanceComparison"]=false;atomicText(area/L"result.json",result.dump(2),false);std::cout<<checks.size()<<" checks; "<<(passed?"passed":error)<<"\n";return passed?0:1;
}
