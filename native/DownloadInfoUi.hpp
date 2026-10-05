// Included inside udm after Form and property helpers.
inline bool downloadInfo(CWnd* parent,Manager& m,JobPtr job,bool properties=false){
 if(properties){fileProperties(parent,m,job);return true;}
 Json data,prefs;{Lock lock(m.mutex);data=job->snapshot();prefs=m.state["Settings"];}const bool media=capturedMedia(data);
 Form dialog("Download File Info",380,128,parent);dialog.dialogUnits=true;HICON fileIcon=nullptr;std::unique_ptr<DownloadPreview> preview;
 dialog.init=[&]{
  dialog.control(L"STATIC","URL",SS_RIGHT,5,5,54,10);dialog.edit(downloadAddress(data),64,3,244,14,true);
  dialog.control(L"STATIC","Category",SS_RIGHT,5,22,54,10);auto category=dialog.combo(m.categories(),str(data,"Category"),64,22,116);
  dialog.button("+",183,21,16,[&,category]{auto name=prompt(&dialog,"Create category");if(name.empty())return;m.editCategory("",name,"","","");category->ResetContent();for(auto& value:m.categories())category->AddString(cs(value));category->SetCurSel(category->FindStringExact(-1,cs(name)));dialog.SendMessage(WM_COMMAND,MAKEWPARAM(category->GetDlgCtrlID(),CBN_SELCHANGE),(LPARAM)category->GetSafeHwnd());});
  SHFILEINFOW info{};SHGetFileInfoW(wide(str(data,"FileName")).c_str(),FILE_ATTRIBUTE_NORMAL,&info,sizeof(info),SHGFI_USEFILEATTRIBUTES|SHGFI_ICON|SHGFI_TYPENAME);fileIcon=info.hIcon;
  auto icon=dialog.control(L"STATIC","",SS_ICON,333,28,20,20);if(fileIcon)icon->SendMessage(STM_SETICON,(WPARAM)fileIcon);
  auto size=dialog.control(L"STATIC",bytes(num(data,"Size",-1)),SS_CENTER,311,57,64,10);
  const auto extensionType=utf8(info.szTypeName);auto type=dialog.control(L"STATIC",str(data,"ContentType",extensionType),SS_CENTER|SS_ENDELLIPSIS,311,69,64,8);
  auto zipPreview=dialog.button("&Preview",319,78,50,[]{});
  auto previewStatus=dialog.control(L"STATIC","",SS_ENDELLIPSIS,7,132,366,9);
  dialog.control(L"STATIC","Save As",SS_RIGHT,5,41,54,10);
  std::vector<std::string> destinations={utf8(job->target().wstring())};if(prefs.contains("SaveFolderHistory")&&prefs["SaveFolderHistory"].is_array())for(auto& value:prefs["SaveFolderHistory"]){if(!value.is_string()||destinations.size()>=21)continue;fs::path folder(wide(value.get<std::string>()));if(folder.is_absolute())destinations.push_back(utf8((folder/wide(str(data,"FileName"))).wstring()));}
  auto destination=dialog.combo(destinations,destinations.front(),64,39,223,true);

  dialog.control(L"BUTTON","",BS_GROUPBOX,64,56,244,32);
  auto pathRule=dialog.check("Remember this path for this category",yes(prefs.value("CategoryRememberLast",Json::object()),str(data,"Category").c_str()),69,55,153);
  auto categoryFolder=dialog.edit(utf8(job->target().parent_path().wstring()),69,69,232);categoryFolder->EnableWindow(dialog.checked(pathRule));dialog.bind(pathRule,[&dialog,pathRule,categoryFolder,destination]{categoryFolder->EnableWindow(dialog.checked(pathRule));if(dialog.checked(pathRule))categoryFolder->SetWindowText(cs(utf8(fs::path(wide(text(destination))).parent_path().wstring())));});
  auto updateCategory=[&,category,destination,categoryFolder,pathRule]{Json settings;{Lock lock(m.mutex);settings=m.state["Settings"];}auto paths=dictionary(settings["CategoryPaths"]);auto folder=str(paths,text(category).c_str());if(folder.empty()){auto target=fs::path(wide(str(settings,"DownloadFolder")));if(yes(settings,"CategoryFolders",true))target/=wide(text(category));folder=utf8(target.wstring());}auto filename=fs::path(wide(text(destination))).filename();destination->SetWindowText(cs(utf8((fs::path(wide(folder))/filename).wstring())));categoryFolder->SetWindowText(cs(folder));pathRule->SendMessage(BM_SETCHECK,yes(settings.value("CategoryRememberLast",Json::object()),text(category).c_str())?BST_CHECKED:BST_UNCHECKED);categoryFolder->EnableWindow(dialog.checked(pathRule));};
  dialog.bindChange(category,updateCategory);
  auto syncPath=[&dialog,destination,categoryFolder,pathRule]{if(dialog.checked(pathRule))categoryFolder->SetWindowText(cs(utf8(fs::path(wide(text(destination))).parent_path().wstring())));};dialog.bindChange(destination,syncPath);
  dialog.button("...",292,39,16,[&dialog,destination,syncPath]{auto folder=chooseFolder(&dialog,fs::path(wide(text(destination))).parent_path().wstring());if(!folder.empty())destination->SetWindowText(cs(utf8((fs::path(folder)/fs::path(wide(text(destination))).filename()).wstring())));syncPath();});
  dialog.control(L"STATIC","Description",SS_RIGHT,5,93,54,10);auto description=dialog.edit(str(data,"Description"),64,91,244);
  auto advanced=std::make_shared<std::vector<CWnd*>>();
  auto track=[advanced](CWnd* w){advanced->push_back(w);return w;};track(previewStatus);
  track(dialog.button("Links...",319,91,50,[&]{downloadLinkDetails(&dialog,m,job);}));
  track(dialog.control(L"STATIC","",SS_ETCHEDHORZ,7,145,366,1));
  track(dialog.label("Web page",7,157,51));auto page=track(dialog.edit(recoveryPage(data),64,154,244,14,true));
  auto open=track(dialog.button("Open page",319,154,50,[&dialog,page]{Url u(text(page));if(u.scheme!="http"&&u.scheme!="https")throw std::runtime_error("No HTTP or HTTPS parent page is available.");openFile(&dialog,fs::path(wide(u.full)));}));open->EnableWindow(!text(page).empty());
  track(dialog.label("Queue",7,179,51));auto queue=dialog.combo(queueNames(m),str(data,"Queue","Main queue"),64,175,244);track(queue);
  auto headers=siteRequestHeaders(str(data,"Url"),readHeaders(data),prefs);auto initial=basicLogin(headers);bool initiallyBasic=lower(headerValue(headers,"Authorization")).rfind("basic ",0)==0;
  auto useLogin=track(dialog.check("Use login and password",initiallyBasic,7,198,266));useLogin->EnableWindow(!media);
  track(dialog.label("Login",7,219,51));auto user=track(dialog.edit(initial.first,64,216,112));
  track(dialog.label("Password",187,219,56));auto password=track(dialog.edit(initial.second,247,216,126,14,false,false,true));
  auto remember=track(dialog.check("Remember for this site",false,64,238,180));auto show=track(dialog.check("Show password",false,247,238,126));
  auto enabled=[&dialog,media,useLogin,user,password,remember,show,data]{bool on=!media&&dialog.checked(useLogin);user->EnableWindow(on);password->EnableWindow(on);show->EnableWindow(on);remember->EnableWindow(on&&siteLoginScheme(Url(str(data,"Url")).scheme));};
  dialog.bind(useLogin,enabled);enabled();dialog.bind(show,[&dialog,show,password]{password->SendMessage(EM_SETPASSWORDCHAR,dialog.checked(show)?0:0x25cf);password->Invalidate();});
  auto detail=track(dialog.label((!str(data,"ProtectedRequest").empty()?"Form download (POST). Restarting submits the form again.":str(data,"FormatDescription")),7,259,260,28));
  auto refreshDetails=track(dialog.button("Refresh details",279,259,94,[&]{}));
  auto refreshLookup=[&,useLogin,user,password,initial,initiallyBasic]{
   if(preview){preview->stop();preview.reset();}Json current,settings;{Lock lock(m.mutex);if(m.isActive(job))return;current=job->data;settings=m.state["Settings"];}if(!canPreviewDownload(current))return;
   if(dialog.checked(useLogin)!=initiallyBasic||text(user)!=initial.first||text(password)!=initial.second){auto h=readHeaders(current);setBasicLogin(h,text(user),text(password),dialog.checked(useLogin));current["ProtectedHeaders"]=h.empty()?"":protect(legacyDictionary(Json(h)).dump());}
   auto saveSession=browserSessionSaver(m,job,readBrowserSession(current));preview=std::make_unique<DownloadPreview>(std::move(current),std::move(settings),10000,std::move(saveSession));
  };
  dialog.bind(refreshDetails,refreshLookup);
  dialog.bind(zipPreview,[&,useLogin,user,password,initial,initiallyBasic,refreshLookup]{
   auto metadata=preview?preview->snapshot():Json::object();if(preview)preview->stop();bool prefetch;{Lock lock(m.mutex);prefetch=yes(job->data,"ConfirmationPending");}m.endPrefetch(job);
   try{Json current,settings;{Lock lock(m.mutex);current=job->data;settings=m.state["Settings"];}if(str(metadata,"Status")=="Ready")current["ContentType"]=str(metadata,"ContentType");
    if(dialog.checked(useLogin)!=initiallyBasic||text(user)!=initial.first||text(password)!=initial.second){auto h=readHeaders(current);setBasicLogin(h,text(user),text(password),dialog.checked(useLogin));current["ProtectedHeaders"]=h.empty()?"":protect(legacyDictionary(Json(h)).dump());}
    remoteZipDialog(&dialog,current,settings,browserSessionSaver(m,job,readBrowserSession(current)));
   }catch(...){if(prefetch)m.beginPrefetch(job);throw;}if(prefetch)m.beginPrefetch(job);refreshLookup();
  });
  dialog.pulse=[&,size,type,detail,previewStatus,refreshDetails,zipPreview,extensionType]{
   Json current;bool active;{Lock lock(m.mutex);current=job->data;active=m.isActive(job);}auto metadata=preview?preview->snapshot():Json::object();auto phase=str(metadata,"Status");
   i64 length=num(current,"Size",-1);auto mime=str(current,"ContentType");std::string status;
   if(!active&&phase=="Ready"){length=num(metadata,"Size",-1);mime=str(metadata,"ContentType");status=length<0?"The server did not report a file size.":"File details checked. No file data has been saved.";}
   else if(!active&&phase=="Checking")status="Checking file details...";
   else if(!active&&phase=="Error")status=str(metadata,"Message");
   size->SetWindowText(cs(length<0&&phase=="Checking"?"Checking...":bytes(length)));type->SetWindowText(cs(mime.empty()?extensionType:mime));previewStatus->SetWindowText(cs(status));
   refreshDetails->EnableWindow(!active&&canPreviewDownload(current));
   zipPreview->ShowWindow(canPreviewZip(current,mime)?SW_SHOW:SW_HIDE);
   if(yes(current,"ConfirmationPending"))detail->SetWindowText(cs("Downloaded: "+bytes(num(current,"Received"))+". Waiting for your confirmation."));
   else if(!status.empty())detail->SetWindowText(cs(status));
  };
  auto apply=[&,category,pathRule,categoryFolder,destination,description,queue,useLogin,user,password,remember,initiallyBasic,initial]{
   if(dialog.checked(pathRule)&&!fs::path(wide(trim(text(categoryFolder)))).is_absolute())throw std::runtime_error("Choose an absolute category folder.");
   if(preview)preview->stop();m.endPrefetch(job);auto path=fs::path(wide(text(destination)));
   if(!media&&dialog.checked(useLogin)){auto validation=readHeaders(data);setBasicLogin(validation,text(user),text(password));}
   m.configure(job,{{"Folder",utf8(path.parent_path().wstring())},{"FileName",utf8(path.filename().wstring())},{"Category",text(category)},{"Description",text(description)},{"Queue",text(queue)}});
   if(!media&&(dialog.checked(useLogin)!=initiallyBasic||text(user)!=initial.first||text(password)!=initial.second||dialog.checked(remember)))m.setDownloadLogin(job,text(user),text(password),dialog.checked(useLogin)&&dialog.checked(remember),dialog.checked(useLogin));
   {Lock lock(m.mutex);auto next=m.state["Settings"];next["CategoryRememberLast"][text(category)]=dialog.checked(pathRule);if(dialog.checked(pathRule)){fs::path remembered(wide(trim(text(categoryFolder))));if(!remembered.is_absolute())throw std::runtime_error("Choose an absolute category folder.");rememberCategoryDestination(next,text(category),remembered);}
    auto recent=Json::array();const auto folder=utf8(path.parent_path().wstring());recent.push_back(folder);if(next.contains("SaveFolderHistory")&&next["SaveFolderHistory"].is_array())for(auto& value:next["SaveFolderHistory"])if(value.is_string()&&lower(value.get<std::string>())!=lower(folder)&&recent.size()<20)recent.push_back(value);next["SaveFolderHistory"]=recent;m.setSettings(next);
   }
  };
  dialog.accept=[&,apply]{apply();m.resume(job);dialog.close();};dialog.cancel=[&]{if(preview)preview->stop();m.endPrefetch(job);m.pause(job);dialog.close(IDCANCEL);};
  auto expanded=std::make_shared<bool>(initiallyBasic||!str(data,"ProtectedRequest").empty());auto more=dialog.button("More >>",319,110,50,[]{});
  auto toggle=[&dialog,advanced,expanded,more]{for(auto w:*advanced)w->ShowWindow(*expanded?SW_SHOW:SW_HIDE);more->SetWindowText(*expanded?L"<< Less":L"More >>");dialog.resizeClient(380,*expanded?295:128);};
  dialog.bind(more,[expanded,toggle]{*expanded=!*expanded;toggle();});toggle();
  dialog.button("Download &Later",64,110,74,[&,apply,queue]{
   bool ask;{Lock lock(m.mutex);ask=yes(m.state["Settings"],"QueuePromptLater",true);}std::optional<DownloadQueueChoice> choice;
   if(ask){choice=chooseDownloadQueue(&dialog,m,text(queue));if(!choice)return;ensureDownloadQueue(m,*choice);if(queue->FindStringExact(-1,cs(choice->name))<0)queue->AddString(cs(choice->name));queue->SetCurSel(queue->FindStringExact(-1,cs(choice->name)));}
   apply();m.setMembership(job,true,text(queue));m.pause(job);if(choice)finishDownloadQueue(m,*choice,"QueuePromptLater");dialog.close();
  });
  dialog.defaultButton(dialog.button("&Start Download",149,110,74,dialog.accept));dialog.button("&Cancel",234,110,74,dialog.cancel);m.beginPrefetch(job);refreshLookup();dialog.pulse();
 };
 INT_PTR result=IDCANCEL;try{result=dialog.DoModal();}catch(...){m.endPrefetch(job);if(fileIcon)DestroyIcon(fileIcon);throw;}m.endPrefetch(job);if(fileIcon)DestroyIcon(fileIcon);if(result==-1)throw std::runtime_error("Cannot open Download File Info.");return result==IDOK;
}
