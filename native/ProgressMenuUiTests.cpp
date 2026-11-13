// Isolated menu/persistence tests. Never opens files, Explorer or clipboard.
#include <afxwin.h>
#include <afxcmn.h>
#include <afxdlgs.h>
#include <afxole.h>
#include <atlimage.h>
#include "Ui.hpp"
namespace udm {
struct CatalogCheckpointTestAccess {
 static void observe(Manager& m,std::function<void(const char*)> fn){Lock lock(m.mutex);m.catalogCheckpoint=std::move(fn);}
 static std::shared_ptr<Cancel> active(Manager& m,const JobPtr& j,bool set){Lock lock(m.mutex);if(!set){m.active.erase(j->id());return {};}auto c=std::make_shared<Cancel>();m.active[j->id()]=c;return c;}
};
struct ProgressMenuTestAccess {static void start(Progress& p,ProgressMenuAction a){p.startMenuCommand(a);}static ProgressMenuState state(Progress& p){return p.displayedMenu;}static std::string limit(Progress& p){return text(p.limit);}static void choose(Progress& p,UINT id){p.dispatchMenuChoice(id,p.displayedMenu);}static bool pending(Progress& p){return p.commandTask.valid();}static void expireClose(Progress& p){p.closeAfter=std::chrono::steady_clock::now()-std::chrono::milliseconds(1);}};
static Json checks=Json::array();
static void check(bool v,const char* name){checks.push_back({{"name",name},{"passed",v}});if(!v)throw std::runtime_error(name);}
static std::wstring title(HWND w){wchar_t value[2048]{};GetWindowTextW(w,value,2048);return value;}
static HWND named(HWND w,const wchar_t* label){for(auto c=GetWindow(w,GW_CHILD);c;c=GetWindow(c,GW_HWNDNEXT))if(title(c)==label)return c;throw std::runtime_error("Missing owned menu control");}
static void settle(Manager& m){auto end=GetTickCount64()+6000;while(m.fileInfoDecisionPending&&GetTickCount64()<end){MSG msg;while(PeekMessageW(&msg,nullptr,0,0,PM_REMOVE)){TranslateMessage(&msg);DispatchMessageW(&msg);}Sleep(2);}if(m.fileInfoDecisionPending)throw std::runtime_error("Menu command did not finish");}
class Owner:public CFrameWnd {void PostNcDestroy()override{}};
static JobPtr seed(Manager& m,const fs::path& output){auto p=m.state["Settings"];p["DownloadFolder"]=utf8((output/L"files").wstring());p["ProgressStartMode"]="Normal size";m.setSettings(p);return m.add("ftp://127.0.0.1:9/menu.bin",utf8((output/L"files").wstring()),"menu.bin","Main queue",true);}
struct Gate {
 Manager& manager;bool fail;std::promise<void> entered,released;std::shared_future<void> ready=entered.get_future().share(),wait=released.get_future().share();std::atomic_bool unlocked{false};std::atomic_int saves{0};
 Gate(Manager& m,bool f):manager(m),fail(f){CatalogCheckpointTestAccess::observe(manager,[this](const char* stage){if(std::string(stage)=="temporary-flushed"&&saves.fetch_add(1)==0){entered.set_value();if(wait.wait_for(std::chrono::seconds(8))!=std::future_status::ready)throw std::runtime_error("Menu gate expired");if(fail)throw std::runtime_error("Forced menu command failure");}});}
 void unlock(){if(!unlocked.exchange(true))released.set_value();}~Gate(){unlock();CatalogCheckpointTestAccess::observe(manager,{});}
};
struct Warning {int count=0;bool matched=true;std::function<void()> beforeAck;};static Warning* warning=nullptr;
static void CALLBACK acknowledge(HWND,UINT,UINT_PTR,DWORD){if(!warning)return;HWND found=nullptr;EnumWindows([](HWND w,LPARAM p)->BOOL{DWORD id=0;GetWindowThreadProcessId(w,&id);if(id==GetCurrentProcessId()&&IsWindowVisible(w)&&title(w)==L"UDM")*reinterpret_cast<HWND*>(p)=w;return TRUE;},reinterpret_cast<LPARAM>(&found));if(!found)return;try{++warning->count;warning->matched&=named(found,L"Forced menu command failure")!=nullptr;if(warning->beforeAck)warning->beforeAck();SendMessageW(named(found,L"OK"),BM_CLICK,0,0);}catch(...){warning->matched=false;PostMessageW(found,WM_CLOSE,0,0);}}
struct WarningTimer {UINT_PTR timer;WarningTimer(Warning& w){warning=&w;timer=SetTimer(nullptr,0,20,acknowledge);}~WarningTimer(){KillTimer(nullptr,timer);warning=nullptr;}};
struct MenuProbe {std::function<void()> onInit;std::array<bool,11> enabled{};bool seen=false,refresh=false,open=false,options=false,recording=false;std::wstring recordingLabel;};static MenuProbe* menuProbe=nullptr;
static LRESULT CALLBACK inspectMenu(HWND w,UINT message,WPARAM wp,LPARAM lp,UINT_PTR,DWORD_PTR value){if(message==WM_INITMENUPOPUP){auto& p=*reinterpret_cast<MenuProbe*>(value);auto m=reinterpret_cast<HMENU>(wp);p.seen=true;const auto enabled=[m](UINT id){const auto s=GetMenuState(m,id,MF_BYCOMMAND);return s!=static_cast<UINT>(-1)&&!(s&(MF_DISABLED|MF_GRAYED));};for(UINT id=1;id<=10;++id)p.enabled[id]=enabled(id);p.refresh=enabled(1);p.open=enabled(5);p.options=enabled(9);p.recording=GetMenuState(m,10,MF_BYCOMMAND)!=static_cast<UINT>(-1);wchar_t label[128]{};GetMenuStringW(m,10,label,128,MF_BYCOMMAND);p.recordingLabel=label;if(p.onInit)p.onInit();}return DefSubclassProc(w,message,wp,lp);}
static void CALLBACK endOwnedMenu(HWND,UINT,UINT_PTR,DWORD){if(menuProbe&&menuProbe->seen)EndMenu();}
static MenuProbe openMenu(Progress& progress,std::function<void()> onInit={}){MenuProbe probe;probe.onInit=std::move(onInit);auto w=progress.GetSafeHwnd();SetWindowSubclass(w,inspectMenu,1,reinterpret_cast<DWORD_PTR>(&probe));menuProbe=&probe;auto timer=SetTimer(nullptr,0,20,endOwnedMenu);SendMessageW(w,WM_CONTEXTMENU,reinterpret_cast<WPARAM>(w),MAKELPARAM(-1,-1));KillTimer(nullptr,timer);menuProbe=nullptr;RemoveWindowSubclass(w,inspectMenu,1);return probe;}
static void popupCase(CWnd& owner,const fs::path& root){
 Manager m(root/L"popup");auto j=seed(m,root);Progress p(m,j,&owner);check(p.Create(101,&owner)!=FALSE,"Create actual progress popup owner");p.ShowWindow(SW_SHOWNOACTIVATE);
 std::promise<void> entered,released;auto ready=entered.get_future();auto wait=released.get_future();std::atomic_bool expired=false;std::thread thread([&]{Lock lock(m.mutex);j->data["Status"]="Complete";entered.set_value();expired=wait.wait_for(std::chrono::seconds(5))!=std::future_status::ready;});
 struct Release{std::promise<void>& signal;std::thread& thread;bool done=false;void finish(){if(!done){done=true;signal.set_value();}if(thread.joinable())thread.join();}~Release(){finish();}}release{released,thread};
 check(ready.wait_for(std::chrono::seconds(2))==std::future_status::ready,"A competing backend holds current state beyond the displayed snapshot");auto first=openMenu(p);
 check(first.seen&&!expired&&first.refresh&&!first.open&&first.options&&!first.recording,"Actual popup opens from the displayed snapshot without waiting for the model");release.finish();SendMessageW(p.GetSafeHwnd(),WM_TIMER,Form::pulseTimer,0);auto second=openMenu(p);
 check(second.seen&&!second.refresh&&second.open&&!second.options,"Next refreshed popup catches up with completion/open eligibility");p.DestroyWindow();m.stop();
}

static void popupCloseCase(CWnd& owner,const fs::path& root,bool preserve){
 Manager m(root/(preserve?L"popup-close-grace":L"popup-close"));auto j=seed(m,root);if(preserve)j->prefetchPeakDisplayUntil=std::chrono::steady_clock::now()+std::chrono::minutes(1);Progress p(m,j,&owner);check(p.Create(101,&owner)!=FALSE,"Create a progress popup receiving completion-close requests");p.ShowWindow(SW_SHOWNOACTIVATE);auto w=p.GetSafeHwnd();bool survivedRequest=false,survivedPulse=false;int initializations=0;
 auto probe=openMenu(p,[&]{++initializations;if(initializations==1)SendMessageW(w,WM_CONTEXTMENU,reinterpret_cast<WPARAM>(w),MAKELPARAM(-1,-1));p.requestCompletionClose(preserve);survivedRequest=IsWindow(w)!=FALSE;SendMessageW(w,WM_TIMER,Form::pulseTimer,0);survivedPulse=IsWindow(w)!=FALSE;});
 check(probe.seen&&initializations==1&&survivedRequest&&survivedPulse&&IsWindow(w),"Completion-close request and nested pulse retain the owner throughout popup tracking");SendMessageW(w,WM_TIMER,Form::pulseTimer,0);
 if(preserve){check(IsWindow(w),"Popup deferral preserves the existing opening-rate display grace");ProgressMenuTestAccess::expireClose(p);SendMessageW(w,WM_TIMER,Form::pulseTimer,0);check(!IsWindow(w),"Ordinary timer closes after the retained opening-rate grace expires");}
 else check(!IsWindow(w),"The normal pulse closes the retained view after popup tracking ends");m.stop();
}
static void persistence(CWnd& owner,const fs::path& root,bool pause,bool fail){
 Manager m(root/wide(std::string(pause?"pause":"resume")+(fail?"-failure":"-success")));auto j=seed(m,root);j->data["LiveRecording"]=true;j->data["Status"]=pause?"Downloading":"Paused";j->data["LimitKbps"]=64;j->sessionLimit=128;m.save();
 auto token=pause?CatalogCheckpointTestAccess::active(m,j,true):std::shared_ptr<Cancel>();struct Clear{Manager& m;JobPtr j;~Clear(){CatalogCheckpointTestAccess::active(m,j,false);}}clear{m,j};const auto before=j->snapshot();
 Progress p(m,j,&owner);check(p.Create(101,&owner)!=FALSE,"Create progress window for a recording menu command");p.ShowWindow(SW_SHOWNOACTIVATE);auto view=openMenu(p);check(view.seen&&view.recording&&view.recordingLabel==(pause?L"Pause recording":L"Resume recording"),"Actual popup displays the current recording command");
 check(!m.fileInfoDecisionPending&&j->snapshot()==before,"Inspecting the popup does not dispatch a recording command");
 Gate gate(m,fail);Warning state;state.beforeAck=[&]{state.matched&=j->snapshot()==before&&j->sessionLimit==128&&(!token||!token->cancelled());};WarningTimer timer(state);auto action=pause?ProgressMenuAction::PauseRecording:ProgressMenuAction::ResumeRecording;
 const auto began=GetTickCount64();ProgressMenuTestAccess::start(p,action);const auto dispatchMs=GetTickCount64()-began;const bool pending=m.fileInfoDecisionPending;const bool entered=gate.ready.wait_for(std::chrono::seconds(2))==std::future_status::ready;
 if(!pending||!entered)atomicText(m.root/L"gate-observation.json",Json{{"pendingAfterDispatch",pending},{"dispatchMs",dispatchMs},{"checkpointEntered",entered},{"saveCheckpoints",gate.saves.load()}}.dump(2),false);
 check(pending&&entered,"Menu dispatch returns while a real catalog write is held on the worker");
 ProgressMenuTestAccess::start(p,action);SendMessageW(p.GetSafeHwnd(),WM_CLOSE,0,0);SendMessageW(named(p.GetSafeHwnd(),L"Show speed graph"),BM_CLICK,0,0);RedrawWindow(p.GetSafeHwnd(),nullptr,nullptr,RDW_INVALIDATE|RDW_ALLCHILDREN|RDW_UPDATENOW);
 check(IsWindow(p.GetSafeHwnd())&&IsWindowVisible(named(p.GetSafeHwnd(),L"Transfer rate history"))&&gate.saves==1,"Pending recording command keeps graph painting available and rejects duplicates/close");KillTimer(p.GetSafeHwnd(),Form::pulseTimer);gate.unlock();settle(m);
 if(fail){check(state.count==1&&state.matched&&j->snapshot()==before,"Failure reports the original error and rolls back before cancellation is signalled");ProgressMenuTestAccess::start(p,action);settle(m);}
 check(gate.saves==(fail?2:1)&&str(j->data,"Status")== (pause?"Pausing":"Queued")&&(!token||token->cancelled()),"Explicit recording command publishes once, or retries once after rollback");
 check(ProgressMenuTestAccess::limit(p)==(pause?"128":"64"),"Recording Resume refreshes the limiter after clearing its temporary override");
 CatalogCheckpointTestAccess::observe(m,{});p.DestroyWindow();CatalogCheckpointTestAccess::active(m,j,false);m.stop();
}


struct InformationProbe {HWND owner;Manager& manager;bool seen=false,readOnly=false,pending=false;};static InformationProbe* informationProbe=nullptr;
static void CALLBACK closeInformation(HWND,UINT,UINT_PTR,DWORD){
 if(!informationProbe)return;HWND dialog=nullptr;EnumWindows([](HWND w,LPARAM out)->BOOL{DWORD pid=0;GetWindowThreadProcessId(w,&pid);if(pid==GetCurrentProcessId()&&IsWindowVisible(w)&&GetWindow(w,GW_OWNER)==informationProbe->owner&&title(w)==L"Download information")*reinterpret_cast<HWND*>(out)=w;return TRUE;},reinterpret_cast<LPARAM>(&dialog));if(!dialog)return;
 informationProbe->seen=true;informationProbe->pending=informationProbe->manager.fileInfoDecisionPending;
 for(auto c=GetWindow(dialog,GW_CHILD);c;c=GetWindow(c,GW_HWNDNEXT)){wchar_t kind[32]{};GetClassNameW(c,kind,32);if(!_wcsicmp(kind,L"EDIT"))informationProbe->readOnly=(GetWindowLongPtrW(c,GWL_STYLE)&ES_READONLY)!=0;}
 try{SendMessageW(named(dialog,L"Close"),BM_CLICK,0,0);}catch(...){PostMessageW(dialog,WM_CLOSE,0,0);}
}
static void sharedDecisionCase(CWnd& owner,const fs::path& root,bool fail){
 Manager m(root/(fail?L"shared-failure":L"shared-success"));auto first=seed(m,root);auto other=m.add("ftp://127.0.0.1:9/other.bin",utf8((root/L"files").wstring()),"other.bin","Main queue",true);other->data["LiveRecording"]=true;m.save();const auto firstBefore=first->snapshot();
 Progress a(m,first,&owner),b(m,other,&owner);check(a.Create(101,&owner)&&b.Create(101,&owner),"Create independent progress views for one shared decision");a.ShowWindow(SW_SHOWNOACTIVATE);b.ShowWindow(SW_SHOWNOACTIVATE);
 auto idle=openMenu(a);check(idle.seen&&idle.enabled[1]&&idle.enabled[2]&&idle.enabled[3]&&idle.enabled[6]&&idle.enabled[8],"Idle popup enables its current model and presentation commands");
 Gate gate(m,fail);Warning state;WarningTimer warningTimer(state);ProgressMenuTestAccess::start(b,ProgressMenuAction::ResumeRecording);check(gate.ready.wait_for(std::chrono::seconds(2))==std::future_status::ready&&m.fileInfoDecisionPending,"Another window holds real publication under the shared decision guard");
 auto busy=openMenu(a);check(busy.seen&&busy.enabled[6]&&busy.enabled[8],"An unrelated progress popup remains available with Hide and Information during a shared save");
 bool disabled=true;for(UINT id:{1,2,3,4,5,7,9,10})disabled&=!busy.enabled[id];check(disabled,"Shared persistence visibly disables model-dependent popup commands");
 InformationProbe info{a.GetSafeHwnd(),m};informationProbe=&info;auto infoTimer=SetTimer(nullptr,0,20,closeInformation);ProgressMenuTestAccess::choose(a,8);KillTimer(nullptr,infoTimer);informationProbe=nullptr;
 check(info.seen&&info.readOnly&&info.pending&&m.fileInfoDecisionPending&&gate.saves==1&&IsWindow(a.GetSafeHwnd()),"Actual read-only Information opens and closes while another publication remains held");
 for(UINT id:{1,2,3,4,5,7,9,10})ProgressMenuTestAccess::choose(a,id);
 check(!ProgressMenuTestAccess::pending(a)&&gate.saves==1&&IsWindow(a.GetSafeHwnd()),"Stale selections are rechecked after popup closure without dispatching or opening model dialogs");
 auto own=openMenu(b);check(!own.seen,"The saving window retains its existing popup deferral to protect accepted work lifetime");ProgressMenuTestAccess::choose(b,6);ProgressMenuTestAccess::choose(b,8);
 check(IsWindow(b.GetSafeHwnd())&&ProgressMenuTestAccess::pending(b),"Synthetic Hide/Information cannot reenter or destroy the window owning an accepted command");
 ProgressMenuTestAccess::choose(a,6);check(!IsWindow(a.GetSafeHwnd())&&m.fileInfoDecisionPending&&gate.saves==1,"Hide dismisses the unrelated view without waiting for or releasing another window's task");
 gate.unlock();settle(m);check(!m.fileInfoDecisionPending&&state.count==(fail?1:0)&&state.matched,"Shared popup availability does not alter success/error acknowledgement of the saving window");
 check(first->snapshot()==firstBefore&&str(other->data,"Status")== (fail?"Paused":"Queued"),"Presentation choices leave the unrelated job unchanged and preserve the saved outcome");
 Progress c(m,first,&owner);check(c.Create(101,&owner)!=FALSE,"Create a fresh view after the shared task drains");c.ShowWindow(SW_SHOWNOACTIVATE);auto ready=openMenu(c);check(ready.seen&&ready.enabled[1]&&ready.enabled[2]&&ready.enabled[3]&&ready.enabled[9],"Model-dependent choices are available again after the shared save or acknowledged error");
 CatalogCheckpointTestAccess::observe(m,{});c.DestroyWindow();b.DestroyWindow();m.stop();
}
static ProgressCommandResult command(Manager& m,const JobPtr& j,ProgressMenuAction a){ProgressCommandRequest r;r.menuAction=a;return applyProgressCommand(m,j,r);}
static void revalidation(const fs::path& root){
 Manager m(root/L"revalidate");auto j=seed(m,root);j->data["Url"]="https://example.invalid/current.bin";j->data["FileName"]="current.bin";
 auto copy=command(m,j,ProgressMenuAction::CopyAddress);check(copy.followup==ProgressCommandResult::CopyAddress&&copy.clipboard=="https://example.invalid/current.bin","Copy resolves current address on the worker without touching the clipboard");
 auto folder=command(m,j,ProgressMenuAction::OpenFolder);check(folder.followup==ProgressCommandResult::Folder&&folder.target==j->target().parent_path(),"Folder command resolves current destination without opening Explorer");
 check(command(m,j,ProgressMenuAction::OpenWith).followup==ProgressCommandResult::Updated,"A stale Open-with choice cannot open an unfinished file");j->data["Status"]="Complete";auto open=command(m,j,ProgressMenuAction::OpenWith);check(open.followup==ProgressCommandResult::OpenWith&&open.target==j->target(),"Open-with uses the current completed target without launching an application");
 j->data["ScanResult"]={{"Status","Pending"}};check(command(m,j,ProgressMenuAction::OpenWith).followup==ProgressCommandResult::Updated,"Pending scanner policy still prevents Open-with");j->data.erase("ScanResult");
 const auto before=j->snapshot();command(m,j,ProgressMenuAction::ResumeRecording);check(j->snapshot()==before,"A stale recording choice cannot restart a completed download");
 j->data["Status"]="Paused";j->data["LiveRecording"]=true;auto paused=j->snapshot();command(m,j,ProgressMenuAction::PauseRecording);check(j->snapshot()==paused,"A displayed Pause cannot become Resume after activity changes");
 j->data["LiveStopRequested"]=true;j->data["Status"]="Queued";auto queued=j->snapshot();command(m,j,ProgressMenuAction::ResumeRecording);check(j->snapshot()==queued,"A queued live save cannot be resumed again from a stale Retry-save choice");
 m.jobs.clear();check(command(m,j,ProgressMenuAction::CopyAddress).followup==ProgressCommandResult::Updated&&j->snapshot()==queued,"Detached membership prevents selected menu followups and mutation");m.stop();
}
static void cachePolicy(const fs::path& root){
 Manager m(root/L"policy");auto j=seed(m,root);ProgressMenuCache cache;
 const auto matches=[&]{return cache.capture(m,j).refresh==m.canRefreshAddress(j);};check(matches()&&cache.capture(m,j).refresh,"Initial refresh-menu eligibility matches the existing backend");
 j->data["ProtectedAdaptive"]=protect(Json{{"live",true}}.dump());check(matches()&&!cache.capture(m,j).refresh,"A changed encrypted live plan invalidates refresh eligibility");j->data["ProtectedAdaptive"]=protect(Json{{"live",false}}.dump());check(matches()&&cache.capture(m,j).refresh,"A changed recorded plan restores refresh eligibility");
 for(const auto* field:{"RequiresRequestCapture","LiveRecording"}){j->data[field]=true;check(matches()&&!cache.capture(m,j).refresh,"Capture/live policy changes invalidate cached refresh eligibility");j->data[field]=false;check(matches()&&cache.capture(m,j).refresh,"Cleared capture/live policy catches up in the next snapshot");}
 j->data["ProtectedRequest"]="opaque";check(matches()&&!cache.capture(m,j).refresh,"Protected request presence invalidates refresh eligibility");j->data.erase("ProtectedRequest");j->data["Url"]="file:///invalid";check(matches()&&!cache.capture(m,j).refresh,"Unsupported current URL invalidates refresh eligibility");m.stop();
}
class MenuApplication:public CWinApp {int result=1;public:BOOL InitInstance()override{
 CWinApp::InitInstance();INITCOMMONCONTROLSEX common{sizeof(common),ICC_WIN95_CLASSES|ICC_DATE_CLASSES};InitCommonControlsEx(&common);AfxOleInit();int argc=0;auto argv=CommandLineToArgvW(GetCommandLineW(),&argc);fs::path output;std::string error;Owner owner;
 try{if(argc!=2)throw std::runtime_error("Pass a fresh private output folder");output=fs::absolute(argv[1]);if(fs::exists(output))throw std::runtime_error("Fresh output required");fs::create_directories(output);wchar_t mode[2]{};uiDark=useDarkUi(GetEnvironmentVariableW(L"UDM_TEST_MENU_DARK",mode,2)==1&&mode[0]==L'1',windowsHighContrast());check(owner.Create(nullptr,L"Private progress menu fixture",WS_OVERLAPPEDWINDOW,CRect(0,0,40,40))!=FALSE,"Create isolated popup owner");m_pMainWnd=&owner;popupCase(owner,output);popupCloseCase(owner,output,false);popupCloseCase(owner,output,true);sharedDecisionCase(owner,output,false);sharedDecisionCase(owner,output,true);for(bool pause:{false,true})for(bool fail:{false,true})persistence(owner,output,pause,fail);revalidation(output);cachePolicy(output);result=0;}catch(const std::exception& e){error=e.what();}
 m_pMainWnd=nullptr;if(owner.GetSafeHwnd())owner.DestroyWindow();if(argv)LocalFree(argv);if(!output.empty())atomicText(output/L"results.json",Json{{"passed",result==0},{"error",error},{"checks",checks},{"darkMode",uiDark}}.dump(2),false);return FALSE;
 }int ExitInstance()override{AfxOleTerm(FALSE);return result;}};MenuApplication menuApplication;
}
