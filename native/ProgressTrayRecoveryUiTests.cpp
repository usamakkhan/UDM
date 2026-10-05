// Drives and renders only native windows owned by this isolated test executable.
#define UDM_TOOLBAR_COMPONENT_TEST
#include "App.cpp"
namespace udm {
static Json results=Json::array();static fs::path output;static std::string failure;static std::wstring expectedTitle;static std::function<void(HWND)> action;static ULONGLONG began;
static void expect(bool value,const char* name){results.push_back({{"name",name},{"passed",value}});if(!value)throw std::runtime_error(name);}
static Json mainQueue(Manager& m){for(const auto& q:m.state["Queues"])if(str(q,"Name")=="Main queue")return q;throw std::runtime_error("Missing Main queue fixture");}
static std::wstring windowText(HWND w){wchar_t text[512]{};GetWindowTextW(w,text,512);return text;}
static HWND child(HWND parent,const wchar_t* label){for(HWND c=GetWindow(parent,GW_CHILD);c;c=GetWindow(c,GW_HWNDNEXT))if(windowText(c)==label&&IsWindowVisible(c))return c;throw std::runtime_error("Missing owned test control: "+utf8(label));}
static std::vector<HWND> children(HWND parent,const wchar_t* type){std::vector<HWND> result;for(HWND c=GetWindow(parent,GW_CHILD);c;c=GetWindow(c,GW_HWNDNEXT)){wchar_t name[128]{};GetClassNameW(c,name,128);if(!_wcsicmp(name,type))result.push_back(c);}return result;}
static void press(HWND w,const wchar_t* label){auto c=child(w,label);expect(IsWindowEnabled(c)!=FALSE,"Requested native button is enabled");SendMessageW(c,BM_CLICK,0,0);}
static void selectCombo(HWND parent,HWND control,int index){SendMessageW(control,CB_SETCURSEL,index,0);SendMessageW(parent,WM_COMMAND,MAKEWPARAM(GetDlgCtrlID(control),CBN_SELCHANGE),(LPARAM)control);}
static CRect bounds(HWND w){CRect r;GetWindowRect(w,&r);return r;}
static void capture(HWND w,const wchar_t* name){
 ShowWindow(w,SW_SHOWNOACTIVATE);RedrawWindow(w,nullptr,nullptr,RDW_INVALIDATE|RDW_ALLCHILDREN|RDW_UPDATENOW|RDW_ERASE);auto r=bounds(w);CImage img;img.Create(r.Width(),r.Height(),32);auto dc=img.GetDC();RECT fill{0,0,r.Width(),r.Height()};FillRect(dc,&fill,GetSysColorBrush(COLOR_BTNFACE));SendMessageW(w,WM_PRINT,(WPARAM)dc,PRF_NONCLIENT|PRF_CLIENT|PRF_ERASEBKGND|PRF_CHILDREN);
 for(HWND c=GetWindow(w,GW_CHILD);c;c=GetWindow(c,GW_HWNDNEXT)){if(!(GetWindowLongW(c,GWL_STYLE)&WS_VISIBLE))continue;auto cr=bounds(c);auto saved=SaveDC(dc);IntersectClipRect(dc,cr.left-r.left,cr.top-r.top,cr.right-r.left,cr.bottom-r.top);SetViewportOrgEx(dc,cr.left-r.left,cr.top-r.top,nullptr);SendMessageW(c,WM_PRINT,(WPARAM)dc,PRF_NONCLIENT|PRF_CLIENT|PRF_ERASEBKGND|PRF_CHILDREN);RestoreDC(dc,saved);}
 img.ReleaseDC();if(FAILED(img.Save((output/name).c_str())))throw std::runtime_error("Could not render native test dialog");
}
static BOOL CALLBACK observe(HWND w,LPARAM){if(windowText(w)!=expectedTitle)return TRUE;try{ShowWindow(w,SW_SHOWNOACTIVATE);action(w);}catch(const std::exception& e){failure=e.what();PostMessageW(w,WM_CLOSE,0,0);}return FALSE;}
static void CALLBACK onTimer(HWND,UINT,UINT_PTR,DWORD){if(GetTickCount64()-began>12000){failure="Owned dialog test deadline";EnumThreadWindows(GetCurrentThreadId(),[](HWND w,LPARAM)->BOOL{PostMessageW(w,WM_CLOSE,0,0);return TRUE;},0);return;}EnumThreadWindows(GetCurrentThreadId(),observe,0);}
static void modal(Form& dialog,const wchar_t* caption,std::function<void(HWND)> callback){expectedTitle=caption;action=std::move(callback);began=GetTickCount64();auto timer=SetTimer(nullptr,0,80,onTimer);dialog.DoModal();KillTimer(nullptr,timer);action={};if(!failure.empty())throw std::runtime_error(failure);}

class ProgressTrayRecoveryApplication:public CWinApp {
 int code=1;
public:BOOL InitInstance()override{
 CWinApp::InitInstance();INITCOMMONCONTROLSEX common{sizeof(common),ICC_WIN95_CLASSES};InitCommonControlsEx(&common);AfxOleInit();int argc=0;auto argv=CommandLineToArgvW(GetCommandLineW(),&argc);
 try{if(argc!=2)throw std::runtime_error("Pass isolated output directory");output=fs::path(argv[1]);fs::create_directories(output);Manager manager(output/L"state");auto prefs=manager.state["Settings"];prefs["PrefetchFileInfo"]=false;manager.setSettings(prefs);
 auto first=manager.receive({{"action","add"},{"url","https://example.invalid/first.bin"},{"downloadLater",true}});auto second=manager.receive({{"action","add"},{"url","https://example.invalid/second.bin"},{"downloadLater",true}});
 CFrameWnd owner;expect(owner.Create(nullptr,L"UDM isolated tray owner",WS_OVERLAPPEDWINDOW,CRect(0,0,40,40))!=FALSE,"Create shared owner matching desktop topology");m_pMainWnd=&owner;Progress a(manager,first,&owner),b(manager,second,&owner);expect(a.Create(101,&owner)!=FALSE&&b.Create(101,&owner)!=FALSE,"Create isolated progress windows");
 auto iconExists=[](HWND w){NOTIFYICONIDENTIFIER id{};id.cbSize=sizeof(id);id.hWnd=w;id.uID=1;RECT r{};return SUCCEEDED(Shell_NotifyIconGetRect(&id,&r));};
 expect(a.showInTray(),"Shell accepts first download icon");expect(b.showInTray(),"Shell accepts independent second download icon");expect(!a.IsWindowVisible()&&!b.IsWindowVisible(),"Tray downloads do not show progress windows");
 expect(iconExists(a.m_hWnd)&&iconExists(b.m_hWnd),"Both per-window icon identities exist in the shell");
 {Lock lock(manager.mutex);first->data["Size"]=1000;first->data["Received"]=370;first->data["FileName"]="updated-name.bin";}
 SendMessageW(a.m_hWnd,WM_TIMER,1,0);
 expect(std::wstring(a.trayIcon.szTip)==L"37% updated-name.bin","Hidden download tooltip follows current percentage and renamed file");
 expect(!a.IsWindowVisible()&&iconExists(a.m_hWnd)&&iconExists(b.m_hWnd),"Tooltip refresh preserves hidden presentation and both shell icons");
 {Lock lock(manager.mutex);first->data["Size"]=-1;first->data["FileName"]=std::string(180,'x')+".bin";}
 SendMessageW(a.m_hWnd,WM_TIMER,1,0);
 expect(wcslen(a.trayIcon.szTip)==127&&a.trayIcon.szTip[0]==L'x',"Unknown-size long filenames produce bounded updated shell tooltips");
 SendMessageW(a.m_hWnd,WM_TIMER,1,0);
 expect(!a.IsWindowVisible()&&iconExists(a.m_hWnd),"Unchanged tooltip refresh leaves its shell icon intact");
 SendMessageW(a.m_hWnd,WM_APP+144,0,WM_LBUTTONUP);expect(a.IsWindowVisible()!=FALSE,"Tray callback restores its progress window");expect(!iconExists(a.m_hWnd)&&iconExists(b.m_hWnd),"Restoring removes only the selected download icon");
 expect(a.showInTray(),"Restored download can return to tray");auto hwnd=a.m_hWnd;a.DestroyWindow();expect(!iconExists(hwnd)&&iconExists(b.m_hWnd),"Window destruction removes only its own icon");
 b.showNormally();expect(b.IsWindowVisible()!=FALSE&&!iconExists(b.m_hWnd),"Explicit opening restores and removes tray icon");b.DestroyWindow();
 auto owned=std::make_unique<Progress>(manager,first,&owner);expect(owned->Create(101,&owner)!=FALSE,"Create recovery fixture window");expect(owned->showInTray(),"Recovery fixture enters tray");auto recoveryHwnd=owned->m_hWnd;
 NOTIFYICONDATAW removed{};removed.cbSize=sizeof(removed);removed.hWnd=recoveryHwnd;removed.uID=1;expect(Shell_NotifyIconW(NIM_DELETE,&removed)!=FALSE,"Simulate notification-area icon loss for owned fixture only");expect(!iconExists(recoveryHwnd),"Owned icon is absent before recovery notification");
 SendMessageW(recoveryHwnd,RegisterWindowMessageW(L"TaskbarCreated"),0,0);expect(iconExists(recoveryHwnd)&&!owned->IsWindowVisible(),"Taskbar recreation notification restores icon while retaining tray presentation");
 SendMessageW(recoveryHwnd,WM_APP+144,0,NIN_KEYSELECT);expect(owned->IsWindowVisible()&&!iconExists(recoveryHwnd),"Keyboard tray activation restores the window");expect(owned->showInTray(),"Recovery fixture returns to tray before object teardown");
 owned.reset();bool cleaned=!iconExists(recoveryHwnd);if(!cleaned)Shell_NotifyIconW(NIM_DELETE,&removed);expect(cleaned,"Destroying a progress object removes its shell icon without explicit DestroyWindow");
 expect(progressPresentation("Minimize to system tray",false,true,false)==ProgressPresentation::Tray,"Individual tray startup policy");expect(progressPresentation("Minimize to system tray",false,true,true)==ProgressPresentation::Normal,"Explicit opening overrides tray policy");expect(progressPresentation("Don't show",true,true,false)==ProgressPresentation::Tray,"Queue startup uses tray independently of individual mode");code=0;
 }catch(const std::exception& e){failure=e.what();}if(argv)LocalFree(argv);if(!output.empty())atomicText(output/L"results.json",Json{{"passed",code==0},{"error",failure},{"checks",results}}.dump(2),false);return FALSE;}
 int ExitInstance()override{AfxOleTerm(FALSE);return code;}
};ProgressTrayRecoveryApplication application;
}