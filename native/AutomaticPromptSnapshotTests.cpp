#include "AutomaticPromptSnapshot.hpp"
#include <future>
#include <iostream>
using namespace udm;
int wmain(int argc,wchar_t** argv){
 if(argc!=2)return 2;const auto root=fs::absolute(argv[1]);if(fs::exists(root))return 2;fs::create_directories(root);Json checks=Json::array();std::string error;
 const auto check=[&](bool ok,const char* name){checks.push_back({{"name",name},{"passed",ok}});if(!ok)throw std::runtime_error(name);};
 try{
  Manager manager(root/L"catalog");const auto make=[&](const char* name,const char* status){return std::make_shared<Job>(Json{{"Id",guid()},{"FileName",name},{"Folder",utf8(root.wstring())},{"Url","https://example.invalid/file"},{"Status",status},{"QueueMember",false}});};
  auto awaiting=make("awaiting.bin","Awaiting confirmation"),duplicate=make("duplicate.bin","Awaiting duplicate choice"),legacy=make("legacy.bin","Complete"),auth=make("auth.bin","Failed");
  auth->data["Url"]="https://auth.example.invalid/file";auth->data["AuthenticationOrigin"]="https://auth.example.invalid";auth->data["AuthenticationScheme"]="Basic";auth->data["LastHttpStatus"]=401;auth->data["AuthenticationPromptPending"]=true;
  manager.jobs={awaiting,duplicate,legacy,auth};manager.pendingOffers={legacy,legacy};std::set<std::string> captured,seen;std::vector<AutomaticConfirmation> reserved;
  const auto review=std::to_string(epoch())+"-11111111-2222-4333-8444-555555555555";manager.state["BrowserCaptures"]={{review,Json{{"protocol",2},{"status","review"}}}};
  auto first=tryAutomaticPromptSnapshot(manager,false,captured,seen,reserved);
  check(first&&first->confirmations.size()==3&&first->confirmations[0].job==legacy&&first->confirmations[0].legacy,"Duplicate explicit legacy offers transfer to one owned presentation reservation");
  check(manager.pendingOffers.empty()&&first->reviews==std::vector<std::string>{review}&&first->authentication==auth,"Successful discovery captures ready reviews and authentication with the confirmation cohort");
  check(seen.empty()&&yes(auth->data,"AuthenticationPromptPending"),"Discovery does not mark a confirmation seen or clear an authentication challenge");
  reserved=first->confirmations;auto retry=tryAutomaticPromptSnapshot(manager,false,captured,seen,reserved);check(retry&&retry->confirmations.size()==3&&retry->confirmations[0].legacy,"Unpresented UI reservations survive the next discovery pass");
  seen.insert(awaiting->id());retry=tryAutomaticPromptSnapshot(manager,false,captured,seen,reserved);check(retry->confirmations.size()==2,"Previously handled automatic confirmations are not repeated");
  seen.insert(legacy->id());check(tryAutomaticConfirmationReady(manager,{legacy,legacy->id(),true},seen)==AutomaticPromptReadiness::Ready,"An explicit legacy request remains presentable even when the job was seen earlier");
  auto capturePass=tryAutomaticPromptSnapshot(manager,true,captured,seen,{});check(capturePass&&capturePass->confirmations.empty()&&!capturePass->authentication&&capturePass->reviews.size()==1,"Capture-only discovery avoids new automatic confirmations and authentication");
  capturePass=tryAutomaticPromptSnapshot(manager,true,captured,seen,reserved);check(capturePass->confirmations.size()==2,"Capture-only wake-ups retain already-owned unpresented reservations");
  const auto receipt=std::to_string(epoch())+"-aaaaaaaa-bbbb-4ccc-8ddd-eeeeeeeeeeee";manager.state["BrowserCaptures"][receipt]={{"status","accepted"},{"presentation",{{"id",duplicate->id()},{"kind","download"}}}};manager.pendingOffers={duplicate};
  auto owned=tryAutomaticPromptSnapshot(manager,false,captured,seen,reserved);check(owned->confirmations.size()==1&&owned->confirmations[0].job==legacy&&manager.pendingOffers.empty(),"A durable browser presentation owns its legacy duplicate without a second dialog");
  check(tryAutomaticConfirmationReady(manager,{duplicate,duplicate->id(),true},seen)==AutomaticPromptReadiness::Obsolete,"Fresh receipt ownership invalidates an older legacy reservation before presentation");
  manager.state["BrowserCaptures"].erase(receipt);duplicate->data["Status"]="Paused";check(tryAutomaticConfirmationReady(manager,{duplicate,duplicate->id(),false},seen)==AutomaticPromptReadiness::Obsolete,"An automatic candidate whose status changed is not presented");
  manager.jobs.erase(std::remove(manager.jobs.begin(),manager.jobs.end(),legacy),manager.jobs.end());check(tryAutomaticConfirmationReady(manager,{legacy,legacy->id(),true},seen)==AutomaticPromptReadiness::Obsolete,"Removed jobs cannot be resurrected by a UI reservation");manager.jobs.push_back(legacy);
  captured.insert(legacy->id());manager.pendingOffers={legacy};owned=tryAutomaticPromptSnapshot(manager,false,captured,seen,reserved);check(owned->confirmations.empty()&&manager.pendingOffers.empty(),"Already handled capture IDs suppress same-pass legacy duplicates");captured.clear();
  std::vector<JobPtr> many;for(int i=0;i<10;++i){auto job=make("indexed.bin","Paused");manager.jobs.push_back(job);many.push_back(job);}auto removed=make("removed.bin","Paused");manager.pendingOffers=many;manager.pendingOffers.push_back(many.front());manager.pendingOffers.push_back(removed);
  auto indexed=tryAutomaticPromptSnapshot(manager,false,captured,seen,{});check(indexed->confirmations.size()==10&&indexed->confirmations.front().job==many.front(),"Large reservation cohorts retain FIFO order while deduplicating repeated offers");
  check(std::all_of(indexed->confirmations.begin(),indexed->confirmations.end(),[&](const auto& candidate){return candidate.legacy&&candidate.job!=removed;}),"Indexed membership validation excludes removed jobs without changing explicit-offer ownership");
  manager.pendingOffers={legacy};const auto good=manager.state["BrowserCaptures"];manager.state["BrowserCaptures"]["invalid"]={{"status","accepted"},{"presentation",{{"id",legacy->id()},{"kind","invalid"}}}};bool rejected=false;try{tryAutomaticPromptSnapshot(manager,false,captured,seen,{});}catch(const std::exception&){rejected=true;}
  check(rejected&&manager.pendingOffers==std::vector<JobPtr>{legacy},"Discovery validation failure cannot consume the original model-owned offer");manager.state["BrowserCaptures"]=good;
  manager.save();fs::create_directory(manager.root/L"state.json.tmp");bool saveFailed=false;try{tryClaimAutomaticAuthentication(manager,auth);}catch(const std::exception&){saveFailed=true;}fs::remove(manager.root/L"state.json.tmp");
  check(saveFailed&&yes(auth->data,"AuthenticationPromptPending"),"A failed durable authentication claim restores its exact pending flag");
  check(tryClaimAutomaticAuthentication(manager,auth)==AutomaticPromptReadiness::Ready&&!yes(auth->data,"AuthenticationPromptPending"),"A successful authentication claim acknowledges the challenge only after saving");
  const auto disk=Json::parse(readText(manager.root/L"state.json"));bool saved=false;for(const auto& job:disk["Downloads"])if(str(job,"Id")==auth->id())saved=!yes(job,"AuthenticationPromptPending");check(saved,"The claimed authentication flag is present in the real saved catalog");
  check(tryClaimAutomaticAuthentication(manager,auth)==AutomaticPromptReadiness::Obsolete,"A previously claimed challenge cannot be claimed twice");auth->data["AuthenticationPromptPending"]=true;auth->data["QueueOrigin"]=true;
  check(tryClaimAutomaticAuthentication(manager,auth)==AutomaticPromptReadiness::Obsolete&&yes(auth->data,"AuthenticationPromptPending"),"Queue-origin authentication remains owned by its existing progress workflow");auth->data["QueueOrigin"]=false;manager.state["Settings"]["SuppressAuthenticationDialog"]=true;
  auto suppressed=tryAutomaticPromptSnapshot(manager,false,captured,seen,{});check(!suppressed->authentication&&tryClaimAutomaticAuthentication(manager,auth)==AutomaticPromptReadiness::Obsolete,"Current suppression settings gate discovery and the eventual claim");manager.state["Settings"]["SuppressAuthenticationDialog"]=false;
  BrowserBatchRow row;row.address="https://example.invalid/batch";row.linkText="Owned batch";row.headers={{"X-Owned","fixture"}};row.observedSize=42;manager.pendingBrowserBatches={{row},{row}};manager.pendingOffers={legacy};manager.browserRecoveryRequested=true;
  std::promise<void> entered,release;auto ready=entered.get_future();auto released=release.get_future().share();std::atomic_bool holding=false,expired=false;
  std::thread writer([&]{Lock lock(manager.mutex);holding=true;entered.set_value();if(released.wait_for(std::chrono::seconds(3))!=std::future_status::ready)expired=true;holding=false;});struct Join{std::promise<void>& release;std::thread& writer;~Join(){try{release.set_value();}catch(...){}if(writer.joinable())writer.join();}} join{release,writer};
  check(ready.wait_for(std::chrono::seconds(3))==std::future_status::ready,"An independent writer holds the model before automatic discovery");
  check(!tryAutomaticPromptSnapshot(manager,false,captured,seen,reserved)&&holding&&!expired,"Busy automatic discovery returns while the writer remains held");
  check(tryAutomaticConfirmationReady(manager,{legacy,legacy->id(),true},seen)==AutomaticPromptReadiness::Busy&&holding&&!expired,"Candidate revalidation is nonblocking and distinguishes busy from obsolete");
  check(tryClaimAutomaticAuthentication(manager,auth)==AutomaticPromptReadiness::Busy&&holding&&!expired,"A busy authentication claim remains unacknowledged");
  check(!tryBrowserBatchSnapshot(manager)&&holding&&!expired&&manager.browserRecoveryRequested,"Busy batch discovery preserves the recovery signal and pending work");
  release.set_value();writer.join();check(manager.pendingOffers==std::vector<JobPtr>{legacy}&&manager.pendingBrowserBatches.size()==2&&yes(auth->data,"AuthenticationPromptPending"),"Busy reads consume neither legacy offers, browser batches nor authentication intent");
  manager.fileInfoDecisionPending=true;check(!tryAutomaticPromptSnapshot(manager,false,captured,seen,reserved)&&!tryBrowserBatchSnapshot(manager)&&tryClaimAutomaticAuthentication(manager,auth)==AutomaticPromptReadiness::Busy,"The File Info acknowledgement guard also defers automatic discovery and claims");manager.fileInfoDecisionPending=false;
  auto batch=tryBrowserBatchSnapshot(manager);check(batch&&batch->size()==1&&(*batch)[0].linkText=="Owned batch"&&(*batch)[0].observedSize==42&&(*batch)[0].headers==row.headers&&manager.pendingBrowserBatches.size()==1,"A successful batch handoff moves exactly one FIFO cohort with its request metadata");
  batch=tryBrowserBatchSnapshot(manager);auto empty=tryBrowserBatchSnapshot(manager);check(batch&&batch->size()==1&&empty&&empty->empty(),"An available empty batch queue is distinct from a busy snapshot");
  manager.stop();
 }catch(const std::exception& e){error=e.what();}
 atomicText(root/L"results.json",Json{{"passed",error.empty()},{"checks",checks},{"error",error}}.dump(2),false);std::cout<<checks.size()<<" checks; "<<(error.empty()?"passed":error)<<"\n";return error.empty()?0:1;
}
