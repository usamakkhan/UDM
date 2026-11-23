#define UDM_TOOLBAR_COMPONENT_TEST
#include "App.cpp"
#include <condition_variable>
#include <future>
namespace udm {
static fs::path output;static Json checks=Json::array(),observations=Json::array();static std::string failure;static bool baseline=false;
static void check(bool ok,const char* name){checks.push_back({{"name",name},{"passed",ok}});if(!ok)throw std::runtime_error(name);}
static HWND named(HWND w,const wchar_t* label){for(auto c=GetWindow(w,GW_CHILD);c;c=GetWindow(c,GW_HWNDNEXT)){wchar_t text[1024]{};GetWindowTextW(c,text,1024);if(!wcscmp(text,label))return c;}throw std::runtime_error("Missing owned completion control");}
struct HeldSuppressionModel {
 std::mutex mutex;std::condition_variable wake;bool released=false;std::atomic_bool held=false,expired=false;std::thread worker;
 HeldSuppressionModel(Manager& manager){std::promise<void> entered;auto ready=entered.get_future();worker=std::thread([this,&manager,entered=std::move(entered)]()mutable{Lock lock(manager.mutex);manager.state["Settings"]["UserAgent"]="Concurrent completion choice";held=true;entered.set_value();std::unique_lock waiting(mutex);expired=!wake.wait_for(waiting,std::chrono::milliseconds(500),[&]{return released;});held=false;});ready.get();}
 void open(){{std::lock_guard lock(mutex);released=true;}wake.notify_all();if(worker.joinable())worker.join();}~HeldSuppressionModel(){open();}
};
class ToolbarComponentTest {
public:static void run(MainWindow& frame){auto& manager=frame.manager;frame.modelMaintenance.stop();frame.presentationAcks.stop();
 for(bool dark:{false,true}){if(baseline&&dark)break;uiDark=dark;auto settings=manager.state["Settings"];settings["SuppressCompletionDialog"]=false;manager.setSettings(settings);
  auto job=manager.add("https://example.invalid/owned-suppression","","suppression.bin","Main queue",true);job->data["Status"]="Complete";job->data["Size"]=4;frame.showCompletionDialog(job,false);auto form=frame.completions.back().get();auto window=form->GetSafeHwnd();auto checkbox=named(window,L"Don't show this dialog again");
  HeldSuppressionModel writer(manager);auto start=GetTickCount64();SendMessageW(checkbox,BM_CLICK,0,0);auto elapsed=GetTickCount64()-start;observations.push_back({{"dark",dark},{"dispatchMs",elapsed},{"writerStillHeld",writer.held.load()},{"watchdogExpired",writer.expired.load()}});
  if(baseline){check(writer.expired&&elapsed>=300,"Frozen completion suppression waits on the UI thread for the catalog writer");writer.open();check(yes(manager.state["Settings"],"SuppressCompletionDialog"),"Frozen suppression eventually persists through its canonical handler");SendMessageW(named(window,L"Close"),BM_CLICK,0,0);continue;}
#ifdef UDM_COMPLETION_SUPPRESSION_TASK_AVAILABLE
  check(writer.held&&!writer.expired&&elapsed<250,"Suppression returns before the held catalog writer releases");check(frame.completionSettingsTask.valid()&&manager.fileInfoDecisionPending&&!IsWindowEnabled(checkbox),"Accepted save owns its guard and disables duplicate checkbox input");check(IsWindowEnabled(named(window,L"Close")),"Close remains usable during the accepted preference save");
  frame.OnTimer(1);check(writer.held&&!writer.expired&&frame.completionSettingsTask.valid(),"Main timer stays responsive while suppression waits for the writer");
  SendMessageW(named(window,L"Close"),BM_CLICK,0,0);check(!IsWindow(window)&&writer.held&&!writer.expired,"Closing completion does not block or abandon its accepted save");writer.open();
  auto deadline=GetTickCount64()+5000;while(!frame.completionSettingsTask.ready()&&GetTickCount64()<deadline)Sleep(1);check(frame.completionSettingsTask.ready(),"The accepted suppression save reaches a terminal result after writer release");frame.pollCompletionSettings();check(!frame.completionSettingsTask.valid()&&!manager.fileInfoDecisionPending&&yes(manager.state["Settings"],"SuppressCompletionDialog"),"Acknowledgement releases the guard and retains the saved choice after dialog Close");check(str(manager.state["Settings"],"UserAgent")=="Concurrent completion choice","Suppression merge preserves unrelated concurrent settings");
  {Manager reopened(manager.root);check(yes(reopened.state["Settings"],"SuppressCompletionDialog"),"Accepted suppression remains durable after the originating dialog is destroyed");}
  frame.showCompletionDialog(job,true);auto later=frame.completions.back()->GetSafeHwnd();auto laterCheck=named(later,L"Don't show this dialog again");check(SendMessageW(laterCheck,BM_GETCHECK,0,0)==BST_CHECKED,"A later explicit completion reflects the saved preference");SendMessageW(laterCheck,BM_CLICK,0,0);deadline=GetTickCount64()+5000;while(!frame.completionSettingsTask.ready()&&GetTickCount64()<deadline)Sleep(1);check(frame.completionSettingsTask.ready(),"A later unchecked choice completes asynchronously");frame.pollCompletionSettings();check(!yes(manager.state["Settings"],"SuppressCompletionDialog")&&IsWindowEnabled(laterCheck),"A later unchecked choice restores automatic dialogs and input");SendMessageW(named(later,L"Close"),BM_CLICK,0,0);
#else
  throw std::runtime_error("Candidate suppression task unavailable");
#endif
 }
 }
};
class CompletionSuppressionApplication:public CWinApp {
 int result=1;bool started=false;std::unique_ptr<Manager> manager;MainWindow* frame=nullptr;
 void finish(){if(frame){closeOwnedMainWindow(frame);frame=nullptr;m_pMainWnd=nullptr;}manager.reset();if(!output.empty())atomicText(output/L"results.json",Json{{"passed",result==0},{"error",failure},{"checks",checks},{"observations",observations},{"baseline",baseline}}.dump(2),false);}
public:BOOL InitInstance()override{CWinApp::InitInstance();INITCOMMONCONTROLSEX common{sizeof(common),ICC_WIN95_CLASSES|ICC_DATE_CLASSES};InitCommonControlsEx(&common);AfxOleInit();int argc=0;auto argv=CommandLineToArgvW(GetCommandLineW(),&argc);try{if(!argv||argc!=3)throw std::runtime_error("Pass mode and fresh output");baseline=utf8(argv[1])=="baseline";if(!baseline&&utf8(argv[1])!="candidate")throw std::runtime_error("Invalid fixture mode");output=fs::absolute(argv[2]);if(fs::exists(output)){output.clear();throw std::runtime_error("Use fresh output");}fs::create_directories(output);manager=std::make_unique<Manager>(output/L"state");auto prefs=manager->state["Settings"];prefs["DownloadFolder"]=utf8((output/L"files").wstring());prefs["CategoryFolders"]=false;prefs["ClipboardMonitor"]=false;prefs["Sound"]=false;prefs["CloseToTray"]=false;manager->setSettings(prefs);frame=new MainWindow(*manager);m_pMainWnd=frame;frame->KillTimer(1);}catch(const std::exception& e){failure=e.what();if(argv)LocalFree(argv);finish();return FALSE;}LocalFree(argv);return TRUE;}
 BOOL OnIdle(LONG)override{if(started)return FALSE;started=true;try{ToolbarComponentTest::run(*frame);result=0;}catch(const std::exception& e){failure=e.what();}finish();PostQuitMessage(result);return FALSE;}int ExitInstance()override{AfxOleTerm(FALSE);return result;}
};CompletionSuppressionApplication application;
}
