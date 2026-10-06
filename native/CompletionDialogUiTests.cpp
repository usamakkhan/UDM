#define UDM_TOOLBAR_COMPONENT_TEST
#include "App.cpp"
namespace udm {
static Json checks=Json::array();static fs::path output,referencePath;static std::string failure;static std::atomic_bool missingNotice{false};
static void expect(bool ok,const char* name){checks.push_back({{"name",name},{"passed",ok}});if(!output.empty())atomicText(output/L"progress.json",checks.dump(2),false);if(!ok)throw std::runtime_error(name);}
static std::wstring caption(HWND w){wchar_t value[2048]{};GetWindowTextW(w,value,2048);return value;}
static HWND named(HWND w,const wchar_t* text){for(auto c=GetWindow(w,GW_CHILD);c;c=GetWindow(c,GW_HWNDNEXT))if(caption(c)==text)return c;throw std::runtime_error("Missing completion fixture control");}
static CRect bounds(HWND w,HWND parent=nullptr){CRect r;GetWindowRect(w,&r);if(parent)MapWindowPoints(nullptr,parent,(POINT*)&r,2);return r;}
static void render(HWND w,const fs::path& path){ShowWindow(w,SW_SHOWNOACTIVATE);RedrawWindow(w,nullptr,nullptr,RDW_INVALIDATE|RDW_ALLCHILDREN|RDW_UPDATENOW|RDW_ERASE);auto r=bounds(w);CImage image;image.Create(r.Width(),r.Height(),32);auto dc=image.GetDC();RECT fill{0,0,r.Width(),r.Height()};FillRect(dc,&fill,GetSysColorBrush(COLOR_BTNFACE));SendMessageW(w,WM_PRINT,(WPARAM)dc,PRF_NONCLIENT|PRF_CLIENT|PRF_ERASEBKGND|PRF_CHILDREN);image.ReleaseDC();if(FAILED(image.Save(path.c_str())))throw std::runtime_error("Could not render completion fixture");}
// Reading cached captions avoids cross-thread WM_GETTEXT while the modal warning opens.
static std::wstring cachedCaption(HWND w){wchar_t value[2048]{};InternalGetWindowText(w,value,2048);return value;}
static BOOL CALLBACK dismissMissing(HWND w,LPARAM){wchar_t type[80]{};GetClassNameW(w,type,80);if(wcscmp(type,L"#32770")||cachedCaption(w)!=L"Opening downloaded file")return TRUE;bool notice=false;HWND button=nullptr;for(auto c=GetWindow(w,GW_CHILD);c;c=GetWindow(c,GW_HWNDNEXT)){auto text=cachedCaption(c);if(text.find(L"file has been moved.")!=std::wstring::npos)notice=true;if(text==L"OK")button=c;}if(notice&&button&&PostMessageW(w,WM_COMMAND,MAKEWPARAM(GetDlgCtrlID(button),BN_CLICKED),(LPARAM)button))missingNotice=true;return TRUE;}
class ToolbarComponentTest {
public:static void run(MainWindow& frame){
 auto& manager=frame.manager;auto job=manager.add("https://example.invalid/completed.bin","","completed.bin","Main queue",true);fs::create_directories(job->target().parent_path());writeBytes(job->target(),Bytes{1,2,3,4});job->data["Status"]="Complete";job->data["Size"]=4;job->data["Received"]=4;job->data["TransferredBytes"]=4;job->data["TransferSeconds"]=2;manager.save();
 auto reference=Json::parse(readText(referencePath));Json dialog;for(auto& d:reference["dialogs"])if(num(d,"id")==286)dialog=d;expect(!dialog.is_null(),"Reference contains Download complete dialog 286");
 frame.complete(job,true);expect(!frame.completions.empty()&&frame.completions.back()->GetSafeHwnd(),"Actual completed record opens its completion dialog");auto w=frame.completions.back()->GetSafeHwnd();
 for(auto title:{L"Open",L"Open with...",L"Open folder",L"Close"})expect(IsWindowEnabled(named(w,title))!=FALSE,"Completion action is enabled for a saved file");
 auto drag=named(w,L"completed.bin");wchar_t dragText[128]{};LVITEMW dragItem{};dragItem.iSubItem=0;dragItem.pszText=dragText;dragItem.cchTextMax=128;SendMessageW(drag,LVM_GETITEMTEXTW,0,(LPARAM)&dragItem);expect(SendMessageW(drag,LVM_GETITEMCOUNT,0,0)==1&&dragText[0]==0,"Drag area contains one icon without clipped filename text");
 std::vector<HWND> edits;for(auto c=GetWindow(w,GW_CHILD);c;c=GetWindow(c,GW_HWNDNEXT)){wchar_t type[80]{};GetClassNameW(c,type,80);if(!_wcsicmp(type,L"Edit"))edits.push_back(c);}
 expect(edits.size()==2&&caption(edits[0])==L"https://example.invalid/completed.bin"&&caption(edits[1])==job->target().wstring(),"Completion shows exact source and saved destination");for(auto field:edits)expect((GetWindowLongW(field,GWL_STYLE)&ES_READONLY)!=0,"Completion source and destination are read-only");
 for(UINT dpi:{96u,144u,192u}){auto suggested=bounds(w);SendMessageW(w,WM_DPICHANGED,MAKEWPARAM(dpi,dpi),(LPARAM)&suggested);CFont font;font.CreateFontW(-MulDiv(8,dpi,72),0,0,0,FW_NORMAL,FALSE,FALSE,FALSE,DEFAULT_CHARSET,OUT_DEFAULT_PRECIS,CLIP_DEFAULT_PRECIS,DEFAULT_QUALITY,DEFAULT_PITCH,L"Tahoma");auto dc=GetDC(w);auto units=DialogUnits::measure(dc,(HFONT)font.GetSafeHandle());ReleaseDC(w,dc);
  for(auto& ref:dialog["controls"]){const auto target=CRect(units.rect((int)ref["x"],(int)ref["y"],(int)ref["width"],(int)ref["height"]));std::wstring type;if(ref["class"].is_string())type=wide(ref["class"]);else{int role=ref["class"]["ordinal"];type=role==128?L"Button":role==129?L"Edit":L"Static";}bool found=false;
   for(auto c=GetWindow(w,GW_CHILD);c;c=GetWindow(c,GW_HWNDNEXT)){wchar_t actual[80]{};GetClassNameW(c,actual,80);if(_wcsicmp(type.c_str(),actual)||bounds(c,w)!=target)continue;if(!ref["title"].is_string()||str(ref,"title").empty()||str(ref,"title")=="List1"||str(ref,"title")=="Downloaded xx bytes."||caption(c)==wide(str(ref,"title")))found=true;}
   if(!found)atomicText(output/L"geometry-failure.json",Json{{"dpi",dpi},{"reference",ref}}.dump(2),false);expect(found,"Completion reference control class, label and geometry match");
  }render(w,output/wide("completion-"+std::to_string(dpi)+".png"));
 }
 // Use the real button ID: Windows can label IDCANCEL as OK in an MB_OK dialog.
 wchar_t missingMode[8]{};if(GetEnvironmentVariableW(L"UDM_TEST_COMPLETION_MISSING",missingMode,8)&&missingMode[0]==L'1'){
 fs::rename(job->target(),output/L"moved-fixture.bin");auto guiThread=GetCurrentThreadId();std::thread noticeObserver([guiThread]{for(int attempt=0;attempt<250&&!missingNotice;++attempt){EnumThreadWindows(guiThread,dismissMissing,0);Sleep(20);}});SendMessageW(named(w,L"Open"),BM_CLICK,0,0);noticeObserver.join();expect(missingNotice&&IsWindow(w),"Opening a moved file explains the problem and retains the completion dialog");}
 SendMessageW(named(w,L"Don't show this dialog again"),BM_CLICK,0,0);SendMessageW(named(w,L"Close"),BM_CLICK,0,0);expect(!IsWindow(w)&&yes(manager.state["Settings"],"SuppressCompletionDialog"),"Close saves Don't show again and closes only the completion dialog");{Manager reopened(manager.root);expect(yes(reopened.state["Settings"],"SuppressCompletionDialog"),"Completion suppression survives catalog reopen");}
 auto count=frame.completions.size();frame.complete(job,false);expect(frame.completions.size()==count,"Automatic completion respects suppression");frame.complete(job,true);expect(frame.completions.back()->GetSafeHwnd()!=nullptr,"Explicit completed-file request still opens the dialog");frame.completions.back()->SendMessage(WM_CLOSE);
 const std::vector<std::pair<std::string,std::string>> scanCases={
  {"Finished","Microsoft Defender completed the scan: no malware found or detected malware remediated. Review Windows Security for details."},
  {"Attention","Microsoft Defender reports an unresolved detection or a scanning error. Review Windows Security before opening the file."},
  {"Interrupted","UDM closed before the scanner result was recorded. Review your antivirus or run a new check."},
  {"Failed","Cannot save scanner result: Access is denied for C:\\Users\\Example\\Downloads\\"+std::string(160,'x')+"\\state.json. The antivirus may still be running. Review its result and the destination permissions before trying again."}
 };
 for(size_t scenario=0;scenario<scanCases.size();++scenario){
  job->data["ScanResult"]={{"Status",scanCases[scenario].first},{"Message",scanCases[scenario].second},{"ExitCode",scenario?2:0}};
  frame.complete(job,true);auto scanWindow=frame.completions.back()->GetSafeHwnd();expect(scanWindow!=nullptr,"Scanner result opens the expanded completion dialog");
  auto expected=wide("Virus check: "+scannerSummary(job->data));auto summary=named(scanWindow,expected.c_str());
  for(UINT dpi:{96u,144u,192u}){
   auto suggested=bounds(scanWindow);SendMessageW(scanWindow,WM_DPICHANGED,MAKEWPARAM(dpi,dpi),(LPARAM)&suggested);
   auto summaryRect=bounds(summary,scanWindow),openRect=bounds(named(scanWindow,L"Open"),scanWindow);RECT client{};GetClientRect(scanWindow,&client);
   expect(summaryRect.bottom<=openRect.top&&summaryRect.left>=0&&summaryRect.right<=client.right,"Scanner summary fits above actions without overlap");
   auto closeRect=bounds(named(scanWindow,L"Close"),scanWindow),checkRect=bounds(named(scanWindow,L"Don't show this dialog again"),scanWindow);
   expect(closeRect.bottom<=client.bottom&&checkRect.bottom<=client.bottom,"Expanded scanner actions and suppression control stay inside the dialog");
   wchar_t type[80]{};GetClassNameW(summary,type,80);auto style=GetWindowLongW(summary,GWL_STYLE);
   bool scrollable=!_wcsicmp(type,L"Edit")&&(style&ES_READONLY)&&(style&ES_MULTILINE)&&(style&WS_VSCROLL);
   auto dc=GetDC(summary);auto old=SelectObject(dc,(HFONT)SendMessageW(summary,WM_GETFONT,0,0));RECT needed{0,0,summaryRect.Width(),0};DrawTextW(dc,expected.c_str(),(int)expected.size(),&needed,DT_CALCRECT|DT_WORDBREAK|DT_NOPREFIX);SelectObject(dc,old);ReleaseDC(summary,dc);
   if(scrollable&&scenario==3){SendMessageW(summary,EM_LINESCROLL,0,10000);expect(SendMessageW(summary,EM_GETFIRSTVISIBLELINE,0,0)>0&&caption(summary)==expected,"Long scanner diagnostic scrolls while retaining its complete text");}
   render(scanWindow,output/wide("scanner-"+std::to_string(scenario)+"-"+std::to_string(dpi)+".png"));
   expect(scrollable||(needed.bottom<=summaryRect.Height()&&needed.right<=summaryRect.Width()),"Complete scanner diagnostic is readable or scrollable at each DPI");
  }
  SendMessageW(named(scanWindow,L"Close"),BM_CLICK,0,0);expect(!IsWindow(scanWindow),"Expanded scanner dialog closes normally");
 }
 }
};
class CompletionDialogApplication:public CWinApp {
 int result=1;bool started=false;std::unique_ptr<Manager> manager;MainWindow* frame=nullptr;
 void finish(){
  if(frame){frame->SendMessage(WM_CLOSE);frame=nullptr;m_pMainWnd=nullptr;}manager.reset();
  if(!output.empty())atomicText(output/L"results.json",Json{{"passed",result==0},{"error",failure},{"checks",checks}}.dump(2),false);
 }
public:BOOL InitInstance()override{
 CWinApp::InitInstance();INITCOMMONCONTROLSEX common{sizeof(common),ICC_WIN95_CLASSES|ICC_DATE_CLASSES};InitCommonControlsEx(&common);AfxOleInit();
 int argc=0;auto argv=CommandLineToArgvW(GetCommandLineW(),&argc);
 try{
  if(!argv||argc!=3)throw std::runtime_error("Pass fresh output and reference file paths");output=argv[1];referencePath=argv[2];
  if(fs::exists(output)){output.clear();throw std::runtime_error("Fresh output required");}fs::create_directories(output);
  manager=std::make_unique<Manager>(output/L"state");auto prefs=manager->state["Settings"];prefs["DownloadFolder"]=utf8((output/L"files").wstring());prefs["CategoryFolders"]=false;prefs["ClipboardMonitor"]=false;prefs["Sound"]=false;prefs["CloseToTray"]=false;manager->setSettings(prefs);
  frame=new MainWindow(*manager);m_pMainWnd=frame;frame->KillTimer(1);
 }catch(const std::exception& e){failure=e.what();if(argv)LocalFree(argv);finish();return FALSE;}
 LocalFree(argv);return TRUE;
 }
 BOOL OnIdle(LONG)override{
  if(started)return FALSE;started=true;
  try{ToolbarComponentTest::run(*frame);result=0;}catch(const std::exception& e){failure=e.what();}
  finish();PostQuitMessage(result);return FALSE;
 }
 int ExitInstance()override{AfxOleTerm(FALSE);return result;}
};CompletionDialogApplication application;
}