// Exercises the real Connect button with an injected UI dialog handler; never dials Windows.
struct DialConnectTestAccess {static void dialog(Options& o,std::function<void(const DialEntry&)> f){o.connectionDialog=std::move(f);}};
struct DialConnectProbe {
 Manager& manager;std::shared_ptr<HeldDialCredentials> store;std::string mode;DialEntry expected,captured;int phase=0,errors=0,requests=0;HWND main=nullptr;std::string failure;ULONGLONG end=GetTickCount64()+20000;DWORD ui=GetCurrentThreadId();
 FileInfoDecisionTask elsewhere;std::promise<void> otherRelease;std::shared_future<void> otherWait=otherRelease.get_future().share();bool otherUnlocked=false;
 bool clean()const{return mode=="clean"||mode=="busy-clean";}
 void releaseOther()noexcept{if(!elsewhere.valid())return;if(!otherUnlocked){otherUnlocked=true;otherRelease.set_value();}try{elsewhere.get();}catch(const std::exception& e){if(failure.empty())failure=e.what();}catch(...){if(failure.empty())failure="Unrelated decision failed";}elsewhere.release();}
 void connect(const DialEntry& entry){
  ++requests;captured=entry;expect(GetCurrentThreadId()==ui&&!manager.fileInfoDecisionPending,"Connection dialog continuation runs on the UI thread after the decision guard is released");
  expect(entry.name==expected.name&&entry.phonebook==expected.phonebook,"Connect continuation retains the clicked connection and exact phonebook identity");expect(store->writes.size()==(clean()?0:1),"Connection dialog cannot open before the selected credentials are committed");
  auto before=requests;SendMessageW(child(main,L"Connect..."),BM_CLICK,0,0);SendMessageW(main,WM_COMMAND,IDOK,0);expect(IsWindow(main)&&requests==before,"Connection dialog invocation defers reentrant Connect and final OK");
  if(mode=="dialog-failure"&&requests==1)throw std::runtime_error("Forced connection dialog failure");
 }
 void finish(){
  expect(requests==(mode=="dialog-failure"?2:1)&&captured.phonebook==expected.phonebook,"One connection dialog is opened per explicitly accepted Connect request");
  expect(store->writes.size()==(clean()?0:1)&&store->attempts==(clean()?0:(mode=="save-failure"||mode=="apply-after-failure")?2:1),"Clean and repeated Connect requests never rewrite already accepted credentials");
  expect(!IsWindowEnabled(applyButton(main,false))&&windowText(editAfter(main,L"User name:"))==(clean()?L"DOMAIN\\alice":L"DOMAIN\\connect-accepted"),"Connect leaves the selected committed identity clean and Apply disabled");
  auto choice=GetWindow(child(main,L"Connection:"),GW_HWNDNEXT);selectCombo(main,choice,1);expect(windowText(editAfter(main,L"User name:"))==L"OTHER\\unfinished\\identity","Connect preserves an unfinished invalid draft for another connection");selectCombo(main,choice,0);
  DialApplyProbe::page(main,0);expect(SendMessageW(child(main,L"Enable browser download capture"),BM_GETCHECK,0,0)==BST_UNCHECKED,"Connect completion retains an unrelated unaccepted Options checkbox");phase=99;SendMessageW(main,WM_COMMAND,IDCANCEL,0);
 }
 void tick(){
  HWND notice=nullptr;struct Windows{DialConnectProbe* p;HWND* notice;} context{this,&notice};EnumThreadWindows(GetCurrentThreadId(),[](HWND w,LPARAM l)->BOOL{auto& c=*reinterpret_cast<Windows*>(l);if(IsWindowVisible(w)){auto title=windowText(w);if(title==L"UDM Configuration")c.p->main=w;else if(title==L"UDM")*c.notice=w;}return TRUE;},reinterpret_cast<LPARAM>(&context));
  try{
   if(notice){
    ++errors;if(mode=="busy-clean"){child(notice,L"Another update is still being saved. Try again shortly.");expect(manager.fileInfoDecisionPending&&elsewhere.valid()&&requests==0&&store->attempts==0,"Clean Connect defers its dialog while an unrelated real decision task owns the guard");}
    else if(mode=="dialog-failure"){child(notice,L"Forced connection dialog failure");expect(!manager.fileInfoDecisionPending&&requests==1&&store->writes.size()==1,"Connection dialog failure reports its original error without rolling back saved credentials");}
    else{child(notice,L"Forced selected credential failure");expect(manager.fileInfoDecisionPending&&requests==0&&store->writes.empty(),"Failed pre-connect credential save opens no connection dialog and retains its guard through acknowledgement");}
    SendMessageW(child(notice,L"OK"),BM_CLICK,0,0);if(mode=="busy-clean")releaseOther();return;
   }
   if(GetTickCount64()>end)throw std::runtime_error("Dial Connect interaction deadline");if(!main||!IsWindow(main))return;
   if(!failure.empty()){store->unlock();releaseOther();if(!manager.fileInfoDecisionPending)PostMessageW(main,WM_CLOSE,0,0);return;}
   if(phase==0){
    dialTab(main);auto choice=GetWindow(child(main,L"Connection:"),GW_HWNDNEXT);selectCombo(main,choice,1);SetWindowTextW(editAfter(main,L"User name:"),L"OTHER\\unfinished\\identity");selectCombo(main,choice,0);if(!clean())SetWindowTextW(editAfter(main,L"User name:"),L"DOMAIN\\connect-accepted");
    DialApplyProbe::page(main,0);SendMessageW(child(main,L"Enable browser download capture"),BM_CLICK,0,0);DialApplyProbe::page(main,7);
    if(mode=="busy-clean"){elsewhere.startAction(manager,[wait=otherWait]{if(wait.wait_for(std::chrono::seconds(10))!=std::future_status::ready)throw std::runtime_error("Unrelated decision gate expired");return FileInfoDecisionResult{};});phase=5;}else phase=1;
    SendMessageW(child(main,L"Connect..."),BM_CLICK,0,0);return;
   }
   if(phase==5&&errors==1&&!manager.fileInfoDecisionPending){expect(requests==0&&store->attempts==0,"A busy clean Connect does not queue a delayed connection dialog");phase=6;SendMessageW(child(main,L"Connect..."),BM_CLICK,0,0);return;}
   if((phase==1||phase==6)&&clean()&&requests==1){expect(errors==(mode=="busy-clean"?1:0),"Clean Connect has no credential save or implicit retry");finish();return;}
   if(phase==1&&store->ready.wait_for(std::chrono::seconds(0))==std::future_status::ready){
    expect(manager.fileInfoDecisionPending&&store->writer!=ui&&store->attempts==1&&requests==0,"Connect returns to its modal loop while the worker holds the pre-connect store write");expect(!IsWindowEnabled(child(main,L"Connect..."))&&!IsWindowEnabled(child(main,L"OK")),"Connect and final OK freeze until the credential decision finishes");
    SendMessageW(child(main,L"Connect..."),BM_CLICK,0,0);SendMessageW(applyButton(main,false),BM_CLICK,0,0);SendMessageW(main,WM_CLOSE,0,0);RedrawWindow(main,nullptr,nullptr,RDW_INVALIDATE|RDW_ALLCHILDREN|RDW_UPDATENOW);expect(IsWindow(main)&&requests==0&&store->attempts==1,"Pending Connect repaints and rejects duplicate Connect, Apply and Close");KillTimer(main,Form::pulseTimer);phase=2;store->unlock();return;
   }
   if(phase==2&&errors==1&&!manager.fileInfoDecisionPending){
    if(mode=="dialog-failure"){expect(requests==1&&!IsWindowEnabled(applyButton(main,false)),"A failed connection dialog leaves credentials accepted and requires another explicit Connect");phase=4;SendMessageW(child(main,L"Connect..."),BM_CLICK,0,0);return;}
    expect(requests==0&&IsWindowEnabled(applyButton(main,false))&&windowText(editAfter(main,L"User name:"))==L"DOMAIN\\connect-accepted","Failed Connect save retains the same entries without running or retaining its connection continuation");phase=3;SendMessageW(mode=="apply-after-failure"?applyButton(main,false):child(main,L"Connect..."),BM_CLICK,0,0);return;
   }
   if(phase==3&&mode=="apply-after-failure"&&!manager.fileInfoDecisionPending){expect(requests==0&&store->writes.size()==1&&!IsWindowEnabled(applyButton(main,false)),"Apply after a failed Connect saves credentials without executing the discarded Connect request");phase=4;SendMessageW(child(main,L"Connect..."),BM_CLICK,0,0);return;}
   if((phase==2||phase==3||phase==4)&&!manager.fileInfoDecisionPending&&requests){expect(errors==((mode=="save-failure"||mode=="apply-after-failure"||mode=="dialog-failure")?1:0),"Connect reports only its explicitly injected failure");finish();}
  }catch(const std::exception& e){failure=e.what();store->unlock();releaseOther();if(notice)PostMessageW(notice,WM_CLOSE,0,0);}
 }
};
static DialConnectProbe* dialConnectProbe=nullptr;
static void CALLBACK driveDialConnect(HWND,UINT,UINT_PTR,DWORD){if(dialConnectProbe)dialConnectProbe->tick();}
static void dialConnectCase(const fs::path& root,const std::string& mode){
 Manager manager(root/wide(mode));DialEntry first{"Same VPN",utf8((manager.root/L"first.pbk").wstring())},second{"Same VPN",utf8((manager.root/L"second.pbk").wstring())};auto settings=manager.state["Settings"];settings["DialEntry"]=first.name;settings["DialPhonebook"]=first.phonebook;settings["DialEnabled"]=false;manager.setSettings(settings);auto before=manager.snapshot();auto bytes=readText(manager.root/L"state.json");
 auto store=std::make_shared<HeldDialCredentials>(mode=="save-failure"||mode=="apply-after-failure");store->entries[first.phonebook]={"DOMAIN\\alice",true,false};store->entries[second.phonebook]={"bob",false,false};Options dialog(manager,nullptr,store,[=]{return std::vector<DialEntry>{first,second};});DialConnectProbe probe{manager,store,mode,first};DialConnectTestAccess::dialog(dialog,[&](const DialEntry& entry){probe.connect(entry);});dialConnectProbe=&probe;auto timer=SetTimer(nullptr,0,20,driveDialConnect);dialog.DoModal();KillTimer(nullptr,timer);dialConnectProbe=nullptr;store->unlock();probe.releaseOther();if(!probe.failure.empty())throw std::runtime_error(probe.failure);
 expect(!manager.fileInfoDecisionPending&&manager.snapshot()==before&&readText(manager.root/L"state.json")==bytes,"Connect credential preparation and parent Cancel preserve catalog state and bytes");expect(store->entries[first.phonebook].userName==(probe.clean()?"DOMAIN\\alice":"DOMAIN\\connect-accepted")&&store->entries[second.phonebook].userName=="bob","Only explicitly accepted selected credentials survive Connect and parent Cancel");manager.stop();
}
static void dialConnectChecks(const fs::path& root){for(const std::string mode:{"clean","busy-clean","success","save-failure","apply-after-failure","dialog-failure"})dialConnectCase(root/L"connect",mode);}
