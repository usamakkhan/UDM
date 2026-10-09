// Drives and renders only native windows owned by this isolated test executable.
#define UDM_TOOLBAR_COMPONENT_TEST
#include "App.cpp"
namespace udm {
static Json results=Json::array();static fs::path output;static std::string failure;static std::wstring expectedTitle;static std::function<void(HWND)> action;static ULONGLONG began;static unsigned observations=0;
static void expect(bool value,const char* name){results.push_back({{"name",name},{"passed",value}});if(!value)throw std::runtime_error(name);}
static Json mainQueue(Manager& m){for(const auto& q:m.state["Queues"])if(str(q,"Name")=="Main queue")return q;throw std::runtime_error("Missing Main queue fixture");}
static std::wstring windowText(HWND w){wchar_t text[512]{};GetWindowTextW(w,text,512);return text;}
static HWND child(HWND parent,const wchar_t* label){for(HWND c=GetWindow(parent,GW_CHILD);c;c=GetWindow(c,GW_HWNDNEXT))if(windowText(c)==label)return c;throw std::runtime_error("Missing owned test control");}
static std::vector<HWND> children(HWND parent,const wchar_t* type){std::vector<HWND> result;for(HWND c=GetWindow(parent,GW_CHILD);c;c=GetWindow(c,GW_HWNDNEXT)){wchar_t name[128]{};GetClassNameW(c,name,128);if(!_wcsicmp(name,type))result.push_back(c);}return result;}
static void press(HWND w,const wchar_t* label){auto c=child(w,label);expect(IsWindowEnabled(c)!=FALSE,"Requested native button is enabled");SendMessageW(c,BM_CLICK,0,0);}
static void selectCombo(HWND parent,HWND control,int index){SendMessageW(control,CB_SETCURSEL,index,0);SendMessageW(parent,WM_COMMAND,MAKEWPARAM(GetDlgCtrlID(control),CBN_SELCHANGE),(LPARAM)control);}
static CRect bounds(HWND w){CRect r;GetWindowRect(w,&r);return r;}
static void capture(HWND w,const wchar_t* name){
 ShowWindow(w,SW_SHOWNOACTIVATE);RedrawWindow(w,nullptr,nullptr,RDW_INVALIDATE|RDW_ALLCHILDREN|RDW_UPDATENOW|RDW_ERASE);auto r=bounds(w);CImage img;img.Create(r.Width(),r.Height(),32);auto dc=img.GetDC();RECT fill{0,0,r.Width(),r.Height()};FillRect(dc,&fill,GetSysColorBrush(COLOR_BTNFACE));SendMessageW(w,WM_PRINT,(WPARAM)dc,PRF_NONCLIENT|PRF_CLIENT|PRF_ERASEBKGND|PRF_CHILDREN);
 for(HWND c=GetWindow(w,GW_CHILD);c;c=GetWindow(c,GW_HWNDNEXT)){if(!(GetWindowLongW(c,GWL_STYLE)&WS_VISIBLE))continue;auto cr=bounds(c);auto saved=SaveDC(dc);IntersectClipRect(dc,cr.left-r.left,cr.top-r.top,cr.right-r.left,cr.bottom-r.top);SetViewportOrgEx(dc,cr.left-r.left,cr.top-r.top,nullptr);SendMessageW(c,WM_PRINT,(WPARAM)dc,PRF_NONCLIENT|PRF_CLIENT|PRF_ERASEBKGND|PRF_CHILDREN);RestoreDC(dc,saved);}
 img.ReleaseDC();if(FAILED(img.Save((output/name).c_str())))throw std::runtime_error("Could not render native test dialog");
}
static BOOL CALLBACK observe(HWND w,LPARAM){if(windowText(w)!=expectedTitle)return TRUE;try{ShowWindow(w,SW_SHOWNOACTIVATE);++observations;action(w);}catch(const std::exception& e){failure=e.what();PostMessageW(w,WM_CLOSE,0,0);}return FALSE;}
static void CALLBACK onTimer(HWND,UINT,UINT_PTR,DWORD){if(GetTickCount64()-began>12000){failure="Owned dialog test deadline";EnumThreadWindows(GetCurrentThreadId(),[](HWND w,LPARAM)->BOOL{PostMessageW(w,WM_CLOSE,0,0);return TRUE;},0);return;}EnumThreadWindows(GetCurrentThreadId(),observe,0);}
static void modal(Form& dialog,const wchar_t* caption,std::function<void(HWND)> callback){const auto before=observations;expectedTitle=caption;action=std::move(callback);began=GetTickCount64();auto timer=SetTimer(nullptr,0,80,onTimer);dialog.DoModal();KillTimer(nullptr,timer);action={};if(!failure.empty())throw std::runtime_error(failure);expect(observations>before,"Owned modal callback actually ran");}
class QueueExportTestApplication:public CWinApp {
 int code=1;
public:BOOL InitInstance()override{
 CWinApp::InitInstance();INITCOMMONCONTROLSEX common{sizeof(common),ICC_WIN95_CLASSES|ICC_DATE_CLASSES};InitCommonControlsEx(&common);AfxOleInit();int argc=0;auto argv=CommandLineToArgvW(GetCommandLineW(),&argc);
 try{
  if(argc!=2)throw std::runtime_error("Pass an isolated output directory");output=fs::path(argv[1]);fs::create_directories(output);Manager manager(output/L"state");auto prefs=manager.state["Settings"];prefs["DownloadFolder"]=utf8((output/L"files").wstring());prefs["CategoryFolders"]=false;manager.setSettings(prefs);auto q=defaultQueue();q["Enabled"]=false;manager.setQueue(q);auto other=defaultQueue("Other");other["Enabled"]=false;manager.setQueue(other);
  auto a=manager.add("https://example.invalid/a","","a.bin");auto b=manager.add("https://example.invalid/b","","b.bin","Other");auto done=manager.add("https://example.invalid/done","","done.bin");done->data["Status"]="Complete";done->data["QueueMember"]=false;auto removed=manager.add("https://example.invalid/removed","","removed.bin");removed->data["QueueMember"]=false;manager.save();
  {
   auto rows=parseIdmExport("<\r\nhttps://example.test/one.zip\r\nreferer: https://example.test/page\r\ncookie: fixture=1\r\npd: x=one\r\nUser-Agent: UDM-fixture\r\n>\r\n<\r\nhttps://example.test/two.zip\r\n>\r\n");
   auto before=manager.snapshot();int stage=0;IdmImportDialog canceled(manager,rows);modal(canceled,L"Import links to UDM",[&](HWND w){expect(children(w,L"SysListView32").size()==1,"Import has a native selectable download list");capture(w,L"import-ef2.png");press(w,L"Cancel");});expect(manager.snapshot()==before,"Canceling EF2 import preserves the full catalog");
   IdmImportDialog importer(manager,rows);modal(importer,L"Import links to UDM",[&](HWND w){auto list=children(w,L"SysListView32").at(0);switch(stage++){
    case 0:expect(ListView_GetItemCount(list)==2,"Saved Referer is displayed as metadata rather than a separate download");press(w,L"Uncheck all");expect(!IsWindowEnabled(child(w,L"Import")),"Import is disabled when no rows are checked");press(w,L"Check all");ListView_SetCheckState(list,1,FALSE);press(w,L"Use saved cookies and form data");capture(w,L"import-ef2-selected.png");break;
    case 1:press(w,L"Import");break;
   }});expect(importer.imported==1&&manager.jobs.size()==5,"Native import adds only the checked row");auto added=manager.jobs.back();expect(str(added->data,"Status")=="Paused"&&str(readPostRequest(added->data),"method")=="POST"&&headerValue(readHeaders(added->data),"Cookie")=="fixture=1","Native import preserves saved request and starts paused");manager.remove(added);
   ExportDownloadsDialog ef2(manager,{a},"Main queue",nullptr,2);modal(ef2,L"Export downloads",[&](HWND w){auto formats=children(w,L"ComboBox");expect(SendMessageW(formats[1],CB_GETCURSEL,0,0)==2,"Export entry opens directly in IDM file format");atomicText(output/L"ef2-visibility.json",Json{{"dialogVisible",IsWindowVisible(w)!=FALSE},{"sessionStyleVisible",(GetWindowLongW(child(w,L"Include cookies and form data (plain text)"),GWL_STYLE)&WS_VISIBLE)!=0},{"credentialStyleVisible",(GetWindowLongW(child(w,L"Include Windows-account-encrypted request credentials"),GWL_STYLE)&WS_VISIBLE)!=0}}.dump(2),false);capture(w,L"export-ef2-before-check.png");expect(IsWindowVisible(child(w,L"Include cookies and form data (plain text)"))&&!IsWindowVisible(child(w,L"Include Windows-account-encrypted request credentials")),"EF2 clearly exposes its plain-text session option");capture(w,L"export-ef2.png");selectCombo(w,formats[1],1);expect(IsWindowVisible(child(w,L"Include Windows-account-encrypted request credentials"))&&!IsWindowVisible(child(w,L"Include cookies and form data (plain text)")),"Catalog export restores its encrypted-credential option");press(w,L"Cancel");});
  }
  int step=0;ExportDownloadsDialog exportDialog(manager,{b,done},"Main queue");modal(exportDialog,L"Export downloads",[&](HWND w){auto list=children(w,L"SysListView32").at(0);auto combos=children(w,L"ComboBox");
   switch(step++){
    case 0:expect(ListView_GetItemCount(list)==2&&SendMessageW(child(w,L"Selected downloads"),BM_GETCHECK,0,0)==BST_CHECKED,"Export opens with exactly the main-window selection");expect(!IsWindowEnabled(combos[0]),"Queue chooser is disabled outside queue scope");capture(w,L"export-selected.png");press(w,L"All downloads");break;
    case 1:expect(ListView_GetItemCount(list)==4,"All scope restores complete download history");ListView_SetCheckState(list,0,FALSE);press(w,L"Files in download queue");break;
    case 2:expect(ListView_GetItemCount(list)==1&&!ListView_GetCheckState(list,0),"Main queue scope excludes completed/removed members and retains the unchecked file");expect(IsWindowEnabled(combos[0])&&!IsWindowEnabled(child(w,L"Export...")),"Empty checked scope disables export while keeping queue selection usable");{auto index=(int)SendMessageW(combos[0],CB_FINDSTRINGEXACT,-1,(LPARAM)L"Other");expect(index>=0,"Named export queue is available");selectCombo(w,combos[0],index);}break;
    case 3:expect(ListView_GetItemCount(list)==1&&ListView_GetCheckState(list,0),"Changing named queue shows its pending member");selectCombo(w,combos[0],0);break;
    case 4:expect(ListView_GetItemCount(list)==2,"All queues scope combines only pending queue members");press(w,L"Check all");expect(ListView_GetCheckState(list,0)&&ListView_GetCheckState(list,1),"Check all affects the displayed scope");capture(w,L"export-queues.png");selectCombo(w,combos[1],0);break;
    case 5:expect(!IsWindowEnabled(child(w,L"Include Windows-account-encrypted request credentials")),"URL-list format disables the catalog credential option");press(w,L"Uncheck all");expect(!IsWindowEnabled(child(w,L"Export...")),"Uncheck all disables empty export");press(w,L"Cancel");break;
   }
  });
  expect(step==6,"All export scope interactions completed");expect(manager.jobs.size()==4&&str(a->data,"Status")=="Paused","Canceling export preserves download records");
  auto namedQueue=[&](const std::string& name){for(const auto& queue:manager.state["Queues"])if(str(queue,"Name")==name)return queue;throw std::runtime_error("Missing test queue");};
  Scheduler scheduler(manager,nullptr);modal(scheduler,L"Scheduler",[&](HWND w){
   auto startup=child(w,L"Start download on UDM startup");expect(IsWindowVisible(startup)&&SendMessageW(startup,BM_GETCHECK,0,0)==BST_UNCHECKED,"Scheduler exposes the unchecked startup preference on Schedule");
   press(w,L"Start download on UDM startup");press(w,L"Apply");expect(yes(mainQueue(manager),"StartOnStartup")&&str(a->data,"Status")=="Paused","Apply persists startup preference without starting downloads");
   press(w,L"Stop");expect(yes(mainQueue(manager),"StartOnStartup")&&!yes(mainQueue(manager),"Enabled"),"Stop preserves startup preference");
   press(w,L"Start download on UDM startup");auto tree=static_cast<CTreeCtrl*>(CWnd::FromHandlePermanent(children(w,L"SysTreeView32").at(0)));auto choose=[&](const wchar_t* name){for(auto item=tree->GetRootItem();item;item=tree->GetNextSiblingItem(item))if(tree->GetItemText(item)==name){tree->SelectItem(item);return;}throw std::runtime_error("Missing queue tree item");};
   choose(L"Other");expect(!yes(mainQueue(manager),"StartOnStartup"),"Switching queues saves the outgoing draft without Apply");expect(SendMessageW(startup,BM_GETCHECK,0,0)==BST_UNCHECKED,"Newly selected queue loads its own startup preference");press(w,L"Start download on UDM startup");press(w,L"Apply");expect(yes(namedQueue("Other"),"StartOnStartup")&&!yes(mainQueue(manager),"StartOnStartup"),"Apply changes only the selected queue's preference");
   choose(L"Main queue");expect(SendMessageW(startup,BM_GETCHECK,0,0)==BST_UNCHECKED,"Switching back restores the saved main-queue value");press(w,L"Start download on UDM startup");capture(w,L"scheduler-startup.png");press(w,L"Close");
  });expect(yes(mainQueue(manager),"StartOnStartup"),"Close saves the current scheduler draft without Apply");
  Scheduler windowClose(manager,nullptr);modal(windowClose,L"Scheduler",[&](HWND w){expect(SendMessageW(child(w,L"Start download on UDM startup"),BM_GETCHECK,0,0)==BST_CHECKED,"Reopening scheduler restores the saved startup choice");press(w,L"Start download on UDM startup");SendMessageW(w,WM_CLOSE,0,0);});expect(!yes(mainQueue(manager),"StartOnStartup"),"Window close saves the edited scheduler preference");
  Scheduler escapeClose(manager,nullptr);modal(escapeClose,L"Scheduler",[&](HWND w){press(w,L"Start download on UDM startup");SendMessageW(w,WM_COMMAND,IDCANCEL,0);});expect(yes(mainQueue(manager),"StartOnStartup"),"Escape's dialog-cancel route saves scheduler changes consistently");
  {Manager restored(manager.root);expect(yes(mainQueue(restored),"StartOnStartup"),"Saved scheduler preference survives catalog reload");bool otherSaved=false;for(const auto& queue:restored.state["Queues"])if(str(queue,"Name")=="Other")otherSaved=yes(queue,"StartOnStartup");expect(otherSaved,"The second queue's saved preference survives catalog reload");}
  expect(!manager.isActive(a)&&!manager.isActive(b)&&str(a->data,"Status")=="Paused","Scheduler editing never starts fixture downloads");
  {
   auto settings=manager.state["Settings"];settings["ClipboardMonitor"]=false;settings["CloseToTray"]=false;settings["DropBasket"]=false;manager.setSettings(settings);auto frame=new MainWindow(manager);m_pMainWnd=frame;
   try{
    auto tasks=GetSubMenu(::GetMenu(frame->GetSafeHwnd()),0);expect(GetMenuItemCount(tasks)==11,"Tasks menu matches the reference grouping and separators");
    expect(GetMenuItemID(tasks,2)==CMD_BATCH_CLIPBOARD&&GetMenuItemID(tasks,5)==CMD_BASKET,"Batch-from-clipboard and drop target occupy the reference Tasks positions");
    auto exports=GetSubMenu(tasks,7),imports=GetSubMenu(tasks,8);expect(exports&&imports&&GetMenuItemID(exports,0)==CMD_EXPORT_IDM&&GetMenuItemID(exports,1)==CMD_EXPORT_TEXT&&GetMenuItemID(imports,0)==CMD_IMPORT_IDM&&GetMenuItemID(imports,1)==CMD_IMPORT_TEXT,"Actual main-window menus expose the IDM-file and text-file routes");
    expectedTitle=L"Export downloads";began=GetTickCount64();action=[&](HWND w){expect(SendMessageW(children(w,L"ComboBox")[1],CB_GETCURSEL,0,0)==2,"The actual main-window EF2 menu command opens the corresponding format");capture(w,L"main-command-export-ef2.png");press(w,L"Cancel");};auto timer=SetTimer(nullptr,0,80,onTimer);frame->SendMessage(WM_COMMAND,CMD_EXPORT_IDM);KillTimer(nullptr,timer);action={};if(!failure.empty())throw std::runtime_error(failure);
   }catch(...){frame->SendMessage(WM_CLOSE);m_pMainWnd=nullptr;throw;}
   frame->SendMessage(WM_CLOSE);m_pMainWnd=nullptr;
  }
  code=0;
 }catch(const std::exception& e){failure=e.what();}
 if(argv)LocalFree(argv);if(!output.empty())atomicText(output/L"results.json",Json{{"passed",code==0},{"error",failure},{"checks",results}}.dump(2),false);return FALSE;
 }
 int ExitInstance()override{AfxOleTerm(FALSE);return code;}
};
QueueExportTestApplication application;
}
