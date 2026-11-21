#define UDM_TOOLBAR_COMPONENT_TEST
#include "App.cpp"
namespace udm {
static fs::path output;static Json columnChecks=Json::array(),columnTimings=Json::object();static std::string columnFailure;
static void check(bool value,const char* name){columnChecks.push_back({{"name",name},{"passed",value}});if(!value)throw std::runtime_error(name);}
static double stamp(){LARGE_INTEGER n,f;QueryPerformanceCounter(&n);QueryPerformanceFrequency(&f);return 1000.0*n.QuadPart/f.QuadPart;}
struct ColumnsWriter {std::promise<void> release;std::thread thread;std::atomic_bool held=false,expired=false;bool opened=false;explicit ColumnsWriter(Manager& manager){auto gate=release.get_future().share();thread=std::thread([this,&manager,gate]{Lock lock(manager.mutex);manager.state["Settings"]["UserAgent"]="Columns concurrent writer";held=true;if(gate.wait_for(std::chrono::milliseconds(2500))!=std::future_status::ready)expired=true;held=false;});const auto end=GetTickCount64()+3000;while(!held&&GetTickCount64()<end)Sleep(1);}void open(){if(!opened){opened=true;release.set_value();}if(thread.joinable())thread.join();}~ColumnsWriter(){open();}};
static std::wstring columnText(HWND window){wchar_t text[512]{};GetWindowTextW(window,text,512);return text;}
static HWND columnChild(HWND window,const wchar_t* label){for(HWND c=GetWindow(window,GW_CHILD);c;c=GetWindow(c,GW_HWNDNEXT))if(columnText(c)==label)return c;throw std::runtime_error("Missing column control");}
static HWND columnKind(HWND window,const wchar_t* kind){for(HWND c=GetWindow(window,GW_CHILD);c;c=GetWindow(c,GW_HWNDNEXT)){wchar_t type[128]{};GetClassNameW(c,type,128);if(std::wstring(type)==kind)return c;}throw std::runtime_error("Missing column control type");}
class ToolbarComponentTest {
 struct Context {MainWindow* frame;Manager* manager;std::string mode,error;int stage=0;double began=0;ULONGLONG deadline=0;std::unique_ptr<ColumnsWriter> read,save,after;std::thread later;std::atomic_bool failRead=false,failSave=false;};
 inline static Context* active=nullptr;
 static void press(HWND window,const wchar_t* label){SendMessageW(columnChild(window,label),BM_CLICK,0,0);}
 static BOOL CALLBACK observe(HWND window,LPARAM){if(columnText(window)!=L"Columns")return TRUE;ShowWindow(window,SW_SHOWNOACTIVATE);if(!IsWindowVisible(window))return TRUE;auto& c=*active;
  try{const auto list=columnKind(window,L"SysListView32"),width=columnKind(window,L"Edit"),ok=columnChild(window,L"OK"),cancel=columnChild(window,L"Cancel");
   if(c.stage==0){columnTimings[c.mode]["openMs"]=stamp()-c.began;check(c.read->held&&!c.read->expired,"Columns becomes visible before its catalog writer releases");check(!IsWindowEnabled(list)&&!IsWindowEnabled(ok)&&IsWindowEnabled(cancel),"Loading columns retain usable Cancel and freeze incomplete fields");
    if(c.mode=="cancel"){c.stage=9;press(window,L"Cancel");return FALSE;}c.read->open();c.stage=1;return FALSE;}
   if(c.stage==1){
    if(c.mode=="read-retry"){const auto retry=columnChild(window,L"Retry");if(IsWindowVisible(retry)){check(!IsWindowEnabled(ok)&&IsWindowEnabled(cancel),"Read failure preserves a cancellable loading dialog");c.stage=2;press(window,L"Retry");return FALSE;}}
    else c.stage=2;
   }
   if(c.stage==2&&IsWindowEnabled(list)){check(ListView_GetItemCount(list)==12,"Ready Columns displays all twelve fields");ListView_SetItemState(list,8,LVIS_SELECTED|LVIS_FOCUSED,LVIS_SELECTED|LVIS_FOCUSED);check(columnText(width)==L"333","Hidden column width comes from the asynchronous settings snapshot");
    if(c.mode=="read-retry"){c.stage=9;press(window,L"Cancel");return FALSE;}
    SetWindowTextW(width,L"345");ListView_SetCheckState(list,8,TRUE);press(window,L"Move Up");check(columnText(width)==L"345","Moving a column retains its edited width");c.save=std::make_unique<ColumnsWriter>(*c.manager);c.stage=3;press(window,L"OK");return FALSE;}
   if(c.stage==3){check(c.save->held&&!c.save->expired&&c.manager->fileInfoDecisionPending,"Accepted Save returns with its guard while the model remains held");check(!IsWindowEnabled(list)&&!IsWindowEnabled(width)&&!IsWindowEnabled(ok)&&!IsWindowEnabled(cancel),"An accepted save freezes all conflicting column controls");SendMessageW(window,WM_COMMAND,IDCANCEL,0);check(IsWindow(window),"Cancel cannot discard an accepted column save");c.stage=4;
    if(c.mode=="forced-close"){c.later=std::thread([&c]{Sleep(50);c.save->open();});EndDialog(window,IDCANCEL);return FALSE;}c.save->open();return FALSE;}
   if(c.stage==4&&c.mode=="save-retry"&&IsWindowEnabled(list)){check(columnText(width)==L"345"&&c.frame->table.GetColumnWidth(8)==0,"Save failure retains editor changes and leaves the main layout unchanged");check(!c.manager->fileInfoDecisionPending&&IsWindowEnabled(cancel),"Failed Save releases its guard for explicit retry");c.stage=5;press(window,L"OK");return FALSE;}
  }catch(const std::exception& error){c.error=error.what();if(c.read)c.read->open();if(c.save)c.save->open();if(c.after)c.after->open();PostMessageW(window,WM_CLOSE,0,0);}return FALSE;
 }
 static void CALLBACK timer(HWND,UINT,UINT_PTR,DWORD){if(!active)return;if(GetTickCount64()>active->deadline){active->error="Columns fixture deadline";if(active->read)active->read->open();if(active->save)active->save->open();if(active->after)active->after->open();EnumThreadWindows(GetCurrentThreadId(),[](HWND window,LPARAM)->BOOL{if(columnText(window)==L"Columns")PostMessageW(window,WM_CLOSE,0,0);return TRUE;},0);return;}EnumThreadWindows(GetCurrentThreadId(),observe,0);}
public:
 static void cleanup(MainWindow*& frame){if(frame){closeOwnedMainWindow(frame);frame=nullptr;}}
 static void run(MainWindow& frame,Manager& manager,const std::string& mode){frame.KillTimer(1);frame.ShowWindow(SW_SHOWNOACTIVATE);frame.table.SetColumnWidth(8,0);const auto initialLayout=frame.listLayout();const auto disk=readText(manager.root/L"state.json");Context context{&frame,&manager,mode};context.failRead=mode=="read-retry";context.failSave=mode=="save-retry";
#ifdef UDM_COLUMN_SETTINGS_TASK_AVAILABLE
  frame.columnSettingsObserver=[&](const char* point){if(std::string(point)=="before-read"&&context.failRead.exchange(false))throw std::runtime_error("Owned columns read failure");if(std::string(point)=="before-save"&&context.failSave.exchange(false))throw std::runtime_error("Owned columns save failure");if(std::string(point)=="captured"&&mode=="save"&&manager.fileInfoDecisionPending)context.after=std::make_unique<ColumnsWriter>(manager);};
#endif
  context.read=std::make_unique<ColumnsWriter>(manager);context.began=stamp();context.deadline=GetTickCount64()+12000;active=&context;const auto id=SetTimer(frame.GetSafeHwnd(),0xC015,20,timer);check(id!=0,"An owned timer can inspect the Columns workflow");frame.SendMessage(WM_COMMAND,CMD_COLUMNS);KillTimer(frame.GetSafeHwnd(),id);active=nullptr;
  columnTimings[mode]["workflowMs"]=stamp()-context.began;if(context.error.empty()&&mode=="cancel")check(context.read->held&&!context.read->expired,"Loading Cancel returns before the catalog writer releases");
  if(context.later.joinable())context.later.join();if(mode=="save"&&context.after){check(context.after->held&&!context.after->expired,"Successful UI application does not reacquire a model held after the save");context.after->open();}context.read->open();if(context.save)context.save->open();
#ifdef UDM_COLUMN_SETTINGS_TASK_AVAILABLE
  frame.columnSettingsObserver={};
#endif
  if(!context.error.empty())throw std::runtime_error(context.error);
  check(!manager.fileInfoDecisionPending,"Columns returns without an abandoned decision guard");if(mode=="cancel"||mode=="read-retry"){check(frame.listLayout()==initialLayout&&readText(manager.root/L"state.json")==disk,"Cancelling initial column preparation preserves the main layout and catalog");}
  else{const auto applied=frame.listLayout();check(applied["Widths"][8]==345&&applied["Order"][7]==8,"Successful or forced-close Save applies the accepted width visibility and order");auto saved=Json::parse(readText(manager.root/L"state.json"));check(saved["Settings"]["ColumnWidths"][8]==345&&saved["Settings"]["ListLayout"]==applied,"The complete accepted column layout matches its durable catalog");check(str(saved["Settings"],"UserAgent")=="Columns concurrent writer","Column Save preserves unrelated current settings");}
 }
};
class ColumnsApplication:public CWinApp {int result=1;public:BOOL InitInstance()override{CWinApp::InitInstance();INITCOMMONCONTROLSEX c{sizeof(c),ICC_WIN95_CLASSES|ICC_DATE_CLASSES|ICC_PROGRESS_CLASS};InitCommonControlsEx(&c);AfxOleInit();int argc=0;auto args=CommandLineToArgvW(GetCommandLineW(),&argc);
 try{if(!args||argc!=2)throw std::runtime_error("Fresh output required");output=args[1];if(fs::exists(output)){output.clear();throw std::runtime_error("Fresh output required");}fs::create_directories(output);for(const std::string mode:{"cancel","read-retry","save","save-retry","forced-close"}){atomicText(output/L"phase.json",Json{{"mode",mode},{"phase","construct-manager"}}.dump(),false);Manager manager(output/wide(mode));auto settings=manager.state["Settings"];settings["CloseToTray"]=false;settings["ShowTipsOnStartup"]=false;settings["ClipboardMonitor"]=false;settings["Sound"]=false;settings["CategoryFolders"]=false;settings["ColumnWidths"]=std::vector<int>{150,20,75,65,75,95,65,200,333,220,200,200};atomicText(output/L"phase.json",Json{{"mode",mode},{"phase","settings"}}.dump(),false);manager.setSettings(settings);atomicText(output/L"phase.json",Json{{"mode",mode},{"phase","create-frame"}}.dump(),false);MainWindow* frame=new MainWindow(manager);m_pMainWnd=frame;atomicText(output/L"phase.json",Json{{"mode",mode},{"phase","dialog"}}.dump(),false);try{ToolbarComponentTest::run(*frame,manager,mode);}catch(...){m_pMainWnd=nullptr;ToolbarComponentTest::cleanup(frame);throw;}m_pMainWnd=nullptr;ToolbarComponentTest::cleanup(frame);manager.stop();}result=0;
 }catch(const std::exception& error){columnFailure=error.what();}if(args)LocalFree(args);if(!output.empty())atomicText(output/L"results.json",Json{{"passed",result==0},{"checks",columnChecks},{"error",columnFailure},{"diagnostics",columnTimings}}.dump(2),false);return FALSE;}int ExitInstance()override{AfxOleTerm(FALSE);return result;}};ColumnsApplication application;
}
