// Headless calls into the actual UI command adapter and guarded backend paths.
// No browser, native window, clipboard or downloaded-file launch is performed.
#include <afxwin.h>
#include <afxcmn.h>
#include <afxdlgs.h>
#include <afxole.h>
#include <atlimage.h>
#include "Ui.hpp"
#include <iostream>
namespace udm {struct CatalogCheckpointTestAccess {
 static void observe(Manager& m,std::function<void(const char*)> f){Lock lock(m.mutex);m.catalogCheckpoint=std::move(f);}
};}
using namespace udm;
static CWinApp application;
struct CommandGate {
 std::promise<void> entered,released;std::shared_future<void> ready=entered.get_future().share(),wait=released.get_future().share();
 std::atomic_bool armed=false,held=false,done=false,expired=false;std::atomic_uint flushes=0;bool failSecond=false;
 void observe(const char* point){const std::string_view phase(point);if(phase=="recovery-catalog-prepared")armed=true;if(armed&&phase=="temporary-flushed"&&flushes.fetch_add(1)==1&&failSecond)throw std::runtime_error("Owned properties save failure");if(armed&&phase=="temporary-flushed"&&!held.exchange(true)){entered.set_value();if(wait.wait_for(std::chrono::seconds(10))!=std::future_status::ready){expired=true;throw std::runtime_error("Owned command gate expired");}}}
 void release(){if(!done.exchange(true))released.set_value();}~CommandGate(){release();}
};
struct ClearCommandObserver {Manager& m;~ClearCommandObserver(){CatalogCheckpointTestAccess::observe(m,{});}};
int wmain(int argc,wchar_t** argv){
 if(argc!=2)return 2;const auto root=fs::absolute(argv[1]);if(fs::exists(root))return 2;fs::create_directories(root);
 if(!AfxWinInit(GetModuleHandleW(nullptr),nullptr,GetCommandLineW(),SW_HIDE))return 3;
 WSADATA wsa{};if(WSAStartup(MAKEWORD(2,2),&wsa))return 3;auto apartment=CoInitializeEx(nullptr,COINIT_MULTITHREADED);
 Json checks=Json::array(),cases=Json::array();std::string error,activeCase,lastWorkflowError;
 auto check=[&](bool good,const char* name){checks.push_back({{"name",name},{"passed",good}});if(!good)throw std::runtime_error(name);};
 try{
  for(const std::string command:{"progress-resume","recording-resume","save-live","properties-complete","properties-merge","properties-origin","cancel-pending-complete","cancel-pending-paused","cancel-file-info-complete","cancel-file-info-paused","cancel-accepted-during-recovery","cancel-receipt-changed","cancel-browser-complete","cancel-browser-paused","cancel-browser-fingerprint","properties-save-failure"}){
   activeCase=command;const auto folder=root/wide(command);Manager m(folder/L"catalog");auto prefs=m.state["Settings"];prefs["DownloadFolder"]=utf8((folder/L"files").wstring());prefs["CategoryFolders"]=false;m.setSettings(prefs);fs::create_directories(folder/L"files");auto j=m.add("https://fixture.example/video","","output.bin","Main queue",true);
   const bool browser=command.find("cancel-browser-")==0;const bool paused=command=="properties-save-failure"||command=="cancel-browser-fingerprint"||command=="properties-merge"||command=="properties-origin"||command.find("paused")!=std::string::npos||command=="cancel-accepted-during-recovery"||command=="cancel-receipt-changed";
   if(command.find("cancel-")==0)j->data["Status"]="Awaiting confirmation";
   if(command=="recording-resume"||command=="save-live"){j->data["LiveRecording"]=true;j->data["LiveSaveReady"]=true;}
   Headers originalHeaders{{"Authorization","Bearer original"},{"Cookie","session=old"},{"Referer","https://fixture.example/old"}};j->data["ProtectedHeaders"]=protect(legacyDictionary(Json(originalHeaders)).dump());m.save();auto desiredHeaders=originalHeaders;setHeader(desiredHeaders,"Authorization","Bearer edited");setHeader(desiredHeaders,"Referer","https://fixture.example/new");PropertiesHeaderPatch patch(originalHeaders,desiredHeaders,str(j->data,"Url"));
   auto uuid=guid();for(int i:{20,16,12,8})uuid.insert(i,"-");const auto token=std::to_string(epoch())+"-"+uuid,fingerprint=std::string(64,'a');
   if(browser){m.state["BrowserCaptures"][token]={{"created",epoch()},{"protocol",3},{"status","accepted"},{"id",j->id()},{"fingerprint",protect(fingerprint)},{"presentation",{{"id",j->id()},{"kind","download"}}}};m.save();}
   const auto stage=folder/L"files"/(paused?L"unrecognized.stage":L"stage.part");atomicText(stage,"verified command bytes",false);const auto hash=fileHash(stage);const auto journal=m.root/L"publications"/(wide(j->id())+L".json");
   CatalogCheckpointTestAccess::observe(m,[&](const char* point){if(std::string_view(point)==(paused?"publication-intent-written":"publication-file-moved"))throw std::runtime_error("Owned interrupted command publication");});try{m.publishFile(j,stage,hash);}catch(...){}CatalogCheckpointTestAccess::observe(m,{});check(fs::exists(journal),"Command begins with a real interrupted publication journal");
   auto gate=std::make_shared<CommandGate>();gate->failSecond=command=="properties-save-failure";CatalogCheckpointTestAccess::observe(m,[gate](const char* point){gate->observe(point);});ClearCommandObserver clear{m};std::atomic_int policyChecks=0;bool removed=false;ProgressCommandResult result;
   auto work=std::async(std::launch::async,[&]{
    if(command=="progress-resume"){ProgressCommandRequest r;r.displayedAction=ProgressAction::Resume;result=applyProgressCommand(m,j,r);}
    else if(command=="recording-resume"){ProgressCommandRequest r;r.menuAction=ProgressMenuAction::ResumeRecording;result=applyProgressCommand(m,j,r);}
    else if(command=="save-live"){ProgressCommandRequest r;r.displayedAction=ProgressAction::SaveLive;result=applyProgressCommand(m,j,r);}
    else if(command.find("properties-")==0)m.configureProperties(j,Json{{"Description","typed properties description"}},patch);
    else if(browser)m.cancelBrowserMediaPresentation(token,fingerprint);
    else removed=m.cancelUnconfirmedOffer(j,command.find("file-info")!=std::string::npos?Manager::OfferCancellation::FileInfo:Manager::OfferCancellation::AwaitingOnly,[&]{++policyChecks;return !yes(j->data,"CancelIdentityChanged");});
   });struct Release {std::shared_ptr<CommandGate> gate;~Release(){gate->release();}} release{gate};
   check(gate->ready.wait_for(std::chrono::seconds(5))==std::future_status::ready,"Actual UI/backend command reaches the held recovery catalog flush");
   {std::unique_lock<std::recursive_mutex> lock(m.mutex,std::try_to_lock);check(lock.owns_lock(),"The migrated command adapter leaves the model available during actual recovery persistence");check(m.isActive(j)&&m.downloadDestinationReserved(j->target()),"Job and destination ownership remain held during the detached checkpoint");check(m.snapshot()["Downloads"].size()==1,"An unrelated UI snapshot is available through the same held boundary");j->data["Description"]="independent current metadata";auto current=readHeaders(j->data);setHeader(current,"Cookie","session=current");current["Accept-Language"]="retained";j->data["ProtectedHeaders"]=protect(legacyDictionary(Json(current)).dump());if(command=="properties-origin")j->data["Url"]="https://other.example/video";if(command=="cancel-accepted-during-recovery")j->data["IndividualStart"]=true;if(command=="cancel-receipt-changed")j->data["CancelIdentityChanged"]=true;if(command=="cancel-browser-fingerprint")m.state["BrowserCaptures"][token]["fingerprint"]=protect(std::string(64,'b'));}
   if(browser&&command!="cancel-browser-fingerprint"){bool refused=false;try{m.cancelBrowserMediaPresentation(token,fingerprint);}catch(const std::exception& e){refused=std::string(e.what()).find("pending file publication")!=std::string::npos;}check(refused,"A duplicate browser cancellation cannot acknowledge a provisional recovered row while its publication owner is active");{Lock lock(m.mutex);check(m.state["BrowserCaptures"][token].contains("presentation"),"Refused overlapping cancellation preserves its durable presentation claim");}}
   gate->release();std::string failure;try{work.get();}catch(const std::exception& e){failure=e.what();}lastWorkflowError=failure;if(!failure.empty())std::cerr<<command<<": "<<failure<<"\n";CatalogCheckpointTestAccess::observe(m,{});if(browser)removed=std::find(m.jobs.begin(),m.jobs.end(),j)==m.jobs.end();check(!gate->expired,"Every held command is explicitly released before its deadline");
   if(command=="properties-complete")check(failure=="The saved file was recovered at its previous destination. Use Move in File Properties to change its location.","Properties retains the exact recovered-completion destination warning");
   else if(command=="properties-origin")check(failure.find("another site")!=std::string::npos,"Sensitive header assignment revalidates the current origin after recovery");
   else if(command=="cancel-browser-fingerprint")check(failure.find("browser cancellation changed")!=std::string::npos,"A receipt fingerprint changed during detached persistence prevents atomic job removal");
   else if(command=="properties-save-failure")check(failure=="Owned properties save failure","A failed final Properties save is reported instead of acknowledging the typed patch");
   else check(failure.empty(),"The guarded command completes without an unexpected backend failure");
   if(command=="properties-merge"){auto current=readHeaders(j->data);check(headerValue(current,"Cookie")=="session=current"&&headerValue(current,"Accept-Language")=="retained"&&headerValue(current,"Authorization")=="Bearer edited"&&headerValue(current,"Referer")=="https://fixture.example/new","Properties merges typed logical header changes with the current recovered session");check(str(j->data,"Description")=="typed properties description","A successful Properties patch applies typed metadata after recovery");}
   else check(str(j->data,"Description")=="independent current metadata","Unsubmitted or rejected commands preserve independent current metadata");
   if(command=="properties-save-failure"){auto current=readHeaders(j->data);check(headerValue(current,"Cookie")=="session=current"&&headerValue(current,"Accept-Language")=="retained"&&headerValue(current,"Authorization")=="Bearer original"&&headerValue(current,"Referer")=="https://fixture.example/old","Failed Properties persistence rolls back typed credentials while preserving the independently refreshed session");}
   const bool shouldRemove=command=="cancel-pending-paused"||command=="cancel-file-info-paused"||command=="cancel-browser-paused";
   check(removed==shouldRemove&&(std::find(m.jobs.begin(),m.jobs.end(),j)==m.jobs.end())==shouldRemove,"Only a still-unconfirmed recovered Paused offer is removed; completion and newly accepted work survive");
   if(!paused)check(str(j->data,"Status")=="Complete"&&fileHash(j->target())==hash,"The current completion wins over stale Resume, Save and cancellation actions");
   else if(!shouldRemove)check(fs::exists(stage)&&fileHash(stage)==hash,"A paused or rejected command preserves its complete staged source");
   check(!m.isActive(j)&&!fs::exists(journal),"Accepted recovery retires its journal and releases runtime ownership");
   if(command.find("cancel-")==0&&!browser)check(policyChecks==2,"Cancellation identity is checked before recovery and again inside its final atomic continuation");
   if(browser)check(m.state["BrowserCaptures"][token].contains("presentation")== (command=="cancel-browser-fingerprint"),"Only the terminal original cancellation releases the browser presentation claim");
   const auto disk=Json::parse(readText(m.root/L"state.json"));check(disk["Downloads"].size()==(shouldRemove?0:1),"The durable catalog records exactly the accepted removal or retained job");cases.push_back({{"command",command},{"removed",removed},{"error",failure},{"modelAvailable",true}});m.stop();
  }
  // No recovery or speculative write is allowed for stale/detached command guards.
  {Manager m(root/L"stale");auto job=m.add("https://fixture.example/stale");auto before=job->snapshot();bool called=false;check(!m.resumeIf(job,[&]{called=true;return false;})&&called&&job->snapshot()==before,"A rejected Resume predicate performs no queue or recovery mutation");m.jobs.clear();check(!m.cancelUnconfirmedOffer(job,Manager::OfferCancellation::AwaitingOnly),"A detached pending cancellation is a benign no-op");m.stop();}
 }catch(const std::exception& e){error=e.what();}
 if(SUCCEEDED(apartment))CoUninitialize();WSACleanup();atomicText(root/L"results.json",Json{{"passed",error.empty()},{"error",error},{"case",activeCase},{"workflowError",lastWorkflowError},{"checks",checks},{"cases",cases},{"headlessActualCommandAdapters",true}}.dump(2),false);std::cout<<checks.size()<<" checks; "<<(error.empty()?"passed":error)<<"\n";return error.empty()?0:1;
}
