#define UDM_TOOLBAR_COMPONENT_TEST
#include "Core.hpp"
#include "Ui.hpp"
#include <future>
namespace udm {
static fs::path output;static Json checks=Json::array();static std::string failure;
static void check(bool ok,const char* name){checks.push_back({{"name",name},{"passed",ok}});if(!ok)throw std::runtime_error(name);}
static HWND windowNamed(const wchar_t* name){struct Query{const wchar_t* name;HWND result=nullptr;}query{name};EnumThreadWindows(GetCurrentThreadId(),[](HWND window,LPARAM value)->BOOL{auto& q=*reinterpret_cast<Query*>(value);wchar_t title[128]{};GetWindowTextW(window,title,128);if(wcscmp(title,q.name)==0){q.result=window;return FALSE;}return TRUE;},reinterpret_cast<LPARAM>(&query));return query.result;}
struct QueueHold {
 std::promise<void> release;std::thread writer;std::atomic_bool held=false,expired=false;bool released=false;
 explicit QueueHold(Manager& manager){auto gate=release.get_future().share();writer=std::thread([this,&manager,gate]{Lock lock(manager.mutex);held=true;if(gate.wait_for(std::chrono::seconds(8))!=std::future_status::ready)expired=true;held=false;});const auto end=GetTickCount64()+3000;while(!held&&GetTickCount64()<end)Sleep(1);}
 void open(){if(!released){released=true;release.set_value();}if(writer.joinable())writer.join();}~QueueHold(){open();}
};
static std::function<void()> tick;
static void CALLBACK onTimer(HWND,UINT,UINT_PTR,DWORD){if(tick)tick();}
static void run(){
 for(const std::string mode:{"cancel","choose","retry","new","schedule","empty","prepared"}){
  Manager manager(output/wide(mode));manager.state["Queues"]=mode=="empty"?Json::array():Json::array({Json{{"Name","Stored default"},{"DefaultQueueRole","Download"}},Json{{"Name","Main download queue"}}});manager.save();const auto before=manager.snapshot();const auto disk=readText(manager.root/L"state.json");
  QueueChoiceView prepared{{"Stored default","Main download queue"},queueDisplayLabels(manager.state["Queues"])};
  auto held=std::make_unique<QueueHold>(manager);std::unique_ptr<QueueHold> after;std::atomic_bool waiting=false,captured=false;std::atomic_int reads=0;int stage=0,schedules=0;std::string error;const auto deadline=GetTickCount64()+18000;
  queueChoiceTestObserver=[&](const char* stage){const std::string event=stage;if(event=="waiting-model")waiting=true;if(event=="before-read"&&++reads==1&&mode=="retry")throw std::runtime_error("Owned selector failure");if(event=="captured"){after=std::make_unique<QueueHold>(manager);captured=true;}};
  check(held->held&&!held->expired,"A real writer holds the catalog before opening the actual queue selector");
  tick=[&]{
   auto window=windowNamed(L"Select a queue");if(!window)return;
   try{
    if(GetTickCount64()>deadline)throw std::runtime_error("Owned queue-selector interaction deadline");
    if(stage==3){auto prompt=windowNamed(L"Create queue");if(!prompt)return;auto edit=FindWindowExW(prompt,nullptr,L"Edit",nullptr),ok=FindWindowExW(prompt,nullptr,L"Button",L"OK");if(!edit||!ok)return;SetWindowTextW(edit,L"Owned new queue");stage=4;SendMessageW(ok,BM_CLICK,0,0);return;}
    auto ok=FindWindowExW(window,nullptr,L"Button",L"OK"),cancel=FindWindowExW(window,nullptr,L"Button",L"Cancel"),combo=FindWindowExW(window,nullptr,L"ComboBox",nullptr),create=FindWindowExW(window,nullptr,L"Button",L"+");if(!ok||!cancel||!combo||!create)return;
    auto form=dynamic_cast<Form*>(CWnd::FromHandlePermanent(window));if(!form)throw std::runtime_error("Missing actual queue form");
    if(stage==0){
     if(mode=="prepared"){
      check(IsWindowEnabled(ok)&&IsWindowEnabled(combo)&&reads==0,"A supplied cohort opens immediately without starting a second queue reader");
      form->Invalidate();form->UpdateWindow();check(held->held&&!held->expired,"The prepared selector paints while Manager remains locked");stage=5;SendMessageW(ok,BM_CLICK,0,0);return;
     }
     if(!waiting)return;
     check(!IsWindowEnabled(ok)&&!IsWindowEnabled(combo)&&!IsWindowEnabled(create)&&IsWindowEnabled(cancel),"The generic selector opens loading with editing disabled and Cancel available");
     form->SendMessage(WM_TIMER,Form::pulseTimer,0);form->Invalidate();form->UpdateWindow();check(held->held&&!held->expired,"The real selector pulse and paint return before the catalog writer releases");
     form->SendMessage(WM_COMMAND,IDOK,0);check(IsWindow(window)&&!IsWindowEnabled(ok),"Enter cannot accept a partially prepared selector");stage=1;
     if(mode=="cancel"){stage=5;SendMessageW(cancel,BM_CLICK,0,0);check(held->held&&!held->expired,"Cancel closes the loading selector before its external writer releases");}else held->open();return;
    }
    if(stage==1&&mode=="retry"){
     auto retry=FindWindowExW(window,nullptr,L"Button",L"Retry");if(!retry||!IsWindowVisible(retry))return;
     check(!IsWindowEnabled(ok)&&IsWindowEnabled(cancel),"A read failure remains inline with editing disabled and Cancel available");stage=2;SendMessageW(retry,BM_CLICK,0,0);return;
    }
    if((stage==1||stage==2)&&IsWindowEnabled(ok)){
     check(captured&&after&&after->held&&!after->expired,"Queue choices become usable while a later writer holds the catalog");form->SendMessage(WM_TIMER,Form::pulseTimer,0);form->Invalidate();form->UpdateWindow();check(after->held&&!after->expired,"The populated selector remains paintable without reacquiring Manager");
     auto queue=dynamic_cast<QueueCombo*>(CWnd::FromHandlePermanent(combo));check(queue!=nullptr,"The production QueueCombo owns the displayed queue identities");
     if(mode!="empty")check(queue->queueName()=="Main download queue"&&text(queue)=="Main download queue (custom)","The selected stored identity and collision-resolved display label come from one cohort");
     if(mode=="retry")check(reads==2,"Retry performs one fresh advisory read after consuming the error");
     if(mode=="empty"){check(queue->GetCount()==0&&queue->queueName().empty()&&IsWindowEnabled(create),"An empty queue list stays empty and still permits creating a new queue");stage=5;SendMessageW(cancel,BM_CLICK,0,0);return;}
     if(mode=="new"){stage=3;SendMessageW(create,BM_CLICK,0,0);return;}
     if(mode=="schedule"){auto schedule=FindWindowExW(window,nullptr,L"Button",L"&Schedule...");check(schedule&&IsWindowEnabled(schedule),"Schedule is available after selector preparation settles");stage=5;SendMessageW(schedule,BM_CLICK,0,0);return;}
     auto run=FindWindowExW(window,nullptr,L"Button",L"Start queue processing"),remember=FindWindowExW(window,nullptr,L"Button",L"Don't ask me again");SendMessageW(run,BM_SETCHECK,BST_CHECKED,0);SendMessageW(remember,BM_SETCHECK,BST_CHECKED,0);stage=5;SendMessageW(ok,BM_CLICK,0,0);return;
    }
    if(stage==4){auto queue=dynamic_cast<QueueCombo*>(CWnd::FromHandlePermanent(combo));check(queue&&queue->queueName()=="Owned new queue"&&after->held&&!after->expired,"Create queue edits only the local selector while the catalog remains held");stage=5;SendMessageW(ok,BM_CLICK,0,0);}
   }catch(const std::exception& e){error=e.what();held->open();if(captured&&after)after->open();if(auto prompt=windowNamed(L"Create queue"))SendMessageW(prompt,WM_COMMAND,IDCANCEL,0);SendMessageW(window,WM_COMMAND,IDCANCEL,0);}
  };
  const auto timer=SetTimer(nullptr,0,10,onTimer);check(timer!=0,"Create the owned selector-control timer");std::optional<DownloadQueueChoice> choice;
  try{choice=chooseDownloadQueue(nullptr,manager,"Main download queue",false,mode=="schedule"?std::function<bool(CWnd*)>([&](CWnd* owner){++schedules;check(owner&&after->held&&!after->expired,"Schedule receives its real selector owner without a new catalog read");return true;}):std::function<bool(CWnd*)>{},mode=="prepared"?&prepared:nullptr);}catch(...){KillTimer(nullptr,timer);tick={};queueChoiceTestObserver={};throw;}
  KillTimer(nullptr,timer);tick={};queueChoiceTestObserver={};check(error.empty()&&stage==5&&!held->expired&&(!after||!after->expired)&&!windowNamed(L"Select a queue"),"Closing the selector joins its reader and leaves no owned dialog behind");
  if(mode=="choose"||mode=="retry")check(choice&&choice->name=="Main download queue"&&choice->start&&choice->dontAsk&&!choice->create,"Accepted selector choice retains stored identity, processing and remember flags");
  else if(mode=="new")check(choice&&choice->create&&choice->name=="Owned new queue","A locally created name is returned as a canonical create request");
  else if(mode=="prepared")check(choice&&choice->name=="Main download queue"&&!choice->create,"Prepared and generic selectors share the same accepted-choice contract");
  else check(!choice&&(mode!="schedule"||schedules==1),"Cancel, empty-list cancel and Schedule do not produce a stray accepted queue choice");
  held->open();if(after)after->open();check(manager.snapshot()==before&&readText(manager.root/L"state.json")==disk&&!manager.fileInfoDecisionPending,"Selector preparation and local choices never mutate the persisted catalog or command guard");manager.stop();
 }
}
class QueueChoiceApplication:public CWinApp {
 int result=1;
public:BOOL InitInstance()override{
 CWinApp::InitInstance();INITCOMMONCONTROLSEX controls{sizeof(controls),ICC_WIN95_CLASSES|ICC_DATE_CLASSES};InitCommonControlsEx(&controls);AfxOleInit();int argc=0;auto args=CommandLineToArgvW(GetCommandLineW(),&argc);
 try{if(!args||argc!=2)throw std::runtime_error("Pass one fresh output directory");output=args[1];if(fs::exists(output)){output.clear();throw std::runtime_error("Fresh output required");}fs::create_directories(output);run();result=0;}catch(const std::exception& error){failure=error.what();}if(args)LocalFree(args);if(!output.empty())atomicText(output/L"results.json",Json{{"passed",result==0},{"checks",checks},{"error",failure}}.dump(2),false);return FALSE;
 }
 int ExitInstance()override{AfxOleTerm(FALSE);return result;}
};QueueChoiceApplication application;
}
