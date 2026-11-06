#pragma once
#include "Core.hpp"
#include <shellapi.h>
#include <objbase.h>
#include <deque>

namespace udm {
struct FileIconResult {
 std::string extension,typeName;uint64_t generation=0;HICON icon=nullptr;
 FileIconResult(std::string value,uint64_t epoch):extension(std::move(value)),generation(epoch){}
 FileIconResult(const FileIconResult&)=delete;FileIconResult& operator=(const FileIconResult&)=delete;
 FileIconResult(FileIconResult&& other)noexcept:extension(std::move(other.extension)),typeName(std::move(other.typeName)),generation(other.generation),icon(other.icon){other.icon=nullptr;}
 ~FileIconResult(){if(icon)DestroyIcon(icon);}
};
// The worker owns shared state only: no CWnd, HWND, manager or image-list access.
// Shutdown abandons a blocked Shell call without waiting on the window thread.
class AsyncFileIcons {
 struct Request {std::string extension;uint64_t generation;};
 struct State {
  explicit State(bool includeDetails):details(includeDetails){}
  const bool details;
  std::mutex mutex;Handle wake{CreateEventW(nullptr,FALSE,FALSE,nullptr)};
  bool closing=false,running=false,disabled=false;uint64_t generation=1;
  std::deque<Request> requests;std::set<std::string> pending;std::vector<FileIconResult> completed;
 };
 std::shared_ptr<State> state;
 static void disable(const std::shared_ptr<State>& current){
  std::lock_guard<std::mutex> lock(current->mutex);current->disabled=true;current->requests.clear();current->pending.clear();current->completed.clear();
 }
 static void run(const std::shared_ptr<State>& current)noexcept{
  const auto initialized=CoInitializeEx(nullptr,COINIT_APARTMENTTHREADED|COINIT_DISABLE_OLE1DDE);
  if(FAILED(initialized)){disable(current);return;}
  struct ComScope {~ComScope(){CoUninitialize();}} com;
  try{
   for(;;){
    {std::lock_guard<std::mutex> lock(current->mutex);if(current->closing)return;}
    // Shell extensions may create STA COM objects. Pump their messages while
    // idle and between requests rather than leaving an unpumped apartment.
    MSG message{};while(PeekMessageW(&message,nullptr,0,0,PM_REMOVE)){if(message.message==WM_QUIT){disable(current);return;}TranslateMessage(&message);DispatchMessageW(&message);}
    std::optional<Request> request;
    {std::lock_guard<std::mutex> lock(current->mutex);if(current->closing)return;if(!current->requests.empty()){request=std::move(current->requests.front());current->requests.pop_front();}}
    if(!request){HANDLE wake=current->wake.h;if(MsgWaitForMultipleObjects(1,&wake,FALSE,INFINITE,QS_ALLINPUT)==WAIT_FAILED){disable(current);return;}continue;}
    FileIconResult result(request->extension,request->generation);SHFILEINFOW info{};
    const UINT flags=SHGFI_USEFILEATTRIBUTES|SHGFI_ICON|(current->details?SHGFI_TYPENAME|SHGFI_LARGEICON:SHGFI_SMALLICON);
    const auto success=SHGetFileInfoW(wide("file"+request->extension).c_str(),FILE_ATTRIBUTE_NORMAL,&info,sizeof(info),flags);
    result.icon=info.hIcon;if(!success&&result.icon){DestroyIcon(result.icon);result.icon=nullptr;}
    if(success&&current->details)result.typeName=utf8(info.szTypeName);
    std::lock_guard<std::mutex> lock(current->mutex);
    if(current->closing)return;
    if(request->generation==current->generation)current->completed.push_back(std::move(result));
   }
  }catch(...){disable(current);}
 }
public:
 explicit AsyncFileIcons(bool details=false):state(std::make_shared<State>(details)){}
 AsyncFileIcons(const AsyncFileIcons&)=delete;AsyncFileIcons& operator=(const AsyncFileIcons&)=delete;
 ~AsyncFileIcons(){stop();}
 bool request(const std::string& extension){
  const bool supported=state->details
   ? extension.empty()||(extension.front()=='.'&&extension.size()<=1024)
   : extension.size()>=2&&extension.size()<=12&&extension.front()=='.';
  if(!supported)return false;
  bool launch=false;
  {
   std::lock_guard<std::mutex> lock(state->mutex);
   if(state->closing||state->disabled||!state->wake||state->pending.count(extension)||state->pending.size()>=128)return false;
   auto inserted=state->pending.insert(extension);try{state->requests.push_back({extension,state->generation});}catch(...){state->pending.erase(inserted.first);return false;}
   if(!state->running){state->running=true;launch=true;}
   SetEvent(state->wake.h);
  }
  if(launch)try{auto current=state;std::thread([current]{run(current);std::lock_guard<std::mutex> lock(current->mutex);current->running=false;}).detach();}catch(...){disable(state);std::lock_guard<std::mutex> lock(state->mutex);state->running=false;return false;}
  return true;
 }
 std::vector<FileIconResult> take(){
  std::vector<FileIconResult> result;std::lock_guard<std::mutex> lock(state->mutex);result.swap(state->completed);for(const auto& item:result)state->pending.erase(item.extension);return result;
 }
 uint64_t generation()const{std::lock_guard<std::mutex> lock(state->mutex);return state->generation;}
 void invalidate(){
  std::lock_guard<std::mutex> lock(state->mutex);++state->generation;state->requests.clear();state->pending.clear();state->completed.clear();if(!state->running)state->disabled=false;
 }
 void stop(){
  std::lock_guard<std::mutex> lock(state->mutex);state->closing=true;state->requests.clear();state->pending.clear();state->completed.clear();if(state->wake)SetEvent(state->wake.h);
 }
};
}
