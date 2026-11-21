// Included inside udm after Form, openRefreshPage and DownloadRefreshMode.
inline bool refreshDownloadAddress(CWnd* owner,Manager& manager,JobPtr job,bool reopenPage=true,DownloadRefreshMode mode=DownloadRefreshMode::AllowResume,Json* acceptedSelection=nullptr){
 FileInfoDialogOwner lifetime;
 DownloadRefreshTask task(manager,job,!reopenPage,mode==DownloadRefreshMode::FileInfo);
 Form dialog("Refresh download address",610,150,owner);
 DownloadRefreshViewPtr shown;std::optional<DownloadRefreshCommand> pending;Json savedSelection=Json::object();
 bool built=false,transition=false,closing=false,closeFailed=false,saved=false,mutating=false,failed=false,pauseRead=false;
 CWnd *status=nullptr,*address=nullptr,*page=nullptr,*save=nullptr,*resume=nullptr,*open=nullptr,*cancel=nullptr,*retry=nullptr;
 std::vector<CWnd*> inputs;std::string launchError;std::exception_ptr constructionFailure;
 const bool allowResume=mode==DownloadRefreshMode::AllowResume;
 std::function<void()> requestClose,requestPage,retryRead,dispatch;
 std::function<void(bool)> requestApply;
 std::function<void(DownloadRefreshViewPtr,bool)> display;
 const auto controls=[&]{
  const bool editable=built&&!mutating&&!closing&&!failed&&!saved;
  for(auto control:inputs)if(control)control->EnableWindow(editable);
  if(save){const auto kind=shown?str(shown->offer(),"kind"):std::string{};const bool ready=shown&&(!shown->media||kind=="sabr"||kind=="adaptive"||kind=="direct-media");save->EnableWindow(editable&&ready);if(resume)resume->EnableWindow(editable&&ready);}
  if(cancel){cancel->SetWindowText(closing?(closeFailed?L"Retry close":L"Closing..."):saved?L"Close":L"Cancel");cancel->EnableWindow(!closing||closeFailed);}
  if(retry){retry->ShowWindow(pauseRead&&!closing&&!saved?SW_SHOW:SW_HIDE);retry->EnableWindow(!task.valid()&&!closing);}
 };
 const auto send=[&](DownloadRefreshCommand command){
  if(!task.start(std::move(command),dialog.taskNotifier()
#ifdef UDM_TOOLBAR_COMPONENT_TEST
   ,downloadRefreshTestObserver
#endif
  ))throw std::runtime_error("Recovery already has an operation in progress.");
 };
 dispatch=[&]{
  if(task.valid()||transition)return;
  if(pending){auto command=std::move(*pending);pending.reset();send(std::move(command));return;}
  if(closing&&!closeFailed){DownloadRefreshCommand command;command.action=DownloadRefreshAction::Close;command.shown=shown;send(std::move(command));}
 };
 requestClose=[&]{
  if(closing&&!closeFailed)return;
  closing=true;closeFailed=false;mutating=true;
  // A queued Save remains accepted. Advisory page/read work may be discarded.
  if(pending&&pending->action!=DownloadRefreshAction::Apply)pending.reset();
  task.cancelRead();if(status)status->SetWindowText(L"Finishing recovery and displayed browser receipts...");controls();dispatch();
 };
 retryRead=[&]{
  if(task.valid()||closing||saved)return;failed=pauseRead=false;launchError.clear();DownloadRefreshCommand command;command.action=built?DownloadRefreshAction::Read:DownloadRefreshAction::Begin;pending=std::move(command);controls();dispatch();
 };
 requestPage=[&]{
  if(!shown||closing||mutating||saved)return;
  if(!shown->media){try{openRefreshPage(dialog.GetSafeHwnd(),text(page));launchError.clear();}catch(const std::exception& failure){launchError=failure.what();status->SetWindowText(cs(launchError));}return;}
  DownloadRefreshCommand command;command.action=DownloadRefreshAction::Page;pending=std::move(command);task.cancelRead();dispatch();
 };
 requestApply=[&](bool start){
  if(!built||!shown||mutating||closing||saved||failed||(start&&!allowResume))return;
  DownloadRefreshCommand command;command.action=DownloadRefreshAction::Apply;command.shown=shown;command.resume=start;command.address=address?text(address):std::string{};command.page=page?text(page):std::string{};
  pending=std::move(command);mutating=true;pauseRead=false;task.cancelRead();status->SetWindowText(L"Saving the reviewed download address...");controls();dispatch();
 };
 display=[&](DownloadRefreshViewPtr view,bool stale){
  if(!view)return;const auto previous=shown;shown=std::move(view);failed=false;
  if(shown->media){
   const auto kind=str(shown->offer(),"kind");const bool ready=kind=="sabr"||kind=="adaptive"||kind=="direct-media";
   status->SetWindowText(cs(stale?"The captured session changed or expired. Review the selection and try again.":ready?(allowResume?"Matching streams received. Review the selection above, then save or resume.":"Matching streams received. Save the session to return to Download File Info."):launchError.empty()?"Waiting for the same video, quality and audio track from the browser (10 minutes).":launchError));
  }else{
   if(shown->offer().is_object()&&(!previous||shown->offerRevision!=previous->offerRevision||shown->presentations!=previous->presentations)){
    address->SetWindowText(cs(str(shown->offer(),"url")));if(!str(shown->offer(),"page").empty())page->SetWindowText(cs(str(shown->offer(),"page")));
    status->SetWindowText(allowResume?L"A matching browser link arrived. Review the address, then choose Save or Resume.":L"A matching browser link arrived. Save it to return to Download File Info.");
   }
   if(stale)status->SetWindowText(allowResume?L"The browser link changed or expired. Review the address, then choose Save or Resume again.":L"The browser link changed or expired. Review the address, then choose Save again.");
   else if(!shown->offer().is_object()&&shown->newReview){if(trim(text(address)).empty()){requestClose();return;}status->SetWindowText(L"A different link arrived. Close this dialog to review it, or continue with the address you entered.");}
  }
  controls();
 };
 const auto build=[&](DownloadRefreshViewPtr view){
  dialog.clearOwnedControls();inputs.clear();status=address=page=save=resume=open=cancel=retry=nullptr;shown.reset();const bool media=view->media;dialog.SetWindowText(media?L"Refresh media session":L"Refresh download address");dialog.resizeClient(media?574:610,media?290:302);
  dialog.label(str(view->record,"FileName"),14,12,media?546:580,24);
  if(media){
   const bool fixedVideo=view->sabr&&str(view->record,"MediaOutput")!="audio";
   dialog.label(fixedVideo?"1. Open the original video and let it play.\r\n2. Click Download this video in the UDM panel to request the saved quality.\r\n3. Return here to save the matching session.":"1. Open the original video and let it play.\r\n2. In its UDM panel, select the same quality or audio track.\r\n3. Return here to apply the matching session to this download.",14,43,546,55);
   dialog.label("Original page",14,109,97);page=dialog.edit(view->page,114,105,446,23,true);open=dialog.button("Open video page",14,143,142,requestPage);
   const auto retained=num(view->record,"SabrRetainedBytes");dialog.label(str(view->record,"FormatDescription")+(view->sabr?"\r\nRetained data: "+bytes(retained)+(retained>0&&num(view->record,"StreamCompletedMs")<=0?". No playable progress yet.":". Filename and history are preserved."):"\r\nSaved segments are checked against the fresh capture before reuse."),170,141,390,38);
   status=dialog.label("",14,187,546,43);save=dialog.button("Save session",allowResume?171:321,246,117,[&]{requestApply(false);});resume=allowResume?dialog.button("Save and resume",298,246,140,[&]{requestApply(true);}):nullptr;cancel=dialog.button("Cancel",448,246,112,requestClose);retry=dialog.button("Retry details",14,246,140,retryRead);
  }else{
   dialog.label(view->ftp?"Paste a replacement FTP address for the same file. UDM checks the saved bytes before resuming.":"Open the download page, then send the same file to UDM from the browser. You can also paste a fresh direct link below.",14,42,580,34);
   dialog.label("Download page",14,91,96);page=dialog.edit(view->page,116,87,359);open=dialog.button("Open page",485,86,109,requestPage);
   dialog.label("New address",14,126,96);address=dialog.edit("",116,122,478,47,false,true);status=dialog.label(view->ftp?"Enter the replacement address to continue.":"Waiting for a matching link from the browser (up to 10 minutes).",14,181,580,30);
   dialog.label("Saved parts: "+bytes(num(view->record,"Received"))+". Resume checks the file size and server validator before reusing them.",14,215,580,29);
   save=dialog.button("Save address",allowResume?219:364,262,116,[&]{requestApply(false);});resume=allowResume?dialog.button("Save and resume",345,262,135,[&]{requestApply(true);}):nullptr;cancel=dialog.button("Cancel",490,262,104,requestClose);retry=dialog.button("Retry details",14,262,140,retryRead);
  }
  inputs={page,address,open,save,resume};built=true;display(std::move(view),false);dialog.refreshTheme();
  if(reopenPage&&shown&&!shown->ftp&&!closing)requestPage();
 };
 dialog.cancel=requestClose;dialog.accept=[&]{if(saved||closing)requestClose();else requestApply(allowResume);};
 dialog.pulse=[&]{
  if(transition)return;
  if(task.valid()&&task.ready()){
   auto result=task.take();
   if(result.action==DownloadRefreshAction::Close){
    if(result.closed){dialog.close(saved?IDOK:IDCANCEL);return;}closeFailed=pauseRead=true;status->SetWindowText(cs(result.error+" Retry close to finish recovery."));controls();return;
   }
   if(result.applied){savedSelection=std::move(result.appliedSelection);saved=true;mutating=false;if(!result.error.empty()){closing=false;pauseRead=true;status->SetWindowText(cs(result.error+" The address was saved."));controls();return;}closing=true;mutating=true;status->SetWindowText(L"Finishing recovery...");controls();}
   else if(!closing&&!(pending&&result.action!=DownloadRefreshAction::Apply)){
    if(!result.error.empty()){mutating=false;pauseRead=true;failed=result.action!=DownloadRefreshAction::Apply;status->SetWindowText(cs(result.error));controls();}
    else if(result.view){mutating=false;pauseRead=false;
     if(!built){transition=true;dialog.afterCurrentPulse([&,view=std::move(result.view)]{transition=false;try{if(!closing)build(view);dispatch();}catch(...){constructionFailure=std::current_exception();task.finish();dialog.close(IDCANCEL);}});return;}
     display(std::move(result.view),result.stale);
    }else if(result.action==DownloadRefreshAction::Page&&!result.cancelled){try{openRefreshPage(dialog.GetSafeHwnd(),result.page);launchError.clear();}catch(const std::exception& failure){launchError=failure.what();status->SetWindowText(cs(launchError));}}
   }
  }
  dispatch();
  if(!task.valid()&&!pending&&!closing&&built&&!saved&&!pauseRead&&!dialog.taskPulseActive()){DownloadRefreshCommand command;command.action=DownloadRefreshAction::Read;send(std::move(command));}
 };
 dialog.init=[&]{dialog.label("Preparing download recovery...",14,12,580,24);status=dialog.label("Reading the saved download and preparing recovery.",14,43,580,36);cancel=dialog.button("Cancel",490,108,104,requestClose);retry=dialog.button("Retry",374,108,104,retryRead);retry->ShowWindow(SW_HIDE);DownloadRefreshCommand command;command.action=DownloadRefreshAction::Begin;send(std::move(command));};
 dialog.afterInit=[&]{showBrowserOwnedDialog(dialog,owner);};
 const auto result=dialog.DoModal();task.finish();if(constructionFailure)std::rethrow_exception(constructionFailure);if(result==-1)throw std::runtime_error("Cannot open the download refresh dialog.");if(result==IDOK&&acceptedSelection)*acceptedSelection=std::move(savedSelection);return result==IDOK;
}
