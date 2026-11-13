#include "Core.hpp"
#include "MediaAdmission.hpp"
#include "AdaptiveCapture.hpp"
#include "BrowserAdmissionContext.hpp"
#include "FileInfoInitialView.hpp"
#include "PendingFileInfoInbox.hpp"
#include <future>
#include <iostream>
using namespace udm;
namespace udm {struct CatalogCheckpointTestAccess {static void observe(Manager& manager,std::function<void(const char*)> callback){Lock lock(manager.mutex);manager.catalogCheckpoint=std::move(callback);}};}
static Json checks=Json::array();
static void check(bool ok,const std::string& name){checks.push_back({{"name",name},{"passed",ok}});if(!ok)throw std::runtime_error(name);}
template<class Action>static void rejects(Action&& action,const std::string& name){bool rejected=false;try{action();}catch(const std::exception&){rejected=true;}check(rejected,name);}
#define UDM_MEDIA_CRASH_FIXTURE_NAME L"Udm.DetachedMediaAdmissionTests.exe"
#include "MediaAdmissionChecks.hpp"
int wmain(int argc,wchar_t** argv){
 BrowserAdmissionContext admissionContext;
 if(argc==4&&std::wstring(argv[1])==L"--media-crash-fixture")return mediaCrashChild(argv[2],utf8(argv[3]));
 if(argc!=2)return 2;const fs::path root=fs::absolute(argv[1]);if(fs::exists(root))return 3;fs::create_directories(root);std::string error;
 try{
  for(bool originalFails:{false,true}){
   const auto folder=root/(originalFails?L"containing-success":L"normal");Manager manager(folder/L"state");mediaSettings(manager,folder);
   const auto request=mediaRequest(mediaToken());const auto token=str(request,"mediaToken");
   std::promise<void> entered,release;auto ready=entered.get_future();auto released=release.get_future().share();
   std::atomic_int rejections{0};const RejectedFileInfoSink rejected=[&](const std::string&){++rejections;};
   auto admission=std::async(std::launch::async,[&]{BrowserAdmissionContext context(true,nullptr,&rejected);return manager.receiveBrowserMedia(request,[&](const char* stage){if(std::string(stage)=="temporary-flushed"){
    entered.set_value();if(released.wait_for(std::chrono::seconds(8))!=std::future_status::ready)throw std::runtime_error("Fixture release timed out");
    if(originalFails)throw std::runtime_error("Original admission write failed");
   }});});
   const bool atGate=ready.wait_for(std::chrono::seconds(5))==std::future_status::ready;
   auto view=std::async(std::launch::async,[&]{return manager.snapshot();});
   const bool viewReady=view.wait_for(std::chrono::milliseconds(500))==std::future_status::ready;
   Json during;if(viewReady)during=view.get();
   auto status=std::async(std::launch::async,[&]{return mediaAdmissionStatus(manager,token);});
   const bool statusReady=status.wait_for(std::chrono::milliseconds(500))==std::future_status::ready;
   Json pendingStatus;if(statusReady)pendingStatus=status.get();
   const auto name=adaptiveCaptureName("receipt fixture",request.at("plan"));const auto reserved=folder/L"files"/wide(name);
   auto destination=std::async(std::launch::async,[&]{return manager.pendingMediaDestination(reserved);});
   const bool destinationReady=destination.wait_for(std::chrono::milliseconds(500))==std::future_status::ready;bool held=destinationReady?destination.get():false;
   // The ordinary writer queues a newer containing snapshot while the first
   // writer is gated. It must not allocate the hidden reservation's filename.
   std::promise<void> adding;auto addingReady=adding.get_future();
   CatalogCheckpointTestAccess::observe(manager,[&](const char* stage){if(std::string(stage)=="snapshot-prepared")adding.set_value();});
   struct ClearCheckpoint {Manager& manager;~ClearCheckpoint(){CatalogCheckpointTestAccess::observe(manager,{});}} clear{manager};
   auto other=std::async(std::launch::async,[&]{return manager.add("https://files.example.test/file",utf8((folder/L"files").wstring()),name,"Main queue",true);});
   const bool otherStarted=addingReady.wait_for(std::chrono::seconds(2))==std::future_status::ready;
   // Give the second writer a bounded opportunity to reach its write wait.
   const bool otherWaiting=other.wait_for(std::chrono::milliseconds(50))==std::future_status::timeout;
   release.set_value();auto accepted=admission.get();auto ordinary=other.get();
   if(!viewReady)during=view.get();if(!statusReady)pendingStatus=status.get();if(!destinationReady)held=destination.get();
   check(atGate,"Admission reaches a real flushed-temporary-file gate");
   check(viewReady&&during["Downloads"].empty(),"History remains readable and excludes the tentative media job during disk IO");
   check(statusReady&&str(pendingStatus,"status")=="uncertain","Recovery can inspect an in-flight token without releasing or accepting it prematurely");
   check(destinationReady&&held,"The hidden media destination remains reserved while its write is pending");
   check(otherStarted&&otherWaiting,"A concurrent ordinary write queues behind the gated publication");
   check(str(ordinary->data,"FileName")!=str(accepted->data,"FileName"),"Ordinary filename allocation respects pending media ownership");
   auto final=manager.snapshot();check(final["Downloads"].size()==2,"Installing the accepted media preserves the concurrent ordinary job");
   const auto receipt=mediaAdmissionStatus(manager,token);check(str(receipt,"status")=="accepted"&&str(receipt,"id")==accepted->id(),"The accepted receipt identifies the installed job exactly once");
   check(manager.receive(request)==accepted&&manager.jobs.size()==2,"Replaying the accepted media token does not append another job");
   check(!manager.pendingMediaDestination(reserved),"The installed job releases its transient destination ownership");
   check(rejections==0,"A successful containing publication never sends a false prepared-dialog rejection");
   auto saved=Json::parse(readText(manager.root/L"state.json"));check(saved["Downloads"].size()==2&&str(saved["BrowserCaptures"][token],"id")==accepted->id(),"Published history contains both jobs and the matching media receipt");
  }
  {
   const auto folder=root/L"failed-alone";Manager manager(folder/L"state");mediaSettings(manager,folder);auto request=mediaRequest(mediaToken());auto before=manager.snapshot();
   std::shared_ptr<const FileInfoInitialView> view;std::string rejectedId;bool rejectionOutsideLock=false;
   const PreparedFileInfoSink prepared=[&](auto input){view=std::move(input);};
   const RejectedFileInfoSink rejected=[&](const std::string& id){rejectedId=id;auto read=std::async(std::launch::async,[&]{std::unique_lock<std::recursive_mutex> lock(manager.mutex,std::try_to_lock);return lock.owns_lock();});rejectionOutsideLock=read.get();throw std::runtime_error("Rejected observer failed");};
   {BrowserAdmissionContext context(true,&prepared,&rejected);std::string failure;try{manager.receiveBrowserMedia(request,[](const char* stage){if(std::string(stage)=="temporary-flushed")throw std::runtime_error("Write failed");});}catch(const std::exception& e){failure=e.what();}
    check(failure=="Write failed","An uncommitted detached write preserves its failure even when the rejection observer throws");}
   check(view&&rejectedId==view->jobId&&rejectionOutsideLock,"Definite rejection identifies the delivered view and runs outside the Manager lock");
   check(manager.snapshot()==before&&!manager.pendingMediaAdmission(str(request,"mediaToken")),"A failed detached admission leaves no live or transient orphan");
   auto accepted=manager.receive(request);check(accepted&&manager.jobs.size()==1,"A detached admission with no successful write can retry once");
  }
  {
   const auto folder=root/L"lost-ack-outcome";Manager manager(folder/L"state");mediaSettings(manager,folder);auto request=mediaRequest(mediaToken());bool rejected=false;
   const RejectedFileInfoSink sink=[&](const std::string&){rejected=true;};BrowserAdmissionContext context(true,nullptr,&sink);
   rejects([&]{manager.receiveBrowserMedia(request,[](const char* stage){if(std::string(stage)=="committed")throw std::runtime_error("Acknowledgement lost after acceptance");});},"A lost acknowledgement still reports an error to its caller");
   check(!rejected&&str(mediaAdmissionStatus(manager,str(request,"mediaToken")),"status")=="accepted"&&manager.jobs.size()==1,"An error after durable acceptance never reports a rejected File Info operation");
  }
  {
   BrowserAdmissionContext legacy(false);const auto folder=root/L"nested-legacy";Manager manager(folder/L"state");mediaSettings(manager,folder);bool detachedSeen=false;
   Lock lock(manager.mutex);auto accepted=manager.receiveBrowserMedia(mediaRequest(mediaToken()),[&](const char* stage){if(std::string(stage)=="detached-ready")detachedSeen=true;});
   check(accepted&&!detachedSeen&&manager.jobs.size()==1,"Nested legacy callers keep synchronous admission and never wait for detached resolution under an outer lock");
  }
  {
   const auto folder=root/L"prepared-view";Manager manager(folder/L"state");mediaSettings(manager,folder);
   std::shared_ptr<const FileInfoInitialView> view;bool flushed=false,deliveredBeforeWrite=false,readable=false;
   const PreparedFileInfoSink sink=[&](std::shared_ptr<const FileInfoInitialView> prepared){
    view=std::move(prepared);deliveredBeforeWrite=!flushed;
    auto read=std::async(std::launch::async,[&]{std::unique_lock<std::recursive_mutex> lock(manager.mutex,std::try_to_lock);return lock.owns_lock()?manager.snapshot():Json();});
    readable=read.wait_for(std::chrono::milliseconds(500))==std::future_status::ready;
    const auto snapshot=read.get();readable=readable&&snapshot.is_object()&&snapshot["Downloads"].empty();
   };
   BrowserAdmissionContext context(true,&sink);auto accepted=manager.receiveBrowserMedia(mediaRequest(mediaToken()),[&](const char* stage){if(std::string(stage)=="temporary-flushed")flushed=true;});
   check(view&&flushed&&deliveredBeforeWrite,"The immutable File Info view is handed off before the catalog write");
   check(readable,"A view consumer runs outside the Manager lock and cannot see a tentative history row");
   check(view->target==accepted->target()&&str(view->data,"FileName")==str(accepted->data,"FileName"),"Prepared File Info destination and name identify the accepted selection");
   check(view->media&&!view->directResponse&&!view->formRequest&&view->categories==manager.categories(),"Prepared media flags and category choices match the existing dialog inputs");
   check(downloadAddress(view->addressInputs)==downloadAddress(downloadAddressInputs(*accepted)),"The render snapshot preserves selected display-address precedence");
   const auto prior=view->data;const auto categories=view->categories;const auto preferences=view->preferences;
   accepted->data["FileName"]="changed-after-view.mp4";accepted->video=std::make_shared<Job>(Json{{"Id",guid()},{"FileName","child.bin"},{"Size",123}});manager.state["Settings"]["SaveFolderHistory"]=Json::array({"changed"});
   check(view->data==prior&&view->categories==categories&&view->preferences==preferences,"Later job, child-track and preference edits cannot mutate a retained view");
   accepted->data["ProtectedRequest"]="FORM_BODY_SENTINEL";accepted->data["Segments"]=Json::array({Json{{"private","segment-sentinel"}}});
   auto projected=captureFileInfoInitialView(manager,*accepted);
   check(!projected->data.contains("ProtectedRequest")&&!projected->data.contains("Segments")&&!projected->data.contains("ProtectedAdaptive"),"Render data excludes form bodies, segment history and adaptive plans");
   check(projected->formRequest&&num(projected->data["Video"],"Size")==123,"Request kind and selected track size survive projection without copying their payloads");
  }
  {
   const auto folder=root/L"view-rejected";Manager manager(folder/L"state");mediaSettings(manager,folder);auto request=mediaRequest(mediaToken());const auto before=manager.snapshot();
   const PreparedFileInfoSink rejecting=[](std::shared_ptr<const FileInfoInitialView>){throw std::runtime_error("The view consumer closed");};
   {BrowserAdmissionContext context(true,&rejecting);rejects([&]{manager.receiveBrowserMedia(request);},"Failed prepared-view delivery does not acknowledge an uncommitted operation");}
   check(manager.snapshot()==before&&!manager.pendingMediaAdmission(str(request,"mediaToken"))&&manager.receive(request),"Rejected delivery restores transient ownership and the same token can retry");
  }
  {
   const auto folder=root/L"view-inbox";Manager manager(folder/L"state");mediaSettings(manager,folder);auto job=manager.receive(mediaRequest(mediaToken()));auto view=captureFileInfoInitialView(manager,*job);
   using Inbox=PendingFileInfoInbox;Inbox inbox;Inbox::Key key=0;auto ticket=inbox.publish(view,[&](auto value){key=value;return true;});
   check(key!=0&&!inbox.take(key+1),"Prepared delivery uses a nonzero opaque key and rejects unknown messages");
   auto operation=inbox.take(key);check(operation&&operation->view==view&&!inbox.take(key),"An inbox message transfers its owned view exactly once");
   check(operation->outcome()==Inbox::Outcome::Pending,"UI can claim a view while its worker outcome is still pending");
   Inbox::Key timerKey=0;auto timerTicket=inbox.publish(view,[&](auto value){timerKey=value;return true;});auto timerView=inbox.takeForJob(job->id());
   check(timerView&&timerView->view==view&&!inbox.take(timerKey)&&!inbox.takeForJob("another-job"),"A committed-offer timer can claim by identity before message dispatch and leave the queued key harmless");
   ticket.accepted("another-job");check(operation->outcome()==Inbox::Outcome::Pending,"A different accepted identity cannot bind this prepared view");
   ticket.accepted(job->id());ticket={};check(operation->outcome()==Inbox::Outcome::Accepted,"Worker destruction cannot erase a proven accepted outcome");
   Inbox::Key abandonedKey=0;auto abandoned=inbox.publish(view,[&](auto value){abandonedKey=value;return true;});auto abandonedView=inbox.take(abandonedKey);abandoned={};
   check(abandonedView->outcome()==Inbox::Outcome::Uncertain,"An interrupted acknowledgement remains uncertain rather than falsely failed");
   Inbox::Key failedKey=0;rejects([&]{inbox.publish(view,[&](auto value){failedKey=value;return false;});},"A failed window notification rejects prepared delivery before persistence");
   check(!inbox.take(failedKey),"Failed notification retracts its unclaimed inbox entry");
   Inbox::Key closingKey=0;auto closing=inbox.publish(view,[&](auto value){closingKey=value;return true;});auto claimed=inbox.take(closingKey);inbox.close();closing.accepted(job->id());
   check(inbox.closed()&&claimed->outcome()==Inbox::Outcome::Accepted,"Closing the UI inbox does not revoke a worker's later durable acceptance");
   bool notified=false;rejects([&]{inbox.publish(view,[&](auto){notified=true;return true;});},"A closed inbox rejects new prepared views");
   check(!notified,"Closed delivery never posts another window message");
   Inbox restarted;Inbox::Key nextKey=0;auto nextTicket=restarted.publish(view,[&](auto value){nextKey=value;return true;});
   check(nextKey!=closingKey&&!restarted.take(closingKey)&&restarted.take(nextKey),"Restarted integration cannot interpret a stale message as a new view");
   Inbox bounded;std::vector<Inbox::Ticket> held;for(int i=0;i<64;++i)held.push_back(bounded.publish(view,[](auto){return true;}));
   rejects([&]{bounded.publish(view,[](auto){return true;});},"Unclaimed prepared deliveries have a fixed capacity");bounded.close();held.clear();
   Inbox rejectionInbox;Inbox::Key rejectedKey=0;std::shared_ptr<const Inbox::Operation> rejected;
   {auto rejectedTicket=rejectionInbox.publish(view,[&](auto value){rejectedKey=value;return true;});rejected=rejectionInbox.take(rejectedKey);
    rejectedTicket.rejected("another-job");check(rejected->outcome()==Inbox::Outcome::Pending,"A rejection for another job cannot terminate a prepared operation");
    rejectedTicket.rejected(job->id());check(rejected->outcome()==Inbox::Outcome::Rejected,"A matching proven rejection terminates the pending operation");}
   check(rejected->outcome()==Inbox::Outcome::Rejected,"Ticket abandonment cannot replace a proven rejection with uncertainty");
   closing.rejected(job->id());check(claimed->outcome()==Inbox::Outcome::Accepted,"A late rejection cannot downgrade proven acceptance");
   bool outerRejected=false;const RejectedFileInfoSink outer=[&](const std::string&){outerRejected=true;};
   {BrowserAdmissionContext context(true,nullptr,&outer);{BrowserAdmissionContext nested(false);notifyRejectedBrowserFileInfo(job->id());check(!outerRejected,"Legacy nested admission masks the outer rejection observer");}notifyRejectedBrowserFileInfo(job->id());}
   check(outerRejected&&rejectedBrowserFileInfo==nullptr,"Admission context restores rejection observers after nested and outer scopes");
   auto fresh=captureFileInfoInitialView(manager,*job);check(sameFileInfoInitialView(*view,*fresh),"An unchanged accepted job can reuse its exact prepared render inputs");
   job->data["Description"]="Edited after acceptance";fresh=captureFileInfoInitialView(manager,*job);
   check(!sameFileInfoInitialView(*view,*fresh),"A later job edit invalidates prepared rendering instead of replacing the user's current values");
  }
  {
   const auto folder=root/L"no-file-info";Manager manager(folder/L"state");mediaSettings(manager,folder);auto settings=manager.state["Settings"];settings["SkipBrowserFileInfo"]=true;manager.setSettings(settings);bool delivered=false;
   const PreparedFileInfoSink sink=[&](std::shared_ptr<const FileInfoInitialView>){delivered=true;};BrowserAdmissionContext context(true,&sink);auto job=manager.receive(mediaRequest(mediaToken()));
   check(job&&!delivered,"A media request without File Info creates no unused prepared delivery");
  }
 }catch(const std::exception& e){error=e.what();}
 atomicText(root/L"results.json",Json{{"passed",error.empty()},{"error",error},{"checks",checks}}.dump(2),false);
 std::cout<<checks.size()<<" checks; "<<(error.empty()?"passed":error)<<"\n";return error.empty()?0:1;
}
