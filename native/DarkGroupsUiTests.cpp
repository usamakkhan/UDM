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
public:static void menuBarAppearance(){
 Manager manager(output/L"menu-state");auto prefs=manager.state["Settings"];prefs["ClipboardMonitor"]=false;prefs["CloseToTray"]=false;manager.setSettings(prefs);
 auto frame=new MainWindow(manager);AfxGetApp()->m_pMainWnd=frame;frame->KillTimer(1);
 try{
  auto root=frame->menu.GetSafeHmenu();std::vector<std::wstring> labels;std::vector<HMENU> children;
  for(int i=0;i<GetMenuItemCount(root);++i){wchar_t text[128]{};GetMenuStringW(root,i,text,128,MF_BYPOSITION);labels.push_back(text);children.push_back(GetSubMenu(root,i));}
  for(UINT dpi:{96u,144u,192u})for(bool dark:{true,false}){
   frame->scale=dpi/96.0f;prefs["DarkMode"]=dark;manager.setSettings(prefs);frame->applyAppearance();
   MENUINFO info{sizeof(info)};info.fMask=MIM_BACKGROUND;GetMenuInfo(root,&info);expect((info.hbrBack!=nullptr)==dark,"Menu bar background follows the effective theme");
   for(int i=0;i<GetMenuItemCount(root);++i){
    wchar_t text[128]{};MENUITEMINFOW item{sizeof(item)};item.fMask=MIIM_FTYPE|MIIM_DATA|MIIM_STRING|MIIM_SUBMENU;item.dwTypeData=text;item.cch=128;GetMenuItemInfoW(root,i,TRUE,&item);
    expect(labels[i]==text&&item.hSubMenu==children[i],"Themed menu preserves native labels and popup handles");expect(((item.fType&MFT_OWNERDRAW)!=0)==dark,"Menu item painting returns to native in light mode");
    if(!dark)continue;
    const auto amp=labels[i].find(L'&');if(amp!=std::wstring::npos){auto response=frame->SendMessage(WM_MENUCHAR,MAKEWPARAM(labels[i][amp+1],MF_POPUP),(LPARAM)root);expect(HIWORD(response)==MNC_EXECUTE&&LOWORD(response)==i,"Dark menu mnemonic opens the original popup position");}
    MEASUREITEMSTRUCT measured{};measured.CtlType=ODT_MENU;measured.itemData=item.dwItemData;frame->SendMessage(WM_MEASUREITEM,0,(LPARAM)&measured);expect(measured.itemWidth>10&&measured.itemHeight>10,"Themed menu measures readable text at each display scale");
    CImage pixels;pixels.Create(measured.itemWidth,measured.itemHeight,32);auto dc=pixels.GetDC();DRAWITEMSTRUCT draw{};draw.CtlType=ODT_MENU;draw.itemData=item.dwItemData;draw.hDC=dc;draw.rcItem={0,0,(LONG)measured.itemWidth,(LONG)measured.itemHeight};draw.itemState=ODS_NOACCEL;{const int popupDcState=SaveDC(dc);frame->SendMessage(WM_DRAWITEM,0,(LPARAM)&draw);RestoreDC(dc,popupDcState);}expect(GetPixel(dc,1,1)==RGB(32,32,32),"Dark menu paints a dark normal background");
    draw.itemState=ODS_SELECTED;{const int popupDcState=SaveDC(dc);frame->SendMessage(WM_DRAWITEM,0,(LPARAM)&draw);RestoreDC(dc,popupDcState);}expect(GetPixel(dc,1,1)==RGB(64,67,72),"Selected menu paints its visible keyboard/mouse highlight");pixels.ReleaseDC();
   }
   frame->status.SetText(L"29 downloads    0 active    0 Bytes/s",0,0);
   {CRect area;frame->status.GetClientRect(&area);CImage image;image.Create(area.Width(),area.Height(),32);auto dc=image.GetDC();frame->status.SendMessage(WM_PRINTCLIENT,(WPARAM)dc,PRF_CLIENT|PRF_ERASEBKGND);if(dark)expect(GetPixel(dc,area.Width()/2,area.Height()/2)==uiBackground(),"Dark status bar removes the light background");image.ReleaseDC();CString text=frame->status.GetText(0);expect(text==L"29 downloads    0 active    0 Bytes/s","Status text remains available through native accessibility metadata");image.Save((output/wide(std::string(dark?"dark-":"light-")+std::to_string(dpi)+"-status.png")).c_str());}
   CMenu popup,nested;popup.CreatePopupMenu();nested.CreatePopupMenu();nested.AppendMenuW(MF_STRING,9004,L"Nested action");popup.AppendMenuW(MF_STRING|MF_CHECKED,9000,L"&Checked\tCtrl+C");popup.AppendMenuW(MF_STRING|MF_GRAYED,9001,L"&Unavailable");popup.AppendMenuW(MF_SEPARATOR);popup.AppendMenuW(MF_POPUP,(UINT_PTR)nested.Detach(),L"&Submenu");popup.AppendMenuW(MF_STRING,9002,L"&Copy");
   frame->menuTheme.popup(popup.GetSafeHmenu(),dark);
   for(int i=0;i<popup.GetMenuItemCount();++i){MENUITEMINFOW item{sizeof(item)};item.fMask=MIIM_FTYPE|MIIM_DATA|MIIM_STATE;GetMenuItemInfoW(popup.GetSafeHmenu(),i,TRUE,&item);expect(((item.fType&MFT_OWNERDRAW)!=0)==dark,"Popup painting follows the selected theme");if(!dark)continue;
    MEASUREITEMSTRUCT measure{};measure.CtlType=ODT_MENU;measure.itemData=item.dwItemData;frame->SendMessage(WM_MEASUREITEM,0,(LPARAM)&measure);expect(measure.itemWidth>0&&measure.itemHeight>0,"Popup actions and separators have measured dimensions");
    CImage image;image.Create(measure.itemWidth,measure.itemHeight,32);auto dc=image.GetDC();DRAWITEMSTRUCT draw{};draw.CtlType=ODT_MENU;draw.itemData=item.dwItemData;draw.hDC=dc;draw.rcItem={0,0,(LONG)measure.itemWidth,(LONG)measure.itemHeight};if(i==1)draw.itemState=ODS_DISABLED;{const int popupDcState=SaveDC(dc);frame->SendMessage(WM_DRAWITEM,0,(LPARAM)&draw);RestoreDC(dc,popupDcState);}expect(GetPixel(dc,1,1)==RGB(32,32,32),"Popup action and separator edges are dark");image.ReleaseDC();image.Save((output/wide(std::to_string(dpi)+"-popup-"+std::to_string(i)+".png")).c_str());
   }
   expect((popup.GetMenuState(9000,MF_BYCOMMAND)&MF_CHECKED)!=0&&(popup.GetMenuState(9001,MF_BYCOMMAND)&MF_GRAYED)!=0,"Popup retains checked and disabled command state");
   if(dark){LRESULT key=0;expect(frame->menuTheme.mnemonic(popup.GetSafeHmenu(),L'c',key)&&HIWORD(key)==MNC_SELECT,"Duplicate popup mnemonics select rather than immediately execute");expect(!frame->menuTheme.mnemonic(popup.GetSafeHmenu(),L'u',key),"Disabled popup actions cannot execute through mnemonics");}
   frame->menuTheme.releasePopup(popup.GetSafeHmenu());expect((popup.GetMenuState(0,MF_BYPOSITION)&MF_OWNERDRAW)==0,"Popup lifecycle restores native item metadata");
   capture(frame->GetSafeHwnd(),wide(std::string(dark?"dark-":"light-")+std::to_string(dpi)+"-main-menu.png").c_str());
  }
  frame->quitting=true;frame->OnClose();AfxGetApp()->m_pMainWnd=nullptr;
 }catch(...){frame->quitting=true;frame->OnClose();AfxGetApp()->m_pMainWnd=nullptr;throw;}
}
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
static void listGridRendering(){
 for(bool dark:{false,true})for(UINT dpi:{96u,144u,192u}){
  uiDark=dark;Form dialog("List grid fixture",360,200);ThemeList* list=nullptr;bool observed=false;
  dialog.init=[&]{list=dialog.make<ThemeList>(WS_TABSTOP|WS_BORDER|LVS_REPORT,10,10,330,140);list->SetExtendedStyle(LVS_EX_GRIDLINES|LVS_EX_FULLROWSELECT|LVS_EX_DOUBLEBUFFER|LVS_EX_HEADERDRAGDROP);list->InsertColumn(0,L"File",LVCFMT_LEFT,140);list->InsertColumn(1,L"Status",LVCFMT_LEFT,260);for(int i=0;i<15;++i)list->InsertItem(i,L"download.iso");list->SetItemState(0,LVIS_SELECTED,LVIS_SELECTED);list->theme();dialog.button("Close",260,165,80,[&]{dialog.close();});};
  modal(dialog,L"List grid fixture",[&](HWND window){
   observed=true;RECT outer{};GetWindowRect(window,&outer);SendMessageW(window,WM_DPICHANGED,MAKELONG(dpi,dpi),(LPARAM)&outer);
   expect(((list->GetExtendedStyle()&LVS_EX_GRIDLINES)!=0)==!dark,"Dark grid replaces native bright lines; light grid remains native");
   auto render=[&](const wchar_t* suffix){
    CRect area,row,column;list->GetClientRect(&area);list->GetItemRect(list->GetTopIndex()+1,&row,LVIR_BOUNDS);auto header=list->GetHeaderCtrl();header->GetItemRect(0,&column);header->MapWindowPoints(list,&column);
    CImage image;image.Create(area.Width(),area.Height(),32);auto dc=image.GetDC();FillRect(dc,&area,GetSysColorBrush(COLOR_WINDOW));list->SendMessage(WM_PRINTCLIENT,(WPARAM)dc,PRF_CLIENT|PRF_ERASEBKGND);
    auto horizontal=GetPixel(dc,25,row.bottom-1),vertical=GetPixel(dc,column.right-1,row.CenterPoint().y);image.ReleaseDC();
    if(dark){expect(horizontal==RGB(64,67,72),"Printed dark list has subtle horizontal separators");expect(vertical==RGB(64,67,72),"Printed dark list has subtle column separators");}
    auto name=std::wstring(dark?L"dark-":L"light-")+std::to_wstring(dpi)+L"-grid-"+suffix+L".png";expect(SUCCEEDED(image.Save((output/name).c_str())),"Save native grid rendering");
   };
   render(L"initial");list->Scroll(CSize(24,20));render(L"scrolled");
   expect((list->GetItemState(0,LVIS_SELECTED)&LVIS_SELECTED)!=0&&list->GetItemCount()==15,"Grid painting preserves selection and rows");
   uiDark=!dark;list->theme();expect(((list->GetExtendedStyle()&LVS_EX_GRIDLINES)!=0)==dark,"Theme transition restores or replaces native grid");
   SendMessageW(window,WM_CLOSE,0,0);
  });expect(observed,"Grid fixture exercised actual native list controls");
 }
}
static void dialogPopupRendering(){
 for(bool dark:{false,true})for(UINT dpi:{96u,144u,192u}){
  uiDark=dark;Form dialog("Dialog menu fixture",360,180);CWnd* edit=nullptr;CComboBox* choice=nullptr;
  dialog.init=[&]{edit=dialog.edit("Preserved draft",12,12,330,70,false,true);choice=dialog.combo({"First","Second"},"First",12,92,230);dialog.button("Close",260,145,85,[&]{dialog.close();});};
  modal(dialog,L"Dialog menu fixture",[&](HWND window){RECT bounds{};GetWindowRect(window,&bounds);SendMessageW(window,WM_DPICHANGED,MAKELONG(dpi,dpi),(LPARAM)&bounds);dialog.refreshTheme();
   {CMenu menu;edit->SendMessage(EM_SETSEL,0,4);buildEditContextMenu(edit->GetSafeHwnd(),menu);expect((menu.GetMenuState(2,MF_BYCOMMAND)&MF_GRAYED)==0,"Writable selected text permits Cut");edit->SendMessage(EM_SETREADONLY,TRUE);CMenu readonlyMenu;buildEditContextMenu(edit->GetSafeHwnd(),readonlyMenu);expect((readonlyMenu.GetMenuState(2,MF_BYCOMMAND)&MF_GRAYED)&&(readonlyMenu.GetMenuState(4,MF_BYCOMMAND)&MF_GRAYED)&&(readonlyMenu.GetMenuState(5,MF_BYCOMMAND)&MF_GRAYED),"Read-only edit menu disables Cut Paste and Delete");edit->SendMessage(EM_SETREADONLY,FALSE);edit->SendMessage(EM_SETPASSWORDCHAR,L'*');CMenu secretMenu;buildEditContextMenu(edit->GetSafeHwnd(),secretMenu);expect((secretMenu.GetMenuState(2,MF_BYCOMMAND)&MF_GRAYED)&&(secretMenu.GetMenuState(3,MF_BYCOMMAND)&MF_GRAYED),"Password edit menu disables Cut and Copy");edit->SendMessage(EM_SETPASSWORDCHAR,0);editContextCommand(edit->GetSafeHwnd(),6);DWORD first=0,last=0;edit->SendMessage(EM_GETSEL,(WPARAM)&first,(LPARAM)&last);expect(first==0&&last==15,"Dark edit menu Select all uses native selection");}
   choice->SetCurSel(1);expect(choice->GetCurSel()==1,"Themed combo retains native selection messages");if(dark){CRect area;choice->GetClientRect(&area);CImage image;image.Create(area.Width(),area.Height(),32);auto dc=image.GetDC();choice->SendMessage(WM_PRINTCLIENT,(WPARAM)dc,PRF_CLIENT);const auto color=GetPixel(dc,area.Width()/2,area.Height()/2);expect(color==uiBackground(),"Closed combo field is dark rather than system white");image.ReleaseDC();}
   for(bool editOwner:{false,true}){CMenu menu,sub;menu.CreatePopupMenu();sub.CreatePopupMenu();sub.AppendMenu(MF_STRING,8103,L"Nested command");menu.AppendMenu(MF_STRING|MF_CHECKED,8100,L"&Checked\tCtrl+C");menu.AppendMenu(MF_STRING|MF_GRAYED,8101,L"Unavailable");menu.AppendMenu(MF_SEPARATOR);menu.AppendMenu(MF_POPUP,(UINT_PTR)sub.GetSafeHmenu(),L"&More");const HWND target=editOwner?edit->GetSafeHwnd():window;
    SendMessageW(target,WM_INITMENUPOPUP,(WPARAM)menu.GetSafeHmenu(),0);MENUITEMINFOW item{sizeof(item)};item.fMask=MIIM_FTYPE|MIIM_DATA;GetMenuItemInfoW(menu,0,TRUE,&item);expect(((item.fType&MFT_OWNERDRAW)!=0)==dark,"Dialog and edit popup follow the selected appearance");
    if(dark){MEASUREITEMSTRUCT measure{};measure.CtlType=ODT_MENU;measure.itemData=item.dwItemData;expect(SendMessageW(target,WM_MEASUREITEM,0,(LPARAM)&measure)&&measure.itemWidth>50&&measure.itemHeight>10,"Dialog popup gets valid DPI-scaled measurements without a menu bar");CImage pixels;pixels.Create(measure.itemWidth,measure.itemHeight,32);auto dc=pixels.GetDC();DRAWITEMSTRUCT draw{};draw.CtlType=ODT_MENU;draw.itemData=item.dwItemData;draw.hDC=dc;draw.rcItem={0,0,(LONG)measure.itemWidth,(LONG)measure.itemHeight};const int saved=SaveDC(dc);expect(SendMessageW(target,WM_DRAWITEM,0,(LPARAM)&draw)!=0,"Dialog or edit owner handles popup drawing");RestoreDC(dc,saved);expect(GetPixel(dc,1,1)==RGB(32,32,32),"Dialog popup surface is dark");pixels.ReleaseDC();pixels.Save((output/wide("dialog-popup-"+std::to_string(dpi)+(editOwner?"-edit.png":"-form.png"))).c_str());const auto key=SendMessageW(target,WM_MENUCHAR,MAKEWPARAM(L'C',MF_POPUP),(LPARAM)menu.GetSafeHmenu());expect(HIWORD(key)==MNC_EXECUTE&&LOWORD(key)==0,"Dialog popup access key retains native command position");}
    SendMessageW(target,WM_UNINITMENUPOPUP,(WPARAM)menu.GetSafeHmenu(),0);GetMenuItemInfoW(menu,0,TRUE,&item);expect(!(item.fType&MFT_OWNERDRAW)&&item.dwItemData==0,"Dialog popup restores native item data after closing");expect((menu.GetMenuState(8100,MF_BYCOMMAND)&MF_CHECKED)!=0&&(menu.GetMenuState(8101,MF_BYCOMMAND)&MF_GRAYED)!=0,"Dialog popup retains checked and disabled state");sub.Detach();
   }expect(text(edit)=="Preserved draft","Popup painting preserves editable drafts");capture(window,wide(std::string(dark?"dark":"light")+"-dialog-controls-"+std::to_string(dpi)+".png").c_str());press(window,L"Close");
  });
 }
}
static void messageDialogRendering(){
 uiDark=true;
 for(auto flags:{UINT(MB_OK|MB_ICONINFORMATION),UINT(MB_YESNO|MB_ICONWARNING|MB_DEFBUTTON2),UINT(MB_OKCANCEL)}){
  expectedTitle=L"Dark message fixture";began=GetTickCount64();action=[&](HWND window){const auto focus=windowText(GetFocus());expect(focus==((flags&MB_TYPEMASK)==MB_YESNO?L"No":L"OK"),"Dark message retains the requested default button focus");capture(window,wide("message-"+std::to_string(flags)+".png").c_str());if((flags&MB_TYPEMASK)==MB_OKCANCEL)SendMessageW(window,WM_CLOSE,0,0);else press(window,(flags&MB_TYPEMASK)==MB_YESNO?L"No":L"OK");};auto timer=SetTimer(nullptr,0,80,onTimer);const int result=themedMessage(nullptr,L"Review this operation. Your saved downloads remain unchanged.",L"Dark message fixture",flags);KillTimer(nullptr,timer);action={};if(!failure.empty())throw std::runtime_error(failure);expect(result==((flags&MB_TYPEMASK)==MB_YESNO?IDNO:(flags&MB_TYPEMASK)==MB_OKCANCEL?IDCANCEL:IDOK),"Dark message returns the correct native confirmation or cancel result");
 }
}
static void dateControlRendering(){
 for(bool dark:{true,false}){uiDark=dark;Form dialog("Native date fixture",420,140);ThemeDateTime* themed=nullptr;CDateTimeCtrl* reference=nullptr;dialog.init=[&]{themed=dialog.make<ThemeDateTime>(DTS_SHORTDATEFORMAT|WS_TABSTOP,12,12,390,26);reference=dialog.make<CDateTimeCtrl>(DTS_SHORTDATEFORMAT|WS_TABSTOP,12,52,390,26);for(CDateTimeCtrl* field:{static_cast<CDateTimeCtrl*>(themed),reference}){field->SetFormat(L"yyyy-MM-dd");SYSTEMTIME initial{2026,10,0,16,0,0,0,0};field->SetTime(&initial);}dialog.button("Close",300,105,100,[&]{dialog.close();});};
  modal(dialog,L"Native date fixture",[&](HWND window){for(UINT dpi:{96u,144u,192u}){RECT bounds{};GetWindowRect(window,&bounds);SendMessageW(window,WM_DPICHANGED,MAKELONG(dpi,dpi),(LPARAM)&bounds);SYSTEMTIME a{},b{};themed->GetTime(&a);reference->GetTime(&b);expect(a.wYear==b.wYear&&a.wMonth==b.wMonth&&a.wDay==b.wDay,"Themed date retains the same value as Windows native control");for(auto key:{VK_RIGHT,VK_UP}){themed->SetFocus();themed->SendMessage(WM_KEYDOWN,key);themed->SendMessage(WM_KEYUP,key);reference->SetFocus();reference->SendMessage(WM_KEYDOWN,key);reference->SendMessage(WM_KEYUP,key);}themed->GetTime(&a);reference->GetTime(&b);expect(a.wYear==b.wYear&&a.wMonth==b.wMonth&&a.wDay==b.wDay,"Themed date keyboard changes match native Windows date handling");CRect area;themed->GetClientRect(&area);CImage image;image.Create(area.Width(),area.Height(),32);auto dc=image.GetDC();themed->SendMessage(WM_PRINTCLIENT,(WPARAM)dc,PRF_CLIENT);auto color=GetPixel(dc,area.Width()*2/3,area.Height()/2);expect(color!=CLR_INVALID&&(dark?GetRValue(color)<100:GetRValue(color)>180),"Native date field follows dark and light client palettes");image.ReleaseDC();image.Save((output/wide(std::string(dark?"dark":"light")+"-date-"+std::to_string(dpi)+".png")).c_str());}press(window,L"Close");});
 }
}
static void actualDialogAudit(){
 uiDark=true;Manager manager(output/L"dialog-audit-state");auto prefs=manager.state["Settings"];prefs["ClipboardMonitor"]=false;manager.setSettings(prefs);auto job=manager.add("https://example.invalid/dialog.bin","","dialog.bin","Main queue",true);
 Options options(manager,nullptr);modal(options,L"UDM Configuration",[&](HWND window){auto tabs=children(window,L"SysTabControl32").front();for(int i=0;i<9;++i){SendMessageW(tabs,TCM_SETCURSEL,i,0);NMHDR notification{tabs,(UINT_PTR)GetDlgCtrlID(tabs),TCN_SELCHANGE};SendMessageW(window,WM_NOTIFY,notification.idFrom,(LPARAM)&notification);capture(window,wide("options-page-"+std::to_string(i)+".png").c_str());}SendMessageW(window,WM_CLOSE,0,0);});
 Scheduler scheduler(manager,nullptr);modal(scheduler,L"Scheduler",[&](HWND window){auto tabs=children(window,L"SysTabControl32").front();for(int i=0;i<3;++i){SendMessageW(tabs,TCM_SETCURSEL,i,0);NMHDR notification{tabs,(UINT_PTR)GetDlgCtrlID(tabs),TCN_SELCHANGE};SendMessageW(window,WM_NOTIFY,notification.idFrom,(LPARAM)&notification);capture(window,wide("scheduler-page-"+std::to_string(i)+".png").c_str());}SendMessageW(window,WM_CLOSE,0,0);});
 GrabberDialog grabber(manager,nullptr);modal(grabber,L"UDM Site Grabber. Step 1 --- Set start page",[&](HWND window){capture(window,L"grabber-dark.png");SendMessageW(window,WM_CLOSE,0,0);});
 ExportDownloadsDialog exporter(manager,{job},"Main queue");modal(exporter,L"Export downloads",[&](HWND window){capture(window,L"export-dark.png");SendMessageW(window,WM_CLOSE,0,0);});
 expectedTitle=L"File Properties";began=GetTickCount64();action=[&](HWND window){capture(window,L"properties-dark.png");SendMessageW(window,WM_CLOSE,0,0);};auto timer=SetTimer(nullptr,0,80,onTimer);fileProperties(nullptr,manager,job);KillTimer(nullptr,timer);action={};if(!failure.empty())throw std::runtime_error(failure);expect(str(job->data,"Status")=="Paused"&&!manager.isActive(job),"Dialog appearance audit never starts the fixture download");
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
 dialogPopupRendering();messageDialogRendering();dateControlRendering();actualDialogAudit();schedulerActions(false);schedulerActions(true);sortHeaderRendering();listGridRendering();ToolbarComponentTest::menuBarAppearance();ToolbarComponentTest::appearanceTransitions();
 code=0;}catch(const std::exception& e){failure=e.what();}if(argv)LocalFree(argv);atomicText(output/L"results.json",Json{{"passed",code==0},{"error",failure},{"checks",results}}.dump(2),false);return FALSE;}
 int ExitInstance()override{AfxOleTerm(FALSE);return code;}
};DarkGroupsApplication application;
}