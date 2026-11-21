#pragma once
#include "QueueChoiceTask.hpp"
#include <future>
#include <condition_variable>
namespace udm {
enum class FileInfoActionKind {Start,Later};
struct FileInfoActionResult {QueueChoiceView queues;bool refreshSession=false,promptLater=true,cancelled=false;std::string error;};
using FileInfoActionObserver=std::function<void(const char*)>;
// Advisory action preparation. It never applies a draft or owns the accepted
// command guard; canonical Start/Later still revalidate on their save worker.
class FileInfoActionTask {
 std::future<FileInfoActionResult> future;std::thread worker;std::atomic_bool cancelled=false;std::mutex mutex;std::condition_variable wake;
public:
 FileInfoActionTask()=default;FileInfoActionTask(const FileInfoActionTask&)=delete;FileInfoActionTask& operator=(const FileInfoActionTask&)=delete;~FileInfoActionTask(){finish();}
 bool valid()const noexcept{return future.valid();}bool ready()const{return valid()&&future.wait_for(std::chrono::seconds(0))==std::future_status::ready;}
 void start(Manager& manager,JobPtr job,std::function<void()> notify={},FileInfoActionObserver observe={},FileInfoActionKind kind=FileInfoActionKind::Later){
  if(valid()||worker.joinable())throw std::runtime_error("This download action is already being prepared.");cancelled=false;std::promise<FileInfoActionResult> promise;future=promise.get_future();
  try{worker=std::thread([this,&manager,job=std::move(job),notify=std::move(notify),observe=std::move(observe),kind,promise=std::move(promise)]()mutable{
   FileInfoActionResult result;
   try{for(;;){
    if(cancelled){result.cancelled=true;break;}
    std::unique_lock lock(manager.mutex,std::try_to_lock);
    if(lock.owns_lock()&&!manager.fileInfoDecisionPending){
     if(observe)observe("before-read");
     if(!job||std::find(manager.jobs.begin(),manager.jobs.end(),job)==manager.jobs.end())throw std::runtime_error("This download is no longer available. Close File Info and choose another download.");
     result.refreshSession=yes(job->data,"SabrSessionRefreshRequired");result.promptLater=yes(manager.state["Settings"],"QueuePromptLater",true);
     if(kind==FileInfoActionKind::Later&&result.promptLater){const auto& queues=manager.state["Queues"];for(const auto& queue:queues)result.queues.names.push_back(str(queue,"Name"));result.queues.labels=queueDisplayLabels(queues);}break;
    }
    if(lock.owns_lock())lock.unlock();if(observe)observe("waiting-model");std::unique_lock waiting(mutex);wake.wait_for(waiting,std::chrono::milliseconds(10),[&]{return cancelled.load();});
   }result.cancelled=result.cancelled||cancelled.load();if(observe&&!result.cancelled)observe("captured");}
   catch(const std::exception& error){result.error=error.what();result.cancelled=cancelled;}catch(...){result.error="The download action could not be prepared. Try again.";result.cancelled=cancelled;}
   promise.set_value(std::move(result));if(notify)try{notify();}catch(...){}
  });}catch(...){future={};throw;}
 }
 void cancelRead()noexcept{cancelled=true;wake.notify_all();}
 FileInfoActionResult take(){if(!ready())throw std::runtime_error("The download action is still being prepared.");auto result=future.get();if(worker.joinable())worker.join();return result;}
 void finish()noexcept{cancelRead();if(worker.joinable())worker.join();future={};}
};
#ifdef UDM_TOOLBAR_COMPONENT_TEST
inline FileInfoActionObserver fileInfoActionTestObserver;
#endif
}
