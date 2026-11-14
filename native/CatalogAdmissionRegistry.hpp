#pragma once
#include "CatalogAdmissionReservation.hpp"
namespace udm {
// Transient ownership only. Durable BrowserCaptures remains the authority after
// retirement/restart. No method takes Manager locks or performs filesystem I/O.
class CatalogAdmissionRegistry {
 struct Entry {std::string binding,selection;std::shared_ptr<const CatalogAdmissionReservation> prepared;std::atomic<unsigned long long> durable{0};size_t publications=0;bool owner=true,installed=false,resolving=false;Entry(std::string value,std::string identity):binding(std::move(value)),selection(std::move(identity)){};};
 struct State {std::mutex mutex;std::condition_variable changed;std::map<std::string,std::shared_ptr<Entry>> entries;};
 std::shared_ptr<State> state=std::make_shared<State>();
 static void retire(State& state,const std::string& token,const std::shared_ptr<Entry>& entry){
  if(entry->owner||entry->publications||(entry->durable.load(std::memory_order_acquire)&&!entry->installed))return;auto found=state.entries.find(token);if(found!=state.entries.end()&&found->second==entry)state.entries.erase(found);
 }
public:
 enum class Phase { Missing, Preparing, Prepared, Resolving, Committed };
 struct Status {Phase phase=Phase::Missing;unsigned long long durableSequence=0;};
 class Lease {
  friend class CatalogAdmissionRegistry;
  std::shared_ptr<State> state;std::shared_ptr<Entry> entry;std::string token;
  Lease(std::shared_ptr<State> owner,std::string operation,std::shared_ptr<Entry> value):state(std::move(owner)),entry(std::move(value)),token(std::move(operation)){}
 public:
  Lease()=default;Lease(const Lease&)=delete;Lease& operator=(const Lease&)=delete;
  Lease(Lease&&)=default;
  Lease& operator=(Lease&& other){if(this!=&other){release();state=std::move(other.state);entry=std::move(other.entry);token=std::move(other.token);}return *this;}
  ~Lease(){release();}
  void release(){if(!state||!entry)return;{std::lock_guard<std::mutex> lock(state->mutex);entry->owner=false;retire(*state,token,entry);}entry.reset();state.reset();}
  bool committed()const noexcept{return entry&&entry->durable.load(std::memory_order_acquire)!=0;}
  // Caller must not hold Manager's lock: a captured writer may need it before
  // retiring its ticket. Freeze new uncommitted captures, then resolve success
  // or wait for all already-captured writes to become terminal.
  bool resolve(){if(!state||!entry)return false;std::unique_lock<std::mutex> lock(state->mutex);entry->resolving=true;state->changed.wait(lock,[&]{return committed()||entry->publications==0;});return committed();}
  // Caller holds Manager's lock and has installed the accepted job/receipt in
  // live state. Until this handoff, even a committed entry remains an overlay.
  void installed(){if(!state||!entry||!committed())throw std::runtime_error("Media acceptance has not been published.");std::lock_guard<std::mutex> lock(state->mutex);entry->installed=true;}
  void prepare(std::shared_ptr<const CatalogAdmissionReservation> candidate){
   if(!state||!entry||!candidate||candidate->operation()!=token||candidate->binding()!=entry->binding)throw std::runtime_error("The selected media operation changed.");
   std::lock_guard<std::mutex> lock(state->mutex);
   if(!entry->owner)throw std::runtime_error("This media operation is no longer active.");
   if(entry->prepared){if(entry->prepared==candidate)return;throw std::runtime_error("This media operation is already prepared.");}
   for(const auto& item:state->entries){const auto& other=item.second;if(other==entry||!other->prepared||other->installed)continue;
    if(candidate->jobId()==other->prepared->jobId()||(candidate->exclusiveDestination()&&other->prepared->exclusiveDestination()&&candidate->sharesDestination(*other->prepared)))throw std::runtime_error("Another pending media operation is using this download or destination.");
   }
   entry->prepared=std::move(candidate);
  }
 };
 class Publication {
  friend class CatalogAdmissionRegistry;
  std::shared_ptr<State> state;std::vector<std::pair<std::string,std::shared_ptr<Entry>>> entries;
  Publication(std::shared_ptr<State> owner,std::vector<std::pair<std::string,std::shared_ptr<Entry>>> included):state(std::move(owner)),entries(std::move(included)){}
  void finish(){if(!state)return;{std::lock_guard<std::mutex> lock(state->mutex);for(const auto& item:entries){--item.second->publications;retire(*state,item.first,item.second);}}state->changed.notify_all();entries.clear();state.reset();}
 public:
  Publication()=default;Publication(const Publication&)=delete;Publication& operator=(const Publication&)=delete;Publication(Publication&&)=default;
  Publication& operator=(Publication&& other){if(this!=&other){finish();state=std::move(other.state);entries=std::move(other.entries);}return *this;}
  ~Publication(){finish();}
  // Call only after these exact snapshot bytes have been durably published.
  // Preallocated entry references make this post-write proof allocation-free.
  void published(unsigned long long sequence)noexcept{
   if(!sequence)return;
   for(const auto& item:entries){auto& value=item.second->durable;auto old=value.load(std::memory_order_relaxed);while(old<sequence&&!value.compare_exchange_weak(old,sequence,std::memory_order_release,std::memory_order_relaxed)){};}
   // Synchronize with resolve's predicate/wait boundary so a successful write
   // cannot notify just before the resolver begins sleeping.
   if(state){std::lock_guard<std::mutex> lock(state->mutex);state->changed.notify_all();}
  }
 };
 Lease acquire(std::string token,std::string binding,std::string selection={}){
  if(token.empty()||binding.empty())throw std::runtime_error("Invalid media operation identity.");
  std::lock_guard<std::mutex> lock(state->mutex);if(state->entries.count(token))throw std::runtime_error("This media operation is already being processed.");
  if(!selection.empty())for(const auto& item:state->entries)if(!item.second->installed&&item.second->selection==selection)throw std::runtime_error("This video selection is already being accepted. Wait for its download to appear.");
  auto entry=std::make_shared<Entry>(std::move(binding),std::move(selection));state->entries.emplace(token,entry);return Lease(state,std::move(token),std::move(entry));
 }
 Publication overlay(Json& snapshot){
  std::lock_guard<std::mutex> lock(state->mutex);std::vector<std::pair<std::string,std::shared_ptr<Entry>>> included;included.reserve(state->entries.size());
  // Do not let a newer snapshot omit an entry while a captured write might
  // already have published it but not yet delivered its success notification.
  for(const auto& item:state->entries){const auto& entry=*item.second;if(entry.prepared&&!entry.installed&&entry.resolving&&!entry.durable.load(std::memory_order_acquire)&&entry.publications)throw std::runtime_error("A media acceptance outcome is still being resolved. Retry shortly.");}
  for(const auto& item:state->entries)if(item.second->prepared&&!item.second->installed&&(!item.second->resolving||item.second->durable.load(std::memory_order_acquire))){item.second->prepared->overlay(snapshot);included.push_back(item);}
  for(const auto& item:included)++item.second->publications;
  return Publication(state,std::move(included));
 }
 Status lookup(const std::string& token)const{
  std::lock_guard<std::mutex> lock(state->mutex);auto found=state->entries.find(token);if(found==state->entries.end())return {};
  const auto& entry=*found->second;const auto durable=entry.durable.load(std::memory_order_acquire);return {durable?Phase::Committed:entry.resolving?Phase::Resolving:entry.prepared?Phase::Prepared:Phase::Preparing,durable};
 }
 bool reservesDirectory(const fs::path& path)const{std::lock_guard<std::mutex> lock(state->mutex);for(const auto& item:state->entries){const auto& entry=*item.second;if(entry.prepared&&!entry.installed&&entry.prepared->exclusiveDestination()&&entry.prepared->requiresDirectory(path))return true;}return false;}
 bool reserves(const fs::path& path,const std::string& ignoreJob={})const{
  std::lock_guard<std::mutex> lock(state->mutex);for(const auto& item:state->entries){const auto& entry=*item.second;
   if(entry.prepared&&!entry.installed&&entry.prepared->jobId()!=ignoreJob&&entry.prepared->exclusiveDestination()&&entry.prepared->ownsDestination(path))return true;
  }return false;
 }
};
}
