#include "Core.hpp"
#include "SiteLogins.hpp"
#include <algorithm>
#include <regex>
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
JobPtr Manager::offerDownload(const std::string& address,const std::string& folder,const std::string& name,const std::string& queue,bool paused,const Headers& headers,const Json& request,const Json& browserProxy){
 Lock lock(mutex);auto candidate=add(address,folder,name,queue,true,headers,"",request,browserProxy);
 try{
  auto existing=findDuplicate(address,readHeaders(candidate->data),candidate,request);
  auto policy=str(state["Settings"],"DuplicatePolicy","Ask");
  if(existing&&(isActive(existing)||str(existing->data,"Status")=="Queued"||str(existing->data,"Status")=="Awaiting confirmation"||!str(existing->data,"DuplicateOf").empty()||!str(existing->data,"ReplacementOf").empty()||policy=="Existing")){remove(candidate);if(policy=="Existing"&&str(existing->data,"Status")!="Awaiting confirmation"&&str(existing->data,"DuplicateOf").empty())existingOffers.insert(existing->id());return existing;}
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
JobPtr Manager::resolveDuplicateChoice(JobPtr candidate,const std::string& choice){
 Lock lock(mutex);if(!candidate||std::find(jobs.begin(),jobs.end(),candidate)==jobs.end()||isActive(candidate)||str(candidate->data,"DuplicateOf").empty())throw std::runtime_error("This download has no pending duplicate choice.");
 JobPtr existing;for(auto job:jobs)if(job->id()==str(candidate->data,"DuplicateOf"))existing=job;
 if(choice=="Cancel"){remove(candidate);return {};}
 if(choice=="Existing"){if(!existing)throw std::runtime_error("The previous record was removed. Choose a numbered copy.");remove(candidate);existingOffers.insert(existing->id());return existing;}
 if(choice!="Numbered"&&choice!="Replace")throw std::runtime_error("Choose an existing download, numbered copy or replacement.");
 auto before=candidate->data;
 if(choice=="Replace"){
  if(!existing||isActive(existing)||(str(existing->data,"Status")!="Complete"&&str(existing->data,"Status")!="Paused"&&str(existing->data,"Status")!="Failed")||!str(existing->data,"ReplacementOf").empty())throw std::runtime_error("Pause the existing download before overwriting it.");
  if(str(existing->data,"Status")!="Complete"||!fs::exists(existing->target()))return restartDuplicate(candidate,existing);
  if(!fs::is_regular_file(existing->target()))throw std::runtime_error("The existing destination is not a file.");
  for(auto job:jobs)if(str(job->data,"ReplacementOf")==existing->id())throw std::runtime_error("A replacement for this file already exists.");
  if(fs::exists(candidate->target()))throw std::runtime_error("The backup location is now occupied. Add this link again to choose a free name.");
  auto hash=fileHash(existing->target());
  auto priorPath=existing->target();auto stem=priorPath.stem().wstring(),extension=priorPath.extension().wstring();for(int index=1;;++index){priorPath=existing->target().parent_path()/(stem+L" (previous "+std::to_wstring(index)+L")"+extension);bool occupied=fs::exists(priorPath);for(auto other:jobs)occupied|=lower(utf8(other->target().wstring()))==lower(utf8(priorPath.wstring()))||lower(str(other->data,"PreviousPath"))==lower(utf8(priorPath.wstring()));if(!occupied)break;}
  candidate->data["PreviousPath"]=utf8(priorPath.wstring());candidate->data["ReplacementOf"]=existing->id();candidate->data["ReplacementHash"]=hash;
  candidate->data["Folder"]=existing->data["Folder"];candidate->data["FileName"]=existing->data["FileName"];
 }
 candidate->data.erase("DuplicateOf");candidate->data["Status"]=yes(candidate->data,"DuplicateQueue")?"Queued":"Paused";candidate->data.erase("DuplicateQueue");
 try{save();}catch(...){candidate->data=before;throw;}return candidate;
}
// An overwrite of an unfinished file starts a new parts generation, then retires
// only the old generation's unchanged part files after the catalog commit.
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
  auto name=utf8(entry.path().filename().wstring());if(!std::regex_match(name,std::regex("[0-9]{4,8}\\.part"))||!entry.is_regular_file()||entry.is_symlink())continue;
  files.push_back({{"Name",name},{"Size",entry.file_size()},{"Stamp",entry.last_write_time().time_since_epoch().count()}});
 }
 for(const char* key:{"Id","FileName","Folder","Category","Queue","Description","Connections","LimitKbps","ExpectedSha256","DownloadPage","Added","SuppressCompletionDialog","OpenFolderOnCompletion","CloseProgressOnCompletion"})if(before.contains(key))after[key]=before[key];
 after.erase("DuplicateOf");after["Status"]=yes(after,"DuplicateQueue")?"Queued":"Paused";after.erase("DuplicateQueue");after["PartsFolder"]=utf8(fresh.wstring());after["RestartAttempt"]=attempt;after["Restarted"]=date();
 auto journal=root/L"restarts"/(wide(attempt)+L".json");
 auto operation=Json{{"Schema",1},{"Id",existing->id()},{"Attempt",attempt},{"Parts",utf8(prior.wstring())},{"Files",files},{"Before",before},{"Committed",false}};
 atomicText(journal,operation.dump(),false);
 existing->data=after;jobs.erase(std::remove(jobs.begin(),jobs.end(),candidate),jobs.end());
 try{save();}catch(...){existing->data=before;jobs=previousJobs;std::error_code ec;fs::remove(journal,ec);throw;}
 existing->workers.clear();existing->speed=0;existing->speedMeter.reset(0);existing->sessionLimit.reset();existingOffers.erase(existing->id());
 recoverRestarts();return existing;
}
void Manager::recoverRestarts(){try{
 Lock lock(mutex);auto directory=root/L"restarts";if(!fs::exists(directory))return;
 for(const auto& entry:fs::directory_iterator(directory)){
  if(!entry.is_regular_file()||entry.path().extension()!=L".json")continue;
  try{
   auto operation=Json::parse(readText(entry.path()));if(num(operation,"Schema")!=1||!operation.contains("Before")||!operation["Files"].is_array())continue;
   auto id=str(operation,"Id"),attempt=str(operation,"Attempt");if(id!=str(operation["Before"],"Id")||attempt!=utf8(entry.path().stem().wstring()))continue;
   JobPtr job;for(auto item:jobs)if(item->id()==id)job=item;
   if(!yes(operation,"Committed")){
    if(!job||str(job->data,"RestartAttempt")!=attempt){fs::remove(entry.path());continue;}
    operation["Committed"]=true;atomicText(entry.path(),operation.dump(),false);
   }
   auto parts=fs::path(wide(str(operation,"Parts")));auto recorded=str(operation["Before"],"PartsFolder");
   auto expected=recorded.empty()?root/L"parts"/wide(id):fs::path(wide(recorded));
   if(!parts.is_absolute()||parts.lexically_normal()!=expected.lexically_normal()||fs::is_symlink(parts)||(GetFileAttributesW(parts.c_str())!=INVALID_FILE_ATTRIBUTES&&(GetFileAttributesW(parts.c_str())&FILE_ATTRIBUTE_REPARSE_POINT)))continue;
   bool inUse=false;for(auto item:jobs){auto selected=str(item->data,"PartsFolder");auto path=selected.empty()?root/L"parts"/wide(item->id()):fs::path(wide(selected));if(path.lexically_normal()==parts.lexically_normal())inUse=true;}if(inUse)continue;
   bool retained=false;for(const auto& file:operation["Files"]){auto name=str(file,"Name");if(!std::regex_match(name,std::regex("[0-9]{4,8}\\.part"))){retained=true;continue;}auto path=parts/wide(name);if(!fs::exists(path))continue;
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
static Json completeData(const Json& data,const fs::path& staging,const std::string& hash){auto next=data;next["Sha256"]=hash;next["Size"]=fs::file_size(staging);next["Received"]=next["Size"];next["Status"]="Complete";next["Finished"]=date();next["Error"]="";next["QueueMember"]=false;return next;}
void Manager::publishFile(JobPtr job,const fs::path& staging,const std::string& hash){
 Lock lock(mutex);auto before=job->data,after=completeData(before,staging,hash);auto target=job->target();auto originalId=str(before,"ReplacementOf");
 if(originalId.empty()){
  if(!MoveFileExW(staging.c_str(),target.c_str(),MOVEFILE_WRITE_THROUGH))throw std::runtime_error("Cannot publish the download. The destination may already exist; choose another file name.");
  job->data=after;save();return;
 }
 auto journal=root/L"replacements"/(wide(job->id())+L".json");if(fs::exists(journal))throw std::runtime_error("A previous replacement needs recovery. Restart UDM before retrying.");
 JobPtr original;for(auto item:jobs)if(item->id()==originalId)original=item;
 auto previous=fs::path(wide(str(before,"PreviousPath")));
 if(!original||isActive(original)||str(original->data,"Status")!="Complete"||original->target().lexically_normal()!=target.lexically_normal())throw std::runtime_error("The original download changed. Its file was not replaced.");
 Handle protectOriginal(CreateFileW(target.c_str(),GENERIC_READ,FILE_SHARE_READ|FILE_SHARE_DELETE,nullptr,OPEN_EXISTING,0,nullptr));
 if(!protectOriginal||!fs::is_regular_file(target)||fileHash(target)!=str(before,"ReplacementHash"))throw std::runtime_error("The original file changed on disk. Its contents were not replaced.");
 if(fs::exists(previous))throw std::runtime_error("The previous-version location is occupied. No file was replaced.");
 for(auto item:jobs)if(item!=job&&item!=original&&(lower(utf8(item->target().wstring()))==lower(utf8(previous.wstring()))||lower(str(item->data,"PreviousPath"))==lower(utf8(previous.wstring()))))throw std::runtime_error("The previous-version location belongs to another download.");
 auto oldBefore=original->data,oldAfter=oldBefore;oldAfter["Folder"]=utf8(previous.parent_path().wstring());oldAfter["FileName"]=utf8(previous.filename().wstring());oldAfter["PreviousVersionOf"]=job->id();oldAfter["Sha256"]=str(before,"ReplacementHash");oldAfter["Size"]=fs::file_size(target);oldAfter["Received"]=oldAfter["Size"];if(str(oldBefore,"Sha256")!=str(before,"ReplacementHash"))oldAfter.erase("ScanResult");
 after["ReplacedDownload"]=originalId;for(const char* key:{"ReplacementOf","ReplacementHash","PreviousPath"})after.erase(key);
 auto operation=Json{{"Id",job->id()},{"OriginalId",originalId},{"Target",utf8(target.wstring())},{"Previous",utf8(previous.wstring())},{"Staging",utf8(staging.wstring())},{"OldHash",str(before,"ReplacementHash")},{"NewHash",hash},{"Before",before},{"After",after},{"OriginalBefore",oldBefore},{"OriginalAfter",oldAfter}};
 atomicText(journal,operation.dump(),false);
 if(!ReplaceFileW(target.c_str(),staging.c_str(),previous.c_str(),0,nullptr,nullptr)){
  auto failure=GetLastError();
  if(fs::is_regular_file(target)&&fileHash(target)==str(before,"ReplacementHash")&&!fs::exists(previous)){std::error_code ec;fs::remove(journal,ec);}
  throw std::runtime_error("Windows could not replace the file (error "+std::to_string(failure)+"). The original and recovery information were preserved.");
 }
 CloseHandle(protectOriginal.h);protectOriginal.h=INVALID_HANDLE_VALUE;
 job->data=after;original->data=oldAfter;
 try{save();}catch(...){
  job->data=before;original->data=oldBefore;
  if(ReplaceFileW(target.c_str(),previous.c_str(),staging.c_str(),0,nullptr,nullptr)){std::error_code ec;fs::remove(journal,ec);}
  else throw std::runtime_error("Saving replacement history failed. Restart UDM to recover using the preserved journal and files.");
  throw;
 }
 std::error_code ec;fs::remove(journal,ec);
}
void Manager::recoverReplacements(){
 auto folder=root/L"replacements";if(!fs::exists(folder))return;
 for(auto& entry:fs::directory_iterator(folder)){
  if(entry.path().extension()!=L".json")continue;auto op=Json::parse(readText(entry.path()));JobPtr job,original;
  for(auto item:jobs){if(item->id()==str(op,"Id"))job=item;if(item->id()==str(op,"OriginalId"))original=item;}
  if(!job||!original)throw std::runtime_error("Replacement recovery requires the preserved history records.");
  auto target=fs::path(wide(str(op,"Target"))),previous=fs::path(wide(str(op,"Previous")));
  auto matches=[](const fs::path& path,const std::string& digest){return fs::is_regular_file(path)&&fileHash(path)==digest;};
  if(matches(target,str(op,"NewHash"))&&matches(previous,str(op,"OldHash"))){job->data=op["After"];original->data=op["OriginalAfter"];}
  else if(matches(target,str(op,"OldHash"))&&!fs::exists(previous)){job->data=op["Before"];original->data=op["OriginalBefore"];job->data["Status"]="Paused";}
  else if(!fs::exists(target)&&matches(previous,str(op,"OldHash"))){if(!MoveFileExW(previous.c_str(),target.c_str(),MOVEFILE_WRITE_THROUGH))throw std::runtime_error("Windows could not restore the original file.");job->data=op["Before"];original->data=op["OriginalBefore"];job->data["Status"]="Paused";}
  else throw std::runtime_error("A replacement needs manual recovery. Keep the journal, original and downloaded files.");
  save();fs::remove(entry.path());
 }
}
}
