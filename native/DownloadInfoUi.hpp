// Included inside udm after Form and property helpers.
inline void downloadInfo(CWnd* parent,Manager& m,JobPtr job,bool properties=false){
 if(properties){fileProperties(parent,m,job);return;}
 Json data,prefs;{Lock lock(m.mutex);data=job->snapshot();prefs=m.state["Settings"];}const bool media=capturedMedia(data);
 Form dialog("Download File Info",510,208,parent);HICON fileIcon=nullptr;std::unique_ptr<DownloadPreview> preview;
 dialog.init=[&]{
  dialog.label("URL",12,15,67);dialog.edit(downloadAddress(data),83,11,330,23,true);
  dialog.button("Links...",421,10,77,[&]{downloadLinkDetails(&dialog,m,job);});
  dialog.label("Category",12,47,67);auto category=dialog.combo(m.categories(),str(data,"Category"),83,43,233);
  SHFILEINFOW info{};SHGetFileInfoW(wide(str(data,"FileName")).c_str(),FILE_ATTRIBUTE_NORMAL,&info,sizeof(info),SHGFI_USEFILEATTRIBUTES|SHGFI_ICON|SHGFI_TYPENAME);fileIcon=info.hIcon;
  auto icon=dialog.control(L"STATIC","",SS_ICON,401,48,32,32);if(fileIcon)icon->SendMessage(STM_SETICON,(WPARAM)fileIcon);
  auto size=dialog.control(L"STATIC",bytes(num(data,"Size",-1)),SS_CENTER,348,88,150,19);
  const auto extensionType=utf8(info.szTypeName);auto type=dialog.control(L"STATIC",str(data,"ContentType",extensionType),SS_CENTER|SS_ENDELLIPSIS,353,109,145,19);
  auto previewStatus=dialog.control(L"STATIC","",SS_ENDELLIPSIS,12,155,486,16);
  dialog.label("Save As",12,79,67);auto destination=dialog.edit(utf8(job->target().wstring()),83,75,233);
  dialog.button("...",323,74,26,[&dialog,destination]{auto folder=chooseFolder(&dialog,fs::path(wide(text(destination))).parent_path().wstring());if(!folder.empty())destination->SetWindowText(cs(utf8((fs::path(folder)/fs::path(wide(text(destination))).filename()).wstring())));});
  auto pathRule=dialog.check("Remember this path for this category",false,83,101,267);
  dialog.label("Description",12,134,67);auto description=dialog.edit(str(data,"Description"),83,130,415);
  auto advanced=std::make_shared<std::vector<CWnd*>>();
  auto track=[advanced](CWnd* w){advanced->push_back(w);return w;};
  track(dialog.control(L"STATIC","",SS_ETCHEDHORZ,12,211,486,2));
  track(dialog.label("Web page",12,230,67));auto page=track(dialog.edit(recoveryPage(data),83,226,330,23,true));
  auto open=track(dialog.button("Open page",421,225,77,[&dialog,page]{Url u(text(page));if(u.scheme!="http"&&u.scheme!="https")throw std::runtime_error("No HTTP or HTTPS parent page is available.");openFile(&dialog,fs::path(wide(u.full)));}));open->EnableWindow(!text(page).empty());
  track(dialog.label("Queue",12,262,67));auto queue=dialog.combo(queueNames(m),str(data,"Queue","Main queue"),83,258,415);track(queue);
  auto headers=siteRequestHeaders(str(data,"Url"),readHeaders(data),prefs);auto initial=basicLogin(headers);bool initiallyBasic=lower(headerValue(headers,"Authorization")).rfind("basic ",0)==0;
  auto useLogin=track(dialog.check("Use login and password",initiallyBasic,12,290,280));useLogin->EnableWindow(!media);
  track(dialog.label("Login",12,325,67));auto user=track(dialog.edit(initial.first,83,321,157));
  track(dialog.label("Password",250,325,62));auto password=track(dialog.edit(initial.second,319,321,179,23,false,false,true));
  auto remember=track(dialog.check("Remember for this HTTPS site",false,83,352,225));auto show=track(dialog.check("Show password",false,319,352,179));
  auto enabled=[&dialog,media,useLogin,user,password,remember,show,data]{bool on=!media&&dialog.checked(useLogin);user->EnableWindow(on);password->EnableWindow(on);show->EnableWindow(on);remember->EnableWindow(on&&Url(str(data,"Url")).scheme=="https");};
  dialog.bind(useLogin,enabled);enabled();dialog.bind(show,[&dialog,show,password]{password->SendMessage(EM_SETPASSWORDCHAR,dialog.checked(show)?0:0x25cf);password->Invalidate();});
  auto detail=track(dialog.label((!str(data,"ProtectedRequest").empty()?"Form download (POST). Restarting submits the form again.":str(data,"FormatDescription")),12,388,341,34));
  auto refreshDetails=track(dialog.button("Refresh details",366,388,132,[&]{}));
  auto refreshLookup=[&,useLogin,user,password,initial,initiallyBasic]{
   if(preview){preview->stop();preview.reset();}Json current,settings;{Lock lock(m.mutex);if(m.isActive(job))return;current=job->data;settings=m.state["Settings"];}if(!canPreviewDownload(current))return;
   if(dialog.checked(useLogin)!=initiallyBasic||text(user)!=initial.first||text(password)!=initial.second){auto h=readHeaders(current);setBasicLogin(h,text(user),text(password),dialog.checked(useLogin));current["ProtectedHeaders"]=h.empty()?"":protect(legacyDictionary(Json(h)).dump());}
   preview=std::make_unique<DownloadPreview>(std::move(current),std::move(settings));
  };
  dialog.bind(refreshDetails,refreshLookup);
  dialog.pulse=[&,size,type,detail,previewStatus,refreshDetails,extensionType]{
   Json current;bool active;{Lock lock(m.mutex);current=job->data;active=m.isActive(job);}auto metadata=preview?preview->snapshot():Json::object();auto phase=str(metadata,"Status");
   i64 length=num(current,"Size",-1);auto mime=str(current,"ContentType");std::string status;
   if(!active&&phase=="Ready"){length=num(metadata,"Size",-1);mime=str(metadata,"ContentType");status=length<0?"The server did not report a file size.":"File details checked. No file data has been saved.";}
   else if(!active&&phase=="Checking")status="Checking file details...";
   else if(!active&&phase=="Error")status=str(metadata,"Message");
   size->SetWindowText(cs(length<0&&phase=="Checking"?"Checking...":bytes(length)));type->SetWindowText(cs(mime.empty()?extensionType:mime));previewStatus->SetWindowText(cs(status));
   refreshDetails->EnableWindow(!active&&canPreviewDownload(current));
   if(yes(current,"ConfirmationPending"))detail->SetWindowText(cs("Downloaded: "+bytes(num(current,"Received"))+". Waiting for your confirmation."));
   else if(!status.empty())detail->SetWindowText(cs(status));
  };
  auto apply=[&,category,pathRule,destination,description,queue,useLogin,user,password,remember,initiallyBasic,initial]{
   if(preview)preview->stop();m.endPrefetch(job);auto path=fs::path(wide(text(destination)));
   if(!media&&dialog.checked(useLogin)){auto validation=readHeaders(data);setBasicLogin(validation,text(user),text(password));}
   m.configure(job,{{"Folder",utf8(path.parent_path().wstring())},{"FileName",utf8(path.filename().wstring())},{"Category",text(category)},{"Description",text(description)},{"Queue",text(queue)}});
   if(!media&&(dialog.checked(useLogin)!=initiallyBasic||text(user)!=initial.first||text(password)!=initial.second||dialog.checked(remember)))m.setDownloadLogin(job,text(user),text(password),dialog.checked(useLogin)&&dialog.checked(remember),dialog.checked(useLogin));
   if(dialog.checked(pathRule)){Lock lock(m.mutex);auto prefs=m.state["Settings"];auto paths=dictionary(prefs["CategoryPaths"]);paths[text(category)]=utf8(path.parent_path().wstring());prefs["CategoryPaths"]=legacyDictionary(paths);m.setSettings(prefs);}
  };
  dialog.accept=[&,apply]{apply();m.resume(job);dialog.close();};dialog.cancel=[&]{if(preview)preview->stop();m.endPrefetch(job);m.pause(job);dialog.close(IDCANCEL);};
  auto expanded=std::make_shared<bool>(initiallyBasic||!str(data,"ProtectedRequest").empty());auto more=dialog.button("More >>",12,173,77,[]{});
  auto toggle=[&dialog,advanced,expanded,more]{for(auto w:*advanced)w->ShowWindow(*expanded?SW_SHOW:SW_HIDE);more->SetWindowText(*expanded?L"<< Less":L"More >>");dialog.resizeClient(510,*expanded?434:208);};
  dialog.bind(more,[expanded,toggle]{*expanded=!*expanded;toggle();});toggle();
  dialog.button("Download Later",151,173,112,[&,apply]{apply();m.pause(job);dialog.close();});
  dialog.defaultButton(dialog.button("Start Download",273,173,122,dialog.accept));dialog.button("Cancel",405,173,93,dialog.cancel);m.beginPrefetch(job);refreshLookup();dialog.pulse();
 };
 try{dialog.DoModal();}catch(...){m.endPrefetch(job);if(fileIcon)DestroyIcon(fileIcon);throw;}m.endPrefetch(job);if(fileIcon)DestroyIcon(fileIcon);
}
