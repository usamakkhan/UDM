// Isolated tests drive only windows created by this executable.
#define UDM_TOOLBAR_COMPONENT_TEST
#include "App.cpp"
namespace udm {
static Json outcomes=Json::array(),details=Json::object();
static fs::path output,fixture;static std::function<void(HWND)> modalAction;
static ULONGLONG modalBegan;static std::string componentError,modalError;static HWND currentModal=nullptr;
static const wchar_t* expectedModal=L"Customize Toolbar";
static HWND menuOwner=nullptr;static bool queueMenuSeen=false;
static void CALLBACK queueMenuTick(HWND,UINT,UINT_PTR,DWORD){GUITHREADINFO info{sizeof(info)};if(GetGUIThreadInfo(GetCurrentThreadId(),&info)&&(info.flags&GUI_INMENUMODE)&&info.hwndMenuOwner==menuOwner){queueMenuSeen=true;EndMenu();}}
static void expect(bool value,const char* name){outcomes.push_back({{"name",name},{"passed",value}});if(!output.empty())atomicText(output/L"progress.json",outcomes.dump(2),false);if(!value)throw std::runtime_error(name);}
static void check(bool value,const char* name){expect(value,name);}
template<class F> static void rejects(F action,const char* name){bool rejected=false;try{action();}catch(const std::exception&){rejected=true;}expect(rejected,name);}
#include "ToolbarChecks.hpp"
static std::wstring title(HWND window){wchar_t text[512]{};GetWindowTextW(window,text,512);return text;}
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
static void selectList(HWND dialog,int id,int index){
 auto list=GetDlgItem(dialog,id);if(!list||SendMessageW(list,LB_SETCURSEL,index,0)==LB_ERR)throw std::runtime_error("Could not select customization item");
 SendMessageW(dialog,WM_COMMAND,MAKEWPARAM(id,LBN_SELCHANGE),(LPARAM)list);
}
static void selectText(HWND dialog,int id,const wchar_t* label){
 auto list=GetDlgItem(dialog,id);auto i=SendMessageW(list,LB_FINDSTRINGEXACT,(WPARAM)-1,(LPARAM)label);
 if(i==LB_ERR)throw std::runtime_error("Customization command is missing");selectList(dialog,id,(int)i);
}
static void press(HWND dialog,int id){
 auto button=GetDlgItem(dialog,id);if(!button||!IsWindowEnabled(button))throw std::runtime_error("Customization button is disabled");
 SendMessageW(button,BM_CLICK,0,0);
}
static BOOL CALLBACK modalObserver(HWND window,LPARAM){
 if(title(window)!=expectedModal)return TRUE;currentModal=window;
 try{modalAction(window);}catch(const std::exception& e){modalError=e.what();PostMessageW(window,WM_CLOSE,0,0);}return FALSE;
}
static void CALLBACK modalTick(HWND,UINT,UINT_PTR,DWORD){
 if(GetTickCount64()-modalBegan>10000){modalError="Customization timed out";if(currentModal)PostMessageW(currentModal,WM_CLOSE,0,0);return;}
 EnumThreadWindows(GetCurrentThreadId(),modalObserver,0);
}
class ToolbarComponentTest{
 static void customize(MainWindow& frame,std::function<void(HWND)> action){
  currentModal=nullptr;modalError.clear();modalAction=action;modalBegan=GetTickCount64();auto timer=SetTimer(nullptr,0,75,modalTick);
  frame.command(CMD_TOOLBAR);KillTimer(nullptr,timer);modalAction={};if(!modalError.empty())throw std::runtime_error(modalError);
 }
public:static void run(Manager& manager,MainWindow& frame){
 frame.KillTimer(1);toolbarChecks(output/L"model");const auto initial=frame.preferences();const auto custom=std::vector<int>{10,-1,0,-1,8,9};
 expect(frame.currentToolbarLayout()==defaultToolbarLayout(),"Default native toolbar has twelve commands");
 expect(frame.toolbar.GetImageList()->GetImageCount()==12&&frame.toolbar.GetHotImageList()->GetImageCount()==12&&frame.toolbar.GetDisabledImageList()->GetImageCount()==12,"Normal, hover and disabled images cover every command");
 TBBUTTON share{};frame.toolbar.GetButton(11,&share);
 expect(share.idCommand==CMD_SHARE&&share.idCommand!=CMD_BATCH&&share.iBitmap==11,"Last toolbar button maps to Share rather than Batch and uses the twelfth image");
 expect(frame.toolbar.IsButtonEnabled(CMD_SHARE),"Share is enabled without a download selection");
 wchar_t tip[128]{};NMTBGETINFOTIPW info{};info.hdr={frame.toolbar.GetSafeHwnd(),506,TBN_GETINFOTIPW};info.iItem=CMD_SHARE;info.pszText=tip;info.cchTextMax=128;LRESULT tipResult=0;frame.toolbarNotification(&info.hdr,&tipResult);
 expect(std::wstring(tip)==L"Tell a friend","Share tooltip follows the explicit command mapping");
 bool shareOpened=false;expectedModal=L"Tell a friend";currentModal=nullptr;modalError.clear();modalBegan=GetTickCount64();modalAction=[&](HWND dialog){shareOpened=true;PostMessageW(dialog,WM_CLOSE,0,0);};auto shareTimer=SetTimer(nullptr,0,75,modalTick);
 frame.SendMessage(WM_COMMAND,MAKEWPARAM(share.idCommand,0),(LPARAM)frame.toolbar.GetSafeHwnd());KillTimer(nullptr,shareTimer);modalAction={};expectedModal=L"Customize Toolbar";
 expect(shareOpened&&modalError.empty(),"Real toolbar command opens Tell a friend without launching email");

 auto oldLayout=std::vector<int>{0,1,2,3,4,5,6,7,8,9,10};auto oldPrefs=initial;saveToolbarLayout(oldPrefs,oldLayout);manager.setSettings(oldPrefs);frame.applyAppearance();
 expect(frame.currentToolbarLayout()==oldLayout,"Existing explicit eleven-button layouts remain unchanged");
 manager.setSettings(initial);frame.applyAppearance();
 wchar_t text[128]{};frame.toolbar.SendMessage(TB_GETBUTTONTEXTW,CMD_CLEAN,(LPARAM)text);
 expect(std::wstring(text)==L"Delete completed","Unicode captions preserve the whole command label");
 for(int id:{CMD_STARTQUEUE,CMD_STOPQUEUE}){TBBUTTON b{};frame.toolbar.GetButton(frame.toolbar.CommandToIndex(id),&b);expect((b.fsStyle&BTNS_DROPDOWN)!=0,"Queue command has a native dropdown");}
 expect(!frame.toolbar.IsButtonEnabled(CMD_RESUME)&&frame.toolbar.IsButtonEnabled(CMD_ADD),"Selection-dependent commands have correct enablement");
 auto tr=bounds(frame.toolbar.GetSafeHwnd()),lr=bounds(frame.table.GetSafeHwnd());
 details["defaultGeometry"]={{"toolbarWidth",tr.Width()},{"toolbarHeight",tr.Height()},{"listWidth",lr.Width()},{"listHeight",lr.Height()}};
 details["requestedButtonWidth"]=frame.toolbarButtonWidth;details["requestedButtonHeight"]=frame.toolbarButtonHeight;details["buttonRects"]=Json::array();
 CRect first,lastButton;frame.toolbar.GetItemRect(0,&first);frame.toolbar.GetItemRect(ToolbarCommandCount-1,&lastButton);
 for(int i=0;i<ToolbarCommandCount;++i){CRect r;frame.toolbar.GetItemRect(i,&r);details["buttonRects"].push_back({r.left,r.top,r.right,r.bottom});}
 expect(first.top==lastButton.top,"Default toolbar fits on one row at the normal window size");
 expect(lr.top>=tr.bottom&&lr.Width()>0&&lr.Height()>0,"Download list fits below the toolbar");
 captureComponent(frame.GetSafeHwnd(),output/L"toolbar-default.png");
 CImage categoryImage;categoryImage.Load((output/L"toolbar-default.png").c_str());auto closeBounds=bounds(frame.categoryClose.m_hWnd),frameBounds=bounds(frame.m_hWnd);int glyphPixels=0;
 for(int y=closeBounds.top+4;y<closeBounds.bottom-4;++y)for(int x=closeBounds.left+4;x<closeBounds.right-4;++x)if(categoryImage.GetPixel(x-frameBounds.left,y-frameBounds.top)==uiForeground())++glyphPixels;
 expect(glyphPixels>=4,"Close glyph renders visibly inside category button");
 CString closeCaption;frame.categoryClose.GetWindowText(closeCaption);
 expect(closeCaption==L"Close categories"&&(frame.categoryClose.GetStyle()&WS_TABSTOP),"Category close button has an accessible name and keyboard focus stop");
 frame.categoryClose.SendMessage(BM_CLICK);
 expect(yes(frame.preferences(),"HideCategories")&&!shown(frame.tree.m_hWnd)&&!shown(frame.categoryClose.m_hWnd),"Panel close button hides categories using the saved preference");
 POINT tableOrigin{0,0};frame.ClientToScreen(&tableOrigin);
 expect(bounds(frame.table.m_hWnd).left==tableOrigin.x,"Closing categories gives its width to the download list");
 expect(GetFocus()==frame.table.m_hWnd,"Closing the category panel moves focus to the download list");
 frame.command(CMD_HIDE_CATEGORIES);
 expect(!yes(frame.preferences(),"HideCategories")&&shown(frame.tree.m_hWnd)&&shown(frame.categoryClose.m_hWnd),"View command restores categories and its close button");
 for(auto id:{CMD_STARTQUEUE,CMD_STOPQUEUE}){
  NMTOOLBARW notification{};notification.hdr={frame.toolbar.GetSafeHwnd(),506,TBN_DROPDOWN};notification.iItem=id;
  menuOwner=frame.GetSafeHwnd();queueMenuSeen=false;auto timer=SetTimer(nullptr,0,75,queueMenuTick);LRESULT result=0;
  frame.toolbarNotification(&notification.hdr,&result);KillTimer(nullptr,timer);expect(queueMenuSeen,"Queue dropdown opens an owned native menu and can be dismissed");
 }
 auto p=initial;saveToolbarLayout(p,custom);manager.setSettings(p);frame.applyAppearance();
 expect(frame.currentToolbarLayout()==custom,"Reordered commands and separators are installed");
 int step=0;customize(frame,[&](HWND dialog){
  switch(step++){
   case 0:selectList(dialog,201,0);SendMessageW(GetDlgItem(dialog,201),WM_KEYDOWN,VK_DOWN,0);expect(SendMessageW(GetDlgItem(dialog,201),LB_GETCURSEL,0,0)==1,"Available commands support keyboard selection");selectList(dialog,203,0);selectText(dialog,201,L"Options");captureComponent(dialog,output/L"customize-toolbar.png");break;
   case 1:press(dialog,1);break;
   case 2:expect(frame.currentToolbarLayout().front()==6,"Add inserts the selected available command");selectList(dialog,203,0);break;
   case 3:press(dialog,204);break;
   case 4:expect(frame.currentToolbarLayout()==custom,"Remove restores the previous command layout");selectList(dialog,203,0);break;
   case 5:press(dialog,207);break;
   case 6:expect(frame.currentToolbarLayout().at(1)==10,"Move Down reorders the current command");press(dialog,206);break;
   case 7:expect(frame.currentToolbarLayout()==custom,"Move Up restores the command position");selectList(dialog,203,2);selectText(dialog,201,L"Separator");break;
   case 8:press(dialog,1);break;
   case 9:{auto items=frame.currentToolbarLayout();expect(items.size()==7&&std::count(items.begin(),items.end(),-1)==3,"Add supports repeated separators");PostMessageW(dialog,WM_CLOSE,0,0);break;}
  }
 });
 expect(step==10,"Native customization completes its add/remove/reorder workflow");
 step=0;customize(frame,[&](HWND dialog){switch(step++){
 case 0:selectList(dialog,203,0);selectText(dialog,201,L"Tell a friend");press(dialog,1);break;
 case 1:expect(frame.currentToolbarLayout().front()==11,"Customize adds Tell a friend at the selected position");selectList(dialog,203,0);press(dialog,207);break;
 case 2:expect(frame.currentToolbarLayout().at(1)==11,"Tell a friend can move down without changing command identity");selectList(dialog,203,1);press(dialog,206);break;
 case 3:expect(frame.currentToolbarLayout().front()==11,"Tell a friend can move up");selectList(dialog,203,0);press(dialog,204);break;
 case 4:{auto layout=frame.currentToolbarLayout();expect(std::count(layout.begin(),layout.end(),11)==0,"Tell a friend can be removed");PostMessageW(dialog,WM_CLOSE,0,0);break;}
 }});

 {Manager restored(manager.root);expect(toolbarLayout(restored.state["Settings"])==frame.currentToolbarLayout(),"Closing customization persists the modified layout");}
 step=0;customize(frame,[&](HWND dialog){if(step++==0)press(dialog,202);else{expect(frame.currentToolbarLayout()==defaultToolbarLayout(),"Reset restores all default commands");PostMessageW(dialog,WM_CLOSE,0,0);}});
 expect(toolbarLayout(frame.preferences())==defaultToolbarLayout(),"Reset is saved when customization closes");
 for(auto size:{"Small","Medium","Large"})for(auto style:{"Icons and text","Icons only","Text only"}){
  p=frame.preferences();p["ToolbarSize"]=size;p["ToolbarStyle"]=style;manager.setSettings(p);frame.applyAppearance();
  CRect last;frame.toolbar.GetItemRect(frame.toolbar.GetButtonCount()-1,&last);
  expect(frame.currentToolbarLayout()==defaultToolbarLayout()&&last.bottom<=bounds(frame.toolbar.GetSafeHwnd()).Height(),"Size and caption mode preserve commands without vertical clipping");
 }
 manager.setSettings(initial);frame.applyAppearance();frame.command(CMD_TOOLBAR_HIDE);
 POINT origin{0,0};ClientToScreen(frame.GetSafeHwnd(),&origin);
 expect(!shown(frame.toolbar.GetSafeHwnd())&&bounds(frame.table.GetSafeHwnd()).top==origin.y,"Hide removes the toolbar and its occupied space");
 frame.command(CMD_TOOLBAR_HIDE);expect(shown(frame.toolbar.GetSafeHwnd()),"View command restores the toolbar");
 frame.toolbarSkins={fixture};frame.command(31000);int w=0,h=0;ImageList_GetIconSize(frame.toolbarNormal.GetSafeHandle(),&w,&h);int dpi=GetDpiForWindow(frame.GetSafeHwnd());
 expect(frame.toolbarSkinError.empty()&&frame.toolbarSkinName=="UDM fixture"&&w==MulDiv(40,dpi,96)&&h==MulDiv(30,dpi,96),"External skin menu loads rectangular image strips at window DPI");
 captureComponent(frame.GetSafeHwnd(),output/L"toolbar-skin.png");
 frame.command(CMD_TOOLBAR_SMALL);ImageList_GetIconSize(frame.toolbarNormal.GetSafeHandle(),&w,&h);
 expect(w==MulDiv(16,dpi,96)&&h==MulDiv(16,dpi,96),"Small size selects the small skin strip");
 p=frame.preferences();p["ToolbarSkin"]=utf8((output/L"missing.tbi").wstring());manager.setSettings(p);frame.applyAppearance();
 expect(!frame.toolbarSkinError.empty()&&frame.toolbarNormal.GetImageCount()==12,"Missing saved skin falls back to usable UDM icons");
 frame.command(CMD_TOOLBAR_BUILTIN);expect(str(frame.preferences(),"ToolbarSkin").empty()&&frame.toolbarSkinError.empty(),"Built-in icon command clears the failed skin preference");
 manager.setSettings(initial);frame.applyAppearance();frame.fillToolbarMenu(frame.toolbarMenu);
 expect(frame.toolbarMenu.GetMenuState(CMD_TOOLBAR,MF_BYCOMMAND)!=0xffffffff,"View toolbar menu retains Customize after rebuilding");
 auto skin=loadToolbarSkin(fixture);auto alpha=toolbarFrame(skin.images.at("hdpi"),0,12,60,45);auto at=(size_t)(20*60+20)*4;
 expect(alpha[at+3]==128&&alpha[at]<128&&alpha[at+1]<128&&alpha[at+2]<128&&alpha[3]==0,"High-DPI alpha images convert to premultiplied pixels with transparent edges");
 p=frame.preferences();p["DarkMode"]=true;manager.setSettings(p);frame.applyAppearance();captureComponent(frame.GetSafeHwnd(),output/L"toolbar-dark.png");
 CImage dark;dark.Load((output/L"toolbar-dark.png").c_str());bool arrowsVisible=true;auto toolbarBounds=bounds(frame.toolbar.GetSafeHwnd()),windowBounds=bounds(frame.GetSafeHwnd());
 for(int id:{CMD_STARTQUEUE,CMD_STOPQUEUE}){CRect r;frame.toolbar.GetRect(id,&r);int light=0,y=(r.top+r.bottom)/2+toolbarBounds.top-windowBounds.top;
  for(int x=r.right-16;x<r.right-3;++x)for(int dy=-3;dy<=3;++dy){auto color=dark.GetPixel(x+toolbarBounds.left-windowBounds.left,y+dy);if(GetRValue(color)>180&&GetGValue(color)>180&&GetBValue(color)>180)++light;}
  arrowsVisible=arrowsVisible&&light>=4;
 }
 expect(arrowsVisible,"Both dropdown arrows remain visible against the dark toolbar background");
 expect(frame.currentToolbarLayout()==defaultToolbarLayout(),"Dark styling retains the native command layout");
 manager.setSettings(initial);frame.applyAppearance();
 auto outer=bounds(frame.GetSafeHwnd());frame.SetWindowPos(nullptr,0,0,650,outer.Height(),SWP_NOMOVE|SWP_NOZORDER|SWP_NOACTIVATE);
 CRect narrowFirst,narrowLast;frame.toolbar.GetItemRect(0,&narrowFirst);frame.toolbar.GetItemRect(ToolbarCommandCount-1,&narrowLast);expect(narrowFirst.top==narrowLast.top&&narrowLast.bottom<=bounds(frame.toolbar.GetSafeHwnd()).Height()&&bounds(frame.table.GetSafeHwnd()).top>=bounds(frame.toolbar.GetSafeHwnd()).bottom,"Narrow windows retain one toolbar row above the download list");captureComponent(frame.GetSafeHwnd(),output/L"toolbar-narrow.png");
 frame.SetWindowPos(nullptr,0,0,outer.Width(),outer.Height(),SWP_NOMOVE|SWP_NOZORDER|SWP_NOACTIVATE);
  const float originalScale=frame.scale;details["layoutMatrix"]=Json::array();
 for(int dpi:{96,144,192})for(int fontHeight:{11,18})for(const char* style:{"Icons and text","Icons only","Text only"}){
  frame.scale=dpi/96.0f;p=initial;p["ToolbarStyle"]=style;p["FontHeight"]=fontHeight;manager.setSettings(p);frame.rebuildListImages();frame.applyAppearance();int previousHeight=-1;
  for(int logicalWidth:{600,778,1100}){frame.SetWindowPos(nullptr,0,0,frame.px(logicalWidth),frame.px(470),SWP_NOMOVE|SWP_NOZORDER|SWP_NOACTIVATE);CRect firstRect;frame.toolbar.GetItemRect(0,&firstRect);bool oneRow=true,insideHeight=true;const auto area=bounds(frame.toolbar.GetSafeHwnd());
   for(int i=0;i<frame.toolbar.GetButtonCount();++i){CRect r;frame.toolbar.GetItemRect(i,&r);oneRow&=r.top==firstRect.top;insideHeight&=r.bottom<=area.Height();}
   expect(oneRow&&insideHeight,"Every toolbar command stays on one unclipped vertical row across widths fonts styles and DPI");expect(previousHeight<0||previousHeight==area.Height(),"Changing window width does not change toolbar height");previousHeight=area.Height();auto table=bounds(frame.table.GetSafeHwnd());expect(table.top==area.bottom&&table.Height()>0,"Download list starts immediately below the single toolbar row");
   details["layoutMatrix"].push_back({{"dpi",dpi},{"fontHeight",fontHeight},{"style",style},{"width",logicalWidth},{"toolbarHeight",area.Height()},{"listHeight",table.Height()}});
   if(fontHeight==11&&std::string(style)=="Icons and text"&&logicalWidth==778)captureComponent(frame.GetSafeHwnd(),output/(L"toolbar-dpi-"+std::to_wstring(dpi)+L".png"));
  }
 }
 frame.scale=originalScale;manager.setSettings(initial);frame.rebuildListImages();frame.applyAppearance();frame.SetWindowPos(nullptr,0,0,outer.Width(),outer.Height(),SWP_NOMOVE|SWP_NOZORDER|SWP_NOACTIVATE);
captureComponent(frame.GetSafeHwnd(),output/L"toolbar-restored.png");
 }
};
class ToolbarTestApplication:public CWinApp{
 int resultCode=1;
public:BOOL InitInstance()override{
 CWinApp::InitInstance();INITCOMMONCONTROLSEX common{sizeof(common),ICC_WIN95_CLASSES|ICC_DATE_CLASSES};InitCommonControlsEx(&common);AfxOleInit();
 int argc=0;auto argv=CommandLineToArgvW(GetCommandLineW(),&argc);std::unique_ptr<Manager> manager;MainWindow* frame=nullptr;
 try{if(argc!=3)throw std::runtime_error("Pass an isolated output directory and fixture.tbi");output=fs::path(argv[1]);fixture=fs::path(argv[2]);fs::create_directories(output);
 manager=std::make_unique<Manager>(output/L"state");auto p=manager->state["Settings"];p["DownloadFolder"]=utf8((output/L"downloads").wstring());p["CloseToTray"]=false;p["ClipboardMonitor"]=false;manager->setSettings(p);
 frame=new MainWindow(*manager);m_pMainWnd=frame;ToolbarComponentTest::run(*manager,*frame);resultCode=0;
 }catch(const std::exception& e){componentError=e.what();}
 if(!output.empty())atomicText(output/L"before-cleanup.json",Json{{"passed",resultCode==0},{"error",componentError},{"checks",outcomes}}.dump(2),false);
 if(frame){frame->SendMessage(WM_CLOSE);m_pMainWnd=nullptr;}manager.reset();if(argv)LocalFree(argv);
 if(!output.empty())atomicText(output/L"results.json",Json{{"passed",resultCode==0},{"error",componentError},{"checks",outcomes},{"details",details}}.dump(2),false);
 return FALSE;
 }
 int ExitInstance()override{AfxOleTerm(FALSE);return resultCode;}
};
ToolbarTestApplication application;
}
