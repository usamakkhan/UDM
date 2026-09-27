// Included inside udm after RangeMap. Values use binary units, matching the main list.
inline std::string progressBytes(double value){if(value<0)return "Unknown";const char* units[]={"Bytes","KB","MB","GB","TB"};int i=0;while(value>=1024&&i<4){value/=1024;++i;}std::ostringstream out;out<<std::fixed<<std::setprecision(i?3:0)<<value<<" "<<units[i];return out.str();}
inline std::string progressTime(i64 seconds){if(seconds<0)return "--";return (seconds>=3600?std::to_string(seconds/3600)+" hr ":"")+(seconds>=60?std::to_string((seconds/60)%60)+" min ":"")+std::to_string(seconds%60)+" sec";}
class Progress:public Form {
 DECLARE_MESSAGE_MAP()
 Manager& manager;JobPtr job;
 CWnd *status=nullptr,*size=nullptr,*received=nullptr,*rate=nullptr,*remaining=nullptr,*resume=nullptr,*limit=nullptr,*remember=nullptr,*limitEnabled=nullptr,*information=nullptr,*pauseButton=nullptr,*detailsButton=nullptr,*cancelButton=nullptr,*pieces=nullptr;
 bool expanded=true,closeRequested=false;CWnd* completionButton=nullptr;CProgressCtrl* bar=nullptr;RangeMap* rangeMap=nullptr;ThemeList* workers=nullptr;ThemeTabs* tabs=nullptr;std::vector<CWnd*> pages[4];std::vector<int> tabPages;
 void update(){
  if(closeRequested&&IsWindowEnabled()){DestroyWindow();return;}
  Json j;std::vector<Worker> rows;double speed;bool active;{Lock l(manager.mutex);j=job->data;active=manager.isActive(job);speed=job->speed;rows=job->workers;if(job->video&&num(j,"SabrConnections")==0){rows=job->video->workers;if(job->audio)rows.insert(rows.end(),job->audio->workers.begin(),job->audio->workers.end());}}
  auto total=num(j,"Size",-1),done=num(j,"Received");int progress=downloadPermille(j);auto state=str(j,"Status");if(completionButton)completionButton->EnableWindow(state!="Complete");
  pauseButton->SetWindowText(cs(state=="Complete"?"Open":active||state=="Queued"?"Pause":"Start"));pauseButton->EnableWindow(state!="Pausing"&&str(j,"DuplicateOf").empty());cancelButton->SetWindowText(state=="Complete"?L"Close":L"Cancel");
  SetWindowText(cs((progress>=0?std::to_string(progress/10)+"% ":"")+str(j,"FileName")));
  auto message=str(j,"ConnectionStatus",state)+(str(j,"Error").empty()?"":" - "+str(j,"Error"));status->SetWindowText(cs(message));
  size->SetWindowText(cs(progressBytes((double)total)));received->SetWindowText(cs(progressBytes((double)done)+(progress>=0?"  ("+std::to_string(progress/10)+"."+std::to_string(progress%10)+"%)":"")+streamProgressText(j)));
  rate->SetWindowText(cs(progressBytes(speed)+"/sec"));remaining->SetWindowText(cs(progressTime(downloadSecondsLeft(j,speed))));
  resume->SetWindowText(cs(num(j,"AdaptiveTotalSegments")>0?"Completed segments retained":yes(j,"RangeSupported")?"Yes":"No"));bar->SetPos(std::max(0,progress));
  auto duration=num(j,"StreamDurationMs");if(duration>0)rangeMap->update(j.value("SabrRanges",Json::array({{{"Start",0},{"End",duration-1},{"Done",std::clamp<i64>(num(j,"StreamCompletedMs"),0,duration)}}})),duration);else rangeMap->update(j.value("Segments",Json::array()),total);
  workers->SetRedraw(FALSE);while(workers->GetItemCount()>(int)rows.size())workers->DeleteItem(workers->GetItemCount()-1);
  for(int i=0;i<(int)rows.size();++i){auto& w=rows[i];if(i>=workers->GetItemCount())workers->InsertItem(i,cs(std::to_string(i+1)));std::string values[]={progressBytes((double)w.received),w.state,w.end>=w.start?std::to_string((int)std::clamp(100.0*(w.position-w.start)/(w.end-w.start+1),0.0,100.0))+"%":"--"};for(int c=0;c<3;++c)if(utf8((LPCWSTR)workers->GetItemText(i,c+1))!=values[c])workers->SetItemText(i,c+1,cs(values[c]));}
  workers->SetRedraw(TRUE);workers->Invalidate(FALSE);
  information->SetWindowText(cs("Status: "+message+"\r\nSaved as: "+utf8(job->target().wstring())+"\r\n\r\nSHA-256: "+str(j,"Sha256")+"\r\n"+str(j,"FormatDescription")+"\r\nTransferred: "+progressBytes((double)num(j,"TransferredBytes"))+"\r\nNetwork time: "+std::to_string(real(j,"TransferSeconds"))+" s\r\nMerge time: "+std::to_string(real(j,"MergeSeconds"))+" s"));
 }
 void page(){int index=tabs->GetCurSel(),selected=index>=0&&index<(int)tabPages.size()?tabPages[index]:0;for(int i=0;i<4;++i)for(auto w:pages[i])w->ShowWindow(i==selected?SW_SHOW:SW_HIDE);}
 void details(){for(CWnd* w:{(CWnd*)workers,(CWnd*)rangeMap,pieces})w->ShowWindow(expanded?SW_SHOW:SW_HIDE);detailsButton->SetWindowText(expanded?L"<< Hide details":L"Show details >>");resizeClient(450,expanded?360:219);}

 void completionAction(){
  Json record;{Lock lock(manager.mutex);record=job->data;}
  Form d("Action after download",430,219,this);
  d.init=[&]{
   d.label("When this download completes successfully:",14,15,402);
   auto action=d.combo({"None","Open downloaded file","Exit UDM","Disconnect dial-up / VPN","Sleep","Hibernate","Shut down","Restart"},str(record,"CompletionAction","None"),14,42,402);
   d.label("Countdown (seconds)",14,81,209);auto delay=d.edit(std::to_string(num(record,"CompletionActionDelay",30)),282,77,134);
   auto wait=d.check("Wait for other active and queued downloads",yes(record,"CompletionWaitForOthers",true),14,112,402);
   d.label("A countdown appears before the action. You can cancel it.",14,141,402,30);
   d.accept=[&,action,delay,wait]{auto seconds=std::stoll(text(delay));if(seconds<15||seconds>3600)throw std::runtime_error("Use a countdown from 15 to 3600 seconds.");manager.setCompletionAction(job,text(action),(int)seconds,d.checked(wait));d.close();};
   d.defaultButton(d.button("OK",236,181,85,d.accept));d.button("Cancel",331,181,85,[&]{d.close(IDCANCEL);});
  };d.DoModal();
 }
 void primary(){std::string state;bool active;{Lock l(manager.mutex);state=str(job->data,"Status");active=manager.isActive(job);}if(state=="Complete")openFile(this,job->target());else if(active||state=="Queued")manager.pause(job);else manager.resume(job);update();}
 void actions(){CMenu menu;menu.CreatePopupMenu();bool complete,refresh;{Lock l(manager.mutex);complete=str(job->data,"Status")=="Complete";refresh=manager.canRefreshAddress(job);}menu.AppendMenu(MF_STRING|(refresh?MF_ENABLED:MF_GRAYED),1,L"Refresh download address...");menu.AppendMenu(MF_STRING,2,L"Download properties...");menu.AppendMenu(MF_STRING,3,L"Copy download address");menu.AppendMenu(MF_SEPARATOR);menu.AppendMenu(MF_STRING,4,L"Open folder");menu.AppendMenu(MF_STRING|(complete?MF_ENABLED:MF_GRAYED),5,L"Open with...");menu.AppendMenu(MF_SEPARATOR);menu.AppendMenu(MF_STRING,6,L"Hide progress window");CPoint p;GetCursorPos(&p);auto choice=menu.TrackPopupMenu(TPM_RETURNCMD|TPM_LEFTALIGN,p.x,p.y,this);if(choice==1)refreshDownloadAddress(this,manager,job);else if(choice==2)fileProperties(this,manager,job);else if(choice==3){std::string address;{Lock lock(manager.mutex);address=downloadAddress(job->data);}auto value=wide(address);if(OpenClipboard()){EmptyClipboard();auto memory=GlobalAlloc(GMEM_MOVEABLE,(value.size()+1)*sizeof(wchar_t));if(memory){auto target=GlobalLock(memory);if(target){memcpy(target,value.c_str(),(value.size()+1)*sizeof(wchar_t));GlobalUnlock(memory);if(!SetClipboardData(CF_UNICODETEXT,memory))GlobalFree(memory);}else GlobalFree(memory);}CloseClipboard();}}else if(choice==4)openFile(this,job->target().parent_path());else if(choice==5)openWith(this,job->target());else if(choice==6)DestroyWindow();}
 afx_msg void OnTimer(UINT_PTR id){try{update();}catch(const std::exception& e){status->SetWindowText(cs(e.what()));}if(GetSafeHwnd())Form::OnTimer(id);}
 BOOL OnNotify(WPARAM w,LPARAM l,LRESULT* result)override{auto hdr=(NMHDR*)l;if(hdr->hwndFrom==tabs->m_hWnd&&hdr->code==TCN_SELCHANGE){page();*result=0;return TRUE;}return Form::OnNotify(w,l,result);}
public:
 void requestCompletionClose(){if(!IsWindowEnabled()){closeRequested=true;return;}DestroyWindow();}
 std::string downloadId()const{return job->id();}
 Progress(Manager& m,JobPtr j,CWnd* owner):Form("Download status",450,360,owner),manager(m),job(j){modeless=true;cancel=[this]{manager.pause(job);DestroyWindow();};init=[this]{
  ModifyStyle(0,WS_MINIMIZEBOX);Json prefs,record;{Lock lock(manager.mutex);prefs=manager.state["Settings"];record=job->data;}expanded=str(prefs,"ProgressStartMode","Normal size")!="Small size";
  tabs=make<ThemeTabs>(WS_TABSTOP,10,7,430,155);tabs->InsertItem(0,L"Download status");tabPages.push_back(0);if(yes(prefs,"ProgressSpeedTab",true)){tabs->InsertItem((int)tabPages.size(),L"Speed Limiter");tabPages.push_back(1);}tabs->InsertItem((int)tabPages.size(),L"Options on completion");tabPages.push_back(2);if(yes(prefs,"ProgressInformationTab",true)){tabs->InsertItem((int)tabPages.size(),L"Information");tabPages.push_back(3);}
  auto url=edit(downloadAddress(record),20,33,408,18,true);url->ModifyStyleEx(WS_EX_CLIENTEDGE,0,SWP_FRAMECHANGED);
  pages[0].push_back(url);const char* captions[]={"Status","File size","Downloaded","Transfer rate","Time left","Resume capability"};CWnd** values[]={&status,&size,&received,&rate,&remaining,&resume};
  for(int i=0;i<6;++i){pages[0].push_back(label(captions[i],22,55+16*i,100));*values[i]=label("",128,55+16*i,300,18);pages[0].push_back(*values[i]);}
  bar=make<CProgressCtrl>(PBS_SMOOTH,10,165,430,13);bar->SetRange32(0,1000);if(uiDark){SetWindowTheme(bar->GetSafeHwnd(),L"",L"");bar->SetBkColor(RGB(48,51,56));bar->SetBarColor(RGB(35,139,63));}
  detailsButton=button("<< Hide details",17,185,105,[this]{expanded=!expanded;details();});button("Actions",130,185,72,[this]{actions();});pauseButton=button("Pause",275,185,74,[this]{primary();});cancelButton=button("Cancel",357,185,74,[this]{manager.pause(job);DestroyWindow();});
  pieces=control(L"STATIC","Start positions and download progress by connections",SS_CENTER,10,216,430,16);rangeMap=make<RangeMap>(0,10,234,430,10);workers=make<ThemeList>(WS_TABSTOP|WS_BORDER|LVS_SHOWSELALWAYS|LVS_REPORT|LVS_SINGLESEL|LVS_NOSORTHEADER,10,254,430,96);workers->SetExtendedStyle(LVS_EX_FULLROWSELECT|LVS_EX_DOUBLEBUFFER);workers->theme();workers->InsertColumn(0,L"N.",LVCFMT_LEFT,(int)(25*scale));workers->InsertColumn(1,L"Downloaded",LVCFMT_LEFT,(int)(88*scale));workers->InsertColumn(2,L"Info",LVCFMT_LEFT,(int)(240*scale));workers->InsertColumn(3,L"Progress",LVCFMT_LEFT,(int)(54*scale));
  limitEnabled=check("Use Speed Limiter",num(record,"LimitKbps")>0,23,43,396);auto hint=label("Maximum download speed (KB/sec)",23,78,247);limit=edit(std::to_string(num(record,"LimitKbps",1000)),280,74,137);remember=check("Remember this limit for this download",false,23,110,396);auto apply=button("Apply",343,121,74,[this]{auto kb=checked(limitEnabled)?std::stoll(text(limit)):0;if(kb<0||kb>1000000)throw std::runtime_error("Use a speed limit from 0 to 1,000,000 KB/s.");Lock l(manager.mutex);if(checked(remember)){job->data["LimitKbps"]=kb;job->sessionLimit.reset();manager.save();}else job->sessionLimit=kb;});pages[1]={limitEnabled,hint,limit,remember,apply};
  auto saveTo=label("Save To: "+utf8(job->target().wstring()),22,36,406,24);auto show=check("Show download complete dialog",!yes(record,"SuppressCompletionDialog")&&!yes(prefs,"SuppressCompletionDialog"),22,63,396);show->EnableWindow(!yes(prefs,"SuppressCompletionDialog"));auto close=check("Close this progress window when done",yes(record,"CloseProgressOnCompletion",true),22,84,396);auto folder=check("Open the download folder when done",yes(record,"OpenFolderOnCompletion"),22,105,396);auto saveOptions=[this,show,close,folder]{Lock lock(manager.mutex);job->data["SuppressCompletionDialog"]=!checked(show);job->data["CloseProgressOnCompletion"]=checked(close);job->data["OpenFolderOnCompletion"]=checked(folder);manager.save();};bind(show,saveOptions);bind(close,saveOptions);bind(folder,saveOptions);completionButton=button("Completion action...",265,128,154,[this]{completionAction();});completionButton->EnableWindow(str(record,"Status")!="Complete");pages[2]={saveTo,show,close,folder,completionButton};
  information=edit("",22,36,406,108,true,true);pages[3]={information};accept=[this]{primary();};page();details();update();SetTimer(1,500,nullptr);
 };}
};
BEGIN_MESSAGE_MAP(Progress,Form)
 ON_WM_TIMER()
END_MESSAGE_MAP()
