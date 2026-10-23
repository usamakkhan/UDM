// Exercises only the About dialog of this isolated fixture against the IDM
// 6.43 capture (docs/gui-parity-2026-10-18/idm-about.uia.txt).
#define UDM_TOOLBAR_COMPONENT_TEST
#include "App.cpp"
namespace udm {
static CRect bounds(HWND window){CRect r;::GetWindowRect(window,&r);return r;}
static bool shown(HWND window){return (::GetWindowLongW(window,GWL_STYLE)&WS_VISIBLE)!=0;}
static Json results=Json::array();static std::string failure;static std::function<void(HWND)> action;static ULONGLONG began;
static void expect(bool value,const char* name){results.push_back({{"name",name},{"passed",value}});if(!value)throw std::runtime_error(name);}
static std::wstring label(HWND w){wchar_t text[512]{};GetWindowTextW(w,text,512);std::wstring value=text;value.erase(std::remove(value.begin(),value.end(),L'&'),value.end());return value;}
static HWND named(HWND w,const wchar_t* name){for(auto c=GetWindow(w,GW_CHILD);c;c=GetWindow(c,GW_HWNDNEXT))if(label(c)==name)return c;throw std::runtime_error("Missing About control");}
static HWND startingWith(HWND w,const wchar_t* prefix){for(auto c=GetWindow(w,GW_CHILD);c;c=GetWindow(c,GW_HWNDNEXT))if(label(c).rfind(prefix,0)==0)return c;throw std::runtime_error("Missing About text");}
static BOOL CALLBACK observe(HWND w,LPARAM){if(label(w)!=L"About UDM Download Manager")return TRUE;try{auto once=std::move(action);if(once)once(w);}catch(const std::exception& e){failure=e.what();PostMessageW(w,WM_CLOSE,0,0);}return FALSE;}
static void CALLBACK tick(HWND,UINT,UINT_PTR,DWORD){if(GetTickCount64()-began>15000){failure="About dialog deadline";EnumThreadWindows(GetCurrentThreadId(),[](HWND w,LPARAM)->BOOL{PostMessageW(w,WM_CLOSE,0,0);return TRUE;},0);return;}EnumThreadWindows(GetCurrentThreadId(),observe,0);}
class ToolbarComponentTest {
public:static void run(MainWindow& frame){ frame.ShowWindow(SW_SHOWNOACTIVATE);
 auto open=[&](std::function<void(HWND)> callback){action=std::move(callback);began=GetTickCount64();auto timer=SetTimer(nullptr,0,75,tick);frame.SendMessage(WM_COMMAND,CMD_ABOUT);KillTimer(nullptr,timer);if(!failure.empty())throw std::runtime_error(failure);};
 // IDM window-relative control rectangles at 96 DPI (x,y,w,h in pixels).
 struct Reference{const wchar_t* caption;int x,y,w,h;};
 const Reference references[]={{L"Check for Update",134,94,173,28},{L"This product is licensed to:",15,128,429,98},{L"Home Site:",15,380,142,18},{L"Support Team:",15,402,142,18},{L"Close",182,456,96,28}};
 open([&](HWND w){
  CRect window=bounds(w);const auto measured="About window matches IDM's 461x503 at 96 DPI (measured "+std::to_string(window.Width())+"x"+std::to_string(window.Height())+")";expect(std::abs(window.Width()-461)<=8&&std::abs(window.Height()-503)<=8,measured.c_str());
  expect(label(startingWith(w,L"Version: ")).find(L" (")!=std::wstring::npos,"Version line carries the build date like IDM");
  for(const auto& ref:references){auto c=named(w,ref.caption);expect(shown(c),"About control is visible");auto r=bounds(c);r.OffsetRect(-window.left,-window.top);
   const auto placed=std::string("About control sits where IDM places it: ")+utf8(ref.caption)+" at "+std::to_string(r.left)+","+std::to_string(r.top)+" "+std::to_string(r.Width())+"x"+std::to_string(r.Height())+" vs "+std::to_string(ref.x)+","+std::to_string(ref.y)+" "+std::to_string(ref.w)+"x"+std::to_string(ref.h);expect(std::abs(r.left-ref.x)<=8&&std::abs(r.top-ref.y)<=8&&std::abs(r.Width()-ref.w)<=8&&std::abs(r.Height()-ref.h)<=8,placed.c_str());}
  expect(label(named(w,L"License")).size()&&label(named(w,L"Source code")).size(),"Licensing group offers License and Source code where IDM offers Purchase and Registration");
  expect(label(startingWith(w,L"Copyright")).find(L"UDM contributors")!=std::wstring::npos,"Copyright line names the project");
  for(auto child=GetWindow(w,GW_CHILD);child;child=GetWindow(child,GW_HWNDNEXT)){if(!shown(child))continue;CRect client;GetClientRect(w,&client);auto r=bounds(child);MapWindowPoints(nullptr,w,(POINT*)&r,2);expect(r.left>=0&&r.top>=0&&r.right<=client.right&&r.bottom<=client.bottom,"About controls fit inside the client area");}
  SendMessageW(named(w,L"Close"),BM_CLICK,0,0);
 });
 expect(failure.empty(),"About closes through its Close button");
 }
};class AboutApplication:public CWinApp {
 int code=1;
public:BOOL InitInstance()override{CWinApp::InitInstance();INITCOMMONCONTROLSEX common{sizeof(common),ICC_WIN95_CLASSES|ICC_DATE_CLASSES};InitCommonControlsEx(&common);AfxOleInit();int argc=0;auto argv=CommandLineToArgvW(GetCommandLineW(),&argc);fs::path output;std::unique_ptr<Manager> manager;MainWindow* frame=nullptr;
 try{if(argc!=2)throw std::runtime_error("Pass isolated output directory");output=argv[1];fs::create_directories(output);manager=std::make_unique<Manager>(output/L"state");auto p=manager->state["Settings"];p["CloseToTray"]=false;p["ClipboardMonitor"]=false;manager->setSettings(p);frame=new MainWindow(*manager);m_pMainWnd=frame;ToolbarComponentTest::run(*frame);code=0;}catch(const std::exception& e){failure=e.what();}
 if(frame){frame->SendMessage(WM_CLOSE);m_pMainWnd=nullptr;}manager.reset();if(argv)LocalFree(argv);if(!output.empty())atomicText(output/L"results.json",Json{{"passed",code==0},{"error",failure},{"checks",results}}.dump(2),false);return FALSE;}
 int ExitInstance()override{AfxOleTerm(FALSE);return code;}
};AboutApplication application;
}
