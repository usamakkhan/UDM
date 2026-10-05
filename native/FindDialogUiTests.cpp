// Exercises only the column dialog belonging to this isolated fixture.
#define UDM_TOOLBAR_COMPONENT_TEST
#include "App.cpp"
namespace udm {
static fs::path renderRoot;
static CRect bounds(HWND window){CRect r;::GetWindowRect(window,&r);return r;}
static bool shown(HWND window){return (::GetWindowLongW(window,GWL_STYLE)&WS_VISIBLE)!=0;}
static void captureComponent(HWND window,const fs::path& file){
 ::ShowWindow(window,SW_SHOWNOACTIVATE);::RedrawWindow(window,nullptr,nullptr,RDW_INVALIDATE|RDW_ALLCHILDREN|RDW_UPDATENOW|RDW_ERASE);
 auto r=bounds(window);CImage image;image.Create(r.Width(),r.Height(),32);auto dc=image.GetDC();
 RECT all{0,0,r.Width(),r.Height()};FillRect(dc,&all,GetSysColorBrush(COLOR_BTNFACE));
 SendMessageW(window,WM_PRINT,(WPARAM)dc,PRF_NONCLIENT|PRF_CLIENT|PRF_ERASEBKGND|PRF_CHILDREN);
 for(auto child=::GetWindow(window,GW_CHILD);child;child=::GetWindow(child,GW_HWNDNEXT)){
  if(!shown(child))continue;auto c=bounds(child);auto save=SaveDC(dc);
  IntersectClipRect(dc,c.left-r.left,c.top-r.top,c.right-r.left,c.bottom-r.top);
  SetViewportOrgEx(dc,c.left-r.left,c.top-r.top,nullptr);
  SendMessageW(child,WM_PRINT,(WPARAM)dc,PRF_NONCLIENT|PRF_CLIENT|PRF_ERASEBKGND|PRF_CHILDREN);RestoreDC(dc,save);
 }
 image.ReleaseDC();if(FAILED(image.Save(file.c_str())))throw std::runtime_error("Could not render own test window");
}
static Json results=Json::array();static std::string failure;static std::function<void(HWND)> action;static ULONGLONG began;
static void expect(bool value,const char* name){results.push_back({{"name",name},{"passed",value}});if(!value)throw std::runtime_error(name);}
static std::wstring label(HWND w){wchar_t text[256]{};GetWindowTextW(w,text,256);return text;}
static HWND named(HWND w,const wchar_t* name){for(auto c=GetWindow(w,GW_CHILD);c;c=GetWindow(c,GW_HWNDNEXT))if(label(c)==name)return c;throw std::runtime_error("Missing fixture button");}
static HWND typed(HWND w,const wchar_t* type){for(auto c=GetWindow(w,GW_CHILD);c;c=GetWindow(c,GW_HWNDNEXT)){wchar_t name[128]{};GetClassNameW(c,name,128);if(!_wcsicmp(name,type))return c;}throw std::runtime_error("Missing fixture control");}
static void click(HWND w,const wchar_t* name){auto c=named(w,name);expect(IsWindowEnabled(c)!=FALSE,"Requested column-dialog action is enabled");SendMessageW(c,BM_CLICK,0,0);}
static void select(HWND list,int row){LVITEMW item{};item.stateMask=LVIS_SELECTED|LVIS_FOCUSED;SendMessageW(list,LVM_SETITEMSTATE,-1,(LPARAM)&item);if(row>=0){item.state=LVIS_SELECTED|LVIS_FOCUSED;SendMessageW(list,LVM_SETITEMSTATE,row,(LPARAM)&item);}}
static BOOL CALLBACK observe(HWND w,LPARAM){if(label(w)!=L"Find file")return TRUE;try{auto once=std::move(action);if(once)once(w);}catch(const std::exception& e){failure=e.what();PostMessageW(w,WM_CLOSE,0,0);}return FALSE;}
static void CALLBACK tick(HWND,UINT,UINT_PTR,DWORD){if(GetTickCount64()-began>15000){failure="Find dialog deadline";EnumThreadWindows(GetCurrentThreadId(),[](HWND w,LPARAM)->BOOL{PostMessageW(w,WM_CLOSE,0,0);return TRUE;},0);return;}EnumThreadWindows(GetCurrentThreadId(),observe,0);}
class ToolbarComponentTest {
public:static void run(MainWindow& frame){ frame.ShowWindow(SW_SHOWNOACTIVATE);
  auto key=[&](UINT code,bool control=false,bool shift=false){
  BYTE saved[256]{},state[256]{};if(!GetKeyboardState(saved))throw std::runtime_error("Read fixture keyboard state");
  state[VK_CONTROL]=control?0x80:0;state[VK_SHIFT]=shift?0x80:0;
  if(!SetKeyboardState(state))throw std::runtime_error("Set fixture keyboard state");
  MSG message{};message.hwnd=frame.table.GetSafeHwnd();message.message=WM_KEYDOWN;message.wParam=code;
  BOOL handled=frame.PreTranslateMessage(&message);SetKeyboardState(saved);return handled!=FALSE;
 };
 auto open=[&](std::function<void(HWND)> callback){action=std::move(callback);began=GetTickCount64();auto timer=SetTimer(nullptr,0,75,tick);bool handled=key('F',true);KillTimer(nullptr,timer);if(!failure.empty())throw std::runtime_error(failure);expect(handled,"Ctrl+F dispatch opens the actual Find dialog");};
 auto checked=[](HWND w,const wchar_t* name){return SendMessageW(named(w,name),BM_GETCHECK,0,0)==BST_CHECKED;};
 auto a=frame.manager.add("https://fixture.example/one","","alpha-match.bin");
 auto b=frame.manager.add("https://fixture.example/two","","beta-other.bin");
 auto c=frame.manager.add("https://fixture.example/three","","gamma-match.bin");
 a->data["Category"]="Documents";b->data["Category"]="Other";c->data["Category"]="Other";
 b->data["Description"]="unique description";
 frame.sortColumn=0;frame.ascending=true;frame.refresh();select(frame.table.GetSafeHwnd(),-1);
 auto selected=[&](JobPtr job){auto chosen=frame.selected();return chosen.size()==1&&chosen.front()==job;};
 open([&](HWND w){
  expect(::GetFocus()==typed(w,L"Edit"),"Find initially focuses its text field");expect(checked(w,L"File name or part of the name"),"Find defaults to file-name matching");
  expect(!checked(w,L"Description or part of the description")&&!checked(w,L"Site name/Download link/Parent web page/Referer"),"Other Find fields are initially unchecked");
  expect(!checked(w,L"Match case")&&!checked(w,L"Match whole string only"),"Find defaults to case-insensitive substring search");
  SetWindowTextW(typed(w,L"Edit"),L"match");click(w,L"Find");
 });
 expect(selected(a),"Find selects first matching row when no row is selected");
 expect(key(VK_F3),"F3 is handled by the main-window keyboard dispatcher");expect(selected(c),"Find Next skips intervening nonmatching rows");
 expect(key(VK_F3),"Repeated F3 is handled");expect(selected(a),"Find Next wraps from last matching row to first");
 auto query=frame.findQuery;
 open([&](HWND w){expect(label(typed(w,L"Edit"))==L"match","Find reopens with the prior search text");SetWindowTextW(typed(w,L"Edit"),L"discarded");click(w,L"Match case");click(w,L"Cancel");});
 expect(frame.findQuery==query&&selected(a),"Cancel preserves the previous query and selected result");
 open([&](HWND w){SetWindowTextW(typed(w,L"Edit"),L"unique description");click(w,L"File name or part of the name");click(w,L"Description or part of the description");click(w,L"Match whole string only");click(w,L"Find");});
 expect(selected(b),"Description-only whole-string search selects the matching record");
 open([&](HWND w){expect(checked(w,L"Description or part of the description")&&checked(w,L"Match whole string only")&&!checked(w,L"File name or part of the name"),"Find retains chosen fields and whole-string option");
  for(UINT dpi:{96u,144u,192u}){
   auto suggested=bounds(w);SendMessageW(w,WM_DPICHANGED,MAKEWPARAM(dpi,dpi),(LPARAM)&suggested);CRect client;GetClientRect(w,&client);
   for(auto child=GetWindow(w,GW_CHILD);child;child=GetWindow(child,GW_HWNDNEXT)){if(!shown(child))continue;auto rect=bounds(child);MapWindowPoints(nullptr,w,(POINT*)&rect,2);expect(rect.left>=0&&rect.top>=0&&rect.right<=client.right&&rect.bottom<=client.bottom,"Find controls fit inside client area at tested DPI");}
   captureComponent(w,renderRoot/wide("find-"+std::to_string(dpi)+".png"));
  }click(w,L"Cancel");
 });
 frame.findQuery={{"Text","match"},{"FileName",true}};frame.filter="category:Other";frame.refresh();select(frame.table.GetSafeHwnd(),-1);
 frame.findNext();expect(selected(c)&&frame.visible.size()==2,"Find Next respects the current category");frame.findNext();expect(selected(c),"Single visible match wraps to itself without leaving the category");
 frame.filter="all";expect(key('F',true,true),"Ctrl+Shift+F is handled separately from Find");expect(::GetFocus()==frame.search.GetSafeHwnd(),"Ctrl+Shift+F focuses the quick filter");frame.search.SetWindowTextW(L"gamma");frame.refresh();frame.findNext();expect(selected(c)&&frame.visible.size()==1,"Find Next respects the list text filter");
 }
};class FindDialogApplication:public CWinApp {
 int code=1;
public:BOOL InitInstance()override{CWinApp::InitInstance();INITCOMMONCONTROLSEX common{sizeof(common),ICC_WIN95_CLASSES|ICC_DATE_CLASSES};InitCommonControlsEx(&common);AfxOleInit();int argc=0;auto argv=CommandLineToArgvW(GetCommandLineW(),&argc);fs::path output;std::unique_ptr<Manager> manager;MainWindow* frame=nullptr;
 try{if(argc!=2)throw std::runtime_error("Pass isolated output directory");output=argv[1];renderRoot=output;fs::create_directories(output);manager=std::make_unique<Manager>(output/L"state");auto p=manager->state["Settings"];p["CloseToTray"]=false;p["ClipboardMonitor"]=false;manager->setSettings(p);frame=new MainWindow(*manager);m_pMainWnd=frame;ToolbarComponentTest::run(*frame);code=0;}catch(const std::exception& e){failure=e.what();}
 if(frame){frame->SendMessage(WM_CLOSE);m_pMainWnd=nullptr;}manager.reset();if(argv)LocalFree(argv);if(!output.empty())atomicText(output/L"results.json",Json{{"passed",code==0},{"error",failure},{"checks",results}}.dump(2),false);return FALSE;}
 int ExitInstance()override{AfxOleTerm(FALSE);return code;}
};FindDialogApplication application;
}
