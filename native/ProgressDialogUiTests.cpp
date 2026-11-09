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
static void checkPrefetchRateSamples(){
 using namespace std::chrono;
 const auto begin=steady_clock::time_point{}+seconds(1);
 Job sustained(Json{{"Id",guid()}, {"ConfirmationPending",true},{"TransferredBytes",i64(512)}});
 sustained.prefetchSampleTime=begin;
 sustained.samplePrefetchRate(begin+milliseconds(25));
 expect(!sustained.prefetchPeakRate&&sustained.prefetchSampleTime==begin,"A short tick keeps its bytes in the open sample");
 sustained.data["TransferredBytes"]=2048;
 sustained.samplePrefetchRate(begin+milliseconds(125));
 expect(sustained.prefetchPeakRate==16384,"The first measured window includes bytes from the early tick");
 sustained.data["TransferredBytes"]=4096;
 sustained.samplePrefetchRate(begin+milliseconds(126),true);
 expect(sustained.prefetchPeakRate==16384&&!sustained.prefetchSampleTime,"A one-millisecond final slice cannot replace the measured peak");
 Job quick(Json{{"Id",guid()}, {"ConfirmationPending",true},{"TransferredBytes",i64(1024)}});
 quick.prefetchSampleTime=begin;
 quick.samplePrefetchRate(begin+milliseconds(25));
 quick.samplePrefetchRate(begin+milliseconds(50),true);
 expect(quick.prefetchPeakRate==20480&&!quick.prefetchSampleTime,"A short prefetch keeps its average through confirmation");
 Job idle(Json{{"Id",guid()}, {"ConfirmationPending",true},{"TransferredBytes",i64(0)}});
 idle.prefetchSampleTime=begin;
 idle.samplePrefetchRate(begin+milliseconds(124));
 idle.data["TransferredBytes"]=2048;
 idle.samplePrefetchRate(begin+milliseconds(125),true);
 expect(idle.prefetchPeakRate==16384,"An idle tick cannot turn the first credited buffer into a one-millisecond peak");
}
static void checkProgressDismissal(Manager& manager,CWnd& owner){
 expect(progressDismissStops("Downloading",true),"Cancel still stops an active download");
 expect(!progressDismissStops("Complete",true),"Closing during a completed file's scanner wait does not cancel it");
 expect(!progressDismissStops("Pausing",true),"Closing during a pause does not submit another pause");
 for(const std::string state:{"Paused","Failed","Complete","Queued"}){
  auto job=manager.add("https://example.invalid/"+state+".bin","",state+".bin","Main queue",true);
  job->data["Status"]=state;
  if(state=="Failed")job->data["Error"]="Fixture server error";
  if(state=="Complete")job->data["SyncPending"]=true;
  manager.save();
  const auto saved=job->snapshot();
  const auto catalogHash=fileHash(manager.root/L"state.json");
  Progress dialog(manager,job,&owner);
  expect(dialog.Create(101,&owner)!=FALSE,"Create progress window for dismissal");
  dialog.ShowWindow(SW_SHOWNOACTIVATE);
  const auto window=dialog.GetSafeHwnd();
  if(state=="Queued"){
   SendMessageW(visibleNamed(window,L"Cancel"),BM_CLICK,0,0);
   expect(!IsWindow(window)&&str(job->snapshot(),"Status")=="Paused",
    "Cancel stops a queued download and closes its progress window");
   continue;
  }
  auto close=visibleNamed(window,L"Close");
  if(state=="Paused")SendMessageW(window,WM_CLOSE,0,0);
  else if(state=="Failed")SendMessageW(window,WM_COMMAND,IDCANCEL,0);
  else SendMessageW(close,BM_CLICK,0,0);
  expect(!IsWindow(window),("Dismissal destroys the "+state+" progress window").c_str());
  expect(job->snapshot()==saved&&fileHash(manager.root/L"state.json")==catalogHash,
   ("Dismissal preserves the "+state+" job and catalog").c_str());
 }
}
static void checkProgressActionTransitions(Manager& manager,CWnd& owner){
 auto job=manager.add("https://example.invalid/actions.bin","","actions.bin","Main queue",true);
 Progress dialog(manager,job,&owner);
 expect(dialog.Create(101,&owner)!=FALSE,"Create progress window for action transitions");
 dialog.ShowWindow(SW_SHOWNOACTIVATE);
 const auto window=dialog.GetSafeHwnd();

 auto start=visibleNamed(window,L"Start");
 manager.resume(job);
 const auto queued=job->snapshot();
 SendMessageW(start,BM_CLICK,0,0);
 expect(job->snapshot()==queued,"A stale Start click cannot stop a newly queued download");

 SendMessageW(window,WM_TIMER,1,0);
 auto pause=visibleNamed(window,L"Pause");
 manager.pause(job);
 const auto paused=job->snapshot();
 SendMessageW(window,WM_COMMAND,IDOK,0);
 expect(job->snapshot()==paused&&IsWindowEnabled(pause),"A stale Pause keyboard action cannot resume a stopped download");

 SendMessageW(window,WM_TIMER,1,0);
 SendMessageW(visibleNamed(window,L"Start"),BM_CLICK,0,0);
 expect(str(job->snapshot(),"Status")=="Queued","A refreshed Start still queues the download");
 SendMessageW(visibleNamed(window,L"Pause"),BM_CLICK,0,0);
 expect(str(job->snapshot(),"Status")=="Paused","A refreshed Pause still stops the queued download");

 auto close=visibleNamed(window,L"Close");
 manager.resume(job);
 const auto beforeClose=job->snapshot();
 SendMessageW(close,BM_CLICK,0,0);
 expect(!IsWindow(window)&&job->snapshot()==beforeClose,"Close never cancels a job queued before the next progress refresh");
 manager.pause(job);
}
static void checkProgressScannerAction(Manager& manager,CWnd& owner,const fs::path& output){
 auto job=manager.add("https://example.invalid/scanned.bin","","scanned.bin","Main queue",true);
 fs::create_directories(job->target().parent_path());
 writeBytes(job->target(),Bytes{1,2,3});
 job->data["Status"]="Complete";
 expect(progressCompletedAction(job->data,true)==ProgressAction::Wait,
  "A completed job busy outside the scanner cannot open or stop a scanner");
 job->data["ScanResult"]={{"Status","Pending"}};
 auto prefs=manager.state["Settings"];
 wchar_t exe[32768]{};
 expect(GetModuleFileNameW(nullptr,exe,(DWORD)std::size(exe))!=0,"Locate the inert scanner fixture");
 const auto marker=output/L"scanner-finished",started=output/L"scanner-started";
 prefs["ScanProgram"]=utf8(exe);
 prefs["ScanArguments"]="--progress-scan-fixture "+utf8(quote(marker.wstring()));
 manager.setSettings(prefs);
 Progress dialog(manager,job,&owner);
 expect(dialog.Create(101,&owner)!=FALSE,"Create completed progress dialog for scanner transitions");
 dialog.ShowWindow(SW_SHOWNOACTIVATE);
 const auto window=dialog.GetSafeHwnd();
 expect(!IsWindowEnabled(visibleNamed(window,L"Open")),"Pending scan disables Open");
 SendMessageW(window,WM_COMMAND,IDOK,0);
 expect(str(job->snapshot()["ScanResult"],"Status")=="Pending","Enter does not bypass a pending scan");
 manager.scanAgain(job);
 pumpUntil([&]{
  Lock lock(manager.mutex);
  return str(job->data["ScanResult"],"Status")=="Running"&&fs::is_regular_file(started);
 });
 SendMessageW(window,WM_TIMER,1,0);
 auto stop=visibleNamed(window,L"Stop wait");
 expect(IsWindowEnabled(stop)!=FALSE&&fs::is_regular_file(started),"An active scanner offers Stop wait");
 SendMessageW(stop,BM_CLICK,0,0);
 pumpUntil([&]{return !manager.isActive(job);});
 SendMessageW(window,WM_TIMER,1,0);
 expect(!manager.isActive(job)&&str(job->snapshot()["ScanResult"],"Status")=="Interrupted"&&
  IsWindowEnabled(visibleNamed(window,L"Open")),"Stop wait interrupts monitoring and restores the manual Open action");
 expect(readText(job->target())==std::string("\x01\x02\x03",3),"Stopping the scan wait preserves the saved file");
 dialog.DestroyWindow();
 pumpUntil([&]{return fs::is_regular_file(marker);});
 if(!fs::is_regular_file(marker))pumpUntil([&]{return fs::is_regular_file(marker);});
 expect(fs::is_regular_file(marker),"The scanner process finishes after monitoring stops");
}
class ProgressDialogApplication:public CWinApp {
 int result=1;
public:BOOL InitInstance()override{
 CWinApp::InitInstance();INITCOMMONCONTROLSEX common{sizeof(common),ICC_WIN95_CLASSES};InitCommonControlsEx(&common);AfxOleInit();int argc=0;auto argv=CommandLineToArgvW(GetCommandLineW(),&argc);fs::path output;std::string error;
 try{
  if(argc==4&&std::wstring_view(argv[1])==L"--progress-scan-fixture"){
   const fs::path marker(argv[2]);
   atomicText(marker.parent_path()/L"scanner-started","started",false);
   Sleep(2200);
   atomicText(marker,"finished",false);
   LocalFree(argv);result=0;return FALSE;
  }
  if(argc!=3&&argc!=4)throw std::runtime_error("Pass fresh output, reference-dialogs.json and optional loopback POST fixture URL");output=fs::absolute(argv[1]);if(fs::exists(output))throw std::runtime_error("Fresh output required");fs::create_directories(output);
  auto reference=Json::parse(readText(argv[2]));Json statusReference;for(auto& d:reference["dialogs"])if(num(d,"id")==363)statusReference=d;
  expect(!statusReference.is_null(),"Reference contains IDM download-status dialog 363");
  checkPrefetchRateSamples();
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
  {auto before=job->data;job->data["SourceUrl"]="https://example.invalid/video";job->data["Size"]=-1;job->data["ReportedMediaSize"]=2048;job->data["ContinuedMediaPrefetch"]=true;SendMessageW(w,WM_TIMER,1,0);
   expect(named(w,L"2.000 KB (tracks; final size pending)")!=nullptr,"Progress retains known track size while final size is unresolved");
   expect(named(w,L"375 Bytes (includes prefetch)")!=nullptr,"Progress credits bytes received before confirmation");
   const auto originalRate=job->speed;
   job->speed=1024;job->data["PrefetchedBytesAtStart"]=375;job->data["PrefetchPeakRate"]=2097152;
   SendMessageW(w,WM_TIMER,1,0);
   expect(named(w,L"1.000 KB/sec (Prefetch peak 2.000 MB/sec)")!=nullptr,"Progress shows measured prefetch peak separately from the current transfer rate");
   job->data["Status"]="Downloading";
   job->prefetchConfirmedAt=std::chrono::steady_clock::now();
   SendMessageW(w,WM_TIMER,1,0);
   expect(named(w,L"2.000 MB/sec (before Start)")!=nullptr,"Opening progress shows the measured prefetch peak");
   job->prefetchPeakDisplayUntil=std::chrono::steady_clock::now()-std::chrono::seconds(1);
   SendMessageW(w,WM_TIMER,1,0);
   expect(named(w,L"1.000 KB/sec (Prefetch peak 2.000 MB/sec)")!=nullptr,"Live rate returns after the opening prefetch display");
   job->prefetchConfirmedAt.reset();job->prefetchPeakDisplayUntil.reset();
   job->speed=originalRate;
   job->data["Size"]=4096;SendMessageW(w,WM_TIMER,1,0);expect(named(w,L"4.000 KB")!=nullptr,"Final size replaces the track-size fallback");job->data=before;SendMessageW(w,WM_TIMER,1,0);}
  {
   auto before=job->data;
   auto beforeWorkers=job->workers;
   HWND connections=nullptr;
   for(auto child=GetWindow(w,GW_CHILD);child;child=GetWindow(child,GW_HWNDNEXT)){
    GetClassNameW(child,kind,80);
    if(!_wcsicmp(kind,WC_LISTVIEWW)){connections=child;break;}
   }
   expect(connections!=nullptr,"Progress dialog exposes its connection details");
   auto cell=[&](int row,int column){
    wchar_t value[128]{};
    LVITEMW item{};item.iSubItem=column;item.pszText=value;item.cchTextMax=128;
    SendMessageW(connections,LVM_GETITEMTEXTW,row,(LPARAM)&item);
    return std::wstring(value);
   };
   job->data["Status"]="Complete";job->data["Size"]=8388608;job->data["Received"]=8388608;
   job->data["ReusedCompletedPrefetch"]=true;job->data["CompletedPrefetch"]={{"Size",8388608}};
   job->workers={Worker{1},Worker{2}};SendMessageW(w,WM_TIMER,1,0);
   expect(ListView_GetItemCount(connections)==1&&cell(0,0).empty()&&cell(0,1)==L"8,192 KB"&&cell(0,2)==L"Verified saved prefetch",
          "Offline prefetch reuse displays verified saved bytes without a connection number");
   job->workers[1].received=8388608;job->workers[1].state="Finished";
   SendMessageW(w,WM_TIMER,1,0);
   expect(ListView_GetItemCount(connections)==1&&cell(0,0)==L"1"&&cell(0,2)==L"Finished",
          "Completed connections retain their number and bytes while unused rows disappear");
   job->data=std::move(before);job->workers=std::move(beforeWorkers);SendMessageW(w,WM_TIMER,1,0);
  }
  expect(caption(named(w,L"Yes"))==L"Yes"&&IsWindowEnabled(named(w,L"Resume")),"A paused partial file with range support offers Resume");
  job->data.erase("RangeSupported");SendMessageW(w,WM_TIMER,1,0);
  expect(caption(named(w,L"Unknown"))==L"Unknown"&&IsWindowEnabled(named(w,L"Resume")),
   "A paused partial offers Resume while its range capability is still unknown");
  job->data["Status"]="Failed";SendMessageW(w,WM_TIMER,1,0);
  expect(IsWindowEnabled(named(w,L"Resume")),"A failed partial with unknown range support can also be resumed");
  job->data["Status"]="Paused";job->data["Received"]=0;SendMessageW(w,WM_TIMER,1,0);
  expect(caption(named(w,L"Unknown"))==L"Unknown"&&IsWindowEnabled(named(w,L"Start")),
   "An unstarted download with unknown range support offers Start");
  job->data["Received"]=375;
  job->data["OriginalResponse"]={{"State","Receiving"}};SendMessageW(w,WM_TIMER,1,0);expect(caption(named(w,L"Unknown"))==L"Unknown","Captured browser response does not falsely report unsupported resume");job->data.erase("OriginalResponse");
  job->data["RangeSupported"]=false;SendMessageW(w,WM_TIMER,1,0);
  expect(caption(named(w,L"No"))==L"No"&&IsWindowEnabled(named(w,L"Start")),
   "Confirmed unsupported server displays No and offers Start");
  job->data["RangeSupported"]=true;SendMessageW(w,WM_TIMER,1,0);
  {auto before=job->data;job->data["Status"]="Pausing";SendMessageW(w,WM_TIMER,1,0);
   expect(!IsWindowEnabled(named(w,L"Pausing...")),"Pausing disables the primary progress action and shows the transition");
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
  job->data["Status"]="Paused";job->data["Error"]="";job->data["ProtectedPostGetUrl"]=protect("https://example.invalid/result.bin");SendMessageW(w,WM_TIMER,1,0);expect(caption(named(w,L"Resume"))==L"Resume"&&!progressNeedsFormRestart(job->data),"Resumable POST-to-GET continuation does not resubmit the form");
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
  auto completion=named(w,L"Show download complete dialog");auto exitGlyph=glyphs.at("Exit UDM when done");
  expect(SendMessageW(completion,BM_GETCHECK,0,0)==BST_CHECKED&&!IsWindowEnabled(exitGlyph),"Showing the completion dialog disables power options");
  SendMessageW(completion,BM_CLICK,0,0);
  expect(yes(job->data,"SuppressCompletionDialog")&&IsWindowEnabled(exitGlyph),"Hiding the completion dialog saves the preference and enables power options");
  {Manager reopened(manager.root);expect(yes(reopened.jobs.at(0)->data,"SuppressCompletionDialog"),"Completion preference survives catalog reopen");}
  auto exitLabel=named(w,L"Exit UDM when done");SendMessageW(w,WM_COMMAND,MAKEWPARAM(GetDlgCtrlID(exitLabel),STN_CLICKED),(LPARAM)exitLabel);auto steps=completionSteps(job->data);expect(SendMessageW(glyphs.at("Exit UDM when done"),BM_GETCHECK,0,0)==BST_CHECKED&&std::find(steps.begin(),steps.end(),"Exit UDM")!=steps.end(),"Clicking the separate exit label checks the glyph and saves its plan without executing it");
  SendMessageW(glyphs.at("Exit UDM when done"),BM_CLICK,0,0);steps=completionSteps(job->data);expect(std::find(steps.begin(),steps.end(),"Exit UDM")==steps.end(),"Clicking the glyph clears the completion plan");
  IAccessible* exitAccessible=nullptr;expect(SUCCEEDED(accessibleObject(exitGlyph,static_cast<DWORD>(OBJID_CLIENT),__uuidof(IAccessible),(void**)&exitAccessible))&&exitAccessible,"Completion checkbox supports accessibility action inspection");
  VARIANT self;VariantInit(&self);self.vt=VT_I4;self.lVal=CHILDID_SELF;BSTR defaultAction=nullptr;auto actionResult=exitAccessible->get_accDefaultAction(self,&defaultAction);bool described=SUCCEEDED(actionResult)&&defaultAction&&SysStringLen(defaultAction)>0;SysFreeString(defaultAction);expect(described,"Completion checkbox describes its accessible default action");
  actionResult=exitAccessible->accDoDefaultAction(self);pumpUntil([&]{return SendMessageW(exitGlyph,BM_GETCHECK,0,0)==BST_CHECKED;});VARIANT accessibleState;VariantInit(&accessibleState);auto stateResult=exitAccessible->get_accState(self,&accessibleState);bool reportedChecked=SUCCEEDED(stateResult)&&accessibleState.vt==VT_I4&&(accessibleState.lVal&STATE_SYSTEM_CHECKED);VariantClear(&accessibleState);steps=completionSteps(job->data);expect(SUCCEEDED(actionResult)&&reportedChecked&&std::find(steps.begin(),steps.end(),"Exit UDM")!=steps.end(),"Accessible default action toggles and reports the persisted completion option");
  actionResult=exitAccessible->accDoDefaultAction(self);pumpUntil([&]{return SendMessageW(exitGlyph,BM_GETCHECK,0,0)==BST_UNCHECKED;});exitAccessible->Release();expect(SUCCEEDED(actionResult)&&SendMessageW(exitGlyph,BM_GETCHECK,0,0)==BST_UNCHECKED,"Accessible default action toggles the option off again");
  SetFocus(exitGlyph);SendMessageW(exitGlyph,WM_KEYDOWN,VK_SPACE,1);SendMessageW(exitGlyph,WM_KEYUP,VK_SPACE,0xC0000001);steps=completionSteps(job->data);expect(SendMessageW(exitGlyph,BM_GETCHECK,0,0)==BST_CHECKED&&std::find(steps.begin(),steps.end(),"Exit UDM")!=steps.end(),"Space key dispatch toggles the checkbox and saves its option");FreeLibrary(accessibility);
  SendMessageW(completion,BM_CLICK,0,0);expect(!yes(job->data,"SuppressCompletionDialog")&&!IsWindowEnabled(exitGlyph)&&!yes(job->data,"CompletionActionArmed"),"Showing the completion dialog again clears and disables the selected exit action");
  select(0);expect(IsWindowVisible(url)!=FALSE,"Returning to status restores the URL label");
  job->data["Url"]="https://example.invalid/refreshed.bin?a=1&b=2";SendMessageW(w,WM_TIMER,1,0);expect(caption(url)==L"https://example.invalid/refreshed.bin?a=1&b=2","Refresh updates the literal displayed URL without changing control role");
  select(1);SendMessageW(visibleNamed(w,L"Hide tab"),BM_CLICK,0,0);expect(SendMessageW(tab,TCM_GETITEMCOUNT,0,0)==2&&SendMessageW(tab,TCM_GETCURSEL,0,0)==0&&IsWindowVisible(url),"Hiding Speed Limiter removes only that tab and returns to status");expect(!yes(manager.state["Settings"],"ProgressSpeedTab")&&yes(manager.state["Settings"],"ProgressCompletionTab",true),"Hiding speed tab preserves completion tab preference");
  select(1);expect(IsWindowVisible(completion)!=FALSE,"Completion tab remains correctly mapped after speed tab removal");SendMessageW(visibleNamed(w,L"Hide tab"),BM_CLICK,0,0);expect(SendMessageW(tab,TCM_GETITEMCOUNT,0,0)==1&&IsWindowVisible(url),"Hiding completion tab leaves status available");
  {Manager reopened(manager.root);expect(!yes(reopened.state["Settings"],"ProgressSpeedTab")&&!yes(reopened.state["Settings"],"ProgressCompletionTab"),"Hidden tab preferences survive catalog reopen");}
  progress.DestroyWindow();Progress reopened(manager,job,&owner);expect(reopened.Create(101,&owner)!=FALSE,"Reopen actual progress dialog after hiding tabs");HWND reopenedTabs=nullptr;for(auto c=GetWindow(reopened.m_hWnd,GW_CHILD);c;c=GetWindow(c,GW_HWNDNEXT)){GetClassNameW(c,kind,80);if(!_wcsicmp(kind,L"SysTabControl32"))reopenedTabs=c;}expect(reopenedTabs&&SendMessageW(reopenedTabs,TCM_GETITEMCOUNT,0,0)==1,"Reopened progress dialog respects both hidden tab preferences");reopened.DestroyWindow();
  checkProgressDismissal(manager,owner);
  checkProgressActionTransitions(manager,owner);
  checkProgressScannerAction(manager,owner,output);
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
