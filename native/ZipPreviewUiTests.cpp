// Standalone test executable: interacts only with dialogs created by this process.
#include "Ui.hpp"
#include <fstream>
namespace udm {
static Json spec,report;static int stage=0;static ULONGLONG began;static UINT_PTR timer=0;static std::string failure;static fs::path output;
static void demand(bool value,const std::string& what){if(!value)throw std::runtime_error(what);}
static HWND childText(HWND parent,const std::wstring& text){for(auto child=GetWindow(parent,GW_CHILD);child;child=GetWindow(child,GW_HWNDNEXT)){wchar_t buffer[512]{};GetWindowTextW(child,buffer,512);if(buffer==text)return child;}return nullptr;}
static void press(HWND parent,HWND control){demand(control!=nullptr,"Missing test control");PostMessageW(parent,WM_COMMAND,MAKEWPARAM(GetDlgCtrlID(control),BN_CLICKED),(LPARAM)control);}
static void capture(HWND window,const fs::path& path){ShowWindow(window,SW_SHOWNOACTIVATE);RedrawWindow(window,nullptr,nullptr,RDW_INVALIDATE|RDW_ERASE|RDW_ALLCHILDREN|RDW_UPDATENOW);RECT r{};GetWindowRect(window,&r);CImage image;image.Create(r.right-r.left,r.bottom-r.top,32);auto dc=image.GetDC();PrintWindow(window,dc,PW_RENDERFULLCONTENT);image.ReleaseDC();demand(SUCCEEDED(image.Save(path.c_str())),"Cannot save component rendering");}
static BOOL CALLBACK observe(HWND window,LPARAM){
 try{
  wchar_t title[128]{};GetWindowTextW(window,title,128);report["stage"]=stage;report["windows"][utf8(title)]=true;
  if(std::wstring(title)==L"Download File Info"){
   if(stage==0){
    HWND preview=childText(window,L"&Preview");demand(preview!=nullptr,"Preview control not created");
    // The harness starts hidden; inspect each control's requested visibility.
    const bool shown=(GetWindowLongW(preview,GWL_STYLE)&WS_VISIBLE)!=0;
    if(yes(spec,"hidden")){demand(!shown,"Ineligible Preview must be hidden");stage=3;PostMessageW(window,WM_CLOSE,0,0);return TRUE;}
    if(!shown){report["previewHidden"]=true;return TRUE;}
    if(yes(spec,"draftLogin")){
     auto more=childText(window,L"More >>");if(more)SendMessageW(window,WM_COMMAND,MAKEWPARAM(GetDlgCtrlID(more),BN_CLICKED),(LPARAM)more);
     auto use=childText(window,L"Use login and password");demand(use!=nullptr,"Login checkbox missing");SendMessageW(use,BM_SETCHECK,BST_CHECKED,0);SendMessageW(window,WM_COMMAND,MAKEWPARAM(GetDlgCtrlID(use),BN_CLICKED),(LPARAM)use);
     auto user=GetWindow(childText(window,L"Login"),GW_HWNDNEXT),password=GetWindow(childText(window,L"Password"),GW_HWNDNEXT);demand(user&&password,"Login edits missing");SetWindowTextW(user,L"u");SetWindowTextW(password,L"p:2");
    }
    stage=1;press(window,preview);
   }else if(stage==2){stage=3;PostMessageW(window,WM_CLOSE,0,0);}
  }else if(std::wstring(title)==L"Zip preview"&&stage==1){
   auto cancel=childText(window,L"Cancel");
   if(yes(spec,"cancel")&&cancel){stage=2;press(window,cancel);report["cancelled"]=true;return TRUE;}
   auto ok=childText(window,L"OK");if(!ok)return TRUE;
   auto form=dynamic_cast<Form*>(CWnd::FromHandlePermanent(window));demand(form!=nullptr,"Missing production Form");
   ZipPreviewList* list=nullptr;for(auto child=GetWindow(window,GW_CHILD);child;child=GetWindow(child,GW_HWNDNEXT)){auto value=dynamic_cast<ZipPreviewList*>(CWnd::FromHandlePermanent(child));if(value)list=value;}
   demand(list!=nullptr,"Missing virtual ZIP list");demand((list->GetStyle()&LVS_OWNERDATA)!=0,"ZIP list must be virtual");
   report["rows"]=list->GetItemCount();report["names"]=Json::array();for(int i=0;i<list->GetItemCount();++i)report["names"].push_back(utf8((LPCWSTR)list->GetItemText(i,0)));
   demand(list->GetItemCount()==num(spec,"entries",3),"Unexpected ZIP entry count");
   if(!yes(spec,"error")){demand(list->GetItemText(0,0)==L"folder/readme.txt","Owner-data row callback did not supply its name");demand(!list->GetItemText(0,1).IsEmpty()&&list->GetItemText(0,3)==L"No","Owner-data size/encryption columns are empty");demand(list->GetItemText(1,0)==L"unicode/文件-é.txt","Unicode names differ");}
   RECT client{};GetClientRect(window,&client);for(auto child=GetWindow(window,GW_CHILD);child;child=GetWindow(child,GW_HWNDNEXT)){if(!(GetWindowLongW(child,GWL_STYLE)&WS_VISIBLE))continue;RECT r{};GetWindowRect(child,&r);MapWindowPoints(nullptr,window,(LPPOINT)&r,2);demand(r.left>=0&&r.top>=0&&r.right<=client.right&&r.bottom<=client.bottom,"Preview control is clipped");}
   if(yes(spec,"screenshot"))capture(window,output/L"zip-preview.png");
   report["dpi"]=GetDpiForWindow(window);stage=2;press(window,ok);
  }
 }catch(const std::exception& e){if(failure.empty())failure=e.what();PostMessageW(window,WM_CLOSE,0,0);}
 return TRUE;
}
static void CALLBACK tick(HWND,UINT,UINT_PTR,DWORD){
 if(GetTickCount64()-began>12000){if(failure.empty())failure="Component test deadline";EnumThreadWindows(GetCurrentThreadId(),[](HWND w,LPARAM)->BOOL{PostMessageW(w,WM_CLOSE,0,0);return TRUE;},0);return;}
 EnumThreadWindows(GetCurrentThreadId(),observe,0);
}
class ZipUiTestApplication:public CWinApp{
 int resultCode=1;
public:BOOL InitInstance()override{
 CWinApp::InitInstance();INITCOMMONCONTROLSEX common{sizeof(common),ICC_WIN95_CLASSES};InitCommonControlsEx(&common);AfxOleInit();
 int argc=0;auto argv=CommandLineToArgvW(GetCommandLineW(),&argc);int code=1;
 try{
  demand(argc==2,"Pass an isolated fixture specification");fs::path input(argv[1]);output=input.parent_path();spec=Json::parse(readText(input));Manager manager(output/L"state");auto prefs=manager.state["Settings"];prefs["DownloadFolder"]=utf8((output/L"downloads").wstring());prefs["CategoryFolders"]=false;prefs["PrefetchFileInfo"]=false;prefs["ProxyMode"]="Connect directly";prefs["QueuePromptLater"]=false;manager.setSettings(prefs);
  Json post=Json::object();if(yes(spec,"post"))post={{"method","POST"},{"body",b64(Bytes{'x'})},{"contentType","text/plain"}};
  auto job=manager.add(str(spec,"url"),"",str(spec,"filename","fixture.zip"),"Main queue",true,{}, "",post);
  began=GetTickCount64();timer=SetTimer(nullptr,0,30,tick);demand(timer!=0,"Cannot create component timer");downloadInfo(nullptr,manager,job);KillTimer(nullptr,timer);timer=0;
  demand(failure.empty(),failure);demand(stage==3,"File Info did not close after preview");
  demand(!manager.isActive(job)&&num(job->data,"Received")==0&&!fs::exists(job->target()),"Preview started or published a download");demand(headerValue(readHeaders(job->data),"Authorization").empty(),"Unsaved preview login persisted");
  report["passed"]=true;report["downloadStarted"]=false;report["draftLoginPersisted"]=false;code=0;
 }catch(const std::exception& e){report["passed"]=false;report["error"]=e.what();if(timer)KillTimer(nullptr,timer);}
 if(argv)LocalFree(argv);report["exitCode"]=code;if(!output.empty())atomicText(output/L"ui-result.json",report.dump(2),false);resultCode=code;return FALSE;
 }
 int ExitInstance()override{AfxOleTerm(FALSE);return resultCode;}
};
ZipUiTestApplication application;
}
