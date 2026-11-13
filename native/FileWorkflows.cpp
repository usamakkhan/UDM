#include "Core.hpp"
#include "FileIdentity.hpp"
#include "MediaPublication.hpp"
#include "MediaStorage.hpp"
#include "OptionsModel.hpp"
#include "QueueMembership.hpp"
#include "QueueRetry.hpp"
#include "GrabberProject.hpp"
#include "GrabberRendered.hpp"
#include "CompletionPolicy.hpp"
#include "BrowserProxy.hpp"
#include "BrowserSession.hpp"
#include "Scanner.hpp"
#include <shobjidl.h>
#include <shlobj.h>
#include "GuiModels.hpp"
#include "SiteLogins.hpp"
#include "CapturePresentation.hpp"
#include "AdaptiveCapture.hpp"
#include <algorithm>
#include <limits>
namespace udm {
static std::string pathKey(const fs::path& path){return lower(utf8(fs::absolute(path).lexically_normal().wstring()));}
struct RelocationReporter {
 const Cancel* cancellation;const RelocationObserver& observer;std::atomic_bool copyCancelled{false};const Cancel* managedCancellation=nullptr;bool restoring=false;
 bool cancelled()const{return (cancellation&&cancellation->cancelled())||(managedCancellation&&managedCancellation->cancelled());}
 void check()const{if(cancelled())throw RelocationCancelled();}
 void report(RelocationPhase phase,i64 completed=0,i64 total=0,bool cancellable=false)const noexcept{
  if(observer)try{observer({phase,completed,total,cancellable});}catch(...){}
 }
 static DWORD CALLBACK copyProgress(LARGE_INTEGER total,LARGE_INTEGER done,LARGE_INTEGER,LARGE_INTEGER,DWORD,DWORD,HANDLE,HANDLE,LPVOID context)noexcept{
  auto& self=*static_cast<RelocationReporter*>(context);
  self.report(self.restoring?RelocationPhase::RestoringOriginal:RelocationPhase::Moving,done.QuadPart,total.QuadPart,!self.restoring);
  if(!self.restoring&&self.cancelled()){self.copyCancelled=true;return PROGRESS_CANCEL;}
  return PROGRESS_CONTINUE;
 }
};
struct RelocationRelease {std::function<void()> release;~RelocationRelease(){release();}};
bool Manager::relocationReserves(const fs::path& path)const{
 Lock lock(mutex);if(relocationId.empty())return false;
 const auto candidate=lower(utf8(fs::absolute(path).lexically_normal().generic_wstring()));
 const auto reserved=lower(utf8(fs::absolute(relocationDestination).lexically_normal().generic_wstring()));
 return candidate==reserved||candidate.rfind(reserved+"/",0)==0;
}
static bool matchesMoveFileIdentity(const fs::path& path,const Json& expected){
 Handle file(CreateFileW(path.c_str(),FILE_READ_ATTRIBUTES,FILE_SHARE_READ|FILE_SHARE_WRITE|FILE_SHARE_DELETE,nullptr,OPEN_EXISTING,FILE_FLAG_OPEN_REPARSE_POINT,nullptr));
 return file&&expected.is_object()&&stableFileIdentity(file.h)==expected;
}
static std::optional<std::wstring> storedFileName(const fs::path& path){
 WIN32_FIND_DATAW data{};const auto handle=FindFirstFileW(path.c_str(),&data);
 if(handle==INVALID_HANDLE_VALUE){const auto error=GetLastError();if(error==ERROR_FILE_NOT_FOUND||error==ERROR_PATH_NOT_FOUND)return {};throw std::runtime_error("Windows could not inspect the saved file name (error "+std::to_string(error)+").");}
 FindClose(handle);return std::wstring(data.cFileName);
}
static void applyDownloadLogin(Json& data,Json& settings,const std::string& user,const std::string& password,bool remember,bool enabled){
 if(capturedMedia(data))throw std::runtime_error("Sign in to this video site in the browser, then capture its streams again.");
 const auto origin=Url(str(data,"Url")).origin;
 const auto challenge=str(data,"AuthenticationOrigin");
 if(!challenge.empty()&&challenge!=origin)throw std::runtime_error("The login challenge came from a different site. Open the download page and obtain its direct link first.");
 if(remember&&!siteLoginScheme(Url(origin).scheme))throw std::runtime_error("Saved logins support HTTP, HTTPS and FTP sites.");
 auto headers=readHeaders(data);const auto previousHeaders=headers;setBasicLogin(headers,user,password,enabled);validateHeaders(headers);
 data["ProtectedHeaders"]=headers.empty()?"":protect(legacyDictionary(Json(headers)).dump());
 data["AuthenticationPromptPending"]=false;
 if(headers!=previousHeaders){
  data.erase("BrowserReportedDirectSize");data.erase("BrowserReportedFileName");
  data.erase("ResponseFileName");data.erase("ResponseFileNameUrl");
 }
 if(remember&&enabled)rememberSiteLogin(settings,makeSiteLogin(origin,user,password));
}
void Manager::setDownloadLogin(JobPtr job,const std::string& user,const std::string& password,bool remember,bool enabled){
 Lock lock(mutex);if(!job||isActive(job))throw std::runtime_error("Stop this download before changing its login.");
 auto next=job->data,settings=state["Settings"];
 applyDownloadLogin(next,settings,user,password,remember,enabled);
 job->data.swap(next);state["Settings"].swap(settings);
 try{save();}catch(...){job->data.swap(next);state["Settings"].swap(settings);throw;}
}

void Manager::finishStoppedFileInfo(JobPtr job,const Json& edit,const FileInfoFolderChoice& folderChoice,const std::optional<FileInfoLogin>& login,bool defer,const DownloadQueueChoice* queueChoice){
 Lock lock(mutex);
 if(!job||std::find(jobs.begin(),jobs.end(),job)==jobs.end())throw std::runtime_error("This download is no longer in the list.");
 recoverBeforeConfigure(job,!defer);
 if(!defer&&!isActive(job)){
  const auto response=job->data.find("OriginalResponse");
  if(response!=job->data.end()){
   const auto phase=str(*response,"State");
   if(phase!="Ready"&&phase!="Publishing"&&phase!="Published")recoverOriginalResponses();
  }
 }
 auto queues=queuesWithDownloadDraft(str(edit,"Queue"),defer?queueChoice:nullptr);
 if(queues)state["Queues"].swap(*queues);
 try{
  auto next=configuredDownload(job,edit,false),settings=state["Settings"];
  rememberFileInfoDestination(settings,str(next,"Category"),fs::path(wide(str(next,"Folder"))),folderChoice);
  if(defer&&queueChoice&&queueChoice->dontAsk)settings["QueuePromptLater"]=false;
  validateOptionsModel(settings);
  if(login)applyDownloadLogin(next,settings,login->user,login->password,login->remember,login->enabled);
  if(defer){
   validateQueueMembership(*this,job,true,str(next,"Queue"));
   next["QueueMember"]=true;
  }
  const auto response=next.find("OriginalResponse");
  const auto responseState=response==next.end()?std::string{}:str(*response,"State");
  const bool publishCapturedResponse=!defer&&(responseState=="Ready"||responseState=="Publishing");
  if(!defer&&response!=next.end()&&responseState!="Published"&&!publishCapturedResponse)
   stageOriginalResponseResume(job,next);
  job->data.swap(next);state["Settings"].swap(settings);
  try{
   if(defer)pause(job); // Save the edited record with the existing durable pause transition.
   else if(publishCapturedResponse)startOriginalResponseSave(job,false,true);
   else queueIndividualResume(job);
  }catch(...){job->data.swap(next);state["Settings"].swap(settings);throw;}
 }catch(...){if(queues)state["Queues"].swap(*queues);throw;}
 if(edit.contains("LimitKbps"))job->sessionLimit.reset();
}

JobPtr Manager::addScheduledDownload(const std::string& address,const fs::path& target,const std::string& category,
                                    const std::string& description,const Headers& headers,
                                    const DownloadQueueChoice& choice,const FileInfoFolderChoice& folderChoice,
                                    std::optional<i64> connections,i64 speedLimit){
 Lock lock(mutex);
 if(catalogTransaction)throw std::runtime_error("Another catalog operation is in progress.");
 if(!choice.create||!choice.window)throw std::runtime_error("Choose a one-time schedule.");
 const auto folder=target.parent_path();
 const auto filename=utf8(target.filename().wstring());
 if(!folder.is_absolute()||safeName(filename)!=filename)
  throw std::runtime_error("Choose a complete destination and valid file name.");
 auto queues=queuesWithDownloadDraft(choice.name,&choice);
 auto settings=state["Settings"];
 if(!activeCategory(settings,category))throw std::runtime_error("The selected category no longer exists.");
 rememberFileInfoDestination(settings,category,folder,folderChoice);
 if(!folderChoice.rememberedCategoryFolder)settings["CategoryRememberLast"]=state["Settings"].at("CategoryRememberLast");
 validateOptionsModel(settings);
 const auto count=jobs.size();
 JobPtr job;
 state["Queues"].swap(*queues);
 state["Settings"].swap(settings);
 catalogTransaction=true;
 try{
  job=add(address,utf8(folder.wstring()),filename,choice.name,true,headers);
  if(str(job->data,"FileName")!=filename)
   throw std::runtime_error("That destination already exists. Edit the destination to choose another file name.");
  Json edit={{"Category",category},{"CategoryExplicit",true},{"Description",description},{"LimitKbps",speedLimit}};
  if(connections)edit["Connections"]=*connections;
  job->data=configuredDownload(job,edit,false);
  catalogTransaction=false;
  save();
 }catch(...){
  catalogTransaction=false;
  jobs.resize(count);
  state["Queues"].swap(*queues);
  state["Settings"].swap(settings);
  throw;
 }
 updateWakeTimer(epoch());
 return job;
}

std::vector<JobPtr> Manager::offerDownloadBatch(const std::vector<BatchDownloadEntry>& entries,const std::string& queue,bool paused,const std::string& category,const DownloadQueueChoice* choice){
 Lock lock(mutex);
 if(entries.empty())throw std::runtime_error("Select at least one file.");
 if(catalogTransaction)throw std::runtime_error("Another catalog operation is in progress.");
 auto queues=queuesWithDownloadDraft(queue,choice);
 if(!queues&&std::none_of(state["Queues"].begin(),state["Queues"].end(),[&](const Json& row){return str(row,"Name")==queue;}))
  throw std::runtime_error("The selected queue no longer exists.");
 if(!category.empty()&&!activeCategory(state["Settings"],category))
  throw std::runtime_error("The selected category no longer exists. Reopen import to choose a current category.");
 const auto categoryDestination=category.empty()?std::string{}:categoryFolder(state["Settings"],category);
 struct Before {JobPtr job;Json data;std::vector<Worker> workers;double speed;SpeedMeter meter;std::optional<i64> limit;};
 std::vector<Before> before;
 std::set<const Job*> known;
 if(str(state["Settings"],"DuplicatePolicy","Ask")=="Replace"){
  recoverRestarts();
  before.reserve(jobs.size());
  for(const auto& job:jobs)before.push_back({job,job->data,job->workers,job->speed,job->speedMeter,job->sessionLimit});
 }
 for(const auto& job:jobs)known.insert(job.get());
 auto previousJobs=jobs;
 auto previousOffers=existingOffers;
 auto captures=state.value("BrowserCaptures",Json());
 auto previousSettings=state["Settings"];
 std::vector<JobPtr> admitted;
 admitted.reserve(entries.size());
 if(queues)state["Queues"].swap(*queues);
 catalogTransaction=true;
 try{
  for(const auto& entry:entries){
   const auto& folder=entry.folder.empty()?categoryDestination:entry.folder;
   JobPtr job;
   if(entry.media.empty())job=offerDownload(entry.address,folder,entry.name,queue,paused,entry.headers,Json::object(),entry.browserProxy,entry.browserSession);
   else{
    if(str(entry.media,"url")!=entry.address)throw std::runtime_error("The streaming source cannot be edited.");
    if(str(entry.media,"action")=="adaptive"){
     requireAdaptiveTools();
     const auto capture=validateAdaptiveCapture(entry.media);
     job=add(entry.address,folder,adaptiveCaptureName(entry.name,capture.plan),queue,paused,capture.headers);
     applyAdaptiveCapture(*job,capture,capture.plan.dump());
    }else{
     validateYouTubeVideoCapture(entry.media);
     job=add(entry.address,folder,youTubeVideoCaptureName(entry.name),queue,paused,browserHeaders(entry.media));
     applyYouTubeVideoCapture(*this,job,entry.media);
    }
   }
   admitted.push_back(job);
   if(!known.insert(job.get()).second){
    if(!entry.downloadPage.empty()&&
       (readBrowserProxy(job->data)!=entry.browserProxy||readBrowserSession(job->data)!=entry.browserSession))
     throw std::runtime_error("An existing download uses another browser proxy route or website session. Pause it and capture a numbered copy.");
    continue;
   }
   Json changes=Json::object();
   if(!entry.description.empty())changes["Description"]=entry.description;
   if(!entry.downloadPage.empty())changes["DownloadPage"]=entry.downloadPage;
   if(!category.empty()){changes["Category"]=category;changes["CategoryExplicit"]=true;}
   if(!changes.empty())configure(job,changes);
  }
  if(choice&&choice->dontAsk)state["Settings"]["QueuePromptBatch"]=false;
  catalogTransaction=false;
  save();
 }catch(...){
  catalogTransaction=false;
  jobs.swap(previousJobs);existingOffers.swap(previousOffers);
  state["Settings"].swap(previousSettings);
  if(queues)state["Queues"].swap(*queues);
  restoreCaptureRows(state,captures);
  for(auto& old:before){old.job->data.swap(old.data);old.job->workers.swap(old.workers);old.job->speed=old.speed;std::swap(old.job->speedMeter,old.meter);old.job->sessionLimit=old.limit;}
  if(!before.empty())recoverRestarts();
  throw;
 }
 for(const auto& old:before)
  if(str(old.data,"RestartAttempt")!=str(old.job->data,"RestartAttempt"))discardRetainedMediaOutput(old.data);
 if(!before.empty())recoverRestarts();
 return admitted;
}

// Called under the catalog lock after removal has committed.
void Manager::discardCancelledPrefetch(const JobPtr& removed) noexcept{
 try{
  auto rangeFolder=[this](const JobPtr& job){
   const auto saved=str(job->data,"PartsFolder");
   return saved.empty()?root/L"parts"/wide(job->id()):fs::path(wide(saved));
  };
  std::set<std::string> occupied,occupiedMedia,protectedFiles{pathKey(removed->target())};
  auto pending=jobs;
  std::set<const Job*> seen;
  while(!pending.empty()){
   auto job=pending.back();pending.pop_back();
   if(!seen.insert(job.get()).second)continue;
   occupied.insert(pathKey(rangeFolder(job)));
   const auto mediaFolder=str(job->data,"MediaFolder");
   occupiedMedia.insert(pathKey(mediaFolder.empty()?root/L"media"/wide(job->id()):fs::path(wide(mediaFolder))));
   protectedFiles.insert(pathKey(job->target()));
   if(job->video)pending.push_back(job->video);
   if(job->audio)pending.push_back(job->audio);
  }
  // Lease each directory from the root so the checked path cannot be redirected.
  auto leaseDirectory=[](const fs::path& folder)->std::vector<Handle>{
   std::vector<fs::path> ancestors;
   for(auto parent=folder;;parent=parent.parent_path()){
    ancestors.push_back(parent);
    if(parent==parent.root_path()||parent.parent_path().empty())break;
   }
   std::vector<Handle> leases;
   for(auto parent=ancestors.rbegin();parent!=ancestors.rend();++parent){
    Handle handle(CreateFileW(parent->c_str(),FILE_READ_ATTRIBUTES,FILE_SHARE_READ|FILE_SHARE_WRITE,nullptr,
                             OPEN_EXISTING,FILE_FLAG_BACKUP_SEMANTICS|FILE_FLAG_OPEN_REPARSE_POINT,nullptr));
    FILE_ATTRIBUTE_TAG_INFO attributes{};
    if(!handle||!GetFileInformationByHandleEx(handle.h,FileAttributeTagInfo,&attributes,sizeof(attributes))||
       !(attributes.FileAttributes&FILE_ATTRIBUTE_DIRECTORY)||(attributes.FileAttributes&FILE_ATTRIBUTE_REPARSE_POINT))break;
    leases.push_back(std::move(handle));
   }
   if(leases.size()!=ancestors.size())return {};
   return leases;
  };
  auto discard=[&](const fs::path& path){
   if(protectedFiles.count(pathKey(path))||relocationReserves(path))return;
   Handle file(CreateFileW(path.c_str(),DELETE|FILE_READ_ATTRIBUTES,FILE_SHARE_READ|FILE_SHARE_DELETE,
                          nullptr,OPEN_EXISTING,FILE_FLAG_OPEN_REPARSE_POINT,nullptr));
   FILE_ATTRIBUTE_TAG_INFO attributes{};
   if(!file||!GetFileInformationByHandleEx(file.h,FileAttributeTagInfo,&attributes,sizeof(attributes))||
      (attributes.FileAttributes&(FILE_ATTRIBUTE_REPARSE_POINT|FILE_ATTRIBUTE_DIRECTORY)))return;
   FILE_DISPOSITION_INFO disposition{TRUE};
   SetFileInformationByHandle(file.h,FileDispositionInfo,&disposition,sizeof(disposition));
  };
  const std::regex validId("[0-9a-f]{32}");
  for(const auto& job:{removed,removed->video,removed->audio}){
   if(!job)continue;
   const auto id=job->id();
   if(!std::regex_match(id,validId))continue;
   const auto folder=rangeFolder(job);
   if(!folder.is_absolute()||folder.lexically_normal()!=folder||folder.filename()!=wide(id))continue;
   if(folder.parent_path()!=root/L"parts"&&folder.parent_path().filename()!=L"UDM-parts")continue;
   if(occupied.count(pathKey(folder)))continue;

   const auto leases=leaseDirectory(folder);
   if(leases.empty())continue;
   discard(folder/(wide(id)+L".data"));
   for(const auto& segment:job->data.value("Segments",Json::array())){
    const auto index=num(segment,"Index",-1);
    if(index<0||index>99999999)continue;
    wchar_t name[32];swprintf_s(name,L"%04lld.part",index);
    discard(folder/name);
   }
  }
  const bool streaming=!str(removed->data,"ProtectedSabr").empty();
  const auto id=removed->id();
  const auto segments=num(removed->data,"AdaptiveTotalSegments");
  if(yes(removed->data,"LiveRecording")||!std::regex_match(id,validId))return;
  if(!streaming&&(str(removed->data,"ProtectedAdaptive").empty()||segments<1||segments>10000))return;
  const auto mediaFolder=fs::path(wide(str(removed->data,"MediaFolder")));
  if(!mediaFolder.is_absolute()||mediaFolder.lexically_normal()!=mediaFolder||mediaFolder.filename()!=wide(id))return;
  if(mediaFolder.parent_path()!=root/L"media"&&mediaFolder.parent_path().filename()!=L"UDM-media")return;
  const auto folderKey=pathKey(mediaFolder);
  if(occupiedMedia.count(folderKey)||occupied.count(folderKey))return;
  if(streaming){
   const auto leases=leaseDirectory(mediaFolder);
   if(leases.empty())return;
   // Match only the files created by the streaming cache and track assembler.
   static const std::wregex segmentFile(
    LR"((retained-(video|audio)-(init|[0-9]{1,10})\.sabr|(video|audio)-(w[1-8]-)?(init|[0-9]{1,10})\.sabrpart(\.decoded)?))");
   for(const auto& entry:fs::directory_iterator(mediaFolder)){
    if(std::regex_match(entry.path().filename().wstring(),segmentFile))discard(entry.path());
   }
   for(const auto* name:{L"stream-cache.json",L"stream-cache.json.tmp",L"stream-cache.json.bak"})
    discard(mediaFolder/name);
   for(const auto& child:{removed->video,removed->audio})if(child){
    const auto track=mediaFolder/(child==removed->video?L"video.mp4":L"audio.mp4");
    if(child->target()!=track)continue;
    discard(track);
    auto assembling=track;assembling+=L".assembling";
    discard(assembling);
   }
   return;
  }
  const auto adaptive=mediaFolder/L"adaptive";
  const auto leases=leaseDirectory(adaptive);
  if(leases.empty())return;
  const std::wregex cacheFile(LR"((0|[1-9][0-9]{0,3})\.(part(\.tmp|\.refresh\.tmp)?|receipt\.json(\.tmp|\.bak)?|prefix\.json(\.tmp)?))");
  for(const auto& entry:fs::directory_iterator(adaptive)){
   const auto name=entry.path().filename().wstring();
   std::wsmatch match;
   if(std::regex_match(name,match,cacheFile)&&std::stoll(match[1].str())<segments)discard(entry.path());
  }
  for(const auto* name:{L"completed.json",L"completed.json.tmp",L"completed.json.bak"})discard(adaptive/name);
 }catch(...){/* Scratch cleanup cannot undo a saved cancellation. */}
}

void Manager::recoverFileOperation(JobPtr scope){
 Lock lock(mutex);
 auto journal=root/L"file-operation.json";if(!fs::exists(journal))return;
 auto operation=Json::parse(readText(journal));
 if(scope&&scope->id()!=str(operation,"Id"))return;
 if(!relocationId.empty())throw std::runtime_error("A file move is still running. Wait for it to finish before recovering its journal.");
 if(catalogTransaction)throw std::runtime_error("Finish the catalog operation before recovering a file move.");
 JobPtr job;for(auto j:jobs)if(j->id()==str(operation,"Id"))job=j;
 if(!job)throw std::runtime_error("A pending file move refers to missing history. Keep file-operation.json for recovery.");
 auto source=fs::path(wide(str(operation,"Source"))),dest=fs::path(wide(str(operation,"Destination")));
 const auto expectedHash=str(operation,"Hash",str(operation.at("Before"),"Sha256"));
 const auto identity=operation.value("FileIdentity",Json());
 auto verified=[&](const fs::path& path){return identity.is_object()?matchesMoveFileIdentity(path,identity):!expectedHash.empty()&&fs::is_regular_file(path)&&fileHash(path)==expectedHash;};
 Json recovered;
 if(yes(operation,"CaseOnly")){
  if(pathKey(source)!=pathKey(dest))throw std::runtime_error("Invalid case-only rename journal. Preserve it for recovery.");
  const auto oldName=wide(str(operation,"SourceFileName"));
  const auto atSource=storedFileName(source),atDestination=storedFileName(dest);
  // Existence is insufficient on a case-insensitive volume: both spellings
  // resolve after the rename. Match the directory entry and recorded identity.
  if(atDestination&&*atDestination==dest.filename().wstring()&&verified(dest)){
   if(atSource&&*atSource!=*atDestination)throw std::runtime_error("Both case-variant files exist. Preserve the files and rename journal for recovery.");
   recovered=operation.at("After");
  }else if(atSource&&*atSource==oldName&&verified(source)){
   if(atDestination&&*atDestination!=*atSource)throw std::runtime_error("Both case-variant files exist. Preserve the files and rename journal for recovery.");
   recovered=operation.at("Before");
  }else throw std::runtime_error("The case-only rename could not be verified. Files and journal were preserved.");
 }
 else if(fs::exists(source)){
  if(!verified(source))throw std::runtime_error("The original file changed during a pending move. Files and journal were preserved for recovery.");
  recovered=operation.at("Before");
 }
 else if(verified(dest)){recovered=operation.at("After");}
 else throw std::runtime_error("A pending file move needs recovery. Neither location could be verified; files and journal were preserved.");
 if(yes(operation,"PathOnly")){auto current=job->data;current["Folder"]=recovered.at("Folder");current["FileName"]=recovered.at("FileName");recovered=std::move(current);}
 job->data.swap(recovered);try{save();}catch(...){job->data.swap(recovered);throw;}fs::remove(journal);
}
void Manager::relocate(JobPtr job,const fs::path& requested,const Cancel* cancellation,const RelocationObserver& observer){
 RelocationReporter reporter{cancellation,observer};reporter.check();reporter.report(RelocationPhase::Preparing);
 std::unique_lock<std::recursive_mutex> lock(mutex);if(stopping)throw std::runtime_error("UDM is closing.");if(!relocationId.empty())throw std::runtime_error("Another file move is in progress.");if(!job||std::find(jobs.begin(),jobs.end(),job)==jobs.end()||isActive(job)||str(job->data,"Status")!="Complete")throw std::runtime_error("Move/Rename is available for completed downloads.");if(catalogTransaction)throw std::runtime_error("Finish the catalog operation before moving a file.");if(fs::exists(root/L"file-operation.json"))recoverFileOperation();
 recoverReplacements(job);
 if(str(job->data,"Status")!="Complete")throw std::runtime_error("Recovery changed this download's status. Review it before moving its file.");
 for(auto other:jobs)if(str(other->data,"ReplacementOf")==job->id())throw std::runtime_error("A replacement still depends on this file. Remove the pending replacement before moving or renaming it.");
 if(!requested.is_absolute()||safeName(utf8(requested.filename().wstring()))!=utf8(requested.filename().wstring()))throw std::runtime_error("Choose an absolute destination with a valid file name.");
 auto source=job->target(),dest=requested.lexically_normal();
 const bool caseOnly=pathKey(source)==pathKey(dest)&&fs::exists(dest.parent_path())&&fs::equivalent(source.parent_path(),dest.parent_path());
 if(caseOnly&&source.filename()==dest.filename()){reporter.check();reporter.report(RelocationPhase::Complete);return;}
 if(!fs::is_regular_file(source))throw std::runtime_error("The downloaded file is missing from its saved location.");
 std::optional<std::wstring> sourceName;
 if(caseOnly){
  sourceName=storedFileName(source);if(!sourceName)throw std::runtime_error("The downloaded file disappeared before renaming.");
  // Rename the leaf only; do not claim to rename parent directories.
  dest=source.parent_path()/dest.filename();source=source.parent_path()/ *sourceName;
 }
 if(fs::exists(dest)&&(!caseOnly||storedFileName(dest)!=sourceName||!fs::equivalent(source,dest)))throw std::runtime_error("Destination already exists. Choose another name; no files were overwritten.");
 for(auto other:jobs)if(other!=job&&(pathKey(other->target())==pathKey(dest)||(!str(other->data,"PreviousPath").empty()&&pathKey(fs::path(wide(str(other->data,"PreviousPath"))))==pathKey(dest))))throw std::runtime_error("That destination belongs to another download.");
 const auto destinationPrefix=lower(utf8(fs::absolute(dest).lexically_normal().generic_wstring()))+"/";
 for(auto other:jobs)if(other!=job){
  if(lower(utf8(fs::absolute(other->target()).lexically_normal().generic_wstring())).rfind(destinationPrefix,0)==0||
     (!str(other->data,"PreviousPath").empty()&&lower(utf8(fs::absolute(fs::path(wide(str(other->data,"PreviousPath")))).lexically_normal().generic_wstring())).rfind(destinationPrefix,0)==0))throw std::runtime_error("That destination is needed as a folder by another download.");
 }
 auto before=job->data,after=before;after["Folder"]=utf8(dest.parent_path().wstring());after["FileName"]=utf8(dest.filename().wstring());
 fs::create_directories(dest.parent_path());auto journal=root/L"file-operation.json";
 const auto operationId=job->id();auto managedCancel=std::make_shared<Cancel>();bool reserved=false;
 RelocationRelease release{[&]{if(!reserved)return;if(!lock.owns_lock())lock.lock();active.erase(operationId);schedulePaused.erase(operationId);relocationId.clear();relocationDestination.clear();relocationFinished.notify_all();}};
 // Enrol under the manager lock before releasing it for filesystem work. The
 // active token protects the job; the destination reservation protects its path.
 reserved=true;relocationId=operationId;relocationDestination=dest;
 try{active.emplace(operationId,managedCancel);}catch(...){relocationId.clear();relocationDestination.clear();throw;}
 reporter.managedCancellation=managedCancel.get();lock.unlock();
 // A same-volume rename preserves the file object. Hold a read handle that
 // denies writes while permitting rename, avoiding content scans for this path.
 Handle sourceLease(CreateFileW(source.c_str(),GENERIC_READ,FILE_SHARE_READ|FILE_SHARE_DELETE,nullptr,OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL,nullptr));
 if(!sourceLease)throw std::runtime_error("Windows could not open the file for a protected move (error "+std::to_string(GetLastError())+"). Close programs editing it and retry.");
 auto identity=stableFileIdentity(sourceLease.h);
 Handle parent(CreateFileW(dest.parent_path().c_str(),FILE_READ_ATTRIBUTES,FILE_SHARE_READ|FILE_SHARE_WRITE|FILE_SHARE_DELETE,nullptr,OPEN_EXISTING,FILE_FLAG_BACKUP_SEMANTICS,nullptr));
 FILE_ID_INFO parentId{};const auto sourceAttributes=GetFileAttributesW(source.c_str());
 if(!identity.is_object()||sourceAttributes==INVALID_FILE_ATTRIBUTES||(sourceAttributes&FILE_ATTRIBUTE_REPARSE_POINT)||!parent||
    !GetFileInformationByHandleEx(parent.h,FileIdInfo,&parentId,sizeof(parentId))||identity["Volume"]!=Json(parentId.VolumeSerialNumber))identity=Json();
 // Cross-volume copies and filesystems without stable IDs retain fresh hashes.
 reporter.check();reporter.report(RelocationPhase::VerifyingSource,0,0,true);
 const auto moveHash=identity.is_object()?std::string{}:fileHash(sourceLease.h,[&]{
  reporter.check();LARGE_INTEGER zero{},position{},size{};
  if(SetFilePointerEx(sourceLease.h,zero,&position,FILE_CURRENT)&&GetFileSizeEx(sourceLease.h,&size))reporter.report(RelocationPhase::VerifyingSource,position.QuadPart,size.QuadPart,true);
 });
 if(!identity.is_object()){CloseHandle(sourceLease.h);sourceLease.h=INVALID_HANDLE_VALUE;}
 auto verified=[&](const fs::path& path){
  if(identity.is_object())return matchesMoveFileIdentity(path,identity);
  if(!fs::is_regular_file(path))return false;
  Handle file(CreateFileW(path.c_str(),GENERIC_READ,FILE_SHARE_READ|FILE_SHARE_DELETE,nullptr,OPEN_EXISTING,FILE_FLAG_SEQUENTIAL_SCAN,nullptr));
  if(!file)throw std::runtime_error("Cannot open the moved file for verification.");
  const auto phase=path==source?RelocationPhase::VerifyingOriginal:RelocationPhase::VerifyingDestination;
  LARGE_INTEGER size{};if(!GetFileSizeEx(file.h,&size))throw std::runtime_error("Cannot read the moved file size.");
  reporter.report(phase,0,size.QuadPart);
  const auto hash=fileHash(file.h,[&]{LARGE_INTEGER zero{},position{};if(SetFilePointerEx(file.h,zero,&position,FILE_CURRENT))reporter.report(phase,position.QuadPart,size.QuadPart);});
  if(hash!=moveHash)return false;
  reporter.report(phase,size.QuadPart,size.QuadPart);return true;
 };
 auto originalLocationIntact=[&]{
  if(!verified(source))return false;
  if(!caseOnly)return !fs::exists(dest);
  return storedFileName(source)==sourceName&&(!fs::exists(dest)||(storedFileName(dest)==sourceName&&fs::equivalent(source,dest)));
 };
 if(identity.is_object()&&!verified(source))throw std::runtime_error("The source file changed before moving. No move was started.");
 reporter.check();
 atomicText(journal,Json{{"Id",operationId},{"Source",utf8(source.wstring())},{"Destination",utf8(dest.wstring())},{"Before",before},{"After",after},{"CaseOnly",caseOnly},{"PathOnly",true},{"SourceFileName",caseOnly?utf8(*sourceName):std::string{}},{"Hash",moveHash},{"FileIdentity",identity}}.dump(),false);
 reporter.report(RelocationPhase::Moving,0,0,true);
 if(reporter.cancelled()){
  std::error_code ec;fs::remove(journal,ec);
  if(ec)throw std::runtime_error("The move was cancelled before copying, but its journal could not be removed. Preserve it for recovery.");
  throw RelocationCancelled();
 }
 if(!MoveFileWithProgressW(source.c_str(),dest.c_str(),RelocationReporter::copyProgress,&reporter,MOVEFILE_COPY_ALLOWED|MOVEFILE_WRITE_THROUGH)){
  const auto failure=GetLastError();bool unchanged=false;
  reporter.report(RelocationPhase::VerifyingOriginal);
  try{unchanged=originalLocationIntact();}catch(...){}
  bool cleared=false;if(unchanged){std::error_code ec;fs::remove(journal,ec);cleared=!ec;}
  if(failure==ERROR_REQUEST_ABORTED&&reporter.copyCancelled&&unchanged&&cleared)throw RelocationCancelled();
  throw std::runtime_error("Windows could not move the file (error "+std::to_string(failure)+")."+(cleared?"":" Files and recovery journal were preserved."));
 }
 // The file has moved. Finish verification/history or rollback even if a late
 // cancellation arrives; never report cancellation with history at a stale path.
 reporter.report(RelocationPhase::VerifyingDestination);
 if(caseOnly&&storedFileName(dest)!=std::optional<std::wstring>{dest.filename().wstring()})throw std::runtime_error("Windows did not confirm the requested filename case. The rename journal was preserved for recovery.");
 if(!verified(dest))throw std::runtime_error("The moved file could not be verified. Files and recovery journal were preserved.");
 // COPY_ALLOWED can report success even when Windows cannot delete the source.
 if(!caseOnly&&fs::exists(source))throw std::runtime_error("Windows copied the file but could not remove the original. History still points to the original; both files and the recovery journal were preserved.");
 lock.lock();
 auto commitBefore=job->data;job->data["Folder"]=after.at("Folder");job->data["FileName"]=after.at("FileName");
 reporter.report(RelocationPhase::SavingHistory);
 try{save();}catch(...){
  reporter.report(RelocationPhase::RestoringOriginal);
  job->data=commitBefore;bool restored=false;lock.unlock();
  reporter.restoring=true;
  try{restored=MoveFileWithProgressW(dest.c_str(),source.c_str(),RelocationReporter::copyProgress,&reporter,MOVEFILE_COPY_ALLOWED|MOVEFILE_WRITE_THROUGH)&&originalLocationIntact();}catch(...){}
  lock.lock();if(restored){std::error_code ec;fs::remove(journal,ec);}
  else{job->data["Folder"]=after.at("Folder");job->data["FileName"]=after.at("FileName");throw std::runtime_error("The file moved, but saving history or restoring its original location failed. Files and recovery journal were preserved.");}
  throw;
 }
 std::error_code ec;fs::remove(journal,ec);
 reporter.report(RelocationPhase::Complete);
}
void Manager::updateCompleted(JobPtr job,const Json& edit){
 Lock lock(mutex);if(!job||isActive(job)||str(job->data,"Status")!="Complete")throw std::runtime_error("Select a completed download.");
 auto before=job->data,next=before;if(capturedMedia(before)&&edit.contains("Url")&&str(edit,"Url")!=str(before,"Url"))throw std::runtime_error("Refresh video streams using the browser panel.");for(const char* key:{"Url","Description","DownloadPage","ProtectedHeaders","Category","Queue","QueueMember","Connections","LimitKbps","ExpectedSha256","SuppressCompletionDialog"})if(edit.contains(key))next[key]=edit[key];
 if(str(next,"Url")!=str(before,"Url")){next.erase("ProtectedResolvedUrl");next.erase("AuthenticationOrigin");next.erase("AuthenticationScheme");next["AuthenticationPromptPending"]=false;}
 Url url(str(next,"Url"));if(!str(next,"DownloadPage").empty()){Url page(str(next,"DownloadPage"));if(page.scheme!="http"&&page.scheme!="https")throw std::runtime_error("The parent page must use HTTP or HTTPS.");}
 if(url.origin!=Url(str(before,"Url")).origin){auto headers=readHeaders(next);for(auto it=headers.begin();it!=headers.end();)if(lower(it->first)=="authorization"||lower(it->first)=="cookie"||lower(it->first)=="referer")it=headers.erase(it);else ++it;next["ProtectedHeaders"]=headers.empty()?"":protect(legacyDictionary(Json(headers)).dump());}
 validateFileMetadata(next);auto cats=categories();if(std::find(cats.begin(),cats.end(),str(next,"Category"))==cats.end())throw std::runtime_error("Choose an existing category.");bool found=false;for(auto& q:state["Queues"])found|=str(q,"Name")==str(next,"Queue");if(!found)throw std::runtime_error("Choose an existing queue.");if(yes(next,"QueueMember")!=yes(before,"QueueMember")||str(next,"Queue")!=str(before,"Queue"))next["SyncPending"]=false;job->data=next;try{validateQueueMembership(*this,job,yes(next,"QueueMember"),str(next,"Queue"));save();}catch(...){job->data=before;throw;}
}
JobPtr Manager::redownload(JobPtr job){
 Lock lock(mutex);if(!job||std::find(jobs.begin(),jobs.end(),job)==jobs.end()||isActive(job)||(str(job->data,"Status")!="Complete"&&!((str(job->data,"Status")=="Paused"||str(job->data,"Status")=="Failed")&&Url(str(job->data,"Url")).scheme=="ftp")))throw std::runtime_error("Select a completed download or a stopped FTP download to download again.");
 recoverFileOperation(job);
 if(yes(job->data,"RequiresRequestCapture"))throw std::runtime_error("This form download needs its original request. Submit the download form again in your browser.");
 if(yes(job->data,"RequiresMediaCapture")||!str(job->data,"SourceUrl").empty()||!str(job->data,"ProtectedAdaptive").empty())throw std::runtime_error("Choose fresh video streams using the browser panel.");
 readBrowserSession(job->data);
 const bool renderedArchive=job->data.contains("OfflineProject")&&!str(job->data["OfflineProject"],"RenderedTicket").empty();
 if(renderedArchive){
  readBrowserSession(job->data);
  if(str(job->data,"ProtectedRenderedPage").empty())throw std::runtime_error("This archive needs its captured page. Capture the browser page again and create a new archive.");
  auto page=validateRenderedGrabberSnapshot(Json::parse(reveal(str(job->data,"ProtectedRenderedPage"))),str(job->data["OfflineProject"],"StartUrl"));
  if(!page.contains("html"))throw std::runtime_error("The saved rendered page has no offline HTML.");
 }
 if(catalogTransaction)throw std::runtime_error("Another catalog operation is in progress.");
 auto createCopy=[&]()->JobPtr {
 if(job->data.contains("OfflineProject")&&!renderedArchive){readBrowserSession(job->data);auto project=job->data["OfflineProject"];project["Name"]=utf8(fs::path(wide(str(job->data,"FileName"))).stem().wstring());project["Folder"]=str(job->data,"Folder");project["Id"]=str(job->data,"ProjectId");if(!str(job->data,"ProtectedBrowserSession").empty()){project["BrowserLogin"]=true;project["ProtectedBrowserSession"]=str(job->data,"ProtectedBrowserSession");project["LogoutPages"]=str(readBrowserSession(job->data),"LogoutPages");}else{auto login=basicLogin(readHeaders(job->data));if(!login.first.empty())setGrabberLogin(project,login.first,login.second);}return addOfflineProject(project,str(job->data,"Queue"),true);}
 auto copy=add(str(job->data,"Url"),str(job->data,"Folder"),str(job->data,"FileName"),str(job->data,"Queue"),true,readHeaders(job->data),str(job->data,"ExpectedSha256"),readPostRequest(job->data),readBrowserProxy(job->data));
 try{for(const char* key:{"Description","DownloadPage","Connections","LimitKbps","Category","CategoryExplicit","ProtectedBrowserSession","RequiresBrowserSessionCapture"})if(job->data.contains(key))copy->data[key]=job->data[key];if(renderedArchive)for(const char* key:{"OfflineProject","ProtectedRenderedPage","ProjectId"})if(job->data.contains(key))copy->data[key]=job->data[key];copy->data["RedownloadOf"]=job->id();save();}catch(...){jobs.erase(std::remove(jobs.begin(),jobs.end(),copy),jobs.end());throw;}return copy;
 };
 auto previousJobs=jobs;catalogTransaction=true;
 try{auto copy=createCopy();catalogTransaction=false;save();return copy;}
 catch(...){catalogTransaction=false;jobs.swap(previousJobs);throw;}
}
void Manager::setMembership(JobPtr job,bool member,const std::string& queue){
 setQueueMembershipBatch(*this,{job},member,queue);
}

void Manager::setCompletionAction(JobPtr job,const std::string& action,int delay,bool wait){
 setCompletionPlan(job,action=="None"?std::vector<std::string>{}:std::vector<std::string>{action},false,delay,wait);
}
void Manager::setCompletionPlan(JobPtr job,const std::vector<std::string>& requested,bool force,int delay,bool wait){
 const auto actions=validateCompletionSteps(requested);
 if(delay<15||delay>3600)throw std::runtime_error("Choose a countdown from 15 to 3600 seconds.");
 if(force&&!completionCanForce(actions))throw std::runtime_error("Force termination applies only to shut down or restart.");
 Lock lock(mutex);if(!job||std::find(jobs.begin(),jobs.end(),job)==jobs.end())throw std::runtime_error("Unknown download.");
 if(!actions.empty()&&str(job->data,"Status")=="Complete")throw std::runtime_error("Set completion actions before the download finishes.");
 auto before=job->data;job->data["CompletionActions"]=actions;job->data["CompletionAction"]=actions.empty()?"None":actions.size()==1?actions[0]:"Multiple actions";
 if(std::any_of(actions.begin(),actions.end(),completionUsesPowerOptions))job->data["SuppressCompletionDialog"]=true;
 job->data["CompletionForce"]=force;job->data["CompletionActionDelay"]=delay;job->data["CompletionWaitForOthers"]=wait;job->data["CompletionActionArmed"]=!actions.empty();
 try{save();}catch(...){job->data=before;throw;}
}
Json Manager::takeDownloadCompletion(JobPtr job){
 Lock lock(mutex);if(!job||std::find(jobs.begin(),jobs.end(),job)==jobs.end()||isActive(job)||!scannerAllowsCompletion(job->data)||str(job->data,"Status")!="Complete"||!yes(job->data,"CompletionActionArmed"))return Json::object();
 std::vector<std::string> actions;try{actions=completionSteps(job->data);}catch(...){actions.clear();}
 if(showDownloadCompletionDialog(job->data,yes(state["Settings"],"SuppressCompletionDialog")))
  actions.erase(std::remove_if(actions.begin(),actions.end(),completionUsesPowerOptions),actions.end());
 auto before=job->data;job->data["CompletionActionArmed"]=false;try{save();}catch(...){job->data=before;throw;}
 if(actions.empty())return Json::object();
 return {{"Id",job->id()},{"Action",completionSummary(actions)},{"Actions",actions},{"Force",yes(job->data,"CompletionForce")&&completionCanForce(actions)},{"DelaySeconds",std::clamp<i64>(num(job->data,"CompletionActionDelay",30),15,3600)},{"WaitForOthers",yes(job->data,"CompletionWaitForOthers",true)}};
}
PrefetchAdmission Manager::beginPrefetch(JobPtr job){
 Lock lock(mutex);
 if(stopping||catalogTransaction||job->data.contains("OfflineProject")||!str(job->data,"DuplicateOf").empty()||
    (!str(job->data,"ProtectedRequest").empty()&&!completedPostResponseCandidate(job->data))||yes(job->data,"RequiresRequestCapture")||
    !yes(state["Settings"],"PrefetchFileInfo")||isActive(job)||str(job->data,"Status")=="Complete"||
    yes(job->data,"AuthenticationPromptPending")||fileInfoRequiresExplicitHttpRetry(job->data)||!serverRetryReady(job->data,epoch())||
    (!str(job->data,"SourceUrl").empty()&&!job->video&&!job->audio)||yes(job->data,"RequiresMediaCapture")||
    yes(job->data,"LiveRecording")||yes(job->data,"SabrSessionRefreshRequired"))
  return PrefetchAdmission::Unavailable;
 const auto originalResponse=job->data.find("OriginalResponse");
 if(originalResponse!=job->data.end()&&str(*originalResponse,"State")!="Published")return PrefetchAdmission::Unavailable;
 if(std::find(jobs.begin(),jobs.end(),job)==jobs.end())return PrefetchAdmission::Unavailable;
 if(activeTransferCount()>=(size_t)num(state["Settings"],"Parallel",3))return PrefetchAdmission::AtCapacity;
 const bool media=hasNonemptyString(job->data,"ProtectedAdaptive")||hasNonemptyString(job->data,"SourceUrl");
 const bool hasDurableStaging=media?hasNonemptyString(job->data,"MediaFolder")&&!job->data.contains("RetainedMediaOutput"):
  !job->video&&!job->audio;
 const bool alreadyDurable=hasDurableStaging&&prefetchCheckpointMatches(*job);
 auto before=job->data;
 job->data.erase("ConnectionStatus");
 job->data.erase("ResponseFileName");job->data.erase("ResponseFileNameUrl");
 job->data.erase("PrefetchedFinalSize");job->data.erase("ReusedPrefetchedOutput");job->data.erase("ContinuedMediaPrefetch");job->data.erase("ContinuedFilePrefetch");job->data.erase("PrefetchedBytesAtStart");job->data.erase("PrefetchPeakRate");job->data["ConfirmationPending"]=true;
 // Admission must be durable before a worker starts; failed storage must not
 // leave an in-memory confirmation gate that was never admitted.
 try{
  if(media){
   mediaWorkingDirectory(*this,job);
   // An interruption before Start must recover this record as paused.
   job->data["Status"]="Awaiting confirmation";
  }
  // Restart already recovers an unconfirmed offer as paused. A matching
  // offer with its required staging path needs no second write.
  if(!alreadyDurable)save();
 }catch(...){job->data=std::move(before);throw;}
 job->syncTrackProgress();
 job->prefetchSampleBytes=std::max<i64>(0,num(job->data,"TransferredBytes"));
 job->prefetchSampleTime=std::chrono::steady_clock::now();
 job->prefetchPeakRate=0;
 start(job);
 return PrefetchAdmission::Started;
}
void Manager::checkpointMediaStart(const JobPtr& job){
 Lock lock(mutex);
 if(yes(job->data,"ConfirmationPending")&&!job->data.contains("RetainedMediaOutput")){
  const auto folder=str(job->data,"MediaFolder");
  if(const auto saved=latestDownloadCheckpoint(*job);saved&&!folder.empty()&&str(*saved,"MediaFolder")==folder&&
     !saved->contains("RetainedMediaOutput")){
   const auto status=str(*saved,"Status");
   // The offer and the prefetch admission both recover as paused.
   if(status=="Awaiting confirmation"||(status=="Downloading"&&yes(*saved,"ConfirmationPending")))return;
  }
 }
 save();
}
bool Manager::confirmPrefetchedDownload(JobPtr job,const Json& edit,const FileInfoFolderChoice* folderChoice,const Json& rememberedLogin){
 return finishPrefetchDecision(job,edit,folderChoice,true,rememberedLogin);
}
bool Manager::deferPrefetchedDownload(JobPtr job,const Json& edit,const FileInfoFolderChoice& folderChoice,const Json& rememberedLogin,const DownloadQueueChoice* queueChoice){
 return finishPrefetchDecision(job,edit,&folderChoice,false,rememberedLogin,queueChoice);
}
bool Manager::finishPrefetchDecision(JobPtr job,const Json& edit,const FileInfoFolderChoice* folderChoice,bool startNow,const Json& rememberedLogin,const DownloadQueueChoice* queueChoice){
 Lock lock(mutex);
 if(!isActive(job)||!yes(job->data,"ConfirmationPending"))return false;
 const bool media=bool(job->video)||bool(job->audio)||!str(job->data,"ProtectedAdaptive").empty();
 const bool ready=str(job->data,"MediaPhase")=="Waiting for confirmation"&&num(job->data,"PrefetchedFinalSize",-1)>=0;
 const bool savingReceipt=!media&&str(job->data,"ConnectionStatus")==SavingPrefetchReceipt;
 const auto worker=active.at(job->id());
 if(worker->cancelled())return false;
 const bool scheduling=!startNow&&queueChoice&&queueChoice->window.has_value();
 for(auto it=edit.begin();it!=edit.end();++it){
  const auto& key=it.key();
  if(key=="Folder"||key=="FileName"||key=="Category"||key=="Description"||key=="Queue")continue;
  // These limits apply to the future run. The current worker will stop
  // after the schedule is committed and keep any verified data.
  if(scheduling&&(key=="Connections"||key=="LimitKbps"))continue;
  throw std::runtime_error("Stop the download before changing transfer properties.");
 }
 if(!rememberedLogin.is_null()){
  const auto address=str(job->data,"Url");
  const auto origin=Url(address).origin;
  const auto challenge=str(job->data,"AuthenticationOrigin");
  if(media||str(rememberedLogin,"Origin")!=origin||str(rememberedLogin,"Path")!="/"||(!challenge.empty()&&challenge!=origin))return false;
  const auto login=basicLogin(siteRequestHeaders(address,readHeaders(job->data),state["Settings"]));
  if(login.first!=str(rememberedLogin,"UserName")||login.second!=reveal(str(rememberedLogin,"ProtectedPassword")))return false;
 }
 const auto confirmationAt=std::chrono::steady_clock::now();
 if(startNow)job->syncTrackProgress();
 const auto sampleTime=job->prefetchSampleTime;
 const auto sampleBytes=job->prefetchSampleBytes,peakRate=job->prefetchPeakRate;
 auto before=job->data;auto oldLimit=job->sessionLimit;auto oldPaused=schedulePaused;auto captures=state.value("BrowserCaptures",Json());
 std::optional<Json> previousSettings;
 auto queues=queuesWithDownloadDraft(str(edit,"Queue"),startNow?nullptr:queueChoice);
 if(queues)state["Queues"].swap(*queues);
 try{
  job->data=configuredDownload(job,edit,true);
  if(folderChoice||!rememberedLogin.is_null()||queueChoice){
   auto settings=state["Settings"];
   if(folderChoice)rememberFileInfoDestination(settings,str(job->data,"Category"),job->target().parent_path(),*folderChoice);
   if(!rememberedLogin.is_null())rememberSiteLogin(settings,rememberedLogin);
   if(!startNow&&queueChoice&&queueChoice->dontAsk)settings["QueuePromptLater"]=false;
   validateOptionsModel(settings);
   previousSettings.emplace();
   previousSettings->swap(state["Settings"]);
   state["Settings"].swap(settings);
  }
  if(startNow)job->samplePrefetchRate(confirmationAt,true);
  job->data["ConfirmationPending"]=false;
  if(savingReceipt)job->data.erase("ConnectionStatus");
  job->data["QueueMember"]=true;job->data["QueueOrigin"]=false;job->data["Error"]="";
  schedulePaused.erase(job->id());eraseCapturePresentations(state,job->id());
  // Both choices keep verified output until publication succeeds.
  if(job->preparedMedia.is_object())job->data["RetainedMediaOutput"]=job->preparedMedia;
  if(startNow){
   job->data["IndividualStart"]=true;job->sessionLimit.reset();job->data["QueueAttempts"]=0;
   job->data["ReusedPrefetchedOutput"]=ready;job->data["ContinuedMediaPrefetch"]=media;job->data["ContinuedFilePrefetch"]=!media;
   if(ready){job->data["Status"]="Merging";job->data["MediaPhase"]="Publishing prepared file";}
   const auto prefetched=std::max<i64>(0,num(job->data,"Received"));
   job->data["PrefetchedBytesAtStart"]=prefetched;
   if(prefetched>0&&job->prefetchPeakRate>0)job->data["PrefetchPeakRate"]=job->prefetchPeakRate;
   else job->data.erase("PrefetchPeakRate");
  }else{
   job->data["Status"]="Paused";
   if(media)job->data["MediaPhase"]="Paused";
   job->data["GrabberImmediate"]=false;job->data["SyncPending"]=false;
   job->data.erase("IndividualStart");job->data.erase("PrefetchedFinalSize");
  }
  save();
 }catch(...){
  if(queues)state["Queues"].swap(*queues);
  if(previousSettings)state["Settings"].swap(*previousSettings);
  job->data=before;job->sessionLimit=oldLimit;schedulePaused=oldPaused;restoreCaptureRows(state,captures);
  job->prefetchSampleTime=sampleTime;job->prefetchSampleBytes=sampleBytes;job->prefetchPeakRate=peakRate;
  throw;
 }
 // The worker cannot pass its confirmation gate until this lock is released.
 if(!startNow){
  if(edit.contains("LimitKbps"))job->sessionLimit.reset();
  job->prefetchDeferring=true;
  // The worker has already closed its network responses. Give it time to
  // checkpoint the completed parts so resuming need not replay the request.
  if(!savingReceipt)worker->stop=true;
 }
 else if(num(job->data,"PrefetchPeakRate")>0)job->prefetchConfirmedAt=std::chrono::steady_clock::now();
 prefetchConfirmed.notify_all();
 if(startNow&&event)event(job,false);
 return true;
}
void Manager::waitForPrefetchConfirmation(const JobPtr& job,const Cancel& cancel,std::optional<i64> verifiedSize,const Json& preparedMedia){
 std::unique_lock lock(mutex);
 cancel.check();
 const bool pending=yes(job->data,"ConfirmationPending");
 if(verifiedSize&&pending){
  job->data["PrefetchedFinalSize"]=*verifiedSize;
  job->data["MediaPhase"]="Waiting for confirmation";
  job->preparedMedia=preparedMedia;
 }
 // Confirmation signals after saving. Keep the timed check for cancellation
 // through shutdown and the other paths that already own the transfer token.
 try{
  while(yes(job->data,"ConfirmationPending")){
   prefetchConfirmed.wait_for(lock,std::chrono::milliseconds(40));
   cancel.check();
  }
 }catch(...){job->preparedMedia=nullptr;throw;}
 job->preparedMedia=nullptr;
 if(!verifiedSize)return;
 // Start may have committed before verification finished. Checkpoint the
 // assembly now, unless confirmation already saved this exact receipt.
 const auto saved=job->data.find("RetainedMediaOutput");
 if(preparedMedia.is_object()&&(saved==job->data.end()||*saved!=preparedMedia)){
  auto before=job->data;
  job->data["RetainedMediaOutput"]=preparedMedia;
  try{save();}catch(...){job->data.swap(before);throw;}
 }
 job->data["Status"]="Merging";
 job->data["MediaPhase"]="Publishing prepared file";
 job->data["PrefetchedFinalSize"]=*verifiedSize;
}
void Manager::endPrefetch(JobPtr job,bool keepPreparedOutput){
 std::unique_lock lock(mutex);
 if(!yes(job->data,"ConfirmationPending"))return;
 if(auto worker=active.find(job->id());worker!=active.end()){
  // Keep the checked file before waking the worker to tear down its staging.
  // A failed checkpoint leaves confirmation available for another decision.
  if(keepPreparedOutput&&job->preparedMedia.is_object()){
   const auto saved=job->data.find("RetainedMediaOutput");
   if(saved==job->data.end()||*saved!=job->preparedMedia){
    auto before=job->data;
    job->data["RetainedMediaOutput"]=job->preparedMedia;
    try{save();}catch(...){job->data.swap(before);throw;}
   }
  }
  worker->second->stop=true;
  prefetchConfirmed.notify_all();
  if(!prefetchConfirmed.wait_for(lock,std::chrono::seconds(5),[&]{return !isActive(job);}))
   throw std::runtime_error("The background transfer is still stopping. Please retry in a moment.");
 }
 if(!yes(job->data,"ConfirmationPending"))return;
 auto before=job->data;
 job->data.erase("PrefetchedFinalSize");
 job->data["ConfirmationPending"]=false;
 job->data["Status"]="Paused";
 if(capturedMedia(job->data))job->data["MediaPhase"]="Paused";
 try{save();}catch(...){job->data.swap(before);throw;}
}
void Manager::editCategory(const std::string& original,const std::string& name,const std::string& extensions,const std::string& hosts,const std::string& folder){
 Lock lock(mutex);auto cats=categories();auto found=std::find(cats.begin(),cats.end(),original);if(!original.empty()&&found==cats.end())throw std::runtime_error("That category no longer exists.");if(name.empty()||name.size()>80||safeName(name)!=name)throw std::runtime_error("Choose a valid category name up to 80 characters.");for(auto c:cats)if(c!=original&&lower(c)==lower(name))throw std::runtime_error("That category already exists.");auto& custom=state["Settings"]["CustomCategories"];bool builtIn=!original.empty()&&std::find(custom.begin(),custom.end(),Json(original))==custom.end();if(original=="Other"&&name!=original)throw std::runtime_error("The fallback category keeps its name.");if(!folder.empty()&&!fs::path(wide(folder)).is_absolute())throw std::runtime_error("Choose an absolute category folder.");for(const auto& ext:words(extensions))if(!validCategoryExtension(ext))throw std::runtime_error("Use extensions or prefixes separated by spaces, such as zip pdf r0*.");for(auto host:words(hosts)){if(host.rfind("*.",0)==0)host.erase(0,2);if(Url("https://"+host+"/").host!=lower(host))throw std::runtime_error("Use site host names without paths or ports.");}
 auto shownExtensions=categoryExtensions(state["Settings"],original);std::string shownHosts;
 for(const auto& rule:state["Settings"]["CategoryRules"])if(str(rule,"Category")==original){shownExtensions=str(rule,"Extensions");shownHosts=str(rule,"Hosts");break;}
 const bool rulesChanged=name!=original||lower(trim(extensions))!=lower(trim(shownExtensions))||lower(trim(hosts))!=lower(trim(shownHosts));
 bool hadTemplates=state.contains("GrabberTemplates");auto oldTemplates=state.value("GrabberTemplates",Json::array());auto before=state["Settings"],oldProjects=state["Projects"];std::vector<Json> oldJobs;for(auto j:jobs)oldJobs.push_back(j->data);try{if(original.empty())custom.push_back(name);else if(builtIn&&name!=original){auto hidden=state["Settings"].value("HiddenBuiltinCategories",Json::array());hidden.push_back(original);state["Settings"]["HiddenBuiltinCategories"]=hidden;custom.push_back(name);}else if(!builtIn)for(auto& c:custom)if(c==original)c=name;auto& rules=state["Settings"]["CategoryRules"];if(rulesChanged){rules.erase(std::remove_if(rules.begin(),rules.end(),[&](const Json& r){return str(r,"Category")==original||str(r,"Category")==name;}),rules.end());if(!trim(extensions).empty()||!trim(hosts).empty())rules.insert(rules.begin(),Json{{"Category",name},{"Extensions",lower(trim(extensions))},{"Hosts",lower(trim(hosts))}});}auto paths=dictionary(state["Settings"]["CategoryPaths"]);paths.erase(original);if(folder.empty())paths.erase(name);else paths[name]=folder;state["Settings"]["CategoryPaths"]=legacyDictionary(paths);auto memory=state["Settings"].value("CategoryRememberLast",Json::object());if(memory.contains(original)){memory[name]=memory[original];if(name!=original)memory.erase(original);}state["Settings"]["CategoryRememberLast"]=memory;auto types=state["Settings"].value("CategoryTypeOverrides",Json::object());if(rulesChanged){types.erase(original);if(trim(hosts).empty())types[name]=lower(trim(extensions));}state["Settings"]["CategoryTypeOverrides"]=types;if(!original.empty())for(auto j:jobs)if(str(j->data,"Category")==original)j->data["Category"]=name;if(!original.empty())for(auto& project:state["Projects"])if(str(project,"SaveCategory")==original)project["SaveCategory"]=name;if(!original.empty())remapGrabberTemplates(state,original,name);save();}catch(...){state["Settings"]=before;state["Projects"]=oldProjects;if(hadTemplates)state["GrabberTemplates"]=oldTemplates;else state.erase("GrabberTemplates");for(size_t i=0;i<jobs.size();++i)jobs[i]->data=oldJobs[i];throw;}
}
void Manager::deleteCategory(const std::string& name){
 Lock lock(mutex);auto& custom=state["Settings"]["CustomCategories"];auto found=std::find(custom.begin(),custom.end(),Json(name));auto names=categories();if(name=="Other")throw std::runtime_error("The fallback category cannot be deleted.");if(std::find(names.begin(),names.end(),name)==names.end())throw std::runtime_error("That category no longer exists.");bool hadTemplates=state.contains("GrabberTemplates");auto oldTemplates=state.value("GrabberTemplates",Json::array());auto before=state["Settings"],oldProjects=state["Projects"];std::vector<Json> oldJobs;for(auto j:jobs)oldJobs.push_back(j->data);try{if(found!=custom.end())custom.erase(found);else{auto hidden=state["Settings"].value("HiddenBuiltinCategories",Json::array());hidden.push_back(name);state["Settings"]["HiddenBuiltinCategories"]=hidden;}auto& rules=state["Settings"]["CategoryRules"];rules.erase(std::remove_if(rules.begin(),rules.end(),[&](const Json& r){return str(r,"Category")==name;}),rules.end());auto paths=dictionary(state["Settings"]["CategoryPaths"]);paths.erase(name);state["Settings"]["CategoryPaths"]=legacyDictionary(paths);if(state["Settings"].contains("CategoryRememberLast"))state["Settings"]["CategoryRememberLast"].erase(name);if(state["Settings"].contains("CategoryTypeOverrides"))state["Settings"]["CategoryTypeOverrides"].erase(name);for(auto j:jobs)if(str(j->data,"Category")==name)j->data["Category"]="Other";for(auto& project:state["Projects"])if(str(project,"SaveCategory")==name)project["SaveCategory"]="Other";remapGrabberTemplates(state,name,"Other");save();}catch(...){state["Settings"]=before;state["Projects"]=oldProjects;if(hadTemplates)state["GrabberTemplates"]=oldTemplates;else state.erase("GrabberTemplates");for(size_t i=0;i<jobs.size();++i)jobs[i]->data=oldJobs[i];throw;}
}

}

namespace udm {
class RecycleOnlySink:public IFileOperationProgressSink {
 LONG refs=1;
public:
 HRESULT result=E_ABORT;bool completed=false;
 HRESULT STDMETHODCALLTYPE QueryInterface(REFIID iid,void** out)override{if(!out)return E_POINTER;*out=nullptr;if(iid==IID_IUnknown||iid==IID_IFileOperationProgressSink){*out=this;AddRef();return S_OK;}return E_NOINTERFACE;}
 ULONG STDMETHODCALLTYPE AddRef()override{return InterlockedIncrement(&refs);}ULONG STDMETHODCALLTYPE Release()override{return InterlockedDecrement(&refs);}
 HRESULT STDMETHODCALLTYPE StartOperations()override{return S_OK;}HRESULT STDMETHODCALLTYPE FinishOperations(HRESULT)override{return S_OK;}
 HRESULT STDMETHODCALLTYPE PreRenameItem(DWORD,IShellItem*,LPCWSTR)override{return E_ABORT;}
 HRESULT STDMETHODCALLTYPE PostRenameItem(DWORD,IShellItem*,LPCWSTR,HRESULT,IShellItem*)override{return S_OK;}
 HRESULT STDMETHODCALLTYPE PreMoveItem(DWORD,IShellItem*,IShellItem*,LPCWSTR)override{return E_ABORT;}
 HRESULT STDMETHODCALLTYPE PostMoveItem(DWORD,IShellItem*,IShellItem*,LPCWSTR,HRESULT,IShellItem*)override{return S_OK;}
 HRESULT STDMETHODCALLTYPE PreCopyItem(DWORD,IShellItem*,IShellItem*,LPCWSTR)override{return E_ABORT;}
 HRESULT STDMETHODCALLTYPE PostCopyItem(DWORD,IShellItem*,IShellItem*,LPCWSTR,HRESULT,IShellItem*)override{return S_OK;}
 HRESULT STDMETHODCALLTYPE PreDeleteItem(DWORD flags,IShellItem*)override{return (flags&TSF_DELETE_RECYCLE_IF_POSSIBLE)?S_OK:E_ABORT;}
 HRESULT STDMETHODCALLTYPE PostDeleteItem(DWORD,IShellItem*,HRESULT hr,IShellItem* created)override{result=hr;completed=SUCCEEDED(hr)&&created!=nullptr;return S_OK;}
 HRESULT STDMETHODCALLTYPE PreNewItem(DWORD,IShellItem*,LPCWSTR)override{return E_ABORT;}
 HRESULT STDMETHODCALLTYPE PostNewItem(DWORD,IShellItem*,LPCWSTR,LPCWSTR,DWORD,HRESULT,IShellItem*)override{return S_OK;}
 HRESULT STDMETHODCALLTYPE UpdateProgress(UINT,UINT)override{return S_OK;}
 HRESULT STDMETHODCALLTYPE ResetTimer()override{return S_OK;}HRESULT STDMETHODCALLTYPE PauseTimer()override{return S_OK;}HRESULT STDMETHODCALLTYPE ResumeTimer()override{return S_OK;}
};
void Manager::recycleCompleted(JobPtr job,HWND owner,bool allowMissingForRemoval){
 Lock lock(mutex);if(!job||std::find(jobs.begin(),jobs.end(),job)==jobs.end()||isActive(job)||str(job->data,"Status")!="Complete")throw std::runtime_error("Only an inactive completed file still in the download list can be recycled.");
 recoverFileOperation(job);
 recoverReplacements(job);
 if(std::find(jobs.begin(),jobs.end(),job)==jobs.end()||isActive(job)||str(job->data,"Status")!="Complete")throw std::runtime_error("Recovery changed this download's status or catalog entry. Review it before recycling its file.");
 for(auto other:jobs)if(str(other->data,"ReplacementOf")==job->id())throw std::runtime_error("A replacement still depends on this file. Remove the pending replacement first.");
 auto path=job->target();if(!path.is_absolute())throw std::runtime_error("Choose an existing regular downloaded file.");
 const DWORD attributes=GetFileAttributesW(path.c_str());
 if(attributes==INVALID_FILE_ATTRIBUTES){const auto failure=GetLastError();
  // Combined list removal can continue when the saved file is already gone.
  // Other failures are not evidence of absence; standalone recycling is strict.
  if(allowMissingForRemoval&&(failure==ERROR_FILE_NOT_FOUND||failure==ERROR_PATH_NOT_FOUND))return;
  throw std::runtime_error("The downloaded file is missing or unavailable. Windows error "+std::to_string(failure)+". The history record was kept.");
 }
 if(attributes&(FILE_ATTRIBUTE_DIRECTORY|FILE_ATTRIBUTE_REPARSE_POINT))throw std::runtime_error("Choose an existing regular downloaded file.");
 // Commit history before invoking the shell, so a disk-full error cannot leave unrecorded intent.
 job->data["RecyclePending"]=true;save();IFileOperation* op=nullptr;IShellItem* item=nullptr;RecycleOnlySink sink;HRESULT hr=CoCreateInstance(CLSID_FileOperation,nullptr,CLSCTX_INPROC_SERVER,IID_PPV_ARGS(&op));
 if(SUCCEEDED(hr))hr=op->SetOwnerWindow(owner);
 if(SUCCEEDED(hr))hr=op->SetOperationFlags(FOFX_RECYCLEONDELETE|FOFX_ADDUNDORECORD|FOFX_EARLYFAILURE|FOF_NORECURSION|FOF_NOERRORUI|FOF_NOCONFIRMATION|FOF_SILENT);
 if(SUCCEEDED(hr))hr=SHCreateItemFromParsingName(path.c_str(),nullptr,IID_PPV_ARGS(&item));
 if(SUCCEEDED(hr))hr=op->DeleteItem(item,&sink);
 if(SUCCEEDED(hr))hr=op->PerformOperations();BOOL aborted=TRUE;if(op)op->GetAnyOperationsAborted(&aborted);if(item)item->Release();if(op)op->Release();
 job->data.erase("RecyclePending");if(FAILED(hr)||aborted||FAILED(sink.result)||!sink.completed||fs::exists(path)){save();throw std::runtime_error("Windows did not confirm recycling this file. The history record was kept; UDM does not fall back to permanent deletion.");}
 job->data["RecycledAt"]=date();job->data["SyncPending"]=false;job->data["SyncStatus"]="File recycled";save();
}
}
