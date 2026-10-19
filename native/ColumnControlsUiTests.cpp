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
static BOOL CALLBACK observe(HWND w,LPARAM){if(label(w)!=L"Columns")return TRUE;try{auto once=std::move(action);if(once)once(w);}catch(const std::exception& e){failure=e.what();PostMessageW(w,WM_CLOSE,0,0);}return FALSE;}
static void CALLBACK tick(HWND,UINT,UINT_PTR,DWORD){if(GetTickCount64()-began>15000){failure="Column dialog deadline";EnumThreadWindows(GetCurrentThreadId(),[](HWND w,LPARAM)->BOOL{PostMessageW(w,WM_CLOSE,0,0);return TRUE;},0);return;}EnumThreadWindows(GetCurrentThreadId(),observe,0);}
class ToolbarComponentTest {
public:static void run(MainWindow& frame){
 auto open=[&](std::function<void(HWND)> callback){action=std::move(callback);began=GetTickCount64();auto timer=SetTimer(nullptr,0,75,tick);frame.customizeColumns();KillTimer(nullptr,timer);if(!failure.empty())throw std::runtime_error(failure);};
 const auto before=frame.listLayout();
 open([&](HWND w){auto list=typed(w,L"SysListView32"),edit=typed(w,L"Edit");
  expect(SendMessageW(w,WM_GETICON,ICON_BIG,0)==0&&SendMessageW(w,WM_GETICON,ICON_SMALL,0)==0,"Columns omits the title icon like the reference");
  expect(label(edit).empty()&&!IsWindowEnabled(edit),"Columns opens without a selected row like the reference");
  expect((GetWindowLongW(list,GWL_STYLE)&LVS_NOCOLUMNHEADER)!=0&&Header_GetItemCount(ListView_GetHeader(list))==1,"Column chooser has one headerless label column");
  auto form=dynamic_cast<Form*>(CWnd::FromHandlePermanent(w));CRect client;GetClientRect(w,&client);
  expect(form&&client.Size()==form->rect(0,0,236,177).Size(),"Columns client matches reference dialog-unit dimensions");
  select(list,0);expect(label(edit)==L"205","File Name selection shows its current width");
  expect(!IsWindowEnabled(named(w,L"Move Up"))&&IsWindowEnabled(named(w,L"Move Down")),"First-column movement respects list boundary");
  expect(!IsWindowEnabled(named(w,L"Show"))&&IsWindowEnabled(named(w,L"Hide")),"File Name can be hidden like the observed reference");
  ListView_SetCheckState(list,0,FALSE);expect(ListView_GetCheckState(list,0)==FALSE,"File Name checkbox accepts hidden state");
  expect(IsWindowEnabled(named(w,L"Show"))&&!IsWindowEnabled(named(w,L"Hide")),"Hidden File Name enables Show");
  select(list,1);expect(label(edit)==L"38","Queue-column selection shows its narrow default width");
  expect(IsWindowEnabled(named(w,L"Hide"))&&!IsWindowEnabled(named(w,L"Show")),"Visible selection enables Hide only");
  SetWindowTextW(edit,L"47");click(w,L"Hide");expect(!ListView_GetCheckState(list,1)&&IsWindowEnabled(named(w,L"Show"))&&!IsWindowEnabled(named(w,L"Hide")),"Hide clears selected checkbox and switches enabled action");
  expect(label(edit)==L"47","Hide preserves unsaved width text");click(w,L"Show");expect(ListView_GetCheckState(list,1)&&IsWindowEnabled(named(w,L"Hide")),"Show restores selected checkbox");
  select(list,2);select(list,1);expect(label(edit)==L"47","Width edit survives selection changes without an apply button");select(list,2);expect(label(edit)==L"72","Changing selection refreshes width");SetWindowTextW(edit,L"91");
  LVITEMW checkbox{};checkbox.stateMask=LVIS_STATEIMAGEMASK;checkbox.state=INDEXTOSTATEIMAGEMASK(1);SendMessageW(list,LVM_SETITEMSTATE,1,(LPARAM)&checkbox);
  expect(label(edit)==L"91","Visibility checkbox changes preserve an uncommitted width edit");
  expect(label(edit)==L"91","Editing a width retains current selection and value");
  click(w,L"Move Down");expect(label(edit)==L"91","Reordering keeps width associated with the same column");
  select(list,-1);expect(!IsWindowEnabled(edit)&&!IsWindowEnabled(named(w,L"Move Up"))&&!IsWindowEnabled(named(w,L"Move Down")),"Cleared selection disables width and movement controls");
  expect(!IsWindowEnabled(named(w,L"Show"))&&!IsWindowEnabled(named(w,L"Hide")),"No selection disables both visibility buttons");
  select(list,11);expect(!IsWindowEnabled(named(w,L"Move Down")),"Last-column selection disables Move Down");
  click(w,L"Cancel");
 });
 expect(frame.listLayout()==before,"Cancel discards column width and order edits");
 open([&](HWND w){select(typed(w,L"SysListView32"),0);click(w,L"Hide");click(w,L"OK");});
 expect(frame.table.GetColumnWidth(0)==0,"File Name Hide applies to the main list");
 frame.table.SetColumnWidth(0,frame.px(205));frame.restoreLayout();
 expect(frame.table.GetColumnWidth(0)==0,"Restoring saved layout preserves hidden File Name");
 open([&](HWND w){auto list=typed(w,L"SysListView32");select(list,0);expect(!ListView_GetCheckState(list,0)&&label(typed(w,L"Edit"))==L"205","Reopening retains hidden File Name and its restore width");click(w,L"Show");click(w,L"Cancel");});
 expect(frame.table.GetColumnWidth(0)==0,"Cancel discards File Name Show");
 open([&](HWND w){select(typed(w,L"SysListView32"),0);click(w,L"Show");click(w,L"OK");});
 expect(frame.table.GetColumnWidth(0)==frame.px(205),"Show restores File Name to its remembered width");
 open([&](HWND w){auto list=typed(w,L"SysListView32"),edit=typed(w,L"Edit");select(list,1);SetWindowTextW(edit,L"25");click(w,L"OK");});
 expect(frame.table.GetColumnWidth(1)==frame.px(25),"Applying a narrow queue width updates the main list");
 open([&](HWND w){auto list=typed(w,L"SysListView32"),edit=typed(w,L"Edit");select(list,1);expect(label(edit)==L"25","Reopened dialog reads saved narrow width");click(w,L"Reset");expect(label(edit)==L"205","Reset updates selection and displayed default width");click(w,L"Cancel");});
 expect(frame.table.GetColumnWidth(1)==frame.px(25),"Cancel after Reset preserves the applied layout");
 open([&](HWND w){auto list=typed(w,L"SysListView32");select(list,1);click(w,L"Hide");click(w,L"OK");});
 expect(frame.table.GetColumnWidth(1)==0,"Hide applies zero width to the chosen main-list column");
 open([&](HWND w){auto list=typed(w,L"SysListView32");select(list,1);expect(label(typed(w,L"Edit"))==L"25","Hidden column retains its previous width on reopen");click(w,L"Show");click(w,L"OK");});
 expect(frame.table.GetColumnWidth(1)==frame.px(25),"Show restores the saved width after reopening");
 open([&](HWND w){
  for(UINT dpi:{96u,144u,192u}){
   auto suggested=bounds(w);SendMessageW(w,WM_DPICHANGED,MAKEWPARAM(dpi,dpi),(LPARAM)&suggested);
   expect((GetWindowLongW(typed(w,L"SysListView32"),GWL_STYLE)&WS_HSCROLL)==0,"Column chooser has no horizontal scrollbar at tested DPI");
   CRect client;GetClientRect(w,&client);std::vector<CRect> buttons;
   for(auto child=GetWindow(w,GW_CHILD);child;child=GetWindow(child,GW_HWNDNEXT)){
    if(!shown(child))continue;auto rect=bounds(child);MapWindowPoints(nullptr,w,(POINT*)&rect,2);
    expect(rect.left>=0&&rect.top>=0&&rect.right<=client.right&&rect.bottom<=client.bottom,"Column controls stay within dialog client bounds at tested DPI");
    wchar_t type[64]{};GetClassNameW(child,type,64);if(!_wcsicmp(type,L"Button"))buttons.push_back(rect);
   }
   for(size_t i=0;i<buttons.size();++i)for(size_t j=i+1;j<buttons.size();++j){CRect overlap;expect(!overlap.IntersectRect(buttons[i],buttons[j]),"Column action buttons do not overlap at tested DPI");}
   captureComponent(w,renderRoot/wide("columns-"+std::to_string(dpi)+".png"));
  }
  click(w,L"Cancel");
 });
 }
};
class ColumnControlsApplication:public CWinApp {
 int code=1;
public:BOOL InitInstance()override{CWinApp::InitInstance();INITCOMMONCONTROLSEX common{sizeof(common),ICC_WIN95_CLASSES|ICC_DATE_CLASSES};InitCommonControlsEx(&common);AfxOleInit();int argc=0;auto argv=CommandLineToArgvW(GetCommandLineW(),&argc);fs::path output;std::unique_ptr<Manager> manager;MainWindow* frame=nullptr;
 try{if(argc!=2)throw std::runtime_error("Pass isolated output directory");output=argv[1];renderRoot=output;fs::create_directories(output);manager=std::make_unique<Manager>(output/L"state");auto p=manager->state["Settings"];p["CloseToTray"]=false;p["ClipboardMonitor"]=false;manager->setSettings(p);frame=new MainWindow(*manager);m_pMainWnd=frame;ToolbarComponentTest::run(*frame);code=0;}catch(const std::exception& e){failure=e.what();}
 if(frame){frame->SendMessage(WM_CLOSE);m_pMainWnd=nullptr;}manager.reset();if(argv)LocalFree(argv);if(!output.empty())atomicText(output/L"results.json",Json{{"passed",code==0},{"error",failure},{"checks",results}}.dump(2),false);return FALSE;}
 int ExitInstance()override{AfxOleTerm(FALSE);return code;}
};ColumnControlsApplication application;
}
