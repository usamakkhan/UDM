// Included inside udm after Form and property helpers.
inline bool downloadInfo(CWnd* parent,Manager& m,JobPtr job,bool properties=false,bool cancelNewOffer=false){
 if(properties){fileProperties(parent,m,job);return true;}
 Json data,prefs;{Lock lock(m.mutex);data=job->snapshot();prefs=m.state["Settings"];}const bool media=capturedMedia(data);
 const bool directResponse=!media&&!data.contains("OfflineProject");
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
 Form dialog("Download File Info",380,128+nameRow,parent);dialog.dialogUnits=true;HICON fileIcon=nullptr;std::unique_ptr<DownloadPreview> preview;std::string previewVideo,previewAudio,previewAddress;
 const auto extension=lower(utf8(fs::path(wide(str(data,"FileName"))).extension().wstring()));
 std::string extensionType;
 bool waitingForPrefetchSlot=false,draftLoginChanged=false;
 // Share one bounded Shell worker across confirmations, including when a handler stalls.
 static AsyncFileIcons fileDetails(true);
 dialog.init=[&]{
  dialog.control(L"STATIC","URL",SS_RIGHT,5,5,54,10);dialog.edit(downloadAddress(data),64,3,244,14,true);
  dialog.control(L"STATIC","Category",SS_RIGHT,5,22,54,10);
  auto category=dialog.combo({},"",64,22,116);
  auto categoryKeys=std::make_shared<std::vector<std::string>>();
  auto selectCategory=[category,categoryKeys](const std::string& key){
   const auto item=std::find(categoryKeys->begin(),categoryKeys->end(),key);
   category->SetCurSel(item==categoryKeys->end()?0:static_cast<int>(item-categoryKeys->begin()));
  };
  auto rebuildCategories=[&m,category,categoryKeys,selectCategory](const std::string& key){
   *categoryKeys=m.categories();
   const auto general=std::find(categoryKeys->begin(),categoryKeys->end(),"Other");
   if(general!=categoryKeys->end())std::rotate(categoryKeys->begin(),general,general+1);
   category->ResetContent();
   for(const auto& name:*categoryKeys){
    auto label=categoryDisplayName(name);
    while(category->FindStringExact(-1,cs(label))!=CB_ERR)label+=" (custom)";
    if(category->AddString(cs(label))<0)throw std::runtime_error("Cannot add a category to the selector.");
   }
   selectCategory(key);
  };
  auto selectedCategory=[category,categoryKeys]{
   const auto item=category->GetCurSel();
   if(item==CB_ERR)throw std::runtime_error("Choose an existing category.");
   return categoryKeys->at(static_cast<size_t>(item));
  };
  rebuildCategories(str(data,"Category"));
  dialog.button("+",183,21,16,[&,category,rebuildCategories]{
   Json settings;{Lock lock(m.mutex);settings=m.state["Settings"];}
   const auto name=showCategoryProperties(&dialog,m,settings,"");
   if(!name)return;
   rebuildCategories(*name);
   dialog.SendMessage(WM_COMMAND,MAKEWPARAM(category->GetDlgCtrlID(),CBN_SELCHANGE),(LPARAM)category->GetSafeHwnd());
  });
  auto icon=dialog.control(L"STATIC","",SS_ICON,333,28,20,20);
  const auto initialSavedSize=savedMediaOutputSize(data);
  const auto initialResponseSize=directResponse&&!yes(data,"RefreshPendingValidation")?num(data,"Size",-1):-1;
  const auto initialExactSize=initialSavedSize>=0?initialSavedSize:initialResponseSize;
  const auto initialTrackSize=trackBytes(data);
  const auto initialReportedSize=directResponse&&initialExactSize<0?reportedDirectResponseSize(data):0;
  auto size=dialog.control(L"STATIC",initialExactSize>=0?std::to_string(initialExactSize)+" bytes":initialTrackSize>0?bytes(initialTrackSize):initialReportedSize>0?std::to_string(initialReportedSize)+" bytes":downloadSizeLabel(data),SS_CENTER,311,57,64,10);
  const bool initialEstimate=initialExactSize<0&&initialTrackSize==0&&num(data,"Size",-1)<0&&mediaSizeHint(data)>0;
  auto type=dialog.control(L"STATIC",initialSavedSize>=0?"Saved output size":initialTrackSize>0?"Selected track size":initialReportedSize>0?"Reported size":initialEstimate?"Estimated size":str(data,"ContentType"),SS_CENTER|SS_ENDELLIPSIS,311,69,64,8);
  auto previewButton=dialog.button(media?"Check si&ze":"&Preview",319,78,50,[]{});
  previewButton->ShowWindow(SW_HIDE);
  auto previewStatus=dialog.control(L"STATIC","",SS_ENDELLIPSIS,7,132+nameRow,366,9);
  dialog.control(L"STATIC","Save As",SS_RIGHT,5,41,54,10);
  std::vector<std::string> destinations={utf8(job->target().wstring())};if(prefs.contains("SaveFolderHistory")&&prefs["SaveFolderHistory"].is_array())for(auto& value:prefs["SaveFolderHistory"]){if(!value.is_string()||destinations.size()>=21)continue;fs::path folder(wide(value.get<std::string>()));if(folder.is_absolute())destinations.push_back(utf8((folder/wide(str(data,"FileName"))).wstring()));}
  auto destination=dialog.combo(destinations,destinations.front(),64,39,223,true);

  dialog.control(L"BUTTON","",BS_GROUPBOX,64,56,244,32);
  auto pathCaption=[category]{
   std::string label;
   for(char ch:text(category)){if(ch=='&')label+='&';label+=ch;}
   return "Remember this path for \""+label+"\" category";
  };
  auto pathRule=dialog.check(pathCaption(),yes(prefs.value("CategoryRememberLast",Json::object()),selectedCategory().c_str()),69,55,232);
  auto updatePathCaption=[pathRule,pathCaption]{pathRule->SetWindowText(cs(pathCaption()));};
  auto categoryFolder=dialog.edit(utf8(job->target().parent_path().wstring()),69,69,232);categoryFolder->EnableWindow(dialog.checked(pathRule));dialog.bind(pathRule,[&dialog,pathRule,categoryFolder,destination]{categoryFolder->EnableWindow(dialog.checked(pathRule));if(dialog.checked(pathRule))categoryFolder->SetWindowText(cs(utf8(fs::path(wide(text(destination))).parent_path().wstring())));});
  auto updateCategory=[&,selectedCategory,destination,categoryFolder,pathRule,updatePathCaption]{
   Json settings;{Lock lock(m.mutex);settings=m.state["Settings"];}
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
  auto syncPath=[&dialog,destination,categoryFolder,pathRule]{if(dialog.checked(pathRule))categoryFolder->SetWindowText(cs(utf8(fs::path(wide(text(destination))).parent_path().wstring())));};dialog.bindChange(destination,syncPath);
  dialog.button("...",292,39,16,[&dialog,destination,syncPath]{auto folder=chooseFolder(&dialog,fs::path(wide(text(destination))).parent_path().wstring());if(!folder.empty())destination->SetWindowText(cs(utf8((fs::path(folder)/fs::path(wide(text(destination))).filename()).wstring())));syncPath();});
  dialog.control(L"STATIC","Description",SS_RIGHT,5,93,54,10);auto description=dialog.edit(str(data,"Description"),64,91,244);
  auto suggestedName=[&](const Json& current,bool active){
   if(media||!str(current,"ReplacementOf").empty())return std::string{};
   if(!draftLoginChanged&&yes(current,"ConfirmationPending")&&str(current,"ResponseFileNameUrl")==str(current,"Url"))return str(current,"ResponseFileName");
   auto metadata=preview?preview->snapshot():Json::object();
   if((draftLoginChanged||(!active&&canPreviewDownload(current)))&&str(current,"Url")==previewAddress&&str(metadata,"Status")=="Ready")return str(metadata,"SuggestedFileName");
   return draftLoginChanged||yes(current,"RefreshPendingValidation")?std::string{}:str(current,"BrowserReportedFileName");
  };
  auto serverName=dialog.control(L"STATIC","",SS_ENDELLIPSIS,7,112,266,10);
  auto useServerName=dialog.button("Use server name",279,109,94,[&,selectedCategory,selectCategory,categoryKeys,categoryTouched,destination,categoryFolder,pathRule,updatePathCaption,syncPath,suggestedName]{
   fs::path path;std::string nextCategory;Json categorySettings;
   {Lock lock(m.mutex);auto current=job->snapshot();auto name=suggestedName(current,m.isActive(job));
    if(name.empty()||safeName(name)!=name)return;
    auto folder=fs::path(wide(text(destination))).parent_path();
    const auto& settings=m.state["Settings"];const auto priorCategory=selectedCategory();auto host=Url(str(current,"Url")).host;
    if(!*categoryTouched&&!yes(current,"CategoryExplicit")&&priorCategory==downloadCategory(str(current,"FileName"),host,settings)){
     auto metadata=preview?preview->snapshot():Json::object();
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
    for(auto other:m.jobs)if(other!=job&&(lower(utf8(other->target().wstring()))==lower(utf8(path.wstring()))||lower(str(other->data,"PreviousPath"))==lower(utf8(path.wstring()))))throw std::runtime_error("That destination belongs to another download.");
   }
   if(!nextCategory.empty()){
    selectCategory(nextCategory);updatePathCaption();
    pathRule->SendMessage(BM_SETCHECK,yes(categorySettings.value("CategoryRememberLast",Json::object()),nextCategory.c_str())?BST_CHECKED:BST_UNCHECKED);
    categoryFolder->SetWindowText(cs(utf8(path.parent_path().wstring())));categoryFolder->EnableWindow(dialog.checked(pathRule));
   }
   destination->SetWindowText(cs(utf8(path.wstring())));syncPath();
  });
  serverName->ShowWindow(media?SW_HIDE:SW_SHOW);useServerName->ShowWindow(media?SW_HIDE:SW_SHOW);useServerName->EnableWindow(FALSE);
  auto advanced=std::make_shared<std::vector<CWnd*>>();
  auto track=[advanced](CWnd* w){advanced->push_back(w);return w;};track(previewStatus);
  track(dialog.button("Links...",319,91,50,[&]{downloadLinkDetails(&dialog,m,job);}));
  track(dialog.control(L"STATIC","",SS_ETCHEDHORZ,7,145+nameRow,366,1));
  track(dialog.label("Web page",7,157+nameRow,51));auto page=track(dialog.edit(recoveryPage(data),64,154+nameRow,244,14,true));
  auto open=track(dialog.button("Open page",319,154+nameRow,50,[&dialog,page]{Url u(text(page));if(u.scheme!="http"&&u.scheme!="https")throw std::runtime_error("No HTTP or HTTPS parent page is available.");openFile(&dialog,fs::path(wide(u.full)));}));open->EnableWindow(!text(page).empty());
  track(dialog.label("Queue",7,179+nameRow,51));auto queue=dialog.queueCombo(m,queueNames(m),str(data,"Queue","Main queue"),64,175+nameRow,244);track(queue);
  auto headers=siteRequestHeaders(str(data,"Url"),readHeaders(data),prefs);auto initial=basicLogin(headers);bool initiallyBasic=lower(headerValue(headers,"Authorization")).rfind("basic ",0)==0;
  auto useLogin=track(dialog.check("Use login and password",initiallyBasic,7,198+nameRow,266));useLogin->EnableWindow(!media);
  track(dialog.label("Login",7,219+nameRow,51));auto user=track(dialog.edit(initial.first,64,216+nameRow,112));
  track(dialog.label("Password",187,219+nameRow,56));auto password=track(dialog.edit(initial.second,247,216+nameRow,126,14,false,false,true));
  auto remember=track(dialog.check("Remember for this site",false,64,238+nameRow,180));auto show=track(dialog.check("Show password",false,247,238+nameRow,126));
  auto enabled=[&dialog,media,useLogin,user,password,remember,show,data]{bool on=!media&&dialog.checked(useLogin);user->EnableWindow(on);password->EnableWindow(on);show->EnableWindow(on);remember->EnableWindow(on&&siteLoginScheme(Url(str(data,"Url")).scheme));};
  auto loginEdited=[&,useLogin,user,password,initial,initiallyBasic]{
   draftLoginChanged=!media&&(dialog.checked(useLogin)!=initiallyBasic||text(user)!=initial.first||text(password)!=initial.second);
   if(preview){preview->stop();preview.reset();}
   if(dialog.pulse)dialog.pulse();
  };
  dialog.bind(useLogin,[enabled,loginEdited]{enabled();loginEdited();});enabled();
  dialog.bindChange(user,loginEdited);dialog.bindChange(password,loginEdited);
  dialog.bind(show,[&dialog,show,password]{password->SendMessage(EM_SETPASSWORDCHAR,dialog.checked(show)?0:0x25cf);password->Invalidate();});
  auto detail=track(dialog.label((!str(data,"ProtectedRequest").empty()?"Form download (POST). Restarting submits the form again.":str(data,"FormatDescription")),7,259+nameRow,260,28));
  auto refreshDetails=media?previewButton:track(dialog.button("Refresh details",279,259+nameRow,94,[]{}));
  const auto draftPreviewRecord=[](Json& record){
   record["Status"]="Awaiting confirmation";record["Received"]=0;record["Segments"]=Json::array();record.erase("ContentType");
  };
  auto refreshLookup=[&,useLogin,user,password,draftPreviewRecord]{
   if(preview){preview->stop();preview.reset();}
   Json current,settings;{Lock lock(m.mutex);if(m.isActive(job)&&!draftLoginChanged)return;current=job->snapshot();settings=m.state["Settings"];}
   if(draftLoginChanged){
    // Preview the edited request without changing the running download.
    draftPreviewRecord(current);
    auto h=readHeaders(current);setBasicLogin(h,text(user),text(password),dialog.checked(useLogin));
    current["ProtectedHeaders"]=h.empty()?"":protect(legacyDictionary(Json(h)).dump());
   }
   if(!canPreviewDownload(current))return;
   previewAddress=str(current,"Url");
   previewVideo=current.contains("Video")&&current["Video"].is_object()?str(current["Video"],"Url"):"";previewAudio=current.contains("Audio")&&current["Audio"].is_object()?str(current["Audio"],"Url"):"";
   preview=std::make_unique<DownloadPreview>(std::move(current),std::move(settings),media?240000:10000,draftLoginChanged?nullptr:&m,job);
  };
  dialog.bind(refreshDetails,refreshLookup);
  if(!media)dialog.bind(previewButton,[&,useLogin,user,password,initial,initiallyBasic,refreshLookup]{
   auto metadata=preview?preview->snapshot():Json::object();if(preview)preview->stop();bool prefetch;{Lock lock(m.mutex);prefetch=yes(job->data,"ConfirmationPending");}m.endPrefetch(job);
   try{Json current,settings;{Lock lock(m.mutex);current=job->data;settings=m.state["Settings"];}if(str(metadata,"Status")=="Ready")current["ContentType"]=str(metadata,"ContentType");
    if(dialog.checked(useLogin)!=initiallyBasic||text(user)!=initial.first||text(password)!=initial.second){auto h=readHeaders(current);setBasicLogin(h,text(user),text(password),dialog.checked(useLogin));current["ProtectedHeaders"]=h.empty()?"":protect(legacyDictionary(Json(h)).dump());}
    remoteZipDialog(&dialog,current,settings,draftLoginChanged?std::function<void(const Json&)>{}:browserSessionSaver(m,job,readBrowserSession(current)));
   }catch(...){if(prefetch&&!draftLoginChanged)m.beginPrefetch(job);throw;}
   if(prefetch&&!draftLoginChanged)m.beginPrefetch(job);
   refreshLookup();
  });
  dialog.pulse=[&,size,type,icon,detail,previewStatus,refreshDetails,previewButton,serverName,useServerName,destination,suggestedName,refreshLookup,draftPreviewRecord,fallbackStarted=false]() mutable{
   if(waitingForPrefetchSlot){
    waitingForPrefetchSlot=false;
    waitingForPrefetchSlot=m.beginPrefetch(job)==PrefetchAdmission::AtCapacity;
   }
   for(auto& result:fileDetails.take())if(result.extension==extension){
    extensionType=std::move(result.typeName);
    if(result.icon){icon->SendMessage(STM_SETICON,(WPARAM)result.icon);std::swap(fileIcon,result.icon);}
   }
   Json current;bool active;{Lock lock(m.mutex);current=job->snapshot();active=m.isActive(job);}
   const bool pending=yes(current,"ConfirmationPending");
   const bool prefetchFailed=pending&&!active&&str(current,"Status")=="Failed";
   if(prefetchFailed&&!media&&!draftLoginChanged&&!preview&&!fallbackStarted&&num(current,"Size",-1)<0&&canPreviewDownload(current)){
    fallbackStarted=true;
    refreshLookup();
   }
   auto metadata=preview?preview->snapshot():Json::object();auto phase=str(metadata,"Status");
   if(yes(metadata,"CalculatedMediaSize")&&(!current["Video"].is_object()||!current["Audio"].is_object()||str(current["Video"],"Url")!=previewVideo||str(current["Audio"],"Url")!=previewAudio)){metadata=Json::object();phase="Unavailable";}
   if(!media&&str(current,"Url")!=previewAddress){metadata=Json::object();phase="Unavailable";}
   if(!media){
    auto name=suggestedName(current,active);
    serverName->SetWindowText(cs(name.empty()?(phase=="Checking"||(active&&yes(current,"ConfirmationPending"))?"Checking server filename...":"No server filename available."):"Server name: "+name));
    useServerName->EnableWindow(!name.empty()&&safeName(name)==name&&utf8(fs::path(wide(text(destination))).filename().wstring())!=name);
   }
   i64 length=draftLoginChanged?-1:num(current,"Size",-1);auto mime=draftLoginChanged?std::string{}:str(current,"ContentType");std::string status;
   const bool showingPreview=!active||draftLoginChanged;
   if(showingPreview&&phase=="Ready"){
    const auto checkedSize=num(metadata,"Size",-1);
    if(checkedSize>=0||!directResponse)length=checkedSize;
    mime=str(metadata,"ContentType");
    status=yes(metadata,"CalculatedMediaSize")?"Calculated final MP4: "+std::to_string(length)+" bytes from complete sample metadata and a codec probe.":checkedSize<0?"The metadata check did not return a size.":"File details checked. No file data has been saved.";
   }
   else if(showingPreview&&phase=="Checking")status=media?"Calculating final size from media headers. You can start now; calculation may take several minutes.":"Checking file details...";
   else if(showingPreview&&phase=="Error")status=str(metadata,"Message");
   else if(draftLoginChanged)status="Login changed. Refresh details to check this request.";
   const auto exactPrefetch=active&&yes(current,"ConfirmationPending")&&str(current,"MediaPhase")=="Waiting for confirmation"?num(current,"PrefetchedFinalSize",-1):-1;
   if(exactPrefetch>=0){length=exactPrefetch;status="Exact final file: "+std::to_string(length)+" bytes. Prefetched and verified; waiting for confirmation.";}
   const bool exactMediaSize=exactPrefetch>=0||(!active&&phase=="Ready"&&yes(metadata,"CalculatedMediaSize")&&length>=0);
   const auto savedSize=exactMediaSize?-1:savedMediaOutputSize(current);
   if(savedSize>=0){
    length=savedSize;
    status="Saved output: "+std::to_string(length)+" bytes. UDM verifies the file before saving it to your destination.";
   }
   const bool preparingMedia=media&&active&&pending&&exactPrefetch<0&&savedSize<0;
   if(preparingMedia)status="Preparing exact final size by downloading and assembling the selected streams in the background. You can start now.";
   const auto trackSize=!exactMediaSize&&savedSize<0?trackBytes(current):0;
   const auto reportedSize=directResponse&&!draftLoginChanged&&length<0?reportedDirectResponseSize(current):0;
   const bool estimated=length<0&&!trackSize&&mediaSizeHint(current)>0;
   auto sizeText=trackSize>0?bytes(trackSize):length>=0?bytes(length):draftLoginChanged?bytes(-1):downloadSizeLabel(current);
   const bool exactResponseSize=directResponse&&length>=0&&!yes(current,"RefreshPendingValidation");
   if(exactMediaSize||savedSize>=0||exactResponseSize)sizeText=std::to_string(length)+" bytes";
   else if(reportedSize>0)sizeText=std::to_string(reportedSize)+" bytes";
   else if(length<0&&!trackSize&&!estimated&&waitingForPrefetchSlot)sizeText="Waiting...";
   else if(length<0&&!trackSize&&!estimated&&preparingMedia)sizeText="Preparing...";
   else if(length<0&&phase=="Checking"&&!trackSize&&!estimated)sizeText="Checking...";
   size->SetWindowText(cs(sizeText));
   if(trackSize>0)status="Selected tracks: "+std::to_string(trackSize)+" bytes. Final file size may differ."+(phase=="Checking"?" Calculating final size...":"");
   else if(reportedSize>0&&status.empty())status="Size reported by the browser. The download response supplies the final size.";
   else if(estimated)status="Approximate size from the selected media tracks. The final file size may differ after combining them.";
   if(waitingForPrefetchSlot)status="Waiting for a transfer slot to prepare the final file size.";
   previewStatus->SetWindowText(cs(status));
   const auto sizeCaption=exactMediaSize?"Exact final size":savedSize>=0?"Saved output size":trackSize>0?"Selected track size":reportedSize>0?"Reported size":estimated?"Estimated size":mime.empty()?extensionType:mime;
   type->SetWindowText(cs(sizeCaption));
   if(draftLoginChanged)draftPreviewRecord(current);
   const bool canCheck=showingPreview&&canPreviewDownload(current);
   refreshDetails->EnableWindow(canCheck&&phase!="Checking");
   previewButton->ShowWindow((media?canCheck:canPreviewZip(current,mime))?SW_SHOW:SW_HIDE);
   if(exactPrefetch>=0)detail->SetWindowText(cs(status));
   else if(prefetchFailed)detail->SetWindowText(cs("Background download stopped: "+str(current,"Error")));
   else if(active&&pending&&!draftLoginChanged)detail->SetWindowText(cs("Downloaded: "+bytes(num(current,"Received"))+". Waiting for your confirmation."));
   else if(!status.empty())detail->SetWindowText(cs(status));
  };
  auto apply=[&,selectedCategory,pathRule,categoryFolder,destination,description,queue,useLogin,user,password,remember,initiallyBasic,initial](bool startNow,const DownloadQueueChoice* choice=nullptr){
   FileInfoFolderChoice folderChoice;
   if(dialog.checked(pathRule)){
    fs::path remembered(wide(trim(text(categoryFolder))));
    if(!remembered.is_absolute())throw std::runtime_error("Choose an absolute category folder.");
    folderChoice.rememberedCategoryFolder=std::move(remembered);
   }
   if(preview)preview->stop();auto path=fs::path(wide(text(destination)));
   if(!media&&dialog.checked(useLogin)){auto validation=readHeaders(data);setBasicLogin(validation,text(user),text(password));}
   const auto categoryName=selectedCategory();
   Json edit={{"Folder",utf8(path.parent_path().wstring())},{"FileName",utf8(path.filename().wstring())},{"Category",categoryName},{"Description",text(description)},{"Queue",choice?choice->name:queue->queueName()}};
   const bool loginUnchanged=dialog.checked(useLogin)==initiallyBasic&&text(user)==initial.first&&text(password)==initial.second;
   Json rememberedLogin;
   if(!media&&loginUnchanged&&dialog.checked(useLogin)&&dialog.checked(remember))
    rememberedLogin=makeSiteLogin(Url(str(data,"Url")).origin,initial.first,initial.second);
   if(startNow&&(media||loginUnchanged)&&m.confirmPrefetchedDownload(job,edit,&folderChoice,rememberedLogin))return;
   if(!startNow&&(media||loginUnchanged)&&m.deferPrefetchedDownload(job,edit,folderChoice,rememberedLogin,choice))return;
   std::optional<FileInfoLogin> login;
   if(!media&&(!loginUnchanged||dialog.checked(remember))){
    const bool enabled=dialog.checked(useLogin);
    login=FileInfoLogin{text(user),text(password),enabled&&dialog.checked(remember),enabled};
   }
   m.endPrefetch(job);
   m.finishStoppedFileInfo(job,edit,folderChoice,login,!startNow,choice);
  };
  dialog.accept=[&,apply]{apply(true);dialog.close();};
  auto dismiss=[&](bool rejectOffer){
   if(preview)preview->stop();
   m.endPrefetch(job);
   {
    Lock lock(m.mutex);
    if(rejectOffer&&cancelNewOffer&&!m.isActive(job)&&!job->data.contains("OriginalResponse")&&
       !yes(job->data,"IndividualStart")){
     const auto status=str(job->data,"Status");
     if(status=="Awaiting confirmation"||status=="Paused"||status=="Failed")m.remove(job,false,true);
     else m.pause(job);
    }else m.pause(job);
   }
   dialog.close(IDCANCEL);
  };
  dialog.cancel=[dismiss]{dismiss(false);};
  auto expanded=std::make_shared<bool>(initiallyBasic||!str(data,"ProtectedRequest").empty());auto more=dialog.button("More >>",319,110+nameRow,50,[]{});
  auto toggle=[&dialog,advanced,expanded,more,nameRow]{for(auto w:*advanced)w->ShowWindow(*expanded?SW_SHOW:SW_HIDE);more->SetWindowText(*expanded?L"<< Less":L"More >>");dialog.resizeClient(380,(*expanded?295:128)+nameRow);};
  dialog.bind(more,[expanded,toggle]{*expanded=!*expanded;toggle();});toggle();
  dialog.button("Download &Later",64,110+nameRow,74,[&,apply,queue]{
   bool ask;{Lock lock(m.mutex);ask=yes(m.state["Settings"],"QueuePromptLater",true);}std::optional<DownloadQueueChoice> choice;
   if(ask){choice=chooseDownloadQueue(&dialog,m,queue->queueName());if(!choice)return;}
   apply(false,choice?&*choice:nullptr);
   if(choice&&choice->start)try{m.startQueueForDownloads(choice->name,{job});}
   catch(const std::exception& e){error(&dialog,std::runtime_error("The download was added to the queue, but queue processing could not start. "+std::string(e.what())));}
   dialog.close();
  });
  dialog.defaultButton(dialog.button("&Start Download",149,110+nameRow,74,dialog.accept));dialog.button("&Cancel",234,110+nameRow,74,[dismiss]{dismiss(true);});
  // MFC normally waits for an idle modal loop before showing the dialog.
  // Transfer notifications can keep that loop busy throughout prefetch.
  // Paint the completed form before starting any transfer or metadata work.
  if(!showBrowserOwnedDialog(dialog,parent)){
   dialog.refreshTheme();
   dialog.ShowWindow(SW_SHOW);
   dialog.UpdateWindow();dialog.SetForegroundWindow();
  }
  fileDetails.request(extension);
  const auto prefetch=m.beginPrefetch(job);
  waitingForPrefetchSlot=media&&prefetch==PrefetchAdmission::AtCapacity;
  if(!media)refreshLookup();dialog.pulse();
 };
 INT_PTR result=IDCANCEL;try{result=dialog.DoModal();}catch(...){m.endPrefetch(job);if(fileIcon)DestroyIcon(fileIcon);throw;}m.endPrefetch(job);if(fileIcon)DestroyIcon(fileIcon);if(result==-1)throw std::runtime_error("Cannot open Download File Info.");return result==IDOK;
}
