// Owns only this executable's dialogs. Native Windows editors are injected.
struct DialEditorFixtureSource {
 std::string mode;std::vector<DialEntry> entries;std::shared_ptr<LookupGate> gate=std::make_shared<LookupGate>();std::atomic_int calls=0;std::atomic_bool heldReturned=false;
 std::vector<DialEntry> read(){int n=++calls;if((mode=="hold-cancel"||mode=="hold-accept")&&n==1){try{gate->hold();}catch(...){heldReturned=true;throw;}heldReturned=true;}if(mode=="failure-retry"&&n==1)throw std::runtime_error("Original editor enumeration failure");if(mode=="refresh-latest"&&n==2)gate->hold();if(mode=="empty")return {};auto result=entries;if(n>=2&&!(mode=="refresh-latest"&&n==2))std::reverse(result.begin(),result.end());return result;}
};
struct DialEditorProbe {
 Json& prefs;Json before;std::shared_ptr<DialEditorFixtureSource> source;int phase=0,errors=0,nativeCalls=0,settled=0;HWND main=nullptr;std::string failure;bool heldOnReturn=false;DWORD ui=GetCurrentThreadId();ULONGLONG end=GetTickCount64()+20000;
 HWND choice(){return GetWindow(child(main,L"Connection"),GW_HWNDNEXT);}HWND number(const wchar_t* label){return GetWindow(child(main,label),GW_HWNDNEXT);}
 bool ready(){return IsWindowEnabled(choice())&&SendMessageW(choice(),CB_GETCOUNT,0,0)==2;}
 void select(int index){auto control=choice();SendMessageW(control,CB_SETCURSEL,index,0);SendMessageW(main,WM_COMMAND,MAKEWPARAM(GetDlgCtrlID(control),CBN_SELCHANGE),(LPARAM)control);}
 void accept(){phase=99;SendMessageW(child(main,L"OK"),BM_CLICK,0,0);}void cancel(){phase=99;SendMessageW(child(main,L"Cancel"),BM_CLICK,0,0);}
 void native(DialEditorAction action,CWnd* owner,const std::optional<DialEntry>& entry){
  ++nativeCalls;expect(GetCurrentThreadId()==ui&&owner->GetSafeHwnd()==main,"Native connection editors remain on their owning UI thread");
  expect(action==DialEditorAction::New?!entry:entry&&entry->phonebook==source->entries[1].phonebook,"Native New has no selected identity; Properties and Connect retain the exact selected phonebook");
  auto n=nativeCalls,reads=source->calls.load();SendMessageW(child(main,L"Connect..."),BM_CLICK,0,0);SendMessageW(child(main,L"Refresh"),BM_CLICK,0,0);SendMessageW(main,WM_COMMAND,IDOK,0);SendMessageW(main,WM_COMMAND,IDCANCEL,0);expect(IsWindow(main)&&nativeCalls==n&&source->calls==reads,"Native editor invocation defers reentrant commands, refresh and parent acceptance/cancellation");
  if(nativeCalls==1){prefs["Connections"]=9;prefs["DialRetrySeconds"]=33;}
  if(source->mode=="native-error"&&nativeCalls==1)throw std::runtime_error("Original native editor failure");
 }
 void tick(){
  HWND notice=nullptr;struct Windows{DialEditorProbe* p;HWND* notice;} context{this,&notice};EnumThreadWindows(GetCurrentThreadId(),[](HWND w,LPARAM l)->BOOL{auto& c=*reinterpret_cast<Windows*>(l);if(IsWindowVisible(w)){auto title=windowText(w);if(title==L"Windows connection options")c.p->main=w;else if(title==L"UDM")*c.notice=w;}return TRUE;},reinterpret_cast<LPARAM>(&context));
  try{
   auto mode=source->mode;
   if(notice){++errors;if(mode=="ambiguous")child(notice,L"Select a Windows connection.");else if(mode=="native-error")child(notice,L"Original native editor failure");else if(mode=="validation-retry")child(notice,L"Use 0-100 dial attempts (0 = until stopped), 1-3600 seconds between attempts, and a 10-600 second connection timeout.");else child(notice,L"Use whole-number dial settings.");
    if(mode=="native-error"){auto expected=before;expected["Connections"]=9;expected["DialRetrySeconds"]=33;expect(prefs==expected,"A native editor error does not publish redial draft changes or undo independent caller changes");}else expect(prefs==before,"Invalid or ambiguous editor OK leaves every caller setting untouched");SendMessageW(child(notice,L"OK"),BM_CLICK,0,0);return;
   }
   if(GetTickCount64()>end)throw std::runtime_error("Connection editor interaction deadline");if(!main||!IsWindow(main))return;
   if(!failure.empty()){source->gate->release();PostMessageW(main,WM_CLOSE,0,0);return;}
   if(phase==0){
    if(mode=="hold-cancel"||mode=="hold-accept"){
     if(source->gate->ready.wait_for(std::chrono::seconds(0))!=std::future_status::ready)return;expect(!IsWindowEnabled(choice())&&IsWindowEnabled(number(L"Dial attempts (0 = until stopped)"))&&IsWindowEnabled(child(main,L"Cancel")),"The editor opens with editable variables and Cancel while enumeration is held");expect(!IsWindowEnabled(child(main,L"Properties..."))&&!IsWindowEnabled(child(main,L"Connect...")),"Unavailable selected-entry actions stay disabled while loading");SetWindowTextW(number(L"Dial attempts (0 = until stopped)"),L"9");heldOnReturn=!source->gate->unlocked;if(mode=="hold-accept")accept();else cancel();return;
    }
    if(mode=="failure-retry"){bool shown=false;for(auto c:children(main,L"Static"))if(windowText(c)==L"Original editor enumeration failure")shown=true;if(!shown)return;expect(IsWindowEnabled(child(main,L"New..."))&&IsWindowEnabled(child(main,L"Refresh"))&&!IsWindowEnabled(child(main,L"Connect...")),"Enumeration error is inline and leaves creation and explicit retry available");SetWindowTextW(number(L"Dial attempts (0 = until stopped)"),L"7");KillTimer(main,Form::pulseTimer);phase=1;SendMessageW(child(main,L"Refresh"),BM_CLICK,0,0);return;}
    if(mode=="empty"){bool shown=false;for(auto c:children(main,L"Static"))if(windowText(c)==L"No Windows connections found. Choose New to set one up.")shown=true;if(!shown)return;expect(!IsWindowEnabled(choice())&&!IsWindowEnabled(child(main,L"Properties..."))&&IsWindowEnabled(child(main,L"New...")),"Empty enumeration retains recovery actions without fabricating a selected connection");SetWindowTextW(number(L"Dial attempts (0 = until stopped)"),L"0");SetWindowTextW(number(L"Seconds between attempts"),L"1");SetWindowTextW(number(L"Connection timeout (seconds)"),L"10");accept();return;}
    if(!ready())return;
    if(mode=="ambiguous"){expect(SendMessageW(choice(),CB_GETCURSEL,0,0)==CB_ERR&&!IsWindowEnabled(child(main,L"Properties..."))&&!IsWindowEnabled(child(main,L"Connect...")),"Matching names in different books require an explicit choice when no default-book match exists");phase=1;SendMessageW(child(main,L"OK"),BM_CLICK,0,0);return;}
    expect(windowText(choice()).find(L"second.pbk")!=std::wstring::npos,"Initial editor selection matches the full stored phonebook identity");
    if(mode=="no-op"){accept();return;}
    if(mode=="validation-cancel"||mode=="validation-retry"){SendMessageW(child(main,L"Connect using Windows dial-up / VPN before downloads"),BM_CLICK,0,0);SetWindowTextW(number(L"Dial attempts (0 = until stopped)"),(mode=="validation-retry"?L"101":L"4x"));phase=1;SendMessageW(child(main,L"OK"),BM_CLICK,0,0);return;}
    if(mode=="refresh-latest"){select(0);SetWindowTextW(number(L"Dial attempts (0 = until stopped)"),L"7");phase=2;SendMessageW(child(main,L"Refresh"),BM_CLICK,0,0);return;}
    SetWindowTextW(number(L"Dial attempts (0 = until stopped)"),L"5");phase=3;SendMessageW(child(main,L"Properties..."),BM_CLICK,0,0);return;
   }
   if(phase==1&&mode=="failure-retry"&&ready()){expect(windowText(number(L"Dial attempts (0 = until stopped)"))==L"7","Enumeration retry via posted completion preserves edited variables without polling");accept();return;}
   if(phase==1&&errors==1&&IsWindowEnabled(main)){
    if(mode=="ambiguous"){select(1);accept();return;}expect(windowText(number(L"Dial attempts (0 = until stopped)"))==(mode=="validation-retry"?L"101":L"4x"),"Rejected numeric input remains in the same editor for correction");if(mode=="validation-cancel"){cancel();return;}SetWindowTextW(number(L"Dial attempts (0 = until stopped)"),L"100");SetWindowTextW(number(L"Seconds between attempts"),L"3600");SetWindowTextW(number(L"Connection timeout (seconds)"),L"600");accept();return;
   }
   if(phase==2&&source->gate->ready.wait_for(std::chrono::seconds(0))==std::future_status::ready){KillTimer(main,Form::pulseTimer);phase=4;SendMessageW(child(main,L"Refresh"),BM_CLICK,0,0);return;}
   if(phase==4&&ready()&&source->calls>=3){expect(windowText(choice()).find(L"first.pbk")!=std::wstring::npos&&SendMessageW(choice(),CB_GETCURSEL,0,0)==1,"Refresh preserves the user's newer selection across reordered matching-name entries");phase=5;source->gate->release();return;}
   if(phase==5&&source->gate->finished&&++settled>=5){wchar_t label[1024]{};SendMessageW(choice(),CB_GETLBTEXT,0,(LPARAM)label);expect(std::wstring(label).find(L"second.pbk")!=std::wstring::npos&&windowText(number(L"Dial attempts (0 = until stopped)"))==L"7","Late older refresh cannot replace latest ordering or edited variables");accept();return;}
   if(phase==3&&ready()&&IsWindowEnabled(main)&&(mode!="native-error"||errors==1)&&source->calls>=2){phase=6;SendMessageW(child(main,L"Connect..."),BM_CLICK,0,0);return;}
   if(phase==6){phase=7;SendMessageW(child(main,L"New..."),BM_CLICK,0,0);return;}
   if(phase==7&&ready()&&source->calls>=3){expect(nativeCalls==3&&source->calls==3,"Only New and Properties trigger one post-dialog enumeration; Connect performs no unnecessary refresh");expect(windowText(number(L"Dial attempts (0 = until stopped)"))==L"5","Native editor refreshes preserve the redial draft");accept();}
  }catch(const std::exception& e){failure=e.what();source->gate->release();if(notice)PostMessageW(notice,WM_CLOSE,0,0);}
 }
};
static DialEditorProbe* dialEditorProbe=nullptr;
static void CALLBACK driveDialEditor(HWND,UINT,UINT_PTR,DWORD){if(dialEditorProbe)dialEditorProbe->tick();}
static void dialEditorCase(const fs::path& root,const std::string& mode){
 auto source=std::make_shared<DialEditorFixtureSource>();source->mode=mode;source->entries={{"Same VPN",utf8((root/wide(mode)/L"first.pbk").wstring())},{"Same VPN",utf8((root/wide(mode)/L"second.pbk").wstring())}};
 Json prefs=defaultSettings();prefs["DialEnabled"]=mode!="empty";prefs["DialEntry"]="Same VPN";prefs["DialPhonebook"]=mode=="ambiguous"?"":source->entries[1].phonebook;prefs["Connections"]=4;auto before=prefs;DialEditorProbe probe{prefs,before,source};dialEditorProbe=&probe;auto timer=SetTimer(nullptr,0,20,driveDialEditor);bool accepted=false;
 try{accepted=dialUpOptions(nullptr,prefs,[source]{return source->read();},[&](DialEditorAction action,CWnd* owner,const std::optional<DialEntry>& entry){probe.native(action,owner,entry);});}catch(...){KillTimer(nullptr,timer);dialEditorProbe=nullptr;source->gate->release();throw;}
 KillTimer(nullptr,timer);dialEditorProbe=nullptr;bool returnedBeforeRead=!source->heldReturned;source->gate->release();if(!probe.failure.empty())throw std::runtime_error(probe.failure);
 if(mode=="hold-cancel"||mode=="hold-accept")expect(probe.heldOnReturn&&returnedBeforeRead,"Editor acceptance or cancellation returns without joining held enumeration");
 bool discarded=mode=="hold-cancel"||mode=="validation-cancel";expect(accepted!=discarded,"Only accepted editor drafts report success");if(discarded||mode=="no-op")expect(prefs==before,"Cancel and unchanged OK preserve the complete caller object");
 else if(mode=="hold-accept")expect(num(prefs,"DialAttempts")==9&&str(prefs,"DialPhonebook")==source->entries[1].phonebook&&yes(prefs,"DialEnabled"),"OK during pending enumeration retains a valid enabled stored connection");
 else if(mode=="validation-retry")expect(!yes(prefs,"DialEnabled")&&num(prefs,"DialAttempts")==100&&num(prefs,"DialRetrySeconds")==3600&&num(prefs,"DialTimeoutSeconds")==600,"Corrected maximum numeric bounds and local enabled change commit together");
 else if(mode=="empty")expect(!yes(prefs,"DialEnabled")&&num(prefs,"DialAttempts")==0&&num(prefs,"DialRetrySeconds")==1&&num(prefs,"DialTimeoutSeconds")==10,"Disabled empty-list settings accept the minimum numeric bounds");
 else if(mode=="native"||mode=="native-error")expect(num(prefs,"DialAttempts")==5&&num(prefs,"DialRetrySeconds")==33&&num(prefs,"Connections")==9,"Dirty-field publication preserves independent caller changes, including an untouched dial variable");
 else if(mode=="ambiguous")expect(str(prefs,"DialPhonebook")==source->entries[1].phonebook,"Explicit ambiguity resolution commits the chosen book rather than a matching name alone");
 else expect(num(prefs,"DialAttempts")==7&&(mode!="refresh-latest"||str(prefs,"DialPhonebook")==source->entries[0].phonebook),"Accepted refresh/retry retains edited variables and the selected identity");
}
struct MoreIntegrationProbe {
 Options& owner;std::string mode;HWND main=nullptr,editor=nullptr;int phase=0;std::string failure;ULONGLONG end=GetTickCount64()+15000;
 void tick(){
  editor=nullptr;EnumThreadWindows(GetCurrentThreadId(),[](HWND w,LPARAM l)->BOOL{auto p=reinterpret_cast<MoreIntegrationProbe*>(l);if(IsWindowVisible(w)){auto title=windowText(w);if(title==L"UDM Configuration")p->main=w;else if(title==L"Windows connection options")p->editor=w;}return TRUE;},reinterpret_cast<LPARAM>(this));
  try{if(GetTickCount64()>end)throw std::runtime_error("Parent More integration deadline");if(!failure.empty()){if(editor)PostMessageW(editor,WM_CLOSE,0,0);else if(main)PostMessageW(main,WM_CLOSE,0,0);return;}if(!main)return;
   if(phase==0){if(!DialReadTestAccess::ready(owner))return;DialApplyProbe::page(main,0);SendMessageW(child(main,L"Enable browser download capture"),BM_CLICK,0,0);dialTab(main);if(mode=="ambiguous")expect(SendMessageW(GetWindow(child(main,L"Connection:"),GW_HWNDNEXT),CB_GETCURSEL,0,0)==CB_ERR&&!IsWindowEnabled(child(main,L"Connect...")),"The main picker also rejects an unresolved matching-name ambiguity");phase=1;SendMessageW(child(main,L"More..."),BM_CLICK,0,0);return;}
   if(phase==1&&editor){auto choice=GetWindow(child(editor,L"Connection"),GW_HWNDNEXT);if(!IsWindowEnabled(choice)||SendMessageW(choice,CB_GETCOUNT,0,0)!=2)return;if(mode=="ambiguous"){expect(SendMessageW(choice,CB_GETCURSEL,0,0)==CB_ERR,"More opens to resolve missing selection even when automatic dial-up is enabled");SendMessageW(choice,CB_SETCURSEL,1,0);SendMessageW(editor,WM_COMMAND,MAKEWPARAM(GetDlgCtrlID(choice),CBN_SELCHANGE),(LPARAM)choice);}SetWindowTextW(GetWindow(child(editor,L"Dial attempts (0 = until stopped)"),GW_HWNDNEXT),L"8");if(mode=="cancel")SendMessageW(child(editor,L"Connect using Windows dial-up / VPN before downloads"),BM_CLICK,0,0);phase=2;SendMessageW(child(editor,mode=="cancel"?L"Cancel":L"OK"),BM_CLICK,0,0);return;}
   if(phase==2&&!editor&&!DialReadTestAccess::childOpen(owner)&&DialReadTestAccess::ready(owner)){auto control=GetWindow(child(main,L"Redial attempts (zero if endlessly):"),GW_HWNDNEXT);expect(windowText(control)==(mode=="cancel"?L"3":L"8")&&SendMessageW(child(main,L"Use Windows Dial Up / VPN Networking"),BM_GETCHECK,0,0)==BST_CHECKED,"The parent reloads only accepted More variables and Cancel cannot leak its local checkbox edit");if(mode=="ambiguous")expect(windowText(GetWindow(child(main,L"Connection:"),GW_HWNDNEXT)).find(L"second.pbk")!=std::wstring::npos,"More acceptance resolves the exact parent phonebook identity");DialApplyProbe::page(main,0);expect(SendMessageW(child(main,L"Enable browser download capture"),BM_GETCHECK,0,0)==BST_UNCHECKED,"Returning from More preserves unrelated parent edits");phase=99;SendMessageW(main,WM_COMMAND,IDCANCEL,0);}
  }catch(const std::exception& e){failure=e.what();if(editor)PostMessageW(editor,WM_CLOSE,0,0);}
 }
};
static MoreIntegrationProbe* moreIntegration=nullptr;
static void CALLBACK driveMoreIntegration(HWND,UINT,UINT_PTR,DWORD){if(moreIntegration)moreIntegration->tick();}
static void moreIntegrationCase(const fs::path& root,const std::string& mode){
 Manager manager(root/wide(mode));DialEntry first{"Same VPN",utf8((manager.root/L"first.pbk").wstring())},second{"Same VPN",utf8((manager.root/L"second.pbk").wstring())};auto settings=manager.state["Settings"];settings["DialEnabled"]=true;settings["DialEntry"]=first.name;settings["DialPhonebook"]=mode=="ambiguous"?"":second.phonebook;settings["DialAttempts"]=3;manager.setSettings(settings);auto before=manager.snapshot();auto bytes=readText(manager.root/L"state.json");auto store=std::make_shared<FixtureCredentials>();store->entries[first.phonebook]={"alice",false,false};store->entries[second.phonebook]={"bob",false,false};Options dialog(manager,nullptr,store,[first,second]{return std::vector<DialEntry>{first,second};});MoreIntegrationProbe probe{dialog,mode};moreIntegration=&probe;auto timer=SetTimer(nullptr,0,20,driveMoreIntegration);dialog.DoModal();KillTimer(nullptr,timer);moreIntegration=nullptr;if(!probe.failure.empty())throw std::runtime_error(probe.failure);expect(manager.snapshot()==before&&readText(manager.root/L"state.json")==bytes&&store->writes.empty(),"Nested More drafts and final parent Cancel write no catalog or credentials");manager.stop();
}
static void dialEditorChecks(const fs::path& root){expect(dialEditorSelection({{"Same VPN","C:/other.pbk"},{"Same VPN",""}},{"Same VPN",""})==1,"An exact default-phonebook match wins over ambiguous legacy name fallback");for(const std::string mode:{"hold-cancel","hold-accept","failure-retry","refresh-latest","validation-cancel","validation-retry","empty","ambiguous","native","native-error","no-op"})dialEditorCase(root/L"editor",mode);for(const std::string mode:{"cancel","accept","ambiguous"})moreIntegrationCase(root/L"more-parent",mode);}
