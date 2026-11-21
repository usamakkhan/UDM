#define UDM_TOOLBAR_COMPONENT_TEST
#include "App.cpp"
namespace udm {
static fs::path output;static Json checks=Json::array(),menuTrace=Json::array();static std::string failure;
static void check(bool value,const char* name){checks.push_back({{"name",name},{"passed",value}});if(!value)throw std::runtime_error(name);}
struct MenuWriter {
 std::promise<void> release,entered;std::shared_future<void> gate=release.get_future().share();
 std::atomic_bool held=false,expired=false;std::thread worker;
 explicit MenuWriter(Manager& manager):worker([this,&manager]{Lock lock(manager.mutex);held=true;entered.set_value();if(gate.wait_for(std::chrono::seconds(2))!=std::future_status::ready)expired=true;held=false;}){if(entered.get_future().wait_for(std::chrono::seconds(2))!=std::future_status::ready)throw std::runtime_error("Owned menu writer did not enter");}
 void finish(){try{release.set_value();}catch(...){}if(worker.joinable())worker.join();}
 ~MenuWriter(){finish();}
};
class ToolbarComponentTest {
 static void select(MainWindow& frame,const std::vector<JobPtr>& jobs){frame.table.SetItemState(-1,0,LVIS_SELECTED);for(int i=0;i<(int)frame.visible.size();++i)if(std::find(jobs.begin(),jobs.end(),frame.visible[i])!=jobs.end())frame.table.SetItemState(i,LVIS_SELECTED|LVIS_FOCUSED,LVIS_SELECTED|LVIS_FOCUSED);}
#ifdef UDM_DOWNLOAD_MENU_PREPARATION
 static void settle(MainWindow& frame){const auto end=GetTickCount64()+5000;while(GetTickCount64()<end){frame.pollDownloadMenu();if(!frame.downloadMenuTask.valid()&&!frame.downloadMenuRequest)return;Sleep(1);}throw std::runtime_error("Owned download menu did not settle");}
#endif
public:
 static void cleanup(MainWindow*& frame){if(frame){closeOwnedMainWindow(frame);frame=nullptr;}}
 static void run(MainWindow*& owner,Manager& manager){
  auto& frame=*owner;
  frame.KillTimer(1);frame.ShowWindow(SW_SHOWNOACTIVATE);
  auto paused=manager.add("http://127.0.0.1:9/paused","","paused.bin","Main queue",true);
  auto complete=manager.add("http://127.0.0.1:9/complete","","complete.bin","Main queue",true);
  {Lock lock(manager.mutex);paused->data["QueueMember"]=false;complete->data["Status"]="Complete";complete->data["QueueMember"]=false;}
  auto queue=defaultQueue("Research & media");queue["Enabled"]=false;manager.setQueue(queue);
  auto sync=defaultQueue("Archive sync");sync["Enabled"]=false;sync["Synchronize"]=true;manager.setQueue(sync);
  frame.refresh();select(frame,{paused});check(frame.selected(false)==std::vector<JobPtr>{paused},"The native table selects the owned paused row");
  int shown=0;bool renderedWithoutModel=false,doubleOpenChecked=false;std::vector<std::string> shownQueues;std::string membership;
  frame.downloadMenuPresenter=[&](CMenu& menu,CPoint){++shown;bool acquired=false;std::thread reader([&]{std::unique_lock lock(manager.mutex,std::try_to_lock);acquired=lock.owns_lock();});reader.join();renderedWithoutModel=acquired;shownQueues=frame.moveQueueNames;CString label;menu.GetMenuString(frame.moveQueuePosition,label,MF_BYPOSITION);membership=utf8(std::wstring(label));if(auto child=menu.GetSubMenu(frame.moveQueuePosition+3))doubleOpenChecked=(child->GetMenuState(CMD_DOUBLE_OPEN,MF_BYCOMMAND)&MF_CHECKED)!=0;check(menu.GetMenuItemCount()>=20,"Prepared native popup preserves the download menu groups");
#ifdef UDM_DOWNLOAD_MENU_PREPARATION
   if(shown==1){MenuWriter writer(manager);const auto advice=frame.advisoryCommandEnabled(CMD_REFRESH_ADDRESS);frame.SendMessage(WM_INITMENUPOPUP,reinterpret_cast<WPARAM>(menu.GetSafeHmenu()),0);const auto state=menu.GetMenuState(CMD_REFRESH_ADDRESS,MF_BYCOMMAND);check(advice.has_value()&&*advice&&(state&(MF_DISABLED|MF_GRAYED))==0&&writer.held&&!writer.expired,"Native popup updates use prepared address advice while Manager remains held");writer.finish();}
#endif
  };
  {
   MenuWriter writer(manager);frame.historyRefreshDeferred=true;const auto begin=GetTickCount64();frame.downloadMenu(CPoint(20,20));const auto elapsed=GetTickCount64()-begin;
   menuTrace.push_back({{"phase","held-writer"},{"returnMs",elapsed},{"writerHeld",writer.held.load()},{"writerExpired",writer.expired.load()}});
   check(writer.held&&!writer.expired,"Opening the context menu returns while the catalog writer is still held");
   frame.SendMessage(WM_NULL);check(writer.held&&!writer.expired,"The main window processes a heartbeat before the writer releases");check(shown==0,"The popup waits for its complete prepared cohort");writer.finish();
  }
#ifdef UDM_DOWNLOAD_MENU_PREPARATION
  settle(frame);check(shown==1&&renderedWithoutModel,"The real popup renders once without holding Manager across native controls");
  check(!frame.downloadMenuAdvice->current,"Popup advice is released when the native popup closes");
  {
   MenuWriter writer(manager);select(frame,{paused});frame.downloadMenu(CPoint(20,21));
   bool disabled=false,pending=false,mainHandled=false,dialogHandled=false,cancelled=false;
   Form dialog("Owned menu keyboard check",260,96,&frame);CWnd* input=nullptr;
   dialog.init=[&]{dialog.label("Escape belongs to this dialog",12,10,230);input=dialog.edit("Owned test",12,35,230);};
   dialog.cancel=[&]{cancelled=true;dialog.close(IDCANCEL);};
   dialog.afterInit=[&]{disabled=!frame.IsWindowEnabled();pending=frame.downloadMenuRequest.has_value();MSG key{};key.hwnd=input->GetSafeHwnd();key.message=WM_KEYDOWN;key.wParam=VK_ESCAPE;mainHandled=frame.PreTranslateMessage(&key)!=FALSE;if(!mainHandled)dialogHandled=static_cast<CWnd&>(dialog).PreTranslateMessage(&key)!=FALSE;if(!cancelled)dialog.PostMessage(WM_COMMAND,IDCANCEL);};
   dialog.DoModal();menuTrace.push_back({{"phase","modal-escape"},{"ownerDisabled",disabled},{"requestPendingBeforeKey",pending},{"mainHandledEscape",mainHandled},{"dialogHandledEscape",dialogHandled},{"dialogCancelled",cancelled},{"requestPendingAfterKey",frame.downloadMenuRequest.has_value()}});
   check(disabled&&pending,"A real modal dialog owns input with a context-menu read still pending");
   check(!mainHandled&&dialogHandled&&cancelled&&!frame.downloadMenuRequest,"Modal Escape reaches its dialog and retires the pending menu request");
   check(writer.held&&!writer.expired,"Modal cancellation completes before the catalog writer releases");writer.finish();
  }
  settle(frame);check(shown==1,"Closing the modal cannot open its abandoned popup later");

  check(!doubleOpenChecked&&membership=="Add to queue"&&std::find(shownQueues.begin(),shownQueues.end(),"Research & media")!=shownQueues.end(),"The paused selection retains Add to queue and its normal destination queues");
  select(frame,{complete});frame.downloadMenu(CPoint(21,21));settle(frame);check(shown==2&&std::find(shownQueues.begin(),shownQueues.end(),"Archive sync")!=shownQueues.end()&&std::find(shownQueues.begin(),shownQueues.end(),"Research & media")==shownQueues.end(),"Completed files retain synchronization-only queue destinations");
  {
   MenuWriter writer(manager);select(frame,{paused});frame.downloadMenu(CPoint(22,22));select(frame,{complete});writer.finish();
  }
  settle(frame);check(shown==2,"A selection change discards the obsolete menu instead of opening it for different files");
  {
   MenuWriter writer(manager);select(frame,{paused});frame.downloadMenu(CPoint(23,23));MSG key{};key.hwnd=frame.table.GetSafeHwnd();key.message=WM_KEYDOWN;key.wParam=VK_ESCAPE;
   check(frame.PreTranslateMessage(&key)!=FALSE&&writer.held&&!writer.expired,"Escape cancels pending menu preparation while the writer remains held");writer.finish();
  }
  settle(frame);check(shown==2,"Cancelled preparation never opens a late popup");
  {
   MenuWriter writer(manager);select(frame,{paused});frame.downloadMenu(CPoint(24,24));select(frame,{complete});frame.downloadMenu(CPoint(25,25));writer.finish();
  }
  settle(frame);check(shown==3&&std::find(shownQueues.begin(),shownQueues.end(),"Research & media")==shownQueues.end(),"A repeated request publishes only the newest selection and queue cohort");
  select(frame,{paused});frame.downloadMenu(CPoint(26,26));
  {const auto end=GetTickCount64()+5000;while(!frame.downloadMenuTask.ready()&&GetTickCount64()<end)Sleep(1);check(frame.downloadMenuTask.ready(),"A prepared cohort is ready before its queue/policy are edited");}
  {Lock lock(manager.mutex);for(auto& q:manager.state["Queues"])if(str(q,"Name")=="Research & media")q["Name"]="Renamed & media";manager.state["Settings"]["CompletedDoubleClick"]="Open";}
  settle(frame);check(shown==4&&doubleOpenChecked&&std::find(shownQueues.begin(),shownQueues.end(),"Renamed & media")!=shownQueues.end()&&std::find(shownQueues.begin(),shownQueues.end(),"Research & media")==shownQueues.end(),"A changed queue/policy cohort is reread before opening the popup");
  {
   MenuWriter writer(manager);select(frame,{paused});frame.downloadMenu(CPoint(26,26));writer.finish();
  }
  {Lock lock(manager.mutex);manager.jobs.erase(std::remove(manager.jobs.begin(),manager.jobs.end(),paused),manager.jobs.end());}
  settle(frame);check(shown==4,"A removed selected job cannot publish a prepared popup");
  {
   MenuWriter writer(manager);select(frame,{complete});frame.downloadMenu(CPoint(27,27));frame.cancelDownloadMenu();frame.downloadMenuTask.finish();check(writer.held&&!writer.expired,"Cancelling and joining an unstarted menu read never waits for the catalog writer");writer.finish();
  }
  select(frame,{complete});frame.downloadMenu(CPoint(27,28));frame.command(CMD_SELECTALL);
  check(!frame.downloadMenuRequest,"A new main-window command immediately supersedes pending menu preparation");settle(frame);check(shown==4,"A superseded menu never appears after the new command");
  frame.modelMaintenance.stop();frame.presentationAcks.stop();frame.authenticationTask->finish();
  {
   MenuWriter writer(manager);select(frame,{complete});frame.downloadMenu(CPoint(28,28));const auto window=frame.GetSafeHwnd();const bool destroyed=frame.DestroyWindow()!=FALSE;if(destroyed)owner=nullptr;
   check(destroyed&&!::IsWindow(window)&&writer.held&&!writer.expired,"Direct window destruction joins a pending menu reader before its fields die");writer.finish();
  }
#endif
 }
};
class DownloadMenuApplication:public CWinApp {
 int result=1;public:BOOL InitInstance()override{CWinApp::InitInstance();INITCOMMONCONTROLSEX controls{sizeof(controls),ICC_WIN95_CLASSES|ICC_DATE_CLASSES|ICC_PROGRESS_CLASS};InitCommonControlsEx(&controls);AfxOleInit();WSADATA sockets{};if(WSAStartup(MAKEWORD(2,2),&sockets))return FALSE;int argc=0;auto args=CommandLineToArgvW(GetCommandLineW(),&argc);std::unique_ptr<Manager> manager;MainWindow* frame=nullptr;
 try{if(!args||argc!=2)throw std::runtime_error("Pass one fresh output directory");output=args[1];if(fs::exists(output)){output.clear();throw std::runtime_error("Fresh output required");}fs::create_directories(output);manager=std::make_unique<Manager>(output/L"catalog");auto settings=manager->state["Settings"];settings["DownloadFolder"]=utf8((output/L"files").wstring());settings["CloseToTray"]=false;settings["ClipboardMonitor"]=false;settings["Sound"]=false;settings["ShowTipsOnStartup"]=false;settings["CategoryFolders"]=false;manager->setSettings(settings);frame=new MainWindow(*manager);m_pMainWnd=frame;ToolbarComponentTest::run(frame,*manager);result=0;}catch(const std::exception& error){failure=error.what();}
 m_pMainWnd=nullptr;ToolbarComponentTest::cleanup(frame);manager.reset();if(args)LocalFree(args);WSACleanup();if(!output.empty())atomicText(output/L"results.json",Json{{"passed",result==0},{"checks",checks},{"error",failure},{"diagnostics",menuTrace}}.dump(2),false);return FALSE;}
 int ExitInstance()override{AfxOleTerm(FALSE);return result;}
};DownloadMenuApplication application;
}
