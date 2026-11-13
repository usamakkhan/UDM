// Operates only windows created by this isolated executable.
#include <afxwin.h>
#include <afxcmn.h>
#include <afxdlgs.h>
#include <afxole.h>
#include <atlimage.h>
#include "Ui.hpp"
namespace udm {
struct CatalogCheckpointTestAccess {static void observe(Manager& m,std::function<void(const char*)> fn){Lock lock(m.mutex);m.catalogCheckpoint=std::move(fn);}};
static Json checks=Json::array();
static void check(bool value,const char* name){checks.push_back({{"name",name},{"passed",value}});if(!value)throw std::runtime_error(name);}
static std::wstring title(HWND w){wchar_t value[2048]{};GetWindowTextW(w,value,2048);return value;}
static HWND ownWindow(const wchar_t* name){struct Match{const wchar_t* name;HWND found=nullptr;}match{name};EnumWindows([](HWND w,LPARAM p)->BOOL{auto& m=*reinterpret_cast<Match*>(p);DWORD pid=0;GetWindowThreadProcessId(w,&pid);if(pid==GetCurrentProcessId()&&IsWindowVisible(w)&&title(w)==m.name)m.found=w;return TRUE;},reinterpret_cast<LPARAM>(&match));return match.found;}
static HWND control(HWND parent,const wchar_t* label){for(auto w=GetWindow(parent,GW_CHILD);w;w=GetWindow(w,GW_HWNDNEXT))if(IsWindowVisible(w)&&title(w)==label)return w;return nullptr;}
static std::vector<HWND> kind(HWND parent,const wchar_t* name){std::vector<HWND> result;for(auto w=GetWindow(parent,GW_CHILD);w;w=GetWindow(w,GW_HWNDNEXT)){wchar_t type[40]{};GetClassNameW(w,type,40);if(IsWindowVisible(w)&&!_wcsicmp(type,name))result.push_back(w);}return result;}
class WizardOwner:public CFrameWnd {void PostNcDestroy()override{}};
struct WizardCase {
 Manager& manager;std::string mode;fs::path folder;std::string queue;
 std::atomic_bool& armed;std::atomic_bool& entered;std::function<void()> release;bool fail;
 HWND wizard=nullptr;bool opened=false,destination=false,dates=false,submitted=false,held=false,notice=false,retried=false;
 std::string error;ULONGLONG deadline=GetTickCount64()+15000;
 void tick(){
  if(!error.empty())return;
  if(GetTickCount64()>deadline)throw std::runtime_error("Scheduled wizard UI timed out");
  if(mode=="file-info"&&!opened){if(auto info=ownWindow(L"Download File Info")){opened=true;auto schedule=control(info,L"Sche&dule...");check(schedule!=nullptr,"File Info exposes its schedule entry point");SendMessageW(schedule,BM_CLICK,0,0);}return;}
  auto w=ownWindow(mode=="revise"?L"Edit download schedule":L"Schedule download");if(!w)return;
  if(!wizard)wizard=w;checkSame(w);
  if(auto warning=ownWindow(L"UDM")){
   if(notice&&retried){std::string message="Second scheduled save failed:";for(auto label:kind(warning,L"STATIC"))message+=" "+utf8(title(label));throw std::runtime_error(message);}
   if(!notice){check(fail&&control(warning,L"Forced scheduled save failure"),"The worker's real persistence error appears in the wizard");notice=true;SendMessageW(control(warning,L"OK"),BM_CLICK,0,0);}return;
  }
  if(submitted){
   if(!held&&entered){check(manager.fileInfoDecisionPending&&IsWindow(w),"The wizard keeps pumping while persistence holds the Manager lock");auto cancel=control(w,L"Cancel");check(cancel&&!IsWindowEnabled(cancel),"Saving a schedule disables conflicting Cancel");RedrawWindow(w,nullptr,nullptr,RDW_INVALIDATE|RDW_UPDATENOW);KillTimer(w,Form::pulseTimer);held=true;release();}
   // The warning temporarily enables its owner before the outer completion
   // handler releases the task. Retry only after that acknowledgement ends.
   if(fail&&notice&&!retried&&!manager.fileInfoDecisionPending&&IsWindowEnabled(w)){
    auto save=control(w,mode=="revise"?L"&Save changes":L"&Schedule");if(!save||!IsWindowEnabled(save))return;
    check(summary(w).find(L"Saved wizard draft")!=std::wstring::npos&&summary(w).find(L"wizard-target.bin")!=std::wstring::npos,"A failed save retains the complete review draft in the same wizard");
    retried=true;SendMessageW(save,BM_CLICK,0,0);
   }
   return;
  }
  std::wstring heading;for(auto text:kind(w,L"STATIC"))if(title(text).rfind(L"Step ",0)==0)heading=title(text);
  if(heading.find(L"Download address")!=std::wstring::npos){SendMessageW(control(w,L"&Next >"),BM_CLICK,0,0);return;}
  if(heading.find(L"Destination")!=std::wstring::npos){
   if(!destination){auto combos=kind(w,L"COMBOBOX");auto edits=kind(w,L"EDIT");std::vector<HWND> editable;for(auto e:edits)if(!(GetWindowLongPtrW(e,GWL_STYLE)&ES_READONLY))editable.push_back(e);check(!combos.empty()&&editable.size()==2,"The actual destination page exposes filename and description editors");SetWindowTextW(combos.front(),folder.c_str());SetWindowTextW(editable[0],L"wizard-target.bin");SetWindowTextW(editable[1],L"Saved wizard draft");destination=true;}
   SendMessageW(control(w,L"&Next >"),BM_CLICK,0,0);return;
  }
  if(heading.find(L"Dates and queue")!=std::wstring::npos){if(!dates){if(mode!="revise"){auto edits=kind(w,L"EDIT");check(!edits.empty(),"The dates page preserves its queue-name editor");SetWindowTextW(edits.front(),wide(queue).c_str());}dates=true;}SendMessageW(control(w,L"&Next >"),BM_CLICK,0,0);return;}
  if(heading.find(L"Options")!=std::wstring::npos){SendMessageW(control(w,L"&Next >"),BM_CLICK,0,0);return;}
  if(heading.find(L"Review")!=std::wstring::npos){check(summary(w).find(L"Saved wizard draft")!=std::wstring::npos,"Review contains the entered description before save");submitted=true;armed=true;SendMessageW(control(w,mode=="revise"?L"&Save changes":L"&Schedule"),BM_CLICK,0,0);}
 }
 void checkSame(HWND w){checkNoRecord(w==wizard,"The save and recovery keep the original wizard HWND");}
 void checkNoRecord(bool ok,const char* message){if(!ok)throw std::runtime_error(message);}
 std::wstring summary(HWND w){for(auto e:kind(w,L"EDIT"))if(GetWindowLongPtrW(e,GWL_STYLE)&ES_READONLY)return title(e);return {};}
};
static WizardCase* active=nullptr;
static void CALLBACK timer(HWND,UINT,UINT_PTR,DWORD){if(!active)return;try{active->tick();}catch(const std::exception& e){active->error=e.what();active->release();if(auto warning=ownWindow(L"UDM"))PostMessageW(control(warning,L"OK"),BM_CLICK,0,0);if(auto w=active->wizard){SetTimer(w,Form::pulseTimer,40,nullptr);PostMessageW(w,WM_CLOSE,0,0);}}}
class WizardUiApplication:public CWinApp {
 int result=1;
public:BOOL InitInstance()override{
 CWinApp::InitInstance();INITCOMMONCONTROLSEX common{sizeof(common),ICC_WIN95_CLASSES|ICC_DATE_CLASSES};InitCommonControlsEx(&common);AfxOleInit();int argc=0;auto argv=CommandLineToArgvW(GetCommandLineW(),&argc);fs::path output;std::string error;
 try{
  if(argc!=2)throw std::runtime_error("Pass a fresh private output folder");output=fs::absolute(argv[1]);if(fs::exists(output))throw std::runtime_error("Fresh output required");fs::create_directories(output);
  wchar_t dark[2]{};uiDark=useDarkUi(GetEnvironmentVariableW(L"UDM_TEST_PENDING_DARK",dark,2)==1&&dark[0]==L'1',windowsHighContrast());
  for(const std::string mode:{"new","listed","revise","file-info"})for(bool fail:{false,true}){
   auto root=output/wide(mode+(fail?"-recovery":"-success"));Manager manager(root/L"state");auto prefs=manager.state["Settings"];prefs["DownloadFolder"]=utf8((root/L"files").native());prefs["CategoryFolders"]=false;prefs["PrefetchFileInfo"]=false;manager.setSettings(prefs);JobPtr job;
   const auto queue="Wizard "+mode+(fail?" recovery":" success");
   if(mode=="revise"){DownloadQueueChoice choice;choice.name=queue;choice.create=true;ScheduledDownloadWindow window;window.startUtc=epoch()+3600000;choice.window=window;job=manager.addScheduledDownload("http://127.0.0.1:9/wizard.bin",root/L"files/before.bin","Other","Before",{},choice,{},4,0);}
   else if(mode!="new")job=manager.add("ftp://127.0.0.1:9/before.bin",utf8((root/L"files").native()),"before.bin","Main queue",true);
   const auto beforeQueues=manager.state["Queues"].size();const auto beforeId=job?job->id():std::string{};
   std::promise<void> release;auto gate=release.get_future().share();std::atomic_bool armed{false},entered{false},released{false};const auto unlock=[&]{if(!released.exchange(true))release.set_value();};
   CatalogCheckpointTestAccess::observe(manager,[&](const char* stage){if(armed&&std::string(stage)=="temporary-flushed"&&!entered.exchange(true)){if(gate.wait_for(std::chrono::seconds(8))!=std::future_status::ready)throw std::runtime_error("Scheduled save gate expired");if(fail)throw std::runtime_error("Forced scheduled save failure");}});
   struct Cleanup {Manager& m;std::function<void()> release;~Cleanup(){release();CatalogCheckpointTestAccess::observe(m,{});}}cleanup{manager,unlock};
   WizardOwner owner;check(owner.Create(nullptr,L"Private scheduled wizard owner",WS_OVERLAPPEDWINDOW,CRect(0,0,80,80))!=FALSE,"Create an isolated hidden wizard owner");m_pMainWnd=&owner;WizardCase state{manager,mode,root/L"files",queue,armed,entered,unlock,fail};active=&state;auto id=SetTimer(nullptr,0,20,timer);
   try{if(mode=="new")addScheduledDownload(&owner,manager,"http://127.0.0.1:9/wizard.bin");else if(mode=="file-info")downloadInfo(&owner,manager,job);else scheduleListedDownload(&owner,manager,job,mode=="revise");}catch(...){unlock();KillTimer(nullptr,id);active=nullptr;m_pMainWnd=nullptr;throw;}
   KillTimer(nullptr,id);active=nullptr;if(!state.error.empty())throw std::runtime_error(state.error);check(state.held&&!manager.fileInfoDecisionPending,"Posted completion ends the wizard with its polling timer removed");
   check(!fail||(state.notice&&state.retried),"Persistence failure is acknowledged and retried explicitly");
   {Lock lock(manager.mutex);check(manager.jobs.size()==1&&str(manager.jobs.front()->data,"FileName")=="wizard-target.bin"&&str(manager.jobs.front()->data,"Description")=="Saved wizard draft","Schedule save applies one complete owned draft exactly once");check(mode=="new"||manager.jobs.front()->id()==beforeId,"Existing scheduled downloads retain their job identity");check(manager.state["Queues"].size()==beforeQueues+(mode=="revise"?0:1),"Failed schedule publication does not append a duplicate queue");check(str(manager.jobs.front()->data,"Status")=="Paused"&&manager.editableDownloadSchedule(manager.jobs.front()).has_value(),"The scheduled job remains paused with an editable future window");}
   CatalogCheckpointTestAccess::observe(manager,{});manager.stop();m_pMainWnd=nullptr;owner.DestroyWindow();
  }result=0;
 }catch(const std::exception& e){error=e.what();m_pMainWnd=nullptr;}
 if(argv)LocalFree(argv);if(!output.empty())atomicText(output/L"results.json",Json{{"passed",result==0},{"error",error},{"checks",checks},{"darkMode",uiDark}}.dump(2),false);return FALSE;
 }
 int ExitInstance()override{AfxOleTerm(FALSE);return result;}
};WizardUiApplication wizardApplication;
}
