#define UDM_TOOLBAR_COMPONENT_TEST
#include "App.cpp"
namespace udm {
static fs::path output;static Json checks=Json::array(),navigationDiagnostics=Json::object();static std::string failure;
static void check(bool value,const char* name){checks.push_back({{"name",name},{"passed",value}});if(!value)throw std::runtime_error(name);}
class ToolbarComponentTest {
 static std::string caret(MainWindow& frame){auto item=frame.tree.GetSelectedItem();if(!item)return {};auto index=frame.tree.GetItemData(item);return index<frame.filters.size()?frame.filters[index]:"invalid";}
 static void refresh(MainWindow& frame){const auto end=GetTickCount64()+5000;do{frame.refresh();if(!frame.historyRefreshDeferred)return;Sleep(1);}while(GetTickCount64()<end);throw std::runtime_error("Owned refresh did not acquire its model");}
public:
 static void cleanup(MainWindow*& frame){if(!frame)return;closeOwnedMainWindow(frame);frame=nullptr;}
 static void run(MainWindow& frame,Manager& manager){
  frame.KillTimer(1);frame.ShowWindow(SW_SHOWNOACTIVATE);
  auto first=manager.add("https://example.invalid/member.bin",utf8((output/L"files").wstring()),"member.bin","Main queue",true);
  auto complete=manager.add("https://example.invalid/complete.bin",utf8((output/L"files").wstring()),"complete.bin","Main queue",true);
  {Lock lock(manager.mutex);first->data["Status"]="Paused";first->data["QueueMember"]=true;complete->data["Status"]="Complete";complete->data["QueueMember"]=false;}
  refresh(frame);check(frame.visible==std::vector<JobPtr>{first,complete},"The actual initial list contains the two owned records");
  auto queue=defaultQueue("Navigation audit");queue["Enabled"]=false;manager.setQueue(queue);
  {Lock lock(manager.mutex);first->data["Queue"]="Navigation audit";complete->data["Queue"]="Navigation audit";}
  frame.filter="queue:Navigation audit";refresh(frame);
  navigationDiagnostics["newQueue"]={{"filter",frame.filter},{"caret",caret(frame)},{"rows",frame.visible.size()},{"firstCell",utf8(std::wstring(frame.table.GetItemText(0,0)))}};
  check(frame.filter=="queue:Navigation audit"&&caret(frame)==frame.filter,"The real tree selects the newly requested queue");
  check(frame.visible==std::vector<JobPtr>{first}&&frame.table.GetItemCount()==1&&frame.table.GetItemText(0,0)==L"member.bin","The first real new-queue view contains only its member");
  refresh(frame);check(frame.visible==std::vector<JobPtr>{first}&&caret(frame)==frame.filter,"A later refresh does not briefly correct a mismatched first cohort");
  const auto before=frame.visible;std::promise<void> release,entered;auto ready=entered.get_future();auto gate=release.get_future().share();std::atomic_bool held=false,expired=false;
  std::thread writer([&]{Lock lock(manager.mutex);held=true;entered.set_value();if(gate.wait_for(std::chrono::seconds(3))!=std::future_status::ready)expired=true;held=false;});
  struct Join{std::promise<void>& release;std::thread& writer;~Join(){try{release.set_value();}catch(...){}if(writer.joinable())writer.join();}} joined{release,writer};
  check(ready.wait_for(std::chrono::seconds(2))==std::future_status::ready,"An independent writer precedes the busy refresh");
  frame.refresh();check(held&&!expired&&frame.historyRefreshDeferred&&frame.visible==before&&caret(frame)==frame.filter,"A busy refresh retains the selected queue and displayed cohort");release.set_value();writer.join();refresh(frame);
  check(frame.visible==before&&!frame.historyRefreshDeferred,"Refresh catches up without losing the selected queue after release");
  auto project=Json{{"Id","navigation-project"},{"Name","Navigation project"}};
  {Lock lock(manager.mutex);manager.state["Projects"].push_back(project);first->data["ProjectId"]="navigation-project";}
  frame.filter="project:navigation-project";refresh(frame);check(caret(frame)==frame.filter&&frame.visible==std::vector<JobPtr>{first},"A new project's tree selection and first row cohort agree");
  {Lock lock(manager.mutex);manager.state["Projects"].back()["Name"]="Renamed navigation project";}
  refresh(frame);check(caret(frame)==frame.filter&&frame.visible==std::vector<JobPtr>{first},"Renaming the selected project retains its cohort");
  {Lock lock(manager.mutex);manager.state["Projects"]=Json::array();}
  refresh(frame);check(frame.filter=="grabber"&&caret(frame)=="grabber"&&frame.visible==std::vector<JobPtr>{first},"Removing a selected project falls back coherently to Grabber");
  frame.filter="finished";refresh(frame);auto another=defaultQueue("Unrelated queue");another["Enabled"]=false;manager.setQueue(another);refresh(frame);
  check(frame.filter=="finished"&&caret(frame)=="finished"&&frame.visible==std::vector<JobPtr>{complete},"Adding an unrelated queue preserves the Finished tree and rows");
  frame.tree.SelectItem(frame.treeItems.at("root:queues"));refresh(frame);manager.deleteQueue("Unrelated queue");refresh(frame);
  check(frame.tree.GetSelectedItem()==frame.treeItems.at("root:queues")&&frame.filter=="all"&&frame.visible.size()==2,"The queues-root alias retains its actual caret across structural changes");
 }
};
class HistoryNavigationApplication:public CWinApp {
 int result=1;public:BOOL InitInstance()override{CWinApp::InitInstance();INITCOMMONCONTROLSEX controls{sizeof(controls),ICC_WIN95_CLASSES|ICC_DATE_CLASSES};InitCommonControlsEx(&controls);AfxOleInit();int argc=0;auto args=CommandLineToArgvW(GetCommandLineW(),&argc);std::unique_ptr<Manager> manager;MainWindow* frame=nullptr;
 try{if(!args||argc!=2)throw std::runtime_error("Pass one fresh output directory");output=args[1];if(fs::exists(output)){output.clear();throw std::runtime_error("Fresh output required");}fs::create_directories(output);manager=std::make_unique<Manager>(output/L"catalog");auto settings=manager->state["Settings"];settings["CloseToTray"]=false;settings["ClipboardMonitor"]=false;settings["Sound"]=false;settings["PrefetchFileInfo"]=false;settings["SuppressCompletionDialog"]=true;settings["SuppressProgressDialog"]=true;settings["ShowTipsOnStartup"]=false;settings["CategoryFolders"]=false;manager->setSettings(settings);frame=new MainWindow(*manager);m_pMainWnd=frame;ToolbarComponentTest::run(*frame,*manager);result=0;}catch(const std::exception& error){failure=error.what();}
 m_pMainWnd=nullptr;ToolbarComponentTest::cleanup(frame);manager.reset();if(args)LocalFree(args);if(!output.empty())atomicText(output/L"results.json",Json{{"passed",result==0},{"checks",checks},{"error",failure},{"navigationDiagnostics",navigationDiagnostics}}.dump(2),false);return FALSE;}
 int ExitInstance()override{AfxOleTerm(FALSE);return result;}
};HistoryNavigationApplication application;
}
