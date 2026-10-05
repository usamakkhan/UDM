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

class DarkGroupsApplication:public CWinApp {
 int code=1;
public:BOOL InitInstance()override{
 CWinApp::InitInstance();INITCOMMONCONTROLSEX common{sizeof(common),ICC_WIN95_CLASSES};InitCommonControlsEx(&common);AfxOleInit();int argc=0;auto argv=CommandLineToArgvW(GetCommandLineW(),&argc);
 try{if(argc!=2)throw std::runtime_error("Pass isolated output directory");output=fs::path(argv[1]);fs::create_directories(output);
 for(bool dark:{true,false})for(UINT dpi:{96u,144u,192u}){
 uiDark=dark;Form dialog("Group contrast fixture",280,120);dialog.dialogUnits=true;CWnd* group=nullptr;
 dialog.init=[&]{group=dialog.control(L"BUTTON","Readable group caption",BS_GROUPBOX,10,10,255,85);dialog.button("Close",215,100,50,[&]{dialog.close();});};
 modal(dialog,L"Group contrast fixture",[&](HWND w){RECT window{};GetWindowRect(w,&window);SendMessageW(w,WM_DPICHANGED,MAKELONG(dpi,dpi),(LPARAM)&window);auto area=bounds(group->GetSafeHwnd());CImage pixels;pixels.Create(area.Width(),area.Height(),32);auto dc=pixels.GetDC();RECT fill{0,0,area.Width(),area.Height()};FillRect(dc,&fill,(HBRUSH)dialogBrush().GetSafeHandle());auto old=SelectObject(dc,(HFONT)SendMessageW(group->GetSafeHwnd(),WM_GETFONT,0,0));SIZE extent{};GetTextExtentPoint32W(dc,L"Readable group caption",22,&extent);SendMessageW(group->GetSafeHwnd(),WM_PRINT,(WPARAM)dc,PRF_CLIENT|PRF_ERASEBKGND);int contrasting=0;for(int y=0;y<std::min<int>(extent.cy+2,area.Height());++y)for(int x=MulDiv(12,dpi,96);x<std::min<int>(extent.cx,area.Width());++x){auto c=GetPixel(dc,x,y);if(c==CLR_INVALID)continue;int brightness=(GetRValue(c)+GetGValue(c)+GetBValue(c))/3;if(dark?brightness>170:brightness<100)++contrasting;}SelectObject(dc,old);pixels.ReleaseDC();auto name=std::string(dark?"dark-":"light-")+std::to_string(dpi);pixels.Save((output/wide(name+"-group.png")).c_str());capture(w,wide(name+"-dialog.png").c_str());results.push_back({{"name",name+" caption contrast"},{"passed",contrasting>20},{"contrastingPixels",contrasting}});expect(contrasting>20,"Group caption has visible contrasting glyphs");press(w,L"Close");});}
 code=0;}catch(const std::exception& e){failure=e.what();}if(argv)LocalFree(argv);atomicText(output/L"results.json",Json{{"passed",code==0},{"error",failure},{"checks",results}}.dump(2),false);return FALSE;}
 int ExitInstance()override{AfxOleTerm(FALSE);return code;}
};DarkGroupsApplication application;
}