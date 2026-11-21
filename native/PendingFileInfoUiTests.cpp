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
struct CancellationRecoveryCase {
 Manager& manager;CFrameWnd& owner;PendingFileInfoInbox& inbox;std::shared_ptr<const PendingFileInfoInbox::Operation> operation;std::string action,error;std::atomic_bool& held;std::function<void()> release;bool clicked=false,done=false;ULONGLONG deadline=GetTickCount64()+12000;
 void tick(){
  if(done)return;if(GetTickCount64()>deadline)throw std::runtime_error("Cancellation recovery UI timed out");auto window=ownedDialog(L"Cancellation failed");if(!window)return;
  auto retry=childNamed(window,L"&Retry cancellation"),close=childNamed(window,L"&Close");
  if(!clicked){check(!IsWindowVisible(owner.GetSafeHwnd()),"Cancellation recovery keeps the main owner hidden");check(retry&&close&&IsWindowEnabled(retry)&&childNamed(window,L"Cancellation could not be confirmed."),"The failure dialog explains the unresolved outcome and offers explicit Retry");check(childNamed(window,L"Owned cancellation persistence failure")!=nullptr,"The recovery dialog shows the actual saved failure detail");clicked=true;if(action=="close"){done=true;SendMessageW(close,BM_CLICK,0,0);return;}SendMessageW(retry,BM_CLICK,0,0);check(IsWindow(window)&&!IsWindowEnabled(retry)&&!IsWindowEnabled(close),"Retry disables duplicate decisions without closing the recovery dialog");return;}
  if(action=="success"){
   if(!held)return;check(manager.fileInfoDecisionPending&&!operation->cancelFinished,"The recovery timer dispatches while cancellation persistence holds the Manager lock");SendMessageW(window,WM_CLOSE,0,0);check(IsWindow(window),"Window close cannot abandon an in-flight cancellation retry");RedrawWindow(window,nullptr,nullptr,RDW_INVALIDATE|RDW_UPDATENOW);done=true;release();return;
  }
  if(IsWindowEnabled(retry)&&operation->cancelFinished&&operation->cancelFailed){auto failure=inbox.cancellationFailure(operation);if(failure.attempt!=1)return;check(childNamed(window,L"Cancellation could not be confirmed.")!=nullptr,"A repeated persistence failure remains visibly unresolved");check(!manager.fileInfoDecisionPending,"A failed retry releases the UI decision guard");done=true;SendMessageW(close,BM_CLICK,0,0);}
 }
};
static CancellationRecoveryCase* activeRecovery=nullptr;
static void CALLBACK recoveryTimer(HWND,UINT,UINT_PTR,DWORD){if(!activeRecovery)return;try{activeRecovery->tick();}catch(const std::exception& e){activeRecovery->error=e.what();activeRecovery->release();activeRecovery->done=true;if(auto window=ownedDialog(L"Cancellation failed"))EndDialog(window,IDCANCEL);}}
static void cancellationRecoveryChecks(const fs::path& output,CWinApp& application){
 for(const std::string action:{"success","again","close"}){
  const auto root=output/wide("cancel-recovery-"+action);Manager manager(root/L"state");auto prefs=manager.state["Settings"];prefs["DownloadFolder"]=utf8((root/L"files").native());prefs["PrefetchFileInfo"]=false;prefs["SkipBrowserFileInfo"]=false;manager.setSettings(prefs);
  auto id=guid();for(int pos:{20,16,12,8})id.insert(pos,"-");const auto token=std::to_string(epoch())+"-"+id;
  Json request={{"action","adaptive"},{"mediaToken",token},{"url","http://127.0.0.1:9/watch"},{"filename","Cancellation recovery fixture"},{"plan",{{"type","hls"},{"height",360},{"audioExpected",false},{"container","mp4"},{"tracks",Json::array({{{"kind","video"},{"segments",Json::array({{{"url","http://127.0.0.1:9/segment.ts"}}})}}})}}}};
  PendingFileInfoInbox inbox;inbox.startWorker([&](const auto& operation){cancelPendingFileInfoOffer(manager,operation);});PendingFileInfoInbox::Key key=0;
  {PendingFileInfoInbox::Ticket ticket;const PreparedFileInfoSink prepared=[&](auto view){ticket=inbox.publish(std::move(view),[&](auto value){key=value;return true;},token);};const RejectedFileInfoSink rejected=[&](const auto& value){ticket.rejected(value);};BrowserAdmissionContext context(true,&prepared,&rejected);std::string failure;try{manager.receiveBrowserMedia(request,[](const char* stage){if(std::string(stage)=="temporary-flushed")throw std::runtime_error("Owned initial recovery failure");});}catch(const std::exception& e){failure=e.what();}check(failure=="Owned initial recovery failure","Recovery UI setup reaches a real rejected admission");}
  auto operation=inbox.take(key);CatalogCheckpointTestAccess::observe(manager,[](const char* stage){if(std::string(stage)=="temporary-flushed")throw std::runtime_error("Owned cancellation persistence failure");});operation->requestCancel();const auto end=GetTickCount64()+6000;while(!operation->cancelFinished&&GetTickCount64()<end)Sleep(5);auto failure=inbox.cancellationFailure();check(failure&&operation->cancelFailed,"A real cancellation write failure feeds the recovery UI");
  std::promise<void> release;auto gate=release.get_future().share();std::atomic_bool held{false},released{false};const auto unblock=[&]{if(!released.exchange(true))release.set_value();};ClearDecisionCheckpoint cleanup{manager,unblock};
  if(action=="success")CatalogCheckpointTestAccess::observe(manager,[&](const char* stage){if(std::string(stage)=="temporary-flushed"){held=true;if(gate.wait_for(std::chrono::seconds(8))!=std::future_status::ready)throw std::runtime_error("Owned recovery write gate expired");}});
  PendingTestOwner owner;check(owner.Create(nullptr,L"Private cancellation recovery owner",WS_OVERLAPPEDWINDOW,CRect(0,0,80,80))!=FALSE,"Create the hidden cancellation recovery owner");application.m_pMainWnd=&owner;CancellationRecoveryCase ui{manager,owner,inbox,operation,action,"",held,unblock};activeRecovery=&ui;const auto timer=SetTimer(nullptr,0,20,recoveryTimer);bool resolved=false;
  try{resolved=presentCancellationFailure(&owner,manager,inbox,failure);}catch(...){unblock();KillTimer(nullptr,timer);activeRecovery=nullptr;application.m_pMainWnd=nullptr;inbox.close();inbox.stopWorker();owner.DestroyWindow();throw;}
  KillTimer(nullptr,timer);activeRecovery=nullptr;unblock();check(ui.done&&ui.error.empty(),"Cancellation recovery completes its actual modal interaction");check(!manager.fileInfoDecisionPending&&!IsWindowVisible(owner.GetSafeHwnd()),"Recovery releases its decision guard without showing the main window");
  if(action=="success")check(resolved&&!operation->cancelFailed&&!inbox.cancellationFailure()&&str(manager.state.at("BrowserCaptures").at(token),"status")=="released","Successful UI Retry closes only after the cancellation barrier is saved");
  else check(!resolved&&operation->cancelFailed&&!inbox.cancellationFailure()&&!manager.state.value("BrowserCaptures",Json::object()).contains(token),"Closing an unresolved failure acknowledges the notice without claiming cancellation or creating a release");
  CatalogCheckpointTestAccess::observe(manager,{});inbox.close();inbox.stopWorker();manager.stop();application.m_pMainWnd=nullptr;owner.DestroyWindow();
 }
}
struct PendingUiCase {
 Manager& manager;CFrameWnd& owner;std::shared_ptr<const PendingFileInfoInbox::Operation> operation;std::promise<void>& release;
 std::string action,error;size_t previous;bool released=false,acted=false,done=false,errorShown=false,retryPressed=false;HWND dialog=nullptr,destinationField=nullptr,descriptionField=nullptr;ULONGLONG deadline=GetTickCount64()+12000;
 std::function<bool()> decisionHeld;std::function<void()> releaseDecision;bool heldChecked=false;
 std::function<bool()> beforeRetry,afterRetry,beforeCancel;
 std::string boundIdentity;
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
  if(action.rfind("retry",0)==0){
   if(!retryPressed){auto retry=childNamed(current,L"&Retry");if(!retry||!IsWindowEnabled(retry))return;
    if(beforeRetry&&!beforeRetry())return;
    check(operation->finished()&&operation->outcome()==PendingFileInfoInbox::Outcome::Rejected,"Retry becomes actionable only after the rejected producer has finished");
    retryPressed=true;SendMessageW(retry,BM_CLICK,0,0);return;}
   if(afterRetry&&!afterRetry())return;
   auto accepted=operation->latest();if(accepted==operation||accepted->outcome()!=PendingFileInfoInbox::Outcome::Accepted||!IsWindowEnabled(childNamed(current,L"More >>")))return;
   boundIdentity=accepted->view->jobId;
   check(current==dialog&&caption(descriptionField)==L"Pending edit survives"&&caption(destinationField)==(operation->view->target.parent_path()/L"pending-draft.mp4").native(),"A retry binds a new accepted identity to the same window and complete edited draft");
   check(accepted->key!=operation->key&&accepted->view->jobId!=operation->view->jobId&&accepted->token==operation->token,"Retry uses a fresh prepared identity and preserves the original media receipt token");
   done=true;SendMessageW(childNamed(current,L"Download &Later"),BM_CLICK,0,0);return;
  }
  if(action=="rejected"||action=="uncertain"||action=="cancel-late-notified"){
   const auto expected=action!="uncertain"?L"UDM could not accept this download. Your draft is retained; close this dialog to capture again.":L"UDM could not confirm acceptance. Your draft is retained; check download history before capturing again.";
   if(!childNamed(current,expected))return;
   if(beforeCancel&&!beforeCancel())return;
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
  if((action=="review"||action=="review-metadata")&&operation->outcome()==PendingFileInfoInbox::Outcome::Accepted){
   if(!IsWindowEnabled(childNamed(current,L"More >>")))return;
   check(current==dialog,"Acceptance binds the same File Info window without replacement");if(action=="review-metadata"){Lock lock(manager.mutex);bool refreshed=false;for(const auto& job:manager.jobs)if(job->id()==operation->view->jobId)refreshed=num(job->data,"Size")==654321;check(refreshed,"The rendered dialog binds after real accepted size and preference refreshes");}
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
  for(const std::string action:{"review","review-metadata","start","later","cancel","start-conflict","later-conflict","rejected","uncertain","retry","retry-join","retry-join-rejected","retry-notified","retry-notified-rejected","retry-notified-cancel","cancel-late-notified","start-held","later-held"}){
   const bool conflict=action.find("-conflict")!=std::string::npos;if(conflictsOnly&&!conflict)continue;
   const auto root=output/wide(action);Manager manager(root/L"state");auto prefs=manager.state["Settings"];prefs["DownloadFolder"]=utf8((root/L"files").native());prefs["CategoryFolders"]=false;prefs["PrefetchFileInfo"]=false;prefs["SkipBrowserFileInfo"]=false;prefs["QueuePromptLater"]=false;prefs["Retries"]=0;manager.setSettings(prefs);
   if(action=="cancel")manager.add("http://127.0.0.1:9/unrelated",utf8((root/L"files").native()),"keep.bin","Main queue",true);
   if(conflict)manager.add("http://127.0.0.1:9/owner",utf8((root/L"files").native()),"pending-draft.mp4","Main queue",true);
   const auto previous=manager.jobs.size();
   auto id=guid();for(int pos:{20,16,12,8})id.insert(pos,"-");const auto token=std::to_string(epoch())+"-"+id;
   Json request={{"action","adaptive"},{"mediaToken",token},{"url","http://127.0.0.1:9/watch"},{"filename","pending UI"},{"plan",{{"type","hls"},{"height",360},{"audioExpected",false},{"container","mp4"},{"tracks",Json::array({{{"kind","video"},{"segments",Json::array({{{"url","http://127.0.0.1:9/segment.ts"}}})}}})}}}};
   const bool lateCancellation=action=="cancel-late-notified";std::promise<void> cancelObserving;auto cancelObserved=cancelObserving.get_future();
   auto inbox=std::make_shared<PendingFileInfoInbox>();inbox->startWorker([&](const auto& operation){if(lateCancellation)manager.cancelBrowserMediaPresentation(operation.token,operation.fingerprint(),[&](const char* stage){if(std::string(stage)=="cancel-owner-waiting")cancelObserving.set_value();});else cancelPendingFileInfoOffer(manager,operation);});
   std::promise<void> entered,release;auto ready=entered.get_future();auto released=release.get_future().share();PendingFileInfoInbox::Key key=0;
   const bool retryAction=action.rfind("retry",0)==0,notifiedAction=action.rfind("retry-notified",0)==0||lateCancellation,joinedAction=action.rfind("retry-join",0)==0||notifiedAction;
   std::promise<void> decisionRelease,competingEntered,competingRelease,competingPrepared,publishRelease;auto competingReady=competingEntered.get_future();auto preparedReady=competingPrepared.get_future();auto publishGate=publishRelease.get_future().share();auto competingGate=competingRelease.get_future().share();std::future<JobPtr> competing;
   auto decisionGate=decisionRelease.get_future().share();std::atomic_bool decisionEntered{false},decisionReleased{false},competingReleased{false},joinSeen{false},publishReleased{false};std::atomic_int notifications{0};
   const auto releaseCompeting=[&]{if(!competingReleased.exchange(true))competingRelease.set_value();};
   const auto releasePublish=[&]{if(!publishReleased.exchange(true))publishRelease.set_value();};
   const auto releaseDecision=[&]{if(!decisionReleased.exchange(true))decisionRelease.set_value();releasePublish();releaseCompeting();};ClearDecisionCheckpoint clearDecision{manager,releaseDecision};
   auto retry=retryAction?std::make_shared<const PendingFileInfoInbox::RetryHandler>([&](const PreparedFileInfoSink& prepared,const RejectedFileInfoSink& rejected){BrowserAdmissionContext context(true,&prepared,&rejected,true);auto job=manager.receiveBrowserMedia(request,[&](const char* phase){if(std::string(phase)=="join-waiting")joinSeen=true;});Lock lock(manager.mutex);return captureFileInfoInitialView(manager,*job);}):nullptr;
   auto admission=std::async(std::launch::async,[&]{PendingFileInfoInbox::Ticket ticket;const PreparedFileInfoSink sink=[&](auto view){ticket=inbox->publish(std::move(view),[&](auto value){++notifications;key=value;return true;},token,retry);};const RejectedFileInfoSink rejected=[&](const std::string& id){ticket.rejected(id);};BrowserAdmissionContext context(true,&sink,action=="uncertain"?nullptr:&rejected);auto job=manager.receiveBrowserMedia(request,[&](const char* stage){if(std::string(stage)=="temporary-flushed"){entered.set_value();if(released.wait_for(std::chrono::seconds(12))!=std::future_status::ready)throw std::runtime_error("Pending UI gate expired");if(action=="rejected"||action=="uncertain"||retryAction||lateCancellation)throw std::runtime_error("Forced uncommitted admission failure");}if(std::string(stage)=="committed"&&(action=="start-held"||action=="later-held"))CatalogCheckpointTestAccess::observe(manager,[&](const char* step){if(std::string(step)=="temporary-flushed"&&!decisionEntered.exchange(true)&&decisionGate.wait_for(std::chrono::seconds(8))!=std::future_status::ready)throw std::runtime_error("Decision gate expired");});});if(action=="review-metadata"){Lock lock(manager.mutex);job->data["Size"]=654321;job->data["FormatDescription"]="Refreshed 360p MP4";manager.state["Settings"]["SaveFolderHistory"]=Json::array({"C:/refreshed-history"});}ticket.accepted(job->id());return job;});
   if(ready.wait_for(std::chrono::seconds(6))!=std::future_status::ready){release.set_value();admission.get();throw std::runtime_error("Admission did not reach its held write");}
   auto operation=inbox->take(key);check(operation!=nullptr,"An eligible held admission delivers its owned view to the UI");
   PendingTestOwner owner;check(owner.Create(nullptr,L"Private pending File Info owner",WS_OVERLAPPEDWINDOW,CRect(0,0,80,80))!=FALSE,"Create a hidden native dialog owner");m_pMainWnd=&owner;
   PendingUiCase ui{manager,owner,operation,release,action,"",previous};ui.decisionHeld=[&]{return decisionEntered.load();};ui.releaseDecision=releaseDecision;activeCase=&ui;const auto timer=::SetTimer(nullptr,0,20,pendingTimer);
   if(joinedAction){
    ui.beforeRetry=[&]{if(!competing.valid())competing=std::async(std::launch::async,[&]{PendingFileInfoInbox::Ticket ticket;const PreparedFileInfoSink prepared=[&](auto view){competingPrepared.set_value();if(publishGate.wait_for(std::chrono::seconds(12))!=std::future_status::ready)throw std::runtime_error("Competing notification gate expired");ticket=inbox->publish(std::move(view),[&](auto){++notifications;return true;},token,retry);};const RejectedFileInfoSink rejected=[&](const auto& id){ticket.rejected(id);};BrowserAdmissionContext context(true,notifiedAction?&prepared:nullptr,notifiedAction?&rejected:nullptr);try{auto job=manager.receiveBrowserMedia(request,[&](const char* stage){if(std::string(stage)=="temporary-flushed"){competingEntered.set_value();if(competingGate.wait_for(std::chrono::seconds(12))!=std::future_status::ready)throw std::runtime_error("Competing UI admission gate expired");if(action=="retry-join-rejected"||action=="retry-notified-rejected")throw std::runtime_error("Owned competing rejection");}});ticket.accepted(job->id());return job;}catch(const std::exception& e){if((action=="retry-join-rejected"||action=="retry-notified-rejected")&&std::string(e.what())=="Owned competing rejection")return JobPtr{};throw;}});return (notifiedAction?preparedReady:competingReady).wait_for(std::chrono::seconds(0))==std::future_status::ready;};
    ui.afterRetry=[&]{if(competingReleased)return true;if(!joinSeen)return false;if(notifiedAction){releasePublish();if(competingReady.wait_for(std::chrono::seconds(0))!=std::future_status::ready)return false;check(notifications==1&&operation->latest()!=operation,"The second producer publishes into the same rendered presentation without another notification");}check(findInfo()==ui.dialog&&caption(ui.descriptionField)==L"Pending edit survives"&&caption(ui.destinationField)==(operation->view->target.parent_path()/L"pending-draft.mp4").native(),"The same rendered dialog and draft survive a held same-token browser/Retry join");check(!IsWindowVisible(owner.GetSafeHwnd())&&manager.snapshot()["Downloads"].size()==previous,"The join leaves the main window hidden and tentative history absent while UI timers dispatch");if(action=="retry-notified-cancel"){ui.boundIdentity=operation->latest()->view->jobId;ui.done=true;SendMessageW(childNamed(findInfo(),L"&Cancel"),BM_CLICK,0,0);return false;}releaseCompeting();return false;};
   }
   if(lateCancellation)ui.beforeCancel=ui.beforeRetry;
#ifdef UDM_CAPTURE_PRESENTATION_COMPONENT_TEST
   const auto started=formClockMs();capturePresentationTrace=[&,started](const char* stage){formLifecycle[action].push_back({{"stage",stage},{"ms",formClockMs()-started}});};
#endif
   bool bound=false;
   try{CaptureDialogOwner captureOwner(&owner,true);downloadInfo(captureOwner.get(&owner),manager,{},false,true,operation->view,operation,&bound);}
   catch(...){ui.unblock();releaseDecision();::KillTimer(nullptr,timer);activeCase=nullptr;try{admission.get();}catch(...){}if(competing.valid())try{competing.get();}catch(...){}inbox->close();inbox->stopWorker();m_pMainWnd=nullptr;throw;}
   ::KillTimer(nullptr,timer);activeCase=nullptr;
#ifdef UDM_CAPTURE_PRESENTATION_COMPONENT_TEST
   capturePresentationTrace={};
#endif
   if(action=="cancel"&&ui.error.empty())check(!ui.released&&!IsWindow(ui.dialog),"The pending dialog closes completely while its admission write remains held");
   if((action=="retry-notified-cancel"||lateCancellation)&&ui.error.empty())check(!competingReleased&&!IsWindow(ui.dialog),"Cancel closes the single dialog while the second producer's write is still held");
   ui.unblock();JobPtr accepted;bool admissionFailed=false;
   try{accepted=admission.get();}catch(const std::exception& e){if((action!="rejected"&&action!="uncertain"&&!retryAction&&!lateCancellation)||std::string(e.what())!="Forced uncommitted admission failure")throw;admissionFailed=true;}
   if(lateCancellation&&ui.error.empty()){check(cancelObserved.wait_for(std::chrono::seconds(6))==std::future_status::ready,"The closed dialog cancellation owns the late producer before its notification");releasePublish();check(competingReady.wait_for(std::chrono::seconds(6))==std::future_status::ready&&notifications==1&&operation->latest()->cancelRequested()&&!findInfo()&&!IsWindowVisible(owner.GetSafeHwnd()),"A producer inside the cancellation callback opens no new dialog or main window");ui.boundIdentity=operation->latest()->view->jobId;}
   releasePublish();releaseCompeting();if(competing.valid()){auto winner=competing.get();check(action=="retry-join-rejected"||action=="retry-notified-rejected"?!winner:winner&&ui.boundIdentity==winner->id(),"Rendered File Info binds the competing winner, or the explicit retry after proven rejection");}
   if(bound)manager.finishBrowserPresentation(Json{{"tokens",Json::array({token})}});
   if(!ui.error.empty())throw std::runtime_error(ui.error);
   if(action=="start-held"||action=="later-held")check(ui.heldChecked&&!manager.fileInfoDecisionPending,"Decision completion wakes File Info without its polling timer and clears the UI acknowledgement guard");
   if(action=="retry-notified-cancel"||lateCancellation){
    const auto deadline=GetTickCount64()+6000;while(!operation->cancelFinished&&GetTickCount64()<deadline)Sleep(5);check(operation->cancelFinished&&!operation->cancelFailed&&!bound,"Cancellation resolves the notified successor without binding or reopening the original dialog");{Lock lock(manager.mutex);check(manager.jobs.size()==previous&&notifications==1,"A cancelled two-producer handoff leaves no download or extra window notification");}
   }else if(retryAction){
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
  cancellationRecoveryChecks(output,*this);result=0;
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
