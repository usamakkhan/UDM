// Owned-window lookup checks. Reads are held without holding the store mutex;
// old metadata can therefore arrive after a newer edit or an accepted write.
struct LookupGate {
 std::promise<void> entered,released;std::shared_future<void> ready=entered.get_future().share(),wait=released.get_future().share();std::atomic_bool unlocked=false,finished=false;
 void hold(){entered.set_value();if(wait.wait_for(std::chrono::seconds(10))!=std::future_status::ready)throw std::runtime_error("UI lookup gate expired");finished=true;}
 void release(){if(!unlocked.exchange(true))released.set_value();}
};
class LookupCredentials:public FixtureCredentials {
public:
 std::shared_ptr<LookupGate> gate;std::string first;int ordinal=1;bool fail=false;std::map<std::string,int> calls;
 DialCredentialInfo read(const DialEntry& entry)override{
  DialCredentialInfo info;int call;{std::lock_guard<std::mutex> lock(mutex);info=entries.at(entry.phonebook);call=++calls[entry.phonebook];}
  if(entry.phonebook==first&&call==ordinal){gate->hold();if(fail)throw std::runtime_error("Original credential lookup failure");info.userName="stale-result";}return info;
 }
};
struct DialLookupProbe {
 Manager& manager;Options& owner;std::shared_ptr<LookupCredentials> store;std::shared_ptr<LookupGate> provider;std::string mode;int phase=0,settled=0;HWND main=nullptr;std::string failure;ULONGLONG end=GetTickCount64()+20000;bool returnedHeld=false;
 static void select(HWND w,int index){auto choice=GetWindow(child(w,L"Connection:"),GW_HWNDNEXT);SendMessageW(choice,CB_SETCURSEL,index,0);SendMessageW(w,WM_COMMAND,MAKEWPARAM(GetDlgCtrlID(choice),CBN_SELCHANGE),(LPARAM)choice);}
 void close(){phase=99;SendMessageW(main,WM_COMMAND,IDCANCEL,0);}
 void tick(){
  EnumThreadWindows(GetCurrentThreadId(),[](HWND w,LPARAM l)->BOOL{auto p=reinterpret_cast<DialLookupProbe*>(l);if(IsWindowVisible(w)&&windowText(w)==L"UDM Configuration")p->main=w;return TRUE;},reinterpret_cast<LPARAM>(this));
  try{
   if(!failure.empty()){provider->release();store->gate->release();if(main&&IsWindow(main)&&!manager.fileInfoDecisionPending)PostMessageW(main,WM_CLOSE,0,0);return;}
   if(GetTickCount64()>end)throw std::runtime_error("Lookup UI interaction deadline");if(!main||!IsWindow(main))return;
   if(phase==0){
    if(mode=="provider-close"||mode=="save-pending"){
     if(provider->ready.wait_for(std::chrono::seconds(0))!=std::future_status::ready)return;
     expect(!manager.fileInfoDecisionPending&&IsWindowEnabled(child(main,L"OK")),"Options opens with editable General controls while enumeration is held off the UI thread");SendMessageW(child(main,L"Enable browser download capture"),BM_CLICK,0,0);if(mode=="save-pending"){returnedHeld=!provider->unlocked;phase=99;SendMessageW(child(main,L"OK"),BM_CLICK,0,0);return;}dialTab(main);auto choice=GetWindow(child(main,L"Connection:"),GW_HWNDNEXT);expect(!IsWindowEnabled(choice)&&!IsWindowEnabled(editAfter(main,L"User name:")),"Only unavailable connection fields remain disabled during enumeration");RedrawWindow(main,nullptr,nullptr,RDW_INVALIDATE|RDW_ALLCHILDREN|RDW_UPDATENOW);returnedHeld=!provider->unlocked;close();return;
    }
    dialTab(main);
    if(mode=="provider-error"){bool shown=false;for(auto c:children(main,L"Static"))if(windowText(c)==L"Original enumeration failure")shown=true;if(!shown)return;expect(!manager.fileInfoDecisionPending&&!IsWindowEnabled(editAfter(main,L"User name:"))&&!IsWindowEnabled(child(main,L"Connect...")),"Enumeration errors remain inline without blocking the parent dialog or inventing connection metadata");close();return;}
    if(mode=="read-close"||mode=="selection"||mode=="read-error"){
     if(store->gate->ready.wait_for(std::chrono::seconds(0))!=std::future_status::ready)return;
     auto choice=GetWindow(child(main,L"Connection:"),GW_HWNDNEXT);expect(IsWindowEnabled(choice)&&!IsWindowEnabled(editAfter(main,L"User name:"))&&!manager.fileInfoDecisionPending,"A held credential read leaves selection and parent dialog responsive without claiming the save guard");
     if(mode=="read-close"){returnedHeld=!store->gate->unlocked;close();return;}
     KillTimer(main,Form::pulseTimer);phase=1;if(mode=="selection")select(main,1);else store->gate->release();return;
    }
    if(!DialReadTestAccess::ready(owner)||!IsWindowEnabled(editAfter(main,L"User name:")))return;
    if(mode=="reopen"){expect(windowText(editAfter(main,L"User name:"))==L"fresh-identity","A reopened Options dialog reads its own fresh connection metadata while the old receiver is held");KillTimer(main,Form::pulseTimer);phase=6;store->gate->release();return;}
    // Warm both entries, then return to cached first credentials while the
    // second first-entry read is held. Do not use the legacy waiting helper.
    phase=2;select(main,1);return;
   }
   if(phase==1){
    if(mode=="read-error"){
     bool shown=false;for(auto c:children(main,L"Static"))if(windowText(c)==L"Original credential lookup failure")shown=true;if(!shown)return;
     expect(!IsWindowEnabled(editAfter(main,L"User name:"))&&IsWindowEnabled(child(main,L"Connect...")),"An original read error is shown inline while Windows Connect remains available for recovery");phase=7;select(main,1);return;
    }
    if(windowText(editAfter(main,L"User name:"))!=L"bob"||!IsWindowEnabled(editAfter(main,L"User name:")))return;expect(!store->gate->unlocked,"A newer selection finishes while its older read is still held");phase=6;store->gate->release();return;
   }
   if(phase==2&&DialReadTestAccess::ready(owner)&&windowText(editAfter(main,L"User name:"))==L"bob"){phase=3;select(main,0);return;}
   if(phase==3&&store->gate->ready.wait_for(std::chrono::seconds(0))==std::future_status::ready){
    expect(IsWindowEnabled(editAfter(main,L"User name:")),"Cached credentials stay editable while their refresh is held");KillTimer(main,Form::pulseTimer);SetWindowTextW(editAfter(main,L"User name:"),L"retained-edit");
    if(mode=="accepted"){phase=4;SendMessageW(applyButton(main,false),BM_CLICK,0,0);}else{phase=6;store->gate->release();}return;
   }
   if(phase==4&&!manager.fileInfoDecisionPending&&!IsWindowEnabled(applyButton(main,false))){expect(store->writes.size()==1&&windowText(editAfter(main,L"User name:"))==L"retained-edit","A selected Apply commits newer credentials while an older metadata result is held");phase=6;store->gate->release();return;}
   if(phase==7&&DialReadTestAccess::ready(owner)&&windowText(editAfter(main,L"User name:"))==L"bob"){phase=8;select(main,0);return;}
   if(phase==8&&DialReadTestAccess::ready(owner)&&windowText(editAfter(main,L"User name:"))==L"alice"){expect(IsWindowEnabled(editAfter(main,L"User name:")),"Reselecting retries a failed credential read and restores its editor");close();return;}
   if(phase==6&&store->gate->finished&&++settled>=5){
    if(mode=="selection")expect(windowText(editAfter(main,L"User name:"))==L"bob","Late metadata from an older selection cannot replace the selected connection");
    else if(mode=="reopen")expect(windowText(editAfter(main,L"User name:"))==L"fresh-identity","A late result from the destroyed dialog cannot overwrite its reopened replacement");
    else expect(windowText(editAfter(main,L"User name:"))==L"retained-edit"&&(IsWindowEnabled(applyButton(main,false))!=FALSE)==(mode!="accepted"),"Late refresh cannot overwrite an unsaved edit or a newer accepted credential snapshot");
    close();
   }
  }catch(const std::exception& e){if(failure.empty()){failure=e.what();atomicText(manager.root/L"lookup-failure.json",Json{{"mode",mode},{"phase",phase},{"error",failure}}.dump(2),false);}provider->release();store->gate->release();}
 }
};
static DialLookupProbe* dialLookupProbe=nullptr;
static void CALLBACK driveDialLookup(HWND,UINT,UINT_PTR,DWORD){if(dialLookupProbe)dialLookupProbe->tick();}
static void dialLookupCase(const fs::path& root,const std::string& mode){
 Manager manager(root/wide(mode));DialEntry first{"Same VPN",utf8((manager.root/L"first.pbk").wstring())},second{"Same VPN",utf8((manager.root/L"second.pbk").wstring())};auto settings=manager.state["Settings"];settings["DialEntry"]=first.name;settings["DialPhonebook"]=first.phonebook;settings["DialEnabled"]=mode=="save-pending";manager.setSettings(settings);auto before=manager.snapshot();auto bytes=readText(manager.root/L"state.json");auto providerGate=std::make_shared<LookupGate>();auto readGate=std::make_shared<LookupGate>();auto store=std::make_shared<LookupCredentials>();store->gate=readGate;store->first=first.phonebook;store->ordinal=(mode=="dirty"||mode=="accepted")?2:(mode=="provider-close"||mode=="save-pending"||mode=="provider-error")?0:1;store->fail=mode=="read-error";store->entries[first.phonebook]={"alice",true,false};store->entries[second.phonebook]={"bob",false,false};
 auto run=[&](const std::string& kind){Options dialog(manager,nullptr,store,[gate=providerGate,held=kind=="provider-close"||kind=="save-pending",fail=kind=="provider-error",first,second]{if(fail)throw std::runtime_error("Original enumeration failure");if(held)gate->hold();return std::vector<DialEntry>{first,second};});DialLookupProbe probe{manager,dialog,store,providerGate,kind};dialLookupProbe=&probe;auto timer=SetTimer(nullptr,0,20,driveDialLookup);dialog.DoModal();KillTimer(nullptr,timer);dialLookupProbe=nullptr;if(!probe.failure.empty())throw std::runtime_error(probe.failure);return probe.returnedHeld;};
 try{bool held=run(mode);if(mode=="provider-close"||mode=="save-pending")expect(held&&!providerGate->unlocked,"Closing Options returns before the blocked enumeration is released");if(mode=="read-close"){expect(held&&!readGate->unlocked,"Closing Options returns before its blocked credential read is released");{std::lock_guard<std::mutex> lock(store->mutex);store->entries[first.phonebook].userName="fresh-identity";}run("reopen");}
 }catch(...){providerGate->release();readGate->release();throw;}
 providerGate->release();readGate->release();auto end=GetTickCount64()+5000;while(store.use_count()!=1&&GetTickCount64()<end)Sleep(1);expect(store.use_count()==1,"Closed read receivers release their owned stores after outstanding calls return");if(mode=="save-pending"){auto after=manager.snapshot();expect(!yes(after["Settings"],"BrowserCaptureEnabled",true)&&yes(after["Settings"],"DialEnabled")&&str(after["Settings"],"DialEntry")==first.name&&str(after["Settings"],"DialPhonebook")==first.phonebook,"Saving General settings while enumeration is held preserves the enabled stored connection identity");Manager reopened(manager.root);expect(!yes(reopened.state["Settings"],"BrowserCaptureEnabled",true)&&str(reopened.state["Settings"],"DialPhonebook")==first.phonebook,"Settings accepted during a pending advisory read survive reopening");}else expect(manager.snapshot()==before&&readText(manager.root/L"state.json")==bytes,"Lookup, reselect, edit and parent Cancel leave UDM catalog state and bytes unchanged");expect(store->writes.size()==(mode=="accepted"?1:0),"Advisory reads never write credentials; only explicit Apply commits one change");manager.stop();
}
static void dialReadChecks(const fs::path& root){for(const std::string mode:{"provider-close","save-pending","provider-error","read-close","selection","dirty","accepted","read-error"})dialLookupCase(root/L"reads",mode);}
