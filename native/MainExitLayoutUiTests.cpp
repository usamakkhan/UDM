#define UDM_TOOLBAR_COMPONENT_TEST
#include "App.cpp"
namespace udm {
static fs::path output;static Json checks=Json::array(),exitDiagnostics=Json::object();static std::string failure;
static void check(bool value,const char* name){checks.push_back({{"name",name},{"passed",value}});if(!value)throw std::runtime_error(name);}
static double clockMs(){LARGE_INTEGER time{},frequency{};QueryPerformanceCounter(&time);QueryPerformanceFrequency(&frequency);return 1000.0*time.QuadPart/frequency.QuadPart;}
struct ExitWriter {
 std::promise<void> release,entered;std::shared_future<void> gate=release.get_future().share();std::atomic_bool held=false,expired=false;std::thread thread;
 explicit ExitWriter(Manager& manager):thread([this,&manager]{Lock lock(manager.mutex);manager.state["Settings"]["ExitConcurrentMarker"]="preserved";held=true;entered.set_value();if(gate.wait_for(std::chrono::seconds(2))!=std::future_status::ready)expired=true;held=false;}){auto ready=entered.get_future();if(ready.wait_for(std::chrono::seconds(3))!=std::future_status::ready){open();throw std::runtime_error("Exit writer did not enter");}}
 void open(){try{release.set_value();}catch(...){}if(thread.joinable())thread.join();}~ExitWriter(){open();}
};
static void pumpExit(){MSG msg{};while(PeekMessageW(&msg,nullptr,0,0,PM_REMOVE)){if(msg.message==WM_QUIT)continue;TranslateMessage(&msg);DispatchMessageW(&msg);}}
class ToolbarComponentTest {
public:
 static void cleanup(MainWindow*& frame){if(!frame)return;
#ifdef UDM_MAIN_EXIT_LAYOUT_AVAILABLE
  frame->exitLayoutObserver={};closeOwnedMainWindow(frame);
#else
  frame->SendMessage(WM_COMMAND,CMD_EXIT);frame=nullptr;
#endif
 }
 static void run(MainWindow*& frame,Manager& manager,const std::string& mode){
  frame->KillTimer(1);frame->ShowWindow(SW_SHOWNOACTIVATE);frame->UpdateWindow();
  frame->table.SetColumnWidth(0,frame->table.GetColumnWidth(0)+17);const auto expected=frame->listLayout();
  const auto oldLayout=manager.state["Settings"].value("ListLayout",Json());const auto disk=readText(manager.root/L"state.json");auto stops=std::make_shared<int>(0);frame->stopIntegration=[stops]{++*stops;};
#ifdef UDM_MAIN_EXIT_LAYOUT_AVAILABLE
  auto failOnce=std::make_shared<std::atomic_bool>(true);
  if(mode=="failure-retry")frame->exitLayoutObserver=[failOnce](const char* point){if(std::string(point)=="before-merge"&&failOnce->exchange(false))throw std::runtime_error("Owned layout merge failure");};
  if(mode=="destroy-pending"){frame->modelMaintenance.stop();frame->presentationAcks.stop();frame->authenticationTask->finish();}
#endif
  const auto window=frame->GetSafeHwnd();ExitWriter writer(manager);const auto begin=clockMs();frame->SendMessage(WM_COMMAND,CMD_EXIT);const auto returned=clockMs();
  if(!IsWindow(window)){frame=nullptr;AfxGetApp()->m_pMainWnd=nullptr;}
  exitDiagnostics[mode]={{"exitReturnMs",returned-begin},{"writerHeldAtReturn",writer.held.load()},{"writerExpiredAtReturn",writer.expired.load()}};
  check(writer.held&&!writer.expired&&IsWindow(window),"Actual CMD_EXIT returns while its layout writer still owns Manager");
#ifdef UDM_MAIN_EXIT_LAYOUT_AVAILABLE
  check(frame->exitLayoutTask.valid()&&!frame->IsWindowEnabled()&&*stops==0,"One owned layout stage disables new input before stopping integration");
  frame->SendMessage(WM_COMMAND,CMD_EXIT);frame->SendMessage(WM_CLOSE);frame->SendMessage(SHOW_APP);frame->SendMessage(WM_NULL);frame->SendMessage(WM_TIMER,1,0);frame->Invalidate();frame->UpdateWindow();
  check(writer.held&&!writer.expired&&frame->exitLayoutTask.valid()&&!frame->IsWindowEnabled()&&*stops==0,"Duplicate Exit Close Show timer and paint preserve the pending owner without waiting");
  check(readText(manager.root/L"state.json")==disk,"Exit preparation does not add an early catalog save");
  if(mode=="destroy-pending"){
   const bool destroyed=frame->DestroyWindow()!=FALSE;if(destroyed){frame=nullptr;AfxGetApp()->m_pMainWnd=nullptr;}
   check(destroyed&&writer.held&&!writer.expired&&manager.pendingModelWork()==1,"Direct destruction returns while Manager retains the cancelled acquisition");writer.open();
   check(manager.state["Settings"].value("ListLayout",Json())==oldLayout&&*stops==0,"A cancelled unaccepted layout stage neither merges nor runs shutdown integration");return;
  }
  writer.open();const auto deadline=GetTickCount64()+6000;
  while(GetTickCount64()<deadline&&IsWindow(window)){pumpExit();if(!IsWindow(window)){frame=nullptr;AfxGetApp()->m_pMainWnd=nullptr;break;}if(mode=="failure-retry"&&!frame->exitLayoutTask.valid())break;Sleep(1);}
  if(mode=="failure-retry"){
   check(IsWindow(window)&&frame->IsWindowEnabled()&&!frame->quitting&&*stops==0,"A layout failure restores usable input before any shutdown work");
   check(manager.state["Settings"].value("ListLayout",Json())==oldLayout&&readText(manager.root/L"state.json")==disk,"A failed merge preserves the prior layout and saved catalog");
   frame->table.SetColumnWidth(0,frame->table.GetColumnWidth(0)+13);const auto retryLayout=frame->listLayout();frame->SendMessage(WM_COMMAND,CMD_EXIT);
   const auto end=GetTickCount64()+6000;while(IsWindow(window)&&GetTickCount64()<end){pumpExit();Sleep(1);}
   if(!IsWindow(window)){frame=nullptr;AfxGetApp()->m_pMainWnd=nullptr;}
   check(!IsWindow(window)&&*stops==1,"Retry completes actual Exit and stops integration exactly once");
   const auto saved=Json::parse(readText(manager.root/L"state.json"));check(saved["Settings"]["ListLayout"]==retryLayout&&str(saved["Settings"],"ExitConcurrentMarker")=="preserved","Retry saves the newest layout and retains the concurrent settings change");
  }else{
   if(!IsWindow(window)){frame=nullptr;AfxGetApp()->m_pMainWnd=nullptr;}
   check(!IsWindow(window)&&*stops==1,"Prepared layout continues the real shutdown exactly once");const auto saved=Json::parse(readText(manager.root/L"state.json"));
   check(saved["Settings"]["ListLayout"]==expected&&str(saved["Settings"],"ExitConcurrentMarker")=="preserved","The final canonical checkpoint contains the captured layout and concurrent settings");
  }
  exitDiagnostics[mode]["exitAndManagerStopMs"]=clockMs()-begin;
#endif
 }
};
class MainExitLayoutApplication:public CWinApp {
 int result=1;public:BOOL InitInstance()override{CWinApp::InitInstance();INITCOMMONCONTROLSEX controls{sizeof(controls),ICC_WIN95_CLASSES|ICC_DATE_CLASSES|ICC_PROGRESS_CLASS};InitCommonControlsEx(&controls);AfxOleInit();int argc=0;auto args=CommandLineToArgvW(GetCommandLineW(),&argc);
 try{if(!args||argc!=2)throw std::runtime_error("Fresh output required");output=args[1];if(fs::exists(output)){output.clear();throw std::runtime_error("Fresh output required");}fs::create_directories(output);
  for(const std::string mode:{"busy-exit","failure-retry","destroy-pending"}){Manager manager(output/wide(mode));auto settings=manager.state["Settings"];settings["CloseToTray"]=false;settings["ClipboardMonitor"]=false;settings["Sound"]=false;settings["ShowTipsOnStartup"]=false;settings["CategoryFolders"]=false;settings["DownloadFolder"]=utf8((output/wide(mode)/L"files").wstring());manager.setSettings(settings);MainWindow* frame=new MainWindow(manager);m_pMainWnd=frame;
   try{ToolbarComponentTest::run(frame,manager,mode);}catch(...){m_pMainWnd=nullptr;ToolbarComponentTest::cleanup(frame);throw;}m_pMainWnd=nullptr;ToolbarComponentTest::cleanup(frame);manager.stop();}
  result=0;
 }catch(const std::exception& error){failure=error.what();}if(args)LocalFree(args);if(!output.empty())atomicText(output/L"results.json",Json{{"passed",result==0},{"checks",checks},{"error",failure},{"diagnostics",exitDiagnostics}}.dump(2),false);return FALSE;}
 int ExitInstance()override{AfxOleTerm(FALSE);return result;}
};MainExitLayoutApplication application;
}
