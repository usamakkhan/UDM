#define UDM_TOOLBAR_COMPONENT_TEST
#include "App.cpp"
#include "TransferFixture.hpp"
namespace udm {
static fs::path output;static Json checks=Json::array(),selectionCommandTrace=Json::object();static std::string failure;
static void check(bool value,const char* name){checks.push_back({{"name",name},{"passed",value}});if(!value)throw std::runtime_error(name);}
class ToolbarComponentTest {
 static void select(MainWindow& frame,const std::vector<JobPtr>& chosen){frame.table.SetItemState(-1,0,LVIS_SELECTED);for(int i=0;i<(int)frame.visible.size();++i)if(std::find(chosen.begin(),chosen.end(),frame.visible[i])!=chosen.end())frame.table.SetItemState(i,LVIS_SELECTED|LVIS_FOCUSED,LVIS_SELECTED|LVIS_FOCUSED);}
 static bool enabled(CMenu& menu,UINT id){const auto state=menu.GetMenuState(id,MF_BYCOMMAND);return state!=UINT(-1)&&(state&(MF_DISABLED|MF_GRAYED))==0;}
 static void update(MainWindow& frame,CMenu& menu){frame.SendMessage(WM_INITMENUPOPUP,reinterpret_cast<WPARAM>(menu.GetSafeHmenu()),MAKELPARAM(1,FALSE));}
 static void refresh(MainWindow& frame){const auto end=GetTickCount64()+5000;do{frame.refresh();if(!frame.historyRefreshDeferred)return;Sleep(1);}while(GetTickCount64()<end);throw std::runtime_error("Owned selection refresh stayed busy");}
public:
 static void cleanup(MainWindow*& frame){if(frame){closeOwnedMainWindow(frame);frame=nullptr;}}
 static void run(MainWindow& frame,Manager& manager){
  frame.KillTimer(1);frame.ShowWindow(SW_SHOWNOACTIVATE);TransferFixture origin(1024*1024,1,1);
  auto complete=manager.add(origin.url("/complete"),utf8((output/L"files").wstring()),"complete.bin","Main queue",true);
  auto running=manager.add(origin.url("/steady"),utf8((output/L"files").wstring()),"running.bin","Main queue",true);
  {Lock lock(manager.mutex);complete->data["Status"]="Complete";complete->data["QueueMember"]=false;running->data["Connections"]=1;running->data["LimitKbps"]=64;}
  fs::create_directories(complete->target().parent_path());writeBytes(complete->target(),Bytes{'x'});manager.resume(running);manager.tick();
  auto end=GetTickCount64()+5000;bool receiving=false;while(GetTickCount64()<end){{Lock lock(manager.mutex);receiving=manager.isActive(running)&&num(running->data,"Received")>0;}if(receiving)break;Sleep(1);}
  check(receiving,"The selected-command fixture receives real loopback bytes");refresh(frame);check(std::find(frame.visible.begin(),frame.visible.end(),running)!=frame.visible.end(),"The real list renders the active transfer before selection");
  auto menu=frame.GetMenu()->GetSubMenu(1);select(frame,{complete});update(frame,*menu);check(!enabled(*menu,CMD_STOP),"Completed selection establishes an initially disabled Stop item");select(frame,{running});
  std::promise<void> entered,release;auto ready=entered.get_future();auto gate=release.get_future().share();std::atomic_bool held=false,expired=false;
  std::thread writer([&]{Lock lock(manager.mutex);held=true;entered.set_value();if(gate.wait_for(std::chrono::seconds(5))!=std::future_status::ready)expired=true;held=false;});
  struct Join{std::promise<void>& release;std::thread& thread;~Join(){try{release.set_value();}catch(...){}if(thread.joinable())thread.join();}} joined{release,writer};
  check(ready.wait_for(std::chrono::seconds(2))==std::future_status::ready,"A separate writer owns Manager before the real menu update");update(frame,*menu);
  selectionCommandTrace["busyActive"]={{"selected",frame.selected(false).size()},{"stop",enabled(*menu,CMD_STOP)},{"resume",enabled(*menu,CMD_RESUME)},{"remove",enabled(*menu,CMD_DELETE)},{"writerHeld",held.load()}};
  check(held&&!expired&&enabled(*menu,CMD_STOP),"Stop follows the displayed active selection while Manager is held");
  check(!enabled(*menu,CMD_RESUME)&&!enabled(*menu,CMD_DELETE),"The same active selection disables Resume and Remove during contention");
  select(frame,{complete});update(frame,*menu);check(held&&!expired&&!enabled(*menu,CMD_STOP)&&!enabled(*menu,CMD_RESUME)&&enabled(*menu,CMD_DELETE),"Switching to a completed row updates advice without inheriting the active row state");
  select(frame,{complete,running});update(frame,*menu);check(held&&!expired&&enabled(*menu,CMD_STOP)&&!enabled(*menu,CMD_RESUME)&&!enabled(*menu,CMD_DELETE),"Mixed selection permits Stop and rejects removing an active row");
  select(frame,{});update(frame,*menu);check(held&&!expired&&!enabled(*menu,CMD_STOP)&&!enabled(*menu,CMD_RESUME)&&!enabled(*menu,CMD_DELETE),"Empty selection clears selection-dependent advice while the model is busy");
  release.set_value();writer.join();select(frame,{running});frame.command(CMD_STOP);end=GetTickCount64()+5000;while(manager.isActive(running)&&GetTickCount64()<end)Sleep(1);
  check(!manager.isActive(running),"The actual Stop command pauses the receiving transfer after live validation");
  check(!frame.commandEnabled(CMD_STOP,true)&&frame.commandEnabled(CMD_RESUME,true),"Authoritative invocation sees the current paused state despite the older displayed snapshot");
  {Lock lock(manager.mutex);running->data["LimitKbps"]=0;}frame.command(CMD_RESUME);end=GetTickCount64()+15000;
  bool completed=false;while(GetTickCount64()<end){frame.pollResumeCommands();manager.tick();{Lock lock(manager.mutex);completed=str(running->data,"Status")=="Complete";}if(completed)break;Sleep(1);}
  check(completed,"The actual Resume command completes the owned transfer");origin.expected(output/L"expected.bin");check(fileHash(running->target())==fileHash(output/L"expected.bin"),"The stopped and resumed file matches every expected byte");
 }
};
class SelectionCommandsApplication:public CWinApp {
 int result=1;public:BOOL InitInstance()override{CWinApp::InitInstance();INITCOMMONCONTROLSEX controls{sizeof(controls),ICC_WIN95_CLASSES|ICC_DATE_CLASSES|ICC_PROGRESS_CLASS};InitCommonControlsEx(&controls);AfxOleInit();WSADATA sockets{};if(WSAStartup(MAKEWORD(2,2),&sockets))return FALSE;int argc=0;auto args=CommandLineToArgvW(GetCommandLineW(),&argc);std::unique_ptr<Manager> manager;MainWindow* frame=nullptr;
 try{if(!args||argc!=2)throw std::runtime_error("Pass one fresh output directory");output=args[1];if(fs::exists(output)){output.clear();throw std::runtime_error("Fresh output required");}fs::create_directories(output);manager=std::make_unique<Manager>(output/L"catalog");auto settings=manager->state["Settings"];settings["CloseToTray"]=false;settings["ClipboardMonitor"]=false;settings["Sound"]=false;settings["PrefetchFileInfo"]=false;settings["SuppressCompletionDialog"]=true;settings["SuppressProgressDialog"]=true;settings["ShowTipsOnStartup"]=false;settings["CategoryFolders"]=false;manager->setSettings(settings);frame=new MainWindow(*manager);m_pMainWnd=frame;ToolbarComponentTest::run(*frame,*manager);result=0;}catch(const std::exception& error){failure=error.what();}
 m_pMainWnd=nullptr;ToolbarComponentTest::cleanup(frame);manager.reset();if(args)LocalFree(args);WSACleanup();if(!output.empty())atomicText(output/L"results.json",Json{{"passed",result==0},{"checks",checks},{"error",failure},{"diagnostics",selectionCommandTrace}}.dump(2),false);return FALSE;}
 int ExitInstance()override{AfxOleTerm(FALSE);return result;}
};SelectionCommandsApplication application;
}
