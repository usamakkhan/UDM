#include "ScopedPublicationSource.hpp"
#include "PreparedTransferCommit.hpp"
#include "Core.hpp"
#ifdef UDM_FINISH_TIMING_TEST
#include "ProductionUiTiming.hpp"
#define UDM_FINISH_POINT(stage) nativeUiTraceRecord(stage)
#else
#define UDM_FINISH_POINT(stage) ((void)0)
#endif

#include "QueueMembership.hpp"
#include "CapturePresentation.hpp"
#include "GrabberDestinations.hpp"
#include "PublicationPreparation.hpp"
#include "PublicationOwnership.hpp"
#include "PublicationCommit.hpp"
#include "PublicationCatalogProof.hpp"
#include "ReplacementCommit.hpp"
#include "PublicationRecovery.hpp"
#include "RecoveryJournal.hpp"
#include "MediaPublication.hpp"
#include "FileIdentity.hpp"
#include "Scanner.hpp"
#include "SiteLogins.hpp"
#include <algorithm>
#include <regex>
#include <unordered_map>
#include <unordered_set>
namespace udm {
static std::string resourceKey(const std::string& value){
 Url u(value);auto query=u.query;auto hash=query.find('#');if(hash!=std::string::npos)query.resize(hash);
 return u.scheme+"://"+u.host+":"+std::to_string(u.port)+(u.path.empty()?"/":u.path)+query;
}
static Headers accountHeaders(const Headers& headers){Headers result;for(auto& [key,value]:headers)if(lower(key)=="authorization"||lower(key)=="cookie")result[lower(key)]=value;return result;}
JobPtr Manager::findDuplicate(const std::string& address,const Headers& headers,JobPtr ignore,const Json& request)const{
 Lock lock(mutex);auto key=resourceKey(address);const auto& settings=state["Settings"];
 auto account=accountHeaders(siteRequestHeaders(address,headers,settings));
 auto post=validatePostRequest(request,address);const bool isPost=!post.empty();JobPtr newest;
 for(auto it=jobs.rbegin();it!=jobs.rend();++it){
  const auto& job=*it;const auto& data=job->data;
  if(job==ignore||data.contains("OfflineProject")||hasNonemptyString(data,"SourceUrl")||
     hasNonemptyString(data,"ProtectedAdaptive")||hasNonemptyString(data,"ProtectedRequest")!=isPost)continue;
  const auto candidateAddress=str(data,"Url");
  // A saved request can be large or unavailable to the current Windows account.
  // Compare the method and URL before opening its protected fields.
  if(resourceKey(candidateAddress)!=key||(isPost&&readPostRequest(data)!=post)||
     accountHeaders(siteRequestHeaders(candidateAddress,readHeaders(data),settings))!=account)continue;
  if(isActive(job)||str(job->data,"Status")=="Queued"||str(job->data,"Status")=="Awaiting confirmation"||!str(job->data,"DuplicateOf").empty()||!str(job->data,"ReplacementOf").empty())return job;
  if(!newest)newest=job;
 }return newest;
}
JobPtr Manager::offerDownload(const std::string& address,const std::string& folder,const std::string& name,const std::string& queue,bool paused,const Headers& headers,const Json& request,const Json& browserProxy,const Json& browserSession){
 Lock lock(mutex);
 return offerDownload(address,folder,name,queue,paused,headers,request,browserProxy,browserSession,nullptr);
}
JobPtr Manager::offerDownload(const std::string& address,const std::string& folder,const std::string& name,
 const std::string& queue,bool paused,const Headers& headers,const Json& request,const Json& browserProxy,
 const Json& browserSession,const JobPtr* knownDuplicate){
 auto candidate=add(address,folder,name,queue,true,headers,"",request,browserProxy,browserSession);
 try{
  auto existing=knownDuplicate?*knownDuplicate:findDuplicate(address,readHeaders(candidate->data),candidate,request);
  auto policy=str(state["Settings"],"DuplicatePolicy","Ask");
  if(existing&&(isActive(existing)||str(existing->data,"Status")=="Queued"||str(existing->data,"Status")=="Awaiting confirmation"||!str(existing->data,"DuplicateOf").empty()||!str(existing->data,"ReplacementOf").empty()||policy=="Existing")){
   if(catalogTransaction)jobs.pop_back(); // This unpresented draft was just appended.
   else remove(candidate);
   if(policy=="Existing"&&str(existing->data,"Status")!="Awaiting confirmation"&&str(existing->data,"DuplicateOf").empty())existingOffers.insert(existing->id());return existing;}
  if(existing&&(policy=="Ask"||policy=="Replace")){candidate->data["DuplicateOf"]=existing->id();candidate->data["Status"]="Awaiting duplicate choice";if(policy=="Replace"){candidate->data["DuplicateQueue"]=!paused;return resolveDuplicate(candidate,"Replace");}}
  else candidate->data["Status"]=paused?"Paused":"Queued";
  save();return candidate;
 }catch(...){jobs.erase(std::remove(jobs.begin(),jobs.end(),candidate),jobs.end());try{save();}catch(...){}throw;}
}
JobPtr Manager::resolveDuplicate(JobPtr candidate,const std::string& choice,bool remember){
 Lock lock(mutex);auto settings=state["Settings"];
 if(remember){if(choice!="Numbered"&&choice!="Existing"&&choice!="Replace")throw std::runtime_error("Choose a duplicate policy before remembering it.");state["Settings"]["DuplicatePolicy"]=choice;}
 try{return resolveDuplicateChoice(candidate,choice);}catch(...){state["Settings"]=settings;throw;}
}
Manager::ReplacementVerification::ReplacementVerification(JobPtr owner,const fs::path& path,const Cancel& cancel):job(owner),target(path),file(CreateFileW(path.c_str(),GENERIC_READ,FILE_SHARE_READ,nullptr,OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL,nullptr)){
 if(!file)throw std::runtime_error("The saved file could not be opened for synchronization verification.");
 hash=fileHash(file.h,[&]{cancel.check();});cancel.check();
}
JobPtr Manager::resolveDuplicateChoice(JobPtr candidate,const std::string& choice,const ReplacementVerification* prepared){
 Lock lock(mutex);if(!candidate||std::find(jobs.begin(),jobs.end(),candidate)==jobs.end()||isActive(candidate)||str(candidate->data,"DuplicateOf").empty())throw std::runtime_error("This download has no pending duplicate choice.");
 JobPtr existing;for(auto job:jobs)if(job->id()==str(candidate->data,"DuplicateOf"))existing=job;
 if(choice=="Cancel"){remove(candidate);return {};}
 if(choice=="Existing"){
  if(!existing)throw std::runtime_error("The previous record was removed. Choose a numbered copy.");
  auto captures=state.value("BrowserCaptures",Json());retargetCapturePresentations(state,candidate->id(),existing->id(),"existing");
  try{remove(candidate);}catch(...){restoreCaptureRows(state,captures);throw;}
  existingOffers.insert(existing->id());return existing;
 }
 if(choice!="Numbered"&&choice!="Replace")throw std::runtime_error("Choose an existing download, numbered copy or replacement.");
 auto before=candidate->data;
 if(choice=="Replace"){
  if(!existing||isActive(existing)||(str(existing->data,"Status")!="Complete"&&str(existing->data,"Status")!="Paused"&&str(existing->data,"Status")!="Failed")||!str(existing->data,"ReplacementOf").empty())throw std::runtime_error("Pause the existing download before overwriting it.");
  if(str(existing->data,"Status")!="Complete"||!fs::exists(existing->target()))return restartDuplicate(candidate,existing);
  if(!fs::is_regular_file(existing->target()))throw std::runtime_error("The existing destination is not a file.");
  for(auto job:jobs)if(str(job->data,"ReplacementOf")==existing->id())throw std::runtime_error("A replacement for this file already exists.");
  if(fs::exists(candidate->target()))throw std::runtime_error("The backup location is now occupied. Add this link again to choose a free name.");
  if(prepared){
   if(prepared->job!=existing||prepared->target!=existing->target()||!prepared->file)throw std::runtime_error("The synchronization destination changed before replacement.");
   Handle current(CreateFileW(existing->target().c_str(),GENERIC_READ,FILE_SHARE_READ,nullptr,OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL,nullptr));BY_HANDLE_FILE_INFORMATION a{},b{};
   if(!current||!GetFileInformationByHandle(prepared->file.h,&a)||!GetFileInformationByHandle(current.h,&b)||a.dwVolumeSerialNumber!=b.dwVolumeSerialNumber||a.nFileIndexHigh!=b.nFileIndexHigh||a.nFileIndexLow!=b.nFileIndexLow)throw std::runtime_error("The synchronization destination changed before replacement.");
  }
  auto hash=prepared?prepared->hash:fileHash(existing->target());
  const auto target=existing->target();
  const auto stem=target.stem().wstring(),extension=target.extension().wstring();
  fs::path priorPath;
  for(int index=1;;++index){
   priorPath=target.parent_path()/(stem+L" (previous "+std::to_wstring(index)+L")"+extension);
   if(!fs::exists(priorPath)&&!relocationReserves(priorPath)&&!downloadDestinationReserved(priorPath))break;
  }
  candidate->data["PreviousPath"]=utf8(priorPath.wstring());candidate->data["ReplacementOf"]=existing->id();candidate->data["ReplacementHash"]=hash;
  candidate->data["Folder"]=existing->data["Folder"];candidate->data["FileName"]=existing->data["FileName"];
 }
 candidate->data.erase("DuplicateOf");candidate->data["Status"]=yes(candidate->data,"DuplicateQueue")?"Queued":"Paused";candidate->data.erase("DuplicateQueue");
 try{save();}catch(...){candidate->data=before;throw;}return candidate;
}
// Files that hold an unfinished download's bytes: one NNNN.part per range in the
// layout before 1.1, one <job-id>.data file since. Nothing else in a parts folder
// is retired by a restart.
bool partialFileName(const std::string& name){static const std::regex pattern("[0-9]{4,8}\\.part|[0-9a-f]{32}\\.data");return std::regex_match(name,pattern);}
// An overwrite of an unfinished file starts a new parts generation, then retires
// only the old generation's unchanged partial files after the catalog commit.
JobPtr Manager::restartDuplicate(JobPtr candidate,JobPtr existing){
 if(fs::exists(existing->target()))throw std::runtime_error("Another file occupies this unfinished download's destination. Choose a numbered copy or another file name.");
 for(auto job:jobs)if(str(job->data,"ReplacementOf")==existing->id())throw std::runtime_error("A pending replacement already uses this download.");
 recoverRestarts();auto before=existing->data,after=candidate->data;auto previousJobs=jobs;
 auto attempt=guid();auto temporary=str(state["Settings"],"TemporaryFolder");
 auto fresh=(temporary.empty()?root/L"parts":fs::path(wide(temporary))/L"UDM-parts")/wide(attempt);
 if(!fresh.is_absolute()||fs::exists(fresh))throw std::runtime_error("Cannot reserve a new temporary folder for the overwrite.");
 auto prior=root/L"parts"/wide(existing->id());if(!str(before,"PartsFolder").empty())prior=fs::path(wide(str(before,"PartsFolder")));
 if(!prior.is_absolute())throw std::runtime_error("The saved temporary folder is invalid.");
 Json files=Json::array();if(fs::exists(prior))for(const auto& entry:fs::directory_iterator(prior)){
  auto name=utf8(entry.path().filename().wstring());if(!partialFileName(name)||!entry.is_regular_file()||entry.is_symlink())continue;
  files.push_back({{"Name",name},{"Size",entry.file_size()},{"Stamp",entry.last_write_time().time_since_epoch().count()}});
 }
 for(const char* key:{"Id","FileName","Folder","Category","CategoryExplicit","Queue","Description","Connections","LimitKbps","ExpectedSha256","DownloadPage","Added","SuppressCompletionDialog","OpenFolderOnCompletion","CloseProgressOnCompletion"})if(before.contains(key))after[key]=before[key];
 after.erase("DuplicateOf");after["Status"]=yes(after,"DuplicateQueue")?"Queued":"Paused";after.erase("DuplicateQueue");after["PartsFolder"]=utf8(fresh.wstring());after["RestartAttempt"]=attempt;after["Restarted"]=date();
 auto journal=root/L"restarts"/(wide(attempt)+L".json");
 auto operation=Json{{"Schema",1},{"Id",existing->id()},{"Attempt",attempt},{"Parts",utf8(prior.wstring())},{"Files",files},{"Before",before},{"Committed",false}};
 atomicText(journal,operation.dump(),false);
 auto captures=state.value("BrowserCaptures",Json());retargetCapturePresentations(state,candidate->id(),existing->id(),"download");
 existing->data=after;jobs.erase(std::remove(jobs.begin(),jobs.end(),candidate),jobs.end());
 try{save();}catch(...){existing->data=before;jobs=previousJobs;restoreCaptureRows(state,captures);std::error_code ec;fs::remove(journal,ec);throw;}
 existing->workers.clear();existing->speed=0;existing->speedMeter.reset(0);existing->sessionLimit.reset();existingOffers.erase(existing->id());
 if(!catalogTransaction)discardRetainedMediaOutput(before);
 recoverRestarts();return existing;
}
void Manager::recoverRestarts(){try{
 Lock lock(mutex);if(catalogTransaction)return;auto directory=root/L"restarts";if(!fs::exists(directory))return;
 auto entries=fs::directory_iterator(directory);if(entries==fs::directory_iterator())return;
 std::unordered_map<std::string,JobPtr> jobsById;
 std::unordered_set<fs::path> occupiedParts;
 jobsById.reserve(jobs.size());occupiedParts.reserve(jobs.size());
 for(auto item:jobs){
  jobsById[item->id()]=item;
  auto selected=str(item->data,"PartsFolder");
  auto path=selected.empty()?root/L"parts"/wide(item->id()):fs::path(wide(selected));
  occupiedParts.insert(path.lexically_normal());
 }
 for(const auto& entry:entries){
  if(!entry.is_regular_file()||entry.path().extension()!=L".json")continue;
  try{
   auto operation=Json::parse(readText(entry.path()));if(num(operation,"Schema")!=1||!operation.contains("Before")||!operation["Files"].is_array())continue;
   auto id=str(operation,"Id"),attempt=str(operation,"Attempt");if(id!=str(operation["Before"],"Id")||attempt!=utf8(entry.path().stem().wstring()))continue;
   JobPtr job;auto found=jobsById.find(id);if(found!=jobsById.end())job=found->second;
   if(!yes(operation,"Committed")){
    if(!job||str(job->data,"RestartAttempt")!=attempt){fs::remove(entry.path());continue;}
    operation["Committed"]=true;atomicText(entry.path(),operation.dump(),false);
   }
   auto parts=fs::path(wide(str(operation,"Parts")));auto recorded=str(operation["Before"],"PartsFolder");
   auto expected=recorded.empty()?root/L"parts"/wide(id):fs::path(wide(recorded));
   if(!parts.is_absolute()||parts.lexically_normal()!=expected.lexically_normal()||fs::is_symlink(parts)||(GetFileAttributesW(parts.c_str())!=INVALID_FILE_ATTRIBUTES&&(GetFileAttributesW(parts.c_str())&FILE_ATTRIBUTE_REPARSE_POINT)))continue;
   if(occupiedParts.count(parts.lexically_normal()))continue;
   bool retained=false;for(const auto& file:operation["Files"]){auto name=str(file,"Name");if(!partialFileName(name)){retained=true;continue;}auto path=parts/wide(name);if(!fs::exists(path))continue;
    if(fs::is_symlink(path)||!fs::is_regular_file(path)||fs::file_size(path)!=(uintmax_t)num(file,"Size")||fs::last_write_time(path).time_since_epoch().count()!=num(file,"Stamp")){retained=true;continue;}
    std::error_code failure;fs::remove(path,failure);if(failure)retained=true;
   }
   if(!retained){std::error_code ec;if(fs::exists(parts)&&fs::is_empty(parts))fs::remove(parts,ec);fs::remove(entry.path());}
  }catch(const std::exception&){/* Retain the cleanup receipt and old files for a later retry. */}
 }
}catch(const std::exception&){/* Cleanup never prevents opening or saving the catalog. */}}

OfferPresentation Manager::presentOffer(JobPtr job){
 std::unique_lock<std::recursive_mutex> lock(mutex);if(!job||std::find(jobs.begin(),jobs.end(),job)==jobs.end())throw std::runtime_error("This download was removed.");
 if(!str(job->data,"DuplicateOf").empty())throw std::runtime_error("Choose how to handle this duplicate first.");
 if(str(job->data,"Status")=="Complete"){existingOffers.erase(job->id());return OfferPresentation::Complete;}
 if(existingOffers.count(job->id())&&!isActive(job)&&str(job->data,"Status")!="Queued"){lock.unlock();resume(job);lock.lock();if(std::find(jobs.begin(),jobs.end(),job)==jobs.end())throw std::runtime_error("This download was removed.");if(str(job->data,"Status")=="Complete"){existingOffers.erase(job->id());return OfferPresentation::Complete;}}
 existingOffers.erase(job->id());
 return isActive(job)||str(job->data,"Status")=="Queued"?OfferPresentation::Progress:OfferPresentation::Information;
}
static Json completeData(const Json& data,i64 measuredSize,const std::string& hash){auto next=data;next.erase("RetainedMediaOutput");next["Sha256"]=hash;if(str(data,"HttpContentEncoding")=="gzip"||(yes(data,"PostResponseComplete")&&yes(data,"StripContentPrefix")&&num(data,"ContentPrefixBytes")>0)){next["HttpWireSize"]=num(data,"Size",-1);next["HttpWireReceived"]=num(data,"Received");const auto length=measuredSize;next["Segments"]=Json::array();if(length>0)next["Segments"].push_back({{"Index",0},{"Start",0},{"End",length-1},{"Done",length}});next.erase("CompletedPrefetch");next["HttpDecodedSize"]=length;next["HttpDecodedReceived"]=length;}next["Size"]=measuredSize;next["Received"]=next["Size"];if(yes(data,"PostResponseComplete")){next.erase("CompletedPrefetch");next.erase("PostResponseComplete");next.erase("PostResponseRequestSha256");next.erase("PostResponseRetentionWarning");}next["Status"]="Complete";next["MediaPhase"]="Complete";next["MediaTracksReady"]=false;next["SabrRetainedBytes"]=0;next["Finished"]=date();next["Error"]="";next["QueueMember"]=false;if(yes(next,"SynchronizationRestoration")){next["SyncStatus"]="Restored missing file";next["LastSync"]=next["Finished"];next["SyncRetryFailed"]=false;next.erase("SynchronizationRestoration");}return next;}
void Manager::publishFile(JobPtr job,const fs::path& staging,const std::string& knownHash,const Cancel* cancel,const LivePublicationDetails* live){
 auto finalCommit=PreparedTransferCommit::publishing(*this,job,cancel);
 fs::path target;std::string originalId,replacementHash,previousPath;bool registered=false,catalogBound=false;Json destinationInputs;std::function<void(const char*)> publicationObservation;std::unique_ptr<PublicationOwnership> publicationOwner;JobPtr leasedOriginal;
 {Lock lock(mutex);if(finalCommit){const auto worker=job?active.find(job->id()):active.end();if(worker==active.end()||worker->second.get()!=cancel)throw std::runtime_error("The prepared transfer lost its worker ownership.");}if(stopping)throw std::runtime_error("UDM is closing.");if(catalogTransaction)throw std::runtime_error("Finish the catalog batch before publishing this file.");if(!job)throw std::runtime_error("The download was removed before publication.");registered=std::find(jobs.begin(),jobs.end(),job)!=jobs.end();target=job->target();originalId=str(job->data,"ReplacementOf");replacementHash=str(job->data,"ReplacementHash");previousPath=str(job->data,"PreviousPath");destinationInputs=publicationDestinationInputs(job->data);publicationObservation=catalogCheckpoint;
  {auto pending=jobs;std::unordered_set<const Job*> seen;while(!pending.empty()){auto item=pending.back();pending.pop_back();if(!item||!seen.insert(item.get()).second)continue;if(item==job){catalogBound=true;break;}if(item->video)pending.push_back(item->video);if(item->audio)pending.push_back(item->audio);}}
  if(!originalId.empty()){
   for(const auto& item:jobs)if(item->id()==originalId)leasedOriginal=item;
   if(!leasedOriginal||leasedOriginal==job||isActive(leasedOriginal))throw std::runtime_error("The original download changed. Its file was not replaced.");
  }
 }
 if(!originalId.empty()){
  if(cancel)cancel->check();{Lock lock(mutex);requirePublicationRecovered(*this,job);}
  // Predecessor recovery must not inherit this new publication's model lease.
  recoverReplacements(leasedOriginal,true);
 }
 {Lock lock(mutex);
  if(stopping||catalogTransaction||(std::find(jobs.begin(),jobs.end(),job)!=jobs.end())!=registered||job->target()!=target||str(job->data,"ReplacementOf")!=originalId||str(job->data,"ReplacementHash")!=replacementHash||str(job->data,"PreviousPath")!=previousPath||publicationDestinationInputs(job->data)!=destinationInputs)throw std::runtime_error("The download changed while preparing publication. Its files were preserved.");
  std::vector<JobPtr> owned{job};std::vector<fs::path> destinations{target,root/L"publications"/(wide(job->id())+L".json"),root/L"replacements"/(wide(job->id())+L".json")};
  if(!originalId.empty()){
   if(std::find(jobs.begin(),jobs.end(),leasedOriginal)==jobs.end()||isActive(leasedOriginal)||str(leasedOriginal->data,"Status")!="Complete"||leasedOriginal->target().lexically_normal()!=target.lexically_normal())throw std::runtime_error("Recovery changed the original download. Review the destination before replacing it.");
   owned.push_back(leasedOriginal);destinations.push_back(leasedOriginal->target());destinations.emplace_back(wide(previousPath));
  }
  publicationOwner=std::make_unique<PublicationOwnership>(*this,std::move(owned),destinations);
 }
 auto timingMark=std::chrono::steady_clock::now();Json publicationTimings=Json::object();
 const auto timing=[&](const char* name)noexcept{const auto now=std::chrono::steady_clock::now();try{publicationTimings[name]=std::chrono::duration<double>(now-timingMark).count();}catch(...){}timingMark=now;};
 // Only Manager-owned workers may dirty diagnostics after publication: they
 // still own their final durable checkpoint. Standalone publication stays saved.
 struct RecordPublicationTiming {std::function<void()> record;~RecordPublicationTiming(){record();}} recordTiming{[&]()noexcept{try{timing("ReleasePins");Lock guard(mutex);const auto owner=active.find(job->id());if(cancel&&owner!=active.end()&&owner->second.get()==cancel)if(finalCommit)finalCommit->publicationTimings=publicationTimings;else job->data["PublicationPhases"]=publicationTimings;}catch(...){}}};
 auto checkpoint=[&]{if(cancel)cancel->check();};checkpoint();
 if(publicationObservation)publicationObservation("publication-preflight-begin");
 auto bodyPin=ScopedPublicationSource::take(*this,job,staging);const bool suppliedSource=bool(bodyPin);
 auto hash=knownHash;
 const bool deferredAdvisory=registered&&finalCommit&&finalCommit->pendingAdvisoryHash();
 const bool overlap=!suppliedSource&&finalCommit&&finalCommit->pendingWriteback()&&finalCommit->hasWritebackPin()&&originalId.empty()&&(!hash.empty()||deferredAdvisory);
 // Private/ephemeral publications retain their digest-bound receipt format.
 // Only a registered ordinary job can use an identity-bound early intent.
 if(finalCommit&&finalCommit->pendingAdvisoryHash()&&!overlap)hash=finalCommit->finishAdvisoryHash();
 if(overlap)bodyPin=finalCommit->takeWritebackPin();
 else if(finalCommit){finalCommit->finishWriteback();finalCommit->releaseWritebackPin();}
 PublicationPreparation prepared(staging,destinationInputs,std::move(bodyPin));
 if(publicationObservation)publicationObservation("publication-preflight-ready");timing("PrepareSource");checkpoint();
 Json stagedIdentity;if(overlap||suppliedSource)stagedIdentity=prepared.sourceIdentity();
 if(hash.empty()){
  if(originalId.empty()){
   stagedIdentity=prepared.sourceIdentity();
   if(stagedIdentity.is_null())hash=prepared.sourceHash(checkpoint);
  }else hash=fileHash(staging,checkpoint);
 }
 std::unique_ptr<Handle> protectOriginal;i64 originalSize=0;
 auto matchesOriginal=[&]{
  if(!protectOriginal||!(*protectOriginal))return false;
  Handle current(CreateFileW(target.c_str(),GENERIC_READ,FILE_SHARE_READ|FILE_SHARE_DELETE,nullptr,OPEN_EXISTING,0,nullptr));BY_HANDLE_FILE_INFORMATION a{},b{};
  return current&&GetFileInformationByHandle(protectOriginal->h,&a)&&GetFileInformationByHandle(current.h,&b)&&!(b.dwFileAttributes&FILE_ATTRIBUTE_DIRECTORY)&&a.dwVolumeSerialNumber==b.dwVolumeSerialNumber&&a.nFileIndexHigh==b.nFileIndexHigh&&a.nFileIndexLow==b.nFileIndexLow;
 };
 if(!originalId.empty()){
  protectOriginal=std::make_unique<Handle>(CreateFileW(target.c_str(),GENERIC_READ,FILE_SHARE_READ|FILE_SHARE_DELETE,nullptr,OPEN_EXISTING,0,nullptr));
  if(!(*protectOriginal)||!fs::is_regular_file(target)||fileHash(protectOriginal->h,checkpoint)!=replacementHash||!matchesOriginal())throw std::runtime_error("The original file changed on disk. Its contents were not replaced.");
  LARGE_INTEGER length{};if(!GetFileSizeEx(protectOriginal->h,&length)||length.QuadPart<0)throw std::runtime_error("Cannot inspect the original file size.");originalSize=length.QuadPart;
 }
 timing("VerifyIdentity");checkpoint();if(!overlap)prepared.validateSourceName(staging);
 // The owned source stays writer-excluded. Its name and captured identity are
 // checked after the body join; querying that file during flush serializes IO.
 // Pinned filesystem handles protect this check; the model lock cannot make
 // filesystem metadata atomic. Revalidate model inputs only after this IO.
 prepared.validateDirectories();
 if(publicationObservation)publicationObservation("publication-directories-validated");
 std::unique_lock<std::recursive_mutex> lock(mutex);checkpoint();
 // Internal audio/video track jobs are deliberately absent from the catalog.
 // Preserve that membership state, rather than rejecting all child publication.
 if((std::find(jobs.begin(),jobs.end(),job)!=jobs.end())!=registered||job->target()!=target||str(job->data,"ReplacementOf")!=originalId||str(job->data,"ReplacementHash")!=replacementHash||str(job->data,"PreviousPath")!=previousPath||publicationDestinationInputs(job->data)!=destinationInputs)throw std::runtime_error("The download changed before publication. Its file was not replaced.");
 publicationOwner->beginCommit();
 auto before=job->data,after=completeData(before,prepared.size,hash);after["QueueMember"]=retainCompletedMembership(*this,before);
 if(registered)prepareCompletionScan(after,state["Settings"]);
 if(live){
  after["LiveSaveReady"]=false;
  after["MergeSeconds"]=live->mergeSeconds;
  after["ElapsedSeconds"]=live->elapsedSeconds;
  if(live->subtitleCues){
   after["SubtitleCueCount"]=*live->subtitleCues;
   if(*live->subtitleCues==0)after["MediaPhase"]="Complete (no captions in recorded interval)";
  }else after.erase("SubtitleCueCount");
 }
 if(originalId.empty()){
  const auto commitId=guid();after["PublicationCommitId"]=commitId;
  auto journal=root/L"publications"/(wide(job->id())+L".json");
  Json intent={{"Id",job->id()},{"Target",utf8(target.wstring())},{"Staging",utf8(staging.wstring())},{"Hash",hash},{"Before",before},{"After",after},{"CommitId",commitId},{"Ephemeral",!catalogBound}};
  if(!stagedIdentity.is_null())intent["StagedIdentity"]=stagedIdentity;
  if(overlap)intent["FlushBeforePublish"]=true; // Recovery must never accept an unflushed early intent.
  lock.unlock();
  if(fs::exists(journal))throw std::runtime_error("A previous publication needs recovery. Resume the download or restart UDM.");
  if(fs::exists(target))throw std::runtime_error("Cannot publish the download. The destination already exists; choose another file name.");
  auto intentMark=std::chrono::steady_clock::now();Json intentOperations=Json::object();
  const auto observeIntent=[&](const char* stage){const auto now=std::chrono::steady_clock::now();try{intentOperations[stage]=std::chrono::duration<double>(now-intentMark).count();}catch(...){}intentMark=now;if(publicationObservation){const auto name=std::string("publication-intent-")+stage;publicationObservation(name.c_str());}};
  timing("PrepareIntent");UDM_FINISH_POINT("intent-write-begin");intentMark=std::chrono::steady_clock::now();atomicText(journal,intent.dump(),false,observeIntent);try{intentOperations["publish"]=std::chrono::duration<double>(std::chrono::steady_clock::now()-intentMark).count();publicationTimings["IntentOperations"]=std::move(intentOperations);}catch(...){}timing("WriteIntent");UDM_FINISH_POINT("intent-write-end");
  if(publicationObservation)publicationObservation("publication-intent-written");
  if(overlap){finalCommit->finishWriteback();checkpoint();timing("JoinWriteback");if(publicationObservation)publicationObservation("publication-writeback-protected");}
  if(deferredAdvisory&&overlap){
   // The durable intent deliberately retains its empty advisory digest and
   // captured source identity. Recovery hashes that exact file independently.
   // The final catalog may include the newly joined advisory digest.
   hash=finalCommit->finishAdvisoryHash();checkpoint();after["Sha256"]=hash;timing("JoinAdvisoryHash");
   if(publicationObservation)publicationObservation("publication-advisory-joined");
  }
  prepared.validateSourceName(staging);prepared.validateDirectories();
  if(overlap){Lock guard(mutex);checkpoint();if(stopping||(std::find(jobs.begin(),jobs.end(),job)!=jobs.end())!=registered||job->target()!=target||publicationDestinationInputs(job->data)!=destinationInputs)throw std::runtime_error("The download changed during body writeback. Keep its file and recovery record.");}
  if(!MoveFileExW(staging.c_str(),target.c_str(),MOVEFILE_WRITE_THROUGH))throw std::runtime_error("Cannot publish the download. The file and publication recovery record were preserved.");
  prepared.pinPublishedName(target);
  timing("Rename");if(publicationObservation)publicationObservation("publication-file-moved");
  lock.lock();
  if((std::find(jobs.begin(),jobs.end(),job)!=jobs.end())!=registered||job->target()!=target||str(job->data,"ReplacementOf")!=originalId||str(job->data,"ReplacementHash")!=replacementHash||str(job->data,"PreviousPath")!=previousPath||publicationDestinationInputs(job->data)!=destinationInputs)throw std::runtime_error("The download changed during publication. Keep its file and recovery record.");
  if(catalogTransaction)throw std::runtime_error("Finish the catalog batch before recording this published file. Its recovery journal was preserved.");
  auto current=job->data;PublicationCommit::validateCurrent(intent,current);auto completed=PublicationCommit::apply(before,after,current);completed["QueueMember"]=retainCompletedMembership(*this,current);if(registered)prepareCompletionScan(completed,state["Settings"]);
  if(finalCommit){completed["PublicationPhases"]=publicationTimings;completed["PublicationPhases"]["TimingScope"]="through-rename-before-final-catalog-save";}
  PublicationCatalogProof::Lease catalogProof;Json receipt;bool insertedReceipt=false;job->data=completed;try{if(!catalogBound){receipt=PublicationCatalogProof::binding(completed);insertedReceipt=PublicationCatalogProof::installReceipt(*this,receipt);}catalogProof=PublicationCatalogProof::prepare(*this,completed,!catalogBound);}catch(...){job->data=std::move(current);if(insertedReceipt)PublicationCatalogProof::eraseReceipt(*this,receipt);throw;}
  timing("PrepareCatalog");UDM_FINISH_POINT("catalog-write-begin");lock.unlock();std::exception_ptr failure;try{if(publicationObservation)publicationObservation("publication-catalog-prepared");if(finalCommit&&publicationObservation){const std::function<void(const char*)> observeFinal=[&](const char* point){if(publicationObservation){if(std::string_view(point)=="snapshot-prepared")publicationObservation("completed-transfer-checkpoint");publicationObservation(point);}};saveCatalog(false,&observeFinal);}else save();}catch(...){failure=std::current_exception();}timing("WriteCatalog");UDM_FINISH_POINT("catalog-write-end");const bool accepted=catalogProof.resolve();UDM_FINISH_POINT("catalog-resolve-end");lock.lock();
  if(!accepted){if(str(job->data,"PublicationCommitId")==commitId)job->data=PublicationCommit::rollback(current,completed,job->data);catalogProof.release();if(insertedReceipt)PublicationCatalogProof::eraseReceipt(*this,receipt);if(failure)std::rethrow_exception(failure);throw std::runtime_error("The completed file has not been durably recorded. Its recovery journal was preserved.");}
  if((std::find(jobs.begin(),jobs.end(),job)!=jobs.end())!=registered||job->target()!=target||!PublicationCommit::accepted(intent,job->data,hash))throw std::runtime_error("The completed download changed after catalog publication. Its recovery journal was preserved.");
  timing("AcceptCatalog");lock.unlock();prepared.releaseAcceptedFile();timing("ReleaseAcceptedFile");if(publicationObservation)publicationObservation("publication-cleanup");
  std::error_code ec;const bool removed=fs::remove(journal,ec);if(removed&&publicationObservation)publicationObservation("publication-journal-removed");if(!catalogBound&&removed){lock.lock();catalogProof.release();PublicationCatalogProof::eraseReceipt(*this,receipt);lock.unlock();}timing("RemoveJournal");UDM_FINISH_POINT("publication-cleanup-end");return;
 }
 auto journal=root/L"replacements"/(wide(job->id())+L".json");JobPtr original;for(auto item:jobs)if(item->id()==originalId)original=item;
 auto previous=fs::path(wide(str(before,"PreviousPath")));
 if(!original||original!=leasedOriginal||!publicationOwner->owns(original)||active.count(original->id())||str(original->data,"Status")!="Complete"||original->target().lexically_normal()!=target.lexically_normal())throw std::runtime_error("The original download changed. Its file was not replaced.");
 if(relocationReserves(previous))throw std::runtime_error("The previous-version location is reserved by a file move.");
 if(downloadDestinationReserved(previous,job))throw std::runtime_error("The previous-version location belongs to another download.");
 auto oldBefore=original->data,oldAfter=oldBefore;const auto originalInputs=publicationDestinationInputs(oldBefore);oldAfter["Folder"]=utf8(previous.parent_path().wstring());oldAfter["FileName"]=utf8(previous.filename().wstring());oldAfter["PreviousVersionOf"]=job->id();oldAfter["QueueMember"]=false;oldAfter["SyncPending"]=false;oldAfter["Sha256"]=replacementHash;oldAfter["Size"]=originalSize;oldAfter["Received"]=originalSize;oldAfter["Finished"]=str(oldBefore,"Finished");if(str(oldBefore,"Sha256")!=replacementHash)oldAfter.erase("ScanResult");
 if(yes(before,"SynchronizationReplacement")){after["SyncStatus"]="Updated; previous version retained";after["LastSync"]=after["Finished"];after.erase("SynchronizationReplacement");oldAfter["SyncStatus"]="Previous version";oldAfter["LastSync"]=after["Finished"];}
 const auto commitId=guid();after["PublicationCommitId"]=commitId;oldAfter["PublicationCommitId"]=commitId;after["ReplacedDownload"]=originalId;for(const char* key:{"ReplacementOf","ReplacementHash","PreviousPath"})after.erase(key);
 auto operation=Json{{"Id",job->id()},{"OriginalId",originalId},{"Target",utf8(target.wstring())},{"Previous",utf8(previous.wstring())},{"Staging",utf8(fs::absolute(staging).wstring())},{"OldHash",replacementHash},{"NewHash",hash},{"Before",before},{"After",after},{"OriginalBefore",oldBefore},{"OriginalAfter",oldAfter},{"CommitId",commitId},{"Ephemeral",!catalogBound}};ReplacementCommit::identity(operation);
 lock.unlock();
 if(fs::exists(journal))throw std::runtime_error("A previous replacement needs recovery. Restart UDM before retrying.");
 if(!matchesOriginal())throw std::runtime_error("The original file changed on disk. Its contents were not replaced.");
 if(fs::exists(previous))throw std::runtime_error("The previous-version location is occupied. No file was replaced.");
 const auto observeIntent=[&](const char* stage){if(publicationObservation){const auto name=std::string("replacement-intent-")+stage;publicationObservation(name.c_str());}};
 atomicText(journal,operation.dump(),false,observeIntent);if(publicationObservation)publicationObservation("replacement-intent-written");
 prepared.validateSourceName(staging);prepared.validateDirectories();if(!matchesOriginal())throw std::runtime_error("The original file changed before replacement. Keep the files and recovery journal.");
 prepared.releaseSource(); // ReplaceFile opens its replacement with write access.
 if(!ReplaceFileW(target.c_str(),staging.c_str(),previous.c_str(),0,nullptr,nullptr)){
  auto failure=GetLastError();if(matchesOriginal()&&!fs::exists(previous)){std::error_code ec;fs::remove(journal,ec);}throw std::runtime_error("Windows could not replace the file (error "+std::to_string(failure)+"). The original and recovery information were preserved.");
 }
 protectOriginal.reset();lock.lock();
 // The swap is irreversible here. Keep the live original bound to its old bytes
 // even when a later batch check or catalog write cannot complete the pair.
 if(std::find(jobs.begin(),jobs.end(),original)==jobs.end()||original->target()!=target||publicationDestinationInputs(original->data)!=originalInputs)throw std::runtime_error("The original history changed during replacement. Keep both versions and the recovery journal.");
 ReplacementCommit::validateCurrent(operation,job->data,original->data);original->data=ReplacementCommit::applyOriginal(oldBefore,oldAfter,original->data);lock.unlock();
 if(publicationObservation)publicationObservation("replacement-file-moved");lock.lock();
 if((std::find(jobs.begin(),jobs.end(),job)!=jobs.end())!=registered||std::find(jobs.begin(),jobs.end(),original)==jobs.end()||job->target()!=target||original->target()!=previous||str(job->data,"ReplacementOf")!=originalId||str(job->data,"ReplacementHash")!=replacementHash||str(job->data,"PreviousPath")!=previousPath||publicationDestinationInputs(job->data)!=destinationInputs||publicationDestinationInputs(original->data)!=publicationDestinationInputs(oldAfter))throw std::runtime_error("A replacement record changed during publication. Keep both files and the recovery journal.");
 if(catalogTransaction)throw std::runtime_error("Finish the catalog batch before recording this replacement. Both files and its journal were preserved.");
 auto current=job->data,currentOriginal=original->data;ReplacementCommit::validateCurrent(operation,current,currentOriginal);auto completed=PublicationCommit::apply(before,after,current),completedOriginal=currentOriginal;completed["QueueMember"]=retainCompletedMembership(*this,current);if(registered)prepareCompletionScan(completed,state["Settings"]);
 PublicationCatalogProof::Lease proof;Json receipt;bool inserted=false;job->data=completed;original->data=completedOriginal;try{if(!catalogBound){receipt=PublicationCatalogProof::binding(completed);inserted=PublicationCatalogProof::installReceipt(*this,receipt);}proof=PublicationCatalogProof::prepare(*this,completed,!catalogBound,&completedOriginal);}catch(...){job->data=std::move(current);original->data=std::move(currentOriginal);if(inserted)PublicationCatalogProof::eraseReceipt(*this,receipt);throw;}
 lock.unlock();std::exception_ptr failure;try{if(publicationObservation)publicationObservation("replacement-catalog-prepared");save();}catch(...){failure=std::current_exception();}const bool accepted=proof.resolve();lock.lock();
 if(!accepted){if(str(job->data,"PublicationCommitId")==commitId)job->data=PublicationCommit::rollback(current,completed,job->data);/* The old bytes already moved: keep their live history bound to the actual previous-version location. */proof.release();if(inserted)PublicationCatalogProof::eraseReceipt(*this,receipt);if(failure){try{std::rethrow_exception(failure);}catch(const std::exception& e){throw std::runtime_error(std::string("Saving replacement history failed: ")+e.what()+" Both file versions and the journal were preserved. Use Resume or restart UDM to reconcile history.");}}throw std::runtime_error("The replacement has not been durably recorded. Both file versions and its recovery journal were preserved.");}
 if((std::find(jobs.begin(),jobs.end(),job)!=jobs.end())!=registered||std::find(jobs.begin(),jobs.end(),original)==jobs.end()||!ReplacementCommit::accepted(operation,job->data,original->data))throw std::runtime_error("A replacement record changed after catalog publication. Keep both files and its recovery journal.");
 lock.unlock();if(publicationObservation)publicationObservation("replacement-cleanup");std::error_code ec;const bool removed=fs::remove(journal,ec);if(removed&&publicationObservation)publicationObservation("replacement-journal-removed");if(!catalogBound&&removed){lock.lock();proof.release();PublicationCatalogProof::eraseReceipt(*this,receipt);lock.unlock();}

}
static bool recoverableStagingPath(const Json& operation,const Json& data,const fs::path& staging,const fs::path& target){
 const auto& before=operation.at("Before");
 if(yes(before,"ConfirmationPending")||fs::exists(target))return false;
 const auto stagedPath=staging.lexically_normal();
 if(stagedPath.parent_path()==target.lexically_normal().parent_path()&&
    stagedPath.filename()==L".udm-"+wide(str(data,"Id"))+L".assembling")return true;
 const auto source=retainedMediaOutputPath(before);
 const auto receipt=before.find("RetainedMediaOutput");
 if(source.empty()||receipt==before.end()||data.value("RetainedMediaOutput",Json())!=*receipt||
    str(*receipt,"Sha256")!=str(operation,"Hash")||num(*receipt,"Size",-1)!=num(operation.at("After"),"Size",-1))return false;
 if(stagedPath==source.lexically_normal())return true;
 return stagedPath.parent_path()==target.lexically_normal().parent_path()&&
        std::regex_match(utf8(stagedPath.filename().wstring()),std::regex("\\.udm-"+str(data,"Id")+"-[0-9a-f]{32}\\.publishing"));
}
struct RecoveredPublicationBytes {
 std::unique_ptr<Handle> file,name;std::string digest;i64 size=-1;
 RecoveredPublicationBytes(Handle&& owned,std::string hash,i64 length=-1):file(std::make_unique<Handle>(std::move(owned))),digest(std::move(hash)),size(length){}
 void release(){name.reset();file.reset();}
};
static void requireRecoveredBytes(const Json& row,const RecoveredPublicationBytes& measured,const fs::path& path){
 const auto recorded=fs::path(wide(str(row,"Folder")))/wide(str(row,"FileName"));
 if(measured.size<0||str(row,"Status")!="Complete"||!PublicationCatalogProof::completeByteCounts(row)||num(row,"Size",-1)!=measured.size||str(row,"Sha256")!=measured.digest||!recorded.is_absolute()||safeName(str(row,"FileName"))!=str(row,"FileName")||!windowsPath::equal(windowsPath::key(recorded),windowsPath::key(path)))throw std::runtime_error("Replacement recovery metadata does not match the verified file. Keep both files and the recovery journal.");
}
static Handle pinRecoveredDestination(const Handle& measured,const fs::path& destination,bool measuredWritable=false){
 Handle pinned(CreateFileW(destination.c_str(),GENERIC_READ,FILE_SHARE_READ|(measuredWritable?FILE_SHARE_WRITE:0),nullptr,OPEN_EXISTING,FILE_FLAG_OPEN_REPARSE_POINT,nullptr));BY_HANDLE_FILE_INFORMATION before{},after{};
 if(!pinned||!GetFileInformationByHandle(measured.h,&before)||!GetFileInformationByHandle(pinned.h,&after)||(after.dwFileAttributes&(FILE_ATTRIBUTE_DIRECTORY|FILE_ATTRIBUTE_REPARSE_POINT))||before.dwVolumeSerialNumber!=after.dwVolumeSerialNumber||before.nFileIndexHigh!=after.nFileIndexHigh||before.nFileIndexLow!=after.nFileIndexLow)
  throw std::runtime_error("The recovered destination changed during publication. Keep its file and recovery journal.");
 return pinned;
}
static std::optional<RecoveredPublicationBytes> recoverReplacementBytes(const fs::path& path,const std::string& expected,bool restoring=false,const std::function<void(const char*)>& checkpoint={}){
 Handle file(CreateFileW(path.c_str(),GENERIC_READ,FILE_SHARE_READ|(restoring?FILE_SHARE_DELETE:0),nullptr,OPEN_EXISTING,FILE_FLAG_OPEN_REPARSE_POINT|FILE_FLAG_SEQUENTIAL_SCAN,nullptr));BY_HANDLE_FILE_INFORMATION info{};
 if(!file||!GetFileInformationByHandle(file.h,&info)||(info.dwFileAttributes&(FILE_ATTRIBUTE_DIRECTORY|FILE_ATTRIBUTE_REPARSE_POINT)))return {};
 LARGE_INTEGER length{};if(!GetFileSizeEx(file.h,&length)||length.QuadPart<0)return {};
 if(checkpoint)checkpoint("recovery-file-opened");
 auto digest=fileHash(file.h,[&]{if(checkpoint)checkpoint("recovery-hash-read");});if(checkpoint)checkpoint("recovery-hash-complete");if(!expected.empty()&&digest!=expected)return {};
 return RecoveredPublicationBytes(std::move(file),std::move(digest),length.QuadPart);
}
static std::optional<RecoveredPublicationBytes> recoverPublicationBytes(const Json& operation,const fs::path& path,const fs::path* destination=nullptr,const std::function<void(const char*)>& observer={}){
 const bool flushRequired=operation.value("FlushBeforePublish",false);
 const auto rejected=[&]()->std::optional<RecoveredPublicationBytes>{if(flushRequired)throw std::runtime_error("The early publication body cannot be verified or published safely. Keep its files and recovery journal; do not reuse unflushed completion accounting.");return {};};
 Handle file(CreateFileW(path.c_str(),GENERIC_READ|(flushRequired?GENERIC_WRITE:0),FILE_SHARE_READ|(destination?FILE_SHARE_DELETE:0),nullptr,OPEN_EXISTING,
                         FILE_FLAG_OPEN_REPARSE_POINT|FILE_FLAG_SEQUENTIAL_SCAN,nullptr));
 if(!file){if(flushRequired)throw std::runtime_error("The preserved publication body cannot be opened for required writeback. Keep its file and recovery journal.");return rejected();}
 BY_HANDLE_FILE_INFORMATION information{};LARGE_INTEGER size{};
 if(!GetFileInformationByHandle(file.h,&information)||(information.dwFileAttributes&(FILE_ATTRIBUTE_DIRECTORY|FILE_ATTRIBUTE_REPARSE_POINT))||
    !GetFileSizeEx(file.h,&size)||size.QuadPart!=num(operation.at("After"),"Size",-1))return rejected();
 const auto expected=str(operation,"Hash");
 if((expected.empty()||operation.contains("StagedIdentity"))&&stableFileIdentity(file.h)!=operation.at("StagedIdentity"))return rejected();
 if(observer)observer("recovery-file-opened");
 auto digest=fileHash(file.h,[&]{if(observer)observer("recovery-hash-read");});if(observer)observer("recovery-hash-complete");
 if(!expected.empty()&&digest!=expected)return rejected();
 if(flushRequired){if(observer)observer("recovery-body-flush-begin");if(!FlushFileBuffers(file.h))throw std::runtime_error("The preserved publication body could not be flushed. Keep its file and recovery journal.");if(observer)observer("recovery-body-flush-end");}
 // Never replace a file that appeared after the interrupted publication.
 if(destination){
  if(observer)observer("recovery-before-move");
  if(!MoveFileExW(path.c_str(),destination->c_str(),MOVEFILE_WRITE_THROUGH))return rejected();
  // The staging name allowed deletion for rename. Pin the resulting name and
  // require the exact measured object before it can enter the saved catalog.
  auto pinned=pinRecoveredDestination(file,*destination,flushRequired);
  if(flushRequired){RecoveredPublicationBytes result(std::move(file),std::move(digest),size.QuadPart);result.name=std::make_unique<Handle>(std::move(pinned));return result;}
  return RecoveredPublicationBytes(std::move(pinned),std::move(digest),size.QuadPart);
 }
 return RecoveredPublicationBytes(std::move(file),std::move(digest),size.QuadPart);
}
static void finishRetainedMediaRecovery(const Json& before,const fs::path& target){
 const auto source=retainedMediaOutputPath(before);
 if(source.empty())return;
 markZone(target);
 const auto from=source.lexically_normal().wstring(),to=target.lexically_normal().wstring();
 if(CompareStringOrdinal(from.c_str(),-1,to.c_str(),-1,TRUE)!=CSTR_EQUAL)discardRetainedMediaOutput(before);
}
void Manager::recoverReplacements(JobPtr scope,bool requireCatalogMember){recoverReplacementsWithContinuation(scope,requireCatalogMember,{},{});}
void Manager::recoverReplacementsWithContinuation(JobPtr scope,bool requireCatalogMember,const std::function<bool()>& before,const std::function<void()>& after){
 std::unique_lock<std::recursive_mutex> recoveryLock(mutex);
 if(stopping)throw std::runtime_error("UDM is closing.");
 if(before&&!before())return;
 if(requireCatalogMember&&(!scope||std::find(jobs.begin(),jobs.end(),scope)==jobs.end()))throw std::runtime_error("This download is no longer in the list.");
 const auto reconcile=[&]{
 auto prepareRecoveredScan=[&](Json& recovered,const Json& current){
  const auto result=current.find("ScanResult");
  // A journal can survive its completed catalog commit. Keep any scan
  // already recorded for that same completion.
  if(str(current,"Status")=="Complete"&&str(current,"Finished")==str(recovered,"Finished")&&
     (str(current,"Sha256").empty()||str(current,"Sha256")==str(recovered,"Sha256"))&&
     result!=current.end())recovered["ScanResult"]=*result;
  else{
   const auto saved=recovered.find("ScanResult");
   if(saved==recovered.end()||str(*saved,"Status")!="Pending")prepareCompletionScan(recovered,state["Settings"]);
  }
  recovered["CompletionActionArmed"]=false;
 };
 // Resume reconciles only its own publication tree. Startup passes no scope.
 std::unordered_set<std::string> scopeIds;
 if(scope){
  std::vector<JobPtr> pending{scope};std::unordered_set<const Job*> seen;
  while(!pending.empty()){auto job=pending.back();pending.pop_back();if(!job||!seen.insert(job.get()).second)continue;if(isActive(job))throw std::runtime_error("Wait for the download to stop before recovering its saved files.");scopeIds.insert(job->id());if(job->video)pending.push_back(job->video);if(job->audio)pending.push_back(job->audio);}
 }
 // Discovery must remain enrolled even before an operation/job is known.
 auto discoveryLifetime=PublicationOwnership::discovery(*this);
 std::unordered_map<std::string,JobPtr> jobsById;
 auto findJob=[&](const std::string& id)->JobPtr{
  if(jobsById.empty()){
   jobsById.reserve(jobs.size());
   for(auto item:jobs)jobsById[item->id()]=item;
  }
  auto found=jobsById.find(id);return found==jobsById.end()?JobPtr{}:found->second;
 };
 // Track records live inside their parent, although each track publishes a file.
 // Replacement history below still uses the top-level catalog lookup.
 std::unordered_map<std::string,JobPtr> publicationJobs;
 bool publicationIndexReady=false;
 auto findPublicationJob=[&](const std::string& id)->JobPtr{
  if(!publicationIndexReady){
   auto pending=jobs;if(scope)pending.push_back(scope);std::unordered_set<const Job*> seen;
   while(!pending.empty()){
    auto job=pending.back();pending.pop_back();if(!job||!seen.insert(job.get()).second)continue;
    auto inserted=publicationJobs.emplace(job->id(),job);
    if(!inserted.second&&inserted.first->second!=job)throw std::runtime_error("Publication recovery found duplicate download identifiers. Keep the saved catalog and recovery records.");
    if(job->video)pending.push_back(job->video);if(job->audio)pending.push_back(job->audio);
   }
   publicationIndexReady=true;
  }
  auto found=publicationJobs.find(id);return found==publicationJobs.end()?JobPtr{}:found->second;
 };
 // Every unlocked checkpoint retains its affected job tree, destination paths,
 // journal and verified bytes. Verification IO and catalog checkpoints leave
 // this lease separately. Discovery has a lifetime lease and scoped guards;
 // accepted cleanup releases verified handles; receipt retirement has its own
 // immutable inventory, path lease and snapshot-bound absence proof.
 auto modelJob=[&](const std::string& id)->JobPtr{auto pending=jobs;std::unordered_set<const Job*> seen;JobPtr result;while(!pending.empty()){auto item=pending.back();pending.pop_back();if(!item||!seen.insert(item.get()).second)continue;if(item->id()==id){if(result&&result!=item)throw std::runtime_error("Recovery found ambiguous catalog membership.");result=item;}if(item->video)pending.push_back(item->video);if(item->audio)pending.push_back(item->audio);}return result;};
 auto recoveryPaths=[&](const std::vector<JobPtr>& owners,std::vector<fs::path> paths){auto pending=owners;std::unordered_set<const Job*> seen;while(!pending.empty()){auto item=pending.back();pending.pop_back();if(!item||!seen.insert(item.get()).second)continue;paths.push_back(item->target());auto previous=str(item->data,"PreviousPath");if(!previous.empty())paths.emplace_back(wide(previous));if(item->video)pending.push_back(item->video);if(item->audio)pending.push_back(item->audio);}paths.erase(std::remove_if(paths.begin(),paths.end(),[](const fs::path& path){return path.empty();}),paths.end());return paths;};
 // File IO consumes only immutable copies; no live JSON iterator/reference
 // survives its unlocked interval. Reservations and held journals remain live.
 const auto ioInputs=[](const Json& data){Json inputs=Json::object();for(const char* key:{"Id","Folder","FileName","ProjectDestinationRoot","PublicationCommitId","ReplacementOf","ReplacementHash","PreviousPath","Status","Size","Received","Sha256","Finished","ExpectedSha256","Error","OriginalResponse","ConfirmationPending","RetainedMediaOutput","PartsFolder","Segments","Url","SourceUrl","ProtectedAdaptive","ProtectedRequest","HttpContentEncoding","PostResponseComplete","StripContentPrefix","ContentPrefixBytes"})if(data.contains(key))inputs[key]=data.at(key);return inputs;};
 struct IoRow {JobPtr job,video,audio;Json data,inputs;bool inCatalog,registered,owned;};
 const auto captureIo=[&](const std::vector<JobPtr>& owners){
  std::vector<IoRow> rows;auto pending=owners;std::unordered_set<const Job*> seen;
  while(!pending.empty()){auto item=pending.back();pending.pop_back();if(!item||!seen.insert(item.get()).second)continue;rows.push_back({item,item->video,item->audio,item->data,ioInputs(item->data),modelJob(item->id())==item,std::find(jobs.begin(),jobs.end(),item)!=jobs.end(),true});if(item->video)pending.push_back(item->video);if(item->audio)pending.push_back(item->audio);}
  if(scope&&!seen.count(scope.get()))rows.push_back({scope,scope->video,scope->audio,scope->data,ioInputs(scope->data),modelJob(scope->id())==scope,std::find(jobs.begin(),jobs.end(),scope)!=jobs.end(),false});return rows;
 };
 const auto clearIndexes=[&]{jobsById.clear();publicationJobs.clear();publicationIndexReady=false;};
 const auto validateIo=[&](const std::vector<IoRow>& rows,const PublicationOwnership& owner,bool acceptedCleanup=false){
  if(stopping&&!acceptedCleanup)throw std::runtime_error("UDM is closing.");
  if(catalogTransaction)throw std::runtime_error("The catalog batch changed during recovery. Keep its bytes and recovery journal.");
  for(const auto& row:rows)if((row.owned&&!owner.owns(row.job))||modelJob(str(row.data,"Id"))!=(row.inCatalog?row.job:JobPtr{})||(std::find(jobs.begin(),jobs.end(),row.job)!=jobs.end())!=row.registered||row.job->video!=row.video||row.job->audio!=row.audio||ioInputs(row.job->data)!=row.inputs)throw std::runtime_error("The download changed during file recovery. Its bytes and recovery journal were preserved.");
 };
 const auto detachedIo=[&](const std::vector<IoRow>& rows,const PublicationOwnership& owner,auto&& work){
  const auto observation=catalogCheckpoint;
  const auto checkpoint=[&](const char* point){
   {Lock guard(mutex);if(stopping)throw std::runtime_error("UDM is closing.");if(std::string_view(point)=="recovery-before-move")validateIo(rows,owner);}
   if(observation)observation(point);
  };
  recoveryLock.unlock();try{work(checkpoint);}catch(...){recoveryLock.lock();clearIndexes();throw;}
  recoveryLock.lock();clearIndexes();validateIo(rows,owner);
 };
 struct JournalDiscovery {
  std::unique_ptr<RecoveryJournal> snapshot;Json operation;std::function<void(JournalDiscovery&)> cleanup;
  JournalDiscovery()=default;JournalDiscovery(const JournalDiscovery&)=delete;JournalDiscovery& operator=(const JournalDiscovery&)=delete;JournalDiscovery(JournalDiscovery&&)=default;
  JournalDiscovery& operator=(JournalDiscovery&& other){if(this!=&other){if(snapshot&&cleanup)cleanup(*this);snapshot=std::move(other.snapshot);operation=std::move(other.operation);cleanup=std::move(other.cleanup);}return *this;}
  ~JournalDiscovery(){if(snapshot&&cleanup)cleanup(*this);}
 };
 const auto discoveryIo=[&](const std::vector<JobPtr>& roots,auto&& work){
  std::unique_ptr<PublicationOwnership> selected;std::vector<IoRow> rows;
  if(!roots.empty()){
   bool relatedWorker=false;if(scope)for(const auto& candidate:jobs)if(scopeIds.count(str(candidate->data,"ReplacementOf"))&&isActive(candidate)){relatedWorker=true;break;}
   for(const auto& row:roots)if(isActive(row))throw std::runtime_error("Wait for the download to stop before discovering its recovery journal.");
   // An active replacement legitimately discovers its predecessor before it
   // publishes. Do not reserve that predecessor out from under the worker.
   // Discovery is read-only; any actual operation still needs full ownership.
   if(!relatedWorker){selected=std::make_unique<PublicationOwnership>(*this,roots,recoveryPaths(roots,{}));selected->beginCommit();}
   rows=captureIo(roots);if(relatedWorker)for(auto& row:rows)row.owned=false;
  }
  const auto observation=catalogCheckpoint;
  const auto checkpoint=[&](const char* point){{Lock guard(mutex);if(stopping)throw std::runtime_error("UDM is closing.");}if(observation)observation(point);};
  recoveryLock.unlock();try{work(checkpoint);}catch(...){recoveryLock.lock();clearIndexes();throw;}
  recoveryLock.lock();clearIndexes();if(selected)validateIo(rows,*selected);else if(!rows.empty())validateIo(rows,discoveryLifetime);else{if(stopping)throw std::runtime_error("UDM is closing.");if(catalogTransaction)throw std::runtime_error("Finish the catalog batch before discovering recovery journals.");}
 };
 const auto enumerateJournals=[&](const fs::path& folder){
  std::vector<fs::path> paths;
  discoveryIo(scope?std::vector<JobPtr>{scope}:std::vector<JobPtr>{},[&](const auto& checkpoint){
   checkpoint("recovery-discovery-enumerate");std::error_code failure;fs::directory_iterator current(folder,failure),end;
   if(failure==std::errc::no_such_file_or_directory)return;
   if(failure)throw fs::filesystem_error("Cannot enumerate recovery journals",folder,failure);
   while(current!=end){const auto path=current->path();if(path.extension()==L".json")paths.push_back(path);current.increment(failure);if(failure)throw fs::filesystem_error("Cannot enumerate recovery journals",folder,failure);}
   if(!paths.empty())checkpoint("recovery-discovery-candidates");
  });return paths;
 };
 const auto readDiscovery=[&](const fs::path& path,const JobPtr& known){
  JournalDiscovery result;result.cleanup=[&](JournalDiscovery& value){const bool held=recoveryLock.owns_lock();if(held)recoveryLock.unlock();value.snapshot.reset();value.operation=Json();if(held){recoveryLock.lock();clearIndexes();}};
  auto roots=scope?std::vector<JobPtr>{scope}:known?std::vector<JobPtr>{known}:std::vector<JobPtr>{};
  discoveryIo(roots,[&](const auto& checkpoint){
   result.snapshot=RecoveryJournal::snapshot(path,32*1024*1024,checkpoint);checkpoint("recovery-discovery-parse");
   result.operation=Json::parse(result.snapshot->text());checkpoint("recovery-discovery-parsed");
  });return result;
 };
 const auto discardDiscovery=[&](JournalDiscovery& result){discoveryIo(scope?std::vector<JobPtr>{scope}:std::vector<JobPtr>{},[&](const auto&){result.snapshot.reset();result.operation=Json();});};
 const auto claimDiscovery=[&](const fs::path& path,JournalDiscovery& discovered,const std::vector<IoRow>& rows,const PublicationOwnership& owner){
  const auto release=[&](RecoveryJournal* value){if(!value)return;const bool held=value->isOpen()&&recoveryLock.owns_lock();if(held)recoveryLock.unlock();delete value;if(held){recoveryLock.lock();clearIndexes();}};
  std::unique_ptr<RecoveryJournal,std::function<void(RecoveryJournal*)>> journal(nullptr,release);
  detachedIo(rows,owner,[&](const auto& checkpoint){auto claimed=RecoveryJournal::claim(path,*discovered.snapshot,checkpoint);journal.reset(claimed.release());discovered.snapshot.reset();});return journal;
 };
 // Close held verified files outside Model on success and exceptional unwind.
 // Full operation ownership was declared first and outlives this exit guard.
 struct RecoveryByteExit {std::function<void()> close;~RecoveryByteExit(){close();}};
 const auto releaseRecoveryBytes=[&](auto&... values){
  const bool heldBytes=((values&&(values->file||values->name))||...);if(!heldBytes)return;
  const bool heldModel=recoveryLock.owns_lock();if(heldModel)recoveryLock.unlock();(values.reset(),...);
  if(heldModel){recoveryLock.lock();clearIndexes();}
 };
 // A retained source may differ from staging. Reserve its lexical absolute
 // name before cleanup; filesystem/receipt validation still runs outside Model.
 const auto retainedCleanupPath=[](const Json& before){const auto entry=before.find("RetainedMediaOutput");if(entry==before.end()||!entry->is_object())return fs::path{};const auto value=fs::path(wide(str(*entry,"Path")));return value.is_absolute()?value:fs::path{};};
 struct RecoveredRow {JobPtr job;Json before,applied,destination;bool inCatalog,registered;};
 auto capturedRow=[&](const JobPtr& job,const Json& before,bool inCatalog,bool registered){return RecoveredRow{job,before,job->data,publicationDestinationInputs(job->data),inCatalog,registered};};
 auto checkpointRecovery=[&](const std::vector<RecoveredRow>& rows,const Json& receipt,bool inserted){
  PublicationCatalogProof::Lease proof;std::function<void(const char*)> observation;
  const auto rollback=[&]{for(const auto& row:rows)row.job->data=PublicationCommit::rollback(row.before,row.applied,row.job->data);if(inserted)PublicationCatalogProof::eraseReceipt(*this,receipt);};
  try{std::vector<PublicationCatalogProof::RecoveryRow> bindings;for(const auto& row:rows)bindings.push_back({row.before,row.applied,row.inCatalog});proof=PublicationCatalogProof::prepareRecovery(*this,bindings,receipt);observation=catalogCheckpoint;}catch(...){rollback();throw;}
  recoveryLock.unlock();std::exception_ptr failure;
  try{if(observation)observation("recovery-catalog-prepared");save();}catch(...){failure=std::current_exception();}
  const bool accepted=proof.resolve();recoveryLock.lock();jobsById.clear();publicationJobs.clear();publicationIndexReady=false;
  if(!accepted){rollback();proof.release();if(failure)std::rethrow_exception(failure);throw std::runtime_error("The recovered file has not been durably recorded. Its journal was preserved.");}
  for(const auto& row:rows)if(modelJob(str(row.applied,"Id"))!=(row.inCatalog?row.job:JobPtr{})||(std::find(jobs.begin(),jobs.end(),row.job)!=jobs.end())!=row.registered||publicationDestinationInputs(row.job->data)!=row.destination||!PublicationCatalogProof::recoveryMatches(row.before,row.applied,row.job->data))throw std::runtime_error("The recovered download changed after catalog publication. Keep its files and recovery journal.");
 };
 // Catalog acceptance is irreversible here. Cleanup never rolls it back.
 // Both the accepted current inputs and a private receipt remain authoritative
 // while Model is released, including after exact-handle journal retirement.
 const auto cleanupRecovery=[&](const std::vector<JobPtr>& roots,const PublicationOwnership& owner,const Json& receipt,auto&& work){
  const auto rows=captureIo(roots);const auto observation=catalogCheckpoint;
  // Once accepted, shutdown drains cleanup rather than abandoning retirement.
  // It still waits for this owner; no new operation may start after closing.
  const auto validate=[&]{Lock guard(mutex);validateIo(rows,owner,true);if(!receipt.is_null()){auto all=state.find("PublicationReceipts");if(all==state.end()||!all->is_object())throw std::runtime_error("The accepted private recovery receipt changed during cleanup.");auto found=all->find(str(receipt,"PublicationCommitId"));if(found==all->end()||*found!=receipt)throw std::runtime_error("The accepted private recovery receipt changed during cleanup.");}};
  const auto point=[&](const char* phase){validate();if(observation)observation(phase);validate();};
  recoveryLock.unlock();try{point("recovery-cleanup-begin");work(point);point("recovery-cleanup-complete");}catch(...){recoveryLock.lock();clearIndexes();throw;}
  recoveryLock.lock();clearIndexes();validate();
 };
 auto publications=root/L"publications";
 for(const auto& journalPath:enumerateJournals(publications)){
  if(stopping)throw std::runtime_error("UDM is closing.");
  if(scope&&!scopeIds.count(utf8(journalPath.stem().wstring())))continue;
  if(catalogTransaction)throw std::runtime_error("Recover this download before starting a catalog batch. Its publication journal has been preserved.");
  auto known=findPublicationJob(utf8(journalPath.stem().wstring()));if(!scope&&known&&(isActive(known)||PublicationOwnership::reserved(*this,known->target(),known)))continue;
  JournalDiscovery discovered;try{discovered=readDiscovery(journalPath,known);}catch(const std::system_error& failure){if(!scope&&(failure.code().value()==ERROR_FILE_NOT_FOUND||failure.code().value()==ERROR_PATH_NOT_FOUND))continue;throw;}if(findPublicationJob(utf8(journalPath.stem().wstring()))!=known)throw std::runtime_error("The download changed during journal discovery. Its bytes and journal were preserved.");auto op=std::move(discovered.operation);auto job=findPublicationJob(str(op,"Id"));const bool ephemeral=op.value("Ephemeral",false);
  bool acceptedReceipt=false;if(ephemeral){const auto id=PublicationCommit::identity(op);if(id.empty())throw std::runtime_error("Missing private publication identity. Keep its recovery record.");auto rows=state.find("PublicationReceipts");if(rows!=state.end()){if(!rows->is_object())throw std::runtime_error("Invalid private publication receipt store.");auto receipt=rows->find(id);if(receipt!=rows->end()){if(*receipt!=PublicationCatalogProof::binding(op.at("After")))throw std::runtime_error("Private publication receipt changed. Keep its file and recovery record.");acceptedReceipt=true;}}}
  if(!job&&ephemeral)job=std::make_shared<Job>(op.at(acceptedReceipt?"After":"Before"));
  if(!job||journalPath.stem()!=wide(job->id())||!op.contains("Before")||!op.contains("After")||str(op["Before"],"Id")!=job->id()||str(op["After"],"Id")!=job->id())throw std::runtime_error("Publication recovery requires its preserved download record.");
  if(!scope&&PublicationOwnership::treeOwned(*this,job)){discardDiscovery(discovered);continue;}
  auto target=fs::path(wide(str(op,"Target"))),staging=fs::path(wide(str(op,"Staging")));
  const auto rowTarget=[](const Json& row){return(fs::path(wide(str(row,"Folder")))/wide(str(row,"FileName"))).lexically_normal();};
  if(rowTarget(op["Before"])!=target.lexically_normal()||rowTarget(op["After"])!=target.lexically_normal())throw std::runtime_error("Recovery record destinations changed. Keep its file and journal.");
  const auto hash=str(op,"Hash");
  if(op.contains("StagedIdentity")&&!op["StagedIdentity"].is_object())throw std::runtime_error("Publication source identity is invalid. Keep its files and recovery record.");
  const bool validIdentity=hash.empty()&&op.contains("StagedIdentity")&&op["StagedIdentity"].is_object();
  if(target.lexically_normal()!=job->target().lexically_normal()||target.lexically_normal()==staging.lexically_normal()||
     (!validIdentity&&!std::regex_match(hash,std::regex("[a-f0-9]{64}")))||str(op["After"],"Sha256")!=hash||
     str(op["After"],"Status")!="Complete")throw std::runtime_error("Publication recovery paths or file identity changed. Keep the files and recovery record.");
  const auto commitId=PublicationCommit::identity(op);PublicationCommit::validateCurrent(op,job->data);
  if(!scope&&isActive(job)){discardDiscovery(discovered);continue;}
  if(!scope&&PublicationOwnership::reserved(*this,target,job)){discardDiscovery(discovered);continue;}
  const bool inCatalog=modelJob(job->id())==job,registered=std::find(jobs.begin(),jobs.end(),job)!=jobs.end();
  PublicationOwnership owner(*this,{job},recoveryPaths({job},{target,staging,journalPath,retainedCleanupPath(op.at("Before"))}));owner.beginCommit();
  const auto prepared=captureIo({job});auto journal=claimDiscovery(journalPath,discovered,prepared,owner);const auto input=job->data;
  std::optional<RecoveredPublicationBytes> published;RecoveryByteExit releaseBytes{[&]{releaseRecoveryBytes(published);}};
  detachedIo(prepared,owner,[&](const auto& checkpoint){
   if(!commitId.empty()&&str(input,"PublicationCommitId")==commitId){
    published=recoverPublicationBytes(op,target,nullptr,checkpoint);if(!published)throw std::runtime_error("An acknowledged publication needs manual recovery. Keep its file and completion record.");
   }else if(fs::exists(staging)){
    if(recoverableStagingPath(op,input,staging,target)){validateGrabberFolder(op["Before"]);published=recoverPublicationBytes(op,staging,&target,checkpoint);}
   }else{
    published=recoverPublicationBytes(op,target,nullptr,checkpoint);if(!published)throw std::runtime_error("A published download needs manual recovery. Keep the saved file and publication record.");
   }
  });
  auto before=job->data;PublicationCommit::validateCurrent(op,before);
  if(!published&&op.value("FlushBeforePublish",false))throw std::runtime_error("The early publication source cannot be recovered safely. Keep its bytes and recovery journal; do not reuse unflushed completion accounting.");
  const bool accepted=published&&!commitId.empty()&&PublicationCommit::accepted(op,before,published->digest);
  if(!commitId.empty())job->data=accepted?before:PublicationCommit::apply(op["Before"],published?op["After"]:op["Before"],before);
  else job->data=PublicationCommit::apply(op["Before"],published?op["After"]:op["Before"],before);
  if(published){
   job->data["Sha256"]=published->digest;
   if(!commitId.empty()&&!accepted){job->data["QueueMember"]=retainCompletedMembership(*this,before);if(findJob(job->id())==job)prepareCompletionScan(job->data,state["Settings"]);job->data["CompletionActionArmed"]=false;}
   else if(findJob(job->id())==job)prepareRecoveredScan(job->data,before);
  }
  else job->data["Status"]="Paused";
  Json receipt;bool insertedReceipt=false;if(ephemeral&&published){receipt=PublicationCatalogProof::binding(op["After"]);insertedReceipt=PublicationCatalogProof::installReceipt(*this,receipt);}
  checkpointRecovery({capturedRow(job,before,inCatalog,registered)},receipt,insertedReceipt);
  cleanupRecovery({job},owner,ephemeral&&published?receipt:Json(),[&](const auto& point){
   point("recovery-cleanup-release");if(published)published->release();
   point("recovery-cleanup-metadata");if(published){if(retainedMediaOutputPath(op.at("Before")).empty())markZone(target);else finishRetainedMediaRecovery(op.at("Before"),target);}
   point("recovery-cleanup-metadata-complete");journal->retire(point);
  });
  if(ephemeral&&published)PublicationCatalogProof::eraseReceipt(*this,receipt);
 }
 // Older builds could publish and hash the final file before a catalog save
 // failed, without a publication journal. Reconcile only that exact failure.
 const auto legacyJobs=jobs;for(auto job:legacyJobs){if(stopping)throw std::runtime_error("UDM is closing.");if(std::find(jobs.begin(),jobs.end(),job)==jobs.end())continue;if(scope&&!scopeIds.count(job->id()))continue;if(isActive(job))continue;const auto data=job->data;const auto hash=lower(str(data,"Sha256"));
  if(str(data,"Status")!="Failed"||str(data,"Error").find("Could not replace the saved state (Windows error 1175).")!=0||str(data,"Finished").empty()||data.contains("OriginalResponse")||!str(data,"ReplacementOf").empty()||num(data,"Size",-1)<0||num(data,"Received")!=num(data,"Size")||!std::regex_match(hash,std::regex("[a-f0-9]{64}")))continue;
  auto expected=lower(str(data,"ExpectedSha256"));if(!expected.empty()&&expected!=hash)continue;
  auto target=job->target();PublicationOwnership owner(*this,{job},recoveryPaths({job},{target}));owner.beginCommit();
  if(catalogTransaction)throw std::runtime_error("Recover this completed file before starting a catalog batch.");
  const auto prepared=captureIo({job});std::optional<RecoveredPublicationBytes> stableFile;RecoveryByteExit releaseBytes{[&]{releaseRecoveryBytes(stableFile);}};
  detachedIo(prepared,owner,[&](const auto& checkpoint){stableFile=recoverReplacementBytes(target,hash,false,checkpoint);});
  if(!stableFile||stableFile->size!=num(data,"Size"))continue;
  auto before=job->data;auto after=completeData(before,stableFile->size,hash);after["Finished"]=before["Finished"];after["QueueMember"]=retainCompletedMembership(*this,before);after["PublicationRecovered"]=date();job->data=std::move(after);
  prepareCompletionScan(job->data,state["Settings"]);job->data["CompletionActionArmed"]=false;
  checkpointRecovery({capturedRow(job,before,true,true)},Json(),false);
  cleanupRecovery({job},owner,Json(),[&](const auto& point){point("recovery-cleanup-release");stableFile.reset();});
 }
 // A completed anonymous internal file never becomes a visible download row.
 // Its private receipt may outlive unlink until the next checkpoint or restart.
 if(!scope&&!catalogTransaction){
  Json receiptInventory;
  {auto store=state.find("PublicationReceipts");if(store!=state.end()){if(!store->is_object())throw std::runtime_error("Invalid private publication receipt store.");receiptInventory=*store;}}
  if(!receiptInventory.is_null()){
   struct ReceiptCandidate {Json receipt;JobPtr job;fs::path publication,replacement;bool absent=false;};
   std::vector<ReceiptCandidate> candidates;std::vector<JobPtr> owners;std::vector<fs::path> paths;
   const auto capturedRoot=root;
   for(auto it=receiptInventory.begin();it!=receiptInventory.end();++it){
    const auto descriptor=PublicationCatalogProof::binding(it.value());
    if(descriptor!=it.value()||it.key()!=str(descriptor,"PublicationCommitId"))throw std::runtime_error("Private publication receipt identity changed.");
    const auto target=fs::path(wide(str(descriptor,"Folder")))/wide(str(descriptor,"FileName"));
    auto job=modelJob(str(descriptor,"Id"));
    if(PublicationOwnership::reserved(*this,target)||(job&&isActive(job)))continue;
    const auto publication=publications/(wide(str(descriptor,"Id"))+L".json"),replacement=root/L"replacements"/(wide(str(descriptor,"Id"))+L".json");
    candidates.push_back({descriptor,job,publication,replacement});if(job)owners.push_back(job);
    paths.push_back(target);paths.push_back(publication);paths.push_back(replacement);
   }
   if(!candidates.empty()){
    auto owner=PublicationOwnership::receiptRetirement(*this,owners,recoveryPaths(owners,std::move(paths)));owner.beginCommit();
    const auto inputs=captureIo(owners);const auto observation=catalogCheckpoint;
    const auto validate=[&]{
     validateIo(inputs,owner);if(root!=capturedRoot)throw std::runtime_error("The receipt cleanup catalog changed.");
     auto current=state.find("PublicationReceipts");if(current==state.end()||!current->is_object())throw std::runtime_error("The private receipt store changed during cleanup.");
     for(const auto& item:candidates){auto found=current->find(str(item.receipt,"PublicationCommitId"));if(found==current->end()||*found!=item.receipt||modelJob(str(item.receipt,"Id"))!=item.job)throw std::runtime_error("A private publication receipt changed during cleanup.");}
    };
    // Only immutable descriptors and paths cross this IO interval. The path
    // lease excludes in-process publishers and remains enrolled through save.
    recoveryLock.unlock();
    try{if(observation)observation("receipt-prune-inventory");for(auto& item:candidates)item.absent=!fs::exists(item.publication)&&!fs::exists(item.replacement);}
    catch(...){recoveryLock.lock();clearIndexes();throw;}
    recoveryLock.lock();clearIndexes();validate();
    Json retired=Json::object();for(const auto& item:candidates)if(item.absent)retired[str(item.receipt,"PublicationCommitId")]=item.receipt;
    if(!retired.empty()){
     // Roll back only our missing exact keys. Never replace a later receipt or
     // the whole store, and never roll back an accepted durable retirement.
     const auto rollback=[&]{auto current=state.find("PublicationReceipts");if(current==state.end())current=state.emplace("PublicationReceipts",Json::object()).first;if(current->is_object())for(auto it=retired.begin();it!=retired.end();++it)if(!current->contains(it.key()))(*current)[it.key()]=it.value();};
     for(auto it=retired.begin();it!=retired.end();++it)PublicationCatalogProof::eraseReceipt(*this,it.value());
     PublicationCatalogProof::Lease proof;
     try{proof=PublicationCatalogProof::prepareReceiptRetirement(*this,retired);}catch(...){rollback();throw;}
     recoveryLock.unlock();std::exception_ptr failure;
     try{if(observation)observation("receipt-prune-catalog-prepared");save();}catch(...){failure=std::current_exception();}
     const bool accepted=proof.resolve();recoveryLock.lock();clearIndexes();
     if(!accepted){proof.release();rollback();if(failure)std::rethrow_exception(failure);throw std::runtime_error("Receipt cleanup has not been durably recorded. Its receipts were preserved.");}
    }
   }
  }
 }
 auto folder=root/L"replacements";
 for(const auto& journalPath:enumerateJournals(folder)){
  if(stopping)throw std::runtime_error("UDM is closing.");
  auto known=findPublicationJob(utf8(journalPath.stem().wstring()));if(!scope&&known&&(isActive(known)||PublicationOwnership::reserved(*this,known->target(),known)))continue;
  const auto knownBefore=known;const auto knownInputs=known?ioInputs(known->data):Json();const auto knownVideo=known?known->video:JobPtr{},knownAudio=known?known->audio:JobPtr{};
  JournalDiscovery discovered;try{discovered=readDiscovery(journalPath,known);}catch(const std::system_error& failure){if(!scope&&(failure.code().value()==ERROR_FILE_NOT_FOUND||failure.code().value()==ERROR_PATH_NOT_FOUND))continue;throw;}auto op=std::move(discovered.operation);
  if(scope&&!scopeIds.count(utf8(journalPath.stem().wstring()))&&!scopeIds.count(str(op,"OriginalId"))){discardDiscovery(discovered);continue;}
  auto currentKnown=findPublicationJob(utf8(journalPath.stem().wstring()));
  if(currentKnown!=knownBefore||(knownBefore&&(ioInputs(knownBefore->data)!=knownInputs||knownBefore->video!=knownVideo||knownBefore->audio!=knownAudio)))throw std::runtime_error("The replacement changed during journal discovery. Its files and recovery journal were preserved.");
  if(catalogTransaction)throw std::runtime_error("Recover this download before starting a catalog batch. Its replacement journal has been preserved.");
  const auto commitId=ReplacementCommit::identity(op);const bool ephemeral=!commitId.empty()&&op.value("Ephemeral",false);auto job=ephemeral?findPublicationJob(str(op,"Id")):findJob(str(op,"Id"));auto original=findJob(str(op,"OriginalId"));
  if(ephemeral){auto rows=state.find("PublicationReceipts");bool acknowledged=false;if(rows!=state.end()){if(!rows->is_object())throw std::runtime_error("Invalid private replacement receipt store.");auto receipt=rows->find(commitId);if(receipt!=rows->end()){if(*receipt!=PublicationCatalogProof::binding(op.at("After")))throw std::runtime_error("Private replacement receipt changed. Keep both files and its journal.");acknowledged=true;}}if(!job)job=std::make_shared<Job>(op.at(acknowledged?"After":"Before"));}
  if(!job||!original)throw std::runtime_error("Replacement recovery requires the preserved history records.");
  if(journalPath.stem()!=wide(job->id()))throw std::runtime_error("Replacement recovery identifier does not match its record. Keep the saved files and journal.");
  if(!scope&&(PublicationOwnership::treeOwned(*this,job)||PublicationOwnership::treeOwned(*this,original))){discardDiscovery(discovered);continue;}
  if(scope&&(isActive(job)||isActive(original)))throw std::runtime_error("Wait for the replacement and its original download to stop before recovery.");
  ReplacementCommit::validateCurrent(op,job->data,original->data);
  auto target=fs::path(wide(str(op,"Target"))),previous=fs::path(wide(str(op,"Previous")));
  if(!scope&&(isActive(job)||isActive(original)||PublicationOwnership::reserved(*this,target,job)||PublicationOwnership::reserved(*this,previous,original))){discardDiscovery(discovered);continue;}
  if(str(op.at("Before"),"Id")!=job->id()||str(op.at("After"),"Id")!=job->id()||str(op.at("OriginalBefore"),"Id")!=original->id()||str(op.at("OriginalAfter"),"Id")!=original->id())throw std::runtime_error("Replacement recovery record identities changed. Keep both files and its journal.");
  const auto samePath=[](const fs::path& a,const fs::path& b){return a.is_absolute()&&b.is_absolute()&&windowsPath::equal(windowsPath::key(a),windowsPath::key(b));};
  if(!samePath(job->target(),target)||(!samePath(original->target(),target)&&!samePath(original->target(),previous))||(!str(job->data,"PreviousPath").empty()&&!samePath(fs::path(wide(str(job->data,"PreviousPath"))),previous))||(!str(job->data,"ReplacementOf").empty()&&str(job->data,"ReplacementOf")!=original->id())||(!str(job->data,"ReplacementHash").empty()&&str(job->data,"ReplacementHash")!=str(op,"OldHash")))throw std::runtime_error("Replacement recovery destinations changed. Keep both files and the recovery journal.");
  const bool inCatalog=modelJob(job->id())==job,registered=std::find(jobs.begin(),jobs.end(),job)!=jobs.end();
  if(modelJob(original->id())!=original)throw std::runtime_error("The recovered original history changed.");
  PublicationOwnership owner(*this,{job,original},recoveryPaths({job,original},{target,previous,journalPath,fs::path(wide(str(op,"Staging"))),retainedCleanupPath(op.at("Before"))}));owner.beginCommit();
  const auto prepared=captureIo({job,original});auto journal=claimDiscovery(journalPath,discovered,prepared,owner);
  std::optional<RecoveredPublicationBytes> newBytes,previousBytes,originalBytes;RecoveryByteExit releaseBytes{[&]{releaseRecoveryBytes(newBytes,previousBytes,originalBytes);}};enum class FileOutcome {Complete,Original};FileOutcome outcome=FileOutcome::Original;
  detachedIo(prepared,owner,[&](const auto& checkpoint){
   // Inspect a target once. A preserved old original must not be opened and
   // hashed again merely because its digest differs from the new version.
   auto targetBytes=recoverReplacementBytes(target,"",false,checkpoint);
   if(targetBytes&&targetBytes->digest==str(op,"NewHash")){
    previousBytes=recoverReplacementBytes(previous,str(op,"OldHash"),false,checkpoint);
    if(previousBytes){requireRecoveredBytes(op.at("After"),*targetBytes,target);requireRecoveredBytes(op.at("OriginalAfter"),*previousBytes,previous);newBytes=std::move(targetBytes);outcome=FileOutcome::Complete;return;}
   }
   if(targetBytes&&targetBytes->digest==str(op,"OldHash")&&!fs::exists(previous)){originalBytes=std::move(targetBytes);return;}
   if(!fs::exists(target)&&(originalBytes=recoverReplacementBytes(previous,str(op,"OldHash"),true,checkpoint))){
    checkpoint("recovery-before-move");if(!MoveFileExW(previous.c_str(),target.c_str(),MOVEFILE_WRITE_THROUGH))throw std::runtime_error("Windows could not restore the original file.");
    const auto restoredSize=originalBytes->size;auto pinned=pinRecoveredDestination(*originalBytes->file,target);originalBytes=RecoveredPublicationBytes(std::move(pinned),std::move(originalBytes->digest),restoredSize);return;
   }
   throw std::runtime_error("A replacement needs manual recovery. Keep the journal, original and downloaded files.");
  });
  ReplacementCommit::validateCurrent(op,job->data,original->data);
  Json recovered,recoveredOriginal;const bool published=outcome==FileOutcome::Complete;
  if(published){
   if(commitId.empty()){recovered=PublicationCommit::apply(op.at("Before"),op.at("After"),job->data);recoveredOriginal=ReplacementCommit::applyOriginal(op.at("OriginalBefore"),op.at("OriginalAfter"),original->data);prepareRecoveredScan(recovered,job->data);}
   else if(ReplacementCommit::accepted(op,job->data,original->data)){recovered=job->data;recoveredOriginal=original->data;}
   else{const bool currentFile=ReplacementCommit::rowAccepted(op.at("After"),job->data),currentOriginal=ReplacementCommit::rowAccepted(op.at("OriginalAfter"),original->data);recovered=currentFile?job->data:PublicationCommit::apply(op.at("Before"),op.at("After"),job->data);recoveredOriginal=currentOriginal?original->data:ReplacementCommit::applyOriginal(op.at("OriginalBefore"),op.at("OriginalAfter"),original->data);if(!currentFile){recovered["QueueMember"]=retainCompletedMembership(*this,job->data);if(findJob(job->id())==job)prepareRecoveredScan(recovered,job->data);recovered["CompletionActionArmed"]=false;}}
  }else{
   recovered=PublicationCommit::rollback(op.at("Before"),op.at("After"),job->data);recoveredOriginal=PublicationCommit::rollback(op.at("OriginalBefore"),op.at("OriginalAfter"),original->data);recovered["Status"]="Paused";
   // The rollback journal can contain stale original accounting. Its verified
   // held file, not Before's counters, supplies the completed original's bytes.
   if(!originalBytes||originalBytes->size<0||str(recoveredOriginal,"Status")!="Complete")throw std::runtime_error("The original completion cannot be reconciled. Keep its bytes and recovery journal.");
   recoveredOriginal["Size"]=originalBytes->size;recoveredOriginal["Received"]=originalBytes->size;recoveredOriginal["Sha256"]=originalBytes->digest;recoveredOriginal["Folder"]=utf8(target.parent_path().wstring());recoveredOriginal["FileName"]=utf8(target.filename().wstring());
  }
  Json receipt;bool inserted=false;if(ephemeral&&published){receipt=PublicationCatalogProof::binding(op.at("After"));inserted=PublicationCatalogProof::installReceipt(*this,receipt);}
  // Prepare both records before changing live state. Preserve independent current
  // metadata on both current and legacy recovery.
  job->data.swap(recovered);original->data.swap(recoveredOriginal);
  checkpointRecovery({capturedRow(job,recovered,inCatalog,registered),capturedRow(original,recoveredOriginal,true,true)},receipt,inserted);
  cleanupRecovery({job,original},owner,ephemeral&&published?receipt:Json(),[&](const auto& point){
   point("recovery-cleanup-release");newBytes.reset();previousBytes.reset();originalBytes.reset();
   point("recovery-cleanup-metadata");if(published)finishRetainedMediaRecovery(op.at("Before"),target);
   point("recovery-cleanup-metadata-complete");journal->retire(point);
  });
  if(ephemeral&&published)PublicationCatalogProof::eraseReceipt(*this,receipt);
 }
 };
 reconcile();if(after)after();
}
}
