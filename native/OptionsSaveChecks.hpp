// Owned Options windows and isolated stores only; no installed app automation.
struct CatalogCheckpointTestAccess {static void observe(Manager& m,std::function<void(const char*)> f){Lock lock(m.mutex);m.catalogCheckpoint=std::move(f);}};
struct OptionsSaveTestAccess {
 static void services(Options& o,std::function<bool()> read,std::function<void(bool)> write,std::function<void()> restart){o.readStartup=std::move(read);o.writeStartup=std::move(write);o.restartHosts=std::move(restart);}
 static void connection(Options& o,const DialEntry& e){o.dialCredentials->load(e);DialReadTestAccess::wait(o.GetSafeHwnd());}
};
struct OptionsFixtureStore:DialCredentialStore {
 std::mutex mutex;std::map<std::string,DialCredentialInfo> values{{"C:/UDM-Fixture/first.pbk",{"old-first",false,false}},{"C:/UDM-Fixture/second.pbk",{"old-second",false,false}}};std::vector<std::string> writes;bool failSecond=true;DWORD worker=0;
 DialCredentialInfo read(const DialEntry& e)override{std::lock_guard<std::mutex> lock(mutex);return values.at(e.phonebook);}
 void write(const DialEntry& e,const DialCredentialChange& c)override{std::lock_guard<std::mutex> lock(mutex);worker=GetCurrentThreadId();if(e.phonebook=="C:/UDM-Fixture/second.pbk"&&failSecond){failSecond=false;throw std::runtime_error("Forced second credential failure");}writes.push_back(e.phonebook);values[e.phonebook]={c.userName,false,false};}
};
struct OptionsSaveGate {
 Manager& m;bool fail;std::atomic_int saves=0;std::promise<void> entered,released;std::shared_future<void> ready=entered.get_future().share(),wait=released.get_future().share();std::atomic_bool unlocked=false;DWORD worker=0;
 OptionsSaveGate(Manager& manager,bool failure):m(manager),fail(failure){CatalogCheckpointTestAccess::observe(m,[this](const char* s){if(std::string(s)=="temporary-flushed"){worker=GetCurrentThreadId();if(saves.fetch_add(1)==0){entered.set_value();if(wait.wait_for(std::chrono::seconds(10))!=std::future_status::ready)throw std::runtime_error("Options save gate expired");if(fail)throw std::runtime_error("Forced Options catalog failure");}}});}
 void unlock(){if(!unlocked.exchange(true))released.set_value();}~OptionsSaveGate(){unlock();CatalogCheckpointTestAccess::observe(m,{});}
};
struct OptionsModelHolder {
 std::promise<void> entered,released;std::future<void> ready=entered.get_future();std::atomic_bool finished=false;Json current;std::thread thread;
 OptionsModelHolder(Manager& m):thread([&,end=released.get_future()]{Lock lock(m.mutex);m.state["Settings"]["Connections"]=5;m.state["Settings"]["UserAgent"]="Concurrent Options owner";current=m.state["Settings"];entered.set_value();end.wait_for(std::chrono::seconds(10));}){}
 void release(){if(!finished.exchange(true))released.set_value();if(thread.joinable())thread.join();}~OptionsModelHolder(){release();}
};
static void optionsTab(HWND w,int index){auto tab=children(w,L"SysTabControl32").at(0);TabCtrl_SetCurSel(tab,index);NMHDR n{tab,(UINT_PTR)GetDlgCtrlID(tab),TCN_SELCHANGE};SendMessageW(w,WM_NOTIFY,n.idFrom,(LPARAM)&n);}
struct OptionsSaveProbe {
 Manager& m;Options& dialog;OptionsSaveGate& gate;std::shared_ptr<OptionsFixtureStore> store;std::string mode;int phase=0,errors=0,restarts=0,startupWrites=0;bool startup=false;std::string failure;std::unique_ptr<OptionsModelHolder> holder;HWND main=nullptr;ULONGLONG end=GetTickCount64()+20000;DWORD ui=GetCurrentThreadId();
 void tick(){
  HWND notice=nullptr;struct Windows{OptionsSaveProbe* p;HWND* notice;} context{this,&notice};EnumThreadWindows(GetCurrentThreadId(),[](HWND w,LPARAM l)->BOOL{auto& c=*reinterpret_cast<Windows*>(l);if(IsWindowVisible(w)){auto title=windowText(w);if(title==L"UDM Configuration")c.p->main=w;else if(title==L"UDM")*c.notice=w;}return TRUE;},reinterpret_cast<LPARAM>(&context));
  try{
   if(notice){++errors;expect(m.fileInfoDecisionPending,"Save error keeps other modal decisions deferred until acknowledgement");auto expected=mode=="credentials"?L"1 connection credential update(s) were saved. The remaining change failed: Forced second credential failure":mode=="startup"?L"Forced Options startup failure":L"Forced Options catalog failure";child(notice,expected);expect(restarts==0,"Failed Options save never restarts browser hosts");{Lock lock(m.mutex);expect(m.state["Settings"]==holder->current,"Failure restores the complete current settings snapshot");}SendMessageW(child(notice,L"OK"),BM_CLICK,0,0);return;}
   if(GetTickCount64()>end)throw std::runtime_error("Options save interaction deadline");if(!main||!IsWindow(main))return;
   if(!failure.empty()){if(holder)holder->release();gate.unlock();if(!m.fileInfoDecisionPending)PostMessageW(main,WM_CLOSE,0,0);return;}
   if(phase==0){if(!DialReadTestAccess::ready(dialog))return;
    if(mode=="cancel"){phase=99;SendMessageW(main,WM_COMMAND,IDCANCEL,0);return;}
    if(mode=="credentials"){
     optionsTab(main,7);SetWindowTextW(GetWindow(child(main,L"User name:"),GW_HWNDNEXT),L"new-first");OptionsSaveTestAccess::connection(dialog,DialEntry{"Demo","C:/UDM-Fixture/second.pbk"});SetWindowTextW(GetWindow(child(main,L"User name:"),GW_HWNDNEXT),L"new-second");optionsTab(main,0);
    }
    SendMessageW(child(main,L"Enable browser download capture"),BM_CLICK,0,0);
    if(mode=="startup")SendMessageW(child(main,L"Launch UDM on startup"),BM_CLICK,0,0);
    holder=std::make_unique<OptionsModelHolder>(m);expect(holder->ready.wait_for(std::chrono::seconds(2))==std::future_status::ready,"Competing settings owner acquires model before Options save");phase=1;SendMessageW(child(main,mode=="restart"?L"Restart":L"OK"),BM_CLICK,0,0);return;
   }
   if(phase==1&&m.fileInfoDecisionPending){expect(!IsWindowEnabled(child(main,L"OK"))&&!IsWindowEnabled(child(main,L"Restart")),"Options OK returns with all editing frozen while worker waits for model");SendMessageW(main,WM_COMMAND,IDOK,0);SendMessageW(main,WM_CLOSE,0,0);SendMessageW(child(main,L"Restart"),BM_CLICK,0,0);RedrawWindow(main,nullptr,nullptr,RDW_INVALIDATE|RDW_ALLCHILDREN|RDW_UPDATENOW);expect(IsWindow(main)&&gate.saves==0&&restarts==0,"Pending Options repaints and rejects duplicate save, restart and close");if(mode=="startup-external")startup=true;phase=2;holder->release();return;}
   if(phase==2&&gate.ready.wait_for(std::chrono::seconds(0))==std::future_status::ready){expect(m.fileInfoDecisionPending&&gate.saves==1&&gate.worker!=ui,"Actual catalog publication runs off the UI thread");KillTimer(main,Form::pulseTimer);phase=3;gate.unlock();return;}
   if(phase==3&&errors==1&&!m.fileInfoDecisionPending&&IsWindowEnabled(child(main,L"OK"))){expect(SendMessageW(child(main,L"Enable browser download capture"),BM_GETCHECK,0,0)==BST_UNCHECKED,"Failed save keeps changed controls for retry");phase=4;SendMessageW(child(main,L"OK"),BM_CLICK,0,0);return;}
   if(phase==3&&mode=="restart"&&restarts==1&&!m.fileInfoDecisionPending){expect(IsWindowEnabled(child(main,L"OK")),"Successful Restart reloads and re-enables the same Options window");phase=99;SendMessageW(main,WM_COMMAND,IDCANCEL,0);}
  }catch(const std::exception& e){failure=e.what();if(holder)holder->release();gate.unlock();if(notice)PostMessageW(notice,WM_CLOSE,0,0);}
 }
};
static OptionsSaveProbe* optionsProbe=nullptr;
static void CALLBACK driveOptionsSave(HWND,UINT,UINT_PTR,DWORD){if(optionsProbe)optionsProbe->tick();}
static void optionsSaveCase(const fs::path& root,const std::string& mode){
 Manager m(root/wide(mode));auto settings=m.state["Settings"];settings["RunAtLogin"]=false;settings["DialEntry"]="Demo";settings["DialPhonebook"]="C:/UDM-Fixture/first.pbk";m.setSettings(settings);auto bytes=readText(m.root/L"state.json");auto store=std::make_shared<OptionsFixtureStore>();Options dialog(m,nullptr,store,[mode]{return mode=="credentials"?std::vector<DialEntry>{{"Demo","C:/UDM-Fixture/first.pbk"},{"Demo","C:/UDM-Fixture/second.pbk"}}:std::vector<DialEntry>{};});
 OptionsSaveGate gate(m,mode=="failure");OptionsSaveProbe probe{m,dialog,gate,store,mode};bool failStartup=true;OptionsSaveTestAccess::services(dialog,[&]{return probe.startup;},[&](bool value){++probe.startupWrites;if(mode=="startup"&&failStartup){failStartup=false;throw std::runtime_error("Forced Options startup failure");}probe.startup=value;},[&]{++probe.restarts;expect(!m.fileInfoDecisionPending,"Browser host restart waits until successful commit is consumed");});
 optionsProbe=&probe;auto timer=SetTimer(nullptr,0,20,driveOptionsSave);dialog.DoModal();KillTimer(nullptr,timer);optionsProbe=nullptr;if(probe.holder)probe.holder->release();if(!probe.failure.empty())throw std::runtime_error(probe.failure);
 if(mode=="cancel"){expect(gate.saves==0&&readText(m.root/L"state.json")==bytes,"Cancel before save leaves catalog bytes unchanged");}
 else{expect(!m.fileInfoDecisionPending&&!yes(m.state["Settings"],"BrowserCaptureEnabled",true),"Accepted Options choices commit and release the shared decision guard");expect(num(m.state["Settings"],"Connections")==5&&str(m.state["Settings"],"UserAgent")=="Concurrent Options owner","Options dirty merge preserves settings changed while its window was open");expect(probe.errors==((mode=="failure"||mode=="credentials"||mode=="startup")?1:0),"Only the expected original failure is reported");expect(probe.restarts==(mode=="restart"?1:0),"Browser hosts restart exactly once only for successful Restart");Manager reopened(m.root);expect(reopened.state["Settings"]==m.state["Settings"],"Committed merged settings survive reopening");
  if(mode=="credentials"){expect(store->writes==std::vector<std::string>{"C:/UDM-Fixture/first.pbk","C:/UDM-Fixture/second.pbk"}&&store->worker!=probe.ui,"Partial credential success is acknowledged; retry writes only remaining credentials off UI thread");expect(store->values["C:/UDM-Fixture/first.pbk"].userName=="new-first"&&store->values["C:/UDM-Fixture/second.pbk"].userName=="new-second","Both isolated connection identities retain their own accepted values");}
  if(mode=="startup-external")expect(probe.startup&&probe.startupWrites==0&&yes(m.state["Settings"],"RunAtLogin"),"Unchanged startup control preserves the current external startup state");
  if(mode=="startup")expect(probe.startup&&probe.startupWrites==2&&yes(m.state["Settings"],"RunAtLogin"),"Startup failure retains intent and succeeds only on explicit retry");
 }
 CatalogCheckpointTestAccess::observe(m,{});m.stop();
}
static void optionsSaveChecks(const fs::path& root){for(const std::string mode:{"cancel","success","failure","restart","credentials","startup","startup-external"})optionsSaveCase(root/L"save",mode);}
