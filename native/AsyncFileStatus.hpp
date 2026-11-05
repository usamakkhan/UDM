#pragma once
#include "Core.hpp"
#include <algorithm>
#include <condition_variable>
#include <memory>
#include <thread>

namespace udm {
// Advisory menu state only. Operations must revalidate the current filesystem.
// Four shared-state workers bound blocked probes without retaining HWND/Manager.
class AsyncFileStatus {
public:
 struct Result {
  uint64_t generation=0;std::map<fs::path,bool> files;
  bool regular(const fs::path& path)const{const auto found=files.find(path);return found!=files.end()&&found->second;}
 };
private:
 static constexpr unsigned WorkerLimit=4;
 struct Request {
  std::shared_ptr<const std::vector<fs::path>> paths;
  size_t next=0,remaining=0;uint64_t revision=0;Result result;
  Request(uint64_t generation,std::shared_ptr<const std::vector<fs::path>> wanted):paths(std::move(wanted)),remaining(paths->size()){result.generation=generation;}
 };
 struct State {
  std::mutex mutex;std::condition_variable wake;bool closing=false,started=false,disabled=false;
  unsigned workers=0;uint64_t publishedRevision=0;
  std::shared_ptr<Request> request;std::shared_ptr<const Result> completed;
 };
 std::shared_ptr<State> state=std::make_shared<State>();
 // Only the UI thread accesses wanted/generation/lastQueued.
 std::shared_ptr<const std::vector<fs::path>> wanted;uint64_t generation=0;ULONGLONG lastQueued=0;
 static void run(const std::shared_ptr<State>& current)noexcept{
  try{for(;;){
   std::shared_ptr<Request> request;size_t index=0;
   {std::unique_lock<std::mutex> lock(current->mutex);
    current->wake.wait(lock,[&]{return current->closing||current->disabled||(current->request&&current->request->next<current->request->paths->size());});
    if(current->closing||current->disabled)return;
    request=current->request;index=request->next++;
   }
   const auto& path=(*request->paths)[index];std::error_code error;
   const bool regular=fs::is_regular_file(path,error)&&!error;
   {std::lock_guard<std::mutex> lock(current->mutex);
    if(current->closing||current->disabled)return;
    if(current->request==request){
     const auto found=request->result.files.find(path);
     if(found==request->result.files.end()||found->second!=regular){request->result.files[path]=regular;++request->revision;}
     --request->remaining;
    }
   }
  }}catch(...){
   {std::lock_guard<std::mutex> lock(current->mutex);current->disabled=true;}
   current->wake.notify_all();
  }
 }
public:
 AsyncFileStatus()=default;AsyncFileStatus(const AsyncFileStatus&)=delete;AsyncFileStatus& operator=(const AsyncFileStatus&)=delete;
 ~AsyncFileStatus(){stop();}
 std::shared_ptr<const Result> observe(std::vector<fs::path> paths){
  std::sort(paths.begin(),paths.end());paths.erase(std::unique(paths.begin(),paths.end()),paths.end());
  if(paths.empty())return {};
  const bool changed=!wanted||*wanted!=paths;
  if(changed){wanted=std::make_shared<const std::vector<fs::path>>(std::move(paths));++generation;}
  const auto now=GetTickCount64();unsigned launch=0;bool notify=false;
  std::shared_ptr<const Result> result,previous;std::shared_ptr<Request> previousRequest;
  {std::lock_guard<std::mutex> lock(state->mutex);if(state->closing||state->disabled)return {};
   if(changed||!state->request||state->request->result.generation!=generation){
    auto replacement=std::make_shared<Request>(generation,wanted);
    previousRequest=std::move(state->request);state->request=std::move(replacement);
    previous=std::move(state->completed);state->publishedRevision=0;lastQueued=now;notify=true;
   }else if(state->request&&state->request->remaining==0&&now-lastQueued>=1000){
    // Keep matching advisory answers visible while refreshing the same paths.
    state->request->next=0;state->request->remaining=wanted->size();lastQueued=now;notify=true;
   }
   if(state->request&&state->request->revision!=state->publishedRevision){
    auto updated=std::make_shared<const Result>(state->request->result);
    if(!previous)previous=std::move(state->completed);
    state->completed=std::move(updated);state->publishedRevision=state->request->revision;
   }
   result=state->completed;
   if(!state->started){state->started=true;state->workers=WorkerLimit;launch=WorkerLimit;}
  }
  for(unsigned index=0;index<launch;++index)try{auto current=state;std::thread([current]{run(current);}).detach();}
   catch(...){std::lock_guard<std::mutex> lock(state->mutex);if(!--state->workers)state->disabled=true;}
  if(notify||launch)state->wake.notify_all();return result;
 }
 void stop()noexcept{
  std::shared_ptr<const Result> previous;std::shared_ptr<Request> request;
  {std::lock_guard<std::mutex> lock(state->mutex);state->closing=true;request=std::move(state->request);previous=std::move(state->completed);}
  state->wake.notify_all();
 }
};
}
