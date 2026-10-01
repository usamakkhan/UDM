// Exercises only windows created by this isolated executable.
#define UDM_CAPTURE_PRESENTATION_COMPONENT_TEST
#include "App.cpp"
#include "CaptureTransactions.hpp"
namespace udm {
static fs::path output;static Json outcomes=Json::array();static std::string failure;static ULONGLONG began;
static unsigned informationShown=0,duplicatesShown=0,refreshShown=0;static bool refreshRaceInjected=false;static Manager* raceManager=nullptr;
static unsigned reviewsShown=0;static bool revisitReviews=false;static Json reviewFixtures=Json::object();
static void admit(Manager&,const std::string&,const std::string&);
static void expect(bool value,const char* name){outcomes.push_back({{"name",name},{"passed",value}});if(!value)throw std::runtime_error(name);}
static std::wstring title(HWND w){wchar_t value[2048]{};GetWindowTextW(w,value,2048);return value;}
static HWND child(HWND w,const wchar_t* label){for(auto c=GetWindow(w,GW_CHILD);c;c=GetWindow(c,GW_HWNDNEXT))if(title(c)==label)return c;throw std::runtime_error("Missing native test control");}
static void press(HWND w,const wchar_t* label){auto c=child(w,label);expect(IsWindowEnabled(c)!=FALSE,"Native presentation button is enabled");SendMessageW(c,BM_CLICK,0,0);}
static std::wstring editTexts(HWND w){std::wstring value;for(auto c=GetWindow(w,GW_CHILD);c;c=GetWindow(c,GW_HWNDNEXT)){wchar_t type[80]{};GetClassNameW(c,type,80);if(!_wcsicmp(type,L"Edit"))value+=title(c)+L"\n";}return value;}
static void capture(HWND w,const wchar_t* name){
 ShowWindow(w,SW_SHOWNOACTIVATE);RedrawWindow(w,nullptr,nullptr,RDW_INVALIDATE|RDW_ALLCHILDREN|RDW_UPDATENOW|RDW_ERASE);CRect r;GetWindowRect(w,&r);CImage image;image.Create(r.Width(),r.Height(),32);auto dc=image.GetDC();RECT full{0,0,r.Width(),r.Height()};FillRect(dc,&full,GetSysColorBrush(COLOR_BTNFACE));SendMessageW(w,WM_PRINT,(WPARAM)dc,PRF_NONCLIENT|PRF_CLIENT|PRF_ERASEBKGND|PRF_CHILDREN);
 for(auto c=GetWindow(w,GW_CHILD);c;c=GetWindow(c,GW_HWNDNEXT)){if(!(GetWindowLongW(c,GWL_STYLE)&WS_VISIBLE))continue;CRect cr;GetWindowRect(c,&cr);auto saved=SaveDC(dc);IntersectClipRect(dc,cr.left-r.left,cr.top-r.top,cr.right-r.left,cr.bottom-r.top);SetViewportOrgEx(dc,cr.left-r.left,cr.top-r.top,nullptr);SendMessageW(c,WM_PRINT,(WPARAM)dc,PRF_NONCLIENT|PRF_CLIENT|PRF_ERASEBKGND|PRF_CHILDREN);RestoreDC(dc,saved);}
 image.ReleaseDC();if(FAILED(image.Save((output/name).c_str())))throw std::runtime_error("Cannot render the native presentation fixture");
}
static BOOL CALLBACK observe(HWND w,LPARAM){
 auto caption=title(w);if(caption!=L"Download File Info"&&caption!=L"Duplicate download link"&&caption!=L"Refresh download address"&&caption!=L"Review captured download")return TRUE;
 try{
  if(caption==L"Review captured download"){
   ++reviewsShown;auto values=editTexts(w);
   if(values.find(L"review-refresh.bin")!=std::wstring::npos){capture(w,L"review-replacement.png");press(w,L"Review replacement...");}
   else if(values.find(L"review-later.bin")!=std::wstring::npos){capture(w,revisitReviews?L"review-reopened.png":L"review-later.png");press(w,revisitReviews?L"Discard captured link":L"Later");}
   else if(values.find(L"review-discard.bin")!=std::wstring::npos){expect(!IsWindowEnabled(child(w,L"Review replacement...")),"Completed original disables replacement review");capture(w,L"review-completed.png");press(w,L"Discard captured link");}
   else if(values.find(L"review-stale.bin")!=std::wstring::npos){
    JobPtr original;for(auto j:raceManager->jobs)if(j->id()==str(reviewFixtures["review-stale.bin"],"id"))original=j;
    raceManager->remove(original);press(w,L"Review replacement...");
    expect(IsWindow(w)&&!IsWindowEnabled(child(w,L"Review replacement...")),"A removed target keeps review open and disables replacement");
    expect(raceManager->pendingBrowserCaptureReviews().size()>=1,"Stale click retains the captured request for another choice");capture(w,L"review-stale.png");press(w,L"Download new file...");
   }else{expect(values.find(L"review-new.bin")!=std::wstring::npos,"Deleted-target review shows its original file");expect(!IsWindowEnabled(child(w,L"Review replacement...")),"Deleted original disables replacement review");capture(w,L"review-deleted.png");press(w,L"Download new file...");}
  }else if(caption==L"Download File Info"){
   ++informationShown;auto values=editTexts(w);
   if(values.find(L"/review-")!=std::wstring::npos){capture(w,L"review-new-file-info.png");press(w,L"Download &Later");}
   else if(values.find(L"/start.bin")!=std::wstring::npos){capture(w,L"information-start.png");press(w,L"&Start Download");}
   else if(values.find(L"/later.bin")!=std::wstring::npos){capture(w,L"information-later.png");press(w,L"Download &Later");}
   else{capture(w,L"information-cancel.png");press(w,L"&Cancel");}
  }else if(caption==L"Duplicate download link"){
   ++duplicatesShown;capture(w,L"duplicate.png");press(w,L"Add the duplicate with a numbered file name");press(w,L"OK");
  }else{
   ++refreshShown;auto values=editTexts(w);
   if(values.find(L"/refresh-cancel.bin?fresh=1")!=std::wstring::npos){
    // The dialog already polled version one. Commit a new link immediately
    // before its Cancel click, without letting the 250 ms poll display it.
    admit(*raceManager,"refresh-cancel.bin","?fresh=2");refreshRaceInjected=true;
    capture(w,L"refresh-cancel-before-new-link.png");press(w,L"Cancel");
   }else if(values.find(L"/refresh-save.bin?fresh=1")!=std::wstring::npos){
    admit(*raceManager,"refresh-save.bin","?fresh=2");press(w,L"Save address");
    expect(IsWindow(w)&&editTexts(w).find(L"/refresh-save.bin?fresh=2")!=std::wstring::npos,"Save keeps the dialog open and displays a newly arrived address for review");
    JobPtr original;for(auto j:raceManager->jobs)if(str(j->data,"FileName")=="refresh-save.bin")original=j;
    expect(original&&str(original->data,"Url")=="http://127.0.0.1:9/refresh-save.bin","The first Save cannot apply an address that changed before the click");
    capture(w,L"refresh-save-review-new-link.png");press(w,L"Save address");
   }else if(values.find(L"/refresh-cancel.bin?fresh=2")!=std::wstring::npos){
    expect(refreshRaceInjected,"The later browser link survives cancellation of the earlier dialog");capture(w,L"refresh-new-link.png");press(w,L"Save address");
   }else{expect(values.find(L"?fresh=1")!=std::wstring::npos,"Recovered refresh dialog shows the captured replacement URL");capture(w,L"refresh.png");press(w,L"Save address");}
  }
 }catch(const std::exception& e){failure=e.what();PostMessageW(w,WM_CLOSE,0,0);}return FALSE;
}
static void CALLBACK tick(HWND,UINT,UINT_PTR,DWORD){
 if(GetTickCount64()-began>20000){failure="Presentation UI fixture deadline";EnumThreadWindows(GetCurrentThreadId(),[](HWND w,LPARAM)->BOOL{if(title(w)!=L"UDM Download Manager")PostMessageW(w,WM_CLOSE,0,0);return TRUE;},0);return;}
 EnumThreadWindows(GetCurrentThreadId(),observe,0);
}
static std::string captureToken(){auto value=guid();for(int p:{20,16,12,8})value.insert(p,"-");return std::to_string(epoch())+"-"+value;}
static void admit(Manager& m,const std::string& file,const std::string& query=""){
 auto token=captureToken();prepareCapture(m,{{"captureToken",token},{"download",{{"action","add"},{"url","http://127.0.0.1:9/"+file+query},{"filename",file}}}});commitPreparedCapture(m,token);
}
static void prepareFixtures(const fs::path& dir){
 Manager m(dir);auto p=m.state["Settings"];p["DownloadFolder"]=utf8((output/L"files").wstring());p["CategoryFolders"]=false;p["SkipBrowserFileInfo"]=false;p["PrefetchFileInfo"]=false;p["QueuePromptLater"]=false;p["DuplicatePolicy"]="Numbered";p["Sound"]=false;p["ClipboardMonitor"]=false;p["CloseToTray"]=false;p["DropBasket"]=false;m.setSettings(p);
 admit(m,"cancel.bin");admit(m,"start.bin");admit(m,"later.bin");
 m.add("http://127.0.0.1:9/existing.bin","","existing.bin");p["DuplicatePolicy"]="Existing";m.setSettings(p);admit(m,"existing.bin");
 auto complete=m.add("http://127.0.0.1:9/complete.bin","","complete.bin");fs::create_directories(complete->target().parent_path());writeBytes(complete->target(),Bytes(12,42));complete->data["Status"]="Complete";complete->data["Size"]=12;complete->data["Received"]=12;m.save();admit(m,"complete.bin");
 m.add("http://127.0.0.1:9/duplicate.bin","","duplicate.bin");p["DuplicatePolicy"]="Ask";m.setSettings(p);admit(m,"duplicate.bin");
 auto refresh=m.add("http://127.0.0.1:9/refresh.bin","","refresh.bin");m.beginAddressRefresh(refresh);admit(m,"refresh.bin","?fresh=1");
 m.cancelAddressRefresh(refresh);auto cancelled=m.add("http://127.0.0.1:9/refresh-cancel.bin","","refresh-cancel.bin");m.beginAddressRefresh(cancelled);admit(m,"refresh-cancel.bin","?fresh=1");
 m.cancelAddressRefresh(cancelled);auto changed=m.add("http://127.0.0.1:9/refresh-save.bin","","refresh-save.bin");m.beginAddressRefresh(changed);admit(m,"refresh-save.bin","?fresh=1");
 expect(m.pendingBrowserPresentations().size()==9,"Nine browser presentations are durably prepared before restart");
}
static void prepareReviewFixtures(Manager& m){
 for(const auto& name:{"review-refresh.bin","review-new.bin","review-discard.bin","review-later.bin","review-stale.bin"}){
  auto job=m.add(std::string("http://127.0.0.1:9/")+name,"",name);auto part=m.root/L"parts"/wide(job->id())/L"0000.part";fs::create_directories(part.parent_path());writeBytes(part,Bytes(71,0x6b));
  job->data["Size"]=2048;job->data["Received"]=71;job->data["Segments"]=Json::array({{{"Index",0},{"Start",0},{"End",2047},{"Done",71}}});m.save();m.beginAddressRefresh(job);
  auto token=captureToken();prepareCapture(m,{{"captureToken",token},{"download",{{"action","add"},{"url",std::string("http://127.0.0.1:9/")+name+"?fresh=1"},{"filename",name}}}});
  if(std::string(name)=="review-new.bin")m.remove(job);else if(std::string(name)=="review-discard.bin")job->data["Status"]="Complete";else job->data["Url"]=str(job->data,"Url")+"?changed=1";m.save();
  expect(str(commitPreparedCapture(m,token),"status")=="review","Changed target is retained as a native review");m.cancelAddressRefresh(job);
  reviewFixtures[name]={{"token",token},{"id",job->id()},{"part",utf8(part.wstring())},{"hash",fileHash(part)}};
 }
}
class CapturePresentationComponentTest {
public:static void run(Manager& manager,MainWindow& frame){
 frame.KillTimer(1);frame.ShowWindow(SW_SHOWNOACTIVATE);auto before=manager.pendingBrowserPresentations();frame.EnableWindow(FALSE);frame.refresh();expect(manager.pendingBrowserPresentations()==before,"A disabled main window retains every pending presentation");frame.EnableWindow(TRUE);
 raceManager=&manager;began=GetTickCount64();auto timer=SetTimer(nullptr,0,75,tick);frame.refresh();KillTimer(nullptr,timer);if(!failure.empty())throw std::runtime_error(failure);
 expect(informationShown==4&&duplicatesShown==1&&refreshShown==3,"Restart opens the expected File Info, duplicate and refresh dialogs");
 expect(refreshRaceInjected&&manager.pendingBrowserPresentations().size()==1,"Cancel cannot acknowledge a newer captured address that was not displayed");
 timer=SetTimer(nullptr,0,75,tick);frame.refresh();KillTimer(nullptr,timer);if(!failure.empty())throw std::runtime_error(failure);
 expect(refreshShown==4,"The undisplayed replacement address opens its own refresh dialog");
 expect(manager.pendingBrowserPresentations().empty(),"Completed native dialog actions consume their saved presentations");
 frame.refresh();expect(!frame.progress.empty()&&frame.progress.front()->GetSafeHwnd(),"Restored existing-download choice opens the real native progress dialog");capture(frame.progress.front()->GetSafeHwnd(),L"progress.png");
 expect(frame.completions.size()==1&&frame.completions.front()->GetSafeHwnd(),"Restored completed-file choice opens the real Download complete dialog");capture(frame.completions.front()->GetSafeHwnd(),L"complete.png");press(frame.completions.front()->GetSafeHwnd(),L"Close");
 auto find=[&](const char* name){for(auto j:manager.jobs)if(str(j->data,"FileName")==name)return j;throw std::runtime_error("Missing expected fixture record");};
 expect(str(find("start.bin")->data,"Status")=="Queued","Restored Start Download queues the original record");
 expect(str(find("later.bin")->data,"Status")=="Paused"&&str(find("cancel.bin")->data,"Status")=="Paused","Restored Later and Cancel leave their original records paused");
 expect(str(find("refresh.bin")->data,"Url")=="http://127.0.0.1:9/refresh.bin?fresh=1","Restored Save address updates the intended download");
 expect(str(find("refresh-cancel.bin")->data,"Url")=="http://127.0.0.1:9/refresh-cancel.bin?fresh=2","The subsequent refresh applies the newer address to the original record");
 expect(str(find("refresh-save.bin")->data,"Url")=="http://127.0.0.1:9/refresh-save.bin?fresh=2","Reviewing the changed address then saving applies the newer address");
 capture(frame.GetSafeHwnd(),L"main.png");auto counts=std::make_tuple(informationShown,duplicatesShown,refreshShown);frame.refresh();expect(counts==std::make_tuple(informationShown,duplicatesShown,refreshShown),"Further refreshes do not reopen handled modal dialogs");
 prepareReviewFixtures(manager);began=GetTickCount64();timer=SetTimer(nullptr,0,75,tick);frame.refresh();KillTimer(nullptr,timer);if(!failure.empty())throw std::runtime_error(failure);
 expect(reviewsShown==5,"Every changed/deleted target opens its native review once");
 expect(str(find("review-refresh.bin")->data,"Url").find("?changed=1")!=std::string::npos,"Review replacement leaves current address unchanged until the address dialog is saved");
 expect(manager.pendingBrowserCaptureReviews()==Json::array({str(reviewFixtures["review-later.bin"],"token")}),"Later is the only undecided review after the first pass");
 timer=SetTimer(nullptr,0,75,tick);frame.refresh();KillTimer(nullptr,timer);if(!failure.empty())throw std::runtime_error(failure);
 expect(reviewsShown==5&&informationShown==6&&refreshShown==5,"Reviewed new files and replacement open their ordinary dialogs without reopening Later");
 expect(str(find("review-refresh.bin")->data,"Url").find("?fresh=1")!=std::string::npos,"Reviewed replacement Save applies to its original record");
 expect(str(find("review-new.bin")->data,"Status")=="Paused"&&str(find("review-stale.bin")->data,"Status")=="Paused","New-file review Later leaves both new records paused");
 expect(find("review-new.bin")->id()!=str(reviewFixtures["review-new.bin"],"id")&&find("review-stale.bin")->id()!=str(reviewFixtures["review-stale.bin"],"id"),"Deleted original identities are never reused for new files");
 {Manager reopened(manager.root);expect(reopened.pendingBrowserCaptureReviews()==manager.pendingBrowserCaptureReviews(),"Undecided Later review survives reopening its catalog");}
 manager.browserRecoveryRequested=true;revisitReviews=true;timer=SetTimer(nullptr,0,75,tick);frame.refresh();KillTimer(nullptr,timer);if(!failure.empty())throw std::runtime_error(failure);
 expect(reviewsShown==6&&manager.pendingBrowserCaptureReviews().empty(),"Manual Recover reopens Later and its discard resolves the review");
 for(const auto& spec:reviewFixtures)expect(fileHash(fs::path(wide(str(spec,"part"))))==str(spec,"hash"),"Every native review choice preserves original partial bytes");
 expect(manager.pendingBrowserPresentations().empty(),"Review decisions leave no unhandled File Info or refresh dialogs");
 }
};
class PresentationTestApplication:public CWinApp {
 int code=1;
public:BOOL InitInstance()override{
 CWinApp::InitInstance();INITCOMMONCONTROLSEX controls{sizeof(controls),ICC_WIN95_CLASSES|ICC_DATE_CLASSES|ICC_PROGRESS_CLASS};InitCommonControlsEx(&controls);AfxOleInit();int argc=0;auto argv=CommandLineToArgvW(GetCommandLineW(),&argc);std::unique_ptr<Manager> manager;MainWindow* frame=nullptr;
 try{if(argc!=2)throw std::runtime_error("Pass a fresh isolated output directory");output=argv[1];if(fs::exists(output))throw std::runtime_error("The native fixture requires a fresh directory");fs::create_directories(output);prepareFixtures(output/L"state");manager=std::make_unique<Manager>(output/L"state");frame=new MainWindow(*manager);m_pMainWnd=frame;CapturePresentationComponentTest::run(*manager,*frame);code=0;}catch(const std::exception& e){failure=e.what();}
 if(frame){frame->SendMessage(WM_CLOSE);m_pMainWnd=nullptr;}manager.reset();if(argv)LocalFree(argv);
 if(!output.empty()){Manager reopened(output/L"state");if(code==0)expect(reopened.pendingBrowserPresentations().empty(),"Handled presentations remain consumed after closing and reopening UDM");atomicText(output/L"results.json",Json{{"passed",code==0},{"error",failure},{"checks",outcomes}}.dump(2),false);}return FALSE;
 }
 int ExitInstance()override{AfxOleTerm(FALSE);return code;}
};
PresentationTestApplication application;
}
