#define UDM_TOOLBAR_COMPONENT_TEST
#include "App.cpp"
#include <condition_variable>
#include <future>
namespace udm {
static fs::path output;static Json checks=Json::array(),observations=Json::array();static std::string failure;static bool baseline=false;
static void check(bool ok,const char* label){checks.push_back({{"name",label},{"passed",ok}});if(!ok)throw std::runtime_error(label);}
template<class F>static void until(F ready){const auto deadline=GetTickCount64()+6000;while(!ready()&&GetTickCount64()<deadline)Sleep(1);if(!ready())throw std::runtime_error("Owned close-priority deadline");}
struct PriorityWriter {std::mutex mutex;std::condition_variable wake;bool released=false;std::atomic_bool held=false,expired=false;std::thread worker;explicit PriorityWriter(Manager& manager){std::promise<void> entered;auto ready=entered.get_future();worker=std::thread([this,&manager,entered=std::move(entered)]()mutable{Lock lock(manager.mutex);manager.state["Settings"]["ClosePriorityMarker"]="preserved";held=true;entered.set_value();std::unique_lock waiting(mutex);expired=!wake.wait_for(waiting,std::chrono::seconds(3),[&]{return released;});held=false;});ready.get();}void open(){{std::lock_guard lock(mutex);released=true;}wake.notify_all();if(worker.joinable())worker.join();}~PriorityWriter(){open();}};
static unsigned dispatchOwnedWakes(HWND window){unsigned count=0;MSG message{};while(IsWindow(window)&&PeekMessageW(&message,window,UiTaskSignal::message,UiTaskSignal::message,PM_REMOVE)){++count;DispatchMessageW(&message);}return count;}
class ToolbarComponentTest {
public:static void run(MainWindow*& frame,Manager& manager,const std::string& mode,bool dark){
 frame->KillTimer(1);frame->modelMaintenance.stop();frame->presentationAcks.stop();frame->ShowWindow(SW_SHOWNOACTIVATE);frame->UpdateWindow();const auto window=frame->GetSafeHwnd();MSG stale{};while(PeekMessageW(&stale,window,UiTaskSignal::message,UiTaskSignal::message,PM_REMOVE)){}
 auto job=manager.add("https://example.invalid/close-priority","","priority.bin","Main queue",true);job->data["Status"]="Complete";job->data["Size"]=4;const auto before=frame->completions.size();auto stopped=std::make_shared<unsigned>(0);auto dialogsAtStop=std::make_shared<size_t>(0);frame->stopIntegration=[owner=frame,stopped,dialogsAtStop]{++*stopped;*dialogsAtStop=owner->completions.size();};
 PriorityWriter writer(manager);manager.filePublished(job);frame->OnTransferEvents(0,0);check(writer.held&&!writer.expired&&!frame->pendingCompletions.empty(),"Publication is retained while the catalog is held");const auto start=GetTickCount64();
 if(mode=="tray-timer"||mode=="tray-wake")frame->SendMessage(WM_CLOSE);
 else if(mode=="cancel-policy"||mode=="failed-policy"){
  frame->closePolicyTask.start(manager,frame->maintenanceSignal->notifier(),[mode](const char* point){if(mode=="failed-policy"&&std::string(point)=="before-read")throw std::runtime_error("Owned close-policy read failure");});
  if(mode=="cancel-policy")frame->closePolicyTask.cancelRead();
 }else{
  if(mode=="failed-layout")frame->exitLayoutObserver=[](const char* point){if(std::string(point)=="before-merge")throw std::runtime_error("Owned priority layout failure");};
  frame->SendMessage(WM_COMMAND,CMD_EXIT);
 }
 check(writer.held&&!writer.expired&&GetTickCount64()-start<250&&IsWindow(window),"Close preparation returns while the model writer remains held");
 if(mode=="cancel-policy"){
  until([&]{return frame->closePolicyTask.ready();});frame->OnTimer(1);
  check(!frame->closePolicyTask.valid()&&writer.held&&!writer.expired&&frame->completions.size()==before,"A cancelled close policy is acknowledged ahead of blocked presentation work");writer.open();
 }else{
  writer.open();until([&]{return frame->completionReadiness.ready()&&(mode.find("layout")!=std::string::npos||mode=="exit"?frame->exitLayoutTask.ready():frame->closePolicyTask.ready());});
  if(mode=="tray-wake"){MSG wake{};check(PeekMessageW(&wake,window,UiTaskSignal::message,UiTaskSignal::message,PM_REMOVE)!=FALSE,"A real owned close/readiness wake is queued");DispatchMessageW(&wake);}else frame->OnTimer(1);
 }
 if(mode=="exit"){
  if(!IsWindow(window)){frame=nullptr;AfxGetApp()->m_pMainWnd=nullptr;}
  check(!IsWindow(window)&&*stopped==1&&*dialogsAtStop==before,"Accepted Exit stops exactly once without presenting a pending completion");const auto saved=Json::parse(readText(manager.root/L"state.json"));check(str(saved["Settings"],"ClosePriorityMarker")=="preserved"&&saved["Downloads"].size()==1,"Exit retains concurrent settings and the completed catalog record");return;
 }
 const auto firstCount=frame->completions.size();observations.push_back({{"mode",mode},{"dark",dark},{"dialogsAtFirstCloseDispatch",firstCount-before},{"mainVisibleAfterFirstDispatch",frame->IsWindowVisible()!=FALSE},{"closePolicyStillPending",frame->closePolicyTask.valid()},{"layoutStillPending",frame->exitLayoutTask.valid()}});
 if(baseline){check(firstCount==before+1&&!frame->IsWindowVisible(),"Frozen dispatch opens completion before applying the ready hide-to-tray decision");return;}
#ifdef UDM_COMPLETION_CLOSE_PRIORITY_AVAILABLE
 check(firstCount==before,"The first close decision dispatch creates no completion window");
 if(mode=="tray-timer"||mode=="tray-wake")check(!frame->IsWindowVisible()&&!frame->closePolicyTask.valid(),"The ready close-to-tray decision hides the main window first");
 else check(frame->IsWindowEnabled()&&!frame->quitting&&!frame->closePolicyTask.valid()&&!frame->exitLayoutTask.valid(),"Cancelled or failed close preparation restores usable presentation ownership");
 unsigned wakes=0;const auto deadline=GetTickCount64()+6000;while(!frame->pendingCompletions.empty()&&GetTickCount64()<deadline){wakes+=dispatchOwnedWakes(window);Sleep(1);}
 check(wakes>0&&frame->pendingCompletions.empty()&&frame->completions.size()==before+1,"A subsequent real owned wake resumes one completion without a main timer tick");
 if(mode=="tray-timer"||mode=="tray-wake")check(!frame->IsWindowVisible(),"Resuming completion leaves the main download list hidden");
 frame->pollCompletionWork();check(frame->completions.size()==before+1&&*stopped==0,"A later admission poll cannot replay completion or start shutdown");
#else
 throw std::runtime_error("Close-priority correction is unavailable");
#endif
 }
};
class CompletionClosePriorityApplication:public CWinApp {int result=1;public:BOOL InitInstance()override{CWinApp::InitInstance();INITCOMMONCONTROLSEX controls{sizeof(controls),ICC_WIN95_CLASSES|ICC_DATE_CLASSES|ICC_PROGRESS_CLASS};InitCommonControlsEx(&controls);AfxOleInit();int argc=0;auto args=CommandLineToArgvW(GetCommandLineW(),&argc);
 try{if(!args||argc!=3)throw std::runtime_error("Pass mode and fresh output");baseline=utf8(args[1])=="baseline";if(!baseline&&utf8(args[1])!="candidate")throw std::runtime_error("Invalid fixture mode");output=fs::absolute(args[2]);if(fs::exists(output)){output.clear();throw std::runtime_error("Fresh output required");}fs::create_directories(output);
  for(bool dark:{false,true}){if(baseline&&dark)break;for(const std::string mode:{"tray-timer","tray-wake","cancel-policy","failed-policy","failed-layout","exit"}){if(baseline&&mode!="tray-timer"&&mode!="tray-wake")continue;Manager manager(output/wide((dark?"dark-":"light-")+mode));auto settings=manager.state["Settings"];settings["CloseToTray"]=true;settings["DarkMode"]=dark;settings["ClipboardMonitor"]=false;settings["Sound"]=false;settings["ShowTipsOnStartup"]=false;settings["CategoryFolders"]=false;settings["DownloadFolder"]=utf8((manager.root/L"files").wstring());manager.setSettings(settings);MainWindow* frame=new MainWindow(manager);m_pMainWnd=frame;
   try{ToolbarComponentTest::run(frame,manager,mode,dark);}catch(...){m_pMainWnd=nullptr;closeOwnedMainWindow(frame);throw;}m_pMainWnd=nullptr;closeOwnedMainWindow(frame);manager.stop();}
  }result=0;
 }catch(const std::exception& error){failure=error.what();}if(args)LocalFree(args);if(!output.empty())atomicText(output/L"results.json",Json{{"passed",result==0},{"checks",checks},{"observations",observations},{"baseline",baseline},{"error",failure}}.dump(2),false);return FALSE;}
 int ExitInstance()override{AfxOleTerm(FALSE);return result;}
};CompletionClosePriorityApplication application;
}
