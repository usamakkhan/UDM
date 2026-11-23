#define UDM_TOOLBAR_COMPONENT_TEST
#include "App.cpp"
#include <condition_variable>
namespace udm {
static fs::path output;static Json checks=Json::array(),observations=Json::array();static std::string failure;static bool baseline=false;
static void check(bool ok,const char* label){checks.push_back({{"name",label},{"passed",ok}});if(!ok)throw std::runtime_error(label);}
struct WakeModelHold {
 std::mutex mutex;std::condition_variable wake;bool released=false;std::atomic_bool held=false,expired=false;std::thread thread;
 explicit WakeModelHold(Manager& manager){thread=std::thread([this,&manager]{Lock model(manager.mutex);held=true;std::unique_lock waiting(mutex);expired=!wake.wait_for(waiting,std::chrono::seconds(4),[&]{return released;});});while(!held)Sleep(1);}
 void open(){{std::lock_guard lock(mutex);released=true;}wake.notify_all();if(thread.joinable())thread.join();}~WakeModelHold(){open();}
};
class ToolbarComponentTest {
 static inline MainWindow* frame=nullptr;static inline HWND dialog=nullptr;static inline bool queue=false,watchdog=false,timerRemoved=false;
 static inline unsigned phase=0;static inline ULONGLONG opened=0,phaseStarted=0,releasedAt=0;static inline uint64_t probeTicks=0;static inline std::string callbackError;
 static inline std::unique_ptr<WakeModelHold> hold;static inline std::function<void()> staleWake,currentWake;
 static HWND cancelButton(){for(auto c=GetWindow(dialog,GW_CHILD);c;c=GetWindow(c,GW_HWNDNEXT)){wchar_t label[128]{};GetWindowTextW(c,label,128);if(!wcscmp(label,L"Cancel action"))return c;}return nullptr;}
 static void CALLBACK drive(HWND,UINT,UINT_PTR,DWORD){
  try{
   if(!dialog)EnumThreadWindows(GetCurrentThreadId(),[](HWND window,LPARAM)->BOOL{wchar_t text[128]{};GetWindowTextW(window,text,128);if(!wcscmp(text,queue?L"Queue finished":L"Download completion action")){dialog=window;return FALSE;}return TRUE;},0);
   if(!dialog)return;const auto now=GetTickCount64();
   if(phase==0){timerRemoved=KillTimer(dialog,Form::pulseTimer)!=FALSE;check(timerRemoved,"The actual countdown fallback timer is disabled for this owned comparison");opened=now;phaseStarted=now;phase=1;
#ifdef UDM_COMPLETION_COUNTDOWN_WAKE_AVAILABLE
    currentWake=frame->completionCountdownWake;check(bool(currentWake),"The active dialog has an owned generation-checked wake route");
#endif
    return;
   }
   if(phase==1){auto state=frame->modelMaintenance.takeResult();if(now-phaseStarted<200||state.active||state.tickPending||state.dispatchPending)return;probeTicks=state.ticks;for(int i=0;i<20;++i)frame->maintenanceSignal->notifier()();if(staleWake)for(int i=0;i<20;++i)staleWake();phaseStarted=now;phase=2;return;}
   if(phase==2){if(now-phaseStarted<200)return;auto state=frame->modelMaintenance.takeResult();check(state.ticks==probeTicks&&!state.active&&!state.tickPending,"Completion and stale-dialog wake bursts cannot create extra scheduler work");hold=std::make_unique<WakeModelHold>(frame->manager);phase=3;return;}
   if(phase==3){if(now-opened<2100)return;const auto begun=GetTickCount64();SendMessageW(dialog,WM_TIMER,Form::pulseTimer,0);check(IsWindow(dialog)&&GetTickCount64()-begun<200&&!hold->expired&&IsWindowEnabled(cancelButton()),"An explicitly expired pulse stays cancellable while its final tick waits for the model");releasedAt=GetTickCount64();hold->open();phase=4;return;}
   if(phase==4&&now-releasedAt>=800){watchdog=true;SendMessageW(cancelButton(),BM_CLICK,0,0);}
  }catch(const std::exception& e){callbackError=e.what();if(hold)hold->open();if(dialog)PostMessageW(dialog,WM_CLOSE,0,0);phase=9;}
 }
public:static void run(MainWindow& owner,bool dark,bool queueMode){
  frame=&owner;queue=queueMode;dialog=nullptr;watchdog=false;timerRemoved=false;phase=0;releasedAt=0;callbackError.clear();currentWake={};uiDark=dark;frame->modelMaintenance.stop();frame->modelMaintenance.restart();
  unsigned performed=0;
  if(queue){Lock lock(frame->manager.mutex);auto& q=frame->manager.state["Queues"][0];setQueueCompletionSteps(q,{"Exit UDM"},false);q["CompletionCycle"]=guid();q["CompletionCanceled"]=false;frame->manager.state["CompletionWorkEpoch"]=guid();auto event=queueCompletionEvent(q);event["WorkEpoch"]=str(frame->manager.state,"CompletionWorkEpoch");event["DelaySeconds"]=2;frame->pendingQueueActions.emplace_back(event);frame->queueCompletionPerformer=[&](const std::string& step,bool,const std::string&){if(step!="Exit UDM")throw std::runtime_error("Unexpected owned queue step");++performed;};}
  else{auto job=frame->manager.add("https://example.invalid/countdown-wake","","owned.bin","Main queue",true);job->data["Status"]="Complete";frame->manager.save();frame->fileActions.emplace_back(job,Json{{"Action","Exit UDM"},{"Actions",Json::array({"Exit UDM"})},{"WaitForOthers",false},{"DelaySeconds",2}});}
  const auto timer=SetTimer(nullptr,0,10,drive);if(!timer)throw std::runtime_error("Owned driver timer");
  try{if(queue)frame->queueCompletionActions();else frame->fileCompletionActions();}catch(...){KillTimer(nullptr,timer);if(hold)hold->open();hold.reset();throw;}
  const auto returned=GetTickCount64();KillTimer(nullptr,timer);if(hold)hold->open();const bool expired=hold&&hold->expired.load();hold.reset();if(!callbackError.empty())throw std::runtime_error(callbackError);
  check(timerRemoved&&phase==4&&!expired&&!IsWindow(dialog),"The owned modal is destroyed after the final-tick hold without a writer watchdog expiry");
  MSG command{};if(!queue&&PeekMessageW(&command,frame->GetSafeHwnd(),WM_COMMAND,WM_COMMAND,PM_REMOVE)){check(LOWORD(command.wParam)==CMD_EXIT,"The fixture consumes only its owned Exit command without executing it");++performed;}
  if(baseline)check(watchdog&&performed==0&&returned-releasedAt>=800,"Frozen 9.94 waits for the disabled timer and requires the owned cancellation watchdog");
  else check(!watchdog&&performed==1&&returned-releasedAt<800,"Current countdown consumes its final acknowledgement and performs once without another timer pulse");
#ifdef UDM_COMPLETION_COUNTDOWN_WAKE_AVAILABLE
  check(!frame->completionCountdownWake,"Closing the modal clears its wake route before another dialog can bind");
#endif
  observations.push_back({{"dark",dark},{"queue",queue},{"fallbackTimerDisabled",timerRemoved},{"releaseToReturnMs",returned-releasedAt},{"cancellationWatchdogRequired",watchdog},{"performed",performed},{"baseline",baseline}});
  staleWake=std::move(currentWake);if(staleWake)staleWake();frame->modelMaintenance.stop();frame->modelMaintenance.restart();frame->queueCompletionPerformer={};
 }
};
class CountdownWakeApplication:public CWinApp {
 int result=1;
public:BOOL InitInstance()override{CWinApp::InitInstance();INITCOMMONCONTROLSEX controls{sizeof(controls),ICC_WIN95_CLASSES|ICC_DATE_CLASSES};InitCommonControlsEx(&controls);AfxOleInit();int argc=0;auto args=CommandLineToArgvW(GetCommandLineW(),&argc);
 try{if(!args||argc!=3)throw std::runtime_error("Pass candidate/baseline and fresh output");baseline=utf8(args[1])=="baseline";if(!baseline&&utf8(args[1])!="candidate")throw std::runtime_error("Invalid fixture mode");output=fs::absolute(args[2]);if(fs::exists(output)){output.clear();throw std::runtime_error("Fresh output required");}fs::create_directories(output);
  for(bool dark:{false,true})for(bool queue:{false,true}){if(baseline&&(dark||queue))continue;Manager manager(output/(dark?L"dark":L"light")/(queue?L"queue":L"file"));auto p=manager.state["Settings"];p["DownloadFolder"]=utf8((manager.root/L"files").wstring());p["CategoryFolders"]=false;p["ClipboardMonitor"]=false;p["CloseToTray"]=false;p["Sound"]=false;manager.setSettings(p);auto window=new MainWindow(manager);m_pMainWnd=window;window->KillTimer(1);try{ToolbarComponentTest::run(*window,dark,queue);}catch(...){m_pMainWnd=nullptr;closeOwnedMainWindow(window);throw;}m_pMainWnd=nullptr;closeOwnedMainWindow(window);manager.stop();}result=0;
 }catch(const std::exception& e){failure=e.what();}if(args)LocalFree(args);if(!output.empty())atomicText(output/L"results.json",Json{{"passed",result==0},{"checks",checks},{"observations",observations},{"baseline",baseline},{"error",failure}}.dump(2),false);return FALSE;}
 int ExitInstance()override{AfxOleTerm(FALSE);return result;}
};CountdownWakeApplication application;
}
