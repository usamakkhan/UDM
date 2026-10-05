// Included inside udm after RangeMap. Values use binary units, matching the main list.
inline std::string progressBytes(double value){if(value<0)return "Unknown";const char* units[]={"Bytes","KB","MB","GB","TB"};int i=0;while(value>=1024&&i<4){value/=1024;++i;}std::ostringstream out;out<<std::fixed<<std::setprecision(i?3:0)<<value<<" "<<units[i];return out.str();}
inline std::string progressTime(i64 seconds){if(seconds<0)return "--";return (seconds>=3600?std::to_string(seconds/3600)+" hr ":"")+(seconds>=60?std::to_string((seconds/60)%60)+" min ":"")+std::to_string(seconds%60)+" sec";}
inline std::string progressResumeCapability(const Json& record){
 if(yes(record,"MediaTracksReady"))return "Streams saved";
 if(yes(record,"SabrResumeSupported")||num(record,"AdaptiveTotalSegments")>0)return "Yes (saved segments)";
 bool tracks=false;
 for(auto name:{"Video","Audio"})if(record.contains(name)&&record[name].is_object()){
  tracks=true;if(!yes(record[name],"RangeSupported"))return "No";
 }
 return (tracks||yes(record,"RangeSupported"))?"Yes":"No";
}
class Progress:public Form {
 DECLARE_MESSAGE_MAP()
 Manager& manager;JobPtr job;
 static constexpr UINT ProgressTrayMessage=WM_APP+144;
#ifdef UDM_TOOLBAR_COMPONENT_TEST
 friend class ProgressTrayRecoveryApplication;
#endif
 NOTIFYICONDATAW trayIcon{};bool trayVisible=false;
 void refreshTrayCaption(){
  if(!trayVisible)return;
  CString caption;GetWindowText(caption);NOTIFYICONDATAW changed=trayIcon;
  wcsncpy_s(changed.szTip,(LPCWSTR)caption,_TRUNCATE);
  if(wcscmp(changed.szTip,trayIcon.szTip)==0)return;
  changed.uFlags=NIF_TIP;
  if(Shell_NotifyIconW(NIM_MODIFY,&changed))wcsncpy_s(trayIcon.szTip,changed.szTip,_TRUNCATE);
 }
 void removeTray(){if(trayVisible){Shell_NotifyIconW(NIM_DELETE,&trayIcon);trayVisible=false;}}
 afx_msg LRESULT OnProgressTray(WPARAM,LPARAM event){if(event==WM_LBUTTONUP||event==WM_LBUTTONDBLCLK||event==NIN_SELECT||event==NIN_KEYSELECT)showNormally();return 0;}
 afx_msg void OnDestroy(){removeTray();Form::OnDestroy();}
 LRESULT WindowProc(UINT message,WPARAM w,LPARAM l)override{
  static const UINT recreated=RegisterWindowMessageW(L"TaskbarCreated");
  if(recreated&&message==recreated&&trayVisible){trayVisible=false;if(!showInTray())showNormally();}
  return Form::WindowProc(message,w,l);
 }
 CWnd *addressField=nullptr,*saveTo=nullptr,*status=nullptr,*size=nullptr,*received=nullptr,*rate=nullptr,*remaining=nullptr,*resume=nullptr,*limit=nullptr,*remember=nullptr,*limitEnabled=nullptr,*information=nullptr,*pauseButton=nullptr,*detailsButton=nullptr,*cancelButton=nullptr,*pieces=nullptr,*limitRate=nullptr,*limitHint=nullptr;
 Json displayedAddressInputs=Json::object();
 void syncAddress(const Json& record){
  Json inputs=Json::object();for(auto key:{"Url","SourceUrl","ProtectedSabr","ProtectedAdaptive","ProtectedResolvedUrl","MediaOutput"})if(record.contains(key))inputs[key]=record[key];
  for(auto key:{"Video","Audio"})if(record.contains(key)&&record[key].is_object())inputs[key]={{"Url",str(record[key],"Url")}};
  if(inputs==displayedAddressInputs)return;auto value=downloadAddress(inputs);if(text(addressField)!=value)addressField->SetWindowText(cs(value));displayedAddressInputs=std::move(inputs);
 }
 bool expanded=true,closeRequested=false,syncingLimit=false,syncingCompletion=false;std::function<void()> syncCompletionControls;std::vector<CWnd*> completionControls;CWnd* completionButton=nullptr;CProgressCtrl* bar=nullptr;RangeMap* rangeMap=nullptr;ThemeList* workers=nullptr;ThemeTabs* tabs=nullptr;std::vector<CWnd*> pages[4];std::vector<int> tabPages;
 void update(){
  if(closeRequested&&IsWindowEnabled()){DestroyWindow();return;}
  Json j;std::vector<Worker> rows;double speed;bool active;{Lock l(manager.mutex);j=job->data;if(job->video)j["Video"]={{"Url",str(job->video->data,"Url")},{"RangeSupported",yes(job->video->data,"RangeSupported")}};if(job->audio)j["Audio"]={{"Url",str(job->audio->data,"Url")},{"RangeSupported",yes(job->audio->data,"RangeSupported")}};active=manager.isActive(job);speed=job->speed;rows=job->workers;if(job->video&&num(j,"SabrConnections")==0){rows=job->video->workers;if(job->audio)rows.insert(rows.end(),job->audio->workers.begin(),job->audio->workers.end());}}
  syncAddress(j);
  const auto destinationText="Save To: "+utf8((fs::path(wide(str(j,"Folder")))/wide(str(j,"FileName"))).wstring());if(text(saveTo)!=destinationText)saveTo->SetWindowText(cs(destinationText));
  auto total=num(j,"Size",-1),done=num(j,"Received");int progress=downloadPermille(j);auto state=str(j,"Status");bool scanning=str(j.value("ScanResult",Json::object()),"Status")=="Running";if(completionButton)completionButton->EnableWindow(state!="Complete");if(state=="Complete")for(auto control:completionControls)control->EnableWindow(FALSE);
  const bool live= yes(j,"LiveRecording")&&state!="Complete",saveLive=live&&yes(j,"LiveSaveReady")&&state!="Merging"&&!yes(j,"LiveStopRequested");
  pauseButton->SetWindowText(cs(scanning?"Stop wait":state=="Complete"?"Open":saveLive?"Stop and save":live&&active?(yes(j,"LiveStopRequested")?"Saving...":"Recording..."):active||state=="Queued"?"Pause":yes(j,"MediaTracksReady")?"Retry":"Start"));pauseButton->EnableWindow(state!="Pausing"&&str(j,"DuplicateOf").empty()&&(!live||saveLive||!active));cancelButton->SetWindowText(state=="Complete"?L"Close":L"Cancel");
  SetWindowText(cs((progress>=0?std::to_string(progress/10)+"% ":"")+str(j,"FileName")));
  refreshTrayCaption();
  auto quota=manager.quotaStatus(job);
  auto message=(yes(quota,"Waiting")?quotaWaitText(quota):scanning?std::string("Scanning file"):str(j,"ConnectionStatus",state))+(str(j,"Error").empty()?"":" - "+str(j,"Error"));status->SetWindowText(cs(message));
  size->SetWindowText(cs(progressBytes((double)total)));received->SetWindowText(cs(progressBytes((double)done)+(progress>=0?"  ("+std::to_string(progress/10)+"."+std::to_string(progress%10)+"%)":"")+streamProgressText(j)));
  rate->SetWindowText(cs(progressBytes(speed)+"/sec"));limitRate->SetWindowText(cs(progressBytes(speed)+"/sec"));remaining->SetWindowText(cs(live?"Until stopped":progressTime(downloadSecondsLeft(j,speed))));
  resume->SetWindowText(cs(progressResumeCapability(j)));bar->SetPos(std::max(0,progress));
  auto duration=num(j,"StreamDurationMs");if(duration>0)rangeMap->update(j.value("SabrRanges",Json::array({{{"Start",0},{"End",duration-1},{"Done",std::clamp<i64>(num(j,"StreamCompletedMs"),0,duration)}}})),duration);else rangeMap->update(j.value("Segments",Json::array()),total);
  workers->SetRedraw(FALSE);while(workers->GetItemCount()>(int)rows.size())workers->DeleteItem(workers->GetItemCount()-1);
  for(int i=0;i<(int)rows.size();++i){auto& w=rows[i];if(i>=workers->GetItemCount())workers->InsertItem(i,cs(std::to_string(i+1)));std::string values[]={progressBytes((double)w.received),w.state,w.end>=w.start?std::to_string((int)std::clamp(100.0*(w.position-w.start)/(w.end-w.start+1),0.0,100.0))+"%":"--"};for(int c=0;c<3;++c)if(utf8((LPCWSTR)workers->GetItemText(i,c+1))!=values[c])workers->SetItemText(i,c+1,cs(values[c]));}
  workers->SetRedraw(TRUE);workers->Invalidate(FALSE);
  information->SetWindowText(cs("Status: "+message+"\r\nSaved as: "+utf8(job->target().wstring())+"\r\n\r\nSHA-256: "+str(j,"Sha256")+"\r\n"+str(j,"FormatDescription")+"\r\nTransferred: "+progressBytes((double)num(j,"TransferredBytes"))+"\r\nRetained media: "+progressBytes((double)num(j,"SabrRetainedBytes"))+"\r\nReused this attempt: "+progressBytes((double)num(j,"SabrReusedBytes"))+"\r\nMedia phase: "+str(j,"MediaPhase")+"\r\nNetwork time: "+std::to_string(real(j,"TransferSeconds"))+" s\r\nMerge time: "+std::to_string(real(j,"MergeSeconds"))+" s\r\nVirus check: "+scannerSummary(j)));
 }
 void page(){int index=tabs->GetCurSel(),selected=index>=0&&index<(int)tabPages.size()?tabPages[index]:0;for(int i=0;i<4;++i)for(auto w:pages[i])w->ShowWindow(i==selected?SW_SHOW:SW_HIDE);}
 void details(){for(CWnd* w:{(CWnd*)workers,(CWnd*)rangeMap,pieces})w->ShowWindow(expanded?SW_SHOW:SW_HIDE);detailsButton->SetWindowText(expanded?L"<< Hide details":L"Show details >>");resizeClient(353,expanded?250:148);}

 void hidePage(int selected){
  auto found=std::find(tabPages.begin(),tabPages.end(),selected);if(found==tabPages.end()||selected==0)return;
  const auto index=(int)std::distance(tabPages.begin(),found);{Lock lock(manager.mutex);auto prefs=manager.state["Settings"];prefs[selected==1?"ProgressSpeedTab":selected==2?"ProgressCompletionTab":"ProgressInformationTab"]=false;manager.setSettings(prefs);}
  tabs->DeleteItem(index);tabPages.erase(found);tabs->SetCurSel(0);page();tabs->SetFocus();
 }
 void syncLimitControls(){
  i64 kb;bool persistent;{Lock lock(manager.mutex);kb=job->sessionLimit.value_or(num(job->data,"LimitKbps"));persistent=!job->sessionLimit.has_value()&&num(job->data,"LimitKbps")>0;}
  syncingLimit=true;limitEnabled->SendMessage(BM_SETCHECK,kb>0?BST_CHECKED:BST_UNCHECKED);remember->SendMessage(BM_SETCHECK,persistent?BST_CHECKED:BST_UNCHECKED);
  limit->SetWindowText(cs(std::to_string(kb>0?kb:1000)));limit->EnableWindow(kb>0);limitHint->SetWindowText(L"KBytes/sec");syncingLimit=false;
 }
 void applyLimit(){
  if(syncingLimit)return;
  const bool enabled=checked(limitEnabled);limit->EnableWindow(enabled);i64 kb=0;
  if(enabled){const auto value=trim(text(limit));if(!std::regex_match(value,std::regex("[0-9]{1,7}"))||std::stoll(value)<1||std::stoll(value)>1000000){limitHint->SetWindowText(L"1..1000000");return;}kb=std::stoll(value);}
  try{setProgressLimit(manager,job,kb,checked(remember));}catch(...){syncLimitControls();throw;}limitHint->SetWindowText(L"KBytes/sec");
 }
 afx_msg void OnContextMenu(CWnd*,CPoint){try{actions();}catch(const std::exception& e){error(this,e);}}
 void completionAction(){
  Json record;{Lock lock(manager.mutex);record=job->data;}
  Form d("Additional completion options",455,269,this);
  d.init=[&]{
   auto close=d.check("Close this progress window when done",yes(record,"CloseProgressOnCompletion",true),14,15,427);
   auto folder=d.check("Open the download folder when done",yes(record,"OpenFolderOnCompletion"),14,47,427);
   const auto steps=completionSteps(record);auto open=d.check("Open the downloaded file after the countdown",std::find(steps.begin(),steps.end(),"Open downloaded file")!=steps.end(),14,79,427);
   d.label("Countdown (seconds)",14,117,240);auto delay=d.edit(std::to_string(num(record,"CompletionActionDelay",30)),282,113,159);
   auto wait=d.check("Wait for other active and queued downloads",yes(record,"CompletionWaitForOthers",true),14,151,427);
   d.label("Power, disconnect and exit choices are on the completion tab.",14,185,427,30);
   d.accept=[&,close,folder,open,delay,wait]{
    const auto seconds=std::stoll(text(delay));if(seconds<15||seconds>3600)throw std::runtime_error("Use a countdown from 15 to 3600 seconds.");
    Lock lock(manager.mutex);auto before=job->data;auto actions=completionSteps(before);actions.erase(std::remove(actions.begin(),actions.end(),"Open downloaded file"),actions.end());if(d.checked(open))actions.push_back("Open downloaded file");
    job->data["CloseProgressOnCompletion"]=d.checked(close);job->data["OpenFolderOnCompletion"]=d.checked(folder);
    try{manager.setCompletionPlan(job,actions,yes(before,"CompletionForce"),(int)seconds,d.checked(wait));}catch(...){job->data=before;throw;}d.close();
   };
   d.defaultButton(d.button("OK",261,229,85,d.accept));d.button("Cancel",356,229,85,[&]{d.close(IDCANCEL);});
  };d.DoModal();if(syncCompletionControls)syncCompletionControls();
 }
 void primary(){std::string state;bool active,saveLive;{Lock l(manager.mutex);state=str(job->data,"Status");active=manager.isActive(job);saveLive=yes(job->data,"LiveRecording")&&yes(job->data,"LiveSaveReady")&&state!="Complete"&&state!="Merging"&&!yes(job->data,"LiveStopRequested");}if(saveLive){requestLiveHlsFinish(manager,job);update();return;}if(active&&state=="Complete")manager.pause(job);else if(state=="Complete")openFile(this,job->target());else if(active||state=="Queued")manager.pause(job);else{manager.resume(job);syncLimitControls();}update();}
 void actions(){CMenu menu;menu.CreatePopupMenu();bool complete,refresh;{Lock l(manager.mutex);complete=str(job->data,"Status")=="Complete";refresh=manager.canRefreshAddress(job);}menu.AppendMenu(MF_STRING|(refresh?MF_ENABLED:MF_GRAYED),1,L"Refresh download address...");menu.AppendMenu(MF_STRING,2,L"Download properties...");menu.AppendMenu(MF_STRING,3,L"Copy download address");menu.AppendMenu(MF_SEPARATOR);menu.AppendMenu(MF_STRING,4,L"Open folder");menu.AppendMenu(MF_STRING|(complete?MF_ENABLED:MF_GRAYED),5,L"Open with...");menu.AppendMenu(MF_SEPARATOR);menu.AppendMenu(MF_STRING,6,L"Hide progress window");menu.AppendMenu(MF_STRING,7,L"Customize download progress dialog...");menu.AppendMenu(MF_STRING,8,L"Download information...");menu.AppendMenu(MF_STRING|(complete?MF_GRAYED:MF_ENABLED),9,L"Additional completion options...");bool recording=false,recordingActive=false,recordingCanResume=false;{Lock lock(manager.mutex);recording=yes(job->data,"LiveRecording")&&!complete;recordingActive=manager.isActive(job);recordingCanResume=!yes(job->data,"LiveStopRequested")&&str(job->data,"Status")!="Merging";}if(recording){menu.AppendMenu(MF_SEPARATOR);menu.AppendMenu(MF_STRING|(recordingCanResume?MF_ENABLED:MF_GRAYED),10,recordingActive?L"Pause recording":L"Resume recording");}CPoint p;GetCursorPos(&p);auto choice=menu.TrackPopupMenu(TPM_RETURNCMD|TPM_LEFTALIGN,p.x,p.y,this);if(choice==1)refreshDownloadAddress(this,manager,job);else if(choice==2)fileProperties(this,manager,job);else if(choice==3){std::string address;{Lock lock(manager.mutex);address=downloadAddress(job->snapshot());}auto value=wide(address);if(OpenClipboard()){EmptyClipboard();auto memory=GlobalAlloc(GMEM_MOVEABLE,(value.size()+1)*sizeof(wchar_t));if(memory){auto target=GlobalLock(memory);if(target){memcpy(target,value.c_str(),(value.size()+1)*sizeof(wchar_t));GlobalUnlock(memory);if(!SetClipboardData(CF_UNICODETEXT,memory))GlobalFree(memory);}else GlobalFree(memory);}CloseClipboard();}}else if(choice==4)openFile(this,job->target().parent_path());else if(choice==5)openWith(this,job->target());else if(choice==6)DestroyWindow();else if(choice==7){Json prefs;{Lock lock(manager.mutex);prefs=manager.state["Settings"];}progressPreferences(this,prefs);manager.setSettings(prefs);}else if(choice==8){Form details("Download information",620,340,this);details.init=[&]{details.edit(text(information),12,12,596,273,true,true);details.button("Close",513,302,95,[&]{details.close();});};details.DoModal();}else if(choice==9)completionAction();else if(choice==10){if(recordingActive)manager.pause(job);else manager.resume(job);update();}}
 afx_msg void OnTimer(UINT_PTR id){try{update();}catch(const std::exception& e){status->SetWindowText(cs(e.what()));}if(GetSafeHwnd())Form::OnTimer(id);}
 BOOL OnNotify(WPARAM w,LPARAM l,LRESULT* result)override{auto hdr=(NMHDR*)l;if(hdr->hwndFrom==tabs->m_hWnd&&hdr->code==TCN_SELCHANGE){page();*result=0;return TRUE;}return Form::OnNotify(w,l,result);}
public:
 ~Progress()override{removeTray();}
 void showNormally(){removeTray();ShowWindow(IsIconic()?SW_RESTORE:SW_SHOW);BringWindowToTop();SetForegroundWindow();}
 bool showInTray(){
  if(trayVisible){ShowWindow(SW_HIDE);return true;}
  trayIcon={};trayIcon.cbSize=sizeof(trayIcon);trayIcon.hWnd=GetSafeHwnd();trayIcon.uID=1;trayIcon.uFlags=NIF_MESSAGE|NIF_ICON|NIF_TIP;trayIcon.uCallbackMessage=ProgressTrayMessage;trayIcon.hIcon=AfxGetApp()->LoadIcon(1);
  CString caption;GetWindowText(caption);wcsncpy_s(trayIcon.szTip,(LPCWSTR)caption,_TRUNCATE);
  if(!Shell_NotifyIconW(NIM_ADD,&trayIcon))return false;
  trayVisible=true;ShowWindow(SW_HIDE);return true;
 }
 void requestCompletionClose(){if(!IsWindowEnabled()){closeRequested=true;return;}DestroyWindow();}
 std::string downloadId()const{return job->id();}
 Progress(Manager& m,JobPtr j,CWnd* owner):Form("Download progress",353,250,owner),manager(m),job(j){dialogUnits=true;modeless=true;cancel=[this]{manager.pause(job);DestroyWindow();};init=[this]{
  ModifyStyle(0,WS_MINIMIZEBOX);Json prefs,record;i64 actualLimit;bool persistentLimit;{Lock lock(manager.mutex);prefs=manager.state["Settings"];record=job->snapshot();actualLimit=job->sessionLimit.value_or(num(record,"LimitKbps"));persistentLimit=!job->sessionLimit.has_value()&&num(record,"LimitKbps")>0;}expanded=str(prefs,"ProgressStartMode","Normal size")!="Small size";
  tabs=make<ThemeTabs>(WS_TABSTOP,0,0,353,106);tabs->InsertItem(0,L"Download status");tabPages.push_back(0);if(yes(prefs,"ProgressSpeedTab",true)){tabs->InsertItem((int)tabPages.size(),L"Speed Limiter");tabPages.push_back(1);}if(yes(prefs,"ProgressCompletionTab",true)){tabs->InsertItem((int)tabPages.size(),L"Options on completion");tabPages.push_back(2);}if(yes(prefs,"ProgressInformationTab",false)){tabs->InsertItem((int)tabPages.size(),L"Information");tabPages.push_back(3);}
  const int ox=4,oy=20;
  addressField=control(L"STATIC",downloadAddress(record),SS_LEFTNOWORDWRAP|SS_NOPREFIX,ox+7,oy+4,329,9);pages[0].push_back(addressField);
  const char* captions[]={"Status","File size","Downloaded","Transfer rate","Time left","Resume capability"};const int y[]={15,28,38,48,58,68},left[]={53,75,75,75,75,127},width[]={281,261,261,261,261,111};CWnd** values[]={&status,&size,&received,&rate,&remaining,&resume};
  for(int i=0;i<6;++i){pages[0].push_back(label(captions[i],ox+7,oy+y[i],i==0?40:i==2?60:i==5?108:63));*values[i]=control(L"STATIC","",SS_ENDELLIPSIS,ox+left[i],oy+y[i],width[i],9);pages[0].push_back(*values[i]);}
  bar=make<CProgressCtrl>(PBS_SMOOTH,7,111,338,10);bar->SetRange32(0,1000);if(uiDark){SetWindowTheme(bar->GetSafeHwnd(),L"",L"");bar->SetBkColor(RGB(48,51,56));bar->SetBarColor(RGB(35,139,63));}
  detailsButton=button("<< Hide details",15,126,93,[this]{expanded=!expanded;details();},15);pauseButton=button("Pause",203,126,56,[this]{primary();},15);cancelButton=button("Cancel",277,126,50,[this]{manager.pause(job);DestroyWindow();},15);
  pieces=control(L"STATIC","Start positions and download progress by connections",SS_CENTER,21,143,306,9);rangeMap=make<RangeMap>(0,7,153,338,10);workers=make<ThemeList>(WS_TABSTOP|WS_BORDER|LVS_SHOWSELALWAYS|LVS_REPORT|LVS_SINGLESEL|LVS_NOSORTHEADER,7,168,338,79);workers->SetExtendedStyle(LVS_EX_FULLROWSELECT|LVS_EX_DOUBLEBUFFER);workers->theme();workers->InsertColumn(0,L"N.",LVCFMT_LEFT,pixelsX(20));workers->InsertColumn(1,L"Downloaded",LVCFMT_LEFT,pixelsX(68));workers->InsertColumn(2,L"Info",LVCFMT_LEFT,pixelsX(188));workers->InsertColumn(3,L"Progress",LVCFMT_LEFT,pixelsX(42));
  auto limitRateTitle=label("Transfer rate",ox+7,oy+7,63);limitRate=label("",ox+75,oy+7,202);
  limitEnabled=check("Use Speed Limiter",actualLimit>0,ox+7,oy+20,154);auto maximum=label("Maximum download speed:",ox+7,oy+32,170);limit=edit(std::to_string(actualLimit>0?actualLimit:1000),ox+7,oy+43,51);limitHint=label("KBytes/sec",ox+64,oy+45,65);
  remember=check("Remember Speed Limiter settings for this file\r\non download stop/resume",persistentLimit,ox+7,oy+59,226,18);limit->EnableWindow(actualLimit>0);bind(limitEnabled,[this]{applyLimit();});bind(remember,[this]{applyLimit();});bindChange(limit,[this]{applyLimit();});pages[1]={limitRateTitle,limitRate,limitEnabled,maximum,limit,limitHint,remember};
  saveTo=control(L"STATIC","Save To: "+utf8(job->target().wstring()),SS_ENDELLIPSIS,ox+7,oy+3,316,9);
  auto show=check("Show download complete dialog",!yes(record,"SuppressCompletionDialog")&&!yes(prefs,"SuppressCompletionDialog"),ox+7,oy+15,169);show->EnableWindow(!yes(prefs,"SuppressCompletionDialog"));
  bind(show,[this,show]{Lock lock(manager.mutex);auto before=job->data;job->data["SuppressCompletionDialog"]=!checked(show);try{manager.save();}catch(...){job->data=before;show->SendMessage(BM_SETCHECK,yes(before,"SuppressCompletionDialog")?BST_UNCHECKED:BST_CHECKED);throw;}});
  auto separator=control(L"STATIC","",SS_ETCHEDHORZ,ox+7,oy+27,316,1);auto powerTitle=label("Power options",ox+7,oy+30,316);
  auto disconnect=glyphCheck("Hang up modem when done",false,ox+7,oy+40,9);
  auto exit=glyphCheck("Exit UDM when done",false,ox+7,oy+51,9);
  auto power=glyphCheck("Turn off computer when done",false,ox+7,oy+62,9);
  auto powerChoice=combo({"Shut down","Restart","Sleep","Hibernate"},"Shut down",ox+163,oy+64,79);
  auto force=glyphCheck("Force processes to terminate",false,ox+20,oy+72,9);
  auto disconnectLabel=control(L"STATIC","Hang up modem when done",SS_NOTIFY,ox+18,oy+41,225,9);
  auto exitLabel=control(L"STATIC","Exit UDM when done",SS_NOTIFY,ox+18,oy+52,225,9);
  auto powerLabel=control(L"STATIC","Turn off computer when done",SS_NOTIFY,ox+18,oy+63,141,9);
  auto forceLabel=control(L"STATIC","Force processes to terminate",SS_NOTIFY,ox+31,oy+73,127,9);
  syncCompletionControls=[this,disconnect,exit,power,powerChoice,force,forceLabel]{
   Json current;{Lock lock(manager.mutex);current=job->data;}const auto steps=completionSteps(current);auto has=[&](const std::string& action){return std::find(steps.begin(),steps.end(),action)!=steps.end();};
   syncingCompletion=true;disconnect->SendMessage(BM_SETCHECK,has("Disconnect dial-up / VPN")?BST_CHECKED:BST_UNCHECKED);exit->SendMessage(BM_SETCHECK,has("Exit UDM")?BST_CHECKED:BST_UNCHECKED);
   auto selected=std::find_if(steps.begin(),steps.end(),completionPower);bool enabled=selected!=steps.end();power->SendMessage(BM_SETCHECK,enabled?BST_CHECKED:BST_UNCHECKED);
   if(enabled)powerChoice->SetCurSel(powerChoice->FindStringExact(-1,cs(*selected)));powerChoice->EnableWindow(enabled);
   const bool canForce=enabled&&completionCanForce(steps);force->SendMessage(BM_SETCHECK,canForce&&yes(current,"CompletionForce")?BST_CHECKED:BST_UNCHECKED);force->EnableWindow(canForce);forceLabel->EnableWindow(canForce);syncingCompletion=false;
  };
  auto savePower=[this,disconnect,exit,power,powerChoice,force]{
   if(syncingCompletion)return;Json current;{Lock lock(manager.mutex);current=job->data;}auto existing=completionSteps(current);std::vector<std::string> steps;
   if(std::find(existing.begin(),existing.end(),"Open downloaded file")!=existing.end())steps.push_back("Open downloaded file");
   if(checked(disconnect))steps.push_back("Disconnect dial-up / VPN");if(checked(exit))steps.push_back("Exit UDM");if(checked(power))steps.push_back(text(powerChoice));
   try{manager.setCompletionPlan(job,steps,checked(force)&&completionCanForce(steps),(int)num(current,"CompletionActionDelay",30),yes(current,"CompletionWaitForOthers",true));}catch(...){syncCompletionControls();throw;}syncCompletionControls();
  };
  for(auto control:{disconnect,exit,power})bind(control,savePower);bindChange(powerChoice,savePower);
  bind(force,[this,force,savePower]{if(checked(force)&&MessageBox(L"Force termination can discard unsaved work in other applications. Enable it for this download?",L"Force processes to terminate",MB_YESNO|MB_ICONWARNING|MB_DEFBUTTON2)!=IDYES){force->SendMessage(BM_SETCHECK,BST_UNCHECKED);return;}savePower();});
  for(auto pair:std::vector<std::pair<CWnd*,CWnd*>>{{disconnectLabel,disconnect},{exitLabel,exit},{powerLabel,power},{forceLabel,force}})bind(pair.first,[pair]{if(pair.second->IsWindowEnabled())pair.second->SendMessage(BM_CLICK);});
  completionControls={disconnect,exit,power,powerChoice,force,disconnectLabel,exitLabel,powerLabel,forceLabel};pages[2]={saveTo,show,separator,powerTitle,disconnect,exit,power,powerChoice,force,disconnectLabel,exitLabel,powerLabel,forceLabel};syncCompletionControls();
  information=edit("",ox+7,oy+4,316,58,true,true);pages[3]={information};
  if(yes(prefs,"ProgressHideTabButtons",true))for(int pageId=1;pageId<=3;++pageId)pages[pageId].push_back(button("Hide tab",ox+252,oy+64,65,[this,pageId]{hidePage(pageId);}));
  accept=[this]{primary();};page();details();update();SetTimer(1,500,nullptr);
 };}

};
BEGIN_MESSAGE_MAP(Progress,Form)
 ON_WM_TIMER()
 ON_WM_DESTROY()
 ON_MESSAGE(WM_APP+144,OnProgressTray)
 ON_WM_CONTEXTMENU()
END_MESSAGE_MAP()
