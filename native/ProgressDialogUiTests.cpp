// Inspects only windows created by this isolated fixture.
#define UDM_TOOLBAR_COMPONENT_TEST
#include "App.cpp"
#include <oleacc.h>
namespace udm {
static Json checks=Json::array();
static void expect(bool ok,const char* name){checks.push_back({{"name",name},{"passed",ok}});if(!ok)throw std::runtime_error(name);}
static std::wstring caption(HWND w){wchar_t value[1024]{};GetWindowTextW(w,value,1024);return value;}
static HWND named(HWND w,const wchar_t* text){for(auto c=GetWindow(w,GW_CHILD);c;c=GetWindow(c,GW_HWNDNEXT))if(caption(c)==text)return c;throw std::runtime_error("Missing progress fixture control");}
static HWND visibleNamed(HWND w,const wchar_t* text){for(auto c=GetWindow(w,GW_CHILD);c;c=GetWindow(c,GW_HWNDNEXT))if(IsWindowVisible(c)&&caption(c)==text)return c;throw std::runtime_error("Missing visible fixture control");}
static void pumpUntil(const std::function<bool()>& done){const auto deadline=GetTickCount64()+1500;while(!done()&&GetTickCount64()<deadline){MSG message;while(PeekMessageW(&message,nullptr,0,0,PM_REMOVE)){TranslateMessage(&message);DispatchMessageW(&message);}Sleep(5);}}
static CRect rectangle(HWND w,HWND parent=nullptr){CRect r;GetWindowRect(w,&r);if(parent)MapWindowPoints(nullptr,parent,(POINT*)&r,2);return r;}
static void render(HWND w,const fs::path& path){ShowWindow(w,SW_SHOWNOACTIVATE);const auto settled=GetTickCount64()+500;pumpUntil([&]{return GetTickCount64()>=settled;});RedrawWindow(w,nullptr,nullptr,RDW_INVALIDATE|RDW_ALLCHILDREN|RDW_UPDATENOW|RDW_ERASE);auto r=rectangle(w);CImage image;image.Create(r.Width(),r.Height(),32);auto dc=image.GetDC();RECT fill{0,0,r.Width(),r.Height()};FillRect(dc,&fill,GetSysColorBrush(COLOR_BTNFACE));SendMessageW(w,WM_PRINT,(WPARAM)dc,PRF_NONCLIENT|PRF_CLIENT|PRF_ERASEBKGND|PRF_CHILDREN);image.ReleaseDC();if(FAILED(image.Save(path.c_str())))throw std::runtime_error("Could not render fixture");}
class ProgressDialogApplication:public CWinApp {
 int result=1;
public:BOOL InitInstance()override{
 CWinApp::InitInstance();INITCOMMONCONTROLSEX common{sizeof(common),ICC_WIN95_CLASSES};InitCommonControlsEx(&common);AfxOleInit();int argc=0;auto argv=CommandLineToArgvW(GetCommandLineW(),&argc);fs::path output;std::string error;
 try{
  if(argc!=3&&argc!=4)throw std::runtime_error("Pass fresh output, reference-dialogs.json and optional loopback POST fixture URL");output=fs::absolute(argv[1]);if(fs::exists(output))throw std::runtime_error("Fresh output required");fs::create_directories(output);
  auto reference=Json::parse(readText(argv[2]));Json statusReference;for(auto& d:reference["dialogs"])if(num(d,"id")==363)statusReference=d;
  expect(!statusReference.is_null(),"Reference contains IDM download-status dialog 363");
  Manager manager(output/L"state");auto prefs=manager.state["Settings"];prefs["DownloadFolder"]=utf8((output/L"files").wstring());prefs["CategoryFolders"]=false;prefs["PrefetchFileInfo"]=false;prefs["ProgressStartMode"]="Normal size";manager.setSettings(prefs);
  auto job=manager.add("https://example.invalid/fixture.bin","","fixture.bin","Main queue",true);job->data["Size"]=1000;job->data["Received"]=375;job->data["RangeSupported"]=true;
  CFrameWnd owner;expect(owner.Create(nullptr,L"Isolated progress dialog fixture",WS_OVERLAPPEDWINDOW,CRect(0,0,40,40))!=FALSE,"Create isolated owner");m_pMainWnd=&owner;
  Progress progress(manager,job,&owner);expect(progress.Create(101,&owner)!=FALSE,"Create actual progress dialog");auto w=progress.GetSafeHwnd();progress.ShowWindow(SW_SHOWNOACTIVATE);
  auto url=named(w,L"https://example.invalid/fixture.bin");wchar_t kind[80]{};GetClassNameW(url,kind,80);
  expect(!_wcsicmp(kind,L"Static"),"Progress URL uses the reference Static control class");
  expect((GetWindowLongW(url,GWL_STYLE)&SS_TYPEMASK)==SS_LEFTNOWORDWRAP,"Progress URL uses reference single-line static style");
  expect(!(GetWindowLongW(url,GWL_EXSTYLE)&WS_EX_CLIENTEDGE),"Progress URL has no edit-box edge");
  expect((GetWindowLongW(url,GWL_STYLE)&SS_NOPREFIX)!=0,"URL ampersands are displayed literally rather than as keyboard mnemonics");
  HWND progressBar=nullptr;for(auto c=GetWindow(w,GW_CHILD);c;c=GetWindow(c,GW_HWNDNEXT)){GetClassNameW(c,kind,80);if(!_wcsicmp(kind,PROGRESS_CLASSW))progressBar=c;}
  expect(progressBar!=nullptr,"Progress dialog exposes its native progress bar");
  PBRANGE range{};SendMessageW(progressBar,PBM_GETRANGE,FALSE,(LPARAM)&range);
  expect(range.iLow==0&&range.iHigh==1000&&SendMessageW(progressBar,PBM_GETPOS,0,0)==375,"Progress bar position agrees with the downloaded byte percentage");
  expect(caption(named(w,L"375 Bytes  ( 37.50 % )"))==L"375 Bytes  ( 37.50 % )","Downloaded value includes exact percentage");
  job->data["Size"]=10000;job->data["Received"]=1369;SendMessageW(w,WM_TIMER,1,0);
  expect(caption(named(w,L"1.337 KB  ( 13.69 % )"))==L"1.337 KB  ( 13.69 % )","Downloaded percentage retains reference hundredth precision");
  expect(SendMessageW(progressBar,PBM_GETPOS,0,0)==136,"Progress bar retains its integer range while the label preserves hundredths");
  job->data["Size"]=1000;job->data["Received"]=375;SendMessageW(w,WM_TIMER,1,0);
  expect(caption(named(w,L"Yes"))==L"Yes","Resume capability reflects range support");
  job->data.erase("RangeSupported");SendMessageW(w,WM_TIMER,1,0);expect(caption(named(w,L"Unknown"))==L"Unknown","Unprobed download displays unknown resume support");
  job->data["OriginalResponse"]={{"State","Receiving"}};SendMessageW(w,WM_TIMER,1,0);expect(caption(named(w,L"Unknown"))==L"Unknown","Captured browser response does not falsely report unsupported resume");job->data.erase("OriginalResponse");
  job->data["RangeSupported"]=false;SendMessageW(w,WM_TIMER,1,0);expect(caption(named(w,L"No"))==L"No","Confirmed unsupported server displays No");
  job->data["RangeSupported"]=true;SendMessageW(w,WM_TIMER,1,0);
  {auto before=job->data;job->data["Status"]="Pausing";SendMessageW(w,WM_TIMER,1,0);
   expect(!IsWindowEnabled(named(w,L"Start")),"Pausing disables the primary progress action");
   SendMessageW(w,WM_COMMAND,IDOK,0);
   expect(str(job->data,"Status")=="Pausing","Enter cannot restart a download while its primary action is disabled");job->data=before;SendMessageW(w,WM_TIMER,1,0);}
  {auto before=job->data;job->data["Status"]="Pausing";
   SendMessageW(w,WM_COMMAND,IDOK,0);expect(str(job->data,"Status")=="Pausing","Primary action rechecks state before the next display timer");job->data=before;
   job->data["Status"]="Downloading";job->data["LiveRecording"]=true;job->data["LiveSaveReady"]=false;
   expect(!progressPrimaryEnabled(job->data,true),"Unready active live recording disables the shared primary action");
   job->data["LiveSaveReady"]=true;expect(progressPrimaryEnabled(job->data,true),"Ready live recording permits Stop and save");
   job->data["LiveStopRequested"]=true;expect(!progressPrimaryEnabled(job->data,true),"Live recording already saving rejects a second primary action");
   job->data=before;job->data["DuplicateOf"]="fixture-duplicate";expect(!progressPrimaryEnabled(job->data,false),"Unresolved duplicate cannot activate the primary action");job->data=before;SendMessageW(w,WM_TIMER,1,0);}
  job->data["ProtectedRequest"]=protect("{}");job->data["PostAttempted"]=false;SendMessageW(w,WM_TIMER,1,0);expect(caption(named(w,L"Start"))==L"Start","Unsubmitted form retains the Start action");
  job->data["PostAttempted"]=true;SendMessageW(w,WM_TIMER,1,0);expect(caption(named(w,L"Restart"))==L"Restart","Interrupted form advertises Restart rather than byte resume");expect(caption(named(w,L"Paused - Restart resubmits this form."))==L"Paused - Restart resubmits this form.","Paused form explains that restarting submits the request again");expect(caption(named(w,L"No"))==L"No","Form request cannot claim byte resume even with a stale range flag");render(w,output/L"form-restart.png");
  job->data["Status"]="Failed";job->data["Error"]="Fixture server error";SendMessageW(w,WM_TIMER,1,0);expect(caption(named(w,L"Failed - Fixture server error"))==L"Failed - Fixture server error","Existing transfer error takes precedence over the form hint");
  job->data["Status"]="Paused";job->data["Error"]="";job->data["ProtectedPostGetUrl"]=protect("https://example.invalid/result.bin");SendMessageW(w,WM_TIMER,1,0);expect(caption(named(w,L"Start"))==L"Start"&&!progressNeedsFormRestart(job->data),"Saved POST-to-GET continuation does not claim it will resubmit the form");
  job->data.erase("ProtectedRequest");job->data.erase("PostAttempted");job->data.erase("ProtectedPostGetUrl");SendMessageW(w,WM_TIMER,1,0);
  expect(progressResumeCapability(Json{{"Video",{{"RangeSupported",true}}},{"Audio",Json::object()}})=="Unknown","Untested audio track keeps combined resume capability unknown");
  expect(progressResumeCapability(Json{{"Video",Json::object()},{"Audio",{{"RangeSupported",false}}}})=="No","Confirmed unsupported track takes precedence over an unknown track");
  expect(progressResumeCapability(Json{{"Video",{{"RangeSupported",true}}},{"Audio",{{"RangeSupported",true}}}})=="Yes","Both verified tracks display Yes");
  expect(progressResumeCapability(Json{{"MediaTracksReady",true}})=="Streams saved"&&progressResumeCapability(Json{{"AdaptiveTotalSegments",3}})=="Yes (saved segments)","Saved media retains its distinct resume capability");
  for(UINT dpi:{96u,144u,192u}){
   auto suggested=rectangle(w);SendMessageW(w,WM_DPICHANGED,MAKEWPARAM(dpi,dpi),(LPARAM)&suggested);
   CFont font;font.CreateFontW(-MulDiv(8,dpi,72),0,0,0,FW_NORMAL,FALSE,FALSE,FALSE,DEFAULT_CHARSET,OUT_DEFAULT_PRECIS,CLIP_DEFAULT_PRECIS,DEFAULT_QUALITY,DEFAULT_PITCH,L"Tahoma");auto dc=GetDC(w);auto units=DialogUnits::measure(dc,(HFONT)font.GetSafeHandle());ReleaseDC(w,dc);
   LOGFONTW actual{};GetObjectW((HFONT)SendMessageW(url,WM_GETFONT,0,0),sizeof(actual),&actual);expect(actual.lfHeight==-MulDiv(8,dpi,72)&&!wcscmp(actual.lfFaceName,L"Tahoma"),"Progress status uses reference Tahoma point size at tested DPI");
   for(auto& ref:statusReference["controls"]){const auto expected=CRect(units.rect((int)ref["x"]+4,(int)ref["y"]+20,(int)ref["width"],(int)ref["height"]));bool matched=false;
    for(auto c=GetWindow(w,GW_CHILD);c;c=GetWindow(c,GW_HWNDNEXT)){GetClassNameW(c,kind,80);if(!_wcsicmp(kind,L"Static")&&rectangle(c,w)==expected){auto text=str(ref,"title");if(text=="FileUrl"||text=="Connecting..."||text=="(unknown)"||text=="0 Bytes"||text.empty()||caption(c)==wide(text))matched=true;}}
    expect(matched,"Progress status reference control class, label and geometry match");
   }
   CRect expandedClient;GetClientRect(w,&expandedClient);auto details=named(w,L"<< Hide details");SendMessageW(details,BM_CLICK,0,0);CRect client;GetClientRect(w,&client);
   atomicText(output/wide("resize-"+std::to_string(dpi)+".json"),Json{{"simulatedDpi",dpi},{"actualWindowDpi",GetDpiForWindow(w)},{"expandedClientHeight",expandedClient.Height()},{"compactClientHeight",client.Height()},{"expectedExpandedClient",units.rect(0,0,353,250).bottom},{"expectedCompactClient",units.rect(0,0,353,148).bottom},{"button",utf8(caption(details))}}.dump(2),false);
   expect(caption(details)==L"Show details >>","Hide details updates the native button caption");
   // Synthetic WM_DPICHANGED changes our layout but not Windows' real nonclient DPI.
   expect(expandedClient.Height()-client.Height()==units.rect(0,0,353,250).bottom-units.rect(0,0,353,148).bottom,"Hide details removes the expected connection-area height at tested DPI");
   expect(!IsWindowVisible(named(w,L"Start positions and download progress by connections")),"Hide details hides connection caption");SendMessageW(named(w,L"Show details >>"),BM_CLICK,0,0);GetClientRect(w,&client);expect(client.Height()==expandedClient.Height(),"Show details restores the previous expanded client height");
   render(w,output/wide("progress-"+std::to_string(dpi)+".png"));
  }
  HWND tab=nullptr;for(auto c=GetWindow(w,GW_CHILD);c;c=GetWindow(c,GW_HWNDNEXT)){GetClassNameW(c,kind,80);if(!_wcsicmp(kind,L"SysTabControl32"))tab=c;}expect(tab!=nullptr,"Progress window exposes its native tab control");
  auto select=[&](int index){SendMessageW(tab,TCM_SETCURSEL,index,0);NMHDR notice{tab,(UINT_PTR)GetDlgCtrlID(tab),TCN_SELCHANGE};SendMessageW(w,WM_NOTIFY,notice.idFrom,(LPARAM)&notice);};
  for(UINT dpi:{96u,144u,192u}){
   auto suggested=rectangle(w);SendMessageW(w,WM_DPICHANGED,MAKEWPARAM(dpi,dpi),(LPARAM)&suggested);
   CFont font;font.CreateFontW(-MulDiv(8,dpi,72),0,0,0,FW_NORMAL,FALSE,FALSE,FALSE,DEFAULT_CHARSET,OUT_DEFAULT_PRECIS,CLIP_DEFAULT_PRECIS,DEFAULT_QUALITY,DEFAULT_PITCH,L"Tahoma");auto dc=GetDC(w);auto units=DialogUnits::measure(dc,(HFONT)font.GetSafeHandle());ReleaseDC(w,dc);
   for(auto pair:{std::pair<int,int>{1,365},{2,364}}){select(pair.first);Json panel;for(auto& d:reference["dialogs"])if(num(d,"id")==pair.second)panel=d;expect(!panel.is_null(),"Reference contains the selected progress options panel");
    for(auto& ref:panel["controls"]){auto target=CRect(units.rect((int)ref["x"]+4,(int)ref["y"]+20,(int)ref["width"],(int)ref["height"]));int role=ref["class"]["ordinal"];bool matched=false;auto text=str(ref,"title");if(text=="Exit Internet Download Manager when done")text="Exit UDM when done";
     for(auto c=GetWindow(w,GW_CHILD);c;c=GetWindow(c,GW_HWNDNEXT)){if(!IsWindowVisible(c))continue;GetClassNameW(c,kind,80);const auto actual=rectangle(c,w);const wchar_t* expectedClass=role==128?L"Button":role==129?L"Edit":role==133?L"ComboBox":L"Static";
      if(_wcsicmp(kind,expectedClass)||actual.left!=target.left||actual.top!=target.top||actual.Width()!=target.Width()||(role!=133&&actual.Height()!=target.Height()))continue;
      if((text.empty()&&(role!=128||caption(c).empty()))||caption(c)==wide(text)||(text=="Save To:"&&caption(c).rfind(L"Save To: ",0)==0))matched=true;
     }
     if(!matched)atomicText(output/L"options-geometry-failure.json",Json{{"dpi",dpi},{"dialog",pair.second},{"reference",ref}}.dump(2),false);
     expect(matched,"Visible progress options control matches reference class, label and geometry");
    }
    render(w,output/wide("progress-panel-"+std::to_string(pair.second)+"-"+std::to_string(dpi)+".png"));
   }
  }
  select(1);expect(!IsWindowVisible(url)&&IsWindowVisible(named(w,L"Use Speed Limiter")),"Speed tab hides URL and displays limiter controls");
  auto enabled=named(w,L"Use Speed Limiter"),remember=named(w,L"Remember Speed Limiter settings for this file\r\non download stop/resume");HWND rate=nullptr;for(auto c=GetWindow(w,GW_CHILD);c;c=GetWindow(c,GW_HWNDNEXT)){GetClassNameW(c,kind,80);if(!_wcsicmp(kind,L"Edit")&&IsWindowVisible(c))rate=c;}
  expect(rate&&!IsWindowEnabled(rate),"Disabled limiter keeps its rate field disabled");SendMessageW(enabled,BM_CLICK,0,0);expect(IsWindowEnabled(rate)&&job->sessionLimit==1000,"Enabling limiter applies its default temporary rate");
  SetWindowTextW(rate,L"256");expect(job->sessionLimit==256&&num(job->data,"LimitKbps")==0,"Editing temporary limiter does not change remembered rate");
  SendMessageW(remember,BM_CLICK,0,0);expect(!job->sessionLimit&&num(job->data,"LimitKbps")==256,"Remember checkbox persists the entered rate");
  {Manager reopened(manager.root);expect(num(reopened.jobs.at(0)->data,"LimitKbps")==256,"Remembered GUI rate survives catalog reopen");}
  SetWindowTextW(rate,L"0");expect(num(job->data,"LimitKbps")==256&&caption(named(w,L"1..1000000"))==L"1..1000000","Invalid GUI rate preserves the saved setting and displays its valid range");
  SetWindowTextW(rate,L"128");SendMessageW(enabled,BM_CLICK,0,0);expect(!IsWindowEnabled(rate)&&!job->sessionLimit&&num(job->data,"LimitKbps")==0,"Disabling remembered limiter saves zero and disables the rate field");
  select(2);expect(!IsWindowVisible(url)&&IsWindowVisible(named(w,L"Show download complete dialog")),"Completion tab hides URL and displays completion controls");
  auto accessibility=LoadLibraryExW(L"oleacc.dll",nullptr,LOAD_LIBRARY_SEARCH_SYSTEM32);expect(accessibility!=nullptr,"Load Windows accessibility provider");
  using AccessibleObject=HRESULT(WINAPI*)(HWND,DWORD,REFIID,void**);auto accessibleObject=reinterpret_cast<AccessibleObject>(GetProcAddress(accessibility,"AccessibleObjectFromWindow"));expect(accessibleObject!=nullptr,"Windows exposes native accessibility inspection");
  std::map<std::string,HWND> glyphs;
  for(auto c=GetWindow(w,GW_CHILD);c;c=GetWindow(c,GW_HWNDNEXT)){GetClassNameW(c,kind,80);if(_wcsicmp(kind,L"Button")||!IsWindowVisible(c)||!caption(c).empty())continue;IAccessible* object=nullptr;auto hr=accessibleObject(c,OBJID_CLIENT,__uuidof(IAccessible),(void**)&object);expect(SUCCEEDED(hr)&&object,"Blank checkbox exposes native accessibility object");VARIANT self;VariantInit(&self);self.vt=VT_I4;self.lVal=CHILDID_SELF;BSTR name=nullptr;VARIANT role;VariantInit(&role);auto nameResult=object->get_accName(self,&name);auto roleResult=object->get_accRole(self,&role);std::string label=name?utf8(name):"";SysFreeString(name);object->Release();bool checkbox=SUCCEEDED(roleResult)&&role.vt==VT_I4&&role.lVal==ROLE_SYSTEM_CHECKBUTTON;VariantClear(&role);expect(SUCCEEDED(nameResult)&&!label.empty()&&checkbox,"Glyph retains its meaningful accessible name and checkbox role");glyphs[label]=c;}
  expect(glyphs.size()==4&&glyphs.count("Hang up modem when done")&&glyphs.count("Exit UDM when done")&&glyphs.count("Turn off computer when done")&&glyphs.count("Force processes to terminate"),"All four blank glyphs expose their distinct completion-option names");
  auto exitLabel=named(w,L"Exit UDM when done");SendMessageW(w,WM_COMMAND,MAKEWPARAM(GetDlgCtrlID(exitLabel),STN_CLICKED),(LPARAM)exitLabel);auto steps=completionSteps(job->data);expect(SendMessageW(glyphs.at("Exit UDM when done"),BM_GETCHECK,0,0)==BST_CHECKED&&std::find(steps.begin(),steps.end(),"Exit UDM")!=steps.end(),"Clicking the separate exit label checks the glyph and saves its plan without executing it");
  SendMessageW(glyphs.at("Exit UDM when done"),BM_CLICK,0,0);steps=completionSteps(job->data);expect(std::find(steps.begin(),steps.end(),"Exit UDM")==steps.end(),"Clicking the glyph clears the completion plan");
  auto exitGlyph=glyphs.at("Exit UDM when done");IAccessible* exitAccessible=nullptr;expect(SUCCEEDED(accessibleObject(exitGlyph,static_cast<DWORD>(OBJID_CLIENT),__uuidof(IAccessible),(void**)&exitAccessible))&&exitAccessible,"Completion checkbox supports accessibility action inspection");
  VARIANT self;VariantInit(&self);self.vt=VT_I4;self.lVal=CHILDID_SELF;BSTR defaultAction=nullptr;auto actionResult=exitAccessible->get_accDefaultAction(self,&defaultAction);bool described=SUCCEEDED(actionResult)&&defaultAction&&SysStringLen(defaultAction)>0;SysFreeString(defaultAction);expect(described,"Completion checkbox describes its accessible default action");
  actionResult=exitAccessible->accDoDefaultAction(self);pumpUntil([&]{return SendMessageW(exitGlyph,BM_GETCHECK,0,0)==BST_CHECKED;});VARIANT accessibleState;VariantInit(&accessibleState);auto stateResult=exitAccessible->get_accState(self,&accessibleState);bool reportedChecked=SUCCEEDED(stateResult)&&accessibleState.vt==VT_I4&&(accessibleState.lVal&STATE_SYSTEM_CHECKED);VariantClear(&accessibleState);steps=completionSteps(job->data);expect(SUCCEEDED(actionResult)&&reportedChecked&&std::find(steps.begin(),steps.end(),"Exit UDM")!=steps.end(),"Accessible default action toggles and reports the persisted completion option");
  actionResult=exitAccessible->accDoDefaultAction(self);pumpUntil([&]{return SendMessageW(exitGlyph,BM_GETCHECK,0,0)==BST_UNCHECKED;});exitAccessible->Release();expect(SUCCEEDED(actionResult)&&SendMessageW(exitGlyph,BM_GETCHECK,0,0)==BST_UNCHECKED,"Accessible default action toggles the option off again");
  SetFocus(exitGlyph);SendMessageW(exitGlyph,WM_KEYDOWN,VK_SPACE,1);SendMessageW(exitGlyph,WM_KEYUP,VK_SPACE,0xC0000001);steps=completionSteps(job->data);expect(SendMessageW(exitGlyph,BM_GETCHECK,0,0)==BST_CHECKED&&std::find(steps.begin(),steps.end(),"Exit UDM")!=steps.end(),"Space key dispatch toggles the checkbox and saves its option");SendMessageW(exitGlyph,BM_CLICK,0,0);FreeLibrary(accessibility);
  auto completion=named(w,L"Show download complete dialog");SendMessageW(completion,BM_CLICK,0,0);expect(yes(job->data,"SuppressCompletionDialog"),"Completion checkbox saves the per-download suppression preference");{Manager reopened(manager.root);expect(yes(reopened.jobs.at(0)->data,"SuppressCompletionDialog"),"Completion preference survives catalog reopen");}SendMessageW(completion,BM_CLICK,0,0);
  select(0);expect(IsWindowVisible(url)!=FALSE,"Returning to status restores the URL label");
  job->data["Url"]="https://example.invalid/refreshed.bin?a=1&b=2";SendMessageW(w,WM_TIMER,1,0);expect(caption(url)==L"https://example.invalid/refreshed.bin?a=1&b=2","Refresh updates the literal displayed URL without changing control role");
  select(1);SendMessageW(visibleNamed(w,L"Hide tab"),BM_CLICK,0,0);expect(SendMessageW(tab,TCM_GETITEMCOUNT,0,0)==2&&SendMessageW(tab,TCM_GETCURSEL,0,0)==0&&IsWindowVisible(url),"Hiding Speed Limiter removes only that tab and returns to status");expect(!yes(manager.state["Settings"],"ProgressSpeedTab")&&yes(manager.state["Settings"],"ProgressCompletionTab",true),"Hiding speed tab preserves completion tab preference");
  select(1);expect(IsWindowVisible(completion)!=FALSE,"Completion tab remains correctly mapped after speed tab removal");SendMessageW(visibleNamed(w,L"Hide tab"),BM_CLICK,0,0);expect(SendMessageW(tab,TCM_GETITEMCOUNT,0,0)==1&&IsWindowVisible(url),"Hiding completion tab leaves status available");
  {Manager reopened(manager.root);expect(!yes(reopened.state["Settings"],"ProgressSpeedTab")&&!yes(reopened.state["Settings"],"ProgressCompletionTab"),"Hidden tab preferences survive catalog reopen");}
  progress.DestroyWindow();Progress reopened(manager,job,&owner);expect(reopened.Create(101,&owner)!=FALSE,"Reopen actual progress dialog after hiding tabs");HWND reopenedTabs=nullptr;for(auto c=GetWindow(reopened.m_hWnd,GW_CHILD);c;c=GetWindow(c,GW_HWNDNEXT)){GetClassNameW(c,kind,80);if(!_wcsicmp(kind,L"SysTabControl32"))reopenedTabs=c;}expect(reopenedTabs&&SendMessageW(reopenedTabs,TCM_GETITEMCOUNT,0,0)==1,"Reopened progress dialog respects both hidden tab preferences");reopened.DestroyWindow();
  if(argc==4){
   const auto endpoint=utf8(argv[3]);Url local(endpoint);expect(local.host=="127.0.0.1"&&local.scheme=="http","Form restart fixture accepts loopback HTTP only");
   Manager forms(output/L"form-state");forms.state["Settings"]["DownloadFolder"]=utf8((output/L"form-files").wstring());forms.state["Settings"]["CategoryFolders"]=false;forms.state["Settings"]["ProxyMode"]="Connect directly";forms.state["Settings"]["Retries"]=0;
   const std::string body="export=fixture&name=%E2%9C%93&value=one%2Btwo";Json request={{"method","POST"},{"contentType","application/x-www-form-urlencoded"},{"body",b64(Bytes(body.begin(),body.end()))}};
   auto formJob=forms.add(endpoint,"","restarted-form.bin","Main queue",true,{},"",request);formJob->data["PostAttempted"]=true;forms.save();
   Progress formProgress(forms,formJob,&owner);expect(formProgress.Create(101,&owner)!=FALSE,"Create actual interrupted form progress dialog");forms.queueRun("Main queue",true);forms.tick();expect(str(formJob->data,"Status")=="Paused"&&!forms.isActive(formJob),"Starting the queue does not resubmit the attempted form");
   auto restart=named(formProgress.m_hWnd,L"Restart");expect(IsWindowEnabled(restart)!=FALSE,"Explicit form Restart action is enabled");SendMessageW(restart,BM_CLICK,0,0);expect(str(formJob->data,"Status")=="Queued"&&!yes(formJob->data,"PostAttempted"),"Actual Restart button rearms the saved request");
   const auto deadline=GetTickCount64()+10000;bool terminal=false;while(!terminal&&GetTickCount64()<deadline){forms.tick();pumpUntil([&]{Lock lock(forms.mutex);auto state=str(formJob->data,"Status");terminal=state=="Complete"||state=="Failed";return terminal;});}
   {Lock lock(forms.mutex);expect(str(formJob->data,"Status")=="Complete","Explicit form restart completes through the native transport");expect(yes(formJob->data,"PostAttempted"),"Completed restart records that the form was submitted");}
   expect(readText(formJob->target())==body,"Restarted form output equals the exact saved request body");expect(forms.jobs.size()==1,"Explicit restart preserves a single download record");formProgress.DestroyWindow();forms.stop();
  }
  m_pMainWnd=nullptr;result=0;
 }catch(const std::exception& e){error=e.what();}
 if(argv)LocalFree(argv);if(!output.empty())atomicText(output/L"results.json",Json{{"passed",result==0},{"error",error},{"checks",checks}}.dump(2),false);return FALSE;
 }
 int ExitInstance()override{AfxOleTerm(FALSE);return result;}
};ProgressDialogApplication application;
}
