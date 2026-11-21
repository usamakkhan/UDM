// Included inside udm after Form and property helpers.
#ifdef UDM_TOOLBAR_COMPONENT_TEST
inline std::function<void(Form&,const FileInfoRefreshResult&)> fileInfoRefreshTestApplied;
inline std::function<void(Form&,const FileInfoCategoryResult&)> fileInfoCategoryTestApplied;
inline std::function<void(Form&,const FileInfoNameResult&)> fileInfoNameTestApplied;
#endif
inline void populateDownloadCategories(CComboBox& control,const std::vector<std::string>& keys,size_t selected){
 const bool visible=(control.GetStyle()&WS_VISIBLE)!=0;
 if(visible)control.SetRedraw(FALSE);
 struct RestoreRedraw {
  CComboBox& control;bool visible;
  ~RestoreRedraw(){if(visible){control.SetRedraw(TRUE);control.Invalidate();}}
 } restore{control,visible};
 const auto locale=control.GetLocale();
 std::vector<CString> labels;labels.reserve(keys.size());
 if(control.GetCount()>0)control.ResetContent();
 for(const auto& key:keys){
  auto label=cs(categoryDisplayName(key));
  const auto duplicate=[&]{
   for(const auto& existing:labels){
    const auto equal=categoryLabelEqual(locale,{existing.GetString(),static_cast<size_t>(existing.GetLength())},{label.GetString(),static_cast<size_t>(label.GetLength())});
    if(!equal)return control.FindStringExact(-1,label)!=CB_ERR;
    if(*equal)return true;
   }
   return false;
  };
  while(duplicate())label+=L" (custom)";
  if(control.AddString(label)<0)throw std::runtime_error("Cannot add a category to the selector.");
  labels.push_back(std::move(label));
 }
 control.SetCurSel(keys.empty()?CB_ERR:static_cast<int>(selected<keys.size()?selected:0));
}
inline bool downloadInfo(CWnd* parent,Manager& m,JobPtr job,bool properties=false,bool cancelNewOffer=false,std::shared_ptr<const FileInfoInitialView> preparedView={},std::shared_ptr<const PendingFileInfoInbox::Operation> pending={},bool* pendingBound=nullptr,Json* pendingPresentation=nullptr){
#ifdef UDM_CAPTURE_PRESENTATION_COMPONENT_TEST
 traceCapturePresentation("file-info-entry");
#endif
#ifdef UDM_PRODUCTION_UI_TIMING_TEST
 nativeUiTraceRecord("file-info-entry");
#endif
 if(properties){fileProperties(parent,m,job);return true;}
 FileInfoDialogOwner displayOwner;
 if(!job&&(!pending||!preparedView||!preparedView->media||preparedView->formRequest))throw std::runtime_error("This prepared download cannot be displayed yet.");
 std::shared_ptr<const FileInfoInitialView> initialView;
 Json data=Json::object(),prefs=Json::object(),addressInputs=Json::object();fs::path initialTarget;
 i64 initialSavedSize=-1;bool media=false,directResponse=false,formRequest=false,trimResponse=false,initialReady=false;
 int nameRow=20,footerOffset=32;std::string extension,initialAddress;
 const auto setInitialView=[&](std::shared_ptr<const FileInfoInitialView> view,std::optional<std::string> address=std::nullopt){
  initialView=std::move(view);data=initialView->data;prefs=initialView->preferences;addressInputs=initialView->addressInputs;initialTarget=initialView->target;
  initialSavedSize=initialView->savedSize;media=initialView->media;directResponse=initialView->directResponse;formRequest=initialView->formRequest;trimResponse=initialView->trimResponse;
  projectHttpOutput(data);nameRow=media?0:20;footerOffset=nameRow+12;extension=lower(utf8(fs::path(wide(str(data,"FileName"))).extension().wstring()));
  initialAddress=address?std::move(*address):downloadAddress(addressInputs);initialReady=true;
 };
 if(!job)setInitialView(preparedView);
 const auto hasDirectResponseHeaders=[&directResponse](const Json& record){
  return directResponse&&hasNonemptyString(record,"ResponseFileNameUrl")&&str(record,"ResponseFileNameUrl")==str(record,"Url");
 };
 const auto replacementNeedsResponse=[&directResponse](const Json& record){
  return directResponse&&yes(record,"RefreshPendingValidation")&&Url(str(record,"Url")).scheme!="ftp";
 };
 const auto trackBytes=[&media](const Json& record){
  if(!media||str(record,"Status")=="Complete")return i64{0};
  i64 total=0;
  for(const auto* name:{"Video","Audio"}){
   const auto track=record.find(name);
   if(track==record.end()||!track->is_object())continue;
   const auto length=num(*track,"Size",-1);
   if(length<0||length>std::numeric_limits<i64>::max()-total)return reportedMediaSize(record);
   total+=length;
  }
  return total>0?total:reportedMediaSize(record);
 };
 constexpr UINT_PTR initialPrefetchTimer=8;
 const auto addressRejected=[](const Json& record){
  const auto status=num(record,"LastHttpStatus");
  return !yes(record,"BrowserVerificationRequired")&&(status==403||status==404||status==410);
 };
 const auto retryLabel=[addressRejected](const Json& record)->std::string{
  if(yes(record,"BrowserVerificationRequired"))return "Complete website verification in your browser";
  if(!fileInfoRequiresExplicitHttpRetry(record))return {};
  const auto status=num(record,"LastHttpStatus");
  const auto next=addressRejected(record)?"Refresh the address if the link expired":"Start Download to retry";
  return "HTTP "+std::to_string(status)+" - "+next;
 };
 Form dialog("Download File Info",380,128+footerOffset,parent);dialog.dialogUnits=true;HICON fileIcon=nullptr;std::shared_ptr<DownloadPreview> preview;std::string previewVideo,previewAudio,previewAddress;
 std::string extensionType;
 bool zipDialogShowing=false,refreshDialogShowing=false;
 bool waitingForPrefetchSlot=false,draftLoginChanged=false,refreshFailedMedia=false,refreshFailedAddress=false;
 enum class PendingIntent {None,Start,Later};PendingIntent pendingIntent=PendingIntent::None;
 std::function<void(bool)> freezeDraft;std::function<void()> runPendingIntent,laterAction;
 bool pendingIconRequested=false,pendingFailed=false;
 CWnd* pendingRetryControl=nullptr;
 FileInfoInitialTask initialTask;bool initialLoginPrepared=false;std::exception_ptr initialConstructionFailure;
 FileInfoPrefetchTask prefetchTask;
 FileInfoRefreshTask refreshTask;std::vector<std::pair<HWND,BOOL>> refreshControls;std::string refreshError;
 FileInfoCategoryTask categoryTask;std::optional<FileInfoCategoryRequest> queuedCategory;uint64_t categoryRevision=0;FileInfoCategoryDraft categoryDraft;std::string categoryError;bool applyingCategory=false,categorySettled=false;
 FileInfoActionTask actionTask;bool actionCompleting=false;std::string actionError;std::vector<std::pair<HWND,BOOL>> actionControls;std::function<void(const FileInfoActionResult&)> actionCompletion;std::function<void(bool,std::function<void(const FileInfoActionResult&)>)> prepareAction;
 FileInfoNameTask nameTask;std::string nameError;bool nameSettled=false;std::function<void()> pumpName,refreshNameActions;
 std::function<void()> pumpCategory,refreshCategoryActions;
 const auto categoryWaiting=[&]{return categoryTask.valid()||queuedCategory.has_value();};
 FileInfoBindingTask bindingTask;
 FileInfoDecisionTask decisionTask;
 FileInfoPreviewTask previewTask;std::optional<FileInfoPreviewRequest> queuedPreview;uint64_t previewGeneration=0,runningPreviewGeneration=0;std::string previewLookupError;
 const auto advancePreviewGeneration=[&]{if(previewGeneration==std::numeric_limits<uint64_t>::max())throw std::runtime_error("File details identities exhausted. Close this dialog and try again.");++previewGeneration;};
 const auto stopPreviewWork=[&]{queuedPreview.reset();previewTask.cancel();if(preview)preview->stop();};
 const auto invalidatePreview=[&]{
  advancePreviewGeneration();queuedPreview.reset();previewTask.cancel();if(preview){preview->stop();preview.reset();}previewAddress.clear();previewVideo.clear();previewAudio.clear();previewLookupError.clear();
 };
 const auto pumpPreview=[&]{
  if(previewTask.valid()&&previewTask.ready()){
   try{auto result=previewTask.take();if(result.generation==previewGeneration&&result.job==job&&!result.cancelled&&!decisionTask.valid()){
    previewLookupError=std::move(result.message);previewAddress=std::move(result.address);previewVideo=std::move(result.video);previewAudio=std::move(result.audio);preview=std::move(result.preview);
#ifdef UDM_TOOLBAR_COMPONENT_TEST
    if(fileInfoPreviewTestPublished)fileInfoPreviewTestPublished(dialog,result);
#endif
   }else if(result.preview)result.preview->stop();}
   catch(const std::exception& failure){if(runningPreviewGeneration==previewGeneration)previewLookupError=failure.what();}
  }
  if(!previewTask.valid()&&queuedPreview&&!decisionTask.valid()){
   auto request=std::move(*queuedPreview);queuedPreview.reset();runningPreviewGeneration=request.generation;
   try{previewTask.start(m,std::move(request),dialog.taskNotifier()
#ifdef UDM_TOOLBAR_COMPONENT_TEST
    ,fileInfoPreviewTestObserver
#endif
   );}catch(const std::exception& failure){previewLookupError=failure.what();}
  }
 };

 std::unique_ptr<FileInfoDisplayTask> displayTask;
 bool admissionNeedsLookup=false,admissionHasInitialSize=false,cancellingDecision=false;
 std::vector<std::pair<HWND,BOOL>> admissionControls;
 std::function<void()> dispatchCancellation;
 std::vector<std::pair<HWND,BOOL>> decisionControls;
 std::function<bool(FileInfoDecisionDraft)> dispatchDecision;
 bool finishingDecision=false;
 std::vector<CWnd*> acceptanceControls;
 std::optional<std::chrono::steady_clock::time_point> deferredLookup;
 bool initiallyBasic=false,updatingLogin=false;
 std::pair<std::string,std::string> initial;
 std::function<void()> updateLoginControls;
 const auto loadLogin=[&]{
  if(initialLoginPrepared){initialLoginPrepared=false;return;}
  const auto headers=media?Headers{}:siteRequestHeaders(str(data,"Url"),readHeaders(data),prefs);
  initial=basicLogin(headers);
  initiallyBasic=lower(headerValue(headers,"Authorization")).rfind("basic ",0)==0;
 };
 struct AdvancedFields{
  std::vector<CWnd*> controls;
  QueueCombo* queue=nullptr;
  CWnd *useLogin=nullptr,*user=nullptr,*password=nullptr,*remember=nullptr,*detail=nullptr,*refreshDetails=nullptr,*page=nullptr,*schedule=nullptr;
 } advanced;
 // Share one bounded Shell worker across confirmations, including when a handler stalls.
 static AsyncFileIcons fileDetails(FileIconMode::LargeWithType);
 // Dialogs share the UI thread. Retain results for other open dialogs, and
 // give each window its own icon handle so either can close independently.
 static std::deque<std::pair<ULONGLONG,FileIconResult>> recentFileDetails;
 bool fileDetailsApplied=false;
 auto applyFileDetails=[&](CWnd* icon,bool collect){
  if(fileDetailsApplied)return;
  const auto now=GetTickCount64();
  while(!recentFileDetails.empty()&&now-recentFileDetails.front().first>=60000)
   recentFileDetails.pop_front();
  if(collect)for(auto& result:fileDetails.take()){
   if(!result.icon&&result.typeName.empty())continue;
   recentFileDetails.emplace_back(now,std::move(result));
   if(recentFileDetails.size()>64)recentFileDetails.pop_front();
  }
  const auto details=std::find_if(recentFileDetails.rbegin(),recentFileDetails.rend(),
   [&](const auto& entry){return entry.second.extension==extension;});
  if(details==recentFileDetails.rend())return;
  extensionType=details->second.typeName;
  if(details->second.icon)if(auto copied=CopyIcon(details->second.icon)){
   icon->SendMessage(STM_SETICON,(WPARAM)copied);
   fileIcon=copied;
  }
  fileDetailsApplied=true;
 };
 std::vector<fs::path> destinationFolders;
 auto destinationListName=initialTarget.filename();
 const auto buildInitialControls=[&]{
  displayTask=std::make_unique<FileInfoDisplayTask>(dialog.taskNotifier()
#ifdef UDM_TOOLBAR_COMPONENT_TEST
   ,fileInfoDisplayTestObserver
#endif
  );
#ifdef UDM_CAPTURE_PRESENTATION_COMPONENT_TEST
  traceCapturePresentation("file-info-controls-start");
#endif
#ifdef UDM_PRODUCTION_UI_TIMING_TEST
  nativeUiTraceRecord("file-info-controls-start");
#endif
  dialog.control(L"STATIC","URL",SS_RIGHT,5,5,54,10);auto address=dialog.edit(initialAddress,64,3,244,14,true);
  dialog.control(L"STATIC","Category",SS_RIGHT,5,22,54,10);
#ifdef UDM_CAPTURE_PRESENTATION_COMPONENT_TEST
  traceCapturePresentation("file-info-category-create");
#endif
  auto category=dialog.combo({},"",64,22,116);
#ifdef UDM_CAPTURE_PRESENTATION_COMPONENT_TEST
  traceCapturePresentation("file-info-category-created");
#endif
  auto categoryKeys=std::make_shared<std::vector<std::string>>();
  auto selectCategory=[category,categoryKeys](const std::string& key){
   const auto item=std::find(categoryKeys->begin(),categoryKeys->end(),key);
   category->SetCurSel(item==categoryKeys->end()?0:static_cast<int>(item-categoryKeys->begin()));
  };
  auto rebuildCategories=[&m,category,categoryKeys](const std::string& key,const std::vector<std::string>* initial=nullptr){
   *categoryKeys=initial?*initial:m.categories();
   const auto general=std::find(categoryKeys->begin(),categoryKeys->end(),"Other");
   if(general!=categoryKeys->end())std::rotate(categoryKeys->begin(),general,general+1);
   const auto item=std::find(categoryKeys->begin(),categoryKeys->end(),key);
   populateDownloadCategories(*category,*categoryKeys,static_cast<size_t>(item-categoryKeys->begin()));
  };
  auto selectedCategory=[category,categoryKeys]{
   const auto item=category->GetCurSel();
   if(item==CB_ERR)throw std::runtime_error("Choose an existing category.");
   return categoryKeys->at(static_cast<size_t>(item));
  };
  rebuildCategories(str(data,"Category"),&initialView->categories);
#ifdef UDM_CAPTURE_PRESENTATION_COMPONENT_TEST
  traceCapturePresentation("file-info-category-populated");
#endif
  auto editCategory=dialog.button("+",183,21,16,[&,category,rebuildCategories]{
   CategoryPropertiesResult saved;
   const auto name=showCategoryProperties(&dialog,m,"",&saved);
   if(!name)return;
   rebuildCategories(*name,&saved.categories);
   dialog.SendMessage(WM_COMMAND,MAKEWPARAM(category->GetDlgCtrlID(),CBN_SELCHANGE),(LPARAM)category->GetSafeHwnd());
  });
  acceptanceControls.push_back(editCategory);editCategory->EnableWindow(job!=nullptr);
  auto icon=dialog.control(L"STATIC","",SS_ICON,333,28,20,20);
  applyFileDetails(icon,false);
  const bool initialAddressPending=directResponse&&yes(data,"RefreshPendingValidation");
  const auto initialResponseSize=directResponse&&!initialAddressPending?num(data,"Size",-1):-1;
  const auto initialExactSize=initialSavedSize>=0?initialSavedSize:initialResponseSize;
  const auto initialTrackSize=trackBytes(data);
  const auto initialEstimateSize=mediaSizeHint(data);
  const bool initialHasResponseHeaders=hasDirectResponseHeaders(data);
  const auto initialReportedSize=directResponse&&!initialHasResponseHeaders&&initialExactSize<0?reportedDirectResponseSize(data):0;
  auto size=dialog.control(L"STATIC",initialExactSize>=0?bytes(initialExactSize):initialTrackSize>0?bytes(initialTrackSize):initialReportedSize>0?bytes(initialReportedSize):initialAddressPending?bytes(-1):downloadSizeLabel(data),SS_CENTER,311,57,64,10);
  auto byteLine=[](const std::string& caption,i64 length){return caption+": "+std::to_string(length)+" bytes";};
  auto retainedLine=[](const Json& record){
   const auto retained=num(record,"SabrRetainedBytes");
   if(retained>0&&num(record,"StreamCompletedMs")<=0)return std::string("No playable progress yet. Refresh to continue.");
   return retained>0?bytes(retained)+" verified. Refresh to continue.":std::string("Refresh the media session to continue.");
  };
  const auto initialRetryLabel=retryLabel(data);
  const auto initialSizeLine=!initialRetryLabel.empty()?initialRetryLabel:
   yes(data,"SabrSessionRefreshRequired")?retainedLine(data):
   initialExactSize>=0?byteLine(initialSavedSize>=0?"Saved output":trimResponse?"Response size":"File size",initialExactSize):
   initialTrackSize>0?byteLine("Selected tracks",initialTrackSize):
   initialReportedSize>0?byteLine("Browser-reported size",initialReportedSize):
   initialEstimateSize>0&&num(data,"Size",-1)<0?byteLine("Estimated size",initialEstimateSize):
   initialAddressPending?"Size pending address validation":initialHasResponseHeaders?"Final size pending":std::string{};
  auto compactSize=dialog.control(L"STATIC",initialSizeLine,SS_ENDELLIPSIS,7,108+nameRow,366,10);
  const bool initialEstimate=initialExactSize<0&&initialTrackSize==0&&num(data,"Size",-1)<0&&initialEstimateSize>0;
  const auto mime=str(data,"ContentType");
  auto type=dialog.control(L"STATIC",initialSavedSize>=0?"Saved output size":initialTrackSize>0?"Selected track size":initialReportedSize>0?"Reported size":initialEstimate?"Estimated size":mime.empty()?extensionType:mime,SS_CENTER|SS_ENDELLIPSIS,311,69,64,8);
  auto previewButton=dialog.button(media?"Check si&ze":"&Preview",319,78,50,[]{});
  previewButton->ShowWindow(SW_HIDE);
  auto previewStatus=dialog.control(L"STATIC","",SS_ENDELLIPSIS,7,132+footerOffset,366,9);
  dialog.control(L"STATIC","Save As",SS_RIGHT,5,41,54,10);
  destinationFolders.push_back(initialTarget.parent_path());
  if(prefs.contains("SaveFolderHistory")&&prefs["SaveFolderHistory"].is_array())for(const auto& value:prefs["SaveFolderHistory"]){
   if(!value.is_string()||destinationFolders.size()>=21)continue;
   fs::path folder(wide(value.get<std::string>()));
   if(folder.is_absolute())destinationFolders.push_back(std::move(folder));
  }
  std::vector<std::string> destinations;
  for(const auto& folder:destinationFolders)destinations.push_back(utf8((folder/destinationListName).wstring()));
#ifdef UDM_CAPTURE_PRESENTATION_COMPONENT_TEST
  traceCapturePresentation("file-info-destination-start");
#endif
  auto destination=dialog.combo(destinations,destinations.front(),64,39,223,true);
#ifdef UDM_CAPTURE_PRESENTATION_COMPONENT_TEST
  traceCapturePresentation("file-info-destination-ready");
#endif

  dialog.control(L"BUTTON","",BS_GROUPBOX,64,56,244,32);
  auto pathCaption=[category]{
   std::string label;
   for(char ch:text(category)){if(ch=='&')label+='&';label+=ch;}
   return "Remember this path for \""+label+"\" category";
  };
  auto pathRule=dialog.check(pathCaption(),yes(prefs.value("CategoryRememberLast",Json::object()),selectedCategory().c_str()),69,55,232);
  auto updatePathCaption=[pathRule,pathCaption]{pathRule->SetWindowText(cs(pathCaption()));};
  auto categoryFolder=dialog.edit(utf8(initialTarget.parent_path().wstring()),69,69,232);categoryFolder->EnableWindow(dialog.checked(pathRule));dialog.bind(pathRule,[&dialog,pathRule,categoryFolder,destination]{categoryFolder->EnableWindow(dialog.checked(pathRule));if(dialog.checked(pathRule))categoryFolder->SetWindowText(cs(utf8(fs::path(wide(text(destination))).parent_path().wstring())));});
  pumpCategory=[&,categoryKeys,selectedCategory,destination,categoryFolder,pathRule,updatePathCaption,compactSize]{
   categorySettled=false;
   if(categoryTask.valid()&&categoryTask.ready()){
    FileInfoCategoryResult result;try{result=categoryTask.take();}catch(const std::exception& failure){result.request={categoryRevision,selectedCategory()};result.error=failure.what();}catch(...){result.request={categoryRevision,selectedCategory()};result.error="The category folder could not be read. Choose the category again.";}
    if(result.request.revision==categoryRevision&&result.request.category==selectedCategory()){
     if(!result.cancelled&&!result.error.empty()){categoryError=std::move(result.error);pendingIntent=PendingIntent::None;if(freezeDraft)freezeDraft(false);compactSize->SetWindowText(cs(categoryError));}
     else if(const auto next=mergeFileInfoCategory(result,categoryRevision,selectedCategory(),categoryDraft,{fs::path(wide(text(destination))),text(categoryFolder),dialog.checked(pathRule)})){
      categorySettled=true;applyingCategory=true;struct ApplyingCategory{bool& active;~ApplyingCategory(){active=false;}} applying{applyingCategory};
      if(text(destination)!=utf8(next->destination.wstring()))destination->SetWindowText(next->destination.c_str());
      categoryFolder->SetWindowText(cs(next->rememberFolder));pathRule->SendMessage(BM_SETCHECK,next->remember?BST_CHECKED:BST_UNCHECKED);categoryFolder->EnableWindow(next->remember);updatePathCaption();
#ifdef UDM_TOOLBAR_COMPONENT_TEST
      if(fileInfoCategoryTestApplied)fileInfoCategoryTestApplied(dialog,result);
#endif
     }
    }
   }
   if(!categoryTask.valid()&&queuedCategory){auto request=std::move(*queuedCategory);queuedCategory.reset();
    try{categoryTask.start(job?&m:nullptr,job?Json::object():fileInfoCategoryPreferences(prefs),job?std::vector<std::string>{}:*categoryKeys,std::move(request),dialog.taskNotifier()
#ifdef UDM_TOOLBAR_COMPONENT_TEST
     ,fileInfoCategoryTestObserver
#endif
    );}catch(const std::exception& failure){categoryError=failure.what();pendingIntent=PendingIntent::None;if(freezeDraft)freezeDraft(false);compactSize->SetWindowText(cs(categoryError));}
   }
   if(refreshCategoryActions)refreshCategoryActions();
  };
  auto updateCategory=[&,selectedCategory,destination,categoryFolder,pathRule,updatePathCaption,compactSize]{
   if(categoryRevision==std::numeric_limits<uint64_t>::max())throw std::runtime_error("Category choices exhausted. Close File Info and try again.");
   categoryTask.cancelRead();categoryError.clear();categoryDraft={fs::path(wide(text(destination))),text(categoryFolder),dialog.checked(pathRule)};queuedCategory=FileInfoCategoryRequest{++categoryRevision,selectedCategory()};updatePathCaption();compactSize->SetWindowText(L"Updating category folder...");pumpCategory();
  };
  auto categoryTouched=std::make_shared<bool>(false);
  dialog.bindChange(category,[categoryTouched,updateCategory]{*categoryTouched=true;updateCategory();});
  auto syncPath=[&dialog,&destinationFolders,&destinationListName,&applyingCategory,categoryFolder,pathRule,destination](const fs::path& path){
   const auto name=path.filename();
   if(!name.empty()&&name!=destinationListName){
    // Replace the list items without clearing the edit or its undo history.
    const auto draft=text(destination);
    const auto selection=destination->GetEditSel();
    destinationListName=name;
    for(int row=0;row<static_cast<int>(destinationFolders.size());++row){
     if(destination->InsertString(row,(destinationFolders[row]/name).c_str())<0)
      throw std::runtime_error("Cannot update recent destinations.");
     destination->DeleteString(row+1);
    }
    if(text(destination)!=draft)destination->SetWindowText(cs(draft));
    if(destination->GetEditSel()!=selection)destination->SetEditSel(LOWORD(selection),HIWORD(selection));
   }
   if(!applyingCategory&&dialog.checked(pathRule))categoryFolder->SetWindowText(path.parent_path().c_str());
  };
  dialog.bindChange(destination,[destination,syncPath](UINT notification){
   if(notification==CBN_SELCHANGE){
    // Windows can notify the selection before replacing the edit text.
    const auto selected=destination->GetCurSel();
    if(selected!=CB_ERR){
     CString path;
     destination->GetLBText(selected,path);
     syncPath(fs::path(path.GetString()));
     return;
    }
   }
   syncPath(fs::path(wide(text(destination))));
  });
  auto browseDestination=dialog.button("...",292,39,16,[&dialog,destination,syncPath]{
   const fs::path current(wide(text(destination)));
   const auto folder=chooseFolder(&dialog,current.parent_path().wstring());
   if(folder.empty())return;
   const auto next=fs::path(folder)/current.filename();
   destination->SetWindowText(next.c_str());
   syncPath(next);
  });
  dialog.control(L"STATIC","Description",SS_RIGHT,5,93,54,10);auto description=dialog.edit(str(data,"Description"),64,91,244);
#ifdef UDM_CAPTURE_PRESENTATION_COMPONENT_TEST
  traceCapturePresentation("file-info-fields-ready");
#endif
  auto suggestedName=[&](const Json& current,bool previewAllowed,const Json& metadata){return media?std::string{}:fileInfoServerSuggestion(current,draftLoginChanged,previewAllowed,previewAddress,metadata);};
  const auto canUseSuggestedName=[destination](const std::string& name){
   return !name.empty()&&safeName(name)==name&&utf8(fs::path(wide(text(destination))).filename().wstring())!=name;
  };
  CWnd* serverName=nullptr;
  CWnd* useServerName=nullptr;
  if(!media){
   const auto name=suggestedName(data,false,Json::object());
   serverName=dialog.control(L"STATIC",name.empty()?"":"Server name: "+name,SS_ENDELLIPSIS,7,112,266,10);
   const auto nameDraft=[&,selectedCategory,categoryTouched,destination,categoryFolder,pathRule]{return FileInfoNameDraft{fs::path(wide(text(destination))),selectedCategory(),text(categoryFolder),dialog.checked(pathRule),*categoryTouched,draftLoginChanged,categoryRevision,previewGeneration};};
   useServerName=dialog.button("Use server name",279,109,94,[&,nameDraft,categoryKeys,serverName,canUseSuggestedName]{
    if(!job||decisionTask.valid()||finishingDecision||actionTask.valid()||actionCompleting||m.fileInfoDecisionPending||nameTask.valid()||categoryWaiting()||prefetchTask.valid()||refreshTask.valid()||pendingIntent!=PendingIntent::None)return;
    const auto label=text(serverName);if(label.rfind("Server name: ",0)!=0)return;const auto name=label.substr(13);if(!canUseSuggestedName(name))return;
    FileInfoNameRequest request{nameDraft(),name,previewAddress,directResponse,*categoryKeys,preview};nameError.clear();nameSettled=false;nameTask.start(m,job,std::move(request),dialog.taskNotifier()
#ifdef UDM_TOOLBAR_COMPONENT_TEST
     ,fileInfoNameTestObserver
#endif
    );if(refreshNameActions)refreshNameActions();if(refreshCategoryActions)refreshCategoryActions();
   });
   refreshNameActions=[&,useServerName,serverName,canUseSuggestedName]{
    if(decisionTask.valid()||finishingDecision){useServerName->EnableWindow(FALSE);return;}
    const auto label=text(serverName);const bool available=label.rfind("Server name: ",0)==0&&canUseSuggestedName(label.substr(13));const bool waiting=nameTask.valid();
    const std::string caption=waiting?"Checking name...":"Use server name";if(text(useServerName)!=caption)useServerName->SetWindowText(cs(caption));const bool enabled=job&&available&&!waiting&&!categoryWaiting()&&!m.fileInfoDecisionPending&&!prefetchTask.valid()&&!refreshTask.valid()&&pendingIntent==PendingIntent::None;if((useServerName->IsWindowEnabled()!=FALSE)!=enabled)useServerName->EnableWindow(enabled);
   };
   pumpName=[&,nameDraft,destination,categoryFolder,pathRule,selectCategory,updatePathCaption,syncPath,serverName,compactSize]{
    if(!nameTask.ready())return;FileInfoNameResult result;try{result=nameTask.take();}catch(const std::exception& error){result.error=error.what();}catch(...){result.error="The server filename could not be checked. Try again.";}nameSettled=true;
    const bool unchanged=sameFileInfoNameDraft(result.request.draft,nameDraft())&&text(serverName)=="Server name: "+result.request.expectedName;
    if(result.cancelled||result.stale||!result.error.empty()||!unchanged){nameError=!unchanged&&!result.request.expectedName.empty()?"Your destination or filename context changed. Choose Use server name again to apply it.":!result.error.empty()?result.error:"Server filename changed. Review it and choose Use server name again.";pendingIntent=PendingIntent::None;if(freezeDraft)freezeDraft(false);}
    else{
     const bool previous=applyingCategory;applyingCategory=true;struct Restore{bool& value;bool previous;~Restore(){value=previous;}} restore{applyingCategory,previous};
     if(result.categoryChanged){selectCategory(result.category);updatePathCaption();pathRule->SendMessage(BM_SETCHECK,result.remember?BST_CHECKED:BST_UNCHECKED);categoryFolder->SetWindowText(cs(utf8(result.destination.parent_path().wstring())));categoryFolder->EnableWindow(result.remember);}
     destination->SetWindowText(result.destination.c_str());syncPath(result.destination);nameError.clear();
    }
#ifdef UDM_TOOLBAR_COMPONENT_TEST
    if(fileInfoNameTestApplied)fileInfoNameTestApplied(dialog,result);
#endif
    if(!nameError.empty())compactSize->SetWindowText(cs(nameError));if(refreshNameActions)refreshNameActions();if(refreshCategoryActions)refreshCategoryActions();
   };
   refreshNameActions();
  }
  loadLogin();
  const auto loginChanged=[&advanced,&dialog,media,&initiallyBasic,&initial]{
   return !media&&advanced.useLogin&&(dialog.checked(advanced.useLogin)!=initiallyBasic||
    text(advanced.user)!=initial.first||text(advanced.password)!=initial.second);
  };
  const auto initialQueue=str(data,"Queue","Main queue");
  const auto selectedQueue=[&advanced,&initialView,initialQueue]{
   if(advanced.queue)return advanced.queue->queueName();
   const auto& names=initialView->queues;
   if(std::find(names.begin(),names.end(),initialQueue)!=names.end())return initialQueue;
   return names.empty()?std::string{}:names.front();
  };
  const auto initialDetail=formRequest?"Form download (POST). Restarting submits the form again.":str(data,"FormatDescription");
  auto refreshLookup=[&](bool automatic=false){
   if(decisionTask.valid()||actionCompleting||actionTask.valid()||prefetchTask.valid()||!job)return;
   if(automatic&&(previewTask.valid()||queuedPreview))return;
   invalidatePreview();FileInfoPreviewRequest request;request.job=job;request.generation=previewGeneration;request.expectedUrl=str(data,"Url");request.automatic=automatic;request.draftLogin=draftLoginChanged;request.timeoutMs=media?240000:10000;
   if(draftLoginChanged){request.user=text(advanced.user);request.password=text(advanced.password);request.loginEnabled=dialog.checked(advanced.useLogin);}
   queuedPreview=std::move(request);pumpPreview();
  };
  auto tryPrefetch=[&,previewButton](bool lookup=true,bool hasSize=false,Json expectedSelection=Json::object()){
   if(decisionTask.valid()||actionCompleting||actionTask.valid()||prefetchTask.valid())return false;
   if(!prefetchTask.start(m,job,dialog.taskNotifier()
#ifdef UDM_TOOLBAR_COMPONENT_TEST
     ,fileInfoPrefetchTestObserver
#else
     ,{}
#endif
     ,std::move(expectedSelection)))return false;
   waitingForPrefetchSlot=false;
   if(lookup){admissionNeedsLookup=true;admissionHasInitialSize=hasSize;}
   admissionControls.clear();for(auto control:acceptanceControls)if(control&&control->GetSafeHwnd()){admissionControls.push_back({control->GetSafeHwnd(),control->IsWindowEnabled()});control->EnableWindow(FALSE);}
   previewButton->EnableWindow(FALSE);
   return true;
  };
  auto lookupAfterAdmission=[&,refreshLookup](PrefetchAdmission admission,bool hasSize){
   deferredLookup.reset();
   if(media||preview)return;
   if(admission==PrefetchAdmission::Unavailable)refreshLookup(true);
   // A short capacity wait can end before a separate lookup is needed.
   else if(admission==PrefetchAdmission::AtCapacity&&!hasSize)
    deferredLookup=std::chrono::steady_clock::now()+std::chrono::seconds(1);
  };
  auto continueRecovery=[&,compactSize](Json selection){
   invalidatePreview();waitingForPrefetchSlot=false;admissionNeedsLookup=false;deferredLookup.reset();refreshError.clear();
   if(!refreshTask.start(m,job,std::move(selection),dialog.taskNotifier()
#ifdef UDM_TOOLBAR_COMPONENT_TEST
    ,fileInfoRefreshTestObserver
#endif
   ))return;
   refreshControls.clear();EnumChildWindows(dialog.GetSafeHwnd(),[](HWND window,LPARAM context)->BOOL{wchar_t kind[40]{},label[64]{};GetClassNameW(window,kind,40);GetWindowTextW(window,label,64);if(wcscmp(label,L"&Cancel")&&(!_wcsicmp(kind,L"BUTTON")||!_wcsicmp(kind,L"EDIT")||!_wcsicmp(kind,L"COMBOBOX")))reinterpret_cast<std::vector<std::pair<HWND,BOOL>>*>(context)->push_back({window,IsWindowEnabled(window)});return TRUE;},reinterpret_cast<LPARAM>(&refreshControls));
   for(const auto& control:refreshControls)::EnableWindow(control.first,FALSE);
   compactSize->SetWindowText(L"Updating refreshed download details...");
  };
  auto refreshMedia=[&,continueRecovery]{
   if(prefetchTask.valid()||refreshTask.valid())return;
   refreshDialogShowing=true;struct RefreshOwner{bool& active;~RefreshOwner(){active=false;}} owner{refreshDialogShowing};Json selection;
   if(refreshDownloadAddress(&dialog,m,job,true,DownloadRefreshMode::FileInfo,&selection))continueRecovery(std::move(selection));
  };
  auto refreshLink=[&,continueRecovery]{
   if(prefetchTask.valid()||refreshTask.valid())return;
   refreshDialogShowing=true;struct RefreshOwner{bool& active;~RefreshOwner(){active=false;}} owner{refreshDialogShowing};Json selection;
   if(refreshDownloadAddress(&dialog,m,job,true,DownloadRefreshMode::FileInfo,&selection))continueRecovery(std::move(selection));
  };
  if(media)dialog.bind(previewButton,[&,refreshLookup,refreshMedia]{
   if(refreshFailedMedia)refreshMedia();else refreshLookup();
  });
  else dialog.bind(previewButton,[&,refreshLookup,tryPrefetch,loginChanged,refreshLink]{
   if(prefetchTask.valid())return;
   if(refreshFailedAddress){refreshLink();return;}
   FileInfoZipRequest request;request.job=job;request.expectedUrl=str(data,"Url");request.draftLogin=draftLoginChanged;
   if(preview){auto metadata=preview->snapshot();if(str(metadata,"Status")=="Ready")request.mime=str(metadata,"ContentType");}
   if(draftLoginChanged){request.user=text(advanced.user);request.password=text(advanced.password);request.loginEnabled=dialog.checked(advanced.useLogin);}
   invalidatePreview();zipDialogShowing=true;struct ReleaseZip{bool& value;~ReleaseZip(){value=false;}} releaseZip{zipDialogShowing};
   const auto outcome=fileInfoPreparedZipDialog(&dialog,m,std::move(request),str(data,"FileName"));
   if(outcome.resumePrefetch)tryPrefetch(true,false,outcome.selection);
   refreshLookup();
  });
  const bool hasInitialSize=initialResponseSize>=0||initialReportedSize>0;
  dialog.pulse=[&,size,type,compactSize,byteLine,retainedLine,icon,initialDetail,previewStatus,previewButton,serverName,useServerName,suggestedName,canUseSuggestedName,refreshLookup,tryPrefetch,lookupAfterAdmission,address,hasInitialSize,firstPulse=true,fallbackStarted=false,checkedMediaRefresh=false,fastSizePulse=false,prefetchError=std::string{}]() mutable{
   if(finishingDecision||actionCompleting||zipDialogShowing||refreshDialogShowing)return;
   if(decisionTask.valid()){
    if(!decisionTask.ready())return;
    finishingDecision=true;
    const auto restoreCancelledAdmission=[&]{if(cancellingDecision){for(const auto& control:actionControls)if(IsWindow(control.first))::EnableWindow(control.first,control.second);actionControls.clear();actionCompletion={};if(!refreshControls.empty()){refreshError="Recovery cleanup failed. Refresh the saved request again before continuing.";refreshFailedMedia=media;refreshFailedAddress=!media;for(const auto& control:refreshControls)if(IsWindow(control.first))::EnableWindow(control.first,control.second);refreshControls.clear();previewButton->SetWindowText(L"&Refresh...");previewButton->EnableWindow(TRUE);}for(const auto& control:admissionControls)if(IsWindow(control.first))::EnableWindow(control.first,control.second);admissionControls.clear();if(freezeDraft)freezeDraft(false);}};
    try{const auto result=decisionTask.get();if(!result.warning.empty())error(&dialog,std::runtime_error(result.warning));dialog.close(cancellingDecision?IDCANCEL:IDOK);}
    catch(const std::exception& failure){for(const auto& control:decisionControls)if(IsWindow(control.first))::EnableWindow(control.first,control.second);decisionControls.clear();restoreCancelledAdmission();dialog.SetTimer(Form::pulseTimer,Form::pulseInterval,nullptr);error(&dialog,failure);}
    catch(...){for(const auto& control:decisionControls)if(IsWindow(control.first))::EnableWindow(control.first,control.second);decisionControls.clear();restoreCancelledAdmission();error(&dialog,std::runtime_error("The download decision failed. Your edits are still available."));}
    decisionTask.release();cancellingDecision=false;finishingDecision=false;if(auto frame=AfxGetMainWnd();frame&&frame->GetSafeHwnd())frame->PostMessage(WM_TIMER,1,0);return;
   }
   if(actionTask.valid()){
    if(!actionTask.ready())return;auto result=actionTask.take();auto completion=std::move(actionCompletion);actionCompleting=true;
    dialog.afterCurrentPulse([&,result=std::move(result),completion=std::move(completion),compactSize]()mutable{
     struct FinishAction{bool& active;~FinishAction(){active=false;}} finish{actionCompleting};
     for(const auto& control:actionControls)if(IsWindow(control.first))::EnableWindow(control.first,control.second);actionControls.clear();
     if(result.cancelled)return;
     if(!result.error.empty()){actionError=result.error;compactSize->SetWindowText(cs(actionError));return;}
     try{if(completion)completion(result);}catch(const std::exception& failure){actionError=failure.what();compactSize->SetWindowText(cs(actionError));}catch(...){actionError="The download action could not be prepared. Try again.";compactSize->SetWindowText(cs(actionError));}
    });return;
   }
   if(pumpName)pumpName();
   if(pumpCategory)pumpCategory();
   if((categorySettled||nameSettled)&&!nameTask.valid()&&!m.fileInfoDecisionPending&&!categoryWaiting()&&categoryError.empty()&&job&&pendingIntent!=PendingIntent::None&&!prefetchTask.valid()&&!refreshTask.valid()&&runPendingIntent){runPendingIntent();return;}
   if(refreshTask.valid()){
    if(!refreshTask.ready())return;
    FileInfoRefreshResult refreshed;try{refreshed=refreshTask.take();}catch(const std::exception& failure){refreshed.error=failure.what();}catch(...){refreshed.error="The refreshed download details could not be read.";}
    for(const auto& control:refreshControls)if(IsWindow(control.first))::EnableWindow(control.first,control.second);refreshControls.clear();
    if(refreshed.cancelled||!refreshed.error.empty()){
     refreshError=refreshed.error.empty()?"Refreshed details were cancelled. Refresh again to continue.":std::move(refreshed.error);pendingIntent=PendingIntent::None;if(freezeDraft)freezeDraft(false);refreshFailedMedia=media;refreshFailedAddress=!media;previewButton->SetWindowText(L"&Refresh...");previewButton->ShowWindow(SW_SHOW);previewButton->EnableWindow(TRUE);compactSize->SetWindowText(cs(refreshError));return;
    }
    refreshError.clear();prefetchTask.clearSelection();
    if(!refreshed.media){
     for(const auto* key:{"Url","ProtectedHeaders","DownloadPage"})data[key]=str(refreshed.data,key);
     if(refreshed.preferences.contains("SiteLogins"))prefs["SiteLogins"]=std::move(refreshed.preferences["SiteLogins"]);else prefs.erase("SiteLogins");
     address->SetWindowText(cs(refreshed.address));initial=std::move(refreshed.login);initiallyBasic=refreshed.basic;
     if(advanced.useLogin){updatingLogin=true;struct RestoreLogin{bool& active;~RestoreLogin(){active=false;}} restoreLogin{updatingLogin};advanced.useLogin->SendMessage(BM_SETCHECK,initiallyBasic?BST_CHECKED:BST_UNCHECKED);advanced.user->SetWindowText(cs(initial.first));advanced.password->SetWindowText(cs(initial.second));advanced.remember->SendMessage(BM_SETCHECK,BST_UNCHECKED);advanced.page->SetWindowText(cs(refreshed.page));updateLoginControls();}
     draftLoginChanged=false;
    }
    try{tryPrefetch(true,false,std::move(refreshed.selection));}catch(const std::exception& failure){refreshError=failure.what();pendingIntent=PendingIntent::None;if(freezeDraft)freezeDraft(false);compactSize->SetWindowText(cs(refreshError));}
#ifdef UDM_TOOLBAR_COMPONENT_TEST
    if(fileInfoRefreshTestApplied)fileInfoRefreshTestApplied(dialog,refreshed);
#endif
    return;
   }
   if(!refreshError.empty())return;
   if(m.fileInfoDecisionPending)return;
   if(!job){
    const auto next=pending->latest();if(next!=pending){pending=next;pendingFailed=false;pendingIntent=PendingIntent::None;}
    dialog.SetTimer(Form::pulseTimer,40,nullptr);
    applyFileDetails(icon,true);if(!pendingIconRequested){pendingIconRequested=true;fileDetails.request(extension);}
    if(!pending->retrying()&&pending->outcome()!=PendingFileInfoInbox::Outcome::Pending&&!pendingFailed){
     if(bindingTask.valid()&&bindingTask.ready()){
      FileInfoBindingResult result;try{result=bindingTask.take();}catch(...){pendingFailed=true;}
      if(!pendingFailed&&result.operation==pending&&!result.deferred){
       if(result.job&&!result.cancelled&&pending->attach()){
      job=std::move(result.job);if(pendingBound)*pendingBound=true;if(pendingPresentation)*pendingPresentation=std::move(result.presentation);
      for(auto control:acceptanceControls)control->EnableWindow(TRUE);
      if(pendingRetryControl){pendingRetryControl->SetWindowText(L"More >>");pendingRetryControl->EnableWindow(TRUE);}
      if(freezeDraft)freezeDraft(false);
      dialog.SetTimer(Form::pulseTimer,Form::pulseInterval,nullptr);
#ifdef UDM_PRODUCTION_UI_TIMING_TEST
      nativeUiTraceRecord("pending-file-info-bound");
#endif
       }else pendingFailed=true;
      }
     }
     if(!job&&!pendingFailed&&!bindingTask.valid()&&!dialog.taskPulseActive())bindingTask.start(m,pending,dialog.taskNotifier()
#ifdef UDM_TOOLBAR_COMPONENT_TEST
      ,fileInfoBindingTestObserver
#endif
     );
    }
    if(!job){
     const auto message=pending->retrying()?"Retrying download admission...":pendingFailed?(pending->outcome()==PendingFileInfoInbox::Outcome::Accepted?"This download was already accepted. Check download history to continue.":pending->outcome()==PendingFileInfoInbox::Outcome::Rejected?(pending->supportsRetry()?"UDM could not accept this download. Your edits are retained; choose Retry.":"UDM could not accept this download. Your draft is retained; close this dialog to capture again."):"UDM could not confirm acceptance. Your draft is retained; check download history before capturing again."):pendingIntent==PendingIntent::Start?"Starting after the download is accepted...":pendingIntent==PendingIntent::Later?"Adding to the queue after acceptance...":"Preparing the download. You can edit the destination or start now.";
     if(text(compactSize)!=message)compactSize->SetWindowText(cs(message));
     if(pendingFailed&&freezeDraft)freezeDraft(true);
     if(pendingRetryControl){const bool retry=pendingFailed&&pending->outcome()==PendingFileInfoInbox::Outcome::Rejected&&pending->supportsRetry();pendingRetryControl->SetWindowText(retry?L"&Retry":L"More >>");pendingRetryControl->EnableWindow(retry&&pending->canRetry());}
     return;
    }
   }
   if(prefetchTask.valid()&&prefetchTask.ready()){
    FileInfoPrefetchResult result;bool failed=false;
    try{result=prefetchTask.take();prefetchError.clear();}catch(const std::exception& e){failed=true;prefetchError=e.what();}catch(...){failed=true;prefetchError="Background preparation failed.";}
    for(const auto& control:admissionControls)if(IsWindow(control.first))::EnableWindow(control.first,control.second);admissionControls.clear();
    waitingForPrefetchSlot=result.deferred||result.admission==PrefetchAdmission::AtCapacity;
    if(result.admission==PrefetchAdmission::Started){
     advancePreviewGeneration();queuedPreview.reset();previewTask.cancel();
     if(preview&&str(preview->snapshot(),"Status")!="Ready"){preview->stop();preview.reset();}
    }
    if(pendingIntent!=PendingIntent::None&&!nameTask.valid()&&!categoryWaiting()&&categoryError.empty()&&runPendingIntent){runPendingIntent();return;}
    if(!result.deferred){
     if(admissionNeedsLookup){admissionNeedsLookup=false;lookupAfterAdmission(result.admission,admissionHasInitialSize);}
     else if(deferredLookup&&(result.admission!=PrefetchAdmission::AtCapacity||std::chrono::steady_clock::now()>=*deferredLookup)){
      deferredLookup.reset();if(result.admission!=PrefetchAdmission::Started&&!preview)refreshLookup(true);
     }
    }
    if(failed)waitingForPrefetchSlot=false;
    displayTask->request(m,job,draftLoginChanged);
   }
   pumpPreview();
   const auto attemptPrefetch=[&](bool lookup,bool hasSize=false){
    try{tryPrefetch(lookup,hasSize);}
    catch(const std::exception& e){prefetchError=e.what();waitingForPrefetchSlot=false;}
   };
   if(firstPulse)dialog.KillTimer(initialPrefetchTimer);
   if(firstPulse&&dialog.IsWindowEnabled()){
#ifdef UDM_CAPTURE_PRESENTATION_COMPONENT_TEST
    traceCapturePresentation("first-prefetch-pulse");
#endif
#ifdef UDM_PRODUCTION_UI_TIMING_TEST
    nativeUiTraceRecord("first-prefetch-pulse");
#endif
    firstPulse=false;
    applyFileDetails(icon,true);
    if(!fileDetailsApplied)fileDetails.request(extension);
    if(!draftLoginChanged&&!preview)attemptPrefetch(true,hasInitialSize);
    else lookupAfterAdmission(PrefetchAdmission::Unavailable,hasInitialSize);
#ifdef UDM_CAPTURE_PRESENTATION_COMPONENT_TEST
    traceCapturePresentation("first-prefetch-returned");
#endif
#ifdef UDM_PRODUCTION_UI_TIMING_TEST
    nativeUiTraceRecord("first-prefetch-returned");
#endif
   }else if(waitingForPrefetchSlot&&!draftLoginChanged&&dialog.IsWindowEnabled()&&!prefetchTask.valid()&&!dialog.taskPulseActive()){
    attemptPrefetch(false);
   }
   const bool waitingForSlot=waitingForPrefetchSlot&&!draftLoginChanged;
   applyFileDetails(icon,true);
   // Completion wakes consume the last coherent view. Only cadence/input
   // pulses request another read, avoiding a completion/request spin loop.
   const auto displayResult=displayTask->take();
   if(!dialog.taskPulseActive())displayTask->request(m,job,draftLoginChanged);
   const auto view=displayResult.snapshot;
   if(!view||view->job!=job||view->draftLogin!=draftLoginChanged)return;
   if(!view->member){if(freezeDraft)freezeDraft(true);compactSize->SetWindowText(L"This download is no longer available.");return;}
   Json current=view->data;
   const bool active=view->active,canCheck=view->canCheck,replacementPending=view->replacementPending;
   const auto savedOutputSize=view->savedOutputSize,assembledOutputSize=view->assembledOutputSize;
   projectHttpOutput(current);
   const bool pending=yes(current,"ConfirmationPending");
   const bool matchingPrefetch=pending&&active&&!draftLoginChanged;
   const bool savingReceipt=matchingPrefetch&&str(current,"ConnectionStatus")==SavingPrefetchReceipt;
   const bool ftpWaiting=matchingPrefetch&&str(current,"ConnectionStatus")=="Waiting for FTP confirmation";
   const bool prefetchFailed=pending&&!active&&str(current,"Status")=="Failed";
   const bool sessionRejected=yes(current,"SabrSessionRefreshRequired");
   const auto explicitRetryLabel=draftLoginChanged?std::string{}:retryLabel(current);
   if(media&&!active&&(prefetchFailed||sessionRejected)){
    if(!checkedMediaRefresh)refreshFailedMedia=view->canRefreshAddress;
    checkedMediaRefresh=true;
   }else checkedMediaRefresh=refreshFailedMedia=false;
   if(prefetchFailed&&!media&&!draftLoginChanged&&!preview&&!fallbackStarted&&num(current,"Size",-1)<0&&canCheck){
    fallbackStarted=true;
    refreshLookup(true);
   }
   auto metadata=preview?preview->snapshot():Json::object();const bool preparingLookup=previewTask.valid()||queuedPreview.has_value();auto phase=str(metadata,"Status");
   if(yes(metadata,"CalculatedMediaSize")&&(!current["Video"].is_object()||!current["Audio"].is_object()||str(current["Video"],"Url")!=previewVideo||str(current["Audio"],"Url")!=previewAudio)){metadata=Json::object();phase="Unavailable";}
   if(!media&&str(current,"Url")!=previewAddress){metadata=Json::object();phase="Unavailable";}
   if(preparingLookup)phase="Checking";else if(!previewLookupError.empty()){metadata={{"Status","Error"},{"Message",previewLookupError}};phase="Error";}
   if(!media){
    auto name=suggestedName(current,canCheck,metadata);
    const auto label=name.empty()?(phase=="Checking"||(active&&yes(current,"ConfirmationPending"))?"Checking server filename...":"No server filename available."):"Server name: "+name;
    if(text(serverName)!=label)serverName->SetWindowText(cs(label));
    if(refreshNameActions)refreshNameActions();
   }
   const bool hasResponseHeaders=hasDirectResponseHeaders(current);
   // Admission clears the old response marker. A matching active HTTP
   // response can report its length while the transfer validates the body.
   const bool replacementHasHeaders=replacementPending&&matchingPrefetch&&hasResponseHeaders;
   const bool addressPending=directResponse&&yes(current,"RefreshPendingValidation")&&!replacementHasHeaders;
   i64 length=draftLoginChanged||addressPending?-1:num(current,"Size",-1);auto mime=draftLoginChanged?std::string{}:str(current,"ContentType");std::string status;
   const bool showingPreview=!pendingHttpGzip(current)&&(draftLoginChanged||(!active&&!hasResponseHeaders));
   const auto checkedSize=phase=="Ready"?num(metadata,"Size",-1):-1;
   // Admission clears the response URL. Keep the HEAD size as a hint until
   // the GET supplies its own headers, even when those headers omit a size.
   const bool keepReportedSize=matchingPrefetch&&directResponse&&length<0&&
    !hasResponseHeaders&&checkedSize>=0;
   bool previewSize=false;
   if((showingPreview&&phase=="Ready")||keepReportedSize){
    if(checkedSize>=0||!directResponse)length=checkedSize;
    previewSize=checkedSize>=0;
    if(showingPreview)mime=str(metadata,"ContentType");
    status=yes(metadata,"CalculatedMediaSize")?"Calculated final MP4: "+std::to_string(length)+" bytes from complete sample metadata and a codec probe.":checkedSize<0?"The metadata check did not return a size.":directResponse?"The server reported this size. The download response supplies the final size.":"File details checked. No file data has been saved.";
   }
   else if(showingPreview&&phase=="Checking")status=media?"Calculating final size from media headers. You can start now; calculation may take several minutes.":"Checking file details...";
   else if(showingPreview&&phase=="Error")status=str(metadata,"Message");
   else if(draftLoginChanged)status="Login changed. Refresh details to check this request.";
   // An edited login describes a different request even though its URL has
   // not changed. Keep the original prefetch available if the edit is undone.
   const auto exactPrefetch=matchingPrefetch&&(directResponse||str(current,"MediaPhase")=="Waiting for confirmation")?num(current,"PrefetchedFinalSize",-1):-1;
   const bool waitingForAssemblySize=media&&matchingPrefetch&&exactPrefetch<0&&
    (str(current,"Status")=="Merging"||assembledOutputSize>=0);
   if(waitingForAssemblySize!=fastSizePulse&&
      dialog.SetTimer(Form::pulseTimer,waitingForAssemblySize?80:Form::pulseInterval,nullptr))
    fastSizePulse=waitingForAssemblySize;
   const auto measuredPrefetch=matchingPrefetch&&exactPrefetch<0?assembledOutputSize:-1;
   if(exactPrefetch>=0){length=exactPrefetch;status=directResponse?
    (num(current,"Received")<num(current,"Size")?
     "Final size from the response. Background download is still receiving bytes.":
     savingReceipt?"Final size is known. UDM is checking the downloaded data.":
     "Final size checked against the prefetched response. Waiting for confirmation."):
    "Exact final file: "+std::to_string(length)+" bytes. Prefetched and verified; waiting for confirmation.";}
   else if(measuredPrefetch>=0){
    length=measuredPrefetch;
    status="Assembled file: "+std::to_string(length)+" bytes. Verification is in progress. You can start now.";
   }
   const bool calculatedSize=!active&&phase=="Ready"&&yes(metadata,"CalculatedMediaSize")&&length>=0;
   const bool exactMediaSize=exactPrefetch>=0||measuredPrefetch>=0||calculatedSize;
   const auto savedSize=exactMediaSize?-1:savedOutputSize;
   if(savedSize>=0){
    length=savedSize;
    status="Saved output: "+std::to_string(length)+" bytes. UDM verifies the file before saving it to your destination.";
   }
   const bool preparingMedia=media&&active&&pending&&!exactMediaSize&&savedSize<0;
   if(preparingMedia)status="Preparing exact final size by downloading and assembling the selected streams in the background. You can start now.";
   const auto trackSize=!exactMediaSize&&savedSize<0?trackBytes(current):0;
   // Once the GET has answered, only that response can establish its final size.
   const auto reportedSize=directResponse&&!hasResponseHeaders&&!draftLoginChanged&&length<0?reportedDirectResponseSize(current):0;
   const bool estimated=length<0&&!trackSize&&mediaSizeHint(current)>0;
   auto sizeText=trackSize>0?bytes(trackSize):length>=0?bytes(length):draftLoginChanged||addressPending?bytes(-1):downloadSizeLabel(current);
   const bool serverReportedSize=directResponse&&previewSize&&exactPrefetch<0;
   const bool responseSize=directResponse&&length>=0&&!addressPending&&!serverReportedSize;
   if(reportedSize>0)sizeText=bytes(reportedSize);
   else if(length<0&&!trackSize&&!estimated&&waitingForSlot)sizeText="Waiting...";
   else if(length<0&&!trackSize&&!estimated&&preparingMedia)sizeText="Preparing...";
   else if(length<0&&phase=="Checking"&&!trackSize&&!estimated)sizeText="Checking...";
   if(text(size)!=sizeText)size->SetWindowText(cs(sizeText));
   if(trackSize>0)status="Selected tracks: "+std::to_string(trackSize)+" bytes. Final file size may differ."+(phase=="Checking"?" Calculating final size...":"");
   else if(reportedSize>0&&status.empty())status="Size reported by the browser. The download response supplies the final size.";
   else if(estimated)status="Approximate size from the selected media tracks. The final file size may differ after combining them.";
   if(waitingForSlot)status="Waiting for a transfer slot to start downloading in the background.";
   if(text(previewStatus)!=status)previewStatus->SetWindowText(cs(status));
   const auto sizeCaption=exactPrefetch>=0?"Exact final size":measuredPrefetch>=0?"Assembled size":calculatedSize?"Calculated size":savedSize>=0?"Saved output size":trackSize>0?"Selected track size":reportedSize>0||serverReportedSize?"Reported size":estimated?"Estimated size":mime.empty()?extensionType:mime;
   if(text(type)!=sizeCaption)type->SetWindowText(cs(sizeCaption));
   std::string compactText;
   if(!explicitRetryLabel.empty())compactText=explicitRetryLabel;
   else if(sessionRejected)compactText=retainedLine(current);
   else if(exactPrefetch>=0)compactText=byteLine("Exact final size",length);
   else if(measuredPrefetch>=0)compactText=byteLine("Assembled size",length);
   else if(calculatedSize)compactText=byteLine("Calculated final size",length);
   else if(savedSize>=0)compactText=byteLine("Saved output",length);
   else if(responseSize)compactText=byteLine(trimResponse||replacementHasHeaders?"Response size":"File size",length);
   else if(serverReportedSize)compactText=byteLine("Server-reported size",length);
   else if(trackSize>0)compactText=byteLine("Selected tracks",trackSize);
   else if(reportedSize>0)compactText=byteLine("Browser-reported size",reportedSize);
   else if(estimated)compactText=byteLine("Estimated size",mediaSizeHint(current));
   else if(waitingForSlot)compactText="Waiting for a transfer slot";
   else if(draftLoginChanged&&phase!="Checking")compactText="Size pending login validation";
   else if(addressPending&&!draftLoginChanged)compactText="Size pending address validation";
   else if(matchingPrefetch&&hasResponseHeaders&&length<0)compactText="Final size pending";
   else if(preparingMedia)compactText="Preparing final size";
   else if(ftpWaiting)compactText="Start Download to begin this FTP transfer";
   else if(phase=="Checking")compactText="Checking size";
   else if(!prefetchError.empty()&&!active&&!draftLoginChanged)compactText="Background download could not start";
   if(prefetchFailed&&explicitRetryLabel.empty()&&!draftLoginChanged&&!sessionRejected){
    if(!compactText.empty())compactText+="  |  ";
    compactText+="Background download stopped";
   }
   if(text(compactSize)!=compactText)compactSize->SetWindowText(cs(compactText));
   const bool canRefresh=!prefetchTask.valid()&&!preparingLookup&&(refreshFailedMedia||(canCheck&&phase!="Checking"));
   auto refreshDetails=media?previewButton:advanced.refreshDetails;
   if(refreshDetails&&(refreshDetails->IsWindowEnabled()!=FALSE)!=canRefresh)refreshDetails->EnableWindow(canRefresh);
   refreshFailedAddress=directResponse&&!active&&!draftLoginChanged&&addressRejected(current)&&view->canRefreshAddress;
   const auto label=refreshFailedMedia||refreshFailedAddress?"&Refresh...":media?"Check si&ze":"&Preview";
   if(text(previewButton)!=label)previewButton->SetWindowText(cs(label));
   bool showPreview=refreshFailedMedia||canCheck;
   if(!media){
    showPreview=refreshFailedAddress||((!replacementPending||matchingPrefetch)&&canPreviewZip(view->zipInputs,mime,!draftLoginChanged));
   }
   if(((previewButton->GetStyle()&WS_VISIBLE)!=0)!=showPreview)previewButton->ShowWindow(showPreview?SW_SHOW:SW_HIDE);
   if(!media){const bool enabled=showPreview&&!prefetchTask.valid();if((previewButton->IsWindowEnabled()!=FALSE)!=enabled)previewButton->EnableWindow(enabled);}
   if(status.empty()&&!previewLookupError.empty())status=previewLookupError;
   auto detailText=status.empty()?initialDetail:status;
   if(!explicitRetryLabel.empty())detailText=str(current,"Error",explicitRetryLabel);
   else if(sessionRejected)detailText="Refresh the media session in your browser to continue.";
   else if(prefetchFailed&&!draftLoginChanged)detailText="Background download stopped: "+str(current,"Error");
   else if(ftpWaiting)detailText="This FTP server does not support resume. Start Download to begin.";
   else if(replacementPending&&!waitingForSlot&&!matchingPrefetch&&!draftLoginChanged)
    detailText="Start Download to check the replacement address and file size.";
   else if(exactPrefetch<0&&matchingPrefetch)detailText="Downloaded: "+bytes(num(current,"Received"))+". Waiting for your confirmation.";
   else if(!prefetchError.empty()&&!active&&!draftLoginChanged)detailText="Background download could not start: "+prefetchError;
   if(advanced.detail&&text(advanced.detail)!=detailText)advanced.detail->SetWindowText(cs(detailText));
#ifdef UDM_TOOLBAR_COMPONENT_TEST
   if(fileInfoDisplayTestRendered)fileInfoDisplayTestRendered(dialog,view);
#endif
   if(!actionError.empty())compactSize->SetWindowText(cs(actionError));else if(!nameError.empty())compactSize->SetWindowText(cs(nameError));else if(!categoryError.empty())compactSize->SetWindowText(cs(categoryError));else if(categoryWaiting())compactSize->SetWindowText(L"Updating category folder...");
   if(refreshCategoryActions)refreshCategoryActions();
   if(pendingIntent!=PendingIntent::None&&!prefetchTask.valid()&&!nameTask.valid()&&!categoryWaiting()&&categoryError.empty()&&runPendingIntent)runPendingIntent();
  };
  auto buildAdvanced=[&,initialQueue,initialDetail,refreshLookup,loginChanged](const std::function<bool(CWnd*)>& schedule){
   if(advanced.queue)return;
   auto track=[&advanced](CWnd* w){advanced.controls.push_back(w);return w;};
   track(dialog.button("Links...",319,91,50,[&]{downloadLinkDetails(&dialog,m,job);}));
   track(dialog.control(L"STATIC","",SS_ETCHEDHORZ,7,145+footerOffset,366,1));
   track(dialog.label("Web page",7,157+footerOffset,51));
   auto page=advanced.page=track(dialog.edit(recoveryPage(data),64,154+footerOffset,244,14,true));
   auto open=track(dialog.button("Open page",319,154+footerOffset,50,[&dialog,page]{Url u(text(page));if(u.scheme!="http"&&u.scheme!="https")throw std::runtime_error("No HTTP or HTTPS parent page is available.");openFile(&dialog,fs::path(wide(u.full)));}));
   open->EnableWindow(!text(page).empty());
   track(dialog.label("Queue",7,179+footerOffset,51));
   auto names=initialView->queues;if(names.empty())names.push_back(initialQueue);
   auto queue=dialog.queueCombo(names,initialView->queueLabels,initialQueue,64,175+footerOffset,244);track(queue);
   advanced.schedule=track(dialog.button("Schedule...",319,175+footerOffset,54,[&dialog,schedule]{if(schedule(&dialog))dialog.close();}));
   auto useLogin=advanced.useLogin=track(dialog.check("Use login and password",initiallyBasic,7,198+footerOffset,266));
   useLogin->EnableWindow(!media);
   track(dialog.label("Login",7,219+footerOffset,51));
   auto user=advanced.user=track(dialog.edit(initial.first,64,216+footerOffset,112));
   track(dialog.label("Password",187,219+footerOffset,56));
   auto password=advanced.password=track(dialog.edit(initial.second,247,216+footerOffset,126,14,false,false,true));
   auto remember=advanced.remember=track(dialog.check("Remember for this site",false,64,238+footerOffset,180));
   auto show=track(dialog.check("Show password",false,247,238+footerOffset,126));
   const bool canRememberLogin=!media&&siteLoginScheme(Url(str(data,"Url")).scheme);
   auto enabled=[&dialog,media,useLogin,user,password,remember,show,canRememberLogin]{bool on=!media&&dialog.checked(useLogin);user->EnableWindow(on);password->EnableWindow(on);show->EnableWindow(on);remember->EnableWindow(on&&canRememberLogin);};
   updateLoginControls=enabled;
   auto loginEdited=[&,loginChanged]{
    if(updatingLogin)return;
    draftLoginChanged=loginChanged();
    invalidatePreview();
    if(dialog.pulse)dialog.pulse();
   };
   dialog.bind(useLogin,[enabled,loginEdited]{enabled();loginEdited();});enabled();
   dialog.bindChange(user,loginEdited);dialog.bindChange(password,loginEdited);
   dialog.bind(show,[&dialog,show,password]{password->SendMessage(EM_SETPASSWORDCHAR,dialog.checked(show)?0:0x25cf);password->Invalidate();});
   advanced.detail=track(dialog.label(initialDetail,7,259+footerOffset,260,28));
   if(!media)advanced.refreshDetails=track(dialog.button("Refresh details",279,259+footerOffset,94,refreshLookup));
   advanced.queue=queue;
  };
  auto captureDecision=[&,selectedCategory,pathRule,categoryFolder,destination,description,selectedQueue,loginChanged](bool startNow,const DownloadQueueChoice* choice=nullptr,const ScheduledDownloadDraft* scheduled=nullptr,bool startQueue=false){
   auto path=scheduled?scheduled->target:fs::path(wide(text(destination)));
   FileInfoFolderChoice folderChoice;
   if(scheduled?scheduled->rememberFolder:dialog.checked(pathRule)){
    auto remembered=scheduled?path.parent_path():fs::path(wide(trim(text(categoryFolder))));
    if(!remembered.is_absolute())throw std::runtime_error("Choose an absolute category folder.");
    folderChoice.rememberedCategoryFolder=std::move(remembered);
   }
   invalidatePreview();
   const bool loginEnabled=!media&&advanced.useLogin&&dialog.checked(advanced.useLogin);
   const bool rememberLogin=!media&&advanced.remember&&dialog.checked(advanced.remember);
   if(loginEnabled){auto validation=readHeaders(data);setBasicLogin(validation,text(advanced.user),text(advanced.password));}
   const auto categoryName=scheduled?scheduled->category:selectedCategory();
   Json edit={{"Folder",utf8(path.parent_path().wstring())},{"FileName",utf8(path.filename().wstring())},{"Category",categoryName},{"Description",scheduled?scheduled->description:text(description)},{"Queue",choice?choice->name:selectedQueue()}};
   if(scheduled){
    if(scheduled->connections)edit["Connections"]=*scheduled->connections;
    if(scheduled->speedLimit)edit["LimitKbps"]=*scheduled->speedLimit;
   }
   const bool loginUnchanged=!loginChanged();
   Json rememberedLogin;
   if(loginUnchanged&&loginEnabled&&rememberLogin)
    rememberedLogin=makeSiteLogin(Url(str(data,"Url")).origin,initial.first,initial.second);
   std::optional<FileInfoLogin> login;
   if(!loginUnchanged||rememberLogin){
    login=FileInfoLogin{text(advanced.user),text(advanced.password),loginEnabled&&rememberLogin,loginEnabled};
   }
   FileInfoDecisionDraft draft{std::move(edit),std::move(rememberedLogin),std::move(folderChoice),std::move(login),choice?std::optional<DownloadQueueChoice>(*choice):std::nullopt,startNow,media,loginUnchanged,startQueue};
   return draft;
  };
  prepareAction=[&,compactSize](bool starting,std::function<void(const FileInfoActionResult&)> completion){
   if(decisionTask.valid()||finishingDecision||actionCompleting||actionTask.valid())return;
   actionControls.clear();EnumChildWindows(dialog.GetSafeHwnd(),[](HWND window,LPARAM context)->BOOL{wchar_t kind[40]{},label[64]{};GetClassNameW(window,kind,40);GetWindowTextW(window,label,64);if(wcscmp(label,L"&Cancel")&&(!_wcsicmp(kind,L"BUTTON")||!_wcsicmp(kind,L"EDIT")||!_wcsicmp(kind,L"COMBOBOX")))reinterpret_cast<std::vector<std::pair<HWND,BOOL>>*>(context)->push_back({window,IsWindowEnabled(window)});return TRUE;},reinterpret_cast<LPARAM>(&actionControls));
   try{actionTask.start(m,job,dialog.taskNotifier()
#ifdef UDM_TOOLBAR_COMPONENT_TEST
    ,fileInfoActionTestObserver
#else
    ,FileInfoActionObserver{}
#endif
    ,starting?FileInfoActionKind::Start:FileInfoActionKind::Later);}catch(...){actionControls.clear();throw;}
   actionCompletion=std::move(completion);actionError.clear();for(const auto& control:actionControls)::EnableWindow(control.first,FALSE);
   compactSize->SetWindowText(starting?L"Preparing download...":L"Preparing queue selection...");dialog.SetTimer(Form::pulseTimer,40,nullptr);dialog.UpdateWindow();
  };
  auto apply=[&,captureDecision](bool startNow,const DownloadQueueChoice* choice=nullptr,bool startQueue=false){return dispatchDecision(captureDecision(startNow,choice,nullptr,startQueue));};
  auto schedule=[&m,&dialog,&job,&prefetchTask,&categoryWaiting,&categoryError,&nameTask,&decisionTask,&finishingDecision,&actionTask,destination,description,selectedCategory,pathRule,captureDecision](CWnd* owner){
   if(!job||decisionTask.valid()||finishingDecision||actionTask.valid()||nameTask.valid()||prefetchTask.valid()||categoryWaiting()||!categoryError.empty())return false;
   ScheduledDownloadDraft initial{fs::path(wide(text(destination))),text(description),selectedCategory(),dialog.checked(pathRule),{}};
   return scheduleDownloadWizard(owner,m,std::move(initial),[&m,&job,captureDecision](const ScheduledDownloadDraft& draft){auto command=captureDecision(false,&draft.queue,&draft);return [source=&m,target=job,command=std::move(command)]{return applyFileInfoDecision(*source,target,command);};},Json(),false,job);
  };
  dialog.accept=[&,apply,refreshMedia,compactSize]{
   if(decisionTask.valid()||finishingDecision||actionTask.valid()||actionCompleting)return;
   if(!job){if(!pendingFailed){pendingIntent=PendingIntent::Start;if(freezeDraft)freezeDraft(true);}return;}
   if(nameTask.valid()||prefetchTask.valid()||refreshTask.valid()||categoryWaiting()){pendingIntent=PendingIntent::Start;if(freezeDraft)freezeDraft(true);return;}
   if(!categoryError.empty()){compactSize->SetWindowText(cs(categoryError));return;}
   if(!refreshError.empty()){compactSize->SetWindowText(cs(refreshError));return;}
   prepareAction(true,[&,apply,refreshMedia](const FileInfoActionResult& result){if(result.refreshSession){refreshMedia();return;}if(apply(true))dialog.close();});
  };
  dialog.cancel=[&]{
   if(decisionTask.valid()||finishingDecision||actionCompleting)return;
   if(!job){pending->requestCancel();dialog.close(IDCANCEL);return;}
   dispatchCancellation();
  };
  const bool initiallyExpanded=initiallyBasic||formRequest;
#ifdef UDM_CAPTURE_PRESENTATION_COMPONENT_TEST
  traceCapturePresentation("file-info-actions-start");
#endif
  if(initiallyExpanded)buildAdvanced(schedule);
  auto more=dialog.button(initiallyExpanded?"<< Less":"More >>",319,110+footerOffset,50,[]{});
  pendingRetryControl=more;
  more->EnableWindow(job!=nullptr);
  auto toggle=[&,more,previewStatus](bool expanded){
   const auto show=expanded?SW_SHOW:SW_HIDE;
   for(auto w:advanced.controls)w->ShowWindow(show);
   previewStatus->ShowWindow(show);
   more->SetWindowText(expanded?L"<< Less":L"More >>");
   dialog.resizeClient(380,(expanded?295:128)+footerOffset);
  };
  dialog.bind(more,[&,more,buildAdvanced,toggle,schedule,expanded=initiallyExpanded]() mutable{
   if(!job){if(pendingFailed&&pending->requestRetry()){pendingFailed=false;pendingIntent=PendingIntent::None;if(freezeDraft)freezeDraft(true);more->EnableWindow(FALSE);}return;}
   const bool needsControls=!advanced.queue;
   if(needsControls){buildAdvanced(schedule);dialog.refreshTheme();}
   expanded=!expanded;toggle(expanded);
   if(expanded&&dialog.pulse)dialog.pulse();
  });
  if(initiallyExpanded)toggle(true);
  else previewStatus->ShowWindow(SW_HIDE);
  auto scheduleButton=dialog.button("Sche&dule...",7,110+footerOffset,51,[&dialog,schedule]{if(schedule(&dialog))dialog.close();});
  acceptanceControls.push_back(scheduleButton);scheduleButton->EnableWindow(job!=nullptr);
  refreshCategoryActions=[&,editCategory,scheduleButton]{const bool enabled=job&&!m.fileInfoDecisionPending&&!nameTask.valid()&&!categoryWaiting()&&!prefetchTask.valid()&&!refreshTask.valid()&&!decisionTask.valid()&&!finishingDecision&&pendingIntent==PendingIntent::None;editCategory->EnableWindow(enabled);scheduleButton->EnableWindow(enabled&&categoryError.empty());if(advanced.schedule)advanced.schedule->EnableWindow(enabled&&categoryError.empty());};
  laterAction=[&,apply,selectedQueue,schedule,compactSize]{
   if(decisionTask.valid()||finishingDecision||actionTask.valid()||actionCompleting)return;
   if(!job){if(!pendingFailed){pendingIntent=PendingIntent::Later;if(freezeDraft)freezeDraft(true);}return;}
   if(nameTask.valid()||prefetchTask.valid()||refreshTask.valid()||categoryWaiting()){pendingIntent=PendingIntent::Later;if(freezeDraft)freezeDraft(true);return;}
   if(!categoryError.empty()){compactSize->SetWindowText(cs(categoryError));return;}
   if(!refreshError.empty()){compactSize->SetWindowText(cs(refreshError));return;}
   prepareAction(false,[&,apply,selectedQueue,schedule](const FileInfoActionResult& result){
    std::optional<DownloadQueueChoice> choice;
    if(result.promptLater){bool scheduled=false;choice=chooseDownloadQueue(&dialog,m,selectedQueue(),false,[&](CWnd* owner){return scheduled=schedule(owner);},&result.queues);if(scheduled){dialog.close();return;}if(!choice)return;}
    if(decisionTask.valid())return;if(apply(false,choice?&*choice:nullptr,true))dialog.close();
   });
  };
  auto later=dialog.button("Download &Later",64,110+footerOffset,74,laterAction);
  auto start=dialog.button("&Start Download",149,110+footerOffset,74,dialog.accept);dialog.defaultButton(start);dialog.button("&Cancel",234,110+footerOffset,74,dialog.cancel);
  freezeDraft=[&,category,destination,description,pathRule,categoryFolder,browseDestination,start,later](bool frozen){
   CWnd* fields[]={category,destination,description,pathRule,browseDestination,start,later};for(auto control:fields)control->EnableWindow(!frozen);
   categoryFolder->EnableWindow(!frozen&&dialog.checked(pathRule));
  };
  dispatchDecision=[&,compactSize](FileInfoDecisionDraft draft){
   if(decisionTask.valid())return false;
   decisionControls.clear();
   EnumChildWindows(dialog.GetSafeHwnd(),[](HWND window,LPARAM value)->BOOL{wchar_t kind[40]{};GetClassNameW(window,kind,40);if(!_wcsicmp(kind,L"BUTTON")||!_wcsicmp(kind,L"EDIT")||!_wcsicmp(kind,L"COMBOBOX"))reinterpret_cast<std::vector<std::pair<HWND,BOOL>>*>(value)->push_back({window,IsWindowEnabled(window)});return TRUE;},reinterpret_cast<LPARAM>(&decisionControls));
   const auto starting=draft.start;
   try{decisionTask.start(m,job,std::move(draft),dialog.taskNotifier());}
   catch(...){decisionControls.clear();throw;}
   for(const auto& control:decisionControls)::EnableWindow(control.first,FALSE);
   compactSize->SetWindowText(starting?L"Starting download...":L"Adding download to queue...");
   dialog.SetTimer(Form::pulseTimer,40,nullptr);dialog.UpdateWindow();return false;
  };
  dispatchCancellation=[&,compactSize]{
   invalidatePreview();refreshTask.cancel();queuedCategory.reset();categoryTask.cancelRead();nameTask.cancelRead();actionTask.cancelRead();
   decisionControls.clear();
   EnumChildWindows(dialog.GetSafeHwnd(),[](HWND window,LPARAM value)->BOOL{wchar_t kind[40]{};GetClassNameW(window,kind,40);if(!_wcsicmp(kind,L"BUTTON")||!_wcsicmp(kind,L"EDIT")||!_wcsicmp(kind,L"COMBOBOX"))reinterpret_cast<std::vector<std::pair<HWND,BOOL>>*>(value)->push_back({window,IsWindowEnabled(window)});return TRUE;},reinterpret_cast<LPARAM>(&decisionControls));
   try{decisionTask.startAction(m,[&,target=job,removeOffer=cancelNewOffer]{
    actionTask.finish();nameTask.finish();categoryTask.finish();refreshTask.finish();cancelFileInfoPrefetch(m,target,prefetchTask,removeOffer);
    return FileInfoDecisionResult{};
   },dialog.taskNotifier());}catch(...){decisionControls.clear();throw;}
   cancellingDecision=true;pendingIntent=PendingIntent::None;waitingForPrefetchSlot=false;admissionNeedsLookup=false;deferredLookup.reset();
   for(const auto& control:decisionControls)::EnableWindow(control.first,FALSE);
   compactSize->SetWindowText(L"Stopping background download...");dialog.SetTimer(Form::pulseTimer,40,nullptr);dialog.UpdateWindow();
  };
  runPendingIntent=[&]{
   const auto intent=pendingIntent;pendingIntent=PendingIntent::None;
   if(freezeDraft)freezeDraft(false);
   try{if(intent==PendingIntent::Start)dialog.accept();else if(intent==PendingIntent::Later)laterAction();}
   catch(const std::exception& failure){freezeDraft(false);error(&dialog,failure);}
   catch(...){freezeDraft(false);error(&dialog,std::runtime_error("The queued download action failed. Your edits are still available."));}
  };
#ifdef UDM_CAPTURE_PRESENTATION_COMPONENT_TEST
  traceCapturePresentation("file-info-controls-ready");
#endif
#ifdef UDM_PRODUCTION_UI_TIMING_TEST
  nativeUiTraceRecord("file-info-controls-ready");
#endif
 };
 dialog.init=[&]{
  if(initialReady){buildInitialControls();return;}
  const auto loading=fileInfoLoadingView(preparedView,job->id());
  dialog.control(L"STATIC","URL",SS_RIGHT,5,5,54,10);auto url=dialog.edit(loading.address,64,3,244,14,true);url->EnableWindow(FALSE);
  const auto loadingCategory=loading.available?categoryDisplayName(loading.category):std::string{};
  dialog.control(L"STATIC","Category",SS_RIGHT,5,22,54,10);dialog.combo(loading.available?std::vector<std::string>{loadingCategory}:std::vector<std::string>{},loadingCategory,64,22,116)->EnableWindow(FALSE);
  dialog.control(L"STATIC","Save As",SS_RIGHT,5,40,54,10);dialog.combo({},loading.destination,64,38,244,true)->EnableWindow(FALSE);
  dialog.control(L"STATIC",loading.amount,SS_CENTER,311,57,64,10);dialog.control(L"STATIC",loading.kind,SS_CENTER|SS_ENDELLIPSIS,311,69,64,8);
  dialog.control(L"STATIC",loading.detail,SS_ENDELLIPSIS,7,74,300,10);
  dialog.control(L"STATIC","Description",SS_RIGHT,5,92,54,10);dialog.edit(loading.description,64,91,244,14,true)->EnableWindow(FALSE);
  if(!loading.serverName.empty()){dialog.control(L"STATIC","Server name: "+loading.serverName,SS_ENDELLIPSIS,7,112,266,10);dialog.button("Use server name",279,109,94,[]{})->EnableWindow(FALSE);}
  auto status=dialog.label("Checking current download details...",7,128,366,10);
  dialog.button("Sche&dule...",7,142,51,[]{})->EnableWindow(FALSE);dialog.button("Download &Later",64,142,74,[]{})->EnableWindow(FALSE);
  dialog.button("&Start Download",149,142,74,[]{})->EnableWindow(FALSE);
  auto cancel=dialog.button("&Cancel",234,142,74,[]{});auto retry=dialog.button("&Retry",319,142,50,[]{});retry->ShowWindow(SW_HIDE);
  const auto startInitial=[&,status,retry]{
   if(initialTask.valid()||decisionTask.valid())return;retry->ShowWindow(SW_HIDE);status->SetWindowText(L"Checking current download details...");
   try{initialTask.start(m,job,preparedView,dialog.taskNotifier()
#ifdef UDM_TOOLBAR_COMPONENT_TEST
    ,fileInfoInitialTestObserver
#endif
   );}catch(const std::exception& failure){status->SetWindowText(cs(failure.what()));retry->ShowWindow(SW_SHOW);}
  };
  dialog.bind(retry,startInitial);dialog.accept=[]{};
  dialog.cancel=[&,status,cancel,retry]{
   if(decisionTask.valid())return;initialTask.cancel();
   try{decisionTask.startAction(m,[&,target=job,removeOffer=cancelNewOffer]{initialTask.finish();cancelFileInfoPrefetch(m,target,prefetchTask,removeOffer);return FileInfoDecisionResult{};},dialog.taskNotifier());}
   catch(const std::exception& failure){status->SetWindowText(cs(failure.what()));retry->ShowWindow(SW_SHOW);return;}
   cancel->EnableWindow(FALSE);retry->ShowWindow(SW_HIDE);status->SetWindowText(L"Stopping background download...");
  };dialog.bind(cancel,dialog.cancel);
  dialog.pulse=[&,status,cancel,retry]{
   if(decisionTask.valid()){
    if(!decisionTask.ready())return;
    try{decisionTask.get();decisionTask.release();dialog.close(IDCANCEL);}
    catch(const std::exception& failure){decisionTask.release();cancel->EnableWindow(TRUE);retry->ShowWindow(SW_SHOW);status->SetWindowText(cs(failure.what()));}
    return;
   }
   if(!initialTask.ready())return;
   FileInfoInitialResult result;
   try{result=initialTask.take();}
   catch(const std::exception& failure){status->SetWindowText(cs(failure.what()));retry->ShowWindow(SW_SHOW);return;}
   if(result.cancelled||!result.view){status->SetWindowText(L"Download details were not prepared. Retry or cancel.");retry->ShowWindow(SW_SHOW);return;}
   dialog.afterCurrentPulse([&,result=std::move(result)]()mutable{
    try{
     const auto focus=::GetFocus();const bool focused=focus==dialog.GetSafeHwnd()||::IsChild(dialog.GetSafeHwnd(),focus);
     setInitialView(std::move(result.view),std::move(result.address));initial=std::move(result.login);initiallyBasic=result.basic;initialLoginPrepared=true;destinationListName=initialTarget.filename();
     dialog.clearOwnedControls();dialog.resizeClient(380,128+footerOffset);buildInitialControls();dialog.refreshTheme();dialog.SetTimer(Form::pulseTimer,Form::pulseInterval,nullptr);
     if(focused)if(auto first=::GetNextDlgTabItem(dialog.GetSafeHwnd(),nullptr,FALSE))::SetFocus(first);
     dialog.PostMessage(WM_TIMER,initialPrefetchTimer,0);
    }catch(...){initialConstructionFailure=std::current_exception();dialog.close(IDCANCEL);}
   });
  };
  dialog.SetTimer(Form::pulseTimer,40,nullptr);startInitial();
 };
 dialog.afterInit=[&]{
  if(!job)dialog.SetTimer(Form::pulseTimer,40,nullptr);
  // MFC normally waits for an idle modal loop before showing the dialog.
  // Transfer notifications can keep that loop busy throughout prefetch.
  // Paint the completed form before starting any transfer or metadata work.
  if(!showBrowserOwnedDialog(dialog,parent,false)){
   dialog.ShowWindow(SW_SHOW);
   dialog.UpdateWindow();dialog.SetForegroundWindow();
  }
  // Painting is complete. Queue the initial pulse rather than waiting for a
  // low-priority generated timer, which ongoing posted notifications can starve.
  // Earlier queued input still runs first; the recurring pulse remains a fallback.
  if(!dialog.PostMessage(WM_TIMER,initialPrefetchTimer,0))
   dialog.SetTimer(initialPrefetchTimer,USER_TIMER_MINIMUM,nullptr);
 };
 INT_PTR result=IDCANCEL;try{result=dialog.DoModal();}catch(...){stopPreviewWork();decisionTask.release();actionTask.finish();nameTask.finish();categoryTask.finish();refreshTask.finish();initialTask.finish();previewTask.finish();bindingTask.finish();prefetchTask.finish();displayTask.reset();if(!job&&pending)pending->requestCancel();if(job&&initialReady)m.endPrefetch(job);if(fileIcon)DestroyIcon(fileIcon);throw;}stopPreviewWork();decisionTask.release();actionTask.finish();nameTask.finish();categoryTask.finish();refreshTask.finish();initialTask.finish();previewTask.finish();bindingTask.finish();prefetchTask.finish();displayTask.reset();if(job&&initialReady)m.endPrefetch(job);if(fileIcon)DestroyIcon(fileIcon);if(result==-1){if(!job&&pending)pending->requestCancel();throw std::runtime_error("Cannot open Download File Info.");}if(initialConstructionFailure)std::rethrow_exception(initialConstructionFailure);return result==IDOK;
}
