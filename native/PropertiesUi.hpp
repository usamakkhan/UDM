// Included inside udm after Form and Move/Rename helpers.
inline std::string displayDate(const Json& value){auto millis=parseDate(value);if(!millis)return "Never";auto seconds=(time_t)(millis/1000);tm local{};localtime_s(&local,&seconds);char buffer[64]{};strftime(buffer,sizeof(buffer),"%b %d %Y %H:%M:%S",&local);return buffer;}
inline void advancedProperties(CWnd* owner,Manager& manager,Json& draft){
 Form d("Advanced download properties",540,399,owner);d.init=[&]{
  d.label("Category",14,18,124);auto cat=d.combo(manager.categories(),str(draft,"Category"),150,14,375);
  d.label("Queue",14,53,124);auto queue=d.combo(queueNames(manager),str(draft,"Queue"),150,49,375);
  auto member=d.check("Include in this queue",yes(draft,"QueueMember",true),150,83,375);member->EnableWindow(str(draft,"Status")!="Complete");
  d.label("Connections (1-32)",14,119,132);auto connections=d.edit(std::to_string(num(draft,"Connections",8)),150,115,87);
  d.label("Speed limit (KB/s)",265,119,124);auto limit=d.edit(std::to_string(num(draft,"LimitKbps")),402,115,123);
  d.label("Use 0 for an unlimited per-file transfer rate.",150,145,375);
  d.label("Expected SHA-256",14,183,128);auto hash=d.edit(str(draft,"ExpectedSha256"),150,179,375);
  auto headers=readHeaders(draft);d.label("User-Agent",14,217,128);auto ua=d.edit(headerValue(headers,"User-Agent"),150,213,375);
  d.label("Authorization header",14,251,130);auto auth=d.edit(headerValue(headers,"Authorization"),150,247,375,23,false,false,true);
  auto suppress=d.check("Do not show the completion dialog for this file",yes(draft,"SuppressCompletionDialog"),14,287,511);
  d.label("Downloaded SHA-256",14,319,132);d.edit(str(draft,"Sha256"),150,315,375,23,true);
  d.accept=[&,cat,queue,member,connections,limit,hash,ua,auth,suppress,headers]()mutable{auto next=draft;next["Category"]=text(cat);next["Queue"]=text(queue);if(str(draft,"Status")!="Complete")next["QueueMember"]=d.checked(member);next["Connections"]=std::stoll(text(connections));next["LimitKbps"]=std::stoll(text(limit));next["ExpectedSha256"]=trim(text(hash));next["SuppressCompletionDialog"]=d.checked(suppress);setHeader(headers,"User-Agent",text(ua));setHeader(headers,"Authorization",text(auth));validateHeaders(headers);next["ProtectedHeaders"]=headers.empty()?"":protect(legacyDictionary(Json(headers)).dump());validateFileMetadata(next);draft=next;d.close();};
  d.button("OK",339,359,88,d.accept);d.button("Cancel",437,359,88,[&]{d.close(IDCANCEL);});
 };d.DoModal();
}
inline void fileProperties(CWnd* parent,Manager& manager,JobPtr job){
 Json draft;{Lock lock(manager.mutex);draft=job->snapshot();}bool complete=str(draft,"Status")=="Complete";bool active=manager.isActive(job);Form d("File Properties",490,482,parent);HICON fileIcon=nullptr;
 d.init=[&]{
  SHFILEINFOW info{};SHGetFileInfoW(wide(str(draft,"FileName")).c_str(),FILE_ATTRIBUTE_NORMAL,&info,sizeof(info),SHGFI_USEFILEATTRIBUTES|SHGFI_TYPENAME|SHGFI_ICON|SHGFI_SMALLICON);fileIcon=info.hIcon;
  auto icon=d.control(L"STATIC","",SS_ICON,14,12,32,32);if(fileIcon)icon->SendMessage(STM_SETICON,(WPARAM)fileIcon);auto name=d.label(str(draft,"FileName"),57,15,420,31);
  d.control(L"STATIC","",SS_ETCHEDHORZ,12,51,465,2);
  d.label("Type:",14,63,68);d.label(utf8(info.szTypeName),90,63,387);
  std::string state=str(draft,"Status");if(!complete&&num(draft,"Size")>0){std::ostringstream s;s<<std::fixed<<std::setprecision(2)<<(100.0*num(draft,"Received")/num(draft,"Size"));state+=" ("+s.str()+"% complete)";}
  d.label("Status:",14,84,68);d.label(state,90,84,387);
  auto size=num(draft,"Size",-1);d.label("Size:",14,105,68);d.label(size<0?"Unknown":bytes(size)+" ("+std::to_string(size)+" bytes)",90,105,387);
  d.label("Save To:",14,137,68);auto path=d.edit(utf8(job->target().wstring()),90,133,300,23,complete);
  auto relocate=d.button(complete?"Move...":"Browse...",398,132,79,[&,name,path]{if(complete){moveCompleted(&d,manager,job);Lock lock(manager.mutex);draft["Folder"]=job->data["Folder"];draft["FileName"]=job->data["FileName"];name->SetWindowText(cs(str(draft,"FileName")));path->SetWindowText(cs(utf8(job->target().wstring())));}else{CFileDialog chooser(FALSE,nullptr,wide(text(path)).c_str(),OFN_PATHMUSTEXIST,L"All files|*.*||",&d);if(chooser.DoModal()==IDOK)path->SetWindowText(chooser.GetPathName());}});
  relocate->EnableWindow(!active);d.label("Address:",14,167,68);auto address=d.edit(capturedMedia(draft)?downloadAddress(draft):str(draft,"Url"),90,163,300,23,capturedMedia(draft));d.button("Links...",398,162,79,[&]{downloadLinkDetails(&d,manager,job);});
  d.label("Description:",14,197,68);auto desc=d.edit(str(draft,"Description"),90,193,387);
  d.label("The web page from which this file was obtained:",14,226,463);auto page=d.edit(recoveryPage(draft),14,246,376);
  auto openPage=d.button("Open page",398,245,79,[&,page]{Url u(trim(text(page)));if(u.scheme!="http"&&u.scheme!="https")throw std::runtime_error("Enter an HTTP or HTTPS parent page.");openFile(&d,fs::path(wide(u.full)));});
  d.control(L"STATIC","",SS_ETCHEDHORZ,12,279,465,2);
  auto headers=readHeaders(draft);auto initial=basicLogin(headers);auto displayed=std::make_shared<std::pair<std::string,std::string>>(initial);d.label("Referer:",14,294,68);auto referer=d.edit(headerValue(headers,"Referer"),90,290,387);
  d.label("Login:",90,324,65);auto user=d.edit(initial.first,159,320,231);
  d.label("Password:",90,354,65);auto password=d.edit(initial.second,159,350,231,23,false,false,true);auto show=d.check("Show",false,398,350,79);d.bind(show,[&d,show,password]{password->SendMessage(EM_SETPASSWORDCHAR,d.checked(show)?0:0x25cf);password->Invalidate();});if(capturedMedia(draft)){user->EnableWindow(FALSE);password->EnableWindow(FALSE);show->EnableWindow(FALSE);}
  d.control(L"STATIC","",SS_ETCHEDHORZ,12,383,465,2);
  d.label("Last try date: "+displayDate(draft.value("LastAttempt",Json())),14,395,463);
  auto result=str(draft,"Error");if(result.empty())result=complete?"Download completed successfully.":"Saved data: "+bytes(num(draft,"Received"))+". "+(yes(draft,"RangeSupported")?"Resume is supported.":"Resume capability will be checked on the next attempt.");d.label("Result: "+result,14,417,463,22);
  auto syncAuth=[&,user,password,referer,displayed]{auto h=readHeaders(draft);setHeader(h,"Referer",trim(text(referer)));if(text(user)!=displayed->first||text(password)!=displayed->second){setBasicLogin(h,text(user),text(password),!text(user).empty()||!text(password).empty());}*displayed={text(user),text(password)};validateHeaders(h);draft["ProtectedHeaders"]=h.empty()?"":protect(legacyDictionary(Json(h)).dump());};
  auto open=d.button("Open",14,447,78,[&]{openFile(&d,job->target());});open->EnableWindow(complete&&fs::is_regular_file(job->target()));
  auto advanced=d.button("Advanced...",102,447,96,[&,syncAuth,user,password,referer,displayed]{syncAuth();advancedProperties(&d,manager,draft);auto h=readHeaders(draft);auto login=basicLogin(h);*displayed=login;user->SetWindowText(cs(login.first));password->SetWindowText(cs(login.second));referer->SetWindowText(cs(headerValue(h,"Referer")));});
  advanced->EnableWindow(!active);if(active)for(auto field:{address,desc,page,path,referer,user,password})field->EnableWindow(FALSE);d.accept=[&,address,desc,page,path,syncAuth]{if(active){d.close();return;}syncAuth();Json changes;for(auto key:{"Category","Queue","QueueMember","Connections","LimitKbps","ExpectedSha256","SuppressCompletionDialog","ProtectedHeaders"})if(draft.contains(key))changes[key]=draft[key];changes["Url"]=capturedMedia(draft)?str(draft,"Url"):trim(text(address));changes["Description"]=text(desc);changes["DownloadPage"]=trim(text(page));if(complete)manager.updateCompleted(job,changes);else{auto destination=fs::path(wide(trim(text(path))));changes["Folder"]=utf8(destination.parent_path().wstring());changes["FileName"]=utf8(destination.filename().wstring());manager.configure(job,changes);}d.close();};
  d.pulse=[page,openPage]{bool valid=false;try{Url u(trim(text(page)));valid=u.scheme=="http"||u.scheme=="https";}catch(...){}openPage->EnableWindow(valid);};d.pulse();d.defaultButton(d.button("OK",291,447,88,d.accept));d.button("Cancel",389,447,88,[&]{d.close(IDCANCEL);});
 };d.DoModal();if(fileIcon)DestroyIcon(fileIcon);
}
