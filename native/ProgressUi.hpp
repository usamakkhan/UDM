// Included inside udm after RangeMap. Values use binary units, matching the main list.
inline std::string progressBytes(double value){if(value<0)return "Unknown";const char* units[]={"Bytes","KB","MB","GB","TB"};int i=0;while(value>=1024&&i<4){value/=1024;++i;}std::ostringstream out;out<<std::fixed<<std::setprecision(i?3:0)<<value<<" "<<units[i];return out.str();}
inline std::string connectionDownloadedText(i64 received){
 if(received<1024)return progressBytes((double)received);
 auto kilobytes=std::to_string(received/1024);
 for(size_t position=kilobytes.size();position>3;){position-=3;kilobytes.insert(position,1,',');}
 return kilobytes+" KB";
}
inline std::string progressTime(i64 seconds){if(seconds<0)return "--";return (seconds>=3600?std::to_string(seconds/3600)+" hr ":"")+(seconds>=60?std::to_string((seconds/60)%60)+" min ":"")+std::to_string(seconds%60)+" sec";}
inline std::string progressPercent(const Json& record){
 if(str(record,"Status")=="Complete")return "100.00";
 if(yes(record,"LiveRecording"))return {};
 auto total=num(record,"StreamDurationMs"),done=num(record,"StreamCompletedMs");bool media=total>0;
 if(!media){total=adaptiveSourceBytes(record);done=num(record,"Received");media=total>0;}
 if(!media){total=num(record,"AdaptiveTotalSegments");done=num(record,"AdaptiveCompletedSegments");media=total>0;}
 if(!media){total=num(record,"Size",-1);done=num(record,"Received");}
 if(total<=0)return {};
 auto percent=std::clamp(100.0*static_cast<double>(done)/static_cast<double>(total),0.0,media?99.99:100.0);
 std::ostringstream out;out<<std::fixed<<std::setprecision(2)<<percent;return out.str();
}
inline bool capturedResponseSavePending(const Json& record){
 if(str(record,"Status")=="Complete")return false;
 const auto response=record.find("OriginalResponse");
 return response!=record.end()&&(str(*response,"State")=="Ready"||str(*response,"State")=="Publishing");
}
inline std::string progressResumeCapability(const Json& record){
 if(capturedResponseSavePending(record))return "Response saved";
 if(!str(record,"ProtectedRequest").empty()&&str(record,"ProtectedPostGetUrl").empty())return "No";
 if(savedMediaOutputSize(record)>=0)return "Saved output";
 if(yes(record,"MediaTracksReady"))return "Streams saved";
 if(yes(record,"AdaptiveRangeResume")&&adaptiveSourceBytes(record)>0)return "Yes (saved ranges)";
 if(yes(record,"SabrResumeSupported")||num(record,"AdaptiveTotalSegments")>0)return "Yes (saved segments)";
 auto capability=[](const Json& item){return item.contains("RangeSupported")&&item["RangeSupported"].is_boolean()?(yes(item,"RangeSupported")?1:0):-1;};
 bool tracks=false,unknown=false;
 for(auto name:{"Video","Audio"})if(record.contains(name)&&record[name].is_object()){
  tracks=true;auto support=capability(record[name]);if(support==0)return "No";unknown|=support<0;
 }
 if(tracks)return unknown?"Unknown":"Yes";
 auto support=capability(record);return support<0?"Unknown":support?"Yes":"No";
}
inline bool progressNeedsFormRestart(const Json& record){auto state=str(record,"Status");return (state=="Paused"||state=="Failed")&&yes(record,"PostAttempted")&&!str(record,"ProtectedRequest").empty()&&str(record,"ProtectedPostGetUrl").empty();}
inline bool liveSavePending(const Json& record){
 if(!yes(record,"LiveRecording")||str(record,"Status")=="Complete")return false;
 if(yes(record,"LiveStopRequested")||yes(record,"LiveCaptureSealed")||str(record,"Status")=="Merging")return true;
 const auto receipt=record.find("RetainedMediaOutput");
 return receipt!=record.end()&&str(*receipt,"Kind")=="LiveHls";
}
inline bool canStopAndSaveLive(const Json& record){
 return yes(record,"LiveRecording")&&str(record,"Status")!="Complete"&&yes(record,"LiveSaveReady")&&!liveSavePending(record);
}
inline bool progressPrimaryEnabled(const Json& record,bool active){
 const auto state=str(record,"Status");const bool live=yes(record,"LiveRecording")&&state!="Complete";
 return state!="Pausing"&&str(record,"DuplicateOf").empty()&&(!live||canStopAndSaveLive(record)||!active);
}
inline bool progressDismissStops(const std::string& state,bool active){
 return state=="Queued"||(active&&state!="Complete"&&state!="Paused"&&state!="Pausing"&&state!="Failed");
}
enum class ProgressAction { Wait, StopScanWait, Open, Pause, SaveLive, Refresh, Resume };
inline std::optional<ProgressAction> progressCompletedAction(const Json& record,bool active){
 if(str(record,"Status")!="Complete")return std::nullopt;
 const auto scan=record.find("ScanResult");
 const auto state=scan==record.end()?std::string{}:str(*scan,"Status");
 if(active&&state=="Running")return ProgressAction::StopScanWait;
 return completedFileOpenReady(record,active)?ProgressAction::Open:ProgressAction::Wait;
}
inline ProgressAction progressPrimaryAction(const Json& record,bool active){
 if(auto completed=progressCompletedAction(record,active))return *completed;
 if(str(record,"Status")=="Queued")return ProgressAction::Pause;
 if(canStopAndSaveLive(record))return ProgressAction::SaveLive;
 if(active)return ProgressAction::Pause;
 return yes(record,"SabrSessionRefreshRequired")?ProgressAction::Refresh:ProgressAction::Resume;
}
inline bool progressOpeningPrefetch(const Job& job,std::chrono::steady_clock::time_point now){
 if(job.prefetchPeakDisplayUntil)return now<*job.prefetchPeakDisplayUntil;
 return job.prefetchConfirmedAt&&now-*job.prefetchConfirmedAt<std::chrono::seconds(10);
}
#include "ProgressGraphUi.hpp"
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
  if(message==WM_CLOSE){OnCancel();return 0;}
  static const UINT recreated=RegisterWindowMessageW(L"TaskbarCreated");
  if(recreated&&message==recreated&&trayVisible){trayVisible=false;if(!showInTray())showNormally();}
  return Form::WindowProc(message,w,l);
 }
 CWnd *addressField=nullptr,*saveTo=nullptr,*status=nullptr,*size=nullptr,*received=nullptr,*rate=nullptr,*remaining=nullptr,*resume=nullptr,*limit=nullptr,*remember=nullptr,*limitEnabled=nullptr,*information=nullptr,*pauseButton=nullptr,*detailsButton=nullptr,*cancelButton=nullptr,*pieces=nullptr,*limitRate=nullptr,*limitHint=nullptr,*limitCaption=nullptr;
 Json displayedAddressInputs=Json::object();
 static void updateText(CWnd* field,const std::string& value){
  if(text(field)!=value)field->SetWindowText(cs(value));
 }
 Json addressInputs()const{return downloadAddressInputs(*job);}
 void syncAddress(Json inputs){
  if(inputs==displayedAddressInputs)return;
  const auto value=downloadAddress(inputs);
  updateText(addressField,value);
  displayedAddressInputs=std::move(inputs);
 }
 bool expanded=true,showRateGraph=false,closeRequested=false,syncingLimit=false;
 std::optional<std::chrono::steady_clock::time_point> closeAfter;
 std::function<void(const Json&,bool)> syncCompletionControls;
 std::optional<ProgressAction> displayedPrimaryAction;
 bool displayedDismissStops=false;
 CProgressCtrl* bar=nullptr;RangeMap* rangeMap=nullptr;ProgressRateGraph* rateGraph=nullptr;CWnd* graphToggle=nullptr;
 ThemeList* workers=nullptr;ThemeTabs* tabs=nullptr;std::vector<CWnd*> pages[4];std::vector<int> tabPages;
 void update(){
  const auto now=std::chrono::steady_clock::now();
  const bool showConnections=expanded&&!showRateGraph;
  if(closeRequested&&IsWindowEnabled()&&(!closeAfter||now>=*closeAfter||!IsWindowVisible()||IsIconic())){
   DestroyWindow();return;
  }
  struct TrackProgress {i64 size;bool complete;Json segments;size_t firstRow,endRow;};
  Json j=Json::object(),address;std::vector<Worker> rows;std::vector<TrackProgress> tracks;
  std::vector<RangeMap::Span> adaptiveCoverage;
  i64 adaptiveMapSize=0;
  std::optional<ResponseSaveProgress> responseSave;SavedDataCheckProgress savedDataCheck;
  double speed;bool active,suppressCompletion,relocating,openingPrefetch=false;
  {
   Lock l(manager.mutex);
   for(auto it=job->data.begin();it!=job->data.end();++it){
    const auto& key=it.key();
    if(key=="ProtectedAdaptive"||key=="ProtectedAdaptiveDisplayAddress"||key=="ProtectedSabr"||
       key=="ProtectedResolvedUrl"||key=="ProtectedMediaCookies"||key=="ProtectedMediaHeaders"||
       key=="ProtectedBrowserSession"||key=="Video"||key=="Audio")continue;
    if(!showConnections&&(key=="Segments"||key=="SabrRanges"))continue;
    j[key]=it.value();
   }
   address=addressInputs();responseSave=job->responseSaveProgress;savedDataCheck=job->savedDataCheck;
   suppressCompletion=yes(manager.state["Settings"],"SuppressCompletionDialog");
   active=manager.isActive(job);relocating=manager.isRelocating(job);speed=job->speed;
   openingPrefetch=progressOpeningPrefetch(*job,now);
   if(openingPrefetch&&!job->prefetchPeakDisplayUntil&&IsWindowVisible()&&!IsIconic()&&
      rate->IsWindowVisible()&&!responseSave&&savedDataCheck.total<=0)
    job->prefetchPeakDisplayUntil=now+std::chrono::seconds(2);
   const bool directTracks=(job->video||job->audio)&&num(j,"SabrConnections")==0;
   const auto addTrack=[&](const JobPtr& track,const char* name){
    if(!track)return;
    const auto& data=track->data;
    j[name]={{"Url",str(data,"Url")},{"RangeSupported",data.value("RangeSupported",Json())}};
    if(!directTracks||!showConnections)return;
    const auto firstRow=rows.size();
    rows.insert(rows.end(),track->workers.begin(),track->workers.end());
    tracks.push_back({num(data,"Size",-1),str(data,"Status")=="Complete",data.value("Segments",Json::array()),firstRow,rows.size()});
   };
   addTrack(job->video,"Video");addTrack(job->audio,"Audio");
   if(showConnections&&!directTracks)rows=job->workers;
   if(showConnections&&!job->adaptiveRanges.empty()&&str(j,"Status")!="Complete"){
    adaptiveMapSize=adaptiveSourceBytes(j);
    if(adaptiveMapSize>0)for(const auto& range:job->adaptiveRanges){
     const auto done=std::clamp<i64>(range.received,0,range.length);
     if(done<=0)continue;
     if(!adaptiveCoverage.empty()&&adaptiveCoverage.back().second==range.start)
      adaptiveCoverage.back().second+=done;
     else adaptiveCoverage.emplace_back(range.start,range.start+done);
    }
   }
  }
  syncCompletionControls(j,suppressCompletion);
  syncAddress(std::move(address));
  const auto destination=utf8((fs::path(wide(str(j,"Folder")))/wide(str(j,"FileName"))).wstring());
  updateText(saveTo,"Save To: "+destination);
  auto total=num(j,"Size",-1),done=num(j,"Received");int progress=downloadPermille(j);auto state=str(j,"Status");bool scanning=str(j.value("ScanResult",Json::object()),"Status")=="Running";
  const bool live=yes(j,"LiveRecording")&&state!="Complete",saving=liveSavePending(j);
  const bool capturedSave=capturedResponseSavePending(j);
  std::string captureStatus;
  if(capturedSave){
   total=responseSave?responseSave->total:done;
   progress=-1;
   if(state=="Pausing")captureStatus="Pausing save";
   else if(active&&responseSave){
    switch(responseSave->phase){
     case ResponseSaveProgress::Phase::VerifyingResponse:captureStatus="Verifying received response";break;
     case ResponseSaveProgress::Phase::Copying:
      captureStatus="Copying to destination";
      if(total>0){
       progress=static_cast<int>(std::clamp(1000.0*static_cast<double>(responseSave->copied)/static_cast<double>(total),0.0,1000.0));
       captureStatus+=" ("+std::to_string(progress/10)+"%)";
      }
      break;
     case ResponseSaveProgress::Phase::VerifyingCopy:captureStatus="Verifying saved copy";progress=1000;break;
     case ResponseSaveProgress::Phase::Publishing:captureStatus="Saving file";progress=1000;break;
    }
   }else if(active)captureStatus="Preparing to save";
   else if(state=="Failed")captureStatus="Save failed";
   else captureStatus=yes(j,"ResponseSaveDeferred")?"Save paused":"Ready to save";
  }
  const auto resumeCapability=progressResumeCapability(j);
  // A saved checkpoint can be offered for resume before the server has proved range support.
  const bool resumablePartial=!live&&(state=="Paused"||state=="Failed")&&
   (done>0||num(j,"StreamCompletedMs")>0||num(j,"AdaptiveCompletedSegments")>0)&&
   (resumeCapability=="Unknown"||resumeCapability=="Yes"||resumeCapability=="Yes (saved segments)"||resumeCapability=="Yes (saved ranges)");
  const auto primaryAction=progressPrimaryAction(j,active);
  const char* action="Start";
  switch(primaryAction){
   case ProgressAction::Wait:
   case ProgressAction::Open: action="Open";break;
   case ProgressAction::StopScanWait: action="Stop wait";break;
   case ProgressAction::Pause:
    action=state=="Queued"||capturedSave||!live?"Pause":saving?"Saving...":"Recording...";
    break;
   case ProgressAction::SaveLive: action=active?"Stop and save":"Save recording";break;
   case ProgressAction::Refresh: action="Refresh...";break;
   case ProgressAction::Resume:
    if(capturedSave||(live&&saving))action="Retry save";
    else if(progressNeedsFormRestart(j))action="Restart";
    else if(yes(j,"MediaTracksReady")||savedMediaOutputSize(j)>=0)action="Retry";
    else if(resumablePartial)action="Resume";
    break;
  }
  if(state=="Pausing")action="Pausing...";
  const bool primaryEnabled=!relocating&&primaryAction!=ProgressAction::Wait&&progressPrimaryEnabled(j,active);
  displayedPrimaryAction=primaryEnabled?std::optional{primaryAction}:std::nullopt;
  updateText(pauseButton,action);
  if((pauseButton->IsWindowEnabled()!=FALSE)!=primaryEnabled)pauseButton->EnableWindow(primaryEnabled);
  displayedDismissStops=!relocating&&progressDismissStops(state,active);
  updateText(cancelButton,displayedDismissStops?"Cancel":"Close");
  const bool showPercent=progress>=0&&(!capturedSave||(responseSave&&responseSave->phase==ResponseSaveProgress::Phase::Copying));
  updateText(this,(showPercent?std::to_string(progress/10)+"% ":"")+str(j,"FileName"));
  refreshTrayCaption();
  auto quota=manager.quotaStatus(job);
  const bool checkingSavedData=active&&savedDataCheck.total>0;
  auto connectionStatus=str(j,"ConnectionStatus",state);
  if(checkingSavedData&&state!="Pausing"){
   const auto percent=(int)(100.0*savedDataCheck.checked/savedDataCheck.total);
   connectionStatus="Checking saved download data ("+std::to_string(percent)+"%)";
  }
  auto detail=capturedSave&&active?std::string{}:str(j,"Error");if(detail.empty()&&progressNeedsFormRestart(j))detail="Restart resubmits this form.";auto message=(capturedSave?captureStatus:yes(quota,"Waiting")?quotaWaitText(quota):scanning?std::string("Scanning file"):connectionStatus)+(detail.empty()?"":" - "+detail);
  auto sizeText=progressBytes((double)total);
  if(total<0){if(auto tracks=std::max(adaptiveSourceBytes(j),reportedMediaSize(j));tracks>0)sizeText=progressBytes((double)tracks)+" (tracks; final size pending)";else if(auto estimate=mediaSizeHint(j);estimate>0)sizeText="~ "+progressBytes((double)estimate)+" (estimated)";}
  const auto publishingSize=state=="Merging"&&str(j,"MediaPhase")=="Publishing prepared file"?num(j,"PrefetchedFinalSize",-1):-1;
  const auto savedSize=savedMediaOutputSize(j);
  if(publishingSize>=0){sizeText=std::to_string(publishingSize)+" bytes (verified output)";message="Publishing prepared file";}
  else if(savedSize>=0)sizeText=std::to_string(savedSize)+" bytes (saved output)";
  updateText(status,message);
  const auto prefetched=num(j,"PrefetchedBytesAtStart");
  const auto prefetchText=prefetched>0?" ("+progressBytes((double)prefetched)+" before Start)":yes(j,"ContinuedMediaPrefetch")&&done>0?" (includes prefetch)":"";
  const auto percent=progressPercent(j);
  updateText(size,sizeText);
  updateText(received,progressBytes((double)done)+(percent.empty()?"":"  ( "+percent+" % )")+streamProgressText(j)+prefetchText);
  const auto rateText=capturedSave||checkingSavedData?"--":progressBytes(speed)+"/sec";
  const auto prefetchPeak=num(j,"PrefetchPeakRate");
  const auto peakText=prefetched>0&&prefetchPeak>0?"Prefetch peak "+progressBytes((double)prefetchPeak)+"/sec":std::string{};
  const auto showPeak=!capturedSave&&!checkingSavedData&&!peakText.empty();
  if(showPeak&&openingPrefetch&&(state=="Downloading"||state=="Merging"||state=="Complete"))
   updateText(rate,progressBytes((double)prefetchPeak)+"/sec (before Start)");
  else updateText(rate,showPeak?rateText+" ("+peakText+")":rateText);
  updateText(limitRate,rateText);
  const bool receiving=active&&state=="Downloading"&&!capturedSave&&!checkingSavedData&&!saving;
  rateGraph->record(now,receiving?std::optional{speed}:std::nullopt);
  updateText(remaining,capturedSave||checkingSavedData?"--":live?(saving?"--":"Until stopped"):progressTime(downloadSecondsLeft(j,speed)));
  updateText(resume,resumeCapability);
  const int position=std::max(0,progress);if(bar->GetPos()!=position)bar->SetPos(position);
  if(showConnections){
   std::vector<RangeMap::Span> coverage;
   std::vector<i64> starts;
   auto mapSize=total;
   const auto appendStarts=[&](i64 offset,i64 extent,size_t first,size_t end){
    if(!active)return;
    for(size_t i=first;i<end;++i){
     const auto& worker=rows[i];
     if(worker.state=="Receiving data"&&worker.receiveStart>=0&&worker.receiveStart<extent)
      starts.push_back(offset+worker.receiveStart);
    }
   };
   const auto duration=num(j,"StreamDurationMs");
   if(state=="Complete"&&total>0)coverage.emplace_back(0,total);
   else if(adaptiveMapSize>0){
    mapSize=adaptiveMapSize;
    coverage=std::move(adaptiveCoverage);
    if(active)for(const auto& row:rows)if(row.receiveStart>=0&&row.receiveStart<mapSize)starts.push_back(row.receiveStart);
   }
   else if(duration>0){
    mapSize=duration;
    if(j.contains("SabrRanges"))RangeMap::appendCoverage(coverage,j["SabrRanges"],duration);
    else if(const auto receivedMs=std::clamp<i64>(num(j,"StreamCompletedMs"),0,duration);receivedMs>0)coverage.emplace_back(0,receivedMs);
   }else if(!tracks.empty()){
    // The scale here is downloaded track bytes, independent of the final muxed size.
    mapSize=0;
    for(const auto& track:tracks){
     if(track.size<0||track.size>(std::numeric_limits<i64>::max)()-mapSize){mapSize=0;coverage.clear();starts.clear();break;}
     if(track.complete&&track.size>0)coverage.emplace_back(mapSize,mapSize+track.size);
     else RangeMap::appendCoverage(coverage,track.segments,track.size,mapSize);
     appendStarts(mapSize,track.size,track.firstRow,track.endRow);
     mapSize+=track.size;
    }
   }else{
    if(const auto plan=j.find("Segments");plan!=j.end())RangeMap::appendCoverage(coverage,*plan,mapSize);
    appendStarts(0,mapSize,0,rows.size());
   }
   rangeMap->update(std::move(coverage),mapSize,std::move(starts));
   struct ConnectionDetail {std::string number; i64 bytes; std::string info;};
   std::vector<ConnectionDetail> details;
   const bool complete=state=="Complete";
   const bool measured=std::any_of(rows.begin(),rows.end(),[](const Worker& worker){return worker.received>0;});
   if(!measured&&!checkingSavedData){
    if((complete||state=="Verifying")&&yes(j,"ReusedCompletedPrefetch")){
     const auto receipt=j.find("CompletedPrefetch");
     const auto reused=receipt==j.end()?done:num(*receipt,"Size",done);
     details.push_back({"",reused,"Verified saved prefetch"});
    }else if((complete||publishingSize>=0)&&yes(j,"ReusedPrefetchedOutput")){
     details.push_back({"",complete?total:publishingSize,"Verified saved output"});
    }else if(complete&&prefetched>0){
     details.push_back({"",prefetched,"Received before Start"});
    }
   }
   if(details.empty())for(size_t i=0;i<rows.size();++i){
    const auto& worker=rows[i];
    if(complete&&worker.received<=0)continue;
    const bool inProgress=worker.state=="Receiving data"||worker.state=="Connecting";
    details.push_back({std::to_string(i),worker.received,inProgress?worker.state+"...":worker.state});
   }
   while(workers->GetItemCount()>(int)details.size())workers->DeleteItem(workers->GetItemCount()-1);
   for(int i=0;i<(int)details.size();++i){
    const auto& row=details[i];
    if(i>=workers->GetItemCount())workers->InsertItem(i,cs(row.number));
    const std::string values[]={row.number,connectionDownloadedText(row.bytes),row.info};
    for(int column=0;column<3;++column)
     if(utf8((LPCWSTR)workers->GetItemText(i,column))!=values[column])workers->SetItemText(i,column,cs(values[column]));
   }
  }
  const auto captionSummary=subtitleCaptureSummary(j);
  updateText(information,"Status: "+message+"\r\nSaved as: "+destination+"\r\n\r\nSHA-256: "+str(j,"Sha256")+"\r\n"+str(j,"FormatDescription")+(captionSummary.empty()?std::string{}:"\r\n"+captionSummary)+"\r\nTransferred: "+progressBytes((double)num(j,"TransferredBytes"))+(peakText.empty()?std::string{}:"\r\n"+peakText)+"\r\nRetained media: "+progressBytes((double)num(j,"SabrRetainedBytes"))+"\r\nReused this attempt: "+progressBytes((double)num(j,"SabrReusedBytes"))+"\r\nMedia phase: "+str(j,"MediaPhase")+"\r\nNetwork time: "+std::to_string(real(j,"TransferSeconds"))+" s\r\nMerge time: "+std::to_string(real(j,"MergeSeconds"))+" s\r\nVirus check: "+scannerSummary(j));
 }
 void page(){int index=tabs->GetCurSel(),selected=index>=0&&index<(int)tabPages.size()?tabPages[index]:0;for(int i=0;i<4;++i)for(auto w:pages[i])w->ShowWindow(i==selected?SW_SHOW:SW_HIDE);}
 void detailView(){
  for(CWnd* w:{static_cast<CWnd*>(workers),static_cast<CWnd*>(rangeMap),pieces})w->ShowWindow(expanded&&!showRateGraph?SW_SHOW:SW_HIDE);
  rateGraph->ShowWindow(expanded&&showRateGraph?SW_SHOW:SW_HIDE);
  graphToggle->ShowWindow(expanded?SW_SHOW:SW_HIDE);
  graphToggle->SetWindowText(showRateGraph?L"Show connections":L"Show speed graph");
 }
 void details(){detailView();detailsButton->SetWindowText(expanded?L"<< Hide details":L"Show details >>");resizeClient(353,expanded?250:148);}

 void hidePage(int selected){
  auto found=std::find(tabPages.begin(),tabPages.end(),selected);if(found==tabPages.end()||selected==0)return;
  const auto index=(int)std::distance(tabPages.begin(),found);{Lock lock(manager.mutex);auto prefs=manager.state["Settings"];prefs[selected==1?"ProgressSpeedTab":selected==2?"ProgressCompletionTab":"ProgressInformationTab"]=false;manager.setSettings(prefs);}
  tabs->DeleteItem(index);tabPages.erase(found);tabs->SetCurSel(0);page();tabs->SetFocus();
 }
 void enableLimitFields(bool enabled){
  for(auto field:{limit,limitCaption,limitHint})field->EnableWindow(enabled);
 }
 void syncLimitControls(){
  i64 kb;bool persistent;{Lock lock(manager.mutex);kb=job->sessionLimit.value_or(num(job->data,"LimitKbps"));persistent=!job->sessionLimit.has_value()&&num(job->data,"LimitKbps")>0;}
  syncingLimit=true;limitEnabled->SendMessage(BM_SETCHECK,kb>0?BST_CHECKED:BST_UNCHECKED);remember->SendMessage(BM_SETCHECK,persistent?BST_CHECKED:BST_UNCHECKED);
  limit->SetWindowText(cs(std::to_string(kb>0?kb:1000)));enableLimitFields(kb>0);limitHint->SetWindowText(L"KBytes/sec");syncingLimit=false;
 }
 void applyLimit(){
  if(syncingLimit)return;
  const bool enabled=checked(limitEnabled);enableLimitFields(enabled);i64 kb=0;
  if(enabled){const auto value=trim(text(limit));if(!std::regex_match(value,std::regex("[0-9]{1,7}"))||std::stoll(value)<1||std::stoll(value)>1000000){limitHint->SetWindowText(L"1..1000000");return;}kb=std::stoll(value);}
  try{setProgressLimit(manager,job,kb,checked(remember));}catch(...){syncLimitControls();throw;}limitHint->SetWindowText(L"KBytes/sec");
 }
 afx_msg void OnContextMenu(CWnd*,CPoint){try{actions();}catch(const std::exception& e){error(this,e);}}
 void completionAction(){
  Json record;{Lock lock(manager.mutex);record=job->data;}
  if(str(record,"Status")=="Complete")return;
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
    Lock lock(manager.mutex);auto before=job->data;
    if(str(before,"Status")=="Complete")throw std::runtime_error("Set completion actions before the download finishes.");
    auto actions=completionSteps(before);actions.erase(std::remove(actions.begin(),actions.end(),"Open downloaded file"),actions.end());
    if(showDownloadCompletionDialog(before,yes(manager.state["Settings"],"SuppressCompletionDialog")))actions.clear();
    if(d.checked(open))actions.push_back("Open downloaded file");
    job->data["CloseProgressOnCompletion"]=d.checked(close);job->data["OpenFolderOnCompletion"]=d.checked(folder);
    try{manager.setCompletionPlan(job,actions,yes(before,"CompletionForce")&&completionCanForce(actions),(int)seconds,d.checked(wait));}catch(...){job->data=before;throw;}d.close();
   };
   d.defaultButton(d.button("OK",261,229,85,d.accept));d.button("Cancel",356,229,85,[&]{d.close(IDCANCEL);});
  };d.DoModal();
 }
 void dismiss(){
  {
   Lock lock(manager.mutex);
   if(displayedDismissStops&&!manager.isRelocating(job)&&progressDismissStops(str(job->data,"Status"),manager.isActive(job)))
    manager.pause(job);
  }
  DestroyWindow();
 }
 void primary(){
  ProgressAction action;
  fs::path target;
  {
   Lock lock(manager.mutex);
   const bool active=manager.isActive(job);
   action=progressPrimaryAction(job->data,active);
   if(action!=displayedPrimaryAction||manager.isRelocating(job)||!progressPrimaryEnabled(job->data,active)){
    PostMessage(WM_TIMER,1);return;
   }
   if(action==ProgressAction::Pause||action==ProgressAction::StopScanWait)manager.pause(job);
   else if(action==ProgressAction::SaveLive)requestLiveHlsFinish(manager,job);
   else if(action==ProgressAction::Open)target=job->target();
  }
  if(action==ProgressAction::Open)openDownloadedFile(this,target);
  else if(action==ProgressAction::Refresh)refreshDownloadAddress(this,manager,job);
  else if(action==ProgressAction::Resume){
   if(!requestQueuedDownloadLogin(this,manager,job))manager.resume(job);
   syncLimitControls();
  }
  update();
 }
 void actions(){CMenu menu;menu.CreatePopupMenu();bool complete,refresh,canOpen;{Lock l(manager.mutex);complete=str(job->data,"Status")=="Complete";refresh=manager.canRefreshAddress(job);canOpen=!manager.isRelocating(job)&&progressCompletedAction(job->data,manager.isActive(job))==ProgressAction::Open;}menu.AppendMenu(MF_STRING|(refresh?MF_ENABLED:MF_GRAYED),1,L"Refresh download address...");menu.AppendMenu(MF_STRING,2,L"Download properties...");menu.AppendMenu(MF_STRING,3,L"Copy download address");menu.AppendMenu(MF_SEPARATOR);menu.AppendMenu(MF_STRING,4,L"Open folder");menu.AppendMenu(MF_STRING|(canOpen?MF_ENABLED:MF_GRAYED),5,L"Open with...");menu.AppendMenu(MF_SEPARATOR);menu.AppendMenu(MF_STRING,6,L"Hide progress window");menu.AppendMenu(MF_STRING,7,L"Customize download progress dialog...");menu.AppendMenu(MF_STRING,8,L"Download information...");menu.AppendMenu(MF_STRING|(complete?MF_GRAYED:MF_ENABLED),9,L"Additional completion options...");bool recording=false,recordingActive=false,recordingCanResume=false,saving=false;{Lock lock(manager.mutex);recording=yes(job->data,"LiveRecording")&&!complete;recordingActive=manager.isActive(job);saving=liveSavePending(job->data);recordingCanResume=saving?(!recordingActive&&str(job->data,"Status")!="Queued"):str(job->data,"Status")!="Pausing";}if(recording){menu.AppendMenu(MF_SEPARATOR);menu.AppendMenu(MF_STRING|(recordingCanResume?MF_ENABLED:MF_GRAYED),10,saving?L"Retry save":recordingActive?L"Pause recording":L"Resume recording");}CPoint p;GetCursorPos(&p);auto choice=menu.TrackPopupMenu(TPM_RETURNCMD|TPM_LEFTALIGN,p.x,p.y,this);if(choice==1)refreshDownloadAddress(this,manager,job);else if(choice==2)fileProperties(this,manager,job);else if(choice==3){Json inputs;{Lock lock(manager.mutex);inputs=addressInputs();}auto value=wide(downloadAddress(inputs));if(OpenClipboard()){EmptyClipboard();auto memory=GlobalAlloc(GMEM_MOVEABLE,(value.size()+1)*sizeof(wchar_t));if(memory){auto target=GlobalLock(memory);if(target){memcpy(target,value.c_str(),(value.size()+1)*sizeof(wchar_t));GlobalUnlock(memory);if(!SetClipboardData(CF_UNICODETEXT,memory))GlobalFree(memory);}else GlobalFree(memory);}CloseClipboard();}}else if(choice==4)openFile(this,job->target().parent_path());else if(choice==5){fs::path path;{Lock lock(manager.mutex);if(manager.isRelocating(job)||progressCompletedAction(job->data,manager.isActive(job))!=ProgressAction::Open)return;path=job->target();}if(confirmDownloadedFilePresent(this,path))openWith(this,path);}else if(choice==6)DestroyWindow();else if(choice==7){Json prefs;{Lock lock(manager.mutex);prefs=manager.state["Settings"];}progressPreferences(this,prefs);manager.setSettings(prefs);}else if(choice==8){Form details("Download information",620,340,this);details.init=[&]{details.edit(text(information),12,12,596,273,true,true);details.button("Close",513,302,95,[&]{details.close();});};details.DoModal();}else if(choice==9)completionAction();else if(choice==10){if(recordingActive)manager.pause(job);else manager.resume(job);update();}}
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
 void requestCompletionClose(bool preserveOpeningRate=false){
  closeRequested=true;closeAfter.reset();
  if(preserveOpeningRate&&IsWindowVisible()&&!IsIconic()&&rate->IsWindowVisible()){
   Lock lock(manager.mutex);
   const auto now=std::chrono::steady_clock::now();
   if(progressOpeningPrefetch(*job,now)){
    if(!job->prefetchPeakDisplayUntil)job->prefetchPeakDisplayUntil=now+std::chrono::seconds(2);
    closeAfter=job->prefetchPeakDisplayUntil;
   }
  }
  if(!closeAfter&&IsWindowEnabled())DestroyWindow();
 }
 std::string downloadId()const{return job->id();}
 Progress(Manager& m,JobPtr j,CWnd* owner):Form("Download progress",353,250,owner),manager(m),job(j){dialogUnits=true;modeless=true;cancel=[this]{dismiss();};init=[this]{
  ModifyStyle(0,WS_MINIMIZEBOX);Json prefs;fs::path initialTarget;i64 actualLimit;bool persistentLimit;
  {Lock lock(manager.mutex);prefs=manager.state["Settings"];initialTarget=job->target();const auto savedLimit=num(job->data,"LimitKbps");actualLimit=job->sessionLimit.value_or(savedLimit);persistentLimit=!job->sessionLimit.has_value()&&savedLimit>0;}
  expanded=str(prefs,"ProgressStartMode","Normal size")!="Small size";
  tabs=make<ThemeTabs>(WS_TABSTOP,0,0,353,106);tabs->InsertItem(0,L"Download status");tabPages.push_back(0);if(yes(prefs,"ProgressSpeedTab",true)){tabs->InsertItem((int)tabPages.size(),L"Speed Limiter");tabPages.push_back(1);}if(yes(prefs,"ProgressCompletionTab",true)){tabs->InsertItem((int)tabPages.size(),L"Options on completion");tabPages.push_back(2);}if(yes(prefs,"ProgressInformationTab",false)){tabs->InsertItem((int)tabPages.size(),L"Information");tabPages.push_back(3);}
  const int ox=4,oy=20;
  addressField=control(L"STATIC","",SS_LEFTNOWORDWRAP|SS_NOPREFIX,ox+7,oy+4,329,9);pages[0].push_back(addressField);
  const char* captions[]={"Status","File size","Downloaded","Transfer rate","Time left","Resume capability"};const int y[]={15,28,38,48,58,68},left[]={53,75,75,75,75,127},width[]={281,261,261,261,261,111};CWnd** values[]={&status,&size,&received,&rate,&remaining,&resume};
  for(int i=0;i<6;++i){pages[0].push_back(label(captions[i],ox+7,oy+y[i],i==0?40:i==2?60:i==5?108:63));*values[i]=control(L"STATIC","",SS_ENDELLIPSIS,ox+left[i],oy+y[i],width[i],9);pages[0].push_back(*values[i]);}
  bar=make<CProgressCtrl>(PBS_SMOOTH,7,111,338,10);bar->SetRange32(0,1000);if(uiDark){SetWindowTheme(bar->GetSafeHwnd(),L"",L"");bar->SetBkColor(RGB(48,51,56));bar->SetBarColor(RGB(35,139,63));}
  detailsButton=button("<< Hide details",15,126,93,[this]{expanded=!expanded;details();if(expanded)update();},15);pauseButton=button("Pause",203,126,56,[this]{primary();},15);cancelButton=button("Cancel",277,126,50,cancel,15);
  pieces=control(L"STATIC","Start positions and download progress by connections",SS_ENDELLIPSIS,7,144,242,9);
  graphToggle=button("Show speed graph",253,142,92,[this]{showRateGraph=!showRateGraph;detailView();update();},13);
  rateGraph=make<ProgressRateGraph>(0,7,158,338,89);
  rangeMap=make<RangeMap>(0,7,158,338,10);workers=make<ThemeList>(WS_TABSTOP|WS_BORDER|LVS_SHOWSELALWAYS|LVS_REPORT|LVS_SINGLESEL|LVS_NOSORTHEADER,7,173,338,74);workers->SetExtendedStyle(LVS_EX_FULLROWSELECT|LVS_EX_DOUBLEBUFFER);workers->theme();workers->InsertColumn(0,L"N.",LVCFMT_LEFT,pixelsX(20));workers->InsertColumn(1,L"Downloaded",LVCFMT_LEFT,pixelsX(68));workers->InsertColumn(2,L"Info",LVCFMT_LEFT,pixelsX(230));
  auto limitRateTitle=label("Transfer rate",ox+7,oy+7,63);limitRate=label("",ox+75,oy+7,202);
  limitEnabled=check("Use Speed Limiter",actualLimit>0,ox+7,oy+20,154);
  limitCaption=label("Maximum download speed:",ox+7,oy+32,170);
  limit=control(L"EDIT",std::to_string(actualLimit>0?actualLimit:1000),WS_TABSTOP|WS_BORDER|ES_NUMBER|ES_RIGHT|ES_AUTOHSCROLL,ox+7,oy+43,51,14);
  limit->SendMessage(EM_SETLIMITTEXT,7);limitHint=label("KBytes/sec",ox+64,oy+45,65);
  remember=check("Remember Speed Limiter settings for this file\r\non download stop/resume",persistentLimit,ox+7,oy+59,226,18);enableLimitFields(actualLimit>0);bind(limitEnabled,[this]{applyLimit();});bind(remember,[this]{applyLimit();});bindChange(limit,[this]{applyLimit();});pages[1]={limitRateTitle,limitRate,limitEnabled,limitCaption,limit,limitHint,remember};
  saveTo=control(L"STATIC","Save To: "+utf8(initialTarget.wstring()),SS_ENDELLIPSIS,ox+7,oy+3,316,9);
  auto show=check("Show download complete dialog",false,ox+7,oy+15,169);
  bind(show,[this,show]{
   Lock lock(manager.mutex);const bool global=yes(manager.state["Settings"],"SuppressCompletionDialog");auto before=job->data;
   if(str(before,"Status")!="Complete"&&!global&&!yes(before,"QueueOrigin")){
    const bool suppress=!checked(show);
    if(suppress!=yes(before,"SuppressCompletionDialog")){
     job->data["SuppressCompletionDialog"]=suppress;
     try{
      auto steps=suppress?std::vector<std::string>{}:completionSteps(before);
      if(!suppress&&std::any_of(steps.begin(),steps.end(),completionUsesPowerOptions)){
       steps.erase(std::remove_if(steps.begin(),steps.end(),completionUsesPowerOptions),steps.end());
       job->data["CompletionActions"]=steps;
       job->data["CompletionAction"]=steps.empty()?"None":steps.front();
       job->data["CompletionForce"]=false;
       if(steps.empty())job->data["CompletionActionArmed"]=false;
      }
      manager.save();
     }catch(...){job->data=before;syncCompletionControls(before,global);throw;}
    }
   }
   syncCompletionControls(job->data,global);
  });
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
  syncCompletionControls=[show,disconnect,exit,power,powerChoice,force,disconnectLabel,exitLabel,powerLabel,forceLabel](const Json& current,bool global){
   const bool editable=str(current,"Status")!="Complete";
   const bool showing=showDownloadCompletionDialog(current,global);
   const bool powerEnabled=editable&&!showing;
   const auto steps=showing?std::vector<std::string>{}:completionSteps(current);
   auto has=[&](const std::string& action){return std::find(steps.begin(),steps.end(),action)!=steps.end();};
   auto enable=[](CWnd* control,bool on){if((control->IsWindowEnabled()!=FALSE)!=on)control->EnableWindow(on);};
   auto check=[](CWnd* control,bool on){const int state=on?BST_CHECKED:BST_UNCHECKED;if(control->SendMessage(BM_GETCHECK)!=state)control->SendMessage(BM_SETCHECK,state);};
   check(show,showing);enable(show,editable&&!global&&!yes(current,"QueueOrigin"));
   for(auto control:{disconnect,exit,power,disconnectLabel,exitLabel,powerLabel})enable(control,powerEnabled);
   check(disconnect,has("Disconnect dial-up / VPN"));check(exit,has("Exit UDM"));
   auto selected=std::find_if(steps.begin(),steps.end(),completionPower);const bool hasPower=selected!=steps.end();
   check(power,hasPower);enable(powerChoice,powerEnabled&&hasPower);
   if(hasPower){const auto index=powerChoice->FindStringExact(-1,cs(*selected));if(powerChoice->GetCurSel()!=index)powerChoice->SetCurSel(index);}
   const bool canForce=hasPower&&completionCanForce(steps);
   check(force,canForce&&yes(current,"CompletionForce"));enable(force,powerEnabled&&canForce);enable(forceLabel,powerEnabled&&canForce);
  };
  auto savePower=[this,disconnect,exit,power,powerChoice,force]{
   Lock lock(manager.mutex);const auto& current=job->data;const bool global=yes(manager.state["Settings"],"SuppressCompletionDialog");
   if(str(current,"Status")=="Complete"||showDownloadCompletionDialog(current,global)){syncCompletionControls(current,global);return;}
   auto existing=completionSteps(current);std::vector<std::string> steps;
   if(std::find(existing.begin(),existing.end(),"Open downloaded file")!=existing.end())steps.push_back("Open downloaded file");
   if(checked(disconnect))steps.push_back("Disconnect dial-up / VPN");if(checked(exit))steps.push_back("Exit UDM");if(checked(power))steps.push_back(text(powerChoice));
   try{manager.setCompletionPlan(job,steps,checked(force)&&completionCanForce(steps),(int)num(current,"CompletionActionDelay",30),yes(current,"CompletionWaitForOthers",true));}catch(...){syncCompletionControls(current,global);throw;}syncCompletionControls(current,global);
  };
  for(auto control:{disconnect,exit,power})bind(control,savePower);bindChange(powerChoice,savePower);
  bind(force,[this,force,powerChoice,savePower]{
   if(!force->IsWindowEnabled())return;
   if(checked(force)){
    const auto choice=text(powerChoice);
    const bool confirmed=MessageBox(L"Force termination can discard unsaved work in other applications. Enable it for this download?",L"Force processes to terminate",MB_YESNO|MB_ICONWARNING|MB_DEFBUTTON2)==IDYES;
    if(!force->IsWindowEnabled()||text(powerChoice)!=choice)return;
    force->SendMessage(BM_SETCHECK,confirmed?BST_CHECKED:BST_UNCHECKED);
    if(!confirmed)return;
   }
   savePower();
  });
  for(auto pair:std::vector<std::pair<CWnd*,CWnd*>>{{disconnectLabel,disconnect},{exitLabel,exit},{powerLabel,power},{forceLabel,force}})bind(pair.first,[pair]{if(pair.second->IsWindowEnabled())pair.second->SendMessage(BM_CLICK);});
  pages[2]={saveTo,show,separator,powerTitle,disconnect,exit,power,powerChoice,force,disconnectLabel,exitLabel,powerLabel,forceLabel};
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
