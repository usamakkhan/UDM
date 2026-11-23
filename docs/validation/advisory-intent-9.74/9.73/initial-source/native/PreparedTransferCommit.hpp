#pragma once
#include "Core.hpp"
#include "PublicationRecovery.hpp"
#include "PublishedWriteback.hpp"
#include "PublicationSourcePin.hpp"
#include <future>
namespace udm {
struct AdvisoryHashResult {std::string digest;double seconds=0;};
// Same-thread handoff from an ordinary transfer to its owning Manager worker.
// The owning worker joins final body writeback before publication. No detached
// work or persisted acceptance token; publishFile owns the durable transaction.
class PreparedTransferCommit {
 friend struct CatalogCheckpointTestAccess;
 inline static thread_local PreparedTransferCommit* current=nullptr;
 PreparedTransferCommit* previous;Manager& manager;JobPtr job;std::shared_ptr<Cancel> cancel;bool allowed;
 bool ready=false,inCommit=false,accepted=false,keepParts=false;
 std::unique_ptr<PublicationSourcePin> writebackPin;std::future<PublishedWritebackResult> writeback;
 std::future<AdvisoryHashResult> advisoryHash;std::shared_ptr<Cancel> advisoryCancel;
 fs::path staging,parts,target;Json destination,cleanupIdentity;std::string digest;
 struct Finalizing {std::mutex mutex;std::map<const Manager*,std::set<const Cancel*>> owners;std::map<const Manager*,std::map<const Cancel*,std::pair<const Job*,const PreparedTransferCommit*>>> independent;};
 static Finalizing& finalizing(){static auto* value=new Finalizing;return *value;}
 bool finalizationOwned=false;
 bool independentOwned=false;
public:
 Json publicationTimings;double cleanupSeconds=0;
 PreparedTransferCommit(Manager& m,JobPtr j,std::shared_ptr<Cancel> c,bool allow):previous(current),manager(m),job(std::move(j)),cancel(std::move(c)),allowed(allow){current=this;}
 PreparedTransferCommit(const PreparedTransferCommit&)=delete;
 static PreparedTransferCommit* available(Manager& m,const JobPtr& j,const std::shared_ptr<Cancel>& c){auto p=current;return p&&p->allowed&&!p->ready&&!p->accepted&&!p->inCommit&& &p->manager==&m&&p->job==j&&p->cancel==c?p:nullptr;}
 static PreparedTransferCommit* publishing(Manager& m,const JobPtr& j,const Cancel* c){auto p=current;return p&&p->inCommit&& &p->manager==&m&&p->job==j&&p->cancel.get()==c?p:nullptr;}
 // Launch failure leaves the caller on its existing synchronous path.
 static PublishedWritebackResult flushPinned(Handle body,const fs::path& path,double opening){
  struct Ops:PublishedWritebackOps {Handle& owned;explicit Ops(Handle& h):owned(h){}HANDLE open(const fs::path&,DWORD){const auto result=owned.h;owned.h=INVALID_HANDLE_VALUE;return result;}} ops(body);
  auto result=publishedWriteback(path,false,ops);result.openSeconds+=opening;return result;
 }
 bool startWriteback(const fs::path& path,bool normalize){
  if(!allowed||ready||writeback.valid()||normalize)return false;
  const auto began=std::chrono::steady_clock::now();
  // The write-only object excludes every other writer from open through exact
  // acceptance. Its duplicate flushes on the task; publication adopts this pin.
  try{
   auto pin=PublicationSourcePin::open(path);if(!pin)return false;auto body=pin->duplicate();if(!body)return false;
   const auto opening=std::chrono::duration<double>(std::chrono::steady_clock::now()-began).count();
   writeback=std::async(std::launch::async,[body=std::move(body),path,opening]()mutable{return flushPinned(std::move(body),path,opening);});writebackPin=std::move(pin);return true;
  }catch(...){return false;}
 }
 bool hasWritebackPin()const noexcept{return bool(writebackPin);}
 std::unique_ptr<PublicationSourcePin> takeWritebackPin()noexcept{return std::move(writebackPin);}
 void releaseWritebackPin()noexcept{writebackPin.reset();}
 bool pendingWriteback()const noexcept{return writeback.valid();}
 // Only a value-owned task may outlive transfer(). Its body/hash state must
 // not refer to the transfer stack. Required checksums never use this handoff.
 void adoptAdvisoryHash(std::future<AdvisoryHashResult> task,std::shared_ptr<Cancel> token){
  if(!allowed||ready||inCommit||accepted||advisoryHash.valid()||!writebackPin||!task.valid()||!token)throw std::logic_error("Invalid advisory hash handoff.");
  {Lock lock(manager.mutex);if(!str(job->data,"ExpectedSha256").empty()||!str(job->data,"ReplacementOf").empty())throw std::logic_error("A required checksum cannot use advisory publication.");}
  advisoryHash=std::move(task);advisoryCancel=std::move(token);
 }
 bool pendingAdvisoryHash()const noexcept{return advisoryHash.valid();}
 std::string finishAdvisoryHash(){
  if(!advisoryHash.valid())return {};
  const auto began=std::chrono::steady_clock::now();AdvisoryHashResult result;
  try{result=advisoryHash.get();}catch(...){/* Advisory failure leaves no digest. */}
  advisoryCancel.reset();
  if(!result.digest.empty()&&!std::regex_match(result.digest,std::regex("[a-f0-9]{64}")))result.digest.clear();
  {Lock lock(manager.mutex);auto& phases=job->data["FinishPhases"];phases["AdvisoryHashSeconds"]=result.seconds;phases["AdvisoryHashOwnerJoin"]=std::chrono::duration<double>(std::chrono::steady_clock::now()-began).count();phases["AdvisoryHashIntentOverlap"]=true;}
  return result.digest;
 }
 void joinWriteback()noexcept{if(writeback.valid())try{writeback.wait();}catch(...){}}
 void finishWriteback(){
  if(!writeback.valid())return;
  const auto began=std::chrono::steady_clock::now();const auto result=writeback.get();
  {Lock lock(manager.mutex);auto& phases=job->data["FinishPhases"];
   phases["PublishedFileOpen"]=result.openSeconds;phases["PublishedSparseNormalize"]=result.normalizeSeconds;
   phases["PublishedFileFlush"]=result.flushSeconds;phases["PublishedFileClose"]=result.closeSeconds;
   phases["PublishedOpenError"]=result.openError;phases["PublishedNormalizeError"]=result.normalizeError;
   phases["PublishedFlushError"]=result.flushError;phases["PublishedCloseError"]=result.closeError;
   phases["PublishedWriteAccess"]="write-only";phases["PublishedWritebackOverlap"]=true;
   phases["PublishedWritebackJoin"]=std::chrono::duration<double>(std::chrono::steady_clock::now()-began).count();
   phases["FlushPublished"]=result.openSeconds+result.normalizeSeconds+result.flushSeconds+result.closeSeconds;
   phases["SparseNormalization"]="not-needed";phases["Unsparse"]=0.0;
   if(!result.confirmed)job->data["FinishWarning"]="Final body writeback failed; the downloaded data and any publication recovery record were kept.";
  }
  endFinalization();
  if(!result.confirmed)throw std::runtime_error("Final download writeback could not be confirmed. Keep its files and recovery record.");
 }
 bool prepared()const noexcept{return ready;}
 bool committed()const noexcept{return accepted;}
 // Caller holds Manager::mutex. The persistent flag can be retired for the
 // publication checkpoint, but stopped-queue policy must still recognize the
 // exact independently requested worker until its finishing lifetime ends.
 void protectIndependentFinish(){
  if(independentOwned||!yes(job->data,"IndividualStart"))return;
  auto& state=finalizing();std::lock_guard<std::mutex> lock(state.mutex);
  const auto inserted=state.independent[&manager].emplace(cancel.get(),std::make_pair(job.get(),this));
  if(!inserted.second)throw std::logic_error("Independent finishing ownership already exists.");independentOwned=true;
 }
 template<class Active>static bool independentlyFinishing(const Manager& manager,const JobPtr& job,const Active& active){
  const auto worker=active.find(job->id());if(worker==active.end())return false;
  auto& state=finalizing();std::lock_guard<std::mutex> lock(state.mutex);const auto found=state.independent.find(&manager);if(found==state.independent.end())return false;
  const auto owner=found->second.find(worker->second.get());return owner!=found->second.end()&&owner->second.first==job.get();
 }
 // A full-body ordinary transfer is about to force its completion checkpoint.
 // Defer only redundant automatic IO, never explicit saves or pending receipts.
 void beginFinalization()noexcept{try{Lock lock(manager.mutex);if(!allowed||ready||accepted||!manager.isActive(job)||cancel->cancelled()||num(job->data,"Size")<=0||num(job->data,"Size")>64*1048576LL||num(job->data,"Received")!=num(job->data,"Size"))return;auto& state=finalizing();std::lock_guard<std::mutex> guard(state.mutex);state.owners[&manager].insert(cancel.get());finalizationOwned=true;}catch(...){/* Optional coalescing failure retains automatic IO. */}}
 void endFinalization()noexcept{if(!finalizationOwned)return;try{auto& state=finalizing();std::lock_guard<std::mutex> guard(state.mutex);auto found=state.owners.find(&manager);if(found!=state.owners.end()){found->second.erase(cancel.get());if(found->second.empty())state.owners.erase(found);}finalizationOwned=false;}catch(...){}}
 static bool defersAutomatic(Manager& manager,const std::map<std::string,std::shared_ptr<Cancel>>& active)noexcept{try{Lock lock(manager.mutex);if(!manager.storageError.empty()||active.empty())return false;auto& state=finalizing();std::lock_guard<std::mutex> guard(state.mutex);const auto found=state.owners.find(&manager);if(found==state.owners.end()||found->second.size()!=active.size())return false;for(const auto& item:active)if(item.second->cancelled()||!found->second.count(item.second.get()))return false;return true;}catch(...){return false;}}
 void prepare(const fs::path& stage,const fs::path& work,const std::string& hash,bool retain){
  if(!allowed||ready||inCommit||accepted||!stage.is_absolute()||!work.is_absolute()||stage.filename()!=L".udm-"+wide(job->id())+L".assembling")throw std::runtime_error("Invalid prepared download handoff.");
  if(advisoryHash.valid()&&!hash.empty())throw std::logic_error("A pending advisory hash cannot replace a known publication digest.");
  // Do not query the synchronous file object while its body flush is active.
  // The owned pin already captured this identity before launching writeback.
  Json identity;
  if(writebackPin)identity=writebackPin->capturedIdentity();
  else{Handle file(CreateFileW(stage.c_str(),FILE_READ_ATTRIBUTES,FILE_SHARE_READ|FILE_SHARE_DELETE,nullptr,OPEN_EXISTING,FILE_FLAG_OPEN_REPARSE_POINT,nullptr));if(file)identity=stableFileIdentity(file.h);}
  Lock lock(manager.mutex);if(!str(job->data,"ReplacementOf").empty())throw std::runtime_error("A replacement cannot use prepared ordinary publication.");const auto final=job->target();
  if(stage.parent_path().lexically_normal()!=final.parent_path().lexically_normal())throw std::runtime_error("The prepared download destination changed.");
  Json inputs=Json::object();for(const char* key:{"Folder","FileName","ProjectDestinationRoot"})if(job->data.contains(key))inputs[key]=job->data[key];
  staging=stage;parts=work;target=final;digest=hash;destination=std::move(inputs);cleanupIdentity=std::move(identity);keepParts=retain;ready=true;
 }
 void commit(){
  if(!ready||accepted)throw std::runtime_error("The prepared download is not awaiting publication.");
  {Lock lock(manager.mutex);Json inputs=Json::object();for(const char* key:{"Folder","FileName","ProjectDestinationRoot"})if(job->data.contains(key))inputs[key]=job->data[key];if(job->target()!=target||inputs!=destination)throw std::runtime_error("The prepared download destination changed before publication.");}
  if(!pendingWriteback())endFinalization(); // Keep redundant automatic IO coalesced through the owned body barrier.
  struct Committing {bool& flag;explicit Committing(bool& value):flag(value){flag=true;}~Committing(){flag=false;}};
  {Committing committing(inCommit);manager.publishFile(job,staging,digest,cancel.get());}
  accepted=true;
  const auto began=std::chrono::steady_clock::now();
  if(!keepParts)try{if(fs::exists(parts))for(const auto& entry:fs::directory_iterator(parts))if(entry.is_regular_file()&&(entry.path().extension()==L".part"||entry.path().extension()==L".data"))fs::remove(entry.path());}catch(...){/* Cleanup cannot revoke an accepted completion. */}
  cleanupSeconds=std::chrono::duration<double>(std::chrono::steady_clock::now()-began).count();
  if(manager.filePublished)try{manager.filePublished(job);}catch(...){/* Accepted output must not fail due to a UI callback. */}
 }
 ~PreparedTransferCommit(){
  if(advisoryCancel)advisoryCancel->stop=true;
  if(advisoryHash.valid())try{advisoryHash.wait();}catch(...){}
  joinWriteback(); // Never delete staging or retire ownership while IO is live.
  endFinalization();
  if(ready&&!accepted)try{if(!publicationNeedsRecovery(manager,job)&&!cleanupIdentity.is_null()){
   writebackPin.reset(); // The task has joined; let cleanup exclude all writers.
   Handle file(CreateFileW(staging.c_str(),DELETE|FILE_READ_ATTRIBUTES,FILE_SHARE_READ,nullptr,OPEN_EXISTING,FILE_FLAG_OPEN_REPARSE_POINT,nullptr));
   if(file&&stableFileIdentity(file.h)==cleanupIdentity){FILE_DISPOSITION_INFO disposition{TRUE};SetFileInformationByHandle(file.h,FileDispositionInfo,&disposition,sizeof(disposition));}
  }}catch(...){/* Missing ownership proof or uncertain recovery retains staging. */}
  current=previous;
  if(independentOwned){auto& state=finalizing();std::lock_guard<std::mutex> lock(state.mutex);const auto found=state.independent.find(&manager);if(found!=state.independent.end()){const auto owner=found->second.find(cancel.get());if(owner!=found->second.end()&&owner->second.second==this)found->second.erase(owner);if(found->second.empty())state.independent.erase(found);}}
 }
};
}
