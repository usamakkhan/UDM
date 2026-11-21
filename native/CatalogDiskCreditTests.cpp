#include "CatalogDiskCredit.hpp"
#include "CatalogAdmissionRegistry.hpp"
#include <future>
#include <iostream>
namespace udm {
struct CatalogCheckpointTestAccess {
 static void active(Manager& m,const JobPtr& j,const std::shared_ptr<Cancel>& c){Lock lock(m.mutex);if(c)m.active[j->id()]=c;else m.active.erase(j->id());}
 static void checkpoint(Manager& m,std::function<void(const char*)> f){Lock lock(m.mutex);m.catalogCheckpoint=std::move(f);}
 static void automatic(Manager& m){m.saveCatalog(true);}
 static CatalogAdmissionRegistry& admission(Manager& m){return *m.admissionRegistry;}
};
}
using namespace udm;
int wmain(int argc,wchar_t** argv){
 if(argc!=2)return 2;fs::path root=argv[1];if(fs::exists(root))return 3;fs::create_directories(root);
 Json checks=Json::array();bool passed=false;std::string error;
 auto check=[&](bool v,const char* name){checks.push_back({{"name",name},{"passed",v}});if(!v)throw std::runtime_error(name);};
 auto refused=[&](auto&& f){try{f();return false;}catch(const std::exception&){return true;}};
 try{
  for(const bool fail:{false,true}){
   Manager m(root/(fail?L"failed":L"success"));auto c=std::make_shared<Cancel>();
   const auto id=guid();auto parts=m.root/L"parts"/wide(id);fs::create_directories(parts);auto path=parts/(wide(id)+L".data");
   Handle file(CreateFileW(path.c_str(),GENERIC_READ|GENERIC_WRITE,FILE_SHARE_READ|FILE_SHARE_WRITE|FILE_SHARE_DELETE,nullptr,CREATE_NEW,FILE_ATTRIBUTE_NORMAL,nullptr));
   check(static_cast<bool>(file),"The Manager fixture owns the expected root parts file");
   Json row={{"Id",id},{"Url","http://127.0.0.1/body"},{"Size",65536},{"Received",0},{"TransferredBytes",65536},{"Folder",utf8((m.root/L"files").wstring())},{"FileName","body.bin"},{"Status","Downloading"},{"Segments",Json::array({{{"Index",0},{"Start",0},{"End",65535},{"Done",0}}})}};
   auto job=std::make_shared<Job>(row);m.jobs.push_back(job);CatalogCheckpointTestAccess::active(m,job,c);
   check(!CatalogDiskCredit::attach(m,job,c),"A direct caller without the owning worker scope keeps the legacy synchronous path");
   CatalogDiskCredit::WorkerScope worker(m,job,c);auto lease=CatalogDiskCredit::attach(m,job,c);
   check(lease&&CatalogDiskCredit::active(m),"The active root worker attaches its actual expected parts file");
   check(refused([&]{CatalogDiskCredit::attach(m,job,c);}),"Duplicate binding admission is refused without replacing the owner");
   const auto pendingId=guid();auto& registry=CatalogCheckpointTestAccess::admission(m);auto admission=registry.acquire("disk-credit-pending","disk-credit-binding");
   admission.prepare(std::make_shared<const CatalogAdmissionReservation>("disk-credit-pending",Json{{"Id",pendingId},{"Folder",utf8((m.root/L"pending").wstring())},{"FileName","pending.bin"},{"Status","Awaiting confirmation"},{"Segments",Json::array()},{"Video",Json()},{"Audio",Json()}},Json{{"protocol",3},{"status","accepted"},{"id",pendingId},{"fingerprint","disk-credit-binding"}}));
   auto write=[&](i64 offset,i64 count){
    auto pending=lease->beginWrite(offset,count);LARGE_INTEGER at{};at.QuadPart=offset;Bytes body(static_cast<size_t>(count));for(size_t i=0;i<body.size();++i)body[i]=static_cast<unsigned char>((offset+i)%251);
    DWORD done=0;if(!SetFilePointerEx(file.h,at,nullptr,FILE_BEGIN)||!WriteFile(file.h,body.data(),static_cast<DWORD>(body.size()),&done,nullptr)||done!=body.size())throw std::runtime_error("Fixture write failed");pending.acknowledge();
   };
   write(0,4096);
   {Lock lock(m.mutex);job->data["Received"]=32768;job->data["Segments"][0]["Done"]=32768;}
   m.save();auto saved=Json::parse(readText(m.root/L"state.json"));
   check(saved["Downloads"].size()==2&&admission.committed()&&str(saved["BrowserCaptures"]["disk-credit-pending"],"id")==pendingId,"The same actual save preserves and acknowledges the pending browser-admission overlay");
   check(num(saved["Downloads"][0],"Received")==4096&&num(saved["Downloads"][0]["Segments"][0],"Done")==4096,"The actual Manager save persists written prefixes rather than live RAM-only progress");
   check(num(saved["Downloads"][0],"TransferredBytes")==65536&&num(job->data,"Received")==32768&&num(m.snapshot()["Downloads"][0],"Received")==32768,"Actual Job/UI progress and network accounting remain separate from persisted resume credit");
   unsigned writes=0;CatalogCheckpointTestAccess::checkpoint(m,[&](const char* stage){if(std::string_view(stage)=="temporary-flushed")++writes;});
   {Lock lock(m.mutex);job->data["Received"]=65536;job->data["Segments"][0]["Done"]=65536;}
   CatalogCheckpointTestAccess::automatic(m);
   check(writes==1,"A pending browser admission retains its forced automatic publication even when disk credit is unchanged");
   // Install the already committed admission in live state before testing the
   // no-pending-overlay dirty shortcut, exactly as its normal caller must.
   {Lock lock(m.mutex);m.jobs.push_back(std::make_shared<Job>(saved["Downloads"][1]));m.state["BrowserCaptures"]=saved["BrowserCaptures"];admission.installed();}
   CatalogCheckpointTestAccess::automatic(m);
   check(writes==1,"The real periodic dirty check skips a RAM-only progress change once the admission is installed");
   write(4096,4096);CatalogCheckpointTestAccess::automatic(m);
   check(writes==2&&num(Json::parse(readText(m.root/L"state.json"))["Downloads"][0],"Received")==8192,"Advancing acknowledged disk credit triggers the real automatic checkpoint");
   auto stable=readText(m.root/L"state.json");
   {
    auto previous=job->data["Url"];job->data["Url"]="http://127.0.0.1/changed";
    check(refused([&]{m.save();})&&readText(m.root/L"state.json")==stable,"A changed live representation cannot publish the previous generation's credit");job->data["Url"]=previous;m.save();
   }
   {
    CatalogCheckpointTestAccess::active(m,job,std::make_shared<Cancel>());
    check(refused([&]{m.save();})&&readText(m.root/L"state.json")==stable,"The Manager private active map rejects the old cancellation generation");CatalogCheckpointTestAccess::active(m,job,c);m.save();
   }
   {
    auto previous=m.jobs;m.jobs.clear();check(refused([&]{m.save();})&&readText(m.root/L"state.json")==stable,"A captured generation cannot disappear from catalog membership");m.jobs=previous;m.save();
   }
   auto blockedDelete=[&]{Handle attempt(CreateFileW(path.c_str(),DELETE,FILE_SHARE_READ|FILE_SHARE_WRITE|FILE_SHARE_DELETE,nullptr,OPEN_EXISTING,0,nullptr));return !attempt;};
   check(blockedDelete(),"The binding physically denies deletion of the expected parts file");
   std::promise<void> held,release,closing;auto ready=held.get_future();auto resume=release.get_future();auto closeEntered=closing.get_future();
   CatalogCheckpointTestAccess::checkpoint(m,[&](const char* stage){if(std::string_view(stage)=="temporary-flushed"){held.set_value();resume.wait();if(fail)throw std::runtime_error("injected held Manager save failure");}});
   std::exception_ptr saveError;std::thread saving([&]{try{m.save();}catch(...){saveError=std::current_exception();}});
   const bool reached=ready.wait_for(std::chrono::seconds(10))==std::future_status::ready;
   // Always release/join before reporting a failed synchronization assertion.
   auto closingWorker=std::async(std::launch::async,[&]{closing.set_value();lease->close();});
   const bool entered=closeEntered.wait_for(std::chrono::seconds(10))==std::future_status::ready;
   const bool waiting=closingWorker.wait_for(std::chrono::milliseconds(30))==std::future_status::timeout;
   const bool filePinned=blockedDelete();release.set_value();saving.join();closingWorker.get();
   check(reached&&entered&&waiting&&filePinned,"Binding retirement waits outside Manager::mutex while an actual detached catalog write is held");
   check(bool(saveError)==fail,"The held Manager write reports the intended success or injected failure");
   check(!CatalogDiskCredit::active(m)&&num(job->data,"Received")==8192&&num(job->data["Segments"][0],"Done")==8192&&num(job->data,"TransferredBytes")==65536,"Retirement atomically reconciles live resume counters and detaches without charging network bytes again");
   check(!blockedDelete(),"The physical parts-file pin is released only after the held save and reconciliation settle");
   if(fail)check(readText(m.root/L"state.json")==stable&&!m.storageError.empty(),"A failed real save preserves the previous catalog and storage error");
   else check(num(Json::parse(readText(m.root/L"state.json"))["Downloads"][0],"Received")==8192,"Successful held catalog IO retains exact acknowledged resume credit");
   CatalogCheckpointTestAccess::checkpoint(m,{});m.save();check(m.storageError.empty(),"A later forced checkpoint clears a resolved storage error");
   check(FlushFileBuffers(file.h)!=FALSE,"The native binding leaves the caller-owned file durability barrier intact");
   Bytes expected(8192);for(size_t i=0;i<expected.size();++i)expected[i]=static_cast<unsigned char>(i%251);writeBytes(m.root/L"expected.bin",expected);
   check(fileHash(file.h)==fileHash(m.root/L"expected.bin"),"The acknowledged actual parts file matches independent complete fixture bytes and hash");
   CatalogCheckpointTestAccess::active(m,job,{});m.stop();
   {Manager reload(m.root);check(reload.jobs.size()==2&&reload.jobs[0]->id()==id&&num(reload.jobs[0]->data,"Received")==8192&&num(reload.jobs[0]->data["Segments"][0],"Done")==8192,"The actual Manager reload restores disk-backed progress and the independently admitted job");}
  }
  {
   Manager m(root/L"network-accounting");auto c=std::make_shared<Cancel>();auto job=std::make_shared<Job>(Json{{"Id",guid()},{"FileName","counter.bin"},{"Received",0},{"TransferredBytes",20},{"Segments",Json::array({{{"Index",0},{"Start",0},{"End",99},{"Done",0}}})}});Worker worker;
   m.progress(job,4,0,&worker,false);
   check(num(job->data,"Received")==4&&num(job->data["Segments"][0],"Done")==4&&num(job->data,"TransferredBytes")==20,"Already-accounted buffered reception advances accepted progress without charging network bytes twice");
   check(worker.received==4&&worker.position==4,"Buffered accepted progress retains the worker UI cursor contract");
   m.progress(job,3,0,&worker);
   check(num(job->data,"Received")==7&&num(job->data,"TransferredBytes")==23&&worker.position==7,"Legacy progress retains its default one-time network accounting");
  }
  passed=true;
 }catch(const std::exception& e){error=e.what();}
 atomicText(root/L"results.json",Json{{"passed",passed},{"error",error},{"checks",checks}}.dump(2),false);std::cout<<checks.size()<<" checks; "<<(passed?"passed":error)<<"\n";return passed?0:1;
}
