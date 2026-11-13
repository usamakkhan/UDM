// Included inside udm after Form and Move/Rename helpers.
inline std::string displayDate(const Json& value){auto millis=parseDate(value);if(!millis)return "Never";auto seconds=(time_t)(millis/1000);tm local{};localtime_s(&local,&seconds);char buffer[64]{};strftime(buffer,sizeof(buffer),"%b %d %Y %H:%M:%S",&local);return buffer;}
inline bool hasAudioMetadata(const Json& draft){auto found=draft.find("AudioMetadata");return found!=draft.end()&&found->is_object()&&(!str(*found,"Title").empty()||!str(*found,"Artist").empty()||!str(*found,"Album").empty());}
inline void audioMetadataProperties(CWnd* owner,const Json& draft){
 const auto metadata=draft.value("AudioMetadata",Json::object());Form d("Audio metadata",480,176,owner);
 d.init=[&]{
  const char* names[]={"Title","Artist","Album"};
  for(int row=0;row<3;++row){d.label(names[row],14,18+row*36,70);d.edit(str(metadata,names[row]),88,14+row*36,378,23,true);}
  d.accept=[&]{d.close();};d.defaultButton(d.button("Close",378,134,88,d.accept));
 };d.DoModal();
}
inline void advancedProperties(CWnd* owner,Manager& manager,Json& draft){
 Form d("Advanced download properties",540,435,owner);d.init=[&]{
  d.label("Category",14,18,124);auto cat=d.combo(manager.categories(),str(draft,"Category"),150,14,375);
  d.label("Queue",14,53,124);auto queue=d.queueCombo(manager,queueNames(manager),str(draft,"Queue"),150,49,375);
  auto member=d.check("Include in this queue",yes(draft,"QueueMember",true),150,83,375);
  d.label("Connections (1-32)",14,119,132);auto connections=d.edit(std::to_string(num(draft,"Connections",8)),150,115,87);
  d.label("Speed limit (KB/s)",265,119,124);auto limit=d.edit(std::to_string(num(draft,"LimitKbps")),402,115,123);
  d.label("Use 0 for an unlimited per-file transfer rate.",150,145,375);
  d.label("Expected SHA-256",14,183,128);auto hash=d.edit(str(draft,"ExpectedSha256"),150,179,375);
  auto headers=readHeaders(draft);d.label("User-Agent",14,217,128);auto ua=d.edit(headerValue(headers,"User-Agent"),150,213,375);
  d.label("Authorization header",14,251,130);auto auth=d.edit(headerValue(headers,"Authorization"),150,247,375,23,false,false,true);
  d.label("Source web page",14,284,132);auto page=d.edit(recoveryPage(draft),150,280,375);
  auto suppress=d.check("Do not show the completion dialog for this file",yes(draft,"SuppressCompletionDialog"),14,320,511);
  d.label("Downloaded SHA-256",14,353,132);d.edit(str(draft,"Sha256"),150,349,375,23,true);
  d.accept=[&,cat,queue,member,connections,limit,hash,ua,auth,suppress,page,headers]()mutable{auto next=draft;next["DownloadPage"]=trim(text(page));next["Category"]=text(cat);next["Queue"]=queue->queueName();next["QueueMember"]=d.checked(member);next["Connections"]=std::stoll(text(connections));next["LimitKbps"]=std::stoll(text(limit));next["ExpectedSha256"]=trim(text(hash));next["SuppressCompletionDialog"]=d.checked(suppress);setHeader(headers,"User-Agent",text(ua));setHeader(headers,"Authorization",text(auth));validateHeaders(headers);next["ProtectedHeaders"]=headers.empty()?"":protect(legacyDictionary(Json(headers)).dump());validateFileMetadata(next);draft=next;d.close();};
  d.button("OK",339,395,88,d.accept);d.button("Cancel",437,395,88,[&]{d.close(IDCANCEL);});
 };d.DoModal();
}
inline std::string propertiesLocation(const fs::path& target,const AsyncFileStatus::Result* status=nullptr){return status&&status->missing(target)?"The file has been moved.":utf8(target.wstring());}
inline void fileProperties(CWnd* parent,Manager& manager,JobPtr job,const std::function<int(CMenu&,CPoint,CWnd*)>& selectMenu={},AsyncFileStatus* locationProbe=nullptr){
 Json draft;bool active;{Lock lock(manager.mutex);draft=job->snapshot();active=manager.isActive(job);}bool complete=str(draft,"Status")=="Complete",everEditable=!active;Form d("File Properties",318,246,parent);d.dialogUnits=true;HICON fileIcon=nullptr;
 static constexpr const char* editableMetadata[]={"Category","Queue","QueueMember","Connections","LimitKbps","ExpectedSha256","SuppressCompletionDialog","ProtectedHeaders","DownloadPage"};
 const auto requestTarget=str(draft,"Url");
 Json originalProperties=Json::object();for(auto key:editableMetadata)if(draft.contains(key))originalProperties[key]=draft[key];
 auto mergeMetadataLocked=[&]{for(auto key:editableMetadata)if(draft.value(key,Json())==originalProperties.value(key,Json())){
  if(job->data.contains(key)){draft[key]=job->data[key];originalProperties[key]=job->data[key];}
  else{draft.erase(key);originalProperties.erase(key);}
 }};
 // Share a bounded worker even if a Shell extension stalls during lookup.
 static AsyncFileIcons fileDetails(FileIconMode::LargeWithType);
 static std::deque<std::pair<ULONGLONG,FileIconResult>> recentFileDetails;
 static AsyncFileStatus fileLocations;
 auto& locationStatus=locationProbe?*locationProbe:fileLocations;
 auto extension=lower(utf8(fs::path(wide(str(draft,"FileName"))).extension().wstring()));
 bool fileDetailsApplied=false;
 d.init=[&]{
  auto icon=d.control(L"STATIC","",SS_ICON,7,7,20,20);icon->SendMessage(STM_SETICON,(WPARAM)AfxGetApp()->LoadIcon(1));auto name=d.control(L"STATIC",str(draft,"FileName"),SS_ENDELLIPSIS,58,13,253,9);
  auto rule=[&](int y){d.control(L"STATIC","",SS_ETCHEDHORZ,7,y,304,1);};rule(31);
  d.label("Type:",7,36,44);auto type=d.control(L"STATIC","File",SS_ENDELLIPSIS,58,36,253,9);
  std::string state=str(draft,"Status");if(!complete&&num(draft,"Size")>0){std::ostringstream value;value<<std::fixed<<std::setprecision(2)<<(100.0*num(draft,"Received")/num(draft,"Size"));state+=" ("+value.str()+"% complete)";}
  d.label("Status:",7,48,43);auto statusText=d.control(L"STATIC",state,SS_ENDELLIPSIS,58,48,253,9);
  const auto savedSize=savedMediaOutputSize(draft);
  const auto length=savedSize>=0?savedSize:num(draft,"Size",-1);
  d.label("Size:",7,60,43);auto sizeText=d.control(L"STATIC",length<0?"Unknown":bytes(length)+" ("+std::to_string(length)+" Bytes)"+(savedSize>=0?" — saved output":""),SS_ENDELLIPSIS,58,60,253,9);
  d.label("Save To:",7,77,45);auto path=d.edit(propertiesLocation(job->target()),58,74,197,14,complete);
  auto displayedDestination=std::make_shared<std::string>(text(path));
  auto relocate=d.button(complete?"&Move":"&Browse...",261,74,50,[&,name,path,displayedDestination]{bool finished;{Lock lock(manager.mutex);if(manager.isActive(job))return;finished=str(job->data,"Status")=="Complete";}if(finished){moveCompleted(&d,manager,job);Lock lock(manager.mutex);draft["Folder"]=job->data["Folder"];draft["FileName"]=job->data["FileName"];name->SetWindowText(cs(str(draft,"FileName")));*displayedDestination=propertiesLocation(job->target());path->SetWindowText(cs(*displayedDestination));}else{CFileDialog chooser(FALSE,nullptr,wide(text(path)).c_str(),OFN_PATHMUSTEXIST|OFN_NOCHANGEDIR,L"All files|*.*||",&d);if(chooser.DoModal()==IDOK)path->SetWindowText(chooser.GetPathName());}});relocate->EnableWindow(!active);
  d.label("Address:",7,96,44);auto address=d.edit(capturedMedia(draft)?downloadAddress(draft):str(draft,"Url"),58,93,253,14,capturedMedia(draft));
  d.label("Description:",7,115,47);auto desc=d.edit(str(draft,"Description"),58,112,253);rule(131);
  auto displayedAddress=std::make_shared<std::string>(text(address)),displayedDescription=std::make_shared<std::string>(text(desc));
  d.label("The web page from which this file was obtained:",7,136,304);
  auto page=d.staticLink(recoveryPage(draft).empty()?"Unknown":recoveryPage(draft),7,149,304,[&]{Url url(recoveryPage(draft));if(url.scheme!="http"&&url.scheme!="https")throw std::runtime_error("No HTTP or HTTPS source page is available.");openFile(&d,fs::path(wide(url.full)));});page->EnableWindow(!recoveryPage(draft).empty());rule(161);
  Json loginPrefs;{Lock lock(manager.mutex);loginPrefs=manager.state["Settings"];}auto headers=siteRequestHeaders(str(draft,"Url"),readHeaders(draft),loginPrefs);auto initial=basicLogin(headers);auto requestDraft=std::make_shared<PropertiesRequestDraft>(PropertiesRequestInput{initial.first,initial.second,headerValue(headers,"Referer")});
  d.label("Referer:",7,171,47);auto referer=d.edit(headerValue(headers,"Referer"),58,167,253);
  d.control(L"STATIC","Login",SS_RIGHT,23,188,50,9);auto user=d.edit(initial.first,80,185,126,13);
  d.control(L"STATIC","Password",SS_RIGHT,21,205,52,9);auto password=d.edit(initial.second,80,203,126,13,false,false,true);rule(220);
  auto syncAuth=[&,user,password,referer,requestDraft]{requestDraft->apply(draft,{text(user),text(password),text(referer)});};
  auto more=d.button("More...",7,227,50,[]{});d.bind(more,[&,syncAuth,user,password,referer,requestDraft,page,loginPrefs,more]{
   {Lock lock(manager.mutex);active=manager.isActive(job);}CMenu menu;menu.CreatePopupMenu();menu.AppendMenu(MF_STRING|(active?MF_GRAYED:MF_ENABLED),1,L"Advanced properties...");menu.AppendMenu(MF_STRING,2,L"Download links...");menu.AppendMenu(MF_STRING,3,L"Virus check...");menu.AppendMenu(MF_STRING,4,L"Last attempt and result...");menu.AppendMenu(MF_STRING|(capturedMedia(draft)||active?MF_GRAYED:MF_ENABLED)|(password->SendMessage(EM_GETPASSWORDCHAR)?MF_UNCHECKED:MF_CHECKED),5,L"Show password");if(hasAudioMetadata(draft))menu.AppendMenu(MF_STRING,7,L"Audio metadata...");menu.AppendMenu(MF_SEPARATOR);menu.AppendMenu(MF_STRING,6,L"Cancel edits");CRect anchor;more->GetWindowRect(&anchor);CPoint point(anchor.left,anchor.bottom);const auto choice=selectMenu?selectMenu(menu,point,&d):menu.TrackPopupMenu(TPM_RETURNCMD,point.x,point.y,&d);{Lock lock(manager.mutex);active=manager.isActive(job);}
   if(choice==1&&!active){syncAuth();advancedProperties(&d,manager,draft);auto h=siteRequestHeaders(str(draft,"Url"),readHeaders(draft),loginPrefs);auto login=basicLogin(h);requestDraft->reset({login.first,login.second,headerValue(h,"Referer")});user->SetWindowText(cs(login.first));password->SetWindowText(cs(login.second));referer->SetWindowText(cs(headerValue(h,"Referer")));page->SetWindowText(cs(recoveryPage(draft).empty()?"Unknown":recoveryPage(draft)));page->EnableWindow(!recoveryPage(draft).empty());}
   else if(choice==2)downloadLinkDetails(&d,manager,job);else if(choice==3)fileScanner(&d,manager,job);
   else if(choice==4){auto result=str(draft,"Error");if(result.empty())result=complete?"Download completed successfully.":"Saved data: "+bytes(num(draft,"Received"));const auto captions=subtitleCaptureSummary(draft);if(!captions.empty())result+="\r\n"+captions;d.MessageBox(cs("Last try: "+displayDate(draft.value("LastAttempt",Json()))+"\r\n\r\nResult: "+result),L"Download details",MB_OK|MB_ICONINFORMATION);}
   else if(choice==7&&hasAudioMetadata(draft))audioMetadataProperties(&d,draft);
   else if(choice==5&&!capturedMedia(draft)&&!active){password->SendMessage(EM_SETPASSWORDCHAR,password->SendMessage(EM_GETPASSWORDCHAR)?0:0x25cf);password->Invalidate();}else if(choice==6)d.close(IDCANCEL);
  });
  auto canOpenLocked=[&]{return completedFileOpenReady(job->data,manager.isActive(job)||manager.isRelocating(job));};
  auto open=d.button("Open",172,227,50,[&,canOpenLocked]{
   fs::path target;
   {
    Lock lock(manager.mutex);
    if(!canOpenLocked())return;
    target=job->target();
   }
   openDownloadedFile(&d,target);
  });
  open->EnableWindow(FALSE);
  d.pulse=[&,open,canOpenLocked,icon,type,path,name,statusText,sizeText,address,desc,referer,user,password,relocate,displayedDestination,displayedAddress,displayedDescription]{
   const auto currentExtension=lower(utf8(fs::path(wide(str(draft,"FileName"))).extension().wstring()));
   if(extension!=currentExtension){
    extension=currentExtension;fileDetailsApplied=false;
    icon->SendMessage(STM_SETICON,(WPARAM)AfxGetApp()->LoadIcon(1));
    if(fileIcon){DestroyIcon(fileIcon);fileIcon=nullptr;}
    type->SetWindowText(L"File");
   }
   if(!fileDetailsApplied){
    const auto now=GetTickCount64();
    while(!recentFileDetails.empty()&&now-recentFileDetails.front().first>=60000)recentFileDetails.pop_front();
    for(auto& result:fileDetails.take()){
     recentFileDetails.emplace_back(now,std::move(result));
     if(recentFileDetails.size()>64)recentFileDetails.pop_front();
    }
    const auto result=std::find_if(recentFileDetails.rbegin(),recentFileDetails.rend(),[&](const auto& entry){return entry.second.extension==extension;});
    if(result==recentFileDetails.rend())fileDetails.request(extension);
    else{
     if(!result->second.typeName.empty())type->SetWindowText(cs(result->second.typeName));
     if(result->second.icon)if(auto copied=CopyIcon(result->second.icon)){
      icon->SendMessage(STM_SETICON,(WPARAM)copied);
      if(fileIcon)DestroyIcon(fileIcon);
      fileIcon=copied;
     }
     fileDetailsApplied=true;
    }
   }
   std::unique_lock lock(manager.mutex,std::try_to_lock);
   if(!lock)return;
   const bool ready=canOpenLocked();
   const auto target=job->target();const auto currentFileName=str(job->data,"FileName"),currentFolder=str(job->data,"Folder");
   const auto currentUrl=str(job->data,"Url"),currentDescription=str(job->data,"Description"),currentAddress=capturedMedia(job->data)?downloadAddress(job->data):currentUrl;
   auto currentState=str(job->data,"Status");const auto currentTotal=num(job->data,"Size",-1),currentReceived=num(job->data,"Received"),savedLength=savedMediaOutputSize(job->data);
   active=manager.isActive(job);complete=currentState=="Complete";
   // Merge untouched metadata so publication's queue and session changes cannot
   // be undone by a dialog opened before the transfer finished.
   mergeMetadataLocked();
   lock.unlock();
   const auto refreshUntouched=[](CWnd* field,std::string& displayed,const std::string& value){
    if(text(field)!=displayed)return;
    if(displayed!=value){
     DWORD begin=0,end=0;field->SendMessage(EM_GETSEL,reinterpret_cast<WPARAM>(&begin),reinterpret_cast<LPARAM>(&end));
     field->SetWindowText(cs(value));field->SendMessage(EM_SETSEL,begin,end);
    }
    displayed=value;
   };
   refreshUntouched(address,*displayedAddress,currentAddress);refreshUntouched(desc,*displayedDescription,currentDescription);
   draft["Url"]=currentUrl;
   draft["FileName"]=currentFileName;draft["Folder"]=currentFolder;everEditable|=!active;
   for(auto field:{address,desc,path,referer})if((field->IsWindowEnabled()!=FALSE)!=!active)field->EnableWindow(!active);
   const bool credentialsEditable=!active&&!capturedMedia(draft);
   for(auto field:{user,password})if((field->IsWindowEnabled()!=FALSE)!=credentialsEditable)field->EnableWindow(credentialsEditable);
   if((relocate->IsWindowEnabled()!=FALSE)!=!active)relocate->EnableWindow(!active);
   const auto buttonLabel=complete?"&Move":"&Browse...";if(text(relocate)!=buttonLabel)relocate->SetWindowText(cs(buttonLabel));
   if(!complete&&currentTotal>0){std::ostringstream percent;percent<<std::fixed<<std::setprecision(2)<<(100.0*currentReceived/currentTotal);currentState+=" ("+percent.str()+"% complete)";}
   if(text(statusText)!=currentState)statusText->SetWindowText(cs(currentState));
   const auto currentLength=savedLength>=0?savedLength:currentTotal;
   const auto currentSize=currentLength<0?std::string("Unknown"):bytes(currentLength)+" ("+std::to_string(currentLength)+" Bytes)"+(savedLength>=0?" — saved output":"");
   if(text(sizeText)!=currentSize)sizeText->SetWindowText(cs(currentSize));
   if(text(name)!=currentFileName)name->SetWindowText(cs(currentFileName));
   const bool destinationEdited=text(path)!=*displayedDestination;
   if((path->GetStyle()&ES_READONLY)!=(complete&&!destinationEdited?ES_READONLY:0))path->SendMessage(EM_SETREADONLY,complete&&!destinationEdited);
   if(!destinationEdited){
    auto location=propertiesLocation(target);
    if(complete){
    const auto status=locationStatus.observe({target});
     location=propertiesLocation(target,status.get());
    }
    if(text(path)!=location)path->SetWindowText(cs(location));
    *displayedDestination=location;
   }
   if((open->IsWindowEnabled()!=FALSE)!=ready)open->EnableWindow(ready);
  };
  d.pulse();
  d.accept=[&,address,desc,path,syncAuth,displayedDestination,displayedAddress,displayedDescription]{
   bool finished;Json changes=Json::object();
   {
    Lock lock(manager.mutex);
    if(manager.isActive(job)){if(everEditable)throw std::runtime_error("Stop the download before saving its properties. Your edits are still in this dialog.");d.close();return;}
    finished=str(job->data,"Status")=="Complete";mergeMetadataLocked();
    // Omit untouched fields. Manager merges this patch with the current job
    // under its lock, including changes made while a relocation is running.
    if(!capturedMedia(job->data)&&text(address)!=*displayedAddress)changes["Url"]=trim(text(address));
    if(text(desc)!=*displayedDescription)changes["Description"]=text(desc);
   }
   syncAuth();
   const PropertiesHeaderPatch requestChanges(readHeaders(originalProperties),readHeaders(draft),requestTarget);
   for(auto key:editableMetadata)if(std::string(key)!="ProtectedHeaders"&&draft.contains(key)&&draft.value(key,Json())!=originalProperties.value(key,Json()))changes[key]=draft[key];
   if(finished){
    if(text(path)!=*displayedDestination){const auto destination=fs::path(wide(trim(text(path))));RelocationWorkDialog progress(&d,manager,job,destination);progress.run();{Lock lock(manager.mutex);if(job->target().lexically_normal()!=destination.lexically_normal())return;}}
    Lock lock(manager.mutex);requestChanges.apply(changes,job->data);manager.updateCompleted(job,changes);
   }else{
    if(text(path)!=*displayedDestination){
     auto destination=fs::path(wide(trim(text(path))));changes["Folder"]=utf8(destination.parent_path().wstring());changes["FileName"]=utf8(destination.filename().wstring());
    }
    Lock lock(manager.mutex);requestChanges.apply(changes,job->data);manager.configure(job,changes);
   }
   d.close();
  };
  d.defaultButton(d.button("OK",261,227,50,d.accept));
 };try{d.DoModal();}catch(...){if(fileIcon)DestroyIcon(fileIcon);throw;}if(fileIcon)DestroyIcon(fileIcon);
}
