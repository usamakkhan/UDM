// Included in the owned preferences fixture; never operates other processes.
struct CatalogCheckpointTestAccess {static void observe(Manager& m,std::function<void(const char*)> f){Lock lock(m.mutex);m.catalogCheckpoint=std::move(f);}};
struct ProgressMenuTestAccess {static void customize(Progress& p){p.dispatchMenuChoice(7,p.displayedMenu);}static Json prefs(Progress& p){return p.displayedProgressPreferences;}};
class PreferencesOwner:public CFrameWnd {void PostNcDestroy()override{}};
struct PreferencesGate {
 Manager& manager;bool fail;std::promise<void> entered,released;std::shared_future<void> ready=entered.get_future().share(),wait=released.get_future().share();std::atomic_bool unlocked=false;std::atomic_int saves=0;
 PreferencesGate(Manager& m,bool f):manager(m),fail(f){CatalogCheckpointTestAccess::observe(m,[this](const char* s){if(std::string(s)=="temporary-flushed"&&saves.fetch_add(1)==0){entered.set_value();if(wait.wait_for(std::chrono::seconds(8))!=std::future_status::ready)throw std::runtime_error("Preference gate expired");if(fail)throw std::runtime_error("Forced progress preference save failure");}});}
 void unlock(){if(!unlocked.exchange(true))released.set_value();}~PreferencesGate(){unlock();CatalogCheckpointTestAccess::observe(manager,{});}
};
struct PreferencesHolder {
 std::promise<void> entered,released;std::future<void> ready=entered.get_future();std::atomic_bool finished=false,expired=false;Json current;std::thread thread;
 PreferencesHolder(Manager& m,bool progressChanged):thread([&,progressChanged,end=released.get_future()]{Lock lock(m.mutex);m.state["Settings"]["Connections"]=5;m.state["Settings"]["UserAgent"]="Concurrent preference owner";if(progressChanged){m.state["Settings"]["ProgressInformationTab"]=true;m.state["Settings"]["QueueProgressMinimized"]=false;}current=m.state["Settings"];entered.set_value();expired=end.wait_for(std::chrono::seconds(8))!=std::future_status::ready;}){}
 void release(){if(!finished.exchange(true))released.set_value();if(thread.joinable())thread.join();}~PreferencesHolder(){release();}
};
struct PreferencesProbe {
 Manager& manager;PreferencesGate& gate;PreferencesHolder& holder;std::string kind;Json* editor;Json beforeEditor;HWND main=nullptr;int phase=0,errors=0;bool enteredBlocked=false;std::string error;ULONGLONG end=GetTickCount64()+15000;
 void tick(){
  HWND more=nullptr,notice=nullptr;struct Windows{PreferencesProbe* p;HWND* more;HWND* notice;} windows{this,&more,&notice};
  EnumThreadWindows(GetCurrentThreadId(),[](HWND w,LPARAM v)->BOOL{auto& x=*reinterpret_cast<Windows*>(v);auto name=windowText(w);if(!IsWindowVisible(w))return TRUE;if(name==L"Customize download progress dialog")x.p->main=w;else if(name==L"Additional progress settings")*x.more=w;else if(name==L"UDM")*x.notice=w;return TRUE;},reinterpret_cast<LPARAM>(&windows));
  try{
   if(notice){++errors;expect(child(notice,L"Forced progress preference save failure")!=nullptr,"Preference failure retains its original backend message");{Lock lock(manager.mutex);expect(manager.state["Settings"]==holder.current,"Failed preference publication restores all current settings before acknowledgement");}SendMessageW(child(notice,L"OK"),BM_CLICK,0,0);return;}
   if(GetTickCount64()>end)throw std::runtime_error("Preferences interaction deadline");
   if(!error.empty()){holder.release();gate.unlock();if(main&&IsWindow(main)&&!manager.fileInfoDecisionPending)PostMessageW(main,WM_CLOSE,0,0);return;}
   if(!main||!IsWindow(main))return;
   if(phase==0){
    enteredBlocked=!holder.finished&&!holder.expired;expect(enteredBlocked,"Preferences dialog opens from owned values while another thread holds the model");
    if(kind=="noop"){phase=99;SendMessageW(child(main,L"OK"),BM_CLICK,0,0);return;}
    selectCombo(main,children(main,L"ComboBox").at(0),1);
    for(const auto* label:{L"Show \"Speed Limiter\" tab",L"Show \"Options on completion\" tab",L"Show \"Hide tab\" buttons"})SendMessageW(child(main,label),BM_SETCHECK,BST_UNCHECKED,0);
    if(kind=="merge"){phase=2;return;}phase=1;SendMessageW(child(main,L"More..."),BM_CLICK,0,0);return;
   }
   if(phase==1&&more){SendMessageW(child(more,L"Show Information tab"),BM_SETCHECK,BST_CHECKED,0);SendMessageW(child(more,L"Start queue progress windows in the system tray"),BM_SETCHECK,BST_UNCHECKED,0);phase=2;SendMessageW(child(more,kind=="more-cancel"?L"Cancel":L"OK"),BM_CLICK,0,0);return;}
   if(phase==2&&!more&&IsWindowEnabled(main)){phase=3;SendMessageW(child(main,kind=="parent-cancel"?L"Cancel":L"OK"),BM_CLICK,0,0);return;}
   if(phase==3&&manager.fileInfoDecisionPending){
    expect(!holder.finished&&!holder.expired&&!IsWindowEnabled(child(main,L"OK"))&&!IsWindowEnabled(child(main,L"More...")),"OK returns with controls frozen while its worker waits for the held model");
    SendMessageW(main,WM_COMMAND,IDOK,0);SendMessageW(child(main,L"More..."),BM_CLICK,0,0);SendMessageW(main,WM_CLOSE,0,0);RedrawWindow(main,nullptr,nullptr,RDW_INVALIDATE|RDW_ALLCHILDREN|RDW_UPDATENOW);
    expect(IsWindow(main)&&gate.saves==0,"Pending settings repaint and reject duplicate OK, More and Cancel without waiting for the model");holder.release();phase=4;return;
   }
   if(phase==4&&gate.ready.wait_for(std::chrono::seconds(0))==std::future_status::ready){expect(manager.fileInfoDecisionPending&&gate.saves==1,"Real preference publication is held after dispatch");KillTimer(main,Form::pulseTimer);phase=5;gate.unlock();return;}
   if(phase==5&&kind=="failure"&&errors==1&&!manager.fileInfoDecisionPending&&IsWindowEnabled(child(main,L"OK"))){
    expect(windowText(children(main,L"ComboBox").at(0))==L"Small size"&&(!editor||*editor==beforeEditor),"Failed preferences retain the same controls and leave the caller draft uncommitted");phase=6;SendMessageW(child(main,L"OK"),BM_CLICK,0,0);
   }
  }catch(const std::exception& e){error=e.what();holder.release();gate.unlock();if(notice)PostMessageW(notice,WM_CLOSE,0,0);}
 }
};
static PreferencesProbe* preferencesProbe=nullptr;
static void CALLBACK drivePreferences(HWND,UINT,UINT_PTR,DWORD){if(preferencesProbe)preferencesProbe->tick();}
static void preferencesCase(CWnd& owner,const fs::path& root,const std::string& kind,bool cached=false){
 Manager manager(root/wide(cached?"cached-"+kind:kind));auto settings=manager.state["Settings"];settings["DownloadFolder"]=utf8((root/L"files").wstring());manager.setSettings(settings);Json editor=progressPreferenceValues(settings);const auto before=editor;const auto bytes=readText(manager.root/L"state.json");
 JobPtr job;std::unique_ptr<Progress> view;
 if(cached){job=manager.add("ftp://127.0.0.1:9/preferences.bin",utf8((root/L"files").wstring()),"preferences.bin","Main queue",true);view=std::make_unique<Progress>(manager,job,&owner);expect(view->Create(101,&owner)!=FALSE,"Create actual cached progress customization entry");view->ShowWindow(SW_SHOWNOACTIVATE);KillTimer(view->GetSafeHwnd(),Form::pulseTimer);}
 PreferencesGate gate(manager,kind=="failure");PreferencesHolder holder(manager,kind=="merge");expect(holder.ready.wait_for(std::chrono::seconds(2))==std::future_status::ready,"Competing settings owner holds newer unrelated settings before dialog entry");
 PreferencesProbe probe{manager,gate,holder,kind,cached?nullptr:&editor,before};preferencesProbe=&probe;auto timer=SetTimer(nullptr,0,20,drivePreferences);
 if(cached)ProgressMenuTestAccess::customize(*view);else progressPreferences(&owner,editor,&manager);
 KillTimer(nullptr,timer);preferencesProbe=nullptr;if(!probe.error.empty())throw std::runtime_error(probe.error);
 expect(probe.enteredBlocked&&!holder.expired,"Customization remains responsive through model contention");
 const bool discarded=kind=="parent-cancel"||kind=="noop";
 if(discarded){expect(gate.saves==0&&!manager.fileInfoDecisionPending&&editor==before,"Cancel or unchanged OK performs no settings save and commits no caller draft");if(!cached)expect(readText(manager.root/L"state.json")==bytes,"Cancel or unchanged OK leaves catalog bytes unchanged");}
 holder.release();
 if(!discarded){
  expect(gate.saves==(kind=="failure"?2:1)&&probe.errors==(kind=="failure"?1:0),"Settings save publishes once or explicitly retries once after the original error");
  const auto& p=manager.state["Settings"];expect(str(p,"ProgressStartMode")=="Small size"&&!yes(p,"ProgressSpeedTab",true)&&!yes(p,"ProgressCompletionTab",true)&&!yes(p,"ProgressHideTabButtons",true),"Changed main-dialog choices persist together");
  expect(yes(p,"ProgressInformationTab")== (kind!="more-cancel")&&yes(p,"QueueProgressMinimized",true)==(kind=="more-cancel"),"More acceptance, More cancellation and untouched concurrent progress fields retain their intended values");
  expect(num(p,"Connections")==5&&str(p,"UserAgent")=="Concurrent preference owner","Dirty-field merge preserves newer unrelated settings");
  {Manager reopened(manager.root);expect(progressPreferenceValues(reopened.state["Settings"])==progressPreferenceValues(p)&&num(reopened.state["Settings"],"Connections")==5,"Merged progress and unrelated settings survive reopening");}
 }
 if(view)expect(ProgressMenuTestAccess::prefs(*view)==(discarded?before:progressPreferenceValues(manager.state["Settings"])),"Immediate customization cache reflects committed choices without polling and Cancel does not replace it");
 CatalogCheckpointTestAccess::observe(manager,{});if(view)view->DestroyWindow();manager.stop();
}
static void preferencesValidation(const fs::path& root){
 Manager manager(root/L"validation");auto before=manager.state["Settings"];
 for(const auto& patch:std::vector<Json>{Json{{"Connections",1}},Json{{"ProgressSpeedTab","yes"}},Json{{"ProgressStartMode","invalid"}}}){bool rejected=false;try{applyProgressPreferences(manager,patch);}catch(const std::exception&){rejected=true;}expect(rejected&&manager.state["Settings"]==before,"Invalid progress patches cannot mutate other settings or bypass typed choices");}
 auto thin=progressPreferenceValues(Json{{"ProxySecret",std::string(1024*1024,'x')}});expect(thin.size()==6&&!thin.contains("ProxySecret"),"Cached customization values exclude unrelated protected settings");manager.stop();
}
static void progressPreferencePersistenceChecks(CWnd& owner,const fs::path& root){for(const std::string mode:{"success","failure","more-cancel","parent-cancel","noop","merge"})preferencesCase(owner,root,mode);preferencesCase(owner,root,"parent-cancel",true);preferencesCase(owner,root,"success",true);preferencesValidation(root);}
