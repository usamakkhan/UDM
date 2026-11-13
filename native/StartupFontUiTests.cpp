// Exercises only frames created under a fresh private catalog by this process.
#define UDM_TOOLBAR_COMPONENT_TEST
#include "App.cpp"
namespace udm {
static Json outcomes=Json::array(),cases=Json::array();
static void expect(bool value,const char* message){outcomes.push_back({{"name",message},{"passed",value}});if(!value)throw std::runtime_error(message);}
struct StartupFontMessages {
 struct Entry {HWND window=nullptr;unsigned count=0;};
 std::array<Entry,256> entries{};size_t used=0;bool overflow=false;HHOOK hook=nullptr;
 static inline StartupFontMessages* current=nullptr;
 static LRESULT CALLBACK observe(int code,WPARAM w,LPARAM l){
  if(code>=0&&current){const auto message=reinterpret_cast<const CWPSTRUCT*>(l);
   if(message&&message->message==WM_SETFONT){
    auto& probe=*current;size_t index=0;while(index<probe.used&&probe.entries[index].window!=message->hwnd)++index;
    if(index<probe.used)++probe.entries[index].count;
    else if(probe.used<probe.entries.size())probe.entries[probe.used++]={message->hwnd,1};else probe.overflow=true;
   }
  }
  return CallNextHookEx(nullptr,code,w,l);
 }
 StartupFontMessages(){current=this;hook=SetWindowsHookExW(WH_CALLWNDPROC,observe,nullptr,GetCurrentThreadId());if(!hook){current=nullptr;throw std::runtime_error("Cannot attach owned-thread font probe.");}}
 void stop(){if(hook){UnhookWindowsHookEx(hook);hook=nullptr;}current=nullptr;}
 ~StartupFontMessages(){stop();}
 unsigned count(HWND window)const{for(size_t i=0;i<used;++i)if(entries[i].window==window)return entries[i].count;return 0;}
};
class ToolbarComponentTest {
public:
 static void run(Manager& manager,const char* name){
  auto prefs=manager.state["Settings"];const bool custom=std::string_view(name)=="custom";
  prefs["FontName"]=custom?"Consolas":"Tahoma";prefs["FontHeight"]=custom?18:11;prefs["FontWeight"]=custom?700:400;
  prefs["DarkMode"]=custom;prefs["ClipboardMonitor"]=false;prefs["DropBasket"]=false;prefs["CloseToTray"]=false;prefs["Sound"]=false;
  manager.setSettings(prefs);StartupFontMessages probe;auto frame=new MainWindow(manager);probe.stop();
  try{
   expect(!probe.overflow,"Owned-thread font probe stayed within its fixed buffer");
   const std::vector<std::pair<const char*,CWnd*>> controls={{"table",&frame->table},{"tree",&frame->tree},{"categoryHeading",&frame->categoryHeading},{"status",&frame->status},{"search",&frame->search},{"searchLabel",&frame->searchLabel},{"searchFind",&frame->searchFind},{"searchSettings",&frame->searchSettings},{"searchClose",&frame->searchClose},{"clipboard",&frame->clipboard}};
   Json counts=Json::object();
   for(const auto& [key,control]:controls){
    expect(control->GetSafeHwnd()!=nullptr,"Every main-frame font target exists before first show");
    counts[key]=probe.count(control->GetSafeHwnd());
    expect(probe.count(control->GetSafeHwnd())>0,"Every main-frame font target received font binding");
   }
   const auto validate=[&](const Json& expected){
    LOGFONTW logical{};expect(frame->font.GetLogFont(&logical)!=0,"The main-frame font has valid logical metrics");
    expect(logical.lfHeight==-frame->px((int)num(expected,"FontHeight",11))&&logical.lfWeight==num(expected,"FontWeight",400)&&utf8(logical.lfFaceName)==str(expected,"FontName","Tahoma"),"Main-frame font matches saved name size weight and DPI");
    for(const auto& [key,control]:controls)expect((HFONT)control->SendMessage(WM_GETFONT)==(HFONT)frame->font.GetSafeHandle(),"Each main-frame control uses the final saved font");
    expect((HFONT)frame->table.GetHeaderCtrl()->SendMessage(WM_GETFONT)==(HFONT)frame->font.GetSafeHandle(),"Column header uses the final saved font");
    expect((HFONT)frame->toolbar.SendMessage(WM_GETFONT)==(HFONT)frame->font.GetSafeHandle(),"Toolbar uses the final saved font");
   };
   validate(prefs);
   expect(!frame->IsWindowVisible(),"Font initialization does not reveal the main frame");
   auto changed=prefs;changed["FontName"]="Arial";changed["FontHeight"]=14;changed["FontWeight"]=600;changed["DarkMode"]=!custom;
   manager.setSettings(changed);frame->applyAppearance();validate(changed);
   manager.setSettings(prefs);frame->applyAppearance();validate(prefs);
   cases.push_back({{"case",name},{"fontMessagesAtStartup",counts},{"fontName",str(prefs,"FontName")},{"fontHeight",num(prefs,"FontHeight")},{"fontWeight",num(prefs,"FontWeight")},{"dpi",frame->windowDpi()}});
  }catch(...){frame->quitting=true;frame->SendMessage(WM_CLOSE);throw;}
  frame->quitting=true;frame->SendMessage(WM_CLOSE);
 }
};
class StartupFontTestApplication:public CWinApp {
 int code=1;
public:
 BOOL InitInstance()override{
  CWinApp::InitInstance();INITCOMMONCONTROLSEX controls{sizeof(controls),ICC_WIN95_CLASSES|ICC_DATE_CLASSES|ICC_PROGRESS_CLASS};InitCommonControlsEx(&controls);AfxOleInit();SetRegistryKey(L"UDM startup-font fixture");
  fs::path output;std::string error;
  try{
   int count=0;auto args=CommandLineToArgvW(GetCommandLineW(),&count);if(!args)throw std::runtime_error("Missing fixture arguments.");
   if(count==2)output=fs::absolute(args[1]);LocalFree(args);
   if(count!=2||output.empty()||fs::exists(output))throw std::runtime_error("Use a fresh private fixture folder.");
   fs::create_directories(output);
   for(const auto* name:{"default","custom"}){Manager manager(output/wide(name));ToolbarComponentTest::run(manager,name);}
   code=0;
  }catch(const std::exception& e){error=e.what();}
  if(!output.empty()&&fs::is_directory(output))atomicText(output/L"results.json",Json{{"passed",code==0},{"error",error},{"cases",cases},{"checks",outcomes}}.dump(2),false);
  return FALSE;
 }
 int ExitInstance()override{AfxOleTerm(FALSE);return code;}
};
StartupFontTestApplication application;
}
