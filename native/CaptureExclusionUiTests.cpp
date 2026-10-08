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
static BOOL CALLBACK observe(HWND w,LPARAM){if(windowText(w)!=expectedTitle)return TRUE;if(expectedTitle==L"UDM Configuration"){bool ready=false;for(HWND c=GetWindow(w,GW_CHILD);c;c=GetWindow(c,GW_HWNDNEXT))ready|=windowText(c)==L"Apply";if(!ready)return TRUE;}try{action(w);}catch(const std::exception& e){failure=e.what();PostMessageW(w,WM_CLOSE,0,0);}return FALSE;}
static void CALLBACK onTimer(HWND,UINT,UINT_PTR,DWORD){if(GetTickCount64()-began>12000){failure="Owned dialog test deadline";EnumThreadWindows(GetCurrentThreadId(),[](HWND w,LPARAM)->BOOL{PostMessageW(w,WM_CLOSE,0,0);return TRUE;},0);return;}EnumThreadWindows(GetCurrentThreadId(),observe,0);}
static void modal(Form& dialog,const wchar_t* caption,std::function<void(HWND)> callback){expectedTitle=caption;action=std::move(callback);began=GetTickCount64();auto timer=SetTimer(nullptr,0,80,onTimer);dialog.DoModal();KillTimer(nullptr,timer);action={};if(!failure.empty())throw std::runtime_error(failure);}
static void runOwned(std::function<void()> run,const wchar_t* caption,std::function<void(HWND)> callback){expectedTitle=caption;action=std::move(callback);began=GetTickCount64();auto timer=SetTimer(nullptr,0,80,onTimer);run();KillTimer(nullptr,timer);action={};if(!failure.empty())throw std::runtime_error(failure);}
static void dialogColumnScalingChecks(){
 for(bool dialogUnits:{false,true}){
  Form form(dialogUnits?"Dialog-unit column scaling":"Pixel column scaling",400,200);form.dialogUnits=dialogUnits;form.modeless=true;CListCtrl* list=nullptr;
  form.init=[&]{list=form.make<CListCtrl>(WS_TABSTOP|WS_BORDER|LVS_REPORT,7,7,380,160);for(int i=0;i<4;++i)list->InsertColumn(i,L"Column",LVCFMT_LEFT,100);};
  expect(form.Create(100)!=FALSE,"Create isolated column scaling dialog");form.ShowWindow(SW_SHOWNOACTIVATE);
  auto setDpi=[&](UINT dpi){auto suggested=bounds(form.GetSafeHwnd());form.SendMessage(WM_DPICHANGED,MAKEWPARAM(dpi,dpi),(LPARAM)&suggested);};
  auto metric=[&](){if(!dialogUnits)return 1.0;auto dc=GetDC(list->GetSafeHwnd());auto units=DialogUnits::measure(dc,(HFONT)list->SendMessage(WM_GETFONT));ReleaseDC(list->GetSafeHwnd(),dc);return double(units.x);};
  setDpi(96);const double baselineMetric=metric();std::vector<int> original{101,143,0,203};for(int i=0;i<4;++i)list->SetColumnWidth(i,original[i]);
  for(int cycle=0;cycle<4;++cycle){for(UINT dpi:{144u,120u,192u,96u})setDpi(dpi);for(int i=0;i<4;++i)expect(list->GetColumnWidth(i)==original[i],dialogUnits?"Dialog-unit columns return exactly to their original widths":"Pixel-layout columns return exactly to their original widths");}
  setDpi(144);const double editedMetric=dialogUnits?metric():1.5;list->SetColumnWidth(0,157);list->SetColumnWidth(1,0);list->SetColumnWidth(2,79);
  setDpi(96);atomicText(output/(dialogUnits?L"dialog-column-metrics.json":L"pixel-column-metrics.json"),Json{{"baselineMetric",baselineMetric},{"editedMetric",editedMetric},{"actual",{list->GetColumnWidth(0),list->GetColumnWidth(1),list->GetColumnWidth(2)}},{"expected",{(int)std::lround(157*baselineMetric/editedMetric),0,(int)std::lround(79*baselineMetric/editedMetric)}}}.dump(2),false);expect(list->GetColumnWidth(0)==(int)std::lround(157*baselineMetric/editedMetric),"User-resized column is measured at the DPI where it changed");expect(list->GetColumnWidth(1)==0,"User-hidden column remains hidden after a DPI change");expect(list->GetColumnWidth(2)==(int)std::lround(79*baselineMetric/editedMetric),"User-revealed column keeps its new width across DPI changes");
  setDpi(144);expect(list->GetColumnWidth(0)==157&&list->GetColumnWidth(2)==79,"Returning to the edit DPI restores exact user widths");
  capture(form.GetSafeHwnd(),dialogUnits?L"column-scaling-dialog-units.png":L"column-scaling-pixels.png");form.DestroyWindow();
 }
}
static std::string captureToken(){auto value=guid();value.insert(20,"-");value.insert(16,"-");value.insert(12,"-");value.insert(8,"-");return std::to_string(epoch())+"-"+value;}
class CaptureExclusionTestApplication:public CWinApp {
 int code=1;
public:BOOL InitInstance()override{
 CWinApp::InitInstance();INITCOMMONCONTROLSEX common{sizeof(common),ICC_WIN95_CLASSES|ICC_DATE_CLASSES};InitCommonControlsEx(&common);AfxOleInit();int argc=0;auto argv=CommandLineToArgvW(GetCommandLineW(),&argc);
 try{
  if(argc!=2)throw std::runtime_error("Pass an isolated output directory");output=fs::path(argv[1]);fs::create_directories(output);Manager manager(output/L"state");auto prefs=manager.state["Settings"];prefs["DownloadFolder"]=utf8((output/L"files").wstring());prefs["CategoryFolders"]=false;prefs["PrefetchFileInfo"]=false;prefs["QueuePromptLater"]=false;prefs["DuplicatePolicy"]="Numbered";manager.setSettings(prefs);
  auto make=[&](const std::string& name){return manager.receive({{"action","add"},{"url","http://127.0.0.1:9/"+name+".zip?literal=*"},{"referrer","https://unrelated.example.test/page"},{"captureToken",captureToken()}});};
  auto a=make("first");int step=0;
  runOwned([&]{presentDownload(nullptr,manager,a,true);},L"Download File Info",[&](HWND w){++step;SendMessageW(w,WM_COMMAND,IDCANCEL,0);});
  expect(step==1&&str(a->data,"Status")=="Paused"&&!manager.isActive(a),"Escape cancels the first automatic File Info without starting a transfer");
  expect(manager.state["Settings"]==prefs,"First cancellation leaves exclusion settings unchanged");
  auto b=make("second");step=0;
  runOwned([&]{presentDownload(nullptr,manager,b,true);},L"Download File Info",[&](HWND w){
   if(step++==0){expectedTitle=L"Cancelling an automatically started download";press(w,L"&Cancel");return;}
   auto site=child(w,L"Don't automatically download from this site:"),address=child(w,L"Don't automatically download from this exact address:");
   expect(SendMessageW(site,BM_GETCHECK,0,0)==BST_UNCHECKED&&SendMessageW(address,BM_GETCHECK,0,0)==BST_CHECKED,"Offer defaults to the narrower exact address");
   auto fields=children(w,L"Edit");expect(windowText(fields[0])==L"127.0.0.1"&&windowText(fields[1]).find(L"/second.zip?literal=*")!=std::wstring::npos,"Offer shows the actual file host and full literal address");
   expect(bounds(child(w,L"OK")).bottom<bounds(child(w,L"Don't show this dialog again")).top,"Offer action buttons and suppression checkbox do not overlap");capture(w,L"cancel-offer.png");press(w,L"OK");
  });
  expect(step==2&&str(b->data,"Status")=="Paused"&&num(b->data,"Received")==0,"Second cancellation opens the offer after the download is stopped");
  expect(str(manager.state["Settings"],"CaptureExcludedUrls")=="=http://127.0.0.1:9/second.zip?literal=*"&&str(manager.state["Settings"],"CaptureExcludedHosts").empty(),"Actual offer OK saves only the reviewed exact address");
  auto c=make("third");runOwned([&]{presentDownload(nullptr,manager,c,true);},L"Download File Info",[&](HWND w){press(w,L"&Cancel");});
  auto before=manager.state["Settings"];auto d=make("fourth");step=0;
  runOwned([&]{presentDownload(nullptr,manager,d,true);},L"Download File Info",[&](HWND w){if(step++==0){expectedTitle=L"Cancelling an automatically started download";press(w,L"&Cancel");return;}press(w,L"Don't show this dialog again");press(w,L"Cancel");});
  expect(manager.state["Settings"]==before,"Canceling the offer discards checked suppression and exception drafts");
  auto e=make("fifth");runOwned([&]{presentDownload(nullptr,manager,e,true);},L"Download File Info",[&](HWND w){press(w,L"&Cancel");});
  auto f=make("sixth");step=0;
  runOwned([&]{presentDownload(nullptr,manager,f,true);},L"Download File Info",[&](HWND w){if(step++==0){expectedTitle=L"Cancelling an automatically started download";press(w,L"&Cancel");return;}press(w,L"Don't automatically download from this exact address:");press(w,L"Don't automatically download from this site:");press(w,L"Don't show this dialog again");press(w,L"OK");});
  expect(str(manager.state["Settings"],"CaptureExcludedHosts")=="127.0.0.1"&&!yes(manager.state["Settings"],"OfferCaptureExclusions",true),"Site-only offer and Don't show again persist from real native controls");
  // Exercise the actual File Types entry point, nested Add, Delete, and parent Cancel.
  auto saved=manager.state["Settings"];Options options(manager,nullptr);step=0;
  runOwned([&]{options.DoModal();},L"UDM Configuration",[&](HWND w){
   switch(step++){
    case 0:{auto tabs=children(w,L"SysTabControl32").at(0);SendMessageW(tabs,TCM_SETCURSEL,1,0);NMHDR notice{tabs,(UINT_PTR)GetDlgCtrlID(tabs),TCN_SELCHANGE};SendMessageW(w,WM_NOTIFY,notice.idFrom,(LPARAM)&notice);expect((GetWindowLongW(child(w,L"Edit list..."),GWL_STYLE)&WS_VISIBLE)!=0,"File Types exposes the address-exceptions editor");capture(w,L"file-types.png");expectedTitle=L"The list of address exceptions";press(w,L"Edit list...");break;}
    case 1:{auto list=children(w,L"SysListView32").at(0);expect(ListView_GetItemCount(list)==1,"File Types list displays the saved exact exclusion");expect(SendMessageW(child(w,L"Offer an exception after two cancelled automatic downloads"),BM_GETCHECK,0,0)==BST_UNCHECKED,"Exception editor shows the saved suppressed state");press(w,L"Offer an exception after two cancelled automatic downloads");ListView_SetItemState(list,0,LVIS_SELECTED,LVIS_SELECTED);press(w,L"Delete");expect(ListView_GetItemCount(list)==0,"Delete removes the selected exception from the draft");expectedTitle=L"Add an address to exceptions list";press(w,L"Add");break;}
    case 2:SetWindowTextW(children(w,L"Edit").at(0),L"https://downloads.example.test/private/*");capture(w,L"add-exception.png");expectedTitle=L"The list of address exceptions";press(w,L"OK");break;
    case 3:{auto list=children(w,L"SysListView32").at(0);expect(ListView_GetItemCount(list)==1,"Add inserts a reviewed wildcard pattern into the exception draft");capture(w,L"address-exceptions.png");expectedTitle=L"UDM Configuration";press(w,L"OK");break;}
    case 4:expect(manager.state["Settings"]==saved,"Nested exception OK remains a draft until parent Options applies");press(w,L"Cancel");break;
   }
  });
  expect(step==5&&manager.state["Settings"]==saved,"Parent Options Cancel rolls back added/deleted exceptions and prompt re-enable");
  Json draft=saved;AddressExceptionsDialog editor(nullptr,draft);runOwned([&]{editor.DoModal();},L"The list of address exceptions",[&](HWND w){press(w,L"Offer an exception after two cancelled automatic downloads");press(w,L"OK");});manager.setSettings(draft);expect(yes(manager.state["Settings"],"OfferCaptureExclusions"),"Saved exception editor can re-enable the repeated-cancel prompt");
  dialogColumnScalingChecks();
  atomicText(output/L"preferences.json",browserPreferences(manager.state["Settings"]).dump(2),false);manager.stop();code=0;
 }catch(const std::exception& e){failure=e.what();}
 if(argv)LocalFree(argv);if(!output.empty())atomicText(output/L"results.json",Json{{"passed",code==0},{"error",failure},{"checks",results}}.dump(2),false);return FALSE;
 }
 int ExitInstance()override{AfxOleTerm(FALSE);return code;}
};
CaptureExclusionTestApplication application;
}
