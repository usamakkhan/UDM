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

static void rejectPartialCount(HWND editor,HWND count,const wchar_t* accept=L"OK"){
 const auto priorTitle=expectedTitle;const auto priorAction=action;bool warned=false;
 SetWindowTextW(count,L"8abc");expectedTitle=L"UDM";action=[&](HWND warning){warned=true;press(warning,L"OK");};
 press(editor,accept);expectedTitle=priorTitle;action=priorAction;
 expect(warned&&IsWindow(editor),"Invalid count shows warning and keeps server editor open for correction");
}


static HWND editAfterLabel(HWND w,const wchar_t* label){
 for(HWND c=GetWindow(w,GW_CHILD);c;c=GetWindow(c,GW_HWNDNEXT))if(windowText(c)==label){auto edit=GetWindow(c,GW_HWNDNEXT);wchar_t type[64]{};GetClassNameW(edit,type,64);if(!_wcsicmp(type,L"Edit"))return edit;}
 throw std::runtime_error("Missing scheduler edit after label: "+utf8(label));
}
class SchedulerNumberUiApplication:public CWinApp {
 int code=1;
public:BOOL InitInstance()override{
 CWinApp::InitInstance();INITCOMMONCONTROLSEX common{sizeof(common),ICC_WIN95_CLASSES|ICC_DATE_CLASSES};InitCommonControlsEx(&common);AfxOleInit();int argc=0;auto argv=CommandLineToArgvW(GetCommandLineW(),&argc);
 try{if(argc!=2||fs::exists(argv[1]))throw std::runtime_error("Fresh fixture required");output=argv[1];fs::create_directories(output);Manager manager(output/L"state");Scheduler dialog(manager,nullptr);
 modal(dialog,L"Scheduler",[&](HWND w){
  struct Field{const wchar_t* label;const char* key;const wchar_t* good;};
  for(const auto& field:std::vector<Field>{{L"Download","Parallel",L"4"},{L"Connection retries (blank = Options)","Retries",L"3"},{L"Retry a failed file","FileRetries",L"2"},{L"Wait between file retries (seconds)","RetryDelaySeconds",L"45"},{L"Countdown (seconds)","FinishDelaySeconds",L"60"}}){
   auto editor=editAfterLabel(w,field.label);auto before=manager.state["Queues"];
   rejectPartialCount(w,editor,L"Apply");
   expect(manager.state["Queues"]==before,"Malformed scheduler field leaves every applied queue unchanged");
   SetWindowTextW(editor,field.good);press(w,L"Apply");
   expect(num(mainQueue(manager),field.key)==std::stoll(field.good),"Corrected scheduler number applies exactly");
  }
  SetWindowTextW(editAfterLabel(w,L"Connection retries (blank = Options)"),L"");press(w,L"Apply");
  expect(mainQueue(manager).at("Retries").is_null(),"Blank scheduler retries retain Options default");
  press(w,L"Close");
 });
 {Manager reopened(manager.root);auto q=mainQueue(reopened);expect(num(q,"Parallel")==4&&q.at("Retries").is_null()&&num(q,"FileRetries")==2&&num(q,"RetryDelaySeconds")==45&&num(q,"FinishDelaySeconds")==60,"Corrected scheduler settings survive catalog restart");}
 code=0;
 }catch(const std::exception& e){failure=e.what();}if(argv)LocalFree(argv);if(!output.empty())atomicText(output/L"results.json",Json{{"passed",code==0},{"error",failure},{"checks",results}}.dump(2),false);return FALSE;}
 int ExitInstance()override{AfxOleTerm(FALSE);return code;}
};SchedulerNumberUiApplication application;
}
