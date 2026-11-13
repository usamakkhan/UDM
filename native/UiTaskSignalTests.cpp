#include "Core.hpp"
#include "UiTaskSignal.hpp"
#include <iostream>
using namespace udm;
static Json checks=Json::array();
static void check(bool value,const char* name){checks.push_back({{"name",name},{"passed",value}});if(!value)throw std::runtime_error(name);}
int wmain(int argc,wchar_t** argv){
 if(argc!=2)return 2;const auto root=fs::absolute(argv[1]);if(fs::exists(root))return 3;fs::create_directories(root);std::string error;HWND window=nullptr;
 try{
  window=CreateWindowExW(0,L"STATIC",L"Owned task signal fixture",0,0,0,1,1,HWND_MESSAGE,nullptr,GetModuleHandleW(nullptr),nullptr);check(window!=nullptr,"Create a private message-only signal target");
  auto first=std::make_shared<UiTaskSignal>();first->arm(window);auto oldNotify=first->notifier();std::thread worker(oldNotify);worker.join();MSG message{};
  check(PeekMessageW(&message,window,UiTaskSignal::message,UiTaskSignal::message,PM_REMOVE)&&first->matches(message.wParam),"A worker posts only the target's owned integer identity");
  auto second=std::make_shared<UiTaskSignal>();second->arm(window);check(!second->matches(message.wParam),"A newer target rejects an older completion identity even on the same HWND");
  first->disarm();oldNotify();check(!PeekMessageW(&message,window,UiTaskSignal::message,UiTaskSignal::message,PM_REMOVE),"Disarmed targets do not post another completion");
  first.reset();oldNotify();check(!PeekMessageW(&message,window,UiTaskSignal::message,UiTaskSignal::message,PM_REMOVE),"Notifications cannot keep or access a destroyed target");
  second->notifier()();check(PeekMessageW(&message,window,UiTaskSignal::message,UiTaskSignal::message,PM_REMOVE)&&second->matches(message.wParam),"A fresh target continues to receive its own completions");second->disarm();DestroyWindow(window);window=nullptr;
 }catch(const std::exception& e){error=e.what();}
 if(window)DestroyWindow(window);atomicText(root/L"results.json",Json{{"passed",error.empty()},{"error",error},{"checks",checks}}.dump(2),false);std::cout<<checks.size()<<" checks; "<<(error.empty()?"passed":error)<<"\n";return error.empty()?0:1;
}
