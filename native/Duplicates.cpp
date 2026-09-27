#include "Core.hpp"
#include <algorithm>
namespace udm {
static std::string resourceKey(const std::string& value){
 Url u(value);auto query=u.query;auto hash=query.find('#');if(hash!=std::string::npos)query.resize(hash);
 return u.scheme+"://"+u.host+":"+std::to_string(u.port)+(u.path.empty()?"/":u.path)+query;
}
static Headers accountHeaders(const Headers& headers){Headers result;for(auto& [key,value]:headers)if(lower(key)=="authorization"||lower(key)=="cookie")result[lower(key)]=value;return result;}
JobPtr Manager::findDuplicate(const std::string& address,const Headers& headers,JobPtr ignore,const Json& request)const{
 Lock lock(mutex);auto key=resourceKey(address);auto account=accountHeaders(headers);auto post=validatePostRequest(request,address);JobPtr newest;
 for(auto it=jobs.rbegin();it!=jobs.rend();++it){auto job=*it;
  if(job==ignore||job->data.contains("OfflineProject")||readPostRequest(job->data)!=post||!str(job->data,"SourceUrl").empty()||!str(job->data,"ProtectedAdaptive").empty()||resourceKey(str(job->data,"Url"))!=key||accountHeaders(readHeaders(job->data))!=account)continue;
  if(isActive(job)||str(job->data,"Status")=="Queued"||str(job->data,"Status")=="Awaiting confirmation"||!str(job->data,"DuplicateOf").empty())return job;
  if(!newest)newest=job;
 }return newest;
}
JobPtr Manager::offerDownload(const std::string& address,const std::string& folder,const std::string& name,const std::string& queue,bool paused,const Headers& headers,const Json& request){
 Lock lock(mutex);auto candidate=add(address,folder,name,queue,true,headers,"",request);
 try{
  auto existing=findDuplicate(address,readHeaders(candidate->data),candidate,request);
  auto policy=str(state["Settings"],"DuplicatePolicy","Ask");
  if(existing&&(isActive(existing)||str(existing->data,"Status")=="Queued"||str(existing->data,"Status")=="Awaiting confirmation"||!str(existing->data,"DuplicateOf").empty()||policy=="Existing")){remove(candidate);return existing;}
  if(existing&&policy=="Ask"){candidate->data["DuplicateOf"]=existing->id();candidate->data["Status"]="Awaiting duplicate choice";}
  else candidate->data["Status"]=paused?"Paused":"Queued";
  save();return candidate;
 }catch(...){jobs.erase(std::remove(jobs.begin(),jobs.end(),candidate),jobs.end());try{save();}catch(...){}throw;}
}
JobPtr Manager::resolveDuplicate(JobPtr candidate,const std::string& choice){
 Lock lock(mutex);if(!candidate||isActive(candidate)||str(candidate->data,"DuplicateOf").empty())throw std::runtime_error("This download has no pending duplicate choice.");
 JobPtr existing;for(auto job:jobs)if(job->id()==str(candidate->data,"DuplicateOf"))existing=job;
 if(choice=="Cancel"){remove(candidate);return {};}
 if(choice=="Existing"){if(!existing)throw std::runtime_error("The previous record was removed. Choose a numbered copy.");remove(candidate);return existing;}
 if(choice!="Numbered"&&choice!="Replace")throw std::runtime_error("Choose an existing download, numbered copy or replacement.");
 auto before=candidate->data;
 if(choice=="Replace"){
  if(!existing||isActive(existing)||str(existing->data,"Status")!="Complete"||!fs::is_regular_file(existing->target()))throw std::runtime_error("Replacement requires an existing completed file.");
  if(Url(str(candidate->data,"Url")).scheme=="ftp")throw std::runtime_error("Replacement currently supports HTTP and HTTPS downloads.");
  for(auto job:jobs)if(str(job->data,"ReplacementOf")==existing->id())throw std::runtime_error("A replacement for this file already exists.");
  if(fs::exists(candidate->target()))throw std::runtime_error("The backup location is now occupied. Add this link again to choose a free name.");
  auto hash=fileHash(existing->target());if(!str(existing->data,"Sha256").empty()&&hash!=str(existing->data,"Sha256"))throw std::runtime_error("The completed file has changed on disk. Choose a numbered copy to preserve your edits.");
  auto priorPath=existing->target();auto stem=priorPath.stem().wstring(),extension=priorPath.extension().wstring();for(int index=1;;++index){priorPath=existing->target().parent_path()/(stem+L" (previous "+std::to_wstring(index)+L")"+extension);bool occupied=fs::exists(priorPath);for(auto other:jobs)occupied|=lower(utf8(other->target().wstring()))==lower(utf8(priorPath.wstring()))||lower(str(other->data,"PreviousPath"))==lower(utf8(priorPath.wstring()));if(!occupied)break;}
  candidate->data["PreviousPath"]=utf8(priorPath.wstring());candidate->data["ReplacementOf"]=existing->id();candidate->data["ReplacementHash"]=hash;
  candidate->data["Folder"]=existing->data["Folder"];candidate->data["FileName"]=existing->data["FileName"];
 }
 candidate->data.erase("DuplicateOf");candidate->data["Status"]="Paused";
 try{save();}catch(...){candidate->data=before;throw;}return candidate;
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
 auto oldBefore=original->data,oldAfter=oldBefore;oldAfter["Folder"]=utf8(previous.parent_path().wstring());oldAfter["FileName"]=utf8(previous.filename().wstring());oldAfter["PreviousVersionOf"]=job->id();
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
