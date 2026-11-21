#define UDM_TOOLBAR_COMPONENT_TEST
#include "App.cpp"
namespace udm {
static fs::path output;static Json checks=Json::array(),dispatchDiagnostics=Json::object();static std::string failure;
static void check(bool value,const char* name){checks.push_back({{"name",name},{"passed",value}});if(!value)throw std::runtime_error(name);}
static std::map<std::string,Json> expectedLayouts;
static double clockMs(){LARGE_INTEGER now,frequency;QueryPerformanceCounter(&now);QueryPerformanceFrequency(&frequency);return 1000.0*now.QuadPart/frequency.QuadPart;}
static void pump(){MSG message{};while(PeekMessageW(&message,nullptr,0,0,PM_REMOVE)){if(message.message==WM_QUIT)continue;TranslateMessage(&message);DispatchMessageW(&message);}}
static bool exportObserved=false,exportValid=false;static ULONGLONG exportBegan=0;static std::wstring exportExpectedName;static int exportExpectedFormat=0;
static BOOL CALLBACK inspectOwnedExport(HWND window,LPARAM){
 wchar_t caption[128]{};GetWindowTextW(window,caption,128);if(std::wstring(caption)!=L"Export downloads")return TRUE;
 HWND list=nullptr,selected=nullptr,format=nullptr;
 for(HWND control=GetWindow(window,GW_CHILD);control;control=GetWindow(control,GW_HWNDNEXT)){wchar_t kind[128]{},label[128]{};GetClassNameW(control,kind,128);GetWindowTextW(control,label,128);if(std::wstring(kind)==L"SysListView32")list=control;if(std::wstring(kind)==L"ComboBox")format=control;if(std::wstring(label)==L"Selected downloads")selected=control;}
 wchar_t name[256]{};if(list)ListView_GetItemText(list,0,0,name,256);exportObserved=true;exportValid=list&&selected&&format&&ListView_GetItemCount(list)==1&&std::wstring(name)==exportExpectedName&&SendMessageW(selected,BM_GETCHECK,0,0)==BST_CHECKED&&SendMessageW(format,CB_GETCURSEL,0,0)==exportExpectedFormat;
 PostMessageW(window,WM_CLOSE,0,0);return FALSE;
}
static void CALLBACK inspectExportTimer(HWND,UINT,UINT_PTR,DWORD){EnumThreadWindows(GetCurrentThreadId(),inspectOwnedExport,0);if(GetTickCount64()-exportBegan>5000)EnumThreadWindows(GetCurrentThreadId(),[](HWND window,LPARAM)->BOOL{wchar_t caption[128]{};GetWindowTextW(window,caption,128);if(std::wstring(caption)==L"Export downloads")PostMessageW(window,WM_CLOSE,0,0);return TRUE;},0);}
struct DispatchWriter {
 std::promise<void> release;std::thread writer;std::atomic_bool held=false,expired=false;bool opened=false;
 explicit DispatchWriter(Manager& manager){auto gate=release.get_future().share();writer=std::thread([this,&manager,gate]{Lock lock(manager.mutex);manager.state["Settings"]["DispatchMarker"]="preserved";held=true;if(gate.wait_for(std::chrono::seconds(2))!=std::future_status::ready)expired=true;held=false;});const auto end=GetTickCount64()+3000;while(!held&&GetTickCount64()<end)Sleep(1);}
 void open(){if(!opened){opened=true;release.set_value();}if(writer.joinable())writer.join();}~DispatchWriter(){open();}
};
class ToolbarComponentTest {
public:
 static void cleanup(MainWindow*& frame){if(frame){frame->stopIntegration={};closeOwnedMainWindow(frame);frame=nullptr;}}
 static void run(MainWindow*& frame,Manager& manager,const std::string& mode){
  frame->KillTimer(1);frame->ShowWindow(SW_SHOWNOACTIVATE);frame->UpdateWindow();
  manager.add("https://example.test/one",utf8((output/L"files").wstring()),"one.bin","Main queue",true);manager.add("https://example.test/two",utf8((output/L"files").wstring()),"two.bin","Main queue",true);
  auto end=GetTickCount64()+5000;do{frame->refresh();if(!frame->historyRefreshDeferred&&frame->visible.size()==2)break;Sleep(1);}while(GetTickCount64()<end);
  check(frame->visible.size()==2&&frame->table.GetItemCount()==2,"The real list renders two saved records before dispatch");frame->table.SetItemState(-1,0,LVIS_SELECTED);frame->table.SetItemState(0,LVIS_SELECTED|LVIS_FOCUSED,LVIS_SELECTED|LVIS_FOCUSED);check(frame->table.GetSelectedCount()==1,"A real native selection precedes the blocked refresh");
  if(mode=="export"){
   exportExpectedName=frame->table.GetItemText(0,0).GetString();
   for(const auto entry:std::vector<std::pair<UINT,int>>{{CMD_EXPORT,1},{CMD_EXPORT_IDM,2},{CMD_EXPORT_TEXT,0}}){exportObserved=false;exportValid=false;exportExpectedFormat=entry.second;exportBegan=GetTickCount64();const auto timer=SetTimer(nullptr,0,20,inspectExportTimer);check(timer!=0,"An owned thread timer can observe the export command");frame->SendMessage(WM_COMMAND,entry.first);KillTimer(nullptr,timer);check(exportObserved,"The actual export command opens its native dialog");check(exportValid,"Each export entry retains exactly the selected row and requested format");}
   check(frame->table.GetSelectedCount()==1&&manager.jobs.size()==2,"Cancelled export commands preserve selection and both saved records");return;
  }
  const auto window=frame->GetSafeHwnd();int stops=0;frame->stopIntegration=[&]{++stops;};DispatchWriter writer(manager);check(writer.held&&!writer.expired,"A separate writer holds Manager before the actual refresh");frame->refresh();check(writer.held&&!writer.expired&&frame->historyRefreshDeferred,"The actual busy refresh defers validation while preserving the selected row");
  if(mode=="sort-menu"||mode=="layout-exit"){
   const auto disk=readText(manager.root/L"state.json");const auto chosen=frame->selected(false);Json samples=Json::array();
   if(mode=="sort-menu"){
    for(UINT command=CMD_SORT_ADDED;command<=CMD_SORT_PAGE;++command){const auto start=clockMs();frame->SendMessage(WM_COMMAND,command);samples.push_back({{"command",command},{"ms",clockMs()-start}});dispatchDiagnostics[mode]={{"samples",samples},{"writerHeldAtReturn",writer.held.load()},{"writerExpired",writer.expired.load()}};check(writer.held&&!writer.expired,"Sort menu returns before the catalog writer releases");check(frame->sortColumn==frame->arrangeColumn(command)&&frame->ascending,"Each sort entry records its own requested column immediately");}
    frame->SendMessage(WM_TIMER,1,0);frame->Invalidate();frame->UpdateWindow();check(writer.held&&!writer.expired,"Timer and paint remain responsive after the sort choices");
   }else{
    const int width=frame->table.GetColumnWidth(0);int order[MainWindow::columnCount]{};frame->table.GetColumnOrderArray(order,MainWindow::columnCount);std::swap(order[0],order[1]);frame->table.SetColumnOrderArray(MainWindow::columnCount,order);
    for(int step=1;step<=3;++step){frame->table.SetColumnWidth(0,width+step*17);const auto start=clockMs();frame->rememberLayout();samples.push_back({{"step",step},{"ms",clockMs()-start}});dispatchDiagnostics[mode]={{"samples",samples},{"writerHeldAtReturn",writer.held.load()},{"writerExpired",writer.expired.load()}};check(writer.held&&!writer.expired,"Column layout capture never waits for the catalog writer");}frame->sortColumn=2;frame->ascending=false;
   }
   const auto expected=frame->listLayout();expectedLayouts[mode]=expected;check(readText(manager.root/L"state.json")==disk,"Busy layout edits add no intermediate catalog write");
   if(mode=="layout-exit"){
    const auto start=clockMs();frame->SendMessage(WM_COMMAND,CMD_EXIT);dispatchDiagnostics[mode]["exitCommandMs"]=clockMs()-start;check(writer.held&&!writer.expired&&IsWindow(window)&&frame->exitLayoutTask.valid(),"Exit captures the newest column layout before the writer releases");writer.open();AfxGetApp()->m_pMainWnd=nullptr;end=GetTickCount64()+6000;while(IsWindow(window)&&GetTickCount64()<end){pump();if(IsWindow(window))MsgWaitForMultipleObjectsEx(0,nullptr,50,QS_ALLINPUT,MWMO_INPUTAVAILABLE);}const bool closed=!IsWindow(window);if(closed)frame=nullptr;check(closed&&stops==1,"Pending layout Exit completes the real shutdown exactly once");
   }else{
    writer.open();frame->SendMessage(WM_TIMER,1,0);bool caughtUp=false;end=GetTickCount64()+5000;do{{std::unique_lock lock(manager.mutex,std::try_to_lock);if(lock.owns_lock())caughtUp=manager.state["Settings"].value("ListLayout",Json())==expected;}if(caughtUp)break;frame->SendMessage(WM_TIMER,1,0);Sleep(1);}while(GetTickCount64()<end);check(caughtUp,"The next available timer persists the latest sort choice in memory");frame->refresh();check(frame->selected(false)==chosen,"Deferred sorting retains the selected download identity");check(frame->visible.size()==2&&stops==0,"Deferred sorting keeps both records and integration alive");frame->stopIntegration={};
   }
   return;
  }
  const auto start=clockMs();frame->SendMessage(WM_COMMAND,mode=="select-all"?CMD_SELECTALL:mode=="quick-filter"?CMD_QUICKFILTER:CMD_EXIT);const auto elapsed=clockMs()-start;dispatchDiagnostics[mode]={{"commandMs",elapsed},{"writerHeldAtReturn",writer.held.load()},{"writerExpired",writer.expired.load()}};
  if(!IsWindow(window)){frame=nullptr;AfxGetApp()->m_pMainWnd=nullptr;}
  check(writer.held&&!writer.expired&&IsWindow(window),"The real command returns before the selected row's catalog writer releases");
  if(mode=="select-all")check(frame->table.GetSelectedCount()==2,"Select All updates the native selection while the model is busy");
  else if(mode=="quick-filter")check(frame->search.IsWindowVisible(),"Quick Filter opens its native controls while the model is busy");
  else{check(frame->quitting&&frame->exitLayoutTask.valid(),"Exit reaches its owned asynchronous layout stage despite deferred history validation");frame->SendMessage(WM_TIMER,1,0);check(writer.held&&!writer.expired&&IsWindow(window),"A pending real Exit still services its timer without acquiring the model");}
  writer.open();
  if(mode=="exit"){AfxGetApp()->m_pMainWnd=nullptr;end=GetTickCount64()+6000;while(IsWindow(window)&&GetTickCount64()<end){pump();if(IsWindow(window))MsgWaitForMultipleObjectsEx(0,nullptr,50,QS_ALLINPUT,MWMO_INPUTAVAILABLE);}const bool closed=!IsWindow(window);if(closed)frame=nullptr;check(closed&&stops==1,"Deferred-selection Exit completes its real shutdown exactly once");check(str(Json::parse(readText(manager.root/L"state.json"))["Settings"],"DispatchMarker")=="preserved","The final checkpoint retains the other writer's setting");}
  else{frame->refresh();check(frame->visible.size()==2&&stops==0,"A local view command leaves both records and integration intact");frame->stopIntegration={};}
 }
};
class MainCommandDispatchApplication:public CWinApp {
 int result=1;public:BOOL InitInstance()override{CWinApp::InitInstance();INITCOMMONCONTROLSEX controls{sizeof(controls),ICC_WIN95_CLASSES|ICC_DATE_CLASSES|ICC_PROGRESS_CLASS};InitCommonControlsEx(&controls);AfxOleInit();int argc=0;auto args=CommandLineToArgvW(GetCommandLineW(),&argc);
 try{if(!args||argc!=2)throw std::runtime_error("Fresh output required");output=args[1];if(fs::exists(output)){output.clear();throw std::runtime_error("Fresh output required");}fs::create_directories(output);
  for(const std::string mode:{"sort-menu","layout-exit","select-all","quick-filter","exit","export"}){Manager manager(output/wide(mode));auto settings=manager.state["Settings"];settings["CloseToTray"]=false;settings["ClipboardMonitor"]=false;settings["Sound"]=false;settings["ShowTipsOnStartup"]=false;settings["CategoryFolders"]=false;settings["DownloadFolder"]=utf8((output/wide(mode)/L"files").wstring());manager.setSettings(settings);MainWindow* frame=new MainWindow(manager);m_pMainWnd=frame;
   try{ToolbarComponentTest::run(frame,manager,mode);}catch(...){m_pMainWnd=nullptr;ToolbarComponentTest::cleanup(frame);throw;}m_pMainWnd=nullptr;ToolbarComponentTest::cleanup(frame);manager.stop();if(expectedLayouts.count(mode)){const auto saved=Json::parse(readText(manager.root/L"state.json"));check(saved["Settings"]["ListLayout"]==expectedLayouts.at(mode),"The final checkpoint preserves the latest width order and sort without a timer catch-up");check(str(saved["Settings"],"DispatchMarker")=="preserved","Deferred layout persistence preserves the concurrent writer setting");}}
  result=0;
 }catch(const std::exception& error){failure=error.what();}if(args)LocalFree(args);if(!output.empty())atomicText(output/L"results.json",Json{{"passed",result==0},{"checks",checks},{"error",failure},{"diagnostics",dispatchDiagnostics}}.dump(2),false);return FALSE;}
 int ExitInstance()override{AfxOleTerm(FALSE);return result;}
};MainCommandDispatchApplication application;
}
