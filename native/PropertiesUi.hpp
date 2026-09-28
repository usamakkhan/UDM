// Included inside udm after Form and Move/Rename helpers.
inline std::string displayDate(const Json& value){auto millis=parseDate(value);if(!millis)return "Never";auto seconds=(time_t)(millis/1000);tm local{};localtime_s(&local,&seconds);char buffer[64]{};strftime(buffer,sizeof(buffer),"%b %d %Y %H:%M:%S",&local);return buffer;}
inline void advancedProperties(CWnd* owner,Manager& manager,Json& draft){
 Form d("Advanced download properties",540,435,owner);d.init=[&]{
  d.label("Category",14,18,124);auto cat=d.combo(manager.categories(),str(draft,"Category"),150,14,375);
  d.label("Queue",14,53,124);auto queue=d.combo(queueNames(manager),str(draft,"Queue"),150,49,375);
  auto member=d.check("Include in this queue",yes(draft,"QueueMember",true),150,83,375);member->EnableWindow(str(draft,"Status")!="Complete");
  d.label("Connections (1-32)",14,119,132);auto connections=d.edit(std::to_string(num(draft,"Connections",8)),150,115,87);
  d.label("Speed limit (KB/s)",265,119,124);auto limit=d.edit(std::to_string(num(draft,"LimitKbps")),402,115,123);
  d.label("Use 0 for an unlimited per-file transfer rate.",150,145,375);
  d.label("Expected SHA-256",14,183,128);auto hash=d.edit(str(draft,"ExpectedSha256"),150,179,375);
  auto headers=readHeaders(draft);d.label("User-Agent",14,217,128);auto ua=d.edit(headerValue(headers,"User-Agent"),150,213,375);
  d.label("Authorization header",14,251,130);auto auth=d.edit(headerValue(headers,"Authorization"),150,247,375,23,false,false,true);
  d.label("Source web page",14,284,132);auto page=d.edit(recoveryPage(draft),150,280,375);
  auto suppress=d.check("Do not show the completion dialog for this file",yes(draft,"SuppressCompletionDialog"),14,320,511);
  d.label("Downloaded SHA-256",14,353,132);d.edit(str(draft,"Sha256"),150,349,375,23,true);
  d.accept=[&,cat,queue,member,connections,limit,hash,ua,auth,suppress,page,headers]()mutable{auto next=draft;next["DownloadPage"]=trim(text(page));next["Category"]=text(cat);next["Queue"]=text(queue);if(str(draft,"Status")!="Complete")next["QueueMember"]=d.checked(member);next["Connections"]=std::stoll(text(connections));next["LimitKbps"]=std::stoll(text(limit));next["ExpectedSha256"]=trim(text(hash));next["SuppressCompletionDialog"]=d.checked(suppress);setHeader(headers,"User-Agent",text(ua));setHeader(headers,"Authorization",text(auth));validateHeaders(headers);next["ProtectedHeaders"]=headers.empty()?"":protect(legacyDictionary(Json(headers)).dump());validateFileMetadata(next);draft=next;d.close();};
  d.button("OK",339,395,88,d.accept);d.button("Cancel",437,395,88,[&]{d.close(IDCANCEL);});
 };d.DoModal();
}
inline void fileProperties(CWnd* parent,Manager& manager,JobPtr job){
 Json draft;{Lock lock(manager.mutex);draft=job->snapshot();}const bool complete=str(draft,"Status")=="Complete",active=manager.isActive(job);Form d("File Properties",318,246,parent);d.dialogUnits=true;HICON fileIcon=nullptr;
 d.init=[&]{
  SHFILEINFOW info{};SHGetFileInfoW(wide(str(draft,"FileName")).c_str(),FILE_ATTRIBUTE_NORMAL,&info,sizeof(info),SHGFI_USEFILEATTRIBUTES|SHGFI_TYPENAME|SHGFI_ICON);fileIcon=info.hIcon;
  auto icon=d.control(L"STATIC","",SS_ICON,7,7,20,20);if(fileIcon)icon->SendMessage(STM_SETICON,(WPARAM)fileIcon);auto name=d.control(L"STATIC",str(draft,"FileName"),SS_ENDELLIPSIS,58,13,253,9);
  auto rule=[&](int y){d.control(L"STATIC","",SS_ETCHEDHORZ,7,y,304,1);};rule(31);
  d.label("Type:",7,36,44);d.control(L"STATIC",utf8(info.szTypeName),SS_ENDELLIPSIS,58,36,253,9);
  std::string state=str(draft,"Status");if(!complete&&num(draft,"Size")>0){std::ostringstream value;value<<std::fixed<<std::setprecision(2)<<(100.0*num(draft,"Received")/num(draft,"Size"));state+=" ("+value.str()+"% complete)";}
  d.label("Status:",7,48,43);d.control(L"STATIC",state,SS_ENDELLIPSIS,58,48,253,9);
  const auto length=num(draft,"Size",-1);d.label("Size:",7,60,43);d.control(L"STATIC",length<0?"Unknown":bytes(length)+" ("+std::to_string(length)+" bytes)",SS_ENDELLIPSIS,58,60,253,9);
  d.label("Save To:",7,77,45);auto path=d.edit(utf8(job->target().wstring()),58,74,197,14,complete);
  auto relocate=d.button("&Browse...",261,74,50,[&,name,path]{if(complete){moveCompleted(&d,manager,job);Lock lock(manager.mutex);draft["Folder"]=job->data["Folder"];draft["FileName"]=job->data["FileName"];name->SetWindowText(cs(str(draft,"FileName")));path->SetWindowText(cs(utf8(job->target().wstring())));}else{CFileDialog chooser(FALSE,nullptr,wide(text(path)).c_str(),OFN_PATHMUSTEXIST,L"All files|*.*||",&d);if(chooser.DoModal()==IDOK)path->SetWindowText(chooser.GetPathName());}});relocate->EnableWindow(!active);
  d.label("Address:",7,96,44);auto address=d.edit(capturedMedia(draft)?downloadAddress(draft):str(draft,"Url"),58,93,253,14,capturedMedia(draft));
  d.label("Description:",7,115,47);auto desc=d.edit(str(draft,"Description"),58,112,253);rule(131);
  d.label("The web page from which this file was obtained:",7,136,304);
  auto page=d.link(recoveryPage(draft).empty()?"Unknown":recoveryPage(draft),7,149,304,[&]{Url url(recoveryPage(draft));if(url.scheme!="http"&&url.scheme!="https")throw std::runtime_error("No HTTP or HTTPS source page is available.");openFile(&d,fs::path(wide(url.full)));});page->EnableWindow(!recoveryPage(draft).empty());rule(161);
  Json loginPrefs;{Lock lock(manager.mutex);loginPrefs=manager.state["Settings"];}auto headers=siteRequestHeaders(str(draft,"Url"),readHeaders(draft),loginPrefs);auto initial=basicLogin(headers);auto displayed=std::make_shared<std::pair<std::string,std::string>>(initial);
  d.label("Referer:",7,171,47);auto referer=d.edit(headerValue(headers,"Referer"),58,167,253);
  d.control(L"STATIC","Login",SS_RIGHT,23,188,50,9);auto user=d.edit(initial.first,80,185,126,13);
  d.control(L"STATIC","Password",SS_RIGHT,21,205,52,9);auto password=d.edit(initial.second,80,203,126,13,false,false,true);if(capturedMedia(draft)){user->EnableWindow(FALSE);password->EnableWindow(FALSE);}rule(220);
  auto syncAuth=[&,user,password,referer,displayed]{auto h=readHeaders(draft);setHeader(h,"Referer",trim(text(referer)));if(text(user)!=displayed->first||text(password)!=displayed->second)setBasicLogin(h,text(user),text(password),!text(user).empty()||!text(password).empty());*displayed={text(user),text(password)};validateHeaders(h);draft["ProtectedHeaders"]=h.empty()?"":protect(legacyDictionary(Json(h)).dump());};
  d.button("More...",7,227,50,[&,syncAuth,user,password,referer,displayed,page]{
   CMenu menu;menu.CreatePopupMenu();menu.AppendMenu(MF_STRING|(active?MF_GRAYED:MF_ENABLED),1,L"Advanced properties...");menu.AppendMenu(MF_STRING,2,L"Download links...");menu.AppendMenu(MF_STRING,3,L"Virus check...");menu.AppendMenu(MF_STRING,4,L"Last attempt and result...");menu.AppendMenu(MF_STRING|(capturedMedia(draft)||active?MF_GRAYED:MF_ENABLED)|(password->SendMessage(EM_GETPASSWORDCHAR)?MF_UNCHECKED:MF_CHECKED),5,L"Show password");menu.AppendMenu(MF_SEPARATOR);menu.AppendMenu(MF_STRING,6,L"Cancel edits");CPoint point;GetCursorPos(&point);const auto choice=menu.TrackPopupMenu(TPM_RETURNCMD,point.x,point.y,&d);
   if(choice==1&&!active){syncAuth();advancedProperties(&d,manager,draft);auto h=readHeaders(draft);auto login=basicLogin(h);*displayed=login;user->SetWindowText(cs(login.first));password->SetWindowText(cs(login.second));referer->SetWindowText(cs(headerValue(h,"Referer")));page->SetWindowText(cs(recoveryPage(draft).empty()?"Unknown":recoveryPage(draft)));page->EnableWindow(!recoveryPage(draft).empty());}
   else if(choice==2)downloadLinkDetails(&d,manager,job);else if(choice==3)fileScanner(&d,manager,job);
   else if(choice==4){auto result=str(draft,"Error");if(result.empty())result=complete?"Download completed successfully.":"Saved data: "+bytes(num(draft,"Received"));d.MessageBox(cs("Last try: "+displayDate(draft.value("LastAttempt",Json()))+"\r\n\r\nResult: "+result),L"Download details",MB_OK|MB_ICONINFORMATION);}
   else if(choice==5&&!capturedMedia(draft)&&!active){password->SendMessage(EM_SETPASSWORDCHAR,password->SendMessage(EM_GETPASSWORDCHAR)?0:0x25cf);password->Invalidate();}else if(choice==6)d.close(IDCANCEL);
  });
  auto open=d.button("Open",172,227,50,[&]{openFile(&d,job->target());});open->EnableWindow(complete&&fs::is_regular_file(job->target()));
  if(active)for(auto field:{address,desc,path,referer,user,password})field->EnableWindow(FALSE);
  d.accept=[&,address,desc,path,syncAuth]{if(active){d.close();return;}syncAuth();Json changes;for(auto key:{"Category","Queue","QueueMember","Connections","LimitKbps","ExpectedSha256","SuppressCompletionDialog","ProtectedHeaders","DownloadPage"})if(draft.contains(key))changes[key]=draft[key];changes["Url"]=capturedMedia(draft)?str(draft,"Url"):trim(text(address));changes["Description"]=text(desc);if(complete)manager.updateCompleted(job,changes);else{auto destination=fs::path(wide(trim(text(path))));changes["Folder"]=utf8(destination.parent_path().wstring());changes["FileName"]=utf8(destination.filename().wstring());manager.configure(job,changes);}d.close();};
  d.defaultButton(d.button("OK",261,227,50,d.accept));
 };try{d.DoModal();}catch(...){if(fileIcon)DestroyIcon(fileIcon);throw;}if(fileIcon)DestroyIcon(fileIcon);
}
