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
  struct Session {std::mutex mutex;std::weak_ptr<const Operation> current;bool advanced=false;const std::string fingerprint;std::string cancelError;uint64_t cancelAttempt=0;std::atomic_bool retryQueued{false},cancelled{false},cancelFinished{false},cancelFailed{false};explicit Session(std::string binding):fingerprint(std::move(binding)){};};
  std::shared_ptr<Session> session;
  std::atomic_bool& retryQueued;
  std::atomic_bool workerFinished{false};
  void wake()const noexcept;
 public:
  const Key key;
  const std::shared_ptr<const FileInfoInitialView> view;
  const std::string token;
  std::atomic_bool& cancelFinished;std::atomic_bool& cancelFailed;
  Operation(Key id,std::shared_ptr<const FileInfoInitialView> input,std::string receipt,std::shared_ptr<State> state,std::shared_ptr<const RetryHandler> retry={},std::shared_ptr<Session> presentation={}):owner(state),retryHandler(std::move(retry)),session(presentation?std::move(presentation):std::make_shared<Session>(input?input->admissionFingerprint:std::string{})),retryQueued(session->retryQueued),key(id),view(std::move(input)),token(std::move(receipt)),cancelFinished(session->cancelFinished),cancelFailed(session->cancelFailed){}
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
  const std::string& fingerprint()const noexcept{return session->fingerprint;}
 };
 struct CancellationFailure {std::shared_ptr<const Operation> operation;uint64_t attempt=0;std::string message;explicit operator bool()const noexcept{return operation!=nullptr;}};
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
 struct State {std::mutex mutex;std::condition_variable changed;bool stopped=false,workerStopping=false,retryStopping=false;std::map<Key,std::shared_ptr<Operation>> entries,active,retryPresentations,failedCancellations;std::vector<std::shared_ptr<const Operation>> retries;};
 std::shared_ptr<State> state=std::make_shared<State>();std::thread cancellationWorker,retryWorker;
 static Key allocate(){auto value=next.load(std::memory_order_relaxed);for(;;){if(value==std::numeric_limits<Key>::max())throw std::runtime_error("File Info message identities exhausted.");if(next.compare_exchange_weak(value,value+1,std::memory_order_relaxed))return value;}}
 // State -> Session is the only nested locking order. All allocations precede
 // retirement, so a failed successor leaves the previous presentation intact.
 static void replacePresentation(State& state,const Operation& parent,const std::shared_ptr<Operation>& operation){
  std::lock_guard<std::mutex> lock(parent.session->mutex);auto previous=parent.session->current.lock();if(!previous)previous=parent.shared_from_this();
  if(!parent.session->fingerprint.empty()&&!operation->view->admissionFingerprint.empty()&&parent.session->fingerprint!=operation->view->admissionFingerprint)throw std::runtime_error("The prepared media selection differs from this File Info session.");
  if(previous->decision.load()==Decision::Attached)throw std::runtime_error("This File Info presentation is already attached.");
  state.active.emplace(operation->key,operation);try{state.retryPresentations.emplace(operation->key,operation);}catch(...){state.active.erase(operation->key);throw;}
  // If the original posted key has not been dispatched, it must open this
  // successor rather than leave the private claim without a visible dialog.
  for(auto& entry:state.entries)if(entry.second->session==parent.session)entry.second=operation;
  state.active.erase(previous->key);state.retryPresentations.erase(previous->key);state.active.erase(parent.key);state.retryPresentations.erase(parent.key);
  parent.session->current=operation;parent.session->advanced=true;if(parent.session->cancelled)operation->decision=Decision::Cancel;
 }
public:
 ~PendingFileInfoInbox(){close();stopWorker();}
 void startWorker(std::function<void(const Operation&)> cancel,std::function<void()> failedNotify={}){
  if(cancellationWorker.joinable())return;
  if(!cancel)throw std::runtime_error("Missing pending File Info cancellation handler.");
  auto shared=state;cancellationWorker=std::thread([shared,cancel=std::move(cancel),failedNotify=std::move(failedNotify)]{
   std::unique_lock<std::mutex> lock(shared->mutex);
   for(;;){
    std::shared_ptr<Operation> pending;
    for(auto entry=shared->active.begin();entry!=shared->active.end();){
     const auto decision=entry->second->decision.load(std::memory_order_acquire);
     if(decision==Decision::Attached){shared->retryPresentations.erase(entry->first);entry=shared->active.erase(entry);continue;}
     if(decision==Decision::Cancel&&entry->second->outcome()!=Outcome::Pending&&!entry->second->retrying()){pending=entry->second;break;}
     ++entry;
    }
    if(pending){
     // Keep the session discoverable while its background cancellation waits
     // for durable ownership. A late validated producer must inherit Cancel,
     // not post another File Info notification after the original closes.
     lock.unlock();try{cancel(*pending);}catch(const std::exception& e){pending->cancelFailed=true;try{std::lock_guard<std::mutex> session(pending->session->mutex);pending->session->cancelError.assign(e.what(),std::min<size_t>(std::char_traits<char>::length(e.what()),2048));}catch(...){}}catch(...){pending->cancelFailed=true;}lock.lock();
     for(auto entry=shared->entries.begin();entry!=shared->entries.end();)if(entry->second->session==pending->session)entry=shared->entries.erase(entry);else ++entry;
     decltype(shared->active)::node_type failed;
     for(auto entry=shared->active.begin();entry!=shared->active.end();)if(entry->second->session==pending->session){shared->retryPresentations.erase(entry->first);if(pending->cancelFailed&&failed.empty()){auto current=entry++;failed=shared->active.extract(current);}else entry=shared->active.erase(entry);}else ++entry;
     if(!failed.empty())shared->failedCancellations.insert(std::move(failed));
     pending->cancelFinished=true;shared->changed.notify_all();
     if(pending->cancelFailed&&failedNotify){lock.unlock();try{failedNotify();}catch(...){}lock.lock();}continue;
    }
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
     {std::lock_guard<std::mutex> lock(shared->mutex);replacePresentation(*shared,*parent,operation);closing=shared->stopped;}
     ticket=Ticket(operation);child=std::move(operation);
     if(parent->cancelRequested()||closing)child->requestCancel();
    };
    const PreparedFileInfoSink prepared=deliver;
    const RejectedFileInfoSink rejected=[&](const std::string& id){ticket.rejected(id);};
    try{auto accepted=(*parent->retryHandler)(prepared,rejected);if(!accepted)throw std::runtime_error("Retry did not return an accepted view.");if(!child){auto current=parent->latest();if(current!=parent&&current->view->jobId==accepted->jobId){child=current;child->result.store(Outcome::Accepted,std::memory_order_release);bool closing;{std::lock_guard<std::mutex> lock(shared->mutex);closing=shared->stopped;}if(closing)child->requestCancel();}else deliver(accepted);}ticket.accepted(accepted->jobId);}
    catch(...){
     // Failure before a prepared identity can include an accepted-but-unreadable
     // receipt. Never turn this uncertainty into permission for another retry.
     if(!child)try{auto current=parent->latest();if(current!=parent){child=current;auto result=child->result.load();while(result!=Outcome::Accepted&&result!=Outcome::Uncertain&&!child->result.compare_exchange_weak(result,Outcome::Uncertain,std::memory_order_release)){};}else deliver(parent->view);}catch(...){parent->result.store(Outcome::Uncertain,std::memory_order_release);}
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
  std::shared_ptr<Operation> operation;bool coalesced=false;
  {std::lock_guard<std::mutex> lock(state->mutex);if(state->stopped)throw std::runtime_error("File Info delivery has closed.");std::shared_ptr<Operation> previous;
   if(!token.empty())for(const auto& failure:state->failedCancellations)if(failure.second->token==token)throw std::runtime_error("The earlier cancellation needs attention before this download can be presented again.");
   // A nonempty receipt is already validated by Manager before this sink runs.
   // Untagged deliveries remain independent. Never replace an attached window.
   if(!token.empty())for(const auto& entry:state->active)if(entry.second->token==token){previous=entry.second;break;}
   if(previous){const auto outcome=previous->outcome();if(previous->decision.load()==Decision::Attached||((outcome!=Outcome::Rejected&&outcome!=Outcome::Uncertain)&&!previous->retrying()))throw std::runtime_error("This File Info receipt is already being presented.");operation=std::make_shared<Operation>(allocate(),std::move(view),std::move(token),state,previous->retryHandler?previous->retryHandler:std::move(retry),previous->session);replacePresentation(*state,*previous,operation);coalesced=true;}
   else{if(state->active.size()+state->failedCancellations.size()>=64)throw std::runtime_error("Too many File Info deliveries are pending.");operation=std::make_shared<Operation>(allocate(),std::move(view),std::move(token),state,std::move(retry));state->active.emplace(operation->key,operation);try{state->entries.emplace(operation->key,operation);}catch(...){state->active.erase(operation->key);throw;}{std::lock_guard<std::mutex> presentation(operation->session->mutex);operation->session->current=operation;}}
  }
  if(coalesced){operation->wake();return Ticket(std::move(operation));}
  try{if(!notify(operation->key))throw std::runtime_error("Cannot notify File Info delivery.");}
  catch(...){operation->result.store(Outcome::Uncertain,std::memory_order_release);std::lock_guard<std::mutex> lock(state->mutex);state->entries.erase(operation->key);state->active.erase(operation->key);throw;}
  return Ticket(std::move(operation));
 }
 std::shared_ptr<const Operation> take(Key key){std::lock_guard<std::mutex> lock(state->mutex);auto found=state->entries.find(key);if(found==state->entries.end())return {};auto result=std::move(found->second);state->entries.erase(found);return result;}
 // The UI timer can discover a committed offer before its posted key is
 // dispatched. Claim that same operation by identity; the queued key goes stale.
 std::shared_ptr<const Operation> takeForJob(const std::string& id){std::lock_guard<std::mutex> lock(state->mutex);for(auto found=state->entries.begin();found!=state->entries.end();++found)if(found->second->view->jobId==id){auto result=std::move(found->second);state->entries.erase(found);return result;}return {};}
 bool closed(){std::lock_guard<std::mutex> lock(state->mutex);return state->stopped;}
 CancellationFailure cancellationFailure(std::shared_ptr<const Operation> member={})const{
  std::lock_guard<std::mutex> lock(state->mutex);auto found=state->failedCancellations.begin();if(member)while(found!=state->failedCancellations.end()&&found->second->session!=member->session)++found;if(found==state->failedCancellations.end())return {};auto operation=found->second;std::lock_guard<std::mutex> session(operation->session->mutex);
  return {operation,operation->session->cancelAttempt,operation->session->cancelError.empty()?"UDM could not save the cancellation. The download may still be present.":operation->session->cancelError};
 }
 bool retryCancellation(const CancellationFailure& failure){
  if(!failure)return false;std::lock_guard<std::mutex> lock(state->mutex);auto found=state->failedCancellations.find(failure.operation->key);if(state->stopped||state->workerStopping||found==state->failedCancellations.end()||found->second!=failure.operation)return false;auto operation=found->second;std::lock_guard<std::mutex> session(operation->session->mutex);
  if(operation->session->cancelAttempt!=failure.attempt||operation->session->cancelAttempt==std::numeric_limits<uint64_t>::max())return false;
  ++operation->session->cancelAttempt;operation->session->cancelError.clear();operation->cancelFailed=false;operation->cancelFinished=false;operation->session->current=operation;
  state->active.insert(state->failedCancellations.extract(found));state->changed.notify_all();return true;
 }
 bool dismissCancellationFailure(const CancellationFailure& failure){
  if(!failure)return false;std::lock_guard<std::mutex> lock(state->mutex);auto found=state->failedCancellations.find(failure.operation->key);if(found==state->failedCancellations.end()||found->second!=failure.operation||found->second->session->cancelAttempt!=failure.attempt)return false;state->failedCancellations.erase(found);return true;
 }
 bool ownsRetryPresentation(const std::string& id){std::lock_guard<std::mutex> lock(state->mutex);for(const auto& entry:state->retryPresentations)if(entry.second->view->jobId==id)return true;for(const auto& entry:state->active)if(entry.second->view->jobId==id&&entry.second->cancelRequested())return true;for(const auto& entry:state->failedCancellations)if(entry.second->view->jobId==id)return true;return false;}
 void close(){std::lock_guard<std::mutex> lock(state->mutex);state->stopped=true;state->entries.clear();}
};
inline void PendingFileInfoInbox::Operation::wake()const noexcept{if(auto state=owner.lock()){std::lock_guard<std::mutex> lock(state->mutex);state->changed.notify_all();}}
inline bool PendingFileInfoInbox::Operation::canRetry()const{if(!retryHandler||!finished()||outcome()!=Outcome::Rejected||retrying()||decision.load()!=Decision::None)return false;auto state=owner.lock();if(!state)return false;std::lock_guard<std::mutex> lock(state->mutex);auto active=state->active.find(key);return !state->stopped&&!state->workerStopping&&!state->retryStopping&&active!=state->active.end()&&active->second.get()==this&&latest().get()==this;}
inline bool PendingFileInfoInbox::Operation::requestRetry()const{if(!canRetry())return false;auto state=owner.lock();if(!state)return false;std::lock_guard<std::mutex> lock(state->mutex);auto active=state->active.find(key);if(state->stopped||state->workerStopping||state->retryStopping||state->retries.size()>=64||retrying()||active==state->active.end()||active->second.get()!=this||latest().get()!=this)return false;auto expected=Decision::None;if(!decision.compare_exchange_strong(expected,Decision::Retry))return false;retryQueued=true;try{state->retries.push_back(shared_from_this());}catch(...){auto retry=Decision::Retry;decision.compare_exchange_strong(retry,Decision::None);retryQueued=false;state->changed.notify_all();throw;}state->changed.notify_all();return true;}
inline void cancelPendingFileInfoOffer(Manager& manager,const PendingFileInfoInbox::Operation& operation){
 if(!operation.token.empty()&&!operation.fingerprint().empty()){manager.cancelBrowserMediaPresentation(operation.token,operation.fingerprint());return;}
 JobPtr job;
 {Lock lock(manager.mutex);for(const auto& candidate:manager.jobs)if(candidate->id()==operation.view->jobId)job=candidate;
  if(!job||str(job->data,"Status")!="Awaiting confirmation"||yes(job->data,"IndividualStart"))return;}
 manager.endPrefetch(job,false);
 if(!manager.cancelUnconfirmedOffer(job,Manager::OfferCancellation::AwaitingOnly))return;
 if(!operation.token.empty())manager.finishBrowserPresentation(Json{{"tokens",Json::array({operation.token})}});
}
}
