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
#include "ProgressMenuUi.hpp"
struct ProgressCompletionEdit {
 enum Kind {Visibility,Power} kind=Visibility;
 bool showDialog=false,force=false;
 std::vector<std::string> powerSteps;
};
struct ProgressCompletionState {Json data=Json::object();bool globallySuppressed=false;};
inline Json progressCompletionData(const Json& record){
 Json data=Json::object();for(const auto* field:{"Status","QueueOrigin","SuppressCompletionDialog","CompletionActions","CompletionAction","CompletionForce","CloseProgressOnCompletion","OpenFolderOnCompletion","CompletionActionDelay","CompletionWaitForOthers"})if(record.contains(field))data[field]=record[field];return data;
}
inline ProgressCompletionState progressCompletionState(const Manager& manager,const Job& job){
 ProgressCompletionState state;state.data=progressCompletionData(job.data);
 state.globallySuppressed=yes(manager.state["Settings"],"SuppressCompletionDialog");return state;
}
#include "ProgressCompletionOptionsUi.hpp"
struct ProgressCommandRequest {
 bool dismiss=false;
 std::optional<ProgressAction> displayedAction;
 std::optional<std::pair<i64,bool>> limitChange;
 uint64_t limitGeneration=0;
 std::optional<ProgressCompletionEdit> completionEdit;
 std::optional<ProgressMenuAction> menuAction;
};
inline std::pair<i64,bool> progressLimitState(const Job& job){return {job.sessionLimit.value_or(num(job.data,"LimitKbps")),!job.sessionLimit.has_value()&&num(job.data,"LimitKbps")>0};}
struct ProgressCommandResult {
 enum Followup {Updated,Close,Open,Refresh,Login,CopyAddress,Folder,OpenWith} followup=Updated;
 fs::path target;
 std::string clipboard;
 std::optional<std::pair<i64,bool>> limit;
 std::optional<ProgressCompletionState> completion;
};
inline ProgressCommandResult applyProgressCommand(Manager& manager,const JobPtr& job,const ProgressCommandRequest& request){
 Lock lock(manager.mutex);ProgressCommandResult result;
 if(request.completionEdit){
  if(std::find(manager.jobs.begin(),manager.jobs.end(),job)==manager.jobs.end())throw std::runtime_error("Unknown download.");
  const auto& edit=*request.completionEdit;const bool global=yes(manager.state["Settings"],"SuppressCompletionDialog");const auto& current=job->data;
  if(edit.kind==ProgressCompletionEdit::Visibility){
   if(str(current,"Status")!="Complete"&&!global&&!yes(current,"QueueOrigin")){
    const bool suppress=!edit.showDialog;
    if(suppress!=yes(current,"SuppressCompletionDialog")){
     const auto before=current;
     job->data["SuppressCompletionDialog"]=suppress;
     try{auto steps=suppress?std::vector<std::string>{}:completionSteps(before);
      if(!suppress&&std::any_of(steps.begin(),steps.end(),completionUsesPowerOptions)){
       steps.erase(std::remove_if(steps.begin(),steps.end(),completionUsesPowerOptions),steps.end());
       job->data["CompletionActions"]=steps;job->data["CompletionAction"]=steps.empty()?"None":steps.front();job->data["CompletionForce"]=false;
       if(steps.empty())job->data["CompletionActionArmed"]=false;
      }
      manager.save();
     }catch(...){job->data=before;throw;}
    }
   }
  }else if(str(current,"Status")!="Complete"&&!showDownloadCompletionDialog(current,global)){
   auto existing=completionSteps(current);std::vector<std::string> steps;
   if(std::find(existing.begin(),existing.end(),"Open downloaded file")!=existing.end())steps.push_back("Open downloaded file");
   steps.insert(steps.end(),edit.powerSteps.begin(),edit.powerSteps.end());
   manager.setCompletionPlan(job,steps,edit.force&&completionCanForce(steps),static_cast<int>(num(current,"CompletionActionDelay",30)),yes(current,"CompletionWaitForOthers",true));
  }
  result.completion=progressCompletionState(manager,*job);return result;
 }
 if(request.limitChange){
  setProgressLimit(manager,job,request.limitChange->first,request.limitChange->second);
  result.limit=progressLimitState(*job);return result;
 }
 if(std::find(manager.jobs.begin(),manager.jobs.end(),job)==manager.jobs.end()){
  if(request.dismiss)result.followup=ProgressCommandResult::Close;
  return result;
 }
 const bool active=manager.isActive(job);
 if(request.menuAction){
  const auto action=*request.menuAction;
  if(action==ProgressMenuAction::CopyAddress){result.clipboard=downloadAddress(downloadAddressInputs(*job));result.followup=ProgressCommandResult::CopyAddress;}
  else if(action==ProgressMenuAction::OpenFolder){result.target=job->target().parent_path();result.followup=ProgressCommandResult::Folder;}
  else if(action==ProgressMenuAction::OpenWith){if(!manager.isRelocating(job)&&progressCompletedAction(job->data,active)==ProgressAction::Open){result.target=job->target();result.followup=ProgressCommandResult::OpenWith;}}
  else{
   const auto& record=job->data;const bool saving=liveSavePending(record);
   const bool enabled=saving?(!active&&str(record,"Status")!="Queued"):str(record,"Status")!="Pausing";
   if(yes(record,"LiveRecording")&&str(record,"Status")!="Complete"&&enabled){
    if(action==ProgressMenuAction::PauseRecording&&active)manager.pause(job);
    else if(action==ProgressMenuAction::ResumeRecording&&!active){manager.resume(job);result.limit=progressLimitState(*job);}
   }
  }
  return result;
 }
 if(request.dismiss){
  if(!manager.isRelocating(job)&&progressDismissStops(str(job->data,"Status"),active))manager.pause(job);
  result.followup=ProgressCommandResult::Close;return result;
 }
 const auto action=progressPrimaryAction(job->data,active);
 if(action!=request.displayedAction||manager.isRelocating(job)||!progressPrimaryEnabled(job->data,active))return result;
 if(action==ProgressAction::Pause||action==ProgressAction::StopScanWait)manager.pause(job);
 else if(action==ProgressAction::SaveLive)requestLiveHlsFinish(manager,job);
 else if(action==ProgressAction::Open){result.followup=ProgressCommandResult::Open;result.target=job->target();}
 else if(action==ProgressAction::Refresh)result.followup=ProgressCommandResult::Refresh;
 else if(action==ProgressAction::Resume){
  if(!active&&yes(job->data,"QueueOrigin")&&!yes(manager.state["Settings"],"SuppressAuthenticationDialog")&&authenticationDialogReady(job->data))
   result.followup=ProgressCommandResult::Login;
  else manager.resume(job);
  result.limit=std::pair{job->sessionLimit.value_or(num(job->data,"LimitKbps")),!job->sessionLimit.has_value()&&num(job->data,"LimitKbps")>0};
 }
 return result;
}
#include "ProgressGraphUi.hpp"
class Progress:public Form {
 DECLARE_MESSAGE_MAP()
 Manager& manager;JobPtr job;
 FileInfoDecisionTask commandTask;
 std::shared_ptr<ProgressCommandResult> commandResult;
 bool handlingCommand=false,contextMenuOpen=false;
 bool commandChangesLimit=false,commandControlsFrozen=false;
 uint64_t limitGeneration=0,activeLimitGeneration=0;
 std::pair<i64,bool> appliedLimit{0,false};
 std::optional<std::pair<std::pair<i64,bool>,uint64_t>> queuedLimit;
 std::optional<ProgressCompletionEdit> queuedCompletion;
 ProgressCompletionState displayedCompletion;
 ProgressMenuCache menuCache;ProgressMenuState displayedMenu;Json displayedProgressPreferences;
 friend struct ProgressMenuTestAccess;
 std::vector<std::pair<HWND,BOOL>> commandEnablement;
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
 afx_msg void OnDestroy(){commandTask.release();commandResult.reset();queuedLimit.reset();queuedCompletion.reset();removeTray();Form::OnDestroy();}
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
  if(manager.fileInfoDecisionPending||queuedLimit||queuedCompletion||commandTask.valid()||handlingCommand)return;
  const auto now=std::chrono::steady_clock::now();
  const bool showConnections=expanded&&!showRateGraph;
  if(closeRequested&&!contextMenuOpen&&IsWindowEnabled()&&(!closeAfter||now>=*closeAfter||!IsWindowVisible()||IsIconic())){
   DestroyWindow();return;
  }
  struct TrackProgress {i64 size;bool complete;Json segments;size_t firstRow,endRow;};
  Json j=Json::object(),address,quota,progressPrefs;std::vector<Worker> rows;std::vector<TrackProgress> tracks;
  std::vector<RangeMap::Span> adaptiveCoverage;
  i64 adaptiveMapSize=0;
  std::optional<ResponseSaveProgress> responseSave;SavedDataCheckProgress savedDataCheck;
  double speed;bool active,suppressCompletion,relocating,openingPrefetch=false,addressChanged=false;ProgressRequestState requestState;
  {
   // A refresh may be skipped; waiting here would stall painting and input for
   // every progress window while another thread publishes a catalog change.
   std::unique_lock lock(manager.mutex,std::try_to_lock);
   if(!lock)return;
   auto view=progressRecordView(job->data,showConnections);j=std::move(view.data);requestState=view.request;
   if(!downloadAddressInputsUnchanged(*job,displayedAddressInputs)){address=downloadAddressInputs(*job);addressChanged=true;}
   responseSave=job->responseSaveProgress;savedDataCheck=job->savedDataCheck;
   suppressCompletion=yes(manager.state["Settings"],"SuppressCompletionDialog");progressPrefs=progressPreferenceValues(manager.state["Settings"]);
   active=manager.isActive(job);relocating=manager.isRelocating(job);speed=job->speed;
   quota=manager.quotaStatus(job);
   displayedMenu=menuCache.capture(manager,job);
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
  const bool savedFormResponse=requestState.saved;
  projectHttpOutput(j);
  syncCompletionControls(j,suppressCompletion);displayedProgressPreferences=std::move(progressPrefs);
  if(addressChanged)syncAddress(std::move(address));
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
  const auto resumeCapability=progressResumeCapability(j,&requestState);
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
    else if(savedFormResponse)action="Resume";
    else if(progressNeedsFormRestart(j,&requestState))action="Restart";
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
  const bool checkingSavedData=active&&savedDataCheck.total>0;
  auto connectionStatus=str(j,"ConnectionStatus",state);
  if(checkingSavedData&&state!="Pausing"){
   const auto percent=(int)(100.0*savedDataCheck.checked/savedDataCheck.total);
   connectionStatus="Checking saved download data ("+std::to_string(percent)+"%)";
  }
  auto detail=capturedSave&&active?std::string{}:str(j,"Error");if(detail.empty()&&progressNeedsFormRestart(j,&requestState))detail="Restart resubmits this form.";auto message=(capturedSave?captureStatus:yes(quota,"Waiting")?quotaWaitText(quota):scanning?std::string("Scanning file"):connectionStatus)+(detail.empty()?"":" - "+detail);
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
 void syncLimitControls(i64 kb,bool persistent){
  appliedLimit={kb,persistent};
  syncingLimit=true;limitEnabled->SendMessage(BM_SETCHECK,kb>0?BST_CHECKED:BST_UNCHECKED);remember->SendMessage(BM_SETCHECK,persistent?BST_CHECKED:BST_UNCHECKED);
  limit->SetWindowText(cs(std::to_string(kb>0?kb:1000)));enableLimitFields(kb>0);limitHint->SetWindowText(L"KBytes/sec");syncingLimit=false;
 }
 void syncLimitControls(){
  i64 kb;bool persistent;{Lock lock(manager.mutex);kb=job->sessionLimit.value_or(num(job->data,"LimitKbps"));persistent=!job->sessionLimit.has_value()&&num(job->data,"LimitKbps")>0;}
  syncLimitControls(kb,persistent);
 }
 void applyLimit(){
  if(syncingLimit||queuedCompletion||(commandTask.valid()&&!commandChangesLimit)||handlingCommand)return;
  ++limitGeneration;
  const bool enabled=checked(limitEnabled);enableLimitFields(enabled);i64 kb=0;
  if(enabled){const auto value=trim(text(limit));if(!std::regex_match(value,std::regex("[0-9]{1,7}"))||std::stoll(value)<1||std::stoll(value)>1000000){limitHint->SetWindowText(L"1..1000000");return;}kb=std::stoll(value);}
  queuedLimit=std::pair{std::pair{kb,checked(remember)},limitGeneration};
  limitHint->SetWindowText(L"KBytes/sec");freezeCommandControls(true);launchQueuedLimit();
 }
 afx_msg void OnContextMenu(CWnd*,CPoint point){try{actions(point);}catch(const std::exception& e){error(this,e);}}
 void completionAction(){
  if(commandTask.valid()||queuedLimit||queuedCompletion||handlingCommand||manager.fileInfoDecisionPending)return;
  progressCompletionOptions(this,manager,job,displayedCompletion.data);
 }
 void dismiss(){
  if(commandTask.valid()||queuedLimit||queuedCompletion||handlingCommand)return;
  // A displayed Close is presentation-only, even if the job changed since its
  // last refresh. It must never turn into a request to stop the new transfer.
  if(!displayedDismissStops){DestroyWindow();return;}
  startCommand({true,{}});
 }
 void primary(){
  if(commandTask.valid()||queuedLimit||queuedCompletion||handlingCommand||!displayedPrimaryAction)return;
  startCommand({false,displayedPrimaryAction});
 }
 void freezeCommandControls(bool editingLimit){
  if(commandControlsFrozen)return;
  commandEnablement.clear();
  struct Enumeration{Progress* self;bool editingLimit;}enumeration{this,editingLimit};
  EnumChildWindows(GetSafeHwnd(),[](HWND child,LPARAM data)->BOOL{
   auto& enumeration=*reinterpret_cast<Enumeration*>(data);auto self=enumeration.self;wchar_t kind[40]{};GetClassNameW(child,kind,40);
   if(child!=self->detailsButton->GetSafeHwnd()&&child!=self->graphToggle->GetSafeHwnd()&&
      (!enumeration.editingLimit||(child!=self->limit->GetSafeHwnd()&&child!=self->limitEnabled->GetSafeHwnd()&&child!=self->remember->GetSafeHwnd()))&&
      (!_wcsicmp(kind,L"BUTTON")||!_wcsicmp(kind,L"EDIT")||!_wcsicmp(kind,L"COMBOBOX")))
    self->commandEnablement.push_back({child,::IsWindowEnabled(child)});
   return TRUE;
  },reinterpret_cast<LPARAM>(&enumeration));
  for(const auto& item:commandEnablement)::EnableWindow(item.first,FALSE);
  commandControlsFrozen=true;
 }
 void restoreCommandControls(){
  for(const auto& item:commandEnablement)if(IsWindow(item.first))::EnableWindow(item.first,item.second);
  commandEnablement.clear();commandControlsFrozen=false;
 }
 void launchQueuedLimit(){
  if(!queuedLimit||commandTask.valid()||handlingCommand||manager.fileInfoDecisionPending)return;
  const auto next=*queuedLimit;ProgressCommandRequest request;request.limitChange=next.first;request.limitGeneration=next.second;
  try{startCommand(request);queuedLimit.reset();}
  catch(...){queuedLimit.reset();restoreCommandControls();if(limitGeneration==next.second)syncLimitControls(appliedLimit.first,appliedLimit.second);throw;}
 }
 void queueCompletionEdit(ProgressCompletionEdit edit){
  if(commandTask.valid()||queuedLimit||queuedCompletion||handlingCommand)return;
  queuedCompletion=std::move(edit);freezeCommandControls(false);launchQueuedCompletion();
 }
 void launchQueuedCompletion(){
  if(!queuedCompletion||commandTask.valid()||handlingCommand||manager.fileInfoDecisionPending)return;
  ProgressCommandRequest request;request.completionEdit=*queuedCompletion;
  try{startCommand(std::move(request));queuedCompletion.reset();}
  catch(...){queuedCompletion.reset();restoreCommandControls();syncCompletionControls(displayedCompletion.data,displayedCompletion.globallySuppressed);throw;}
 }
 void startCommand(ProgressCommandRequest request){
  auto result=std::make_shared<ProgressCommandResult>();
  const bool alreadyFrozen=commandControlsFrozen;freezeCommandControls(request.limitChange.has_value());
  try{commandTask.startAction(manager,[source=&manager,target=job,request,result]{
   try{*result=applyProgressCommand(*source,target,request);}
   catch(...){const auto failure=std::current_exception();try{Lock lock(source->mutex);if(request.limitChange)result->limit=progressLimitState(*target);if(request.completionEdit)result->completion=progressCompletionState(*source,*target);}catch(...){}std::rethrow_exception(failure);}
   return FileInfoDecisionResult{};
  },taskNotifier());}catch(...){if(!alreadyFrozen)restoreCommandControls();throw;}
  commandResult=std::move(result);commandChangesLimit=request.limitChange.has_value();activeLimitGeneration=request.limitGeneration;
 }
 void finishCommand(){
  if(handlingCommand||!commandTask.valid()||!commandTask.ready())return;
  handlingCommand=true;std::optional<ProgressCommandResult> result;
  const bool changedLimit=commandChangesLimit;const auto generation=activeLimitGeneration;const auto ownedResult=commandResult;bool failed=false;
  try{commandTask.get();result=*commandResult;}
  catch(const std::exception& failure){failed=true;if(changedLimit&&ownedResult->limit)appliedLimit=*ownedResult->limit;error(this,failure);}
  catch(...){failed=true;if(changedLimit&&ownedResult->limit)appliedLimit=*ownedResult->limit;error(this,std::runtime_error("The download action could not finish. You can try again."));}
  commandTask.release();commandResult.reset();
  if(!queuedLimit&&!queuedCompletion)restoreCommandControls();
  if(changedLimit){
   if(result&&result->limit)appliedLimit=*result->limit;
   if(failed&&!queuedLimit&&generation==limitGeneration)syncLimitControls(appliedLimit.first,appliedLimit.second);
  }
  try{
   if(ownedResult->completion)syncCompletionControls(ownedResult->completion->data,ownedResult->completion->globallySuppressed);
   if(result){
    if(result->followup==ProgressCommandResult::Close){handlingCommand=false;DestroyWindow();return;}
    if(result->limit&&!changedLimit)syncLimitControls(result->limit->first,result->limit->second);
    if(result->followup==ProgressCommandResult::CopyAddress){
     const auto value=wide(result->clipboard);
     if(OpenClipboard()){EmptyClipboard();auto memory=GlobalAlloc(GMEM_MOVEABLE,(value.size()+1)*sizeof(wchar_t));if(memory){auto target=GlobalLock(memory);if(target){memcpy(target,value.c_str(),(value.size()+1)*sizeof(wchar_t));GlobalUnlock(memory);if(!SetClipboardData(CF_UNICODETEXT,memory))GlobalFree(memory);}else GlobalFree(memory);}CloseClipboard();}
    }
    else if(result->followup==ProgressCommandResult::Folder)openFile(this,result->target);
    else if(result->followup==ProgressCommandResult::OpenWith){if(confirmDownloadedFilePresent(this,result->target))openWith(this,result->target);}
    else if(result->followup==ProgressCommandResult::Open)openDownloadedFile(this,result->target);
    else if(result->followup==ProgressCommandResult::Refresh)refreshDownloadAddress(this,manager,job);
    else if(result->followup==ProgressCommandResult::Login){requestQueuedDownloadLogin(this,manager,job);syncLimitControls();}
   }
  }catch(const std::exception& failure){error(this,failure);}
  handlingCommand=false;
  if(auto frame=AfxGetMainWnd();frame&&frame->GetSafeHwnd())frame->PostMessage(WM_TIMER,1,0);
 }
 void startMenuCommand(ProgressMenuAction action){
  if(commandTask.valid()||queuedLimit||queuedCompletion||handlingCommand)return;
  ProgressCommandRequest request;request.menuAction=action;startCommand(std::move(request));
 }
 bool menuOwnWorkPending()const{return commandTask.valid()||queuedLimit||queuedCompletion||handlingCommand;}
 void dispatchMenuChoice(UINT choice,const ProgressMenuState& shown){
  const bool ownPending=menuOwnWorkPending();
  if(!progressMenuChoiceAllowed(choice,ownPending||manager.fileInfoDecisionPending,ownPending))return;
  if(choice==1)refreshDownloadAddress(this,manager,job);
  else if(choice==2)fileProperties(this,manager,job);
  else if(choice==3)startMenuCommand(ProgressMenuAction::CopyAddress);
  else if(choice==4)startMenuCommand(ProgressMenuAction::OpenFolder);
  else if(choice==5)startMenuCommand(ProgressMenuAction::OpenWith);
  else if(choice==6)DestroyWindow();
  else if(choice==7){auto prefs=displayedProgressPreferences;const auto before=prefs;progressPreferences(this,prefs,&manager);for(const auto* key:progressPreferenceKeys())if(prefs.at(key)!=before.at(key))displayedProgressPreferences[key]=prefs.at(key);}
  else if(choice==8){Form details("Download information",620,340,this);details.init=[&]{details.edit(text(information),12,12,596,273,true,true);details.button("Close",513,302,95,[&]{details.close();});};details.DoModal();}
  else if(choice==9)completionAction();
  else if(choice==10)startMenuCommand(shown.recordingActive?ProgressMenuAction::PauseRecording:ProgressMenuAction::ResumeRecording);
 }
 void actions(CPoint point){
  const bool ownPending=menuOwnWorkPending();if(ownPending||contextMenuOpen)return;
  const auto shown=displayedMenu;CMenu menu;menu.CreatePopupMenu();populateProgressMenu(menu,shown,ownPending||manager.fileInfoDecisionPending,ownPending);
  if(point.x==-1&&point.y==-1){CRect bounds;GetClientRect(&bounds);point=bounds.CenterPoint();ClientToScreen(&point);}
  UINT choice=0;
  {struct PopupLifetime {bool& open;explicit PopupLifetime(bool& value):open(value){open=true;}~PopupLifetime(){open=false;}}lifetime(contextMenuOpen);
   choice=menu.TrackPopupMenu(TPM_RETURNCMD|TPM_LEFTALIGN,point.x,point.y,this);}
  dispatchMenuChoice(choice,shown);
 }
 afx_msg void OnTimer(UINT_PTR id){if(GetSafeHwnd())Form::OnTimer(id);}
 BOOL OnNotify(WPARAM w,LPARAM l,LRESULT* result)override{auto hdr=(NMHDR*)l;if(hdr->hwndFrom==tabs->m_hWnd&&hdr->code==TCN_SELCHANGE){page();*result=0;return TRUE;}return Form::OnNotify(w,l,result);}
public:
 ~Progress()override{removeTray();}
 // App shutdown drains accepted backend work before Manager::stop takes its
 // final snapshot. UI followups (Open/Login/Refresh) are superseded by exit.
 void finishPendingCommandForShutdown(){
  if(!commandTask.valid())return;
  std::exception_ptr failure;const bool changedLimit=commandChangesLimit;const auto generation=activeLimitGeneration;
  commandTask.release();
  // A nested error modal may already have consumed the future. Joining is
  // still required, but that acknowledged exception must not be read twice.
  if(commandTask.ready())try{commandTask.get();}catch(...){failure=std::current_exception();}
  if(changedLimit&&commandResult&&commandResult->limit)appliedLimit=*commandResult->limit;
  const auto completion=commandResult?commandResult->completion:std::nullopt;
  commandResult.reset();if(!queuedLimit&&!queuedCompletion)restoreCommandControls();
  if(completion)syncCompletionControls(completion->data,completion->globallySuppressed);
  if(failure&&changedLimit&&!queuedLimit&&generation==limitGeneration)syncLimitControls(appliedLimit.first,appliedLimit.second);
  if(failure)std::rethrow_exception(failure);
 }
 void flushQueuedLimitsForShutdown(){
  while(queuedLimit||queuedCompletion){launchQueuedLimit();launchQueuedCompletion();if(!commandTask.valid())throw std::runtime_error("Another download decision is still saving. Try closing again shortly.");finishPendingCommandForShutdown();}
 }
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
  if(commandTask.valid()||queuedLimit||queuedCompletion||handlingCommand)return;
  if(preserveOpeningRate&&IsWindowVisible()&&!IsIconic()&&rate->IsWindowVisible()){
   Lock lock(manager.mutex);
   const auto now=std::chrono::steady_clock::now();
   if(progressOpeningPrefetch(*job,now)){
    if(!job->prefetchPeakDisplayUntil)job->prefetchPeakDisplayUntil=now+std::chrono::seconds(2);
    closeAfter=job->prefetchPeakDisplayUntil;
   }
  }
  if(!closeAfter&&IsWindowEnabled()&&!contextMenuOpen)DestroyWindow();
 }
 std::string downloadId()const{return job->id();}
 Progress(Manager& m,JobPtr j,CWnd* owner):Form("Download progress",353,250,owner),manager(m),job(j){dialogUnits=true;modeless=true;cancel=[this]{dismiss();};init=[this]{
  ModifyStyle(0,WS_MINIMIZEBOX);Json prefs;fs::path initialTarget;i64 actualLimit;bool persistentLimit;
  {Lock lock(manager.mutex);prefs=manager.state["Settings"];displayedProgressPreferences=progressPreferenceValues(prefs);initialTarget=job->target();displayedMenu=menuCache.capture(manager,job);const auto savedLimit=num(job->data,"LimitKbps");actualLimit=job->sessionLimit.value_or(savedLimit);persistentLimit=!job->sessionLimit.has_value()&&savedLimit>0;appliedLimit={actualLimit,persistentLimit};}
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
   ProgressCompletionEdit edit;edit.showDialog=checked(show);queueCompletionEdit(std::move(edit));
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
  syncCompletionControls=[this,show,disconnect,exit,power,powerChoice,force,disconnectLabel,exitLabel,powerLabel,forceLabel](const Json& current,bool global){
   displayedCompletion={progressCompletionData(current),global};
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
   if(commandTask.valid()||queuedLimit||queuedCompletion||handlingCommand)return;
   ProgressCompletionEdit edit;edit.kind=ProgressCompletionEdit::Power;
   if(checked(disconnect))edit.powerSteps.push_back("Disconnect dial-up / VPN");
   if(checked(exit))edit.powerSteps.push_back("Exit UDM");
   if(checked(power))edit.powerSteps.push_back(text(powerChoice));
   edit.force=checked(force)&&completionCanForce(edit.powerSteps);queueCompletionEdit(std::move(edit));
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
  accept=[this]{primary();};pulse=[this]{finishCommand();if(!GetSafeHwnd())return;try{launchQueuedLimit();launchQueuedCompletion();}catch(const std::exception& failure){error(this,failure);}try{update();}catch(const std::exception& failure){status->SetWindowText(cs(failure.what()));}};pulsePeriod=500;page();details();update();
 };}

};
BEGIN_MESSAGE_MAP(Progress,Form)
 ON_WM_TIMER()
 ON_WM_DESTROY()
 ON_MESSAGE(WM_APP+144,OnProgressTray)
 ON_WM_CONTEXTMENU()
END_MESSAGE_MAP()
