// Drives and renders only native windows owned by this isolated test executable.
#define UDM_TOOLBAR_COMPONENT_TEST
#include "App.cpp"
namespace udm {
static Json results=Json::array();static fs::path output;static std::string failure;static std::wstring expectedTitle;static std::function<void(HWND)> action;static ULONGLONG began;
static void expect(bool value,const char* name){results.push_back({{"name",name},{"passed",value}});if(!value)throw std::runtime_error(name);}

static std::wstring windowText(HWND w){wchar_t text[512]{};GetWindowTextW(w,text,512);return text;}
static HWND child(HWND parent,const wchar_t* label){for(HWND c=GetWindow(parent,GW_CHILD);c;c=GetWindow(c,GW_HWNDNEXT))if(windowText(c)==label&&IsWindowVisible(c))return c;throw std::runtime_error("Missing owned test control: "+utf8(label));}
static std::vector<HWND> children(HWND parent,const wchar_t* type){std::vector<HWND> result;for(HWND c=GetWindow(parent,GW_CHILD);c;c=GetWindow(c,GW_HWNDNEXT)){wchar_t name[128]{};GetClassNameW(c,name,128);if(!_wcsicmp(name,type))result.push_back(c);}return result;}
static void press(HWND w,const wchar_t* label){auto c=child(w,label);expect(IsWindowEnabled(c)!=FALSE,"Requested native button is enabled");SendMessageW(c,BM_CLICK,0,0);}
static void selectCombo(HWND parent,HWND control,int index){SendMessageW(control,CB_SETCURSEL,index,0);SendMessageW(parent,WM_COMMAND,MAKEWPARAM(GetDlgCtrlID(control),CBN_SELCHANGE),(LPARAM)control);}
static CRect bounds(HWND w){CRect r;GetWindowRect(w,&r);return r;}
static void capture(HWND w,const wchar_t* name){
 ShowWindow(w,SW_SHOWNOACTIVATE);RedrawWindow(w,nullptr,nullptr,RDW_INVALIDATE|RDW_ALLCHILDREN|RDW_UPDATENOW|RDW_ERASE);auto r=bounds(w);CImage img;img.Create(r.Width(),r.Height(),32);auto dc=img.GetDC();RECT fill{0,0,r.Width(),r.Height()};FillRect(dc,&fill,GetSysColorBrush(COLOR_BTNFACE));SendMessageW(w,WM_PRINT,(WPARAM)dc,PRF_NONCLIENT|PRF_CLIENT|PRF_ERASEBKGND|PRF_CHILDREN);
 for(HWND c=GetWindow(w,GW_CHILD);c;c=GetWindow(c,GW_HWNDNEXT)){if(!(GetWindowLongW(c,GWL_STYLE)&WS_VISIBLE))continue;auto cr=bounds(c);auto saved=SaveDC(dc);IntersectClipRect(dc,cr.left-r.left,cr.top-r.top,cr.right-r.left,cr.bottom-r.top);SetViewportOrgEx(dc,cr.left-r.left,cr.top-r.top,nullptr);SendMessageW(c,WM_PRINT,(WPARAM)dc,PRF_NONCLIENT|PRF_CLIENT|PRF_ERASEBKGND|PRF_CHILDREN);RestoreDC(dc,saved);}
 img.ReleaseDC();if(FAILED(img.Save((output/name).c_str())))throw std::runtime_error("Could not render native test dialog");
}
static BOOL CALLBACK observe(HWND w,LPARAM){if(windowText(w)!=expectedTitle)return TRUE;try{ShowWindow(w,SW_SHOWNOACTIVATE);auto invoke=action;invoke(w);}catch(const std::exception& e){failure=e.what();PostMessageW(w,WM_CLOSE,0,0);}return FALSE;}
static void CALLBACK onTimer(HWND,UINT,UINT_PTR,DWORD){if(GetTickCount64()-began>12000){failure="Owned dialog test deadline";EnumThreadWindows(GetCurrentThreadId(),[](HWND w,LPARAM)->BOOL{PostMessageW(w,WM_CLOSE,0,0);return TRUE;},0);return;}EnumThreadWindows(GetCurrentThreadId(),observe,0);}
static void modal(Form& dialog,const wchar_t* caption,std::function<void(HWND)> callback){expectedTitle=caption;action=std::move(callback);began=GetTickCount64();auto timer=SetTimer(nullptr,0,80,onTimer);dialog.DoModal();KillTimer(nullptr,timer);action={};if(!failure.empty())throw std::runtime_error(failure);}

static bool hasText(HWND dialog,const wchar_t* label){
 for(HWND c=GetWindow(dialog,GW_CHILD);c;c=GetWindow(c,GW_HWNDNEXT))if(IsWindowVisible(c)&&windowText(c)==label)return true;
 return false;
}

static void checkFileInfoScheduleLimits(const std::string& origin,bool prefetch){
 const std::string route=prefetch?"schedule-limits-prefetch":"schedule-limits-cold";
 Manager manager(output/wide(route));
 auto prefs=manager.state["Settings"];
 prefs["PrefetchFileInfo"]=prefetch;
 prefs["SkipBrowserFileInfo"]=false;
 prefs["Connections"]=6;
 prefs["DownloadFolder"]=utf8((output/L"scheduled-files").wstring());
 manager.setSettings(prefs);
 const auto job=manager.receive({{"action","add"},{"url",origin+"/"+route},{"filename",route+".zip"}});
 const auto target=job->target();
 job->sessionLimit=1024;
 expectedTitle=L"Download File Info";
 began=GetTickCount64();
 action=[&](HWND info){
  {
   Lock lock(manager.mutex);
   if(prefetch?!job->data.contains("CompletedPrefetch"):!hasText(info,L"Server-reported size: 4097 bytes"))return;
  }
  const auto infoAction=action;
  auto openWizard=[&](bool cancel){
   expectedTitle=L"Schedule download";
   action=[&,cancel](HWND wizard){
    press(wizard,L"&Next >");
    press(wizard,L"&Next >");
    const auto connections=GetWindow(child(wizard,L"Max. connections (1 to 32)"),GW_HWNDNEXT);
    const auto speed=GetWindow(child(wizard,L"Speed limit (KB/s, 0 for no per-file limit)"),GW_HWNDNEXT);
    expect(windowText(connections)==L"6"&&windowText(speed)==L"0","File Info Schedule seeds its transfer controls from the job");
    SetWindowTextW(connections,L"2");
    SetWindowTextW(speed,L"512");
    press(wizard,L"&Next >");
    bool reviewed=false;
    for(const auto field:children(wizard,L"Edit"))if(IsWindowVisible(field)){
     std::wstring value(static_cast<size_t>(GetWindowTextLengthW(field))+1,L'\0');
     GetWindowTextW(field,value.data(),static_cast<int>(value.size()));
     reviewed|=value.find(L"Max. connections: 2")!=std::wstring::npos&&value.find(L"Speed limit: 512 KB/s")!=std::wstring::npos;
    }
    expect(reviewed,"File Info Schedule reviews both edited transfer limits");
    press(wizard,cancel?L"Cancel":L"&Schedule");
   };
   press(info,L"Sche&dule...");
   expectedTitle=L"Download File Info";
   action=infoAction;
  };
  if(prefetch){
   const auto queues=manager.state["Queues"];
   openWizard(true);
   Lock lock(manager.mutex);
   expect(IsWindow(info)&&manager.state["Queues"]==queues&&num(job->data,"Connections")==6&&num(job->data,"LimitKbps")==0&&
    job->sessionLimit==1024&&yes(job->data,"ConfirmationPending")&&job->data.contains("CompletedPrefetch"),
    "Cancelling File Info Schedule leaves the catalog, limits and verified prefetch intact");
  }
  openWizard(false);
 };
 const auto timer=SetTimer(nullptr,0,80,onTimer);
 const bool scheduled=downloadInfo(nullptr,manager,job);
 KillTimer(nullptr,timer);
 action={};
 if(!failure.empty())throw std::runtime_error(failure);
 const auto deadline=GetTickCount64()+10000;
 while(manager.isActive(job)&&GetTickCount64()<deadline)Sleep(10);
 expect(scheduled,"Saving the schedule closes File Info with confirmation");
 expect(!manager.isActive(job)&&!job->sessionLimit&&!fs::exists(target),
  "Saving the schedule clears the temporary limit and leaves the destination unpublished");
 {
  Manager reopened(manager.root);
  expect(reopened.jobs.size()==1&&reopened.jobs.front()->id()==job->id(),"File Info Schedule keeps one durable job");
  const auto saved=reopened.jobs.front();
  const auto queue=reopened.editableDownloadSchedule(saved);
  expect(queue&&parseDate(queue->at("StartOnceUtc"))>epoch()&&str(saved->data,"Status")=="Paused"&&
   num(saved->data,"Connections")==2&&num(saved->data,"LimitKbps")==512,
   "File Info Schedule persists the future start, connection count and speed limit");
  reopened.tick();
  expect(!reopened.isActive(saved)&&!fs::exists(target),"The saved schedule does not start early");
 }
 manager.resume(job);
 const auto finish=GetTickCount64()+10000;
 while((str(job->snapshot(),"Status")=="Queued"||manager.isActive(job))&&GetTickCount64()<finish){manager.tick();Sleep(10);}
 expect(str(job->data,"Status")=="Complete"&&readText(target)==std::string(4097,1)&&
  yes(job->data,"ReusedCompletedPrefetch")==prefetch,
  "Starting the scheduled file saves exact bytes and reuses the completed prefetch when available");
}

static void checkBrowserOfferDismissal(const std::string& origin){
 Manager manager(output/L"browser-dismissal");
 auto prefs=manager.state["Settings"];
 prefs["DownloadFolder"]=utf8((output/L"browser-dismissal-files").wstring());
 prefs["CategoryFolders"]=false;
 prefs["SkipBrowserFileInfo"]=false;
 prefs["PrefetchFileInfo"]=true;
 prefs["Connections"]=1;
 prefs["OfferCaptureExclusions"]=false;
 manager.setSettings(prefs);
 for(const std::string mode:{"button","escape","close-stream","existing-escape","existing-close"}){
  const bool existing=mode.rfind("existing-",0)==0,streaming=mode=="close-stream";
  const auto name="browser-dismiss-"+mode+".zip";
  const auto job=manager.receive({{"action","add"},{"url",origin+"/browser-dismiss-"+mode},{"filename",name}});
  if(existing)manager.pause(job);
  const auto target=job->target(),staged=manager.root/L"parts"/wide(job->id())/(wide(job->id())+L".data");
  bool dismissed=false;
  expectedTitle=L"Download File Info";
  began=GetTickCount64();
  action=[&](HWND dialog){
   if(dismissed)return;
   {
    Lock lock(manager.mutex);
    if(streaming?num(job->data,"Received")<2048:!job->data.contains("CompletedPrefetch"))return;
    expect(yes(job->data,"ConfirmationPending")&&manager.isActive(job)&&fs::is_regular_file(staged)&&!fs::exists(target),
     "Browser dismissal begins with real unpublished prefetch bytes");
    if(streaming)expect(num(job->data,"Received")<num(job->data,"Size"),"Title-bar Close interrupts an in-flight browser response");
   }
   dismissed=true;
   if(mode=="button")press(dialog,L"&Cancel");
   else if(mode=="escape"||mode=="existing-escape"){
    MSG key{};
    key.hwnd=child(dialog,L"&Start Download");
    key.message=WM_KEYDOWN;
    key.wParam=VK_ESCAPE;
    expect(IsDialogMessageW(dialog,&key)!=FALSE,"Escape dispatches through the native dialog manager");
   }else SendMessageW(dialog,WM_SYSCOMMAND,SC_CLOSE,0);
  };
  const auto timer=SetTimer(nullptr,0,80,onTimer);
  presentDownload(nullptr,manager,job,true);
  KillTimer(nullptr,timer);
  action={};
  if(!failure.empty())throw std::runtime_error(failure);
  const auto saved=Json::parse(readText(manager.root/L"state.json")).at("Downloads");
  const bool persisted=std::any_of(saved.begin(),saved.end(),[&](const Json& entry){return str(entry,"Id")==job->id();});
  const bool listed=std::find(manager.jobs.begin(),manager.jobs.end(),job)!=manager.jobs.end();
  expect(dismissed&&!manager.isActive(job)&&!fs::exists(target),"Browser dismissal stops the worker without publishing the destination");
  if(existing)expect(listed&&persisted&&str(job->data,"Status")=="Paused"&&readText(staged)==std::string(4097,1),
   "Dismissing a previously saved download retains its paused record and exact staged bytes");
  else expect(!listed&&!persisted&&!fs::exists(staged),
   "Dismissing a fresh browser offer removes the catalog entry and staged bytes");
 }
}

static void checkPrefetchedZipPreview(const std::string& origin,bool streaming){
 const std::string suffix=streaming?"-stream":"";
 Manager manager(output/wide("zip-prefetch-state"+suffix));
 auto prefs=manager.state["Settings"];
 prefs["PrefetchFileInfo"]=true;
 prefs["TemporaryFolder"]=utf8((output/wide("zip-prefetch-temp"+suffix)).wstring());
 manager.setSettings(prefs);
 auto job=manager.add(origin+"/zip-prefetch-once"+suffix,utf8((output/wide("zip-prefetch-files"+suffix)).wstring()),"checked.zip","Main queue",true);
 const auto target=job->target();
 bool listed=false;
 expectedTitle=L"Download File Info";began=GetTickCount64();
 action=[&](HWND dialog){
  const auto before=[&]{Lock lock(manager.mutex);return job->snapshot();}();
  if(!hasText(dialog,L"&Preview"))return;
  if(streaming){
   if(!yes(before,"ConfirmationPending")||str(before,"ResponseFileNameUrl").empty())return;
   expect(!before.contains("CompletedPrefetch"),"The slow ZIP has accepted headers and has no completed receipt when Preview opens");
  }else if(!before.contains("CompletedPrefetch"))return;
  const auto parentAction=action;
  expectedTitle=L"Zip preview";
  if(streaming){
   action=[&](HWND preview){
    if(!hasText(preview,L"Waiting for the background download to finish..."))return;
    {Lock lock(manager.mutex);
     expect(manager.isActive(job)&&yes(job->data,"ConfirmationPending")&&!job->data.contains("CompletedPrefetch"),
      "ZIP Preview waits while the same background response is still incomplete");}
    press(preview,L"Cancel");
   };
   press(dialog,L"&Preview");
   if(!failure.empty())throw std::runtime_error(failure);
   {Lock lock(manager.mutex);
    expect(manager.isActive(job)&&yes(job->data,"ConfirmationPending")&&!fs::exists(target),
     "Cancel from a waiting preview keeps the background download and File Info active");}
  }
  action=[&](HWND preview){
   if(!hasText(preview,L"OK"))return;
   auto list=dynamic_cast<ZipPreviewList*>(CWnd::FromHandlePermanent(children(preview,L"SysListView32").at(0)));
   expect(list&&list->listing&&list->GetItemCount()==1&&list->listing->entries.at(0).name=="readme.txt",
    "ZIP Preview lists the entry from the completed prefetch");
   expect(!fs::exists(target),"ZIP Preview does not publish the prefetched archive");
   listed=true;
   press(preview,L"OK");
  };
  press(dialog,L"&Preview");
  expectedTitle=L"Download File Info";action=parentAction;
  if(!failure.empty())throw std::runtime_error(failure);
  {
   Lock lock(manager.mutex);
   const auto receipt=job->data.value("CompletedPrefetch",Json());
   expect(listed&&manager.isActive(job)&&yes(job->data,"ConfirmationPending")&&receipt.is_object()&&
     (streaming||receipt==before.at("CompletedPrefetch")),
    "Closing ZIP Preview keeps the same prefetch and confirmation active");
  }
  press(dialog,L"&Start Download");
 };
 const auto timer=SetTimer(nullptr,0,80,onTimer);
 downloadInfo(nullptr,manager,job);
 KillTimer(nullptr,timer);action={};
 if(!failure.empty())throw std::runtime_error(failure);
 const auto deadline=GetTickCount64()+10000;
 while(manager.isActive(job)&&GetTickCount64()<deadline)Sleep(10);
 expect(listed&&str(job->data,"Status")=="Complete"&&yes(job->data,"ContinuedFilePrefetch")&&fs::is_regular_file(target),
  "Start publishes the same ZIP prefetch after listing the archive");
}

static LRESULT CALLBACK staleDestinationText(HWND window,UINT message,WPARAM w,LPARAM l,UINT_PTR,DWORD_PTR context){
 const auto& old=*reinterpret_cast<const std::wstring*>(context);
 if(message==WM_GETTEXTLENGTH)return static_cast<LRESULT>(old.size());
 if(message==WM_GETTEXT&&w&&l){
  lstrcpynW(reinterpret_cast<LPWSTR>(l),old.c_str(),static_cast<int>(w));
  return static_cast<LRESULT>(std::min<size_t>(old.size(),w-1));
 }
 return DefSubclassProc(window,message,w,l);
}

static void checkRecentDestination(Manager& manager,const std::string& origin){
 const auto original=output/L"previous-folder",recent=output/L"recent-folder";
 fs::create_directories(original);
 fs::create_directories(recent);
 auto job=manager.add(origin+"/recent-destination",utf8(original.wstring()),"recent.zip","Main queue",true);
 const auto category=str(job->data,"Category");
 manager.state["Settings"]["PrefetchFileInfo"]=false;
 manager.state["Settings"]["SaveFolderHistory"]=Json::array({utf8(recent.wstring())});
 manager.state["Settings"]["CategoryRememberLast"][category]=true;
 expectedTitle=L"Download File Info";
 began=GetTickCount64();
 action=[&](HWND dialog){
  const auto destination=children(dialog,L"ComboBox").at(1);
  const auto previous=(original/L"chosen \u2603.zip").wstring();
  COMBOBOXINFO combo{sizeof(combo)};
  expect(GetComboBoxInfo(destination,&combo)&&combo.hwndItem,"Find the editable part of the destination combo");
  SendMessageW(combo.hwndItem,EM_SETSEL,0,-1);
  SendMessageW(combo.hwndItem,EM_REPLACESEL,TRUE,(LPARAM)previous.c_str());
  const auto caret=MAKELPARAM(previous.size(),previous.size());
  expect(windowText(destination)==previous&&SendMessageW(destination,CB_GETEDITSEL,0,0)==caret,
   "Updating recent destinations preserves the typed Unicode name and caret");
  expect(SendMessageW(combo.hwndItem,EM_CANUNDO,0,0)!=0,"Updating recent destinations keeps the native edit undo history");
  expect(SetWindowSubclass(destination,staleDestinationText,1,reinterpret_cast<DWORD_PTR>(&previous))!=FALSE,
   "The editable combo reports its old text during the selection notification");
  selectCombo(dialog,destination,1);
  RemoveWindowSubclass(destination,staleDestinationText,1);
  expect(hasText(dialog,recent.c_str()),"Selecting a recent destination updates the category folder before the edit text changes");
  expect(windowText(destination)==(recent/L"chosen \u2603.zip").wstring(),
   "Choosing a recent folder retains the typed filename");
  press(dialog,L"Download &Later");
 };
 const auto timer=SetTimer(nullptr,0,80,onTimer);
 downloadInfo(nullptr,manager,job);
 KillTimer(nullptr,timer);
 action={};
 if(!failure.empty())throw std::runtime_error(failure);
 expect(job->target()==recent/L"chosen \u2603.zip"&&
  categoryFolder(manager.state["Settings"],category)==utf8(recent.wstring()),
  "Download Later saves the selected destination and remembers its folder");
}

static void checkSabrDisplayAddress(Manager& manager){
 const std::string source="https://www.youtube.com/watch?v=abcdefghijk";
 const std::string endpoint="https://r1.googlevideo.com/videoplayback?id=display-fixture";
 {Lock lock(manager.mutex);manager.state["Settings"]["PrefetchFileInfo"]=false;}
 for(bool bound:{true,false}){
  auto job=manager.add(source,utf8(output.wstring()),bound?"bound-address.mp4":"legacy-address.mp4");
  job->data["SourceUrl"]=source;
  setStreams(manager,job,endpoint,endpoint);
  // The bound path must open even if the large session payload cannot be decrypted.
  job->data["ProtectedSabr"]=bound?std::string(131072,'!'):protect(Json{{"url",endpoint}}.dump());
  if(bound)job->data["SabrTrackAddressBound"]=true;
  expectedTitle=L"Download File Info";
  began=GetTickCount64();
  action=[&](HWND dialog){
   expect(hasText(dialog,wide(endpoint).c_str()),bound?
    "File Info displays the bound stream address without opening the session payload":
    "File Info still opens the saved stream address for a legacy session");
   press(dialog,L"&Cancel");
  };
  const auto timer=SetTimer(nullptr,0,80,onTimer);
  downloadInfo(nullptr,manager,job);
  KillTimer(nullptr,timer);
  action={};
  if(!failure.empty())throw std::runtime_error(failure);
 }
}

static void checkDurableDirectOffer(const std::string& origin){
 Manager manager(output/L"durable-offers");
 auto prefs=manager.state["Settings"];
 prefs["DownloadFolder"]=utf8((output/L"durable-files").wstring());
 prefs["CategoryFolders"]=false;
 prefs["Connections"]=1;
 manager.setSettings(prefs);
 for(bool changed:{false,true}){
  const std::string name=changed?"durable-changed":"durable-offer";
  auto job=manager.receive({{"action","add"},{"url",origin+"/"+name},{"filename",name+".zip"}});
  const auto stateFile=manager.root/L"state.json";
  const auto saved=readText(stateFile);
  {
   Lock lock(manager.mutex);
   expect(str(job->data,"Status")=="Awaiting confirmation","The browser offer is saved before File Info prefetch");
   if(changed)job->data["Description"]="A change since browser admission";
   expect(manager.beginPrefetch(job)==PrefetchAdmission::Started&&yes(job->data,"ConfirmationPending"),
    "Direct browser prefetch starts with its confirmation gate held");
   const auto checkpoint=readText(stateFile);
   expect(changed?checkpoint!=saved:checkpoint==saved,changed?
    "A changed browser offer still saves its prefetch admission before the worker starts":
    "An unchanged browser offer starts without rewriting the catalog");
   if(!changed){
    const auto recovery=output/L"durable-recovery";
    fs::create_directory(recovery);
    atomicText(recovery/L"state.json",checkpoint,false);
    Manager reopened(recovery);
    expect(reopened.jobs.size()==1&&str(reopened.jobs.front()->data,"Status")=="Paused"&&
     !reopened.isActive(reopened.jobs.front())&&!fs::exists(job->target()),
     "Reopening the unchanged checkpoint recovers the unconfirmed job as paused");
   }
  }
  bool confirmed=false;
  expectedTitle=L"Download File Info";
  began=GetTickCount64();
  action=[&](HWND dialog){
   if(confirmed||!hasText(dialog,L"File size: 4097 bytes"))return;
   expect(!fs::exists(job->target()),"The direct offer shows exact size before publication");
   confirmed=true;
   press(dialog,L"&Start Download");
  };
  const auto timer=SetTimer(nullptr,0,80,onTimer);
  downloadInfo(nullptr,manager,job);
  KillTimer(nullptr,timer);
  action={};
  if(!failure.empty())throw std::runtime_error(failure);
  const auto deadline=GetTickCount64()+10000;
  while(manager.isActive(job)&&GetTickCount64()<deadline)Sleep(10);
  expect(confirmed&&str(job->snapshot(),"Status")=="Complete"&&readText(job->target())==std::string(4097,1),
   "The direct offer publishes the exact displayed bytes after Start");
 }
}

static void checkPrefetchCapacity(Manager& manager,const std::string& origin){
 {
  Lock lock(manager.mutex);
  manager.state["Settings"]["Parallel"]=1;
  manager.state["Settings"]["PrefetchFileInfo"]=true;
 }
 for(const std::string route:{"capacity-reported","capacity-short-wait","capacity-cancel-wait","capacity-unknown","capacity-interrupted"}){
  const bool reported=route=="capacity-reported",interrupted=route=="capacity-interrupted";
  const bool cancelledWhileWaiting=route=="capacity-cancel-wait";
  const bool briefWait=cancelledWhileWaiting||route=="capacity-short-wait";
  Json previousRetries;
  if(interrupted){
   Lock lock(manager.mutex);
   auto& retries=manager.state["Settings"]["Retries"];
   previousRetries=retries;retries=0;
  }
  const auto folder=output/wide(route);
  fs::create_directories(folder);
  auto blocker=manager.add(origin+"/"+route+"-blocker",utf8(folder.wstring()),"blocker.zip","Main queue",true);
  expect(manager.beginPrefetch(blocker)==PrefetchAdmission::Started,"The fixture fills the only transfer slot");
  auto job=manager.add(origin+"/"+route,utf8(folder.wstring()),"download.zip","Main queue",true);
  if(reported){Lock lock(manager.mutex);job->data["BrowserReportedDirectSize"]=4000;}
  bool observedHint=false,released=false,confirmed=false,keptHeadSize=false,failed=false,cancelled=false;
  expectedTitle=L"Download File Info";
  began=GetTickCount64();
  action=[&](HWND dialog){
   if(!released){
    if(reported)observedHint|=hasText(dialog,L"Browser-reported size: 4000 bytes");
    if(briefWait){
     if(GetTickCount64()-began<400||!hasText(dialog,L"Waiting for a transfer slot"))return;
     expect(!manager.isActive(job),"The briefly queued File Info remains actionable before prefetch admission");
     if(cancelledWhileWaiting){
      cancelled=true;
      press(dialog,L"&Cancel");
     }else{
      manager.endPrefetch(blocker);
      released=true;
     }
     return;
    }
    if(GetTickCount64()-began<1000)return;
    const bool checked=hasText(dialog,L"Server-reported size: 4001 bytes");
    if(!reported&&!checked)return;
    expect(!manager.isActive(job),"File Info waits while the only prefetch slot is held");
    expect(reported?observedHint:checked,reported?"The browser size hint is visible while the transfer waits":
     "An unknown size can still be checked while the transfer waits");
    manager.endPrefetch(blocker);
    released=true;
    return;
   }
   if(interrupted){
    if(!hasText(dialog,L"File size: 4097 bytes  |  Background download stopped"))return;
    const auto snapshot=job->snapshot();
    expect(str(snapshot,"Status")=="Failed"&&num(snapshot,"Size")==4097&&!fs::exists(job->target()),
     "The compact dialog keeps the GET size after an interruption and shows the background failure before Start");
    failed=true;
    press(dialog,L"&Cancel");
    return;
   }
   if(confirmed||!manager.isActive(job))return;
   if(!reported&&hasText(dialog,L"Server-reported size: 4001 bytes")){
    Lock lock(manager.mutex);
    keptHeadSize|=str(job->data,"ResponseFileNameUrl").empty();
   }
   if(!hasText(dialog,L"File size: 4097 bytes"))return;
   {
    Lock lock(manager.mutex);
    if(num(job->data,"Received")!=4097)return;
    expect(yes(job->data,"ConfirmationPending")&&!fs::exists(job->target()),"Actual response bytes and exact File Info size precede Start without publication");
   }
   if(!reported&&!briefWait)expect(keptHeadSize,"File Info keeps the reported size during prefetch startup and replaces it with the GET size");
   confirmed=true;
   press(dialog,L"&Start Download");
  };
  const auto timer=SetTimer(nullptr,0,80,onTimer);
  downloadInfo(nullptr,manager,job);
  KillTimer(nullptr,timer);
  action={};
  if(!failure.empty())throw std::runtime_error(failure);
  if(cancelledWhileWaiting){
   manager.endPrefetch(blocker);
   expect(cancelled&&!manager.isActive(job)&&!fs::exists(job->target()),
    "Cancel during a short wait leaves no active transfer or destination file");
   continue;
  }
  if(interrupted){
   Lock lock(manager.mutex);
   manager.state["Settings"]["Retries"]=std::move(previousRetries);
   expect(failed,"The interrupted response never publishes a file");
   continue;
  }
  const auto deadline=GetTickCount64()+10000;
  while(manager.isActive(job)&&GetTickCount64()<deadline)Sleep(10);
  expect(confirmed&&str(job->snapshot(),"Status")=="Complete"&&readText(job->target())==std::string(4097,1),
   "The complete file matches the exact size shown while waiting for Start");
 }
}

static void checkUnknownLengthBrowserOffer(Manager& manager,const std::string& origin){
 auto prefs=manager.state["Settings"];
 prefs["SkipBrowserFileInfo"]=false;prefs["PrefetchFileInfo"]=true;prefs["Connections"]=1;
 prefs["CategoryFolders"]=false;prefs["DownloadFolder"]=utf8((output/L"unknown-length-files").wstring());
 manager.setSettings(prefs);
 const auto url=origin+"/unknown-length-browser";
 auto job=manager.receive({{"action","add"},{"url",url},{"filename","unknown.zip"},{"reportedDirectSize",4000}});
 const auto snapshot=[&]{Lock lock(manager.mutex);return job->snapshot();};
 bool sawHint=false,sawPending=false,confirmed=false;
 expectedTitle=L"Download File Info";began=GetTickCount64();
 action=[&](HWND dialog){
  sawHint|=hasText(dialog,L"Browser-reported size: 4000 bytes");
  if(hasText(dialog,L"Final size pending")){
   const auto record=snapshot();
   if(num(record,"Size",-1)>=0)return;
   if(!sawPending)expect(str(record,"ResponseFileNameUrl")==url&&
     !hasText(dialog,L"Browser-reported size: 4000 bytes")&&hasText(dialog,L"Unknown"),
     "The accepted GET replaces the browser hint when its length is unknown");
   sawPending=true;
  }
  if(confirmed||!hasText(dialog,L"File size: 4097 bytes"))return;
  const auto record=snapshot();
  expect(sawHint&&sawPending&&num(record,"Received")==4097&&yes(record,"ConfirmationPending")&&
   !fs::exists(job->target()),"The actual EOF size replaces the pending state before Start");
  confirmed=true;
  press(dialog,L"&Start Download");
 };
 const auto timer=SetTimer(nullptr,0,80,onTimer);
 downloadInfo(nullptr,manager,job);
 KillTimer(nullptr,timer);
 action={};
 if(!failure.empty())throw std::runtime_error(failure);
 const auto deadline=GetTickCount64()+10000;
 while(manager.isActive(job)&&GetTickCount64()<deadline)Sleep(10);
 expect(confirmed&&str(snapshot(),"Status")=="Complete"&&readText(job->target())==std::string(4097,1),
  "Start publishes exactly the bytes counted from the unknown-length response");
}

static void checkFileInfoExplicitRetry(const std::string& origin,int status){
 const bool challenge=status==200;
 const auto route=challenge?"verification-retry":status==403?"forbidden-retry":"expired-retry";
 const auto state=output/wide(std::string(route)+"-state");
 const auto url=origin+"/"+route;
 const auto label=challenge?std::wstring(L"Complete website verification in your browser"):
  wide("HTTP "+std::to_string(status)+" - Refresh the address if the link expired");
 const auto rejected=[=](const Json& record){
  return num(record,"LastHttpStatus")==status&&yes(record,"BrowserVerificationRequired")==challenge;
 };
 const auto show=[](Manager& owner,const JobPtr& job,std::function<void(HWND)> inspect){
  expectedTitle=L"Download File Info";began=GetTickCount64();action=std::move(inspect);
  const auto timer=SetTimer(nullptr,0,80,onTimer);
  downloadInfo(nullptr,owner,job);
  KillTimer(nullptr,timer);action={};
  if(!failure.empty())throw std::runtime_error(failure);
 };
 std::string id;
 {
  Manager manager(state);
  auto prefs=manager.state["Settings"];
  prefs["SkipBrowserFileInfo"]=false;prefs["PrefetchFileInfo"]=true;prefs["Connections"]=1;
  prefs["CategoryFolders"]=false;prefs["DownloadFolder"]=utf8((output/wide(std::string(route)+"-files")).wstring());
  manager.setSettings(prefs);
  auto job=manager.receive({{"action","add"},{"url",url},{"filename","retry.zip"}});
  id=job->id();
  show(manager,job,[&](HWND dialog){
   if(!hasText(dialog,label.c_str())||manager.isActive(job)||GetTickCount64()-began<500)return;
   const auto snapshot=job->snapshot();
   expect(rejected(snapshot)&&str(snapshot,"Status")=="Failed"&&!fs::exists(job->target()),
    ("File Info shows retry guidance for HTTP "+std::to_string(status)).c_str());
   press(dialog,L"&Cancel");
  });
  expect(rejected(job->data),"Dismissing File Info preserves the HTTP rejection");
 }
 Manager reopened(state);
 auto job=reopened.jobs.at(0);
 expect(job->id()==id&&rejected(job->data),"Reopening the catalog retains the HTTP rejection");
 show(reopened,job,[&](HWND dialog){
  if(!hasText(dialog,label.c_str())||GetTickCount64()-began<500)return;
  expect(!reopened.isActive(job)&&rejected(job->data)&&!fs::exists(job->target()),
   "Reopened File Info keeps the HTTP guidance without starting prefetch");
  press(dialog,L"&Start Download");
 });
 const auto deadline=GetTickCount64()+10000;
 while(GetTickCount64()<deadline){
  bool busy;
  {
   Lock lock(reopened.mutex);
   busy=reopened.isActive(job)||str(job->data,"Status")=="Queued";
  }
  if(!busy)break;
  reopened.tick();
  Sleep(10);
 }
 {
  Lock lock(reopened.mutex);
  expect(str(job->data,"Status")=="Complete"&&num(job->data,"LastHttpStatus")==0&&
   !yes(job->data,"BrowserVerificationRequired")&&readText(job->target())==std::string(4097,1),
   "Explicit Start retries and publishes the verified file");
 }
}

constexpr i64 refreshFixtureSize=512*1024,refreshFixtureSaved=256*1024;
constexpr auto refreshFixtureDate="Wed, 21 Oct 2015 07:28:00 GMT";
static fs::path seedPartialAddressRefresh(Manager& manager,const JobPtr& job,bool dateOnly=false){
 const auto folder=manager.root/L"parts"/wide(job->id());
 fs::create_directories(folder);
 const auto part=folder/(wide(job->id())+L".data");
 Bytes data(refreshFixtureSize,0);
 std::fill_n(data.begin(),refreshFixtureSaved,1);
 writeBytes(part,data);
 job->data["Size"]=refreshFixtureSize;
 job->data["Received"]=refreshFixtureSaved;
 if(dateOnly){
  job->data["Modified"]=refreshFixtureDate;
  job->data["HttpStrongLastModified"]=refreshFixtureDate;
 }else job->data["ETag"]="\"refresh-fixture\"";
 job->data["RangeSupported"]=true;
 job->data["Segments"]=Json::array({{{"Index",0},{"Start",0},{"End",refreshFixtureSize-1},{"Done",refreshFixtureSaved}}});
 manager.save();
 return part;
}

static void checkAddressRefreshInsideFileInfo(const std::string& origin,const std::string& suffix){
 const bool weakDate=suffix=="-date-legacy";
 const bool shortWait=suffix=="-capacity-short",longWait=suffix=="-capacity-long";
 const bool waitForSlot=shortWait||longWait,chunked=suffix=="-chunked";
 const bool partial=!suffix.empty()&&!waitForSlot&&!chunked,dateOnly=weakDate||suffix=="-date";
 Manager manager(output/wide("refresh-expired-state"+suffix));
 auto prefs=manager.state["Settings"];
 prefs["SkipBrowserFileInfo"]=false;prefs["PrefetchFileInfo"]=true;prefs["Connections"]=1;
 if(dateOnly){prefs["Retries"]=0;prefs["IgnoreLastModified"]=true;}
 if(waitForSlot)prefs["Parallel"]=1;
 prefs["CategoryFolders"]=false;prefs["DownloadFolder"]=utf8((output/wide("refresh-expired-files"+suffix)).wstring());
 manager.setSettings(prefs);
 const auto oldAddress=origin+"/refresh-expired-original"+suffix;
 const auto freshAddress="http://localhost:"+std::to_string(Url(origin).port)+"/refresh-expired-replacement"+suffix;
 const auto finalSize=partial?refreshFixtureSize:4097;
 const auto sizeLabel=wide("File size: "+std::to_string(finalSize)+" bytes");
 auto job=manager.receive({{"action","add"},{"url",oldAddress},{"filename","expired.zip"},
  {"headers",{{"Authorization","Basic b2xkOnNlY3JldA=="}}}});
 if(dateOnly){
  bool interrupted=false;
  try{transfer(manager,job,std::make_shared<Cancel>());}
  catch(const std::runtime_error&){interrupted=true;}
  expect(interrupted&&num(job->data,"Received")==refreshFixtureSaved&&
   str(job->data,"HttpStrongLastModified")== (weakDate?"":refreshFixtureDate),
   "The original response saves partial bytes and records a modification date only when strong");
 }else if(partial)seedPartialAddressRefresh(manager,job);
 const auto draft=job->target().parent_path()/L"kept-draft.zip";
 const auto snapshot=[&]{Lock lock(manager.mutex);return job->snapshot();};
 JobPtr blocker;
 ULONGLONG refreshedAt=0;
 bool cancelled=false,refreshed=false,confirmed=false,sawUnknownResponse=false;
 expectedTitle=L"Download File Info";began=GetTickCount64();
 action=[&](HWND dialog){
  if(!refreshed){
   if(!hasText(dialog,L"HTTP 410 - Refresh the address if the link expired"))return;
   auto destination=children(dialog,L"ComboBox").at(1);
   SetWindowTextW(destination,draft.c_str());
   const auto parentAction=action;
   expectedTitle=L"Refresh download address";
   action=[&](HWND refresh){
    expect(!hasText(refresh,L"Save and resume"),"Refresh opened from File Info leaves Start in the parent dialog");
    if(!cancelled){press(refresh,L"Cancel");return;}
    if(waitForSlot){
     blocker=manager.add(origin+"/capacity-refresh-blocker",utf8(draft.parent_path().wstring()),"blocker.zip","Main queue",true);
     expect(manager.beginPrefetch(blocker)==PrefetchAdmission::Started,"Refreshing File Info starts while the only transfer slot is held");
    }
    auto edits=children(refresh,L"Edit");
    SetWindowTextW(edits.at(1),wide(freshAddress).c_str());
    press(refresh,L"Save address");
   };
   press(dialog,L"&Refresh...");
   expectedTitle=L"Download File Info";action=parentAction;
   if(!failure.empty())throw std::runtime_error(failure);
   expect(windowText(destination)==draft.wstring(),"Returning from refresh preserves the File Info destination draft");
   if(!cancelled){
    expect(str(snapshot(),"Url")==oldAddress,"Cancel leaves the rejected address on the original record");
    cancelled=true;
   }else{
    expect(str(snapshot(),"Url")==freshAddress&&hasText(dialog,wide(freshAddress).c_str())&&
     readHeaders(snapshot()).empty(),"Saving replaces the displayed URL and removes the old origin's login");
    if(waitForSlot)expect(hasText(dialog,L"Waiting for a transfer slot"),"File Info shows that the replacement is waiting for a slot");
    refreshedAt=GetTickCount64();
    refreshed=true;
   }
   return;
  }
  if(blocker){
   if(GetTickCount64()-refreshedAt<(longWait?1700:400))return;
   if(longWait){
    if(hasText(dialog,L"More >>"))press(dialog,L"More >>");
    auto details=child(dialog,L"Refresh details");
    expect(details&&!IsWindowEnabled(details)&&!hasText(dialog,L"Server-reported size: 4097 bytes"),
     "A long wait keeps metadata lookup disabled for the replacement");
    expect(!hasText(dialog,L"&Preview"),"The waiting replacement cannot start a remote ZIP preview");
   }
   expect(!manager.isActive(job),"The replacement remains queued until the occupied slot is released");
   manager.endPrefetch(blocker);
   blocker.reset();
   return;
  }
  if(chunked&&hasText(dialog,L"Final size pending"))sawUnknownResponse=true;
  if(shortWait&&hasText(dialog,L"Response size: 4097 bytes")){
   const auto record=snapshot();
   expect(yes(record,"RefreshPendingValidation")&&yes(record,"ConfirmationPending")&&
    num(record,"Received")==0&&!fs::exists(draft),
    "Replacement headers show the response size before any body bytes arrive");
   confirmed=true;
   press(dialog,L"&Start Download");
   return;
  }
  if(!hasText(dialog,sizeLabel.c_str()))return;
  expect(!shortWait,"The slow replacement must show its response size before EOF");
  if(chunked)expect(sawUnknownResponse,"A lengthless replacement remains pending until clean EOF");
  const auto record=snapshot();
  expect(!fs::exists(draft)&&yes(record,"ConfirmationPending")&&num(record,"Received")==finalSize,
   "The fresh response waits for Start in the same File Info dialog");
  confirmed=true;press(dialog,L"&Start Download");
 };
 const auto timer=SetTimer(nullptr,0,80,onTimer);
 downloadInfo(nullptr,manager,job);
 KillTimer(nullptr,timer);action={};
 if(!failure.empty())throw std::runtime_error(failure);
 const auto deadline=GetTickCount64()+10000;
 while(manager.isActive(job)&&GetTickCount64()<deadline)Sleep(10);
 expect(confirmed&&job->target()==draft&&str(snapshot(),"Status")=="Complete"&&
  yes(snapshot(),"ContinuedFilePrefetch")&&readText(draft)==std::string(static_cast<size_t>(finalSize),1),
  "Start reuses the replacement response and publishes it at the drafted destination");
}

static void checkUnsafeRefreshedRange(const std::string& origin){
 Manager manager(output/L"refresh-unsafe-state");
 for(const std::string route:{"changed-tag","changed-overlap","changed-date","weak-date"}){
  auto job=manager.add(origin+"/refresh-expired-original",utf8(output.wstring()),route+".zip");
  const auto part=seedPartialAddressRefresh(manager,job,route=="changed-date"||route=="weak-date");
  const auto saved=fileHash(part);
  manager.refreshAddress(job,origin+"/refresh-expired-"+route);
  const auto plan=job->data["Segments"];
  std::string error;
  try{transfer(manager,job,std::make_shared<Cancel>());}
  catch(const std::runtime_error& e){error=e.what();}
  const auto diagnostic=route=="changed-overlap"?"The old partial download was kept":"Saved partial data was kept";
  expect(error.find(diagnostic)!=std::string::npos&&fileHash(part)==saved&&job->data["Segments"]==plan&&
   num(job->data,"Received")==refreshFixtureSaved&&yes(job->data,"RefreshPendingValidation")&&!fs::exists(job->target()),
   "Changed replacement leaves the saved file and range plan intact");
 }
}

static void checkRefreshedAddressSize(Manager& manager,const std::string& origin){
 constexpr i64 oldSize=10*1024*1024;
 const auto staleLabel=wide(bytes(oldSize));
 for(bool prefetch:{false,true}){
  {Lock lock(manager.mutex);manager.state["Settings"]["PrefetchFileInfo"]=prefetch;}
  const std::string route=prefetch?"refresh-size-prefetch":"refresh-size-manual-start";
  const auto folder=output/wide(route);
  fs::create_directories(folder);
  auto job=manager.add(origin+"/old-"+route,utf8(folder.wstring()),"download.zip","Main queue",true);
  {Lock lock(manager.mutex);job->data["Size"]=oldSize;}
  manager.refreshAddress(job,origin+"/"+route);
  bool observedPending=false,finished=false;
  expectedTitle=L"Download File Info";
  began=GetTickCount64();
  action=[&](HWND dialog){
   if(hasText(dialog,staleLabel.c_str()))throw std::runtime_error("The previous address size is still visible in File Info.");
   observedPending|=hasText(dialog,L"Unknown")&&hasText(dialog,L"Size pending address validation");
   if(finished||(prefetch?!hasText(dialog,L"File size: 4097 bytes"):GetTickCount64()-began<1000))return;
   expect(observedPending,"A refreshed address shows no old size before the new response");
   const auto record=job->snapshot();
   if(prefetch){
    expect(!yes(record,"RefreshPendingValidation")&&yes(record,"ConfirmationPending")&&
     num(record,"Received")==4097&&!fs::exists(job->target()),"The verified replacement size appears before Start and publication");
   }else{
    expect(hasText(dialog,L"Size pending address validation")&&yes(record,"RefreshPendingValidation")&&num(record,"Size")==oldSize,
     "With prefetch off, the replacement size remains pending until Start");
   }
   finished=true;
   press(dialog,L"&Start Download");
  };
  const auto timer=SetTimer(nullptr,0,80,onTimer);
  downloadInfo(nullptr,manager,job);
  KillTimer(nullptr,timer);
  action={};
  if(!failure.empty())throw std::runtime_error(failure);
  expect(finished,"The replacement can be started with or without prefetch");
  const auto deadline=GetTickCount64()+10000;
  while(GetTickCount64()<deadline){
   const auto state=str(job->snapshot(),"Status");
   if(!manager.isActive(job)&&state!="Queued")break;
   manager.tick();
   Sleep(10);
  }
  expect(str(job->snapshot(),"Status")=="Complete"&&readText(job->target())==std::string(4097,1),
   "The refreshed download publishes the complete response");
 }
}

static void checkPrefixedAudioSize(Manager& manager,const std::string& origin,bool declaredLength,bool replacement=false){
 constexpr i64 outputSize=512*1024+1;
 std::string route=declaredLength?"prefixed-audio":"prefixed-audio-unknown";
 if(replacement)route+="-refreshed";
 const auto folder=output/wide(route);
 fs::create_directories(folder);
 {
  Lock lock(manager.mutex);
  manager.state["Settings"]["PrefetchFileInfo"]=true;
  manager.state["Settings"]["CategoryFolders"]=false;
  manager.state["Settings"]["DownloadFolder"]=utf8(folder.wstring());
 }
 auto token=guid();
 for(int index:{20,16,12,8})token.insert(index,"-");
 auto job=manager.receive({{"action","add"},{"mediaToken",std::to_string(epoch())+"-"+token},
  {"url","https://rr1.googlevideo.com/videoplayback?id=fixture"},{"youtubeDirectAudio",true},
  {"youtubePage","https://www.youtube.com/watch?v=abcdefghijk"},{"filename","fixture.m4a"}});
 expect(str(job->snapshot(),"FormatDescription")=="Browser capture - AAC audio (M4A)",
  "An admitted direct YouTube audio selection uses media content handling");
 // Only the fixture substitutes an owned origin for the admitted playback URL.
 if(replacement){
  manager.refreshAddress(job,origin+"/"+route);
  expect(yes(job->snapshot(),"RefreshPendingValidation"),"The direct audio replacement starts with address validation pending");
 }else{
  Lock lock(manager.mutex);job->data["Url"]=origin+"/"+route;
 }
 bool responseSeen=false,partialSeen=false,confirmed=false;
 const auto expectedLabel=wide("Exact final size: "+std::to_string(outputSize)+" bytes");
 expectedTitle=L"Download File Info";
 began=GetTickCount64();
 action=[&](HWND dialog){
  const auto data=job->snapshot();
  const auto responseSize=num(data,"Size",-1);
  const auto received=num(data,"Received");
  partialSeen|=received>0&&received<outputSize;
  if(responseSize>outputSize)
   responseSeen|=hasText(dialog,wide("Response size: "+std::to_string(responseSize)+" bytes").c_str());
  if(confirmed||!hasText(dialog,expectedLabel.c_str()))return;
  expect(responseSize>outputSize&&partialSeen&&
   (declaredLength?responseSeen&&received<responseSize:received==responseSize),
   declaredLength?"File Info shows the exact saved size while the declared body is still arriving":
   "An unbounded response shows its exact saved size only after clean EOF");
  if(replacement)expect(yes(data,"RefreshPendingValidation")==declaredLength,
   "The declared replacement proves the output size during transfer and the unknown-length replacement waits for EOF");
  expect(yes(data,"ConfirmationPending")&&!fs::exists(job->target())&&!data.contains("StrippedPrefixBytes"),
   "Checking the final size does not trim or publish before Start");
  confirmed=true;
  press(dialog,L"&Start Download");
 };
 const auto timer=SetTimer(nullptr,0,80,onTimer);
 downloadInfo(nullptr,manager,job);
 KillTimer(nullptr,timer);
 action={};
 if(!failure.empty())throw std::runtime_error(failure);
 const auto deadline=GetTickCount64()+10000;
 while(manager.isActive(job)&&GetTickCount64()<deadline)Sleep(10);
 const auto final=job->snapshot();
 const auto expected=std::string("\0\0\0\x18" "ftypM4A ",12)+std::string(outputSize-12,1);
 expect(confirmed&&str(final,"Status")=="Complete"&&num(final,"Size")==outputSize&&
  num(final,"StrippedPrefixBytes")>0&&readText(job->target())==expected,
  "Start saves exactly the prefetched size and the original M4A payload bytes");
}

static void checkBrowserMetadataWithoutExtraProbe(const std::string& origin){
 Manager manager(output/L"browser-metadata");
 manager.state["Settings"]["PrefetchFileInfo"]=false;
 manager.state["Settings"]["CategoryFolders"]=false;
 manager.state["Settings"]["DownloadFolder"]=utf8((output/L"browser-metadata-files").wstring());
 for(const std::string mode:{"cancel","start","manual","size-only"}){
  Json message={{"action","add"},{"url",origin+"/browser-metadata-"+mode},
                {"filename",mode+".zip"},{"reportedDirectSize",4000}};
  if(mode!="size-only")message["reportedServerName"]="browser.zip";
  auto job=manager.receive(message);
  const auto destination=job->target();
  bool complete=false,asked=false;
  expectedTitle=L"Download File Info";
  began=GetTickCount64();
  action=[&](HWND dialog){
   if(GetTickCount64()-began<800)return;
   if(mode=="size-only"||asked){
    if(!hasText(dialog,L"Server-reported size: 4097 bytes"))return;
    expect(hasText(dialog,L"Server name: server.zip"),"The metadata lookup still supplies a server filename");
   }else{
    expect(hasText(dialog,L"Browser-reported size: 4000 bytes")&&hasText(dialog,L"Server name: browser.zip"),
     "File Info keeps both browser hints without replacing them with a speculative lookup");
    expect(!manager.isActive(job)&&!fs::exists(destination),"Opening the dialog with prefetch disabled publishes nothing");
    if(mode=="manual"){
     press(dialog,L"More >>");
     press(dialog,L"Refresh details");
     asked=true;
     return;
    }
   }
   complete=true;
   press(dialog,mode=="start"?L"&Start Download":L"&Cancel");
  };
  const auto timer=SetTimer(nullptr,0,80,onTimer);
  downloadInfo(nullptr,manager,job);
  KillTimer(nullptr,timer);
  action={};
  if(!failure.empty())throw std::runtime_error(failure);
  expect(complete,"The browser metadata dialog exercised its intended action");
  if(mode=="start"){
   const auto deadline=GetTickCount64()+10000;
   while((str(job->snapshot(),"Status")=="Queued"||manager.isActive(job))&&GetTickCount64()<deadline){
    manager.tick();Sleep(10);
   }
   expect(str(job->snapshot(),"Status")=="Complete"&&num(job->snapshot(),"Size")==4097&&readText(destination)==std::string(4097,1),
    "Start downloads the actual response and replaces the browser size hint with the verified output length");
  }else expect(!fs::exists(destination),"Closing File Info never publishes the browser-reported file");
 }
}
class FilenamePreviewUiApplication:public CWinApp {
 int code=1;
public:BOOL InitInstance()override{
 CWinApp::InitInstance();INITCOMMONCONTROLSEX common{sizeof(common),ICC_WIN95_CLASSES|ICC_DATE_CLASSES};InitCommonControlsEx(&common);AfxOleInit();int argc=0;auto argv=CommandLineToArgvW(GetCommandLineW(),&argc);
 try{
  if((argc!=3&&argc!=4)||fs::exists(argv[1])||(argc==4&&fs::exists(argv[3])))throw std::runtime_error("Fresh fixture and local HTTP URL required");output=argv[1];fs::create_directories(output);Manager manager(output/L"state");manager.state["Settings"]["QueuePromptLater"]=false;manager.state["Settings"]["PrefetchFileInfo"]=false;
  constexpr i64 streamBytes=512*1024;
  for(const std::string mode:{"save","cancel","collision","collision-alias","replacement","missing","stale","prefetch","start-complete","start-stream","start-move-stream","start-cross-stream","later-complete","later-damaged","later-missing","later-checksum"}){
   if(mode=="start-cross-stream"&&argc!=4)continue;
   const bool later=mode.rfind("later-",0)==0;
   const bool streaming=mode.find("stream")!=std::string::npos;
   const bool startNow=mode.rfind("start-",0)==0;
   manager.state["Settings"]["PrefetchFileInfo"]=(mode=="prefetch"||startNow||later);
   auto folder=output/wide(mode);fs::create_directories(folder);auto alternate=mode=="start-cross-stream"?fs::path(argv[3])/L"new-folder":mode=="start-move-stream"?folder/L"new-folder":startNow?folder:folder/L"chosen-folder";if(mode!="start-move-stream"&&mode!="start-cross-stream")fs::create_directories(alternate);
   auto job=manager.add(utf8(argv[2])+"/"+mode,utf8(folder.wstring()),"my-name.zip","Main queue",true);
   if(mode=="replacement")job->data["ReplacementOf"]="fixture-original";
   if(mode=="collision")atomicText(alternate/L"server.zip","keep existing bytes",false);
   if(mode=="collision-alias"){
    const auto reserved=manager.add(utf8(argv[2])+"/reserved",utf8(alternate.wstring()),"server.zip","Main queue",true);
    reserved->data["PreviousPath"]=utf8((alternate/L"previous.zip").wstring());
    fs::create_directory(alternate/L"child");
    alternate/=L"child/..";
    const auto numbered=manager.add(utf8(argv[2])+"/numbered",utf8(alternate.generic_wstring()),"SERVER.zip","Main queue",true);
    expect(str(numbered->data,"FileName")=="SERVER (1).zip"&&!fs::exists(reserved->target()),
     "New download numbering protects an unpublished destination through an aliased folder");
   }
   const auto original=job->target();bool edited=false,done=false,warned=false;
   expectedTitle=L"Download File Info";began=GetTickCount64();
   action=[&](HWND w){
    auto combos=children(w,L"ComboBox");expect(combos.size()>=2,"Destination combo is present");auto destination=combos[1];
    if(!edited){SetWindowTextW(destination,(alternate/L"typed-name.zip").c_str());edited=true;return;}
    auto button=child(w,L"Use server name");
    if(mode=="replacement"){
     bool ready=false;for(HWND c=GetWindow(w,GW_CHILD);c;c=GetWindow(c,GW_HWNDNEXT))ready|=windowText(c)==L"No server filename available.";
     if(!ready)return;expect(!IsWindowEnabled(button),"Replacement cannot adopt another destination name");
    }else{
     const std::wstring suggestion=mode=="missing"?L"missing.zip":L"server.zip";
     const auto label=L"Server name: "+suggestion;
     bool ready=false;for(HWND c=GetWindow(w,GW_CHILD);c;c=GetWindow(c,GW_HWNDNEXT))ready|=windowText(c)==label;
     if(!ready)return;
     if(later&&!job->data.contains("CompletedPrefetch"))return;
     if(mode=="start-complete"&&num(job->data,"Received")<4097)return;
     if(streaming&&num(job->data,"Received")<2048)return;
     if(streaming)expect(num(job->data,"Received")<streamBytes,"Start is clicked while the original response body is still arriving");
     if(startNow)expect(num(job->data,"Received")>0&&!fs::exists(original),"Real prefetch bytes arrive without publishing before Start");
     expect(windowText(destination)==(alternate/L"typed-name.zip").wstring(),"Late metadata preserves user filename and folder");
     expect(job->target()==original,"Preview alone never renames the catalog entry");
     if(mode=="stale"){
      const auto savedRecord=job->data;
      job->data["Url"]=utf8(argv[2])+"/changed";press(w,L"Use server name");
      expect(windowText(destination)==(alternate/L"typed-name.zip").wstring(),"Stale metadata cannot rename a changed URL");
      job->data=savedRecord;
      // Deliver the response metadata before the dialog processes its next timer.
      job->data["ConfirmationPending"]=true;
      job->data["ResponseFileNameUrl"]=str(job->data,"Url");
      job->data["ResponseFileName"]="new-server.zip";
      press(w,L"Use server name");
      expect(windowText(destination)==(alternate/L"typed-name.zip").wstring()&&hasText(w,label.c_str()),
       "A click on the displayed preview name cannot silently adopt a different response name");
      SendMessageW(w,WM_TIMER,7,0);
      expect(hasText(w,L"Server name: new-server.zip"),"The latest response name becomes visible before it can be adopted");
      press(w,L"Use server name");
      expect(windowText(destination)==(alternate/L"new-server.zip").wstring()&&job->target()==original,
       "An explicit click on the refreshed suggestion adopts that name in the current folder");
      job->data=savedRecord;
     }else if(mode=="collision"||mode=="collision-alias"){
      auto oldAction=action;expectedTitle=L"UDM";action=[&](HWND warning){warned=true;press(warning,L"OK");};press(w,L"Use server name");expectedTitle=L"Download File Info";action=oldAction;
      expect(warned&&windowText(destination)==(alternate/L"typed-name.zip").wstring(),"Occupied destination rejects suggestion without changing the draft");
      if(mode=="collision-alias"){
       for(const auto* name:{L"server.zip",L"previous.zip"}){
        warned=false;
        const auto path=alternate/name;
        SetWindowTextW(destination,path.c_str());
        expectedTitle=L"UDM";
        action=[&](HWND warning){
         warned=hasText(warning,L"That destination belongs to another download.");
         press(warning,L"OK");
        };
        press(w,L"Download &Later");
        expectedTitle=L"Download File Info";
        action=oldAction;
        expect(warned&&IsWindow(w)&&windowText(destination)==path.wstring()&&job->target()==original,
         "Download Later keeps the dialog and draft when an alias reaches a catalog reservation");
       }
      }
     }else{
      press(w,L"Use server name");expect(windowText(destination)==(alternate/suggestion).wstring(),"Explicit adoption preserves the selected folder");
      expect(job->target()==original,"Adoption stays a draft until confirmation");
     }
    }
    if(mode=="start-complete"){
     bool invalidFolder=false;try{manager.confirmPrefetchedDownload(job,{{"Folder","relative-folder"}});}catch(...){invalidFolder=true;}
     expect(invalidFolder&&yes(job->data,"ConfirmationPending"),"Invalid destination folder preserves the confirmation gate");
     const auto reservedFolder=folder/L"reserved-folder";
     fs::create_directories(reservedFolder/L"child");
     const auto reserved=manager.add(utf8(argv[2])+"/reserved",utf8(reservedFolder.wstring()),"server.zip","Main queue",true);
     bool occupied=false;
     try{
      manager.confirmPrefetchedDownload(job,{{"Folder",utf8((reservedFolder/L"child/..").generic_wstring())},{"FileName","SERVER.zip"}});
     }catch(const std::runtime_error& error){occupied=std::string_view(error.what())=="That destination belongs to another download.";}
     expect(occupied&&yes(job->data,"ConfirmationPending")&&job->target()==original&&!fs::exists(reserved->target()),
      "Prefetch Start refuses the aliased destination without publishing or releasing confirmation");
     bool rejected=false;try{manager.confirmPrefetchedDownload(job,{{"ExpectedSha256",std::string(64,'0')}});}catch(...){rejected=true;}
     expect(rejected&&yes(job->data,"ConfirmationPending"),"In-flight confirmation rejects transfer-property edits");
     {
      Handle held;
      const auto lockDeadline=GetTickCount64()+500;
      do{
       held.h=CreateFileW((manager.root/L"state.json").c_str(),GENERIC_READ,FILE_SHARE_READ,nullptr,OPEN_EXISTING,0,nullptr);
       if(held||GetLastError()!=ERROR_SHARING_VIOLATION)break;
       Sleep(10);
      }while(GetTickCount64()<lockDeadline);
      expect(static_cast<bool>(held),"Rollback fixture locks the catalog before attempting confirmation");
      bool failed=false;try{manager.confirmPrefetchedDownload(job,{{"FileName","failed-save.zip"},{"Folder",utf8((folder/L"failed-save-folder").wstring())}});}catch(...){failed=true;}
      expect(held&&failed&&yes(job->data,"ConfirmationPending")&&job->target()==original&&!fs::exists(folder/L"failed-save-folder")&&!yes(job->data,"ContinuedFilePrefetch"),"Failed durable confirmation restores the original name and publication gate");
     }
    }
    capture(w,wide(mode+".png").c_str());done=true;press(w,startNow?L"&Start Download":(mode=="save"||mode=="prefetch"||later)?L"Download &Later":L"&Cancel");
   };
   auto timer=SetTimer(nullptr,0,80,onTimer);downloadInfo(nullptr,manager,job);KillTimer(nullptr,timer);action={};if(!failure.empty())throw std::runtime_error(failure);
   expect(done,"Metadata arrived while the native dialog remained responsive");
   expect(job->target()==((mode=="save"||mode=="prefetch"||startNow||later)?alternate/L"server.zip":original),"Save commits suggestion and Cancel preserves original catalog name");
   if(later){
    expect(!fs::exists(job->target())&&job->data.contains("CompletedPrefetch"),"Download Later retains verified complete response without publication");
    const auto id=wide(job->id());
    const auto staged=manager.root/L"parts"/id/(id+L".data");
    if(mode=="later-damaged"){
     expect(fs::is_regular_file(staged),"The saved single-file prefetch exists before damage");
     atomicText(staged,std::string(4097,2),false);
    }
    if(mode=="later-missing")expect(fs::remove(staged),"Remove the actual saved prefetch before resuming");
    if(mode=="later-checksum")manager.configure(job,{{"ExpectedSha256",std::string(64,'0')}});
    auto resumeAndWait=[](Manager& owner,JobPtr item){owner.resume(item);const auto deadline=GetTickCount64()+10000;while((str(item->data,"Status")=="Queued"||owner.isActive(item))&&GetTickCount64()<deadline){owner.tick();Sleep(10);}};
    if(mode=="later-complete"){
     Manager reopened(manager.root);JobPtr restored;for(auto item:reopened.jobs)if(item->id()==job->id())restored=item;
     expect(restored&&restored->data.contains("CompletedPrefetch"),"Completed response receipt survives catalog reopening");
     resumeAndWait(reopened,restored);job->data=restored->snapshot();manager.save();
    }else resumeAndWait(manager,job);
    if(mode=="later-damaged")expect(str(job->data,"Status")=="Failed"&&!fs::exists(job->target()),"Changed completed prefetch cannot publish unverified bytes");
    else if(mode=="later-missing")expect(str(job->data,"Status")=="Failed"&&!fs::exists(job->target()),"Missing completed prefetch fails without publishing a partial file");
    else if(mode=="later-checksum")expect(str(job->data,"Status")=="Failed"&&yes(job->data,"ReusedCompletedPrefetch")&&!fs::exists(job->target())&&str(job->data,"Error").find("SHA-256 mismatch")!=std::string::npos,"Reused prefetch still enforces the user's expected final checksum");
    else expect(str(job->data,"Status")=="Complete"&&yes(job->data,"ReusedCompletedPrefetch")&&readText(job->target())==std::string(4097,1),"Download Later resumes exact completed bytes without another request");
   }else if(startNow){
    const auto deadline=GetTickCount64()+10000;while(manager.isActive(job)&&GetTickCount64()<deadline)Sleep(10);
    expect(str(job->data,"Status")=="Complete"&&yes(job->data,"ContinuedFilePrefetch"),"Start completes ordinary prefetch without stopping its worker");
    expect(num(job->data,"PrefetchedBytesAtStart")>0&&num(job->data,"PrefetchPeakRate")>0,
     "Start retains the prefetched byte count and measured rate");
    expect(!fs::exists(original),"Prefetch never publishes into the original destination after confirmation changes it");
    expect(readText(job->target())==std::string(streaming?streamBytes:4097,1),"Final file exactly matches prefetched response bytes under the chosen server name");
   }else expect(!fs::exists(job->target()),"Preview and Download Later never create destination file data");
  }
  for(bool prefetch:{false,true})checkFileInfoScheduleLimits(utf8(argv[2]),prefetch);
  checkBrowserOfferDismissal(utf8(argv[2]));
  checkPrefetchCapacity(manager,utf8(argv[2]));
  checkBrowserMetadataWithoutExtraProbe(utf8(argv[2]));
  checkSabrDisplayAddress(manager);
  checkDurableDirectOffer(utf8(argv[2]));
  checkRefreshedAddressSize(manager,utf8(argv[2]));
  checkUnknownLengthBrowserOffer(manager,utf8(argv[2]));
  for(int status:{200,403,410})checkFileInfoExplicitRetry(utf8(argv[2]),status);
  for(const auto* suffix:{"","-partial","-date","-date-legacy","-capacity-short","-capacity-long","-chunked"})checkAddressRefreshInsideFileInfo(utf8(argv[2]),suffix);
  checkUnsafeRefreshedRange(utf8(argv[2]));
  for(bool streaming:{false,true})checkPrefetchedZipPreview(utf8(argv[2]),streaming);
  checkPrefixedAudioSize(manager,utf8(argv[2]),true);
  checkPrefixedAudioSize(manager,utf8(argv[2]),false);
  checkPrefixedAudioSize(manager,utf8(argv[2]),true,true);
  checkPrefixedAudioSize(manager,utf8(argv[2]),false,true);
  checkRecentDestination(manager,utf8(argv[2]));
  code=0;
 }catch(const std::exception& e){failure=e.what();}if(argv)LocalFree(argv);if(!output.empty())atomicText(output/L"results.json",Json{{"passed",code==0},{"error",failure},{"checks",results}}.dump(2),false);return FALSE;
 }
 int ExitInstance()override{AfxOleTerm(FALSE);return code;}
};FilenamePreviewUiApplication application;
}
