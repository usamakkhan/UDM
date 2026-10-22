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


class FilenamePreviewUiApplication:public CWinApp {
 int code=1;
public:BOOL InitInstance()override{
 CWinApp::InitInstance();INITCOMMONCONTROLSEX common{sizeof(common),ICC_WIN95_CLASSES|ICC_DATE_CLASSES};InitCommonControlsEx(&common);AfxOleInit();int argc=0;auto argv=CommandLineToArgvW(GetCommandLineW(),&argc);
 try{
  if((argc!=3&&argc!=4)||fs::exists(argv[1])||(argc==4&&fs::exists(argv[3])))throw std::runtime_error("Fresh fixture and local HTTP URL required");output=argv[1];fs::create_directories(output);Manager manager(output/L"state");manager.state["Settings"]["QueuePromptLater"]=false;manager.state["Settings"]["PrefetchFileInfo"]=false;
  for(const std::string mode:{"save","cancel","collision","replacement","missing","stale","prefetch","start-complete","start-stream","start-move-stream","start-cross-stream","later-complete","later-damaged","later-missing","later-checksum"}){
   if(mode=="start-cross-stream"&&argc!=4)continue;
   const bool later=mode.rfind("later-",0)==0;
   const bool streaming=mode.find("stream")!=std::string::npos;
   const bool startNow=mode.rfind("start-",0)==0;
   manager.state["Settings"]["PrefetchFileInfo"]=(mode=="prefetch"||startNow||later);
   auto folder=output/wide(mode);fs::create_directories(folder);auto alternate=mode=="start-cross-stream"?fs::path(argv[3])/L"new-folder":mode=="start-move-stream"?folder/L"new-folder":startNow?folder:folder/L"chosen-folder";if(mode!="start-move-stream"&&mode!="start-cross-stream")fs::create_directories(alternate);
   auto job=manager.add(utf8(argv[2])+"/"+mode,utf8(folder.wstring()),"my-name.zip","Main queue",true);
   if(mode=="replacement")job->data["ReplacementOf"]="fixture-original";
   if(mode=="collision")atomicText(alternate/L"server.zip","keep existing bytes",false);
   const auto original=job->target();bool edited=false,done=false,warned=false;
   expectedTitle=L"Download File Info";began=GetTickCount64();
   action=[&](HWND w){
    auto combos=children(w,L"ComboBox");expect(combos.size()>=2,"Destination combo is present");auto destination=combos[1];
    if(!edited){SetWindowTextW(destination,(alternate/L"typed-name.zip").c_str());edited=true;return;}
    auto button=child(w,L"Use server name");
    if(mode=="missing"||mode=="replacement"){
     bool ready=false;for(HWND c=GetWindow(w,GW_CHILD);c;c=GetWindow(c,GW_HWNDNEXT))ready|=windowText(c)==L"No server filename available.";
     if(!ready)return;expect(!IsWindowEnabled(button),"No header leaves server-name action disabled");
    }else{
     bool ready=false;for(HWND c=GetWindow(w,GW_CHILD);c;c=GetWindow(c,GW_HWNDNEXT))ready|=windowText(c)==L"Server name: server.zip";
     if(!ready)return;
     if(later&&!job->data.contains("CompletedPrefetch"))return;
     if(mode=="start-complete"&&num(job->data,"Received")<4097)return;
     if(streaming&&num(job->data,"Received")<2048)return;
     if(streaming)expect(num(job->data,"Received")<262144,"Start is clicked while the original response body is still arriving");
     if(startNow)expect(num(job->data,"Received")>0&&!fs::exists(original),"Real prefetch bytes arrive without publishing before Start");
     expect(windowText(destination)==(alternate/L"typed-name.zip").wstring(),"Late metadata preserves user filename and folder");
     expect(job->target()==original,"Preview alone never renames the catalog entry");
     if(mode=="stale"){
      job->data["Url"]=utf8(argv[2])+"/changed";press(w,L"Use server name");
      expect(windowText(destination)==(alternate/L"typed-name.zip").wstring(),"Stale metadata cannot rename a changed URL");
     }else if(mode=="replacement")expect(!IsWindowEnabled(button),"Replacement cannot adopt another destination name");
     else if(mode=="collision"){
      auto oldAction=action;expectedTitle=L"UDM";action=[&](HWND warning){warned=true;press(warning,L"OK");};press(w,L"Use server name");expectedTitle=L"Download File Info";action=oldAction;
      expect(warned&&windowText(destination)==(alternate/L"typed-name.zip").wstring(),"Existing file rejects suggestion without changing the draft");
     }else{
      press(w,L"Use server name");expect(windowText(destination)==(alternate/L"server.zip").wstring(),"Explicit adoption preserves the selected folder");
      expect(job->target()==original,"Adoption stays a draft until confirmation");
     }
    }
    if(mode=="start-complete"){
     bool invalidFolder=false;try{manager.confirmPrefetchedDownload(job,{{"Folder","relative-folder"}});}catch(...){invalidFolder=true;}
     expect(invalidFolder&&yes(job->data,"ConfirmationPending"),"Invalid destination folder preserves the confirmation gate");
     bool rejected=false;try{manager.confirmPrefetchedDownload(job,{{"ExpectedSha256",std::string(64,'0')}});}catch(...){rejected=true;}
     expect(rejected&&yes(job->data,"ConfirmationPending"),"In-flight confirmation rejects transfer-property edits");
     {Handle held(CreateFileW((manager.root/L"state.json").c_str(),GENERIC_READ,FILE_SHARE_READ,nullptr,OPEN_EXISTING,0,nullptr));
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
    if(mode=="later-damaged"){
     auto part=manager.root/L"parts"/wide(job->id())/L"0000.part";
     atomicText(part,std::string(4097,2),false);
    }
    if(mode=="later-missing")fs::remove(manager.root/L"parts"/wide(job->id())/L"0000.part");
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
    expect(!fs::exists(original),"Prefetch never publishes into the original destination after confirmation changes it");
    expect(readText(job->target())==std::string(streaming?262144:4097,1),"Final file exactly matches prefetched response bytes under the chosen server name");
   }else expect(!fs::exists(job->target()),"Preview and Download Later never create destination file data");
  }
  code=0;
 }catch(const std::exception& e){failure=e.what();}if(argv)LocalFree(argv);if(!output.empty())atomicText(output/L"results.json",Json{{"passed",code==0},{"error",failure},{"checks",results}}.dump(2),false);return FALSE;
 }
 int ExitInstance()override{AfxOleTerm(FALSE);return code;}
};FilenamePreviewUiApplication application;
}
