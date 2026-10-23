// Exercises only the Site Grabber wizard of this isolated fixture against IDM
// 6.43's capture (docs/gui-parity-2026-10-18/idm-grabber-1.uia.txt) and its
// dialog templates 239 (wizard shell) and 241 (exploration result).
#define UDM_TOOLBAR_COMPONENT_TEST
#include "App.cpp"
namespace udm {
static CRect bounds(HWND window){CRect r;::GetWindowRect(window,&r);return r;}
static bool shown(HWND window){return (::GetWindowLongW(window,GWL_STYLE)&WS_VISIBLE)!=0;}
static Json results=Json::array();static std::string failure;static std::function<void(HWND)> action;static ULONGLONG began;
static void expect(bool value,const std::string& name){results.push_back({{"name",name},{"passed",value}});if(!value)throw std::runtime_error(name);}
static std::wstring label(HWND w){wchar_t text[512]{};GetWindowTextW(w,text,512);std::wstring value=text;value.erase(std::remove(value.begin(),value.end(),L'&'),value.end());return value;}
static HWND named(HWND w,const wchar_t* name,bool visible=true){for(auto c=GetWindow(w,GW_CHILD);c;c=GetWindow(c,GW_HWNDNEXT))if(label(c)==name&&(!visible||shown(c)))return c;throw std::runtime_error("Missing wizard control: "+utf8(name));}
static HWND editAfter(HWND w,const wchar_t* caption){auto c=GetWindow(named(w,caption),GW_HWNDNEXT);wchar_t cls[64]{};GetClassNameW(c,cls,64);if(_wcsicmp(cls,L"Edit"))throw std::runtime_error("No edit follows "+utf8(caption));return c;}
static void placed(HWND w,HWND c,const char* what,int x,int y,int width,int height){CRect window=bounds(w);auto r=bounds(c);r.OffsetRect(-window.left,-window.top);
 expect(std::abs(r.left-x)<=8&&std::abs(r.top-y)<=8&&std::abs(r.Width()-width)<=8&&std::abs(r.Height()-height)<=8,std::string("Wizard control sits where IDM places it: ")+what+" at "+std::to_string(r.left)+","+std::to_string(r.top)+" "+std::to_string(r.Width())+"x"+std::to_string(r.Height())+" vs "+std::to_string(x)+","+std::to_string(y)+" "+std::to_string(width)+"x"+std::to_string(height));}
static void sized(HWND w,const char* what,int width,int height){CRect window=bounds(w);expect(std::abs(window.Width()-width)<=8&&std::abs(window.Height()-height)<=8,std::string(what)+" window matches IDM's "+std::to_string(width)+"x"+std::to_string(height)+" at 96 DPI (measured "+std::to_string(window.Width())+"x"+std::to_string(window.Height())+")");}
static void inside(HWND w,const char* what){CRect client;GetClientRect(w,&client);for(auto child=GetWindow(w,GW_CHILD);child;child=GetWindow(child,GW_HWNDNEXT)){if(!shown(child))continue;auto r=bounds(child);MapWindowPoints(nullptr,w,(POINT*)&r,2);expect(r.left>=0&&r.top>=0&&r.right<=client.right&&r.bottom<=client.bottom,std::string(what)+" controls fit inside the client area ("+utf8(label(child))+")");}}
static std::string popup;static HHOOK cbt;
// Any message box the wizard raises is recorded and dismissed so the harness names it instead of waiting on it.
static LRESULT CALLBACK onCbt(int code,WPARAM w,LPARAM l){if(code==HCBT_ACTIVATE){HWND h=(HWND)w;wchar_t cls[64]{};GetClassNameW(h,cls,64);if(!wcscmp(cls,L"#32770")&&label(h)==L"UDM"){std::wstring text;for(auto c=GetWindow(h,GW_CHILD);c;c=GetWindow(c,GW_HWNDNEXT)){wchar_t k[64]{};GetClassNameW(c,k,64);if(!wcscmp(k,L"Static")&&!label(c).empty())text+=label(c)+L" ";}popup+=utf8(text)+"; ";PostMessageW(h,WM_CLOSE,0,0);}}return CallNextHookEx(cbt,code,w,l);}
static BOOL CALLBACK observe(HWND w,LPARAM){if(label(w)!=L"UDM Site Grabber. Step 1 --- Set start page")return TRUE;try{auto once=std::move(action);if(once)once(w);}catch(const std::exception& e){failure=e.what();PostMessageW(w,WM_CLOSE,0,0);}return FALSE;}
static void CALLBACK tick(HWND,UINT,UINT_PTR,DWORD){if(GetTickCount64()-began>20000){failure="Site Grabber deadline";EnumThreadWindows(GetCurrentThreadId(),[](HWND w,LPARAM)->BOOL{PostMessageW(w,WM_CLOSE,0,0);return TRUE;},0);return;}EnumThreadWindows(GetCurrentThreadId(),observe,0);}
class WizardTest {
public:static void run(Manager& manager){
 action=[&](HWND w){
  // Step 1: shell and controls from the capture (window 717x475 with the menu bar; client origin 8,59).
  sized(w,"Step 1",717,475);auto menu=::GetMenu(w);expect(menu!=nullptr&&GetMenuItemCount(menu)==4,"Wizard carries a four-item menu bar like IDM");
  const wchar_t* menus[]={L"Project",L"Options",L"Actions",L"Help"};for(int i=0;i<4;++i){wchar_t text[64]{};GetMenuStringW(menu,i,text,64,MF_BYPOSITION);std::wstring item=text;item.erase(std::remove(item.begin(),item.end(),L'&'),item.end());expect(item==menus[i],"Menu bar item "+std::to_string(i)+" is "+utf8(menus[i]));}
  placed(w,named(w,L"< Back"),"< Back",274,437,105,28);placed(w,named(w,L"Next >"),"Next >",390,437,105,28);placed(w,named(w,L"Close"),"Close",570,437,105,28);
  placed(w,named(w,L"Grabber Project Name"),"Grabber Project Name",189,61,287,18);placed(w,editAfter(w,L"Grabber Project Name"),"project name edit",189,81,291,28);
  placed(w,named(w,L"Start page/address"),"Start page/address",189,115,502,18);placed(w,editAfter(w,L"Start page/address"),"start page edit",189,135,506,28);
  placed(w,named(w,L"Use authorization"),"Use authorization",189,287,282,20);placed(w,named(w,L"Advanced >>"),"Advanced >>",537,363,128,28);
  expect(!IsWindowEnabled(named(w,L"< Back")),"Back is disabled on step 1 like IDM");inside(w,"Step 1");
  // Advanced >> opens IDM's manual-login area and grows the window to the 303 DU template.
  SendMessageW(named(w,L"Advanced >>"),BM_CLICK,0,0);sized(w,"Step 1 (advanced)",717,673);
  placed(w,named(w,L"Enter login and password manually at the following web page:"),"manual login check",187,425,506,20);placed(w,named(w,L"Don't open the logout page:"),"logout check",189,497,504,20);inside(w,"Step 1 (advanced)");
  SendMessageW(named(w,L"Advanced >>"),BM_CLICK,0,0);sized(w,"Step 1 (compact again)",717,475);expect(!shown(named(w,L"Don't open the logout page:",false)),"Advanced << hides the manual-login area again");
  // Walk the steps with IDM's names.
  SetWindowTextW(editAfter(w,L"Grabber Project Name"),L"wizard_1");SetWindowTextW(editAfter(w,L"Start page/address"),L"http://127.0.0.1:9/");
  const wchar_t* titles[]={L"UDM Site Grabber. Step 2 --- Save files to",L"UDM Site Grabber. Step 3 --- Set explorer filters",L"UDM Site Grabber. Step 4 --- Set file filters",L"UDM Site Grabber. Step 5 --- Explore and download"};
  for(int i=0;i<4;++i){SendMessageW(named(w,L"Next >"),BM_CLICK,0,0);expect(popup.empty(),"Next raised no error ("+popup+")");expect(label(w)==titles[i],"Next reaches "+utf8(titles[i])+" (title now: "+utf8(label(w))+")");expect(IsWindowEnabled(named(w,L"< Back"))!=FALSE,"Back is enabled after step 1");if(i<3){sized(w,"Wizard step",717,475);inside(w,"Wizard step");}}
  // Step 5: IDM dialog 241 (508x286 DU) with the result tree and list; Next gives way to Add selected.
  sized(w,"Step 5",905,639);placed(w,named(w,L"Close"),"Close (result)",780,589,105,28);placed(w,named(w,L"< Back"),"< Back (result)",445,589,105,28);placed(w,named(w,L"Add selected"),"Add selected",561,589,105,28);
  expect(!shown(named(w,L"Next >",false)),"Next is hidden on the result step");
  bool tree=false,list=false;for(auto c=GetWindow(w,GW_CHILD);c;c=GetWindow(c,GW_HWNDNEXT)){wchar_t cls[64]{};GetClassNameW(c,cls,64);if(!shown(c))continue;if(!_wcsicmp(cls,L"SysTreeView32")){tree=true;placed(w,c,"result tree",20,73,177,488);}if(!_wcsicmp(cls,L"SysListView32")){list=true;placed(w,c,"result list",209,73,676,488);}}
  expect(tree&&list,"Result step shows IDM's tree and file list");inside(w,"Step 5");
  for(UINT id:{40010u,40013u,40022u}){expect((GetMenuState(menu,id,MF_BYCOMMAND)&MF_GRAYED)==0,"Actions menu entry "+std::to_string(id)+" is enabled on the result step");}
  SendMessageW(named(w,L"< Back"),BM_CLICK,0,0);expect(label(w)==L"UDM Site Grabber. Step 4 --- Set file filters","Back returns to step 4");sized(w,"Step 4 again",717,475);
  SendMessageW(named(w,L"Close"),BM_CLICK,0,0);
 };
 began=GetTickCount64();auto timer=SetTimer(nullptr,0,75,tick);cbt=SetWindowsHookExW(WH_CBT,onCbt,nullptr,GetCurrentThreadId());GrabberDialog grabber(manager,nullptr);grabber.DoModal();UnhookWindowsHookEx(cbt);KillTimer(nullptr,timer);if(!failure.empty())throw std::runtime_error(failure);
 expect(true,"Site Grabber closes through its Close button");
 }
};class WizardApplication:public CWinApp {
 int code=1;
public:BOOL InitInstance()override{CWinApp::InitInstance();INITCOMMONCONTROLSEX common{sizeof(common),ICC_WIN95_CLASSES|ICC_DATE_CLASSES};InitCommonControlsEx(&common);AfxOleInit();int argc=0;auto argv=CommandLineToArgvW(GetCommandLineW(),&argc);fs::path output;std::unique_ptr<Manager> manager;
 try{if(argc!=2)throw std::runtime_error("Pass isolated output directory");output=argv[1];fs::create_directories(output);manager=std::make_unique<Manager>(output/L"state");auto p=manager->state["Settings"];p["CloseToTray"]=false;p["ClipboardMonitor"]=false;manager->setSettings(p);WizardTest::run(*manager);code=0;}catch(const std::exception& e){failure=e.what();}
 manager.reset();if(argv)LocalFree(argv);if(!output.empty())atomicText(output/L"results.json",Json{{"passed",code==0},{"error",failure},{"checks",results}}.dump(2),false);return FALSE;}
 int ExitInstance()override{AfxOleTerm(FALSE);return code;}
};WizardApplication application;
}
