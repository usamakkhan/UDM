#define UDM_TOOLBAR_COMPONENT_TEST
#include "App.cpp"
namespace udm {
static fs::path output;
static Json appearanceChecks=Json::array(),appearanceTimings=Json::object();
static std::string appearanceFailure;
static void check(bool value,const char* name){appearanceChecks.push_back({{"name",name},{"passed",value}});if(!value)throw std::runtime_error(name);}
static double stamp(){LARGE_INTEGER n,f;QueryPerformanceCounter(&n);QueryPerformanceFrequency(&f);return 1000.0*n.QuadPart/f.QuadPart;}
struct AppearanceWriter {
 std::promise<void> release;std::thread thread;std::atomic_bool held=false,expired=false;bool opened=false;
 explicit AppearanceWriter(Manager& manager){auto gate=release.get_future().share();thread=std::thread([this,&manager,gate]{Lock lock(manager.mutex);held=true;if(gate.wait_for(std::chrono::milliseconds(2500))!=std::future_status::ready)expired=true;held=false;});const auto end=GetTickCount64()+3000;while(!held&&GetTickCount64()<end)Sleep(1);}
 void open(){if(!opened){opened=true;release.set_value();}if(thread.joinable())thread.join();}~AppearanceWriter(){open();}
};
class ToolbarComponentTest {
 static LOGFONTW fontOf(MainWindow& frame){LOGFONTW value{};frame.font.GetLogFont(&value);return value;}
 static void dpi(MainWindow& frame,int next){CRect area;frame.GetWindowRect(&area);const int previous=frame.windowDpi();area.right=area.left+MulDiv(area.Width(),next,previous);area.bottom=area.top+MulDiv(area.Height(),next,previous);frame.SendMessage(WM_DPICHANGED,MAKEWPARAM(next,next),(LPARAM)(RECT*)&area);}
 template<class Action>static void heldMessage(MainWindow& frame,Manager& manager,const char* name,Action action){
  const auto disk=readText(manager.root/L"state.json");AppearanceWriter writer(manager);const double began=stamp();action();appearanceTimings[name]={{"returnMs",stamp()-began},{"writerHeld",writer.held.load()},{"writerExpired",writer.expired.load()}};
  check(writer.held&&!writer.expired,"Appearance event returns before the catalog writer releases");check(readText(manager.root/L"state.json")==disk,"Appearance event adds no catalog write");writer.open();
 }
public:
 static void cleanup(MainWindow*& frame){if(frame){closeOwnedMainWindow(frame);frame=nullptr;}}
 static void run(MainWindow& frame,Manager& manager){
  frame.KillTimer(1);frame.ShowWindow(SW_SHOWNOACTIVATE);const int originalDpi=frame.windowDpi();
  heldMessage(frame,manager,"system-colors",[&]{frame.SendMessage(WM_SYSCOLORCHANGE);});
#ifdef UDM_APPEARANCE_REPAINT_AVAILABLE
  check(!frame.appearanceReadSnapshot&&str(frame.renderedAppearance,"FontName")=="Tahoma","Initial appearance is owned after its render scope ends");
  Form companion("Appearance companion",180,60,&frame);companion.modeless=true;CWnd* button=nullptr;companion.init=[&]{companion.label("Companion theme",8,8,160);button=companion.button("Close",8,30,70,[]{});};check(companion.Create(100,&frame)!=FALSE,"Create a real modeless companion for theme propagation");companion.ShowWindow(SW_SHOWNOACTIVATE);
  struct CloseCompanion{Form& window;~CloseCompanion(){if(window.GetSafeHwnd())window.DestroyWindow();}} closeCompanion{companion};
  const auto old=frame.renderedAppearance;auto next=manager.snapshot()["Settings"];next["DarkMode"]=false;next["FontName"]="Consolas";next["FontHeight"]=14;next["FontWeight"]=700;next["ToolbarStyle"]="Text only";next["HideCategories"]=false;next["HideToolbar"]=true;next["TrayIcon"]="Hidden";next["UserAgent"]="Live settings distinct from rendered appearance";manager.setSettings(next);const auto disk=readText(manager.root/L"state.json");
  heldMessage(frame,manager,"setting-change",[&]{frame.SendMessage(WM_SETTINGCHANGE,0,(LPARAM)L"ImmersiveColorSet");});
  check(uiDark==useDarkUi(true,windowsHighContrast()),"Busy theme refresh uses the last rendered dark preference");check((button->GetStyle()&BS_TYPEMASK)==(uiDark?BS_OWNERDRAW:BS_PUSHBUTTON),"Busy theme refresh reaches the real companion button");
  check(frame.renderedAppearance==old,"A busy theme refresh does not replace cached font toolbar or layout settings");
  frame.SendMessage(WM_SYSCOLORCHANGE);check(!uiDark&&!yes(frame.renderedAppearance,"DarkMode"),"Available settings supply the latest dark preference");check(str(frame.renderedAppearance,"FontName")==str(old,"FontName")&&str(frame.renderedAppearance,"ToolbarStyle")==str(old,"ToolbarStyle"),"A theme-only refresh advances only its rendered preference");check((button->GetStyle()&BS_TYPEMASK)==BS_PUSHBUTTON,"Latest light theme restores the companion native button");
  const int originalWidth=frame.table.GetColumnWidth(0)+13;frame.table.SetColumnWidth(0,originalWidth);frame.table.SetColumnWidth(8,0);const int nextDpi=originalDpi==144?192:144;const auto order=frame.listLayout()["Order"];
  heldMessage(frame,manager,"dpi-busy",[&]{dpi(frame,nextDpi);});
  auto actual=fontOf(frame);check(frame.windowDpi()==nextDpi&&actual.lfHeight==-frame.px(12)&&std::wstring(actual.lfFaceName)==L"Tahoma","Busy DPI change renders the cached font at the new scale");check(!uiDark&&yes(frame.renderedAppearance,"HideCategories")&&!yes(frame.renderedAppearance,"HideToolbar"),"Busy DPI change preserves the last applied layout and latest applied palette");check(!(frame.categoryFrame.GetStyle()&WS_VISIBLE)&&(::GetWindowLongW(frame.toolbar.GetSafeHwnd(),GWL_STYLE)&WS_VISIBLE),"Busy DPI layout uses the same owned appearance snapshot as its font");check(str(frame.renderedAppearance,"ToolbarStyle")=="Icons only"&&!frame.appearanceReadSnapshot,"Toolbar rendering and scope cleanup retain the cached snapshot");
  heldMessage(frame,manager,"dpi-roundtrip",[&]{dpi(frame,originalDpi);});check(frame.table.GetColumnWidth(0)==originalWidth&&frame.table.GetColumnWidth(8)==0&&frame.listLayout()["Order"]==order,"DPI roundtrip preserves custom and hidden widths and column order");
  dpi(frame,nextDpi);actual=fontOf(frame);check(actual.lfHeight==-frame.px(14)&&actual.lfWeight==700&&std::wstring(actual.lfFaceName)==L"Consolas","Available DPI repaint uses the latest complete settings");check(frame.renderedAppearance==next&&!frame.appearanceReadSnapshot,"Successful full repaint owns the complete latest settings and ends its scope");check((frame.categoryFrame.GetStyle()&WS_VISIBLE)&&!(::GetWindowLongW(frame.toolbar.GetSafeHwnd(),GWL_STYLE)&WS_VISIBLE),"Fresh DPI layout applies latest toolbar and category visibility");
  appearanceTimings["toolbar-visibility"]={{"storedToolbarStyleVisible",(frame.toolbar.GetStyle()&WS_VISIBLE)!=0},{"actualWindowStyleVisible",(::GetWindowLongW(frame.toolbar.GetSafeHwnd(),GWL_STYLE)&WS_VISIBLE)!=0}};heldMessage(frame,manager,"resize-hidden-toolbar",[&]{CRect client;frame.GetClientRect(&client);frame.layout(client.Width(),client.Height());});check(!(::GetWindowLongW(frame.toolbar.GetSafeHwnd(),GWL_STYLE)&WS_VISIBLE),"Busy ordinary resize keeps a genuinely hidden toolbar hidden");
  auto owned=next;owned["DarkMode"]=true;owned["FontName"]="Tahoma";owned["FontHeight"]=10;owned["FontWeight"]=400;owned["ToolbarStyle"]="Icons and text";owned["HideToolbar"]=false;owned["HideCategories"]=true;frame.optionsAppearanceSnapshot=owned;
  heldMessage(frame,manager,"dpi-options",[&]{dpi(frame,originalDpi);});actual=fontOf(frame);check(actual.lfHeight==-frame.px(10)&&std::wstring(actual.lfFaceName)==L"Tahoma"&&frame.renderedAppearance==owned,"Accepted Options snapshot takes priority over cache and busy model");check(frame.optionsAppearanceSnapshot&&*frame.optionsAppearanceSnapshot==owned&&!frame.appearanceReadSnapshot,"Repaint preserves the outer Options owner and restores its own read scope");frame.optionsAppearanceSnapshot.reset();
  const auto before=frame.renderedAppearance;const auto beforeFont=fontOf(frame);frame.appearanceRenderObserver=[]{throw std::runtime_error("Owned render preparation failure");};bool rejected=false;try{frame.applyAppearanceSnapshot(next);}catch(const std::exception& error){rejected=std::string(error.what())=="Owned render preparation failure";}frame.appearanceRenderObserver={};
  actual=fontOf(frame);check(rejected&&!frame.appearanceReadSnapshot&&frame.renderedAppearance==before&&actual.lfHeight==beforeFont.lfHeight,"Render preparation failure restores scope and preserves the previously applied cache and font");
  bool renderReader=false,liveReader=false;frame.appearanceRenderObserver=[&]{renderReader=frame.appearanceReadSnapshot&&frame.appearancePreferences()==owned&&frame.repaintAppearancePreferences()==owned;liveReader=frame.preferences()==next;};frame.applyAppearanceSnapshot(owned);frame.appearanceRenderObserver={};check(renderReader&&liveReader,"Render readers use their owned snapshot without redirecting ordinary live preference commands");check(!frame.appearanceReadSnapshot&&readText(manager.root/L"state.json")==disk&&manager.snapshot()["Settings"]==next,"All theme and DPI rendering leaves the model and durable settings unchanged");
  bool nested=false,restored=false,innerReader=false;frame.appearanceRenderObserver=[&]{if(nested){innerReader=frame.appearancePreferences()==next;return;}nested=true;const auto outer=frame.appearanceReadSnapshot;frame.applyAppearanceSnapshot(next);restored=frame.appearanceReadSnapshot==outer&&frame.appearancePreferences()==owned;};frame.applyAppearanceSnapshot(owned);frame.appearanceRenderObserver={};check(innerReader&&restored&&!frame.appearanceReadSnapshot&&frame.renderedAppearance==owned,"Nested appearance apply restores the outer read scope before its final render");
  frame.applyAppearance();check(frame.renderedAppearance==next&&!frame.appearanceReadSnapshot,"An ordinary successful appearance apply still reads current committed settings");
#endif
 }
};
class AppearanceApplication:public CWinApp {
 int result=1;
public:BOOL InitInstance()override{CWinApp::InitInstance();INITCOMMONCONTROLSEX common{sizeof(common),ICC_WIN95_CLASSES|ICC_DATE_CLASSES|ICC_PROGRESS_CLASS};InitCommonControlsEx(&common);AfxOleInit();int argc=0;auto args=CommandLineToArgvW(GetCommandLineW(),&argc);
 try{if(!args||argc!=2)throw std::runtime_error("Fresh output required");output=args[1];if(fs::exists(output)){output.clear();throw std::runtime_error("Fresh output required");}fs::create_directories(output);Manager manager(output/L"catalog");auto settings=manager.state["Settings"];settings["CloseToTray"]=false;settings["ShowTipsOnStartup"]=false;settings["ClipboardMonitor"]=false;settings["Sound"]=false;settings["CategoryFolders"]=false;settings["DarkMode"]=true;settings["FontName"]="Tahoma";settings["FontHeight"]=12;settings["FontWeight"]=500;settings["ToolbarStyle"]="Icons only";settings["ToolbarSize"]="Small";settings["HideCategories"]=true;settings["HideToolbar"]=false;settings["TrayIcon"]="Classic";manager.setSettings(settings);MainWindow* frame=new MainWindow(manager);m_pMainWnd=frame;try{ToolbarComponentTest::run(*frame,manager);}catch(...){m_pMainWnd=nullptr;ToolbarComponentTest::cleanup(frame);throw;}m_pMainWnd=nullptr;ToolbarComponentTest::cleanup(frame);manager.stop();result=0;
 }catch(const std::exception& error){appearanceFailure=error.what();}if(args)LocalFree(args);if(!output.empty())atomicText(output/L"results.json",Json{{"passed",result==0},{"checks",appearanceChecks},{"error",appearanceFailure},{"diagnostics",appearanceTimings}}.dump(2),false);return FALSE;}
 int ExitInstance()override{AfxOleTerm(FALSE);return result;}
};AppearanceApplication application;
}
