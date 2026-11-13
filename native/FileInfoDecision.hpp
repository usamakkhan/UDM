#pragma once
#include "Core.hpp"
#include <future>
namespace udm {
// An owned command, never controls or HWNDs. UI preparation finishes before
// the joined task reads these values.
struct FileInfoDecisionDraft {
 Json edit,rememberedLogin;
 FileInfoFolderChoice folder;
 std::optional<FileInfoLogin> login;
 std::optional<DownloadQueueChoice> queue;
 bool start=false,media=false,loginUnchanged=false,startQueue=false;
};
struct FileInfoDecisionResult {std::string warning;};
using FileInfoDecisionAction=std::function<FileInfoDecisionResult()>;
inline FileInfoDecisionResult applyFileInfoDecision(Manager& manager,JobPtr job,const FileInfoDecisionDraft& draft){
 const auto choice=draft.queue?&*draft.queue:nullptr;
 bool applied=false;
 if(draft.start&&(draft.media||draft.loginUnchanged))applied=manager.confirmPrefetchedDownload(job,draft.edit,&draft.folder,draft.rememberedLogin);
 if(!draft.start&&(draft.media||draft.loginUnchanged))applied=manager.deferPrefetchedDownload(job,draft.edit,draft.folder,draft.rememberedLogin,choice);
 if(!applied){manager.endPrefetch(job);manager.finishStoppedFileInfo(job,draft.edit,draft.folder,draft.login,!draft.start,choice);}
 FileInfoDecisionResult result;
 if(draft.start){
  bool queued=false;{Lock lock(manager.mutex);queued=!manager.isActive(job)&&str(job->data,"Status")=="Queued"&&yes(job->data,"IndividualStart");}
  if(queued)try{manager.dispatchQueueWork();}catch(const std::exception& error){result.warning="The download settings were saved, but transfer scheduling could not start. "+std::string(error.what());}
 }
 if(draft.startQueue&&choice&&choice->start)try{manager.startQueueForDownloads(choice->name,{job});}
 catch(const std::exception& error){result.warning="The download was added to the queue, but queue processing could not start. "+std::string(error.what());}
 return result;
}
class FileInfoDecisionTask {
 Manager* owner=nullptr;std::future<FileInfoDecisionResult> task;std::thread worker;
public:
 FileInfoDecisionTask()=default;FileInfoDecisionTask(const FileInfoDecisionTask&)=delete;FileInfoDecisionTask& operator=(const FileInfoDecisionTask&)=delete;
 ~FileInfoDecisionTask(){release();}
 bool valid()const noexcept{return owner!=nullptr;}
 bool ready(){return task.valid()&&task.wait_for(std::chrono::seconds(0))==std::future_status::ready;}
 void start(Manager& manager,JobPtr job,FileInfoDecisionDraft draft,std::function<void()> notify={}){
  startAction(manager,[&manager,job=std::move(job),draft=std::move(draft)]{return applyFileInfoDecision(manager,job,draft);},std::move(notify));
 }
 void startAction(Manager& manager,FileInfoDecisionAction action,std::function<void()> notify={}){
  if(!action)throw std::runtime_error("Missing download decision command.");
  if(valid())throw std::runtime_error("This download decision is already being saved.");
  bool expected=false;if(!manager.fileInfoDecisionPending.compare_exchange_strong(expected,true))throw std::runtime_error("Another download decision is being saved. Try again shortly.");
  owner=&manager;
  try{std::promise<FileInfoDecisionResult> result;task=result.get_future();worker=std::thread([action=std::move(action),result=std::move(result),notify=std::move(notify)]()mutable{try{result.set_value(action());}catch(...){result.set_exception(std::current_exception());}if(notify)try{notify();}catch(...){}});}
  catch(...){owner=nullptr;manager.fileInfoDecisionPending=false;throw;}
 }
 FileInfoDecisionResult get(){return task.get();}
 // Keep refreshes deferred until the UI has handled success/error. Otherwise a
 // fast completed transfer can open its completion window before File Info ends.
 void release()noexcept{if(worker.joinable())worker.join();if(owner){owner->fileInfoDecisionPending=false;owner=nullptr;}}
};
}
