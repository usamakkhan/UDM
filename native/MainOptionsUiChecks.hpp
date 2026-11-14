// Real MainWindow command/message path, isolated catalog and own native windows.
struct MainOptionsModelGate {
 std::promise<void> entered,released;std::shared_future<void> ready=entered.get_future().share();std::atomic_bool unlocked=false,expired=false;std::thread thread;
 explicit MainOptionsModelGate(Manager& manager,JobPtr job):thread([&,job=std::move(job),wait=released.get_future()]{Lock lock(manager.mutex);job->data["Description"]="Updated during Options";entered.set_value();expired=wait.wait_for(std::chrono::seconds(20))!=std::future_status::ready;}){}
 void release(){if(!unlocked.exchange(true))released.set_value();if(thread.joinable())thread.join();}~MainOptionsModelGate(){release();}
};
struct MainAppearanceProbe {
 Manager& manager;unsigned queries=0,available=0;
 static LRESULT CALLBACK observe(HWND window,UINT message,WPARAM w,LPARAM l,UINT_PTR id,DWORD_PTR context){auto& p=*reinterpret_cast<MainAppearanceProbe*>(context);if(message==WM_SETFONT){++p.queries;bool read=false;std::thread worker([&]{std::unique_lock<std::recursive_mutex> lock(p.manager.mutex,std::try_to_lock);if(lock.owns_lock()){p.manager.state["Settings"]["MainOptionsAppearanceMarker"]="preserved";read=true;}});worker.join();if(read)++p.available;}return DefSubclassProc(window,message,w,l);}
};
class ToolbarComponentTest {
public:
 static void routing(MainWindow& w,JobPtr job){expect(uniqueMainWakeups(),"All twelve main-window wake-up IDs are distinct");enqueue(w,job);const auto pending=events(w);w.SendMessage(PREPARED_FILE_INFO,0,0);expect(events(w)==pending,"A prepared File Info wake-up cannot consume a transfer event");w.SendMessage(TRANSFER_EVENTS);expect(events(w)==0,"A transfer wake-up drains its event through the message map without waiting for a timer");}
 static void history(MainWindow& w,JobPtr job){w.SendMessage(REFRESH_VISIBLE_HISTORY);expect(!w.historyRefreshDeferred&&!w.visible.empty()&&w.visible[0]==job&&w.table.GetItemText(0,7)==L"Updated during Options","Deferred history repaints new download details after Options closes");}
 static bool active(const MainWindow& w){return w.optionsUiActive;}
 static bool pending(const MainWindow& w){return w.optionsRefreshPending;}
 static bool queuePending(const MainWindow& w){return w.optionsQueueDispatchPending;}
 static HWND table(MainWindow& w){return w.table.GetSafeHwnd();}
 static size_t events(MainWindow& w){std::lock_guard<std::mutex> lock(w.eventsMutex);return w.events.size();}
 static void enqueue(MainWindow& w,JobPtr job){std::lock_guard<std::mutex> lock(w.eventsMutex);w.events.emplace_back(job,MainWindow::TransferEvent::Started);}
 static void prepare(MainWindow& w,std::function<void(Options&)> configure){w.configureOptionsDialog=std::move(configure);w.refresh();expect(!w.visible.empty(),"Main Options fixture has a real selected download row");w.table.SetItemState(0,LVIS_SELECTED,LVIS_SELECTED);w.historyRefreshDeferred=true;}
 static void heldMessages(MainWindow& w,MainOptionsModelGate& gate){
  w.SendMessage(WM_TIMER,1);w.SendMessage(DISPATCH_QUEUE_WORK);w.SendMessage(REFRESH_VISIBLE_HISTORY);w.SendMessage(COMMAND_FILE_REFRESH);w.SendMessage(TRANSFER_EVENTS);w.SendMessage(WM_COMMAND,CMD_OPTIONS);
  expect(!gate.unlocked&&!gate.expired&&w.optionsUiActive,"Actual main timer, queue, history, advisory and duplicate Options messages return while the model is held");expect(w.optionsQueueDispatchPending&&w.historyRefreshDeferred&&!w.refreshing,"Deferred queue/history work is retained without entering a parent refresh");expect(!w.commandEnabled(CMD_RESUME),"Parent command UI avoids selected-job catalog reads during Options");
 }
 static void availableTick(MainWindow& w,Manager& m,JobPtr job){ {Lock lock(m.mutex);job->speed=123;}w.SendMessage(WM_TIMER,1);{Lock lock(m.mutex);expect(job->speed==0,"Normal model rate and queue tick still runs while Options is open");}expect(w.optionsUiActive,"Available model maintenance does not close or replace Options");}
 static void cleanup(MainWindow* w){w->quitting=true;w->OnClose();AfxGetApp()->m_pMainWnd=nullptr;}
};
struct MainOptionsProbe {
 Manager& manager;MainWindow& frame;JobPtr job;std::string mode;Options* dialog=nullptr;HWND window=nullptr;std::unique_ptr<MainOptionsModelGate> gate;int phase=0;std::string failure;ULONGLONG deadline=GetTickCount64()+25000;
 void hold(){gate=std::make_unique<MainOptionsModelGate>(manager,job);expect(gate->ready.wait_for(std::chrono::seconds(2))==std::future_status::ready,"Independent owner holds the actual catalog for the main Options path");}
 void tick(){
  EnumThreadWindows(GetCurrentThreadId(),[](HWND w,LPARAM p)->BOOL{auto probe=reinterpret_cast<MainOptionsProbe*>(p);if(IsWindowVisible(w)&&windowText(w)==L"UDM Configuration")probe->window=w;return TRUE;},reinterpret_cast<LPARAM>(this));
  try{
   if(GetTickCount64()>deadline)throw std::runtime_error("Main Options interaction deadline");if(!window||!IsWindow(window)||!dialog)return;if(!failure.empty()){if(gate)gate->release();if(!manager.fileInfoDecisionPending)PostMessageW(window,WM_CLOSE,0,0);return;}if(phase!=0)return;if(mode!="initial-cancel"&&!DialReadTestAccess::ready(*dialog))return;
   expect(ToolbarComponentTest::active(frame)&&!frame.IsWindowEnabled(),"The real Options command owns a disabled main frame throughout its modal session");
   if(mode=="initial-cancel"){
    expect(!OptionsInitialTestAccess::loaded(*dialog),"Real main command displays the loading shell before the catalog is released");ToolbarComponentTest::heldMessages(frame,*gate);capture(window,L"main-options-loading.png");phase=1;SendMessageW(window,WM_CLOSE,0,0);return;
   }
   ToolbarComponentTest::availableTick(frame,manager,job);
   if(mode=="loaded-cancel"){hold();ToolbarComponentTest::heldMessages(frame,*gate);capture(window,L"main-options-loaded-held.png");phase=1;SendMessageW(window,WM_CLOSE,0,0);return;}
   {Lock lock(manager.mutex);job->data["Description"]="Updated during Options";}SendMessageW(child(window,L"Enable browser download capture"),BM_CLICK,0,0);phase=1;SendMessageW(window,WM_COMMAND,IDOK,0);
  }catch(const std::exception& e){failure=e.what();if(gate)gate->release();PostMessageW(window,WM_CLOSE,0,0);}
 }
};
static MainOptionsProbe* mainOptionsProbe=nullptr;
static void CALLBACK driveMainOptions(HWND,UINT,UINT_PTR,DWORD){if(mainOptionsProbe)mainOptionsProbe->tick();}
static void mainOptionsCase(Manager& manager,MainWindow& frame,JobPtr job,const std::string& mode){
 auto settings=manager.state["Settings"];settings["BrowserCaptureEnabled"]=true;{Lock lock(manager.mutex);job->data["Description"]="Before Options";}manager.setSettings(settings);MainOptionsProbe probe{manager,frame,job,mode};ToolbarComponentTest::prepare(frame,[&](Options& dialog){probe.dialog=&dialog;OptionsInitialTestAccess::services(dialog,[]{return false;},[]{return false;},[](bool){throw std::runtime_error("Unexpected fixture Windows startup write");});OptionsInitialTestAccess::connections(dialog,[]{return std::vector<DialEntry>{};});});ToolbarComponentTest::enqueue(frame,job);const auto retained=ToolbarComponentTest::events(frame);MainAppearanceProbe appearance{manager};auto table=ToolbarComponentTest::table(frame);if(!SetWindowSubclass(table,MainAppearanceProbe::observe,796,reinterpret_cast<DWORD_PTR>(&appearance)))throw std::runtime_error("Cannot observe owned appearance controls");
 try{
  if(mode=="initial-cancel")probe.hold();mainOptionsProbe=&probe;auto timer=SetTimer(nullptr,0,20,driveMainOptions);frame.SendMessage(WM_COMMAND,CMD_OPTIONS);KillTimer(nullptr,timer);mainOptionsProbe=nullptr;if(!probe.failure.empty())throw std::runtime_error(probe.failure);
  expect(!ToolbarComponentTest::active(frame)&&frame.IsWindowEnabled(),"Returning from Options releases the modal main-frame guard");
  if(probe.gate){
   expect(!probe.gate->unlocked&&!probe.gate->expired&&ToolbarComponentTest::pending(frame)&&appearance.queries==0,"Real Options Cancel returns before model release without applying appearance on the held lock");frame.SendMessage(WM_TIMER,1);frame.SendMessage(TRANSFER_EVENTS);frame.SendMessage(REFRESH_VISIBLE_HISTORY);expect(ToolbarComponentTest::pending(frame)&&ToolbarComponentTest::events(frame)==retained,"Close handoff retains appearance, history and transfer events while the model remains held");probe.gate->release();frame.SendMessage(WM_TIMER,1);
  }
  expect(!ToolbarComponentTest::pending(frame)&&appearance.queries>0&&appearance.available==appearance.queries,"Appearance applies from detached preferences with the catalog available during native font queries");expect(str(manager.state["Settings"],"MainOptionsAppearanceMarker")=="preserved","Appearance application preserves a concurrent unrelated catalog edit");frame.SendMessage(DISPATCH_QUEUE_WORK);frame.SendMessage(TRANSFER_EVENTS);expect(!ToolbarComponentTest::queuePending(frame)&&ToolbarComponentTest::events(frame)==0,"Deferred queue work and transfer events drain after the Options handoff");ToolbarComponentTest::history(frame,job);expect(yes(manager.state["Settings"],"BrowserCaptureEnabled")==(mode!="accept"),"Main-command OK commits its edit and both Cancel paths leave that setting unchanged");if(mode=="accept"){Manager reopened(manager.root);expect(!yes(reopened.state["Settings"],"BrowserCaptureEnabled"),"An accepted real main Options edit survives catalog reopening");}capture(frame.GetSafeHwnd(),wide("main-options-returned-"+mode+".png").c_str());
 }catch(...){if(probe.gate)probe.gate->release();mainOptionsProbe=nullptr;RemoveWindowSubclass(table,MainAppearanceProbe::observe,796);throw;}RemoveWindowSubclass(table,MainAppearanceProbe::observe,796);
}
static void mainOptionsChecks(const fs::path& root){
 Manager manager(root/L"main-options");auto settings=manager.state["Settings"];settings["CloseToTray"]=false;settings["ClipboardMonitor"]=false;settings["DarkMode"]=uiDark;settings["DropBasket"]=false;settings["SuppressProgressDialog"]=true;manager.setSettings(settings);auto job=manager.add("https://example.test/main-options-fixture.bin",utf8((root/L"files").wstring()),"main-options-fixture.bin");MainWindow* frame=nullptr;
 try{frame=new MainWindow(manager);AfxGetApp()->m_pMainWnd=frame;frame->KillTimer(1);frame->ShowWindow(SW_SHOWNOACTIVATE);ToolbarComponentTest::routing(*frame,job);for(const std::string mode:{"initial-cancel","loaded-cancel","accept"})mainOptionsCase(manager,*frame,job,mode);ToolbarComponentTest::cleanup(frame);frame=nullptr;}catch(...){if(frame)ToolbarComponentTest::cleanup(frame);throw;}
}
