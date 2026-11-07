#include "Core.hpp"
#include "QueueMembership.hpp"
#include "CapturePresentation.hpp"
#include "GrabberDestinations.hpp"
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
 Lock lock(mutex);auto key=resourceKey(address);auto account=accountHeaders(siteRequestHeaders(address,headers,state["Settings"]));auto post=validatePostRequest(request,address);JobPtr newest;
 for(auto it=jobs.rbegin();it!=jobs.rend();++it){auto job=*it;
  if(job==ignore||job->data.contains("OfflineProject")||readPostRequest(job->data)!=post||!str(job->data,"SourceUrl").empty()||!str(job->data,"ProtectedAdaptive").empty()||resourceKey(str(job->data,"Url"))!=key||accountHeaders(siteRequestHeaders(str(job->data,"Url"),readHeaders(job->data),state["Settings"]))!=account)continue;
  if(isActive(job)||str(job->data,"Status")=="Queued"||str(job->data,"Status")=="Awaiting confirmation"||!str(job->data,"DuplicateOf").empty()||!str(job->data,"ReplacementOf").empty())return job;
  if(!newest)newest=job;
 }return newest;
}
JobPtr Manager::offerDownload(const std::string& address,const std::string& folder,const std::string& name,const std::string& queue,bool paused,const Headers& headers,const Json& request,const Json& browserProxy,const Json& browserSession){
 Lock lock(mutex);auto candidate=add(address,folder,name,queue,true,headers,"",request,browserProxy,browserSession);
 try{
  auto existing=findDuplicate(address,readHeaders(candidate->data),candidate,request);
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
  auto priorPath=existing->target();auto stem=priorPath.stem().wstring(),extension=priorPath.extension().wstring();for(int index=1;;++index){priorPath=existing->target().parent_path()/(stem+L" (previous "+std::to_wstring(index)+L")"+extension);bool occupied=fs::exists(priorPath)||relocationReserves(priorPath);for(auto other:jobs)occupied|=lower(utf8(other->target().wstring()))==lower(utf8(priorPath.wstring()))||lower(str(other->data,"PreviousPath"))==lower(utf8(priorPath.wstring()));if(!occupied)break;}
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
 Lock lock(mutex);if(!job||std::find(jobs.begin(),jobs.end(),job)==jobs.end())throw std::runtime_error("This download was removed.");
 if(!str(job->data,"DuplicateOf").empty())throw std::runtime_error("Choose how to handle this duplicate first.");
 if(str(job->data,"Status")=="Complete"){existingOffers.erase(job->id());return OfferPresentation::Complete;}
 if(existingOffers.count(job->id())&&!isActive(job)&&str(job->data,"Status")!="Queued"){auto before=job->data;auto limit=job->sessionLimit;try{resume(job);}catch(...){job->data=before;job->sessionLimit=limit;throw;}}
 existingOffers.erase(job->id());
 return isActive(job)||str(job->data,"Status")=="Queued"?OfferPresentation::Progress:OfferPresentation::Information;
}
static Json completeData(const Json& data,const fs::path& staging,const std::string& hash){auto next=data;next.erase("RetainedMediaOutput");next["Sha256"]=hash;next["Size"]=fs::file_size(staging);next["Received"]=next["Size"];next["Status"]="Complete";next["MediaPhase"]="Complete";next["MediaTracksReady"]=false;next["SabrRetainedBytes"]=0;next["Finished"]=date();next["Error"]="";next["QueueMember"]=false;if(yes(next,"SynchronizationRestoration")){next["SyncStatus"]="Restored missing file";next["LastSync"]=next["Finished"];next["SyncRetryFailed"]=false;next.erase("SynchronizationRestoration");}return next;}
void Manager::publishFile(JobPtr job,const fs::path& staging,const std::string& knownHash,const Cancel* cancel,const LivePublicationDetails* live){
 fs::path target;std::string originalId,replacementHash,previousPath;bool registered=false;
 {Lock lock(mutex);if(!job)throw std::runtime_error("The download was removed before publication.");registered=std::find(jobs.begin(),jobs.end(),job)!=jobs.end();target=job->target();originalId=str(job->data,"ReplacementOf");replacementHash=str(job->data,"ReplacementHash");previousPath=str(job->data,"PreviousPath");}
 auto checkpoint=[&]{if(cancel)cancel->check();};checkpoint();
 auto hash=knownHash;
 Json stagedIdentity;
 Handle stagedFile;
 if(hash.empty()){
  if(originalId.empty()){
   stagedFile.h=CreateFileW(staging.c_str(),GENERIC_READ,FILE_SHARE_READ|FILE_SHARE_DELETE,nullptr,
                           OPEN_EXISTING,FILE_FLAG_OPEN_REPARSE_POINT|FILE_FLAG_SEQUENTIAL_SCAN,nullptr);
   if(!stagedFile)throw std::runtime_error("Cannot open the completed download for publication.");
   stagedIdentity=stableFileIdentity(stagedFile.h);
   if(stagedIdentity.is_null())hash=fileHash(stagedFile.h,checkpoint);
  }else hash=fileHash(staging,checkpoint);
 }
 std::unique_ptr<Handle> protectOriginal;
 auto matchesOriginal=[&]{
  if(!protectOriginal||!(*protectOriginal))return false;
  Handle current(CreateFileW(target.c_str(),GENERIC_READ,FILE_SHARE_READ|FILE_SHARE_DELETE,nullptr,OPEN_EXISTING,0,nullptr));BY_HANDLE_FILE_INFORMATION a{},b{};
  return current&&GetFileInformationByHandle(protectOriginal->h,&a)&&GetFileInformationByHandle(current.h,&b)&&!(b.dwFileAttributes&FILE_ATTRIBUTE_DIRECTORY)&&a.dwVolumeSerialNumber==b.dwVolumeSerialNumber&&a.nFileIndexHigh==b.nFileIndexHigh&&a.nFileIndexLow==b.nFileIndexLow;
 };
 if(!originalId.empty()){
  protectOriginal=std::make_unique<Handle>(CreateFileW(target.c_str(),GENERIC_READ,FILE_SHARE_READ|FILE_SHARE_DELETE,nullptr,OPEN_EXISTING,0,nullptr));
  if(!(*protectOriginal)||!fs::is_regular_file(target)||fileHash(protectOriginal->h,checkpoint)!=replacementHash||!matchesOriginal())throw std::runtime_error("The original file changed on disk. Its contents were not replaced.");
 }
 checkpoint();
 Lock lock(mutex);checkpoint();
 // Internal audio/video track jobs are deliberately absent from the catalog.
 // Preserve that membership state, rather than rejecting all child publication.
 if((std::find(jobs.begin(),jobs.end(),job)!=jobs.end())!=registered||job->target()!=target||str(job->data,"ReplacementOf")!=originalId||str(job->data,"ReplacementHash")!=replacementHash||str(job->data,"PreviousPath")!=previousPath)throw std::runtime_error("The download changed before publication. Its file was not replaced.");
 validateGrabberFolder(job->data);auto before=job->data,after=completeData(before,staging,hash);after["QueueMember"]=retainCompletedMembership(*this,before);
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
  auto journal=root/L"publications"/(wide(job->id())+L".json");
  if(fs::exists(journal))throw std::runtime_error("A previous publication needs recovery. Resume the download or restart UDM.");
  if(fs::exists(target))throw std::runtime_error("Cannot publish the download. The destination already exists; choose another file name.");
  Json intent={{"Id",job->id()},{"Target",utf8(target.wstring())},{"Staging",utf8(staging.wstring())},{"Hash",hash},{"Before",before},{"After",after}};
  if(!stagedIdentity.is_null())intent["StagedIdentity"]=stagedIdentity;
  atomicText(journal,intent.dump(),false);
  if(!MoveFileExW(staging.c_str(),target.c_str(),MOVEFILE_WRITE_THROUGH))throw std::runtime_error("Cannot publish the download. The file and publication recovery record were preserved.");
  job->data=after;try{save();}catch(...){job->data=before;throw;}
  std::error_code ec;fs::remove(journal,ec);return;
 }
 auto journal=root/L"replacements"/(wide(job->id())+L".json");if(fs::exists(journal))throw std::runtime_error("A previous replacement needs recovery. Restart UDM before retrying.");
 JobPtr original;for(auto item:jobs)if(item->id()==originalId)original=item;
 auto previous=fs::path(wide(str(before,"PreviousPath")));
 if(!original||isActive(original)||str(original->data,"Status")!="Complete"||original->target().lexically_normal()!=target.lexically_normal())throw std::runtime_error("The original download changed. Its file was not replaced.");
 if(!matchesOriginal())throw std::runtime_error("The original file changed on disk. Its contents were not replaced.");
 if(fs::exists(previous))throw std::runtime_error("The previous-version location is occupied. No file was replaced.");
 if(relocationReserves(previous))throw std::runtime_error("The previous-version location is reserved by a file move.");for(auto item:jobs)if(item!=job&&item!=original&&(lower(utf8(item->target().wstring()))==lower(utf8(previous.wstring()))||lower(str(item->data,"PreviousPath"))==lower(utf8(previous.wstring()))))throw std::runtime_error("The previous-version location belongs to another download.");
 auto oldBefore=original->data,oldAfter=oldBefore;oldAfter["Folder"]=utf8(previous.parent_path().wstring());oldAfter["FileName"]=utf8(previous.filename().wstring());oldAfter["PreviousVersionOf"]=job->id();oldAfter["QueueMember"]=false;oldAfter["SyncPending"]=false;oldAfter["Sha256"]=str(before,"ReplacementHash");oldAfter["Size"]=fs::file_size(target);oldAfter["Received"]=oldAfter["Size"];if(str(oldBefore,"Sha256")!=str(before,"ReplacementHash"))oldAfter.erase("ScanResult");
 if(yes(before,"SynchronizationReplacement")){after["SyncStatus"]="Updated; previous version retained";after["LastSync"]=after["Finished"];after.erase("SynchronizationReplacement");oldAfter["SyncStatus"]="Previous version";oldAfter["LastSync"]=after["Finished"];}
 after["ReplacedDownload"]=originalId;for(const char* key:{"ReplacementOf","ReplacementHash","PreviousPath"})after.erase(key);
 auto operation=Json{{"Id",job->id()},{"OriginalId",originalId},{"Target",utf8(target.wstring())},{"Previous",utf8(previous.wstring())},{"Staging",utf8(staging.wstring())},{"OldHash",str(before,"ReplacementHash")},{"NewHash",hash},{"Before",before},{"After",after},{"OriginalBefore",oldBefore},{"OriginalAfter",oldAfter}};
 atomicText(journal,operation.dump(),false);
 if(!ReplaceFileW(target.c_str(),staging.c_str(),previous.c_str(),0,nullptr,nullptr)){
  auto failure=GetLastError();
  if(matchesOriginal()&&!fs::exists(previous)){std::error_code ec;fs::remove(journal,ec);}
  throw std::runtime_error("Windows could not replace the file (error "+std::to_string(failure)+"). The original and recovery information were preserved.");
 }
 protectOriginal.reset();
 job->data=after;original->data=oldAfter;
 try{save();}catch(...){
  job->data=before;original->data=oldBefore;
  if(ReplaceFileW(target.c_str(),previous.c_str(),staging.c_str(),0,nullptr,nullptr)){std::error_code ec;fs::remove(journal,ec);}
  else throw std::runtime_error("Saving replacement history failed. Restart UDM to recover using the preserved journal and files.");
  throw;
 }
 std::error_code ec;fs::remove(journal,ec);
}
static bool recoverableStagingPath(const Json& operation,const Job& job,const fs::path& staging,const fs::path& target){
 const auto& before=operation.at("Before");
 if(yes(before,"ConfirmationPending")||fs::exists(target))return false;
 const auto stagedPath=staging.lexically_normal();
 if(stagedPath.parent_path()==target.lexically_normal().parent_path()&&
    stagedPath.filename()==L".udm-"+wide(job.id())+L".assembling")return true;
 const auto source=retainedMediaOutputPath(before);
 const auto receipt=before.find("RetainedMediaOutput");
 if(source.empty()||receipt==before.end()||job.data.value("RetainedMediaOutput",Json())!=*receipt||
    str(*receipt,"Sha256")!=str(operation,"Hash")||num(*receipt,"Size",-1)!=num(operation.at("After"),"Size",-1))return false;
 if(stagedPath==source.lexically_normal())return true;
 return stagedPath.parent_path()==target.lexically_normal().parent_path()&&
        std::regex_match(utf8(stagedPath.filename().wstring()),std::regex("\\.udm-"+job.id()+"-[0-9a-f]{32}\\.publishing"));
}
static std::optional<std::string> recoverPublicationBytes(const Json& operation,const fs::path& path,const fs::path* destination=nullptr){
 Handle file(CreateFileW(path.c_str(),GENERIC_READ,FILE_SHARE_READ|FILE_SHARE_DELETE,nullptr,OPEN_EXISTING,
                         FILE_FLAG_OPEN_REPARSE_POINT|FILE_FLAG_SEQUENTIAL_SCAN,nullptr));
 if(!file)return {};
 BY_HANDLE_FILE_INFORMATION information{};LARGE_INTEGER size{};
 if(!GetFileInformationByHandle(file.h,&information)||(information.dwFileAttributes&(FILE_ATTRIBUTE_DIRECTORY|FILE_ATTRIBUTE_REPARSE_POINT))||
    !GetFileSizeEx(file.h,&size)||size.QuadPart!=num(operation.at("After"),"Size",-1))return {};
 const auto expected=str(operation,"Hash");
 if(expected.empty()&&stableFileIdentity(file.h)!=operation.at("StagedIdentity"))return {};
 auto digest=fileHash(file.h);
 if(!expected.empty()&&digest!=expected)return {};
 // Never replace a file that appeared after the interrupted publication.
 if(destination&&!MoveFileExW(path.c_str(),destination->c_str(),MOVEFILE_WRITE_THROUGH))return {};
 return digest;
}
static void finishRetainedMediaRecovery(const Json& before,const fs::path& target){
 const auto source=retainedMediaOutputPath(before);
 if(source.empty())return;
 markZone(target);
 const auto from=source.lexically_normal().wstring(),to=target.lexically_normal().wstring();
 if(CompareStringOrdinal(from.c_str(),-1,to.c_str(),-1,TRUE)!=CSTR_EQUAL)discardRetainedMediaOutput(before);
}
void Manager::recoverReplacements(JobPtr scope){
 Lock recoveryLock(mutex);
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
   auto pending=jobs;std::unordered_set<const Job*> seen;
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
 auto publications=root/L"publications";
 if(fs::exists(publications))for(const auto& entry:fs::directory_iterator(publications)){
  if(entry.path().extension()!=L".json")continue;
  if(scope&&!scopeIds.count(utf8(entry.path().stem().wstring())))continue;
  if(catalogTransaction)throw std::runtime_error("Recover this download before starting a catalog batch. Its publication journal has been preserved.");
  auto op=Json::parse(readText(entry.path()));auto job=findPublicationJob(str(op,"Id"));
  if(!job||entry.path().stem()!=wide(job->id())||!op.contains("Before")||!op.contains("After")||str(op["Before"],"Id")!=job->id()||str(op["After"],"Id")!=job->id())throw std::runtime_error("Publication recovery requires its preserved download record.");
  auto target=fs::path(wide(str(op,"Target"))),staging=fs::path(wide(str(op,"Staging")));
  const auto hash=str(op,"Hash");
  const bool validIdentity=hash.empty()&&op.contains("StagedIdentity")&&op["StagedIdentity"].is_object();
  if(target.lexically_normal()!=job->target().lexically_normal()||target.lexically_normal()==staging.lexically_normal()||
     (!validIdentity&&!std::regex_match(hash,std::regex("[a-f0-9]{64}")))||str(op["After"],"Sha256")!=hash||
     str(op["After"],"Status")!="Complete")throw std::runtime_error("Publication recovery paths or file identity changed. Keep the files and recovery record.");
  auto before=job->data;
  std::optional<std::string> published;
  if(fs::exists(staging)){
   if(recoverableStagingPath(op,*job,staging,target)){
    validateGrabberFolder(op["Before"]);
    published=recoverPublicationBytes(op,staging,&target);
   }
  }
  else{
   published=recoverPublicationBytes(op,target);
   if(!published)throw std::runtime_error("A published download needs manual recovery. Keep the saved file and publication record.");
  }
  job->data=published?op["After"]:op["Before"];
  if(published){
   job->data["Sha256"]=*published;
   if(findJob(job->id())==job)prepareRecoveredScan(job->data,before);
  }
  else job->data["Status"]="Paused";
  try{save();}catch(...){job->data=before;throw;}
  if(published){
   if(retainedMediaOutputPath(op["Before"]).empty())markZone(target);
   else finishRetainedMediaRecovery(op["Before"],target);
  }
  fs::remove(entry.path());
 }
 // Older builds could publish and hash the final file before a catalog save
 // failed, without a publication journal. Reconcile only that exact failure.
 for(auto job:jobs){if(scope&&!scopeIds.count(job->id()))continue;const auto& data=job->data;const auto hash=lower(str(data,"Sha256"));
  if(str(data,"Status")!="Failed"||str(data,"Error").find("Could not replace the saved state (Windows error 1175).")!=0||str(data,"Finished").empty()||data.contains("OriginalResponse")||!str(data,"ReplacementOf").empty()||num(data,"Size",-1)<0||num(data,"Received")!=num(data,"Size")||!std::regex_match(hash,std::regex("[a-f0-9]{64}")))continue;
  auto expected=lower(str(data,"ExpectedSha256"));if(!expected.empty()&&expected!=hash)continue;
  std::error_code ec;auto target=job->target();Handle stableFile(CreateFileW(target.c_str(),GENERIC_READ,FILE_SHARE_READ,nullptr,OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL,nullptr));if(!stableFile||!fs::is_regular_file(target,ec)||ec)continue;auto length=fs::file_size(target,ec);if(ec||length!=static_cast<uint64_t>(num(data,"Size")))continue;
  try{if(fileHash(target)!=hash)continue;}catch(const std::exception&){continue;}
  if(catalogTransaction)throw std::runtime_error("Recover this completed file before starting a catalog batch.");
  auto before=job->data;auto after=completeData(before,target,hash);after["Finished"]=before["Finished"];after["QueueMember"]=retainCompletedMembership(*this,before);after["PublicationRecovered"]=date();job->data=std::move(after);
  prepareCompletionScan(job->data,state["Settings"]);job->data["CompletionActionArmed"]=false;
  try{save();}catch(...){job->data=before;throw;}
 }
 auto folder=root/L"replacements";if(!fs::exists(folder))return;
 for(auto& entry:fs::directory_iterator(folder)){
  if(entry.path().extension()!=L".json")continue;if(scope&&!scopeIds.count(utf8(entry.path().stem().wstring())))continue;
  if(catalogTransaction)throw std::runtime_error("Recover this download before starting a catalog batch. Its replacement journal has been preserved.");
  auto op=Json::parse(readText(entry.path()));auto job=findJob(str(op,"Id")),original=findJob(str(op,"OriginalId"));
  if(!job||!original)throw std::runtime_error("Replacement recovery requires the preserved history records.");
  if(entry.path().stem()!=wide(job->id()))throw std::runtime_error("Replacement recovery identifier does not match its record. Keep the saved files and journal.");
  if(scope&&(isActive(job)||isActive(original)))throw std::runtime_error("Wait for the replacement and its original download to stop before recovery.");
  auto target=fs::path(wide(str(op,"Target"))),previous=fs::path(wide(str(op,"Previous")));
  auto matches=[](const fs::path& path,const std::string& digest){return fs::is_regular_file(path)&&fileHash(path)==digest;};
  Json recovered,recoveredOriginal;
  const bool published=matches(target,str(op,"NewHash"))&&matches(previous,str(op,"OldHash"));
  if(published){recovered=op.at("After");recoveredOriginal=op.at("OriginalAfter");prepareRecoveredScan(recovered,job->data);}
  else if(matches(target,str(op,"OldHash"))&&!fs::exists(previous)){recovered=op.at("Before");recoveredOriginal=op.at("OriginalBefore");recovered["Status"]="Paused";}
  else if(!fs::exists(target)&&matches(previous,str(op,"OldHash"))){recovered=op.at("Before");recoveredOriginal=op.at("OriginalBefore");recovered["Status"]="Paused";if(!MoveFileExW(previous.c_str(),target.c_str(),MOVEFILE_WRITE_THROUGH))throw std::runtime_error("Windows could not restore the original file.");}
  else throw std::runtime_error("A replacement needs manual recovery. Keep the journal, original and downloaded files.");
  // Prepare both records before changing live state; swap retains rollback data
  // without allocating when a catalog write fails. Keep the journal for retry.
  job->data.swap(recovered);original->data.swap(recoveredOriginal);
  try{save();}catch(...){original->data.swap(recoveredOriginal);job->data.swap(recovered);throw;}
  if(published)finishRetainedMediaRecovery(op.at("Before"),target);
  fs::remove(entry.path());
 }
}
}
