// Operates only the windows and catalogs created by this executable.
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
static std::wstring title(HWND window){wchar_t value[2048]{};GetWindowTextW(window,value,2048);return value;}
static HWND named(HWND window,const wchar_t* label){for(auto child=GetWindow(window,GW_CHILD);child;child=GetWindow(child,GW_HWNDNEXT))if(title(child)==label)return child;throw std::runtime_error("Missing owned limiter control");}
static void pump(const std::function<bool()>& done){const auto end=GetTickCount64()+6000;while(!done()&&GetTickCount64()<end){MSG message;while(PeekMessageW(&message,nullptr,0,0,PM_REMOVE)){TranslateMessage(&message);DispatchMessageW(&message);}Sleep(2);}if(!done())throw std::runtime_error("Limiter completion timed out");}
static void settle(Manager& manager,HWND window){pump([&]{return !manager.fileInfoDecisionPending&&IsWindowEnabled(named(window,L"Start"));});}
class LimiterOwner:public CFrameWnd {void PostNcDestroy()override{}};
struct Controls {HWND window,rate,enabled,remember;};
static Controls controls(Progress& progress){
 auto window=progress.GetSafeHwnd();HWND tabs=nullptr,rate=nullptr;wchar_t kind[80]{};
 for(auto child=GetWindow(window,GW_CHILD);child;child=GetWindow(child,GW_HWNDNEXT)){GetClassNameW(child,kind,80);if(!_wcsicmp(kind,L"SysTabControl32"))tabs=child;}
 if(!tabs)throw std::runtime_error("Missing limiter tabs");SendMessageW(tabs,TCM_SETCURSEL,1,0);NMHDR notice{tabs,static_cast<UINT_PTR>(GetDlgCtrlID(tabs)),TCN_SELCHANGE};SendMessageW(window,WM_NOTIFY,notice.idFrom,reinterpret_cast<LPARAM>(&notice));
 for(auto child=GetWindow(window,GW_CHILD);child;child=GetWindow(child,GW_HWNDNEXT)){GetClassNameW(child,kind,80);if(IsWindowVisible(child)&&!_wcsicmp(kind,L"EDIT"))rate=child;}
 if(!rate)throw std::runtime_error("Missing limiter edit");return {window,rate,named(window,L"Use Speed Limiter"),named(window,L"Remember Speed Limiter settings for this file\r\non download stop/resume")};
}
static JobPtr seed(Manager& manager,const fs::path& files,const char* name="limiter.bin"){
 auto prefs=manager.state["Settings"];prefs["DownloadFolder"]=utf8(files.wstring());prefs["ProgressStartMode"]="Normal size";prefs["ProgressSpeedTab"]=true;manager.setSettings(prefs);
 auto job=manager.add("ftp://127.0.0.1:9/"+std::string(name),utf8(files.wstring()),name,"Main queue",true);setProgressLimit(manager,job,64,true);return job;
}
struct WriteGate {
 Manager& manager;JobPtr job;bool fail;
 std::promise<void> entered,released;std::shared_future<void> ready=entered.get_future().share(),gate=released.get_future().share();
 std::atomic_bool unlocked{false};std::atomic_int saves{0};std::vector<i64> values;
 WriteGate(Manager& source,JobPtr target,bool failure):manager(source),job(std::move(target)),fail(failure){
  CatalogCheckpointTestAccess::observe(manager,[this](const char* stage){if(std::string(stage)!="temporary-flushed")return;values.push_back(num(job->data,"LimitKbps"));if(saves.fetch_add(1)==0){entered.set_value();if(gate.wait_for(std::chrono::seconds(8))!=std::future_status::ready)throw std::runtime_error("Limiter write gate expired");if(fail)throw std::runtime_error("Forced limiter save failure");}});
 }
 void unlock(){if(!unlocked.exchange(true))released.set_value();}
 ~WriteGate(){unlock();CatalogCheckpointTestAccess::observe(manager,{});}
};
struct Warning {bool seen=false,matched=false;int count=0;std::function<void()> inspect;};
static Warning* warningState=nullptr;
static void CALLBACK acknowledge(HWND,UINT,UINT_PTR,DWORD){
 if(!warningState)return;HWND warning=nullptr;
 EnumWindows([](HWND window,LPARAM value)->BOOL{DWORD pid=0;GetWindowThreadProcessId(window,&pid);if(pid==GetCurrentProcessId()&&IsWindowVisible(window)&&title(window)==L"UDM")*reinterpret_cast<HWND*>(value)=window;return TRUE;},reinterpret_cast<LPARAM>(&warning));
 if(!warning)return;warningState->seen=true;++warningState->count;
 try{warningState->matched=named(warning,L"Forced limiter save failure")!=nullptr;if(warningState->inspect)warningState->inspect();SendMessageW(named(warning,L"OK"),BM_CLICK,0,0);}catch(...){warningState->matched=false;PostMessageW(warning,WM_CLOSE,0,0);}
}
struct WarningTimer {UINT_PTR id;explicit WarningTimer(Warning& state){warningState=&state;id=SetTimer(nullptr,0,20,acknowledge);}~WarningTimer(){KillTimer(nullptr,id);warningState=nullptr;}};
static void rememberedCase(CWnd& owner,const fs::path& output,const std::string& mode){
 const bool fail=mode.find("failure")!=std::string::npos,coalesce=mode.find("coalesce")!=std::string::npos,invalid=mode.rfind("invalid-latest",0)==0;
 Manager manager(output/wide(mode)/L"state");auto job=seed(manager,output/wide(mode)/L"files");Progress progress(manager,job,&owner);
 check(progress.Create(101,&owner)!=FALSE,"Create actual progress dialog for remembered limiter edits");progress.ShowWindow(SW_SHOWNOACTIVATE);const auto c=controls(progress);WriteGate gate(manager,job,fail);
 SetWindowTextW(c.rate,L"128");
 check(manager.fileInfoDecisionPending&&gate.ready.wait_for(std::chrono::seconds(2))==std::future_status::ready,"A remembered edit returns while its real catalog write is held on the worker");
 check(IsWindowEnabled(c.rate)&&IsWindowEnabled(c.enabled)&&IsWindowEnabled(c.remember)&&!IsWindowEnabled(named(c.window,L"Start")),"Limiter fields remain editable while competing commands are disabled");
 SendMessageW(c.window,WM_COMMAND,IDOK,0);SendMessageW(c.window,WM_CLOSE,0,0);
 check(IsWindow(c.window)&&gate.saves==1,"Primary and Close cannot discard an accepted limiter save");
 if(coalesce){SetWindowTextW(c.rate,L"192");SetWindowTextW(c.rate,L"256");SetWindowTextW(c.rate,L"00320");SendMessageW(c.rate,EM_SETSEL,1,3);}
 if(invalid)SetWindowTextW(c.rate,L"0");
 SendMessageW(named(c.window,L"Show speed graph"),BM_CLICK,0,0);RedrawWindow(c.window,nullptr,nullptr,RDW_INVALIDATE|RDW_ALLCHILDREN|RDW_UPDATENOW);
 check(IsWindowVisible(named(c.window,L"Transfer rate history")),"Graph switching and painting continue during limiter persistence");
 KillTimer(c.window,Form::pulseTimer);Warning warning;bool newerDraftPreserved=false;
 if(coalesce&&fail)warning.inspect=[&]{newerDraftPreserved=title(c.rate)==L"00320";};WarningTimer timer(warning);
 gate.unlock();settle(manager,c.window);
 check(!manager.fileInfoDecisionPending,"Posted completion drains limiter edits with polling removed");
 if(fail)check(warning.seen&&warning.matched&&warning.count==1,"Limiter persistence preserves and acknowledges its actual backend error once");
 else check(!warning.seen,"Successful limiter edits do not raise a warning");
 if(coalesce){
  check(gate.values==std::vector<i64>({128,320})&&gate.saves==2&&num(job->data,"LimitKbps")==320&&!job->sessionLimit,"Only the active and newest queued valid rates are submitted");
  check(title(c.rate)==L"00320"&&(!fail||newerDraftPreserved),"Completion and older failures preserve the newer text draft without normalization");
  if(!fail){DWORD first=0,last=0;SendMessageW(c.rate,EM_GETSEL,reinterpret_cast<WPARAM>(&first),reinterpret_cast<LPARAM>(&last));check(first==1&&last==3,"Background completion preserves the current edit selection");}
 }else if(invalid){
  check(gate.saves==1&&num(job->data,"LimitKbps")== (fail?64:128)&&title(c.rate)==L"0"&&named(c.window,L"1..1000000"),"A newer invalid draft remains visible and never replaces the accepted valid rate");
 }else if(fail){
  check(num(job->data,"LimitKbps")==64&&title(c.rate)==L"64"&&!job->sessionLimit,"A failed latest edit restores the actual rolled-back limiter state");
  SetWindowTextW(c.rate,L"128");settle(manager,c.window);
  check(gate.saves==2&&num(job->data,"LimitKbps")==128&&warning.count==1,"An explicit new edit retries once after the failure");
 }else check(gate.saves==1&&num(job->data,"LimitKbps")==128&&!job->sessionLimit,"A remembered rate is committed exactly once");
 {Manager reopened(manager.root);check(num(reopened.jobs.front()->data,"LimitKbps")==num(job->data,"LimitKbps"),"The last accepted remembered rate survives catalog reopen");}
 CatalogCheckpointTestAccess::observe(manager,{});progress.DestroyWindow();manager.stop();
}
static void temporaryCase(CWnd& owner,const fs::path& output){
 Manager manager(output/L"temporary/state");auto job=seed(manager,output/L"temporary/files");const auto before=fileHash(manager.root/L"state.json");Progress progress(manager,job,&owner);
 check(progress.Create(101,&owner)!=FALSE,"Create actual progress dialog for a temporary limiter override");progress.ShowWindow(SW_SHOWNOACTIVATE);const auto c=controls(progress);
 std::promise<void> entered,released;std::atomic_bool held{true};auto ready=entered.get_future();auto gate=released.get_future().share();std::thread blocker([&]{Lock lock(manager.mutex);entered.set_value();gate.wait_for(std::chrono::seconds(8));held=false;});
 struct Release{std::thread& thread;std::promise<void>& release;bool done=false;void finish(){if(!done){done=true;release.set_value();}if(thread.joinable())thread.join();}~Release(){finish();}}release{blocker,released};
 check(ready.wait_for(std::chrono::seconds(2))==std::future_status::ready,"A competing backend owns the model before the temporary edit");
 SendMessageW(c.remember,BM_CLICK,0,0);SetWindowTextW(c.rate,L"96");SetWindowTextW(c.rate,L"00160");SendMessageW(c.rate,EM_SETSEL,1,3);
 check(held&&manager.fileInfoDecisionPending&&IsWindowEnabled(c.rate)&&title(c.rate)==L"00160","Temporary edits queue without waiting for model ownership or dropping typed digits");
 release.finish();
 settle(manager,c.window);
 check(job->sessionLimit==160&&num(job->data,"LimitKbps")==64&&fileHash(manager.root/L"state.json")==before,"The newest temporary override changes no persisted rate or catalog bytes");
 check(title(c.rate)==L"00160","Temporary completion leaves the user's numeric spelling intact");
 progress.DestroyWindow();manager.stop();
}
static void shutdownCase(CWnd& owner,const fs::path& output){
 Manager manager(output/L"shutdown/state");auto first=seed(manager,output/L"shutdown/files","first.bin");auto second=manager.add("ftp://127.0.0.1:9/second.bin",utf8((output/L"shutdown/files").wstring()),"second.bin","Main queue",true);
 Progress limiter(manager,first,&owner),other(manager,second,&owner);check(limiter.Create(101,&owner)&&other.Create(101,&owner),"Create two actual progress windows for queued-edit exit ordering");limiter.ShowWindow(SW_SHOWNOACTIVATE);other.ShowWindow(SW_SHOWNOACTIVATE);const auto c=controls(limiter);WriteGate gate(manager,first,false);
 SendMessageW(named(other.GetSafeHwnd(),L"Start"),BM_CLICK,0,0);check(gate.ready.wait_for(std::chrono::seconds(2))==std::future_status::ready,"Another accepted command holds publication before a limiter edit");
 SetWindowTextW(c.rate,L"512");check(title(c.rate)==L"512"&&IsWindowEnabled(c.rate),"An edit waits as owned data while another window owns the shared decision guard");
 std::thread releaser([&]{gate.unlock();});limiter.finishPendingCommandForShutdown();other.finishPendingCommandForShutdown();releaser.join();
 limiter.flushQueuedLimitsForShutdown();other.flushQueuedLimitsForShutdown();
 check(!manager.fileInfoDecisionPending&&num(first->data,"LimitKbps")==512&&str(second->data,"Status")=="Queued","Exit first joins existing commands, then flushes the latest waiting limiter edit");
 CatalogCheckpointTestAccess::observe(manager,{});limiter.DestroyWindow();other.DestroyWindow();manager.stop();Manager reopened(manager.root);
 check(reopened.jobs.size()==2&&num(reopened.jobs[0]->data,"LimitKbps")==512&&str(reopened.jobs[1]->data,"Status")=="Queued","The final catalog contains both drained user decisions exactly once");
}
static void liveCase(CWnd& owner,const fs::path& output,const std::string& endpoint){
 const Url url(endpoint);check(url.scheme=="http"&&url.host=="127.0.0.1","The live limiter fixture accepts its loopback HTTP server only");
 Manager manager(output/L"live/state");auto prefs=manager.state["Settings"];prefs["DownloadFolder"]=utf8((output/L"live/files").wstring());prefs["CategoryFolders"]=false;prefs["ProxyMode"]="Connect directly";prefs["Connections"]=1;prefs["Retries"]=0;manager.setSettings(prefs);
 auto job=manager.add(endpoint,utf8((output/L"live/files").wstring()),"live-limiter.bin","Main queue",true);setProgressLimit(manager,job,64,true);
 Progress progress(manager,job,&owner);check(progress.Create(101,&owner)!=FALSE,"Create actual limiter controls beside a real HTTP transfer");progress.ShowWindow(SW_SHOWNOACTIVATE);const auto c=controls(progress);manager.resume(job);manager.tick();
 auto nextTick=GetTickCount64();const auto tick=[&]{if(!manager.fileInfoDecisionPending&&GetTickCount64()>=nextTick){manager.tick();nextTick=GetTickCount64()+100;}};
 pump([&]{tick();Lock lock(manager.mutex);return manager.isActive(job)&&num(job->data,"Received")>0;});
 check(manager.isActive(job),"The limiter edit targets an active receiving download");
 SendMessageW(c.remember,BM_CLICK,0,0);SetWindowTextW(c.rate,L"256");pump([&]{tick();return !manager.fileInfoDecisionPending;});
 {Lock lock(manager.mutex);check(job->sessionLimit==256&&num(job->data,"LimitKbps")==64,"The active transfer receives the newest temporary override without changing its saved rate");}
 SendMessageW(c.remember,BM_CLICK,0,0);SetWindowTextW(c.rate,L"512");pump([&]{tick();return !manager.fileInfoDecisionPending;});
 {Lock lock(manager.mutex);check(!job->sessionLimit&&num(job->data,"LimitKbps")==512,"Remembering the active rate replaces the temporary override with persisted settings");}
 pump([&]{tick();Lock lock(manager.mutex);const auto state=str(job->data,"Status");return (state=="Complete"||state=="Failed")&&!manager.isActive(job);});
 {Lock lock(manager.mutex);check(str(job->data,"Status")=="Complete"&&num(job->data,"Received")==1024*1024,"The rate changes preserve a complete one-MiB HTTP download");}
 Bytes expected(1024*1024);for(size_t i=0;i<expected.size();++i)expected[i]=static_cast<unsigned char>(i%251);
 check(readText(job->target())==std::string(expected.begin(),expected.end()),"The completed HTTP file matches every expected source byte");
 progress.DestroyWindow();manager.stop();Manager reopened(manager.root);check(num(reopened.jobs.front()->data,"LimitKbps")==512&&str(reopened.jobs.front()->data,"Status")=="Complete","The completed download and remembered rate survive reopening together");
}
class LimiterApplication:public CWinApp {
 int result=1;
public:BOOL InitInstance()override{
 CWinApp::InitInstance();INITCOMMONCONTROLSEX common{sizeof(common),ICC_WIN95_CLASSES|ICC_DATE_CLASSES};InitCommonControlsEx(&common);AfxOleInit();int argc=0;auto argv=CommandLineToArgvW(GetCommandLineW(),&argc);fs::path output;std::string error;LimiterOwner owner;
 try{
  if(argc!=2&&argc!=3)throw std::runtime_error("Pass a fresh private output folder and optional loopback HTTP endpoint");output=fs::absolute(argv[1]);if(fs::exists(output))throw std::runtime_error("Fresh output required");fs::create_directories(output);
  wchar_t dark[2]{};uiDark=useDarkUi(GetEnvironmentVariableW(L"UDM_TEST_LIMITER_DARK",dark,2)==1&&dark[0]==L'1',windowsHighContrast());
  check(owner.Create(nullptr,L"Private limiter fixture",WS_OVERLAPPEDWINDOW,CRect(0,0,40,40))!=FALSE,"Create isolated limiter owner");m_pMainWnd=&owner;
  for(const std::string mode:{"remembered-success","remembered-failure","coalesce-success","coalesce-failure","invalid-latest","invalid-latest-failure"})rememberedCase(owner,output,mode);
  temporaryCase(owner,output);shutdownCase(owner,output);if(argc==3)liveCase(owner,output,utf8(argv[2]));result=0;
 }catch(const std::exception& failure){error=failure.what();}
 m_pMainWnd=nullptr;if(owner.GetSafeHwnd())owner.DestroyWindow();if(argv)LocalFree(argv);if(!output.empty())atomicText(output/L"results.json",Json{{"passed",result==0},{"error",error},{"checks",checks},{"darkMode",uiDark}}.dump(2),false);return FALSE;
 }
 int ExitInstance()override{AfxOleTerm(FALSE);return result;}
};LimiterApplication limiterApplication;
}
