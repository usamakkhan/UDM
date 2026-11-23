#include "Version.hpp"
#include "HelpContent.hpp"
#include "GlobalLimiter.hpp"
#include "RecoveryAvailability.hpp"
#include <afxwin.h>
#include <afxcmn.h>
#include <afxdlgs.h>
#include <afxole.h>
#include <atlimage.h>
#include <shlwapi.h>
#include <regex>
#include "Core.hpp"
#include "ColumnSettingsTask.hpp"
#include "CompletionSettingsTask.hpp"
#include "ShellFolderTask.hpp"
#include "CompletionReadinessTask.hpp"
#include "CompletionModalFence.hpp"
#include "CompletionClaimTask.hpp"
#include "CompletionCountdown.hpp"
#include "QueueCompletionCountdown.hpp"
#include <deque>
#include "GlobalLimitSettingsTask.hpp"
#include "AppearanceSettingsTask.hpp"
#include "UiDesktopAttestation.hpp"
#include "MainClosePolicyTask.hpp"
#include "MainExitLayoutTask.hpp"
#include "DownloadMenuTask.hpp"
#define UDM_DOWNLOAD_MENU_PREPARATION
#ifdef UDM_FINISH_TIMING_TEST
#include "ProductionUiTiming.hpp"
#define UDM_FINISH_POINT(stage) nativeUiTraceRecord(stage)
#else
#define UDM_FINISH_POINT(stage) ((void)0)
#endif

#ifdef UDM_PRODUCTION_UI_TIMING_TEST
#include "ProductionUiTiming.hpp"
#endif
#include "DownloadListSnapshot.hpp"
#include "DownloadListOrder.hpp"
#include "DownloadListStatusOrder.hpp"
#include "DownloadListEtaOrder.hpp"
#include "DownloadListSizeOrder.hpp"
#include "NavigationTreeSnapshot.hpp"
#include "DownloadListTooltip.hpp"
#include "ModalSizeRefresh.hpp"
#include "HistoryReadSnapshot.hpp"
#include "ModelMaintenance.hpp"
#include "AutomaticPromptSnapshot.hpp"
#include "CommandAvailability.hpp"
#include "AsyncFileStatus.hpp"
#include "PendingFileInfoInbox.hpp"
#include "IdmTextSort.hpp"
#include "HttpRepresentation.hpp"
#include "AsyncFileIcons.hpp"
#include "QueueMembership.hpp"
#include "QueueCompletion.hpp"
#include "CategoryEditor.hpp"
#include "LaunchFailure.hpp"
#include "CliCompletion.hpp"
#include "RecoverySession.hpp"
#include "RecoveryUi.hpp"
namespace udm {
inline constexpr wchar_t mainWindowTitle[]=L"UDM " UDM_VERSION_WSTRING;
inline i64 displayedFileSize(const Json& data){
  if(yes(data,"ConfirmationPending")&&str(data,"Status")=="Merging"&&str(data,"MediaPhase")=="Waiting for confirmation"){
   const auto verified=num(data,"PrefetchedFinalSize",-1);if(verified>=0)return verified;
  }
  return httpOutputSize(data);
 }
}
#include "Ui.hpp"
#include "MenuBarTheme.hpp"
#include "UpdateUi.hpp"
#include "ShareUi.hpp"
#include "AboutUi.hpp"
#include "RemovalUi.hpp"
#include "WorkflowsUi.hpp"
#include "SystemActions.hpp"
#include "ZipPreview.hpp"
#include "Catalog.hpp"
#include "ExportUi.hpp"
#include "AutomationUi.hpp"
#include "AutomationServer.hpp"
#include "Udm.Automation_i.c"
#include "IdmImportUi.hpp"
#include "ClipboardPolicy.hpp"
#include "ClipboardSnapshot.hpp"
#include "SortKeyCache.hpp"
#include <string_view>
#include <unordered_map>
#include "ToolbarImages.hpp"
#include "ToolbarImageCache.hpp"
#include "QueueIndicators.hpp"
#include "TrayIcon.hpp"
#include <shellapi.h>
#include <fstream>
namespace udm {
enum : UINT { CMD_ADD=200,CMD_RESUME,CMD_STOP,CMD_STOPALL,CMD_DELETE,CMD_CLEAN,CMD_OPTIONS,CMD_SCHEDULER,CMD_STARTQUEUE,CMD_STOPQUEUE,CMD_GRABBER,CMD_BATCH,CMD_IMPORT,CMD_EXPORT,CMD_EXIT,CMD_PROPERTIES,CMD_OPEN,CMD_FOLDER,CMD_NETWORK,CMD_BROWSER,CMD_ABOUT,CMD_NEWQUEUE,CMD_NEWCATEGORY,CMD_UP,CMD_DOWN,CMD_PASTE,CMD_PROGRESS,CMD_MOVEQUEUE,CMD_SEARCH,CMD_REFRESH_ADDRESS,CMD_OPENWITH,CMD_RELOCATE,CMD_REDOWNLOAD,CMD_DOUBLE_OPEN,CMD_DOUBLE_PROPERTIES,CMD_COLUMNS,CMD_DARK,CMD_FONT,CMD_TRAY_COLOR,CMD_TRAY_SYSTEM,CMD_TRAY_HIDE,CMD_FINDNEXT,CMD_HIDE_CATEGORIES,CMD_REMOVEQUEUE,CMD_TOOLBAR,CMD_EDITCATEGORY,CMD_DELETECATEGORY,CMD_DELETEQUEUE,CMD_QUICKFILTER,CMD_STARTQUEUEMENU,CMD_STOPQUEUEMENU,CMD_SELECTALL,CMD_LIMIT,CMD_BASKET,CMD_RECYCLE,CMD_ZIP,CMD_CLIPBOARD_CAPTURE,CMD_TOOLBAR_SMALL,CMD_TOOLBAR_MEDIUM,CMD_TOOLBAR_LARGE,CMD_TOOLBAR_ICONSTEXT,CMD_TOOLBAR_ICONSONLY,CMD_TOOLBAR_TEXTONLY,CMD_TOOLBAR_HIDE,CMD_TOOLBAR_BUILTIN,CMD_TOOLBAR_LOAD,CMD_TOOLBAR_FOLDER,CMD_TOOLBAR_RELOAD,CMD_TOOLBAR_NEON,CMD_TOOLBAR_3D,CMD_IMPORT_IDM,CMD_IMPORT_TEXT,CMD_EXPORT_IDM,CMD_EXPORT_TEXT,CMD_BATCH_CLIPBOARD,CMD_RECOVERY,CMD_SORT_ADDED,CMD_SORT_NAME,CMD_SORT_SIZE,CMD_SORT_STATUS,CMD_SORT_TIME,CMD_SORT_RATE,CMD_SORT_LAST,CMD_SORT_DESCRIPTION,CMD_SORT_PATH,CMD_SORT_REFERER,CMD_SORT_PAGE,CMD_LIMIT_ON,CMD_LIMIT_OFF,CMD_FONT_RESET,CMD_STOP_CLOSE_ALL,CMD_LANGUAGE_ENGLISH,CMD_HELP_CONTENTS,CMD_HELP_TUTORIALS,CMD_HELP_SCHEDULER,CMD_HELP_GRABBER,CMD_TIP_DAY,CMD_CHECK_UPDATES,CMD_HOME,CMD_SUPPORT,CMD_SHARE,CMD_TOOLBAR_DISCOVER };
constexpr UINT SHOW_APP=WM_APP+20,TRAY_MESSAGE=WM_APP+21,DROP_URL=WM_APP+22,INITIAL_REFRESH=WM_APP+23,PRESENT_CAPTURE=WM_APP+24,UPDATE_RESULT=WM_APP+25,UPDATE_LAUNCHED=WM_APP+26,COMMAND_FILE_REFRESH=WM_APP+27,PREPARED_FILE_INFO=WM_APP+28;
constexpr UINT CMD_HIDE_BASKET=CMD_TOOLBAR_DISCOVER+1;
constexpr UINT CMD_EDIT_DOWNLOAD_SCHEDULE=CMD_HIDE_BASKET+1;
constexpr UINT CMD_ADD_SCHEDULED_DOWNLOAD=CMD_EDIT_DOWNLOAD_SCHEDULE+1;
constexpr UINT CMD_SCHEDULE_EXISTING_DOWNLOAD=CMD_ADD_SCHEDULED_DOWNLOAD+1;
constexpr UINT TRANSFER_EVENTS=WM_APP+31,REFRESH_VISIBLE_HISTORY=WM_APP+29;
constexpr UINT DISPATCH_QUEUE_WORK=WM_APP+30,CANCELLATION_FAILED=WM_APP+32;
constexpr bool uniqueMainWakeups(){constexpr UINT ids[]={SHOW_APP,TRAY_MESSAGE,DROP_URL,INITIAL_REFRESH,PRESENT_CAPTURE,UPDATE_RESULT,UPDATE_LAUNCHED,COMMAND_FILE_REFRESH,PREPARED_FILE_INFO,TRANSFER_EVENTS,REFRESH_VISIBLE_HISTORY,DISPATCH_QUEUE_WORK,CANCELLATION_FAILED,UiTaskSignal::message};for(size_t i=0;i<std::size(ids);++i)for(size_t j=0;j<i;++j)if(ids[i]==ids[j])return false;return true;}
static_assert(uniqueMainWakeups(),"Each main-window wake-up message must have a unique ID.");
class DropTarget:public COleDropTarget {
 CWnd* owner;
 static CLIPFORMAT urlFormat(){static const auto format=(CLIPFORMAT)RegisterClipboardFormatW(L"UniformResourceLocatorW");return format;}
 static bool accepts(COleDataObject* data){const auto format=urlFormat();return (format&&data->IsDataAvailable(format))||data->IsDataAvailable(CF_UNICODETEXT);}
 static std::wstring dropText(COleDataObject* data,CLIPFORMAT format){
  if(!format)return {};
  HGLOBAL buffer=data->GetGlobalData(format);if(!buffer)return {};
  struct LockedText {
   HGLOBAL buffer;
   const wchar_t* text;
   ~LockedText(){if(text)GlobalUnlock(buffer);GlobalFree(buffer);}
  } locked{buffer,static_cast<const wchar_t*>(GlobalLock(buffer))};
  if(!locked.text)return {};
  const size_t capacity=GlobalSize(buffer)/sizeof(wchar_t),length=wcsnlen_s(locked.text,std::min<size_t>(capacity,32768));
  return length<capacity&&length<32768?std::wstring(locked.text,length):std::wstring{};
 }
public:explicit DropTarget(CWnd* w):owner(w){}
 DROPEFFECT OnDragEnter(CWnd*,COleDataObject* data,DWORD,CPoint)override{return accepts(data)?DROPEFFECT_COPY:DROPEFFECT_NONE;}
 DROPEFFECT OnDragOver(CWnd*,COleDataObject* data,DWORD,CPoint)override{return accepts(data)?DROPEFFECT_COPY:DROPEFFECT_NONE;}
 BOOL OnDrop(CWnd*,COleDataObject* data,DROPEFFECT,CPoint)override{
  auto text=dropText(data,urlFormat());if(text.empty())text=dropText(data,CF_UNICODETEXT);
  if(text.empty())return FALSE;
  auto value=std::make_unique<std::string>(utf8(text));
  if(value->empty()||!owner->PostMessage(DROP_URL,0,reinterpret_cast<LPARAM>(value.get())))return FALSE;
  value.release();return TRUE;
 }
};
#include "BasketUi.hpp"
class MainWindow:public CFrameWnd {
 DECLARE_MESSAGE_MAP()
#ifdef UDM_TOOLBAR_COMPONENT_TEST
 friend class ToolbarComponentTest;
#endif
#ifdef UDM_CAPTURE_PRESENTATION_COMPONENT_TEST
 friend class CapturePresentationComponentTest;
#endif
 int MessageBox(LPCTSTR message,LPCTSTR title=nullptr,UINT flags=MB_OK){return themedMessage(this,message,title?title:L"UDM",flags);}
 static void tickDesktopModel(Manager& owner){Lock lock(owner.mutex);if(!owner.fileInfoDecisionPending)owner.tick();}
 static void dispatchDesktopModel(Manager& owner){Lock lock(owner.mutex);if(!owner.fileInfoDecisionPending)owner.dispatchQueueWork();}
 AppearanceSettingsTask appearanceCommandTask;bool appearanceClosePending=false,appearanceToolbarEnabled=true;std::vector<BOOL> appearanceToolbarState;std::optional<std::vector<int>> appearanceToolbarRollback;
 GlobalLimitSettingsTask globalLimitCommandTask;bool globalLimitClosePending=false;std::vector<BOOL> globalLimitToolbarState;
 MainClosePolicyTask closePolicyTask;
 MainExitLayoutTask exitLayoutTask;bool exitLayoutPrepared=false,exitLayoutOwnerEnabled=true,exitLayoutRestorePending=false;
#ifdef UDM_TOOLBAR_COMPONENT_TEST
 #define UDM_COMPLETION_MODAL_OWNER_AVAILABLE
 #define UDM_COMPLETION_SECONDARY_MODAL_AVAILABLE
 #define UDM_MANUAL_EXIT_SECONDARY_AVAILABLE
 #define UDM_AUTOMATIC_PRESENTATION_AVAILABLE
 #define UDM_COMPLETION_CLOSE_PRIORITY_AVAILABLE
 #define UDM_COMPLETION_ACTION_QUEUE_TEST_AVAILABLE
 std::function<void(const fs::path&)> shellFolderLauncher;std::function<void(const ShellFolderResult&)> shellFolderErrorPresenter;
 std::function<void(const Json&)> automaticBrowserPresenter;std::function<void(const std::shared_ptr<const PendingFileInfoInbox::Operation>&)> automaticPreparedPresenter;
 std::function<void()> completionShutdownObserver;
 std::function<void(const char*)> exitLayoutObserver;std::function<void(const char*)> appearanceCommandObserver;
 std::function<void(const char*)> browserIntegrationObserver;std::function<void(const char*)> columnSettingsObserver;std::function<void(const char*)> globalLimitSettingsObserver;std::function<void(const char*)> globalLimitCommandObserver;
 std::function<void(const char*,const fs::path&)> completionActionObserver;
 std::function<void(const std::string&,bool,const std::string&)> queueCompletionPerformer;
#endif
 DownloadMenuTask downloadMenuTask;
 struct DownloadMenuAdvice {std::shared_ptr<const std::map<UINT,bool>> current;};
 std::shared_ptr<DownloadMenuAdvice> downloadMenuAdvice=std::make_shared<DownloadMenuAdvice>();
 struct DownloadMenuRequest {uint64_t generation;CPoint point;std::vector<JobPtr> selection;bool requireActive=false;};
 std::optional<DownloadMenuRequest> downloadMenuRequest;std::optional<DownloadMenuView> preparedDownloadMenu;
 uint64_t downloadMenuGeneration=0,downloadMenuReadGeneration=0;
#ifdef UDM_TOOLBAR_COMPONENT_TEST
 std::function<void(CMenu&,CPoint)> downloadMenuPresenter;
#endif
 Manager& manager;std::shared_ptr<UiTaskSignal> maintenanceSignal=std::make_shared<UiTaskSignal>();ModelMaintenance modelMaintenance;BrowserPresentationAcks presentationAcks{[this](const Json& offer){manager.finishBrowserPresentation(offer);},maintenanceSignal->notifier()};bool presentationRecoveryShowing=false;std::unique_ptr<NetworkCaptureSession> networkCapture;uint64_t networkErrorRevision=0;CMenu menu;MenuBarTheme menuTheme;CFont font;CTreeCtrl tree;ThemeList table;ThemeStatusBar status;CategoryPaneFrame categoryFrame;CStatic categoryHeading;PanelCloseButton categoryClose{mainDarkSurface};CEdit search;CStatic searchLabel;CButton searchFind,searchSettings;PanelCloseButton searchClose;
 static constexpr UINT SearchSubmit=505,SearchSettings=506,SearchClose=507;
 CButton clipboard;CImageList images;std::map<std::string,int> iconIndex;
 // These caches follow visible's row order; refresh owns all list text and icon updates.
 std::vector<std::string> filters;std::vector<JobPtr> visible;std::vector<std::array<std::string,12>> rowText;std::vector<std::array<int,2>> rowImages;std::vector<std::shared_ptr<const DownloadListRow>> rowSnapshots;std::map<JobPtr,std::shared_ptr<const DownloadListRow>> listRecordCache;std::vector<bool> rowQuota;Json rowPreferences,rowView;bool rowAdded=false,rowPath=false,rowReferer=false,rowRecovery=false;std::string rowTree;std::vector<std::unique_ptr<Progress>> progress;std::set<std::string> infoSeen,captureReviewsSeen;
 // Update check: one background fetch at a time; the result arrives as UPDATE_RESULT.
 struct UpdateOutcome{UpdateManifest manifest;std::string error;};
 std::unique_ptr<UpdateOutcome> deferredUpdateResult;
 std::thread updateThread;std::shared_ptr<Cancel> updateCancel;bool updateChecking=false,updateInteractive=false;
 // A downloaded setup is verified off the UI thread; the result arrives as UPDATE_LAUNCHED.
 struct UpdateLaunch{std::string error;fs::path verified;};std::thread updateLaunchThread;bool updateLaunching=false;
 enum class TransferEvent { Started, Published, Finished };
 std::mutex eventsMutex;std::vector<std::pair<JobPtr,TransferEvent>> events;
 std::set<std::string> presentedPublications;
 enum class CompletionRequestKind {Dialog,Complete,Finished,Started};
 struct CompletionRequest {JobPtr job;CompletionRequestKind kind;bool requested=false,publication=false;};
 std::deque<CompletionRequest> pendingCompletions;CompletionModalFence completionModalFence;CompletionReadinessTask completionReadiness;bool completionDraining=false;
 std::deque<QueueCompletionCountdownAction> pendingQueueActions;
 std::function<void()> completionCountdownWake;
#ifdef UDM_TOOLBAR_COMPONENT_TEST
 #define UDM_COMPLETION_COUNTDOWN_WAKE_AVAILABLE
#endif
 CompletionSettingsTask completionSettingsTask;bool completionSettingsClosePending=false;
 CompletionClaimTask completionClaimTask;JobPtr completionClaimJob;std::deque<JobPtr> pendingCompletionClaims;bool completionClaimReview=false;
 struct CompletionPreferenceView {Form* dialog;CWnd* checkbox;bool confirmed;};
 std::weak_ptr<CompletionPreferenceView> completionSettingsSource;
 std::shared_ptr<PendingFileInfoInbox> preparedFileInfo=std::make_shared<PendingFileInfoInbox>();
 std::map<std::string,std::shared_ptr<const PendingFileInfoInbox::Operation>> preparedFileInfoViews;
 bool cancellationRecoveryShowing=false;
 std::vector<AutomaticConfirmation> automaticConfirmations;
 std::optional<std::vector<BrowserBatchRow>> browserBatchPresentation;
 std::unique_ptr<AuthenticationPromptTask> authenticationTask=std::make_unique<AuthenticationPromptTask>(manager);bool authenticationShowing=false;
 ResumeSelectionTask resumeTask;std::vector<JobPtr> resumePending,resumeActive;std::vector<ResumeRequestView> resumeFollowups;std::optional<ResumeContentDecision> resumeDecision;bool resumePresenting=false;
#ifdef UDM_TOOLBAR_COMPONENT_TEST
 std::function<void(const char*)> resumeObserver;std::function<std::optional<ResumeContentDecision>(const ResumeRequestView&)> resumeContentPresenter;
#endif
 std::unique_ptr<RequestedAuthenticationTask> requestedAuthenticationTask=std::make_unique<RequestedAuthenticationTask>(manager);std::vector<JobPtr> requestedAuthenticationRequests;
#ifdef UDM_TOOLBAR_COMPONENT_TEST
 std::function<void(const RequestedAuthenticationViewPtr&)> requestedAuthenticationPresenter;
#endif
#ifdef UDM_TOOLBAR_COMPONENT_TEST
 std::function<void(const AutomaticConfirmation&)> automaticConfirmationPresenter;
 std::function<void(const std::string&)> automaticReviewPresenter;
 std::function<void(const JobPtr&)> automaticAuthenticationPresenter;
 std::function<void(const std::vector<BrowserBatchRow>&)> automaticBatchPresenter;
#endif
 NOTIFYICONDATAW tray{};DropTarget drop{this};bool recoveryOnClose=false,quitting=false,refreshing=false,captureRefreshRequested=false,clipboardPromptPending=false;float scale=1;std::string filter="all",copiedUrl,lastClipboard;int sortColumn=-1;bool ascending=true;std::string lastTree;bool treeRebuilding=false;HTREEITEM treeSelection=nullptr;std::map<std::string,std::string> observedStates;std::unique_ptr<Basket> basket;std::vector<std::unique_ptr<Form>> completions;bool handlingQueueAction=false;std::vector<std::pair<JobPtr,Json>> fileActions;QueueDropTarget tableDrop,treeDrop;JobPtr draggedJob;
 ClipboardSnapshot clipboardSnapshot;SortKeyCache refererKeyCache;DownloadListOrder listOrderCache;
 bool historyRefreshDeferred=false,historyRefreshPosted=false,automationReviewDeferred=false;
 NavigationTreeSnapshot treeSnapshot;bool treeSnapshotValid=false;std::string treeFilter;
 AsyncFileStatus commandFiles;bool commandFileRefreshPending=false;
 std::map<HTREEITEM,std::string> treeKeys;std::map<std::string,HTREEITEM> treeItems;
 std::vector<HTREEITEM> treeQueueNodes;std::vector<int> treeQueueImages;
 int px(int n)const{return (int)(n*scale);}
 int windowDpi()const{return (int)std::lround(scale*96);}
 std::vector<double> dpiColumnWidths;std::vector<int> dpiRenderedWidths;
 std::vector<JobPtr> selected(bool validateDeferred=true){
  std::vector<JobPtr> jobs;
  if(!table.GetSafeHwnd())return jobs;
  for(int i=-1;(i=table.GetNextItem(i,LVNI_SELECTED))>=0;)if(i<(int)visible.size())jobs.push_back(visible[i]);
  if(validateDeferred&&historyRefreshDeferred&&!jobs.empty()){
   Lock lock(manager.mutex);
   jobs.erase(std::remove_if(jobs.begin(),jobs.end(),[&](const JobPtr& job){
    return std::find(manager.jobs.begin(),manager.jobs.end(),job)==manager.jobs.end();
   }),jobs.end());
  }
  return jobs;
 }
 static bool downloadQueueMember(const Json& data){return downloadListQueueMember(data);}
 static std::string queueColumnText(const Json& data){return downloadQueueMember(data)?str(data,"Queue"):"";}
 std::array<int,8> queueImages{};
 void rebuildListImages(){
  CImageList next;if(!next.Create(px(16),px(16),ILC_COLOR32|ILC_MASK,20,8))throw std::runtime_error("Cannot create category images.");
  std::map<std::string,int> indices;std::array<int,8> queues{};CClientDC screen(this);
  for(auto key:{"folder","all","archives","documents","music","programs","video","images","other","complete","queue","grabber"}){
   CDC dc;dc.CreateCompatibleDC(&screen);CBitmap bitmap;bitmap.CreateCompatibleBitmap(&screen,px(16),px(16));auto old=dc.SelectObject(&bitmap);dc.FillSolidRect(0,0,px(16),px(16),RGB(255,0,255));
   CImage image;if(SUCCEEDED(image.Load((appDir()/L"assets"/(wide(key)+L".png")).c_str())))image.Draw(dc.m_hDC,0,0,px(16),px(16));
   else DrawIconEx(dc.m_hDC,0,0,AfxGetApp()->LoadIcon(1),px(16),px(16),0,nullptr,DI_NORMAL);
   dc.SelectObject(old);int index=next.Add(&bitmap,RGB(255,0,255));if(index<0)throw std::runtime_error("Cannot install category image.");indices[key]=index;
  }
  for(int kind=0;kind<8;++kind){CBitmap bitmap;bitmap.Attach(queueIndicatorBitmap(screen,px(16),kind));queues[kind]=next.Add(&bitmap,RGB(255,0,255));if(queues[kind]<0)throw std::runtime_error("Cannot install queue indicator.");}
  if(tree.GetSafeHwnd())tree.SetImageList(&next,TVSIL_NORMAL);if(table.GetSafeHwnd())table.SetImageList(&next,LVSIL_SMALL);
  images.DeleteImageList();images.Attach(next.Detach());iconIndex=std::move(indices);fileIconLoader.invalidate();fileTypeImages.clear();queueImages=queues;treeQueueImages.assign(treeQueueNodes.size(),-1);rowSnapshots.assign(visible.size(),std::shared_ptr<const DownloadListRow>{});rowImages.assign(visible.size(),std::array<int,2>{-1,-1});
 }
 std::string cachedListTooltip(int row)const{return refreshing||row<0||static_cast<size_t>(row)>=rowSnapshots.size()?std::string{}:downloadListTooltip(rowSnapshots[row],treeSnapshot);}
 std::string selectedQueue(){return filter.rfind("queue:",0)==0?filter.substr(6):"Main queue";}
 void runPrimaryDownloadQueue(bool start){
  Lock lock(manager.mutex);
  const auto name=primaryDownloadQueueName(manager.state["Queues"]);
  if(name.empty())return;
  if(start)manager.changeQueueRunState(name,true);else manager.queueRun(name,false);
 }
 std::string clipboardAddress(){
  auto value=clipboardText();
  if(!value.empty())try{Url parsed(value);}catch(const std::exception&){value.clear();}
  return value;
 }
 std::string clipboardText(bool* readable=nullptr){if(readable)*readable=false;if(!OpenClipboard())return {};std::string value;bool valid=true;auto h=GetClipboardData(CF_UNICODETEXT);if(h){auto p=(const wchar_t*)GlobalLock(h);if(p){auto max=GlobalSize(h)/sizeof(wchar_t),n=wcsnlen_s(p,max);if(n<max&&n<1024*1024)value=utf8(std::wstring(p,n));GlobalUnlock(h);}else valid=false;}else if(IsClipboardFormatAvailable(CF_UNICODETEXT))valid=false;CloseClipboard();if(readable)*readable=valid;return trim(value);}
 static int arrangeColumn(UINT id){static constexpr int columns[]={8,0,2,3,4,5,6,7,9,10,11};return id>=CMD_SORT_ADDED&&id<=CMD_SORT_PAGE?columns[id-CMD_SORT_ADDED]:-1;}
 bool commandCanResume(JobPtr job){
  Lock lock(manager.mutex);
  return downloadResumeAvailable(job->data,manager.isActive(job),job->prefetchDeferring);
 }
 bool ownsResumeRequest(const JobPtr& job)const{
  return std::find(resumePending.begin(),resumePending.end(),job)!=resumePending.end()||std::find(resumeActive.begin(),resumeActive.end(),job)!=resumeActive.end()||
   std::any_of(resumeFollowups.begin(),resumeFollowups.end(),[&](const ResumeRequestView& view){return view.job==job;})||(resumeDecision&&resumeDecision->view.job==job)||
   std::find(requestedAuthenticationRequests.begin(),requestedAuthenticationRequests.end(),job)!=requestedAuthenticationRequests.end();
 }
 void resumeSelectedDownloads(const std::vector<JobPtr>& chosen){
  if(quitting||resumePresenting)return;for(const auto& job:chosen)if(job&&!ownsResumeRequest(job))resumePending.push_back(job);pollResumeCommands();
 }
 std::optional<ResumeContentDecision> presentResumeContent(const ResumeRequestView& view){
#ifdef UDM_TOOLBAR_COMPONENT_TEST
  if(resumeObserver)resumeObserver("before-content-presentation");
  if(resumeContentPresenter)return resumeContentPresenter(view);
#endif
  ResumeContentDecision decision;decision.view=view;
  if(view.contentPrefix){
   const auto choice=MessageBox(cs("The file starts with "+std::to_string(view.prefixBytes)+" bytes of web page text before the actual content.\n\nYes: remove that text and save only the file.\nNo: keep the data exactly as received."),L"Web page text before the file",MB_YESNOCANCEL|MB_ICONQUESTION);
   if(choice==IDCANCEL)return {};decision.stripPrefix=choice==IDYES;
  }
  if(view.webPage){
   const auto choice=MessageBox(L"The server sent a web page or text instead of the expected file.\n\nYes: save the received content as the file anyway.\nNo: try the download again.",L"Web page received",MB_YESNOCANCEL|MB_ICONQUESTION);
   if(choice==IDCANCEL)return {};decision.allowWebPage=choice==IDYES;
  }
  return decision;
 }
 void pollResumeCommands(){
  if(resumePresenting)return;
  if(resumeTask.active()){
   if(!resumeTask.ready())return;
   try{auto result=resumeTask.take();resumeActive.clear();for(auto& request:result.requests)resumeFollowups.push_back(std::move(request));if(!result.warning.empty())showStatus(result.warning);}
   catch(...){resumeActive.clear();throw;}
  }
  if(quitting||authenticationDialogShowing()||!automaticPresentationAllowed()||refreshing||optionsUiActive||optionsRefreshPending||optionsAppearanceSnapshot||manager.fileInfoDecisionPending)return;
  if(resumeDecision){
   resumeActive={resumeDecision->view.job};
   try{
#ifdef UDM_TOOLBAR_COMPONENT_TEST
    resumeTask.confirm(manager,*resumeDecision,maintenanceSignal->notifier(),resumeObserver);
#else
    resumeTask.confirm(manager,*resumeDecision,maintenanceSignal->notifier());
#endif
    resumeDecision.reset();
   }catch(...){resumeActive.clear();if(!manager.fileInfoDecisionPending)resumeDecision.reset();throw;}return;
  }
  if(!resumeFollowups.empty()){
   const auto view=resumeFollowups.front();const auto current=tryResumeRequestCurrent(manager,view);if(current==AutomaticPromptReadiness::Busy)return;
   if(current==AutomaticPromptReadiness::Ready&&view.kind==ResumeRequestKind::Duplicate&&view.duplicate){const auto duplicateReady=tryDuplicatePromptCurrent(manager,*view.duplicate);if(duplicateReady==DuplicateViewReadiness::Busy)return;if(duplicateReady==DuplicateViewReadiness::Obsolete){resumeFollowups.erase(resumeFollowups.begin());maintenanceSignal->notifier()();return;}}
   if(current==AutomaticPromptReadiness::Ready){
    resumePresenting=true;struct PresentingGuard{bool& value;~PresentingGuard(){value=false;}} presenting{resumePresenting};
    try{
     if(view.kind==ResumeRequestKind::Login)enqueueRequestedAuthentication(view.job);
     else if(view.kind==ResumeRequestKind::Duplicate)presentDownload(this,manager,view.job,false,{},view.duplicate);
     else if(view.kind==ResumeRequestKind::Refresh){if(!refreshDownloadAddress(this,manager,view.job)){resumeFollowups.clear();return;}}
     else if(view.kind==ResumeRequestKind::Content)resumeDecision=presentResumeContent(view);
    }catch(...){resumeFollowups.erase(resumeFollowups.begin());throw;}
   }
   resumeFollowups.erase(resumeFollowups.begin());maintenanceSignal->notifier()();return;
  }
  if(!resumePending.empty()){
   resumeActive=std::move(resumePending);resumePending.clear();
   try{
#ifdef UDM_TOOLBAR_COMPONENT_TEST
    resumeTask.start(manager,resumeActive,maintenanceSignal->notifier(),resumeObserver);
#else
    resumeTask.start(manager,resumeActive,maintenanceSignal->notifier());
#endif
   }catch(...){resumePending=std::move(resumeActive);resumeActive.clear();throw;}
  }
 }
 bool commandCanStop(JobPtr job){Lock lock(manager.mutex);return downloadStopAvailable(job->data,manager.isActive(job));}
 void closeStoppedProgress(const std::set<std::string>& stopped){
  for(auto& p:progress)if(p->GetSafeHwnd()&&stopped.count(p->downloadId()))p->requestCompletionClose();
  std::lock_guard<std::mutex> lock(eventsMutex);
  events.erase(std::remove_if(events.begin(),events.end(),[&](const auto& event){return event.second==TransferEvent::Started&&stopped.count(event.first->id());}),events.end());
 }
 std::vector<std::string> membershipQueues(const std::vector<JobPtr>& selection){
  Lock lock(manager.mutex);bool completed=std::any_of(selection.begin(),selection.end(),[](const JobPtr& j){return str(j->data,"Status")=="Complete";});
  std::vector<std::string> names;for(const auto& q:manager.state["Queues"])if(!completed||yes(q,"Synchronize"))names.push_back(str(q,"Name"));return names;
 }
 static bool commandUsesSelection(UINT id){
  switch(id){case CMD_RESUME:case CMD_STOP:case CMD_DELETE:case CMD_ZIP:case CMD_RECYCLE:case CMD_PROPERTIES:case CMD_PROGRESS:case CMD_FOLDER:case CMD_REMOVEQUEUE:case CMD_MOVEQUEUE:case CMD_UP:case CMD_DOWN:case CMD_REFRESH_ADDRESS:case CMD_EDIT_DOWNLOAD_SCHEDULE:case CMD_SCHEDULE_EXISTING_DOWNLOAD:case CMD_REDOWNLOAD:case CMD_OPENWITH:case CMD_RELOCATE:case CMD_OPEN:return true;default:return false;}
 }
 static bool commandUsesManager(UINT id){
  return (id>=32000&&id<34000)||(commandUsesSelection(id)&&id!=CMD_PROPERTIES&&id!=CMD_PROGRESS&&id!=CMD_FOLDER)||id==CMD_RECOVERY||id==CMD_STOP_CLOSE_ALL||id==CMD_STOPALL||id==CMD_CLEAN;
 }
 struct CommandGlobalNeeds {bool recovery=false,stop=false,close=false,clean=false;bool any()const{return recovery||stop||close||clean;}};
 struct CommandGlobalState {bool active=false,stoppable=false,unfinishedWindow=false,cleanable=false;};
 // Caller holds Manager::mutex. Open windows were identified on the UI thread;
 // downloadId only reads the job ID and sends no window messages.
 CommandGlobalState commandGlobalStateLocked(const CommandGlobalNeeds& needs,const std::vector<Progress*>& windows){
  CommandGlobalState value;std::set<std::string> openIds;for(auto* window:windows)openIds.insert(window->downloadId());
  for(const auto& job:manager.jobs){const bool active=manager.isActive(job);const auto state=str(job->data,"Status");
   value.active=value.active||active;value.stoppable=value.stoppable||active||state=="Queued";
   value.cleanable=value.cleanable||(state=="Complete"&&!active);
   if(needs.close&&!value.stoppable&&!value.unfinishedWindow&&state!="Complete"&&!openIds.empty())value.unfinishedWindow=openIds.count(job->id())!=0;
   if((!needs.recovery||value.active)&&(!needs.stop||value.stoppable)&&(!needs.close||value.stoppable||value.unfinishedWindow)&&(!needs.clean||value.cleanable))break;
  }
  return value;
 }
 bool canOpenDownloadLocked(const JobPtr& job){
  return completedFileOpenReady(job->data,manager.isActive(job)||manager.isRelocating(job));
 }
 std::vector<fs::path> completedDragPaths(const std::vector<JobPtr>& chosen){
  Lock lock(manager.mutex);
  std::vector<fs::path> paths;
  for(const auto& job:chosen)if(canOpenDownloadLocked(job))paths.push_back(job->target());
  return paths;
 }
 CommandAvailability commandAvailabilityLocked(UINT id,const std::vector<JobPtr>& chosen,const CommandGlobalState& global){
  CommandAvailability value;
  if(id>=32000&&id<34000){const bool start=id<33000;const auto& names=start?startQueueNames:stopQueueNames;const auto index=id-(start?32000:33000);value.enabled=index<names.size()&&namedQueueActionAvailable(names[index],start);return value;}
  auto any=[&](auto predicate){return std::any_of(chosen.begin(),chosen.end(),predicate);};
  auto all=[&](auto predicate){return !chosen.empty()&&std::all_of(chosen.begin(),chosen.end(),predicate);};
  switch(id){
   case CMD_RECOVERY:value.enabled=!global.active;break;
   case CMD_RESUME:value.enabled=any([&](JobPtr job){return commandCanResume(job);});break;
   case CMD_STOP:value.enabled=any([&](JobPtr job){return commandCanStop(job);});break;
   case CMD_DELETE:value.enabled=all([&](JobPtr job){return !manager.isActive(job);});break;
   case CMD_ZIP:value.enabled=chosen.size()==1&&str(chosen[0]->data,"Status")=="Complete";if(value.enabled){value.zipOnly=true;value.regularFiles.push_back(chosen[0]->target());}break;
   case CMD_RECYCLE:value.enabled=all([&](JobPtr job){return !manager.isActive(job)&&str(job->data,"Status")=="Complete";});if(value.enabled)for(const auto& job:chosen)value.regularFiles.push_back(job->target());break;
   case CMD_REMOVEQUEUE:value.enabled=all([&](JobPtr job){return !manager.isActive(job)&&yes(job->data,"QueueMember",str(job->data,"Status")!="Complete");});break;
   case CMD_MOVEQUEUE:{value.enabled=all([&](JobPtr job){return !manager.isActive(job);});if(value.enabled){const bool complete=any([&](JobPtr job){return str(job->data,"Status")=="Complete";});const auto& queues=manager.state["Queues"];value.enabled=std::any_of(queues.begin(),queues.end(),[&](const Json& queue){return !complete||yes(queue,"Synchronize");});}break;}
   case CMD_UP:case CMD_DOWN:value.enabled=chosen.size()==1&&!manager.isActive(chosen[0])&&yes(chosen[0]->data,"QueueMember",str(chosen[0]->data,"Status")!="Complete");break;
   case CMD_EDIT_DOWNLOAD_SCHEDULE:value.enabled=chosen.size()==1&&manager.editableDownloadSchedule(chosen[0]).has_value();break;
   case CMD_SCHEDULE_EXISTING_DOWNLOAD:value.enabled=chosen.size()==1&&manager.canScheduleExistingDownload(chosen[0]);break;
   case CMD_REFRESH_ADDRESS:value.enabled=chosen.size()==1&&manager.canRefreshAddress(chosen[0]);break;
   case CMD_REDOWNLOAD:{value.enabled=chosen.size()==1&&!manager.isActive(chosen[0]);if(value.enabled){const auto state=str(chosen[0]->data,"Status");if(state!="Complete"){value.enabled=state=="Paused"||state=="Failed";if(value.enabled){value.ftpOnly=true;value.url=str(chosen[0]->data,"Url");}}}break;}
   case CMD_RELOCATE:value.enabled=all([&](JobPtr job){return !manager.isActive(job)&&str(job->data,"Status")=="Complete";});break;
   case CMD_OPENWITH:case CMD_OPEN:value.enabled=chosen.size()==1&&canOpenDownloadLocked(chosen[0]);break;
   case CMD_STOP_CLOSE_ALL:value.enabled=global.stoppable||global.unfinishedWindow;break;
   case CMD_STOPALL:value.enabled=global.stoppable;break;
   case CMD_CLEAN:value.enabled=global.cleanable;break;
   default:break;
  }
  return value;
 }
 std::map<UINT,bool> commandStates(const std::vector<UINT>& commands,const std::vector<JobPtr>* suppliedSelection=nullptr,bool* addingToQueue=nullptr,bool freshFiles=false,bool* modelBusy=nullptr){
  if(modelBusy)*modelBusy=false;
  const bool needSelection=addingToQueue||std::any_of(commands.begin(),commands.end(),commandUsesSelection);
  const auto chosen=suppliedSelection?*suppliedSelection:needSelection?selected(modelBusy==nullptr):std::vector<JobPtr>{};
  CommandGlobalNeeds needs;bool needManager=addingToQueue!=nullptr;std::map<UINT,CommandAvailability> decisions;
  for(const auto id:commands){decisions.emplace(id,CommandAvailability{});needManager=needManager||commandUsesManager(id);needs.recovery=needs.recovery||id==CMD_RECOVERY;needs.stop=needs.stop||id==CMD_STOPALL;needs.close=needs.close||id==CMD_STOP_CLOSE_ALL;needs.clean=needs.clean||id==CMD_CLEAN;}
  std::vector<Progress*> windows;if(needs.close)for(const auto& window:progress)if(window->GetSafeHwnd())windows.push_back(window.get());
  if(needManager){std::unique_lock lock(manager.mutex,std::defer_lock);if(modelBusy){if(!lock.try_lock()){*modelBusy=true;return {};}}else lock.lock();if(addingToQueue)*addingToQueue=std::none_of(chosen.begin(),chosen.end(),[](const JobPtr& job){return downloadQueueMember(job->data);});const auto global=needs.any()?commandGlobalStateLocked(needs,windows):CommandGlobalState{};
   for(auto& entry:decisions)if(commandUsesManager(entry.first))entry.second=commandAvailabilityLocked(entry.first,chosen,global);
  }
  // Advisory UI checks never query the disk. One detached selection batch is
  // refreshed in the background; actual command invocation checks afresh.
  std::shared_ptr<const AsyncFileStatus::Result> fileStatus;
  if(!freshFiles){std::vector<fs::path> paths;for(const auto& entry:decisions)if(resolveCommandAvailabilityWith(entry.second,[](const fs::path&){return true;}))paths.insert(paths.end(),entry.second.regularFiles.begin(),entry.second.regularFiles.end());if(!paths.empty())fileStatus=commandFiles.observe(std::move(paths));}
  std::map<UINT,bool> result;std::map<fs::path,bool> files;
  for(auto& entry:decisions){if(entry.first==CMD_PROPERTIES||entry.first==CMD_PROGRESS||entry.first==CMD_FOLDER)entry.second.enabled=chosen.size()==1;/* ZIP performs its fresh authoritative open in the background task. */result.emplace(entry.first,freshFiles?(entry.first==CMD_ZIP?resolveCommandAvailabilityWith(entry.second,[](const fs::path&){return true;}):resolveCommandAvailability(entry.second,files)):resolveCommandAvailabilityWith(entry.second,[&](const fs::path& path){return fileStatus&&fileStatus->regular(path);}));}
  return result;
 }
 static bool globalLimitIndependentCommand(UINT id){return id==CMD_EXIT||id==CMD_SELECTALL||arrangeColumn(id)>=0;}
 bool commandEnabled(UINT id,bool freshFiles=false){
  if((appearanceCommandTask.valid()||globalLimitCommandTask.valid())&&!globalLimitIndependentCommand(id))return false;
  if(appearanceMutation(id)&&(manager.fileInfoDecisionPending||optionsUiActive||optionsRefreshPending||optionsAppearanceSnapshot))return false;
  if((id==CMD_LIMIT_ON||id==CMD_LIMIT_OFF||id==CMD_LIMIT)&&(globalLimitCommandTask.valid()||manager.fileInfoDecisionPending))return false;
  if((optionsUiActive||optionsRefreshPending||optionsAppearanceSnapshot)&&(commandUsesManager(id)||commandUsesSelection(id)||(id==CMD_OPTIONS&&optionsUiActive)))return false;
  if(!commandUsesManager(id))return !commandUsesSelection(id)||selected(freshFiles).size()==1;
  return commandStates({id},nullptr,nullptr,freshFiles).at(id);
 }
 std::optional<bool> displayedSelectionCommandEnabled(UINT id){
  DownloadSelectionAction action;
  switch(id){case CMD_RESUME:action=DownloadSelectionAction::Resume;break;case CMD_STOP:action=DownloadSelectionAction::Stop;break;case CMD_DELETE:action=DownloadSelectionAction::Remove;break;case CMD_RELOCATE:action=DownloadSelectionAction::Relocate;break;case CMD_REMOVEQUEUE:action=DownloadSelectionAction::RemoveQueue;break;case CMD_UP:case CMD_DOWN:action=DownloadSelectionAction::Reorder;break;default:return {};}
  if(!table.GetSafeHwnd())return false;
  if(refreshing)return {};
  std::vector<std::shared_ptr<const DownloadListRow>> rows;
  for(int index=-1;(index=table.GetNextItem(index,LVNI_SELECTED))>=0;){
   if(index>=(int)visible.size()||index>=(int)rowSnapshots.size()||!rowSnapshots[index])return {};
   const auto captured=listRecordCache.find(visible[index]);if(captured==listRecordCache.end()||captured->second!=rowSnapshots[index])return {};
   rows.push_back(rowSnapshots[index]);
  }
  return downloadSelectionAvailable(action,rows);
 }
 // A busy model must not leave advice belonging to a different selection.
 // Use the displayed row cohort for supported commands; invocation stays live.
 std::optional<bool> advisoryCommandEnabled(UINT id){
  if((appearanceCommandTask.valid()||globalLimitCommandTask.valid())&&!globalLimitIndependentCommand(id))return false;
  if(appearanceMutation(id)&&(manager.fileInfoDecisionPending||optionsUiActive||optionsRefreshPending||optionsAppearanceSnapshot))return false;
  if((id==CMD_LIMIT_ON||id==CMD_LIMIT_OFF||id==CMD_LIMIT)&&(globalLimitCommandTask.valid()||manager.fileInfoDecisionPending))return false;
  if(downloadMenuAdvice->current){const auto found=downloadMenuAdvice->current->find(id);if(found!=downloadMenuAdvice->current->end())return found->second;}
  if((optionsUiActive||optionsRefreshPending||optionsAppearanceSnapshot)&&(commandUsesManager(id)||commandUsesSelection(id)||(id==CMD_OPTIONS&&optionsUiActive)))return false;
  if(!commandUsesManager(id))return !commandUsesSelection(id)||selected(false).size()==1;
  bool busy=false;const auto states=commandStates({id},nullptr,nullptr,false,&busy);if(busy)return displayedSelectionCommandEnabled(id);return states.at(id);
 }
 afx_msg LRESULT OnCommandFileRefresh(WPARAM,LPARAM){commandFileRefreshPending=false;if(optionsUiActive||optionsRefreshPending||optionsAppearanceSnapshot)return 0;if(!quitting)try{bool busy=false;commandStates({CMD_ZIP,CMD_RECYCLE},nullptr,nullptr,false,&busy);}catch(const std::exception& e){showStatus(e.what());}return 0;}
 void scheduleCommandFileRefresh(){if(!commandFileRefreshPending&&!quitting&&GetSafeHwnd()){commandFileRefreshPending=PostMessage(COMMAND_FILE_REFRESH)!=FALSE;}}
 void updateToolbarCommands(){
  if(appearanceCommandTask.valid()||globalLimitCommandTask.valid()){for(int i=0;i<ToolbarCommandCount;++i)toolbar.EnableButton(toolbarCommand(i),FALSE);return;}
  std::vector<UINT> commands;commands.reserve(ToolbarCommandCount+2);for(int i=0;i<ToolbarCommandCount;++i)commands.push_back(i==3?CMD_STOP_CLOSE_ALL:toolbarCommand(i));commands.push_back(CMD_ZIP);commands.push_back(CMD_RECYCLE);
  bool busy=false;const auto enabled=optionsAppearanceCommands?*optionsAppearanceCommands:commandStates(commands,nullptr,nullptr,false,&busy);if(busy){for(int i=0;i<ToolbarCommandCount;++i)if(const auto advice=displayedSelectionCommandEnabled(commands[i]))toolbar.EnableButton(toolbarCommand(i),*advice);return;}for(int i=0;i<ToolbarCommandCount;++i)toolbar.EnableButton(toolbarCommand(i),enabled.at(commands[i]));
 }
 #include "QueueMenus.hpp"
 #include "ToolbarUi.hpp"
 void releaseExitLayoutOwner(){if(completionModalFence.modalActive())exitLayoutRestorePending=true;else EnableWindow(exitLayoutOwnerEnabled);}
 void restoreExitLayoutOwnerIfReady(){if(exitLayoutRestorePending&&!completionModalFence.modalActive()){exitLayoutRestorePending=false;EnableWindow(exitLayoutOwnerEnabled);}}
 ShellFolderTask shellFolderTask;std::deque<fs::path> shellFolders;std::deque<ShellFolderResult> shellFolderErrors;bool shellFolderClosePending=false,shellFolderExplicitExit=false;
 void requestShellFolder(fs::path path){
  if(quitting||shellFolderClosePending)throw std::runtime_error("UDM is closing. Try opening the folder again after restart.");
  if(path.empty()||!path.is_absolute())throw std::runtime_error("The destination folder must be an absolute path.");
  if(shellFolders.size()>=64)throw std::runtime_error("Too many folder requests are waiting for Windows. Try again shortly.");
  shellFolders.push_back(std::move(path));pollShellFolders();
 }
 bool pollShellFolders(){
  if(shellFolderTask.ready()){auto result=shellFolderTask.take();if(!result.error.empty())shellFolderErrors.push_back(std::move(result));}
  if(!shellFolderTask.valid()&&!shellFolders.empty()&&automaticPresentationContextReady()&&automaticPresentationAllowed()){
   const auto path=shellFolders.front();
   try{
#ifdef UDM_TOOLBAR_COMPONENT_TEST
   shellFolderTask.start(path,maintenanceSignal->notifier(),shellFolderLauncher);
#else
   shellFolderTask.start(path,maintenanceSignal->notifier());
#endif
   }catch(const std::exception& e){shellFolderErrors.push_back({path,e.what()});}
   shellFolders.pop_front();
  }
  if(!shellFolderErrors.empty()&&automaticPresentationContextReady()&&automaticPresentationAllowed()){
   auto result=std::move(shellFolderErrors.front());shellFolderErrors.pop_front();
#ifdef UDM_TOOLBAR_COMPONENT_TEST
   if(shellFolderErrorPresenter)shellFolderErrorPresenter(result);else
#endif
   error(this,std::runtime_error(result.error+"\nFolder: "+utf8(result.path.wstring())));
  }
  if(shellFolderClosePending&&!shellFolderTask.valid()&&shellFolders.empty()&&shellFolderErrors.empty()&&automaticPresentationContextReady()&&automaticPresentationAllowed()){
   const auto explicitExit=shellFolderExplicitExit;shellFolderClosePending=false;quitting=explicitExit;OnClose();return true;
  }
  return false;
 }
 bool automaticPresentationDeferred=false,automaticPresentationDraining=false,automaticPresentationRetryPending=false,preparedPresentationActive=false,clipboardCaptureDeferred=false;
 bool automaticPresentationAllowed(){if(quitting||!GetSafeHwnd())return false;const bool modal=completionModalFence.modalActive();if(modal||!IsWindowEnabled()||automaticPresentationRetryPending){automaticPresentationDeferred=true;return false;}return true;}
 void presentClipboardCapture(){
  if(!automaticPresentationAllowed()){clipboardCaptureDeferred=true;return;}
  clipboardCaptureDeferred=false;clipboardPromptPending=false;auto value=copiedUrl;copiedUrl.clear();
  if(!clipboardCandidate(value,preferences())||!IsWindowEnabled())return;
  auto job=manager.offerDownload(value);presentDownload(this,manager,job);
 }
 bool automaticPresentationContextReady()const{return !quitting&&!refreshing&&!presentationRecoveryShowing&&!cancellationRecoveryShowing&&!optionsUiActive&&!optionsRefreshPending&&!optionsAppearanceSnapshot&&!manager.fileInfoDecisionPending;}
 void wakeDeferredAutomaticPresentation()noexcept{try{if(automaticPresentationDeferred&&!automaticPresentationDraining&&automaticPresentationContextReady()&&automaticPresentationAllowed())maintenanceSignal->notifier()();}catch(...){/* Main timer retries. */}}
 bool presentReadyPreparedFileInfo(){
  if(preparedPresentationActive||preparedFileInfoViews.empty())return false;
  std::shared_ptr<const PendingFileInfoInbox::Operation> operation;
  for(const auto& entry:preparedFileInfoViews){const auto& candidate=entry.second;if(!candidate||!candidate->view)continue;const auto state=candidate->outcome();if((state==PendingFileInfoInbox::Outcome::Pending||state==PendingFileInfoInbox::Outcome::Accepted)&&candidate->view->media&&!candidate->view->formRequest&&!candidate->cancelRequested()){operation=candidate;break;}}
  if(!operation)return false;
  if(!automaticPresentationContextReady()||!automaticPresentationAllowed()){automaticPresentationDeferred=true;return false;}
  preparedPresentationActive=true;struct Dispatch{bool& active;~Dispatch(){active=false;}}dispatch{preparedPresentationActive};
  try{
#ifdef UDM_TOOLBAR_COMPONENT_TEST
   if(automaticPreparedPresenter)automaticPreparedPresenter(operation);else
#endif
   presentPendingBrowserCapture(this,manager,operation,presentationAcknowledger());
  }catch(...){automaticPresentationDeferred=true;throw;}
  const auto current=preparedFileInfoViews.find(operation->view->jobId);
  if(current!=preparedFileInfoViews.end()&&current->second==operation&&(!operation->cancelRequested()||operation->cancelFinished))preparedFileInfoViews.erase(current);
  if(!preparedFileInfoViews.empty())automaticPresentationDeferred=true;
  PostMessage(PRESENT_CAPTURE);return true;
 }
 bool resumeAutomaticPresentation(){
  if(automaticPresentationDraining||!automaticPresentationDeferred||!automaticPresentationContextReady()||!automaticPresentationAllowed())return false;
  automaticPresentationDeferred=false;automaticPresentationDraining=true;bool completed=false;
  struct Drain{MainWindow& frame;bool& completed;~Drain(){frame.automaticPresentationDraining=false;if(completed)frame.wakeDeferredAutomaticPresentation();}}drain{*this,completed};
  try{
  presentReadyPreparedFileInfo();
  if(!automaticPresentationContextReady()||!automaticPresentationAllowed()){completed=true;return true;}
  showCancellationRecovery();showPresentationRecovery();
  if(automationReviewDeferred)reviewAutomationBatches();
  refresh(false,true);showQuotaWarning();
  if(automaticPresentationContextReady()&&automaticPresentationAllowed()){
   if(deferredUpdateResult&&PostMessage(UPDATE_RESULT,0,reinterpret_cast<LPARAM>(deferredUpdateResult.get())))deferredUpdateResult.release();
   if(clipboardCaptureDeferred){if(PostMessage(WM_COMMAND,CMD_CLIPBOARD_CAPTURE))clipboardCaptureDeferred=false;}
   if(manager.browserSettingsRequested.load()&&PostMessage(WM_COMMAND,CMD_BROWSER))manager.browserSettingsRequested=false;
  }
  completed=true;return true;
  }catch(...){automaticPresentationDeferred=false;automaticPresentationRetryPending=true;throw;}
 }
 bool closeBlockedByDialog(bool includeMain=false){return (includeMain?completionModalFence.blocked():completionModalFence.modalActive())||presentationRecoveryShowing||fileInfoDialogShowing()||networkDialogShowing()||resumePresenting||duplicateDialogShowing()||authenticationShowing||authenticationDialogShowing()||manager.fileInfoDecisionPending;}
 bool cancelClosePolicy(){if(!closePolicyTask.valid())return false;closePolicyTask.finish();showStatus("");wakeCompletionWork();return true;}
 bool prepareExitLayout(){
  if(!table.GetSafeHwnd())return true;
  auto layout=listLayout();
  {std::unique_lock lock(manager.mutex,std::try_to_lock);if(lock.owns_lock()&&!manager.fileInfoDecisionPending){mergeMainExitLayout(manager,std::move(layout));return true;}}
  exitLayoutOwnerEnabled=IsWindowEnabled()!=FALSE;
#ifdef UDM_TOOLBAR_COMPONENT_TEST
  exitLayoutTask.start(manager,std::move(layout),maintenanceSignal->notifier(),exitLayoutObserver);
#else
  exitLayoutTask.start(manager,std::move(layout),maintenanceSignal->notifier());
#endif
  EnableWindow(FALSE);showStatus("Waiting to save download layout...");return false;
 }
 void pollExitLayout(){
  // Retire a ready accepted Resume without presenting follow-ups during Exit.
  if(resumeTask.active()&&resumeTask.ready())pollResumeCommands();
  if(!exitLayoutTask.ready())return;
  const auto result=exitLayoutTask.take();
  if(!result.applied){exitLayoutPrepared=false;quitting=false;recoveryOnClose=false;releaseExitLayoutOwner();showStatus("The download layout could not be saved. Try Exit again.");wakeCompletionWork();return;}
  exitLayoutPrepared=true;OnClose();
 }
 void pollClosePolicy(){
  if(!closePolicyTask.ready())return;auto result=closePolicyTask.take();showStatus("");
  if(result.cancelled){wakeCompletionWork();return;}
  if(!result.error.empty()){showStatus("Download settings could not be read. Try closing again.");wakeCompletionWork();return;}
  if(closeBlockedByDialog(true)){quitting=false;recoveryOnClose=false;wakeCompletionWork();return;}
  if(result.closeToTray){ShowWindow(SW_HIDE);wakeCompletionWork();return;}
  quitting=true;OnClose();
 }
 #include "MainFeatures.hpp"
 #include "MainOptionsUi.hpp"
 #include "DesktopUi.hpp"
 void showBasket(){auto p=preferences();bool enabled=yes(p,"DropBasket");if(!enabled){basket.reset();return;}if(!basket){auto window=std::make_unique<Basket>(this);if(!window->open((int)num(p,"BasketX",25),(int)num(p,"BasketY",100)))throw std::runtime_error("Cannot create the drop target.");window->moved=[this](int x,int y){try{auto prefs=preferences();prefs["BasketX"]=x;prefs["BasketY"]=y;manager.setSettings(prefs);}catch(const std::exception& e){error(this,e);}};basket=std::move(window);}basket->ShowWindow(SW_SHOWNOACTIVATE);}

 void fileCompletionActions(){
  if(handlingQueueAction||!IsWindowEnabled()||completionModalFence.blocked())return;
  for(size_t index=0;index<fileActions.size();){
   auto job=fileActions[index].first;auto spec=fileActions[index].second;
   const auto admission=tryFileCompletionCountdown(manager,job,spec);
   if(admission.state==CompletionCountdownState::Busy)return;
   if(admission.state==CompletionCountdownState::WaitingForOthers){++index;continue;}
   fileActions.erase(fileActions.begin()+index);if(admission.state==CompletionCountdownState::Obsolete)continue;
   struct Guard{bool& v;Guard(bool& value):v(value){v=true;}~Guard(){v=false;}} guard(handlingQueueAction);
   auto action=str(spec,"Action")+(yes(spec,"Force")?" (force closing enabled)":"");bool execute=false;ULONGLONG end=GetTickCount64()+num(spec,"DelaySeconds",30)*1000;
   CompletionCountdownDeadline deadline;CompletionCountdownWakeGuard countdownWake(completionCountdownWake);Form d("Download completion action",442,181,this);
   d.init=[&]{countdownWake.bind(d.taskNotifier());d.label(admission.fileName+" completed.",15,16,412,28);d.label("Requested action: "+action,15,49,412,34);auto remaining=d.label("",15,88,412,28);d.defaultButton(d.button("Cancel action",276,140,150,[&]{d.close(IDCANCEL);}));d.accept=[&]{d.close(IDCANCEL);};d.pulse=[&,remaining]{const auto now=GetTickCount64();std::string tickError;const auto tick=deadline.poll(modelMaintenance,now>=end,tickError,!d.taskPulseActive());if(!tickError.empty()){showStatus(tickError);d.close(IDCANCEL);return;}if(tick==CompletionCountdownTick::Failed){d.close(IDCANCEL);return;}if(tick==CompletionCountdownTick::Pending){remaining->SetWindowText(L"Checking download status...");return;}const auto current=tryFileCompletionCountdown(manager,job,spec);if(current.state==CompletionCountdownState::Busy){remaining->SetWindowText(L"Checking download status...");return;}if(current.state!=CompletionCountdownState::Ready){d.close(IDCANCEL);return;}if(now>=end){execute=true;d.close();return;}remaining->SetWindowText(cs("Action starts in "+std::to_string((end-now+999)/1000)+" seconds."));};d.pulse();};
   d.DoModal();countdownWake.reset();if(!execute)continue;
   const auto steps=validateCompletionSteps(spec.at("Actions").get<std::vector<std::string>>());
   for(const auto& step:steps){if(step=="Exit UDM"){PostMessage(WM_COMMAND,CMD_EXIT);return;}if(step=="Open downloaded file")openDownloadedFile(this,job->target());else performSystemAction(step,yes(spec,"Force")&&(step=="Shut down"||step=="Restart"));}
  }
 }
 void queueCompletionActions(){
  if(quitting||handlingQueueAction||!IsWindowEnabled()||completionModalFence.blocked())return;
  struct Guard{bool& value;Guard(bool& v):value(v){value=true;}~Guard(){value=false;}} guard(handlingQueueAction);
  {std::unique_lock lock(manager.mutex,std::try_to_lock);if(!lock.owns_lock()||manager.fileInfoDecisionPending)return;
   for(auto& completion:manager.takeQueueCompletions())pendingQueueActions.emplace_back(std::move(completion));}
  while(!pendingQueueActions.empty()){
   auto& request=pendingQueueActions.front();const auto prepared=tryQueueCompletionCountdown(manager,request.completion,!request.confirmed);
   if(prepared.state==CompletionCountdownState::Busy)return;
   if(prepared.state!=CompletionCountdownState::Ready){pendingQueueActions.pop_front();continue;}
   if(!request.confirmed){
    const auto& completion=request.completion;const auto action=completionSummary(request.steps);bool execute=false;
    const ULONGLONG end=GetTickCount64()+num(completion,"DelaySeconds",30)*1000;
    CompletionCountdownDeadline deadline;CompletionCountdownWakeGuard countdownWake(completionCountdownWake);Form d("Queue finished",442,181,this);
    d.init=[&]{
     countdownWake.bind(d.taskNotifier());d.label(queueCompletionMessage(prepared.shown),15,16,412,28);
     d.label("Requested action: "+action+(action=="Open file"?" — "+str(completion,"File"):""),15,49,412,38);
     auto remaining=d.label("",15,88,412,28);
     d.defaultButton(d.button("Cancel action",276,140,150,[&]{d.close(IDCANCEL);}));d.accept=[&]{d.close(IDCANCEL);};
     d.pulse=[&,remaining]{
      const auto now=GetTickCount64();std::string tickError;const auto tick=deadline.poll(modelMaintenance,now>=end,tickError,!d.taskPulseActive());
      if(!tickError.empty()){showStatus(tickError);d.close(IDCANCEL);return;}
      if(tick==CompletionCountdownTick::Failed){d.close(IDCANCEL);return;}
      if(tick==CompletionCountdownTick::Pending){remaining->SetWindowText(L"Checking queue status...");return;}
      const auto current=tryQueueCompletionCountdown(manager,completion,false);
      if(current.state==CompletionCountdownState::Busy){remaining->SetWindowText(L"Checking queue status...");return;}
      if(current.state!=CompletionCountdownState::Ready){d.close(IDCANCEL);return;}
      if(now>=end){execute=true;d.close();return;}
      remaining->SetWindowText(cs("Action starts in "+std::to_string((end-now+999)/1000)+" seconds."));
     };d.pulse();
    };d.DoModal();countdownWake.reset();
    if(!execute){pendingQueueActions.pop_front();continue;}request.confirmed=true;
   }
   while(request.nextStep<request.steps.size()){
    const auto current=tryQueueCompletionCountdown(manager,request.completion,false);
    if(current.state==CompletionCountdownState::Busy)return;
    if(current.state!=CompletionCountdownState::Ready)break;
    const auto step=request.steps[request.nextStep++];
    try{
#ifdef UDM_TOOLBAR_COMPONENT_TEST
     if(queueCompletionPerformer){queueCompletionPerformer(step,yes(request.completion,"Force"),str(request.completion,"File"));continue;}
#endif
     if(step=="Exit UDM"){if(!PostMessage(WM_COMMAND,CMD_EXIT))throw std::runtime_error("Cannot request UDM exit.");pendingQueueActions.pop_front();return;}
     if(step=="Open file")openFile(this,fs::path(wide(str(request.completion,"File"))));else performSystemAction(step,yes(request.completion,"Force"));
    }catch(...){pendingQueueActions.pop_front();throw;}
   }
   pendingQueueActions.pop_front();
  }
 }
 // Called only by the owned reader with Manager held. No HWND, filesystem
 // probing, AsyncFileStatus state or mutable window selection is read here.
 DownloadMenuView captureDownloadMenu(const std::vector<JobPtr>& selection){
  DownloadMenuView view;view.selection=selection;
  for(const auto& job:selection)if(std::find(manager.jobs.begin(),manager.jobs.end(),job)==manager.jobs.end()){view.obsolete=true;return view;}
  for(const auto id:std::array<UINT,19>{{CMD_OPEN,CMD_OPENWITH,CMD_FOLDER,CMD_ZIP,CMD_RELOCATE,CMD_REDOWNLOAD,CMD_RESUME,CMD_STOP,CMD_PROGRESS,CMD_REFRESH_ADDRESS,CMD_DELETE,CMD_RECYCLE,CMD_MOVEQUEUE,CMD_REMOVEQUEUE,CMD_UP,CMD_DOWN,CMD_PROPERTIES,CMD_EDIT_DOWNLOAD_SCHEDULE,CMD_SCHEDULE_EXISTING_DOWNLOAD}}){
   auto value=commandAvailabilityLocked(id,selection,CommandGlobalState{});
   if(id==CMD_PROPERTIES||id==CMD_PROGRESS||id==CMD_FOLDER)value.enabled=selection.size()==1;
   view.commands.emplace(id,std::move(value));
  }
  view.adding=std::none_of(selection.begin(),selection.end(),[](const JobPtr& job){return downloadQueueMember(job->data);});
  for(const auto& job:selection)view.membership.emplace_back(downloadQueueMember(job->data),str(job->data,"Status")=="Complete");
  view.queues=manager.state["Queues"];view.queueLabels=queueDisplayLabels(view.queues);view.queueNames=membershipQueues(selection);view.doubleClick=str(manager.state["Settings"],"CompletedDoubleClick","Properties");return view;
 }
 void fillDownloadMenu(CMenu& popup,const DownloadMenuView& view,std::map<UINT,bool>* advice=nullptr){
  const bool adding=view.adding;std::map<UINT,bool> enabled;std::vector<fs::path> paths;
  for(const auto& entry:view.commands)if(resolveCommandAvailabilityWith(entry.second,[](const fs::path&){return true;}))paths.insert(paths.end(),entry.second.regularFiles.begin(),entry.second.regularFiles.end());
  const auto files=paths.empty()?std::shared_ptr<const AsyncFileStatus::Result>{}:commandFiles.observe(std::move(paths));
  for(const auto& entry:view.commands)enabled.emplace(entry.first,resolveCommandAvailabilityWith(entry.second,[&](const fs::path& path){return files&&files->regular(path);}));
  if(advice)*advice=enabled;
  popup.CreatePopupMenu();
  // Keep the captured IDM groups together and put UDM extensions after them.
  auto flags=[&](UINT id){return MF_STRING|(enabled.at(id)?MF_ENABLED:MF_GRAYED);};
  for(auto entry:std::vector<std::pair<UINT,const wchar_t*>>{{CMD_OPEN,L"Open"},{CMD_OPENWITH,L"Open with..."},{CMD_FOLDER,L"Open folder"},{0,nullptr},{CMD_RELOCATE,L"Move/Rename (Ctrl-M)"},{0,nullptr},{CMD_REDOWNLOAD,L"Redownload"},{0,nullptr},{CMD_RESUME,L"Resume Download"},{CMD_STOP,L"Stop Download"},{0,nullptr},{CMD_REFRESH_ADDRESS,L"Refresh download address"},{0,nullptr},{CMD_DELETE,L"Remove"},{0,nullptr}})
   if(entry.first)popup.AppendMenuW(flags(entry.first),entry.first,entry.second);else popup.AppendMenuW(MF_SEPARATOR);
  {CMenu queues;queues.CreatePopupMenu();fillMoveQueueCommands(queues,view.queueNames,view.queueLabels);MENUITEMINFOW info{sizeof(info)};info.fMask=MIIM_ID|MIIM_SUBMENU|MIIM_STRING|MIIM_STATE;info.wID=CMD_MOVEQUEUE;info.hSubMenu=queues.Detach();info.fState=enabled.at(CMD_MOVEQUEUE)?MFS_ENABLED:MFS_GRAYED;std::wstring label=adding?L"Add to queue":L"Move to queue";info.dwTypeData=label.data();moveQueuePosition=popup.GetMenuItemCount();popup.InsertMenuItemW(moveQueuePosition,&info,TRUE);}
  popup.AppendMenuW(flags(CMD_REMOVEQUEUE),CMD_REMOVEQUEUE,L"Delete from queue");popup.AppendMenuW(MF_SEPARATOR);
  CMenu doubleClick;doubleClick.CreatePopupMenu();const auto& behavior=view.doubleClick;doubleClick.AppendMenuW(MF_STRING|(behavior=="Open"?MF_CHECKED:0),CMD_DOUBLE_OPEN,L"Open");doubleClick.AppendMenuW(MF_STRING|(behavior=="Properties"?MF_CHECKED:0),CMD_DOUBLE_PROPERTIES,L"Properties");popup.AppendMenuW(MF_POPUP,(UINT_PTR)doubleClick.Detach(),L"On Double click");
  popup.AppendMenuW(MF_SEPARATOR);popup.AppendMenuW(flags(CMD_PROPERTIES),CMD_PROPERTIES,L"Properties");
  CMenu more;more.CreatePopupMenu();
  for(const auto& entry:std::array<std::pair<UINT,const wchar_t*>,5>{{
       {CMD_PROGRESS,L"Show progress"},{CMD_ZIP,L"ZIP contents..."},{CMD_RECYCLE,L"Recycle downloaded file..."},
       {CMD_UP,L"Move up in queue"},{CMD_DOWN,L"Move down in queue"}}})
   more.AppendMenuW(flags(entry.first),entry.first,entry.second);
  if(enabled.at(CMD_EDIT_DOWNLOAD_SCHEDULE))popup.AppendMenuW(MF_STRING,CMD_EDIT_DOWNLOAD_SCHEDULE,L"Edit download schedule...");
  else if(enabled.at(CMD_SCHEDULE_EXISTING_DOWNLOAD))popup.AppendMenuW(MF_STRING,CMD_SCHEDULE_EXISTING_DOWNLOAD,L"Schedule this download...");
  popup.AppendMenuW(MF_SEPARATOR);popup.AppendMenuW(MF_POPUP,(UINT_PTR)more.Detach(),L"More actions");
 }
#ifdef UDM_CAPTURE_PRESENTATION_COMPONENT_TEST
 // Legacy layout/policy assertions prepare explicitly. Interactive opening
 // always uses the owned asynchronous request below.
 void fillDownloadMenu(CMenu& popup){DownloadMenuView view;{Lock lock(manager.mutex);view=captureDownloadMenu(selected(false));}fillDownloadMenu(popup,view);}
#endif
 void cancelDownloadMenu(){downloadMenuRequest.reset();preparedDownloadMenu.reset();downloadMenuTask.cancelRead();}
 void startDownloadMenuRead(){
  if(!downloadMenuRequest||downloadMenuTask.valid())return;
  const auto chosen=downloadMenuRequest->selection;downloadMenuReadGeneration=downloadMenuRequest->generation;
  downloadMenuTask.start(manager,[this,chosen]{return captureDownloadMenu(chosen);},maintenanceSignal->notifier());
 }
 void pollDownloadMenu(){
  if(downloadMenuRequest&&(quitting||!IsWindowEnabled()||!IsWindowVisible()||IsIconic()||optionsUiActive||optionsRefreshPending||optionsAppearanceSnapshot||selected(false)!=downloadMenuRequest->selection||(downloadMenuRequest->requireActive&&::GetActiveWindow()!=GetSafeHwnd())))cancelDownloadMenu();
  if(downloadMenuTask.ready()){
   auto result=downloadMenuTask.take();
   if(downloadMenuRequest&&downloadMenuReadGeneration==downloadMenuRequest->generation){
    if(!result.error.empty()){cancelDownloadMenu();showStatus("The download menu could not be prepared. Try again.");return;}
    if(result.cancelled||result.view.obsolete){cancelDownloadMenu();return;}
    preparedDownloadMenu=std::move(result.view);
   }
  }
  if(!downloadMenuRequest)return;
  if(!preparedDownloadMenu){startDownloadMenuRead();return;}
  // The read can outlive deletion or a queue/policy edit. Never wait here;
  // retry changed cohorts and reject removed selections before native display.
  bool stale=false;{
   std::unique_lock lock(manager.mutex,std::try_to_lock);if(!lock.owns_lock()||manager.fileInfoDecisionPending)return;
   auto& view=*preparedDownloadMenu;
   for(size_t i=0;i<view.selection.size();++i){const auto& job=view.selection[i];if(std::find(manager.jobs.begin(),manager.jobs.end(),job)==manager.jobs.end()){cancelDownloadMenu();return;}stale=stale||view.membership[i]!=std::make_pair(downloadQueueMember(job->data),str(job->data,"Status")=="Complete");}
   stale=stale||manager.state["Queues"]!=view.queues||str(manager.state["Settings"],"CompletedDoubleClick","Properties")!=view.doubleClick;
  }
  if(stale){preparedDownloadMenu.reset();startDownloadMenuRead();return;}
  auto point=downloadMenuRequest->point;auto view=std::move(*preparedDownloadMenu);downloadMenuRequest.reset();preparedDownloadMenu.reset();
  CMenu popup;std::map<UINT,bool> advice;fillDownloadMenu(popup,view,&advice);
  // Native WM_INITMENUPOPUP routing must use the prepared cohort too. Keep
  // this guard independent of MainWindow's lifetime if the owner closes in
  // the nested popup loop; invocation still uses commandEnabled's live path.
  auto state=downloadMenuAdvice;auto previous=state->current;state->current=std::make_shared<const std::map<UINT,bool>>(std::move(advice));
  struct RestoreAdvice {std::shared_ptr<DownloadMenuAdvice> state;std::shared_ptr<const std::map<UINT,bool>> previous;~RestoreAdvice(){state->current=std::move(previous);}} restore{state,std::move(previous)};
#ifdef UDM_TOOLBAR_COMPONENT_TEST
  if(downloadMenuPresenter){downloadMenuPresenter(popup,point);return;}
#endif
  popup.TrackPopupMenu(TPM_RIGHTBUTTON,point.x,point.y,this);
 }
 void downloadMenu(CPoint point){
  auto chosen=selected(false);if(chosen.empty()||quitting||!IsWindowEnabled())return;
  preparedDownloadMenu.reset();downloadMenuRequest=DownloadMenuRequest{++downloadMenuGeneration,point,std::move(chosen),::GetActiveWindow()==GetSafeHwnd()};
  if(downloadMenuTask.valid())downloadMenuTask.cancelRead();else startDownloadMenuRead();
 }
 void addMenu(const wchar_t* title,const std::vector<std::pair<UINT,const wchar_t*>>& items){CMenu child;child.CreatePopupMenu();for(auto& item:items)if(item.first)child.AppendMenuW(MF_STRING,item.first,item.second);else child.AppendMenuW(MF_SEPARATOR);menu.AppendMenuW(MF_POPUP,(UINT_PTR)child.Detach(),title);}
 HTREEITEM node(std::string caption,std::string value,HTREEITEM parent=TVI_ROOT,std::string icon="folder",std::string key={}){
  const auto image=iconIndex.count(icon)?iconIndex.at(icon):0;
  auto item=tree.InsertItem(cs(caption),image,image,parent);if(!item)throw std::runtime_error("Cannot create navigation item.");
  filters.push_back(value);tree.SetItemData(item,filters.size()-1);
  const auto stableKey=key.empty()?value:key;treeKeys.emplace(item,stableKey);treeItems.emplace(stableKey,item);
  if(value==filter&&!treeSelection)treeSelection=item;return item;
 }
 void buildTree(){buildTree(navigationTreeSnapshot(manager));}
 void buildTree(NavigationTreeSnapshot snapshot){
  if(treeRebuilding||!tree.GetSafeHwnd())return;
  // The caller has released the model before any control message below.
  if(treeSnapshotValid&&!lastTree.empty()&&treeSnapshot.sameStructure(snapshot)&&treeQueueNodes.size()==snapshot.queueNames.size()&&treeQueueImages.size()==snapshot.queueNames.size()){
   for(size_t i=0;i<treeQueueNodes.size();++i){const auto image=queueImages.at(snapshot.queueKinds.at(i));
    if(treeQueueImages[i]!=image){if(!tree.SetItemImage(treeQueueNodes[i],image,image)){treeSnapshotValid=false;throw std::runtime_error("Cannot update navigation queue image.");}treeQueueImages[i]=image;}
   }
   if(treeFilter!=filter){
    const auto caret=tree.GetSelectedItem();const auto index=caret?tree.GetItemData(caret):filters.size();
    if(!caret||index>=filters.size()||filters[index]!=filter){
     auto target=treeItems.find(filter);if(target==treeItems.end()){target=treeItems.find("all");filter="all";}
     treeRebuilding=true;struct SelectionGuard{bool& rebuilding;~SelectionGuard(){rebuilding=false;}} selectionGuard{treeRebuilding};
     if(target==treeItems.end()||!tree.SelectItem(target->second)){treeSnapshotValid=false;throw std::runtime_error("Cannot select navigation item.");}
    }
    treeFilter=filter;
   }
   treeSnapshot=std::move(snapshot);return;
  }
  auto key=snapshot.structureKey();std::set<std::string> expanded;std::vector<std::string> selectedKeys;
  const bool initial=treeKeys.empty();
  for(const auto& item:treeKeys)if(tree.GetItemState(item.first,TVIS_EXPANDED)&TVIS_EXPANDED)expanded.insert(item.second);
  const auto selectedItem=tree.GetSelectedItem();
  if(selectedItem){const auto index=tree.GetItemData(selectedItem);
   // A programmatic filter change takes precedence over the old tree caret.
   if(index<filters.size()&&filters[index]==filter)for(auto item=selectedItem;item;item=tree.GetParentItem(item)){auto found=treeKeys.find(item);if(found!=treeKeys.end())selectedKeys.push_back(found->second);}
  }
  std::string firstVisible;const auto first=treeKeys.find(tree.GetFirstVisibleItem());if(first!=treeKeys.end())firstVisible=first->second;
  treeRebuilding=true;tree.SetRedraw(FALSE);
  struct RedrawGuard{CTreeCtrl& control;bool& rebuilding;~RedrawGuard(){rebuilding=false;control.SetRedraw(TRUE);control.Invalidate();}} redraw{tree,treeRebuilding};
  treeSnapshotValid=false;lastTree.clear();treeSelection=nullptr;
  tree.DeleteAllItems();filters.clear();treeKeys.clear();treeItems.clear();treeQueueNodes.clear();treeQueueImages.clear();
  auto all=node("All Downloads","all",TVI_ROOT,"all");
  for(const auto& category:snapshot.categories)if(category!="Other")node(category=="Archives"?"Compressed":category,"category:"+category,all,lower(category));
  auto unfinished=node("Unfinished","unfinished");auto finished=node("Finished","finished",TVI_ROOT,"complete");
  for(const auto& category:snapshot.categories){if(category=="Other")continue;node(category=="Archives"?"Compressed":category,"unfinished:"+category,unfinished,lower(category));node(category=="Archives"?"Compressed":category,"finished:"+category,finished,lower(category));}
  auto grab=node("Grabber projects","grabber",TVI_ROOT,"grabber");for(const auto& project:snapshot.projects)node(project.second,"project:"+project.first,grab,"grabber");
  auto queues=node("Queues","all",TVI_ROOT,"queue","root:queues");
  for(size_t i=0;i<snapshot.queueNames.size();++i){const auto& name=snapshot.queueNames[i];auto item=node(queueDisplayLabel(snapshot.queueLabels,name),"queue:"+name,queues,"queue");const auto image=queueImages.at(snapshot.queueKinds.at(i));if(!tree.SetItemImage(item,image,image))throw std::runtime_error("Cannot create navigation queue image.");treeQueueNodes.push_back(item);treeQueueImages.push_back(image);}
  if(initial)tree.Expand(all,TVE_EXPAND);
  else for(const auto& identity:expanded){const auto item=treeItems.find(identity);if(item!=treeItems.end())tree.Expand(item->second,TVE_EXPAND);}
  // A removed selection falls back to its nearest surviving ancestor.
  bool restoreAnchor=false;
  for(const auto& identity:selectedKeys){const auto item=treeItems.find(identity);if(item!=treeItems.end()){treeSelection=item->second;restoreAnchor=identity==selectedKeys.front();break;}}
  if(!treeSelection)treeSelection=all;
  const auto selectionIndex=tree.GetItemData(treeSelection);if(selectionIndex<filters.size())filter=filters[selectionIndex];
  if(!tree.SelectItem(treeSelection))throw std::runtime_error("Cannot restore navigation selection.");
  const auto anchor=treeItems.find(firstVisible);if(restoreAnchor&&anchor!=treeItems.end())tree.SelectSetFirstVisible(anchor->second);
  treeFilter=filter;treeSnapshot=std::move(snapshot);lastTree=std::move(key);treeSnapshotValid=true;
 }
 int layoutSearchBar(int w,int top){
  if(!search.IsWindowVisible())return 0;
  CClientDC dc(this);const auto units=DialogUnits::measure(dc.GetSafeHdc(),font.GetSafeHandle()?static_cast<HFONT>(font.GetSafeHandle()):static_cast<HFONT>(GetStockObject(DEFAULT_GUI_FONT)));
  const int buttonWidth=units.px(56),gap=units.px(8),closeX=std::max(0,w-units.px(15));
  const int settingsX=std::min(units.px(361),std::max(0,closeX-gap-buttonWidth));
  const int findX=std::max(0,settingsX-gap-buttonWidth),editX=units.px(72);
  searchLabel.MoveWindow(units.px(11),top+units.py(5),std::max(0,std::min(units.px(55),editX-units.px(17))),units.py(9));
  search.MoveWindow(editX,top+units.py(3),std::max(0,findX-units.px(9)-editX),units.py(14));
  searchFind.MoveWindow(findX,top+units.py(3),buttonWidth,units.py(14));
  searchSettings.MoveWindow(settingsX,top+units.py(3),buttonWidth,units.py(14));
  searchClose.MoveWindow(closeX,top+units.py(1),units.px(9),units.py(8));
  return units.py(17);
 }
 void layout(int w,int h){
  if(!table.m_hWnd)return;
  // Keep the rendered layout responsive when a model writer is busy. A later
  // successful layout applies fresh settings; no close policy is guessed here.
  Json settings;
  if(appearanceReadSnapshot)settings=*appearanceReadSnapshot;
  else if(optionsAppearanceSnapshot)settings=*optionsAppearanceSnapshot;
  else{std::unique_lock lock(manager.mutex,std::try_to_lock);
   if(lock.owns_lock())settings=manager.state["Settings"];
   else settings={{"HideToolbar",!(::GetWindowLongW(toolbar.GetSafeHwnd(),GWL_STYLE)&WS_VISIBLE)},{"HideCategories",!(categoryFrame.GetStyle()&WS_VISIBLE)}};
  }
  int bar=layoutToolbar(w,settings);
  const int bottom=statusShown()?px(24):0;
  bar+=layoutSearchBar(w,bar);
  // Insets measured from the toolbar and list in the IDM capture at 120 DPI.
  const auto reference=[dpi=windowDpi()](int value){return MulDiv(value,dpi,120);};
  const int top=bar+reference(8),end=h-bottom-reference(2);
  const bool hide=yes(settings,"HideCategories");
  for(CWnd* control:{static_cast<CWnd*>(&categoryFrame),static_cast<CWnd*>(&categoryHeading),
                     static_cast<CWnd*>(&categoryClose),static_cast<CWnd*>(&tree)})
   control->ShowWindow(hide?SW_HIDE:SW_SHOW);
  if(!hide){
   LOGFONTW headingFont{};
   font.GetLogFont(&headingFont);
   const int labelHeight=std::max(reference(16),std::abs(static_cast<int>(headingFont.lfHeight))+reference(2));
   const int headerHeight=std::max(reference(22),labelHeight+reference(6));
   categoryFrame.SetWindowPos(&CWnd::wndBottom,reference(7),top,reference(166),std::max(0,end-top),SWP_NOACTIVATE);
   categoryHeading.MoveWindow(reference(14),top+reference(4),reference(135),labelHeight);
   categoryClose.MoveWindow(reference(152),top+reference(6)+(headerHeight-reference(22))/2,reference(16),reference(16));
   tree.MoveWindow(reference(9),top+headerHeight,reference(161),std::max(0,end-reference(6)-top-headerHeight));
  }
  const int listLeft=reference(hide?7:179);
  table.MoveWindow(listLeft,top,std::max(0,w-listLeft-reference(8)),std::max(0,end-top));
  status.MoveWindow(0,h-bottom,w,bottom);
  clipboard.MoveWindow(px(155),h-bottom-px(31),w-px(175),px(27));
 }
 void showProgress(JobPtr job,bool explicitOpen=true){
  auto prefs=preferences();bool queueOrigin;
  {Lock lock(manager.mutex);queueOrigin=yes(job->data,"QueueOrigin");}
  const auto presentation=progressPresentation(str(prefs,"ProgressStartMode"),queueOrigin,yes(prefs,"QueueProgressMinimized",true),explicitOpen);
  if(presentation==ProgressPresentation::Hidden)return;
  for(auto& window:progress)if(window->GetSafeHwnd()&&window->downloadId()==job->id()){
   if(explicitOpen)window->showNormally();
   return;
  }
  auto window=std::make_unique<Progress>(manager,job,this);
  if(!window->Create(101,this))throw std::runtime_error("Cannot create progress dialog.");
  if(presentation==ProgressPresentation::Normal)window->showNormally();
  else if(presentation!=ProgressPresentation::Tray||!window->showInTray())window->ShowWindow(SW_SHOWMINNOACTIVE);
  progress.push_back(std::move(window));
 }
 static std::string completionSizeText(i64 size){
  if(size<0)return {};
  const auto exact=std::to_string(size)+" Bytes";
  if(size<1024)return exact;
  static constexpr const char* units[]={"KB","MB","GB","TB"};
  i64 divisor=1024;size_t unit=0;
  while(unit+1<std::size(units)&&size/divisor>=1024){divisor*=1024;++unit;}
  const auto hundredths=(size%divisor)*100/divisor;
  return std::to_string(size/divisor)+"."+(hundredths<10?"0":"")+std::to_string(hundredths)+"  "+units[unit]+" ("+exact+")";
 }
 // The caller holds the catalog lock. Track sizes describe the completed input
 // files and exclude repeated requests, preflight bytes and muxing overhead.
 static i64 completionTrackBytes(const Job& job){
  i64 total=0;
  for(const auto& track:{job.video,job.audio})if(track){
   const auto size=num(track->data,"Size",-1);
   if(size<=0||size>std::numeric_limits<i64>::max()-total||
      str(track->data,"Status")!="Complete"||str(track->data,"Sha256").empty())return 0;
   total+=size;
  }
  return total;
 }
 void queueCompletionWork(JobPtr job,CompletionRequestKind kind,bool requested=false,bool publication=false){
  if(quitting)return;
  if(std::none_of(pendingCompletions.begin(),pendingCompletions.end(),[&](const auto& value){return value.job==job&&value.kind==kind&&value.requested==requested&&value.publication==publication;}))pendingCompletions.push_back({std::move(job),kind,requested,publication});
  pollCompletionWork();
 }
 void wakeCompletionWork()noexcept{if(quitting||(pendingCompletions.empty()&&!completionReadiness.valid()))return;try{maintenanceSignal->notifier()();}catch(...){/* The main timer remains a fallback for an undelivered wake. */}}
 void serviceCompletionClaims(bool start){
  if(completionClaimReview)return;
  if(completionClaimTask.ready()){
   if(!GetSafeHwnd()||!IsWindowEnabled()||completionModalFence.blocked())return;
   auto job=std::move(completionClaimJob);auto result=completionClaimTask.take();
   if(result.saved){if(!result.action.empty())fileActions.push_back({job,std::move(result.action)});}
   else if(!quitting){
    completionClaimReview=true;struct ReviewGuard{bool& value;~ReviewGuard(){value=false;}} guard{completionClaimReview};
    const auto message="The completion action was not started.\n\n"+result.error+"\n\nRetry preparing this action? Cancel leaves its saved setting unchanged.";
    if(MessageBox(cs(message),L"Completion action",MB_RETRYCANCEL|MB_ICONERROR)==IDRETRY)pendingCompletionClaims.push_front(std::move(job));
   }
  }
  if(start&&!quitting&&!completionClaimTask.valid()&&!pendingCompletionClaims.empty()){
   auto job=pendingCompletionClaims.front();
   completionClaimTask.start(manager,job,maintenanceSignal->notifier());
   completionClaimJob=std::move(job);pendingCompletionClaims.pop_front();
  }
 }
 bool pollCompletionWork(){
  if(completionDraining)return true;
  serviceCompletionClaims(false);
  if(closePolicyTask.valid()||exitLayoutTask.valid()||exitLayoutPrepared)return false;
  if(!GetSafeHwnd()||!IsWindowEnabled()||completionModalFence.blocked())return false;
  if(pendingCompletions.empty()){
   if(completionReadiness.valid()&&quitAfterDownload&&!quitting){
    if(manager.fileInfoDecisionPending)return false;
    if(!completionReadiness.ready())return true;
    auto result=completionReadiness.take();if(!result.error.empty())throw std::runtime_error(result.error);
   }else completionReadiness.finish();
   serviceCompletionClaims(true);return false;
  }
  if(quitting)return false;
  auto wait=[this]{if(!completionReadiness.valid())completionReadiness.start(manager,maintenanceSignal->notifier());};
  // Do not starve the owner that must acknowledge an accepted decision.
  if(manager.fileInfoDecisionPending||resumePresenting||optionsUiActive||optionsRefreshPending||optionsAppearanceSnapshot){wait();return false;}
  if(completionReadiness.valid()){
   if(!completionReadiness.ready())return true;
   auto result=completionReadiness.take();if(!result.error.empty())throw std::runtime_error(result.error);
  }
  completionDraining=true;struct DrainingGuard{bool& value;~DrainingGuard(){value=false;}} guard{completionDraining};
  while(!pendingCompletions.empty()){
   auto request=std::move(pendingCompletions.front());pendingCompletions.pop_front();bool accepted=false;
   switch(request.kind){
    case CompletionRequestKind::Dialog:accepted=tryShowCompletionDialog(request.job,request.requested,request.publication);break;
    case CompletionRequestKind::Complete:accepted=tryComplete(request.job,request.requested);break;
    case CompletionRequestKind::Finished:accepted=tryFinishTransferNotification(request.job);break;
    case CompletionRequestKind::Started:accepted=tryPresentStartedTransfer(request.job);break;
   }
   if(!accepted){pendingCompletions.push_front(std::move(request));wait();return true;}
  }
  completionReadiness.finish();serviceCompletionClaims(true);return false;
 }
 void complete(JobPtr job,bool requested=false){queueCompletionWork(std::move(job),CompletionRequestKind::Complete,requested);}
 bool tryComplete(JobPtr job,bool requested){
  if(!requested){
   Json sound;bool claimAction=false;
   bool showDialog,closeProgress;
   fs::path completionFolder;
   {
    std::unique_lock lock(manager.mutex,std::try_to_lock);if(!lock.owns_lock())return false;
    claimAction=yes(job->data,"CompletionActionArmed");
    const auto& settings=manager.state["Settings"];
    sound={{"Sound",yes(settings,"Sound",true)},{"CompletionSoundFile",str(settings,"CompletionSoundFile")}};
    showDialog=showDownloadCompletionDialog(job->data,yes(settings,"SuppressCompletionDialog"));
    closeProgress=yes(job->data,"CloseProgressOnCompletion",true);
    if(yes(job->data,"OpenFolderOnCompletion"))completionFolder=job->target().parent_path();
   }
   // Queue the claim without starting its writer until completion presentation
   // has drained. Durable catalog I/O must not precede this dialog's creation.
   if(claimAction&&completionClaimJob!=job&&std::find(pendingCompletionClaims.begin(),pendingCompletionClaims.end(),job)==pendingCompletionClaims.end())pendingCompletionClaims.push_back(job);
   playDownloadSound(sound,"Sound","CompletionSoundFile");
   if(closeProgress)for(auto& p:progress)if(p->GetSafeHwnd()&&p->downloadId()==job->id())p->requestCompletionClose(true);
   if(!completionFolder.empty())try{requestShellFolder(completionFolder);}catch(const std::exception& e){error(this,e);}
   if(presentedPublications.erase(job->id())||!showDialog)return true;
  }
  showCompletionDialog(job,requested);
  return true;
 }
 bool pollCompletionSettings(){
  if(!completionSettingsTask.valid()){if(completionSettingsClosePending){completionSettingsClosePending=false;OnClose();return true;}return false;}
  if(!completionSettingsTask.ready())return true;
  auto source=completionSettingsSource.lock();completionSettingsSource.reset();auto result=completionSettingsTask.take();
  if(source&&source->dialog->GetSafeHwnd()){
   if(result.persisted)source->confirmed=*result.persisted;
   source->checkbox->SendMessage(BM_SETCHECK,source->confirmed?BST_CHECKED:BST_UNCHECKED);source->checkbox->EnableWindow(TRUE);
  }
  for(auto& dialog:completions)if(dialog->GetSafeHwnd())dialog->PostMessage(WM_TIMER,Form::pulseTimer,0);
  if(!result.saved){completionSettingsClosePending=false;quitting=false;recoveryOnClose=false;auto owner=source&&source->dialog->GetSafeHwnd()?static_cast<CWnd*>(source->dialog):static_cast<CWnd*>(this);error(owner,std::runtime_error(result.error.empty()?"The completion preference could not be saved.":result.error));}
  else if(completionSettingsClosePending)try{maintenanceSignal->notifier()();}catch(...){/* The main timer retains the accepted close decision. */}
  return true;
 }
 void showCompletionDialog(JobPtr job,bool requested,bool publication=false){queueCompletionWork(std::move(job),CompletionRequestKind::Dialog,requested,publication);}
 bool tryShowCompletionDialog(JobPtr job,bool requested,bool publication){
  Json addressInputs;
  fs::path target;
  i64 fileSize,trackBytes;bool suppressed;
  std::string publicationId;
  std::optional<std::string> scanSummary;
  {
   std::unique_lock lock(manager.mutex,std::try_to_lock);if(!lock.owns_lock())return false;
   if(publication){
    const auto& record=job->data;
    const auto& settings=manager.state["Settings"];
    publicationId=job->id();
    if(presentedPublications.count(publicationId)||str(record,"Status")!="Complete"||record.contains("ScanResult")||
       !str(settings,"ScanProgram").empty()||!str(record,"UpdateSetupVersion").empty()||
       !showDownloadCompletionDialog(record,yes(settings,"SuppressCompletionDialog"))||
       std::find(manager.jobs.begin(),manager.jobs.end(),job)==manager.jobs.end())return true;
   }
   addressInputs=downloadAddressInputs(*job);
   target=job->target();
   fileSize=num(job->data,"Size",-1);
   trackBytes=completionTrackBytes(*job);
   suppressed=yes(manager.state["Settings"],"SuppressCompletionDialog");
   if(job->data.contains("ScanResult"))scanSummary=scannerSummary(job->data);
  }
  // IDM 6.43 "Download complete" (resource dialog 286, docs/reference/completion-dialog.json):
  // the file-type icon, the two-line "Download complete / Downloaded X MB (N Bytes)" text,
  // Address and "The file saved as" edits, four buttons, the suppression check and the drag handle.
  // Saved transport context is advisory once the local output is complete.
  // Damaged/foreign-account encryption must not suppress the completion
  // window or replace the source with a guessed URL.
  std::string address;bool addressUnavailable=false;
  try{address=downloadAddress(addressInputs);}catch(const std::exception&){addressUnavailable=true;}
  const int extra=scanSummary?33:0;
  auto dialog=std::make_unique<Form>("Download complete",303,132+extra,this);auto d=dialog.get();d->modeless=true;d->dialogUnits=true;
  d->init=[this,d,job,address=std::move(address),addressUnavailable,target=std::move(target),fileSize,trackBytes,suppressed,scanSummary=std::move(scanSummary),extra]{
   // The reference puts the file actions and suppression checkbox before the read-only edits in tab order.
   auto openButton=d->button("Open",7,96+extra,61,{});
   auto openWithButton=d->button("Open with...",76,96+extra,61,{});
   auto folderButton=d->button("Open folder",145,96+extra,61,{});
   auto closeButton=d->button("Close",235,96+extra,61,{});
   auto suppress=d->check("Don't show this dialog again",suppressed,16,117+extra,238);
   auto preferenceView=std::make_shared<CompletionPreferenceView>(CompletionPreferenceView{d,suppress,suppressed});
   enum class CompletionAction {Open,OpenWith,Folder};
   struct CompletionActionState {std::optional<CompletionAction> pending;bool dispatching=false;bool busy()const{return pending.has_value()||dispatching;}};
   auto actionState=std::make_shared<CompletionActionState>();
   auto close=[d,actionState]{actionState->pending.reset();d->close();};
   d->bind(closeButton,close);
   d->bind(suppress,[this,d,suppress,preferenceView]{
    if(completionSettingsTask.valid()||manager.fileInfoDecisionPending){suppress->SendMessage(BM_SETCHECK,preferenceView->confirmed?BST_CHECKED:BST_UNCHECKED);return;}
    const bool checked=d->checked(suppress);
    if(preferenceView->confirmed==checked)return;
    try{completionSettingsTask.start(manager,checked,maintenanceSignal->notifier());completionSettingsSource=preferenceView;suppress->EnableWindow(FALSE);}
    catch(...){suppress->SendMessage(BM_SETCHECK,preferenceView->confirmed?BST_CHECKED:BST_UNCHECKED);throw;}
   });
   auto icon=d->make<FileDragIcon>(0,7,7,21,20);icon->setPath(target);
   std::string result="Download complete";
   if(fileSize>=0){
    result+="\r\nDownloaded "+completionSizeText(trackBytes?trackBytes:fileSize);
    if(trackBytes&&trackBytes!=fileSize)result+="\r\nSaved file: "+completionSizeText(fileSize);
   }
   d->label(result,34,7,262,32);
   constexpr DWORD fieldStyle=WS_BORDER|WS_TABSTOP|ES_AUTOHSCROLL|ES_READONLY;
   d->label(addressUnavailable?"Address (unavailable)":"Address",7,38,289);
   d->control(L"EDIT",address,fieldStyle,7,48,289,14);
   d->label("The file saved as",7,66,289);
   auto location=d->control(L"EDIT",utf8(target.wstring()),fieldStyle,7,76,289,14);
   if(scanSummary)d->control(L"EDIT","Virus check: "+*scanSummary,WS_TABSTOP|ES_READONLY|ES_MULTILINE|ES_AUTOVSCROLL|WS_VSCROLL,7,94,289,29,WS_EX_CLIENTEDGE);
   auto drag=d->make<FileDragList>(WS_TABSTOP,276,115+extra,18,12);drag->setPath(target);
   struct CompletionTarget {fs::path path;bool canOpen=false;};
   auto syncTarget=[this,d,job,location,icon,drag,openButton,openWithButton,folderButton,suppress,preferenceView,actionState]()->std::optional<CompletionTarget>{
    if(!d->GetSafeHwnd())return {};
    std::unique_lock lock(manager.mutex,std::try_to_lock);
    if(!lock.owns_lock())return {};
    CompletionTarget current{job->target(),canOpenDownloadLocked(job)};
    const bool suppressed=yes(manager.state["Settings"],"SuppressCompletionDialog");
    lock.unlock();
    if(!completionSettingsTask.valid()){preferenceView->confirmed=suppressed;if(d->checked(suppress)!=suppressed)suppress->SendMessage(BM_SETCHECK,suppressed?BST_CHECKED:BST_UNCHECKED);}
    suppress->EnableWindow(!completionSettingsTask.valid()&&!manager.fileInfoDecisionPending);
    if(current.path!=drag->path){
     location->SetWindowText(current.path.c_str());
     icon->setPath(current.path);
     drag->setPath(current.path);
    }
    for(CWnd* control:{openButton,openWithButton,static_cast<CWnd*>(icon),static_cast<CWnd*>(drag)})
     if((control->IsWindowEnabled()!=FALSE)!=(current.canOpen&&!actionState->busy()))control->EnableWindow(current.canOpen&&!actionState->busy());
    if((folderButton->IsWindowEnabled()!=FALSE)!=!actionState->busy())folderButton->EnableWindow(!actionState->busy());
    return current;
   };
   for(CWnd* control:{openButton,openWithButton,static_cast<CWnd*>(icon),static_cast<CWnd*>(drag)})control->EnableWindow(FALSE);
   syncTarget();
   icon->resolvePath=drag->resolvePath=[syncTarget,actionState]{
    if(actionState->busy())return fs::path{};
    auto current=syncTarget();
    return current&&current->canOpen?std::move(current->path):fs::path{};
   };
   auto drain=[this,d,close,syncTarget,actionState]{
    if(!d->GetSafeHwnd()){actionState->pending.reset();return;}
    if(!actionState->pending||actionState->dispatching)return;
    std::optional<CompletionTarget> current;
    try{current=syncTarget();}catch(...){actionState->pending.reset();throw;}
    if(!current)return;
    const auto action=*actionState->pending;actionState->pending.reset();actionState->dispatching=true;
    try{
     if(action==CompletionAction::Folder||current->canOpen){
#ifdef UDM_TOOLBAR_COMPONENT_TEST
      if(completionActionObserver)completionActionObserver(action==CompletionAction::Open?"Open":action==CompletionAction::OpenWith?"Open with...":"Open folder",current->path);
#endif
      if(action==CompletionAction::Open){if(openDownloadedFile(d,current->path))close();}
      else if(action==CompletionAction::OpenWith){if(confirmDownloadedFilePresent(d,current->path))openWith(d,current->path);}
      else{requestShellFolder(current->path.parent_path());close();}
     }
    }catch(...){actionState->dispatching=false;throw;}
    actionState->dispatching=false;if(d->GetSafeHwnd())syncTarget();
   };
   auto request=[drain,actionState,openButton,openWithButton,folderButton,icon,drag](CompletionAction action){
    if(actionState->busy())return;
    actionState->pending=action;
    for(CWnd* control:{openButton,openWithButton,folderButton,static_cast<CWnd*>(icon),static_cast<CWnd*>(drag)})control->EnableWindow(FALSE);
    drain();
   };
   d->bind(openButton,[request]{request(CompletionAction::Open);});
   d->bind(openWithButton,[request]{request(CompletionAction::OpenWith);});
   d->bind(folderButton,[request]{request(CompletionAction::Folder);});
   d->pulse=[d,syncTarget,drain,icon,actionState]{if(!d->GetSafeHwnd()){actionState->pending.reset();return;}syncTarget();drain();if(d->GetSafeHwnd())icon->refreshIcon();};
   d->accept=[openButton,openWithButton,folderButton,closeButton,close]{
    // Owner-drawn buttons can leave Enter on the dialog's IDOK path.
    const auto focused=::GetFocus();
    for(auto button:{openButton,openWithButton,folderButton,closeButton}){
     if(button->GetSafeHwnd()!=focused)continue;
     if(button->IsWindowEnabled())button->SendMessage(BM_CLICK);
     return;
    }
    close();
   };
   d->cancel=close;
   if(scanSummary)d->resizeClient(303,132+extra);
  };if(!d->Create(101,this))throw std::runtime_error("Cannot open the completion dialog.");d->ShowWindow(requested?SW_SHOW:SW_SHOWNOACTIVATE);if(requested){d->BringWindowToTop();d->SetForegroundWindow();}completions.push_back(std::move(dialog));
  if(publication)presentedPublications.insert(std::move(publicationId));
  return true;
 }

 std::unique_ptr<Form> quotaWarning;
 void showQuotaWarning(){
  if(quotaWarning&&quotaWarning->GetSafeHwnd())return;quotaWarning.reset();
  if(!automaticPresentationAllowed())return;Json notice;{std::unique_lock lock(manager.mutex,std::try_to_lock);if(!lock.owns_lock())return;notice=manager.takeQuotaWarning();}if(notice.empty())return;
  quotaWarning=std::make_unique<Form>("Download limits exceeded!",339,93,this);auto d=quotaWarning.get();d->modeless=true;d->dialogUnits=true;
  d->init=[this,d,notice]{
   auto message=d->label("",7,7,325,60);
   d->accept=[d]{d->close();};d->cancel=d->accept;d->defaultButton(d->button("OK",144,72,50,d->accept));
   d->pulse=[this,d,message,notice]{Json current;bool warn=true;{std::unique_lock lock(manager.mutex,std::try_to_lock);if(!lock.owns_lock())return;current=manager.quotaStatus();warn=yes(manager.state["Settings"],"WarnQuota",true);}if(!yes(current,"Waiting")||num(current,"PeriodStart")!=num(notice,"PeriodStart")||!warn){d->close();return;}
    message->SetWindowText(cs("The download limit of "+bytes(num(current,"LimitBytes"))+" every "+std::to_string(num(current,"Hours"))+" hour(s) has been reached.\r\n\r\n"+quotaWaitText(current)+"\r\nYou can change the limit in Options > Connection."));
   };d->pulse();
  };if(!d->Create(101,this))throw std::runtime_error("Cannot open the download-limit warning.");d->ShowWindow(SW_SHOWNOACTIVATE);
 }
 static i64 displayedFileSize(const Json& data){return udm::displayedFileSize(data);}

 void refreshModalSizes(){
  // Only the rows behind the modal dialog can be seen. Update the control
  // after releasing the catalog lock.
  if(!table.GetSafeHwnd()||!table.IsWindowVisible()||IsIconic())return;
  const auto first=static_cast<size_t>(std::max(0,table.GetTopIndex()));
  const auto count=static_cast<size_t>(std::max(0,table.GetCountPerPage()))+1;
  std::vector<std::pair<size_t,std::string>> changed;
  if(!tryModalSizeChanges(manager,visible,rowText,first,count,changed,[](const Json& data){return sizeCellText(data,udm::displayedFileSize(data));}))return;
  for(auto& [row,value]:changed){table.SetItemText(static_cast<int>(row),2,cs(value));rowText[row][2]=std::move(value);}
 }
 void requestDeferredHistoryRefresh(){
  if(!quitting&&historyRefreshDeferred&&!historyRefreshPosted)
   historyRefreshPosted=PostMessage(REFRESH_VISIBLE_HISTORY)!=FALSE;
 }
 void presentHttpFailureNotice(const JobPtr& job){
  if(!automaticPresentationAllowed())return;
  std::string fileName,message;
  {
   Lock lock(manager.mutex);
   if(!job->httpFailureNoticePending||manager.isActive(job)||std::find(manager.jobs.begin(),manager.jobs.end(),job)==manager.jobs.end())return;
   job->httpFailureNoticePending=false;
   if(str(job->data,"Status")!="Failed")return;
   if(yes(job->data,"BrowserVerificationRequired"))message=str(job->data,"Error");
   else if(num(job->data,"LastHttpStatus")==403)
    message="The server replies that you don't have permissions to download this file.\r\n\r\nDetails:\r\nHTTP 403 Forbidden";
   if(message.empty())return;
   fileName=str(job->data,"FileName","Download failed");
  }
  if(fileName.empty())fileName="Download failed";
  Form dialog(fileName,308,111,this);dialog.dialogUnits=true;
  dialog.init=[&]{
   auto icon=dialog.control(L"STATIC","",SS_ICON,16,16,23,20);
   icon->SendMessage(STM_SETICON,(WPARAM)LoadIconW(nullptr,IDI_WARNING));
   dialog.label(message,44,16,239,50);
   dialog.accept=[&]{dialog.close();};
   dialog.defaultButton(dialog.button("OK",238,88,56,dialog.accept,16));
  };
  dialog.DoModal();
 }
 void presentTransferEvents(){
  if(completionModalFence.blocked())return;
  if(manager.fileInfoDecisionPending||resumePresenting)return;
  if(optionsUiActive||optionsRefreshPending||optionsAppearanceSnapshot)return;
  std::vector<std::pair<JobPtr,TransferEvent>> pending;
  {std::lock_guard<std::mutex> lock(eventsMutex);pending.swap(events);}
  if(pending.empty()){pollCompletionWork();return;}
  for(const auto& [job,kind]:pending){
   if(kind==TransferEvent::Published){showCompletionDialog(job,false,true);continue;}
   if(kind==TransferEvent::Finished){queueCompletionWork(job,CompletionRequestKind::Finished);continue;}
   queueCompletionWork(job,CompletionRequestKind::Started);
  }
 }
 bool tryPresentStartedTransfer(const JobPtr& job){
   bool show;
   {
    std::unique_lock lock(manager.mutex,std::try_to_lock);if(!lock.owns_lock())return false;
    const auto& prefs=manager.state["Settings"];
    const auto& record=job->data;
    const bool completedAndClosing=str(record,"Status")=="Complete"&&!manager.isActive(job)&&yes(record,"CloseProgressOnCompletion",true);
    const bool showFinishedPrefetch=completedAndClosing&&progressOpeningPrefetch(*job,std::chrono::steady_clock::now())&&
     progressPresentation(str(prefs,"ProgressStartMode"),yes(record,"QueueOrigin"),yes(prefs,"QueueProgressMinimized",true),false)==ProgressPresentation::Normal;
    show=(!completedAndClosing||showFinishedPrefetch)&&!yes(record,"ConfirmationPending")&&!yes(prefs,"SuppressProgressDialog")&&!yes(record,"SuppressProgressDialog");
   }
   if(show)showProgress(job,false);
   return true;
 }
 void enqueueRequestedAuthentication(const JobPtr& job){
  if(std::find(requestedAuthenticationRequests.begin(),requestedAuthenticationRequests.end(),job)==requestedAuthenticationRequests.end())requestedAuthenticationRequests.push_back(job);
  pollRequestedAuthentication();
 }
 void pollRequestedAuthentication(){
  if(quitting||authenticationShowing||authenticationDialogShowing()||!automaticPresentationAllowed()||optionsUiActive||optionsRefreshPending||optionsAppearanceSnapshot)return;
  try{
   const bool completed=requestedAuthenticationTask->active()&&requestedAuthenticationTask->poll();if(requestedAuthenticationTask->active())return;
   const auto view=requestedAuthenticationTask->prepared();
   if(view){
    const auto current=tryRequestedAuthenticationCurrent(manager,*view);if(current==AutomaticPromptReadiness::Busy)return;
    if(current==AutomaticPromptReadiness::Ready){
     authenticationShowing=true;struct ShowingGuard{bool& value;~ShowingGuard(){value=false;}} showing{authenticationShowing};
#ifdef UDM_TOOLBAR_COMPONENT_TEST
     if(requestedAuthenticationPresenter)requestedAuthenticationPresenter(view);else
#endif
     {CaptureDialogOwner owner(this,true);requestDownloadLogin(owner.get(this),manager,view->prompt.job,&view->prompt);}
    }
    requestedAuthenticationTask->acknowledge();if(!requestedAuthenticationRequests.empty())requestedAuthenticationRequests.erase(requestedAuthenticationRequests.begin());
   }else if(!requestedAuthenticationRequests.empty()&&completed){requestedAuthenticationRequests.erase(requestedAuthenticationRequests.begin());}
   if(!requestedAuthenticationRequests.empty()){requestedAuthenticationTask->prepare(requestedAuthenticationRequests.front(),true,maintenanceSignal->notifier());}
  }catch(...){requestedAuthenticationTask->finish();if(!requestedAuthenticationRequests.empty())requestedAuthenticationRequests.erase(requestedAuthenticationRequests.begin());throw;}
 }
 void pollAutomaticAuthentication(){
  if(!requestedAuthenticationRequests.empty())return;
  if(authenticationTask->active()){authenticationTask->poll();if(authenticationTask->active())return;}
  if(authenticationTask->rearmPending()){authenticationTask->rearm(maintenanceSignal->notifier());return;}
  const auto prepared=authenticationTask->prepared();if(!prepared||quitting||authenticationShowing||authenticationDialogShowing()||!automaticPresentationAllowed()||optionsUiActive||optionsRefreshPending||optionsAppearanceSnapshot)return;
  const auto current=tryAuthenticationPromptCurrent(manager,*prepared);if(current==AutomaticPromptReadiness::Busy)return;
  if(current==AutomaticPromptReadiness::Obsolete){authenticationTask->rearm(maintenanceSignal->notifier());return;}
  authenticationShowing=true;struct ShowingGuard{bool& value;~ShowingGuard(){value=false;}} showing{authenticationShowing};
  try{
   bool shown=true;
#ifdef UDM_TOOLBAR_COMPONENT_TEST
   if(automaticAuthenticationPresenter)automaticAuthenticationPresenter(prepared->job);else
#endif
   {CaptureDialogOwner owner(this,true);shown=requestDownloadLogin(owner.get(this),manager,prepared->job,prepared.get());}
   if(shown)authenticationTask->acknowledge();else authenticationTask->rearm(maintenanceSignal->notifier());
  }catch(...){authenticationTask->rearm(maintenanceSignal->notifier());throw;}
 }
 void presentAutomaticPrompts(bool captureOnly,const std::set<std::string>& capturedIds){
  if(!automaticPresentationAllowed())return;pollRequestedAuthentication();pollAutomaticAuthentication();
  auto snapshot=tryAutomaticPromptSnapshot(manager,captureOnly,capturedIds,infoSeen,automaticConfirmations);if(!snapshot)return;
  automaticConfirmations=std::move(snapshot->confirmations);
  if(snapshot->authentication&&requestedAuthenticationRequests.empty()&&!authenticationTask->ownsPrompt())authenticationTask->prepare(snapshot->authentication,maintenanceSignal->notifier());
  if(manager.browserRecoveryRequested.exchange(false))captureReviewsSeen.clear();
  for(const auto& token:snapshot->reviews){
   if(!automaticPresentationAllowed())break;const auto added=captureReviewsSeen.insert(token).second;if(!added)continue;
   try{
#ifdef UDM_TOOLBAR_COMPONENT_TEST
    if(automaticReviewPresenter)automaticReviewPresenter(token);else
#endif
    {CaptureDialogOwner owner(this,true);CaptureReviewDialog dialog(manager,token,owner.get(this));if(dialog.DoModal()==-1)throw std::runtime_error("Cannot open captured download review.");}
   }catch(...){captureReviewsSeen.erase(token);throw;}
  }
  while(!automaticConfirmations.empty()&&automaticPresentationAllowed()){
   const auto candidate=automaticConfirmations.front();const auto ready=tryAutomaticConfirmationReady(manager,candidate,infoSeen);
   if(ready==AutomaticPromptReadiness::Busy)break;
   if(ready==AutomaticPromptReadiness::Obsolete){automaticConfirmations.erase(automaticConfirmations.begin());continue;}
   const auto added=infoSeen.insert(candidate.id).second;
   try{
#ifdef UDM_TOOLBAR_COMPONENT_TEST
    if(automaticConfirmationPresenter)automaticConfirmationPresenter(candidate);else
#endif
    presentDownload(this,manager,candidate.job,true);
   }catch(...){if(added)infoSeen.erase(candidate.id);throw;}
   automaticConfirmations.erase(automaticConfirmations.begin());
  }
 }
 void refresh(bool captureOnly=false,bool reviewBrowser=false){if(presentationRecoveryShowing||cancellationRecoveryShowing)return;if(optionsUiActive||optionsRefreshPending||optionsAppearanceSnapshot){historyRefreshDeferred=true;return;}if(manager.fileInfoDecisionPending)return;if(refreshing){refreshModalSizes();return;}refreshing=true;struct RefreshGuard{bool& value;~RefreshGuard(){value=false;}} refreshGuard{refreshing};try{
 Json prefs;JobPtr httpFailureNotice;
 std::set<std::string> capturedIds;
 // Browser admission must not wait for history sorting, row painting or unrelated dialogs.
 if(IsWindowEnabled()){
  const auto offerSnapshot=tryBrowserPresentationSnapshot(manager);if(!offerSnapshot){if(!captureOnly)historyRefreshDeferred=true;return;}
  const auto& offers=*offerSnapshot;
  captureRefreshRequested=false;
  for(auto entry=preparedFileInfoViews.begin();entry!=preparedFileInfoViews.end();)if(entry->second->outcome()==PendingFileInfoInbox::Outcome::Uncertain&&!entry->second->cancelRequested()){entry->second->attach();entry=preparedFileInfoViews.erase(entry);}else ++entry;
  std::vector<std::string> terminalBefore;
  for(const auto& entry:preparedFileInfoViews)if(entry.second->outcome()!=PendingFileInfoInbox::Outcome::Pending&&(!entry.second->cancelRequested()||entry.second->cancelFinished))terminalBefore.push_back(entry.first);
  for(const auto& offer:offers)capturedIds.insert(str(offer,"id"));
  for(const auto& id:terminalBefore)if(!capturedIds.count(id)){auto found=preparedFileInfoViews.find(id);if(found!=preparedFileInfoViews.end()){found->second->attach();preparedFileInfoViews.erase(found);}}
  for(const auto& offer:offers){
   if(presentationAcks.owns(offer)||preparedFileInfo->ownsRetryPresentation(str(offer,"id")))continue;
   if(auto waiting=preparedFileInfoViews.find(str(offer,"id"));waiting!=preparedFileInfoViews.end()&&waiting->second->cancelRequested()&&!waiting->second->cancelFinished)continue;
   if(automaticPresentationAllowed()){
#ifdef UDM_CAPTURE_PRESENTATION_COMPONENT_TEST
    traceCapturePresentation("pending-offer-ready");
#endif
    std::shared_ptr<const FileInfoInitialView> prepared;
    std::shared_ptr<const PendingFileInfoInbox::Operation> preparedOperation;
    if(auto found=preparedFileInfoViews.find(str(offer,"id"));found!=preparedFileInfoViews.end()){
     preparedOperation=found->second;
     preparedFileInfoViews.erase(found);
    }else{
     preparedOperation=preparedFileInfo->takeForJob(str(offer,"id"));
#ifdef UDM_PRODUCTION_UI_TIMING_TEST
     if(preparedOperation)nativeUiTraceRecord("prepared-view-claimed");
#endif
    }
    // The saved offer and restoreBrowserPresentation prove acceptance even if
    // this UI thread runs between live installation and the worker's return.
    #ifdef UDM_TOOLBAR_COMPONENT_TEST
    if(automaticBrowserPresenter)automaticBrowserPresenter(offer);else
#endif
    if(preparedOperation&&preparedOperation->view->media&&!preparedOperation->view->formRequest&&str(offer,"kind")=="download"&&!preparedOperation->cancelRequested())presentPendingBrowserCapture(this,manager,preparedOperation,presentationAcknowledger());
    else{
     if(preparedOperation&&preparedOperation->outcome()!=PendingFileInfoInbox::Outcome::Uncertain&&preparedOperation->attach())prepared=preparedOperation->view;
     presentBrowserCapture(this,manager,offer,std::move(prepared),presentationAcknowledger());
    }
    if(IsWindowEnabled())presentTransferEvents();
   }
  }
 }
 if(reviewBrowser&&!captureRefreshRequested)reviewBrowserRequests();
 if(!captureOnly){
 const bool historyVisible=IsWindowVisible()&&!IsIconic();
 const auto searchQuery=normalizedFindQuery(findQuery);auto filterQuery=searchQuery;if(!findFilterActive){filterQuery["Text"]="";filterQuery.erase("TextUtf16");}const auto searchText=str(filterQuery,"Text");const DownloadSearch filterMatcher(filterQuery);
 const bool showAdded=table.GetColumnWidth(8)>0,showPath=table.GetColumnWidth(9)>0,showReferer=table.GetColumnWidth(10)>0,showRecovery=table.GetColumnWidth(11)>0;
 HistoryReadRequest read;read.visible=historyVisible;read.includeHeaders=showReferer||showRecovery||sortColumn==10||sortColumn==11||!searchText.empty();read.searchActive=!searchText.empty();read.filter=filter;read.searchQuery=searchQuery;read.filterQuery=filterQuery;read.queueImages=queueImages;
 if(historyVisible){
  read.selected=selected(false);const int focus=table.GetNextItem(-1,LVNI_FOCUSED);if(focus>=0&&focus<(int)visible.size())read.focused=visible[focus];
  if(treeSnapshotValid&&!lastTree.empty()&&treeQueueNodes.size()==treeSnapshot.queueNames.size()&&treeQueueImages.size()==treeSnapshot.queueNames.size())read.previousTree=&treeSnapshot;
  const auto caret=tree.GetSelectedItem();if(caret){const auto index=tree.GetItemData(caret);if(index<filters.size()&&filters[index]==filter)for(auto item=caret;item;item=tree.GetParentItem(item)){const auto found=treeKeys.find(item);if(found!=treeKeys.end())read.selectedAncestors.push_back(found->second);}}
 }
 auto captured=tryHistoryReadSnapshot(manager,read,listRecordCache,findFilterCache);if(!captured){historyRefreshDeferred=true;return;}auto& model=*captured;prefs=model.preferences;
 std::vector<std::string> sounds;
 for(const auto& observation:model.observations){
  if(!httpFailureNotice&&observation.httpFailure)httpFailureNotice=observation.job;
  const auto& id=observation.id;const auto& state=observation.state;const auto previous=observedStates.find(id);
  if(previous!=observedStates.end()&&previous->second!=state){if(state=="Failed")sounds.push_back("Failure");if(state=="Paused"&&(previous->second=="Downloading"||previous->second=="Pausing"))sounds.push_back("Pause");}
  observedStates[id]=state;
 }
 const auto playObservedSounds=[&]{for(const auto& sound:sounds)playDownloadSound(prefs,(sound+"SoundEnabled").c_str(),(sound+"SoundFile").c_str());};
 historyRefreshDeferred=!historyVisible;
 if(!historyRefreshDeferred){
 table.sortIndicator(sortColumn,ascending);buildTree(std::move(model.navigation));
 const Json view=Json::array({filter,findFilterActive,filterQuery,sortColumn,ascending});const bool viewChanged=rowView!=view;
 const int previousTop=table.GetTopIndex();JobPtr scrollAnchor=(!viewChanged&&previousTop>0&&previousTop<(int)visible.size())?visible[previousTop]:nullptr;
 CRect anchorRect;const int anchorY=scrollAnchor&&table.GetItemRect(previousTop,&anchorRect,LVIR_BOUNDS)?anchorRect.top:0;
 auto selection=std::move(model.selection);auto focusedId=std::move(model.focusedId);auto next=std::move(model.rows);auto searchRecords=std::move(model.searchRecords);auto cachedSearchMatches=std::move(model.searchMatches);auto quotaWaiting=std::move(model.quotaWaiting);auto storageWarning=std::move(model.storageWarning);
 std::unordered_map<const Job*,std::shared_ptr<const DownloadListRow>> snapshotRows;snapshotRows.reserve(model.records.size());for(const auto& entry:model.records)snapshotRows.emplace(entry.first.get(),entry.second);
 listRecordCache.swap(model.records);model.records.clear();
 // Everything below uses immutable per-refresh records. No sorting, DPAPI
 // header decoding, shell icon lookup or list control messages hold the manager lock.
 playObservedSounds();
 if(!searchText.empty())next.erase(std::remove_if(next.begin(),next.end(),[&](const JobPtr& job){const auto cached=cachedSearchMatches.find(job);return !(cached!=cachedSearchMatches.end()?cached->second:findFilterCache.matches(job,std::move(searchRecords.at(job)),filterMatcher,&searchQuery));}),next.end());
 findFilterCache.finish();searchRecords.clear();
 // Reuse keys across ticks while their encrypted header source is unchanged.
 // Entries remain owned by refererKeyCache for this entire refresh.
 std::map<JobPtr,std::string_view> refererSortKeys;
 if(sortColumn==10){refererKeyCache.begin();for(const auto& job:next)refererSortKeys.emplace(job,refererKeyCache.get(str(snapshotRows.at(job.get())->data,"Id"),str(snapshotRows.at(job.get())->data,"ProtectedHeaders"),[&]{return readHeaders(snapshotRows.at(job.get())->data)["Referer"];}));refererKeyCache.finish();}else refererKeyCache.clear();
 // Derive numeric/status keys once from the captured row records.
 // Sorting never recalculates progress, ETA or dates inside the comparator.
 std::unordered_map<const Job*,i64> numericSortKeys;
 std::unordered_map<const Job*,DownloadListStatusKey> statusSortKeys;
 std::unordered_map<const Job*,DownloadListEtaKey> etaSortKeys;
 std::unordered_map<const Job*,DownloadListSizeKey> sizeSortKeys;
 if(sortColumn==5||sortColumn==6||sortColumn==8){
  numericSortKeys.reserve(next.size());
  for(const auto& job:next){i64 key=0;switch(sortColumn){
   case 5:key=downloadListIntegerRate(snapshotRows.at(job.get())->speed);break;
   default:key=parseDate(snapshotRows.at(job.get())->data.value(sortColumn==6?"LastAttempt":"Added",Json()));break;
  }numericSortKeys.emplace(job.get(),key);}
 }else if(sortColumn==2){
  sizeSortKeys.reserve(next.size());
  for(const auto& job:next){const auto& data=snapshotRows.at(job.get())->data;sizeSortKeys.emplace(job.get(),downloadListSizeKey(data,displayedFileSize(data)));}
 }else if(sortColumn==4){
  etaSortKeys.reserve(next.size());
  for(const auto& job:next)etaSortKeys.emplace(job.get(),downloadListEtaKey(snapshotRows.at(job.get())->data,snapshotRows.at(job.get())->speed));
 }else if(sortColumn==3){
  statusSortKeys.reserve(next.size());
  for(const auto& job:next)statusSortKeys.emplace(job.get(),downloadListStatusKey(snapshotRows.at(job.get())->data));
 }
 // Text conversion and path construction are per-row work, not per-comparison
 // work. Keep keys local so renames, queue changes and recovery edits are seen
 // on the next refresh without introducing another invalidation cache.
 std::unordered_map<const Job*,std::string> textSortKeys;
 if(sortColumn>=0&&(sortColumn==1||sortColumn==7||sortColumn==9||sortColumn==11||sortColumn>11)){
  textSortKeys.reserve(next.size());
  for(const auto& job:next){std::string key;switch(sortColumn){
   case 1:key=queueColumnText(snapshotRows.at(job.get())->data);break;
   case 7:key=str(snapshotRows.at(job.get())->data,"Description");break;
   case 9:key=utf8(downloadListTarget(snapshotRows.at(job.get())->data).wstring());break;
   case 11:key=recoveryPage(snapshotRows.at(job.get())->data);break;
   default:key=str(snapshotRows.at(job.get())->data,"FileName");break;
  }textSortKeys.emplace(job.get(),sortColumn==7||sortColumn==9||sortColumn==11?std::move(key):lower(key));}
 }
 if(sortColumn>=0){
  auto sameSortKey=[&](const JobPtr& job,const DownloadListSortKey& previous){
   switch(sortColumn){
    case 0:return previous.text==downloadListFilename(snapshotRows.at(job.get())->data);
    case 5:case 6:case 8:return previous.integer==numericSortKeys.at(job.get());
    case 2:{const auto& key=sizeSortKeys.at(job.get());return previous.integer==key.bytes&&previous.secondaryInteger==key.durationMs;}
    case 4:{const auto& key=etaSortKeys.at(job.get());return previous.integer==key.seconds&&previous.completed==key.complete;}
    case 3:{const auto& key=statusSortKeys.at(job.get());return previous.text==key.label&&previous.integer==static_cast<i64>(key.complete)&&previous.rate==key.progress;}
    case 10:return previous.text==refererSortKeys.at(job);
    default:return previous.text==textSortKeys.at(job.get());
   }
  };
  if(!listOrderCache.reuse(sortColumn,ascending,next,sameSortKey,next)){
   auto sortSource=next;
   // Convert bounded reference keys once per row, only on a sort cache miss.
   // Raw full strings above conservatively invalidate reuse after any edit.
   std::unordered_map<const Job*,std::wstring> wideSortKeys;
   if(sortColumn==0||sortColumn==9||sortColumn==11){wideSortKeys.reserve(next.size());for(const auto& job:next)wideSortKeys.emplace(job.get(),referenceWideSortKey(sortColumn==0?downloadListFilename(snapshotRows.at(job.get())->data):textSortKeys.at(job.get())));}
   std::unordered_map<const Job*,std::string> ansiSortKeys;
   if(sortColumn==7||sortColumn==10){ansiSortKeys.reserve(next.size());for(const auto& job:next)ansiSortKeys.emplace(job.get(),referenceAnsiSortKey(sortColumn==10?refererSortKeys.at(job):std::string_view(textSortKeys.at(job.get()))));}
   bool ansiByteFallback=false;
   auto compareRows=[&](const JobPtr& a,const JobPtr& b){
    auto compare=[](auto x,auto y){return x<y?-1:x>y?1:0;};int cmp=0;
    switch(sortColumn){
     case 0:cmp=StrCmpLogicalW(wideSortKeys.at(a.get()).c_str(),wideSortKeys.at(b.get()).c_str());break;
     case 1:cmp=textSortKeys.at(a.get()).compare(textSortKeys.at(b.get()));break;
     case 5:case 6:case 8:cmp=compare(numericSortKeys.at(a.get()),numericSortKeys.at(b.get()));break;
     case 2:cmp=compareDownloadListSize(sizeSortKeys.at(a.get()),sizeSortKeys.at(b.get()));break;
     case 4:cmp=compareDownloadListEta(etaSortKeys.at(a.get()),etaSortKeys.at(b.get()));break;
     case 3:cmp=compareDownloadListStatus(statusSortKeys.at(a.get()),statusSortKeys.at(b.get()),ansiByteFallback);break;
     case 9:case 11:cmp=compareOptionalWideText(wideSortKeys.at(a.get()),wideSortKeys.at(b.get()));break;
     case 7:case 10:cmp=compareOptionalAnsiText(ansiSortKeys.at(a.get()),ansiSortKeys.at(b.get()),ansiByteFallback);break;
     default:cmp=textSortKeys.at(a.get()).compare(textSortKeys.at(b.get()));break;
    }
    return ascending?cmp<0:cmp>0;
   };
   try{std::stable_sort(next.begin(),next.end(),compareRows);}
   catch(const ReferenceTextCompareError&){
    // Never mix a CRT error sentinel with valid ordering results. Restart from
    // catalog order with one consistent byte comparator for the whole sort.
    ansiByteFallback=true;next=sortSource;
    std::stable_sort(next.begin(),next.end(),compareRows);
   }
   std::vector<DownloadListSortKey> savedKeys;savedKeys.reserve(sortSource.size());
   for(const auto& job:sortSource){DownloadListSortKey key;
    switch(sortColumn){
     case 0:key.text=downloadListFilename(snapshotRows.at(job.get())->data);break;
     case 5:case 6:case 8:key.integer=numericSortKeys.at(job.get());break;
     case 2:{const auto& size=sizeSortKeys.at(job.get());key.integer=size.bytes;key.secondaryInteger=size.durationMs;break;}
     case 4:{const auto& eta=etaSortKeys.at(job.get());key.integer=eta.seconds;key.completed=eta.complete;break;}
     case 3:{auto& statusKey=statusSortKeys.at(job.get());key.text=std::move(statusKey.label);key.integer=statusKey.complete;key.rate=statusKey.progress;break;}
     case 10:key.text=refererSortKeys.at(job);break;
     default:key.text=std::move(textSortKeys.at(job.get()));break;
    }savedKeys.push_back(std::move(key));
   }
   listOrderCache.remember(sortColumn,ascending,std::move(sortSource),std::move(savedKeys),next);
  }
 }else listOrderCache.clear();
 bool rebuild=next!=visible;if(rebuild)table.SetRedraw(FALSE);const bool settingsChanged=rowPreferences!=prefs||rowTree!=lastTree||rowAdded!=showAdded||rowPath!=showPath||rowReferer!=showReferer||rowRecovery!=showRecovery;if(rebuild){table.DeleteAllItems();visible=next;rowText.assign(visible.size(),{});rowImages.assign(visible.size(),std::array<int,2>{-1,-1});rowSnapshots.assign(visible.size(),std::shared_ptr<const DownloadListRow>{});rowQuota.assign(visible.size(),false);}for(int i=0;i<(int)visible.size();++i){auto& cached=rowText[i];auto& icons=rowImages[i];auto j=visible[i];const auto& row=*snapshotRows.at(j.get());const auto& d=row.data;const bool waiting=quotaWaiting.count(j)>0;if(!rebuild&&!settingsChanged&&rowSnapshots[i]==snapshotRows.at(j.get())&&rowQuota[i]==waiting)continue;auto state=str(d,"Status"),file=str(d,"FileName");if(rebuild){icons[0]=fileImage(d);table.InsertItem(i,cs(file),icons[0]);cached[0]=file;if(selection.count(str(d,"Id")))table.SetItemState(i,LVIS_SELECTED,LVIS_SELECTED);if(focusedId==str(d,"Id"))table.SetItemState(i,LVIS_FOCUSED,LVIS_FOCUSED);}else{if(cached[0]!=file){table.SetItemText(i,0,cs(file));cached[0]=file;}const int expected=fileImage(d);if(icons[0]!=expected){LVITEMW item{};item.mask=LVIF_IMAGE;item.iItem=i;item.iImage=expected;table.SetItem(&item);icons[0]=expected;}}auto total=displayedFileSize(d);auto rowProgress=downloadPermille(d);auto eta=downloadListSecondsLeft(d,row.speed);const auto listRate=downloadListIntegerRate(row.speed);LVITEMW queueItem{};queueItem.mask=LVIF_IMAGE;queueItem.iItem=i;queueItem.iSubItem=1;const int queueImage=static_cast<int>(num(d,"ListQueueImage",I_IMAGENONE));if(icons[1]!=queueImage){queueItem.iImage=queueImage;table.SetItem(&queueItem);icons[1]=queueImage;}std::string values[]={"",sizeCellText(d,total),waiting?"Waiting for quota":statusText(d,state,rowProgress),idmDuration(eta),listRate>0?bytes(static_cast<double>(listRate))+"/sec":"",str(d,"LastAttempt").empty()?"":dateText(d.value("LastAttempt",Json())),str(d,"Description"),(showAdded?dateText(d.value("Added",Json())):""),(showPath?utf8(downloadListTarget(d).wstring()):""),(showReferer?readHeaders(d)["Referer"]:""),(showRecovery?recoveryPage(d):"")};for(int c=0;c<11;++c)if(cached[c+1]!=values[c]){table.SetItemText(i,c+1,cs(values[c]));cached[c+1]=values[c];}rowSnapshots[i]=snapshotRows.at(j.get());rowQuota[i]=waiting;}rowPreferences=prefs;rowView=view;rowTree=lastTree;rowAdded=showAdded;rowPath=showPath;rowReferer=showReferer;rowRecovery=showRecovery;if(rebuild){
 // Keep the same history row at the top during automatic list changes.
 // Explicit sort, search and category changes start their new view normally.
 if(scrollAnchor&&!visible.empty()){
  const auto found=std::find(visible.begin(),visible.end(),scrollAnchor);
  const int row=found!=visible.end()?(int)(found-visible.begin()):std::min(previousTop,(int)visible.size()-1);
  table.EnsureVisible(row,FALSE);CRect rect;
  if(table.GetItemRect(row,&rect,LVIR_BOUNDS))table.Scroll(CSize(0,rect.top-anchorY));
 }
 table.SetRedraw(TRUE);table.Invalidate(FALSE);
 }
 // Common controls can retain the old scroll offset across DeleteAllItems,
 // and an explicit view change may not rebuild rows at all.
 if(viewChanged&&!visible.empty())table.EnsureVisible(0,FALSE);
 std::optional<size_t> findSelection;if(rebuild||findCursor.orderChanged(sortColumn,ascending)){const int first=table.GetNextItem(-1,LVNI_SELECTED);if(first>=0)findSelection=(size_t)first;}
 findCursor.updateView(Json::array({filter,findFilterActive,filterQuery}),sortColumn,ascending,rebuild,findSelection);
 showStatus(storageWarning);updateFileIcons();updateToolbarCommands();
 }else{
  playObservedSounds();
 }
 if(yes(prefs,"ClipboardMonitor")){auto snapshot=clipboardSnapshot.read([]{return GetClipboardSequenceNumber();},[this]()->std::optional<std::string>{bool readable=false;auto value=clipboardText(&readable);if(!readable)return std::nullopt;return value;});if(snapshot){const auto& value=*snapshot;if(value!=lastClipboard){lastClipboard=value;if(clipboardCandidate(value,prefs)){Url u(value);copiedUrl=value;if(str(prefs,"ClipboardMode")=="Open Download File Info"){clipboard.ShowWindow(SW_HIDE);if(!clipboardPromptPending){clipboardPromptPending=true;PostMessage(WM_COMMAND,CMD_CLIPBOARD_CAPTURE);}}else{clipboard.SetWindowText(cs("Download copied URL: "+u.host+u.path));clipboard.ShowWindow(SW_SHOW);}}else{copiedUrl.clear();clipboard.ShowWindow(SW_HIDE);}}}}else{clipboardSnapshot.reset();clipboard.ShowWindow(SW_HIDE);}
 presentTransferEvents();
 }
 if(IsWindowEnabled())presentAutomaticPrompts(captureOnly,capturedIds);
 if(httpFailureNotice&&IsWindowEnabled())presentHttpFailureNotice(httpFailureNotice);
 }catch(const std::exception& e){showStatus(e.what());}refreshing=false;
 // A notification delivered inside a modal refresh is picked up after it closes.
 wakeDeferredAutomaticPresentation();
 if(!quitting&&IsWindowEnabled()){
  if(captureRefreshRequested)PostMessage(PRESENT_CAPTURE);
  if(IsWindowVisible()&&!IsIconic())requestDeferredHistoryRefresh();
  std::lock_guard<std::mutex> lock(eventsMutex);if(!events.empty())PostMessage(TRANSFER_EVENTS);
 }
 }

 std::string textValue(int row,int column){return utf8((LPCWSTR)table.GetItemText(row,column));}
 // IDM list formats: "Aug 05 09:14" dates, "19 min 26 sec" countdowns, two-decimal progress in the Status column.
 static std::string dateText(const Json& d){auto ms=parseDate(d);if(!ms)return "";auto t=toLocal(ms);static const char* months[]={"Jan","Feb","Mar","Apr","May","Jun","Jul","Aug","Sep","Oct","Nov","Dec"};char text[40];sprintf_s(text,"%s %02u %02u:%02u",months[(t.wMonth-1)%12],t.wDay,t.wHour,t.wMinute);return text;}
 static std::string idmDuration(i64 seconds){return downloadTimeLeftText(seconds);}
 static std::string statusText(const Json& d,const std::string& state,int permille){return downloadStatusText(d,state,permille);}

 AsyncFileIcons fileIconLoader;std::map<std::string,int> fileTypeImages;
 int fileImage(const Json& data,bool load=false){
  const auto category=lower(str(data,"Category"));const int fallback=iconIndex.count(category)?iconIndex.at(category):0;
  const auto extension=lower(utf8(fs::path(wide(str(data,"FileName"))).extension().wstring()));if(extension.size()<2||extension.size()>12)return fallback;
  const auto found=fileTypeImages.find(extension);if(found!=fileTypeImages.end())return found->second>=0?found->second:fallback;
  if(load)fileIconLoader.request(extension);return fallback;
 }
 void updateFileIcons(){
  if(!table.GetSafeHwnd()||!images.GetSafeHandle())return;
  for(auto& result:fileIconLoader.take())if(result.generation==fileIconLoader.generation())fileTypeImages[result.extension]=result.icon?images.Add(result.icon):-1;
  if(!table.IsWindowVisible())return;
  const int top=std::max(0,table.GetTopIndex()),end=std::min({static_cast<int>(visible.size()),static_cast<int>(rowSnapshots.size()),static_cast<int>(rowImages.size()),top+std::max(0,table.GetCountPerPage())+2});
  for(int row=top;row<end;++row){
   if(!rowSnapshots[row])continue;const int icon=fileImage(rowSnapshots[row]->data,true);
   if(rowImages[row][0]==icon)continue;LVITEMW item{};item.mask=LVIF_IMAGE;item.iItem=row;item.iImage=icon;table.SetItem(&item);rowImages[row][0]=icon;
  }
 }

 bool statusShown()const{return status.GetSafeHwnd()&&(::GetWindowLongW(status.GetSafeHwnd(),GWL_STYLE)&WS_VISIBLE);}
 void showStatus(const std::string& text){
  if(!status.GetSafeHwnd())return;const bool show=!text.empty();if(show)status.SetText(cs(text),0,0);
  if(statusShown()!=show){status.ShowWindow(show?SW_SHOW:SW_HIDE);CRect area;GetClientRect(&area);layout(area.Width(),area.Height());}
 }
 void showTipOfDay(){
  static const std::vector<std::string> tips={
   "Double-click a completed download to open its properties. You can change this behavior from the download context menu.",
   "Use Downloads > Speed Limiter to turn the limit off without forgetting your chosen rate.",
   "Pause All keeps progress windows open. Stop All stops transfers and closes unfinished progress windows.",
   "An expired address can be replaced through Refresh download address. Review the new link before resuming saved parts.",
   "Use Downloads > Start queue to run a named queue. Scheduler lets you configure its times and parallel downloads.",
   "Press Ctrl+F to find a download and F3 to find the next match.",
   "View > Customize URL List lets you choose the columns shown in the download list.",
   "Backup and recovery is under Tasks > Export. Pause active transfers before opening it."
  };
  auto settings=preferences();size_t index=(size_t)std::max<int64_t>(0,num(settings,"NextTipIndex"))%tips.size();
  Form dialog("Tip of the Day",253,164,this);dialog.dialogUnits=true;
  dialog.init=[&]{dialog.control(L"STATIC","",SS_ETCHEDFRAME,9,11,232,123);auto art=dialog.control(L"STATIC","",SS_ICON|SS_CENTERIMAGE,17,17,23,26);art->SendMessage(STM_SETICON,(WPARAM)AfxGetApp()->LoadIcon(1)); // IDM: 399x246 picture panel with a 40x53 image at its top left
   dialog.label("Did you know...",55,29,168,12);auto body=dialog.label(tips[index],28,57,196,66);
   auto startup=dialog.check("&Show Tips on StartUp",yes(settings,"ShowTipsOnStartup",true),12,146,101,10);
   auto finish=[&,startup]{auto current=preferences();current["ShowTipsOnStartup"]=dialog.checked(startup);current["NextTipIndex"]=(index+1)%tips.size();manager.setSettings(current);dialog.close();};
   dialog.button("&Next Tip",121,143,54,[&,body]{index=(index+1)%tips.size();body->SetWindowText(cs(tips[index]));},14);
   dialog.defaultButton(dialog.button("&Close",183,143,54,finish,14));dialog.accept=finish;dialog.cancel=finish;
  };dialog.DoModal();
 }
 void showHelpTopic(size_t topic){
  Form dialog("UDM Help",620,420,this);auto titles=helpTopicTitles();std::function<void()> show;
  dialog.init=[&]{auto chooser=dialog.combo(titles,titles.at(topic),12,12,596);auto body=dialog.edit(helpTopicText(topic),12,45,596,325,true,true);
   show=[&,chooser,body]{chooser->SetCurSel((int)topic);body->SetWindowText(cs(helpTopicText(topic)));};
   dialog.bindChange(chooser,[&,chooser]{auto selected=chooser->GetCurSel();if(selected>=0&&(size_t)selected<titles.size()){topic=(size_t)selected;show();}});
   dialog.button("Previous",12,383,90,[&]{topic=(topic+titles.size()-1)%titles.size();show();});dialog.button("Next",110,383,90,[&]{topic=(topic+1)%titles.size();show();});
   dialog.defaultButton(dialog.button("Close",518,383,90,[&]{dialog.close();}));dialog.accept=[&]{dialog.close();};
  };dialog.DoModal();
 }
 void command(UINT id){try{if((appearanceClosePending&&quitting)||(appearanceCommandTask.valid()&&!globalLimitIndependentCommand(id)))return;if(appearanceClosePending)appearanceClosePending=false;if(exitLayoutTask.valid()||(globalLimitClosePending&&quitting)||(globalLimitCommandTask.valid()&&!globalLimitIndependentCommand(id)))return;if(globalLimitClosePending)globalLimitClosePending=false;cancelDownloadMenu();cancelClosePolicy();
 if(id==CMD_EDIT_DOWNLOAD_SCHEDULE||id==CMD_SCHEDULE_EXISTING_DOWNLOAD){
  if(quitting||resumePresenting||authenticationDialogShowing()||fileInfoDialogShowing()||!IsWindowEnabled()||optionsUiActive||optionsRefreshPending||optionsAppearanceSnapshot||manager.fileInfoDecisionPending)return;
  const auto chosen=selected(false);if(chosen.size()!=1)return;
  scheduleListedDownload(this,manager,chosen.front(),id==CMD_EDIT_DOWNLOAD_SCHEDULE);PostMessage(WM_TIMER,1,0);return;
 }
 if(id==CMD_CLIPBOARD_CAPTURE){presentClipboardCapture();return;}if(id==CMD_BROWSER&&!automaticPresentationAllowed()){manager.browserSettingsRequested=true;return;}if(id==CMD_RESUME){if(!quitting&&!resumePresenting&&!authenticationDialogShowing()&&IsWindowEnabled()&&!optionsUiActive&&!optionsRefreshPending&&!optionsAppearanceSnapshot)resumeSelectedDownloads(selected(false));return;}if(runQueueMenuCommand(id)){refresh();return;}if(auto column=arrangeColumn(id);column>=0){sortColumn=column;ascending=true;rememberLayout();refresh();return;}if(toolbarMenuAction(id))return;if(!commandEnabled(id,true))return;
 // Local view commands and Exit do not consume download records. Validating
 // an unrelated selection here would wait for a deferred history refresh.
 // Export accepts a selection without requiring one for command availability.
 const bool capturesSelection=commandUsesSelection(id)||id==CMD_EXPORT||id==CMD_EXPORT_IDM||id==CMD_EXPORT_TEXT;
 auto chosen=capturesSelection?selected():std::vector<JobPtr>{};switch(id){
 case CMD_CLIPBOARD_CAPTURE:presentClipboardCapture();break;
 case CMD_ZIP:previewZip(chosen[0]);break;
 case CMD_BASKET:case CMD_HIDE_BASKET:preference("DropBasket",id==CMD_BASKET&&!yes(preferences(),"DropBasket"));showBasket();break;
 case CMD_RECYCLE:recycleDownloads(chosen);break;
 case CMD_OPEN:case CMD_OPENWITH:{
  if(chosen.size()!=1)break;
  fs::path target;
  {
   Lock lock(manager.mutex);
   if(!canOpenDownloadLocked(chosen[0]))break;
   target=chosen[0]->target();
  }
  if(id==CMD_OPEN)openDownloadedFile(this,target);
  else if(confirmDownloadedFilePresent(this,target))openWith(this,target);
  break;
 }
 case CMD_RELOCATE:if(chosen.size()==1)moveCompleted(this,manager,chosen[0]);else moveCompletedToFolder(this,manager,chosen);break;
 case CMD_REDOWNLOAD:{auto copy=manager.redownload(chosen[0]);downloadInfo(this,manager,copy);break;}
 case CMD_DOUBLE_OPEN:preference("CompletedDoubleClick","Open");break;
 case CMD_DOUBLE_PROPERTIES:preference("CompletedDoubleClick","Properties");break;
 case CMD_COLUMNS:customizeColumns();break;
 case CMD_TOOLBAR:toolbar.Customize();break;
 case CMD_EDITCATEGORY:if(!currentCategory().empty())categoryEditor(currentCategory());break;
 case CMD_DELETECATEGORY:if(!currentCategory().empty()&&MessageBox(L"Delete this category? Its records will move to Other; downloaded files stay in place.",L"Delete category",MB_YESNO|MB_ICONQUESTION)==IDYES){manager.deleteCategory(currentCategory());filter="all";}break;
 case CMD_DELETEQUEUE:if(filter.rfind("queue:",0)==0&&MessageBox(L"Delete this queue? Its records will be reassigned to another queue.",L"Delete queue",MB_YESNO|MB_ICONQUESTION)==IDYES){manager.deleteQueue(selectedQueue());filter="all";}break;
 case CMD_STARTQUEUEMENU:case CMD_STOPQUEUEMENU:{CPoint point;GetCursorPos(&point);queuePopup(id==CMD_STARTQUEUEMENU,point);break;}
 case CMD_LIMIT_ON:case CMD_LIMIT_OFF:startGlobalLimitCommand(id==CMD_LIMIT_ON);break;
 case CMD_LIMIT:globalLimitSettingsDialog();break;
 case CMD_SELECTALL:table.SetItemState(-1,LVIS_SELECTED,LVIS_SELECTED);break;
 case CMD_DARK:startAppearanceCommand({AppearanceChange::ToggleDark});break;
 case CMD_HOME:openProductLink(m_hWnd,productHome);break;case CMD_SUPPORT:openProductLink(m_hWnd,productSupport);break;case CMD_SHARE:showShareDialog(this);break;case CMD_CHECK_UPDATES:startUpdateCheck(true);break;case CMD_TIP_DAY:showTipOfDay();break;case CMD_HELP_CONTENTS:case CMD_HELP_TUTORIALS:case CMD_HELP_SCHEDULER:case CMD_HELP_GRABBER:showHelpTopic(id-CMD_HELP_CONTENTS);break;case CMD_LANGUAGE_ENGLISH:break;case CMD_FONT:chooseFont();break;
 case CMD_FONT_RESET:startAppearanceCommand({AppearanceChange::ResetFont});break;
 case CMD_TRAY_COLOR:startAppearanceCommand({AppearanceChange::Tray,{},11,400,"Color"});break;
 case CMD_TRAY_SYSTEM:startAppearanceCommand({AppearanceChange::Tray,{},11,400,"Classic"});break;
 case CMD_TRAY_HIDE:startAppearanceCommand({AppearanceChange::Tray,{},11,400,"Hidden"});break;
 case CMD_FINDNEXT:findNext();break;
 case CMD_HIDE_CATEGORIES:startAppearanceCommand({AppearanceChange::ToggleCategories});break;
 case CMD_REMOVEQUEUE:setQueueMembershipBatch(manager,chosen,false);break;
 case CMD_ADD:addAddress(this,manager,clipboardAddress());break;
 case CMD_ADD_SCHEDULED_DOWNLOAD:addScheduledDownload(this,manager,clipboardAddress());break;
 case CMD_PASTE:{auto value=copiedUrl.empty()?clipboardText():copiedUrl;clipboard.ShowWindow(SW_HIDE);copiedUrl.clear();if(value.find('\n')!=std::string::npos)selectExpandedBatch(this,manager,value);else addAddress(this,manager,value);break;}case CMD_BATCH:batchDialog(this,manager,clipboardAddress());break;case CMD_BATCH_CLIPBOARD:selectBatch(this,manager,importTextUrls(clipboardText()));break;case CMD_RESUME:resumeSelectedDownloads(chosen);break;case CMD_STOP:{std::set<std::string> stopped;for(auto j:chosen)if(commandCanStop(j)){manager.pause(j);stopped.insert(j->id());}closeStoppedProgress(stopped);break;}case CMD_STOPALL:case CMD_STOP_CLOSE_ALL:{std::vector<JobPtr> jobs;std::set<std::string> stopped;{Lock l(manager.mutex);jobs=manager.jobs;for(auto j:jobs)if(str(j->data,"Status")!="Complete"||manager.isActive(j))stopped.insert(j->id());}for(auto j:jobs)if(commandCanStop(j))manager.pause(j);if(id==CMD_STOP_CLOSE_ALL)closeStoppedProgress(stopped);break;}case CMD_DELETE:removeDownloadSelection(this,manager,chosen);break;case CMD_CLEAN:{std::vector<JobPtr> completed;{Lock l(manager.mutex);for(auto j:manager.jobs)if(str(j->data,"Status")=="Complete"&&!manager.isActive(j))completed.push_back(j);}removeDownloadSelection(this,manager,completed);}break;case CMD_OPTIONS:openMainOptions();return;case CMD_SCHEDULER:{Scheduler d(manager,this,selectedQueue());d.DoModal();break;}case CMD_STARTQUEUE:case CMD_STOPQUEUE:runPrimaryDownloadQueue(id==CMD_STARTQUEUE);break;case CMD_GRABBER:{GrabberDialog d(manager,this);d.DoModal();break;}case CMD_PROPERTIES:if(!chosen.empty()){if(str(chosen[0]->data,"Status")=="Complete")completedProperties(this,manager,chosen[0]);else if(!str(chosen[0]->data,"DuplicateOf").empty())presentDownload(this,manager,chosen[0]);else downloadInfo(this,manager,chosen[0],true);}break;case CMD_PROGRESS:if(!chosen.empty()){if(!str(chosen[0]->data,"DuplicateOf").empty())presentDownload(this,manager,chosen[0]);else showProgress(chosen[0]);}break;case CMD_REFRESH_ADDRESS:if(!chosen.empty())refreshDownloadAddress(this,manager,chosen[0]);break; case CMD_FOLDER:if(!chosen.empty())requestShellFolder(chosen[0]->target().parent_path());break;case CMD_NETWORK:{if(!networkCapture)networkCapture=std::make_unique<NetworkCaptureSession>(&manager,maintenanceSignal->notifier());NetworkDialog d(this,&manager,networkCapture.get());d.DoModal();break;}case CMD_BROWSER:openBrowserIntegration();return;case CMD_ABOUT:showAboutDialog(this,preferences());break;case CMD_IMPORT_IDM:importDownloads(1);break;case CMD_IMPORT_TEXT:importDownloads(2);break;case CMD_EXPORT_IDM:exportDownloads(chosen,2);break;case CMD_EXPORT_TEXT:exportDownloads(chosen,0);break;case CMD_IMPORT:importDownloads();break;case CMD_EXPORT:exportDownloads(chosen);break;case CMD_RECOVERY:{{Lock lock(manager.mutex);for(auto job:manager.jobs)if(manager.isActive(job))throw std::runtime_error("Pause active downloads before opening backup and recovery.");}recoveryOnClose=true;quitting=true;OnClose();return;}case CMD_EXIT:quitting=true;OnClose();return;case CMD_NEWQUEUE:{auto name=prompt(this,"Create new queue");if(!name.empty()){auto q=defaultQueue(name);q["Enabled"]=false;{Lock lock(manager.mutex);for(const auto& current:manager.state["Queues"])if(lower(str(current,"Name"))==lower(name))throw std::runtime_error("That queue already exists.");manager.setQueue(q);}Scheduler d(manager,this,name);d.DoModal();}break;}case CMD_NEWCATEGORY:categoryEditor();break;case CMD_UP:if(!chosen.empty())manager.move(chosen[0],-1);break;case CMD_DOWN:if(!chosen.empty())manager.move(chosen[0],1);break;case CMD_MOVEQUEUE:if(!chosen.empty()){auto choices=membershipQueues(chosen);if(choices.empty())throw std::runtime_error("Create a synchronization queue in Scheduler for completed files.");auto current=str(chosen[0]->data,"Queue");if(std::find(choices.begin(),choices.end(),current)==choices.end())current=choices.front();const bool adding=std::none_of(chosen.begin(),chosen.end(),[](const JobPtr& job){return downloadQueueMember(job->data);});Form d(adding?"Add to queue":"Move to queue",349,103,this);d.init=[&]{auto queue=d.queueCombo(manager,choices,current,13,14,322);d.button(adding?"Add":"Move",163,64,82,[&,queue]{setQueueMembershipBatch(manager,chosen,true,queue->queueName());d.close();});d.button("Cancel",257,64,79,[&]{d.close(IDCANCEL);});};d.DoModal();}break;case CMD_SEARCH:showFind();break;case CMD_QUICKFILTER:showSearchBar(!search.IsWindowVisible(),true);break;}refresh();}catch(const std::exception& e){error(this,e);}}
 afx_msg int OnCreate(LPCREATESTRUCT c){if(CFrameWnd::OnCreate(c)==-1)return -1;maintenanceSignal->arm(GetSafeHwnd());completionModalFence.attach(GetSafeHwnd(),maintenanceSignal->notifier());scale=GetDpiForWindow(m_hWnd)/96.0f;font.CreateFontW(-px(11),0,0,0,FW_NORMAL,FALSE,FALSE,FALSE,DEFAULT_CHARSET,OUT_DEFAULT_PRECIS,CLIP_DEFAULT_PRECIS,DEFAULT_QUALITY,DEFAULT_PITCH,L"Tahoma");menu.CreateMenu();addMenu(L"&Tasks",{{CMD_ADD,L"&Add new download"},{CMD_ADD_SCHEDULED_DOWNLOAD,L"Schedule a new &download..."},{CMD_BATCH,L"Add &batch download"},{CMD_BATCH_CLIPBOARD,L"Add batch download from &clipboard"},{CMD_GRABBER,L"Run site &grabber"},{0,L""},{CMD_BASKET,L"Show &drop target"},{0,L""},{CMD_EXPORT,L"E&xport"},{CMD_IMPORT,L"&Import"},{0,L""},{CMD_EXIT,L"&Exit"}});auto tasks=menu.GetSubMenu(0);CMenu exports;exports.CreatePopupMenu();exports.AppendMenuW(MF_STRING,CMD_EXPORT_IDM,L"To IDM export &file");exports.AppendMenuW(MF_STRING,CMD_EXPORT_TEXT,L"To &text file");exports.AppendMenuW(MF_STRING,CMD_EXPORT,L"To UDM &catalog...");exports.AppendMenuW(MF_SEPARATOR);exports.AppendMenuW(MF_STRING,CMD_RECOVERY,L"Backup and &recovery...");tasks->ModifyMenuW(CMD_EXPORT,MF_BYCOMMAND|MF_POPUP,(UINT_PTR)exports.Detach(),L"E&xport");CMenu imports;imports.CreatePopupMenu();imports.AppendMenuW(MF_STRING,CMD_IMPORT_IDM,L"&From IDM export file");imports.AppendMenuW(MF_STRING,CMD_IMPORT_TEXT,L"From &text file");imports.AppendMenuW(MF_STRING,CMD_IMPORT,L"From UDM &catalog...");tasks->ModifyMenuW(CMD_IMPORT,MF_BYCOMMAND|MF_POPUP,(UINT_PTR)imports.Detach(),L"&Import");addMenu(L"&File",{{CMD_STOP,L"&Stop Download"},{CMD_DELETE,L"&Remove"},{CMD_RESUME,L"&Download Now"},{CMD_REDOWNLOAD,L"R&edownload"}});addMenu(L"&Downloads",{{CMD_STOPALL,L"&Pause All"},{CMD_STOP_CLOSE_ALL,L"&Stop All"},{0,L""},{CMD_CLEAN,L"&Delete All Completed"},{0,L""},{CMD_SEARCH,L"Find (Ctrl-F)"},{CMD_FINDNEXT,L"Find Next (F3)"},{0,L""},{CMD_SCHEDULER,L"S&cheduler"},{CMD_STARTQUEUE,L"Start &queue"},{CMD_STOPQUEUE,L"S&top queue"},{0,L""},{CMD_LIMIT,L"Speed &Limiter"},{0,L""},{CMD_OPTIONS,L"&Options"}});CMenu startQueueMenu;startQueueMenu.CreatePopupMenu();startQueueMenu.AppendMenuW(MF_STRING,CMD_STARTQUEUE,L"Start");startQueueMenu.AppendMenuW(MF_STRING,CMD_STARTQUEUEMENU,L"Choose queue...");menu.GetSubMenu(2)->ModifyMenuW(CMD_STARTQUEUE,MF_BYCOMMAND|MF_POPUP,(UINT_PTR)startQueueMenu.Detach(),L"Start &queue");CMenu stopQueueMenu;stopQueueMenu.CreatePopupMenu();stopQueueMenu.AppendMenuW(MF_STRING,CMD_STOPQUEUE,L"Stop");stopQueueMenu.AppendMenuW(MF_STRING,CMD_STOPQUEUEMENU,L"Choose queue...");menu.GetSubMenu(2)->ModifyMenuW(CMD_STOPQUEUE,MF_BYCOMMAND|MF_POPUP,(UINT_PTR)stopQueueMenu.Detach(),L"S&top queue");addMenu(L"&View",{{CMD_HIDE_CATEGORIES,L"Hide &categories"},{CMD_TOOLBAR,L"&Toolbar"},{CMD_TRAY_COLOR,L"&UDM tray icon"},{CMD_COLUMNS,L"C&ustomize URL List..."},{CMD_DARK,L"Dark Mode support"},{CMD_FONT,L"Font"},{0,L""},{CMD_LANGUAGE_ENGLISH,L"&Language"}});CMenu trayChoices;trayChoices.CreatePopupMenu();trayChoices.AppendMenuW(MF_STRING,CMD_TRAY_COLOR,L"3&D style");trayChoices.AppendMenuW(MF_STRING,CMD_TRAY_SYSTEM,L"C&lassic style");trayChoices.AppendMenuW(MF_STRING,CMD_TRAY_HIDE,L"Do&n't show");menu.GetSubMenu(3)->ModifyMenuW(CMD_TRAY_COLOR,MF_BYCOMMAND|MF_POPUP,(UINT_PTR)trayChoices.Detach(),L"&UDM tray icon");CMenu languages;languages.CreatePopupMenu();languages.AppendMenuW(MF_STRING|MF_CHECKED,CMD_LANGUAGE_ENGLISH,L"&English");menu.GetSubMenu(3)->ModifyMenuW(CMD_LANGUAGE_ENGLISH,MF_BYCOMMAND|MF_POPUP,(UINT_PTR)languages.Detach(),L"&Language");addMenu(L"&Help",{{CMD_HELP_CONTENTS,L"H&elp contents"},{CMD_HELP_TUTORIALS,L"&Tutorials"},{CMD_HELP_SCHEDULER,L"&Scheduler and queues"},{CMD_HELP_GRABBER,L"&Grabber Help"},{CMD_TIP_DAY,L"Tip of the &Day..."},{0,L""},{CMD_HOME,L"UDM &Home Page"},{CMD_SUPPORT,L"&Contact UDM Support"},{0,L""},{CMD_CHECK_UPDATES,L"Check for &updates..."},{0,L""},{CMD_ABOUT,L"&About UDM"},{CMD_SHARE,L"Tell a &friend"}});CMenu limiter;limiter.CreatePopupMenu();limiter.AppendMenuW(MF_STRING,CMD_LIMIT_ON,L"Turn o&n");limiter.AppendMenuW(MF_STRING,CMD_LIMIT_OFF,L"Turn of&f");limiter.AppendMenuW(MF_SEPARATOR);limiter.AppendMenuW(MF_STRING,CMD_LIMIT,L"S&ettings...");
menu.GetSubMenu(2)->ModifyMenuW(CMD_LIMIT,MF_BYCOMMAND|MF_POPUP,(UINT_PTR)limiter.Detach(),L"Speed &Limiter");
CMenu fontMenu;fontMenu.CreatePopupMenu();fontMenu.AppendMenuW(MF_STRING,CMD_FONT,L"Choose Font...");fontMenu.AppendMenuW(MF_STRING,CMD_FONT_RESET,L"Reset to default Font");menu.GetSubMenu(3)->ModifyMenuW(CMD_FONT,MF_BYCOMMAND|MF_POPUP,(UINT_PTR)fontMenu.Detach(),L"Font");
CMenu arrange;arrange.CreatePopupMenu();
const wchar_t* arrangeLabels[]={L"By &Order Of Addition",L"By &File Name",L"By &Size",L"By St&atus",L"By &Time Left",L"By Transfer &Rate",L"By &Last Try Date",L"By &Description",L"By Save &Path",L"By R&eferer",L"By Parent &Web Page"};
for(UINT i=0;i<11;++i)arrange.AppendMenuW(MF_STRING,CMD_SORT_ADDED+i,arrangeLabels[i]);
menu.GetSubMenu(3)->InsertMenuW(1,MF_BYPOSITION|MF_POPUP,(UINT_PTR)arrange.Detach(),L"&Arrange files");
// Bind controls once through saved appearance after all children exist.
SetMenu(&menu);rebuildListImages();createToolbar();if(!categoryFrame.Create(L"",WS_CHILD|WS_VISIBLE|WS_CLIPSIBLINGS|SS_GRAYFRAME,CRect(0,0,1,1),this))return -1;categoryHeading.Create(L"Categories",WS_CHILD|WS_VISIBLE|SS_LEFT,CRect(0,0,1,1),this);categoryClose.Create(L"Close categories",WS_CHILD|WS_VISIBLE|WS_TABSTOP|BS_OWNERDRAW,CRect(0,0,1,1),this,CMD_HIDE_CATEGORIES);tree.Create(WS_CHILD|WS_VISIBLE|WS_TABSTOP|TVS_HASBUTTONS|TVS_HASLINES|TVS_LINESATROOT|TVS_SHOWSELALWAYS,CRect(0,0,1,1),this,501);tree.SetImageList(&images,TVSIL_NORMAL);tree.SetItemHeight((SHORT)px(17));table.Create(WS_CHILD|WS_VISIBLE|WS_TABSTOP|WS_BORDER|LVS_REPORT|LVS_SHOWSELALWAYS,CRect(0,0,1,1),this,502);if(!SetWindowSubclass(table.GetHeaderCtrl()->GetSafeHwnd(),headerContextProc,1,(DWORD_PTR)m_hWnd))throw std::runtime_error("Cannot initialize column header menu.");table.SetImageList(&images,LVSIL_SMALL);table.SetExtendedStyle(LVS_EX_FULLROWSELECT|LVS_EX_GRIDLINES|LVS_EX_DOUBLEBUFFER|LVS_EX_HEADERDRAGDROP|LVS_EX_SUBITEMIMAGES|LVS_EX_INFOTIP);for(int i=0;i<columnCount;++i)table.InsertColumn(i,columnName(i),LVCFMT_LEFT,i<visibleDefaultColumns?scaledColumnWidth(i,defaultColumnWidths[i],windowDpi()):0);restoreLayout();status.Create(WS_CHILD,CRect(0,0,1,1),this,503);search.Create(WS_CHILD|WS_TABSTOP|ES_AUTOHSCROLL|WS_BORDER,CRect(0,0,1,1),this,504);searchLabel.Create(L"Find:",WS_CHILD|SS_RIGHT,CRect(0,0,1,1),this);searchFind.Create(L"Find",WS_CHILD|WS_TABSTOP|BS_PUSHBUTTON,CRect(0,0,1,1),this,SearchSubmit);searchSettings.Create(L"Settings...",WS_CHILD|WS_TABSTOP|BS_PUSHBUTTON,CRect(0,0,1,1),this,SearchSettings);searchClose.Create(L"Close search",WS_CHILD|WS_TABSTOP|BS_OWNERDRAW,CRect(0,0,1,1),this,SearchClose);search.SendMessage(EM_SETCUEBANNER,TRUE,(LPARAM)L"Find using the fields and options selected in Find file");clipboard.Create(L"",WS_CHILD|WS_TABSTOP|BS_PUSHBUTTON,CRect(0,0,1,1),this,CMD_PASTE);tray.cbSize=sizeof(tray);tray.hWnd=m_hWnd;tray.uID=1;tray.uFlags=NIF_MESSAGE|NIF_ICON|NIF_TIP;tray.uCallbackMessage=TRAY_MESSAGE;tray.hIcon=AfxGetApp()->LoadIcon(1);wcscpy_s(tray.szTip,L"UDM Download Manager");Shell_NotifyIconW(NIM_ADD,&tray);drop.Register(this);tableDrop.attach(&table);treeDrop.attach(&tree);
 tableDrop.apply=[this](const std::string& id,CPoint point){JobPtr job;{Lock lock(manager.mutex);for(auto j:manager.jobs)if(j->id()==id)job=j;}int row=table.HitTest(point);JobPtr before=row>=0&&row<(int)visible.size()?visible[row]:JobPtr();auto queue=before?str(before->data,"Queue"):selectedQueue();manager.reorder(job,queue,before);sortColumn=-1;refresh();};
 treeDrop.apply=[this](const std::string& id,CPoint point){auto item=tree.HitTest(point);if(!item)return;auto index=tree.GetItemData(item);if(index>=filters.size()||filters[index].rfind("queue:",0)!=0)throw std::runtime_error("Drop an unfinished download onto a queue.");JobPtr job;{Lock lock(manager.mutex);for(auto j:manager.jobs)if(j->id()==id)job=j;}manager.reorder(job,filters[index].substr(6));refresh();};
 SetTimer(1,500,nullptr);manager.queueSound=[this](bool started){playDownloadSound(manager.state["Settings"],started?"QueueStartSoundEnabled":"QueueStopSoundEnabled",started?"QueueStartSoundFile":"QueueStopSoundFile");};manager.showCompletedDownload=[this](JobPtr job){complete(job,true);};
 const auto enqueueTransferEvent=[this,window=GetSafeHwnd()](JobPtr job,TransferEvent kind){
  std::lock_guard<std::mutex> lock(eventsMutex);const bool first=events.empty();events.emplace_back(std::move(job),kind);
  if(first)::PostMessageW(window,TRANSFER_EVENTS,0,0);
 };
 manager.event=[enqueueTransferEvent](JobPtr job,bool complete){
  enqueueTransferEvent(std::move(job),complete?TransferEvent::Finished:TransferEvent::Started);
 };
 manager.filePublished=[enqueueTransferEvent](JobPtr job){
  enqueueTransferEvent(std::move(job),TransferEvent::Published);
 };
 buildTree();applyAppearance();showBasket();return 0;}
 afx_msg void OnLButtonUp(UINT flags,CPoint point){if(draggedJob){auto job=draggedJob;draggedJob.reset();ReleaseCapture();ClientToScreen(&point);CRect area;tree.GetWindowRect(&area);try{if(area.PtInRect(point)){tree.ScreenToClient(&point);treeDrop.apply(job->id(),point);}else{table.GetWindowRect(&area);if(area.PtInRect(point)){table.ScreenToClient(&point);tableDrop.apply(job->id(),point);}}}catch(const std::exception& e){error(this,e);}return;}CFrameWnd::OnLButtonUp(flags,point);}
 afx_msg void OnCaptureChanged(CWnd* window){draggedJob.reset();CFrameWnd::OnCaptureChanged(window);}
 afx_msg void OnMouseMove(UINT flags,CPoint point){if(draggedJob)::SetCursor(LoadCursor(nullptr,IDC_SIZEALL));CFrameWnd::OnMouseMove(flags,point);}
 afx_msg LRESULT OnMainDpiChanged(WPARAM dpi,LPARAM position){
  const int next=HIWORD(dpi),previous=windowDpi();if(next<=0||!position||!table.GetSafeHwnd())return 0;
  auto suggested=*reinterpret_cast<RECT*>(position);SetRedraw(FALSE);
  try{
   if(dpiColumnWidths.size()!=columnCount){dpiColumnWidths.assign(columnCount,0);dpiRenderedWidths.assign(columnCount,-1);}
   for(int i=0;i<columnCount;++i){const int current=table.GetColumnWidth(i);if(current!=dpiRenderedWidths[i])dpiColumnWidths[i]=isReferenceColumnWidth(i,current,previous)?(double)referenceColumnWidths[i]*96/referenceColumnDpi:(double)current*96/previous;}
   scale=next/96.0f;rebuildListImages();applyRenderedAppearance();
   for(int i=0;i<columnCount;++i){dpiRenderedWidths[i]=(int)std::lround(dpiColumnWidths[i]*next/96);table.SetColumnWidth(i,dpiRenderedWidths[i]);}
   SetWindowPos(nullptr,suggested.left,suggested.top,suggested.right-suggested.left,suggested.bottom-suggested.top,SWP_NOZORDER|SWP_NOACTIVATE);
  }catch(const std::exception& e){showStatus(e.what());}
  SetRedraw(TRUE);RedrawWindow(nullptr,nullptr,RDW_INVALIDATE|RDW_ALLCHILDREN|RDW_ERASE);return 0;
 }
 afx_msg void OnSize(UINT type,int w,int h){
  CFrameWnd::OnSize(type,w,h);layout(w,h);
  if(type==SIZE_MINIMIZED)historyRefreshDeferred=true;
  else requestDeferredHistoryRefresh();
 }
 afx_msg void OnShowWindow(BOOL show,UINT reason){
  CFrameWnd::OnShowWindow(show,reason);
  if(show)requestDeferredHistoryRefresh();
  else historyRefreshDeferred=true;
 }
 afx_msg void OnEnable(BOOL enabled){
  CFrameWnd::OnEnable(enabled);
  // Wake-ups contain no request data. Process the transfer inbox before CLI
  // Exit, including when a modal consumed the original disabled-owner wake.
  if(enabled&&!quitting&&GetSafeHwnd())PostMessage(TRANSFER_EVENTS);
 }
  void pollNetworkCapture(bool request=true){if(!networkCapture)return;networkCapture->pollReady();const auto& view=networkCapture->view();if(view.revision!=networkErrorRevision){networkErrorRevision=view.revision;if(!view.error.empty())showStatus(view.error);}if(request&&!quitting)networkCapture->requestPoll();}
  afx_msg void OnTimer(UINT_PTR id){if(pollShellFolders())return;restoreExitLayoutOwnerIfReady();if(automaticPresentationRetryPending){automaticPresentationRetryPending=false;automaticPresentationDeferred=true;}try{resumeAutomaticPresentation();}catch(const std::exception& e){showStatus(e.what());}if(pollCompletionSettings())return;if(pollAppearanceCommand())return;if(pollGlobalLimitCommand())return;if(exitLayoutTask.valid()){pollExitLayout();return;}try{pollDownloadMenu();}catch(const std::exception& e){cancelDownloadMenu();showStatus(e.what());}if(closePolicyTask.valid()){pollClosePolicy();return;}try{if(pollCompletionWork())return;}catch(const std::exception& e){showStatus(e.what());return;}if(automationReviewDeferred&&!quitting)try{reviewAutomationBatches();}catch(const std::exception& e){showStatus(e.what());}try{pollResumeCommands();}catch(const std::exception& e){showStatus(e.what());}if(!cancellationRecoveryShowing)showCancellationRecovery();showPresentationRecovery();if(manager.fileInfoDecisionPending){CFrameWnd::OnTimer(id);return;}if(optionsAppearanceSnapshot){CFrameWnd::OnTimer(id);return;}if(optionsUiActive){tickOptionsModel();CFrameWnd::OnTimer(id);return;}if(optionsRefreshPending){try{finishOptionsAppearance();}catch(const std::exception& e){showStatus(e.what());}CFrameWnd::OnTimer(id);return;}if(deferredUpdateResult&&PostMessage(UPDATE_RESULT,0,reinterpret_cast<LPARAM>(deferredUpdateResult.get())))deferredUpdateResult.release();if(IsWindowEnabled())try{while(takeCliHangup(manager,false))performSystemAction("Disconnect dial-up / VPN");}catch(const std::exception& e){error(this,e);return;}if(postCliCompletionExit())return;try{pollNetworkCapture();}catch(const std::exception& e){showStatus(e.what());}try{showQuotaWarning();}catch(const std::exception& e){showStatus(e.what());}if(automaticPresentationAllowed()&&manager.browserSettingsRequested.exchange(false))PostMessage(WM_COMMAND,CMD_BROWSER);completions.erase(std::remove_if(completions.begin(),completions.end(),[](const auto& d){return !d->GetSafeHwnd();}),completions.end());rememberLayout();refresh(false,true);if(!refreshing)try{fileCompletionActions();queueCompletionActions();}catch(const std::exception& e){error(this,e);}if(!quitting)try{requestModelMaintenance();}catch(const std::exception& e){showStatus(e.what());}CFrameWnd::OnTimer(id);}
  void consumeModelMaintenance(){const auto result=modelMaintenance.takeResult();if(!result.dispatchError.empty())showStatus(result.dispatchError);if(!result.tickError.empty())showStatus(result.tickError);}
  void requestModelMaintenance(){consumeModelMaintenance();modelMaintenance.requestTick();}
  afx_msg LRESULT OnModelMaintenanceReady(WPARAM identity,LPARAM){
   if(!maintenanceSignal->matches(identity))return 0;
   if(pollShellFolders())return 0;
   restoreExitLayoutOwnerIfReady();
   // Forward only a wake; Form rejects stale/recycled identities. The timer
   // remains the fallback if a notification cannot be delivered.
   if(completionCountdownWake)try{completionCountdownWake();}catch(...){}
   if(pollCompletionSettings())return 0;if(pollAppearanceCommand())return 0;if(pollGlobalLimitCommand())return 0;
   if(exitLayoutTask.valid()){pollExitLayout();return 0;}if(quitting)return 0;
   if(closePolicyTask.valid()){pollClosePolicy();return 0;}try{if(resumeAutomaticPresentation()){consumeModelMaintenance();return 0;}}catch(const std::exception& e){showStatus(e.what());}try{if(pollCompletionWork())return 0;}catch(const std::exception& e){showStatus(e.what());return 0;}
   if(postCliCompletionExit())return 0;
   try{pollDownloadMenu();}catch(const std::exception& e){cancelDownloadMenu();showStatus(e.what());}
   showPresentationRecovery();
   try{pollNetworkCapture(false);}catch(const std::exception& e){showStatus(e.what());}
   try{pollResumeCommands();}catch(const std::exception& e){showStatus(e.what());}
   if(!refreshing&&!optionsUiActive&&!optionsRefreshPending&&!optionsAppearanceSnapshot&&!manager.fileInfoDecisionPending)refresh();
   consumeModelMaintenance();
   return 0;
  }
  void reviewBrowserRequests(){
   if(!automaticPresentationAllowed())return;
   if(manager.browserQualityHelpRequested.load()){
    try{
     CaptureDialogOwner owner(this,true);
     CompletionModalFence::ModalScope modal(owner.get(this)->GetSafeHwnd());
     if(!::MessageBoxW(owner.get(this)->GetSafeHwnd(),
      L"Choose a different quality in the web player and let it play. Open the UDM video menu again to see the formats captured for this player.",
      L"UDM",MB_OK|MB_TOPMOST|MB_SETFOREGROUND))
      showStatus("Cannot open the Other qualities message box.");
    }catch(const std::exception& e){showStatus(e.what());}
    manager.browserQualityHelpRequested.store(false);
    if(!automaticPresentationAllowed())return;
   }
   if(!browserBatchPresentation){auto rows=tryBrowserBatchSnapshot(manager);if(!rows||rows->empty())return;browserBatchPresentation=std::move(*rows);}
   try{
#ifdef UDM_TOOLBAR_COMPONENT_TEST
    if(automaticBatchPresenter)automaticBatchPresenter(*browserBatchPresentation);else
#endif
    {std::vector<std::string> urls;urls.reserve(browserBatchPresentation->size());for(const auto& row:*browserBatchPresentation)urls.push_back(row.address);CaptureDialogOwner owner(this,true);selectBatch(owner.get(this),manager,urls,{},{},&*browserBatchPresentation);}
    browserBatchPresentation.reset();
   }catch(const std::exception& e){showStatus(e.what());}
  }

  BrowserPresentationAcknowledger presentationAcknowledger(){return [this](const Json& offer){if(!presentationAcks.enqueue(offer))throw std::runtime_error("Browser acknowledgement queue is full or stopping. The accepted download remains in history.");};}
  void showPresentationRecovery(){
   if(quitting||presentationRecoveryShowing||cancellationRecoveryShowing||refreshing||optionsUiActive||optionsRefreshPending||optionsAppearanceSnapshot||!automaticPresentationAllowed()||manager.fileInfoDecisionPending||fileInfoDialogShowing())return;
   try{const auto failure=presentationAcks.failure();if(!failure)return;presentationRecoveryShowing=true;struct Reset{bool& value;~Reset(){value=false;}}reset{presentationRecoveryShowing};presentBrowserAcknowledgementFailure(this,presentationAcks,failure);}catch(const std::exception& e){showStatus(e.what());}
  }
  void showCancellationRecovery(){if(quitting||presentationRecoveryShowing||cancellationRecoveryShowing||refreshing||optionsUiActive||optionsRefreshPending||optionsAppearanceSnapshot||!automaticPresentationAllowed()||manager.fileInfoDecisionPending)return;try{auto failure=preparedFileInfo->cancellationFailure();if(!failure)return;cancellationRecoveryShowing=true;struct Reset{bool& value;~Reset(){value=false;}}reset{cancellationRecoveryShowing};presentCancellationFailure(this,manager,*preparedFileInfo,std::move(failure));}catch(const std::exception& e){showStatus(e.what());}}
  afx_msg LRESULT OnCancellationFailed(WPARAM,LPARAM){showCancellationRecovery();return 0;}
  afx_msg LRESULT OnPreparedFileInfo(WPARAM key,LPARAM){
   static_assert(sizeof(WPARAM)>=sizeof(PendingFileInfoInbox::Key));
   auto operation=preparedFileInfo->take(static_cast<PendingFileInfoInbox::Key>(key));
   if(operation&&!quitting)try{preparedFileInfoViews.insert_or_assign(operation->view->jobId,operation);
#ifdef UDM_PRODUCTION_UI_TIMING_TEST
    nativeUiTraceRecord("prepared-view-claimed");
#endif
    if(!presentReadyPreparedFileInfo()&&operation->view->media&&!operation->view->formRequest&&!operation->cancelRequested())automaticPresentationDeferred=true;
   }catch(const std::exception& e){showStatus(e.what());}
   return 0;
  }
  afx_msg LRESULT OnPresentCapture(WPARAM,LPARAM){
#ifdef UDM_PRODUCTION_UI_TIMING_TEST
   nativeUiTraceRecord("capture-ui-dispatch");
#endif
#ifdef UDM_CAPTURE_PRESENTATION_COMPONENT_TEST
   traceCapturePresentation("ui-dispatch");
#endif
   if(!quitting){
    if(refreshing||!automaticPresentationAllowed())captureRefreshRequested=true;
    else refresh(true,true);
   }
   return 0;
  }
  // The final worker event arrives after its checkpoint and active ownership
  // retire. Use it immediately; the main timer remains a retry after modal UI,
  // a failed PostMessage or a later explicit save/scanner decision.
  bool postCliCompletionExit(){
   if(quitting||!pendingCompletions.empty()||completionDraining||completionReadiness.valid()||completionClaimTask.valid()||!pendingCompletionClaims.empty()||shellFolderTask.valid()||!shellFolders.empty()||!shellFolderErrors.empty()||!quitAfterDownload||!IsWindowEnabled()||completionModalFence.blocked()||manager.fileInfoDecisionPending||optionsUiActive||optionsRefreshPending||optionsAppearanceSnapshot)return false;
   std::unique_lock lock(manager.mutex,std::try_to_lock);if(!lock.owns_lock()){if(!completionReadiness.valid())completionReadiness.start(manager,maintenanceSignal->notifier());return false;}const auto job=quitAfterDownload;
   if(std::find(manager.jobs.begin(),manager.jobs.end(),job)==manager.jobs.end()){quitAfterDownload.reset();return false;}
   if(!cliCompletionReady(manager,job)||!PostMessage(WM_COMMAND,CMD_EXIT))return false;
   quitAfterDownload.reset();return true;
  }
  afx_msg LRESULT OnTransferEvents(WPARAM,LPARAM){
   if(!quitting&&!refreshing&&IsWindowEnabled())try{resumeAutomaticPresentation();presentTransferEvents();postCliCompletionExit();}catch(const std::exception& e){showStatus(e.what());}
   return 0;
  }
  afx_msg LRESULT OnDispatchQueueWork(WPARAM,LPARAM){
   if(optionsUiActive||optionsRefreshPending||optionsAppearanceSnapshot){optionsQueueDispatchPending=true;if(optionsUiActive)try{dispatchOptionsQueue();}catch(const std::exception& e){showStatus(e.what());}return 0;}
   if(manager.fileInfoDecisionPending)return 0; // UI acknowledgement posts an immediate main timer tick.
   MSG pending{};
   while(PeekMessageW(&pending,m_hWnd,DISPATCH_QUEUE_WORK,DISPATCH_QUEUE_WORK,PM_REMOVE)){}
   if(!quitting)try{if(modelMaintenance.requestDispatch())optionsQueueDispatchPending=false;}catch(const std::exception& e){showStatus(e.what());}
   return 0;
  }

#ifdef UDM_PRODUCTION_UI_TIMING_TEST
  afx_msg LRESULT OnInitialRefresh(WPARAM,LPARAM){nativeUiTraceRecord("initial-refresh-entry");refresh();reviewAutomationBatches();nativeUiTraceRecord("initial-refresh-returned");return 0;}
#else
  afx_msg LRESULT OnInitialRefresh(WPARAM,LPARAM){refresh();reviewAutomationBatches();return 0;}
#endif
  afx_msg LRESULT OnRefreshVisibleHistory(WPARAM,LPARAM){
   historyRefreshPosted=false;
   if(!quitting&&!refreshing&&historyRefreshDeferred&&IsWindowVisible()&&!IsIconic())refresh();
   return 0;
  }
  afx_msg LRESULT OnUpdateResult(WPARAM,LPARAM l){
   std::unique_ptr<UpdateOutcome> outcome((UpdateOutcome*)l);if(!automaticPresentationAllowed()||manager.fileInfoDecisionPending||optionsUiActive||optionsRefreshPending||optionsAppearanceSnapshot){deferredUpdateResult=std::move(outcome);return 0;}updateChecking=false;if(updateThread.joinable())updateThread.join();
   try{
    if(!outcome->error.empty()){if(updateInteractive)themedMessage(this,cs("Could not check for updates.\r\n"+outcome->error),L"Check for Update",MB_OK|MB_ICONWARNING);return 0;}
    // Only a check that reached the manifest counts: an offline start is tried again at the next start.
    {auto settings=preferences();settings["UpdateCheckedAt"]=date();manager.setSettings(settings);}
    const auto& manifest=outcome->manifest;
    if(!manifest.newer){if(updateInteractive)themedMessage(this,cs(std::string("UDM ")+currentProductVersion+" is up to date."),L"Check for Update",MB_OK|MB_ICONINFORMATION);return 0;}
    if(!updateInteractive&&updateSkipped(preferences(),manifest.version))return 0;
    if(!IsWindowVisible())ShowWindow(SW_SHOW);  // a tray or -background start: the prompt needs a visible owner
    showUpdateAvailableDialog(this,manifest,preferences(),[this](const Json& next){manager.setSettings(next);},[this,manifest]{downloadUpdate(manifest);});
   }catch(const std::exception& e){error(this,e);}
   return 0;
  }
  void downloadUpdate(const UpdateManifest& manifest){
   // UDM's own transfer with the manifest's SHA-256 as the expected checksum: the file is published only when it matches.
   // The job carries the version in its data, so a download finished after a restart, or the same update offered
   // again, still goes through the gate instead of becoming a second copy.
   JobPtr existing;{Lock lock(manager.mutex);for(const auto& j:manager.jobs)if(str(j->data,"UpdateSetupVersion")==manifest.version&&lower(str(j->data,"ExpectedSha256"))==lower(manifest.setupSha256)){existing=j;break;}}
   if(existing){
    bool complete,active;{Lock lock(manager.mutex);complete=str(existing->data,"Status")=="Complete";active=manager.isActive(existing);}
    if(complete){finishUpdateDownload(existing);return;}
    if(!active)manager.resume(existing);
   }else{
    auto job=manager.add(manifest.setupUrl,"",updateSetupFileName(manifest),"Main queue",false,{},manifest.setupSha256);
    {Lock lock(manager.mutex);job->data["UpdateSetupVersion"]=manifest.version;}manager.save();
   }
   showStatus("Downloading UDM "+manifest.version+"...");refresh();
  }
  struct UpdateCompletionInput {std::string version,sha;fs::path path;};
  UpdateCompletionInput updateCompletionInput(const JobPtr& job){UpdateCompletionInput input{str(job->data,"UpdateSetupVersion"),str(job->data,"ExpectedSha256"),{}};if(!input.version.empty()&&!input.sha.empty())input.path=job->target();return input;}
  bool tryFinishTransferNotification(const JobPtr& job){
   UpdateCompletionInput input;{std::unique_lock lock(manager.mutex,std::try_to_lock);if(!lock.owns_lock())return false;input=updateCompletionInput(job);}
   if(!beginUpdateCompletion(std::move(input)))complete(job);return true;
  }
  // True when the job is an update setup UDM downloaded: its gate replaces the ordinary completion handling.
  bool finishUpdateDownload(JobPtr job){UpdateCompletionInput input;{Lock lock(manager.mutex);input=updateCompletionInput(job);}return beginUpdateCompletion(std::move(input));}
  bool beginUpdateCompletion(UpdateCompletionInput input){
   if(input.version.empty()||input.sha.empty())return false;
   if(updateLaunching)return true;
   updateLaunching=true;showStatus("Checking the downloaded UDM "+input.version+" setup...");
   if(updateLaunchThread.joinable())updateLaunchThread.join();
   // Copying, hashing and WinVerifyTrust on a setup take seconds: off the UI thread.
   updateLaunchThread=std::thread([path=std::move(input.path),sha=std::move(input.sha),hwnd=m_hWnd]{auto outcome=std::make_unique<UpdateLaunch>();try{outcome->verified=verifyDownloadedSetup(path,sha);}catch(const std::exception& e){outcome->error=e.what();}::PostMessage(hwnd,UPDATE_LAUNCHED,0,(LPARAM)outcome.release());});
   return true;
  }
  afx_msg LRESULT OnUpdateLaunched(WPARAM,LPARAM l){
   std::unique_ptr<UpdateLaunch> outcome((UpdateLaunch*)l);updateLaunching=false;if(updateLaunchThread.joinable())updateLaunchThread.join();showStatus("");
   if(!outcome->error.empty()){themedMessage(this,cs(outcome->error),L"UDM update",MB_OK|MB_ICONWARNING);return 0;}
   try{startDownloadedSetup(outcome->verified,m_hWnd);PostMessage(WM_COMMAND,CMD_EXIT);}
   catch(const std::exception& e){themedMessage(this,cs(e.what()),L"UDM update",MB_OK|MB_ICONWARNING);}
   return 0;
  }
 afx_msg void OnGetMinMaxInfo(MINMAXINFO* info){info->ptMinTrackSize.x=px(596);info->ptMinTrackSize.y=px(340);CFrameWnd::OnGetMinMaxInfo(info);}
 afx_msg void OnClose(){restoreExitLayoutOwnerIfReady();if(completionSettingsTask.valid()){completionSettingsClosePending=true;showStatus("Saving completion preference before closing...");return;}completionSettingsClosePending=false;cancelDownloadMenu();if(appearanceCommandTask.valid()){appearanceClosePending=true;showStatus("Finishing appearance changes before closing...");return;}appearanceClosePending=false;if(globalLimitCommandTask.valid()){globalLimitClosePending=true;showStatus("Saving Speed Limiter settings before closing...");return;}globalLimitClosePending=false;if(exitLayoutTask.valid()){pollExitLayout();return;}if(closePolicyTask.valid()){if(!quitting)return;cancelClosePolicy();}if(closeBlockedByDialog()||(!IsWindowEnabled()&&!exitLayoutPrepared)){if(exitLayoutPrepared)releaseExitLayoutOwner();exitLayoutPrepared=false;quitting=false;recoveryOnClose=false;wakeCompletionWork();return;}UDM_FINISH_POINT("close-entry");if(!quitting){std::unique_lock lock(manager.mutex,std::try_to_lock);if(!lock.owns_lock()){try{closePolicyTask.start(manager,maintenanceSignal->notifier());showStatus("Waiting for download settings...");}catch(const std::exception& error){showStatus(error.what());}return;}if(yes(manager.state["Settings"],"CloseToTray")){ShowWindow(SW_HIDE);wakeCompletionWork();return;}}if(shellFolderTask.valid()||!shellFolders.empty()||!shellFolderErrors.empty()){shellFolderClosePending=true;shellFolderExplicitExit=quitting;quitting=false;showStatus("Waiting for Windows folder handoffs before closing...");pollShellFolders();return;}shellFolderClosePending=false;quitting=true;if(!exitLayoutPrepared)try{if(!prepareExitLayout())return;}catch(const std::exception& e){exitLayoutTask.finish();EnableWindow(exitLayoutOwnerEnabled);quitting=false;recoveryOnClose=false;showStatus(e.what());return;}exitLayoutPrepared=false;completionReadiness.finish();downloadMenuTask.finish();preparedFileInfo->close();preparedFileInfoViews.clear();if(updateCancel)updateCancel->stop=true;if(updateThread.joinable())updateThread.join();if(updateLaunchThread.joinable())updateLaunchThread.join();KillTimer(1);maintenanceSignal->disarm();modelMaintenance.stop();presentationAcks.stop();UDM_FINISH_POINT("integration-stop-begin");if(stopIntegration)stopIntegration();UDM_FINISH_POINT("integration-stop-end");preparedFileInfo->stopWorker();UDM_FINISH_POINT("file-info-stop-end");showStatus("Saving downloads and closing...");EnableWindow(FALSE);try{
#ifdef UDM_TOOLBAR_COMPONENT_TEST
 if(completionShutdownObserver)completionShutdownObserver();
#endif
 resumeTask.finish();resumePending.clear();resumeActive.clear();resumeFollowups.clear();resumeDecision.reset();requestedAuthenticationTask->finish();requestedAuthenticationRequests.clear();authenticationTask->finish();if(networkCapture)networkCapture->stop();for(auto& window:progress)window->finishPendingCommandForShutdown();for(auto& window:progress)window->flushQueuedLimitsForShutdown();if(completionClaimTask.valid()){auto claimedJob=std::move(completionClaimJob);auto claimed=completionClaimTask.finish();if(!claimed.saved){if(claimedJob)pendingCompletionClaims.push_front(std::move(claimedJob));throw std::runtime_error(claimed.error.empty()?"The completion action could not be saved before closing.":claimed.error);}}pendingCompletionClaims.clear();UDM_FINISH_POINT("manager-stop-begin");manager.stop();UDM_FINISH_POINT("manager-stop-end");}catch(const std::exception& e){EnableWindow(TRUE);quitting=false;recoveryOnClose=false;modelMaintenance.restart();presentationAcks.restart();maintenanceSignal->arm(GetSafeHwnd());SetTimer(1,500,nullptr);try{if(restartIntegration)restartIntegration();}catch(const std::exception& bridge){error(this,bridge);}error(this,e);return;}pendingCompletions.clear();UDM_FINISH_POINT("icons-stop-begin");fileIconLoader.stop();UDM_FINISH_POINT("icons-stop-end");commandFiles.stop();UDM_FINISH_POINT("command-files-stop-end");if(recoveryOnClose){try{launchRecoveryMode(manager.root,true);}catch(const std::exception& e){error(this,e);}}manager.event={};manager.filePublished={};manager.queueSound={};manager.showCompletedDownload={};if(quotaWarning&&quotaWarning->GetSafeHwnd())quotaWarning->DestroyWindow();quotaWarning.reset();for(auto& d:completions)if(d->GetSafeHwnd())d->DestroyWindow();completions.clear();basket.reset();tableDrop.Revoke();treeDrop.Revoke();drop.Revoke();for(auto& p:progress)if(p->GetSafeHwnd()){p->DestroyWindow();}Shell_NotifyIconW(NIM_DELETE,&tray);UDM_FINISH_POINT("windows-cleanup-end");completionModalFence.detach();CFrameWnd::OnClose();UDM_FINISH_POINT("close-return");}
 afx_msg LRESULT OnShow(WPARAM,LPARAM){if(exitLayoutTask.valid()||(appearanceClosePending&&quitting)||(globalLimitClosePending&&quitting))return 0;appearanceClosePending=false;globalLimitClosePending=false;cancelClosePolicy();ShowWindow(SW_RESTORE);SetForegroundWindow();requestDeferredHistoryRefresh();reviewAutomationBatches();return 0;}
 // Link arrays sent through automation wait, encrypted, until reviewed here.
 void reviewAutomationBatches(){
  automationReviewDeferred=true;
  if(!automaticPresentationAllowed())return;
  for(;;){if(!automaticPresentationAllowed())return;Json offer;
   {std::unique_lock lock(manager.mutex,std::try_to_lock);if(!lock.owns_lock())return;offer=nextAutomationBatch(manager);}
   automationReviewDeferred=false;if(offer.empty())break;
   ShowWindow(SW_SHOW);automationBatchDialog(this,manager,offer);
   automationReviewDeferred=true;
  }
 }
 afx_msg LRESULT OnDropUrl(WPARAM,LPARAM l){std::unique_ptr<std::string> s((std::string*)l);try{if(s->find('\n')!=std::string::npos)selectExpandedBatch(this,manager,*s);else addAddress(this,manager,*s);}catch(const std::exception& e){error(this,e);}return 0;}
 LRESULT WindowProc(UINT message,WPARAM w,LPARAM l)override{
  if(message==WM_UNINITMENUPOPUP)menuTheme.releasePopup(reinterpret_cast<HMENU>(w));
  if(message==WM_MEASUREITEM&&menuTheme.measure(reinterpret_cast<MEASUREITEMSTRUCT*>(l)))return TRUE;
  if(message==WM_DRAWITEM&&menuTheme.draw(reinterpret_cast<DRAWITEMSTRUCT*>(l)))return TRUE;
  if(message==WM_MENUCHAR){LRESULT result=0;if(menuTheme.mnemonic(reinterpret_cast<HMENU>(l),(wchar_t)LOWORD(w),result))return result;}
  static const UINT recreated=RegisterWindowMessageW(L"TaskbarCreated");
  if(recreated&&message==recreated&&!quitting){
   try{if(!updateTray())ShowWindow(SW_RESTORE);}catch(...){ShowWindow(SW_RESTORE);}
  }
  return CFrameWnd::WindowProc(message,w,l);
 }
 afx_msg LRESULT OnTray(WPARAM,LPARAM l){if(l==WM_LBUTTONDBLCLK||l==NIN_KEYSELECT||l==NIN_SELECT)OnShow(0,0);else if(l==WM_RBUTTONUP){if(closePolicyTask.valid())return 0;CMenu popup;popup.CreatePopupMenu();popup.AppendMenuW(MF_STRING,CMD_ADD,L"Add URL");popup.AppendMenuW(MF_STRING,CMD_RESUME,L"Resume selected");popup.AppendMenuW(MF_STRING,CMD_STOPALL,L"Pause all downloads");popup.AppendMenuW(MF_STRING,CMD_STOP_CLOSE_ALL,L"Stop all downloads");CMenu startQueues,stopQueues;startQueues.CreatePopupMenu();stopQueues.CreatePopupMenu();const auto queueStates=manager.queueCommandStates();fillQueueCommands(startQueues,true,queueStates);fillQueueCommands(stopQueues,false,queueStates);popup.AppendMenuW(MF_POPUP,(UINT_PTR)startQueues.Detach(),L"Start queue");popup.AppendMenuW(MF_POPUP,(UINT_PTR)stopQueues.Detach(),L"Stop queue");popup.AppendMenuW(MF_STRING,CMD_SCHEDULER,L"Scheduler");popup.AppendMenuW(MF_STRING,CMD_LIMIT,L"Speed limiter...");popup.AppendMenuW(MF_STRING,CMD_OPTIONS,L"Options");popup.AppendMenuW(MF_STRING|(yes(preferences(),"DropBasket")?MF_CHECKED:0),CMD_BASKET,L"Drop basket");popup.AppendMenuW(MF_SEPARATOR);popup.AppendMenuW(MF_STRING,CMD_EXIT,L"Exit UDM");CPoint p;GetCursorPos(&p);SetForegroundWindow();popup.TrackPopupMenu(TPM_RIGHTBUTTON,p.x,p.y,this);}return 0;}
 // MFC queries update routing before showing menus; OnCommand alone does not
 // advertise handlers and otherwise leaves every custom menu command disabled.
 afx_msg void OnUpdateAppCommand(CCmdUI* ui){
  // A popup is routed using its first child's ID. An inactive first queue
  // must not disable access to the other queues in that submenu.
  if(ui->m_pSubMenu&&ui->m_nID>=32000&&ui->m_nID<34000){ui->Enable(TRUE);return;}
  if(ui->m_pSubMenu&&ui->m_nID==CMD_PROGRESS){ui->Enable(TRUE);return;}
  if(queuePopupUpdating&&ui->m_pMenu&&!ui->m_pSubMenu&&ui->m_nID>=32000&&ui->m_nID<34000){const auto state=ui->m_pMenu->GetMenuState(ui->m_nIndex,MF_BYPOSITION);ui->Enable(state!=UINT(-1)&&(state&(MF_DISABLED|MF_GRAYED))==0);return;}
  const auto enabled=advisoryCommandEnabled(ui->m_nID);if(!enabled)return;ui->Enable(*enabled);if(auto column=arrangeColumn(ui->m_nID);column>=0)ui->SetRadio(sortColumn==column);
  if(ui->m_nID==CMD_LANGUAGE_ENGLISH){ui->SetRadio(TRUE);return;}
  switch(ui->m_nID){case CMD_TRAY_COLOR:case CMD_TRAY_SYSTEM:case CMD_TRAY_HIDE:case CMD_LIMIT_ON:case CMD_LIMIT_OFF:case CMD_BASKET:case CMD_DARK:case CMD_HIDE_CATEGORIES:break;default:return;}
  Json prefs;const auto captureFlags=[&](const Json& settings){for(const auto* key:{"TrayIcon","LimitKbps","DropBasket","DarkMode","HideCategories"})if(auto found=settings.find(key);found!=settings.end())prefs[key]=*found;};
  if(optionsAppearanceSnapshot)captureFlags(*optionsAppearanceSnapshot);else{std::unique_lock lock(manager.mutex,std::try_to_lock);if(!lock.owns_lock())return;captureFlags(manager.state["Settings"]);}
  switch(ui->m_nID){
   case CMD_TRAY_COLOR:ui->SetRadio(str(prefs,"TrayIcon","Color")=="Color");break;
   case CMD_TRAY_SYSTEM:ui->SetRadio(classicTrayStyle(str(prefs,"TrayIcon")));break;
   case CMD_TRAY_HIDE:ui->SetRadio(str(prefs,"TrayIcon")=="Hidden");break;
   case CMD_LIMIT_ON:ui->SetRadio(num(prefs,"LimitKbps")>0);break;
   case CMD_LIMIT_OFF:ui->SetRadio(num(prefs,"LimitKbps")==0);break;
   case CMD_BASKET:ui->SetCheck(yes(prefs,"DropBasket"));break;
   case CMD_DARK:ui->SetCheck(yes(prefs,"DarkMode"));break;
   case CMD_HIDE_CATEGORIES:ui->SetCheck(yes(prefs,"HideCategories"));break;
  }
 }
 BOOL OnCommand(WPARAM w,LPARAM l)override{if(HIWORD(w)==BN_CLICKED)cancelClosePolicy();UINT id=LOWORD(w);if(id==CMD_STOPALL&&(HWND)l==toolbar.GetSafeHwnd())id=CMD_STOP_CLOSE_ALL;if((id>=CMD_ADD&&id<=CMD_SCHEDULE_EXISTING_DOWNLOAD)||(id>=31000&&id<31064)||isQueueMenuCommand(id)){command(id);return TRUE;}if(HIWORD(w)==BN_CLICKED&&(id==SearchSubmit||id==SearchSettings||id==SearchClose)){try{if(id==SearchSubmit)submitFindFilter();else if(id==SearchSettings)command(CMD_SEARCH);else{showSearchBar(false);refresh();}}catch(const std::exception& e){error(this,e);}return TRUE;}if(id==504&&HIWORD(w)==EN_CHANGE)return TRUE;return CFrameWnd::OnCommand(w,l);}
 void activateDownloadRow(int row){
  if(row<0||row>=(int)visible.size())return;
  cancelDownloadMenu();
  table.SetItemState(-1,0,LVIS_SELECTED);
  table.SetItemState(row,LVIS_SELECTED|LVIS_FOCUSED,LVIS_SELECTED|LVIS_FOCUSED);
  const auto chosen=selected();
  if(chosen.empty())return;
  bool complete;
  {Lock lock(manager.mutex);complete=str(chosen[0]->data,"Status")=="Complete";}
  if(!complete)command(CMD_PROGRESS);
  else command(str(preferences(),"CompletedDoubleClick","Properties")=="Open"?CMD_OPEN:CMD_PROPERTIES);
 }
 BOOL OnNotify(WPARAM w,LPARAM l,LRESULT* result)override{auto header=(NMHDR*)l;if(toolbarNotification(header,result))return TRUE;if(header->hwndFrom==tree.m_hWnd&&header->code==TVN_SELCHANGED){auto change=(NMTREEVIEW*)l;if(!change->itemNew.hItem){*result=0;return TRUE;}auto index=tree.GetItemData(change->itemNew.hItem);if(!treeRebuilding&&index<filters.size()){filter=filters[index];refresh();}*result=0;return TRUE;}if(header->hwndFrom==table.m_hWnd){if(header->code==LVN_ITEMCHANGED&&!refreshing){const auto change=reinterpret_cast<NMLISTVIEW*>(header);if((change->uChanged&LVIF_STATE)&&((change->uOldState^change->uNewState)&LVIS_SELECTED))scheduleCommandFileRefresh();}if(header->code==LVN_ENDSCROLL){if(!refreshing)updateFileIcons();*result=0;return TRUE;}if(header->code==LVN_GETINFOTIPW){auto tip=reinterpret_cast<NMLVGETINFOTIPW*>(header);if(tip->pszText&&tip->cchTextMax>0)tip->pszText[0]=L'\0';if(tip->iItem>=0&&tip->iItem<(int)visible.size()&&tip->pszText&&tip->cchTextMax>0){auto value=wide(cachedListTooltip(tip->iItem));wcsncpy_s(tip->pszText,tip->cchTextMax,value.c_str(),_TRUNCATE);}*result=0;return TRUE;}if(header->code==LVN_BEGINDRAG){auto chosen=selected();if(chosen.size()==1&&str(chosen[0]->data,"Status")!="Complete"&&!manager.isActive(chosen[0])){draggedJob=chosen[0];SetCapture();::SetCursor(LoadCursor(nullptr,IDC_SIZEALL));*result=0;return TRUE;}try{dragSavedFiles(completedDragPaths(chosen));}catch(const std::exception& e){error(this,e);}*result=0;return TRUE;}if(header->code==NM_DBLCLK){activateDownloadRow(reinterpret_cast<NMITEMACTIVATE*>(header)->iItem);*result=0;return TRUE;}if(header->code==NM_RETURN){const int row=table.GetNextItem(-1,LVNI_FOCUSED);if(row>=0&&(table.GetItemState(row,LVIS_SELECTED)&LVIS_SELECTED))activateDownloadRow(row);*result=0;return TRUE;}if(header->code==LVN_COLUMNCLICK){auto change=(NMLISTVIEW*)l;if(sortColumn==change->iSubItem)ascending=!ascending;else{sortColumn=change->iSubItem;ascending=true;}refresh();*result=0;return TRUE;}}return CFrameWnd::OnNotify(w,l,result);}
 static LRESULT CALLBACK headerContextProc(HWND window,UINT message,WPARAM wParam,LPARAM lParam,UINT_PTR id,DWORD_PTR owner){
  if(message==WM_CONTEXTMENU){::SendMessageW((HWND)owner,WM_CONTEXTMENU,(WPARAM)window,lParam);return 0;}
  if(message==WM_NCDESTROY)RemoveWindowSubclass(window,headerContextProc,id);
  return DefSubclassProc(window,message,wParam,lParam);
 }
 afx_msg void OnContextMenu(CWnd* origin,CPoint point){try{
  if(!origin)return;
  auto header=table.GetHeaderCtrl();CRect headerArea;if(header)header->GetWindowRect(&headerArea);
  if(header&&(origin->GetSafeHwnd()==header->GetSafeHwnd()||(origin->GetSafeHwnd()==table.GetSafeHwnd()&&headerArea.PtInRect(point)))){
   if(point.x==-1&&point.y==-1)point=CPoint(headerArea.left+px(12),headerArea.bottom);
   CMenu popup;popup.CreatePopupMenu();popup.AppendMenuW(MF_STRING,CMD_COLUMNS,L"&Columns...");
   auto choice=popup.TrackPopupMenu(TPM_RETURNCMD|TPM_RIGHTBUTTON,point.x,point.y,this);if(choice)command(choice);return;
  }
  if(origin->GetSafeHwnd()==tree.GetSafeHwnd()){treeMenu(point);return;}if(origin->GetSafeHwnd()==toolbar.GetSafeHwnd()){toolbarPopup(point);return;}if(origin->GetSafeHwnd()!=table.GetSafeHwnd()){CFrameWnd::OnContextMenu(origin,point);return;}
  if(point.x==-1&&point.y==-1){
   int row=table.GetNextItem(-1,LVNI_FOCUSED);if(row<0||!(table.GetItemState(row,LVIS_SELECTED)&LVIS_SELECTED))row=table.GetNextItem(-1,LVNI_SELECTED);
   if(row<0)return;table.EnsureVisible(row,FALSE);CRect rect;if(!table.GetItemRect(row,&rect,LVIR_BOUNDS))return;
   CRect area;table.GetClientRect(&area);point=CPoint(std::clamp(rect.left+px(30),area.left,area.right),std::min(rect.bottom,area.bottom));table.ClientToScreen(&point);
  }else{
   CPoint local=point;table.ScreenToClient(&local);
   LVHITTESTINFO hit{};hit.pt=local;
   const int row=table.SubItemHitTest(&hit);if(row<0)return;
   if(!(table.GetItemState(row,LVIS_SELECTED)&LVIS_SELECTED))table.SetItemState(-1,0,LVIS_SELECTED);
   table.SetItemState(row,LVIS_SELECTED|LVIS_FOCUSED,LVIS_SELECTED|LVIS_FOCUSED);
  }
  downloadMenu(point);
 }catch(const std::exception& e){error(this,e);} }
 BOOL PreTranslateMessage(MSG* msg)override{if(msg->message==WM_KEYDOWN){
  if(msg->wParam==VK_ESCAPE&&downloadMenuRequest){
   const bool ownsInput=IsWindowEnabled()&&(msg->hwnd==GetSafeHwnd()||::IsChild(GetSafeHwnd(),msg->hwnd));
   cancelDownloadMenu();return ownsInput?TRUE:FALSE;
  }
  if(msg->wParam==VK_RETURN){if(msg->hwnd==search.GetSafeHwnd()||msg->hwnd==searchFind.GetSafeHwnd()){searchFind.SendMessage(BM_CLICK);return TRUE;}if(msg->hwnd==searchSettings.GetSafeHwnd()){searchSettings.SendMessage(BM_CLICK);return TRUE;}if(msg->hwnd==searchClose.GetSafeHwnd()){searchClose.SendMessage(BM_CLICK);return TRUE;}}
  if(msg->wParam==VK_TAB&&GetKeyState(VK_CONTROL)>=0&&GetKeyState(VK_MENU)>=0&&::IsChild(m_hWnd,msg->hwnd)&&::IsDialogMessageW(m_hWnd,msg))return TRUE;
if(msg->wParam==VK_F1){command(CMD_HELP_CONTENTS);return TRUE;}if(msg->wParam==VK_ESCAPE&&draggedJob){draggedJob.reset();ReleaseCapture();return TRUE;}if(GetKeyState(VK_CONTROL)<0){if(msg->wParam=='N'){command(CMD_ADD);return TRUE;}if(msg->wParam=='V'&&GetFocus()!=&search){command(CMD_PASTE);return TRUE;}if(msg->wParam=='A'&&GetFocus()==&table){command(CMD_SELECTALL);return TRUE;}if(msg->wParam=='F'){command(GetKeyState(VK_SHIFT)<0?CMD_QUICKFILTER:CMD_SEARCH);return TRUE;}if(msg->wParam=='M'){command(CMD_RELOCATE);return TRUE;}}if(msg->wParam==VK_F3){command(CMD_FINDNEXT);return TRUE;}if(msg->wParam==VK_DELETE&&GetFocus()==&table){command(CMD_DELETE);return TRUE;}}return CFrameWnd::PreTranslateMessage(msg);}
public:
 std::shared_ptr<PendingFileInfoInbox> fileInfoInbox(){if(preparedFileInfo->closed()){preparedFileInfo=std::make_shared<PendingFileInfoInbox>();preparedFileInfoViews.clear();}preparedFileInfo->startWorker([source=&manager](const PendingFileInfoInbox::Operation& operation){cancelPendingFileInfoOffer(*source,operation);},[window=GetSafeHwnd()]{::PostMessageW(window,CANCELLATION_FAILED,0,0);});return preparedFileInfo;}
 // Fetches and verifies the signed manifest off the window thread. Without a built-in update key the
 // interactive command keeps the release-page dialog and the startup check does nothing.
 void startUpdateCheck(bool interactive){
  if(updateChecking){if(interactive)updateInteractive=true;return;}  // the running check answers the menu too
  if(!updateKeyConfigured()){if(interactive)showUpdateDialog(this,preferences());return;}
  updateChecking=true;updateInteractive=interactive;auto settings=preferences();updateCancel=std::make_shared<Cancel>();updateCancel->deadline=GetTickCount64()+20000;
  if(updateThread.joinable())updateThread.join();
  updateThread=std::thread([this,settings,cancel=updateCancel]{auto outcome=std::make_unique<UpdateOutcome>();try{outcome->manifest=fetchUpdateManifest(settings,*cancel);}catch(const Cancelled&){outcome->error="The update check was canceled or timed out.";}catch(const std::exception& e){outcome->error=e.what();}PostMessage(UPDATE_RESULT,0,(LPARAM)outcome.release());});
 }
 JobPtr quitAfterDownload;std::function<void()> stopIntegration,restartIntegration; ~MainWindow()override{completionModalFence.detach();maintenanceSignal->disarm();appearanceCommandTask.finish();globalLimitCommandTask.finish();exitLayoutTask.finish();downloadMenuTask.finish();closePolicyTask.finish();modelMaintenance.stop();presentationAcks.stop();try{authenticationTask->finish();}catch(...) {}}
 explicit MainWindow(Manager& m):manager(m),modelMaintenance([owner=&m]{tickDesktopModel(*owner);},[owner=&m]{dispatchDesktopModel(*owner);},maintenanceSignal->notifier()){findQuery=normalizedFindQuery(preferences().value("FindQuery",Json::object()));auto cls=AfxRegisterWndClass(CS_DBLCLKS,LoadCursor(nullptr,IDC_ARROW),(HBRUSH)(COLOR_BTNFACE+1),AfxGetApp()->LoadIcon(1));constexpr int initialWidth=759,initialHeight=449;if(!Create(cls,mainWindowTitle,WS_OVERLAPPEDWINDOW,CRect(160,150,160+initialWidth,150+initialHeight)))throw std::runtime_error("Cannot create UDM window.");SetWindowPos(nullptr,0,0,MulDiv(initialWidth,windowDpi(),96),MulDiv(initialHeight,windowDpi(),96),SWP_NOMOVE|SWP_NOZORDER);CenterWindow();}
};
BEGIN_MESSAGE_MAP(MainWindow,CFrameWnd)
 ON_WM_ENABLE()
 ON_WM_SYSCOLORCHANGE()
 ON_WM_SETTINGCHANGE()
 ON_UPDATE_COMMAND_UI_RANGE(CMD_ADD,CMD_TOOLBAR_DISCOVER,OnUpdateAppCommand)
 ON_UPDATE_COMMAND_UI(CMD_EDIT_DOWNLOAD_SCHEDULE,OnUpdateAppCommand)
 ON_UPDATE_COMMAND_UI(CMD_ADD_SCHEDULED_DOWNLOAD,OnUpdateAppCommand)
 ON_UPDATE_COMMAND_UI(CMD_SCHEDULE_EXISTING_DOWNLOAD,OnUpdateAppCommand)
 ON_UPDATE_COMMAND_UI_RANGE(32000,33999,OnUpdateAppCommand)
 ON_UPDATE_COMMAND_UI_RANGE(31000,31063,OnUpdateToolbarSkin)
 ON_WM_INITMENUPOPUP()
 ON_WM_ERASEBKGND()
 ON_WM_CTLCOLOR()
 ON_WM_CONTEXTMENU()
 ON_WM_LBUTTONUP()
 ON_WM_MOUSEMOVE()
 ON_WM_CAPTURECHANGED()
 ON_WM_CREATE()
 ON_WM_SIZE()
 ON_MESSAGE(WM_DPICHANGED,OnMainDpiChanged)
 ON_WM_TIMER()
 ON_WM_CLOSE()
 ON_WM_GETMINMAXINFO()
 ON_MESSAGE(SHOW_APP,OnShow)
 ON_MESSAGE(INITIAL_REFRESH,OnInitialRefresh)
 ON_MESSAGE(UPDATE_RESULT,OnUpdateResult)
ON_MESSAGE(UPDATE_LAUNCHED,OnUpdateLaunched)
 ON_MESSAGE(COMMAND_FILE_REFRESH,OnCommandFileRefresh)
 ON_MESSAGE(PRESENT_CAPTURE,OnPresentCapture)
 ON_MESSAGE(PREPARED_FILE_INFO,OnPreparedFileInfo)
 ON_MESSAGE(CANCELLATION_FAILED,OnCancellationFailed)
 ON_MESSAGE(DISPATCH_QUEUE_WORK,OnDispatchQueueWork)
 ON_MESSAGE(UiTaskSignal::message,OnModelMaintenanceReady)
 ON_MESSAGE(TRANSFER_EVENTS,OnTransferEvents)
 ON_MESSAGE(REFRESH_VISIBLE_HISTORY,OnRefreshVisibleHistory)
 ON_WM_SHOWWINDOW()
 ON_MESSAGE(TRAY_MESSAGE,OnTray)
 ON_MESSAGE(DROP_URL,OnDropUrl)
END_MESSAGE_MAP()
#if defined(UDM_TOOLBAR_COMPONENT_TEST) || defined(UDM_CAPTURE_PRESENTATION_COMPONENT_TEST)
// InitInstance-based fixtures have no regular message loop. Keep their model
// owner alive until the same asynchronous Exit used by the app has completed.
inline void closeOwnedMainWindow(MainWindow*& frame){
 if(!frame)return;const auto window=frame->GetSafeHwnd();frame->SendMessage(WM_COMMAND,CMD_EXIT);
 const auto deadline=GetTickCount64()+10000;
 while(::IsWindow(window)&&GetTickCount64()<deadline){MSG message{};while(PeekMessageW(&message,nullptr,0,0,PM_REMOVE)){if(message.message==WM_QUIT)continue;TranslateMessage(&message);DispatchMessageW(&message);}Sleep(1);}
 if(::IsWindow(window)){frame->DestroyWindow();frame=nullptr;throw std::runtime_error("Owned main Exit did not complete.");}frame=nullptr;
}
#endif
#if !defined(UDM_TOOLBAR_COMPONENT_TEST) && !defined(UDM_CAPTURE_PRESENTATION_COMPONENT_TEST)
class Application:public CWinApp {
 std::unique_ptr<Manager> manager;std::unique_ptr<PipeServer> pipe;std::unique_ptr<AutomationHost> automation;Handle mutex;std::unique_ptr<RecoveryDataLease> recoveryLease;bool recoveryClosed=false,silentLaunchFailure=false;int startupExitCode=0;
public:BOOL InitInstance()override{
#ifdef UDM_PRODUCTION_UI_TIMING_TEST
 if(!nativeUiTraceInitialize())return FALSE;
 nativeUiTraceRecord("app-init-entry");
#endif
 CWinApp::InitInstance();INITCOMMONCONTROLSEX controls{sizeof(controls),ICC_WIN95_CLASSES|ICC_DATE_CLASSES|ICC_PROGRESS_CLASS};InitCommonControlsEx(&controls);AfxOleInit();SetRegistryKey(L"UDM");
 try{
  int argc=0;auto argv=CommandLineToArgvW(GetCommandLineW(),&argc);if(!argv)throw std::runtime_error("Cannot read launch options.");
  std::vector<std::wstring> args;for(int i=1;i<argc;++i)args.emplace_back(argv[i]);LocalFree(argv);silentLaunchFailure=launchSilentFailureRequested(args);auto options=parseLaunch(args);
  if(!options.tag.empty())SetEnvironmentVariableW(L"UDM_INSTANCE_TAG",wide(options.tag).c_str());
  if(options.waitProcess){
   if(options.waitProcess==GetCurrentProcessId())throw std::runtime_error("Cannot wait for the current process.");
   Handle previous(OpenProcess(SYNCHRONIZE,FALSE,options.waitProcess));
   if(previous&&WaitForSingleObject(previous.h,30000)!=WAIT_OBJECT_0)throw std::runtime_error("UDM is still closing. Finish closing it, then open recovery again.");
   if(!previous&&GetLastError()!=ERROR_INVALID_PARAMETER)throw std::runtime_error("Cannot confirm that the previous UDM process closed.");
  }
  if(options.recovery){
   RecoveryAvailability availability;
   recoveryLease=std::make_unique<RecoveryDataLease>(options.data);
   if(fs::exists(recoveryLease->marker())){Json result;RecoveryWorkDialog pending(nullptr,"Recovering interrupted restore",[this](const Cancel&){return recoverPendingRestore(*recoveryLease);},false);if(!pending.run(result))return FALSE;}
   RecoveryCenterDialog dialog(*recoveryLease);m_pMainWnd=&dialog;auto result=dialog.DoModal();m_pMainWnd=nullptr;if(result==-1)throw std::runtime_error("Cannot open UDM recovery.");recoveryClosed=true;return FALSE;
  }

  auto instance=L"Local\\"+wide(pipeName());mutex.h=CreateMutexW(nullptr,TRUE,instance.c_str());if(!mutex)throw std::runtime_error("Cannot create UDM instance lock.");
  if(GetLastError()==ERROR_ALREADY_EXISTS){
   const bool launchOnly=options.address.empty()&&!options.startQueue;
   if(launchOnly&&options.background)return FALSE;
   auto reply=send(launchOnly?Json{{"action","show"}}:options.request(),12000);
   if(!yes(reply,"ok"))throw std::runtime_error(str(reply,"error","UDM did not accept this command."));
   return FALSE;
  }
  recoveryLease=std::make_unique<RecoveryDataLease>(options.data);{RecoveryAvailability availability;recoverPendingRestore(*recoveryLease);}
  manager=std::make_unique<Manager>(options.data,Manager::StartupRecovery::Desktop);
#ifdef UDM_PRODUCTION_UI_TIMING_TEST
  nativeUiTraceRecord("manager-ready");
#endif
  applyStartupGlobalLimiter(*manager);
  auto frame=new MainWindow(*manager);
#ifdef UDM_PRODUCTION_UI_TIMING_TEST
  nativeUiTraceRecord("frame-created");
#endif
  m_pMainWnd=frame;
  frame->stopIntegration=[this]{pipe.reset();};
  const auto requestQueueWork=[frame]{frame->PostMessage(DISPATCH_QUEUE_WORK);};
  frame->restartIntegration=[this,frame,requestQueueWork]{
   if(!pipe)pipe=std::make_unique<PipeServer>(*manager,[frame]{frame->PostMessage(SHOW_APP);},
#ifdef UDM_PRODUCTION_UI_TIMING_TEST
    [frame]{nativeUiTraceRecord("capture-posted");frame->PostMessage(PRESENT_CAPTURE);},requestQueueWork,frame->fileInfoInbox(),[window=frame->GetSafeHwnd()](uint64_t key){return ::PostMessageW(window,PREPARED_FILE_INFO,static_cast<WPARAM>(key),0)!=FALSE;});
#else
    [frame]{frame->PostMessage(PRESENT_CAPTURE);},requestQueueWork,frame->fileInfoInbox(),[window=frame->GetSafeHwnd()](uint64_t key){return ::PostMessageW(window,PREPARED_FILE_INFO,static_cast<WPARAM>(key),0)!=FALSE;});
#endif
  };
  manager->startQueuesOnStartup();
  frame->restartIntegration();
#ifdef UDM_PRODUCTION_UI_TIMING_TEST
  nativeUiTraceRecord("pipe-ready");
#endif
  // Host the automation class object here: COM connects callers to the running
  // desktop directly, so Udm.Automation.exe is only launched when UDM is closed.
#ifdef UDM_PRODUCTION_UI_TIMING_TEST
  nativeUiTraceRecord("automation-start");
#endif
  try{automation=std::make_unique<AutomationHost>([this,frame](const Json& message){auto action=str(message,"action");if(action=="automation-add"){manager->receive(message);if(!(num(message.at("link"),"flags")&1))frame->PostMessage(SHOW_APP);}else if(action=="automation-batch"){queueAutomationBatch(*manager,message);frame->PostMessage(SHOW_APP);}else throw std::invalid_argument("Unknown automation command.");});}catch(const std::exception&){automation.reset();}
#ifdef UDM_PRODUCTION_UI_TIMING_TEST
  nativeUiTraceRecord("automation-finished");
#endif
  if(!options.address.empty()){
   auto job=manager->receive(options.request());
   Lock lock(manager->mutex);
   const auto status=str(job->data,"Status");
   if(options.quitAfterDownload&&status!="Complete")frame->quitAfterDownload=job;
   if(options.silent&&!options.paused&&status=="Queued")requestQueueWork();
  }
  if(options.startQueue){manager->queueRun("Main queue",true);requestQueueWork();}
#ifdef UDM_PRODUCTION_UI_TIMING_TEST
  frame->ShowWindow(options.background?SW_HIDE:SW_SHOW);frame->UpdateWindow();frame->PostMessage(INITIAL_REFRESH);if(!options.silent&&!options.startQueue&&!options.waitProcess&&!options.recovery&&updateCheckDue(manager->state["Settings"],epoch()))frame->startUpdateCheck(false);if(!options.background&&!options.silent&&!options.startQueue&&!options.waitProcess&&options.address.empty()&&yes(manager->state["Settings"],"ShowTipsOnStartup",true))frame->PostMessage(WM_COMMAND,CMD_TIP_DAY);nativeUiTraceRecord("app-init-returning");return TRUE;
#else
  frame->ShowWindow(options.background?SW_HIDE:SW_SHOW);frame->UpdateWindow();frame->PostMessage(INITIAL_REFRESH);if(!options.silent&&!options.startQueue&&!options.waitProcess&&!options.recovery&&updateCheckDue(manager->state["Settings"],epoch()))frame->startUpdateCheck(false);if(!options.background&&!options.silent&&!options.startQueue&&!options.waitProcess&&options.address.empty()&&yes(manager->state["Settings"],"ShowTipsOnStartup",true))frame->PostMessage(WM_COMMAND,CMD_TIP_DAY);return TRUE;
#endif
 }catch(const std::exception& e){startupExitCode=1;if(silentLaunchFailure)writeLaunchFailure(e.what());else AfxMessageBox(cs(e.what()),MB_OK|MB_ICONERROR);return FALSE;}
}
 int ExitInstance()override{UDM_FINISH_POINT("exit-instance-begin");automation.reset();UDM_FINISH_POINT("automation-reset-end");pipe.reset();manager.reset();UDM_FINISH_POINT("manager-reset-end");recoveryLease.reset();AfxOleTerm(FALSE);UDM_FINISH_POINT("ole-term-end");auto result=CWinApp::ExitInstance();UDM_FINISH_POINT("exit-instance-end");return startupExitCode?startupExitCode:(recoveryClosed?0:result);}
};
Application application;
#endif
}
