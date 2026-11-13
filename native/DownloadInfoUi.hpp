// Included inside udm after Form and property helpers.
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
inline bool downloadInfo(CWnd* parent,Manager& m,JobPtr job,bool properties=false,bool cancelNewOffer=false,std::shared_ptr<const FileInfoInitialView> preparedView={},std::shared_ptr<const PendingFileInfoInbox::Operation> pending={},bool* pendingBound=nullptr){
#ifdef UDM_CAPTURE_PRESENTATION_COMPONENT_TEST
 traceCapturePresentation("file-info-entry");
#endif
#ifdef UDM_PRODUCTION_UI_TIMING_TEST
 nativeUiTraceRecord("file-info-entry");
#endif
 if(properties){fileProperties(parent,m,job);return true;}
 if(!job&&(!pending||!preparedView||!preparedView->media||preparedView->formRequest))throw std::runtime_error("This prepared download cannot be displayed yet.");
 const auto currentView=job?captureFileInfoInitialView(m,*job):preparedView;
 const auto initialView=preparedView&&sameFileInfoInitialView(*preparedView,*currentView)?std::move(preparedView):currentView;
#ifdef UDM_PRODUCTION_UI_TIMING_TEST
 nativeUiTraceRecord(!job?"file-info-view-pending":initialView==currentView?"file-info-view-current":"file-info-view-prepared");
#endif
 Json data=initialView->data,prefs=initialView->preferences;
 const auto& addressInputs=initialView->addressInputs;
 const auto initialTarget=initialView->target;
 const auto initialSavedSize=initialView->savedSize;
 const bool media=initialView->media,directResponse=initialView->directResponse;
 const bool formRequest=initialView->formRequest,trimResponse=initialView->trimResponse;
 projectHttpOutput(data);
 const auto hasDirectResponseHeaders=[directResponse](const Json& record){
  return directResponse&&hasNonemptyString(record,"ResponseFileNameUrl")&&str(record,"ResponseFileNameUrl")==str(record,"Url");
 };
 const auto replacementNeedsResponse=[directResponse](const Json& record){
  return directResponse&&yes(record,"RefreshPendingValidation")&&Url(str(record,"Url")).scheme!="ftp";
 };
 const auto trackBytes=[media](const Json& record){
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
 const int nameRow=media?0:20;
 const int footerOffset=nameRow+12;
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
 Form dialog("Download File Info",380,128+footerOffset,parent);dialog.dialogUnits=true;HICON fileIcon=nullptr;std::unique_ptr<DownloadPreview> preview;std::string previewVideo,previewAudio,previewAddress;
 const auto extension=lower(utf8(fs::path(wide(str(data,"FileName"))).extension().wstring()));
 std::string extensionType;
 bool waitingForPrefetchSlot=false,draftLoginChanged=false,refreshFailedMedia=false,refreshFailedAddress=false;
 enum class PendingIntent {None,Start,Later};PendingIntent pendingIntent=PendingIntent::None;
 std::function<void(bool)> freezeDraft;std::function<void()> runPendingIntent,laterAction;
 bool pendingIconRequested=false,pendingFailed=false;
 CWnd* pendingRetryControl=nullptr;
 FileInfoDecisionTask decisionTask;
 std::vector<std::pair<HWND,BOOL>> decisionControls;
 std::function<bool(FileInfoDecisionDraft)> dispatchDecision;
 bool finishingDecision=false;
 std::vector<CWnd*> acceptanceControls;
 std::optional<std::chrono::steady_clock::time_point> deferredLookup;
 bool initiallyBasic=false,updatingLogin=false;
 std::pair<std::string,std::string> initial;
 std::function<void()> updateLoginControls;
 const auto loadLogin=[&]{
  const auto headers=media?Headers{}:siteRequestHeaders(str(data,"Url"),readHeaders(data),prefs);
  initial=basicLogin(headers);
  initiallyBasic=lower(headerValue(headers,"Authorization")).rfind("basic ",0)==0;
 };
 struct AdvancedFields{
  std::vector<CWnd*> controls;
  QueueCombo* queue=nullptr;
  CWnd *useLogin=nullptr,*user=nullptr,*password=nullptr,*remember=nullptr,*detail=nullptr,*refreshDetails=nullptr,*page=nullptr;
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
 dialog.init=[&]{
#ifdef UDM_CAPTURE_PRESENTATION_COMPONENT_TEST
  traceCapturePresentation("file-info-controls-start");
#endif
#ifdef UDM_PRODUCTION_UI_TIMING_TEST
  nativeUiTraceRecord("file-info-controls-start");
#endif
  dialog.control(L"STATIC","URL",SS_RIGHT,5,5,54,10);auto address=dialog.edit(downloadAddress(addressInputs),64,3,244,14,true);
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
   Json settings;{Lock lock(m.mutex);settings=m.state["Settings"];}
   const auto name=showCategoryProperties(&dialog,m,settings,"");
   if(!name)return;
   rebuildCategories(*name);
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
  auto updateCategory=[&,selectedCategory,destination,categoryFolder,pathRule,updatePathCaption]{
   Json settings;if(!job)settings=prefs;else{Lock lock(m.mutex);settings=m.state["Settings"];}
   const auto key=selectedCategory();
   const auto folder=udm::categoryFolder(settings,key);
   const auto filename=fs::path(wide(text(destination))).filename();
   destination->SetWindowText(cs(utf8((fs::path(wide(folder))/filename).wstring())));
   categoryFolder->SetWindowText(cs(folder));
   pathRule->SendMessage(BM_SETCHECK,yes(settings.value("CategoryRememberLast",Json::object()),key.c_str())?BST_CHECKED:BST_UNCHECKED);
   categoryFolder->EnableWindow(dialog.checked(pathRule));
   updatePathCaption();
  };
  auto categoryTouched=std::make_shared<bool>(false);
  dialog.bindChange(category,[categoryTouched,updateCategory]{*categoryTouched=true;updateCategory();});
  auto syncPath=[&dialog,&destinationFolders,&destinationListName,categoryFolder,pathRule,destination](const fs::path& path){
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
   if(dialog.checked(pathRule))categoryFolder->SetWindowText(path.parent_path().c_str());
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
  auto suggestedName=[&](const Json& current,bool previewAllowed,const Json& metadata){
   if(media||!str(current,"ReplacementOf").empty())return std::string{};
   if(!draftLoginChanged&&yes(current,"ConfirmationPending")&&str(current,"ResponseFileNameUrl")==str(current,"Url"))return str(current,"ResponseFileName");
   if((draftLoginChanged||previewAllowed)&&str(current,"Url")==previewAddress&&str(metadata,"Status")=="Ready")return str(metadata,"SuggestedFileName");
   return draftLoginChanged||yes(current,"RefreshPendingValidation")?std::string{}:str(current,"BrowserReportedFileName");
  };
  const auto canUseSuggestedName=[destination](const std::string& name){
   return !name.empty()&&safeName(name)==name&&utf8(fs::path(wide(text(destination))).filename().wstring())!=name;
  };
  CWnd* serverName=nullptr;
  CWnd* useServerName=nullptr;
  if(!media){
   const auto name=suggestedName(data,false,Json::object());
   serverName=dialog.control(L"STATIC",name.empty()?"":"Server name: "+name,SS_ENDELLIPSIS,7,112,266,10);
   useServerName=dialog.button("Use server name",279,109,94,[&,selectedCategory,selectCategory,categoryKeys,categoryTouched,destination,categoryFolder,pathRule,updatePathCaption,syncPath,suggestedName,canUseSuggestedName,serverName]{
   fs::path path;std::string nextCategory;Json categorySettings;
   {Lock lock(m.mutex);auto current=job->snapshot();const auto metadata=preview?preview->snapshot():Json::object();
    const bool canCheck=!replacementNeedsResponse(current)&&(!m.isActive(job)||draftLoginChanged)&&canPreviewDownload(*job,draftLoginChanged);
    const auto name=suggestedName(current,canCheck,metadata);
    // The response can replace the suggestion before the next UI refresh.
    if(!canUseSuggestedName(name)||text(serverName)!="Server name: "+name){
     dialog.PostMessage(WM_TIMER,7);
     return;
    }
    auto folder=fs::path(wide(text(destination))).parent_path();
    const auto& settings=m.state["Settings"];const auto priorCategory=selectedCategory();auto host=Url(str(current,"Url")).host;
    if(!*categoryTouched&&!yes(current,"CategoryExplicit")&&priorCategory==downloadCategory(str(current,"FileName"),host,settings)){
     if(!draftLoginChanged&&yes(current,"ConfirmationPending")&&str(current,"ResponseFileNameUrl")==str(current,"Url")){auto resolved=reveal(str(current,"ProtectedResolvedUrl"));if(!resolved.empty())host=Url(resolved).host;}
     else if((draftLoginChanged||!m.isActive(job))&&str(current,"Url")==previewAddress&&str(metadata,"Status")=="Ready"&&!str(metadata,"ResponseHost").empty())host=str(metadata,"ResponseHost");
     auto proposed=downloadCategory(name,host,settings);
     if(proposed!=priorCategory&&std::find(categoryKeys->begin(),categoryKeys->end(),proposed)!=categoryKeys->end()){
      nextCategory=proposed;categorySettings=settings;
      const auto automaticFolder=fs::path(wide(udm::categoryFolder(settings,priorCategory)));
      if(lower(utf8(folder.lexically_normal().wstring()))==lower(utf8(automaticFolder.lexically_normal().wstring())))folder=fs::path(wide(udm::categoryFolder(settings,nextCategory)));
     }
    }
    path=folder/wide(name);
    if(!path.is_absolute())throw std::runtime_error("Choose an absolute destination folder first.");
    if(m.relocationReserves(path))throw std::runtime_error("That destination is reserved by a file move.");if(path!=job->target()&&fs::exists(path))throw std::runtime_error("Destination already exists. Keep your current name or choose another one.");
    if(m.downloadDestinationReserved(path,job))throw std::runtime_error("That destination belongs to another download.");
   }
   if(!nextCategory.empty()){
    selectCategory(nextCategory);updatePathCaption();
    pathRule->SendMessage(BM_SETCHECK,yes(categorySettings.value("CategoryRememberLast",Json::object()),nextCategory.c_str())?BST_CHECKED:BST_UNCHECKED);
    categoryFolder->SetWindowText(cs(utf8(path.parent_path().wstring())));categoryFolder->EnableWindow(dialog.checked(pathRule));
   }
   destination->SetWindowText(path.c_str());syncPath(path);
  });
   useServerName->EnableWindow(canUseSuggestedName(name));
  }
  loadLogin();
  const auto loginChanged=[&advanced,&dialog,media,&initiallyBasic,&initial]{
   return !media&&advanced.useLogin&&(dialog.checked(advanced.useLogin)!=initiallyBasic||
    text(advanced.user)!=initial.first||text(advanced.password)!=initial.second);
  };
  const auto initialQueue=str(data,"Queue","Main queue");
  const auto selectedQueue=[&advanced,&m,initialQueue]{
   if(advanced.queue)return advanced.queue->queueName();
   const auto names=queueNames(m);
   if(std::find(names.begin(),names.end(),initialQueue)!=names.end())return initialQueue;
   return names.empty()?std::string{}:names.front();
  };
  const auto initialDetail=formRequest?"Form download (POST). Restarting submits the form again.":str(data,"FormatDescription");
  const auto draftPreviewRecord=[](Json& record){
   record["Status"]="Awaiting confirmation";record["Received"]=0;record["Segments"]=Json::array();record.erase("ContentType");
  };
  auto refreshLookup=[&,draftPreviewRecord](bool automatic=false){
   if(preview){preview->stop();preview.reset();}
   Json current,settings;
   {
    Lock lock(m.mutex);
    if(m.isActive(job)&&!draftLoginChanged)return;
    const auto& record=job->data;
    // A temporary replacement URL may expire on any request, including HEAD.
    if(replacementNeedsResponse(record))return;
    if(automatic&&(!serverRetryReady(record,epoch())||yes(record,"AuthenticationPromptPending")||yes(record,"BrowserVerificationRequired")||
       num(record,"LastHttpStatus")>=400||num(record,"LastFtpStatus")>=400||
       yes(record,"WebPageReceived")||yes(record,"ContentPrefixPending")))return;
    // These are display hints. The GET still establishes the transfer size.
    if(automatic&&directResponse&&!draftLoginChanged&&reportedDirectResponseSize(record)>0&&
       hasNonemptyString(record,"BrowserReportedFileName"))return;
    current=job->snapshot();settings=m.state["Settings"];
   }
   if(draftLoginChanged){
    // Preview the edited request without changing the running download.
    draftPreviewRecord(current);
    auto h=readHeaders(current);setBasicLogin(h,text(advanced.user),text(advanced.password),dialog.checked(advanced.useLogin));
    current["ProtectedHeaders"]=h.empty()?"":protect(legacyDictionary(Json(h)).dump());
   }
   if(!canPreviewDownload(current))return;
   previewAddress=str(current,"Url");
   previewVideo=current.contains("Video")&&current["Video"].is_object()?str(current["Video"],"Url"):"";previewAudio=current.contains("Audio")&&current["Audio"].is_object()?str(current["Audio"],"Url"):"";
   preview=std::make_unique<DownloadPreview>(std::move(current),std::move(settings),media?240000:10000,draftLoginChanged?nullptr:&m,job);
  };
  auto tryPrefetch=[&]{
   waitingForPrefetchSlot=false;
   const auto admission=m.beginPrefetch(job);
   waitingForPrefetchSlot=admission==PrefetchAdmission::AtCapacity;
   if(admission==PrefetchAdmission::Started&&preview&&str(preview->snapshot(),"Status")!="Ready")preview.reset();
   return admission;
  };
  auto lookupAfterAdmission=[&,refreshLookup](PrefetchAdmission admission,bool hasSize){
   deferredLookup.reset();
   if(media||preview)return;
   if(admission==PrefetchAdmission::Unavailable)refreshLookup(true);
   // A short capacity wait can end before a separate lookup is needed.
   else if(admission==PrefetchAdmission::AtCapacity&&!hasSize)
    deferredLookup=std::chrono::steady_clock::now()+std::chrono::seconds(1);
  };
  auto refreshMedia=[&,tryPrefetch]{
   if(refreshDownloadAddress(&dialog,m,job,true,DownloadRefreshMode::FileInfo)){
    preview.reset();
    tryPrefetch();
    dialog.pulse();
   }
  };
  auto refreshLink=[&,address,tryPrefetch,lookupAfterAdmission]{
   if(!refreshDownloadAddress(&dialog,m,job,true,DownloadRefreshMode::FileInfo))return;
   m.endPrefetch(job);
   preview.reset();
   {
    Lock lock(m.mutex);
    for(const auto* key:{"Url","ProtectedHeaders","DownloadPage"})data[key]=str(job->data,key);
    const auto& settings=m.state["Settings"];
    if(settings.contains("SiteLogins"))prefs["SiteLogins"]=settings.at("SiteLogins");
    else prefs.erase("SiteLogins");
   }
   address->SetWindowText(cs(str(data,"Url")));
   loadLogin();
   if(advanced.useLogin){
    updatingLogin=true;
    advanced.useLogin->SendMessage(BM_SETCHECK,initiallyBasic?BST_CHECKED:BST_UNCHECKED);
    advanced.user->SetWindowText(cs(initial.first));
    advanced.password->SetWindowText(cs(initial.second));
    advanced.remember->SendMessage(BM_SETCHECK,BST_UNCHECKED);
    advanced.page->SetWindowText(cs(recoveryPage(data)));
    updateLoginControls();
    updatingLogin=false;
   }
   draftLoginChanged=false;
   lookupAfterAdmission(tryPrefetch(),false);
   dialog.pulse();
  };
  if(media)dialog.bind(previewButton,[&,refreshLookup,refreshMedia]{
   if(refreshFailedMedia)refreshMedia();else refreshLookup();
  });
  else dialog.bind(previewButton,[&,refreshLookup,tryPrefetch,loginChanged,refreshLink]{
   if(refreshFailedAddress){refreshLink();return;}
   std::string zipName,zipUrl,zipHeaders;
   {
    Lock lock(m.mutex);
    const auto& record=job->data;
    if(!draftLoginChanged&&m.isActive(job)&&yes(record,"ConfirmationPending")&&Url(str(record,"Url")).scheme!="ftp"){
     zipName=str(record,"FileName");
     zipUrl=str(record,"Url");
     zipHeaders=str(record,"ProtectedHeaders");
    }else if(replacementNeedsResponse(record))return;
   }
   if(!zipUrl.empty()){
    // A disengaged result keeps waiting. An empty path ends the preview
    // without issuing a replacement request for this prefetch.
    auto ready=[&,zipUrl,zipHeaders]()->std::optional<fs::path>{
     fs::path savedZip;
     i64 savedSize;
     {
      Lock lock(m.mutex);
      const auto& record=job->data;
      if(!m.isActive(job)||!yes(record,"ConfirmationPending")||str(record,"Url")!=zipUrl||
         str(record,"ProtectedHeaders")!=zipHeaders)return fs::path{};
      const auto receipt=record.find("CompletedPrefetch");
      savedSize=num(record,"Size",-1);
      if(yes(record,"RefreshPendingValidation")||receipt==record.end()||num(*receipt,"Version")!=1||
         str(*receipt,"Url")!=zipUrl||str(*receipt,"Headers")!=zipHeaders||
         savedSize<0||num(*receipt,"Size",-1)!=savedSize||num(record,"Received")!=savedSize)return std::nullopt;
      const auto custom=str(record,"PartsFolder");
      const auto folder=custom.empty()?m.root/L"parts"/wide(job->id()):fs::path(wide(custom));
      savedZip=folder/(wide(job->id())+L".data");
     }
     std::error_code diskError;
     if(fs::file_size(savedZip,diskError)!=static_cast<uintmax_t>(savedSize)||diskError)return fs::path{};
     return savedZip;
    };
    fileInfoZipDialog(&dialog,Json{{"FileName",zipName}},{},{},std::move(ready));
    return;
   }
   auto metadata=preview?preview->snapshot():Json::object();if(preview)preview->stop();bool prefetch;{Lock lock(m.mutex);prefetch=yes(job->data,"ConfirmationPending");}m.endPrefetch(job);
   try{Json current,settings;{Lock lock(m.mutex);current=job->data;settings=m.state["Settings"];}if(str(metadata,"Status")=="Ready")current["ContentType"]=str(metadata,"ContentType");
    if(loginChanged()){auto h=readHeaders(current);setBasicLogin(h,text(advanced.user),text(advanced.password),dialog.checked(advanced.useLogin));current["ProtectedHeaders"]=h.empty()?"":protect(legacyDictionary(Json(h)).dump());}
    fileInfoZipDialog(&dialog,current,settings,draftLoginChanged?std::function<void(const Json&)>{}:browserSessionSaver(m,job,readBrowserSession(current)));
   }catch(...){if(prefetch&&!draftLoginChanged)tryPrefetch();throw;}
   if(prefetch&&!draftLoginChanged)tryPrefetch();
   refreshLookup();
  });
  const bool hasInitialSize=initialResponseSize>=0||initialReportedSize>0;
  dialog.pulse=[&,size,type,compactSize,byteLine,retainedLine,icon,initialDetail,previewStatus,previewButton,serverName,useServerName,suggestedName,canUseSuggestedName,refreshLookup,tryPrefetch,lookupAfterAdmission,hasInitialSize,firstPulse=true,fallbackStarted=false,checkedMediaRefresh=false,fastSizePulse=false,prefetchError=std::string{}]() mutable{
   if(finishingDecision)return;
   if(decisionTask.valid()){
    if(!decisionTask.ready())return;
    finishingDecision=true;
    try{const auto result=decisionTask.get();if(!result.warning.empty())error(&dialog,std::runtime_error(result.warning));dialog.close();}
    catch(const std::exception& failure){for(const auto& control:decisionControls)if(IsWindow(control.first))::EnableWindow(control.first,control.second);decisionControls.clear();dialog.SetTimer(Form::pulseTimer,Form::pulseInterval,nullptr);error(&dialog,failure);}
    catch(...){for(const auto& control:decisionControls)if(IsWindow(control.first))::EnableWindow(control.first,control.second);decisionControls.clear();error(&dialog,std::runtime_error("The download decision failed. Your edits are still available."));}
    decisionTask.release();finishingDecision=false;if(auto frame=AfxGetMainWnd();frame&&frame->GetSafeHwnd())frame->PostMessage(WM_TIMER,1,0);return;
   }
   if(m.fileInfoDecisionPending)return;
   if(!job){
    const auto next=pending->latest();if(next!=pending){pending=next;pendingFailed=false;pendingIntent=PendingIntent::None;}
    dialog.SetTimer(Form::pulseTimer,40,nullptr);
    applyFileDetails(icon,true);if(!pendingIconRequested){pendingIconRequested=true;fileDetails.request(extension);}
    if(!pending->retrying()&&pending->outcome()!=PendingFileInfoInbox::Outcome::Pending&&!pendingFailed){
     JobPtr accepted;{Lock lock(m.mutex);for(const auto& candidate:m.jobs)if(candidate->id()==pending->view->jobId&&str(candidate->data,"Status")=="Awaiting confirmation"&&!yes(candidate->data,"IndividualStart"))accepted=candidate;}
     if(accepted&&pending->attach()){
      job=std::move(accepted);if(pendingBound)*pendingBound=true;
      for(auto control:acceptanceControls)control->EnableWindow(TRUE);
      if(pendingRetryControl)pendingRetryControl->SetWindowText(L"More >>");
      if(freezeDraft)freezeDraft(false);
      dialog.SetTimer(Form::pulseTimer,Form::pulseInterval,nullptr);
#ifdef UDM_PRODUCTION_UI_TIMING_TEST
      nativeUiTraceRecord("pending-file-info-bound");
#endif
     }else pendingFailed=true;
    }
    if(!job){
     const auto message=pending->retrying()?"Retrying download admission...":pendingFailed?(pending->outcome()==PendingFileInfoInbox::Outcome::Accepted?"This download was already accepted. Check download history to continue.":pending->outcome()==PendingFileInfoInbox::Outcome::Rejected?(pending->supportsRetry()?"UDM could not accept this download. Your edits are retained; choose Retry.":"UDM could not accept this download. Your draft is retained; close this dialog to capture again."):"UDM could not confirm acceptance. Your draft is retained; check download history before capturing again."):pendingIntent==PendingIntent::Start?"Starting after the download is accepted...":pendingIntent==PendingIntent::Later?"Adding to the queue after acceptance...":"Preparing the download. You can edit the destination or start now.";
     if(text(compactSize)!=message)compactSize->SetWindowText(cs(message));
     if(pendingFailed&&freezeDraft)freezeDraft(true);
     if(pendingRetryControl){const bool retry=pendingFailed&&pending->outcome()==PendingFileInfoInbox::Outcome::Rejected&&pending->supportsRetry();pendingRetryControl->SetWindowText(retry?L"&Retry":L"More >>");pendingRetryControl->EnableWindow(retry&&pending->canRetry());}
     return;
    }
   }
   const auto attemptPrefetch=[&]{
    try{const auto admission=tryPrefetch();prefetchError.clear();return admission;}
    catch(const std::exception& e){prefetchError=e.what();return PrefetchAdmission::Unavailable;}
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
    const auto admission=!draftLoginChanged&&!preview?attemptPrefetch():PrefetchAdmission::Unavailable;
    lookupAfterAdmission(admission,hasInitialSize);
#ifdef UDM_CAPTURE_PRESENTATION_COMPONENT_TEST
    traceCapturePresentation("first-prefetch-returned");
#endif
#ifdef UDM_PRODUCTION_UI_TIMING_TEST
    nativeUiTraceRecord("first-prefetch-returned");
#endif
   }else if(waitingForPrefetchSlot&&!draftLoginChanged&&dialog.IsWindowEnabled()){
    const auto admission=attemptPrefetch();
    if(deferredLookup&&(admission!=PrefetchAdmission::AtCapacity||std::chrono::steady_clock::now()>=*deferredLookup)){
     deferredLookup.reset();
     if(admission!=PrefetchAdmission::Started&&!preview)refreshLookup(true);
    }
   }
   const bool waitingForSlot=waitingForPrefetchSlot&&!draftLoginChanged;
   applyFileDetails(icon,true);
   // The display needs progress and metadata, not the capture plan, request
   // context, or nested transfer journals. Preview actions take their own snapshot.
   Json current=Json::object();
   bool active,canCheck,replacementPending;
   i64 savedOutputSize,assembledOutputSize;
   {
    Lock lock(m.mutex);
    const auto& source=job->data;
    for(const char* key:{"Url","SourceUrl","Status","ConfirmationPending","Size","HttpContentEncoding","HttpDecodedSize","HttpDecodedReceived","ContentType","MediaPhase","PrefetchedFinalSize",
     "Received","Error","LastHttpStatus","RefreshPendingValidation","BrowserVerificationRequired","SabrSessionRefreshRequired","SabrRetainedBytes","StreamCompletedMs","EstimatedMediaSize","ReportedMediaSize","BrowserReportedDirectSize",
     "ReplacementOf","ResponseFileNameUrl","ResponseFileName","BrowserReportedFileName","ConnectionStatus"}){
     const auto value=source.find(key);
     if(value!=source.end())current[key]=*value;
    }
    if(job->video)current["Video"]={{"Url",str(job->video->data,"Url")},{"Size",num(job->video->data,"Size",-1)}};
    if(job->audio)current["Audio"]={{"Url",str(job->audio->data,"Url")},{"Size",num(job->audio->data,"Size",-1)}};
    active=m.isActive(job);
    replacementPending=replacementNeedsResponse(source);
    canCheck=!replacementPending&&(!active||draftLoginChanged)&&canPreviewDownload(*job,draftLoginChanged);
    savedOutputSize=savedMediaOutputSize(source);
    assembledOutputSize=job->assembledPrefetchSize;
   }
   projectHttpOutput(current);
   const bool pending=yes(current,"ConfirmationPending");
   const bool matchingPrefetch=pending&&active&&!draftLoginChanged;
   const bool savingReceipt=matchingPrefetch&&str(current,"ConnectionStatus")==SavingPrefetchReceipt;
   const bool ftpWaiting=matchingPrefetch&&str(current,"ConnectionStatus")=="Waiting for FTP confirmation";
   const bool prefetchFailed=pending&&!active&&str(current,"Status")=="Failed";
   const bool sessionRejected=yes(current,"SabrSessionRefreshRequired");
   const auto explicitRetryLabel=draftLoginChanged?std::string{}:retryLabel(current);
   if(media&&!active&&(prefetchFailed||sessionRejected)){
    if(!checkedMediaRefresh)refreshFailedMedia=m.canRefreshAddress(job);
    checkedMediaRefresh=true;
   }else checkedMediaRefresh=refreshFailedMedia=false;
   if(prefetchFailed&&!media&&!draftLoginChanged&&!preview&&!fallbackStarted&&num(current,"Size",-1)<0&&canCheck){
    fallbackStarted=true;
    refreshLookup(true);
   }
   auto metadata=preview?preview->snapshot():Json::object();auto phase=str(metadata,"Status");
   if(yes(metadata,"CalculatedMediaSize")&&(!current["Video"].is_object()||!current["Audio"].is_object()||str(current["Video"],"Url")!=previewVideo||str(current["Audio"],"Url")!=previewAudio)){metadata=Json::object();phase="Unavailable";}
   if(!media&&str(current,"Url")!=previewAddress){metadata=Json::object();phase="Unavailable";}
   if(!media){
    auto name=suggestedName(current,canCheck,metadata);
    const auto label=name.empty()?(phase=="Checking"||(active&&yes(current,"ConfirmationPending"))?"Checking server filename...":"No server filename available."):"Server name: "+name;
    if(text(serverName)!=label)serverName->SetWindowText(cs(label));
    const bool canUseName=canUseSuggestedName(name);
    if((useServerName->IsWindowEnabled()!=FALSE)!=canUseName)useServerName->EnableWindow(canUseName);
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
   const bool canRefresh=refreshFailedMedia||(canCheck&&phase!="Checking");
   auto refreshDetails=media?previewButton:advanced.refreshDetails;
   if(refreshDetails&&(refreshDetails->IsWindowEnabled()!=FALSE)!=canRefresh)refreshDetails->EnableWindow(canRefresh);
   refreshFailedAddress=directResponse&&!active&&!draftLoginChanged&&addressRejected(current)&&m.canRefreshAddress(job);
   const auto label=refreshFailedMedia||refreshFailedAddress?"&Refresh...":media?"Check si&ze":"&Preview";
   if(text(previewButton)!=label)previewButton->SetWindowText(cs(label));
   bool showPreview=refreshFailedMedia||canCheck;
   if(!media){
    Lock lock(m.mutex);
    showPreview=refreshFailedAddress||((!replacementPending||matchingPrefetch)&&canPreviewZip(job->data,mime,!draftLoginChanged));
   }
   if(((previewButton->GetStyle()&WS_VISIBLE)!=0)!=showPreview)previewButton->ShowWindow(showPreview?SW_SHOW:SW_HIDE);
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
   if(pendingIntent!=PendingIntent::None&&runPendingIntent)runPendingIntent();
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
   auto queue=dialog.queueCombo(m,queueNames(m),initialQueue,64,175+footerOffset,244);track(queue);
   track(dialog.button("Schedule...",319,175+footerOffset,54,[&dialog,schedule]{if(schedule(&dialog))dialog.close();}));
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
    if(preview){preview->stop();preview.reset();}
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
   if(preview)preview->stop();
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
  auto apply=[&,captureDecision](bool startNow,const DownloadQueueChoice* choice=nullptr,bool startQueue=false){return dispatchDecision(captureDecision(startNow,choice,nullptr,startQueue));};
  auto schedule=[&m,&dialog,&job,destination,description,selectedCategory,pathRule,captureDecision](CWnd* owner){
   ScheduledDownloadDraft initial{fs::path(wide(text(destination))),text(description),selectedCategory(),dialog.checked(pathRule),{}};
   {
    Lock lock(m.mutex);
    initial.connections=num(job->data,"Connections",8);
    initial.speedLimit=num(job->data,"LimitKbps");
   }
   return scheduleDownloadWizard(owner,m,std::move(initial),[&m,&job,captureDecision](const ScheduledDownloadDraft& draft){auto command=captureDecision(false,&draft.queue,&draft);return [source=&m,target=job,command=std::move(command)]{return applyFileInfoDecision(*source,target,command);};});
  };
  dialog.accept=[&,apply,refreshMedia]{
   if(decisionTask.valid()||finishingDecision)return;
   if(!job){if(!pendingFailed){pendingIntent=PendingIntent::Start;if(freezeDraft)freezeDraft(true);}return;}
   bool sessionRejected;{Lock lock(m.mutex);sessionRejected=yes(job->data,"SabrSessionRefreshRequired");}
   if(sessionRejected){refreshMedia();return;}
   if(decisionTask.valid())return;
   if(apply(true))dialog.close();
  };
  dialog.cancel=[&]{
   if(decisionTask.valid()||finishingDecision)return;
   if(!job){pending->requestCancel();dialog.close(IDCANCEL);return;}
   if(preview)preview->stop();
   m.endPrefetch(job,!cancelNewOffer);
   {
    Lock lock(m.mutex);
    if(cancelNewOffer&&!m.isActive(job)&&!job->data.contains("OriginalResponse")&&
       !yes(job->data,"IndividualStart")){
     const auto status=str(job->data,"Status");
     if(status=="Awaiting confirmation"||status=="Paused"||status=="Failed")m.remove(job,false,true);
     else m.pause(job);
    }else m.pause(job);
   }
   dialog.close(IDCANCEL);
  };
  const bool initiallyExpanded=initiallyBasic||formRequest;
#ifdef UDM_CAPTURE_PRESENTATION_COMPONENT_TEST
  traceCapturePresentation("file-info-actions-start");
#endif
  if(initiallyExpanded)buildAdvanced(schedule);
  auto more=dialog.button(initiallyExpanded?"<< Less":"More >>",319,110+footerOffset,50,[]{});
  pendingRetryControl=more;
  acceptanceControls.push_back(more);more->EnableWindow(job!=nullptr);
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
  laterAction=[&,apply,selectedQueue,schedule]{
   if(decisionTask.valid()||finishingDecision)return;
   if(!job){if(!pendingFailed){pendingIntent=PendingIntent::Later;if(freezeDraft)freezeDraft(true);}return;}
   bool ask;{Lock lock(m.mutex);ask=yes(m.state["Settings"],"QueuePromptLater",true);}std::optional<DownloadQueueChoice> choice;
   if(ask){
    bool scheduled=false;
    choice=chooseDownloadQueue(&dialog,m,selectedQueue(),false,[&](CWnd* owner){return scheduled=schedule(owner);});
    if(scheduled){dialog.close();return;}
    if(!choice)return;
   }
   if(decisionTask.valid())return;
   if(apply(false,choice?&*choice:nullptr,true))dialog.close();
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
  runPendingIntent=[&]{
   const auto intent=pendingIntent;pendingIntent=PendingIntent::None;
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
 INT_PTR result=IDCANCEL;try{result=dialog.DoModal();}catch(...){if(!job&&pending)pending->requestCancel();if(job)m.endPrefetch(job);if(fileIcon)DestroyIcon(fileIcon);throw;}if(job)m.endPrefetch(job);if(fileIcon)DestroyIcon(fileIcon);if(result==-1){if(!job&&pending)pending->requestCancel();throw std::runtime_error("Cannot open Download File Info.");}return result==IDOK;
}
