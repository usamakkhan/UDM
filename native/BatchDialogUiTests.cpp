// Drives only the Batch download dialog owned by this isolated test executable.
#define UDM_TOOLBAR_COMPONENT_TEST
#include "App.cpp"
namespace udm {
static Json results=Json::array();static fs::path output;static std::string failure;static std::function<void(HWND)> action;static ULONGLONG began;
static void expect(bool value,const char* name){results.push_back({{"name",name},{"passed",value}});if(!value)throw std::runtime_error(name);}
static std::wstring windowText(HWND w){wchar_t text[1024]{};GetWindowTextW(w,text,1024);return text;}
static HWND child(HWND parent,const wchar_t* label){for(HWND c=GetWindow(parent,GW_CHILD);c;c=GetWindow(c,GW_HWNDNEXT))if(windowText(c)==label)return c;throw std::runtime_error("Missing batch dialog control: "+utf8(label));}
static std::vector<HWND> children(HWND parent,const wchar_t* type){std::vector<HWND> result;for(HWND c=GetWindow(parent,GW_CHILD);c;c=GetWindow(c,GW_HWNDNEXT)){wchar_t name[128]{};GetClassNameW(c,name,128);if(!_wcsicmp(name,type))result.push_back(c);}return result;}
static void press(HWND w,const wchar_t* label){auto c=child(w,label);expect(IsWindowEnabled(c)!=FALSE,"Requested batch dialog button is enabled");SendMessageW(c,BM_CLICK,0,0);}
static void type(HWND dialog,HWND control,const wchar_t* value,WORD notification){SetWindowTextW(control,value);SendMessageW(dialog,WM_COMMAND,MAKEWPARAM(GetDlgCtrlID(control),notification),(LPARAM)control);}
static CRect bounds(HWND w){CRect r;GetWindowRect(w,&r);return r;}
static void capture(HWND w,const wchar_t* name){
 ShowWindow(w,SW_SHOWNOACTIVATE);RedrawWindow(w,nullptr,nullptr,RDW_INVALIDATE|RDW_ALLCHILDREN|RDW_UPDATENOW|RDW_ERASE);auto r=bounds(w);CImage img;img.Create(r.Width(),r.Height(),32);auto dc=img.GetDC();RECT fill{0,0,r.Width(),r.Height()};FillRect(dc,&fill,GetSysColorBrush(COLOR_BTNFACE));SendMessageW(w,WM_PRINT,(WPARAM)dc,PRF_NONCLIENT|PRF_CLIENT|PRF_ERASEBKGND|PRF_CHILDREN);
 for(HWND c=GetWindow(w,GW_CHILD);c;c=GetWindow(c,GW_HWNDNEXT)){if(!(GetWindowLongW(c,GWL_STYLE)&WS_VISIBLE))continue;auto cr=bounds(c);auto saved=SaveDC(dc);IntersectClipRect(dc,cr.left-r.left,cr.top-r.top,cr.right-r.left,cr.bottom-r.top);SetViewportOrgEx(dc,cr.left-r.left,cr.top-r.top,nullptr);SendMessageW(c,WM_PRINT,(WPARAM)dc,PRF_NONCLIENT|PRF_CLIENT|PRF_ERASEBKGND|PRF_CHILDREN);RestoreDC(dc,saved);}
 img.ReleaseDC();if(FAILED(img.Save((output/name).c_str())))throw std::runtime_error("Could not render the batch dialog");
}
static BOOL CALLBACK observe(HWND w,LPARAM){if(windowText(w)!=L"Batch download")return TRUE;try{auto once=std::move(action);if(once){ShowWindow(w,SW_SHOWNOACTIVATE);once(w);}}catch(const std::exception& e){failure=e.what();PostMessageW(w,WM_CLOSE,0,0);}return FALSE;}
static void CALLBACK onTimer(HWND,UINT,UINT_PTR,DWORD){if(GetTickCount64()-began>15000){failure="Batch dialog deadline";EnumThreadWindows(GetCurrentThreadId(),[](HWND w,LPARAM)->BOOL{PostMessageW(w,WM_CLOSE,0,0);return TRUE;},0);return;}EnumThreadWindows(GetCurrentThreadId(),observe,0);}
class BatchDialogApplication:public CWinApp {
 int code=1;
public:BOOL InitInstance()override{
 CWinApp::InitInstance();INITCOMMONCONTROLSEX common{sizeof(common),ICC_WIN95_CLASSES|ICC_DATE_CLASSES};InitCommonControlsEx(&common);AfxOleInit();int argc=0;auto argv=CommandLineToArgvW(GetCommandLineW(),&argc);
 try{if(argc!=2)throw std::runtime_error("Pass isolated output directory");output=fs::path(argv[1]);fs::create_directories(output);Manager manager(output/L"state");
  // Pattern expansion is checked without a window first.
  auto numbered=batchPattern("http://example.test/img*.jpg",false,"0","100",2);expect(numbered.size()==101&&numbered.front()=="http://example.test/img00.jpg"&&numbered[1]=="http://example.test/img01.jpg"&&numbered.back()=="http://example.test/img100.jpg","Numbers replace the asterisk with zero-padded values of the wildcard size");
  auto lettered=batchPattern("http://example.test/part-*.zip",true,"a","e",2);expect(lettered.size()==5&&lettered.front()=="http://example.test/part-a.zip"&&lettered.back()=="http://example.test/part-e.zip","Letters replace the asterisk with a letter range");
  bool rejected=false;try{batchPattern("http://example.test/img*.jpg",false,"0","1000",1);}catch(const std::exception&){rejected=true;}expect(rejected,"Batches above 1,000 files are rejected");
  expect(batchPattern("http://example.test/one.zip",false,"0","9",1).size()==1,"An address without an asterisk stays a single download");
  action=[&](HWND w){
   auto combos=children(w,L"ComboBox"),edits=children(w,L"Edit"),spins=children(w,L"msctls_updown32"),statics=children(w,L"Static");
   expect(combos.size()==1&&edits.size()==5&&spins.size()==3,"Dialog has the address combo, five edits and three spin controls");
   for(auto label:{L"Address",L"Replace asterisk to",L"&Numbers",L"&Letters",L"From:",L"To:",L"Wildcard size",L"Use &authorization",L"Login",L"Password",L"First file:",L"Second file:",L"...",L"Last file:",L"O&K",L"&Cancel"})child(w,label);
   std::vector<HWND> previews;for(auto s:statics)if(windowText(s).empty())previews.push_back(s);expect(previews.size()==3,"Three live preview fields are present");
   expect(windowText(edits[0])==L"0"&&windowText(edits[1])==L"100"&&windowText(edits[2])==L"2","Defaults are From 0, To 100, wildcard size 2");
   expect(!IsWindowEnabled(edits[3])&&!IsWindowEnabled(edits[4]),"Login and Password stay disabled until Use authorization is checked");
   press(w,L"Use &authorization");expect(IsWindowEnabled(edits[3])&&IsWindowEnabled(edits[4]),"Use authorization enables the login fields");
   type(w,combos[0],L"http://example.test/img*.jpg",CBN_EDITCHANGE);
   expect(windowText(previews[0])==L"http://example.test/img00.jpg"&&windowText(previews[1])==L"http://example.test/img01.jpg"&&windowText(previews[2])==L"http://example.test/img100.jpg","Typing a pattern updates the first, second and last file previews");
   type(w,edits[0],L"5",EN_CHANGE);type(w,edits[2],L"3",EN_CHANGE);
   expect(windowText(previews[0])==L"http://example.test/img005.jpg"&&windowText(previews[1])==L"http://example.test/img006.jpg","Changing From and wildcard size refreshes the previews");
   press(w,L"&Letters");expect(windowText(edits[0])==L"a"&&windowText(edits[1])==L"z"&&!IsWindowEnabled(edits[2])&&!IsWindowEnabled(spins[2]),"Letters mode switches the range to a-z and disables the wildcard size");
   expect(windowText(previews[0])==L"http://example.test/imga.jpg"&&windowText(previews[2])==L"http://example.test/imgz.jpg","Letters mode previews letter file names");
   press(w,L"&Numbers");expect(windowText(edits[0])==L"5"&&windowText(edits[1])==L"100"&&IsWindowEnabled(edits[2]),"Numbers mode restores the numeric range");
   for(UINT dpi:{96u,144u,192u}){auto suggested=bounds(w);SendMessageW(w,WM_DPICHANGED,MAKEWPARAM(dpi,dpi),(LPARAM)&suggested);CRect client;GetClientRect(w,&client);std::vector<CRect> buttons;
    for(auto c=GetWindow(w,GW_CHILD);c;c=GetWindow(c,GW_HWNDNEXT)){if(!(GetWindowLongW(c,GWL_STYLE)&WS_VISIBLE))continue;auto rect=bounds(c);MapWindowPoints(nullptr,w,(POINT*)&rect,2);expect(rect.left>=0&&rect.top>=0&&rect.right<=client.right&&rect.bottom<=client.bottom,"Batch controls stay within the dialog at tested DPI");wchar_t kind[64]{};GetClassNameW(c,kind,64);if(!_wcsicmp(kind,L"Button")&&(GetWindowLongW(c,GWL_STYLE)&BS_TYPEMASK)!=BS_GROUPBOX)buttons.push_back(rect);}
    for(size_t i=0;i<buttons.size();++i)for(size_t j=i+1;j<buttons.size();++j){CRect overlap;expect(!overlap.IntersectRect(buttons[i],buttons[j]),"Batch buttons do not overlap at tested DPI");}
    capture(w,(L"batch-"+std::to_wstring(dpi)+L".png").c_str());}
   press(w,L"&Cancel");
  };
  began=GetTickCount64();auto timer=SetTimer(nullptr,0,80,onTimer);batchDialog(nullptr,manager);KillTimer(nullptr,timer);if(!failure.empty())throw std::runtime_error(failure);
  expect(manager.jobs.empty(),"Cancel adds no downloads");code=0;
 }catch(const std::exception& e){failure=e.what();}if(argv)LocalFree(argv);if(!output.empty())atomicText(output/L"results.json",Json{{"passed",code==0},{"error",failure},{"checks",results}}.dump(2),false);return FALSE;}
 int ExitInstance()override{AfxOleTerm(FALSE);return code;}
};BatchDialogApplication application;
}
