// Isolated fixture: only its own window and notification icon are manipulated.
#define UDM_TOOLBAR_COMPONENT_TEST
#include "App.cpp"
namespace udm {
static Json results=Json::array();
static void expect(bool value,const char* name){results.push_back({{"name",name},{"passed",value}});if(!value)throw std::runtime_error(name);}
class ToolbarComponentTest {
public:static void run(MainWindow& frame){
 auto exists=[&]{NOTIFYICONIDENTIFIER id{};id.cbSize=sizeof(id);id.hWnd=frame.m_hWnd;id.uID=1;RECT r{};return SUCCEEDED(Shell_NotifyIconGetRect(&id,&r));};
 auto recreated=RegisterWindowMessageW(L"TaskbarCreated");
 expect(frame.updateTray()&&exists(),"Main notification icon is registered");
 frame.ShowWindow(SW_HIDE);
 expect(Shell_NotifyIconW(NIM_DELETE,&frame.tray)!=FALSE&&!exists(),"Simulated taskbar loss removes only the fixture icon");
 frame.SendMessage(recreated);
 expect(exists()&&!frame.IsWindowVisible(),"Taskbar recreation restores main icon without revealing hidden window");
 frame.SendMessage(TRAY_MESSAGE,0,NIN_KEYSELECT);
 expect(frame.IsWindowVisible()!=FALSE,"Keyboard tray activation opens the main window");
 frame.ShowWindow(SW_HIDE);frame.preference("TrayIcon","Hidden");frame.updateTray();frame.SendMessage(recreated);
 expect(!exists()&&!frame.IsWindowVisible(),"Dont-show preference survives taskbar recovery");
 frame.preference("TrayIcon","Classic");frame.updateTray();auto classic=frame.tray.hIcon;Shell_NotifyIconW(NIM_DELETE,&frame.tray);frame.SendMessage(recreated);
 expect(exists()&&frame.tray.hIcon==classic,"Classic icon preference survives recovery");
 frame.SendMessage(recreated);expect(exists(),"Repeated recovery notifications retain the icon");
 frame.SendMessage(TRAY_MESSAGE,0,NIN_SELECT);expect(frame.IsWindowVisible()!=FALSE,"Shell selection activates the main window");
 }
};
class MainTrayTestApplication:public CWinApp {
 int code=1;
public:BOOL InitInstance()override{
 CWinApp::InitInstance();INITCOMMONCONTROLSEX common{sizeof(common),ICC_WIN95_CLASSES|ICC_DATE_CLASSES};InitCommonControlsEx(&common);AfxOleInit();
 int argc=0;auto argv=CommandLineToArgvW(GetCommandLineW(),&argc);fs::path output;std::string failure;std::unique_ptr<Manager> manager;MainWindow* frame=nullptr;
 try{if(argc!=2)throw std::runtime_error("Pass isolated output directory");output=argv[1];fs::create_directories(output);manager=std::make_unique<Manager>(output/L"state");auto p=manager->state["Settings"];p["CloseToTray"]=false;p["ClipboardMonitor"]=false;manager->setSettings(p);frame=new MainWindow(*manager);m_pMainWnd=frame;ToolbarComponentTest::run(*frame);code=0;}catch(const std::exception& e){failure=e.what();}
 if(frame){frame->SendMessage(WM_CLOSE);m_pMainWnd=nullptr;}manager.reset();if(argv)LocalFree(argv);
 if(!output.empty())atomicText(output/L"results.json",Json{{"passed",code==0},{"error",failure},{"checks",results}}.dump(2),false);return FALSE;
 }
 int ExitInstance()override{AfxOleTerm(FALSE);return code;}
};MainTrayTestApplication application;
}
