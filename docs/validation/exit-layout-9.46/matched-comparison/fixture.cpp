#define UDM_TOOLBAR_COMPONENT_TEST
#include "ObservedApp.cpp"
namespace udm {
static Json exitEvents=Json::array();static fs::path output;static std::mutex exitEventMutex;
static double stamp(){LARGE_INTEGER time{},frequency{};QueryPerformanceCounter(&time);QueryPerformanceFrequency(&frequency);return 1000.0*time.QuadPart/frequency.QuadPart;}
void observeOwnedExitStage(const char* stage){std::lock_guard<std::mutex> lock(exitEventMutex);exitEvents.push_back({{"stage",stage},{"ms",stamp()},{"thread",GetCurrentThreadId()}});}
class ToolbarComponentTest {public:static void observe(MainWindow& frame){
#ifdef UDM_MAIN_EXIT_LAYOUT_AVAILABLE
 frame.exitLayoutObserver=[](const char* point){if(std::string(point)!="waiting-model")observeOwnedExitStage(point);};
#else
 (void)frame;
#endif
}};
static void pumpOwnedExit(){MSG message{};while(PeekMessageW(&message,nullptr,0,0,PM_REMOVE)){if(message.message==WM_QUIT)continue;TranslateMessage(&message);DispatchMessageW(&message);}}
class ExitObservationApplication:public CWinApp {
 int result=1;public:BOOL InitInstance()override{CWinApp::InitInstance();INITCOMMONCONTROLSEX controls{sizeof(controls),ICC_WIN95_CLASSES|ICC_DATE_CLASSES|ICC_PROGRESS_CLASS};InitCommonControlsEx(&controls);AfxOleInit();int argc=0;auto args=CommandLineToArgvW(GetCommandLineW(),&argc);std::unique_ptr<Manager> manager;MainWindow* frame=nullptr;std::thread writer;std::atomic_bool held=false;Json record;std::string error;
 try{if(!args||argc!=2)throw std::runtime_error("Fresh owned fixture required");output=args[1];if(fs::exists(output)){output.clear();throw std::runtime_error("Fresh output required");}fs::create_directories(output);manager=std::make_unique<Manager>(output/L"catalog");auto settings=manager->state["Settings"];settings["CloseToTray"]=false;settings["ClipboardMonitor"]=false;settings["Sound"]=false;settings["ShowTipsOnStartup"]=false;settings["DownloadFolder"]=utf8((output/L"files").wstring());settings["CategoryFolders"]=false;manager->setSettings(settings);frame=new MainWindow(*manager);m_pMainWnd=frame;frame->ShowWindow(SW_SHOWNOACTIVATE);ToolbarComponentTest::observe(*frame);
 auto entered=std::make_shared<std::promise<void>>();auto ready=entered->get_future();writer=std::thread([&,entered]{Lock lock(manager->mutex);held=true;entered->set_value();Sleep(750);held=false;});if(ready.wait_for(std::chrono::seconds(3))!=std::future_status::ready)throw std::runtime_error("Writer did not acquire Manager");
 const auto window=frame->GetSafeHwnd();const auto begin=stamp();frame->SendMessage(WM_COMMAND,CMD_EXIT);const auto closeReturned=stamp();const bool heldAtReturn=held.load();const auto end=GetTickCount64()+6000;while(IsWindow(window)&&GetTickCount64()<end){pumpOwnedExit();if(!IsWindow(window))break;MsgWaitForMultipleObjectsEx(0,nullptr,50,QS_ALLINPUT,MWMO_INPUTAVAILABLE);}if(IsWindow(window))throw std::runtime_error("Owned asynchronous Exit did not settle");const auto windowClosed=stamp();frame=nullptr;m_pMainWnd=nullptr;if(writer.joinable())writer.join();manager.reset();const auto finished=stamp();record={{"passed",true},{"diagnosticOnly",true},{"qualification",false},{"controlledCatalogHoldMs",750},{"exitCommandMs",closeReturned-begin},{"ownerAndBackgroundCleanupMs",finished-begin},{"postWindowCleanupMs",finished-windowClosed},{"windowCloseMs",windowClosed-begin},{"writerHeldAtReturn",heldAtReturn},{"events",exitEvents},{"writerHeldAfterExit",held.load()},{"installedIdmComparison",false}};result=0;
 }catch(const std::exception& failed){error=failed.what();}if(writer.joinable())writer.join();if(frame){frame->SendMessage(WM_COMMAND,CMD_EXIT);frame=nullptr;}m_pMainWnd=nullptr;manager.reset();if(args)LocalFree(args);if(!output.empty()){if(result)record={{"passed",false},{"error",error},{"diagnosticOnly",true},{"events",exitEvents}};atomicText(output/L"results.json",record.dump(2),false);}return FALSE;}
 int ExitInstance()override{AfxOleTerm(FALSE);return result;}
};ExitObservationApplication application;
}
