// Owned Options transactions only. All Windows/credential services are injected.
static void optionsRollbackChecks(){
 auto j=[](const char* text){return Json::parse(text);};
 auto check=[&](const char* before,const char* applied,const char* current,const char* expected,const char* label){expect(rollbackOptionsCommit(j(before),j(applied),j(current))==j(expected),label);};
 check("{\"x\":1}","{\"x\":2}","{\"x\":2}","{\"x\":1}","Rollback restores this attempt's unchanged scalar contribution");
 check("{\"x\":1}","{\"x\":2}","{\"x\":3}","{\"x\":3}","Rollback preserves a later conflicting scalar writer");
 check("{}","{\"x\":2}","{\"x\":2}","{}","Rollback removes an unchanged newly inserted setting");
 check("{\"x\":1}","{}","{}","{\"x\":1}","Rollback restores a removed setting only while it stays absent");
 check("{\"x\":1}","{}","{\"x\":3}","{\"x\":3}","Rollback preserves a later replacement of a removed setting");
 check("{\"x\":1}","{\"x\":2}","{}","{}","Rollback respects a later removal of an edited setting");
 check("{\"x\":1}","{\"x\":1}","{\"x\":3,\"z\":4}","{\"x\":3,\"z\":4}","Rollback leaves untouched and independently added settings intact");
 check("{\"o\":{\"a\":1,\"b\":2}}","{\"o\":{\"a\":3,\"b\":4}}","{\"o\":{\"a\":9,\"b\":4,\"c\":5}}","{\"o\":{\"a\":9,\"b\":2,\"c\":5}}","Rollback merges nested conflicts without discarding sibling changes");
 check("{}","{\"o\":{\"a\":3}}","{\"o\":{\"a\":3,\"b\":4}}","{\"o\":{\"b\":4}}","Rollback removes only owned members of a newly introduced object");
 check("{\"a\":[1]}","{\"a\":[2]}","{\"a\":[3]}","{\"a\":[3]}","Rollback treats conflicting ordered arrays as independently owned values");
 Json before={{"CategoryPaths",legacyDictionary(j("{\"A\":\"old-a\",\"B\":\"old-b\"}"))}},applied={{"CategoryPaths",legacyDictionary(j("{\"A\":\"saved-a\",\"B\":\"saved-b\"}"))}},current={{"CategoryPaths",legacyDictionary(j("{\"A\":\"live-a\",\"B\":\"saved-b\",\"C\":\"new-c\"}"))}};
 auto restored=rollbackOptionsCommit(before,applied,current);expect(dictionary(restored["CategoryPaths"])==j("{\"A\":\"live-a\",\"B\":\"old-b\",\"C\":\"new-c\"}"),"Rollback preserves independently edited legacy category entries and restores only its own entries");
 bool invalid=false;try{rollbackOptionsCommit(Json::array(),Json::object(),Json::object());}catch(const std::exception&){invalid=true;}expect(invalid,"Rollback rejects malformed nonobject transaction snapshots");
}
struct OptionsExternalSource {
 std::string mode;std::promise<void> entered,released;std::shared_future<void> ready=entered.get_future().share(),wait=released.get_future().share();std::atomic_bool unlocked=false,actual=false;std::atomic_int reads=0,writes=0,credentials=0;std::atomic<DWORD> worker=0;std::atomic_bool apartment=false;
 explicit OptionsExternalSource(std::string value):mode(std::move(value)){}
 void hold(){worker=GetCurrentThreadId();APTTYPE type;APTTYPEQUALIFIER q;apartment=SUCCEEDED(CoGetApartmentType(&type,&q))&&(type==APTTYPE_STA||type==APTTYPE_MAINSTA);entered.set_value();if(wait.wait_for(std::chrono::seconds(15))!=std::future_status::ready)throw std::runtime_error("External settings fixture gate expired");}
 bool read(){int n=++reads;if(n==1&&(mode=="read"||mode=="read-error")){hold();if(mode=="read-error")throw std::runtime_error("Original current Startup read failure");}return actual;}
 void write(bool value){int n=++writes;actual=value;if((mode=="write"||mode=="write-error")&&n==1)hold();if(mode=="rollback"&&n==2)hold();if((mode=="write-error"||mode=="rollback")&&n==1)throw std::runtime_error("Original Startup write failure");}
 void release(){if(!unlocked.exchange(true))released.set_value();}~OptionsExternalSource(){release();}
};
class OptionsExternalCredentials:public DialCredentialStore {
 std::shared_ptr<OptionsExternalSource> source;
public:explicit OptionsExternalCredentials(std::shared_ptr<OptionsExternalSource> value):source(std::move(value)){}
 DialCredentialInfo read(const DialEntry&)override{return {"before",false,false};}
 void write(const DialEntry&,const DialCredentialChange&)override{++source->credentials;source->hold();throw std::runtime_error("Original credential write failure");}
};
struct OptionsExternalProbe {
 Manager& manager;Options& dialog;std::shared_ptr<OptionsExternalSource> source;std::string failure;HWND main=nullptr;int phase=0,errors=0;DWORD ui=GetCurrentThreadId();ULONGLONG end=GetTickCount64()+25000;
 void tick(){
  HWND notice=nullptr;struct Context{OptionsExternalProbe* probe;HWND* notice;} context{this,&notice};EnumThreadWindows(GetCurrentThreadId(),[](HWND w,LPARAM p)->BOOL{auto& c=*reinterpret_cast<Context*>(p);if(IsWindowVisible(w)){auto title=windowText(w);if(title==L"UDM Configuration")c.probe->main=w;else if(title==L"UDM")*c.notice=w;}return TRUE;},reinterpret_cast<LPARAM>(&context));
  try{
   if(!failure.empty()){source->release();if(notice)PostMessageW(notice,WM_CLOSE,0,0);if(main&&IsWindow(main)&&!manager.fileInfoDecisionPending)PostMessageW(main,WM_CLOSE,0,0);return;}
   if(notice){++errors;auto label=source->mode=="credentials"?L"Original credential write failure":source->mode=="read-error"?L"Original current Startup read failure":L"Original Startup write failure";child(notice,label);expect(manager.fileInfoDecisionPending,"External failure keeps the decision guard until original-error acknowledgement");std::unique_lock<std::recursive_mutex> lock(manager.mutex,std::try_to_lock);expect(lock.owns_lock(),"Error acknowledgement can acquire the catalog after external failure rollback");expect(str(manager.state["Settings"],"UserAgent")=="Concurrent external phase"&&num(manager.state["Settings"],"Connections")==6&&yes(manager.state["Settings"],"BrowserCaptureEnabled",true),"Failed external work restores accepted settings without erasing independent current changes");lock.unlock();SendMessageW(child(notice,L"OK"),BM_CLICK,0,0);return;}
   if(GetTickCount64()>end)throw std::runtime_error("External Options interaction deadline");if(!main||!IsWindow(main))return;
   if(phase==0){if(!DialReadTestAccess::ready(dialog))return;SendMessageW(child(main,L"Enable browser download capture"),BM_CLICK,0,0);if(source->mode=="write"||source->mode=="write-error"||source->mode=="rollback")SendMessageW(child(main,L"Launch UDM on startup"),BM_CLICK,0,0);if(source->mode=="credentials"){optionsTab(main,7);SetWindowTextW(GetWindow(child(main,L"User name:"),GW_HWNDNEXT),L"changed");optionsTab(main,0);}phase=1;SendMessageW(child(main,L"OK"),BM_CLICK,0,0);return;}
   if(phase==1&&source->ready.wait_for(std::chrono::seconds(0))==std::future_status::ready){
    expect(manager.fileInfoDecisionPending&&source->worker!=ui&&source->apartment,"Held Windows and credential phases run on the joined STA worker with the decision guard retained");
    std::unique_lock<std::recursive_mutex> lock(manager.mutex,std::try_to_lock);expect(lock.owns_lock(),"Held current Startup read, Startup write, rollback write or credential provider never owns the catalog mutex");auto current=manager.state["Settings"];current["UserAgent"]="Concurrent external phase";current["Connections"]=6;manager.setSettings(current);lock.unlock();expect(!IsWindowEnabled(child(main,L"OK")),"External work keeps accepted Options controls frozen");SendMessageW(main,WM_CLOSE,0,0);SendMessageW(main,WM_COMMAND,IDOK,0);RedrawWindow(main,nullptr,nullptr,RDW_INVALIDATE|RDW_ALLCHILDREN|RDW_UPDATENOW);expect(IsWindow(main),"Held external work repaints while deferring Close and duplicate OK");if(source->mode=="credentials")source->actual=true;KillTimer(main,Form::pulseTimer);phase=2;source->release();return;
   }
   if(phase==2&&errors==1&&!manager.fileInfoDecisionPending){expect(IsWindowEnabled(child(main,L"OK"))&&SendMessageW(child(main,L"Enable browser download capture"),BM_GETCHECK,0,0)==BST_UNCHECKED,"Original external failure preserves editable accepted draft after acknowledgement");phase=3;if(source->mode=="write-error"||source->mode=="read-error")SendMessageW(child(main,L"OK"),BM_CLICK,0,0);else SendMessageW(main,WM_COMMAND,IDCANCEL,0);}
  }catch(const std::exception& e){if(failure.empty())failure=e.what();source->release();}
 }
};
static OptionsExternalProbe* externalOptionsProbe=nullptr;
static void CALLBACK driveOptionsExternal(HWND,UINT,UINT_PTR,DWORD){if(externalOptionsProbe)externalOptionsProbe->tick();}
static void optionsExternalCase(const fs::path& root,const std::string& mode){
 Manager manager(root/wide(mode));auto prefs=manager.state["Settings"];prefs["RunAtLogin"]=false;if(mode=="credentials"){prefs["DialEntry"]="Owned connection";prefs["DialPhonebook"]="C:/UDM-Fixture/external.pbk";}manager.setSettings(prefs);
 auto source=std::make_shared<OptionsExternalSource>(mode);auto store=std::make_shared<OptionsExternalCredentials>(source);Options dialog(manager,nullptr,store,[mode]{return mode=="credentials"?std::vector<DialEntry>{{"Owned connection","C:/UDM-Fixture/external.pbk"}}:std::vector<DialEntry>{};});OptionsInitialTestAccess::services(dialog,[]{return false;},[source]{return source->read();},[source](bool value){source->write(value);});OptionsExternalProbe probe{manager,dialog,source};externalOptionsProbe=&probe;auto timer=SetTimer(nullptr,0,20,driveOptionsExternal);dialog.DoModal();KillTimer(nullptr,timer);externalOptionsProbe=nullptr;source->release();if(!probe.failure.empty())throw std::runtime_error(probe.failure);
 const bool accepted=mode=="read"||mode=="write"||mode=="write-error"||mode=="read-error";expect(!manager.fileInfoDecisionPending&&str(manager.state["Settings"],"UserAgent")=="Concurrent external phase"&&num(manager.state["Settings"],"Connections")==6,"External-phase independent settings survive completion, rollback and explicit retry");expect(yes(manager.state["Settings"],"BrowserCaptureEnabled",true)!=accepted,"Only accepted external transactions retain the user's changed configuration");expect(probe.errors==((mode=="read"||mode=="write")?0:1),"External transaction reports the expected original error once");
 if(mode=="credentials")expect(source->actual&&source->writes==0&&source->reads==1&&source->credentials==1,"Credential failure never rolls back or rewrites untouched external Startup state");else expect(source->actual==(mode=="write"||mode=="write-error"),"Successful Startup choice or owned failed-write compensation reaches its expected Windows state");
 if(mode=="write-error")expect(source->reads==3&&source->writes==3,"Startup failure compensates once and repeats the desired write only after explicit retry");if(mode=="read-error")expect(source->reads==2&&source->writes==0,"Current Startup read failure retries explicitly without touching Windows Startup");Manager reopened(manager.root);expect(reopened.state["Settings"]==manager.state["Settings"],"External-phase merged or rolled-back settings survive catalog reopening");manager.stop();
}
static void optionsExternalChecks(const fs::path& root){optionsRollbackChecks();for(const std::string mode:{"read","write","write-error","rollback","credentials","read-error"})optionsExternalCase(root/L"external",mode);}
