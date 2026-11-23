#define UDM_TOOLBAR_COMPONENT_TEST
#include "App.cpp"
#include <condition_variable>
#include <future>
namespace udm {
static fs::path output;static Json checks=Json::array(),observations=Json::array();static std::string failure;static bool baseline=false;
static void check(bool ok,const char* name){checks.push_back({{"name",name},{"passed",ok}});if(!ok)throw std::runtime_error(name);}
static std::wstring text(HWND w){wchar_t b[2048]{};GetWindowTextW(w,b,2048);return b;}
static HWND child(HWND w,const wchar_t* label){for(auto c=GetWindow(w,GW_CHILD);c;c=GetWindow(c,GW_HWNDNEXT))if(text(c)==label)return c;throw std::runtime_error("Missing owned completion control");}
struct HeldModel {
 std::mutex mutex;std::condition_variable wake;bool released=false;std::atomic_bool held=false,expired=false;std::thread worker;
 HeldModel(Manager& manager,std::function<void()> change={}){
  std::promise<void> entered;auto ready=entered.get_future();worker=std::thread([this,&manager,change=std::move(change),entered=std::move(entered)]()mutable{Lock lock(manager.mutex);if(change)change();held=true;entered.set_value();std::unique_lock waiting(mutex);expired=!wake.wait_for(waiting,std::chrono::milliseconds(500),[&]{return released;});held=false;});ready.get();
 }
 void open(){{std::lock_guard lock(mutex);released=true;}wake.notify_all();if(worker.joinable())worker.join();}
 ~HeldModel(){open();}
};
struct WarningObserver {
 static inline WarningObserver* active=nullptr;HWND owner;UINT_PTR timer=0;unsigned count=0;std::wstring message;
 static void CALLBACK tick(HWND,UINT,UINT_PTR,DWORD){if(!active)return;EnumThreadWindows(GetCurrentThreadId(),[](HWND w,LPARAM)->BOOL{
  if(GetWindow(w,GW_OWNER)!=active->owner||!IsWindowVisible(w))return TRUE;wchar_t kind[64]{};GetClassNameW(w,kind,64);if(wcscmp(kind,L"#32770"))return TRUE;
  HWND ok=nullptr;std::wstring body;for(auto c=GetWindow(w,GW_CHILD);c;c=GetWindow(c,GW_HWNDNEXT)){if(text(c)==L"OK")ok=c;else if(!text(c).empty())body=text(c);}if(ok){++active->count;active->message=body;PostMessageW(ok,BM_CLICK,0,0);}return TRUE;
 },0);}
 explicit WarningObserver(HWND window):owner(window){active=this;timer=SetTimer(nullptr,0,10,tick);if(!timer)throw std::runtime_error("Cannot observe owned warning");}
 ~WarningObserver(){KillTimer(nullptr,timer);active=nullptr;}
};
class ToolbarComponentTest {
public:static void run(MainWindow& frame){
 auto& manager=frame.manager;frame.modelMaintenance.stop();frame.presentationAcks.stop();
 for(bool dark:{false,true}){
  if(baseline&&dark)break;uiDark=dark;
  for(const auto* label:{L"Open",L"Open with..."}){
   if(baseline&&wcscmp(label,L"Open"))break;
   auto job=manager.add("https://example.invalid/owned-completion","",dark?"dark.bin":"light.bin","Main queue",true);job->data["Status"]="Complete";job->data["Size"]=4;job->data["Received"]=4;
   frame.showCompletionDialog(job,false);auto form=frame.completions.back().get();auto window=form->GetSafeHwnd();auto button=child(window,label);const auto missing=output/(dark?L"dark renamed 文件.bin":L"light renamed 文件.bin");
   unsigned dispatched=0;fs::path dispatchedPath;
#ifdef UDM_COMPLETION_ACTION_QUEUE_TEST_AVAILABLE
   frame.completionActionObserver=[&](const char*,const fs::path& path){++dispatched;dispatchedPath=path;};
#endif
   WarningObserver warnings(window);HeldModel writer(manager,[&]{job->data["Folder"]=utf8(output.wstring());job->data["FileName"]=utf8(missing.filename().wstring());});
   const auto start=GetTickCount64();SendMessageW(button,BM_CLICK,0,0);const auto elapsed=GetTickCount64()-start;
   observations.push_back({{"dark",dark},{"action",utf8(label)},{"dispatchMs",elapsed},{"writerStillHeld",writer.held.load()},{"writerWatchdogExpired",writer.expired.load()}});
   if(baseline){check(writer.expired&&elapsed>=300,"Frozen completion Open waits for the catalog writer on its UI thread");check(warnings.count>0,"The frozen action subsequently reaches the owned missing-file warning");writer.open();SendMessageW(child(window,L"Close"),BM_CLICK,0,0);continue;}
   check(writer.held&&!writer.expired&&elapsed<250,"Completion command returns while the model writer remains held");
   check(warnings.count==0&&dispatched==0,"No path or shell command is used before live validation");
   check(!IsWindowEnabled(button)&&IsWindowEnabled(child(window,L"Close")),"Pending file action disables duplicates and leaves Close usable");
   SendMessageW(window,WM_COMMAND,MAKEWPARAM(GetDlgCtrlID(button),BN_CLICKED),(LPARAM)button);form->SendMessage(WM_TIMER,Form::pulseTimer,0);
   check(writer.held&&!writer.expired&&warnings.count==0&&dispatched==0,"Queued duplicate notifications and timer paint do not wait for the writer");
   writer.open();form->SendMessage(WM_TIMER,Form::pulseTimer,0);
   check(warnings.count>0&&warnings.message.find(missing.wstring())!=std::wstring::npos,"The queued command resolves the latest Unicode destination after release");
   check(dispatched==1&&dispatchedPath==missing&&IsWindow(window),"Exactly one canonical file action runs and its missing-file dialog remains usable");
   form->SendMessage(WM_TIMER,Form::pulseTimer,0);check(dispatched==1&&IsWindowEnabled(button),"A later pulse cannot replay the completed command");
   SendMessageW(child(window,L"Close"),BM_CLICK,0,0);
  }
  if(baseline)continue;
  for(const auto* label:{L"Open",L"Open with...",L"Open folder"}){
   auto job=manager.add("https://example.invalid/owned-cancel","","cancel.bin","Main queue",true);job->data["Status"]="Complete";job->data["Size"]=4;job->data["Received"]=4;frame.showCompletionDialog(job,false);auto form=frame.completions.back().get();const auto window=form->GetSafeHwnd();unsigned dispatched=0;
#ifdef UDM_COMPLETION_ACTION_QUEUE_TEST_AVAILABLE
   frame.completionActionObserver=[&](const char*,const fs::path&){++dispatched;};
#endif
   HeldModel writer(manager);SendMessageW(child(window,label),BM_CLICK,0,0);const auto start=GetTickCount64();SendMessageW(child(window,L"Close"),BM_CLICK,0,0);
   check(!IsWindow(window)&&writer.held&&!writer.expired&&GetTickCount64()-start<250,"Close cancels each pending file action while the writer remains held");writer.open();if(form->pulse)form->pulse();check(dispatched==0,"A destroyed completion cannot replay a queued file or folder action");
  }
  auto job=manager.add("https://example.invalid/owned-scan","","scan.bin","Main queue",true);job->data["Status"]="Complete";frame.showCompletionDialog(job,false);auto form=frame.completions.back().get();auto window=form->GetSafeHwnd();unsigned dispatched=0;
#ifdef UDM_COMPLETION_ACTION_QUEUE_TEST_AVAILABLE
  frame.completionActionObserver=[&](const char*,const fs::path&){++dispatched;};
#endif
  HeldModel writer(manager,[&]{job->data["ScanResult"]={{"Status","Pending"}};});SendMessageW(child(window,L"Open"),BM_CLICK,0,0);writer.open();form->SendMessage(WM_TIMER,Form::pulseTimer,0);
  check(dispatched==0&&!IsWindowEnabled(child(window,L"Open"))&&IsWindowEnabled(child(window,L"Open folder"))&&IsWindow(window),"A scan accepted during contention invalidates the queued Open instead of launching stale bytes");SendMessageW(child(window,L"Close"),BM_CLICK,0,0);
 }
#ifdef UDM_COMPLETION_ACTION_QUEUE_TEST_AVAILABLE
 frame.completionActionObserver={};
#else
 if(!baseline)throw std::runtime_error("Candidate completion action observation is unavailable");
#endif
 }
};
class CompletionContentionApplication:public CWinApp {
 int result=1;bool started=false;std::unique_ptr<Manager> manager;MainWindow* frame=nullptr;
 void finish(){if(frame){closeOwnedMainWindow(frame);frame=nullptr;m_pMainWnd=nullptr;}manager.reset();if(!output.empty())atomicText(output/L"results.json",Json{{"passed",result==0},{"error",failure},{"checks",checks},{"observations",observations},{"baseline",baseline}}.dump(2),false);}
public:BOOL InitInstance()override{
 CWinApp::InitInstance();INITCOMMONCONTROLSEX common{sizeof(common),ICC_WIN95_CLASSES|ICC_DATE_CLASSES};InitCommonControlsEx(&common);AfxOleInit();int argc=0;auto argv=CommandLineToArgvW(GetCommandLineW(),&argc);
 try{if(!argv||argc!=3)throw std::runtime_error("Pass mode and fresh output");baseline=utf8(argv[1])=="baseline";if(!baseline&&utf8(argv[1])!="candidate")throw std::runtime_error("Invalid fixture mode");output=fs::absolute(argv[2]);if(fs::exists(output)){output.clear();throw std::runtime_error("Use fresh output");}fs::create_directories(output);manager=std::make_unique<Manager>(output/L"state");auto prefs=manager->state["Settings"];prefs["DownloadFolder"]=utf8((output/L"files").wstring());prefs["CategoryFolders"]=false;prefs["ClipboardMonitor"]=false;prefs["Sound"]=false;prefs["CloseToTray"]=false;manager->setSettings(prefs);frame=new MainWindow(*manager);m_pMainWnd=frame;frame->KillTimer(1);}catch(const std::exception& e){failure=e.what();if(argv)LocalFree(argv);finish();return FALSE;}LocalFree(argv);return TRUE;
 }
 BOOL OnIdle(LONG)override{if(started)return FALSE;started=true;try{ToolbarComponentTest::run(*frame);result=0;}catch(const std::exception& e){failure=e.what();}finish();PostQuitMessage(result);return FALSE;}
 int ExitInstance()override{AfxOleTerm(FALSE);return result;}
};CompletionContentionApplication application;
}
