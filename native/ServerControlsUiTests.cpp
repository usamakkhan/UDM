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
static BOOL CALLBACK observe(HWND w,LPARAM){if(windowText(w)!=expectedTitle)return TRUE;try{ShowWindow(w,SW_SHOWNOACTIVATE);auto invoke=action;invoke(w);}catch(const std::exception& e){failure=e.what();PostMessageW(w,WM_CLOSE,0,0);}return FALSE;}
static void CALLBACK onTimer(HWND,UINT,UINT_PTR,DWORD){if(GetTickCount64()-began>12000){failure="Owned dialog test deadline";EnumThreadWindows(GetCurrentThreadId(),[](HWND w,LPARAM)->BOOL{PostMessageW(w,WM_CLOSE,0,0);return TRUE;},0);return;}EnumThreadWindows(GetCurrentThreadId(),observe,0);}
static void modal(Form& dialog,const wchar_t* caption,std::function<void(HWND)> callback){expectedTitle=caption;action=std::move(callback);began=GetTickCount64();auto timer=SetTimer(nullptr,0,80,onTimer);dialog.DoModal();KillTimer(nullptr,timer);action={};if(!failure.empty())throw std::runtime_error(failure);}

class ServerControlsUiApplication:public CWinApp {
 int code=1;
public:BOOL InitInstance()override{
 CWinApp::InitInstance();INITCOMMONCONTROLSEX common{sizeof(common),ICC_WIN95_CLASSES};InitCommonControlsEx(&common);AfxOleInit();int argc=0;auto argv=CommandLineToArgvW(GetCommandLineW(),&argc);
 try{if(argc!=2)throw std::runtime_error("Pass output directory");output=fs::path(argv[1]);fs::create_directories(output);Manager manager(output/L"state");auto before=manager.state["Settings"];Options options(manager,nullptr,{},[]{return std::vector<DialEntry>{};});
 modal(options,L"UDM Configuration",[&](HWND w){auto tab=children(w,L"SysTabControl32").at(0);TabCtrl_SetCurSel(tab,4);NMHDR notice{tab,(UINT_PTR)GetDlgCtrlID(tab),TCN_SELCHANGE};SendMessageW(w,WM_NOTIFY,notice.idFrom,(LPARAM)&notice);
 expect(!IsWindowEnabled(child(w,L"Edit"))&&!IsWindowEnabled(child(w,L"Delete")),"Empty exception list disables Edit and Delete");
 expectedTitle=L"Max. connections number for a server";action=[&](HWND e){auto combos=children(e,L"ComboBox");expect(combos.size()==2,"Connection editor exposes protocol and count choices");selectCombo(e,combos[0],2);SetWindowTextW(children(e,L"Edit").at(0),L"cdn*.example.test");selectCombo(e,combos[1],2);capture(e,L"options-rule.png");press(e,L"OK");};press(w,L"New");expectedTitle=L"not-open";
 expect(manager.state["Settings"]==before,"Rule editing remains a draft until Apply");press(w,L"Apply");auto rules=manager.state["Settings"]["ServerConnections"];expect(rules.size()==1&&str(rules[0],"Scheme")=="https"&&str(rules[0],"Host")=="cdn*.example.test"&&num(rules[0],"Connections")==4,"Apply persists protocol wildcard and count"); auto lists=children(w,L"SysListView32");HWND list=nullptr;for(auto candidate:lists)if(IsWindowVisible(candidate))list=candidate;expect(list!=nullptr,"Connection exception list is visible");
 expect(!IsWindowEnabled(child(w,L"Edit"))&&!IsWindowEnabled(child(w,L"Delete")),"Unselected populated list disables actions");
 ListView_SetItemState(list,0,LVIS_SELECTED|LVIS_FOCUSED,LVIS_SELECTED|LVIS_FOCUSED);expect(IsWindowEnabled(child(w,L"Edit"))&&IsWindowEnabled(child(w,L"Delete")),"Selecting exception enables Edit and Delete");
 ListView_SetItemState(list,0,0,LVIS_SELECTED);expect(!IsWindowEnabled(child(w,L"Edit"))&&!IsWindowEnabled(child(w,L"Delete")),"Clearing selection immediately disables actions");
 ListView_SetItemState(list,0,LVIS_SELECTED,LVIS_SELECTED);press(w,L"Delete");expect(ListView_GetItemCount(list)==0&&!IsWindowEnabled(child(w,L"Edit"))&&!IsWindowEnabled(child(w,L"Delete")),"Deleting last draft rule clears and disables actions");
 expect(manager.state["Settings"]["ServerConnections"].size()==1,"Deleting a draft does not change applied rules");capture(w,L"empty-exceptions.png");press(w,L"Cancel");});
 expect(manager.state["Settings"]["ServerConnections"].size()==1,"Cancel preserves the last applied exception");
 Json prefs=manager.state["Settings"];expectedTitle=L"Server connection limits";action=[&](HWND w){expectedTitle=L"New server limit";action=[&](HWND e){auto combos=children(e,L"ComboBox");selectCombo(e,combos[0],3);SetWindowTextW(children(e,L"Edit").at(0),L"files*.example.test");selectCombo(e,combos[1],0);capture(e,L"advanced-rule.png");press(e,L"OK");};press(w,L"New...");expectedTitle=L"not-open";press(w,L"OK");};began=GetTickCount64();auto timer=SetTimer(nullptr,0,80,onTimer);serverRulesDialog(nullptr,prefs);KillTimer(nullptr,timer);if(!failure.empty())throw std::runtime_error(failure);
 auto rules=prefs["ServerConnections"];expect(rules.size()==2&&str(rules[1],"Scheme")=="ftp"&&str(rules[1],"Host")=="files*.example.test"&&num(rules[1],"Connections")==1,"Additional server editor preserves scoped wildcard rule");manager.setSettings(prefs);{Manager reopened(manager.root);expect(reopened.state["Settings"]["ServerConnections"]==rules,"Rules from both editors survive restart");}code=0;
 }catch(const std::exception& e){failure=e.what();}if(argv)LocalFree(argv);if(!output.empty())atomicText(output/L"results.json",Json{{"passed",code==0},{"error",failure},{"checks",results}}.dump(2),false);return FALSE;}
 int ExitInstance()override{AfxOleTerm(FALSE);return code;}
};ServerControlsUiApplication application;
}