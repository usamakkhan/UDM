// Operates only dialogs and controls created by this isolated executable.
#include <afxwin.h>
#include <afxcmn.h>
#include <afxdlgs.h>
#include <afxole.h>
#include <atlimage.h>
#include "Ui.hpp"
#include "BrowserAdmissionContext.hpp"
#include <future>
namespace udm {
struct CatalogCheckpointTestAccess {static void observe(Manager& manager,std::function<void(const char*)> callback){Lock lock(manager.mutex);manager.catalogCheckpoint=std::move(callback);}};
struct ClearDecisionCheckpoint {Manager& manager;std::function<void()> release;~ClearDecisionCheckpoint(){release();CatalogCheckpointTestAccess::observe(manager,{});}};
static Json checks=Json::array();
#ifdef UDM_CAPTURE_PRESENTATION_COMPONENT_TEST
static Json formLifecycle=Json::object(),formClasses=Json::object(),formLayouts=Json::object();
#endif
class PendingTestOwner:public CFrameWnd {void PostNcDestroy()override{}};
static void check(bool ok,const char* name){checks.push_back({{"name",name},{"passed",ok}});if(!ok)throw std::runtime_error(name);}
static std::wstring caption(HWND w){wchar_t text[32768]{};GetWindowTextW(w,text,32768);return text;}
static HWND ownedDialog(const wchar_t* name){struct Match{const wchar_t* name;HWND window=nullptr;}match{name};EnumWindows([](HWND w,LPARAM p)->BOOL{auto& match=*reinterpret_cast<Match*>(p);DWORD pid=0;GetWindowThreadProcessId(w,&pid);if(pid==GetCurrentProcessId()&&IsWindowVisible(w)&&caption(w)==match.name)match.window=w;return TRUE;},reinterpret_cast<LPARAM>(&match));return match.window;}
static HWND findInfo(){return ownedDialog(L"Download File Info");}
static HWND childNamed(HWND window,const wchar_t* label){HWND result=nullptr;for(auto child=GetWindow(window,GW_CHILD);child;child=GetWindow(child,GW_HWNDNEXT))if(caption(child)==label)result=child;return result;}
struct PendingUiCase {
 Manager& manager;CFrameWnd& owner;std::shared_ptr<const PendingFileInfoInbox::Operation> operation;std::promise<void>& release;
 std::string action,error;size_t previous;bool released=false,acted=false,done=false,errorShown=false,retryPressed=false;HWND dialog=nullptr,destinationField=nullptr,descriptionField=nullptr;ULONGLONG deadline=GetTickCount64()+12000;
 std::function<bool()> decisionHeld;std::function<void()> releaseDecision;bool heldChecked=false;
 void unblock(){if(!released){released=true;release.set_value();}}
 void tick(){
  if(done)return;auto current=findInfo();
  if(GetTickCount64()>deadline)throw std::runtime_error("Pending UI test timed out");
  if(!current)return;
  if(!acted){
   dialog=current;check(!IsWindowVisible(owner.GetSafeHwnd()),"Pending File Info keeps its main owner hidden");
#ifdef UDM_CAPTURE_PRESENTATION_COMPONENT_TEST
   for(auto child=GetWindow(current,GW_CHILD);child;child=GetWindow(child,GW_HWNDNEXT)){wchar_t kind[80]{};GetClassNameW(child,kind,80);formClasses[std::to_string(GetDlgCtrlID(child))]=utf8(kind);}
   Json layout=Json::array();for(auto child=GetWindow(current,GW_CHILD);child;child=GetWindow(child,GW_HWNDNEXT)){
    RECT bounds{};GetWindowRect(child,&bounds);MapWindowPoints(nullptr,current,reinterpret_cast<POINT*>(&bounds),2);LOGFONTW font{};auto handle=reinterpret_cast<HFONT>(SendMessageW(child,WM_GETFONT,0,0));GetObjectW(handle,sizeof(font),&font);
    layout.push_back({{"id",GetDlgCtrlID(child)},{"style",static_cast<unsigned long>(GetWindowLongPtrW(child,GWL_STYLE))},{"exStyle",static_cast<unsigned long>(GetWindowLongPtrW(child,GWL_EXSTYLE))},{"rect",{bounds.left,bounds.top,bounds.right,bounds.bottom}},{"fontHeight",font.lfHeight},{"fontWeight",font.lfWeight},{"fontQuality",font.lfQuality},{"fontCharset",font.lfCharSet},{"fontFace",utf8(font.lfFaceName)}});
   }
   Json tabs=Json::array();auto tab=GetNextDlgTabItem(current,nullptr,FALSE);const auto first=tab;for(int count=0;tab&&count<64;++count){tabs.push_back(GetDlgCtrlID(tab));tab=GetNextDlgTabItem(current,tab,FALSE);if(tab==first)break;}
   formLayouts[action]={{"controls",layout},{"tabs",tabs},{"dpi",GetDpiForWindow(current)}};
#endif
   check(manager.snapshot()["Downloads"].size()==previous,"The actual dialog paints while its catalog write is held and its job remains absent from history");
   auto start=childNamed(current,L"&Start Download");check(start&&IsWindowEnabled(start),"The pending dialog exposes an actionable Start button");
   HWND destination=nullptr,description=nullptr;
   struct Fields {const std::wstring& path;HWND& destination;HWND& description;} fields{operation->view->target.native(),destination,description};
   EnumChildWindows(current,[](HWND w,LPARAM p)->BOOL{auto& f=*reinterpret_cast<Fields*>(p);wchar_t kind[80]{};GetClassNameW(w,kind,80);if(!_wcsicmp(kind,L"ComboBox")&&caption(w)==f.path)f.destination=w;if(!_wcsicmp(kind,L"Edit")&&IsWindowVisible(w)&&caption(w).empty())f.description=w;return TRUE;},reinterpret_cast<LPARAM>(&fields));
   check(destination&&description,"Pending File Info creates the real destination and description editors");
   destinationField=destination;descriptionField=description;
   const auto next=operation->view->target.parent_path()/L"pending-draft.mp4";
   SetWindowTextW(destination,next.c_str());SetWindowTextW(description,L"Pending edit survives");
   check(caption(destination)==next.native(),"Destination edits are accepted before durable admission");
   acted=true;
   if(action=="cancel"){
    SendMessageW(childNamed(current,L"&Cancel"),BM_CLICK,0,0);
    check(!released,"Pending Cancel returns to the modal loop without releasing or waiting for the disk write");
    check(manager.snapshot()["Downloads"].size()==previous,"Pending Cancel does not expose or delete a tentative history row");done=true;return;
   }
   if(action.rfind("start",0)==0||action.rfind("later",0)==0){
    SendMessageW(action.rfind("start",0)==0?start:childNamed(current,L"Download &Later"),BM_CLICK,0,0);
    check(IsWindow(current)&&!IsWindowEnabled(start)&&!IsWindowEnabled(destination),"A pending decision freezes one draft and keeps the same dialog alive");
   }
   unblock();return;
  }
  if(action=="start-held"||action=="later-held"){
   if(!heldChecked&&decisionHeld&&decisionHeld()){
    check(manager.fileInfoDecisionPending&&current==dialog,"The UI timer continues dispatching while decision persistence holds the Manager lock");
    check(!IsWindowEnabled(childNamed(current,L"&Start Download"))&&!IsWindowEnabled(childNamed(current,L"&Cancel")),"An in-flight durable decision blocks duplicate Start and conflicting Cancel");
    ::RedrawWindow(current,nullptr,nullptr,RDW_INVALIDATE|RDW_UPDATENOW);
    ::KillTimer(current,Form::pulseTimer);heldChecked=true;releaseDecision();
   }
   return;
  }
  if(action=="retry"){
   if(!retryPressed){auto retry=childNamed(current,L"&Retry");if(!retry||!IsWindowEnabled(retry))return;
    check(operation->finished()&&operation->outcome()==PendingFileInfoInbox::Outcome::Rejected,"Retry becomes actionable only after the rejected producer has finished");
    retryPressed=true;SendMessageW(retry,BM_CLICK,0,0);return;}
   auto accepted=operation->latest();if(accepted==operation||accepted->outcome()!=PendingFileInfoInbox::Outcome::Accepted||!IsWindowEnabled(childNamed(current,L"More >>")))return;
   check(current==dialog&&caption(descriptionField)==L"Pending edit survives"&&caption(destinationField)==(operation->view->target.parent_path()/L"pending-draft.mp4").native(),"A retry binds a new accepted identity to the same window and complete edited draft");
   check(accepted->key!=operation->key&&accepted->view->jobId!=operation->view->jobId&&accepted->token==operation->token,"Retry uses a fresh prepared identity and preserves the original media receipt token");
   done=true;SendMessageW(childNamed(current,L"Download &Later"),BM_CLICK,0,0);return;
  }
  if(action=="rejected"||action=="uncertain"){
   const auto expected=action=="rejected"?L"UDM could not accept this download. Your draft is retained; close this dialog to capture again.":L"UDM could not confirm acceptance. Your draft is retained; check download history before capturing again.";
   if(!childNamed(current,expected))return;
   check(current==dialog&&caption(descriptionField)==L"Pending edit survives"&&caption(destinationField)==(operation->view->target.parent_path()/L"pending-draft.mp4").native(),"Admission outcome reporting retains the same dialog and edited draft");
   check(!IsWindowEnabled(destinationField)&&!IsWindowEnabled(childNamed(current,L"&Start Download")),"An unresolved or rejected admission cannot accidentally submit a second decision");
   check(manager.snapshot()["Downloads"].size()==previous,"Reporting a failed admission does not create another download");
   done=true;SendMessageW(childNamed(current,L"&Cancel"),BM_CLICK,0,0);return;
  }
  if(action.find("-conflict")!=std::string::npos){
   if(!errorShown){
    const auto notice=ownedDialog(L"UDM");if(!notice)return;
    check(childNamed(notice,L"That destination belongs to another download.")!=nullptr,"A queued decision reports its actual backend validation error");
    check(current==dialog&&caption(descriptionField)==L"Pending edit survives"&&caption(destinationField)==(operation->view->target.parent_path()/L"pending-draft.mp4").native(),"A rejected queued decision retains the same window and complete draft");
    errorShown=true;SendMessageW(childNamed(notice,L"OK"),BM_CLICK,0,0);return;
   }
   if(!IsWindowEnabled(current))return;
   check(IsWindowEnabled(destinationField)&&IsWindowEnabled(childNamed(current,L"&Start Download")),"The rejected decision restores editable controls for recovery");
   SetWindowTextW(destinationField,(operation->view->target.parent_path()/L"recovered-draft.mp4").c_str());done=true;
   SendMessageW(childNamed(current,action.rfind("start",0)==0?L"&Start Download":L"Download &Later"),BM_CLICK,0,0);return;
  }
  if(action=="review"&&operation->outcome()==PendingFileInfoInbox::Outcome::Accepted){
   if(!IsWindowEnabled(childNamed(current,L"More >>")))return;
   check(current==dialog,"Acceptance binds the same File Info window without replacement");
   bool path=false,description=false;
   for(auto w=GetWindow(current,GW_CHILD);w;w=GetWindow(w,GW_HWNDNEXT)){path|=caption(w)==(operation->view->target.parent_path()/L"pending-draft.mp4").native();description|=caption(w)==L"Pending edit survives";}
   check(path&&description,"Acceptance preserves the destination and description edited while pending");
   done=true;SendMessageW(childNamed(current,L"&Cancel"),BM_CLICK,0,0);
  }
 }
};
static PendingUiCase* activeCase=nullptr;
static void CALLBACK pendingTimer(HWND,UINT,UINT_PTR,DWORD){if(!activeCase)return;try{activeCase->tick();}catch(const std::exception& e){activeCase->error=e.what();activeCase->done=true;activeCase->unblock();if(activeCase->releaseDecision)activeCase->releaseDecision();if(auto w=findInfo()){::SetTimer(w,Form::pulseTimer,40,nullptr);PostMessageW(w,WM_CLOSE,0,0);}}}
class PendingFileInfoApplication:public CWinApp {
 int result=1;
public:BOOL InitInstance()override{
 CWinApp::InitInstance();INITCOMMONCONTROLSEX common{sizeof(common),ICC_WIN95_CLASSES};InitCommonControlsEx(&common);AfxOleInit();
 int argc=0;auto argv=CommandLineToArgvW(GetCommandLineW(),&argc);fs::path output;std::string error;
 try{
  if(argc!=2)throw std::runtime_error("Pass a fresh private output folder");output=fs::absolute(argv[1]);if(fs::exists(output))throw std::runtime_error("Fresh output required");fs::create_directories(output);
  wchar_t dark[2]{};uiDark=useDarkUi(GetEnvironmentVariableW(L"UDM_TEST_PENDING_DARK",dark,2)==1&&dark[0]==L'1',windowsHighContrast());
  wchar_t conflicts[2]{};const bool conflictsOnly=GetEnvironmentVariableW(L"UDM_TEST_PENDING_CONFLICT_ONLY",conflicts,2)==1&&conflicts[0]==L'1';
  for(const std::string action:{"review","start","later","cancel","start-conflict","later-conflict","rejected","uncertain","retry","start-held","later-held"}){
   const bool conflict=action.find("-conflict")!=std::string::npos;if(conflictsOnly&&!conflict)continue;
   const auto root=output/wide(action);Manager manager(root/L"state");auto prefs=manager.state["Settings"];prefs["DownloadFolder"]=utf8((root/L"files").native());prefs["CategoryFolders"]=false;prefs["PrefetchFileInfo"]=false;prefs["SkipBrowserFileInfo"]=false;prefs["QueuePromptLater"]=false;prefs["Retries"]=0;manager.setSettings(prefs);
   if(action=="cancel")manager.add("http://127.0.0.1:9/unrelated",utf8((root/L"files").native()),"keep.bin","Main queue",true);
   if(conflict)manager.add("http://127.0.0.1:9/owner",utf8((root/L"files").native()),"pending-draft.mp4","Main queue",true);
   const auto previous=manager.jobs.size();
   auto id=guid();for(int pos:{20,16,12,8})id.insert(pos,"-");const auto token=std::to_string(epoch())+"-"+id;
   Json request={{"action","adaptive"},{"mediaToken",token},{"url","http://127.0.0.1:9/watch"},{"filename","pending UI"},{"plan",{{"type","hls"},{"height",360},{"audioExpected",false},{"container","mp4"},{"tracks",Json::array({{{"kind","video"},{"segments",Json::array({{{"url","http://127.0.0.1:9/segment.ts"}}})}}})}}}};
   auto inbox=std::make_shared<PendingFileInfoInbox>();inbox->startWorker([&](const auto& operation){cancelPendingFileInfoOffer(manager,operation);});
   std::promise<void> entered,release;auto ready=entered.get_future();auto released=release.get_future().share();PendingFileInfoInbox::Key key=0;
   std::promise<void> decisionRelease;auto decisionGate=decisionRelease.get_future().share();std::atomic_bool decisionEntered{false},decisionReleased{false};
   const auto releaseDecision=[&]{if(!decisionReleased.exchange(true))decisionRelease.set_value();};ClearDecisionCheckpoint clearDecision{manager,releaseDecision};
   auto retry=action=="retry"?std::make_shared<const PendingFileInfoInbox::RetryHandler>([&](const PreparedFileInfoSink& prepared,const RejectedFileInfoSink& rejected){BrowserAdmissionContext context(true,&prepared,&rejected);auto job=manager.receive(request);Lock lock(manager.mutex);return captureFileInfoInitialView(manager,*job);}):nullptr;
   auto admission=std::async(std::launch::async,[&]{PendingFileInfoInbox::Ticket ticket;const PreparedFileInfoSink sink=[&](auto view){ticket=inbox->publish(std::move(view),[&](auto value){key=value;return true;},token,retry);};const RejectedFileInfoSink rejected=[&](const std::string& id){ticket.rejected(id);};BrowserAdmissionContext context(true,&sink,action=="uncertain"?nullptr:&rejected);auto job=manager.receiveBrowserMedia(request,[&](const char* stage){if(std::string(stage)=="temporary-flushed"){entered.set_value();if(released.wait_for(std::chrono::seconds(12))!=std::future_status::ready)throw std::runtime_error("Pending UI gate expired");if(action=="rejected"||action=="uncertain"||action=="retry")throw std::runtime_error("Forced uncommitted admission failure");}if(std::string(stage)=="committed"&&(action=="start-held"||action=="later-held"))CatalogCheckpointTestAccess::observe(manager,[&](const char* step){if(std::string(step)=="temporary-flushed"&&!decisionEntered.exchange(true)&&decisionGate.wait_for(std::chrono::seconds(8))!=std::future_status::ready)throw std::runtime_error("Decision gate expired");});});ticket.accepted(job->id());return job;});
   if(ready.wait_for(std::chrono::seconds(6))!=std::future_status::ready){release.set_value();admission.get();throw std::runtime_error("Admission did not reach its held write");}
   auto operation=inbox->take(key);check(operation!=nullptr,"An eligible held admission delivers its owned view to the UI");
   PendingTestOwner owner;check(owner.Create(nullptr,L"Private pending File Info owner",WS_OVERLAPPEDWINDOW,CRect(0,0,80,80))!=FALSE,"Create a hidden native dialog owner");m_pMainWnd=&owner;
   PendingUiCase ui{manager,owner,operation,release,action,"",previous};ui.decisionHeld=[&]{return decisionEntered.load();};ui.releaseDecision=releaseDecision;activeCase=&ui;const auto timer=::SetTimer(nullptr,0,20,pendingTimer);
#ifdef UDM_CAPTURE_PRESENTATION_COMPONENT_TEST
   const auto started=formClockMs();capturePresentationTrace=[&,started](const char* stage){formLifecycle[action].push_back({{"stage",stage},{"ms",formClockMs()-started}});};
#endif
   bool bound=false;
   try{CaptureDialogOwner captureOwner(&owner,true);downloadInfo(captureOwner.get(&owner),manager,{},false,true,operation->view,operation,&bound);}
   catch(...){ui.unblock();::KillTimer(nullptr,timer);activeCase=nullptr;admission.get();inbox->close();inbox->stopWorker();m_pMainWnd=nullptr;throw;}
   ::KillTimer(nullptr,timer);activeCase=nullptr;
#ifdef UDM_CAPTURE_PRESENTATION_COMPONENT_TEST
   capturePresentationTrace={};
#endif
   if(action=="cancel"&&ui.error.empty())check(!ui.released&&!IsWindow(ui.dialog),"The pending dialog closes completely while its admission write remains held");
   ui.unblock();JobPtr accepted;bool admissionFailed=false;
   try{accepted=admission.get();}catch(const std::exception& e){if((action!="rejected"&&action!="uncertain"&&action!="retry")||std::string(e.what())!="Forced uncommitted admission failure")throw;admissionFailed=true;}
   if(bound)manager.finishBrowserPresentation(Json{{"tokens",Json::array({token})}});
   if(!ui.error.empty())throw std::runtime_error(ui.error);
   if(action=="start-held"||action=="later-held")check(ui.heldChecked&&!manager.fileInfoDecisionPending,"Decision completion wakes File Info without its polling timer and clears the UI acknowledgement guard");
   if(action=="retry"){
    Lock lock(manager.mutex);check(admissionFailed&&bound&&manager.jobs.size()==1&&str(manager.jobs.front()->data,"FileName")=="pending-draft.mp4"&&str(manager.jobs.front()->data,"Description")=="Pending edit survives"&&str(manager.jobs.front()->data,"Status")=="Paused","The corrected draft is applied once after retry and Download Later stays paused");
   }else if(action=="rejected"||action=="uncertain"){
    check(admissionFailed&&!bound&&operation->outcome()==(action=="rejected"?PendingFileInfoInbox::Outcome::Rejected:PendingFileInfoInbox::Outcome::Uncertain),"The dialog distinguishes a proven rejection from an abandoned acknowledgement");
   }else if(action=="cancel"){
    const auto deadline=GetTickCount64()+6000;while(!operation->cancelFinished&&GetTickCount64()<deadline)Sleep(5);
    check(operation->cancelFinished&&!operation->cancelFailed,"The cancellation worker resolves the accepted pending offer");
    {Lock lock(manager.mutex);check(manager.jobs.size()==previous&&str(manager.jobs.front()->data,"FileName")=="keep.bin","Pending cancellation preserves unrelated downloads");}
   }else if(action.rfind("start",0)==0||action.rfind("later",0)==0){
    Lock lock(manager.mutex);check(bound&&str(accepted->data,"FileName")== (conflict?"recovered-draft.mp4":"pending-draft.mp4")&&str(accepted->data,"Description")=="Pending edit survives","The queued decision applies the complete edited draft after acceptance");
    if(action.rfind("later",0)==0)check(!manager.isActive(accepted)&&str(accepted->data,"Status")=="Paused","Pending Download Later remains paused without starting media transfer");
    if(conflict)check(ui.errorShown&&manager.jobs.size()==previous+1&&str(manager.jobs.front()->data,"FileName")=="pending-draft.mp4","Recovery succeeds without replacing the other download or creating a duplicate");
   }
   inbox->close();inbox->stopWorker();manager.stop();m_pMainWnd=nullptr;owner.DestroyWindow();
  }
  result=0;
 }catch(const std::exception& e){error=e.what();m_pMainWnd=nullptr;}
 if(argv)LocalFree(argv);if(!output.empty()){
  Json record={{"passed",result==0},{"error",error},{"checks",checks},{"darkMode",uiDark}};
#ifdef UDM_CAPTURE_PRESENTATION_COMPONENT_TEST
  record["formStages"]=formInitializationTiming;record["formLifecycle"]=formLifecycle;record["controlClasses"]=formClasses;record["formLayouts"]=formLayouts;
#endif
  atomicText(output/L"results.json",record.dump(2),false);
 }return FALSE;
 }
 int ExitInstance()override{AfxOleTerm(FALSE);return result;}
};PendingFileInfoApplication pendingApplication;
}
