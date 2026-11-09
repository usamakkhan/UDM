// Exercises only windows created by this isolated executable.
#define UDM_CAPTURE_PRESENTATION_COMPONENT_TEST
#include "App.cpp"
#include "AuthenticationFixture.hpp"
#include "CaptureTransactions.hpp"
#include "CapturedResponseTestAccess.hpp"
#include "../drivers/signed-network/DurableResponseSink.hpp"
namespace udm {
static fs::path output;static Json outcomes=Json::array();static std::string failure;static ULONGLONG began;
static unsigned informationShown=0,duplicatesShown=0,refreshShown=0;static bool refreshRaceInjected=false;static Manager* raceManager=nullptr;
static unsigned reviewsShown=0;static bool revisitReviews=false;static Json reviewFixtures=Json::object();
static HWND presentationMain=nullptr;
static Json presentationTiming=Json::object();static DWORD presentationDispatch=0;static bool timingArmed=false;
static unsigned queuedLoginsShown=0;
static bool acceptQueuedLogin=false;

static void admit(Manager&,const std::string&,const std::string&);
static std::string captureToken();static bool mediaCancelInjected=false;
static Json mediaMessage(const std::string& name,int version){return {{"action","adaptive"},{"mediaToken",captureToken()},{"url","http://127.0.0.1:9/"+name},{"filename",name},{"plan",{{"type","hls"},{"height",360},{"audioExpected",false},{"container","mp4"},{"tracks",Json::array({{{"kind","video"},{"segments",Json::array({{{"url","http://127.0.0.1:9/segment-"+std::to_string(version)+".ts"}}})}}})}}}};}
static void expect(bool value,const char* name){outcomes.push_back({{"name",name},{"passed",value}});if(!value)throw std::runtime_error(name);}
static std::wstring title(HWND w){wchar_t value[2048]{};GetWindowTextW(w,value,2048);return value;}
static void CALLBACK presentationShown(HWINEVENTHOOK,DWORD event,HWND window,LONG object,LONG,DWORD,DWORD when){
 if(!timingArmed||object!=OBJID_WINDOW)return;
 if(event==EVENT_OBJECT_CREATE&&title(GetParent(window))==L"Download File Info"){
  wchar_t kind[80]{};GetClassNameW(window,kind,80);
  presentationTiming["childCreates"].push_back({{"class",utf8(kind)},{"id",GetDlgCtrlID(window)},{"ms",static_cast<DWORD>(when-presentationDispatch)}});
 }
 if(title(window)!=L"Download File Info")return;
 const char* key=event==EVENT_OBJECT_CREATE?"nativeDispatchToInfoCreateMs":event==EVENT_OBJECT_NAMECHANGE?"nativeDispatchToInfoTitleMs":event==EVENT_OBJECT_SHOW?"nativeDispatchToInfoShowMs":nullptr;
 if(key&&!presentationTiming.contains(key))presentationTiming[key]=static_cast<DWORD>(when-presentationDispatch);
 if(event==EVENT_OBJECT_SHOW)timingArmed=false;
}
static HWND findChild(HWND w,const wchar_t* label){for(auto c=GetWindow(w,GW_CHILD);c;c=GetWindow(c,GW_HWNDNEXT))if(title(c)==label)return c;return nullptr;}
static HWND child(HWND w,const wchar_t* label){if(auto control=findChild(w,label))return control;throw std::runtime_error("Missing native test control");}
static void press(HWND w,const wchar_t* label){auto c=child(w,label);expect(IsWindowEnabled(c)!=FALSE,"Native presentation button is enabled");SendMessageW(c,BM_CLICK,0,0);}
static std::wstring editTexts(HWND w){std::wstring value;for(auto c=GetWindow(w,GW_CHILD);c;c=GetWindow(c,GW_HWNDNEXT)){wchar_t type[80]{};GetClassNameW(c,type,80);if(!_wcsicmp(type,L"Edit"))value+=title(c)+L"\n";}return value;}
static void capture(HWND w,const wchar_t* name){
 ShowWindow(w,SW_SHOWNOACTIVATE);RedrawWindow(w,nullptr,nullptr,RDW_INVALIDATE|RDW_ALLCHILDREN|RDW_UPDATENOW|RDW_ERASE);CRect r;GetWindowRect(w,&r);CImage image;image.Create(r.Width(),r.Height(),32);auto dc=image.GetDC();RECT full{0,0,r.Width(),r.Height()};FillRect(dc,&full,GetSysColorBrush(COLOR_BTNFACE));SendMessageW(w,WM_PRINT,(WPARAM)dc,PRF_NONCLIENT|PRF_CLIENT|PRF_ERASEBKGND|PRF_CHILDREN);
 for(auto c=GetWindow(w,GW_CHILD);c;c=GetWindow(c,GW_HWNDNEXT)){if(!(GetWindowLongW(c,GWL_STYLE)&WS_VISIBLE))continue;CRect cr;GetWindowRect(c,&cr);auto saved=SaveDC(dc);IntersectClipRect(dc,cr.left-r.left,cr.top-r.top,cr.right-r.left,cr.bottom-r.top);SetViewportOrgEx(dc,cr.left-r.left,cr.top-r.top,nullptr);SendMessageW(c,WM_PRINT,(WPARAM)dc,PRF_NONCLIENT|PRF_CLIENT|PRF_ERASEBKGND|PRF_CHILDREN);RestoreDC(dc,saved);}
 image.ReleaseDC();if(FAILED(image.Save((output/name).c_str())))throw std::runtime_error("Cannot render the native presentation fixture");
}
static BOOL CALLBACK observe(HWND w,LPARAM){
 auto caption=title(w);if(caption!=L"Download File Info"&&caption!=L"Duplicate download link"&&caption!=L"Refresh download address"&&caption!=L"Review captured download"&&caption!=L"Refresh media session"&&caption!=L"Login required")return TRUE;
 try{
  if(caption==L"Login required"){
   ++queuedLoginsShown;
   if(!acceptQueuedLogin)press(w,L"Cancel");
   else{
    HWND user=nullptr,password=nullptr;
    for(auto control=GetWindow(w,GW_CHILD);control;control=GetWindow(control,GW_HWNDNEXT)){
     wchar_t type[16]{};GetClassNameW(control,type,16);
     if(_wcsicmp(type,L"Edit"))continue;
     const auto style=GetWindowLongW(control,GWL_STYLE);
     if(style&ES_PASSWORD)password=control;
     else if(!(style&ES_READONLY))user=control;
    }
    expect(user&&password,"Saved queue challenge opens the native login fields");
    SetWindowTextW(user,L"u");SetWindowTextW(password,L"p:2");
    press(w,L"Sign in and retry");
   }
  }else if(caption==L"Refresh media session"){
   KillTimer(w,2); // Owned harness never opens a personal browser.
   auto values=editTexts(w);const bool cancel=values.find(L"media-cancel")!=std::wstring::npos;
   JobPtr job;for(auto j:raceManager->jobs)if(str(j->data,"Url")==std::string("http://127.0.0.1:9/")+(cancel?"media-cancel":"media-save"))job=j;
   expect(job&&IsWindowEnabled(child(w,L"Save session")),"Recovered media dialog retains its captured session across catalog restart");
   if(cancel&&!mediaCancelInjected){raceManager->receive(mediaMessage("media-cancel",2));mediaCancelInjected=true;capture(w,L"media-cancel-before-new-session.png");press(w,L"Cancel");}
   else if(!cancel){auto before=str(job->data,"ProtectedAdaptive");raceManager->receive(mediaMessage("media-save",2));press(w,L"Save session");expect(IsWindow(w)&&str(job->data,"ProtectedAdaptive")==before,"First media Save reviews a replacement that changed before the click");capture(w,L"media-save-review-new-session.png");press(w,L"Save session");}
   else{capture(w,L"media-cancel-restored-new-session.png");press(w,L"Save session");}
  }else if(caption==L"Review captured download"){
   ++reviewsShown;auto values=editTexts(w);
   if(values.find(L"review-refresh.bin")!=std::wstring::npos){capture(w,L"review-replacement.png");press(w,L"Review replacement...");}
   else if(values.find(L"review-later.bin")!=std::wstring::npos){capture(w,revisitReviews?L"review-reopened.png":L"review-later.png");if(revisitReviews)press(w,L"Discard captured link");else{auto before=raceManager->pendingBrowserCaptureReviews();SendMessageW(w,WM_COMMAND,IDCANCEL,0);expect(raceManager->pendingBrowserCaptureReviews()==before,"Cancel command preserves its saved request until review is reopened");}}
   else if(values.find(L"review-discard.bin")!=std::wstring::npos){expect(!IsWindowEnabled(child(w,L"Review replacement...")),"Completed original disables replacement review");capture(w,L"review-completed.png");press(w,L"Discard captured link");}
   else if(values.find(L"review-stale.bin")!=std::wstring::npos){
    JobPtr original;for(auto j:raceManager->jobs)if(j->id()==str(reviewFixtures["review-stale.bin"],"id"))original=j;
    raceManager->remove(original);press(w,L"Review replacement...");
    expect(IsWindow(w)&&!IsWindowEnabled(child(w,L"Review replacement...")),"A removed target keeps review open and disables replacement");
    expect(raceManager->pendingBrowserCaptureReviews().size()>=1,"Stale click retains the captured request for another choice");capture(w,L"review-stale.png");press(w,L"Download new file...");
   }else{expect(values.find(L"review-new.bin")!=std::wstring::npos,"Deleted-target review shows its original file");expect(!IsWindowEnabled(child(w,L"Review replacement...")),"Deleted original disables replacement review");capture(w,L"review-deleted.png");press(w,L"Download new file...");}
  }else if(caption==L"Download File Info"){
   expect(IsWindowVisible(w)!=FALSE,"File Info is visible before the screenshot helper can show it");
   if((GetWindowLongPtrW(w,GWL_EXSTYLE)&(WS_EX_APPWINDOW|WS_EX_TOPMOST))!=(WS_EX_APPWINDOW|WS_EX_TOPMOST))outcomes.push_back({{"diagnostic","capture styles"},{"styles",(uint64_t)GetWindowLongPtrW(w,GWL_EXSTYLE)},{"ownerMarked",GetPropW(GetWindow(w,GW_OWNER),L"UDM.CaptureDialogOwner")!=nullptr},{"fields",utf8(editTexts(w))}});
   expect((GetWindowLongPtrW(w,GWL_EXSTYLE)&(WS_EX_APPWINDOW|WS_EX_TOPMOST))==(WS_EX_APPWINDOW|WS_EX_TOPMOST),"Browser File Info remains reachable without main-window foreground permission");
   expect(GetAncestor(w,GA_ROOTOWNER)!=presentationMain,"Automatic File Info is owned independently of the main window");
   ++informationShown;auto values=editTexts(w);
   if(values.find(L"/review-")!=std::wstring::npos){capture(w,L"review-new-file-info.png");press(w,L"Download &Later");}
   else if(values.find(L"/start.bin")!=std::wstring::npos){capture(w,L"information-start.png");press(w,L"&Start Download");}
   else if(values.find(L"/later.bin")!=std::wstring::npos){
    expect(!findChild(w,L"Queue")&&!findChild(w,L"Use login and password"),"Compact File Info does not create advanced controls before More");
    press(w,L"More >>");
    const auto login=child(w,L"Use login and password");
    expect(IsWindowVisible(child(w,L"Queue"))&&IsWindowVisible(login),"More displays queue and login controls");
    QueueCombo* queue=nullptr;
    for(auto control=GetWindow(w,GW_CHILD);control;control=GetWindow(control,GW_HWNDNEXT))
     if(auto selector=dynamic_cast<QueueCombo*>(CWnd::FromHandlePermanent(control)))queue=selector;
    expect(queue&&queue->selectQueue("Deferred fixture queue"),"More lets the user select another queue");
    press(w,L"Use login and password");
    expect(SendMessageW(login,BM_GETCHECK,0,0)==BST_CHECKED,"Expanded File Info accepts a login draft");
    press(w,L"<< Less");
    expect(!IsWindowVisible(login),"Less hides advanced controls");
    press(w,L"More >>");
    expect(child(w,L"Use login and password")==login&&SendMessageW(login,BM_GETCHECK,0,0)==BST_CHECKED,"Reopening More preserves the login draft and controls");
    press(w,L"Use login and password");
    press(w,L"<< Less");
    capture(w,L"information-later.png");press(w,L"Download &Later");
   }
   else{capture(w,L"information-cancel.png");press(w,L"&Cancel");}
  }else if(caption==L"Duplicate download link"){
   ++duplicatesShown;capture(w,L"duplicate.png");press(w,L"Add the duplicate with a numbered file name");press(w,L"OK");
  }else{
   ++refreshShown;auto values=editTexts(w);
   if(values.find(L"/refresh-cancel.bin?fresh=1")!=std::wstring::npos){
    // The dialog already polled version one. Commit a new link immediately
    // before its Cancel click, without letting the 250 ms poll display it.
    admit(*raceManager,"refresh-cancel.bin","?fresh=2");refreshRaceInjected=true;
    capture(w,L"refresh-cancel-before-new-link.png");press(w,L"Cancel");
   }else if(values.find(L"/refresh-save.bin?fresh=1")!=std::wstring::npos){
    admit(*raceManager,"refresh-save.bin","?fresh=2");press(w,L"Save address");
    expect(IsWindow(w)&&editTexts(w).find(L"/refresh-save.bin?fresh=2")!=std::wstring::npos,"Save keeps the dialog open and displays a newly arrived address for review");
    JobPtr original;for(auto j:raceManager->jobs)if(str(j->data,"FileName")=="refresh-save.bin")original=j;
    expect(original&&str(original->data,"Url")=="http://127.0.0.1:9/refresh-save.bin","The first Save cannot apply an address that changed before the click");
    capture(w,L"refresh-save-review-new-link.png");press(w,L"Save address");
   }else if(values.find(L"/refresh-cancel.bin?fresh=2")!=std::wstring::npos){
    expect(refreshRaceInjected,"The later browser link survives cancellation of the earlier dialog");capture(w,L"refresh-new-link.png");press(w,L"Save address");
   }else{expect(values.find(L"?fresh=1")!=std::wstring::npos,"Recovered refresh dialog shows the captured replacement URL");capture(w,L"refresh.png");press(w,L"Save address");}
  }
 }catch(const std::exception& e){failure=e.what();PostMessageW(w,WM_CLOSE,0,0);}return FALSE;
}
static void CALLBACK tick(HWND,UINT,UINT_PTR,DWORD){
 if(GetTickCount64()-began>20000){failure="Presentation UI fixture deadline";EnumThreadWindows(GetCurrentThreadId(),[](HWND w,LPARAM)->BOOL{if(title(w)!=mainWindowTitle)PostMessageW(w,WM_CLOSE,0,0);return TRUE;},0);return;}
 EnumThreadWindows(GetCurrentThreadId(),observe,0);
}
static std::string captureToken(){auto value=guid();for(int p:{20,16,12,8})value.insert(p,"-");return std::to_string(epoch())+"-"+value;}
static void admit(Manager& m,const std::string& file,const std::string& query=""){
 auto token=captureToken();prepareCapture(m,{{"captureToken",token},{"download",{{"action","add"},{"url","http://127.0.0.1:9/"+file+query},{"filename",file}}}});commitPreparedCapture(m,token);
}
static void prepareFixtures(const fs::path& dir){
 Manager m(dir);auto p=m.state["Settings"];p["DownloadFolder"]=utf8((output/L"files").wstring());p["CategoryFolders"]=false;p["SkipBrowserFileInfo"]=false;p["PrefetchFileInfo"]=false;p["QueuePromptLater"]=false;p["DuplicatePolicy"]="Numbered";p["Sound"]=false;p["ClipboardMonitor"]=false;p["CloseToTray"]=false;p["DropBasket"]=false;m.setSettings(p);
 m.setQueue(defaultQueue("Deferred fixture queue"));
 admit(m,"cancel.bin");admit(m,"start.bin");admit(m,"later.bin");
 m.add("http://127.0.0.1:9/existing.bin","","existing.bin");p["DuplicatePolicy"]="Existing";m.setSettings(p);admit(m,"existing.bin");
 auto complete=m.add("http://127.0.0.1:9/complete.bin","","complete.bin");fs::create_directories(complete->target().parent_path());writeBytes(complete->target(),Bytes(12,42));complete->data["Status"]="Complete";complete->data["Size"]=12;complete->data["Received"]=12;m.save();admit(m,"complete.bin");
 m.add("http://127.0.0.1:9/duplicate.bin","","duplicate.bin");p["DuplicatePolicy"]="Ask";m.setSettings(p);admit(m,"duplicate.bin");
 auto refresh=m.add("http://127.0.0.1:9/refresh.bin","","refresh.bin");m.beginAddressRefresh(refresh);admit(m,"refresh.bin","?fresh=1");
 m.cancelAddressRefresh(refresh);auto cancelled=m.add("http://127.0.0.1:9/refresh-cancel.bin","","refresh-cancel.bin");m.beginAddressRefresh(cancelled);admit(m,"refresh-cancel.bin","?fresh=1");
 m.cancelAddressRefresh(cancelled);auto changed=m.add("http://127.0.0.1:9/refresh-save.bin","","refresh-save.bin");m.beginAddressRefresh(changed);admit(m,"refresh-save.bin","?fresh=1");
 m.cancelAddressRefresh(changed);
 for(const auto& name:{"media-save","media-cancel"}){
  auto job=m.receive(mediaMessage(name,0));for(const auto& offer:m.pendingBrowserPresentations())if(str(offer,"id")==job->id())m.finishBrowserPresentation(offer);
  m.beginAddressRefresh(job);expect(m.receive(mediaMessage(name,1))==job,"Media receipt stages a replacement on the original job");m.cancelAddressRefresh(job);
 }
 expect(m.pendingBrowserPresentations().size()==11,"Eleven browser presentations are durably prepared before restart");
}
static void prepareReviewFixtures(Manager& m){
 for(const auto& name:{"review-refresh.bin","review-new.bin","review-discard.bin","review-later.bin","review-stale.bin"}){
  auto job=m.add(std::string("http://127.0.0.1:9/")+name,"",name);auto part=m.root/L"parts"/wide(job->id())/L"0000.part";fs::create_directories(part.parent_path());writeBytes(part,Bytes(71,0x6b));
  job->data["Size"]=2048;job->data["Received"]=71;job->data["Segments"]=Json::array({{{"Index",0},{"Start",0},{"End",2047},{"Done",71}}});m.save();m.beginAddressRefresh(job);
  auto token=captureToken();prepareCapture(m,{{"captureToken",token},{"download",{{"action","add"},{"url",std::string("http://127.0.0.1:9/")+name+"?fresh=1"},{"filename",name}}}});
  if(std::string(name)=="review-new.bin")m.remove(job);else if(std::string(name)=="review-discard.bin")job->data["Status"]="Complete";else job->data["Url"]=str(job->data,"Url")+"?changed=1";m.save();
  expect(str(commitPreparedCapture(m,token),"status")=="review","Changed target is retained as a native review");m.cancelAddressRefresh(job);
  reviewFixtures[name]={{"token",token},{"id",job->id()},{"part",utf8(part.wstring())},{"hash",fileHash(part)}};
 }
}
class CapturePresentationComponentTest {
 static void queuedAuthentication(Manager& manager,MainWindow& frame){
  WSADATA winsock{};
  if(WSAStartup(MAKEWORD(2,2),&winsock)!=0)throw std::runtime_error("Cannot initialize the queue login origin.");
  struct SocketCleanup{~SocketCleanup(){WSACleanup();}} cleanup;
  AuthenticationFixture origin;
  const std::string queue="Authentication fixture queue";
  manager.setQueue(defaultQueue(queue));
  auto job=manager.add(origin.url("/basic"),utf8((output/L"queue-authentication").wstring()),"protected.bin",queue,true);
  auto wait=[&]{
   const auto deadline=GetTickCount64()+10000;
   do{
    manager.tick();
    {Lock lock(manager.mutex);if(!manager.isActive(job)&&str(job->data,"Status")!="Queued")return;}
    Sleep(10);
   }while(GetTickCount64()<deadline);
   throw std::runtime_error("The queue login transfer did not settle.");
  };
  auto observeLogin=[&](const std::function<void()>& action){
   began=GetTickCount64();const auto timer=SetTimer(nullptr,0,75,tick);
   action();KillTimer(nullptr,timer);
   if(!failure.empty())throw std::runtime_error(failure);
  };
  manager.queueRun(queue,true);
  wait();
  expect(yes(job->data,"QueueOrigin")&&authenticationDialogReady(job->data)&&origin.challenges==1,
   "A real queue download records its login challenge without retrying it");
  const auto requests=origin.requests.load();
  observeLogin([&]{frame.refresh();});
  expect(queuedLoginsShown==0&&origin.requests==requests,"The failed queue stays silent during a main window refresh");

  Progress progress(manager,job,&frame);
  expect(progress.Create(101,&frame)!=FALSE,"Create progress for the failed queue download");
  progress.ShowWindow(SW_SHOWNOACTIVATE);
  const auto before=job->data;
  observeLogin([&]{press(progress.GetSafeHwnd(),L"Start");});
  expect(queuedLoginsShown==1&&job->data==before&&origin.requests==requests&&!manager.isActive(job),
   "Cancel from progress keeps the saved challenge and makes no extra request");
  progress.DestroyWindow();

  frame.filter="all";frame.refresh();frame.table.SetItemState(-1,0,LVIS_SELECTED);
  const auto row=std::find(frame.visible.begin(),frame.visible.end(),job);
  expect(row!=frame.visible.end(),"The failed queue download is selectable in the main list");
  frame.table.SetItemState(static_cast<int>(row-frame.visible.begin()),LVIS_SELECTED,LVIS_SELECTED);
  acceptQueuedLogin=true;
  observeLogin([&]{frame.command(CMD_RESUME);});
  acceptQueuedLogin=false;
  expect(queuedLoginsShown==2&&origin.requests==requests&&yes(job->data,"IndividualStart")&&!yes(job->data,"QueueOrigin"),
   "Resume from the main list asks for login before scheduling an individual retry");
  wait();
  expect(str(job->data,"Status")=="Complete"&&origin.challenges==1&&origin.accepted==1&&
   origin.requests==requests+1&&readText(job->target())==origin.payload,
   "The next request authenticates and saves the complete file without another challenge");
 }
 static void recoveryCommands(Manager& manager,MainWindow& frame){
  const std::string source="https://www.youtube.com/watch?v=abcdefghijk";
  auto makeHeld=[&](const char* name){
   auto job=manager.add(source,"",name);job->data["SourceUrl"]=source;
   job->data["ProtectedSabr"]=protect(Json{{"video",{{"id","137"}}}}.dump());
   job->data["Status"]="Failed";job->data["SabrSessionRefreshRequired"]=true;
   job->data["SabrRetainedBytes"]=1234;job->data["SabrConnections"]=1;
   job->data["StreamDurationMs"]=180000;job->data["StreamCompletedMs"]=65234;
   job->data["SabrRanges"]=Json::array({{{"Start",0},{"Done",65234}}});
   return job;
  };
  auto first=makeHeld("held-first.mp4"),second=makeHeld("held-second.mp4");
  auto ordinary=manager.add("http://127.0.0.1:9/ordinary-resume.bin","","ordinary-resume.bin");
  ordinary->data["Error"]="Awaiting fixture resume";manager.save();
  auto invoke=[&](const std::function<void()>& command){
   int opens=0;std::string page;bool resumeButton=false;
   formRefreshPageObserver=[&](const std::string& value){
    ++opens;page=value;
    EnumThreadWindows(GetCurrentThreadId(),[](HWND w,LPARAM result)->BOOL{
     if(title(w)!=L"Refresh media session")return TRUE;
     bool& found=*reinterpret_cast<bool*>(result);
     for(auto item=GetWindow(w,GW_CHILD);item;item=GetWindow(item,GW_HWNDNEXT)){
      if(title(item)==L"Save and resume")found=true;
      if(title(item)==L"Cancel")PostMessageW(item,BM_CLICK,0,0);
     }
     return FALSE;
    },reinterpret_cast<LPARAM>(&resumeButton));
   };
   auto watchdog=SetTimer(nullptr,0,4000,[](HWND,UINT,UINT_PTR,DWORD){
    EnumThreadWindows(GetCurrentThreadId(),[](HWND w,LPARAM)->BOOL{
     const auto caption=title(w);if(caption==L"Refresh media session"||caption==L"UDM")PostMessageW(w,WM_CLOSE,0,0);return TRUE;
    },0);
   });
   command();KillTimer(nullptr,watchdog);formRefreshPageObserver={};
   expect(opens==1&&resumeButton&&page.rfind(source+"&t=65s#udm-stream-refresh=",0)==0,"The action opens one media recovery dialog with the saved playback position");
  };
  frame.filter="all";frame.refresh();frame.table.SetItemState(-1,0,LVIS_SELECTED);
  for(const auto& job:{first,second,ordinary}){
   const auto row=std::find(frame.visible.begin(),frame.visible.end(),job);
   expect(row!=frame.visible.end(),"Resume fixture job appears in the download list");
   frame.table.SetItemState((int)(row-frame.visible.begin()),LVIS_SELECTED,LVIS_SELECTED);
  }
  invoke([&]{frame.command(CMD_RESUME);});
  {Lock lock(manager.mutex);expect(str(ordinary->data,"Error")!="Awaiting fixture resume","Mixed Resume advances the ordinary download before opening recovery");}
  expect(yes(first->data,"SabrSessionRefreshRequired")&&yes(second->data,"SabrSessionRefreshRequired")&&
   num(first->data,"SabrRetainedBytes")==1234&&num(second->data,"SabrRetainedBytes")==1234,"Cancel keeps both held downloads and their checkpoints");
  Progress progress(manager,first,&frame);
  expect(progress.Create(101,&frame)!=FALSE,"Create actual progress window for a held download");
  progress.ShowWindow(SW_SHOWNOACTIVATE);
  const auto refresh=child(progress.GetSafeHwnd(),L"Refresh...");
  expect(IsWindowEnabled(refresh)!=FALSE,"Held progress exposes an enabled Refresh button");
  invoke([&]{progress.SendMessage(WM_COMMAND,MAKEWPARAM(GetDlgCtrlID(refresh),BN_CLICKED),(LPARAM)refresh);});
  expect(yes(first->data,"SabrSessionRefreshRequired")&&!manager.isActive(first),"Cancel from progress keeps the job held without starting it");
  progress.DestroyWindow();
 }
public:static void run(Manager& manager,MainWindow& frame){
 {SetEnvironmentVariableW(L"UDM_INSTANCE_TAG",wide(guid()).c_str());
  Manager isolated(output/L"bridge-focus-state");
  std::atomic<int> shows=0,presentations=0,dispatches=0;
  PipeServer bridge(isolated,[&]{++shows;},[&]{++presentations;},[&]{++dispatches;});
  auto token=captureToken();auto prepared=send({{"action","capture-prepare"},{"captureToken",token},{"download",{{"action","add"},{"url","http://127.0.0.1:9/focus.bin"},{"filename","focus.bin"}}}},3000);
  expect(yes(prepared,"ok"),"Real browser pipe prepares isolated focus fixture");
  expect(presentations==0,"Preparing a browser capture does not present an uncommitted download");
  auto committed=send({{"action","capture-commit"},{"captureToken",token}},3000);
  expect(str(committed,"status")=="accepted"&&shows==0,"Accepted browser capture never requests main-window activation");
  expect(presentations==1,"Accepted capture immediately notifies the UI without waiting for polling");
  auto direct=send({{"action","add"},{"url","http://127.0.0.1:9/direct-focus.bin"},{"filename","direct-focus.bin"}},3000);
  expect(yes(direct,"ok")&&shows==0,"Direct browser download handoff never requests main-window activation");
  expect(presentations==2,"Direct browser handoff immediately notifies the UI without activation");
  expect(dispatches==0,"Browser offers awaiting File Info do not request a queue dispatch");
  send({{"action","ping"}},3000);expect(presentations==2,"Read-only browser requests do not schedule a presentation");
  {
   Lock lock(isolated.mutex);
   isolated.state["Settings"]["SkipBrowserFileInfo"]=true;
  }
  auto queued=send({{"action","add"},{"url","http://127.0.0.1:9/queued-focus.bin"},{"filename","queued-focus.bin"}},3000);
  expect(yes(queued,"ok")&&dispatches==1&&shows==0,"A saved direct browser download requests a queue dispatch without activation");
  auto later=send({{"action","add"},{"url","http://127.0.0.1:9/later-focus.bin"},{"filename","later-focus.bin"},{"downloadLater",true}},3000);
  expect(yes(later,"ok")&&dispatches==1,"Browser Download Later does not request a queue dispatch");
  const auto queuedToken=captureToken();
  auto queuedPrepared=send({{"action","capture-prepare"},{"captureToken",queuedToken},
   {"download",{{"action","add"},{"url","http://127.0.0.1:9/prepared-focus.bin"},{"filename","prepared-focus.bin"}}}},3000);
  expect(str(queuedPrepared,"status")=="prepared"&&dispatches==1,"Preparing a browser download does not request a queue dispatch");
  auto queuedCommitted=send({{"action","capture-commit"},{"captureToken",queuedToken}},3000);
  expect(str(queuedCommitted,"status")=="accepted"&&dispatches==2&&shows==0,"An accepted queued capture requests a queue dispatch without activation");
  send({{"action","show"}},3000);expect(shows==1,"Explicit show command still activates the main window");
 }
 presentationMain=frame.GetSafeHwnd();
 frame.ShowWindow(SW_HIDE);
 {CaptureDialogOwner isolated(&frame,true);expect(!frame.IsWindowVisible()&&!frame.IsWindowEnabled(),"Capture owner keeps the main window hidden and disables modal reentry");expect(isolated.get(&frame)!=&frame,"Capture has its own invisible owner");}
 expect(!frame.IsWindowVisible()&&frame.IsWindowEnabled(),"Closing the capture owner restores availability without showing the main window");
 {
  expect(!fs::exists(appDir()/L"network"/L"Udm.Network.exe"),"Restart fixture has no elevated helper executable");
  Manager interrupted(output/L"restart-state");
  Json offer={{"Id",uint64_t(1)},{"Method","GET"},{"Status",200},{"WasInterceptable",true},{"Host","example.invalid"},{"Target","/restart.bin"},{"Length",uint64_t(4)}};
  Json context={{"Session",std::string(32,'a')},{"Generation",uint64_t(1)},{"ResponseAdmissionEnabled",true},{"PendingResponses",Json::array({offer})}};JobPtr partial;
  {auto reserved=interrupted.reserveOriginalResponse(context,offer);partial=reserved.job;auto body=reserved.body.h,receipt=reserved.receipt.h;reserved.body.h=reserved.receipt.h=INVALID_HANDLE_VALUE;udmnet::DurableResponseSink sink(body,receipt,true,4);sink.write("ab");}
  partial->data["Status"]="Downloading";partial->speed=1024;interrupted.save();
  NetworkCaptureSession session(&interrupted);session.monitor=std::make_unique<Monitor>();session.capturing=true;session.watched[GetCurrentProcessId()]={};bool missing=false;
  try{session.start();}catch(const std::exception& e){missing=std::string(e.what()).find("network helper is missing")!=std::string::npos;}
  expect(missing,"Restart reports missing replacement helper without elevation");
  expect(!session.monitor&&!session.capturing&&session.watched.empty(),"Failed restart clears stale monitor and capture state");
  expect(str(partial->data,"Status")=="Paused"&&num(partial->data,"Received")==2&&partial->speed==0,"Restart reconciles partial bytes and pauses the old capture before replacement");
  {Manager reopened(interrupted.root);expect(reopened.jobs.size()==1&&str(reopened.jobs.front()->data,"Status")=="Paused"&&num(reopened.jobs.front()->data,"Received")==2,"Interrupted capture recovery survives reopening after failed restart");}
 }
 frame.KillTimer(1);frame.ShowWindow(SW_SHOWNOACTIVATE);auto before=manager.pendingBrowserPresentations();frame.EnableWindow(FALSE);frame.refresh();expect(manager.pendingBrowserPresentations()==before,"A disabled main window retains every pending presentation");frame.EnableWindow(TRUE);
 const auto presentationRowView=frame.rowView;frame.filter="finished";
 raceManager=&manager;began=GetTickCount64();auto timer=SetTimer(nullptr,0,75,tick);
 auto showHook=SetWinEventHook(EVENT_OBJECT_CREATE,EVENT_OBJECT_NAMECHANGE,nullptr,presentationShown,GetCurrentProcessId(),GetCurrentThreadId(),WINEVENT_OUTOFCONTEXT);
 expect(showHook!=nullptr,"Native timing installs a process-scoped show-event observer");
 presentationDispatch=GetTickCount();timingArmed=true;frame.SendMessage(PRESENT_CAPTURE);timingArmed=false;UnhookWinEvent(showHook);KillTimer(nullptr,timer);
 expect(presentationTiming.contains("nativeDispatchToInfoShowMs"),"Native timing receives the File Info show-event timestamp");if(!failure.empty())throw std::runtime_error(failure);
 expect(frame.rowView==presentationRowView,"Immediate capture opens dialogs before rebuilding the download history");frame.filter="all";
 expect(informationShown==4&&duplicatesShown==1&&refreshShown==3,"Restart opens the expected File Info, duplicate and refresh dialogs");
 expect(refreshRaceInjected&&mediaCancelInjected&&manager.pendingBrowserPresentations().size()==2,"Cancel cannot acknowledge a newer captured address that was not displayed");
 timer=SetTimer(nullptr,0,75,tick);frame.refresh();KillTimer(nullptr,timer);if(!failure.empty())throw std::runtime_error(failure);
 expect(refreshShown==4,"The undisplayed replacement address opens its own refresh dialog");
 expect(manager.pendingBrowserPresentations().empty(),"Completed native dialog actions consume their saved presentations");
 for(auto j:manager.jobs)if(str(j->data,"Url").find("/media-")!=std::string::npos){auto plan=Json::parse(reveal(str(j->data,"ProtectedAdaptive")));expect(plan.dump().find("segment-2.ts")!=std::string::npos,"Media Save applies the latest reviewed session to the original download");expect(yes(j->data,"AdaptiveRefreshPendingValidation"),"Refreshed adaptive segments require validation before reuse");}

 frame.refresh();expect(!frame.progress.empty()&&frame.progress.front()->GetSafeHwnd(),"Restored existing-download choice opens the real native progress dialog");capture(frame.progress.front()->GetSafeHwnd(),L"progress.png");
 expect(frame.completions.size()==1&&frame.completions.front()->GetSafeHwnd(),"Restored completed-file choice opens the real Download complete dialog");capture(frame.completions.front()->GetSafeHwnd(),L"complete.png");press(frame.completions.front()->GetSafeHwnd(),L"Close");
 auto find=[&](const char* name){for(auto j:manager.jobs)if(str(j->data,"FileName")==name)return j;throw std::runtime_error("Missing expected fixture record");};
 expect(str(find("start.bin")->data,"Status")=="Queued","Restored Start Download queues the original record");
 expect(str(find("later.bin")->data,"Status")=="Paused"&&str(find("cancel.bin")->data,"Status")=="Paused","Restored Later and Cancel leave their original records paused");
 expect(str(find("later.bin")->data,"Queue")=="Deferred fixture queue","Download Later saves the queue selected before collapsing More");
 expect(str(find("refresh.bin")->data,"Url")=="http://127.0.0.1:9/refresh.bin?fresh=1","Restored Save address updates the intended download");
 expect(str(find("refresh-cancel.bin")->data,"Url")=="http://127.0.0.1:9/refresh-cancel.bin?fresh=2","The subsequent refresh applies the newer address to the original record");
 expect(str(find("refresh-save.bin")->data,"Url")=="http://127.0.0.1:9/refresh-save.bin?fresh=2","Reviewing the changed address then saving applies the newer address");
 capture(frame.GetSafeHwnd(),L"main.png");auto counts=std::make_tuple(informationShown,duplicatesShown,refreshShown);frame.refresh();expect(counts==std::make_tuple(informationShown,duplicatesShown,refreshShown),"Further refreshes do not reopen handled modal dialogs");
 prepareReviewFixtures(manager);began=GetTickCount64();timer=SetTimer(nullptr,0,75,tick);frame.refresh();KillTimer(nullptr,timer);if(!failure.empty())throw std::runtime_error(failure);
 expect(reviewsShown==5,"Every changed/deleted target opens its native review once");
 expect(str(find("review-refresh.bin")->data,"Url").find("?changed=1")!=std::string::npos,"Review replacement leaves current address unchanged until the address dialog is saved");
 expect(manager.pendingBrowserCaptureReviews()==Json::array({str(reviewFixtures["review-later.bin"],"token")}),"Later is the only undecided review after the first pass");
 timer=SetTimer(nullptr,0,75,tick);frame.refresh();KillTimer(nullptr,timer);if(!failure.empty())throw std::runtime_error(failure);
 expect(reviewsShown==5&&informationShown==6&&refreshShown==5,"Reviewed new files and replacement open their ordinary dialogs without reopening Later");
 expect(str(find("review-refresh.bin")->data,"Url").find("?fresh=1")!=std::string::npos,"Reviewed replacement Save applies to its original record");
 expect(str(find("review-new.bin")->data,"Status")=="Paused"&&str(find("review-stale.bin")->data,"Status")=="Paused","New-file review Later leaves both new records paused");
 expect(find("review-new.bin")->id()!=str(reviewFixtures["review-new.bin"],"id")&&find("review-stale.bin")->id()!=str(reviewFixtures["review-stale.bin"],"id"),"Deleted original identities are never reused for new files");
 {Manager reopened(manager.root);expect(reopened.pendingBrowserCaptureReviews()==manager.pendingBrowserCaptureReviews(),"Undecided Later review survives reopening its catalog");}
 manager.browserRecoveryRequested=true;revisitReviews=true;timer=SetTimer(nullptr,0,75,tick);frame.refresh();KillTimer(nullptr,timer);if(!failure.empty())throw std::runtime_error(failure);
 expect(reviewsShown==6&&manager.pendingBrowserCaptureReviews().empty(),"Manual Recover reopens Later and its discard resolves the review");
 for(const auto& spec:reviewFixtures)expect(fileHash(fs::path(wide(str(spec,"part"))))==str(spec,"hash"),"Every native review choice preserves original partial bytes");
 expect(manager.pendingBrowserPresentations().empty(),"Review decisions leave no unhandled File Info or refresh dialogs");
 {
  Json offer={{"Id",uint64_t(71)},{"Method","GET"},{"Status",200},{"WasInterceptable",true},{"Host","example.invalid"},{"Target","/uncertain-ui.bin"},{"Length",uint64_t(4)}};
  Json context={{"Session",std::string(32,'b')},{"Generation",uint64_t(1)},{"ResponseAdmissionEnabled",true},{"PendingResponses",Json::array({offer})}};
  JobPtr pending;
  {
   auto reservation=manager.reserveOriginalResponse(context,offer);pending=reservation.job;
   frame.filter="all";frame.refresh();frame.table.SetItemState(-1,0,LVIS_SELECTED);
   auto row=std::find(frame.visible.begin(),frame.visible.end(),pending);
   expect(row!=frame.visible.end(),"Unknown-admission capture appears in the real download list");
   frame.table.SetItemState(static_cast<int>(row-frame.visible.begin()),LVIS_SELECTED|LVIS_FOCUSED,LVIS_SELECTED|LVIS_FOCUSED);
   expect(frame.commandEnabled(CMD_STOP)&&!frame.commandEnabled(CMD_RESUME)&&!frame.commandEnabled(CMD_DELETE),"Unacknowledged capture enables Stop and blocks Resume and Remove");
   expect(!frame.commandEnabled(CMD_RECOVERY)&&!frame.commandEnabled(CMD_MOVEQUEUE),"Pending helper ownership blocks catalog recovery and queue reassignment");
   frame.command(CMD_RESUME);frame.command(CMD_DELETE);
   expect(manager.isActive(pending)&&str(pending->data["OriginalResponse"],"State")=="Reserved","Disabled commands cannot start or remove an uncertain capture");
   frame.command(CMD_STOP);
   expect(yes(pending->data["OriginalResponse"],"PauseRequested")&&str(pending->data,"Status")=="Pausing","Main-window Stop persists pause intent before admission is acknowledged");
   expect(!frame.commandEnabled(CMD_RESUME)&&!frame.commandEnabled(CMD_DELETE),"Pausing retains ownership until the response writer releases its files");
   capture(frame.GetSafeHwnd(),L"uncertain-capture-pausing.png");
  }
  manager.recoverOriginalResponses();frame.refresh();
  expect(!manager.isActive(pending)&&str(pending->data,"Status")=="Paused","Released uncertain capture recovers to an idle paused download");
  expect(frame.commandEnabled(CMD_RESUME)&&frame.commandEnabled(CMD_DELETE)&&!frame.commandEnabled(CMD_STOP),"Recovered download exposes Resume and Remove and disables Stop");
  pending->data["Size"]=100;frame.refresh();
  auto row=std::find(frame.visible.begin(),frame.visible.end(),pending);
  expect(row!=frame.visible.end(),"Size fixture has a visible list row");
  const int index=(int)(row-frame.visible.begin());auto original=pending->data;
  expect(frame.table.EnsureVisible(index,FALSE)!=FALSE,"Size fixture row is in the modal refresh viewport");
  pending->data["ConfirmationPending"]=true;pending->data["Status"]="Merging";
  pending->data["MediaPhase"]="Waiting for confirmation";pending->data["PrefetchedFinalSize"]=12164620;
  const auto presentations=std::make_tuple(informationShown,duplicatesShown,refreshShown);
  frame.refreshing=true;frame.refresh();
  expect(frame.textValue(index,2)==bytes(12164620),"Modal refresh shows verified final size instead of track size");
  expect(frame.refreshing&&presentations==std::make_tuple(informationShown,duplicatesShown,refreshShown),"Modal size refresh preserves the guard and does not reenter presentations");
  pending->data["MediaPhase"]="Assembling file";frame.refresh();
  expect(frame.textValue(index,2)==bytes(100),"Unverified phase cannot display a stale final-size claim");
  pending->data["MediaPhase"]="Waiting for confirmation";pending->data["ConfirmationPending"]=false;frame.refresh();
  expect(frame.textValue(index,2)==bytes(100),"Confirmed download no longer displays a stale prefetch size");
  pending->data=original;frame.refreshing=false;frame.refresh();
 }
 {
  Form combos("Dropdown initialization regression",380,180,&frame);
  combos.init=[&]{
   auto selected=combos.combo({"First","Second"},"Second",10,10,180);
   auto fallback=combos.combo({"First","Second"},"Missing",10,40,180);
   auto editable=combos.combo({"Saved path"},"Custom path",10,70,180,true);
   auto empty=combos.combo({},"",10,100,180);
   expect(selected->GetCount()==2&&selected->GetCurSel()==1,"Batched dropdown retains items and requested selection");
   expect(fallback->GetCurSel()==0,"Batched dropdown retains missing-selection fallback");
   expect(text(editable)=="Custom path","Batched editable dropdown retains unsaved text");
   expect(empty->GetCount()==0&&empty->GetCurSel()==CB_ERR,"Batched empty dropdown stays unselected");
   for(auto combo:{selected,fallback,editable,empty})expect((combo->GetStyle()&WS_VISIBLE)!=0&&!GetPropW(combo->GetSafeHwnd(),L"SysSetRedraw"),"Batched dropdown restores visibility and redraw");
   combos.PostMessage(WM_COMMAND,IDCANCEL);
  };
  combos.DoModal();
 }
 queuedAuthentication(manager,frame);
 recoveryCommands(manager,frame);
 }
};
static std::function<void()> livePrefetchPulse;
static void CALLBACK pollLivePrefetch(HWND,UINT,UINT_PTR,DWORD){if(livePrefetchPulse)livePrefetchPulse();}
static HWND visibleControl(HWND parent,const std::wstring& label){
 for(auto window=GetWindow(parent,GW_CHILD);window;window=GetWindow(window,GW_HWNDNEXT))
  if(IsWindowVisible(window)&&title(window)==label)return window;
 return nullptr;
}
static i64 fixtureEpochMs(){
 return std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
}
static void browserColdFileInfoFixture(const fs::path& root){
 wchar_t tag[40]{};
 if(!GetEnvironmentVariableW(L"UDM_INSTANCE_TAG",tag,40)||!fs::is_regular_file(root/L".udm-live-file-info-fixture"))
  throw std::runtime_error("Expected an isolated tagged File Info fixture.");
 std::vector<std::tuple<const char*,double,i64>> stages;
 std::mutex timingMutex;
 Manager manager(root/L"state");
 if(!manager.jobs.empty()||yes(manager.state["Settings"],"SkipBrowserFileInfo")||!yes(manager.state["Settings"],"PrefetchFileInfo"))
  throw std::runtime_error("Expected an empty catalog with File Info prefetch enabled.");
 auto frame=new MainWindow(manager);
 const auto main=frame->GetSafeHwnd();
 JobPtr job;
 std::atomic<i64> admissionEpoch{0};
 std::atomic<ULONGLONG> admissionTick{0};
 std::atomic<double> admissionClock{0};
 const bool trace=GetEnvironmentVariableW(L"UDM_FILE_INFO_TRACE",nullptr,0)>0;
 auto recordStage=[&](const char* stage){
  std::lock_guard<std::mutex> lock(timingMutex);
  if(stages.size()<64)stages.emplace_back(stage,formClockMs(),fixtureEpochMs());
 };
 if(trace){
  stages.reserve(64);
  capturePresentationTrace=[&manager,recordStage](const char* stage){
   recordStage(stage);
   if(std::string_view(stage)=="first-prefetch-pulse"){
    Lock lock(manager.mutex);
    CapturedResponseTestAccess::saveCheckpoint(manager,[recordStage](const char* catalogStage){
     if(std::string_view(catalogStage)=="temporary-flushed")recordStage("catalog-temporary-flushed");
    });
   }
  };
 }
 i64 paintEpoch=0,exactEpoch=0,bodyEpoch=0,confirmedEpoch=0,shownSize=-1;
 ULONGLONG paintTick=0;
 std::string shownHash;
 const bool observeHeld=GetEnvironmentVariableW(L"UDM_FILE_INFO_OBSERVE_HELD",nullptr,0)>0;
 Json held;
 const auto deadline=GetTickCount64()+(observeHeld?180000:65000);
 const auto inspect=[&](HWND window){
  if(title(window)!=L"Download File Info"||!IsWindowVisible(window)||!IsWindowEnabled(window))return;
  const auto start=visibleControl(window,L"&Start Download");
  if(!start||!IsWindowEnabled(start))return;
  if(!paintEpoch){
   traceCapturePresentation("actionable-observed");
   paintEpoch=fixtureEpochMs();paintTick=GetTickCount64();
   expect(!IsWindowVisible(main)&&GetAncestor(window,GA_ROOTOWNER)!=main,"File Info is actionable while the main list stays hidden");
   expect(!findChild(window,L"Queue"),"First actionable compact File Info has no advanced queue control");
   expect(!findChild(window,L"Use server name"),"Media File Info does not create the ordinary server filename action");
  }
  Json prepared,heldState;
  fs::path target;
  {
   Lock lock(manager.mutex);
   if(!job&&!manager.jobs.empty())job=manager.jobs.front();
   if(!job)return;
   expect(manager.jobs.size()==1,"The browser selection creates exactly one native job");
   if(!bodyEpoch&&num(job->data,"Received")>0)bodyEpoch=fixtureEpochMs();
   target=job->target();
   if(observeHeld&&yes(job->data,"SabrSessionRefreshRequired")&&!manager.isActive(job)){
    heldState={{"retainedBytes",num(job->data,"SabrRetainedBytes")},{"completedMs",num(job->data,"StreamCompletedMs")}};
   }else{
    if(confirmedEpoch||!yes(job->data,"ConfirmationPending")||str(job->data,"MediaPhase")!="Waiting for confirmation")return;
    shownSize=num(job->data,"PrefetchedFinalSize",-1);prepared=job->preparedMedia;
   }
  }
  if(heldState.is_object()){
   const auto refresh=visibleControl(window,L"&Refresh...");
   const auto retained=num(heldState,"retainedBytes");
   const auto line=retained<=0?std::wstring(L"Refresh the media session to continue."):
    num(heldState,"completedMs")<=0?std::wstring(L"No playable progress yet. Refresh to continue."):wide(bytes(retained)+" verified. Refresh to continue.");
   if(!refresh||!IsWindowEnabled(refresh)||!visibleControl(window,line))return;
   expect(!fs::exists(target)&&shownSize<0,"A held session shows refresh without an exact-size claim or a published file");
   heldState["refreshVisibleEpochMs"]=fixtureEpochMs();
   held=std::move(heldState);
   PostMessageW(window,WM_CLOSE,0,0);
   return;
  }
  if(!visibleControl(window,wide("Exact final size: "+std::to_string(shownSize)+" bytes")))return;
  exactEpoch=fixtureEpochMs();shownHash=str(prepared,"Sha256");
  expect(shownSize>0&&shownSize==num(prepared,"Size")&&shownHash.size()==64&&!fs::exists(target),"The compact exact size matches verified staging before Start and the final file is absent");
  capture(window,L"exact-size-before-start.png");
  confirmedEpoch=fixtureEpochMs();PostMessageW(start,BM_CLICK,0,0);
 };
 livePrefetchPulse=[&]{
  try{
   if(GetTickCount64()>deadline)throw std::runtime_error("Cold File Info fixture deadline.");
   EnumThreadWindows(GetCurrentThreadId(),[](HWND window,LPARAM ptr)->BOOL{
    (*reinterpret_cast<const decltype(inspect)*>(ptr))(window);return TRUE;
   },reinterpret_cast<LPARAM>(&inspect));
  }catch(const std::exception& e){
   failure=e.what();
   EnumThreadWindows(GetCurrentThreadId(),[](HWND window,LPARAM)->BOOL{
    if(title(window)==L"Download File Info")PostMessageW(window,WM_CLOSE,0,0);return TRUE;
   },0);
  }
 };
 const auto timer=SetTimer(nullptr,0,15,pollLivePrefetch);
 try{
  PipeServer server(manager,[]{},[&]{
   if(trace){double empty=0;admissionClock.compare_exchange_strong(empty,formClockMs());}
   admissionEpoch.store(fixtureEpochMs());admissionTick.store(GetTickCount64());
   PostMessageW(main,PRESENT_CAPTURE,0,0);
  });
  bool complete=false;
  while(failure.empty()&&!complete&&held.is_null()&&GetTickCount64()<deadline){
   MSG message;
   while(PeekMessageW(&message,nullptr,0,0,PM_REMOVE)){TranslateMessage(&message);DispatchMessageW(&message);}
   {Lock lock(manager.mutex);complete=job&&str(job->data,"Status")=="Complete"&&!manager.isActive(job);}
   if(!complete&&held.is_null())MsgWaitForMultipleObjects(0,nullptr,FALSE,15,QS_ALLINPUT);
  }
  if(!failure.empty())throw std::runtime_error(failure);
  if(held.is_object()){
   Lock lock(manager.mutex);
   expect(yes(job->data,"SabrSessionRefreshRequired")&&num(job->data,"SabrRetainedBytes")==num(held,"retainedBytes")&&!fs::exists(job->target()),"Closing File Info preserves the held checkpoint");
  }else{
   expect(complete&&confirmedEpoch&&bodyEpoch,"The browser download receives bytes before confirmation and completes after Start");
   expect((i64)fs::file_size(job->target())==shownSize&&fileHash(job->target())==shownHash,"Published file matches the exact size and hash verified before Start");
  }
  Json traceResult=Json::array();
  const auto origin=admissionClock.load();
  {std::lock_guard<std::mutex> lock(timingMutex);
   for(const auto& [stage,stamp,wall]:stages)traceResult.push_back({{"stage",stage},{"msAfterAdmission",stamp-origin},{"epochMs",wall}});
  }
  const auto paintDelay=static_cast<i64>(paintTick)-static_cast<i64>(admissionTick.load());
  atomicText(root/L"file-info-result.json",Json{{"status",complete?"Complete":"Held"},{"admissionEpochMs",admissionEpoch.load()},
   {"actionableEpochMs",paintEpoch},{"admissionToActionableMs",paintDelay},{"firstReceivedObservationEpochMs",bodyEpoch},
   {"exactVisibleEpochMs",exactEpoch},{"startEpochMs",confirmedEpoch},{"publishedEpochMs",complete?fixtureEpochMs():0},
   {"sizeBeforeStart",shownSize},{"sha256BeforeStart",shownHash},{"held",held},{"trace",traceResult},
   {"formStages",trace?formInitializationTiming:Json::array()},{"checks",outcomes}}.dump(2),false);
 }catch(...){KillTimer(nullptr,timer);livePrefetchPulse={};capturePresentationTrace={};frame->SendMessage(WM_CLOSE);throw;}
 KillTimer(nullptr,timer);livePrefetchPulse={};capturePresentationTrace={};frame->SendMessage(WM_CLOSE);
}
static void browserPrefetchFixture(const fs::path& root){
 if(!fs::is_regular_file(root/L".udm-live-recovery-fixture"))throw std::runtime_error("Expected an isolated browser fixture.");
 Manager manager(root/L"state");
 if(manager.jobs.size()!=1)throw std::runtime_error("Expected one isolated media job.");
 auto job=manager.jobs.front();const auto media=root/L"state"/L"media"/wide(job->id());
 if(!yes(job->data,"SabrSessionRefreshRequired")||num(job->data,"SabrRetainedBytes")<=0||
    fs::path(wide(str(job->data,"MediaFolder")))!=media||job->target().parent_path()!=root/L"downloads"||
    !job->video||!job->audio||job->video->target().parent_path()!=media||job->audio->target().parent_path()!=media)
  throw std::runtime_error("Expected a held video and owned checkpoint.");
 PipeServer server(manager,[]{});
 const auto retained=num(job->data,"SabrRetainedBytes");
 const auto markerPath=root/L"refresh-page-1.txt";
 formRefreshPageObserver=[&](const std::string& page){
  atomicText(markerPath,Json{{"marker",manager.streamingRefreshMarker(job)},{"page",page}}.dump(),false);
 };
 auto deadline=GetTickCount64()+170000;bool opened=false,saved=false,confirmed=false;
 i64 shownSize=-1;std::string shownHash,shownLabel;
 auto pulse=[&](HWND w){
  const auto caption=title(w);
  if(caption==L"Refresh media session"&&!saved&&fs::exists(root/L"save-and-resume-1")){
   const auto save=visibleControl(w,L"Save session");
   if(save&&IsWindowEnabled(save)){
    expect(!visibleControl(w,L"Save and resume"),"File Info refresh does not offer publication");
    saved=true;fs::remove(markerPath);PostMessageW(save,BM_CLICK,0,0);
   }
  }else if(caption==L"Download File Info"&&IsWindowEnabled(w)){
   if(!opened){
    auto refresh=visibleControl(w,L"&Refresh...");
    if(refresh&&IsWindowEnabled(refresh)){
     const auto line=num(job->data,"StreamCompletedMs")<=0?std::wstring(L"No playable progress yet. Refresh to continue."):wide(bytes(retained)+" verified. Refresh to continue.");
     expect(visibleControl(w,line)!=nullptr,"Compact File Info shows the retained checkpoint and refresh instruction");
     opened=true;PostMessageW(refresh,BM_CLICK,0,0);
    }
   }else if(saved&&!confirmed){
    Json prepared;{Lock lock(manager.mutex);
     if(!yes(job->data,"ConfirmationPending")||str(job->data,"MediaPhase")!="Waiting for confirmation")return;
     shownSize=num(job->data,"PrefetchedFinalSize",-1);prepared=job->preparedMedia;
    }
    shownLabel="Exact final size: "+std::to_string(shownSize)+" bytes";
    if(!visibleControl(w,wide(shownLabel)))return;
    shownHash=str(prepared,"Sha256");
    expect(shownSize>0&&shownSize==num(prepared,"Size")&&shownHash.size()==64&&!fs::exists(job->target()),"Verified exact size is visible before Start and the destination is absent");
    capture(w,L"exact-size-before-start.png");
    confirmed=true;PostMessageW(child(w,L"&Start Download"),BM_CLICK,0,0);
   }
  }
 };
 livePrefetchPulse=[&]{
  try{
   if(GetTickCount64()>=deadline)throw std::runtime_error("File Info prefetch fixture deadline.");
   EnumThreadWindows(GetCurrentThreadId(),[](HWND w,LPARAM ptr)->BOOL{
    (*reinterpret_cast<decltype(pulse)*>(ptr))(w);return TRUE;
   },reinterpret_cast<LPARAM>(&pulse));
  }catch(const std::exception& e){
   failure=e.what();
   EnumThreadWindows(GetCurrentThreadId(),[](HWND w,LPARAM)->BOOL{const auto caption=title(w);if(caption==L"Download File Info"||caption==L"Refresh media session"||caption==L"UDM")PostMessageW(w,WM_CLOSE,0,0);return TRUE;},0);
  }
 };
 auto timer=SetTimer(nullptr,0,80,pollLivePrefetch);
 try{downloadInfo(nullptr,manager,job);}catch(const std::exception& e){failure=e.what();}
 KillTimer(nullptr,timer);livePrefetchPulse={};formRefreshPageObserver={};
 if(!failure.empty())throw std::runtime_error(failure);
 expect(confirmed,"Start follows the visible exact size");
 deadline=GetTickCount64()+60000;
 while(manager.isActive(job)&&GetTickCount64()<deadline){manager.tick();Sleep(100);}
 expect(!manager.isActive(job)&&str(job->data,"Status")=="Complete","Start publishes the verified prefetch");
 expect((i64)fs::file_size(job->target())==shownSize&&fileHash(job->target())==shownHash,"Published file matches the exact bytes and hash verified before Start");
 atomicText(root/L"refresh-result-1.json",Json{{"status","Complete"},{"retainedBefore",retained},{"reused",num(job->data,"SabrReusedBytes")},{"timelineMs",num(job->data,"StreamCompletedMs")},{"sizeBeforeStart",shownSize},{"sizeLabel",shownLabel},{"sha256BeforeStart",shownHash},{"checks",outcomes}}.dump(2),false);
}
static void inspectFileInfoFirstPaint(Manager& manager,const JobPtr& job,const std::function<void(HWND)>& inspect){
 capturePresentationTrace=[&](const char* stage){
  if(std::string_view(stage)!="owned-paint-returned")return;
  EnumThreadWindows(GetCurrentThreadId(),[](HWND window,LPARAM data)->BOOL{
   if(title(window)!=L"Download File Info"||!IsWindowVisible(window))return TRUE;
   (*reinterpret_cast<const std::function<void(HWND)>*>(data))(window);
   PostMessageW(window,WM_CLOSE,0,0);
   return FALSE;
  },reinterpret_cast<LPARAM>(&inspect));
 };
 try{
  CaptureDialogOwner owner(nullptr,true);
  downloadInfo(owner.get(nullptr),manager,job);
 }catch(...){capturePresentationTrace={};throw;}
 capturePresentationTrace={};
}
static void firstPaintSizeFixture(const fs::path& root){
 Manager manager(root/L"state");
 manager.state["Settings"]["PrefetchFileInfo"]=false;
 for(const auto* kind:{"reported","estimated","unknown"}){
  const std::string name=kind;
  Json message={{"action","media"},{"url","https://www.youtube.com/watch?v=Q3TI27IN7X0&fixture="+name},
   {"filename",name+".mp4"},{"height",360},{"formatId","18"},
   {"videoUrl","https://rr1.googlevideo.com/videoplayback?id="+name}};
  const bool reported=std::string_view(kind)=="reported",estimated=std::string_view(kind)=="estimated";
  if(reported)message["reportedSize"]=12345678;
  if(estimated)message["estimatedSize"]=23456789;
  auto job=manager.receive(message);
  expect(job->video&&num(job->video->data,"Size",-1)<0&&num(job->data,"Size",-1)<0,
   "Media fixture has no exact output or known child track length");
  const auto bytesLabel=wide(reported?bytes(12345678):estimated?"~ "+bytes(23456789):bytes(-1));
  const wchar_t* detail=reported?L"Selected tracks: 12345678 bytes":L"Estimated size: 23456789 bytes";
  bool painted=false,amountVisible=false,detailMatches=false,verifiedClaim=false;
  inspectFileInfoFirstPaint(manager,job,[&](HWND window){
   painted=true;
   amountVisible=findChild(window,bytesLabel.c_str())!=nullptr;
   detailMatches=reported||estimated?findChild(window,detail)!=nullptr:
    !findChild(window,L"Selected tracks: 12345678 bytes")&&!findChild(window,L"Estimated size: 23456789 bytes");
   verifiedClaim=findChild(window,L"Exact final size")!=nullptr;
  });
  expect(painted,"The browser-owned File Info completed its first paint");
  expect(amountVisible&&detailMatches&&!verifiedClaim,reported?"First paint labels reported track bytes":
   estimated?"First paint labels an approximate size":"First paint retains unknown size when no hint exists");
 }
}
static void firstPaintNameFixture(const fs::path& root){
 Manager manager(root/L"name-state");
 manager.state["Settings"]["PrefetchFileInfo"]=false;
 manager.state["Settings"]["CategoryFolders"]=false;
 manager.state["Settings"]["DownloadFolder"]=utf8((root/L"downloads").wstring());
 for(const std::string kind:{"reported","matched","response","refresh","replacement"}){
  const auto address="https://example.invalid/"+kind;
  const auto filename=kind+".bin";
  const auto browserName=kind=="matched"?filename:"browser-"+kind+".bin";
  auto job=manager.receive({{"action","add"},{"url",address},{"filename",filename},{"reportedServerName",browserName}});
  expect(str(job->data,"BrowserReportedFileName")==browserName,"Browser admission retains the reported filename");
  auto suggested=browserName;
  if(kind=="response"){
   suggested="verified-response.bin";
   job->data["ConfirmationPending"]=true;
   job->data["ResponseFileNameUrl"]=address;
   job->data["ResponseFileName"]=suggested;
  }else if(kind=="refresh")job->data["RefreshPendingValidation"]=true;
  else if(kind=="replacement")job->data["ReplacementOf"]="previous-download";
  const bool show=kind!="refresh"&&kind!="replacement";
  const bool enable=show&&kind!="matched";
  const auto original=job->target();
  bool seen=false,display=false,enabled=false,unchanged=false,adopted=false;
  inspectFileInfoFirstPaint(manager,job,[&](HWND window){
   seen=true;
   const auto button=findChild(window,L"Use server name");
   const auto category=FindWindowExW(window,nullptr,L"ComboBox",nullptr);
   const auto destination=category?FindWindowExW(window,category,L"ComboBox",nullptr):nullptr;
   display=findChild(window,wide("Server name: "+suggested).c_str())!=nullptr;
   enabled=button&&IsWindowEnabled(button);
   unchanged=destination&&title(destination)==original.wstring();
   if(kind=="reported"&&enabled&&destination){
    SendMessageW(button,BM_CLICK,0,0);
    adopted=title(destination)==(original.parent_path()/wide(suggested)).wstring();
   }
  });
  expect(seen&&unchanged&&job->target()==original,"First paint and an unconfirmed name choice preserve the catalog destination");
  expect(display==show&&enabled==enable,("Initial server filename and adoption state match "+kind).c_str());
  if(kind=="reported")expect(adopted,"Using the first-paint suggestion changes only the Save As draft");
 }
}
class PresentationTestApplication:public CWinApp {
 int code=1;
public:BOOL InitInstance()override{
 CWinApp::InitInstance();INITCOMMONCONTROLSEX controls{sizeof(controls),ICC_WIN95_CLASSES|ICC_DATE_CLASSES|ICC_PROGRESS_CLASS};InitCommonControlsEx(&controls);AfxOleInit();int argc=0;auto argv=CommandLineToArgvW(GetCommandLineW(),&argc);std::unique_ptr<Manager> manager;MainWindow* frame=nullptr;
 if(argc==3&&std::wstring(argv[1])==L"--first-paint-size"){
  output=fs::absolute(argv[2]);
  try{firstPaintSizeFixture(output);firstPaintNameFixture(output);code=0;}catch(const std::exception& e){failure=e.what();}
  atomicText(output/L"first-paint-size.json",Json{{"passed",code==0},{"error",failure},{"checks",outcomes}}.dump(2),false);
  LocalFree(argv);return FALSE;
 }
 if(argc==3&&std::wstring(argv[1])==L"--browser-cold-file-info"){
  output=fs::absolute(argv[2]);
  try{browserColdFileInfoFixture(output);code=0;}catch(const std::exception& e){atomicText(output/L"file-info-result.json",Json{{"status","Failed"},{"error",e.what()},{"checks",outcomes}}.dump(2),false);}
  LocalFree(argv);return FALSE;
 }
 if(argc==3&&std::wstring(argv[1])==L"--browser-prefetch"){
  output=fs::absolute(argv[2]);
  try{browserPrefetchFixture(output);code=0;}catch(const std::exception& e){atomicText(output/L"refresh-result-1.json",Json{{"status","Failed"},{"error",e.what()},{"checks",outcomes}}.dump(2),false);}
  LocalFree(argv);return FALSE;
 }
 try{if(argc!=2)throw std::runtime_error("Pass a fresh isolated output directory");output=argv[1];if(fs::exists(output))throw std::runtime_error("The native fixture requires a fresh directory");fs::create_directories(output);prepareFixtures(output/L"state");manager=std::make_unique<Manager>(output/L"state");frame=new MainWindow(*manager);m_pMainWnd=frame;CapturePresentationComponentTest::run(*manager,*frame);code=0;}catch(const std::exception& e){failure=e.what();}
 if(frame){frame->SendMessage(WM_CLOSE);m_pMainWnd=nullptr;}manager.reset();if(argv)LocalFree(argv);
 if(!output.empty()){Manager reopened(output/L"state");if(code==0)expect(reopened.pendingBrowserPresentations().empty(),"Handled presentations remain consumed after closing and reopening UDM");atomicText(output/L"results.json",Json{{"passed",code==0},{"error",failure},{"checks",outcomes},{"timing",presentationTiming},{"formStages",formInitializationTiming}}.dump(2),false);}return FALSE;
 }
 int ExitInstance()override{AfxOleTerm(FALSE);return code;}
};
PresentationTestApplication application;
}
