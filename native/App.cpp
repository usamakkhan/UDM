#include "Version.hpp"
#include "HelpContent.hpp"
#include "GlobalLimiter.hpp"
#include "RecoveryAvailability.hpp"
#include <afxwin.h>
#include <afxcmn.h>
#include <afxdlgs.h>
#include <afxole.h>
#include <atlimage.h>
#include <regex>
#include "Core.hpp"
#include "QueueMembership.hpp"
#include "QueueCompletion.hpp"
#include "CategoryEditor.hpp"
#include "Launch.hpp"
#include "CliCompletion.hpp"
#include "RecoverySession.hpp"
#include "RecoveryUi.hpp"
#include "Ui.hpp"
#include "UpdateUi.hpp"
#include "ShareUi.hpp"
#include "AboutUi.hpp"
#include "RemovalUi.hpp"
#include "WorkflowsUi.hpp"
#include "SystemActions.hpp"
#include "ZipPreview.hpp"
#include "Catalog.hpp"
#include "ExportUi.hpp"
#include "IdmImportUi.hpp"
#include "ClipboardPolicy.hpp"
#include "ToolbarImages.hpp"
#include "QueueIndicators.hpp"
#include "TrayIcon.hpp"
#include <shellapi.h>
#include <fstream>
namespace udm {
enum : UINT { CMD_ADD=200,CMD_RESUME,CMD_STOP,CMD_STOPALL,CMD_DELETE,CMD_CLEAN,CMD_OPTIONS,CMD_SCHEDULER,CMD_STARTQUEUE,CMD_STOPQUEUE,CMD_GRABBER,CMD_BATCH,CMD_IMPORT,CMD_EXPORT,CMD_EXIT,CMD_PROPERTIES,CMD_OPEN,CMD_FOLDER,CMD_NETWORK,CMD_BROWSER,CMD_ABOUT,CMD_NEWQUEUE,CMD_NEWCATEGORY,CMD_UP,CMD_DOWN,CMD_PASTE,CMD_PROGRESS,CMD_MOVEQUEUE,CMD_SEARCH,CMD_REFRESH_ADDRESS,CMD_OPENWITH,CMD_RELOCATE,CMD_REDOWNLOAD,CMD_DOUBLE_OPEN,CMD_DOUBLE_PROPERTIES,CMD_COLUMNS,CMD_DARK,CMD_FONT,CMD_TRAY_COLOR,CMD_TRAY_SYSTEM,CMD_TRAY_HIDE,CMD_FINDNEXT,CMD_HIDE_CATEGORIES,CMD_REMOVEQUEUE,CMD_TOOLBAR,CMD_EDITCATEGORY,CMD_DELETECATEGORY,CMD_DELETEQUEUE,CMD_QUICKFILTER,CMD_STARTQUEUEMENU,CMD_STOPQUEUEMENU,CMD_SELECTALL,CMD_LIMIT,CMD_BASKET,CMD_RECYCLE,CMD_ZIP,CMD_CLIPBOARD_CAPTURE,CMD_TOOLBAR_SMALL,CMD_TOOLBAR_MEDIUM,CMD_TOOLBAR_LARGE,CMD_TOOLBAR_ICONSTEXT,CMD_TOOLBAR_ICONSONLY,CMD_TOOLBAR_TEXTONLY,CMD_TOOLBAR_HIDE,CMD_TOOLBAR_BUILTIN,CMD_TOOLBAR_LOAD,CMD_TOOLBAR_FOLDER,CMD_TOOLBAR_RELOAD,CMD_IMPORT_IDM,CMD_IMPORT_TEXT,CMD_EXPORT_IDM,CMD_EXPORT_TEXT,CMD_BATCH_CLIPBOARD,CMD_RECOVERY,CMD_SORT_ADDED,CMD_SORT_NAME,CMD_SORT_SIZE,CMD_SORT_STATUS,CMD_SORT_TIME,CMD_SORT_RATE,CMD_SORT_LAST,CMD_SORT_DESCRIPTION,CMD_SORT_PATH,CMD_SORT_REFERER,CMD_SORT_PAGE,CMD_LIMIT_ON,CMD_LIMIT_OFF,CMD_FONT_RESET,CMD_STOP_CLOSE_ALL,CMD_LANGUAGE_ENGLISH,CMD_HELP_CONTENTS,CMD_HELP_TUTORIALS,CMD_HELP_SCHEDULER,CMD_HELP_GRABBER,CMD_TIP_DAY,CMD_CHECK_UPDATES,CMD_HOME,CMD_SUPPORT,CMD_SHARE };
constexpr UINT SHOW_APP=WM_APP+20,TRAY_MESSAGE=WM_APP+21,DROP_URL=WM_APP+22;
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
 Manager& manager;CMenu menu;CFont font;CTreeCtrl tree;ThemeList table;CStatusBarCtrl status;CStatic categoryHeading;PanelCloseButton categoryClose;CEdit search;CButton clipboard;CImageList images;std::map<std::string,int> iconIndex;
 std::vector<std::string> filters;std::vector<JobPtr> visible;std::vector<std::unique_ptr<Progress>> progress;std::set<std::string> infoSeen,captureReviewsSeen;
 std::mutex eventsMutex;std::vector<std::pair<JobPtr,bool>> events;NOTIFYICONDATAW tray{};DropTarget drop{this};bool recoveryOnClose=false,quitting=false,refreshing=false,clipboardPromptPending=false;float scale=1;std::string filter="all",copiedUrl,lastClipboard;int sortColumn=-1;bool ascending=true;std::string lastTree;bool treeRebuilding=false;HTREEITEM treeSelection=nullptr;std::map<std::string,std::string> observedStates;std::unique_ptr<Basket> basket;std::vector<std::unique_ptr<Form>> completions;bool handlingQueueAction=false;std::vector<std::pair<JobPtr,Json>> fileActions;QueueDropTarget tableDrop,treeDrop;JobPtr draggedJob;
 int px(int n)const{return (int)(n*scale);}
 int windowDpi()const{return (int)std::lround(scale*96);}
 std::vector<double> dpiColumnWidths;std::vector<int> dpiRenderedWidths;
 std::vector<JobPtr> selected(){std::vector<JobPtr> jobs;for(int i=-1;(i=table.GetNextItem(i,LVNI_SELECTED))>=0;)if(i<(int)visible.size())jobs.push_back(visible[i]);return jobs;}
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
  images.DeleteImageList();images.Attach(next.Detach());iconIndex=std::move(indices);queueImages=queues;
 }
 int queueImageFor(const Json& data){Lock lock(manager.mutex);if(!downloadQueueMember(data))return I_IMAGENONE;for(const auto& q:manager.state["Queues"])if(str(q,"Name")==str(data,"Queue"))return queueImages[queueIndicatorKind(q)];return I_IMAGENONE;}
 std::string queueInfo(const Json& data){Lock lock(manager.mutex);if(!downloadQueueMember(data))return "Not in a queue";for(const auto& q:manager.state["Queues"])if(str(q,"Name")==str(data,"Queue"))return queueIndicatorDescription(q);return "Not in an existing queue";}

 std::string selectedQueue(){return filter.rfind("queue:",0)==0?filter.substr(6):"Main queue";}
 std::string clipboardText(){if(!OpenClipboard())return {};std::string value;auto h=GetClipboardData(CF_UNICODETEXT);if(h){auto p=(const wchar_t*)GlobalLock(h);if(p){auto max=GlobalSize(h)/sizeof(wchar_t),n=wcsnlen_s(p,max);if(n<max&&n<1024*1024)value=utf8(std::wstring(p,n));GlobalUnlock(h);}}CloseClipboard();return trim(value);}
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
 bool commandEnabled(UINT id){
  Lock lock(manager.mutex);auto chosen=selected();
  auto any=[&](auto predicate){return std::any_of(chosen.begin(),chosen.end(),predicate);};
  auto all=[&](auto predicate){return !chosen.empty()&&std::all_of(chosen.begin(),chosen.end(),predicate);};
  switch(id){
   case CMD_RECOVERY:return std::none_of(manager.jobs.begin(),manager.jobs.end(),[&](JobPtr j){return manager.isActive(j);});
   case CMD_RESUME:return any([&](JobPtr j){return commandCanResume(j);});
   case CMD_STOP:return any([&](JobPtr j){return commandCanStop(j);});
   case CMD_DELETE:return all([&](JobPtr j){return !manager.isActive(j);});
   case CMD_ZIP:return chosen.size()==1&&str(chosen[0]->data,"Status")=="Complete"&&lower(utf8(chosen[0]->target().extension().wstring()))==".zip"&&fs::is_regular_file(chosen[0]->target());
   case CMD_RECYCLE:return all([&](JobPtr j){return !manager.isActive(j)&&str(j->data,"Status")=="Complete"&&fs::is_regular_file(j->target());});
   case CMD_PROPERTIES:return chosen.size()==1;
   case CMD_PROGRESS:case CMD_FOLDER:return chosen.size()==1;
   case CMD_REMOVEQUEUE:return all([&](JobPtr j){return !manager.isActive(j)&&yes(j->data,"QueueMember",str(j->data,"Status")!="Complete");});
   case CMD_MOVEQUEUE:return all([&](JobPtr j){return !manager.isActive(j);})&&!membershipQueues(chosen).empty();
   case CMD_UP:case CMD_DOWN:return chosen.size()==1&&!manager.isActive(chosen[0])&&yes(chosen[0]->data,"QueueMember",str(chosen[0]->data,"Status")!="Complete");
   case CMD_REFRESH_ADDRESS:return chosen.size()==1&&manager.canRefreshAddress(chosen[0]);
   case CMD_REDOWNLOAD:return chosen.size()==1&&(str(chosen[0]->data,"Status")=="Complete"||((str(chosen[0]->data,"Status")=="Paused"||str(chosen[0]->data,"Status")=="Failed")&&Url(str(chosen[0]->data,"Url")).scheme=="ftp"));
   case CMD_OPENWITH:case CMD_RELOCATE:case CMD_OPEN:return chosen.size()==1&&str(chosen[0]->data,"Status")=="Complete";
   case CMD_STOP_CLOSE_ALL:for(auto& p:progress)if(p->GetSafeHwnd())for(auto j:manager.jobs)if(j->id()==p->downloadId()&&str(j->data,"Status")!="Complete")return true;[[fallthrough]];
   case CMD_STOPALL:return std::any_of(manager.jobs.begin(),manager.jobs.end(),[&](JobPtr j){return manager.isActive(j)||str(j->data,"Status")=="Queued";});
   case CMD_CLEAN:return std::any_of(manager.jobs.begin(),manager.jobs.end(),[](JobPtr j){return str(j->data,"Status")=="Complete";});
   default:return true;
  }
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
  Lock lock(manager.mutex);const auto selection=selected();const bool adding=std::none_of(selection.begin(),selection.end(),[](const JobPtr& job){return downloadQueueMember(job->data);});
  popup.CreatePopupMenu();
  for(auto entry:std::vector<std::pair<UINT,const wchar_t*>>{{CMD_RESUME,L"Resume"},{CMD_STOP,L"Stop"},{CMD_PROGRESS,L"Show progress"},{CMD_PROPERTIES,L"Properties"},{CMD_REFRESH_ADDRESS,L"Refresh download address"},{CMD_OPEN,L"Open"},{CMD_OPENWITH,L"Open with..."},{CMD_ZIP,L"ZIP contents..."},{CMD_RELOCATE,L"Move/Rename\tCtrl+M"},{CMD_REDOWNLOAD,L"Redownload"},{CMD_FOLDER,L"Open folder"},{CMD_REMOVEQUEUE,L"Remove from queue"},{CMD_MOVEQUEUE,L"Move to queue"},{CMD_UP,L"Move up in queue"},{CMD_DOWN,L"Move down in queue"},{CMD_DELETE,L"Remove from list"},{CMD_RECYCLE,L"Recycle downloaded file..."}})popup.AppendMenuW(MF_STRING|(commandEnabled(entry.first)?MF_ENABLED:MF_GRAYED),entry.first,entry.first==CMD_MOVEQUEUE&&adding?L"Add to queue":entry.second);
  CMenu doubleClick;doubleClick.CreatePopupMenu();auto behavior=str(preferences(),"CompletedDoubleClick","Properties");doubleClick.AppendMenuW(MF_STRING|(behavior=="Open"?MF_CHECKED:0),CMD_DOUBLE_OPEN,L"Open");doubleClick.AppendMenuW(MF_STRING|(behavior=="Properties"?MF_CHECKED:0),CMD_DOUBLE_PROPERTIES,L"Properties");popup.AppendMenuW(MF_POPUP,(UINT_PTR)doubleClick.Detach(),L"On double-click");
 }
 void downloadMenu(CPoint point){if(selected().empty())return;CMenu popup;fillDownloadMenu(popup);popup.TrackPopupMenu(TPM_RIGHTBUTTON,point.x,point.y,this);}
 void addMenu(const wchar_t* title,const std::vector<std::pair<UINT,const wchar_t*>>& items){CMenu child;child.CreatePopupMenu();for(auto& item:items)if(item.first)child.AppendMenuW(MF_STRING,item.first,item.second);else child.AppendMenuW(MF_SEPARATOR);menu.AppendMenuW(MF_POPUP,(UINT_PTR)child.Detach(),title);}
 HTREEITEM node(std::string caption,std::string value,HTREEITEM parent=TVI_ROOT,std::string icon="folder"){auto item=tree.InsertItem(cs(caption),iconIndex.count(icon)?iconIndex[icon]:0,iconIndex.count(icon)?iconIndex[icon]:0,parent);filters.push_back(value);tree.SetItemData(item,filters.size()-1);if(value==filter&&!treeSelection)treeSelection=item;return item;}
 void buildTree(){Lock l(manager.mutex);std::string key=manager.state["Queues"].dump()+manager.state["Projects"].dump()+Json(manager.categories()).dump();if(key==lastTree)return;lastTree=key;treeRebuilding=true;treeSelection=nullptr;tree.SetRedraw(FALSE);tree.DeleteAllItems();filters.clear();auto cats=manager.categories();auto all=node("All Downloads","all",TVI_ROOT,"all");for(auto c:cats)node(c=="Archives"?"Compressed":c,"category:"+c,all,lower(c));auto unfinished=node("Unfinished","unfinished");auto finished=node("Finished","finished",TVI_ROOT,"complete");for(auto c:cats){node(c=="Archives"?"Compressed":c,"unfinished:"+c,unfinished,lower(c));node(c=="Archives"?"Compressed":c,"finished:"+c,finished,lower(c));}auto grab=node("Grabber projects","grabber",TVI_ROOT,"grabber");for(auto p:manager.state["Projects"])node(str(p,"Name"),"project:"+str(p,"Id"),grab,"grabber");auto queues=node("Queues","all",TVI_ROOT,"queue");for(auto q:manager.state["Queues"]){auto item=node(str(q,"Name"),"queue:"+str(q,"Name"),queues,"queue");auto icon=queueImages[queueIndicatorKind(q)];tree.SetItemImage(item,icon,icon);}tree.Expand(all,TVE_EXPAND);tree.Expand(queues,TVE_EXPAND);if(treeSelection)tree.SelectItem(treeSelection);treeRebuilding=false;tree.SetRedraw(TRUE);tree.Invalidate();}
 void layout(int w,int h){if(!table.m_hWnd)return;int bar=layoutToolbar(w),bottom=px(24);bool hide=yes(preferences(),"HideCategories");categoryHeading.ShowWindow(hide?SW_HIDE:SW_SHOW);categoryClose.ShowWindow(hide?SW_HIDE:SW_SHOW);tree.ShowWindow(hide?SW_HIDE:SW_SHOW);categoryHeading.MoveWindow(0,bar,px(117),px(19));categoryClose.MoveWindow(px(117),bar+px(1),px(18),px(17));categoryClose.BringWindowToTop();tree.MoveWindow(0,bar+px(20),px(136),h-bar-bottom-px(20));int findHeight=search.IsWindowVisible()?px(31):0;table.MoveWindow(hide?0:px(140),bar+findHeight,w-(hide?0:px(140)),h-bar-bottom-findHeight);status.MoveWindow(0,h-bottom,w,bottom);if(search.IsWindowVisible()){search.MoveWindow(hide?px(8):px(148),bar+px(3),w-(hide?px(16):px(156)),px(25));search.BringWindowToTop();}clipboard.MoveWindow(px(155),h-bottom-px(31),w-px(175),px(27));}
 void showProgress(JobPtr job,bool explicitOpen=true){
 auto prefs=preferences();auto presentation=progressPresentation(str(prefs,"ProgressStartMode"),yes(job->data,"QueueOrigin"),yes(prefs,"QueueProgressMinimized",true),explicitOpen);if(presentation==ProgressPresentation::Hidden)return;
 for(auto& p:progress)if(p->GetSafeHwnd()){if(p->downloadId()==job->id()){p->showNormally();return;}}auto p=std::make_unique<Progress>(manager,job,this);if(!p->Create(101,this))throw std::runtime_error("Cannot create progress dialog.");if(presentation==ProgressPresentation::Tray&&p->showInTray()){progress.push_back(std::move(p));return;}bool minimized=presentation==ProgressPresentation::Minimized;p->ShowWindow(minimized?SW_SHOWMINNOACTIVE:SW_SHOW);if(!minimized){p->BringWindowToTop();p->SetForegroundWindow();}progress.push_back(std::move(p));}
 void complete(JobPtr job,bool requested=false){
  if(!requested){
  auto action=manager.takeDownloadCompletion(job);if(!action.empty())fileActions.push_back({job,action});
  Json record,prefs;{Lock lock(manager.mutex);record=job->data;prefs=manager.state["Settings"];}playDownloadSound(prefs,"Sound","CompletionSoundFile");
  for(auto& p:progress)if(p->GetSafeHwnd()&&p->downloadId()==job->id()&&yes(record,"CloseProgressOnCompletion",true))p->requestCompletionClose();
  if(yes(record,"OpenFolderOnCompletion"))try{openFile(this,job->target().parent_path());}catch(const std::exception& e){error(this,e);}
  if(yes(prefs,"SuppressCompletionDialog")||yes(record,"SuppressCompletionDialog"))return;
  {Lock lock(manager.mutex);for(auto q:manager.state["Queues"])if(str(q,"Name")==str(record,"Queue")&&(!queueCompletionSteps(q).empty()||num(q,"RepeatMinutes")>0))return;}
  }
  Json record;{Lock lock(manager.mutex);record=job->snapshot();}
  bool checkedFile=record.contains("ScanResult");auto dialog=std::make_unique<Form>("Download complete",303,checkedFile?165:132,this);auto d=dialog.get();d->modeless=true;d->dialogUnits=true;
  d->init=[this,d,job,record,checkedFile]{
   d->ModifyStyle(0,WS_MINIMIZEBOX);const int extra=checkedFile?33:0;
   auto icon=d->make<FileDragIcon>(0,7,7,21,20);icon->path=job->target();
   const auto seconds=real(record,"TransferSeconds");const auto speed=seconds>0?num(record,"TransferredBytes")/seconds:0;
   d->label("Downloaded "+std::to_string(num(record,"Size"))+" bytes ("+bytes(num(record,"Size"))+").\r\nAverage transfer rate: "+progressBytes(speed)+"/sec",34,7,262,32);
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
 void refresh(){if(refreshing)return;refreshing=true;try{buildTree();std::set<std::string> selection;for(auto j:selected())selection.insert(j->id());std::string focused;int focus=table.GetNextItem(-1,LVNI_FOCUSED);if(focus>=0&&focus<(int)visible.size())focused=visible[focus]->id();std::vector<JobPtr> next;Json prefs;int active=0;double speed=0;{Lock l(manager.mutex);prefs=manager.state["Settings"];auto searchText=lower(text(&search));for(auto j:manager.jobs){auto& d=j->data;auto category=str(d,"Category"),state=str(d,"Status");auto previous=observedStates.find(j->id());if(previous!=observedStates.end()&&previous->second!=state){if(state=="Failed")playDownloadSound(prefs,"FailureSoundEnabled","FailureSoundFile");if(state=="Paused"&&(previous->second=="Downloading"||previous->second=="Pausing"))playDownloadSound(prefs,"PauseSoundEnabled","PauseSoundFile");}observedStates[j->id()]=state;active+=manager.isActive(j)?1:0;speed+=j->speed;bool match=filter=="all"||(filter=="unfinished"&&state!="Complete")||(filter=="finished"&&state=="Complete")||(filter=="grabber"&&!str(d,"ProjectId").empty())||(filter.rfind("category:",0)==0&&category==filter.substr(9))||(filter.rfind("unfinished:",0)==0&&category==filter.substr(11)&&state!="Complete")||(filter.rfind("finished:",0)==0&&category==filter.substr(9)&&state=="Complete")||(filter.rfind("queue:",0)==0&&downloadQueueMember(d)&&str(d,"Queue")==filter.substr(6))||(filter.rfind("project:",0)==0&&str(d,"ProjectId")==filter.substr(8));if(match&&(searchText.empty()||lower(str(d,"FileName")+" "+str(d,"Url")+" "+str(d,"Description")+" "+recoveryPage(d)).find(searchText)!=std::string::npos))next.push_back(j);}if(sortColumn>=0)std::stable_sort(next.begin(),next.end(),[&](JobPtr a,JobPtr b){
 auto compare=[](auto x,auto y){return x<y?-1:x>y?1:0;};int cmp=0;
 switch(sortColumn){
  case 1:cmp=lower(queueColumnText(a->data)).compare(lower(queueColumnText(b->data)));break;
  case 2:cmp=compare(num(a->data,"Size",-1),num(b->data,"Size",-1));break;
  case 3:{auto status=[](JobPtr j){auto state=str(j->data,"Status");if(state=="Downloading"&&num(j->data,"Size")>0)return std::string("%");return state;};cmp=status(a).compare(status(b));if(!cmp&&status(a)=="%")cmp=compare((double)num(a->data,"Received")/num(a->data,"Size"),(double)num(b->data,"Received")/num(b->data,"Size"));break;}
  case 4:{auto eta=[](JobPtr j){auto remaining=num(j->data,"Size")-num(j->data,"Received");return j->speed>0&&remaining>0?remaining/j->speed:-1.0;};cmp=compare(eta(a),eta(b));break;}
  case 5:cmp=compare(a->speed,b->speed);break;
  case 6:cmp=compare(parseDate(a->data.value("LastAttempt",Json())),parseDate(b->data.value("LastAttempt",Json())));break;
  case 8:cmp=compare(parseDate(a->data.value("Added",Json())),parseDate(b->data.value("Added",Json())));break;
  case 9:cmp=lower(utf8(a->target().wstring())).compare(lower(utf8(b->target().wstring())));break;
  case 10:{auto ah=readHeaders(a->data),bh=readHeaders(b->data);cmp=lower(ah["Referer"]).compare(lower(bh["Referer"]));break;}
  case 11:cmp=lower(recoveryPage(a->data)).compare(lower(recoveryPage(b->data)));break;
  case 7:cmp=lower(str(a->data,"Description")).compare(lower(str(b->data,"Description")));break;
  default:cmp=lower(str(a->data,"FileName")).compare(lower(str(b->data,"FileName")));break;
 }
 return ascending?cmp<0:cmp>0;
});bool rebuild=next!=visible;table.SetRedraw(FALSE);if(rebuild){table.DeleteAllItems();visible=next;}for(int i=0;i<(int)visible.size();++i){auto j=visible[i];auto& d=j->data;auto state=str(d,"Status"),file=str(d,"FileName");if(rebuild){table.InsertItem(i,cs(file),iconIndex.count(lower(str(d,"Category")))?iconIndex[lower(str(d,"Category"))]:0);if(selection.count(j->id()))table.SetItemState(i,LVIS_SELECTED,LVIS_SELECTED);if(focused==j->id())table.SetItemState(i,LVIS_FOCUSED,LVIS_FOCUSED);}else{if(textValue(i,0)!=file)table.SetItemText(i,0,cs(file));LVITEMW item{};item.mask=LVIF_IMAGE;item.iItem=i;const auto category=lower(str(d,"Category"));const int expected=iconIndex.count(category)?iconIndex.at(category):0;if(table.GetItem(&item)&&item.iImage!=expected){item.iImage=expected;table.SetItem(&item);}}auto total=num(d,"Size",-1);auto rowProgress=downloadPermille(d);auto eta=downloadSecondsLeft(d,j->speed);LVITEMW queueItem{};queueItem.mask=LVIF_IMAGE;queueItem.iItem=i;queueItem.iSubItem=1;queueItem.iImage=queueImageFor(d);table.SetItem(&queueItem);std::string values[]={"",bytes(total),yes(manager.quotaStatus(j),"Waiting")?"Waiting for quota":state=="Downloading"&&rowProgress>=0?std::to_string(rowProgress/10)+"%":state,eta>=0?std::to_string(eta)+" sec":"--",j->speed>0?bytes(j->speed)+"/s":"--",str(d,"LastAttempt").empty()?"--":dateText(d.value("LastAttempt",Json())),str(d,"Description"),dateText(d.value("Added",Json())),utf8(j->target().wstring()),(table.GetColumnWidth(10)>0?readHeaders(d)["Referer"]:""),(table.GetColumnWidth(11)>0?recoveryPage(d):"")};for(int c=0;c<11;++c)if(textValue(i,c+1)!=values[c])table.SetItemText(i,c+1,cs(values[c]));}table.SetRedraw(TRUE);table.Invalidate(FALSE);status.SetText(cs(manager.storageError.empty()?std::to_string(manager.jobs.size())+" downloads    "+std::to_string(active)+" active    "+bytes(speed)+"/s    Quota ("+std::to_string(num(prefs,"QuotaHours",1))+" h): "+bytes(num(manager.state,"QuotaBytes")):manager.storageError),0,0);}
 for(int i=0;i<ToolbarCommandCount;++i)toolbar.EnableButton(toolbarCommand(i),commandEnabled(i==3?CMD_STOP_CLOSE_ALL:toolbarCommand(i)));if(yes(prefs,"ClipboardMonitor")){auto value=clipboardText();if(value!=lastClipboard){lastClipboard=value;if(clipboardCandidate(value,prefs)){Url u(value);copiedUrl=value;if(str(prefs,"ClipboardMode")=="Open Download File Info"){clipboard.ShowWindow(SW_HIDE);if(!clipboardPromptPending){clipboardPromptPending=true;PostMessage(WM_COMMAND,CMD_CLIPBOARD_CAPTURE);}}else{clipboard.SetWindowText(cs("Download copied URL: "+u.host+u.path));clipboard.ShowWindow(SW_SHOW);}}else{copiedUrl.clear();clipboard.ShowWindow(SW_HIDE);}}}else clipboard.ShowWindow(SW_HIDE);
 std::vector<std::pair<JobPtr,bool>> pending;{std::lock_guard<std::mutex> l(eventsMutex);pending.swap(events);}for(auto& [j,done]:pending){if(done)complete(j);else if(!yes(j->data,"ConfirmationPending")&&!yes(prefs,"SuppressProgressDialog")&&!yes(j->data,"SuppressProgressDialog"))showProgress(j,false);}std::vector<JobPtr> confirmations;Json capturePresentations=Json::array();
 if(IsWindowEnabled()){
  Lock l(manager.mutex);capturePresentations=manager.pendingBrowserPresentations();std::set<std::string> capturedIds;for(const auto& offer:capturePresentations)capturedIds.insert(str(offer,"id"));
  std::vector<JobPtr> legacy;legacy.swap(manager.pendingOffers);
  for(auto j:legacy)if(!capturedIds.count(j->id())&&std::find(confirmations.begin(),confirmations.end(),j)==confirmations.end())confirmations.push_back(j);
  for(auto j:manager.jobs)if((str(j->data,"Status")=="Awaiting confirmation"||str(j->data,"Status")=="Awaiting duplicate choice")&&!capturedIds.count(j->id())&&infoSeen.insert(j->id()).second&&std::find(confirmations.begin(),confirmations.end(),j)==confirmations.end())confirmations.push_back(j);
 }if(IsWindowEnabled()&&!yes(prefs,"SuppressAuthenticationDialog")){JobPtr authentication;{Lock lock(manager.mutex);for(auto candidate:manager.jobs)if(!manager.isActive(candidate)&&authenticationDialogReady(candidate->data)){candidate->data["AuthenticationPromptPending"]=false;authentication=candidate;manager.save();break;}}if(authentication){ShowWindow(SW_SHOW);requestDownloadLogin(this,manager,authentication);}}
 if(IsWindowEnabled()){
  if(manager.browserRecoveryRequested.exchange(false))captureReviewsSeen.clear();
  for(const auto& token:manager.pendingBrowserCaptureReviews())if(IsWindowEnabled()&&captureReviewsSeen.insert(token.get<std::string>()).second){ShowWindow(SW_SHOW);CaptureReviewDialog dialog(manager,token.get<std::string>(),this);if(dialog.DoModal()==-1)throw std::runtime_error("Cannot open captured download review.");}
 }
 for(const auto& offer:capturePresentations)if(IsWindowEnabled())presentBrowserCapture(this,manager,offer);
 for(auto j:confirmations){bool exists;{Lock lock(manager.mutex);exists=std::find(manager.jobs.begin(),manager.jobs.end(),j)!=manager.jobs.end();}if(exists){ShowWindow(SW_SHOW);presentDownload(this,manager,j,true);}} }catch(const std::exception& e){status.SetText(cs(e.what()),0,0);}refreshing=false;}
 std::string textValue(int row,int column){return utf8((LPCWSTR)table.GetItemText(row,column));}
 static std::string dateText(const Json& d){auto ms=parseDate(d);if(!ms)return "--";auto t=toLocal(ms);char text[40];sprintf_s(text,"%04u-%02u-%02u %02u:%02u",t.wYear,t.wMonth,t.wDay,t.wHour,t.wMinute);return text;}
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
  dialog.init=[&]{dialog.label("Do you know...",55,29,168,12);auto body=dialog.label(tips[index],28,57,196,66);
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
 void command(UINT id){try{if(runQueueMenuCommand(id)){refresh();return;}if(auto column=arrangeColumn(id);column>=0){sortColumn=column;ascending=true;rememberLayout();refresh();return;}if(toolbarMenuAction(id))return;if(!commandEnabled(id))return;auto chosen=selected();switch(id){
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
 case CMD_LIMIT:{Form d("Global speed limiter",405,148,this);auto p=preferences();d.init=[&]{auto enabled=d.check("Use global speed limiter",num(p,"LimitKbps")>0,13,14,378);d.label("Maximum KB/s",13,57,140);auto value=d.edit(std::to_string(rememberedGlobalLimit(p)),160,53,229);d.label(str(p,"GlobalLimitMode")=="Each download"?"This limit applies separately to each download.":"This limit is shared by all active downloads.",13,87,378);d.accept=[&,enabled,value]{auto next=preferences();auto raw=trim(text(value));if(raw.empty()||raw.size()>7||raw.find_first_not_of("0123456789")!=std::string::npos)throw std::runtime_error("Enter a speed limit from 1 to 1,000,000 KB/s.");manager.setSettings(globalLimiterSettings(next,d.checked(enabled),std::stoll(raw)));d.close();};d.button("OK",201,111,88,d.accept);d.button("Cancel",301,111,88,[&]{d.close(IDCANCEL);});};d.DoModal();break;}
 case CMD_SELECTALL:table.SetItemState(-1,LVIS_SELECTED,LVIS_SELECTED);break;
 case CMD_DARK:preference("DarkMode",!yes(preferences(),"DarkMode"));applyAppearance();break;
 case CMD_HOME:openProductLink(m_hWnd,productHome);break;case CMD_SUPPORT:openProductLink(m_hWnd,productSupport);break;case CMD_SHARE:showShareDialog(this);break;case CMD_CHECK_UPDATES:showUpdateDialog(this,preferences());break;case CMD_TIP_DAY:showTipOfDay();break;case CMD_HELP_CONTENTS:case CMD_HELP_TUTORIALS:case CMD_HELP_SCHEDULER:case CMD_HELP_GRABBER:showHelpTopic(id-CMD_HELP_CONTENTS);break;case CMD_LANGUAGE_ENGLISH:break;case CMD_FONT:chooseFont();break;
 case CMD_FONT_RESET:{auto p=preferences();auto defaults=defaultSettings();for(auto key:{"FontName","FontHeight","FontWeight"})p[key]=defaults[key];manager.setSettings(p);applyAppearance();break;}
 case CMD_TRAY_COLOR:preference("TrayIcon","Color");updateTray();break;
 case CMD_TRAY_SYSTEM:preference("TrayIcon","Classic");updateTray();break;
 case CMD_TRAY_HIDE:preference("TrayIcon","Hidden");updateTray();break;
 case CMD_FINDNEXT:findNext();break;
 case CMD_HIDE_CATEGORIES:preference("HideCategories",!yes(preferences(),"HideCategories"));applyAppearance();if(yes(preferences(),"HideCategories"))table.SetFocus();break;
 case CMD_REMOVEQUEUE:setQueueMembershipBatch(manager,chosen,false);break;
 case CMD_ADD:addAddress(this,manager);break;case CMD_PASTE:{auto value=copiedUrl.empty()?clipboardText():copiedUrl;clipboard.ShowWindow(SW_HIDE);copiedUrl.clear();if(value.find('\n')!=std::string::npos)batchDialog(this,manager,value);else addAddress(this,manager,value);break;}case CMD_BATCH:batchDialog(this,manager);break;case CMD_BATCH_CLIPBOARD:selectBatch(this,manager,importTextUrls(clipboardText()));break;case CMD_RESUME:for(auto j:chosen){if(!commandCanResume(j))continue;if(!str(j->data,"DuplicateOf").empty())presentDownload(this,manager,j);else manager.resume(j);}break;case CMD_STOP:{std::set<std::string> stopped;for(auto j:chosen)if(commandCanStop(j)){manager.pause(j);stopped.insert(j->id());}closeStoppedProgress(stopped);break;}case CMD_STOPALL:case CMD_STOP_CLOSE_ALL:{std::vector<JobPtr> jobs;std::set<std::string> stopped;{Lock l(manager.mutex);jobs=manager.jobs;for(auto j:jobs)if(str(j->data,"Status")!="Complete"||manager.isActive(j))stopped.insert(j->id());}for(auto j:jobs)if(commandCanStop(j))manager.pause(j);if(id==CMD_STOP_CLOSE_ALL)closeStoppedProgress(stopped);break;}case CMD_DELETE:removeDownloadSelection(this,manager,chosen);break;case CMD_CLEAN:{std::vector<JobPtr> completed;{Lock l(manager.mutex);for(auto j:manager.jobs)if(str(j->data,"Status")=="Complete")completed.push_back(j);}removeDownloadSelection(this,manager,completed);}break;case CMD_OPTIONS:{Options d(manager,this);d.DoModal();applyAppearance();break;}case CMD_SCHEDULER:{Scheduler d(manager,this,selectedQueue());d.DoModal();break;}case CMD_STARTQUEUE:manager.queueRun(selectedQueue(),true);break;case CMD_STOPQUEUE:manager.queueRun(selectedQueue(),false);break;case CMD_GRABBER:{GrabberDialog d(manager,this);d.DoModal();break;}case CMD_PROPERTIES:if(!chosen.empty()){if(str(chosen[0]->data,"Status")=="Complete")completedProperties(this,manager,chosen[0]);else if(!str(chosen[0]->data,"DuplicateOf").empty())presentDownload(this,manager,chosen[0]);else downloadInfo(this,manager,chosen[0],true);}break;case CMD_PROGRESS:if(!chosen.empty()){if(!str(chosen[0]->data,"DuplicateOf").empty())presentDownload(this,manager,chosen[0]);else showProgress(chosen[0]);}break;case CMD_REFRESH_ADDRESS:if(!chosen.empty())refreshDownloadAddress(this,manager,chosen[0]);break;case CMD_OPEN:if(!chosen.empty()&&str(chosen[0]->data,"Status")=="Complete")openDownloadedFile(this,chosen[0]->target());break;case CMD_FOLDER:if(!chosen.empty())openFile(this,chosen[0]->target().parent_path());break;case CMD_NETWORK:{NetworkDialog d(this);d.DoModal();break;}case CMD_BROWSER:{auto p=preferences();if(browserIntegration(this,p))manager.setSettings(p);break;}case CMD_ABOUT:showAboutDialog(this,preferences());break;case CMD_IMPORT_IDM:importDownloads(1);break;case CMD_IMPORT_TEXT:importDownloads(2);break;case CMD_EXPORT_IDM:exportDownloads(chosen,2);break;case CMD_EXPORT_TEXT:exportDownloads(chosen,0);break;case CMD_IMPORT:importDownloads();break;case CMD_EXPORT:exportDownloads(chosen);break;case CMD_RECOVERY:{{Lock lock(manager.mutex);for(auto job:manager.jobs)if(manager.isActive(job))throw std::runtime_error("Pause active downloads before opening backup and recovery.");}recoveryOnClose=true;quitting=true;OnClose();return;}case CMD_EXIT:quitting=true;OnClose();return;case CMD_NEWQUEUE:{auto name=prompt(this,"Create queue");if(!name.empty()){auto q=defaultQueue(name);q["Enabled"]=false;manager.setQueue(q);Scheduler d(manager,this,name);d.DoModal();}break;}case CMD_NEWCATEGORY:categoryEditor();break;case CMD_UP:if(!chosen.empty())manager.move(chosen[0],-1);break;case CMD_DOWN:if(!chosen.empty())manager.move(chosen[0],1);break;case CMD_MOVEQUEUE:if(!chosen.empty()){auto choices=membershipQueues(chosen);if(choices.empty())throw std::runtime_error("Create a synchronization queue in Scheduler for completed files.");auto current=str(chosen[0]->data,"Queue");if(std::find(choices.begin(),choices.end(),current)==choices.end())current=choices.front();const bool adding=std::none_of(chosen.begin(),chosen.end(),[](const JobPtr& job){return downloadQueueMember(job->data);});Form d(adding?"Add to queue":"Move to queue",349,103,this);d.init=[&]{auto queue=d.combo(choices,current,13,14,322);d.button(adding?"Add":"Move",163,64,82,[&,queue]{setQueueMembershipBatch(manager,chosen,true,text(queue));d.close();});d.button("Cancel",257,64,79,[&]{d.close(IDCANCEL);});};d.DoModal();}break;case CMD_SEARCH:showFind();break;case CMD_QUICKFILTER:search.ShowWindow(search.IsWindowVisible()?SW_HIDE:SW_SHOW);if(search.IsWindowVisible())search.SetFocus();else search.SetWindowText(L"");{CRect r;GetClientRect(&r);layout(r.Width(),r.Height());}break;}refresh();}catch(const std::exception& e){error(this,e);}}
 afx_msg int OnCreate(LPCREATESTRUCT c){if(CFrameWnd::OnCreate(c)==-1)return -1;scale=GetDpiForWindow(m_hWnd)/96.0f;font.CreateFontW(-px(11),0,0,0,FW_NORMAL,FALSE,FALSE,FALSE,DEFAULT_CHARSET,OUT_DEFAULT_PRECIS,CLIP_DEFAULT_PRECIS,DEFAULT_QUALITY,DEFAULT_PITCH,L"Tahoma");menu.CreateMenu();addMenu(L"&Tasks",{{CMD_ADD,L"&Add new download"},{CMD_BATCH,L"Add &batch download"},{CMD_BATCH_CLIPBOARD,L"Add batch download from &clipboard"},{CMD_GRABBER,L"Run site &grabber"},{0,L""},{CMD_BASKET,L"Show &drop target"},{0,L""},{CMD_EXPORT,L"E&xport"},{CMD_IMPORT,L"&Import"},{0,L""},{CMD_EXIT,L"&Exit"}});auto tasks=menu.GetSubMenu(0);CMenu exports;exports.CreatePopupMenu();exports.AppendMenuW(MF_STRING,CMD_EXPORT_IDM,L"To IDM export &file");exports.AppendMenuW(MF_STRING,CMD_EXPORT_TEXT,L"To &text file");exports.AppendMenuW(MF_STRING,CMD_EXPORT,L"To UDM &catalog...");exports.AppendMenuW(MF_SEPARATOR);exports.AppendMenuW(MF_STRING,CMD_RECOVERY,L"Backup and &recovery...");tasks->ModifyMenuW(CMD_EXPORT,MF_BYCOMMAND|MF_POPUP,(UINT_PTR)exports.Detach(),L"E&xport");CMenu imports;imports.CreatePopupMenu();imports.AppendMenuW(MF_STRING,CMD_IMPORT_IDM,L"&From IDM export file");imports.AppendMenuW(MF_STRING,CMD_IMPORT_TEXT,L"From &text file");imports.AppendMenuW(MF_STRING,CMD_IMPORT,L"From UDM &catalog...");tasks->ModifyMenuW(CMD_IMPORT,MF_BYCOMMAND|MF_POPUP,(UINT_PTR)imports.Detach(),L"&Import");addMenu(L"&File",{{CMD_STOP,L"&Stop Download"},{CMD_DELETE,L"&Remove"},{CMD_RESUME,L"&Download Now"},{CMD_REDOWNLOAD,L"R&edownload"}});addMenu(L"&Downloads",{{CMD_STOPALL,L"&Pause All"},{CMD_STOP_CLOSE_ALL,L"&Stop All"},{0,L""},{CMD_CLEAN,L"&Delete All Completed"},{0,L""},{CMD_SEARCH,L"Find (Ctrl-F)"},{CMD_FINDNEXT,L"Find Next (F3)"},{0,L""},{CMD_SCHEDULER,L"S&cheduler"},{CMD_STARTQUEUE,L"Start &queue"},{CMD_STOPQUEUE,L"S&top queue"},{0,L""},{CMD_LIMIT,L"Speed &Limiter"},{0,L""},{CMD_OPTIONS,L"&Options"}});CMenu startQueueMenu;startQueueMenu.CreatePopupMenu();startQueueMenu.AppendMenuW(MF_STRING,CMD_STARTQUEUE,L"Start");startQueueMenu.AppendMenuW(MF_STRING,CMD_STARTQUEUEMENU,L"Choose queue...");menu.GetSubMenu(2)->ModifyMenuW(CMD_STARTQUEUE,MF_BYCOMMAND|MF_POPUP,(UINT_PTR)startQueueMenu.Detach(),L"Start &queue");CMenu stopQueueMenu;stopQueueMenu.CreatePopupMenu();stopQueueMenu.AppendMenuW(MF_STRING,CMD_STOPQUEUE,L"Stop");stopQueueMenu.AppendMenuW(MF_STRING,CMD_STOPQUEUEMENU,L"Choose queue...");menu.GetSubMenu(2)->ModifyMenuW(CMD_STOPQUEUE,MF_BYCOMMAND|MF_POPUP,(UINT_PTR)stopQueueMenu.Detach(),L"S&top queue");addMenu(L"&View",{{CMD_HIDE_CATEGORIES,L"Hide &categories"},{CMD_TOOLBAR,L"&Toolbar"},{CMD_TRAY_COLOR,L"&UDM tray icon"},{CMD_COLUMNS,L"C&ustomize URL List..."},{CMD_DARK,L"Dark Mode support"},{CMD_FONT,L"Font"},{0,L""},{CMD_LANGUAGE_ENGLISH,L"&Language"}});CMenu trayChoices;trayChoices.CreatePopupMenu();trayChoices.AppendMenuW(MF_STRING,CMD_TRAY_COLOR,L"3&D style");trayChoices.AppendMenuW(MF_STRING,CMD_TRAY_SYSTEM,L"C&lassic style");trayChoices.AppendMenuW(MF_STRING,CMD_TRAY_HIDE,L"Do&n't show");menu.GetSubMenu(3)->ModifyMenuW(CMD_TRAY_COLOR,MF_BYCOMMAND|MF_POPUP,(UINT_PTR)trayChoices.Detach(),L"&UDM tray icon");CMenu languages;languages.CreatePopupMenu();languages.AppendMenuW(MF_STRING|MF_CHECKED,CMD_LANGUAGE_ENGLISH,L"&English");menu.GetSubMenu(3)->ModifyMenuW(CMD_LANGUAGE_ENGLISH,MF_BYCOMMAND|MF_POPUP,(UINT_PTR)languages.Detach(),L"&Language");addMenu(L"&Help",{{CMD_HELP_CONTENTS,L"H&elp contents"},{CMD_HELP_TUTORIALS,L"&Tutorials"},{CMD_HELP_SCHEDULER,L"&Scheduler and queues"},{CMD_HELP_GRABBER,L"&Grabber Help"},{CMD_TIP_DAY,L"Tip of the &Day..."},{0,L""},{CMD_HOME,L"UDM &Home Page"},{CMD_SUPPORT,L"&Contact UDM Support"},{0,L""},{CMD_CHECK_UPDATES,L"Check for &updates..."},{0,L""},{CMD_ABOUT,L"&About UDM"},{CMD_SHARE,L"Tell a &friend"}});CMenu limiter;limiter.CreatePopupMenu();limiter.AppendMenuW(MF_STRING,CMD_LIMIT_ON,L"Turn o&n");limiter.AppendMenuW(MF_STRING,CMD_LIMIT_OFF,L"Turn of&f");limiter.AppendMenuW(MF_SEPARATOR);limiter.AppendMenuW(MF_STRING,CMD_LIMIT,L"S&ettings...");
menu.GetSubMenu(2)->ModifyMenuW(CMD_LIMIT,MF_BYCOMMAND|MF_POPUP,(UINT_PTR)limiter.Detach(),L"Speed &Limiter");
CMenu fontMenu;fontMenu.CreatePopupMenu();fontMenu.AppendMenuW(MF_STRING,CMD_FONT,L"Choose Font...");fontMenu.AppendMenuW(MF_STRING,CMD_FONT_RESET,L"Reset to default Font");menu.GetSubMenu(3)->ModifyMenuW(CMD_FONT,MF_BYCOMMAND|MF_POPUP,(UINT_PTR)fontMenu.Detach(),L"Font");
CMenu arrange;arrange.CreatePopupMenu();
const wchar_t* arrangeLabels[]={L"By &Order Of Addition",L"By &File Name",L"By &Size",L"By St&atus",L"By &Time Left",L"By Transfer &Rate",L"By &Last Try Date",L"By &Description",L"By Save &Path",L"By R&eferer",L"By Parent &Web Page"};
for(UINT i=0;i<11;++i)arrange.AppendMenuW(MF_STRING,CMD_SORT_ADDED+i,arrangeLabels[i]);
menu.GetSubMenu(3)->InsertMenuW(1,MF_BYPOSITION|MF_POPUP,(UINT_PTR)arrange.Detach(),L"&Arrange files");
SetMenu(&menu);rebuildListImages();createToolbar();categoryHeading.Create(L"Categories",WS_CHILD|WS_VISIBLE|SS_LEFT|WS_BORDER,CRect(0,0,1,1),this);categoryHeading.SetFont(&font);categoryClose.Create(L"Close categories",WS_CHILD|WS_VISIBLE|WS_TABSTOP|BS_OWNERDRAW,CRect(0,0,1,1),this,CMD_HIDE_CATEGORIES);tree.Create(WS_CHILD|WS_VISIBLE|WS_TABSTOP|TVS_HASBUTTONS|TVS_HASLINES|TVS_LINESATROOT|TVS_SHOWSELALWAYS,CRect(0,0,1,1),this,501);tree.SetFont(&font);tree.SetImageList(&images,TVSIL_NORMAL);tree.SetItemHeight((SHORT)px(17));table.Create(WS_CHILD|WS_VISIBLE|WS_TABSTOP|LVS_REPORT|LVS_SHOWSELALWAYS,CRect(0,0,1,1),this,502);if(!SetWindowSubclass(table.GetHeaderCtrl()->GetSafeHwnd(),headerContextProc,1,(DWORD_PTR)m_hWnd))throw std::runtime_error("Cannot initialize column header menu.");table.SetFont(&font);table.SetImageList(&images,LVSIL_SMALL);table.SetExtendedStyle(LVS_EX_FULLROWSELECT|LVS_EX_GRIDLINES|LVS_EX_DOUBLEBUFFER|LVS_EX_HEADERDRAGDROP|LVS_EX_SUBITEMIMAGES|LVS_EX_INFOTIP);const wchar_t* columns[]={L"File Name",L"Q",L"Size",L"Status",L"Time left",L"Transfer rate",L"Last Try Date",L"Description"};int widths[]={205,38,72,97,77,91,132,160};for(int i=0;i<columnCount;++i)table.InsertColumn(i,i<8?columns[i]:columnName(i),LVCFMT_LEFT,i<8?px(widths[i]):0);restoreLayout();status.Create(WS_CHILD|WS_VISIBLE,CRect(0,0,1,1),this,503);status.SetFont(&font);search.Create(WS_CHILD|WS_TABSTOP|ES_AUTOHSCROLL|WS_BORDER,CRect(0,0,1,1),this,504);search.SetFont(&font);search.SendMessage(EM_SETCUEBANNER,TRUE,(LPARAM)L"Find in file name, address, description or source page");clipboard.Create(L"",WS_CHILD|WS_TABSTOP|BS_PUSHBUTTON,CRect(0,0,1,1),this,CMD_PASTE);clipboard.SetFont(&font);tray.cbSize=sizeof(tray);tray.hWnd=m_hWnd;tray.uID=1;tray.uFlags=NIF_MESSAGE|NIF_ICON|NIF_TIP;tray.uCallbackMessage=TRAY_MESSAGE;tray.hIcon=AfxGetApp()->LoadIcon(1);wcscpy_s(tray.szTip,L"UDM Download Manager");Shell_NotifyIconW(NIM_ADD,&tray);drop.Register(this);tableDrop.attach(&table);treeDrop.attach(&tree);
 tableDrop.apply=[this](const std::string& id,CPoint point){JobPtr job;{Lock lock(manager.mutex);for(auto j:manager.jobs)if(j->id()==id)job=j;}int row=table.HitTest(point);JobPtr before=row>=0&&row<(int)visible.size()?visible[row]:JobPtr();auto queue=before?str(before->data,"Queue"):selectedQueue();manager.reorder(job,queue,before);sortColumn=-1;refresh();};
 treeDrop.apply=[this](const std::string& id,CPoint point){auto item=tree.HitTest(point);if(!item)return;auto index=tree.GetItemData(item);if(index>=filters.size()||filters[index].rfind("queue:",0)!=0)throw std::runtime_error("Drop an unfinished download onto a queue.");JobPtr job;{Lock lock(manager.mutex);for(auto j:manager.jobs)if(j->id()==id)job=j;}manager.reorder(job,filters[index].substr(6));refresh();};
 SetTimer(1,500,nullptr);manager.showCompletedDownload=[this](JobPtr job){complete(job,true);};manager.event=[this](JobPtr j,bool complete){std::lock_guard<std::mutex> l(eventsMutex);events.push_back({j,complete});};buildTree();applyAppearance();showBasket();return 0;}
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
  }catch(const std::exception& e){status.SetText(cs(e.what()),0,0);}
  SetRedraw(TRUE);RedrawWindow(nullptr,nullptr,RDW_INVALIDATE|RDW_ALLCHILDREN|RDW_ERASE);return 0;
 }
 afx_msg void OnSize(UINT type,int w,int h){CFrameWnd::OnSize(type,w,h);layout(w,h);}
  afx_msg void OnTimer(UINT_PTR id){if(IsWindowEnabled())try{while(takeCliHangup(manager))performSystemAction("Disconnect dial-up / VPN");}catch(const std::exception& e){error(this,e);return;}if(quitAfterDownload&&IsWindowEnabled()){Lock lock(manager.mutex);auto job=quitAfterDownload;if(std::find(manager.jobs.begin(),manager.jobs.end(),job)==manager.jobs.end())quitAfterDownload.reset();else if(cliCompletionReady(manager,job)){quitAfterDownload.reset();PostMessage(WM_COMMAND,CMD_EXIT);return;}}try{manager.tick();showQuotaWarning();}catch(const std::exception& e){status.SetText(cs(e.what()),0,0);}if(IsWindowEnabled()&&manager.browserSettingsRequested.exchange(false))PostMessage(WM_COMMAND,CMD_BROWSER);completions.erase(std::remove_if(completions.begin(),completions.end(),[](const auto& d){return !d->GetSafeHwnd();}),completions.end());rememberLayout();refresh();if(!refreshing)try{fileCompletionActions();queueCompletionActions();}catch(const std::exception& e){error(this,e);}CFrameWnd::OnTimer(id);}
 afx_msg void OnGetMinMaxInfo(MINMAXINFO* info){info->ptMinTrackSize.x=px(596);info->ptMinTrackSize.y=px(340);CFrameWnd::OnGetMinMaxInfo(info);}
 afx_msg void OnClose(){if(!quitting){Lock l(manager.mutex);if(yes(manager.state["Settings"],"CloseToTray")){ShowWindow(SW_HIDE);return;}}quitting=true;rememberLayout();KillTimer(1);if(stopIntegration)stopIntegration();status.SetText(L"Saving downloads and closing...",0,0);EnableWindow(FALSE);try{manager.stop();}catch(const std::exception& e){EnableWindow(TRUE);quitting=false;recoveryOnClose=false;SetTimer(1,500,nullptr);try{if(restartIntegration)restartIntegration();}catch(const std::exception& bridge){error(this,bridge);}error(this,e);return;}if(recoveryOnClose){try{launchRecoveryMode(manager.root,true);}catch(const std::exception& e){error(this,e);}}manager.event={};manager.showCompletedDownload={};if(quotaWarning&&quotaWarning->GetSafeHwnd())quotaWarning->DestroyWindow();quotaWarning.reset();for(auto& d:completions)if(d->GetSafeHwnd())d->DestroyWindow();completions.clear();basket.reset();tableDrop.Revoke();treeDrop.Revoke();drop.Revoke();for(auto& p:progress)if(p->GetSafeHwnd()){p->DestroyWindow();}Shell_NotifyIconW(NIM_DELETE,&tray);CFrameWnd::OnClose();}
 afx_msg LRESULT OnShow(WPARAM,LPARAM){ShowWindow(SW_RESTORE);SetForegroundWindow();return 0;}
 afx_msg LRESULT OnDropUrl(WPARAM,LPARAM l){std::unique_ptr<std::string> s((std::string*)l);try{if(s->find('\n')!=std::string::npos)batchDialog(this,manager,*s);else addAddress(this,manager,*s);}catch(const std::exception& e){error(this,e);}return 0;}
 LRESULT WindowProc(UINT message,WPARAM w,LPARAM l)override{
  static const UINT recreated=RegisterWindowMessageW(L"TaskbarCreated");
  if(recreated&&message==recreated&&!quitting){
   try{if(!updateTray())ShowWindow(SW_RESTORE);}catch(...){ShowWindow(SW_RESTORE);}
  }
  return CFrameWnd::WindowProc(message,w,l);
 }
 afx_msg LRESULT OnTray(WPARAM,LPARAM l){if(l==WM_LBUTTONDBLCLK||l==NIN_KEYSELECT||l==NIN_SELECT)OnShow(0,0);else if(l==WM_RBUTTONUP){CMenu popup;popup.CreatePopupMenu();popup.AppendMenuW(MF_STRING,CMD_ADD,L"Add URL");popup.AppendMenuW(MF_STRING,CMD_RESUME,L"Resume selected");popup.AppendMenuW(MF_STRING,CMD_STOPALL,L"Pause all downloads");popup.AppendMenuW(MF_STRING,CMD_STOP_CLOSE_ALL,L"Stop all downloads");popup.AppendMenuW(MF_STRING,CMD_STARTQUEUEMENU,L"Start queue...");popup.AppendMenuW(MF_STRING,CMD_STOPQUEUEMENU,L"Stop queue...");popup.AppendMenuW(MF_STRING,CMD_SCHEDULER,L"Scheduler");popup.AppendMenuW(MF_STRING,CMD_LIMIT,L"Speed limiter...");popup.AppendMenuW(MF_STRING,CMD_OPTIONS,L"Options");popup.AppendMenuW(MF_STRING|(yes(preferences(),"DropBasket")?MF_CHECKED:0),CMD_BASKET,L"Drop basket");popup.AppendMenuW(MF_SEPARATOR);popup.AppendMenuW(MF_STRING,CMD_EXIT,L"Exit UDM");CPoint p;GetCursorPos(&p);SetForegroundWindow();popup.TrackPopupMenu(TPM_RIGHTBUTTON,p.x,p.y,this);}return 0;}
 // MFC queries update routing before showing menus; OnCommand alone does not
 // advertise handlers and otherwise leaves every custom menu command disabled.
 afx_msg void OnUpdateAppCommand(CCmdUI* ui){ui->Enable(commandEnabled(ui->m_nID));if(auto column=arrangeColumn(ui->m_nID);column>=0)ui->SetRadio(sortColumn==column);auto p=preferences();if(ui->m_nID==CMD_LANGUAGE_ENGLISH)ui->SetRadio(TRUE);if(ui->m_nID==CMD_TRAY_COLOR)ui->SetRadio(str(p,"TrayIcon","Color")=="Color");if(ui->m_nID==CMD_TRAY_SYSTEM)ui->SetRadio(classicTrayStyle(str(p,"TrayIcon")));if(ui->m_nID==CMD_TRAY_HIDE)ui->SetRadio(str(p,"TrayIcon")=="Hidden");if(ui->m_nID==CMD_LIMIT_ON)ui->SetRadio(num(p,"LimitKbps")>0);if(ui->m_nID==CMD_LIMIT_OFF)ui->SetRadio(num(p,"LimitKbps")==0);if(ui->m_nID==CMD_BASKET)ui->SetCheck(yes(p,"DropBasket"));if(ui->m_nID==CMD_DARK)ui->SetCheck(yes(p,"DarkMode"));if(ui->m_nID==CMD_HIDE_CATEGORIES)ui->SetCheck(yes(p,"HideCategories"));}
 BOOL OnCommand(WPARAM w,LPARAM l)override{UINT id=LOWORD(w);if(id==CMD_STOPALL&&(HWND)l==toolbar.GetSafeHwnd())id=CMD_STOP_CLOSE_ALL;if((id>=CMD_ADD&&id<=CMD_SHARE)||(id>=31000&&id<31064)||isQueueMenuCommand(id)){command(id);return TRUE;}if(id==504&&HIWORD(w)==EN_CHANGE){refresh();return TRUE;}return CFrameWnd::OnCommand(w,l);}
 BOOL OnNotify(WPARAM w,LPARAM l,LRESULT* result)override{auto header=(NMHDR*)l;if(toolbarNotification(header,result))return TRUE;if(header->hwndFrom==tree.m_hWnd&&header->code==TVN_SELCHANGED){auto change=(NMTREEVIEW*)l;if(!change->itemNew.hItem){*result=0;return TRUE;}auto index=tree.GetItemData(change->itemNew.hItem);if(!treeRebuilding&&index<filters.size()){filter=filters[index];refresh();}*result=0;return TRUE;}if(header->hwndFrom==table.m_hWnd){if(header->code==LVN_GETINFOTIPW){auto tip=reinterpret_cast<NMLVGETINFOTIPW*>(header);if(tip->iItem>=0&&tip->iItem<(int)visible.size()&&tip->pszText&&tip->cchTextMax>0){auto value=wide(str(visible[tip->iItem]->data,"FileName")+"\n"+queueInfo(visible[tip->iItem]->data));wcsncpy_s(tip->pszText,tip->cchTextMax,value.c_str(),_TRUNCATE);}*result=0;return TRUE;}if(header->code==LVN_BEGINDRAG){auto chosen=selected();if(chosen.size()==1&&str(chosen[0]->data,"Status")!="Complete"&&!manager.isActive(chosen[0])){draggedJob=chosen[0];SetCapture();::SetCursor(LoadCursor(nullptr,IDC_SIZEALL));*result=0;return TRUE;}std::vector<fs::path> paths;{Lock lock(manager.mutex);for(auto job:selected())if(str(job->data,"Status")=="Complete")paths.push_back(job->target());}try{dragSavedFiles(paths);}catch(const std::exception& e){error(this,e);}*result=0;return TRUE;}if(header->code==NM_DBLCLK){auto item=reinterpret_cast<NMITEMACTIVATE*>(header);if(item->iItem>=0&&item->iItem<(int)visible.size()){table.SetItemState(-1,0,LVIS_SELECTED);table.SetItemState(item->iItem,LVIS_SELECTED|LVIS_FOCUSED,LVIS_SELECTED|LVIS_FOCUSED);auto chosen=selected();if(!chosen.empty())command(str(chosen[0]->data,"Status")=="Complete"?(str(preferences(),"CompletedDoubleClick","Properties")=="Open"?CMD_OPEN:CMD_PROPERTIES):CMD_PROGRESS);}*result=0;return TRUE;}if(header->code==LVN_COLUMNCLICK){auto change=(NMLISTVIEW*)l;if(sortColumn==change->iSubItem)ascending=!ascending;else{sortColumn=change->iSubItem;ascending=true;}refresh();*result=0;return TRUE;}}return CFrameWnd::OnNotify(w,l,result);}
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
  if(msg->wParam==VK_TAB&&GetKeyState(VK_CONTROL)>=0&&GetKeyState(VK_MENU)>=0&&::IsChild(m_hWnd,msg->hwnd)&&::IsDialogMessageW(m_hWnd,msg))return TRUE;
if(msg->wParam==VK_F1){command(CMD_HELP_CONTENTS);return TRUE;}if(msg->wParam==VK_ESCAPE&&draggedJob){draggedJob.reset();ReleaseCapture();return TRUE;}if(GetKeyState(VK_CONTROL)<0){if(msg->wParam=='N'){command(CMD_ADD);return TRUE;}if(msg->wParam=='V'&&GetFocus()!=&search){command(CMD_PASTE);return TRUE;}if(msg->wParam=='A'&&GetFocus()==&table){command(CMD_SELECTALL);return TRUE;}if(msg->wParam=='F'){command(GetKeyState(VK_SHIFT)<0?CMD_QUICKFILTER:CMD_SEARCH);return TRUE;}if(msg->wParam=='M'){command(CMD_RELOCATE);return TRUE;}}if(msg->wParam==VK_F3){command(CMD_FINDNEXT);return TRUE;}if(msg->wParam==VK_DELETE&&GetFocus()==&table){command(CMD_DELETE);return TRUE;}}return CFrameWnd::PreTranslateMessage(msg);}
public:JobPtr quitAfterDownload;std::function<void()> stopIntegration,restartIntegration; explicit MainWindow(Manager& m):manager(m){auto cls=AfxRegisterWndClass(CS_DBLCLKS,LoadCursor(nullptr,IDC_ARROW),(HBRUSH)(COLOR_BTNFACE+1),AfxGetApp()->LoadIcon(1));if(!Create(cls,L"UDM Download Manager",WS_OVERLAPPEDWINDOW,CRect(160,150,922,610)))throw std::runtime_error("Cannot create UDM window.");SetWindowPos(nullptr,0,0,px(778),px(470),SWP_NOMOVE|SWP_NOZORDER);CenterWindow();}
};
BEGIN_MESSAGE_MAP(MainWindow,CFrameWnd)
 ON_UPDATE_COMMAND_UI_RANGE(CMD_ADD,CMD_SHARE,OnUpdateAppCommand)
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
 ON_MESSAGE(TRAY_MESSAGE,OnTray)
 ON_MESSAGE(DROP_URL,OnDropUrl)
END_MESSAGE_MAP()
#if !defined(UDM_TOOLBAR_COMPONENT_TEST) && !defined(UDM_CAPTURE_PRESENTATION_COMPONENT_TEST)
class Application:public CWinApp {
 std::unique_ptr<Manager> manager;std::unique_ptr<PipeServer> pipe;Handle mutex;std::unique_ptr<RecoveryDataLease> recoveryLease;bool recoveryClosed=false;
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
  manager=std::make_unique<Manager>(options.data);auto frame=new MainWindow(*manager);m_pMainWnd=frame;frame->stopIntegration=[this]{pipe.reset();};frame->restartIntegration=[this,frame]{if(!pipe)pipe=std::make_unique<PipeServer>(*manager,[frame]{frame->PostMessage(SHOW_APP);});};manager->startQueuesOnStartup();pipe=std::make_unique<PipeServer>(*manager,[frame]{frame->PostMessage(SHOW_APP);});
  if(!options.address.empty()){auto job=manager->receive(options.request());if(options.quitAfterDownload){Lock lock(manager->mutex);if(str(job->data,"Status")!="Complete")frame->quitAfterDownload=job;}}
  if(options.startQueue)manager->queueRun("Main queue",true);
  frame->ShowWindow(options.background?SW_HIDE:SW_SHOW);frame->UpdateWindow();if(!options.background&&!options.silent&&!options.startQueue&&!options.waitProcess&&options.address.empty()&&yes(manager->state["Settings"],"ShowTipsOnStartup",true))frame->PostMessage(WM_COMMAND,CMD_TIP_DAY);return TRUE;
 }catch(const std::exception& e){AfxMessageBox(cs(e.what()),MB_OK|MB_ICONERROR);return FALSE;}
}
 int ExitInstance()override{pipe.reset();manager.reset();recoveryLease.reset();AfxOleTerm(FALSE);auto result=CWinApp::ExitInstance();return recoveryClosed?0:result;}
};
Application application;
#endif
}
