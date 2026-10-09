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

class ToolbarComponentTest {
public:static void appearanceTransitions(){
 Manager manager(output/L"appearance-state");auto prefs=manager.state["Settings"];prefs["DarkMode"]=false;prefs["CloseToTray"]=false;prefs["ClipboardMonitor"]=false;manager.setSettings(prefs);
 auto frame=new MainWindow(manager);AfxGetApp()->m_pMainWnd=frame;
 try{
  auto job=manager.add("https://example.invalid/theme-fixture.bin","","theme-fixture.bin","Main queue",true);job->data["Size"]=1000;job->data["Received"]=375;
  Progress progress(manager,job,frame);expect(progress.Create(101,frame)!=FALSE,"Create a real progress dialog before appearance changes");progress.ShowWindow(SW_SHOWNOACTIVATE);
  Form form("Open dialog appearance",360,210,frame);form.modeless=true;ThemeList* list=nullptr;CWnd *value=nullptr,*primary=nullptr,*disabled=nullptr;int clicks=0;
  form.init=[&]{form.control(L"BUTTON","Download details",BS_GROUPBOX,7,7,345,195);value=form.edit("Keep this draft",17,29,320);list=form.make<ThemeList>(WS_TABSTOP|WS_BORDER|LVS_REPORT|LVS_SINGLESEL,17,64,320,85);list->InsertColumn(0,L"File",LVCFMT_LEFT,220);list->InsertItem(0,L"selected.zip");list->SetItemState(0,LVIS_SELECTED|LVIS_FOCUSED,LVIS_SELECTED|LVIS_FOCUSED);list->theme();primary=form.button("Apply",150,164,85,[&]{++clicks;});form.defaultButton(primary);disabled=form.button("Unavailable",245,164,85,[]{});disabled->EnableWindow(FALSE);};
  expect(form.Create(100,frame)!=FALSE,"Create a live dialog before appearance changes");form.ShowWindow(SW_SHOWNOACTIVATE);value->SetFocus();const auto window=form.GetSafeHwnd();
  for(bool dark:{true,false,true,false}){
   const auto focusBefore=::GetFocus();const auto selectionBefore=list->GetItemState(0,LVIS_SELECTED);prefs["DarkMode"]=dark;manager.setSettings(prefs);frame->applyAppearance();
   const bool effectiveDark=useDarkUi(dark,windowsHighContrast());expect(uiDark==effectiveDark,"Main appearance uses the requested light or dark setting");
   expect((primary->GetStyle()&BS_TYPEMASK)==(DWORD)(effectiveDark?BS_OWNERDRAW:BS_DEFPUSHBUTTON),"Already-open primary button updates its theme style");
   expect(list->GetBkColor()==uiBackground()&&list->GetTextBkColor()==uiBackground()&&list->GetTextColor()==uiForeground(),"Already-open list adopts the current foreground and background");
   expect(form.GetSafeHwnd()==window&&text(value)=="Keep this draft","Appearance updates preserve the dialog and unsaved text");
   atomicText(output/L"focus-diagnostic.json",Json{{"before",(uint64_t)(uintptr_t)focusBefore},{"after",(uint64_t)(uintptr_t)::GetFocus()},{"edit",(uint64_t)(uintptr_t)value->GetSafeHwnd()},{"selectionBefore",selectionBefore},{"selectionAfter",list->GetItemState(0,LVIS_SELECTED)}}.dump(2),false);
   expect(focusBefore&&::GetFocus()==focusBefore&&selectionBefore==LVIS_SELECTED&&list->GetItemState(0,LVIS_SELECTED)==selectionBefore,"Appearance updates preserve the established GUI focus and list selection");
   expect(!disabled->IsWindowEnabled(),"Appearance updates do not enable unavailable actions");
   const auto previousClicks=clicks;primary->SendMessage(BM_CLICK);expect(clicks==previousClicks+1,"The existing action remains callable after appearance refresh");
   const auto start=child(progress.GetSafeHwnd(),L"Start");expect((GetWindowLongW(start,GWL_STYLE)&BS_TYPEMASK)==(effectiveDark?BS_OWNERDRAW:BS_PUSHBUTTON),"Open progress action updates its theme style");
   expect(str(job->data,"Status")=="Paused"&&num(job->data,"Received")==375&&!manager.isActive(job),"Appearance changes do not start, stop or alter a download");
   if(effectiveDark){
    for(const auto kind:{L"SysTabControl32",L"SysHeader32"}){
     HWND painted=nullptr;if(!_wcsicmp(kind,L"SysTabControl32")){auto candidates=children(progress.GetSafeHwnd(),kind);if(!candidates.empty())painted=candidates.front();}else{auto candidates=children(progress.GetSafeHwnd(),L"SysListView32");if(!candidates.empty())painted=(HWND)SendMessageW(candidates.front(),LVM_GETHEADER,0,0);}
     expect(painted!=nullptr,"Progress themed control exists for print rendering");RECT area{};GetClientRect(painted,&area);CImage pixels;pixels.Create(area.right,area.bottom,32);auto dc=pixels.GetDC();FillRect(dc,&area,GetSysColorBrush(COLOR_WINDOW));SendMessageW(painted,WM_PRINTCLIENT,(WPARAM)dc,PRF_CLIENT);auto color=GetPixel(dc,area.right-4,area.bottom/2);pixels.ReleaseDC();
     expect(color!=CLR_INVALID&&GetRValue(color)<100&&GetGValue(color)<100&&GetBValue(color)<100,"Dark progress tabs and headers retain dark surfaces when printed");
    }
   }
   capture(progress.GetSafeHwnd(),dark?L"progress-dark.png":L"progress-light.png");
   capture(window,dark?L"open-dialog-dark.png":L"open-dialog-light.png");
  }
  expect(!useDarkUi(true,true)&&!useDarkUi(false,true),"Windows contrast overrides either stored dark-mode preference");
  expect(useDarkUi(true,false)&&!useDarkUi(false,false),"Leaving contrast mode restores the stored appearance preference");
  expect(!uiDark&&dialogBrush().GetSafeHandle()==GetSysColorBrush(COLOR_BTNFACE)&&uiBrush().GetSafeHandle()==GetSysColorBrush(COLOR_WINDOW),"Light controls use system-owned brushes that track palette changes");
  const auto saved=manager.state["Settings"];frame->SendMessage(WM_SYSCOLORCHANGE);expect(manager.state["Settings"]==saved&&text(value)=="Keep this draft","System color notifications refresh without changing preferences or drafts");
  frame->SendMessage(WM_SETTINGCHANGE,SPI_SETHIGHCONTRAST,0);expect(manager.state["Settings"]==saved&&list->GetBkColor()==uiBackground(),"Contrast-setting notification refreshes current controls without changing Windows settings");
  form.DestroyWindow();progress.DestroyWindow();
 }catch(...){frame->SendMessage(WM_CLOSE);AfxGetApp()->m_pMainWnd=nullptr;throw;}
 frame->SendMessage(WM_CLOSE);AfxGetApp()->m_pMainWnd=nullptr;
 }
};

static void schedulerActions(bool initialDark){
 Manager manager(output/(initialDark?L"scheduler-dark-state":L"scheduler-light-state"));auto first=manager.add("https://example.invalid/first.bin","","first.bin","Main queue",true);auto second=manager.add("https://example.invalid/second.bin","","second.bin","Main queue",true);
 uiDark=initialDark;Scheduler dialog(manager,nullptr);bool observed=false;
 modal(dialog,L"Scheduler",[&](HWND w){
  observed=true;auto tab=children(w,L"SysTabControl32").at(0);SendMessageW(tab,TCM_SETCURSEL,1,0);NMHDR notify{tab,(UINT_PTR)GetDlgCtrlID(tab),TCN_SELCHANGE};SendMessageW(w,WM_NOTIFY,notify.idFrom,(LPARAM)&notify);
  auto list=static_cast<CListCtrl*>(CWnd::FromHandlePermanent(children(w,L"SysListView32").at(0)));expect(list&&list->GetItemCount()==2,"Scheduler exposes both isolated queue entries");
  auto tree=static_cast<CTreeCtrl*>(CWnd::FromHandlePermanent(children(w,L"SysTreeView32").at(0)));const auto queue=tree->GetSelectedItem();
  auto verifySurfaces=[&]{
   expect(tree->GetBkColor()==uiBackground()&&tree->GetTextColor()==uiForeground(),"Scheduler queue tree follows the current theme");expect(list->GetBkColor()==uiBackground()&&list->GetTextColor()==uiForeground(),"Scheduler file list follows the current theme");
   if(uiDark)for(auto painted:{tab,list->GetHeaderCtrl()->GetSafeHwnd()}){RECT area{};GetClientRect(painted,&area);CImage pixels;pixels.Create(area.right,area.bottom,32);auto dc=pixels.GetDC();FillRect(dc,&area,GetSysColorBrush(COLOR_WINDOW));SendMessageW(painted,WM_PRINTCLIENT,(WPARAM)dc,PRF_CLIENT);auto color=GetPixel(dc,area.right-4,area.bottom/2);pixels.ReleaseDC();expect(color!=CLR_INVALID&&GetRValue(color)<100&&GetGValue(color)<100&&GetBValue(color)<100,"Scheduler tabs and file headers render dark surfaces");}
   expect(tree->GetSelectedItem()==queue&&SendMessageW(tab,TCM_GETCURSEL,0,0)==1,"Scheduler theme refresh preserves selected queue and tab");
  };verifySurfaces();
  BYTE savedKeys[256]{},cycleKeys[256]{};expect(GetKeyboardState(savedKeys)!=FALSE,"Read scheduler fixture keyboard state");memcpy(cycleKeys,savedKeys,sizeof(cycleKeys));cycleKeys[VK_CONTROL]=0x80;cycleKeys[VK_SHIFT]=0;cycleKeys[VK_MENU]=0;SetFocus(tab);MSG key{};key.hwnd=tab;key.message=WM_KEYDOWN;key.wParam=VK_TAB;key.lParam=1;expect(SetKeyboardState(cycleKeys)!=FALSE,"Set scheduler fixture modifiers");auto handled=static_cast<CWnd*>(&dialog)->PreTranslateMessage(&key);SetKeyboardState(savedKeys);expect(handled&&SendMessageW(tab,TCM_GETCURSEL,0,0)==2,"Scheduler Ctrl+Tab opens Automation");cycleKeys[VK_SHIFT]=0x80;expect(SetKeyboardState(cycleKeys)!=FALSE,"Set scheduler reverse modifiers");handled=static_cast<CWnd*>(&dialog)->PreTranslateMessage(&key);SetKeyboardState(savedKeys);expect(handled&&SendMessageW(tab,TCM_GETCURSEL,0,0)==1,"Scheduler Ctrl+Shift+Tab returns to Files in the queue");
  const auto up=child(w,L"Move up"),down=child(w,L"Move down"),remove=child(w,L"Remove from queue");
  expect(up&&down&&remove,"Compact scheduler actions retain readable accessible names");
  list->SetItemState(-1,0,LVIS_SELECTED);list->SetItemState(1,LVIS_SELECTED|LVIS_FOCUSED,LVIS_SELECTED|LVIS_FOCUSED);auto selected=list->GetItemText(1,0);SendMessageW(up,BM_CLICK,0,0);expect(list->GetItemText(0,0)==selected,"Compact Move up reorders the selected queue entry");SendMessageW(down,BM_CLICK,0,0);expect(list->GetItemText(1,0)==selected,"Compact Move down restores the selected queue entry");
  for(bool dark:{true,false}){uiDark=dark;dialog.refreshTheme();verifySurfaces();for(auto button:{up,down,remove})expect((GetWindowLongW(button,GWL_STYLE)&BS_TYPEMASK)==BS_OWNERDRAW,"Compact symbols retain their painter across theme changes");capture(w,dark?L"scheduler-actions-dark.png":L"scheduler-actions-light.png");}
  SendMessageW(remove,BM_CLICK,0,0);expect(list->GetItemCount()==1&&manager.jobs.size()==2,"Remove from queue retains the download catalog record");expect(!manager.isActive(first)&&!manager.isActive(second),"Queue action testing does not start downloads");SendMessageW(w,WM_CLOSE,0,0);
 });expect(observed,"Scheduler fixture actually opened and ran its actions");
}
static void sortHeaderRendering(){
 for(bool dark:{false,true})for(UINT dpi:{96u,144u,192u}){
  uiDark=dark;Form dialog("Sort header fixture",340,160);ThemeList* list=nullptr;bool observed=false;
  dialog.init=[&]{list=dialog.make<ThemeList>(WS_TABSTOP|WS_BORDER|LVS_REPORT,10,10,320,110);list->InsertColumn(0,L"File",LVCFMT_LEFT,280);list->InsertItem(0,L"download.iso");list->theme();dialog.button("Close",250,130,80,[&]{dialog.close();});};
  modal(dialog,L"Sort header fixture",[&](HWND window){
   observed=true;RECT outer{};GetWindowRect(window,&outer);SendMessageW(window,WM_DPICHANGED,MAKELONG(dpi,dpi),(LPARAM)&outer);auto header=list->GetHeaderCtrl();
   auto render=[&](int column,bool up,const wchar_t* suffix){
    list->sortIndicator(column,up);CRect area;header->GetClientRect(&area);CImage img;img.Create(area.Width(),area.Height(),32);auto dc=img.GetDC();FillRect(dc,&area,GetSysColorBrush(COLOR_WINDOW));header->SendMessage(WM_PRINTCLIENT,(WPARAM)dc,PRF_CLIENT);
    std::vector<COLORREF> pixels;for(int y=0;y<area.Height();++y)for(int x=0;x<area.Width();++x)pixels.push_back(GetPixel(dc,x,y));img.ReleaseDC();
    auto name=std::wstring(dark?L"dark-":L"light-")+std::to_wstring(dpi)+L"-sort-"+suffix+L".png";expect(SUCCEEDED(img.Save((output/name).c_str())),"Save native sort-header rendering");return pixels;
   };
   auto plain=render(-1,true,L"none"),up=render(0,true,L"up"),down=render(0,false,L"down");
   auto differences=[](const auto& a,const auto& b){size_t count=0;for(size_t i=0;i<std::min(a.size(),b.size());++i)if(a[i]!=b[i])++count;return count;};
   expect(differences(plain,up)>3,"Ascending sort arrow changes the rendered header");expect(differences(up,down)>3,"Ascending and descending arrows render differently");
   HDITEMW item{};item.mask=HDI_FORMAT;header->GetItem(0,&item);expect((item.fmt&HDF_SORTDOWN)!=0,"Rendered descending arrow retains native header metadata");
   auto cleared=render(-1,true,L"cleared");expect(cleared==plain,"Clearing sort restores the original header rendering");
   SendMessageW(window,WM_CLOSE,0,0);
  });expect(observed,"Sort-header fixture actually exercised native controls");
 }
}
class DarkGroupsApplication:public CWinApp {
 int code=1;
public:BOOL InitInstance()override{
 CWinApp::InitInstance();INITCOMMONCONTROLSEX common{sizeof(common),ICC_WIN95_CLASSES};InitCommonControlsEx(&common);AfxOleInit();int argc=0;auto argv=CommandLineToArgvW(GetCommandLineW(),&argc);
 try{if(argc!=2)throw std::runtime_error("Pass isolated output directory");output=fs::path(argv[1]);fs::create_directories(output);
 for(bool dark:{true,false})for(UINT dpi:{96u,144u,192u}){
 uiDark=dark;Form dialog("Group contrast fixture",280,120);dialog.dialogUnits=true;CWnd* group=nullptr;
 dialog.init=[&]{group=dialog.control(L"BUTTON","Readable group caption",BS_GROUPBOX,10,10,255,85);dialog.button("Close",215,100,50,[&]{dialog.close();});};
 modal(dialog,L"Group contrast fixture",[&](HWND w){RECT window{};GetWindowRect(w,&window);SendMessageW(w,WM_DPICHANGED,MAKELONG(dpi,dpi),(LPARAM)&window);auto area=bounds(group->GetSafeHwnd());CImage pixels;pixels.Create(area.Width(),area.Height(),32);auto dc=pixels.GetDC();RECT fill{0,0,area.Width(),area.Height()};FillRect(dc,&fill,(HBRUSH)dialogBrush().GetSafeHandle());auto old=SelectObject(dc,(HFONT)SendMessageW(group->GetSafeHwnd(),WM_GETFONT,0,0));SIZE extent{};GetTextExtentPoint32W(dc,L"Readable group caption",22,&extent);SendMessageW(group->GetSafeHwnd(),WM_PRINT,(WPARAM)dc,PRF_CLIENT|PRF_ERASEBKGND);int contrasting=0;for(int y=0;y<std::min<int>(extent.cy+2,area.Height());++y)for(int x=MulDiv(12,dpi,96);x<std::min<int>(extent.cx,area.Width());++x){auto c=GetPixel(dc,x,y);if(c==CLR_INVALID)continue;int brightness=(GetRValue(c)+GetGValue(c)+GetBValue(c))/3;if(dark?brightness>170:brightness<100)++contrasting;}SelectObject(dc,old);pixels.ReleaseDC();auto name=std::string(dark?"dark-":"light-")+std::to_string(dpi);pixels.Save((output/wide(name+"-group.png")).c_str());capture(w,wide(name+"-dialog.png").c_str());results.push_back({{"name",name+" caption contrast"},{"passed",contrasting>20},{"contrastingPixels",contrasting}});expect(contrasting>20,"Group caption has visible contrasting glyphs");press(w,L"Close");});}
 schedulerActions(false);schedulerActions(true);sortHeaderRendering();ToolbarComponentTest::appearanceTransitions();
 code=0;}catch(const std::exception& e){failure=e.what();}if(argv)LocalFree(argv);atomicText(output/L"results.json",Json{{"passed",code==0},{"error",failure},{"checks",results}}.dump(2),false);return FALSE;}
 int ExitInstance()override{AfxOleTerm(FALSE);return code;}
};DarkGroupsApplication application;
}