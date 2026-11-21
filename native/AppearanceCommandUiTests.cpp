#define UDM_TOOLBAR_COMPONENT_TEST
#include "App.cpp"
namespace udm {
static fs::path output;static Json checks=Json::array(),timings=Json::object();static std::string failure;
static void check(bool value,const char* name){checks.push_back({{"name",name},{"passed",value}});if(!value)throw std::runtime_error(name);}
static double stamp(){LARGE_INTEGER n,f;QueryPerformanceCounter(&n);QueryPerformanceFrequency(&f);return 1000.0*n.QuadPart/f.QuadPart;}
struct HeldAppearance {
 std::promise<void> release;std::thread thread;std::atomic_bool held=false,expired=false;bool opened=false;
 explicit HeldAppearance(Manager& manager,bool change=true){auto gate=release.get_future().share();thread=std::thread([this,&manager,gate,change]{Lock lock(manager.mutex);if(change){manager.state["Settings"]["UserAgent"]="Appearance concurrent writer";manager.state["Settings"]["DarkMode"]=true;}held=true;if(gate.wait_for(std::chrono::milliseconds(2500))!=std::future_status::ready)expired=true;held=false;});const auto end=GetTickCount64()+3000;while(!held&&GetTickCount64()<end)Sleep(1);}
 void open(){if(!opened){opened=true;release.set_value();}if(thread.joinable())thread.join();}~HeldAppearance(){open();}
};
static void pumpOne(){MSG message{};if(PeekMessageW(&message,nullptr,0,0,PM_REMOVE)&&message.message!=WM_QUIT){TranslateMessage(&message);DispatchMessageW(&message);}}
class ToolbarComponentTest {
public:
 static void cleanup(MainWindow*& frame){if(frame){closeOwnedMainWindow(frame);frame=nullptr;}}
 static void run(MainWindow*& frame,Manager& manager,const std::string& mode){
  frame->KillTimer(1);frame->ShowWindow(SW_SHOWNOACTIVATE);const auto window=frame->GetSafeHwnd();const auto disk=readText(manager.root/L"state.json");
#ifdef UDM_APPEARANCE_COMMAND_AVAILABLE
  const auto rendered=frame->renderedAppearance;frame->fillToolbarMenu(frame->toolbarMenu);std::vector<BOOL> buttons;for(int i=0;i<ToolbarCommandCount;++i)buttons.push_back(frame->toolbar.IsButtonEnabled(frame->toolbarCommand(i)));
  std::unique_ptr<HeldAppearance> after;std::atomic_bool fail=mode=="failure-retry"||mode=="exit-failure";
  frame->appearanceCommandObserver=[&](const char* stage){if(std::string(stage)=="before-save"&&fail.exchange(false))throw std::runtime_error("Owned appearance save failure");if(std::string(stage)=="captured"&&mode=="dark")after=std::make_unique<HeldAppearance>(manager,false);};
  if(mode=="destroy-pending"){frame->modelMaintenance.stop();frame->presentationAcks.stop();frame->authenticationTask->finish();}
#endif
  HeldAppearance writer(manager);
  try{const auto began=stamp();
#ifdef UDM_APPEARANCE_COMMAND_AVAILABLE
   if(mode=="font")frame->startAppearanceCommand({AppearanceChange::Font,"Consolas",15,700});
   else frame->SendMessage(WM_COMMAND,mode=="categories"?CMD_HIDE_CATEGORIES:mode=="font-reset"?CMD_FONT_RESET:mode=="tray-color"?CMD_TRAY_COLOR:mode=="tray-classic"?CMD_TRAY_SYSTEM:mode=="tray-hidden"?CMD_TRAY_HIDE:CMD_DARK);
#else
   frame->SendMessage(WM_COMMAND,CMD_DARK);
#endif
   timings[mode]["commandReturnMs"]=stamp()-began;check(writer.held&&!writer.expired,"Appearance command returns before the catalog writer releases");
#ifdef UDM_APPEARANCE_COMMAND_AVAILABLE
   check(frame->appearanceCommandTask.valid()&&manager.fileInfoDecisionPending&&frame->IsWindowEnabled(),"An accepted appearance intent keeps the main window responsive and its decision guarded");check(readText(manager.root/L"state.json")==disk&&frame->renderedAppearance==rendered,"Pending appearance work neither writes speculatively nor announces an uncommitted render");
   auto fontMenu=frame->menu.GetSubMenu(3)->GetSubMenu(6);frame->SendMessage(WM_INITMENUPOPUP,(WPARAM)fontMenu->GetSafeHmenu(),MAKELPARAM(6,FALSE));check((fontMenu->GetMenuState(CMD_FONT,MF_BYCOMMAND)&MF_DISABLED)&&(fontMenu->GetMenuState(CMD_FONT_RESET,MF_BYCOMMAND)&MF_DISABLED),"Pending appearance disables conflicting font choices without reading Manager");
   frame->SendMessage(WM_INITMENUPOPUP,(WPARAM)frame->toolbarMenu.GetSafeHmenu(),MAKELPARAM(2,FALSE));auto queues=frame->menu.GetSubMenu(2)->GetSubMenu(9);frame->SendMessage(WM_INITMENUPOPUP,(WPARAM)queues->GetSafeHmenu(),MAKELPARAM(9,FALSE));check(writer.held&&!writer.expired,"Toolbar and queue popup preparation does not wait for a pending appearance save");
   frame->SendMessage(WM_COMMAND,CMD_DARK);frame->SendMessage(WM_COMMAND,CMD_SELECTALL);frame->SendMessage(WM_COMMAND,CMD_SORT_ADDED);frame->SendMessage(WM_TIMER,1,0);frame->SendMessage(WM_NULL);frame->Invalidate();frame->UpdateWindow();check(writer.held&&!writer.expired&&frame->appearanceCommandTask.valid()&&frame->sortColumn==8,"Duplicate appearance selection sort timer and paint do not wait for the catalog");
   for(int i=0;i<ToolbarCommandCount;++i)check(frame->toolbar.CommandToIndex(frame->toolbarCommand(i))==UINT(-1)||!frame->toolbar.IsButtonEnabled(frame->toolbarCommand(i)),"Conflicting present toolbar buttons remain disabled during accepted work");
   if(mode=="close-tray"||mode=="cancel-close"||mode=="cancel-after-ack"){frame->SendMessage(WM_CLOSE);check(frame->appearanceClosePending&&!frame->quitting&&IsWindowVisible(window),"Normal Close defers until the appearance result is acknowledged");if(mode=="cancel-close"){frame->SendMessage(SHOW_APP);check(!frame->appearanceClosePending&&frame->appearanceCommandTask.valid(),"Show cancels a normal deferred Close without cancelling the accepted save");}}
   if(mode=="exit"||mode=="exit-failure"){frame->SendMessage(WM_COMMAND,CMD_EXIT);frame->SendMessage(SHOW_APP);frame->SendMessage(WM_COMMAND,CMD_SELECTALL);check(frame->appearanceClosePending&&frame->quitting&&IsWindow(window),"Explicit Exit stays queued through Show and duplicate commands");}
   if(mode=="destroy-pending"){std::thread release([&]{Sleep(40);writer.open();});frame->DestroyWindow();frame=nullptr;AfxGetApp()->m_pMainWnd=nullptr;release.join();check(!IsWindow(window)&&!manager.fileInfoDecisionPending,"Forced destruction joins accepted appearance work and releases its guard");check(!yes(Json::parse(readText(manager.root/L"state.json"))["Settings"],"DarkMode"),"Forced destruction preserves the completed durable appearance change");return;}
   writer.open();const auto end=GetTickCount64()+10000;while(frame->appearanceCommandTask.valid()&&GetTickCount64()<end){pumpOne();Sleep(1);}check(!frame->appearanceCommandTask.valid()&&!manager.fileInfoDecisionPending,"Weak wake or timer acknowledges and joins the accepted appearance save");
   if(mode=="cancel-after-ack"){check(frame->appearanceClosePending&&!frame->quitting,"Acknowledgement preserves the cancellable normal Close decision");frame->SendMessage(SHOW_APP);check(!frame->appearanceClosePending,"Show can cancel normal Close after acknowledgement before replay");}
   if(mode=="exit"){frame->SendMessage(SHOW_APP);check(frame->appearanceClosePending&&frame->quitting,"Show cannot cancel an acknowledged explicit Exit");}
   if(mode=="dark"){check(after&&after->held&&!after->expired&&!yes(frame->renderedAppearance,"DarkMode")&&!uiDark,"Acknowledged appearance renders from its owned result while a new writer holds Manager");after->open();}
   if(mode=="failure-retry"||mode=="exit-failure"){check(readText(manager.root/L"state.json")==disk&&frame->renderedAppearance==rendered&&!frame->quitting&&!frame->appearanceClosePending&&IsWindowVisible(window),"Failed save preserves the catalog and render and cancels deferred Exit for explicit retry");frame->appearanceCommandObserver={};frame->SendMessage(WM_COMMAND,CMD_DARK);const auto retryEnd=GetTickCount64()+10000;while(frame->appearanceCommandTask.valid()&&GetTickCount64()<retryEnd){pumpOne();Sleep(1);}check(!frame->appearanceCommandTask.valid()&&!manager.fileInfoDecisionPending,"A failed appearance command can be retried and acknowledged");}
   frame->appearanceCommandObserver={};const auto saved=Json::parse(readText(manager.root/L"state.json"))["Settings"];check(str(saved,"UserAgent")=="Appearance concurrent writer","Accepted appearance preserves unrelated live settings");
   if(mode=="font"||mode=="font-reset"){LOGFONTW font{};frame->font.GetLogFont(&font);check(font.lfHeight==-frame->px(mode=="font"?15:11)&&font.lfWeight==(mode=="font"?700:400)&&std::wstring(font.lfFaceName)==(mode=="font"?L"Consolas":L"Tahoma"),"Font intent and Reset render the acknowledged exact font metrics");}
   else if(mode=="categories")check(yes(saved,"HideCategories")&&yes(frame->renderedAppearance,"HideCategories")&&!(frame->categoryFrame.GetStyle()&WS_VISIBLE)&&GetFocus()==frame->table.GetSafeHwnd(),"Acknowledged category hiding changes the pane and returns focus to downloads");
   else if(mode.rfind("tray-",0)==0){auto expected=rendered;expected["TrayIcon"]=mode=="tray-color"?"Color":mode=="tray-classic"?"Classic":"Hidden";check(frame->renderedAppearance==expected&&str(saved,"TrayIcon")==str(expected,"TrayIcon"),"Tray acknowledgement updates only its rendered preference and exact durable style");}
   else check(!yes(saved,"DarkMode")&&!yes(frame->renderedAppearance,"DarkMode"),"Dark toggle uses the current committed value and publishes its saved render");
   if(mode=="exit"||mode=="exit-failure"){if(mode=="exit-failure")frame->SendMessage(WM_COMMAND,CMD_EXIT);const auto closeEnd=GetTickCount64()+10000;while(IsWindow(window)&&GetTickCount64()<closeEnd){pumpOne();Sleep(1);}check(!IsWindow(window),"Deferred or retried explicit Exit completes after all accepted appearance work");const auto final=Json::parse(readText(manager.root/L"state.json"))["Settings"];check(!yes(final,"DarkMode")&&num(final["ListLayout"],"SortColumn",-1)==8,"Final background cleanup preserves the accepted setting and newest layout");frame=nullptr;AfxGetApp()->m_pMainWnd=nullptr;}
   else if(mode=="close-tray"){const auto closeEnd=GetTickCount64()+5000;while(IsWindowVisible(window)&&GetTickCount64()<closeEnd){pumpOne();Sleep(1);}check(IsWindow(window)&&!IsWindowVisible(window),"Deferred Close uses the current Close to Tray policy after saving");}
   else{if(mode=="cancel-after-ack")for(int i=0;i<20;++i){pumpOne();Sleep(1);}check(IsWindowVisible(window),"Ordinary appearance acknowledgement keeps the main window visible");}
#endif
  }catch(...){writer.open();
#ifdef UDM_APPEARANCE_COMMAND_AVAILABLE
   if(frame)frame->appearanceCommandTask.finish();if(after)after->open();
#endif
   throw;}
 }
};
class AppearanceCommandApplication:public CWinApp {int result=1;public:BOOL InitInstance()override{CWinApp::InitInstance();INITCOMMONCONTROLSEX c{sizeof(c),ICC_WIN95_CLASSES|ICC_DATE_CLASSES|ICC_PROGRESS_CLASS};InitCommonControlsEx(&c);AfxOleInit();int argc=0;auto args=CommandLineToArgvW(GetCommandLineW(),&argc);
 try{if(!args||argc!=2)throw std::runtime_error("Fresh output required");output=args[1];if(fs::exists(output)){output.clear();throw std::runtime_error("Fresh output required");}fs::create_directories(output);for(const std::string mode:{"dark","categories","font","font-reset","tray-color","tray-classic","tray-hidden","failure-retry","close-tray","cancel-close","cancel-after-ack","exit","exit-failure","destroy-pending"}){Manager manager(output/wide(mode));auto settings=manager.state["Settings"];settings["CloseToTray"]=mode=="close-tray"||mode=="cancel-close"||mode=="cancel-after-ack";settings["ShowTipsOnStartup"]=false;settings["ClipboardMonitor"]=false;settings["Sound"]=false;settings["CategoryFolders"]=false;settings["DarkMode"]=false;settings["HideCategories"]=false;settings["TrayIcon"]="Color";settings["FontName"]="Consolas";settings["FontHeight"]=14;settings["FontWeight"]=700;manager.setSettings(settings);MainWindow* frame=new MainWindow(manager);m_pMainWnd=frame;try{ToolbarComponentTest::run(frame,manager,mode);}catch(...){m_pMainWnd=nullptr;ToolbarComponentTest::cleanup(frame);throw;}m_pMainWnd=nullptr;ToolbarComponentTest::cleanup(frame);manager.stop();}result=0;
 }catch(const std::exception& error){failure=error.what();}if(args)LocalFree(args);if(!output.empty())atomicText(output/L"results.json",Json{{"passed",result==0},{"checks",checks},{"error",failure},{"diagnostics",timings}}.dump(2),false);return FALSE;}int ExitInstance()override{AfxOleTerm(FALSE);return result;}};AppearanceCommandApplication application;
}
