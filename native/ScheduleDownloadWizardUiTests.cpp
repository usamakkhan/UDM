// Operates only windows created by this isolated executable.
#include <afxwin.h>
#include <afxcmn.h>
#include <afxdlgs.h>
#include <afxole.h>
#include <atlimage.h>
#define UDM_SCHEDULE_WIZARD_COMPONENT_TEST
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
  if(mode=="file-info"&&!opened){if(auto info=ownWindow(L"Download File Info")){auto schedule=control(info,L"Sche&dule...");if(!schedule||!IsWindowEnabled(schedule))return;opened=true;check(schedule!=nullptr,"File Info exposes its schedule entry point");SendMessageW(schedule,BM_CLICK,0,0);}return;}
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
static std::function<void()> preparationPulse;
static void CALLBACK timer(HWND,UINT,UINT_PTR,DWORD){if(preparationPulse){preparationPulse();return;}if(!active)return;try{active->tick();}catch(const std::exception& e){active->error=e.what();active->release();if(auto warning=ownWindow(L"UDM"))PostMessageW(control(warning,L"OK"),BM_CLICK,0,0);if(auto w=active->wizard){SetTimer(w,Form::pulseTimer,40,nullptr);PostMessageW(w,WM_CLOSE,0,0);}}}
struct PreparationModelHold {
 std::promise<void> release;std::shared_future<void> gate=release.get_future().share();std::atomic_bool held=false,expired=false,released=false;std::thread worker;
 explicit PreparationModelHold(Manager& manager,std::function<void()> beforeWait={}){worker=std::thread([this,&manager,beforeWait=std::move(beforeWait)]{Lock lock(manager.mutex);if(beforeWait)beforeWait();held=true;if(gate.wait_for(std::chrono::seconds(8))!=std::future_status::ready)expired=true;held=false;});const auto end=GetTickCount64()+3000;while(!held&&GetTickCount64()<end)Sleep(1);if(!held){open();throw std::runtime_error("Cannot hold the owned schedule model");}}
 void open(){if(!released.exchange(true))release.set_value();if(worker.joinable())worker.join();}
 ~PreparationModelHold(){open();}
};
static void preparationCases(const fs::path& output,CWinApp& app){
 for(const std::string mode:{"direct-cancel","file-info-cancel","direct-ready","file-info-ready","retry","listed-cancel","revise-cancel","listed-ready","revise-ready","listed-ineligible","revise-ineligible"}){
  const bool parentInfo=mode.rfind("file-info",0)==0,cancelRead=mode.find("cancel")!=std::string::npos,retry=mode=="retry",revise=mode.rfind("revise",0)==0,listed=revise||mode.rfind("listed",0)==0,ineligible=mode.find("ineligible")!=std::string::npos;const auto wizardTitle=revise?L"Edit download schedule":L"Schedule download";const auto root=output/wide("preparation-"+mode);Manager manager(root/L"state");auto prefs=manager.state["Settings"];prefs["DownloadFolder"]=utf8((root/L"files").wstring());prefs["CategoryFolders"]=false;prefs["PrefetchFileInfo"]=false;manager.setSettings(prefs);JobPtr job;if(revise){DownloadQueueChoice choice;choice.name="Preparation revision";choice.create=true;ScheduledDownloadWindow window;window.startUtc=epoch()+3600000;choice.window=window;job=manager.addScheduledDownload("ftp://127.0.0.1:9/before.bin",root/L"original/before.bin","Other","",{},choice,{},5,17);}else job=manager.add("ftp://127.0.0.1:9/before.bin",utf8((root/L"original").wstring()),"before.bin","Main queue",true);job->data["Description"]="Original note";job->data["Connections"]=5;job->data["LimitKbps"]=17;manager.save();
  WizardOwner owner;check(owner.Create(nullptr,L"Owned preparation fixture",WS_OVERLAPPEDWINDOW,CRect(0,0,80,80))!=FALSE,"Create a private owner for wizard preparation");app.m_pMainWnd=&owner;std::unique_ptr<PreparationModelHold> hold;std::atomic_bool waiting=false;std::atomic_int faults=0;int phase=0,retries=0;bool inspected=false;std::string error;const auto deadline=GetTickCount64()+18000;
  const auto open=[&]{if(hold)hold->open();};schedulePreparationTestObserver=[&](const char* stage){if(std::string(stage)=="waiting-model")waiting=true;if(retry&&std::string(stage)=="before-read"&&faults.fetch_add(1)==0)throw std::runtime_error("Owned wizard preparation failure");};
  preparationPulse=[&]{
   try{
    if(GetTickCount64()>deadline)throw std::runtime_error("Owned preparation dialog timed out");auto wizard=ownWindow(wizardTitle);auto info=parentInfo?ownWindow(L"Download File Info"):nullptr;
    if(parentInfo&&phase==0){if(!info)return;auto schedule=control(info,L"Sche&dule...");if(!schedule||!IsWindowEnabled(schedule))return;auto combos=kind(info,L"COMBOBOX");check(combos.size()>=2,"File Info exposes its destination before scheduling");SetWindowTextW(combos[1],(root/L"manual/chosen.bin").c_str());HWND notes=nullptr;for(auto edit:kind(info,L"EDIT"))if(title(edit)==L"Original note")notes=edit;check(notes!=nullptr,"File Info exposes the existing description draft");SetWindowTextW(notes,L"Unapplied wizard note");hold=std::make_unique<PreparationModelHold>(manager);phase=1;SendMessageW(schedule,BM_CLICK,0,0);return;}
    if(phase==1&&wizard&&waiting){auto next=control(wizard,L"&Next >"),cancel=control(wizard,L"Cancel");check(hold&&hold->held&&!hold->expired&&IsWindowVisible(wizard),"The real schedule wizard paints before its model can be read");check(next&&!IsWindowEnabled(next)&&cancel&&IsWindowEnabled(cancel),"Loading exposes Cancel while navigation and submission remain disabled");auto form=dynamic_cast<Form*>(CWnd::FromHandlePermanent(wizard));if(!form)throw std::runtime_error("Missing preparation Form");form->SendMessage(WM_TIMER,Form::pulseTimer,0);form->Invalidate();form->UpdateWindow();check(hold->held&&!hold->expired&&fileInfoDialogShowing(),"Wizard timer and paint return under the held model with shutdown ownership retained");inspected=true;
     if(cancelRead){phase=2;SendMessageW(cancel,BM_CLICK,0,0);check(hold->held&&!hold->expired,"Cancelling advisory preparation returns before the writer releases");}else{open();phase=3;}return;
    }
    if(phase==3&&wizard){
     if(ineligible){auto retryButton=control(wizard,L"&Retry");if(!retryButton)return;check(control(wizard,revise?L"This download no longer has an editable schedule.":L"Pause the download and its queue before adding a schedule.")!=nullptr,"Listed/revised eligibility is checked against the state changed by the held writer");auto next=control(wizard,L"&Next >"),cancel=control(wizard,L"Cancel");check(next&&!IsWindowEnabled(next)&&cancel&&IsWindowEnabled(cancel)&&!control(wizard,L"Use category folder"),"Ineligible listed/revised state never constructs an editable wizard");phase=4;SendMessageW(cancel,BM_CLICK,0,0);return;}
     if(retry&&!retries){auto button=control(wizard,L"&Retry");if(!button||!IsWindowEnabled(button))return;check(control(wizard,L"Owned wizard preparation failure")!=nullptr,"Initial-reader errors remain inline and expose explicit Retry");++retries;SendMessageW(button,BM_CLICK,0,0);return;}
     auto use=control(wizard,L"Use category folder"),next=control(wizard,L"&Next >");if(!use||!next||!IsWindowEnabled(next))return;auto combos=kind(wizard,L"COMBOBOX");auto edits=kind(wizard,L"EDIT");check(!combos.empty()&&title(combos.front())==(root/(listed?L"original":L"manual")).wstring(),"The wizard preserves the supplied destination instead of reloading the job target");bool name=false,notes=false;for(auto edit:edits){name|=title(edit)==(listed?L"before.bin":L"chosen.bin");notes|=title(edit)==(listed?L"Original note":L"Unapplied wizard note");}check(name&&notes,"The filename and description drafts survive owned preparation");
     hold=std::make_unique<PreparationModelHold>(manager);SendMessageW(use,BM_CLICK,0,0);check(hold->held&&!hold->expired&&title(combos.front())==(root/L"files").wstring(),"Use category folder applies owned preferences without waiting for Manager");SendMessageW(next,BM_CLICK,0,0);SendMessageW(next,BM_CLICK,0,0);check(hold->held&&!hold->expired&&control(wizard,L"Step 3 of 4   Options"),"Destination and date validation advance using advisory queue names while Manager is held");bool connections=false,limit=false;for(auto edit:kind(wizard,L"EDIT")){connections|=title(edit)==L"5";limit|=title(edit)==L"17";}check(connections&&limit,"Per-file connection and speed defaults are prepared coherently for the Options page");phase=4;SendMessageW(control(wizard,L"Cancel"),BM_CLICK,0,0);return;
    }
    if(parentInfo&&(phase==2||phase==4)&&!wizard&&info){check(hold&&hold->held&&!hold->expired,"The nested wizard closes before its advisory writer must release");open();phase=5;SendMessageW(control(info,L"&Cancel"),BM_CLICK,0,0);}
   }catch(const std::exception& failure){error=failure.what();open();if(auto wizard=ownWindow(wizardTitle))PostMessageW(wizard,WM_CLOSE,0,0);if(auto info=ownWindow(L"Download File Info"))PostMessageW(info,WM_CLOSE,0,0);}
  };
  struct Cleanup{std::function<void()> open;~Cleanup(){open();preparationPulse={};schedulePreparationTestObserver={};}} cleanup{open};
  if(!parentInfo){std::function<void()> mutate;if(ineligible)mutate=[job]{job->data["Status"]="Complete";};hold=std::make_unique<PreparationModelHold>(manager,std::move(mutate));phase=1;}
  auto timerId=SetTimer(nullptr,0,10,timer);check(timerId!=0,"Create an owned preparation fixture timer");bool accepted=false;
  try{if(parentInfo)accepted=downloadInfo(&owner,manager,job);else if(listed)scheduleListedDownload(&owner,manager,job,revise);else{ScheduledDownloadDraft draft;draft.target=root/L"manual/chosen.bin";draft.description="Unapplied wizard note";draft.category="Other";accepted=scheduleDownloadWizard(&owner,manager,std::move(draft),[](const ScheduledDownloadDraft&){return FileInfoDecisionAction([]{return FileInfoDecisionResult{};});},Json(),false,job);}}catch(...){KillTimer(nullptr,timerId);open();preparationPulse={};schedulePreparationTestObserver={};app.m_pMainWnd=nullptr;throw;}
  KillTimer(nullptr,timerId);preparationPulse={};schedulePreparationTestObserver={};if(!parentInfo&&!ineligible)check(hold&&hold->held&&!hold->expired,"An owned wizard returns after Cancel without waiting for an external model writer");open();check(error.empty()&&inspected&&!accepted&&!manager.fileInfoDecisionPending,"Preparation cancellation or review closes with all owned tasks settled");check(!retry||retries==1,"A consumed initial-read error is retried exactly once");check(str(job->data,"FileName")=="before.bin"&&str(job->data,"Description")=="Original note"&&num(job->data,"Connections")==5&&num(job->data,"LimitKbps")==17,"Cancelling the wizard leaves the saved download and staged File Info edits uncommitted");manager.stop();app.m_pMainWnd=nullptr;owner.DestroyWindow();if(!error.empty())throw std::runtime_error(error);
 }
}
class WizardUiApplication:public CWinApp {
 int result=1;
public:BOOL InitInstance()override{
 CWinApp::InitInstance();INITCOMMONCONTROLSEX common{sizeof(common),ICC_WIN95_CLASSES|ICC_DATE_CLASSES};InitCommonControlsEx(&common);AfxOleInit();int argc=0;auto argv=CommandLineToArgvW(GetCommandLineW(),&argc);fs::path output;std::string error;
 try{
  if(argc!=2)throw std::runtime_error("Pass a fresh private output folder");output=fs::absolute(argv[1]);if(fs::exists(output))throw std::runtime_error("Fresh output required");fs::create_directories(output);
  wchar_t dark[2]{};uiDark=useDarkUi(GetEnvironmentVariableW(L"UDM_TEST_PENDING_DARK",dark,2)==1&&dark[0]==L'1',windowsHighContrast());
  preparationCases(output,*this);
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
