#pragma once
#include <windows.h>
#include <functional>
namespace udm {
// Inspect this UI thread's current owned top-level windows, not an activation
// snapshot. Common dialogs can disable a progress/completion window while the
// main frame remains enabled. No catalog lock, foreign thread or input hook.
class CompletionModalFence {
public:
 class ModalScope;
private:
 inline static thread_local CompletionModalFence* first=nullptr;
 inline static thread_local HHOOK hook=nullptr;
 inline static thread_local ModalScope* scopes=nullptr;
 CompletionModalFence* next=nullptr;
 HWND root=nullptr;
 std::function<void()> notify;
 bool waiting=false;
 bool owns(HWND window)const noexcept{
  for(unsigned depth=0;window&&depth<64;++depth){if(window==root)return true;window=GetWindow(window,GW_OWNER);}
  return false;
 }
 static LRESULT CALLBACK observe(int code,WPARAM parameter,LPARAM detail)noexcept{
  if(code>=0&&detail){
   const auto* message=reinterpret_cast<const CWPSTRUCT*>(detail);
   if((message->message==WM_ENABLE&&message->wParam)||message->message==WM_NCDESTROY)
    for(auto* fence=first;fence;fence=fence->next)
     if(fence->waiting&&fence->owns(message->hwnd))try{fence->notify();}catch(...){/* Main timer retries. */}
  }
  return CallNextHookEx(hook,code,parameter,detail);
 }
public:
 class ModalScope {
  friend class CompletionModalFence;
  HWND owner;ModalScope* next;
 public:
  explicit ModalScope(HWND window)noexcept:owner(window),next(scopes){scopes=this;}
  ModalScope(const ModalScope&)=delete;ModalScope& operator=(const ModalScope&)=delete;
  ~ModalScope(){
   auto** current=&scopes;while(*current&&*current!=this)current=&(*current)->next;
   if(*current)*current=next;
   for(auto* fence=first;fence;fence=fence->next)
    if(fence->waiting&&fence->owns(owner))try{fence->notify();}catch(...){/* Main timer retries. */}
  }
 };
 CompletionModalFence()=default;
 CompletionModalFence(const CompletionModalFence&)=delete;
 CompletionModalFence& operator=(const CompletionModalFence&)=delete;
 ~CompletionModalFence(){detach();}
 void attach(HWND window,std::function<void()> wake){
  detach();root=window;notify=std::move(wake);next=first;first=this;
  if(!hook)hook=SetWindowsHookExW(WH_CALLWNDPROC,observe,nullptr,GetCurrentThreadId());
  // Hook failure leaves the live-state check intact; the main timer retries.
 }
 void detach()noexcept{
  auto** current=&first;while(*current&&*current!=this)current=&(*current)->next;
  if(*current)*current=next;
  root=nullptr;next=nullptr;waiting=false;notify=nullptr;
  if(!first&&hook){UnhookWindowsHookEx(hook);hook=nullptr;}
 }
private:
 bool query(bool enabledPopupOnly)noexcept{
  if(!root||!IsWindow(root))return false;
  // Reserve ownership before a native dialog pumps messages or creates its
  // HWND, and retain it through the native function's teardown/return.
  for(auto* scope=scopes;scope;scope=scope->next)if(owns(scope->owner)){waiting=true;return true;}
  struct Search {CompletionModalFence* fence;bool enabledPopupOnly;bool blocked=false;} search{this,enabledPopupOnly};
  const auto complete=EnumThreadWindows(GetCurrentThreadId(),[](HWND window,LPARAM data)->BOOL{
   auto& state=*reinterpret_cast<Search*>(data);
   // Windows also creates hidden, disabled IME/helper windows in this owner
   // family. They are not modal owners. Follow each visible window's chain so
   // a visible modal still protects a hidden/disabled progress owner.
   if((!state.enabledPopupOnly||IsWindowEnabled(window))&&
      (window==state.fence->root||IsWindowVisible(window))&&state.fence->owns(window))
    for(auto owner=window;owner;owner=GetWindow(owner,GW_OWNER)){
     if(!IsWindowEnabled(owner)){state.blocked=true;return FALSE;}
     if(owner==state.fence->root)break;
    }
   return TRUE;
  },reinterpret_cast<LPARAM>(&search));
  waiting=search.blocked||!complete;return waiting;
 }
public:
 bool blocked()noexcept{return query(false);}
 // An enabled popup in a disabled owner's chain witnesses active modality.
 // Exit's own frame disable can disable its modeless progress windows too;
 // those disabled windows alone must not strand the accepted shutdown.
 bool modalActive()noexcept{return query(true);}
 bool wakeAvailable()const noexcept{return hook!=nullptr;}
};
}
