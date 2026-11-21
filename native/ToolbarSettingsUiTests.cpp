#define UDM_TOOLBAR_COMPONENT_TEST
#include "App.cpp"
namespace udm {
static fs::path output;static Json checks=Json::array(),timings=Json::object();static std::string failure;
static void check(bool value,const char* name){checks.push_back({{"name",name},{"passed",value}});if(!value)throw std::runtime_error(name);}
static double stamp(){LARGE_INTEGER n,f;QueryPerformanceCounter(&n);QueryPerformanceFrequency(&f);return 1000.0*n.QuadPart/f.QuadPart;}
static void strip(const fs::path& path,uint8_t blue=190){Bytes bytes(54+192*16*3);auto put=[&](int at,uint32_t value,int count){for(int i=0;i<count;++i)bytes[at+i]=(uint8_t)(value>>(i*8));};put(0,0x4d42,2);put(2,(uint32_t)bytes.size(),4);put(10,54,4);put(14,40,4);put(18,192,4);put(22,16,4);put(26,1,2);put(28,24,2);for(size_t at=54;at<bytes.size();at+=3){bytes[at]=blue;bytes[at+1]=90;bytes[at+2]=40;}std::ofstream file(path,std::ios::binary);file.write((char*)bytes.data(),bytes.size());if(!file)throw std::runtime_error("Owned skin write failure");}
struct ToolbarWriter {
 std::promise<void> release;std::thread thread;std::atomic_bool held=false,expired=false;bool opened=false;
 explicit ToolbarWriter(Manager& manager,bool change=true){auto gate=release.get_future().share();thread=std::thread([this,&manager,gate,change]{Lock lock(manager.mutex);if(change){manager.state["Settings"]["UserAgent"]="Toolbar concurrent writer";manager.state["Settings"]["ToolbarSize"]="Medium";saveToolbarLayout(manager.state["Settings"],{0,-1,2,11});}held=true;if(gate.wait_for(std::chrono::milliseconds(2500))!=std::future_status::ready)expired=true;held=false;});const auto end=GetTickCount64()+3000;while(!held&&GetTickCount64()<end)Sleep(1);}
 void open(){if(!opened){opened=true;release.set_value();}if(thread.joinable())thread.join();}~ToolbarWriter(){open();}
};
static void pumpOne(){MSG message{};if(PeekMessageW(&message,nullptr,0,0,PM_REMOVE)&&message.message!=WM_QUIT){TranslateMessage(&message);DispatchMessageW(&message);}}
class ToolbarComponentTest {
 static COLORREF center(MainWindow& frame){int w=0,h=0;ImageList_GetIconSize(frame.toolbarNormal.GetSafeHandle(),&w,&h);CImage image;image.Create(w,h,32);auto dc=image.GetDC();ImageList_Draw(frame.toolbarNormal.GetSafeHandle(),0,dc,0,0,ILD_NORMAL);image.ReleaseDC();return image.GetPixel(w/2,h/2);}
public:
 static void cleanup(MainWindow*& frame){if(frame){closeOwnedMainWindow(frame);frame=nullptr;}}
 static void run(MainWindow*& frame,Manager& manager,const std::string& mode,const fs::path& info,const fs::path& bitmap){
  frame->KillTimer(1);frame->ShowWindow(SW_SHOWNOACTIVATE);const auto window=frame->GetSafeHwnd();const auto disk=readText(manager.root/L"state.json");
#ifdef UDM_TOOLBAR_SETTINGS_COMMAND_AVAILABLE
  const auto rendered=frame->renderedAppearance;std::unique_ptr<ToolbarWriter> after;int skinReads=0;bool fail=mode=="layout-failure";
  frame->appearanceCommandObserver=[&](const char* stage){if(std::string(stage)=="before-skin-read")++skinReads;if(std::string(stage)=="before-save"&&fail){fail=false;throw std::runtime_error("Owned toolbar save failure");}if(std::string(stage)=="captured"&&(mode=="small"||mode=="skin"||mode=="skin-refresh")){if(mode!="small"){fs::remove(info);fs::remove(bitmap);}after=std::make_unique<ToolbarWriter>(manager,false);}};
  if(mode=="skin-refresh"||mode=="reload")strip(bitmap,80);
  const std::vector<int> desired=mode=="empty"?std::vector<int>{}:std::vector<int>{11,-1,0,-1,2};
#endif
  ToolbarWriter writer(manager);
  try{const auto began=stamp();
#ifdef UDM_TOOLBAR_SETTINGS_COMMAND_AVAILABLE
   if(mode=="layout"||mode=="empty"||mode=="layout-failure"){frame->setToolbarButtons(desired);frame->saveToolbarButtons();}
   else if(mode=="skin"){frame->toolbarSkins={info};frame->SendMessage(WM_COMMAND,31000);}
   else frame->SendMessage(WM_COMMAND,mode=="medium"?CMD_TOOLBAR_MEDIUM:mode=="large"||mode=="skin-refresh"?CMD_TOOLBAR_LARGE:mode=="icons"?CMD_TOOLBAR_ICONSONLY:mode=="text"?CMD_TOOLBAR_TEXTONLY:mode=="captions"?CMD_TOOLBAR_ICONSTEXT:mode=="neon"?CMD_TOOLBAR_NEON:mode=="raised"?CMD_TOOLBAR_3D:mode=="hide"?CMD_TOOLBAR_HIDE:mode=="reload"?CMD_TOOLBAR_RELOAD:CMD_TOOLBAR_SMALL);
#else
   frame->SendMessage(WM_COMMAND,CMD_TOOLBAR_SMALL);
#endif
   timings[mode]["commandReturnMs"]=stamp()-began;check(writer.held&&!writer.expired,"Toolbar command returns before the catalog writer releases");
#ifdef UDM_TOOLBAR_SETTINGS_COMMAND_AVAILABLE
   check(frame->appearanceCommandTask.valid()&&manager.fileInfoDecisionPending&&frame->IsWindowEnabled(),"One toolbar choice is accepted without disabling the main window");check(readText(manager.root/L"state.json")==disk,"A pending toolbar choice does not write uncommitted settings");
   CMenu popup;popup.CreatePopupMenu();const auto menuStart=stamp();frame->fillToolbarMenu(popup);timings[mode]["menuReturnMs"]=stamp()-menuStart;check(writer.held&&!writer.expired&&popup.GetMenuItemCount()>0,"Toolbar menu uses an owned fallback instead of waiting for Manager");
   frame->SendMessage(WM_COMMAND,CMD_TOOLBAR_MEDIUM);frame->SendMessage(WM_COMMAND,CMD_SORT_ADDED);frame->SendMessage(WM_COMMAND,CMD_SELECTALL);frame->SendMessage(WM_TIMER,1,0);frame->Invalidate();frame->UpdateWindow();check(writer.held&&!writer.expired&&frame->sortColumn==8,"Duplicate toolbar requests local sorting timers and painting remain nonblocking");
   if(mode=="exit"||mode=="layout-failure"){frame->SendMessage(WM_COMMAND,CMD_EXIT);frame->SendMessage(SHOW_APP);check(frame->appearanceClosePending&&frame->quitting&&IsWindow(window),"Exit waits for the accepted toolbar choice and cannot be cancelled by Show");}
   writer.open();
   if(mode=="layout-failure"){const auto end=GetTickCount64()+10000;while(!frame->appearanceCommandTask.ready()&&GetTickCount64()<end)Sleep(1);check(frame->appearanceCommandTask.ready(),"Failed layout result becomes ready without UI acknowledgement");ToolbarWriter held(manager,false);frame->pollAppearanceCommand();check(held.held&&!held.expired&&frame->currentToolbarLayout()==std::vector<int>({0,-1,2,11})&&toolbarLayout(frame->renderedAppearance)==std::vector<int>({0,-1,2,11}),"Failed customization restores the worker-owned current layout without reading Manager again");check(!frame->quitting&&!frame->appearanceClosePending&&readText(manager.root/L"state.json")==disk,"Failed layout cancels deferred Exit and preserves the prior catalog");held.open();frame->appearanceCommandObserver={};frame->setToolbarButtons(desired);frame->saveToolbarButtons();}
   const auto end=GetTickCount64()+10000;while(frame->appearanceCommandTask.valid()&&GetTickCount64()<end){pumpOne();Sleep(1);}check(!frame->appearanceCommandTask.valid()&&!manager.fileInfoDecisionPending&&!frame->toolbarPreparedRead,"Toolbar acknowledgement joins work releases the guard and clears the prepared read scope");
   if(after){check(after->held&&!after->expired,"Acknowledged toolbar rendering completes while another writer holds Manager");after->open();}
   frame->appearanceCommandObserver={};
   if(mode=="reload"){check(readText(manager.root/L"state.json")==disk&&skinReads==1,"Reload reads the current skin exactly once without a catalog write");}
   else{const auto saved=Json::parse(readText(manager.root/L"state.json"))["Settings"];check(str(saved,"UserAgent")=="Toolbar concurrent writer","Toolbar save preserves concurrent unrelated settings");}
   if(mode=="layout"||mode=="empty"||mode=="layout-failure"){check(frame->currentToolbarLayout()==desired&&toolbarLayout(frame->renderedAppearance)==desired&&toolbarLayout(Json::parse(readText(manager.root/L"state.json"))["Settings"])==desired,"Native layout cache and durable compatibility fields agree after saving");if(mode=="empty")check(!frame->toolbar.IsWindowVisible(),"An explicitly empty saved toolbar remains hidden");}
   else if(mode=="skin"||mode=="skin-refresh"||mode=="reload"){int w=0,h=0;ImageList_GetIconSize(frame->toolbarNormal.GetSafeHandle(),&w,&h);const double factor=str(frame->renderedAppearance,"ToolbarSize")=="Medium"?0.75:1.0;check(frame->toolbarSkinError.empty()&&frame->toolbarSkinName=="Owned toolbar"&&w==(int)std::lround(16*factor*frame->windowDpi()/96.0)&&h==w&&center(*frame)==RGB(40,90,mode=="skin"?190:80),"Prepared custom pixels render exactly without reopening removed or replaced skin files");check(skinReads==1,"A selected or refreshed skin is validated and decoded once for its acknowledgement");}
   else if(mode=="icons"||mode=="text"||mode=="captions"){const auto style=mode=="icons"?"Icons only":mode=="text"?"Text only":"Icons and text";TBBUTTON first{};frame->toolbar.GetButton(0,&first);check(str(frame->renderedAppearance,"ToolbarStyle")==style&&frame->toolbar.GetMaxTextRows()==(mode=="icons"?0:1)&&(mode!="text"||first.iBitmap==I_IMAGENONE),"Caption and icon modes apply their acknowledged native button definitions");}
   else if(mode=="neon"||mode=="raised")check(str(frame->renderedAppearance,"ToolbarLook")== (mode=="neon"?"Neon":"3D Style")&&str(frame->renderedAppearance,"ToolbarSkin").empty(),"Built-in looks clear the external skin and update the rendered preference");
   else if(mode=="hide")check(!yes(frame->renderedAppearance,"HideToolbar")&&frame->toolbar.IsWindowVisible(),"Visibility toggle uses the current value and restores the native toolbar");
   else{int w=0,h=0;ImageList_GetIconSize(frame->toolbarNormal.GetSafeHandle(),&w,&h);const int size=mode=="medium"?24:mode=="large"?32:16;check(w==MulDiv(size,frame->windowDpi(),96)&&h==w,"Toolbar size choices publish exact native image dimensions");}
   if(mode=="exit"){const auto closeEnd=GetTickCount64()+10000;while(IsWindow(window)&&GetTickCount64()<closeEnd){pumpOne();Sleep(1);}check(!IsWindow(window),"Accepted toolbar Exit completes through final background cleanup");check(str(Json::parse(readText(manager.root/L"state.json"))["Settings"],"ToolbarSize")=="Small","Completed Exit retains its acknowledged toolbar choice");frame=nullptr;AfxGetApp()->m_pMainWnd=nullptr;}
   else check(IsWindowVisible(window),"Ordinary toolbar acknowledgement leaves the main window visible");
#endif
  }catch(...){writer.open();
#ifdef UDM_TOOLBAR_SETTINGS_COMMAND_AVAILABLE
   if(frame)frame->appearanceCommandTask.finish();if(after)after->open();
#endif
   throw;}
 }
};
class ToolbarSettingsApplication:public CWinApp {int result=1;public:BOOL InitInstance()override{CWinApp::InitInstance();INITCOMMONCONTROLSEX c{sizeof(c),ICC_WIN95_CLASSES|ICC_DATE_CLASSES|ICC_PROGRESS_CLASS};InitCommonControlsEx(&c);AfxOleInit();int argc=0;auto args=CommandLineToArgvW(GetCommandLineW(),&argc);
 try{if(!args||argc!=2)throw std::runtime_error("Fresh output required");output=args[1];if(fs::exists(output)){output.clear();throw std::runtime_error("Fresh output required");}fs::create_directories(output);for(const std::string mode:{"small","medium","large","icons","text","captions","neon","raised","hide","layout","empty","skin","skin-refresh","reload","layout-failure","exit"}){const auto folder=output/wide(mode);fs::create_directories(folder);const auto info=folder/L"owned.tbi",bitmap=folder/L"owned.bmp";strip(bitmap);atomicText(info,"v=3\nname=Owned toolbar\nlarge=owned.bmp\n",false);Manager manager(folder/L"catalog");auto settings=manager.state["Settings"];settings["CloseToTray"]=false;settings["ShowTipsOnStartup"]=false;settings["ClipboardMonitor"]=false;settings["Sound"]=false;settings["CategoryFolders"]=false;settings["HideToolbar"]=mode=="hide";settings["ToolbarSize"]=mode=="large"?"Small":"Large";settings["ToolbarStyle"]=mode=="captions"?"Icons only":"Icons and text";settings["ToolbarLook"]=mode=="neon"?"3D Style":"Neon";settings["ToolbarSkin"]=(mode=="skin-refresh"||mode=="reload")?utf8(info.wstring()):"";manager.setSettings(settings);MainWindow* frame=new MainWindow(manager);m_pMainWnd=frame;try{ToolbarComponentTest::run(frame,manager,mode,info,bitmap);}catch(...){m_pMainWnd=nullptr;ToolbarComponentTest::cleanup(frame);throw;}m_pMainWnd=nullptr;ToolbarComponentTest::cleanup(frame);manager.stop();}result=0;
 }catch(const std::exception& error){failure=error.what();}if(args)LocalFree(args);if(!output.empty())atomicText(output/L"results.json",Json{{"passed",result==0},{"checks",checks},{"error",failure},{"diagnostics",timings}}.dump(2),false);return FALSE;}int ExitInstance()override{AfxOleTerm(FALSE);return result;}};ToolbarSettingsApplication application;
}
