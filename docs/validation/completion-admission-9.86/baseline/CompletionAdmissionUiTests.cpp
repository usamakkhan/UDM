#define UDM_TOOLBAR_COMPONENT_TEST
#include "App.cpp"
#include <condition_variable>
#include <future>
namespace udm {
static fs::path output;static Json checks=Json::array(),observations=Json::array();static std::string failure;static bool baseline=false;
static void check(bool ok,const char* label){checks.push_back({{"name",label},{"passed",ok}});if(!ok)throw std::runtime_error(label);}
static std::wstring text(HWND w){wchar_t b[2048]{};GetWindowTextW(w,b,2048);return b;}
static HWND named(HWND w,const wchar_t* label){for(auto c=GetWindow(w,GW_CHILD);c;c=GetWindow(c,GW_HWNDNEXT))if(text(c)==label)return c;return nullptr;}
struct AdmissionWriter {std::mutex mutex;std::condition_variable wake;bool released=false;std::atomic_bool held=false,expired=false;std::thread worker;AdmissionWriter(Manager& manager,std::function<void()> change={}){std::promise<void> entered;auto ready=entered.get_future();worker=std::thread([this,&manager,change=std::move(change),entered=std::move(entered)]()mutable{Lock lock(manager.mutex);if(change)change();held=true;entered.set_value();std::unique_lock waiting(mutex);expired=!wake.wait_for(waiting,std::chrono::milliseconds(500),[&]{return released;});held=false;});ready.get();}void open(){{std::lock_guard lock(mutex);released=true;}wake.notify_all();if(worker.joinable())worker.join();}~AdmissionWriter(){open();}};
class ToolbarComponentTest {
public:static void run(MainWindow& frame){auto& manager=frame.manager;frame.modelMaintenance.stop();frame.presentationAcks.stop();
 const auto jobFor=[&](const char* name){auto job=manager.add("https://example.invalid/admission","",name,"Main queue",true);job->data["Status"]="Complete";job->data["Size"]=4;job->data["Received"]=4;return job;};
#ifdef UDM_COMPLETION_ADMISSION_AVAILABLE
 const auto settle=[&]{const auto deadline=GetTickCount64()+5000;while(!frame.pendingCompletions.empty()&&GetTickCount64()<deadline){frame.pollCompletionWork();Sleep(1);}check(frame.pendingCompletions.empty()&&!frame.completionReadiness.valid(),"Completion admission drains and joins its readiness reader");};
#endif
 for(bool dark:{false,true}){if(baseline&&dark)break;uiDark=dark;
  for(bool eventPath:{false,true}){auto job=jobFor(eventPath?"event.bin":"explicit.bin");const auto before=frame.completions.size();AdmissionWriter writer(manager,[&]{job->data["FileName"]="latest 文件.bin";job->data["Size"]=16;});auto start=GetTickCount64();
   if(eventPath){manager.filePublished(job);manager.event(job,true);frame.OnTransferEvents(0,0);}else frame.showCompletionDialog(job,true);
   auto elapsed=GetTickCount64()-start;observations.push_back({{"dark",dark},{"eventPath",eventPath},{"dispatchMs",elapsed},{"writerStillHeld",writer.held.load()},{"watchdogExpired",writer.expired.load()}});
   if(baseline){check(writer.expired&&elapsed>=300,"Frozen completion opening waits for the catalog writer on the UI thread");writer.open();check(frame.completions.size()==before+1,"Frozen request eventually creates its completion dialog");SendMessageW(named(frame.completions.back()->GetSafeHwnd(),L"Close"),BM_CLICK,0,0);continue;}
#ifdef UDM_COMPLETION_ADMISSION_AVAILABLE
   check(writer.held&&!writer.expired&&elapsed<250,"Explicit and real publication/finished dispatch return while the writer remains held");check(frame.completions.size()==before&&!frame.pendingCompletions.empty(),"Busy admission retains intent without showing stale metadata");
   if(eventPath){manager.filePublished(job);frame.OnTransferEvents(0,0);check(frame.pendingCompletions.size()==2,"Repeated pending publication coalesces without reordering Finished");}
   frame.pollCompletionWork();check(writer.held&&!writer.expired,"Admission polling does not wait for the held writer");writer.open();settle();check(frame.completions.size()==before+1,"Publication followed by Finished creates exactly one completion dialog");auto window=frame.completions.back()->GetSafeHwnd();check(named(window,job->target().c_str())&&named(window,L"Download complete\r\nDownloaded 16 Bytes"),"Opening uses the current Unicode destination and size captured after contention");frame.pollCompletionWork();check(frame.completions.size()==before+1,"A subsequent poll cannot replay a completed presentation");SendMessageW(named(window,L"Close"),BM_CLICK,0,0);
#else
   throw std::runtime_error("Candidate admission unavailable");
#endif
  }
  if(baseline)continue;
#ifdef UDM_COMPLETION_ADMISSION_AVAILABLE
  for(int mode=0;mode<3;++mode){auto job=jobFor("obsolete.bin");auto before=frame.completions.size();AdmissionWriter writer(manager,[&]{if(mode==0)job->data["Status"]="Failed";else if(mode==1)job->data["SuppressCompletionDialog"]=true;else manager.jobs.erase(std::find(manager.jobs.begin(),manager.jobs.end(),job));});manager.filePublished(job);frame.OnTransferEvents(0,0);check(writer.held&&!writer.expired&&!frame.pendingCompletions.empty(),"Publication retains a busy request until current eligibility is readable");writer.open();settle();check(frame.completions.size()==before,"Failed, suppressed and removed publications cannot open a stale completion");}
  auto job=jobFor("cancelled.bin");auto before=frame.completions.size();AdmissionWriter writer(manager);frame.showCompletionDialog(job,true);auto start=GetTickCount64();frame.completionReadiness.finish();frame.pendingCompletions.clear();check(writer.held&&!writer.expired&&GetTickCount64()-start<250&&!frame.completionReadiness.valid(),"Closing admission joins its cancelled reader without waiting for the model");writer.open();frame.pollCompletionWork();check(frame.completions.size()==before,"A cancelled pending presentation cannot replay on a later wake");
#endif
 }
 }
};
class CompletionAdmissionApplication:public CWinApp {int result=1;bool started=false;std::unique_ptr<Manager> manager;MainWindow* frame=nullptr;void finish(){if(frame){closeOwnedMainWindow(frame);frame=nullptr;m_pMainWnd=nullptr;}manager.reset();if(!output.empty())atomicText(output/L"results.json",Json{{"passed",result==0},{"error",failure},{"checks",checks},{"observations",observations},{"baseline",baseline}}.dump(2),false);}
public:BOOL InitInstance()override{CWinApp::InitInstance();INITCOMMONCONTROLSEX common{sizeof(common),ICC_WIN95_CLASSES|ICC_DATE_CLASSES};InitCommonControlsEx(&common);AfxOleInit();int argc=0;auto argv=CommandLineToArgvW(GetCommandLineW(),&argc);try{if(!argv||argc!=3)throw std::runtime_error("Pass mode and fresh output");baseline=utf8(argv[1])=="baseline";if(!baseline&&utf8(argv[1])!="candidate")throw std::runtime_error("Invalid fixture mode");output=fs::absolute(argv[2]);if(fs::exists(output)){output.clear();throw std::runtime_error("Use fresh output");}fs::create_directories(output);manager=std::make_unique<Manager>(output/L"state");auto prefs=manager->state["Settings"];prefs["DownloadFolder"]=utf8((output/L"files").wstring());prefs["CategoryFolders"]=false;prefs["ClipboardMonitor"]=false;prefs["Sound"]=false;prefs["CloseToTray"]=false;manager->setSettings(prefs);frame=new MainWindow(*manager);m_pMainWnd=frame;frame->KillTimer(1);}catch(const std::exception& e){failure=e.what();if(argv)LocalFree(argv);finish();return FALSE;}LocalFree(argv);return TRUE;}BOOL OnIdle(LONG)override{if(started)return FALSE;started=true;try{ToolbarComponentTest::run(*frame);result=0;}catch(const std::exception& e){failure=e.what();}finish();PostQuitMessage(result);return FALSE;}int ExitInstance()override{AfxOleTerm(FALSE);return result;}};CompletionAdmissionApplication application;
}
