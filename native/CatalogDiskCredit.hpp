#pragma once
#include "WrittenDiskCheckpoint.hpp"
namespace udm {
// Runtime-only bindings; no pointers, generations or RAM coverage are persisted.
// The normal transfer worker scopes ownership. Capture is called only while the
// Manager model lock is held; release waits outside it. No registry/credit lock
// is held while acquiring Manager::mutex or doing IO.
class CatalogDiskCredit {
 struct Entry {
  JobPtr job;std::shared_ptr<const Cancel> cancel;std::shared_ptr<Handle> file;
  WrittenDiskCheckpoint credit;
  Entry(JobPtr j,std::shared_ptr<const Cancel> c,std::shared_ptr<Handle> f,const Json& row):job(std::move(j)),cancel(std::move(c)),file(std::move(f)),credit(row,cancel,file){}
 };
 struct Entries {std::map<const Job*,std::shared_ptr<Entry>> values;};
 struct Registry {std::mutex mutex;std::map<const Manager*,std::shared_ptr<Entries>> managers;};
 static Registry& registry(){static auto* value=new Registry;return *value;}
 static std::shared_ptr<Entries> find(const Manager& manager){auto& r=registry();std::lock_guard<std::mutex> lock(r.mutex);auto it=r.managers.find(&manager);return it==r.managers.end()?nullptr:it->second;}
 static void erase(Manager& manager,const std::shared_ptr<Entry>& entry){
  auto& r=registry();std::lock_guard<std::mutex> lock(r.mutex);auto found=r.managers.find(&manager);
  if(found==r.managers.end())return;auto& entries=found->second->values;auto it=entries.find(entry->job.get());
  if(it!=entries.end()&&it->second==entry)entries.erase(it);if(entries.empty())r.managers.erase(found);
 }
public:
 class WorkerScope {
  friend class CatalogDiskCredit;
  inline static thread_local WorkerScope* current=nullptr;
  WorkerScope* previous;const Manager* manager;JobPtr job;std::shared_ptr<const Cancel> cancel;
 public:
  WorkerScope(const Manager& m,JobPtr j,std::shared_ptr<const Cancel> c):previous(current),manager(&m),job(std::move(j)),cancel(std::move(c)){current=this;}
  ~WorkerScope(){current=previous;}
  WorkerScope(const WorkerScope&)=delete;WorkerScope& operator=(const WorkerScope&)=delete;
 };
 class Lease {
  friend class CatalogDiskCredit;
  Manager& manager;std::shared_ptr<Entry> entry;
  Lease(Manager& m,std::shared_ptr<Entry> e):manager(m),entry(std::move(e)){}
 public:
  Lease(const Lease&)=delete;Lease& operator=(const Lease&)=delete;
  ~Lease(){try{close();}catch(const std::exception& e){try{Lock lock(manager.mutex);manager.storageError=e.what();}catch(...){}}}
  WrittenDiskCheckpoint::Write beginWrite(i64 offset,i64 length){if(!entry)throw std::logic_error("Disk-credit binding closed");return entry->credit.beginWrite(entry->cancel,entry->file,offset,length);}
  // The ordered writer owns these already accepted bytes before cancellation.
  // Preserve them on pause; this is not admission for a new network operation.
  WrittenDiskCheckpoint::Write beginBufferedWrite(i64 offset,i64 length){if(!entry)throw std::logic_error("Disk-credit binding closed");return entry->credit.beginWrite(entry->cancel,entry->file,offset,length,WrittenDiskCheckpoint::WriteIntent::AcceptedBuffer);}
  // Producers must be joined before close. Call without an outer model lock.
  void close(){
   if(!entry)return;
   for(;;){
    std::shared_ptr<Entry> retired;
    {Lock lock(manager.mutex);auto row=entry->credit.tryRetireSnapshot(entry->job->data,entry->cancel,entry->file);
     if(row){entry->job->data.swap(*row);erase(manager,entry);retired=std::move(entry);}}
    if(retired)return; // Last file-owner destruction occurs outside the model lock.
    entry->credit.waitForPins();
   }
  }
 };
 class Publication {
  friend class CatalogDiskCredit;
  std::vector<WrittenDiskCheckpoint::Snapshot> snapshots;
 public:
  Publication()=default;Publication(Publication&&)=default;Publication& operator=(Publication&&)=default;
  Publication(const Publication&)=delete;Publication& operator=(const Publication&)=delete;
  explicit operator bool()const noexcept{return !snapshots.empty();}
 };
 // Called with Manager::mutex held. The generation predicate is supplied by
 // Manager's private active map, not by a persisted job ID or browser request.
 template<class Current>static Publication capture(const Manager& manager,Json& catalog,Current&& current){
  Publication result;auto entries=find(manager);if(!entries)return result;result.snapshots.reserve(entries->values.size());
  for(const auto& item:entries->values){const auto& entry=*item.second;
   if(!current(entry.job->id(),entry.cancel.get())||std::find(manager.jobs.begin(),manager.jobs.end(),entry.job)==manager.jobs.end())throw std::runtime_error("Disk-credit generation no longer owns this download");
   Json* row=nullptr;
   for(auto& candidate:catalog["Downloads"])if(str(candidate,"Id")==entry.job->id()){if(row)throw std::runtime_error("Duplicate disk-credit catalog row");row=&candidate;}
   if(!row)throw std::runtime_error("A catalog cannot omit its active disk-credit download");
   auto snapshot=entry.credit.capture(*row,entry.cancel,entry.file);*row=snapshot.value();result.snapshots.push_back(std::move(snapshot));
  }
  return result;
 }
 // No active binding means callers retain their original fast dirty check.
 static bool active(const Manager& manager){auto entries=find(manager);return entries&&!entries->values.empty();}
 static std::unique_ptr<Lease> attach(Manager& manager,const JobPtr& job,const std::shared_ptr<Cancel>& cancel){
  const auto scope=WorkerScope::current;
  if(!scope||scope->manager!=&manager||scope->job!=job||scope->cancel!=cancel)return {};
  cancel->check();Json before;fs::path path;
  {Lock lock(manager.mutex);if(!manager.isActive(job)||std::find(manager.jobs.begin(),manager.jobs.end(),job)==manager.jobs.end())throw std::runtime_error("Disk-credit attachment requires an active root download");
   if(num(job->data,"Received")!=0)return {};before=job->data;
   auto parts=manager.root/L"parts"/wide(job->id());if(!str(before,"PartsFolder").empty())parts=fs::path(wide(str(before,"PartsFolder")));path=parts/(wide(job->id())+L".data");}
  // Pin the actual expected parts path before workers start, denying rename or
  // deletion until all captured checkpoints and this binding are released.
  auto file=std::make_shared<Handle>(CreateFileW(path.c_str(),GENERIC_READ,FILE_SHARE_READ|FILE_SHARE_WRITE,nullptr,OPEN_EXISTING,FILE_FLAG_OPEN_REPARSE_POINT,nullptr));
  if(!*file)return {};
  auto entry=std::make_shared<Entry>(job,cancel,std::move(file),before);
  auto lease=std::unique_ptr<Lease>(new Lease(manager,{}));
  {Lock lock(manager.mutex);cancel->check();
   if(!manager.isActive(job)||std::find(manager.jobs.begin(),manager.jobs.end(),job)==manager.jobs.end())throw std::runtime_error("Download lost its active membership during disk-credit attachment");
   if(num(job->data,"Received")!=0||job->data.value("Segments",Json())!=before.value("Segments",Json()))throw std::runtime_error("Download progressed during disk-credit attachment");
   {auto checked=entry->credit.capture(job->data,entry->cancel,entry->file);(void)checked;}
   auto& r=registry();std::lock_guard<std::mutex> guard(r.mutex);auto found=r.managers.find(&manager);
   if(found==r.managers.end()){auto value=std::make_shared<Entries>();value->values.emplace(job.get(),entry);r.managers.emplace(&manager,std::move(value));}
   else if(!found->second->values.emplace(job.get(),entry).second)throw std::runtime_error("Another disk-credit binding owns this download");
  }
  lease->entry=std::move(entry);return lease;
 }
 // Manager destruction follows worker/checkpoint joins. Detached publications
 // own their entry/file lifetime independently; no callback retains Manager.
 static void forget(const Manager& manager){std::shared_ptr<Entries> released;auto& r=registry();{std::lock_guard<std::mutex> lock(r.mutex);auto it=r.managers.find(&manager);if(it!=r.managers.end()){released=std::move(it->second);r.managers.erase(it);}}}
};
}
