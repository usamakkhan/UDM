// Exercises only this executable's Options dialogs and isolated credential stores.
class HeldDialCredentials:public FixtureCredentials {
 std::promise<void> entered,released;std::shared_future<void> wait=released.get_future().share();std::atomic_bool unlocked=false;bool failFirst;
public:
 std::shared_future<void> ready=entered.get_future().share();std::atomic_int attempts=0;DWORD writer=0;
 explicit HeldDialCredentials(bool failure):failFirst(failure){}
 void write(const DialEntry& entry,const DialCredentialChange& change)override{
  const int attempt=++attempts;writer=GetCurrentThreadId();if(attempt==1){entered.set_value();if(wait.wait_for(std::chrono::seconds(10))!=std::future_status::ready)throw std::runtime_error("Dial Apply gate expired");if(failFirst)throw std::runtime_error("Forced selected credential failure");}
  FixtureCredentials::write(entry,change);
 }
 void unlock(){if(!unlocked.exchange(true))released.set_value();}~HeldDialCredentials(){unlock();}
};
struct DialApplyProbe {
 Manager& manager;std::shared_ptr<HeldDialCredentials> store;std::string mode;int phase=0,errors=0;HWND main=nullptr;std::string failure;ULONGLONG end=GetTickCount64()+18000;DWORD ui=GetCurrentThreadId();
 static void page(HWND w,int index){auto tab=children(w,L"SysTabControl32").at(0);TabCtrl_SetCurSel(tab,index);NMHDR change{tab,(UINT_PTR)GetDlgCtrlID(tab),TCN_SELCHANGE};SendMessageW(w,WM_NOTIFY,change.idFrom,(LPARAM)&change);}
 void tick(){
  HWND notice=nullptr;struct Windows{DialApplyProbe* p;HWND* notice;} context{this,&notice};EnumThreadWindows(GetCurrentThreadId(),[](HWND w,LPARAM l)->BOOL{auto& c=*reinterpret_cast<Windows*>(l);if(IsWindowVisible(w)){auto name=windowText(w);if(name==L"UDM Configuration")c.p->main=w;else if(name==L"UDM")*c.notice=w;}return TRUE;},reinterpret_cast<LPARAM>(&context));
  try{
   if(notice){++errors;child(notice,L"Forced selected credential failure");expect(manager.fileInfoDecisionPending&&store->writes.empty(),"Credential error preserves the original message and shared guard before acknowledgement");SendMessageW(child(notice,L"OK"),BM_CLICK,0,0);return;}
   if(GetTickCount64()>end)throw std::runtime_error("Dial Apply interaction deadline");if(!main||!IsWindow(main))return;
   if(!failure.empty()){store->unlock();if(!manager.fileInfoDecisionPending)PostMessageW(main,WM_CLOSE,0,0);return;}
   if(phase==0){DialReadTestAccess::wait(main);
    dialTab(main);auto choice=GetWindow(child(main,L"Connection:"),GW_HWNDNEXT);
    if(mode=="noop"){expect(!IsWindowEnabled(applyButton(main,false)),"Unchanged selected credentials keep Apply disabled");SendMessageW(applyButton(main,false),BM_CLICK,0,0);expect(store->attempts==0&&!manager.fileInfoDecisionPending,"Unchanged Apply starts no worker and performs no credential write");phase=99;SendMessageW(main,WM_COMMAND,IDCANCEL,0);return;}
    selectCombo(main,choice,1);SetWindowTextW(editAfter(main,L"User name:"),L"OTHER\\invalid\\identity");selectCombo(main,choice,0);
    SetWindowTextW(editAfter(main,L"User name:"),L"DOMAIN\\accepted");SetWindowTextW(editAfter(main,L"Password:"),L"session-fixture");press(main,L"Save password");
    page(main,0);SendMessageW(child(main,L"Enable browser download capture"),BM_CLICK,0,0);page(main,7);
    expect((GetWindowLongW(editAfter(main,L"Password:"),GWL_STYLE)&ES_PASSWORD)!=0,"Credential entry remains masked before asynchronous Apply");phase=1;SendMessageW(applyButton(main,false),BM_CLICK,0,0);return;
   }
   if(phase==1&&store->ready.wait_for(std::chrono::seconds(0))==std::future_status::ready){
    expect(manager.fileInfoDecisionPending&&store->writer!=ui&&store->attempts==1,"Selected connection write runs on a worker while Apply has returned to the modal loop");
    expect(!IsWindowEnabled(editAfter(main,L"User name:"))&&!IsWindowEnabled(child(main,L"OK"))&&!IsWindowEnabled(applyButton(main,false)),"Editing and final Options acceptance defer while the selected credential write is pending");
    SendMessageW(applyButton(main,false),BM_CLICK,0,0);SendMessageW(main,WM_COMMAND,IDOK,0);SendMessageW(main,WM_CLOSE,0,0);RedrawWindow(main,nullptr,nullptr,RDW_INVALIDATE|RDW_ALLCHILDREN|RDW_UPDATENOW);
    expect(IsWindow(main)&&store->attempts==1&&store->writes.empty(),"Held credential save repaints and rejects duplicate Apply, OK and Close");KillTimer(main,Form::pulseTimer);phase=2;store->unlock();return;
   }
   if(phase==2&&mode=="failure"&&errors==1&&!manager.fileInfoDecisionPending){
    expect(IsWindowEnabled(editAfter(main,L"User name:"))&&IsWindowEnabled(applyButton(main,false))&&windowText(editAfter(main,L"User name:"))==L"DOMAIN\\accepted"&&windowText(editAfter(main,L"Password:"))==L"session-fixture","Failed selected save restores editable controls and the same protected credential draft for retry");phase=3;SendMessageW(applyButton(main,false),BM_CLICK,0,0);return;
   }
   if((phase==2||phase==3)&&!manager.fileInfoDecisionPending){
    expect(store->writes.size()==1&&store->attempts==(mode=="failure"?2:1)&&errors==(mode=="failure"?1:0),"Selected Apply commits once or retries once only after explicit failure acknowledgement");
    expect(!IsWindowEnabled(applyButton(main,false))&&windowText(editAfter(main,L"Password:"))==L"********","Successful selected Apply restores the disabled clean Apply button and masked password placeholder");
    const auto& change=store->writes.front().second;expect(change.passwordEdited&&!change.savePassword&&change.protectedPassword!="session-fixture"&&reveal(change.protectedPassword)=="session-fixture","Worker receives the selected connection's Windows-protected session password and save preference");
    auto choice=GetWindow(child(main,L"Connection:"),GW_HWNDNEXT);selectCombo(main,choice,1);expect(windowText(editAfter(main,L"User name:"))==L"OTHER\\invalid\\identity","Selected Apply neither validates nor discards a different connection's unfinished draft");selectCombo(main,choice,0);expect(windowText(editAfter(main,L"User name:"))==L"DOMAIN\\accepted"&&!IsWindowEnabled(applyButton(main,false)),"Returning to the saved connection reloads its committed identity without a dirty draft");
    SendMessageW(applyButton(main,false),BM_CLICK,0,0);expect(!manager.fileInfoDecisionPending&&store->writes.size()==1,"A clean Apply cannot resubmit the accepted credential change");
    page(main,0);expect(SendMessageW(child(main,L"Enable browser download capture"),BM_GETCHECK,0,0)==BST_UNCHECKED,"Credential-only completion retains unrelated unaccepted Options controls");phase=99;SendMessageW(main,WM_COMMAND,IDCANCEL,0);
   }
  }catch(const std::exception& e){failure=e.what();store->unlock();if(notice)PostMessageW(notice,WM_CLOSE,0,0);}
 }
};
static DialApplyProbe* dialApplyProbe=nullptr;
static void CALLBACK driveDialApply(HWND,UINT,UINT_PTR,DWORD){if(dialApplyProbe)dialApplyProbe->tick();}
static void dialApplyCase(const fs::path& root,const std::string& mode){
 Manager manager(root/wide(mode));DialEntry first{"Same VPN",utf8((manager.root/L"first.pbk").wstring())},second{"Same VPN",utf8((manager.root/L"second.pbk").wstring())};auto settings=manager.state["Settings"];settings["DialEntry"]=first.name;settings["DialPhonebook"]=first.phonebook;settings["DialEnabled"]=false;manager.setSettings(settings);auto before=manager.snapshot();auto bytes=readText(manager.root/L"state.json");
 auto store=std::make_shared<HeldDialCredentials>(mode=="failure");store->entries[first.phonebook]={"DOMAIN\\alice",true,false};store->entries[second.phonebook]={"bob",false,false};Options dialog(manager,nullptr,store,[=]{return std::vector<DialEntry>{first,second};});DialApplyProbe probe{manager,store,mode};dialApplyProbe=&probe;auto timer=SetTimer(nullptr,0,20,driveDialApply);dialog.DoModal();KillTimer(nullptr,timer);dialApplyProbe=nullptr;store->unlock();if(!probe.failure.empty())throw std::runtime_error(probe.failure);
 expect(!manager.fileInfoDecisionPending&&manager.snapshot()==before&&readText(manager.root/L"state.json")==bytes,"Credential-only Apply and parent Cancel leave the full catalog and its bytes unchanged");
 if(mode!="noop"){expect(store->entries[first.phonebook].userName=="DOMAIN\\accepted"&&!store->entries[first.phonebook].savedPassword&&store->entries[first.phonebook].sessionPassword&&store->entries[second.phonebook].userName=="bob","Explicit selected Apply survives parent Cancel without applying the other connection");expect(store->writes.size()==1&&manager.snapshot().dump().find("session-fixture")==std::string::npos,"Parent Cancel does not repeat or undo explicit credentials and no password enters catalog/history");}
 manager.stop();
}
static void dialApplyChecks(const fs::path& root){for(const std::string mode:{"noop","success","failure"})dialApplyCase(root/L"selected-apply",mode);}
