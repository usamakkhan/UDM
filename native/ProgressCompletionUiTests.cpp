// Edits inert completion settings on paused fixture jobs; never executes them.
#include <afxwin.h>
#include <afxcmn.h>
#include <afxdlgs.h>
#include <afxole.h>
#include <atlimage.h>
#include "Ui.hpp"
namespace udm {
struct CatalogCheckpointTestAccess {static void observe(Manager& m,std::function<void(const char*)> fn){Lock lock(m.mutex);m.catalogCheckpoint=std::move(fn);}};
static Json checks=Json::array();
static void check(bool value,const char* name){checks.push_back({{"name",name},{"passed",value}});if(!value)throw std::runtime_error(name);}
static std::wstring title(HWND w){wchar_t value[2048]{};GetWindowTextW(w,value,2048);return value;}
static HWND named(HWND w,const wchar_t* label){for(auto c=GetWindow(w,GW_CHILD);c;c=GetWindow(c,GW_HWNDNEXT))if(title(c)==label)return c;throw std::runtime_error("Missing owned completion control");}
static void settle(Manager& manager){const auto end=GetTickCount64()+6000;while(manager.fileInfoDecisionPending&&GetTickCount64()<end){MSG message;while(PeekMessageW(&message,nullptr,0,0,PM_REMOVE)){TranslateMessage(&message);DispatchMessageW(&message);}Sleep(2);}if(manager.fileInfoDecisionPending)throw std::runtime_error("Completion setting timed out");}
class Owner:public CFrameWnd {void PostNcDestroy()override{}};
struct Controls {HWND window,show,disconnect,exit,power,force,choice;};
static Controls controls(Progress& progress){
 auto w=progress.GetSafeHwnd();HWND tabs=nullptr,choice=nullptr;std::vector<HWND> glyphs;wchar_t type[64]{};
 for(auto c=GetWindow(w,GW_CHILD);c;c=GetWindow(c,GW_HWNDNEXT)){GetClassNameW(c,type,64);if(!_wcsicmp(type,L"SysTabControl32"))tabs=c;}
 if(!tabs)throw std::runtime_error("Missing completion tabs");SendMessageW(tabs,TCM_SETCURSEL,2,0);NMHDR notice{tabs,static_cast<UINT_PTR>(GetDlgCtrlID(tabs)),TCN_SELCHANGE};SendMessageW(w,WM_NOTIFY,notice.idFrom,reinterpret_cast<LPARAM>(&notice));
 for(auto c=GetWindow(w,GW_CHILD);c;c=GetWindow(c,GW_HWNDNEXT)){GetClassNameW(c,type,64);if(!IsWindowVisible(c))continue;if(!_wcsicmp(type,L"BUTTON")&&title(c).empty())glyphs.push_back(c);if(!_wcsicmp(type,L"COMBOBOX"))choice=c;}
 std::sort(glyphs.begin(),glyphs.end(),[](HWND a,HWND b){RECT x{},y{};GetWindowRect(a,&x);GetWindowRect(b,&y);return x.top<y.top;});
 if(glyphs.size()!=4||!choice)throw std::runtime_error("Missing completion glyphs or power selector");
 return {w,named(w,L"Show download complete dialog"),glyphs[0],glyphs[1],glyphs[2],glyphs[3],choice};
}
static JobPtr seed(Manager& manager,const fs::path& files){
 auto prefs=manager.state["Settings"];prefs["DownloadFolder"]=utf8(files.wstring());prefs["ProgressCompletionTab"]=true;prefs["ProgressSpeedTab"]=true;prefs["ProgressStartMode"]="Normal size";manager.setSettings(prefs);
 return manager.add("ftp://127.0.0.1:9/inert.bin",utf8(files.wstring()),"inert.bin","Main queue",true);
}
struct Gate {
 Manager& manager;bool fail;std::promise<void> entered,released;std::shared_future<void> ready=entered.get_future().share(),wait=released.get_future().share();std::atomic_bool unlocked{false};std::atomic_int saves{0};
 Gate(Manager& m,bool failure):manager(m),fail(failure){CatalogCheckpointTestAccess::observe(manager,[this](const char* stage){if(std::string(stage)!="temporary-flushed")return;if(saves.fetch_add(1)==0){entered.set_value();if(wait.wait_for(std::chrono::seconds(8))!=std::future_status::ready)throw std::runtime_error("Completion setting gate expired");if(fail)throw std::runtime_error("Forced completion save failure");}});}
 void unlock(){if(!unlocked.exchange(true))released.set_value();}
 ~Gate(){unlock();CatalogCheckpointTestAccess::observe(manager,{});}
};
struct Modals {bool consent=true;int confirmations=0,errors=0;bool expectedError=true;std::function<void()> beforeConsent;};
static Modals* modals=nullptr;
static void CALLBACK answer(HWND,UINT,UINT_PTR,DWORD){
 if(!modals)return;std::vector<HWND> windows;EnumWindows([](HWND w,LPARAM value)->BOOL{DWORD pid=0;GetWindowThreadProcessId(w,&pid);if(pid==GetCurrentProcessId()&&IsWindowVisible(w)&&(title(w)==L"UDM"||title(w)==L"Force processes to terminate"))reinterpret_cast<std::vector<HWND>*>(value)->push_back(w);return TRUE;},reinterpret_cast<LPARAM>(&windows));
 for(auto w:windows)try{
  if(title(w)==L"Force processes to terminate"){++modals->confirmations;if(modals->beforeConsent)modals->beforeConsent();auto button=GetDlgItem(w,modals->consent?IDYES:IDNO);if(!button)button=named(w,modals->consent?L"Yes":L"No");if(!button)throw std::runtime_error("Missing consent button");SendMessageW(button,BM_CLICK,0,0);}
  else{++modals->errors;modals->expectedError&=named(w,L"Forced completion save failure")!=nullptr;SendMessageW(named(w,L"OK"),BM_CLICK,0,0);}
 }catch(...){modals->expectedError=false;PostMessageW(w,WM_CLOSE,0,0);}
}
struct ModalTimer {UINT_PTR timer;explicit ModalTimer(Modals& state){modals=&state;timer=SetTimer(nullptr,0,20,answer);}~ModalTimer(){KillTimer(nullptr,timer);modals=nullptr;}};
static void click(const std::string& mode,const Controls& c){
 if(mode=="hide"||mode=="show")SendMessageW(c.show,BM_CLICK,0,0);
 else if(mode=="power")SendMessageW(c.exit,BM_CLICK,0,0);
 else if(mode=="dropdown"){SendMessageW(c.choice,CB_SETCURSEL,2,0);SendMessageW(c.window,WM_COMMAND,MAKEWPARAM(GetDlgCtrlID(c.choice),CBN_SELCHANGE),reinterpret_cast<LPARAM>(c.choice));}
 else SendMessageW(c.force,BM_CLICK,0,0);
}
static void persistence(CWnd& owner,const fs::path& output,const std::string& mode,bool fail){
 const auto name=mode+(fail?"-failure":"-success");Manager manager(output/wide(name)/L"state");auto job=seed(manager,output/wide(name)/L"files");
 if(mode!="hide"){
  auto actions=std::vector<std::string>{"Open downloaded file"};if(mode=="show"||mode=="dropdown"||mode=="force")actions.push_back("Restart");
  manager.setCompletionPlan(job,actions,mode=="show",75,false);job->data["SuppressCompletionDialog"]=true;manager.save();
 }
 const auto before=job->snapshot();Progress progress(manager,job,&owner);check(progress.Create(101,&owner)!=FALSE,"Create actual completion controls for an inert paused job");progress.ShowWindow(SW_SHOWNOACTIVATE);auto c=controls(progress);Gate gate(manager,fail);Modals state;ModalTimer timer(state);
 click(mode,c);
 check(manager.fileInfoDecisionPending&&gate.ready.wait_for(std::chrono::seconds(2))==std::future_status::ready,"Completion choice returns while its real publication is held on the worker");
 check(!IsWindowEnabled(c.show)&&!IsWindowEnabled(c.exit)&&!IsWindowEnabled(named(c.window,L"Start")),"A pending completion choice disables conflicting settings and commands");
 SendMessageW(c.window,WM_COMMAND,IDOK,0);SendMessageW(c.window,WM_CLOSE,0,0);
 SendMessageW(named(c.window,L"Show speed graph"),BM_CLICK,0,0);RedrawWindow(c.window,nullptr,nullptr,RDW_INVALIDATE|RDW_ALLCHILDREN|RDW_UPDATENOW);
 check(IsWindow(c.window)&&IsWindowVisible(named(c.window,L"Transfer rate history"))&&gate.saves==1,"Graph painting continues and duplicate commands cannot replace the accepted choice");
 KillTimer(c.window,Form::pulseTimer);gate.unlock();settle(manager);
 check(!manager.fileInfoDecisionPending,"Posted completion applies or restores the choice with polling removed");
 if(fail){
  check(state.errors==1&&state.expectedError&&job->snapshot()==before,"The real persistence error is acknowledged and the entire job rolls back");
  if(mode=="dropdown"){SendMessageW(c.choice,CB_SETCURSEL,2,0);SendMessageW(c.window,WM_COMMAND,MAKEWPARAM(GetDlgCtrlID(c.choice),CBN_SELCHANGE),reinterpret_cast<LPARAM>(c.choice));}else click(mode,c);
  settle(manager);check(gate.saves==2&&state.errors==1,"An explicit retry persists once in the same progress window");
 }else check(gate.saves==1&&state.errors==0,"Successful completion settings persist once without an error");
 if(mode=="hide")check(yes(job->data,"SuppressCompletionDialog")&&IsWindowEnabled(c.exit),"Hiding the completion dialog enables its power choices");
 if(mode=="show")check(!yes(job->data,"SuppressCompletionDialog")&&completionSteps(job->data)==std::vector<std::string>{"Open downloaded file"}&&!yes(job->data,"CompletionForce")&&!IsWindowEnabled(c.exit),"Showing the dialog clears power/force choices and preserves file opening");
 if(mode=="power")check(completionSteps(job->data)==std::vector<std::string>({"Open downloaded file","Exit UDM"}),"A power-tab edit preserves the existing file-opening action");
 if(mode=="dropdown")check(completionSteps(job->data)==std::vector<std::string>({"Open downloaded file","Sleep"})&&!yes(job->data,"CompletionForce"),"The selected power action persists without inheriting unsupported force");
 if(mode=="force")check(yes(job->data,"CompletionForce")&&state.confirmations==(fail?2:1),"Every force-enabling attempt obtains its own UI consent");
 if(mode!="hide")check(num(job->data,"CompletionActionDelay")==75&&!yes(job->data,"CompletionWaitForOthers",true),"Power-tab edits preserve the existing delay and wait policy");
 {Manager reopened(manager.root);check(progressCompletionData(reopened.jobs.front()->data)==progressCompletionData(job->data),"Saved completion settings survive catalog reopen");}
 CatalogCheckpointTestAccess::observe(manager,{});progress.DestroyWindow();manager.stop();
}
static void consentCases(CWnd& owner,const fs::path& output){
 for(bool stale:{false,true}){
  Manager manager(output/(stale?L"consent-stale":L"consent-declined"));auto job=seed(manager,output/L"consent-files");manager.setCompletionPlan(job,{"Restart"},false);Progress progress(manager,job,&owner);check(progress.Create(101,&owner)!=FALSE,"Create force-consent controls without running any completion action");progress.ShowWindow(SW_SHOWNOACTIVATE);auto c=controls(progress);Gate gate(manager,false);Modals state;state.consent=stale;
  if(stale)state.beforeConsent=[&]{Lock lock(manager.mutex);job->data["CompletionActions"]=std::vector<std::string>{"Sleep"};job->data["CompletionAction"]="Sleep";SendMessageW(c.window,WM_TIMER,Form::pulseTimer,0);};
  ModalTimer timer(state);SendMessageW(c.force,BM_CLICK,0,0);
  check(state.confirmations==1&&!manager.fileInfoDecisionPending&&gate.saves==0&&!yes(job->data,"CompletionForce"),"Declined or stale force consent cannot publish a forced completion action");
  CatalogCheckpointTestAccess::observe(manager,{});progress.DestroyWindow();manager.stop();
 }
}
static void revalidation(CWnd& owner,const fs::path& output,bool global){
 Manager manager(output/(global?L"stale-policy":L"stale-complete"));auto job=seed(manager,output/L"stale-files");Progress progress(manager,job,&owner);check(progress.Create(101,&owner)!=FALSE,"Create completion controls before concurrent state changes");progress.ShowWindow(SW_SHOWNOACTIVATE);auto c=controls(progress);Gate observer(manager,false);
 std::promise<void> entered,released;auto ready=entered.get_future();auto wait=released.get_future().share();std::thread blocker([&]{Lock lock(manager.mutex);if(global)manager.state["Settings"]["SuppressCompletionDialog"]=true;else job->data["Status"]="Complete";entered.set_value();wait.wait_for(std::chrono::seconds(8));});
 struct Release {std::promise<void>& signal;std::thread& thread;bool done=false;void finish(){if(!done){done=true;signal.set_value();}if(thread.joinable())thread.join();}~Release(){finish();}}release{released,blocker};
 check(ready.wait_for(std::chrono::seconds(2))==std::future_status::ready,"Concurrent completion or policy state is held before the stale click");
 SendMessageW(c.show,BM_CLICK,0,0);check(manager.fileInfoDecisionPending,"The stale click is captured without waiting for model ownership");release.finish();settle(manager);
 check(observer.saves==0&&!yes(job->data,"SuppressCompletionDialog")&&!IsWindowEnabled(c.show),"Worker revalidation rejects stale completion or global-policy changes without writing");
 CatalogCheckpointTestAccess::observe(manager,{});progress.DestroyWindow();manager.stop();
}
static void shutdownCase(CWnd& owner,const fs::path& output){
 Manager manager(output/L"shutdown");auto first=seed(manager,output/L"shutdown-files");auto second=manager.add("ftp://127.0.0.1:9/other.bin",utf8((output/L"shutdown-files").wstring()),"other.bin","Main queue",true);Progress a(manager,first,&owner),b(manager,second,&owner);check(a.Create(101,&owner)&&b.Create(101,&owner),"Create two windows for a completion choice waiting behind another command");a.ShowWindow(SW_SHOWNOACTIVATE);b.ShowWindow(SW_SHOWNOACTIVATE);auto c=controls(a);Gate gate(manager,false);
 SendMessageW(named(b.GetSafeHwnd(),L"Start"),BM_CLICK,0,0);check(gate.ready.wait_for(std::chrono::seconds(2))==std::future_status::ready,"Another progress command holds publication before the completion choice");SendMessageW(c.show,BM_CLICK,0,0);
 check(!IsWindowEnabled(c.show)&&IsWindow(c.window),"The waiting completion intent remains owned with conflicting controls disabled");
 std::thread releaser([&]{gate.unlock();});a.finishPendingCommandForShutdown();b.finishPendingCommandForShutdown();releaser.join();a.flushQueuedLimitsForShutdown();b.flushQueuedLimitsForShutdown();
 check(yes(first->data,"SuppressCompletionDialog")&&str(second->data,"Status")=="Queued"&&!manager.fileInfoDecisionPending,"Exit flushes the queued completion choice after joining the other command");
 CatalogCheckpointTestAccess::observe(manager,{});a.DestroyWindow();b.DestroyWindow();manager.stop();Manager reopened(manager.root);check(reopened.jobs.size()==2&&yes(reopened.jobs[0]->data,"SuppressCompletionDialog"),"The final catalog retains the waiting completion choice exactly once");
}

struct OptionsProbe {
 Manager& manager;Gate& gate;bool failure;int phase=0;HWND window=nullptr;std::string error;std::function<void()> beforeRetry;bool cancelOnly=false;ULONGLONG deadline=GetTickCount64()+12000;
 void pulse(){
  if(!window){EnumWindows([](HWND w,LPARAM p)->BOOL{DWORD pid=0;GetWindowThreadProcessId(w,&pid);if(pid==GetCurrentProcessId()&&IsWindowVisible(w)&&title(w)==L"Additional completion options")reinterpret_cast<OptionsProbe*>(p)->window=w;return TRUE;},reinterpret_cast<LPARAM>(this));}
  if(!window||!IsWindow(window))return;
  if(!error.empty()){gate.unlock();if(!manager.fileInfoDecisionPending)SendMessageW(named(window,L"Cancel"),BM_CLICK,0,0);return;}
  try{
   if(GetTickCount64()>deadline)throw std::runtime_error("Options probe deadline expired");
   if(phase==0){
    SendMessageW(named(window,L"Close this progress window when done"),BM_SETCHECK,BST_UNCHECKED,0);
    SendMessageW(named(window,L"Open the download folder when done"),BM_SETCHECK,BST_CHECKED,0);
    SendMessageW(named(window,L"Open the downloaded file after the countdown"),BM_SETCHECK,BST_CHECKED,0);
    SendMessageW(named(window,L"Wait for other active and queued downloads"),BM_SETCHECK,BST_UNCHECKED,0);
    for(auto c=GetWindow(window,GW_CHILD);c;c=GetWindow(c,GW_HWNDNEXT)){wchar_t kind[32]{};GetClassNameW(c,kind,32);if(!_wcsicmp(kind,L"EDIT"))SetWindowTextW(c,L"60");}
    phase=1;if(cancelOnly)SendMessageW(named(window,L"Cancel"),BM_CLICK,0,0);else SendMessageW(window,WM_COMMAND,IDOK,0);
   }else if(phase==1&&gate.ready.wait_for(std::chrono::seconds(0))==std::future_status::ready){
    check(manager.fileInfoDecisionPending&&!IsWindowEnabled(named(window,L"OK"))&&!IsWindowEnabled(named(window,L"Cancel")),"Modal OK returns with conflicting controls disabled while publication is held");
    SendMessageW(window,WM_COMMAND,IDOK,0);SendMessageW(window,WM_CLOSE,0,0);RedrawWindow(window,nullptr,nullptr,RDW_INVALIDATE|RDW_ALLCHILDREN|RDW_UPDATENOW);
    check(IsWindow(window)&&gate.saves==1,"Pending modal paints and ignores duplicate OK and close");
    KillTimer(window,Form::pulseTimer);phase=2;gate.unlock();
   }else if(phase==2&&failure&&!manager.fileInfoDecisionPending&&IsWindowEnabled(named(window,L"OK"))){
    check(SendMessageW(named(window,L"Open the download folder when done"),BM_GETCHECK,0,0)==BST_CHECKED,"Failed modal keeps the user's draft in the same window");
    if(beforeRetry)beforeRetry();phase=3;SendMessageW(window,WM_COMMAND,IDOK,0);
   }
  }catch(const std::exception& e){error=e.what();gate.unlock();}
 }
};
static OptionsProbe* optionsProbe=nullptr;
static void CALLBACK optionsPulse(HWND,UINT,UINT_PTR,DWORD){if(optionsProbe)optionsProbe->pulse();}
static void optionsCase(CWnd& owner,const fs::path& output,bool fail,bool cancelOnly=false){
 Manager manager(output/(cancelOnly?L"modal-cancel":fail?L"modal-failure":L"modal-success"));auto job=seed(manager,output/L"modal-files");manager.setCompletionPlan(job,{"Restart"},true,75,true);auto before=job->snapshot();Gate gate(manager,fail);Modals state;ModalTimer errors(state);OptionsProbe probe{manager,gate,fail};probe.cancelOnly=cancelOnly;probe.beforeRetry=[&]{check(job->snapshot()==before,"Modal persistence failure restores the entire job before explicit retry");};optionsProbe=&probe;auto timer=SetTimer(nullptr,0,20,optionsPulse);
 progressCompletionOptions(&owner,manager,job,progressCompletionData(before));KillTimer(nullptr,timer);optionsProbe=nullptr;
 check(probe.error.empty(),"Completion-options modal interaction completes within its deadline");
 if(cancelOnly){check(!manager.fileInfoDecisionPending&&gate.saves==0&&job->snapshot()==before,"Cancelling an edited modal writes nothing and leaves the job unchanged");CatalogCheckpointTestAccess::observe(manager,{});manager.stop();return;}
 check(!manager.fileInfoDecisionPending&&gate.saves==(fail?2:1)&&state.errors==(fail?1:0)&&state.expectedError,"Modal saves once or acknowledges the original failure and retries once");
 check(!yes(job->data,"CloseProgressOnCompletion",true)&&yes(job->data,"OpenFolderOnCompletion")&&num(job->data,"CompletionActionDelay")==60&&!yes(job->data,"CompletionWaitForOthers",true),"Modal saves all captured flags and countdown together");
 check(completionSteps(job->data)==std::vector<std::string>({"Open downloaded file","Restart"})&&yes(job->data,"CompletionForce"),"Modal preserves existing power choice and force policy");
 {Manager reopened(manager.root);check(progressCompletionData(reopened.jobs.front()->data)==progressCompletionData(job->data),"All modal completion settings survive normal startup recovery");}
 CatalogCheckpointTestAccess::observe(manager,{});manager.stop();
}
static void optionsRevalidation(const fs::path& output){
 Manager manager(output/L"modal-revalidation");auto job=seed(manager,output/L"modal-files");ProgressCompletionOptionsDraft draft;draft.folder=true;
 for(int mode=0;mode<3;++mode){auto before=job->snapshot();if(mode==0)draft.seconds=14;if(mode==1){draft.seconds=30;job->data["Status"]="Complete";before=job->snapshot();}if(mode==2)manager.jobs.clear();bool rejected=false;try{applyProgressCompletionOptions(manager,job,draft);}catch(const std::exception&){rejected=true;}check(rejected&&job->snapshot()==before,"Modal command revalidates invalid countdown, completed job or detached membership without mutation");}
 manager.stop();
}
class CompletionApplication:public CWinApp {
 int result=1;
public:BOOL InitInstance()override{
 CWinApp::InitInstance();INITCOMMONCONTROLSEX common{sizeof(common),ICC_WIN95_CLASSES|ICC_DATE_CLASSES};InitCommonControlsEx(&common);AfxOleInit();int argc=0;auto argv=CommandLineToArgvW(GetCommandLineW(),&argc);fs::path output;std::string error;Owner owner;
 try{
  if(argc!=2)throw std::runtime_error("Pass a fresh private output folder");output=fs::absolute(argv[1]);if(fs::exists(output))throw std::runtime_error("Fresh output required");fs::create_directories(output);wchar_t dark[2]{};uiDark=useDarkUi(GetEnvironmentVariableW(L"UDM_TEST_COMPLETION_DARK",dark,2)==1&&dark[0]==L'1',windowsHighContrast());
  check(owner.Create(nullptr,L"Private completion setting fixture",WS_OVERLAPPEDWINDOW,CRect(0,0,40,40))!=FALSE,"Create isolated completion owner");m_pMainWnd=&owner;
  for(const std::string mode:{"hide","show","power","dropdown","force"})for(bool fail:{false,true})persistence(owner,output,mode,fail);
  optionsCase(owner,output,false);optionsCase(owner,output,true);optionsCase(owner,output,false,true);optionsRevalidation(output);consentCases(owner,output);revalidation(owner,output,false);revalidation(owner,output,true);shutdownCase(owner,output);
  const auto thin=progressCompletionData(Json{{"Status","Paused"},{"ProtectedHeaders",std::string(1024*1024,'x')}});check(!thin.contains("ProtectedHeaders")&&thin.dump().size()<100,"Completion render results omit unrelated protected request data");result=0;
 }catch(const std::exception& failure){error=failure.what();}
 m_pMainWnd=nullptr;if(owner.GetSafeHwnd())owner.DestroyWindow();if(argv)LocalFree(argv);if(!output.empty())atomicText(output/L"results.json",Json{{"passed",result==0},{"error",error},{"checks",checks},{"darkMode",uiDark}}.dump(2),false);return FALSE;
 }
 int ExitInstance()override{AfxOleTerm(FALSE);return result;}
};CompletionApplication completionApplication;
}
