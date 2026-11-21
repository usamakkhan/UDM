#define UDM_TOOLBAR_COMPONENT_TEST
#include "App.cpp"
namespace udm {
static fs::path output;static Json checks=Json::array(),closeDiagnostics=Json::object();static std::string failure;
static void check(bool value,const char* name){checks.push_back({{"name",name},{"passed",value}});if(!value)throw std::runtime_error(name);}
static double clockMs(){LARGE_INTEGER now,frequency;QueryPerformanceCounter(&now);QueryPerformanceFrequency(&frequency);return 1000.0*now.QuadPart/frequency.QuadPart;}
struct Hold {std::promise<void> release;std::thread writer;std::atomic_bool held=false,expired=false;bool opened=false;Hold(Manager& manager,bool tray){auto ready=release.get_future().share();writer=std::thread([this,&manager,ready,tray]{Lock lock(manager.mutex);manager.state["Settings"]["CloseToTray"]=tray;held=true;if(ready.wait_for(std::chrono::milliseconds(400))!=std::future_status::ready)expired=true;held=false;});const auto end=GetTickCount64()+3000;while(!held&&GetTickCount64()<end)Sleep(1);}void open(){if(!opened){opened=true;release.set_value();}if(writer.joinable())writer.join();}~Hold(){open();}};
static void pump(){MSG message;while(PeekMessageW(&message,nullptr,0,0,PM_REMOVE)){if(message.message==WM_QUIT)continue;TranslateMessage(&message);DispatchMessageW(&message);}}
class ToolbarComponentTest {
public:static void cleanup(MainWindow*& frame){if(!frame)return;closeOwnedMainWindow(frame);frame=nullptr;}
 static void run(MainWindow*& frame,Manager& manager,const std::string& mode){
  frame->ShowWindow(SW_SHOWNOACTIVATE);frame->UpdateWindow();const auto window=frame->GetSafeHwnd();int stops=0;frame->stopIntegration=[&]{++stops;};
  const auto disk=readText(manager.root/L"state.json");Hold writer(manager,mode!="exit-decision");std::string error;Json sample;
  try{const auto start=clockMs();frame->SendMessage(WM_CLOSE);sample["closeReturnMs"]=clockMs()-start;sample["writerHeldAtReturn"]=writer.held.load();sample["writerExpiredAtReturn"]=writer.expired.load();
   check(writer.held&&!writer.expired&&IsWindow(window),"The actual main Close returns before its policy writer releases");
   frame->SendMessage(WM_CLOSE);check(writer.held&&!writer.expired&&IsWindow(window),"Duplicate Close does not wait for or destroy the pending policy owner");
   frame->SendMessage(WM_TIMER,1,0);frame->Invalidate();frame->UpdateWindow();check(writer.held&&!writer.expired,"Main timer and paint return while the close policy is waiting");
   if(mode=="cancel-via-show"){frame->SendMessage(SHOW_APP,0,0);check(writer.held&&!writer.expired&&IsWindowVisible(window),"Show app cancels a waiting tray decision without waiting for Manager");}
   writer.open();if(mode=="exit-decision")AfxGetApp()->m_pMainWnd=nullptr;
   const auto end=GetTickCount64()+6000;while(GetTickCount64()<end){pump();if(!IsWindow(window)||!IsWindowVisible(window)||mode=="cancel-via-show")break;Sleep(1);}
   if(mode=="exit-decision"){check(!IsWindow(window)&&stops==1,"A fresh non-tray policy continues the real shutdown exactly once");frame=nullptr;}
   else if(mode=="cancel-via-show"){for(int i=0;i<10;++i){pump();Sleep(1);}check(IsWindow(window)&&IsWindowVisible(window)&&stops==0,"A cancelled tray decision cannot hide the restored app later");}
   else{check(IsWindow(window)&&!IsWindowVisible(window)&&stops==0,"Fresh close-to-tray policy hides the window without stopping integration");check(readText(manager.root/L"state.json")==disk,"A tray policy read never saves layout or changes the fixture catalog");}
  }catch(const std::exception& failed){error=failed.what();writer.open();}
  sample["error"]=error;sample["integrationStops"]=stops;closeDiagnostics[mode]=std::move(sample);if(frame)frame->stopIntegration={};check(error.empty(),"The owned main close workflow completes without a held writer or stale result");
 }
};
class MainClosePolicyApplication:public CWinApp {
 int result=1;public:BOOL InitInstance()override{CWinApp::InitInstance();INITCOMMONCONTROLSEX controls{sizeof(controls),ICC_WIN95_CLASSES|ICC_DATE_CLASSES};InitCommonControlsEx(&controls);AfxOleInit();int argc=0;auto args=CommandLineToArgvW(GetCommandLineW(),&argc);
 try{if(!args||argc!=2)throw std::runtime_error("Pass one fresh output directory");output=args[1];if(fs::exists(output)){output.clear();throw std::runtime_error("Fresh output required");}fs::create_directories(output);
  for(const std::string mode:{"tray-busy","tray-late-setting","cancel-via-show","exit-decision"}){Manager manager(output/wide(mode));auto settings=manager.state["Settings"];settings["CloseToTray"]=mode!="tray-late-setting"&&mode!="exit-decision";settings["ClipboardMonitor"]=false;settings["Sound"]=false;settings["ConfirmExit"]=false;settings["PrefetchFileInfo"]=false;settings["SuppressCompletionDialog"]=true;settings["SuppressProgressDialog"]=true;settings["ShowTipsOnStartup"]=false;settings["DownloadFolder"]=utf8((output/wide(mode)/L"files").wstring());settings["CategoryFolders"]=false;manager.setSettings(settings);MainWindow* frame=new MainWindow(manager);m_pMainWnd=frame;try{ToolbarComponentTest::run(frame,manager,mode);}catch(...){m_pMainWnd=nullptr;ToolbarComponentTest::cleanup(frame);throw;}m_pMainWnd=nullptr;ToolbarComponentTest::cleanup(frame);manager.stop();}
  result=0;
 }catch(const std::exception& error){failure=error.what();}if(args)LocalFree(args);if(!output.empty())atomicText(output/L"results.json",Json{{"passed",result==0},{"checks",checks},{"error",failure},{"diagnostics",closeDiagnostics}}.dump(2),false);return FALSE;}
 int ExitInstance()override{AfxOleTerm(FALSE);return result;}
};MainClosePolicyApplication application;
}
