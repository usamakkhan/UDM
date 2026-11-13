#pragma once
#include "FileInfoInitialView.hpp"
#include "BrowserAdmissionContext.hpp"
#include <limits>
namespace udm {
// Only an integer key crosses the window message boundary. UI and worker keep
// independent shared ownership; neither object contains a JobPtr or window.
class PendingFileInfoInbox {
 struct State;
public:
 using Key=uint64_t;
 using RetryHandler=std::function<std::shared_ptr<const FileInfoInitialView>(const PreparedFileInfoSink&,const RejectedFileInfoSink&)>;
 enum class Outcome {Pending,Accepted,Rejected,Uncertain};
 enum class Decision {None,Attached,Cancel,Retry};
 class Ticket;
 class Operation:public std::enable_shared_from_this<Operation> {
  friend class PendingFileInfoInbox;
  friend class Ticket;
  mutable std::atomic<Outcome> result{Outcome::Pending};
  mutable std::atomic<Decision> decision{Decision::None};
  std::weak_ptr<State> owner;
  std::shared_ptr<const RetryHandler> retryHandler;
  struct Session {std::mutex mutex;std::weak_ptr<const Operation> current;bool advanced=false;std::atomic_bool cancelled{false},cancelFinished{false},cancelFailed{false};};
  std::shared_ptr<Session> session;
  mutable std::atomic_bool retryQueued{false};
  std::atomic_bool workerFinished{false};
  void wake()const noexcept;
 public:
  const Key key;
  const std::shared_ptr<const FileInfoInitialView> view;
  const std::string token;
  std::atomic_bool& cancelFinished;std::atomic_bool& cancelFailed;
  Operation(Key id,std::shared_ptr<const FileInfoInitialView> input,std::string receipt,std::shared_ptr<State> state,std::shared_ptr<const RetryHandler> retry={},std::shared_ptr<Session> presentation={}):owner(state),retryHandler(std::move(retry)),session(presentation?std::move(presentation):std::make_shared<Session>()),key(id),view(std::move(input)),token(std::move(receipt)),cancelFinished(session->cancelFinished),cancelFailed(session->cancelFailed){}
  Outcome outcome()const noexcept{return result.load(std::memory_order_acquire);}
  bool finished()const noexcept{return workerFinished.load(std::memory_order_acquire);}
  bool retrying()const noexcept{return retryQueued.load(std::memory_order_acquire);}
  bool supportsRetry()const noexcept{return retryHandler!=nullptr;}
  std::shared_ptr<const Operation> latest()const{std::lock_guard<std::mutex> lock(session->mutex);auto current=session->current.lock();return current?current:shared_from_this();}
  bool canRetry()const;
  bool requestRetry()const;
  bool attach()const noexcept{auto expected=Decision::None;const bool result=decision.compare_exchange_strong(expected,Decision::Attached,std::memory_order_acq_rel);wake();return result||expected==Decision::Attached;}
  bool requestCancel()const noexcept{std::shared_ptr<const Operation> current;bool cancelled=false;{std::lock_guard<std::mutex> lock(session->mutex);current=session->current.lock();if(!current){if(session->advanced)return false;current=shared_from_this();}auto expected=current->decision.load();while(expected==Decision::None||expected==Decision::Retry){if(current->decision.compare_exchange_weak(expected,Decision::Cancel,std::memory_order_acq_rel))break;}cancelled=current->decision.load()==Decision::Cancel;if(cancelled)session->cancelled=true;}current->wake();return cancelled;}
  bool cancelRequested()const noexcept{return session->cancelled.load(std::memory_order_acquire);}
 };
 class Ticket {
  std::shared_ptr<Operation> operation;
  friend class PendingFileInfoInbox;
  explicit Ticket(std::shared_ptr<Operation> value):operation(std::move(value)){}
  void uncertain()noexcept{if(operation){auto pending=Outcome::Pending;operation->result.compare_exchange_strong(pending,Outcome::Uncertain,std::memory_order_release);operation->workerFinished.store(true,std::memory_order_release);operation->wake();}}
 public:
  Ticket()=default;Ticket(const Ticket&)=delete;Ticket& operator=(const Ticket&)=delete;Ticket(Ticket&&)=default;
  Ticket& operator=(Ticket&& other)noexcept{if(this!=&other){uncertain();operation=std::move(other.operation);}return *this;}
  ~Ticket(){uncertain();}
  void accepted(const std::string& id)noexcept{if(operation&&operation->view->jobId==id){operation->result.store(Outcome::Accepted,std::memory_order_release);operation->wake();}}
  void rejected(const std::string& id)noexcept{if(operation&&operation->view->jobId==id){auto pending=Outcome::Pending;operation->result.compare_exchange_strong(pending,Outcome::Rejected,std::memory_order_release);operation->wake();}}
 };
private:
 inline static std::atomic<Key> next{1};
 struct State {std::mutex mutex;std::condition_variable changed;bool stopped=false,workerStopping=false,retryStopping=false;std::map<Key,std::shared_ptr<Operation>> entries,active,retryPresentations;std::vector<std::shared_ptr<const Operation>> retries;};
 std::shared_ptr<State> state=std::make_shared<State>();std::thread cancellationWorker,retryWorker;
 static Key allocate(){auto value=next.load(std::memory_order_relaxed);for(;;){if(value==std::numeric_limits<Key>::max())throw std::runtime_error("File Info message identities exhausted.");if(next.compare_exchange_weak(value,value+1,std::memory_order_relaxed))return value;}}
public:
 ~PendingFileInfoInbox(){close();stopWorker();}
 void startWorker(std::function<void(const Operation&)> cancel){
  if(cancellationWorker.joinable())return;
  if(!cancel)throw std::runtime_error("Missing pending File Info cancellation handler.");
  auto shared=state;cancellationWorker=std::thread([shared,cancel=std::move(cancel)]{
   std::unique_lock<std::mutex> lock(shared->mutex);
   for(;;){
    std::shared_ptr<Operation> pending;
    for(auto entry=shared->active.begin();entry!=shared->active.end();){
     const auto decision=entry->second->decision.load(std::memory_order_acquire);
     if(decision==Decision::Attached){shared->retryPresentations.erase(entry->first);entry=shared->active.erase(entry);continue;}
     if(decision==Decision::Cancel&&entry->second->outcome()!=Outcome::Pending&&!entry->second->retrying()){pending=entry->second;shared->active.erase(entry);break;}
     ++entry;
    }
    if(pending){lock.unlock();try{cancel(*pending);}catch(...){pending->cancelFailed=true;}pending->cancelFinished=true;lock.lock();shared->retryPresentations.erase(pending->key);continue;}
    if(shared->workerStopping)return;
    shared->changed.wait(lock);
   }
  });
  try{retryWorker=std::thread([this,shared]{
   for(;;){
    std::shared_ptr<const Operation> parent;
    {std::unique_lock<std::mutex> lock(shared->mutex);shared->changed.wait(lock,[&]{return shared->retryStopping||!shared->retries.empty();});if(shared->retryStopping)return;parent=std::move(shared->retries.front());shared->retries.erase(shared->retries.begin());if(shared->stopped){parent->retryQueued=false;shared->changed.notify_all();continue;}}
    if(parent->cancelRequested()){parent->retryQueued=false;parent->wake();continue;}
    Ticket ticket;std::shared_ptr<const Operation> child;
    const auto deliver=[&](std::shared_ptr<const FileInfoInitialView> view){
     if(child)throw std::runtime_error("Retry delivered more than one prepared identity.");
     if(!view)throw std::runtime_error("Retry did not return an accepted view.");
     auto operation=std::make_shared<Operation>(allocate(),std::move(view),parent->token,shared,parent->retryHandler,parent->session);bool closing=false;
     {std::lock_guard<std::mutex> lock(shared->mutex);shared->active.emplace(operation->key,operation);try{shared->retryPresentations.emplace(operation->key,operation);}catch(...){shared->active.erase(operation->key);throw;}shared->active.erase(parent->key);shared->retryPresentations.erase(parent->key);closing=shared->stopped;}
     ticket=Ticket(operation);child=std::move(operation);
     {std::lock_guard<std::mutex> lock(parent->session->mutex);parent->session->current=child;parent->session->advanced=true;if(parent->session->cancelled)child->decision=Decision::Cancel;}
     if(parent->cancelRequested()||closing)child->requestCancel();
    };
    const PreparedFileInfoSink prepared=deliver;
    const RejectedFileInfoSink rejected=[&](const std::string& id){ticket.rejected(id);};
    try{auto accepted=(*parent->retryHandler)(prepared,rejected);if(!accepted)throw std::runtime_error("Retry did not return an accepted view.");if(!child)deliver(accepted);ticket.accepted(accepted->jobId);}
    catch(...){
     // Failure before a prepared identity can include an accepted-but-unreadable
     // receipt. Never turn this uncertainty into permission for another retry.
     if(!child)try{deliver(parent->view);}catch(...){parent->result.store(Outcome::Uncertain,std::memory_order_release);}
    }
    ticket=Ticket{};parent->retryQueued=false;parent->wake();
   }
  });}catch(...){stopWorker();throw;}
 }
 void stopWorker(){
  {std::lock_guard<std::mutex> lock(state->mutex);state->retryStopping=true;}state->changed.notify_all();
  if(retryWorker.joinable())retryWorker.join();
  {std::lock_guard<std::mutex> lock(state->mutex);state->workerStopping=true;for(const auto& pending:state->retries)pending->retryQueued=false;}state->changed.notify_all();
  if(cancellationWorker.joinable())cancellationWorker.join();
  std::lock_guard<std::mutex> lock(state->mutex);state->retries.clear();state->active.clear();state->retryPresentations.clear();
 }
 Ticket publish(std::shared_ptr<const FileInfoInitialView> view,const std::function<bool(Key)>& notify,std::string token={},std::shared_ptr<const RetryHandler> retry={}){
  if(!view||view->jobId.empty()||!notify)throw std::runtime_error("Invalid prepared File Info delivery.");
  auto operation=std::make_shared<Operation>(allocate(),std::move(view),std::move(token),state,std::move(retry));
  {std::lock_guard<std::mutex> lock(state->mutex);if(state->stopped)throw std::runtime_error("File Info delivery has closed.");if(state->active.size()>=64)throw std::runtime_error("Too many File Info deliveries are pending.");state->active.emplace(operation->key,operation);try{state->entries.emplace(operation->key,operation);}catch(...){state->active.erase(operation->key);throw;}}
  try{if(!notify(operation->key))throw std::runtime_error("Cannot notify File Info delivery.");}
  catch(...){operation->result.store(Outcome::Uncertain,std::memory_order_release);std::lock_guard<std::mutex> lock(state->mutex);state->entries.erase(operation->key);state->active.erase(operation->key);throw;}
  return Ticket(std::move(operation));
 }
 std::shared_ptr<const Operation> take(Key key){std::lock_guard<std::mutex> lock(state->mutex);auto found=state->entries.find(key);if(found==state->entries.end())return {};auto result=std::move(found->second);state->entries.erase(found);return result;}
 // The UI timer can discover a committed offer before its posted key is
 // dispatched. Claim that same operation by identity; the queued key goes stale.
 std::shared_ptr<const Operation> takeForJob(const std::string& id){std::lock_guard<std::mutex> lock(state->mutex);for(auto found=state->entries.begin();found!=state->entries.end();++found)if(found->second->view->jobId==id){auto result=std::move(found->second);state->entries.erase(found);return result;}return {};}
 bool closed(){std::lock_guard<std::mutex> lock(state->mutex);return state->stopped;}
 bool ownsRetryPresentation(const std::string& id){std::lock_guard<std::mutex> lock(state->mutex);for(const auto& entry:state->retryPresentations)if(entry.second->view->jobId==id)return true;return false;}
 void close(){std::lock_guard<std::mutex> lock(state->mutex);state->stopped=true;state->entries.clear();}
};
inline void PendingFileInfoInbox::Operation::wake()const noexcept{if(auto state=owner.lock()){std::lock_guard<std::mutex> lock(state->mutex);state->changed.notify_all();}}
inline bool PendingFileInfoInbox::Operation::canRetry()const{if(!retryHandler||!finished()||outcome()!=Outcome::Rejected||retrying()||decision.load()!=Decision::None)return false;auto state=owner.lock();if(!state)return false;std::lock_guard<std::mutex> lock(state->mutex);return !state->stopped&&!state->workerStopping&&!state->retryStopping;}
inline bool PendingFileInfoInbox::Operation::requestRetry()const{if(!canRetry())return false;auto state=owner.lock();if(!state)return false;std::lock_guard<std::mutex> lock(state->mutex);if(state->stopped||state->workerStopping||state->retryStopping||state->retries.size()>=64)return false;auto expected=Decision::None;if(!decision.compare_exchange_strong(expected,Decision::Retry))return false;retryQueued=true;try{state->retries.push_back(shared_from_this());}catch(...){auto retry=Decision::Retry;decision.compare_exchange_strong(retry,Decision::None);retryQueued=false;state->changed.notify_all();throw;}state->changed.notify_all();return true;}
inline void cancelPendingFileInfoOffer(Manager& manager,const PendingFileInfoInbox::Operation& operation){
 JobPtr job;
 {Lock lock(manager.mutex);for(const auto& candidate:manager.jobs)if(candidate->id()==operation.view->jobId)job=candidate;
  if(!job||str(job->data,"Status")!="Awaiting confirmation"||yes(job->data,"IndividualStart"))return;}
 manager.endPrefetch(job,false);
 {Lock lock(manager.mutex);if(std::find(manager.jobs.begin(),manager.jobs.end(),job)==manager.jobs.end()||str(job->data,"Status")!="Awaiting confirmation"||yes(job->data,"IndividualStart"))return;
  manager.remove(job,false,true);}
 if(!operation.token.empty())manager.finishBrowserPresentation(Json{{"tokens",Json::array({operation.token})}});
}
}
