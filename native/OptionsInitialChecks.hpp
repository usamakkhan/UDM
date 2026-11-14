// Exercises only this fixture's Options windows, owned stores and isolated catalog.
struct OptionsInitialTestAccess {
 static void connections(Options& o,std::function<std::vector<DialEntry>()> read){o.dialEntriesProvider=std::move(read);}
 static HWND status(const Options& o){return o.startupStatus?o.startupStatus->GetSafeHwnd():nullptr;}
 static HWND retry(const Options& o){return o.startupRetry?o.startupRetry->GetSafeHwnd():nullptr;}
 static bool loaded(const Options& o){return o.settingsReady;}
 static bool startupPending(const Options& o){return o.startupLookup.pending();}
 static void services(Options& o,std::function<bool()> initial,std::function<bool()> current,std::function<void(bool)> write){o.startupInitialRead=std::move(initial);o.readStartup=std::move(current);o.writeStartup=std::move(write);}
};
struct InitialModelGate {
 std::promise<void> entered,released;std::shared_future<void> ready=entered.get_future().share();std::atomic_bool unlocked=false;std::thread thread;
 InitialModelGate(Manager& manager):thread([&,wait=released.get_future()]{Lock lock(manager.mutex);entered.set_value();wait.wait_for(std::chrono::seconds(15));}){}
 void release(){if(!unlocked.exchange(true))released.set_value();if(thread.joinable())thread.join();}~InitialModelGate(){release();}
};
struct InitialStartupSource {
 std::string mode;std::promise<void> entered,released;std::shared_future<void> ready=entered.get_future().share(),wait=released.get_future().share();std::atomic_bool unlocked=false,returned=false,actual=true,apartment=false;std::atomic_int calls=0,writes=0;DWORD ui=GetCurrentThreadId();std::atomic<DWORD> worker=0;
 explicit InitialStartupSource(std::string value):mode(std::move(value)){}
 bool initial(){int n=++calls;worker=GetCurrentThreadId();APTTYPE type;APTTYPEQUALIFIER qualifier;apartment=SUCCEEDED(CoGetApartmentType(&type,&qualifier))&&(type==APTTYPE_STA||type==APTTYPE_MAINSTA);if(n==1){entered.set_value();if(mode=="startup-cancel"||mode=="startup-save"){if(wait.wait_for(std::chrono::seconds(15))!=std::future_status::ready)throw std::runtime_error("Startup gate expired");}if(mode=="startup-error"||mode=="startup-retry"){returned=true;throw std::runtime_error("Original startup metadata failure");}}returned=true;return actual;}
 void release(){if(!unlocked.exchange(true))released.set_value();}
 ~InitialStartupSource(){release();}
};
struct OptionsInitialProbe {
 Options& dialog;std::string mode;std::shared_ptr<InitialStartupSource> source;InitialModelGate* model=nullptr;HWND main=nullptr;int phase=0;std::string failure;ULONGLONG deadline=GetTickCount64()+20000;
 void tick(){
  EnumThreadWindows(GetCurrentThreadId(),[](HWND w,LPARAM p)->BOOL{if(IsWindowVisible(w)&&windowText(w)==L"UDM Configuration")reinterpret_cast<OptionsInitialProbe*>(p)->main=w;return TRUE;},reinterpret_cast<LPARAM>(this));
  try{
   if(GetTickCount64()>deadline)throw std::runtime_error("Initial Options interaction deadline");if(!main||!IsWindow(main))return;if(!failure.empty()){source->release();if(model)model->release();PostMessageW(main,WM_CLOSE,0,0);return;}
   if(mode=="model-cancel"||mode=="model-release"){
    if(phase==0){expect(!OptionsInitialTestAccess::loaded(dialog)&&!IsWindowEnabled(child(main,L"OK")),"Options shows its shell and disables save while the catalog lock is held");child(main,L"Loading configuration... Close this window to cancel.");expect(IsWindowEnabled(child(main,L"Help")),"Loading Options remains responsive with Help and window Close available");SendMessageW(main,WM_COMMAND,IDOK,0);RedrawWindow(main,nullptr,nullptr,RDW_INVALIDATE|RDW_UPDATENOW|RDW_ALLCHILDREN);expect(IsWindow(main)&&!OptionsInitialTestAccess::loaded(dialog)&&!model->unlocked,"Premature OK and painting never wait for the held catalog lock");capture(main,L"initial-options-pending.png");phase=1;if(mode=="model-cancel")SendMessageW(main,WM_COMMAND,IDCANCEL,0);else model->release();return;}
    if(phase==1&&OptionsInitialTestAccess::loaded(dialog)&&!OptionsInitialTestAccess::startupPending(dialog)){expect(IsWindowEnabled(child(main,L"OK"))&&IsWindowVisible(child(main,L"Browser/System Integration")),"Settings pages appear after the model becomes available");bool tabBeforeSave=false;auto tab=children(main,L"SysTabControl32").at(0);for(HWND c=GetWindow(main,GW_CHILD);c;c=GetWindow(c,GW_HWNDNEXT)){if(c==tab)tabBeforeSave=true;if(c==child(main,L"OK"))break;}expect(tabBeforeSave,"Late-created pages retain tab navigation order before OK and Help");optionsTab(main,1);HWND edit=nullptr;for(auto c:children(main,L"Edit"))if(IsWindowVisible(c)){edit=c;break;}DWORD_PTR data=0;expect(edit&&GetWindowSubclass(edit,themeEditMenu,1,&data)&&data==(DWORD_PTR)main,"Late-created Options editors receive their themed edit menu subclass");optionsTab(main,0);capture(main,L"initial-options-loaded.png");phase=99;SendMessageW(main,WM_COMMAND,IDCANCEL,0);}return;
   }
   if(!OptionsInitialTestAccess::loaded(dialog)||source->ready.wait_for(std::chrono::seconds(0))!=std::future_status::ready)return;
   if(phase==0){expect(bounds(OptionsInitialTestAccess::status(dialog)).right<=bounds(OptionsInitialTestAccess::retry(dialog)).left&&bounds(OptionsInitialTestAccess::retry(dialog)).right<=bounds(child(main,L"OK")).left,"Startup status and Retry fit left of OK without covering any settings page");}
   if(phase==0&&(mode=="startup-cancel"||mode=="startup-save")){
    expect(OptionsInitialTestAccess::startupPending(dialog)&&!IsWindowEnabled(child(main,L"Launch UDM on startup")),"Only the startup choice waits for held Windows metadata");expect(IsWindowEnabled(child(main,L"Enable browser download capture"))&&IsWindowEnabled(child(main,L"OK")),"Other Options edits and save remain usable while startup is held");expect(source->worker!=source->ui&&source->apartment,"Windows startup metadata runs off the UI thread in an initialized STA");capture(main,L"startup-check-pending.png");SendMessageW(child(main,L"Enable browser download capture"),BM_CLICK,0,0);phase=99;SendMessageW(main,WM_COMMAND,mode=="startup-save"?IDOK:IDCANCEL,0);return;
   }
   if(OptionsInitialTestAccess::startupPending(dialog))return;
   if(phase==0&&(mode=="startup-error"||mode=="startup-retry")){
    child(main,L"Original startup metadata failure");expect(IsWindowEnabled(child(main,L"Launch UDM on startup"))&&IsWindowVisible(child(main,L"Retry")),"Startup read error stays inline and permits explicit choice or retry");SendMessageW(child(main,L"Enable browser download capture"),BM_CLICK,0,0);
    if(mode=="startup-retry"){KillTimer(main,Form::pulseTimer);phase=1;SendMessageW(child(main,L"Retry"),BM_CLICK,0,0);return;}
    SendMessageW(child(main,L"Launch UDM on startup"),BM_CLICK,0,0);SendMessageW(child(main,L"Launch UDM on startup"),BM_CLICK,0,0);expect(SendMessageW(child(main,L"Launch UDM on startup"),BM_GETCHECK,0,0)==BST_UNCHECKED,"Explicit startup intent can return to the stored value after metadata failure");phase=99;SendMessageW(main,WM_COMMAND,IDOK,0);return;
   }
   if(phase==1&&mode=="startup-retry"){
    expect(source->calls==2&&SendMessageW(child(main,L"Launch UDM on startup"),BM_GETCHECK,0,0)==BST_CHECKED,"Posted startup retry applies the fresh Windows value without modal polling");expect(SendMessageW(child(main,L"Enable browser download capture"),BM_GETCHECK,0,0)==BST_UNCHECKED&&!IsWindowVisible(OptionsInitialTestAccess::retry(dialog)),"Startup retry preserves other unfinished controls and hides its retry action");phase=99;SendMessageW(main,WM_COMMAND,IDCANCEL,0);return;
   }
   if(phase==0&&mode=="startup-success"){
    expect(SendMessageW(child(main,L"Launch UDM on startup"),BM_GETCHECK,0,0)==BST_CHECKED&&IsWindowEnabled(child(main,L"Launch UDM on startup")),"Successful advisory read presents the actual Windows startup choice");SendMessageW(child(main,L"Launch UDM on startup"),BM_CLICK,0,0);phase=99;SendMessageW(main,WM_COMMAND,IDOK,0);return;
   }
   if(phase==0&&mode=="fresh"){expect(SendMessageW(child(main,L"Launch UDM on startup"),BM_GETCHECK,0,0)==BST_UNCHECKED,"Reopened Options has independent startup metadata while an old provider is held");phase=99;SendMessageW(main,WM_COMMAND,IDCANCEL,0);}
  }catch(const std::exception& e){failure=e.what();source->release();if(model)model->release();PostMessageW(main,WM_CLOSE,0,0);}
 }
};
static OptionsInitialProbe* initialOptionsProbe=nullptr;
static void CALLBACK driveInitialOptions(HWND,UINT,UINT_PTR,DWORD){if(initialOptionsProbe)initialOptionsProbe->tick();}
static void runInitialOptions(Options& dialog,OptionsInitialProbe& probe){initialOptionsProbe=&probe;auto timer=SetTimer(nullptr,0,20,driveInitialOptions);dialog.DoModal();KillTimer(nullptr,timer);initialOptionsProbe=nullptr;if(!probe.failure.empty())throw std::runtime_error(probe.failure);}
static void initialOptionsCase(const fs::path& root,const std::string& mode){
 Manager manager(root/wide(mode));auto settings=manager.state["Settings"];settings["RunAtLogin"]=false;manager.setSettings(settings);const auto before=manager.snapshot();const auto bytes=readText(manager.root/L"state.json");auto source=std::make_shared<InitialStartupSource>(mode);std::unique_ptr<InitialModelGate> model;if(mode=="model-cancel"||mode=="model-release"){model=std::make_unique<InitialModelGate>(manager);expect(model->ready.wait_for(std::chrono::seconds(2))==std::future_status::ready,"Competing owner holds the actual Manager mutex before Options construction");}
 Options dialog(manager,nullptr,{},[]{return std::vector<DialEntry>{};});expect(!model||!model->unlocked,"Options construction does not acquire or wait for Manager settings");OptionsInitialTestAccess::services(dialog,[source]{return source->initial();},[source]{return source->actual.load();},[source](bool value){++source->writes;source->actual=value;});OptionsInitialProbe probe{dialog,mode,source,model.get()};runInitialOptions(dialog,probe);
 if(mode=="model-cancel")expect(!model->unlocked,"Closing loading Options returns before the catalog lock is released");if(model)model->release();
 if(mode=="startup-cancel"||mode=="startup-save")expect(!source->returned&&!source->unlocked,"Closing or saving Options returns before its held advisory startup read");
 if(mode=="startup-cancel"){
  auto fresh=std::make_shared<InitialStartupSource>("fresh");fresh->actual=false;Options reopened(manager,nullptr,{},[]{return std::vector<DialEntry>{};});OptionsInitialTestAccess::services(reopened,[fresh]{return fresh->initial();},[fresh]{return fresh->actual.load();},[fresh](bool v){++fresh->writes;fresh->actual=v;});OptionsInitialProbe follow{reopened,"fresh",fresh};runInitialOptions(reopened,follow);expect(!source->returned,"A reopened dialog finishes while the old advisory call is still held");
 }
 source->release();auto end=GetTickCount64()+3000;if(mode=="startup-cancel"||mode=="startup-save")while(!source->returned&&GetTickCount64()<end)Sleep(1);expect(mode=="model-cancel"||source->returned,"Owned startup provider returns after release without retaining a dialog reference");
 bool saved=mode=="startup-save"||mode=="startup-success"||mode=="startup-error";
 if(!saved)expect(manager.snapshot()==before&&readText(manager.root/L"state.json")==bytes&&source->writes==0,"Closing initial Options or cancelling a startup retry writes no catalog or Windows setting");
 else{Manager reopened(manager.root);expect(reopened.state["Settings"]==manager.state["Settings"],"Startup-aware Options save survives reopening");if(mode=="startup-save")expect(yes(manager.state["Settings"],"RunAtLogin")&&source->writes==0&&!yes(manager.state["Settings"],"BrowserCaptureEnabled",true),"Saving other settings during held metadata preserves the current external startup state");else expect(!yes(manager.state["Settings"],"RunAtLogin")&&!source->actual&&source->writes==1,"An explicit startup choice overrides current Windows state exactly once, including after a failed advisory read");}
 manager.stop();
}
static void initialOptionsChecks(const fs::path& root){for(const std::string mode:{"model-cancel","model-release","startup-cancel","startup-save","startup-success","startup-error","startup-retry"})initialOptionsCase(root/L"initial",mode);}
