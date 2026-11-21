#pragma once
#include "QueueChoiceTask.hpp"
#include "FileInfoDecision.hpp"
namespace udm {
struct BatchAdmissionDraft {std::vector<BatchDownloadEntry> entries;std::string queue,category;bool queueOnly=true;std::optional<DownloadQueueChoice> choice;};
using BatchAdmissionObserver=std::function<void(const char*)>;
inline FileInfoDecisionResult applyBatchAdmission(Manager& manager,const BatchAdmissionDraft& draft,const BatchAdmissionObserver& observe={}){
 if(observe)observe("before-offer");
 const auto jobs=manager.offerDownloadBatch(draft.entries,draft.choice?draft.choice->name:draft.queue,bool(draft.choice)||draft.queueOnly,draft.category,draft.choice?&*draft.choice:nullptr);
 FileInfoDecisionResult result;
 if(draft.choice&&draft.choice->start)try{if(observe)observe("before-start");manager.startQueueForDownloads(draft.choice->name,jobs);}
 catch(const std::exception& error){result.warning="The batch was saved, but queue processing could not start. "+std::string(error.what());}
 return result;
}
struct BatchPreparationResult {std::string queue,error;QueueChoiceView queues;bool prompt=false,cancelled=false;};
using BatchPreparationObserver=std::function<void(const char*)>;
// Advisory preparation does not claim the accepted command guard or change jobs.
class BatchPreparationTask {
 std::future<BatchPreparationResult> future;std::thread worker;std::atomic_bool cancelled=false;std::mutex mutex;std::condition_variable wake;
public:
 BatchPreparationTask()=default;BatchPreparationTask(const BatchPreparationTask&)=delete;BatchPreparationTask& operator=(const BatchPreparationTask&)=delete;~BatchPreparationTask(){finish();}
 bool valid()const noexcept{return future.valid();}bool ready()const{return valid()&&future.wait_for(std::chrono::seconds(0))==std::future_status::ready;}
 void start(Manager& manager,bool browser,std::string initial,std::function<void()> notify={},BatchPreparationObserver observe={}){
  if(valid()||worker.joinable())throw std::runtime_error("This batch is already being prepared.");cancelled=false;std::promise<BatchPreparationResult> promise;future=promise.get_future();
  try{worker=std::thread([this,&manager,browser,initial=std::move(initial),notify=std::move(notify),observe=std::move(observe),promise=std::move(promise)]()mutable{
   BatchPreparationResult result;
   try{for(;;){
    if(cancelled){result.cancelled=true;break;}
    std::unique_lock lock(manager.mutex,std::try_to_lock);
    if(lock.owns_lock()&&!manager.fileInfoDecisionPending){
     if(observe)observe("before-read");const auto& queues=manager.state["Queues"];result.queue=browser?primaryDownloadQueueName(queues):initial;
     if(result.queue.empty())throw std::runtime_error("Choose or create a download queue in Scheduler.");
     result.prompt=!browser&&yes(manager.state["Settings"],"QueuePromptBatch",true);
     if(result.prompt){for(const auto& queue:queues)result.queues.names.push_back(str(queue,"Name"));result.queues.labels=queueDisplayLabels(queues);}break;
    }
    if(lock.owns_lock())lock.unlock();if(observe)observe("waiting-model");std::unique_lock waiting(mutex);wake.wait_for(waiting,std::chrono::milliseconds(10),[&]{return cancelled.load();});
   }result.cancelled=result.cancelled||cancelled.load();if(observe&&!result.cancelled)observe("captured");}
   catch(const std::exception& error){result.error=error.what();result.cancelled=cancelled;}catch(...){result.error="The batch could not be prepared. Try again.";result.cancelled=cancelled;}
   promise.set_value(std::move(result));if(notify)try{notify();}catch(...){}
  });}catch(...){future={};throw;}
 }
 void cancelRead()noexcept{cancelled=true;wake.notify_all();}
 BatchPreparationResult take(){if(!ready())throw std::runtime_error("The batch is still being prepared.");auto result=future.get();if(worker.joinable())worker.join();return result;}
 void finish()noexcept{cancelRead();if(worker.joinable())worker.join();future={};}
};
#ifdef UDM_TOOLBAR_COMPONENT_TEST
inline BatchPreparationObserver batchPreparationTestObserver;
inline BatchAdmissionObserver batchAdmissionTestObserver;
#endif
}
