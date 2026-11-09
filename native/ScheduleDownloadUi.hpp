// Included inside udm after the common dialog controls.
struct ScheduledDownloadDraft {
 fs::path target;
 std::string description,category;
 bool rememberFolder=false;
 DownloadQueueChoice queue;
 std::string address;
 Headers headers;
 std::string previousQueue;
 std::optional<i64> connections,speedLimit;
};

inline bool scheduleDownloadWizard(CWnd* owner,Manager& manager,ScheduledDownloadDraft draft,
 const std::function<void(const ScheduledDownloadDraft&)>& save,const Json& existingQueue=Json(),bool enterSource=false){
 const bool revising=existingQueue.is_object();
 Form dialog(revising?"Edit download schedule":"Schedule download",370,265,owner);
 dialog.dialogUnits=true;
 enum Page {Source,Destination,Dates,Options,Review,PageCount};
 const auto firstPage=enterSource?Source:Destination;
 Page page=firstPage;
 bool editingReview=false;
 std::array<std::vector<CWnd*>,PageCount> controls;
 std::array<CWnd*,PageCount> first{};
 const auto categoryKeys=manager.categories();
  const auto initialFolder=utf8(draft.target.parent_path().wstring());
  std::vector<std::string> recentFolders;
  if(!initialFolder.empty())recentFolders.push_back(initialFolder);
  {
   Lock lock(manager.mutex);
   const auto history=manager.state["Settings"].find("SaveFolderHistory");
   if(history!=manager.state["Settings"].end()&&history->is_array())
    for(const auto& item:*history){
     if(!item.is_string())continue;
     const auto value=item.get<std::string>();
     if(fs::path(wide(value)).is_absolute()&&std::find(recentFolders.begin(),recentFolders.end(),value)==recentFolders.end())
      recentFolders.push_back(value);
     if(recentFolders.size()==20)break;
    }
  }
 const auto savedStart=existingQueue.is_object()?existingQueue.value("StartOnceUtc",Json()):Json();
 const auto savedStop=existingQueue.is_object()?existingQueue.value("StopOnceUtc",Json()):Json();
 const auto stopUtc=parseDate(savedStop);
 auto stopAnchor=stopUtc;
 auto startAnchor=savedStart;
 ScheduledDownloadWindow window;
 std::string suggestedName,suggestedFolder;
 int suggestedCategory=CB_ERR;
 draft.queue.create=!revising;
 dialog.init=[&]{
  auto heading=dialog.label("",9,8,352);
  auto add=[&](Page step,auto* control){controls[step].push_back(control);return control;};

  CWnd *address=nullptr,*auth=nullptr,*user=nullptr,*password=nullptr;
  std::function<void()> enableLogin;
  if(enterSource){
   add(Source,dialog.label("Download &address",9,27,352));
   address=add(Source,dialog.edit(draft.address,9,39,352));
   first[Source]=address;
   auth=add(Source,dialog.check("Use authori&zation",false,9,66,352));
   auto loginLabel=add(Source,dialog.label("&Login",9,91,170));
   user=add(Source,dialog.edit("",9,103,170));
   auto passwordLabel=add(Source,dialog.label("&Password",187,91,174));
   password=add(Source,dialog.edit("",187,103,174,14,false,false,true));
   enableLogin=[&dialog,auth,user,password,loginLabel,passwordLabel]{
    for(auto control:{user,password,loginLabel,passwordLabel})control->EnableWindow(dialog.checked(auth));
   };
   dialog.bind(auth,enableLogin);
   enableLogin();
   add(Source,dialog.label("Enter a direct HTTP, HTTPS or FTP link. UDM will contact the server when the schedule starts.",9,137,352,25));
  }

  add(Destination,dialog.label("Save to &folder",9,27,352));
  auto folder=add(Destination,dialog.combo(recentFolders,initialFolder,9,39,283,true));
  folder->SetWindowText(cs(initialFolder));
  first[Destination]=folder;
  auto browse=add(Destination,dialog.button("&Browse...",298,39,63,[]{}));
  add(Destination,dialog.label("File &name",9,59,352));
  auto name=add(Destination,dialog.edit(utf8(draft.target.filename().wstring()),9,71,352));
  add(Destination,dialog.label("Cate&gory",9,91,352));
  auto category=add(Destination,dialog.combo({},"",9,103,171));
  for(const auto& key:categoryKeys){
   auto label=categoryDisplayName(key);
   while(category->FindStringExact(-1,cs(label))!=CB_ERR)label+=" (custom)";
   if(category->AddString(cs(label))<0)throw std::runtime_error("Cannot add a category to the selector.");
   if(key==draft.category)category->SetCurSel(category->GetCount()-1);
  }
  auto useCategoryFolder=add(Destination,dialog.button("Use category folder",187,103,90,[]{}));
  auto remember=add(Destination,dialog.check("Remember this folder for the selected category",draft.rememberFolder,9,123,352));
  add(Destination,dialog.label("Des&cription",9,142,352));
  auto notes=add(Destination,dialog.edit(draft.description,9,154,352));
  add(Destination,dialog.label("Full destination",9,176,352));
  auto destinationPreview=add(Destination,dialog.edit("",9,188,352,14,true));
  destinationPreview->ModifyStyle(WS_TABSTOP,0);
  auto previewDestination=[name,destinationPreview](const std::string& value){
   const fs::path directory(wide(trim(value)));
   const auto filename=trim(text(name));
   destinationPreview->SetWindowText(directory.empty()||filename.empty()?L"":(directory/wide(filename)).c_str());
  };
  auto setFolder=[folder,previewDestination](const std::string& value){
   folder->SetWindowText(cs(value));
   previewDestination(value);
  };
  dialog.bind(browse,[&dialog,folder,setFolder]{
   const auto chosen=chooseFolder(&dialog,wide(text(folder)));
   if(!chosen.empty())setFolder(utf8(chosen));
  });
  dialog.bind(useCategoryFolder,[&manager,&categoryKeys,category,setFolder]{
   const auto index=category->GetCurSel();
   if(index==CB_ERR)throw std::runtime_error("Choose a category.");
   Json settings;
   {Lock lock(manager.mutex);settings=manager.state["Settings"];}
   setFolder(categoryFolder(settings,categoryKeys.at(index)));
  });
  dialog.bindChange(folder,[folder,previewDestination](UINT code){
   if(code==CBN_SELCHANGE){
    const auto index=folder->GetCurSel();
    if(index!=CB_ERR){
     CString selected;
     folder->GetLBText(index,selected);
     previewDestination(utf8(std::wstring(selected.GetString())));
     return;
    }
   }
   previewDestination(text(folder));
  });
  dialog.bindChange(name,[folder,previewDestination]{previewDestination(text(folder));});

  const auto scheduledStart=parseDate(savedStart);
  const auto startUtc=scheduledStart?scheduledStart:((epoch()+5*60000)/60000)*60000;
  auto dateTime=[&](int y,const char* dateLabel,const char* timeLabel,SYSTEMTIME value){
   add(Dates,dialog.label(dateLabel,9,y,225));
   auto day=add(Dates,dialog.make<ThemeDateTime>(DTS_LONGDATEFORMAT|WS_TABSTOP,9,y+12,225,14));
   add(Dates,dialog.label(timeLabel,245,y,116));
   auto time=add(Dates,dialog.make<ThemeDateTime>(DTS_TIMEFORMAT|DTS_UPDOWN|WS_TABSTOP,245,y+12,116,14));
   time->SetFormat(L"h:mm:ss tt");
   day->SetTime(&value);
   time->SetTime(&value);
   return std::pair{day,time};
  };
  auto readTime=[](const std::pair<ThemeDateTime*,ThemeDateTime*>& fields,const Json& previous){
   SYSTEMTIME day{},time{};
   if(fields.first->GetTime(&day)!=GDT_VALID||fields.second->GetTime(&time)!=GDT_VALID)
    throw std::runtime_error("Choose a valid date and time.");
   day.wHour=time.wHour;
   day.wMinute=time.wMinute;
   day.wSecond=time.wSecond;
   return editedScheduleSecond(day,previous);
  };
  add(Dates,dialog.label("&Queue name",9,27,352));
  auto queue=add(Dates,dialog.edit(revising?str(existingQueue,"Name"):"Scheduled download "+guid().substr(0,8),9,39,352,14,revising));
  const auto start=dateTime(63,"Start &date","Start &time",localScheduleTime(startUtc));
  first[Dates]=revising?static_cast<CWnd*>(start.first):queue;
  auto chooseStartDate=[start](bool tomorrow){
   auto day=localScheduleTime(epoch());
   if(tomorrow){
    // Advance the local calendar day independently of daylight-saving transitions.
    day.wHour=day.wMinute=day.wSecond=day.wMilliseconds=0;
    FILETIME stamp{};
    if(!SystemTimeToFileTime(&day,&stamp))throw std::runtime_error("Cannot read today's date.");
    ULARGE_INTEGER value{};
    value.LowPart=stamp.dwLowDateTime;
    value.HighPart=stamp.dwHighDateTime;
    value.QuadPart+=864000000000ULL;
    stamp={value.LowPart,value.HighPart};
    if(!FileTimeToSystemTime(&stamp,&day))throw std::runtime_error("Cannot read tomorrow's date.");
   }
   start.first->SetTime(&day);
  };
  add(Dates,dialog.button("Today",9,94,49,[chooseStartDate]{chooseStartDate(false);}));
  add(Dates,dialog.button("Tomorrow",64,94,59,[chooseStartDate]{chooseStartDate(true);}));
  auto startIn=[start,&startAnchor](i64 minutes){
   const auto instant=((epoch()+minutes*60000+999)/1000)*1000;
   auto local=localScheduleTime(instant);
   start.first->SetTime(&local);
   start.second->SetTime(&local);
   startAnchor=date(instant);
  };
  add(Dates,dialog.button("In 5 minutes",129,94,72,[startIn]{startIn(5);}));
  add(Dates,dialog.button("In 1 hour",207,94,59,[startIn]{startIn(60);}));
  auto stopEnabled=add(Dates,dialog.check("Set a sto&p date and time",stopUtc!=0,9,119,352));
  const auto stop=dateTime(137,"Stop d&ate","Stop ti&me",localScheduleTime(stopUtc?stopUtc:startUtc+3600000));
  auto stopAfter=[start,stop,stopEnabled,&stopAnchor,&startAnchor,readTime](i64 minutes){
   const auto deadline=readTime(start,startAnchor)+minutes*60000;
   auto local=localScheduleTime(deadline);
   stop.first->SetTime(&local);
   stop.second->SetTime(&local);
   stopAnchor=deadline;
   stopEnabled->SendMessage(BM_SETCHECK,BST_CHECKED);
   stop.first->EnableWindow(TRUE);
   stop.second->EnableWindow(TRUE);
  };
  add(Dates,dialog.button("Stop 1 hour after start",9,168,101,[stopAfter]{stopAfter(60);}));
  add(Dates,dialog.button("Stop 2 hours after start",116,168,105,[stopAfter]{stopAfter(120);}));
  auto enableStop=[&dialog,stopEnabled,stop]{
   const auto enabled=dialog.checked(stopEnabled);
   stop.first->EnableWindow(enabled);
   stop.second->EnableWindow(enabled);
  };
  dialog.bind(stopEnabled,[&dialog,&stopAnchor,stopEnabled,stopAfter,enableStop]{
   enableStop();
   if(!dialog.checked(stopEnabled)||stopAnchor)return;
   try{
    stopAfter(60);
   }catch(const std::runtime_error&){
    // Keep the initial stop suggestion while the start fields need correction.
   }
  });
  enableStop();
  add(Dates,dialog.label("This schedule runs once. Dates and times use your Windows time zone.",9,193,352,16));

  add(Options,dialog.label("Connection &retries (0 to 10)",9,30,198));
  const auto savedRetries=num(existingQueue,"Retries",-1);
  auto retries=add(Options,dialog.edit(savedRetries>=0?std::to_string(savedRetries):"",218,27,49));
  first[Options]=retries;
  add(Options,dialog.label("Leave blank to use the retry count from Options.",9,49,352,18));
  enum FileRetryMode {NoFileRetries,LimitedFileRetries,UnlimitedFileRetries};
  const auto savedFileRetries=num(existingQueue,"FileRetries");
  const bool unlimited=!yes(existingQueue,"FileRetryLimitEnabled",true);
  add(Options,dialog.label("If the &file fails",9,74,198));
  auto retryMode=add(Options,dialog.combo({"Do not retry","Retry a limited number of times","Retry until stopped"},"",218,70,143));
  retryMode->SetCurSel(unlimited?UnlimitedFileRetries:savedFileRetries>0?LimitedFileRetries:NoFileRetries);
  add(Options,dialog.label("File retry count (1 to 20)",9,92,198));
  auto fileRetries=add(Options,dialog.edit(std::to_string(savedFileRetries>0?savedFileRetries:3),218,89,49));
  add(Options,dialog.label("Delay between file retries in seconds (1 to 86400)",9,111,203));
  auto retryDelay=add(Options,dialog.edit(std::to_string(num(existingQueue,"RetryDelaySeconds",30)),218,108,49));
  auto enableFileRetries=[retryMode,fileRetries,retryDelay]{
   fileRetries->EnableWindow(retryMode->GetCurSel()==LimitedFileRetries);
   retryDelay->EnableWindow(retryMode->GetCurSel()!=NoFileRetries);
  };
  dialog.bindChange(retryMode,enableFileRetries);
  enableFileRetries();
  auto wake=add(Options,dialog.check("Request &wake from sleep",yes(existingQueue,"WakeComputer"),9,134,352));
  add(Options,dialog.label("Keep UDM running until the scheduled start. Windows and your hardware must allow wake timers for UDM to wake the computer.",9,152,352,26));
  add(Options,dialog.label("A stop date also ends any remaining file retries.",9,179,352,14));
  CWnd *connections=nullptr,*speedLimit=nullptr;
  if(draft.speedLimit){
   add(Options,dialog.label("Max. connections (1 to 32)",9,203,198));
   connections=add(Options,dialog.edit(draft.connections?std::to_string(*draft.connections):"",218,200,49));
   if(enterSource)add(Options,dialog.label("Blank uses Options",274,203,87));
   add(Options,dialog.label("Speed limit (KB/s, 0 for no per-file limit)",9,221,203));
   speedLimit=add(Options,dialog.edit(std::to_string(*draft.speedLimit),218,218,67));
  }

  add(Review,dialog.label(revising?"Review your changes, then select Save.":"Review your choices, then select Schedule.",9,27,352));
  auto summary=add(Review,dialog.control(L"EDIT","",WS_TABSTOP|ES_MULTILINE|ES_READONLY|ES_AUTOVSCROLL|WS_VSCROLL,9,40,352,169,WS_EX_CLIENTEDGE));
  first[Review]=summary;
  auto editDestination=add(Review,dialog.button("Edit destination",9,216,80,[]{}));
  auto editDates=add(Review,dialog.button("Edit dates",95,216,71,[]{}));
  auto editOptions=add(Review,dialog.button("Edit options",172,216,73,[]{}));
  CWnd* editSource=nullptr;
  if(enterSource)editSource=add(Review,dialog.button("Edit address",251,216,80,[]{}));
  dialog.control(L"STATIC","",SS_ETCHEDHORZ,9,237,352,1);

  auto back=dialog.button("< &Back",166,244,55,[]{});
  auto next=dialog.button("&Next >",228,244,67,[&]{dialog.accept();});
  dialog.defaultButton(next);
  dialog.button("Cancel",302,244,59,[&]{dialog.close(IDCANCEL);});
  auto show=[&,heading,back,next,folder,previewDestination]{
   const char* titles[]={"Download address","Destination","Dates and queue","Options","Review"};
   heading->SetWindowText(cs("Step "+std::to_string(page-firstPage+1)+" of "+
     std::to_string(PageCount-firstPage)+"   "+titles[page]));
   for(size_t i=0;i<controls.size();++i)
    for(auto control:controls[i])control->ShowWindow(i==static_cast<size_t>(page)?SW_SHOW:SW_HIDE);
   back->EnableWindow(page!=firstPage);
   next->SetWindowText(page==Review?(revising?L"&Save changes":L"&Schedule"):editingReview?L"&Review >":L"&Next >");
   if(page==Destination)previewDestination(text(folder));
   first[page]->SetFocus();
  };
  dialog.bind(back,[&,show]{if(page!=firstPage){editingReview=false;page=static_cast<Page>(page-1);show();}});
  auto editPage=[&,show](Page step){editingReview=true;page=step;show();};
  dialog.bind(editDestination,[editPage]{editPage(Destination);});
  dialog.bind(editDates,[editPage]{editPage(Dates);});
  dialog.bind(editOptions,[editPage]{editPage(Options);});
  if(editSource)dialog.bind(editSource,[editPage]{editPage(Source);});

  auto refreshReview=[&,summary]{
   auto localLabel=[](i64 ms){
    auto local=localScheduleTime(ms);
    wchar_t day[128]{},time[128]{};
    GetDateFormatEx(LOCALE_NAME_USER_DEFAULT,DATE_LONGDATE,&local,nullptr,day,128,nullptr);
    GetTimeFormatEx(LOCALE_NAME_USER_DEFAULT,0,&local,nullptr,time,128);
    return utf8(std::wstring(day)+L" "+time);
   };
   const auto source=enterSource?"Address: "+draft.address+
    "\r\nAuthorization: "+(draft.headers.empty()?"Off":"On")+"\r\n":std::string{};
   summary->SetWindowText(cs(source+"Save to: "+utf8(draft.target.wstring())+
    "\r\nCategory: "+categoryDisplayName(draft.category)+
    "\r\nRemember category folder: "+(draft.rememberFolder?"Yes":"No")+
    "\r\nDescription: "+draft.description+
    (draft.previousQueue.empty()?std::string{}:"\r\nMove this download from: "+draft.previousQueue)+
    (revising?"\r\nQueue: ":"\r\nNew queue: ")+draft.queue.name+
    "\r\nStart: "+localLabel(window.startUtc)+
    "\r\nStop: "+(window.stopUtc?localLabel(window.stopUtc):"When the download finishes")+
    "\r\nConnection retries: "+(window.retries?std::to_string(*window.retries):"Use Options")+
    "\r\nFile retries: "+(window.fileRetries||window.unlimitedFileRetries?
     (window.unlimitedFileRetries?"Unlimited":std::to_string(*window.fileRetries))+" every "+std::to_string(window.retryDelaySeconds)+" seconds":"Off")+
    "\r\nRequest wake: "+(window.wakeComputer?"Yes":"No")+
    (draft.speedLimit?"\r\nMax. connections: "+(draft.connections?std::to_string(*draft.connections):"Use Options")+
      "\r\nSpeed limit: "+(*draft.speedLimit?std::to_string(*draft.speedLimit)+" KB/s":"No per-file limit"):std::string{})));
   summary->SendMessage(EM_SETSEL,0,0);
   summary->SendMessage(EM_SCROLLCARET);
  };
  auto acceptPage=[&,show,refreshReview,address,auth,user,password,enableLogin,folder,name,notes,category,remember,queue,retries,retryMode,fileRetries,retryDelay,wake,connections,speedLimit,start,stop,stopEnabled,readTime](CWnd*& field){
   switch(page){
    case Source:{
     field=address;
     const Url url(trim(text(address)));
     if(hostIs(url.host,"youtube.com")||url.host=="youtu.be")
      throw std::runtime_error("Open this video in the browser and use the UDM video panel to schedule a quality.");
     if(suggestedCategory!=CB_ERR&&Url(draft.address).origin!=url.origin&&
        (dialog.checked(auth)||!text(user).empty()||!text(password).empty())){
      draft.headers.clear();
      auth->SendMessage(BM_SETCHECK,BST_UNCHECKED);
      user->SetWindowText(L"");
      password->SetWindowText(L"");
      enableLogin();
      field=auth;
      throw std::runtime_error("The website changed. Enter a login for the new website or continue without authorization.");
     }
     Headers headers;
     if(dialog.checked(auth)){field=user;setBasicLogin(headers,text(user),text(password));}
     if(url.full!=draft.address||suggestedCategory==CB_ERR){
      Json settings;
      {Lock lock(manager.mutex);settings=manager.state["Settings"];}
      const auto filename=safeName(unescape(url.path));
      const auto key=downloadCategory(filename,url.host,settings);
      const auto index=std::find(categoryKeys.begin(),categoryKeys.end(),key)-categoryKeys.begin();
      const auto selected=category->GetCurSel();
      if(index<static_cast<decltype(index)>(categoryKeys.size())&&(selected==CB_ERR||selected==suggestedCategory))
       category->SetCurSel(static_cast<int>(index));
      const auto selectedCategory=category->GetCurSel();
      if(selectedCategory==CB_ERR)throw std::runtime_error("The download categories changed. Reopen the wizard.");
      const auto directory=categoryFolder(settings,categoryKeys.at(selectedCategory));
      if(text(name).empty()||text(name)==suggestedName)name->SetWindowText(cs(filename));
      if(text(folder).empty()||text(folder)==suggestedFolder)folder->SetWindowText(cs(directory));
      suggestedName=filename;
      suggestedFolder=directory;
      suggestedCategory=static_cast<int>(index);
     }
     if(editingReview&&url.full!=draft.address)editingReview=false;
     draft.address=url.full;
     draft.headers=std::move(headers);
     break;
    }
    case Destination:{
     field=folder;
     const auto filename=trim(text(name));
     const fs::path directory(wide(trim(text(folder))));
     if(!directory.is_absolute())throw std::runtime_error("Choose an absolute destination folder.");
     field=name;
     if(filename.empty()||safeName(filename)!=filename)throw std::runtime_error("Enter a valid Windows file name.");
     field=category;
     const auto index=category->GetCurSel();
     if(index==CB_ERR)throw std::runtime_error("Choose a category.");
     draft.target=directory/wide(filename);
     draft.category=categoryKeys.at(index);
     draft.rememberFolder=dialog.checked(remember);
     draft.description=text(notes);
     break;
    }
    case Dates:{
     field=queue;
     const auto queueName=trim(text(queue));
     if(queueName.empty()||queueName.size()>80)throw std::runtime_error("Enter a queue name up to 80 characters.");
     const auto key=lower(queueName);
     if(key=="download limits")throw std::runtime_error("Choose a different name. Scheduler uses Download limits for its quota page.");
     if(!revising)for(const auto& existing:queueNames(manager))
      if(lower(existing)==key)throw std::runtime_error("A queue with that name already exists.");
     field=start.second;
     const auto startTime=readTime(start,startAnchor);
     if(startTime<=epoch()){field=start.first;throw std::runtime_error("Choose a future start date and time.");}
     field=stop.second;
     const auto stopTime=dialog.checked(stopEnabled)?readTime(stop,stopAnchor?Json(date(stopAnchor)):Json()):0;
     if(stopTime&&stopTime<=startTime)throw std::runtime_error("Choose a stop date and time after the start.");
     window.startUtc=startTime;
     window.stopUtc=stopTime;
     if(stopTime)stopAnchor=stopTime;
     draft.queue.name=queueName;
     break;
    }
    case Options:{
     field=retries;
     const auto retryText=trim(text(retries));
     if(retryText.empty())window.retries.reset();
     else window.retries=static_cast<int>(schedulerNumber(retryText,0,10,"connection retries"));
     window.fileRetries.reset();
     window.unlimitedFileRetries=retryMode->GetCurSel()==UnlimitedFileRetries;
     if(retryMode->GetCurSel()==LimitedFileRetries){
      field=fileRetries;
      window.fileRetries=static_cast<int>(schedulerNumber(text(fileRetries),1,20,"file retries"));
     }
     if(window.fileRetries||window.unlimitedFileRetries){
      field=retryDelay;
      window.retryDelaySeconds=static_cast<int>(schedulerNumber(text(retryDelay),1,86400,"retry delay in seconds"));
     }
     window.wakeComputer=dialog.checked(wake);
     if(connections){
      field=connections;
      const auto value=trim(text(connections));
      if(value.empty()&&enterSource)draft.connections.reset();
      else draft.connections=schedulerNumber(value,1,32,"maximum connections");
      field=speedLimit;
      draft.speedLimit=schedulerNumber(text(speedLimit),0,1000000,"speed limit in KB/s");
     }
     break;
    }
    case Review:
     if(window.startUtc<=epoch()){
      editingReview=true;
      page=Dates;
      show();
      field=start.first;
      throw std::runtime_error("The selected start time has passed. Choose a future date and time.");
     }
     draft.queue.window=window;
     save(draft);
     dialog.close();
     return;
    case PageCount:return;
   }
   page=editingReview?Review:static_cast<Page>(page+1);
   editingReview=false;
   if(page==Review)refreshReview();
   show();
  };
  dialog.accept=[&dialog,acceptPage,folder]{
   CWnd* field=nullptr;
   try{acceptPage(field);}
   catch(const std::exception& e){
    if(!field)throw;
    error(&dialog,e);
    dialog.SendMessage(WM_NEXTDLGCTL,reinterpret_cast<WPARAM>(field->GetSafeHwnd()),TRUE);
    if(field==folder)folder->SetEditSel(0,-1);
    else{
     wchar_t type[16]{};
     GetClassNameW(field->GetSafeHwnd(),type,16);
     if(!_wcsicmp(type,L"Edit"))field->SendMessage(EM_SETSEL,0,-1);
    }
   }
  };
  show();
 };
 return dialog.DoModal()==IDOK;
}

inline void addScheduledDownload(CWnd* owner,Manager& manager,std::string address){
 ScheduledDownloadDraft initial;
 initial.address=std::move(address);
 initial.speedLimit=0;
 scheduleDownloadWizard(owner,manager,std::move(initial),[&](const ScheduledDownloadDraft& draft){
  FileInfoFolderChoice choice;
  if(draft.rememberFolder)choice.rememberedCategoryFolder=draft.target.parent_path();
  manager.addScheduledDownload(draft.address,draft.target,draft.category,draft.description,draft.headers,draft.queue,choice,
                               draft.connections,draft.speedLimit.value_or(0));
 },Json(),true);
}

inline void scheduleListedDownload(CWnd* owner,Manager& manager,const JobPtr& job,bool revise){
 std::optional<Json> queue;
 Json original=Json::object();
 ScheduledDownloadDraft initial;
 {
  Lock lock(manager.mutex);
  if(revise){
   queue=manager.editableDownloadSchedule(job);
   if(!queue)throw std::runtime_error("This download no longer has an editable schedule.");
  }else{
   if(!manager.canScheduleExistingDownload(job))throw std::runtime_error("Pause the download and its queue before adding a schedule.");
   initial.previousQueue=str(job->data,"Queue");
  }
  for(const auto* field:{"Queue","Folder","FileName","Category","Description","Connections","LimitKbps"})
   original[field]=job->data.value(field,Json());
  initial.target=job->target();
  initial.category=str(job->data,"Category");
  initial.description=str(job->data,"Description");
  initial.connections=num(job->data,"Connections",8);
  initial.speedLimit=num(job->data,"LimitKbps");
 }
 scheduleDownloadWizard(owner,manager,std::move(initial),[&](const ScheduledDownloadDraft& draft){
  const auto folder=draft.target.parent_path();
  FileInfoFolderChoice choice;
  if(draft.rememberFolder)choice.rememberedCategoryFolder=folder;
  const Json edit={{"Folder",utf8(folder.wstring())},{"FileName",utf8(draft.target.filename().wstring())},
                   {"Category",draft.category},{"Description",draft.description},
                   {"Connections",*draft.connections},{"LimitKbps",*draft.speedLimit}};
  if(queue)manager.reviseDownloadSchedule(job,*queue,original,*draft.queue.window,edit,choice);
  else manager.scheduleExistingDownload(job,original,draft.queue,edit,choice);
 },queue.value_or(Json()));
}
