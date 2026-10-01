// Drives and renders only native windows owned by this isolated test executable.
#include <afxwin.h>
#include <afxcmn.h>
#include <afxdlgs.h>
#include <afxole.h>
#include <atlimage.h>
#include "WorkflowsUi.hpp"
#include "ExportUi.hpp"
namespace udm {
static Json results=Json::array();static fs::path output;static std::string failure;static std::wstring expectedTitle;static std::function<void(HWND)> action;static ULONGLONG began;
static void expect(bool value,const char* name){results.push_back({{"name",name},{"passed",value}});if(!value)throw std::runtime_error(name);}
static std::wstring windowText(HWND w){wchar_t text[512]{};GetWindowTextW(w,text,512);return text;}
static HWND child(HWND parent,const wchar_t* label){for(HWND c=GetWindow(parent,GW_CHILD);c;c=GetWindow(c,GW_HWNDNEXT))if(windowText(c)==label)return c;throw std::runtime_error("Missing owned test control");}
static std::vector<HWND> children(HWND parent,const wchar_t* type){std::vector<HWND> result;for(HWND c=GetWindow(parent,GW_CHILD);c;c=GetWindow(c,GW_HWNDNEXT)){wchar_t name[128]{};GetClassNameW(c,name,128);if(!_wcsicmp(name,type))result.push_back(c);}return result;}
static void press(HWND w,const wchar_t* label){auto c=child(w,label);expect(IsWindowEnabled(c)!=FALSE,"Requested native button is enabled");SendMessageW(c,BM_CLICK,0,0);}
static void selectCombo(HWND parent,HWND control,int index){SendMessageW(control,CB_SETCURSEL,index,0);SendMessageW(parent,WM_COMMAND,MAKEWPARAM(GetDlgCtrlID(control),CBN_SELCHANGE),(LPARAM)control);}
static CRect bounds(HWND w){CRect r;GetWindowRect(w,&r);return r;}
static void capture(HWND w,const wchar_t* name){
 ShowWindow(w,SW_SHOWNOACTIVATE);RedrawWindow(w,nullptr,nullptr,RDW_INVALIDATE|RDW_ALLCHILDREN|RDW_UPDATENOW|RDW_ERASE);auto r=bounds(w);CImage img;img.Create(r.Width(),r.Height(),32);auto dc=img.GetDC();RECT fill{0,0,r.Width(),r.Height()};FillRect(dc,&fill,GetSysColorBrush(COLOR_BTNFACE));SendMessageW(w,WM_PRINT,(WPARAM)dc,PRF_NONCLIENT|PRF_CLIENT|PRF_ERASEBKGND|PRF_CHILDREN);
 for(HWND c=GetWindow(w,GW_CHILD);c;c=GetWindow(c,GW_HWNDNEXT)){if(!(GetWindowLongW(c,GWL_STYLE)&WS_VISIBLE))continue;auto cr=bounds(c);auto saved=SaveDC(dc);IntersectClipRect(dc,cr.left-r.left,cr.top-r.top,cr.right-r.left,cr.bottom-r.top);SetViewportOrgEx(dc,cr.left-r.left,cr.top-r.top,nullptr);SendMessageW(c,WM_PRINT,(WPARAM)dc,PRF_NONCLIENT|PRF_CLIENT|PRF_ERASEBKGND|PRF_CHILDREN);RestoreDC(dc,saved);}
 img.ReleaseDC();if(FAILED(img.Save((output/name).c_str())))throw std::runtime_error("Could not render native test dialog");
}
static BOOL CALLBACK observe(HWND w,LPARAM){if(windowText(w)!=expectedTitle)return TRUE;try{action(w);}catch(const std::exception& e){failure=e.what();PostMessageW(w,WM_CLOSE,0,0);}return FALSE;}
static void CALLBACK onTimer(HWND,UINT,UINT_PTR,DWORD){if(GetTickCount64()-began>12000){failure="Owned dialog test deadline";EnumThreadWindows(GetCurrentThreadId(),[](HWND w,LPARAM)->BOOL{PostMessageW(w,WM_CLOSE,0,0);return TRUE;},0);return;}EnumThreadWindows(GetCurrentThreadId(),observe,0);}
static void modal(Form& dialog,const wchar_t* caption,std::function<void(HWND)> callback){expectedTitle=caption;action=std::move(callback);began=GetTickCount64();auto timer=SetTimer(nullptr,0,80,onTimer);dialog.DoModal();KillTimer(nullptr,timer);action={};if(!failure.empty())throw std::runtime_error(failure);}
class QueueExportTestApplication:public CWinApp {
 int code=1;
public:BOOL InitInstance()override{
 CWinApp::InitInstance();INITCOMMONCONTROLSEX common{sizeof(common),ICC_WIN95_CLASSES|ICC_DATE_CLASSES};InitCommonControlsEx(&common);AfxOleInit();int argc=0;auto argv=CommandLineToArgvW(GetCommandLineW(),&argc);
 try{
  if(argc!=2)throw std::runtime_error("Pass an isolated output directory");output=fs::path(argv[1]);fs::create_directories(output);Manager manager(output/L"state");auto prefs=manager.state["Settings"];prefs["DownloadFolder"]=utf8((output/L"files").wstring());prefs["CategoryFolders"]=false;manager.setSettings(prefs);auto q=defaultQueue();q["Enabled"]=false;manager.setQueue(q);auto other=defaultQueue("Other");other["Enabled"]=false;manager.setQueue(other);
  auto a=manager.add("https://example.invalid/a","","a.bin");auto b=manager.add("https://example.invalid/b","","b.bin","Other");auto done=manager.add("https://example.invalid/done","","done.bin");done->data["Status"]="Complete";done->data["QueueMember"]=false;auto removed=manager.add("https://example.invalid/removed","","removed.bin");removed->data["QueueMember"]=false;manager.save();
  int step=0;ExportDownloadsDialog exportDialog(manager,{b,done},"Main queue");modal(exportDialog,L"Export downloads",[&](HWND w){auto list=children(w,L"SysListView32").at(0);auto combos=children(w,L"ComboBox");
   switch(step++){
    case 0:expect(ListView_GetItemCount(list)==2&&SendMessageW(child(w,L"Selected downloads"),BM_GETCHECK,0,0)==BST_CHECKED,"Export opens with exactly the main-window selection");expect(!IsWindowEnabled(combos[0]),"Queue chooser is disabled outside queue scope");capture(w,L"export-selected.png");press(w,L"All downloads");break;
    case 1:expect(ListView_GetItemCount(list)==4,"All scope restores complete download history");ListView_SetCheckState(list,0,FALSE);press(w,L"Files in download queue");break;
    case 2:expect(ListView_GetItemCount(list)==1&&!ListView_GetCheckState(list,0),"Main queue scope excludes completed/removed members and retains the unchecked file");expect(IsWindowEnabled(combos[0])&&!IsWindowEnabled(child(w,L"Export...")),"Empty checked scope disables export while keeping queue selection usable");selectCombo(w,combos[0],2);break;
    case 3:expect(ListView_GetItemCount(list)==1&&ListView_GetCheckState(list,0),"Changing named queue shows its pending member");selectCombo(w,combos[0],0);break;
    case 4:expect(ListView_GetItemCount(list)==2,"All queues scope combines only pending queue members");press(w,L"Check all");expect(ListView_GetCheckState(list,0)&&ListView_GetCheckState(list,1),"Check all affects the displayed scope");capture(w,L"export-queues.png");selectCombo(w,combos[1],0);break;
    case 5:expect(!IsWindowEnabled(child(w,L"Include Windows-account-encrypted request credentials")),"URL-list format disables the catalog credential option");press(w,L"Uncheck all");expect(!IsWindowEnabled(child(w,L"Export...")),"Uncheck all disables empty export");press(w,L"Cancel");break;
   }
  });
  expect(step==6,"All export scope interactions completed");expect(manager.jobs.size()==4&&str(a->data,"Status")=="Paused","Canceling export preserves download records");
  step=0;Scheduler scheduler(manager,nullptr);modal(scheduler,L"Scheduler",[&](HWND w){
   switch(step++){
    case 0:{auto startup=child(w,L"Start download on UDM startup"),enabled=child(w,L"Queue enabled");expect(SendMessageW(startup,BM_GETCHECK,0,0)==BST_UNCHECKED,"Scheduler exposes an initially disabled startup preference");expect(bounds(enabled).right<=bounds(startup).left,"Startup checkbox does not overlap queue enablement");press(w,L"Start download on UDM startup");capture(w,L"scheduler-startup.png");press(w,L"Apply");break;}
    case 1:expect(yes(manager.state["Queues"][0],"StartOnStartup")&&str(a->data,"Status")=="Paused","Apply persists startup preference without starting a current download");press(w,L"Stop");break;
    case 2:expect(yes(manager.state["Queues"][0],"StartOnStartup")&&!yes(manager.state["Queues"][0],"Enabled"),"Native Stop retains the startup preference");press(w,L"Start download on UDM startup");press(w,L"Cancel");break;
   }
  });
  expect(yes(manager.state["Queues"][0],"StartOnStartup"),"Cancel discards the unchecked startup draft");step=0;Scheduler reopened(manager,nullptr);modal(reopened,L"Scheduler",[&](HWND w){expect(SendMessageW(child(w,L"Start download on UDM startup"),BM_GETCHECK,0,0)==BST_CHECKED,"Reopening Scheduler restores the saved startup choice");++step;press(w,L"Cancel");});expect(step==1,"Scheduler closes normally after restoring preferences");code=0;
 }catch(const std::exception& e){failure=e.what();}
 if(argv)LocalFree(argv);if(!output.empty())atomicText(output/L"results.json",Json{{"passed",code==0},{"error",failure},{"checks",results}}.dump(2),false);return FALSE;
 }
 int ExitInstance()override{AfxOleTerm(FALSE);return code;}
};
QueueExportTestApplication application;
}
