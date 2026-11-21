#pragma once
#include "QueueChoiceTask.hpp"
#include "BatchRefreshProjection.hpp"
namespace udm {
enum class BatchInitialKind {Initial,Preview};
struct BatchInitialResult {Json settings=Json::object();std::vector<std::string> categories;QueueChoiceView queues;BatchRowProjection projection;BatchRefreshProjection refresh;std::shared_ptr<BatchDownloadPreview> refreshedPreview;bool cancelled=false;std::string error;};
using BatchInitialObserver=std::function<void(const char*)>;
class BatchInitialTask {
 std::future<BatchInitialResult> future;std::thread worker;std::atomic_bool cancelled=false;std::mutex mutex;std::condition_variable wake;
public:
 BatchInitialTask()=default;BatchInitialTask(const BatchInitialTask&)=delete;BatchInitialTask& operator=(const BatchInitialTask&)=delete;~BatchInitialTask(){finish();}
 bool valid()const noexcept{return future.valid();}bool ready()const{return valid()&&future.wait_for(std::chrono::seconds(0))==std::future_status::ready;}
 // Optional immutable input containers are borrowed only until take/finish joins
 // this scoped reader. selectBatch keeps its caller inputs alive for that scope.
 void start(Manager& manager,std::function<void()> notify={},BatchInitialObserver observe={},BatchInitialKind kind=BatchInitialKind::Initial,const std::vector<std::string>* urls=nullptr,const std::vector<BrowserBatchRow>* browser=nullptr,Headers headers={},std::shared_ptr<const BatchRefreshInput> refresh={},std::shared_ptr<BatchDownloadPreview> previous={}){
  if(valid()||worker.joinable())throw std::runtime_error("Batch options are already being prepared.");cancelled=false;std::promise<BatchInitialResult> promise;future=promise.get_future();
  try{worker=std::thread([this,&manager,notify=std::move(notify),observe=std::move(observe),kind,urls,browser,headers=std::move(headers),refresh=std::move(refresh),previous=std::move(previous),promise=std::move(promise)]()mutable{
   BatchInitialResult result;
   try{if(refresh&&refresh->changed&&previous){result.settings=refresh->preferences;}else for(;;){
    if(cancelled){result.cancelled=true;break;}
    std::unique_lock lock(manager.mutex,std::try_to_lock);
    if(lock.owns_lock()&&!manager.fileInfoDecisionPending){
     if(observe)observe("before-read");result.settings=manager.state["Settings"];
     if(kind==BatchInitialKind::Initial){result.categories=manager.categories();const auto& queues=manager.state["Queues"];for(const auto& queue:queues)result.queues.names.push_back(str(queue,"Name"));result.queues.labels=queueDisplayLabels(queues);}break;
    }
    if(lock.owns_lock())lock.unlock();if(observe)observe("waiting-model");std::unique_lock waiting(mutex);wake.wait_for(waiting,std::chrono::milliseconds(10),[&]{return cancelled.load();});
   }if(!result.cancelled&&kind==BatchInitialKind::Initial&&urls){result.projection=projectBatchRows(*urls,browser,headers,result.settings,[&]{if(cancelled)throw Cancelled();if(observe)observe("projecting-row");});if(cancelled)throw Cancelled();result.projection.preview=std::make_shared<BatchDownloadPreview>(std::move(result.projection.requests),result.settings);}if(!result.cancelled&&refresh){const auto checkpoint=[&]{if(cancelled)throw Cancelled();if(observe)observe("refreshing-row");};if(refresh->changed&&previous)previous->replace(*refresh->changed,{});result.refresh=projectBatchRefresh(*refresh,browser,headers,result.settings,checkpoint);checkpoint();if(refresh->changed){if(!previous)previous=std::make_shared<BatchDownloadPreview>(std::vector<BatchPreviewRequest>{},result.settings);previous->replace(*refresh->changed,std::move(result.refresh.requests.front()));result.refreshedPreview=std::move(previous);}else result.refreshedPreview=std::make_shared<BatchDownloadPreview>(std::move(result.refresh.requests),result.settings);}result.cancelled=result.cancelled||cancelled.load();if(observe&&!result.cancelled)observe("captured");}
   catch(const Cancelled&){result.cancelled=true;}
   catch(const std::exception& error){result.error=error.what();result.cancelled=cancelled;}catch(...){result.error="Download options could not be read. Try again.";result.cancelled=cancelled;}
   promise.set_value(std::move(result));if(notify)try{notify();}catch(...){}
  });}catch(...){future={};throw;}
 }
 void cancelRead()noexcept{cancelled=true;wake.notify_all();}
 BatchInitialResult take(){if(!ready())throw std::runtime_error("Batch options are still being prepared.");auto result=future.get();if(worker.joinable())worker.join();return result;}
 void finish()noexcept{cancelRead();if(worker.joinable())worker.join();future={};}
};
#ifdef UDM_TOOLBAR_COMPONENT_TEST
inline BatchInitialObserver batchInitialTestObserver;
#endif
}
