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
#include "DownloadListSnapshot.hpp"
#include "DownloadListOrder.hpp"
#include "DownloadListStatusOrder.hpp"
#include "DownloadListEtaOrder.hpp"
#include "DownloadListSizeOrder.hpp"
#include "NavigationTreeSnapshot.hpp"
#include "CommandAvailability.hpp"
#include "AsyncFileStatus.hpp"
#include "IdmTextSort.hpp"
#include "AsyncFileIcons.hpp"
#include "QueueMembership.hpp"
#include "QueueCompletion.hpp"
#include "CategoryEditor.hpp"
#include "Launch.hpp"
#include "CliCompletion.hpp"
#include "RecoverySession.hpp"
#include "RecoveryUi.hpp"
namespace udm {
inline i64 displayedFileSize(const Json& data){
  if(yes(data,"ConfirmationPending")&&str(data,"Status")=="Merging"&&str(data,"MediaPhase")=="Waiting for confirmation"){
   const auto verified=num(data,"PrefetchedFinalSize",-1);if(verified>=0)return verified;
  }
  return num(data,"Size",-1);
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
#include "QueueIndicators.hpp"
#include "TrayIcon.hpp"
#include <shellapi.h>
#include <fstream>
namespace udm {
enum : UINT { CMD_ADD=200,CMD_RESUME,CMD_STOP,CMD_STOPALL,CMD_DELETE,CMD_CLEAN,CMD_OPTIONS,CMD_SCHEDULER,CMD_STARTQUEUE,CMD_STOPQUEUE,CMD_GRABBER,CMD_BATCH,CMD_IMPORT,CMD_EXPORT,CMD_EXIT,CMD_PROPERTIES,CMD_OPEN,CMD_FOLDER,CMD_NETWORK,CMD_BROWSER,CMD_ABOUT,CMD_NEWQUEUE,CMD_NEWCATEGORY,CMD_UP,CMD_DOWN,CMD_PASTE,CMD_PROGRESS,CMD_MOVEQUEUE,CMD_SEARCH,CMD_REFRESH_ADDRESS,CMD_OPENWITH,CMD_RELOCATE,CMD_REDOWNLOAD,CMD_DOUBLE_OPEN,CMD_DOUBLE_PROPERTIES,CMD_COLUMNS,CMD_DARK,CMD_FONT,CMD_TRAY_COLOR,CMD_TRAY_SYSTEM,CMD_TRAY_HIDE,CMD_FINDNEXT,CMD_HIDE_CATEGORIES,CMD_REMOVEQUEUE,CMD_TOOLBAR,CMD_EDITCATEGORY,CMD_DELETECATEGORY,CMD_DELETEQUEUE,CMD_QUICKFILTER,CMD_STARTQUEUEMENU,CMD_STOPQUEUEMENU,CMD_SELECTALL,CMD_LIMIT,CMD_BASKET,CMD_RECYCLE,CMD_ZIP,CMD_CLIPBOARD_CAPTURE,CMD_TOOLBAR_SMALL,CMD_TOOLBAR_MEDIUM,CMD_TOOLBAR_LARGE,CMD_TOOLBAR_ICONSTEXT,CMD_TOOLBAR_ICONSONLY,CMD_TOOLBAR_TEXTONLY,CMD_TOOLBAR_HIDE,CMD_TOOLBAR_BUILTIN,CMD_TOOLBAR_LOAD,CMD_TOOLBAR_FOLDER,CMD_TOOLBAR_RELOAD,CMD_TOOLBAR_NEON,CMD_TOOLBAR_3D,CMD_IMPORT_IDM,CMD_IMPORT_TEXT,CMD_EXPORT_IDM,CMD_EXPORT_TEXT,CMD_BATCH_CLIPBOARD,CMD_RECOVERY,CMD_SORT_ADDED,CMD_SORT_NAME,CMD_SORT_SIZE,CMD_SORT_STATUS,CMD_SORT_TIME,CMD_SORT_RATE,CMD_SORT_LAST,CMD_SORT_DESCRIPTION,CMD_SORT_PATH,CMD_SORT_REFERER,CMD_SORT_PAGE,CMD_LIMIT_ON,CMD_LIMIT_OFF,CMD_FONT_RESET,CMD_STOP_CLOSE_ALL,CMD_LANGUAGE_ENGLISH,CMD_HELP_CONTENTS,CMD_HELP_TUTORIALS,CMD_HELP_SCHEDULER,CMD_HELP_GRABBER,CMD_TIP_DAY,CMD_CHECK_UPDATES,CMD_HOME,CMD_SUPPORT,CMD_SHARE,CMD_TOOLBAR_DISCOVER };
constexpr UINT SHOW_APP=WM_APP+20,TRAY_MESSAGE=WM_APP+21,DROP_URL=WM_APP+22,INITIAL_REFRESH=WM_APP+23,PRESENT_CAPTURE=WM_APP+24,UPDATE_RESULT=WM_APP+25,UPDATE_LAUNCHED=WM_APP+26,COMMAND_FILE_REFRESH=WM_APP+27;
class DropTarget:public COleDropTarget {
 CWnd* owner;
public:explicit DropTarget(CWnd* w):owner(w){}
 DROPEFFECT OnDragEnter(CWnd*,COleDataObject* data,DWORD,CPoint)override{return data->IsDataAvailable(CF_UNICODETEXT)?DROPEFFECT_COPY:DROPEFFECT_NONE;}
 DROPEFFECT OnDragOver(CWnd*,COleDataObject* data,DWORD,CPoint)override{return data->IsDataAvailable(CF_UNICODETEXT)?DROPEFFECT_COPY:DROPEFFECT_NONE;}
 BOOL OnDrop(CWnd*,COleDataObject* data,DROPEFFECT,CPoint)override{HGLOBAL h=data->GetGlobalData(CF_UNICODETEXT);if(!h)return FALSE;auto p=(const wchar_t*)GlobalLock(h);if(!p)return FALSE;size_t max=GlobalSize(h)/sizeof(wchar_t),n=wcsnlen_s(p,max);std::string value=n<max&&n<32768?utf8(std::wstring(p,n)):"";GlobalUnlock(h);GlobalFree(h);if(value.empty())return FALSE;owner->PostMessage(DROP_URL,0,(LPARAM)new std::string(value));return TRUE;}
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
 Manager& manager;std::unique_ptr<NetworkCaptureSession> networkCapture;CMenu menu;MenuBarTheme menuTheme;CFont font;CTreeCtrl tree;ThemeList table;ThemeStatusBar status;CStatic categoryHeading;PanelCloseButton categoryClose;CEdit search;CStatic searchLabel;CButton searchFind,searchSettings;PanelCloseButton searchClose;
 static constexpr UINT SearchSubmit=505,SearchSettings=506,SearchClose=507;
 CButton clipboard;CImageList images;std::map<std::string,int> iconIndex;
 // These caches follow visible's row order; refresh owns all list text and icon updates.
 std::vector<std::string> filters;std::vector<JobPtr> visible;std::vector<std::array<std::string,12>> rowText;std::vector<std::array<int,2>> rowImages;std::vector<std::shared_ptr<const DownloadListRow>> rowSnapshots;std::map<JobPtr,std::shared_ptr<const DownloadListRow>> listRecordCache;std::vector<bool> rowQuota;Json rowPreferences,rowView;bool rowAdded=false,rowPath=false,rowReferer=false,rowRecovery=false;std::string rowTree;std::vector<std::unique_ptr<Progress>> progress;std::set<std::string> infoSeen,captureReviewsSeen;
 // Update check: one background fetch at a time; the result arrives as UPDATE_RESULT.
 struct UpdateOutcome{UpdateManifest manifest;std::string error;};
 std::thread updateThread;std::shared_ptr<Cancel> updateCancel;bool updateChecking=false,updateInteractive=false;
 // A downloaded setup is verified off the UI thread; the result arrives as UPDATE_LAUNCHED.
 struct UpdateLaunch{std::string error;fs::path verified;};std::thread updateLaunchThread;bool updateLaunching=false;
 std::mutex eventsMutex;std::vector<std::pair<JobPtr,bool>> events;NOTIFYICONDATAW tray{};DropTarget drop{this};bool recoveryOnClose=false,quitting=false,refreshing=false,clipboardPromptPending=false;float scale=1;std::string filter="all",copiedUrl,lastClipboard;int sortColumn=-1;bool ascending=true;std::string lastTree;bool treeRebuilding=false;HTREEITEM treeSelection=nullptr;std::map<std::string,std::string> observedStates;std::unique_ptr<Basket> basket;std::vector<std::unique_ptr<Form>> completions;bool handlingQueueAction=false;std::vector<std::pair<JobPtr,Json>> fileActions;QueueDropTarget tableDrop,treeDrop;JobPtr draggedJob;
 ClipboardSnapshot clipboardSnapshot;SortKeyCache refererKeyCache;DownloadListOrder listOrderCache;
 NavigationTreeSnapshot treeSnapshot;bool treeSnapshotValid=false;std::string treeFilter;
 AsyncFileStatus commandFiles;bool commandFileRefreshPending=false;
 std::map<HTREEITEM,std::string> treeKeys;std::map<std::string,HTREEITEM> treeItems;
 std::vector<HTREEITEM> treeQueueNodes;std::vector<int> treeQueueImages;
 int px(int n)const{return (int)(n*scale);}
 int windowDpi()const{return (int)std::lround(scale*96);}
 std::vector<double> dpiColumnWidths;std::vector<int> dpiRenderedWidths;
 std::vector<JobPtr> selected(){std::vector<JobPtr> jobs;if(!table.GetSafeHwnd())return jobs;for(int i=-1;(i=table.GetNextItem(i,LVNI_SELECTED))>=0;)if(i<(int)visible.size())jobs.push_back(visible[i]);return jobs;}
 static bool downloadQueueMember(const Json& data){return yes(data,"QueueMember",str(data,"Status")!="Complete");}
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
 std::string queueInfo(const Json& data){Lock lock(manager.mutex);if(!downloadQueueMember(data))return "Not in a queue";for(const auto& q:manager.state["Queues"])if(str(q,"Name")==str(data,"Queue"))return queueIndicatorDescription(q);return "Not in an existing queue";}
 std::string selectedQueue(){return filter.rfind("queue:",0)==0?filter.substr(6):"Main queue";}
 std::string clipboardText(bool* readable=nullptr){if(readable)*readable=false;if(!OpenClipboard())return {};std::string value;bool valid=true;auto h=GetClipboardData(CF_UNICODETEXT);if(h){auto p=(const wchar_t*)GlobalLock(h);if(p){auto max=GlobalSize(h)/sizeof(wchar_t),n=wcsnlen_s(p,max);if(n<max&&n<1024*1024)value=utf8(std::wstring(p,n));GlobalUnlock(h);}else valid=false;}else if(IsClipboardFormatAvailable(CF_UNICODETEXT))valid=false;CloseClipboard();if(readable)*readable=valid;return trim(value);}
 static int arrangeColumn(UINT id){static constexpr int columns[]={8,0,2,3,4,5,6,7,9,10,11};return id>=CMD_SORT_ADDED&&id<=CMD_SORT_PAGE?columns[id-CMD_SORT_ADDED]:-1;}
 bool commandCanResume(JobPtr job){Lock lock(manager.mutex);auto state=str(job->data,"Status");return !manager.isActive(job)&&state!="Complete"&&state!="Queued";}
 bool commandCanStop(JobPtr job){Lock lock(manager.mutex);return manager.isActive(job)||str(job->data,"Status")=="Queued";}
 void closeStoppedProgress(const std::set<std::string>& stopped){
  for(auto& p:progress)if(p->GetSafeHwnd()&&stopped.count(p->downloadId()))p->requestCompletionClose();
  std::lock_guard<std::mutex> lock(eventsMutex);
  events.erase(std::remove_if(events.begin(),events.end(),[&](const auto& event){return !event.second&&stopped.count(event.first->id());}),events.end());
 }
 std::vector<std::string> membershipQueues(const std::vector<JobPtr>& selection){
  Lock lock(manager.mutex);bool completed=std::any_of(selection.begin(),selection.end(),[](const JobPtr& j){return str(j->data,"Status")=="Complete";});
  std::vector<std::string> names;for(const auto& q:manager.state["Queues"])if(!completed||yes(q,"Synchronize"))names.push_back(str(q,"Name"));return names;
 }
 static bool commandUsesSelection(UINT id){
  switch(id){case CMD_RESUME:case CMD_STOP:case CMD_DELETE:case CMD_ZIP:case CMD_RECYCLE:case CMD_PROPERTIES:case CMD_PROGRESS:case CMD_FOLDER:case CMD_REMOVEQUEUE:case CMD_MOVEQUEUE:case CMD_UP:case CMD_DOWN:case CMD_REFRESH_ADDRESS:case CMD_REDOWNLOAD:case CMD_OPENWITH:case CMD_RELOCATE:case CMD_OPEN:return true;default:return false;}
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
 CommandAvailability commandAvailabilityLocked(UINT id,const std::vector<JobPtr>& chosen,const CommandGlobalState& global){
  CommandAvailability value;
  if(id>=32000&&id<34000){const bool start=id<33000;const auto& names=start?startQueueNames:stopQueueNames;const auto index=id-(start?32000:33000);value.enabled=index<names.size()&&(start||manager.canStopQueue(names[index]));return value;}
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
   case CMD_REFRESH_ADDRESS:value.enabled=chosen.size()==1&&manager.canRefreshAddress(chosen[0]);break;
   case CMD_REDOWNLOAD:{value.enabled=chosen.size()==1&&!manager.isActive(chosen[0]);if(value.enabled){const auto state=str(chosen[0]->data,"Status");if(state!="Complete"){value.enabled=state=="Paused"||state=="Failed";if(value.enabled){value.ftpOnly=true;value.url=str(chosen[0]->data,"Url");}}}break;}
   case CMD_OPENWITH:case CMD_RELOCATE:case CMD_OPEN:value.enabled=chosen.size()==1&&!manager.isActive(chosen[0])&&str(chosen[0]->data,"Status")=="Complete";break;
   case CMD_STOP_CLOSE_ALL:value.enabled=global.stoppable||global.unfinishedWindow;break;
   case CMD_STOPALL:value.enabled=global.stoppable;break;
   case CMD_CLEAN:value.enabled=global.cleanable;break;
   default:break;
  }
  return value;
 }
 std::map<UINT,bool> commandStates(const std::vector<UINT>& commands,const std::vector<JobPtr>* suppliedSelection=nullptr,bool* addingToQueue=nullptr,bool freshFiles=false){
  const bool needSelection=addingToQueue||std::any_of(commands.begin(),commands.end(),commandUsesSelection);
  const auto chosen=suppliedSelection?*suppliedSelection:needSelection?selected():std::vector<JobPtr>{};
  CommandGlobalNeeds needs;bool needManager=addingToQueue!=nullptr;std::map<UINT,CommandAvailability> decisions;
  for(const auto id:commands){decisions.emplace(id,CommandAvailability{});needManager=needManager||commandUsesManager(id);needs.recovery=needs.recovery||id==CMD_RECOVERY;needs.stop=needs.stop||id==CMD_STOPALL;needs.close=needs.close||id==CMD_STOP_CLOSE_ALL;needs.clean=needs.clean||id==CMD_CLEAN;}
  std::vector<Progress*> windows;if(needs.close)for(const auto& window:progress)if(window->GetSafeHwnd())windows.push_back(window.get());
  if(needManager){Lock lock(manager.mutex);if(addingToQueue)*addingToQueue=std::none_of(chosen.begin(),chosen.end(),[](const JobPtr& job){return downloadQueueMember(job->data);});const auto global=needs.any()?commandGlobalStateLocked(needs,windows):CommandGlobalState{};
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
 bool commandEnabled(UINT id,bool freshFiles=false){
  if(!commandUsesManager(id))return !commandUsesSelection(id)||selected().size()==1;
  return commandStates({id},nullptr,nullptr,freshFiles).at(id);
 }
 afx_msg LRESULT OnCommandFileRefresh(WPARAM,LPARAM){commandFileRefreshPending=false;if(!quitting)try{commandStates({CMD_ZIP,CMD_RECYCLE});}catch(const std::exception& e){showStatus(e.what());}return 0;}
 void scheduleCommandFileRefresh(){if(!commandFileRefreshPending&&!quitting&&GetSafeHwnd()){commandFileRefreshPending=PostMessage(COMMAND_FILE_REFRESH)!=FALSE;}}
 void updateToolbarCommands(){
  std::vector<UINT> commands;commands.reserve(ToolbarCommandCount+2);for(int i=0;i<ToolbarCommandCount;++i)commands.push_back(i==3?CMD_STOP_CLOSE_ALL:toolbarCommand(i));commands.push_back(CMD_ZIP);commands.push_back(CMD_RECYCLE);
  const auto enabled=commandStates(commands);for(int i=0;i<ToolbarCommandCount;++i)toolbar.EnableButton(toolbarCommand(i),enabled.at(commands[i]));
 }
 #include "QueueMenus.hpp"
 #include "ToolbarUi.hpp"
 #include "MainFeatures.hpp"
 #include "DesktopUi.hpp"
 void showBasket(){auto p=preferences();bool enabled=yes(p,"DropBasket");if(!enabled){basket.reset();return;}if(!basket){basket=std::make_unique<Basket>(this);if(!basket->open((int)num(p,"BasketX",25),(int)num(p,"BasketY",100)))throw std::runtime_error("Cannot create the drop basket.");basket->moved=[this](int x,int y){try{auto prefs=preferences();prefs["BasketX"]=x;prefs["BasketY"]=y;manager.setSettings(prefs);}catch(const std::exception& e){error(this,e);}};}basket->ShowWindow(SW_SHOWNOACTIVATE);}

 bool otherDownloadsRunning(JobPtr job){Lock lock(manager.mutex);for(auto other:manager.jobs)if(other!=job&&(manager.isActive(other)||str(other->data,"Status")=="Queued"))return true;return false;}
 void fileCompletionActions(){
  if(handlingQueueAction||!IsWindowEnabled())return;
  for(size_t index=0;index<fileActions.size();){
   auto job=fileActions[index].first;auto spec=fileActions[index].second;
   if(yes(spec,"WaitForOthers",true)&&otherDownloadsRunning(job)){++index;continue;}
   fileActions.erase(fileActions.begin()+index);{Lock lock(manager.mutex);if(std::find(manager.jobs.begin(),manager.jobs.end(),job)==manager.jobs.end()||str(job->data,"Status")!="Complete"||manager.isActive(job)||!scannerAllowsCompletion(job->data))continue;}
   struct Guard{bool& v;Guard(bool& value):v(value){v=true;}~Guard(){v=false;}} guard(handlingQueueAction);
   auto action=str(spec,"Action")+(yes(spec,"Force")?" (force closing enabled)":"");bool execute=false;ULONGLONG end=GetTickCount64()+num(spec,"DelaySeconds",30)*1000;
   Form d("Download completion action",442,181,this);
   d.init=[&]{d.label(str(job->data,"FileName")+" completed.",15,16,412,28);d.label("Requested action: "+action,15,49,412,34);auto remaining=d.label("",15,88,412,28);d.defaultButton(d.button("Cancel action",276,140,150,[&]{d.close(IDCANCEL);}));d.accept=[&]{d.close(IDCANCEL);};d.pulse=[&,remaining]{manager.tick();if(yes(spec,"WaitForOthers",true)&&otherDownloadsRunning(job)){d.close(IDCANCEL);return;}auto now=GetTickCount64();if(now>=end){execute=true;d.close();return;}remaining->SetWindowText(cs("Action starts in "+std::to_string((end-now+999)/1000)+" seconds."));};d.pulse();};
   d.DoModal();if(!execute)continue;
   const auto steps=validateCompletionSteps(spec.at("Actions").get<std::vector<std::string>>());
   for(const auto& step:steps){if(step=="Exit UDM"){PostMessage(WM_COMMAND,CMD_EXIT);return;}if(step=="Open downloaded file")openDownloadedFile(this,job->target());else performSystemAction(step,yes(spec,"Force")&&(step=="Shut down"||step=="Restart"));}
  }
 }
 void queueCompletionActions(){if(handlingQueueAction)return;struct Guard{bool& value;Guard(bool& v):value(v){value=true;}~Guard(){value=false;}} guard(handlingQueueAction);for(auto completion:manager.takeQueueCompletions()){
  auto queue=str(completion,"Queue"),action=completionSummary(queueEventSteps(completion));if(!manager.completionEventReady(completion))continue;
  Form d("Queue finished",442,181,this);bool execute=false;ULONGLONG end=GetTickCount64()+num(completion,"DelaySeconds",30)*1000;
  d.init=[&]{d.label(queueCompletionMessage(completion),15,16,412,28);d.label("Requested action: "+action+(action=="Open file"?" — "+str(completion,"File"):""),15,49,412,38);auto remaining=d.label("",15,88,412,28);d.button("Cancel action",276,140,150,[&]{d.close(IDCANCEL);});d.pulse=[&,remaining]{manager.tick();if(!manager.completionEventReady(completion)){d.close(IDCANCEL);return;}auto now=GetTickCount64();if(now>=end){execute=true;d.close();return;}remaining->SetWindowText(cs("Action starts in "+std::to_string((end-now+999)/1000)+" seconds."));};d.pulse();};d.DoModal();
  if(execute){bool exiting=false;executeQueueCompletion(completion,[&]{return manager.completionEventReady(completion);},[&](const std::string& step,bool force,const std::string& file){if(step=="Exit UDM"){PostMessage(WM_COMMAND,CMD_EXIT);exiting=true;}else if(step=="Open file")openFile(this,fs::path(wide(file)));else performSystemAction(step,force);});if(exiting)return;}
 }}
 void fillDownloadMenu(CMenu& popup){
  const auto selection=selected();bool adding=false;
  const auto enabled=commandStates({CMD_OPEN,CMD_OPENWITH,CMD_FOLDER,CMD_ZIP,CMD_RELOCATE,CMD_REDOWNLOAD,CMD_RESUME,CMD_STOP,CMD_PROGRESS,CMD_REFRESH_ADDRESS,CMD_DELETE,CMD_RECYCLE,CMD_MOVEQUEUE,CMD_REMOVEQUEUE,CMD_UP,CMD_DOWN,CMD_PROPERTIES},&selection,&adding);
  popup.CreatePopupMenu();
  // IDM 6.43 row menu order: Open, Open with, Open folder | Move/Rename | Redownload | Resume, Stop | Refresh address, Remove | Move to queue, Delete from queue | On Double click | Properties.
  auto flags=[&](UINT id){return MF_STRING|(enabled.at(id)?MF_ENABLED:MF_GRAYED);};
  for(auto entry:std::vector<std::pair<UINT,const wchar_t*>>{{CMD_OPEN,L"Open"},{CMD_OPENWITH,L"Open with..."},{CMD_FOLDER,L"Open folder"},{CMD_ZIP,L"ZIP contents..."},{0,nullptr},{CMD_RELOCATE,L"Move/Rename (Ctrl-M)"},{0,nullptr},{CMD_REDOWNLOAD,L"Redownload"},{0,nullptr},{CMD_RESUME,L"Resume Download"},{CMD_STOP,L"Stop Download"},{CMD_PROGRESS,L"Show progress"},{0,nullptr},{CMD_REFRESH_ADDRESS,L"Refresh download address"},{CMD_DELETE,L"Remove"},{CMD_RECYCLE,L"Recycle downloaded file..."},{0,nullptr}})
   if(entry.first)popup.AppendMenuW(flags(entry.first),entry.first,entry.second);else popup.AppendMenuW(MF_SEPARATOR);
  {CMenu queues;queues.CreatePopupMenu();fillMoveQueueCommands(queues,selection);MENUITEMINFOW info{sizeof(info)};info.fMask=MIIM_ID|MIIM_SUBMENU|MIIM_STRING|MIIM_STATE;info.wID=CMD_MOVEQUEUE;info.hSubMenu=queues.Detach();info.fState=enabled.at(CMD_MOVEQUEUE)?MFS_ENABLED:MFS_GRAYED;std::wstring label=adding?L"Add to queue":L"Move to queue";info.dwTypeData=label.data();moveQueuePosition=popup.GetMenuItemCount();popup.InsertMenuItemW(moveQueuePosition,&info,TRUE);}
  for(auto entry:std::vector<std::pair<UINT,const wchar_t*>>{{CMD_REMOVEQUEUE,L"Delete from queue"},{CMD_UP,L"Move up in queue"},{CMD_DOWN,L"Move down in queue"},{0,nullptr}})
   if(entry.first)popup.AppendMenuW(flags(entry.first),entry.first,entry.second);else popup.AppendMenuW(MF_SEPARATOR);
  CMenu doubleClick;doubleClick.CreatePopupMenu();auto behavior=str(preferences(),"CompletedDoubleClick","Properties");doubleClick.AppendMenuW(MF_STRING|(behavior=="Open"?MF_CHECKED:0),CMD_DOUBLE_OPEN,L"Open");doubleClick.AppendMenuW(MF_STRING|(behavior=="Properties"?MF_CHECKED:0),CMD_DOUBLE_PROPERTIES,L"Properties");popup.AppendMenuW(MF_POPUP,(UINT_PTR)doubleClick.Detach(),L"On Double click");
  popup.AppendMenuW(MF_SEPARATOR);popup.AppendMenuW(flags(CMD_PROPERTIES),CMD_PROPERTIES,L"Properties");
 }
 void downloadMenu(CPoint point){if(selected().empty())return;CMenu popup;fillDownloadMenu(popup);popup.TrackPopupMenu(TPM_RIGHTBUTTON,point.x,point.y,this);}
 void addMenu(const wchar_t* title,const std::vector<std::pair<UINT,const wchar_t*>>& items){CMenu child;child.CreatePopupMenu();for(auto& item:items)if(item.first)child.AppendMenuW(MF_STRING,item.first,item.second);else child.AppendMenuW(MF_SEPARATOR);menu.AppendMenuW(MF_POPUP,(UINT_PTR)child.Detach(),title);}
 HTREEITEM node(std::string caption,std::string value,HTREEITEM parent=TVI_ROOT,std::string icon="folder",std::string key={}){
  const auto image=iconIndex.count(icon)?iconIndex.at(icon):0;
  auto item=tree.InsertItem(cs(caption),image,image,parent);if(!item)throw std::runtime_error("Cannot create navigation item.");
  filters.push_back(value);tree.SetItemData(item,filters.size()-1);
  const auto stableKey=key.empty()?value:key;treeKeys.emplace(item,stableKey);treeItems.emplace(stableKey,item);
  if(value==filter&&!treeSelection)treeSelection=item;return item;
 }
 void buildTree(){
  if(treeRebuilding||!tree.GetSafeHwnd())return;
  // Capture only display inputs. All control messages, comparisons and key
  // serialization below run after the manager lock has been released.
  auto snapshot=navigationTreeSnapshot(manager);
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
  for(size_t i=0;i<snapshot.queueNames.size();++i){const auto& name=snapshot.queueNames[i];auto item=node(name,"queue:"+name,queues,"queue");const auto image=queueImages.at(snapshot.queueKinds.at(i));if(!tree.SetItemImage(item,image,image))throw std::runtime_error("Cannot create navigation queue image.");treeQueueNodes.push_back(item);treeQueueImages.push_back(image);}
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
 void layout(int w,int h){if(!table.m_hWnd)return;int bar=layoutToolbar(w),bottom=statusShown()?px(24):0;bar+=layoutSearchBar(w,bar);bool hide=yes(preferences(),"HideCategories");categoryHeading.ShowWindow(hide?SW_HIDE:SW_SHOW);categoryClose.ShowWindow(hide?SW_HIDE:SW_SHOW);tree.ShowWindow(hide?SW_HIDE:SW_SHOW);categoryHeading.MoveWindow(0,bar,px(117),px(19));categoryClose.MoveWindow(px(117),bar+px(1),px(18),px(17));categoryClose.BringWindowToTop();tree.MoveWindow(0,bar+px(20),px(136),std::max(0,h-bar-bottom-px(20)));table.MoveWindow(hide?0:px(140),bar,std::max(0,w-(hide?0:px(140))),std::max(0,h-bar-bottom));status.MoveWindow(0,h-bottom,w,bottom);clipboard.MoveWindow(px(155),h-bottom-px(31),w-px(175),px(27));}
 void showProgress(JobPtr job,bool explicitOpen=true){
 auto prefs=preferences();const bool queueOrigin=yes(job->data,"QueueOrigin");const bool selectedVideo=!queueOrigin&&!str(job->data,"SourceUrl").empty();auto presentation=progressPresentation(str(prefs,"ProgressStartMode"),queueOrigin,yes(prefs,"QueueProgressMinimized",false),explicitOpen||selectedVideo);if(presentation==ProgressPresentation::Hidden)return;
 for(auto& p:progress)if(p->GetSafeHwnd()){if(p->downloadId()==job->id()){p->showNormally();return;}}auto p=std::make_unique<Progress>(manager,job,this);if(!p->Create(101,this))throw std::runtime_error("Cannot create progress dialog.");if(presentation==ProgressPresentation::Tray&&p->showInTray()){progress.push_back(std::move(p));return;}bool minimized=presentation==ProgressPresentation::Minimized;p->ShowWindow(minimized?SW_SHOWMINNOACTIVE:SW_SHOW);if(!minimized){p->BringWindowToTop();p->SetForegroundWindow();}progress.push_back(std::move(p));}
 void complete(JobPtr job,bool requested=false){
  if(!requested){
  auto action=manager.takeDownloadCompletion(job);if(!action.empty())fileActions.push_back({job,action});
  Json record,prefs;{Lock lock(manager.mutex);record=job->data;prefs=manager.state["Settings"];}playDownloadSound(prefs,"Sound","CompletionSoundFile");
  for(auto& p:progress)if(p->GetSafeHwnd()&&p->downloadId()==job->id()&&yes(record,"CloseProgressOnCompletion",true))p->requestCompletionClose();
  if(yes(record,"OpenFolderOnCompletion"))try{openFile(this,job->target().parent_path());}catch(const std::exception& e){error(this,e);}
  if(yes(record,"QueueOrigin")||yes(prefs,"SuppressCompletionDialog")||yes(record,"SuppressCompletionDialog"))return;

  }
  Json record;{Lock lock(manager.mutex);record=job->snapshot();}
  // IDM 6.43 "Download complete" (resource dialog 286, docs/reference/completion-dialog.json):
  // the file-type icon, the two-line "Download complete / Downloaded X MB (N Bytes)" text,
  // Address and "The file saved as" edits, four buttons, the suppression check and the drag handle.
  bool checkedFile=record.contains("ScanResult");auto dialog=std::make_unique<Form>("Download complete",303,checkedFile?165:132,this);auto d=dialog.get();d->modeless=true;d->dialogUnits=true;
  d->init=[this,d,job,record,checkedFile]{
   d->ModifyStyle(0,WS_MINIMIZEBOX);const int extra=checkedFile?33:0;
   auto icon=d->make<FileDragIcon>(0,7,7,21,20);icon->path=job->target();
   d->label("Download complete\r\nDownloaded "+bytes(num(record,"Size"))+" ("+std::to_string(num(record,"Size"))+" Bytes)",34,7,262,32);
   d->label("Address",7,38,289);d->edit(downloadAddress(record),7,48,289,14,true);
   d->label("The file saved as",7,66,289);d->edit(utf8(job->target().wstring()),7,76,289,14,true);
   if(checkedFile)d->control(L"EDIT","Virus check: "+scannerSummary(record),WS_TABSTOP|ES_READONLY|ES_MULTILINE|ES_AUTOVSCROLL|WS_VSCROLL,7,94,289,29,WS_EX_CLIENTEDGE);
   auto suppress=d->check("Don't show this dialog again",false,16,117+extra,238);
   auto close=[this,d,suppress]{if(d->checked(suppress))preference("SuppressCompletionDialog",true);d->close();};
   d->button("Open",7,96+extra,61,[d,job,close]{if(openDownloadedFile(d,job->target()))close();});d->button("Open with...",76,96+extra,61,[d,job]{openWith(d,job->target());});d->button("Open folder",145,96+extra,61,[d,job,close]{openFile(d,job->target().parent_path());close();});d->button("Close",235,96+extra,61,close);
   auto drag=d->make<FileDragList>(WS_TABSTOP,276,115+extra,18,12);drag->setPath(job->target());d->accept=close;d->cancel=close;
  };if(!d->Create(101,this))throw std::runtime_error("Cannot open the completion dialog.");d->ShowWindow(requested?SW_SHOW:SW_SHOWNOACTIVATE);if(requested){d->BringWindowToTop();d->SetForegroundWindow();}completions.push_back(std::move(dialog));
 }

 std::unique_ptr<Form> quotaWarning;
 void showQuotaWarning(){
  if(quotaWarning&&quotaWarning->GetSafeHwnd())return;quotaWarning.reset();
  if(!IsWindowEnabled())return;auto notice=manager.takeQuotaWarning();if(notice.empty())return;
  quotaWarning=std::make_unique<Form>("Download limits exceeded!",339,93,this);auto d=quotaWarning.get();d->modeless=true;d->dialogUnits=true;
  d->init=[this,d,notice]{
   auto message=d->label("",7,7,325,60);
   d->accept=[d]{d->close();};d->cancel=d->accept;d->defaultButton(d->button("OK",144,72,50,d->accept));
   d->pulse=[this,d,message,notice]{auto current=manager.quotaStatus();if(!yes(current,"Waiting")||num(current,"PeriodStart")!=num(notice,"PeriodStart")||!yes(preferences(),"WarnQuota",true)){d->close();return;}
    message->SetWindowText(cs("The download limit of "+bytes(num(current,"LimitBytes"))+" every "+std::to_string(num(current,"Hours"))+" hour(s) has been reached.\r\n\r\n"+quotaWaitText(current)+"\r\nYou can change the limit in Options > Connection."));
   };d->pulse();
  };if(!d->Create(101,this))throw std::runtime_error("Cannot open the download-limit warning.");d->ShowWindow(SW_SHOWNOACTIVATE);
 }
 static i64 displayedFileSize(const Json& data){return udm::displayedFileSize(data);}

 void refreshModalSizes(){
  // A modal File Info runs inside refresh(). Keep size cells current without
  // re-entering presentation, completion, sorting or selection handling.
  Lock lock(manager.mutex);
  for(size_t row=0;row<visible.size()&&row<rowText.size();++row){
   const auto value=sizeCellText(visible[row]->data,displayedFileSize(visible[row]->data));
   if(rowText[row][2]!=value){table.SetItemText((int)row,2,cs(value));rowText[row][2]=value;}
  }
 }
 void refresh(bool captureOnly=false){if(refreshing){refreshModalSizes();return;}refreshing=true;try{
 Json prefs;{Lock lock(manager.mutex);prefs=manager.state["Settings"];}
 // Browser admission must not wait for history sorting, row painting or unrelated dialogs.
 if(!captureOnly){
 auto filterQuery=normalizedFindQuery(findQuery);if(!findFilterActive){filterQuery["Text"]="";filterQuery.erase("TextUtf16");}const auto searchText=str(filterQuery,"Text");const DownloadSearch filterMatcher(filterQuery);findFilterCache.begin(filterQuery);
 const Json view=Json::array({filter,findFilterActive,filterQuery,sortColumn,ascending});
 const int previousTop=table.GetTopIndex();
 JobPtr scrollAnchor=(rowView==view&&previousTop>0&&previousTop<(int)visible.size())?visible[previousTop]:nullptr;
 CRect anchorRect;const int anchorY=scrollAnchor&&table.GetItemRect(previousTop,&anchorRect,LVIR_BOUNDS)?anchorRect.top:0;
 table.sortIndicator(sortColumn,ascending);buildTree();const auto selectedJobs=selected();std::set<std::string> selection;std::string focusedId;
 JobPtr focused;const int focus=table.GetNextItem(-1,LVNI_FOCUSED);if(focus>=0&&focus<(int)visible.size())focused=visible[focus];
 const bool showAdded=table.GetColumnWidth(8)>0,showPath=table.GetColumnWidth(9)>0,showReferer=table.GetColumnWidth(10)>0,showRecovery=table.GetColumnWidth(11)>0;
 const bool includeHeaders=showReferer||showRecovery||sortColumn==10||sortColumn==11||!searchText.empty();
 std::vector<JobPtr> next;std::map<JobPtr,Json> searchRecords;std::unordered_map<const Job*,std::shared_ptr<const DownloadListRow>> snapshotRows;std::map<JobPtr,std::shared_ptr<const DownloadListRow>> refreshedRecords;std::set<JobPtr> quotaWaiting;std::vector<std::string> sounds;std::string storageWarning;
 {
  Lock lock(manager.mutex);prefs=manager.state["Settings"];storageWarning=manager.storageError;
  for(const auto& job:selectedJobs)selection.insert(job->id());if(focused)focusedId=focused->id();
  if(num(prefs,"QuotaMb")>0)quotaWaiting=manager.quotaWaitingJobs();
  std::map<std::string,int> queueIcons;for(const auto& queue:manager.state["Queues"])queueIcons.emplace(str(queue,"Name"),queueImages[queueIndicatorKind(queue)]);
  next.reserve(manager.jobs.size());snapshotRows.reserve(manager.jobs.size());
  for(const auto& job:manager.jobs){
   const auto& data=job->data;const auto category=str(data,"Category"),state=str(data,"Status"),id=job->id();const auto previous=observedStates.find(id);
   if(previous!=observedStates.end()&&previous->second!=state){if(state=="Failed")sounds.push_back("Failure");if(state=="Paused"&&(previous->second=="Downloading"||previous->second=="Pausing"))sounds.push_back("Pause");}observedStates[id]=state;
   const bool match=filter=="all"||(filter=="unfinished"&&state!="Complete")||(filter=="finished"&&state=="Complete")||(filter=="grabber"&&!str(data,"ProjectId").empty())||(filter.rfind("category:",0)==0&&category==filter.substr(9))||(filter.rfind("unfinished:",0)==0&&category==filter.substr(11)&&state!="Complete")||(filter.rfind("finished:",0)==0&&category==filter.substr(9)&&state=="Complete")||(filter.rfind("queue:",0)==0&&downloadQueueMember(data)&&str(data,"Queue")==filter.substr(6))||(filter.rfind("project:",0)==0&&str(data,"ProjectId")==filter.substr(8));
   if(!match)continue;
   const auto icon=queueIcons.find(str(data,"Queue"));const int image=downloadQueueMember(data)&&icon!=queueIcons.end()?icon->second:I_IMAGENONE;
   const auto cached=listRecordCache.find(job);auto record=downloadListRow(data,job->speed,includeHeaders,image,cached==listRecordCache.end()?std::shared_ptr<const DownloadListRow>{}:cached->second);
   snapshotRows.emplace(job.get(),record);refreshedRecords.emplace(job,std::move(record));if(!searchText.empty())searchRecords.emplace(job,filterMatcher.snapshot(data));next.push_back(job);
  }
 }
 listRecordCache.swap(refreshedRecords);refreshedRecords.clear();
 // Everything below uses immutable per-refresh records. No sorting, DPAPI
 // header decoding, shell icon lookup or list control messages hold the manager lock.
 for(const auto& sound:sounds)playDownloadSound(prefs,(sound+"SoundEnabled").c_str(),(sound+"SoundFile").c_str());
 if(!searchText.empty())next.erase(std::remove_if(next.begin(),next.end(),[&](const JobPtr& job){return !findFilterCache.matches(job,searchRecords.at(job),filterMatcher);}),next.end());
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
 std::optional<size_t> findSelection;if(rebuild||findCursor.orderChanged(sortColumn,ascending)){const int first=table.GetNextItem(-1,LVNI_SELECTED);if(first>=0)findSelection=(size_t)first;}
 findCursor.updateView(Json::array({filter,findFilterActive,filterQuery}),sortColumn,ascending,rebuild,findSelection);
 showStatus(storageWarning);updateFileIcons();
 updateToolbarCommands();if(yes(prefs,"ClipboardMonitor")){auto snapshot=clipboardSnapshot.read([]{return GetClipboardSequenceNumber();},[this]()->std::optional<std::string>{bool readable=false;auto value=clipboardText(&readable);if(!readable)return std::nullopt;return value;});if(snapshot){const auto& value=*snapshot;if(value!=lastClipboard){lastClipboard=value;if(clipboardCandidate(value,prefs)){Url u(value);copiedUrl=value;if(str(prefs,"ClipboardMode")=="Open Download File Info"){clipboard.ShowWindow(SW_HIDE);if(!clipboardPromptPending){clipboardPromptPending=true;PostMessage(WM_COMMAND,CMD_CLIPBOARD_CAPTURE);}}else{clipboard.SetWindowText(cs("Download copied URL: "+u.host+u.path));clipboard.ShowWindow(SW_SHOW);}}else{copiedUrl.clear();clipboard.ShowWindow(SW_HIDE);}}}}else{clipboardSnapshot.reset();clipboard.ShowWindow(SW_HIDE);}
 std::vector<std::pair<JobPtr,bool>> pending;{std::lock_guard<std::mutex> l(eventsMutex);pending.swap(events);}for(auto& [j,done]:pending){if(done){if(!finishUpdateDownload(j))complete(j);}else if(!yes(j->data,"ConfirmationPending")&&!yes(prefs,"SuppressProgressDialog")&&!yes(j->data,"SuppressProgressDialog"))showProgress(j,false);}
 }
 std::vector<JobPtr> confirmations;Json capturePresentations=Json::array();
 if(IsWindowEnabled()){
  Lock l(manager.mutex);capturePresentations=manager.pendingBrowserPresentations();std::set<std::string> capturedIds;for(const auto& offer:capturePresentations)capturedIds.insert(str(offer,"id"));
  std::vector<JobPtr> legacy;legacy.swap(manager.pendingOffers);
  for(auto j:legacy)if(!capturedIds.count(j->id())&&std::find(confirmations.begin(),confirmations.end(),j)==confirmations.end())confirmations.push_back(j);
  for(auto j:manager.jobs)if((str(j->data,"Status")=="Awaiting confirmation"||str(j->data,"Status")=="Awaiting duplicate choice")&&!capturedIds.count(j->id())&&infoSeen.insert(j->id()).second&&std::find(confirmations.begin(),confirmations.end(),j)==confirmations.end())confirmations.push_back(j);
 }if(!captureOnly&&IsWindowEnabled()&&!yes(prefs,"SuppressAuthenticationDialog")){JobPtr authentication;{Lock lock(manager.mutex);for(auto candidate:manager.jobs)if(!manager.isActive(candidate)&&!yes(candidate->data,"QueueOrigin")&&authenticationDialogReady(candidate->data)){candidate->data["AuthenticationPromptPending"]=false;authentication=candidate;manager.save();break;}}if(authentication){ShowWindow(SW_SHOW);requestDownloadLogin(this,manager,authentication);}}
 if(!captureOnly&&IsWindowEnabled()){
  if(manager.browserRecoveryRequested.exchange(false))captureReviewsSeen.clear();
  for(const auto& token:manager.pendingBrowserCaptureReviews())if(IsWindowEnabled()&&captureReviewsSeen.insert(token.get<std::string>()).second){ShowWindow(SW_SHOW);CaptureReviewDialog dialog(manager,token.get<std::string>(),this);if(dialog.DoModal()==-1)throw std::runtime_error("Cannot open captured download review.");}
 }
 for(const auto& offer:capturePresentations)if(IsWindowEnabled())presentBrowserCapture(this,manager,offer);
 for(auto j:confirmations){bool exists;{Lock lock(manager.mutex);exists=std::find(manager.jobs.begin(),manager.jobs.end(),j)!=manager.jobs.end();}if(exists){presentDownload(this,manager,j,true);}} }catch(const std::exception& e){showStatus(e.what());}refreshing=false;}

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
 void command(UINT id){try{if(runQueueMenuCommand(id)){refresh();return;}if(auto column=arrangeColumn(id);column>=0){sortColumn=column;ascending=true;rememberLayout();refresh();return;}if(toolbarMenuAction(id))return;if(!commandEnabled(id,true))return;auto chosen=selected();switch(id){
 case CMD_CLIPBOARD_CAPTURE:{clipboardPromptPending=false;auto value=copiedUrl;copiedUrl.clear();if(!clipboardCandidate(value,preferences())||!IsWindowEnabled())break;auto job=manager.offerDownload(value);presentDownload(this,manager,job);break;}
 case CMD_ZIP:previewZip(chosen[0]);break;
 case CMD_BASKET:preference("DropBasket",!yes(preferences(),"DropBasket"));showBasket();break;
 case CMD_RECYCLE:recycleDownloads(chosen);break;
 case CMD_OPENWITH:openWith(this,chosen[0]->target());break;
 case CMD_RELOCATE:moveCompleted(this,manager,chosen[0]);break;
 case CMD_REDOWNLOAD:{auto copy=manager.redownload(chosen[0]);downloadInfo(this,manager,copy);break;}
 case CMD_DOUBLE_OPEN:preference("CompletedDoubleClick","Open");break;
 case CMD_DOUBLE_PROPERTIES:preference("CompletedDoubleClick","Properties");break;
 case CMD_COLUMNS:customizeColumns();break;
 case CMD_TOOLBAR:toolbar.Customize();break;
 case CMD_EDITCATEGORY:if(!currentCategory().empty())categoryEditor(currentCategory());break;
 case CMD_DELETECATEGORY:if(!currentCategory().empty()&&MessageBox(L"Delete this category? Its records will move to Other; downloaded files stay in place.",L"Delete category",MB_YESNO|MB_ICONQUESTION)==IDYES){manager.deleteCategory(currentCategory());filter="all";}break;
 case CMD_DELETEQUEUE:if(filter.rfind("queue:",0)==0&&MessageBox(L"Delete this queue? Its records will be reassigned to another queue.",L"Delete queue",MB_YESNO|MB_ICONQUESTION)==IDYES){manager.deleteQueue(selectedQueue());filter="all";}break;
 case CMD_STARTQUEUEMENU:case CMD_STOPQUEUEMENU:{CPoint point;GetCursorPos(&point);queuePopup(id==CMD_STARTQUEUEMENU,point);break;}
 case CMD_LIMIT_ON:case CMD_LIMIT_OFF:{auto p=preferences();manager.setSettings(globalLimiterSettings(p,id==CMD_LIMIT_ON,rememberedGlobalLimit(p)));break;}
 case CMD_LIMIT:{Form d("Speed Limiter settings",302,89,this);d.dialogUnits=true;auto p=preferences();d.init=[&]{
  d.control(L"STATIC",str(p,"GlobalLimitMode")=="Each download"?"Maximum download speed for one file":"Maximum total download speed",SS_RIGHT,7,7,170,9);
  auto value=d.edit(std::to_string(rememberedGlobalLimit(p)),183,4,33,14);value->ModifyStyle(ES_LEFT,ES_RIGHT|ES_NUMBER);value->SendMessage(EM_SETLIMITTEXT,7);d.label("KBytes/sec",224,7,65);
  d.label("Note: Very low speeds can cause servers to disconnect. Avoid using Speed Limiter with servers that do not support resuming downloads.",7,23,288,26);
  auto startup=d.check("Always turn on Speed Limiter on UDM startup",yes(p,"GlobalLimitOnStartup"),7,52,288);
  d.accept=[&,startup,value]{auto next=preferences();auto raw=trim(text(value));if(raw.empty()||raw.size()>7||raw.find_first_not_of("0123456789")!=std::string::npos)throw std::runtime_error("Enter a speed limit from 1 to 1,000,000 KB/s.");next=globalLimiterSettings(next,num(next,"LimitKbps")>0,std::stoll(raw));next["GlobalLimitOnStartup"]=d.checked(startup);manager.setSettings(next);d.close();};d.defaultButton(d.button("OK",189,68,50,d.accept));d.button("Cancel",245,68,50,[&]{d.close(IDCANCEL);});
 };d.DoModal();break;}
 case CMD_SELECTALL:table.SetItemState(-1,LVIS_SELECTED,LVIS_SELECTED);break;
 case CMD_DARK:preference("DarkMode",!yes(preferences(),"DarkMode"));applyAppearance();break;
 case CMD_HOME:openProductLink(m_hWnd,productHome);break;case CMD_SUPPORT:openProductLink(m_hWnd,productSupport);break;case CMD_SHARE:showShareDialog(this);break;case CMD_CHECK_UPDATES:startUpdateCheck(true);break;case CMD_TIP_DAY:showTipOfDay();break;case CMD_HELP_CONTENTS:case CMD_HELP_TUTORIALS:case CMD_HELP_SCHEDULER:case CMD_HELP_GRABBER:showHelpTopic(id-CMD_HELP_CONTENTS);break;case CMD_LANGUAGE_ENGLISH:break;case CMD_FONT:chooseFont();break;
 case CMD_FONT_RESET:{auto p=preferences();auto defaults=defaultSettings();for(auto key:{"FontName","FontHeight","FontWeight"})p[key]=defaults[key];manager.setSettings(p);applyAppearance();break;}
 case CMD_TRAY_COLOR:preference("TrayIcon","Color");updateTray();break;
 case CMD_TRAY_SYSTEM:preference("TrayIcon","Classic");updateTray();break;
 case CMD_TRAY_HIDE:preference("TrayIcon","Hidden");updateTray();break;
 case CMD_FINDNEXT:findNext();break;
 case CMD_HIDE_CATEGORIES:preference("HideCategories",!yes(preferences(),"HideCategories"));applyAppearance();if(yes(preferences(),"HideCategories"))table.SetFocus();break;
 case CMD_REMOVEQUEUE:setQueueMembershipBatch(manager,chosen,false);break;
 case CMD_ADD:addAddress(this,manager);break;case CMD_PASTE:{auto value=copiedUrl.empty()?clipboardText():copiedUrl;clipboard.ShowWindow(SW_HIDE);copiedUrl.clear();if(value.find('\n')!=std::string::npos)batchDialog(this,manager,value);else addAddress(this,manager,value);break;}case CMD_BATCH:batchDialog(this,manager);break;case CMD_BATCH_CLIPBOARD:selectBatch(this,manager,importTextUrls(clipboardText()));break;case CMD_RESUME:for(auto j:chosen){if(!commandCanResume(j))continue;if(!str(j->data,"DuplicateOf").empty())presentDownload(this,manager,j);else{if(yes(j->data,"ContentPrefixPending")){auto bytes=num(j->data,"ContentPrefixBytes");auto choice=MessageBox(cs("The file starts with "+std::to_string(bytes)+" bytes of web page text before the actual content.\n\nYes: remove that text and save only the file.\nNo: keep the data exactly as received."),L"Web page text before the file",MB_YESNOCANCEL|MB_ICONQUESTION);if(choice==IDCANCEL)continue;{Lock l(manager.mutex);j->data[choice==IDYES?"StripContentPrefix":"AllowContentPrefix"]=true;}}if(yes(j->data,"WebPageReceived")){auto choice=MessageBox(L"The server sent a web page or text instead of the expected file.\n\nYes: save the received content as the file anyway.\nNo: try the download again.",L"Web page received",MB_YESNOCANCEL|MB_ICONQUESTION);if(choice==IDCANCEL)continue;if(choice==IDYES){Lock l(manager.mutex);j->data["AllowWebPageContent"]=true;}}manager.resume(j);}}break;case CMD_STOP:{std::set<std::string> stopped;for(auto j:chosen)if(commandCanStop(j)){manager.pause(j);stopped.insert(j->id());}closeStoppedProgress(stopped);break;}case CMD_STOPALL:case CMD_STOP_CLOSE_ALL:{std::vector<JobPtr> jobs;std::set<std::string> stopped;{Lock l(manager.mutex);jobs=manager.jobs;for(auto j:jobs)if(str(j->data,"Status")!="Complete"||manager.isActive(j))stopped.insert(j->id());}for(auto j:jobs)if(commandCanStop(j))manager.pause(j);if(id==CMD_STOP_CLOSE_ALL)closeStoppedProgress(stopped);break;}case CMD_DELETE:removeDownloadSelection(this,manager,chosen);break;case CMD_CLEAN:{std::vector<JobPtr> completed;{Lock l(manager.mutex);for(auto j:manager.jobs)if(str(j->data,"Status")=="Complete"&&!manager.isActive(j))completed.push_back(j);}removeDownloadSelection(this,manager,completed);}break;case CMD_OPTIONS:{Options d(manager,this);d.help=[this]{showHelpTopic(0);};d.networkIntegration=[this](CWnd* owner){if(!networkCapture)networkCapture=std::make_unique<NetworkCaptureSession>(&manager);NetworkDialog dialog(owner,&manager,networkCapture.get());dialog.DoModal();};d.DoModal();applyAppearance();break;}case CMD_SCHEDULER:{Scheduler d(manager,this,selectedQueue());d.DoModal();break;}case CMD_STARTQUEUE:manager.queueRun(selectedQueue(),true);break;case CMD_STOPQUEUE:manager.queueRun(selectedQueue(),false);break;case CMD_GRABBER:{GrabberDialog d(manager,this);d.DoModal();break;}case CMD_PROPERTIES:if(!chosen.empty()){if(str(chosen[0]->data,"Status")=="Complete")completedProperties(this,manager,chosen[0]);else if(!str(chosen[0]->data,"DuplicateOf").empty())presentDownload(this,manager,chosen[0]);else downloadInfo(this,manager,chosen[0],true);}break;case CMD_PROGRESS:if(!chosen.empty()){if(!str(chosen[0]->data,"DuplicateOf").empty())presentDownload(this,manager,chosen[0]);else showProgress(chosen[0]);}break;case CMD_REFRESH_ADDRESS:if(!chosen.empty())refreshDownloadAddress(this,manager,chosen[0]);break;case CMD_OPEN:if(!chosen.empty()&&str(chosen[0]->data,"Status")=="Complete")openDownloadedFile(this,chosen[0]->target());break;case CMD_FOLDER:if(!chosen.empty())openFile(this,chosen[0]->target().parent_path());break;case CMD_NETWORK:{if(!networkCapture)networkCapture=std::make_unique<NetworkCaptureSession>(&manager);NetworkDialog d(this,&manager,networkCapture.get());d.DoModal();break;}case CMD_BROWSER:{auto p=preferences();if(browserIntegration(this,p))manager.setSettings(p);break;}case CMD_ABOUT:showAboutDialog(this,preferences());break;case CMD_IMPORT_IDM:importDownloads(1);break;case CMD_IMPORT_TEXT:importDownloads(2);break;case CMD_EXPORT_IDM:exportDownloads(chosen,2);break;case CMD_EXPORT_TEXT:exportDownloads(chosen,0);break;case CMD_IMPORT:importDownloads();break;case CMD_EXPORT:exportDownloads(chosen);break;case CMD_RECOVERY:{{Lock lock(manager.mutex);for(auto job:manager.jobs)if(manager.isActive(job))throw std::runtime_error("Pause active downloads before opening backup and recovery.");}recoveryOnClose=true;quitting=true;OnClose();return;}case CMD_EXIT:quitting=true;OnClose();return;case CMD_NEWQUEUE:{auto name=prompt(this,"Create queue");if(!name.empty()){auto q=defaultQueue(name);q["Enabled"]=false;manager.setQueue(q);Scheduler d(manager,this,name);d.DoModal();}break;}case CMD_NEWCATEGORY:categoryEditor();break;case CMD_UP:if(!chosen.empty())manager.move(chosen[0],-1);break;case CMD_DOWN:if(!chosen.empty())manager.move(chosen[0],1);break;case CMD_MOVEQUEUE:if(!chosen.empty()){auto choices=membershipQueues(chosen);if(choices.empty())throw std::runtime_error("Create a synchronization queue in Scheduler for completed files.");auto current=str(chosen[0]->data,"Queue");if(std::find(choices.begin(),choices.end(),current)==choices.end())current=choices.front();const bool adding=std::none_of(chosen.begin(),chosen.end(),[](const JobPtr& job){return downloadQueueMember(job->data);});Form d(adding?"Add to queue":"Move to queue",349,103,this);d.init=[&]{auto queue=d.combo(choices,current,13,14,322);d.button(adding?"Add":"Move",163,64,82,[&,queue]{setQueueMembershipBatch(manager,chosen,true,text(queue));d.close();});d.button("Cancel",257,64,79,[&]{d.close(IDCANCEL);});};d.DoModal();}break;case CMD_SEARCH:showFind();break;case CMD_QUICKFILTER:showSearchBar(!search.IsWindowVisible(),true);break;}refresh();}catch(const std::exception& e){error(this,e);}}
 afx_msg int OnCreate(LPCREATESTRUCT c){if(CFrameWnd::OnCreate(c)==-1)return -1;scale=GetDpiForWindow(m_hWnd)/96.0f;font.CreateFontW(-px(11),0,0,0,FW_NORMAL,FALSE,FALSE,FALSE,DEFAULT_CHARSET,OUT_DEFAULT_PRECIS,CLIP_DEFAULT_PRECIS,DEFAULT_QUALITY,DEFAULT_PITCH,L"Tahoma");menu.CreateMenu();addMenu(L"&Tasks",{{CMD_ADD,L"&Add new download"},{CMD_BATCH,L"Add &batch download"},{CMD_BATCH_CLIPBOARD,L"Add batch download from &clipboard"},{CMD_GRABBER,L"Run site &grabber"},{0,L""},{CMD_BASKET,L"Show &drop target"},{0,L""},{CMD_EXPORT,L"E&xport"},{CMD_IMPORT,L"&Import"},{0,L""},{CMD_EXIT,L"&Exit"}});auto tasks=menu.GetSubMenu(0);CMenu exports;exports.CreatePopupMenu();exports.AppendMenuW(MF_STRING,CMD_EXPORT_IDM,L"To IDM export &file");exports.AppendMenuW(MF_STRING,CMD_EXPORT_TEXT,L"To &text file");exports.AppendMenuW(MF_STRING,CMD_EXPORT,L"To UDM &catalog...");exports.AppendMenuW(MF_SEPARATOR);exports.AppendMenuW(MF_STRING,CMD_RECOVERY,L"Backup and &recovery...");tasks->ModifyMenuW(CMD_EXPORT,MF_BYCOMMAND|MF_POPUP,(UINT_PTR)exports.Detach(),L"E&xport");CMenu imports;imports.CreatePopupMenu();imports.AppendMenuW(MF_STRING,CMD_IMPORT_IDM,L"&From IDM export file");imports.AppendMenuW(MF_STRING,CMD_IMPORT_TEXT,L"From &text file");imports.AppendMenuW(MF_STRING,CMD_IMPORT,L"From UDM &catalog...");tasks->ModifyMenuW(CMD_IMPORT,MF_BYCOMMAND|MF_POPUP,(UINT_PTR)imports.Detach(),L"&Import");addMenu(L"&File",{{CMD_STOP,L"&Stop Download"},{CMD_DELETE,L"&Remove"},{CMD_RESUME,L"&Download Now"},{CMD_REDOWNLOAD,L"R&edownload"}});addMenu(L"&Downloads",{{CMD_STOPALL,L"&Pause All"},{CMD_STOP_CLOSE_ALL,L"&Stop All"},{0,L""},{CMD_CLEAN,L"&Delete All Completed"},{0,L""},{CMD_SEARCH,L"Find (Ctrl-F)"},{CMD_FINDNEXT,L"Find Next (F3)"},{0,L""},{CMD_SCHEDULER,L"S&cheduler"},{CMD_STARTQUEUE,L"Start &queue"},{CMD_STOPQUEUE,L"S&top queue"},{0,L""},{CMD_LIMIT,L"Speed &Limiter"},{0,L""},{CMD_OPTIONS,L"&Options"}});CMenu startQueueMenu;startQueueMenu.CreatePopupMenu();startQueueMenu.AppendMenuW(MF_STRING,CMD_STARTQUEUE,L"Start");startQueueMenu.AppendMenuW(MF_STRING,CMD_STARTQUEUEMENU,L"Choose queue...");menu.GetSubMenu(2)->ModifyMenuW(CMD_STARTQUEUE,MF_BYCOMMAND|MF_POPUP,(UINT_PTR)startQueueMenu.Detach(),L"Start &queue");CMenu stopQueueMenu;stopQueueMenu.CreatePopupMenu();stopQueueMenu.AppendMenuW(MF_STRING,CMD_STOPQUEUE,L"Stop");stopQueueMenu.AppendMenuW(MF_STRING,CMD_STOPQUEUEMENU,L"Choose queue...");menu.GetSubMenu(2)->ModifyMenuW(CMD_STOPQUEUE,MF_BYCOMMAND|MF_POPUP,(UINT_PTR)stopQueueMenu.Detach(),L"S&top queue");addMenu(L"&View",{{CMD_HIDE_CATEGORIES,L"Hide &categories"},{CMD_TOOLBAR,L"&Toolbar"},{CMD_TRAY_COLOR,L"&UDM tray icon"},{CMD_COLUMNS,L"C&ustomize URL List..."},{CMD_DARK,L"Dark Mode support"},{CMD_FONT,L"Font"},{0,L""},{CMD_LANGUAGE_ENGLISH,L"&Language"}});CMenu trayChoices;trayChoices.CreatePopupMenu();trayChoices.AppendMenuW(MF_STRING,CMD_TRAY_COLOR,L"3&D style");trayChoices.AppendMenuW(MF_STRING,CMD_TRAY_SYSTEM,L"C&lassic style");trayChoices.AppendMenuW(MF_STRING,CMD_TRAY_HIDE,L"Do&n't show");menu.GetSubMenu(3)->ModifyMenuW(CMD_TRAY_COLOR,MF_BYCOMMAND|MF_POPUP,(UINT_PTR)trayChoices.Detach(),L"&UDM tray icon");CMenu languages;languages.CreatePopupMenu();languages.AppendMenuW(MF_STRING|MF_CHECKED,CMD_LANGUAGE_ENGLISH,L"&English");menu.GetSubMenu(3)->ModifyMenuW(CMD_LANGUAGE_ENGLISH,MF_BYCOMMAND|MF_POPUP,(UINT_PTR)languages.Detach(),L"&Language");addMenu(L"&Help",{{CMD_HELP_CONTENTS,L"H&elp contents"},{CMD_HELP_TUTORIALS,L"&Tutorials"},{CMD_HELP_SCHEDULER,L"&Scheduler and queues"},{CMD_HELP_GRABBER,L"&Grabber Help"},{CMD_TIP_DAY,L"Tip of the &Day..."},{0,L""},{CMD_HOME,L"UDM &Home Page"},{CMD_SUPPORT,L"&Contact UDM Support"},{0,L""},{CMD_CHECK_UPDATES,L"Check for &updates..."},{0,L""},{CMD_ABOUT,L"&About UDM"},{CMD_SHARE,L"Tell a &friend"}});CMenu limiter;limiter.CreatePopupMenu();limiter.AppendMenuW(MF_STRING,CMD_LIMIT_ON,L"Turn o&n");limiter.AppendMenuW(MF_STRING,CMD_LIMIT_OFF,L"Turn of&f");limiter.AppendMenuW(MF_SEPARATOR);limiter.AppendMenuW(MF_STRING,CMD_LIMIT,L"S&ettings...");
menu.GetSubMenu(2)->ModifyMenuW(CMD_LIMIT,MF_BYCOMMAND|MF_POPUP,(UINT_PTR)limiter.Detach(),L"Speed &Limiter");
CMenu fontMenu;fontMenu.CreatePopupMenu();fontMenu.AppendMenuW(MF_STRING,CMD_FONT,L"Choose Font...");fontMenu.AppendMenuW(MF_STRING,CMD_FONT_RESET,L"Reset to default Font");menu.GetSubMenu(3)->ModifyMenuW(CMD_FONT,MF_BYCOMMAND|MF_POPUP,(UINT_PTR)fontMenu.Detach(),L"Font");
CMenu arrange;arrange.CreatePopupMenu();
const wchar_t* arrangeLabels[]={L"By &Order Of Addition",L"By &File Name",L"By &Size",L"By St&atus",L"By &Time Left",L"By Transfer &Rate",L"By &Last Try Date",L"By &Description",L"By Save &Path",L"By R&eferer",L"By Parent &Web Page"};
for(UINT i=0;i<11;++i)arrange.AppendMenuW(MF_STRING,CMD_SORT_ADDED+i,arrangeLabels[i]);
menu.GetSubMenu(3)->InsertMenuW(1,MF_BYPOSITION|MF_POPUP,(UINT_PTR)arrange.Detach(),L"&Arrange files");
SetMenu(&menu);rebuildListImages();createToolbar();categoryHeading.Create(L"Categories",WS_CHILD|WS_VISIBLE|SS_LEFT|WS_BORDER,CRect(0,0,1,1),this);categoryHeading.SetFont(&font);categoryClose.Create(L"Close categories",WS_CHILD|WS_VISIBLE|WS_TABSTOP|BS_OWNERDRAW,CRect(0,0,1,1),this,CMD_HIDE_CATEGORIES);tree.Create(WS_CHILD|WS_VISIBLE|WS_TABSTOP|TVS_HASBUTTONS|TVS_HASLINES|TVS_LINESATROOT|TVS_SHOWSELALWAYS,CRect(0,0,1,1),this,501);tree.SetFont(&font);tree.SetImageList(&images,TVSIL_NORMAL);tree.SetItemHeight((SHORT)px(17));table.Create(WS_CHILD|WS_VISIBLE|WS_TABSTOP|LVS_REPORT|LVS_SHOWSELALWAYS,CRect(0,0,1,1),this,502);if(!SetWindowSubclass(table.GetHeaderCtrl()->GetSafeHwnd(),headerContextProc,1,(DWORD_PTR)m_hWnd))throw std::runtime_error("Cannot initialize column header menu.");table.SetFont(&font);table.SetImageList(&images,LVSIL_SMALL);table.SetExtendedStyle(LVS_EX_FULLROWSELECT|LVS_EX_GRIDLINES|LVS_EX_DOUBLEBUFFER|LVS_EX_HEADERDRAGDROP|LVS_EX_SUBITEMIMAGES|LVS_EX_INFOTIP);const wchar_t* columns[]={L"File Name",L"Q",L"Size",L"Status",L"Time left",L"Transfer rate",L"Last Try Date",L"Description"};int widths[]={205,38,72,97,77,91,132,160};for(int i=0;i<columnCount;++i)table.InsertColumn(i,i<8?columns[i]:columnName(i),LVCFMT_LEFT,i<8?px(widths[i]):0);restoreLayout();status.Create(WS_CHILD,CRect(0,0,1,1),this,503);status.SetFont(&font);search.Create(WS_CHILD|WS_TABSTOP|ES_AUTOHSCROLL|WS_BORDER,CRect(0,0,1,1),this,504);search.SetFont(&font);searchLabel.Create(L"Find:",WS_CHILD|SS_RIGHT,CRect(0,0,1,1),this);searchLabel.SetFont(&font);searchFind.Create(L"Find",WS_CHILD|WS_TABSTOP|BS_PUSHBUTTON,CRect(0,0,1,1),this,SearchSubmit);searchFind.SetFont(&font);searchSettings.Create(L"Settings...",WS_CHILD|WS_TABSTOP|BS_PUSHBUTTON,CRect(0,0,1,1),this,SearchSettings);searchSettings.SetFont(&font);searchClose.Create(L"Close search",WS_CHILD|WS_TABSTOP|BS_OWNERDRAW,CRect(0,0,1,1),this,SearchClose);searchClose.SetFont(&font);search.SendMessage(EM_SETCUEBANNER,TRUE,(LPARAM)L"Find using the fields and options selected in Find file");clipboard.Create(L"",WS_CHILD|WS_TABSTOP|BS_PUSHBUTTON,CRect(0,0,1,1),this,CMD_PASTE);clipboard.SetFont(&font);tray.cbSize=sizeof(tray);tray.hWnd=m_hWnd;tray.uID=1;tray.uFlags=NIF_MESSAGE|NIF_ICON|NIF_TIP;tray.uCallbackMessage=TRAY_MESSAGE;tray.hIcon=AfxGetApp()->LoadIcon(1);wcscpy_s(tray.szTip,L"UDM Download Manager");Shell_NotifyIconW(NIM_ADD,&tray);drop.Register(this);tableDrop.attach(&table);treeDrop.attach(&tree);
 tableDrop.apply=[this](const std::string& id,CPoint point){JobPtr job;{Lock lock(manager.mutex);for(auto j:manager.jobs)if(j->id()==id)job=j;}int row=table.HitTest(point);JobPtr before=row>=0&&row<(int)visible.size()?visible[row]:JobPtr();auto queue=before?str(before->data,"Queue"):selectedQueue();manager.reorder(job,queue,before);sortColumn=-1;refresh();};
 treeDrop.apply=[this](const std::string& id,CPoint point){auto item=tree.HitTest(point);if(!item)return;auto index=tree.GetItemData(item);if(index>=filters.size()||filters[index].rfind("queue:",0)!=0)throw std::runtime_error("Drop an unfinished download onto a queue.");JobPtr job;{Lock lock(manager.mutex);for(auto j:manager.jobs)if(j->id()==id)job=j;}manager.reorder(job,filters[index].substr(6));refresh();};
 SetTimer(1,500,nullptr);manager.queueSound=[this](bool started){playDownloadSound(manager.state["Settings"],started?"QueueStartSoundEnabled":"QueueStopSoundEnabled",started?"QueueStartSoundFile":"QueueStopSoundFile");};manager.showCompletedDownload=[this](JobPtr job){complete(job,true);};manager.event=[this](JobPtr j,bool complete){std::lock_guard<std::mutex> l(eventsMutex);events.push_back({j,complete});};buildTree();applyAppearance();showBasket();return 0;}
 afx_msg void OnLButtonUp(UINT flags,CPoint point){if(draggedJob){auto job=draggedJob;draggedJob.reset();ReleaseCapture();ClientToScreen(&point);CRect area;tree.GetWindowRect(&area);try{if(area.PtInRect(point)){tree.ScreenToClient(&point);treeDrop.apply(job->id(),point);}else{table.GetWindowRect(&area);if(area.PtInRect(point)){table.ScreenToClient(&point);tableDrop.apply(job->id(),point);}}}catch(const std::exception& e){error(this,e);}return;}CFrameWnd::OnLButtonUp(flags,point);}
 afx_msg void OnCaptureChanged(CWnd* window){draggedJob.reset();CFrameWnd::OnCaptureChanged(window);}
 afx_msg void OnMouseMove(UINT flags,CPoint point){if(draggedJob)::SetCursor(LoadCursor(nullptr,IDC_SIZEALL));CFrameWnd::OnMouseMove(flags,point);}
 afx_msg LRESULT OnMainDpiChanged(WPARAM dpi,LPARAM position){
  const int next=HIWORD(dpi),previous=windowDpi();if(next<=0||!position||!table.GetSafeHwnd())return 0;
  auto suggested=*reinterpret_cast<RECT*>(position);SetRedraw(FALSE);
  try{
   if(dpiColumnWidths.size()!=columnCount){dpiColumnWidths.assign(columnCount,0);dpiRenderedWidths.assign(columnCount,-1);}
   for(int i=0;i<columnCount;++i){const int current=table.GetColumnWidth(i);if(current!=dpiRenderedWidths[i])dpiColumnWidths[i]=(double)current*96/previous;}
   scale=next/96.0f;rebuildListImages();applyAppearance();
   for(int i=0;i<columnCount;++i){dpiRenderedWidths[i]=(int)std::lround(dpiColumnWidths[i]*next/96);table.SetColumnWidth(i,dpiRenderedWidths[i]);}
   SetWindowPos(nullptr,suggested.left,suggested.top,suggested.right-suggested.left,suggested.bottom-suggested.top,SWP_NOZORDER|SWP_NOACTIVATE);
  }catch(const std::exception& e){showStatus(e.what());}
  SetRedraw(TRUE);RedrawWindow(nullptr,nullptr,RDW_INVALIDATE|RDW_ALLCHILDREN|RDW_ERASE);return 0;
 }
 afx_msg void OnSize(UINT type,int w,int h){CFrameWnd::OnSize(type,w,h);layout(w,h);}
  afx_msg void OnTimer(UINT_PTR id){if(IsWindowEnabled())try{while(takeCliHangup(manager))performSystemAction("Disconnect dial-up / VPN");}catch(const std::exception& e){error(this,e);return;}if(quitAfterDownload&&IsWindowEnabled()){Lock lock(manager.mutex);auto job=quitAfterDownload;if(std::find(manager.jobs.begin(),manager.jobs.end(),job)==manager.jobs.end())quitAfterDownload.reset();else if(cliCompletionReady(manager,job)){quitAfterDownload.reset();PostMessage(WM_COMMAND,CMD_EXIT);return;}}try{if(networkCapture)networkCapture->poll();}catch(const std::exception& e){showStatus(e.what());}try{manager.tick();showQuotaWarning();}catch(const std::exception& e){showStatus(e.what());}if(IsWindowEnabled()&&manager.browserSettingsRequested.exchange(false))PostMessage(WM_COMMAND,CMD_BROWSER);completions.erase(std::remove_if(completions.begin(),completions.end(),[](const auto& d){return !d->GetSafeHwnd();}),completions.end());rememberLayout();refresh();if(!refreshing)try{fileCompletionActions();queueCompletionActions();}catch(const std::exception& e){error(this,e);}CFrameWnd::OnTimer(id);}
  afx_msg LRESULT OnPresentCapture(WPARAM,LPARAM){if(!quitting&&IsWindowEnabled())refresh(true);return 0;}

  afx_msg LRESULT OnInitialRefresh(WPARAM,LPARAM){refresh();reviewAutomationBatches();return 0;}
  afx_msg LRESULT OnUpdateResult(WPARAM,LPARAM l){
   std::unique_ptr<UpdateOutcome> outcome((UpdateOutcome*)l);updateChecking=false;if(updateThread.joinable())updateThread.join();
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
  // True when the job is an update setup UDM downloaded: its gate replaces the ordinary completion handling.
  bool finishUpdateDownload(JobPtr job){
   std::string version,sha;{Lock lock(manager.mutex);version=str(job->data,"UpdateSetupVersion");sha=str(job->data,"ExpectedSha256");}
   if(version.empty()||sha.empty())return false;
   if(updateLaunching)return true;
   updateLaunching=true;showStatus("Checking the downloaded UDM "+version+" setup...");
   if(updateLaunchThread.joinable())updateLaunchThread.join();
   // Copying, hashing and WinVerifyTrust on a setup take seconds: off the UI thread.
   updateLaunchThread=std::thread([path=job->target(),sha,hwnd=m_hWnd]{auto outcome=std::make_unique<UpdateLaunch>();try{outcome->verified=verifyDownloadedSetup(path,sha);}catch(const std::exception& e){outcome->error=e.what();}::PostMessage(hwnd,UPDATE_LAUNCHED,0,(LPARAM)outcome.release());});
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
 afx_msg void OnClose(){if(!quitting){Lock l(manager.mutex);if(yes(manager.state["Settings"],"CloseToTray")){ShowWindow(SW_HIDE);return;}}quitting=true;rememberLayout();if(updateCancel)updateCancel->stop=true;if(updateThread.joinable())updateThread.join();if(updateLaunchThread.joinable())updateLaunchThread.join();KillTimer(1);if(stopIntegration)stopIntegration();showStatus("Saving downloads and closing...");EnableWindow(FALSE);try{if(networkCapture)networkCapture->stop();manager.stop();}catch(const std::exception& e){EnableWindow(TRUE);quitting=false;recoveryOnClose=false;SetTimer(1,500,nullptr);try{if(restartIntegration)restartIntegration();}catch(const std::exception& bridge){error(this,bridge);}error(this,e);return;}fileIconLoader.stop();commandFiles.stop();if(recoveryOnClose){try{launchRecoveryMode(manager.root,true);}catch(const std::exception& e){error(this,e);}}manager.event={};manager.queueSound={};manager.showCompletedDownload={};if(quotaWarning&&quotaWarning->GetSafeHwnd())quotaWarning->DestroyWindow();quotaWarning.reset();for(auto& d:completions)if(d->GetSafeHwnd())d->DestroyWindow();completions.clear();basket.reset();tableDrop.Revoke();treeDrop.Revoke();drop.Revoke();for(auto& p:progress)if(p->GetSafeHwnd()){p->DestroyWindow();}Shell_NotifyIconW(NIM_DELETE,&tray);CFrameWnd::OnClose();}
 afx_msg LRESULT OnShow(WPARAM,LPARAM){ShowWindow(SW_RESTORE);SetForegroundWindow();reviewAutomationBatches();return 0;}
 // Link arrays sent through automation wait, encrypted, until reviewed here.
 void reviewAutomationBatches(){if(!IsWindowEnabled())return;for(;;){auto offer=nextAutomationBatch(manager);if(offer.empty())break;ShowWindow(SW_SHOW);automationBatchDialog(this,manager,offer);}}
 afx_msg LRESULT OnDropUrl(WPARAM,LPARAM l){std::unique_ptr<std::string> s((std::string*)l);try{if(s->find('\n')!=std::string::npos)batchDialog(this,manager,*s);else addAddress(this,manager,*s);}catch(const std::exception& e){error(this,e);}return 0;}
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
 afx_msg LRESULT OnTray(WPARAM,LPARAM l){if(l==WM_LBUTTONDBLCLK||l==NIN_KEYSELECT||l==NIN_SELECT)OnShow(0,0);else if(l==WM_RBUTTONUP){CMenu popup;popup.CreatePopupMenu();popup.AppendMenuW(MF_STRING,CMD_ADD,L"Add URL");popup.AppendMenuW(MF_STRING,CMD_RESUME,L"Resume selected");popup.AppendMenuW(MF_STRING,CMD_STOPALL,L"Pause all downloads");popup.AppendMenuW(MF_STRING,CMD_STOP_CLOSE_ALL,L"Stop all downloads");CMenu startQueues,stopQueues;startQueues.CreatePopupMenu();stopQueues.CreatePopupMenu();const auto queueStates=manager.queueCommandStates();fillQueueCommands(startQueues,true,queueStates);fillQueueCommands(stopQueues,false,queueStates);popup.AppendMenuW(MF_POPUP,(UINT_PTR)startQueues.Detach(),L"Start queue");popup.AppendMenuW(MF_POPUP,(UINT_PTR)stopQueues.Detach(),L"Stop queue");popup.AppendMenuW(MF_STRING,CMD_SCHEDULER,L"Scheduler");popup.AppendMenuW(MF_STRING,CMD_LIMIT,L"Speed limiter...");popup.AppendMenuW(MF_STRING,CMD_OPTIONS,L"Options");popup.AppendMenuW(MF_STRING|(yes(preferences(),"DropBasket")?MF_CHECKED:0),CMD_BASKET,L"Drop basket");popup.AppendMenuW(MF_SEPARATOR);popup.AppendMenuW(MF_STRING,CMD_EXIT,L"Exit UDM");CPoint p;GetCursorPos(&p);SetForegroundWindow();popup.TrackPopupMenu(TPM_RIGHTBUTTON,p.x,p.y,this);}return 0;}
 // MFC queries update routing before showing menus; OnCommand alone does not
 // advertise handlers and otherwise leaves every custom menu command disabled.
 afx_msg void OnUpdateAppCommand(CCmdUI* ui){
  // A popup is routed using its first child's ID. An inactive first queue
  // must not disable access to the other queues in that submenu.
  if(ui->m_pSubMenu&&ui->m_nID>=32000&&ui->m_nID<34000){ui->Enable(TRUE);return;}
  if(queuePopupUpdating&&ui->m_pMenu&&!ui->m_pSubMenu&&ui->m_nID>=32000&&ui->m_nID<34000){const auto state=ui->m_pMenu->GetMenuState(ui->m_nIndex,MF_BYPOSITION);ui->Enable(state!=UINT(-1)&&(state&(MF_DISABLED|MF_GRAYED))==0);return;}
  ui->Enable(commandEnabled(ui->m_nID));if(auto column=arrangeColumn(ui->m_nID);column>=0)ui->SetRadio(sortColumn==column);
  if(ui->m_nID==CMD_LANGUAGE_ENGLISH){ui->SetRadio(TRUE);return;}
  switch(ui->m_nID){case CMD_TRAY_COLOR:case CMD_TRAY_SYSTEM:case CMD_TRAY_HIDE:case CMD_LIMIT_ON:case CMD_LIMIT_OFF:case CMD_BASKET:case CMD_DARK:case CMD_HIDE_CATEGORIES:break;default:return;}
  const auto prefs=preferences();
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
 BOOL OnCommand(WPARAM w,LPARAM l)override{UINT id=LOWORD(w);if(id==CMD_STOPALL&&(HWND)l==toolbar.GetSafeHwnd())id=CMD_STOP_CLOSE_ALL;if((id>=CMD_ADD&&id<=CMD_TOOLBAR_DISCOVER)||(id>=31000&&id<31064)||isQueueMenuCommand(id)){command(id);return TRUE;}if(HIWORD(w)==BN_CLICKED&&(id==SearchSubmit||id==SearchSettings||id==SearchClose)){try{if(id==SearchSubmit)submitFindFilter();else if(id==SearchSettings)command(CMD_SEARCH);else{showSearchBar(false);refresh();}}catch(const std::exception& e){error(this,e);}return TRUE;}if(id==504&&HIWORD(w)==EN_CHANGE)return TRUE;return CFrameWnd::OnCommand(w,l);}
 BOOL OnNotify(WPARAM w,LPARAM l,LRESULT* result)override{auto header=(NMHDR*)l;if(toolbarNotification(header,result))return TRUE;if(header->hwndFrom==tree.m_hWnd&&header->code==TVN_SELCHANGED){auto change=(NMTREEVIEW*)l;if(!change->itemNew.hItem){*result=0;return TRUE;}auto index=tree.GetItemData(change->itemNew.hItem);if(!treeRebuilding&&index<filters.size()){filter=filters[index];refresh();}*result=0;return TRUE;}if(header->hwndFrom==table.m_hWnd){if(header->code==LVN_ITEMCHANGED&&!refreshing){const auto change=reinterpret_cast<NMLISTVIEW*>(header);if((change->uChanged&LVIF_STATE)&&((change->uOldState^change->uNewState)&LVIS_SELECTED))scheduleCommandFileRefresh();}if(header->code==LVN_ENDSCROLL){if(!refreshing)updateFileIcons();*result=0;return TRUE;}if(header->code==LVN_GETINFOTIPW){auto tip=reinterpret_cast<NMLVGETINFOTIPW*>(header);if(tip->pszText&&tip->cchTextMax>0)tip->pszText[0]=L'\0';if(tip->iItem>=0&&tip->iItem<(int)visible.size()&&tip->pszText&&tip->cchTextMax>0){auto value=wide(str(visible[tip->iItem]->data,"FileName")+"\n"+queueInfo(visible[tip->iItem]->data));wcsncpy_s(tip->pszText,tip->cchTextMax,value.c_str(),_TRUNCATE);}*result=0;return TRUE;}if(header->code==LVN_BEGINDRAG){auto chosen=selected();if(chosen.size()==1&&str(chosen[0]->data,"Status")!="Complete"&&!manager.isActive(chosen[0])){draggedJob=chosen[0];SetCapture();::SetCursor(LoadCursor(nullptr,IDC_SIZEALL));*result=0;return TRUE;}std::vector<fs::path> paths;{Lock lock(manager.mutex);for(auto job:selected())if(str(job->data,"Status")=="Complete")paths.push_back(job->target());}try{dragSavedFiles(paths);}catch(const std::exception& e){error(this,e);}*result=0;return TRUE;}if(header->code==NM_DBLCLK){auto item=reinterpret_cast<NMITEMACTIVATE*>(header);if(item->iItem>=0&&item->iItem<(int)visible.size()){table.SetItemState(-1,0,LVIS_SELECTED);table.SetItemState(item->iItem,LVIS_SELECTED|LVIS_FOCUSED,LVIS_SELECTED|LVIS_FOCUSED);auto chosen=selected();if(!chosen.empty())command(str(chosen[0]->data,"Status")=="Complete"?(str(preferences(),"CompletedDoubleClick","Properties")=="Open"?CMD_OPEN:CMD_PROPERTIES):CMD_PROGRESS);}*result=0;return TRUE;}if(header->code==LVN_COLUMNCLICK){auto change=(NMLISTVIEW*)l;if(sortColumn==change->iSubItem)ascending=!ascending;else{sortColumn=change->iSubItem;ascending=true;}refresh();*result=0;return TRUE;}}return CFrameWnd::OnNotify(w,l,result);}
 static LRESULT CALLBACK headerContextProc(HWND window,UINT message,WPARAM wParam,LPARAM lParam,UINT_PTR id,DWORD_PTR owner){
  if(message==WM_CONTEXTMENU){::SendMessageW((HWND)owner,WM_CONTEXTMENU,(WPARAM)window,lParam);return 0;}
  if(message==WM_NCDESTROY)RemoveWindowSubclass(window,headerContextProc,id);
  return DefSubclassProc(window,message,wParam,lParam);
 }
 afx_msg void OnContextMenu(CWnd* origin,CPoint point){try{
  auto header=table.GetHeaderCtrl();CRect headerArea;if(header)header->GetWindowRect(&headerArea);
  if(header&&(origin->GetSafeHwnd()==header->GetSafeHwnd()||(origin->GetSafeHwnd()==table.GetSafeHwnd()&&headerArea.PtInRect(point)))){
   if(point.x==-1&&point.y==-1)point=CPoint(headerArea.left+px(12),headerArea.bottom);
   CMenu popup;popup.CreatePopupMenu();popup.AppendMenuW(MF_STRING,CMD_COLUMNS,L"&Columns...");
   auto choice=popup.TrackPopupMenu(TPM_RETURNCMD|TPM_RIGHTBUTTON,point.x,point.y,this);if(choice)command(choice);return;
  }
  if(origin->GetSafeHwnd()==tree.GetSafeHwnd()){treeMenu(point);return;}if(origin->GetSafeHwnd()==toolbar.GetSafeHwnd()){toolbarPopup(point);return;}if(origin->GetSafeHwnd()!=table.GetSafeHwnd()){CFrameWnd::OnContextMenu(origin,point);return;}
  if(point.x==-1&&point.y==-1){
   int row=table.GetNextItem(-1,LVNI_FOCUSED);if(row<0||!(table.GetItemState(row,LVIS_SELECTED)&LVIS_SELECTED))row=table.GetNextItem(-1,LVNI_SELECTED);
   if(row<0)return;table.EnsureVisible(row,FALSE);CRect rect;table.GetItemRect(row,&rect,LVIR_BOUNDS);point=CPoint(rect.left+px(30),rect.bottom);table.ClientToScreen(&point);
  }else{CPoint local=point;table.ScreenToClient(&local);UINT flags=0;int row=table.HitTest(local,&flags);if(row<0)return;if(!(table.GetItemState(row,LVIS_SELECTED)&LVIS_SELECTED)){table.SetItemState(-1,0,LVIS_SELECTED);table.SetItemState(row,LVIS_SELECTED|LVIS_FOCUSED,LVIS_SELECTED|LVIS_FOCUSED);}}
  downloadMenu(point);
 }catch(const std::exception& e){error(this,e);} }
 BOOL PreTranslateMessage(MSG* msg)override{if(msg->message==WM_KEYDOWN){
  if(msg->wParam==VK_RETURN){if(msg->hwnd==search.GetSafeHwnd()||msg->hwnd==searchFind.GetSafeHwnd()){searchFind.SendMessage(BM_CLICK);return TRUE;}if(msg->hwnd==searchSettings.GetSafeHwnd()){searchSettings.SendMessage(BM_CLICK);return TRUE;}if(msg->hwnd==searchClose.GetSafeHwnd()){searchClose.SendMessage(BM_CLICK);return TRUE;}}
  if(msg->wParam==VK_TAB&&GetKeyState(VK_CONTROL)>=0&&GetKeyState(VK_MENU)>=0&&::IsChild(m_hWnd,msg->hwnd)&&::IsDialogMessageW(m_hWnd,msg))return TRUE;
if(msg->wParam==VK_F1){command(CMD_HELP_CONTENTS);return TRUE;}if(msg->wParam==VK_ESCAPE&&draggedJob){draggedJob.reset();ReleaseCapture();return TRUE;}if(GetKeyState(VK_CONTROL)<0){if(msg->wParam=='N'){command(CMD_ADD);return TRUE;}if(msg->wParam=='V'&&GetFocus()!=&search){command(CMD_PASTE);return TRUE;}if(msg->wParam=='A'&&GetFocus()==&table){command(CMD_SELECTALL);return TRUE;}if(msg->wParam=='F'){command(GetKeyState(VK_SHIFT)<0?CMD_QUICKFILTER:CMD_SEARCH);return TRUE;}if(msg->wParam=='M'){command(CMD_RELOCATE);return TRUE;}}if(msg->wParam==VK_F3){command(CMD_FINDNEXT);return TRUE;}if(msg->wParam==VK_DELETE&&GetFocus()==&table){command(CMD_DELETE);return TRUE;}}return CFrameWnd::PreTranslateMessage(msg);}
public:
 // Fetches and verifies the signed manifest off the window thread. Without a built-in update key the
 // interactive command keeps the release-page dialog and the startup check does nothing.
 void startUpdateCheck(bool interactive){
  if(updateChecking){if(interactive)updateInteractive=true;return;}  // the running check answers the menu too
  if(!updateKeyConfigured()){if(interactive)showUpdateDialog(this,preferences());return;}
  updateChecking=true;updateInteractive=interactive;auto settings=preferences();updateCancel=std::make_shared<Cancel>();updateCancel->deadline=GetTickCount64()+20000;
  if(updateThread.joinable())updateThread.join();
  updateThread=std::thread([this,settings,cancel=updateCancel]{auto outcome=std::make_unique<UpdateOutcome>();try{outcome->manifest=fetchUpdateManifest(settings,*cancel);}catch(const Cancelled&){outcome->error="The update check was canceled or timed out.";}catch(const std::exception& e){outcome->error=e.what();}PostMessage(UPDATE_RESULT,0,(LPARAM)outcome.release());});
 }
 JobPtr quitAfterDownload;std::function<void()> stopIntegration,restartIntegration; explicit MainWindow(Manager& m):manager(m){findQuery=normalizedFindQuery(preferences().value("FindQuery",Json::object()));auto cls=AfxRegisterWndClass(CS_DBLCLKS,LoadCursor(nullptr,IDC_ARROW),(HBRUSH)(COLOR_BTNFACE+1),AfxGetApp()->LoadIcon(1));if(!Create(cls,L"UDM Download Manager",WS_OVERLAPPEDWINDOW,CRect(160,150,922,610)))throw std::runtime_error("Cannot create UDM window.");SetWindowPos(nullptr,0,0,px(778),px(470),SWP_NOMOVE|SWP_NOZORDER);CenterWindow();}
};
BEGIN_MESSAGE_MAP(MainWindow,CFrameWnd)
 ON_WM_SYSCOLORCHANGE()
 ON_WM_SETTINGCHANGE()
 ON_UPDATE_COMMAND_UI_RANGE(CMD_ADD,CMD_TOOLBAR_DISCOVER,OnUpdateAppCommand)
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
 ON_MESSAGE(TRAY_MESSAGE,OnTray)
 ON_MESSAGE(DROP_URL,OnDropUrl)
END_MESSAGE_MAP()
#if !defined(UDM_TOOLBAR_COMPONENT_TEST) && !defined(UDM_CAPTURE_PRESENTATION_COMPONENT_TEST)
class Application:public CWinApp {
 std::unique_ptr<Manager> manager;std::unique_ptr<PipeServer> pipe;std::unique_ptr<AutomationHost> automation;Handle mutex;std::unique_ptr<RecoveryDataLease> recoveryLease;bool recoveryClosed=false;
public:BOOL InitInstance()override{
 CWinApp::InitInstance();INITCOMMONCONTROLSEX controls{sizeof(controls),ICC_WIN95_CLASSES|ICC_DATE_CLASSES|ICC_PROGRESS_CLASS};InitCommonControlsEx(&controls);AfxOleInit();SetRegistryKey(L"UDM");
 try{
  int argc=0;auto argv=CommandLineToArgvW(GetCommandLineW(),&argc);if(!argv)throw std::runtime_error("Cannot read launch options.");
  std::vector<std::wstring> args;for(int i=1;i<argc;++i)args.emplace_back(argv[i]);LocalFree(argv);auto options=parseLaunch(args);
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
  if(GetLastError()==ERROR_ALREADY_EXISTS){auto reply=send(options.address.empty()&&!options.startQueue?Json{{"action","show"}}:options.request(),12000);if(!yes(reply,"ok"))throw std::runtime_error(str(reply,"error","UDM did not accept this command."));return FALSE;}
  recoveryLease=std::make_unique<RecoveryDataLease>(options.data);{RecoveryAvailability availability;recoverPendingRestore(*recoveryLease);}
  manager=std::make_unique<Manager>(options.data);applyStartupGlobalLimiter(*manager);auto frame=new MainWindow(*manager);m_pMainWnd=frame;frame->stopIntegration=[this]{pipe.reset();};frame->restartIntegration=[this,frame]{if(!pipe)pipe=std::make_unique<PipeServer>(*manager,[frame]{frame->PostMessage(SHOW_APP);},[frame]{frame->PostMessage(PRESENT_CAPTURE);});};manager->startQueuesOnStartup();pipe=std::make_unique<PipeServer>(*manager,[frame]{frame->PostMessage(SHOW_APP);},[frame]{frame->PostMessage(PRESENT_CAPTURE);});
  // Host the automation class object here: COM connects callers to the running
  // desktop directly, so Udm.Automation.exe is only launched when UDM is closed.
  try{automation=std::make_unique<AutomationHost>([this,frame](const Json& message){auto action=str(message,"action");if(action=="automation-add"){manager->receive(message);if(!(num(message.at("link"),"flags")&1))frame->PostMessage(SHOW_APP);}else if(action=="automation-batch"){queueAutomationBatch(*manager,message);frame->PostMessage(SHOW_APP);}else throw std::invalid_argument("Unknown automation command.");});}catch(const std::exception&){automation.reset();}
  if(!options.address.empty()){auto job=manager->receive(options.request());if(options.quitAfterDownload){Lock lock(manager->mutex);if(str(job->data,"Status")!="Complete")frame->quitAfterDownload=job;}}
  if(options.startQueue)manager->queueRun("Main queue",true);
  frame->ShowWindow(options.background?SW_HIDE:SW_SHOW);frame->UpdateWindow();frame->PostMessage(INITIAL_REFRESH);if(!options.silent&&!options.startQueue&&!options.waitProcess&&!options.recovery&&updateCheckDue(manager->state["Settings"],epoch()))frame->startUpdateCheck(false);if(!options.background&&!options.silent&&!options.startQueue&&!options.waitProcess&&options.address.empty()&&yes(manager->state["Settings"],"ShowTipsOnStartup",true))frame->PostMessage(WM_COMMAND,CMD_TIP_DAY);return TRUE;
 }catch(const std::exception& e){AfxMessageBox(cs(e.what()),MB_OK|MB_ICONERROR);return FALSE;}
}
 int ExitInstance()override{automation.reset();pipe.reset();manager.reset();recoveryLease.reset();AfxOleTerm(FALSE);auto result=CWinApp::ExitInstance();return recoveryClosed?0:result;}
};
Application application;
#endif
}
