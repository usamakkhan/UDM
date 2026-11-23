#include <afxwin.h>
#include <shellapi.h>
static bool ownedShellCalled=false;
static HINSTANCE WINAPI ownedShellExecute(HWND,LPCWSTR,LPCWSTR,LPCWSTR,LPCWSTR,INT){ownedShellCalled=true;Sleep(350);return reinterpret_cast<HINSTANCE>(33);}
#define ShellExecuteW ownedShellExecute
#define UDM_TOOLBAR_COMPONENT_TEST
#include "App.cpp"
#undef ShellExecuteW
namespace udm {
static fs::path output;static Json checks=Json::array(),observations=Json::array();static bool baseline=false;
static void check(bool ok,const char* name){checks.push_back({{"name",name},{"passed",ok}});if(!ok)throw std::runtime_error(name);}
static unsigned pump(HWND window){unsigned count=0;MSG message{};while(PeekMessageW(&message,window,UiTaskSignal::message,UiTaskSignal::message,PM_REMOVE)){DispatchMessageW(&message);++count;}return count;}
class ToolbarComponentTest {
public:static void run(MainWindow*& frame,Manager& manager,bool dark){
 frame->modelMaintenance.stop();frame->presentationAcks.stop();frame->KillTimer(1);uiDark=dark;
 const auto window=frame->GetSafeHwnd();auto job=manager.add("https://example.invalid/owned-folder","","owned.bin","Main queue",true);job->data["Status"]="Complete";job->data["Size"]=4;job->data["Received"]=4;job->data["OpenFolderOnCompletion"]=true;job->data["Folder"]=utf8((output/L"文件 folder").wstring());manager.save();
#ifdef UDM_SHELL_FOLDER_TASK_AVAILABLE
 std::promise<void> entered,release;auto running=entered.get_future();auto gate=release.get_future().share();std::atomic_uint calls=0;std::atomic_bool expired=false;fs::path launched;DWORD launchThread=0;
 struct JoinOwnedShell{MainWindow*& frame;~JoinOwnedShell(){if(frame){frame->shellFolderTask.finish();frame->shellFolderLauncher={};}}}firstJoin{frame};
 frame->shellFolderLauncher=[&](const fs::path& path){++calls;launched=path;launchThread=GetCurrentThreadId();entered.set_value();expired=gate.wait_for(std::chrono::seconds(5))!=std::future_status::ready;};
 const auto begin=GetTickCount64();const auto accepted=frame->tryComplete(job,false);const auto elapsed=GetTickCount64()-begin;const bool began=running.wait_for(std::chrono::seconds(1))==std::future_status::ready;
 const bool visible=!frame->completions.empty()&&IsWindow(frame->completions.back()->GetSafeHwnd());const bool pending=frame->shellFolderTask.valid()&&!frame->shellFolderTask.ready();
 // Release before assertions so even a rejected sample cannot strand a worker.
 release.set_value();const auto deadline=GetTickCount64()+2000;while(!frame->shellFolderTask.ready()&&GetTickCount64()<deadline)Sleep(1);
 check(accepted&&began&&visible&&pending&&elapsed<250,"Completion exists while the Windows folder handoff remains held off the UI thread");
 check(!ownedShellCalled&&launchThread!=GetCurrentThreadId()&&!expired,"Automatic folder launch never enters the old synchronous shell path");
 check(launched==job->target().parent_path()&&calls==1,"The accepted automatic handoff captures the exact Unicode destination once");
 unsigned wakes=pump(window);check(!frame->shellFolderTask.valid()&&wakes>0,"Its actual maintenance notification joins the result with the main timer disabled");
 frame->completions.back()->close();frame->shellFolderLauncher={};
 unsigned failures=0;frame->shellFolderErrorPresenter=[&](const ShellFolderResult& result){check(result.error=="Owned folder error"&&result.path==output/L"failure","The original destination and shell failure reach the UI owner");++failures;};
 frame->shellFolderLauncher=[](const fs::path&){throw std::runtime_error("Owned folder error");};frame->requestShellFolder(output/L"failure");
 const auto failureDeadline=GetTickCount64()+2000;while(!frame->shellFolderTask.ready()&&GetTickCount64()<failureDeadline)Sleep(1);
 {CompletionModalFence::ModalScope modal(window);pump(window);check(failures==0&&!frame->shellFolderTask.valid()&&frame->shellFolderErrors.size()==1,"Acknowledgement can retire while error presentation stays behind an owned modal");}
 pump(window);check(failures==1&&frame->shellFolderErrors.empty(),"Modal release presents the retained failure once without a timer");pump(window);check(failures==1,"Later notifications cannot duplicate the failure");
 unsigned queued=0;JoinOwnedShell queuedJoin{frame};frame->shellFolderLauncher=[&](const fs::path& path){if(path!=output/L"queued")throw std::runtime_error("Wrong queued folder");++queued;};
 {CompletionModalFence::ModalScope modal(window);frame->requestShellFolder(output/L"queued");check(!frame->shellFolderTask.valid()&&frame->shellFolders.size()==1,"A new folder request waits before shell dispatch during modal ownership");}
 auto until=GetTickCount64()+2000;while((frame->shellFolderTask.valid()||!frame->shellFolders.empty())&&GetTickCount64()<until){pump(window);Sleep(1);}check(queued==1&&!frame->shellFolderTask.valid()&&frame->shellFolders.empty(),"Release wake drains the queued folder once and joins its result");
 // The actual completion button uses the same asynchronous owner.
 std::promise<void> buttonRelease;auto buttonGate=buttonRelease.get_future().share();std::promise<void> buttonEntered;auto buttonRunning=buttonEntered.get_future();JoinOwnedShell buttonJoin{frame};
 frame->shellFolderLauncher=[&](const fs::path&){buttonEntered.set_value();buttonGate.wait_for(std::chrono::seconds(5));};frame->showCompletionDialog(job,true);auto form=frame->completions.back().get();const auto dialog=form->GetSafeHwnd();HWND folder=nullptr;
 for(auto child=GetWindow(dialog,GW_CHILD);child;child=GetWindow(child,GW_HWNDNEXT)){wchar_t label[128]{};GetWindowTextW(child,label,128);if(!wcscmp(label,L"Open folder")){folder=child;break;}}
 check(folder!=nullptr,"The real completion dialog exposes Open folder");const auto clickStart=GetTickCount64();SendMessageW(folder,BM_CLICK,0,0);const auto clickMs=GetTickCount64()-clickStart;bool clicked=buttonRunning.wait_for(std::chrono::seconds(1))==std::future_status::ready;bool closed=!IsWindow(dialog);buttonRelease.set_value();until=GetTickCount64()+2000;while(frame->shellFolderTask.valid()&&GetTickCount64()<until){pump(window);Sleep(1);}check(clicked&&closed&&clickMs<250,"Open folder closes its completion dialog promptly while accepted work continues independently");
 std::promise<void> exitRelease,exitEntered;auto exitGate=exitRelease.get_future().share();auto exitRunning=exitEntered.get_future();unsigned integrationStops=0;JoinOwnedShell exitJoin{frame};frame->stopIntegration=[&]{++integrationStops;};frame->shellFolderLauncher=[&](const fs::path&){exitEntered.set_value();exitGate.wait_for(std::chrono::seconds(5));};frame->requestShellFolder(output/L"exit");exitRunning.wait_for(std::chrono::seconds(1));
 const auto exitBegin=GetTickCount64();frame->SendMessage(WM_COMMAND,CMD_EXIT);const auto exitMs=GetTickCount64()-exitBegin;const bool deferred=IsWindow(window)&&frame->shellFolderClosePending&&!frame->quitting;bool rejected=false;try{frame->requestShellFolder(output/L"after-exit");}catch(const std::exception&){rejected=true;}exitRelease.set_value();
 check(deferred&&rejected&&exitMs<250&&integrationStops==0,"Explicit Exit remains responsive and retains accepted shell work before integration shutdown");
 until=GetTickCount64()+10000;while(IsWindow(window)&&GetTickCount64()<until){MSG message{};while(PeekMessageW(&message,nullptr,0,0,PM_REMOVE)){if(message.message!=WM_QUIT){TranslateMessage(&message);DispatchMessageW(&message);}}Sleep(1);}const bool gone=!IsWindow(window);if(gone)frame=nullptr;check(gone&&integrationStops==1,"Actual shutdown follows the shell acknowledgement and stops integration exactly once");
 observations.push_back({{"dark",dark},{"completionDispatchMs",elapsed},{"folderClickMs",clickMs},{"exitDispatchMs",exitMs},{"ownedHeldLauncher",true},{"realExplorerTiming",false}});
#else
 const auto begin=GetTickCount64();check(frame->tryComplete(job,false),"Frozen automatic completion is accepted");const auto elapsed=GetTickCount64()-begin;check(ownedShellCalled&&elapsed>=300&&!frame->completions.empty(),"Frozen 9.98 waits for the owned synchronous shell probe before creating completion");frame->completions.back()->close();observations.push_back({{"dispatchMs",elapsed},{"baseline",true},{"realExplorerTiming",false}});
#endif
 }
 static void cleanup(MainWindow*& frame){if(frame){
#ifdef UDM_SHELL_FOLDER_TASK_AVAILABLE
 frame->shellFolderTask.finish();frame->shellFolders.clear();frame->shellFolderErrors.clear();frame->shellFolderClosePending=false;frame->shellFolderLauncher={};frame->shellFolderErrorPresenter={};frame->stopIntegration={};
#endif
 for(auto& dialog:frame->completions)if(dialog->GetSafeHwnd())dialog->close();closeOwnedMainWindow(frame);}MSG quit{};while(PeekMessageW(&quit,nullptr,WM_QUIT,WM_QUIT,PM_REMOVE)){};}
};
class ShellFolderApplication:public CWinApp {int result=1;public:BOOL InitInstance()override{CWinApp::InitInstance();AfxOleInit();INITCOMMONCONTROLSEX controls{sizeof(controls),ICC_WIN95_CLASSES|ICC_DATE_CLASSES|ICC_PROGRESS_CLASS};InitCommonControlsEx(&controls);int argc=0;auto args=CommandLineToArgvW(GetCommandLineW(),&argc);std::string error;
 try{if(!args||argc!=3)throw std::runtime_error("Pass mode and fresh output");baseline=utf8(args[1])=="baseline";if(!baseline&&utf8(args[1])!="candidate")throw std::runtime_error("Invalid mode");output=fs::absolute(args[2]);if(fs::exists(output)){output.clear();throw std::runtime_error("Fresh output required");}fs::create_directories(output);
  for(bool dark:{false,true}){if(baseline&&dark)break;Manager manager(output/(dark?L"dark-state":L"light-state"));auto settings=manager.state["Settings"];settings["DownloadFolder"]=utf8((output/L"files").wstring());settings["CategoryFolders"]=false;settings["Sound"]=false;settings["ClipboardMonitor"]=false;settings["CloseToTray"]=false;settings["SuppressCompletionDialog"]=false;manager.setSettings(settings);auto frame=new MainWindow(manager);m_pMainWnd=frame;try{ToolbarComponentTest::run(frame,manager,dark);}catch(...){ToolbarComponentTest::cleanup(frame);m_pMainWnd=nullptr;throw;}ToolbarComponentTest::cleanup(frame);m_pMainWnd=nullptr;manager.stop();}result=0;
 }catch(const std::exception& e){error=e.what();}if(args)LocalFree(args);if(!output.empty())atomicText(output/L"results.json",Json{{"passed",result==0},{"error",error},{"checks",checks},{"observations",observations},{"baseline",baseline}}.dump(2),false);return FALSE;}int ExitInstance()override{AfxOleTerm(FALSE);return result;}};ShellFolderApplication application;
}
