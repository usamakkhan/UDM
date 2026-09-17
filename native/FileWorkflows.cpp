#include "Core.hpp"
#include <algorithm>
namespace udm {
static std::string pathKey(const fs::path& path){return lower(utf8(fs::absolute(path).lexically_normal().wstring()));}
void Manager::recoverFileOperation(){
 auto journal=root/L"file-operation.json";if(!fs::exists(journal))return;
 auto operation=Json::parse(readText(journal));JobPtr job;for(auto j:jobs)if(j->id()==str(operation,"Id"))job=j;
 if(!job)throw std::runtime_error("A pending file move refers to missing history. Keep file-operation.json for recovery.");
 auto source=fs::path(wide(str(operation,"Source"))),dest=fs::path(wide(str(operation,"Destination")));
 if(fs::exists(source)){job->data=operation["Before"];}
 else if(fs::is_regular_file(dest)&&!str(operation,"Hash",str(operation["Before"],"Sha256")).empty()&&fileHash(dest)==str(operation,"Hash",str(operation["Before"],"Sha256"))){job->data=operation["After"];}
 else throw std::runtime_error("A pending file move needs recovery. Neither location could be verified; files and journal were preserved.");
 save();fs::remove(journal);
}
void Manager::relocate(JobPtr job,const fs::path& requested){
 Lock lock(mutex);if(!job||isActive(job)||str(job->data,"Status")!="Complete")throw std::runtime_error("Move/Rename is available for completed downloads.");if(fs::exists(root/L"file-operation.json"))recoverFileOperation();
 if(!requested.is_absolute()||safeName(utf8(requested.filename().wstring()))!=utf8(requested.filename().wstring()))throw std::runtime_error("Choose an absolute destination with a valid file name.");
 auto source=job->target(),dest=requested.lexically_normal();if(pathKey(source)==pathKey(dest)){if(source!=dest)throw std::runtime_error("Choose a different name when changing only letter case.");return;}
 if(!fs::is_regular_file(source))throw std::runtime_error("The downloaded file is missing from its saved location.");
 if(fs::exists(dest))throw std::runtime_error("Destination already exists. Choose another name; no files were overwritten.");
 for(auto other:jobs)if(other!=job&&(pathKey(other->target())==pathKey(dest)||(!str(other->data,"PreviousPath").empty()&&pathKey(fs::path(wide(str(other->data,"PreviousPath"))))==pathKey(dest))))throw std::runtime_error("That destination belongs to another download.");
 auto before=job->data,after=before;after["Folder"]=utf8(dest.parent_path().wstring());after["FileName"]=utf8(dest.filename().wstring());
 fs::create_directories(dest.parent_path());auto journal=root/L"file-operation.json";
 atomicText(journal,Json{{"Id",job->id()},{"Source",utf8(source.wstring())},{"Destination",utf8(dest.wstring())},{"Before",before},{"After",after},{"Hash",str(before,"Sha256").empty()?fileHash(source):str(before,"Sha256")}}.dump(),false);
 if(!MoveFileExW(source.c_str(),dest.c_str(),MOVEFILE_COPY_ALLOWED|MOVEFILE_WRITE_THROUGH)){auto failure=GetLastError();std::error_code ec;fs::remove(journal,ec);throw std::runtime_error("Windows could not move the file (error "+std::to_string(failure)+").");}
 job->data=after;
 try{save();}catch(...){job->data=before;if(MoveFileExW(dest.c_str(),source.c_str(),MOVEFILE_COPY_ALLOWED|MOVEFILE_WRITE_THROUGH)){std::error_code ec;fs::remove(journal,ec);}else{job->data=after;throw std::runtime_error("The file moved, but saving history failed. The recovery journal preserves its new location.");}throw;}
 std::error_code ec;fs::remove(journal,ec);
}
void Manager::updateCompleted(JobPtr job,const Json& edit){
 Lock lock(mutex);if(!job||isActive(job)||str(job->data,"Status")!="Complete")throw std::runtime_error("Select a completed download.");
 auto before=job->data,next=before;for(const char* key:{"Url","Description","DownloadPage","ProtectedHeaders"})if(edit.contains(key))next[key]=edit[key];
 Url url(str(next,"Url"));if(!str(next,"DownloadPage").empty()){Url page(str(next,"DownloadPage"));if(page.scheme!="http"&&page.scheme!="https")throw std::runtime_error("The parent page must use HTTP or HTTPS.");}
 if(url.origin!=Url(str(before,"Url")).origin){auto headers=readHeaders(next);for(auto it=headers.begin();it!=headers.end();)if(lower(it->first)=="authorization"||lower(it->first)=="cookie"||lower(it->first)=="referer")it=headers.erase(it);else ++it;next["ProtectedHeaders"]=headers.empty()?"":protect(legacyDictionary(Json(headers)).dump());}
 validateHeaders(readHeaders(next));job->data=next;try{save();}catch(...){job->data=before;throw;}
}
JobPtr Manager::redownload(JobPtr job){
 Lock lock(mutex);if(!job||isActive(job)||str(job->data,"Status")!="Complete")throw std::runtime_error("Select a completed download to download again.");
 if(!str(job->data,"SourceUrl").empty()||!str(job->data,"ProtectedAdaptive").empty())throw std::runtime_error("Choose fresh video streams using the browser panel.");
 auto copy=add(str(job->data,"Url"),str(job->data,"Folder"),str(job->data,"FileName"),str(job->data,"Queue"),true,readHeaders(job->data),str(job->data,"ExpectedSha256"));
 try{for(const char* key:{"Description","DownloadPage","Connections","LimitKbps"})if(job->data.contains(key))copy->data[key]=job->data[key];copy->data["RedownloadOf"]=job->id();save();}catch(...){jobs.erase(std::remove(jobs.begin(),jobs.end(),copy),jobs.end());throw;}return copy;
}
void Manager::setMembership(JobPtr job,bool member,const std::string& queue){
 Lock lock(mutex);if(!job||isActive(job))throw std::runtime_error("Pause this download before changing its queue membership.");
 if(member&&str(job->data,"Status")=="Complete")throw std::runtime_error("Use Redownload to queue a new copy of a completed file.");
 auto before=job->data;if(!queue.empty()){bool found=false;for(const auto& q:state["Queues"])found|=str(q,"Name")==queue;if(!found)throw std::runtime_error("Choose an existing queue.");job->data["Queue"]=queue;}
 job->data["QueueMember"]=member;if(!member&&str(job->data,"Status")=="Queued")job->data["Status"]="Paused";
 try{save();}catch(...){job->data=before;throw;}
}
void Manager::beginPrefetch(JobPtr job){
 Lock lock(mutex);if(!str(job->data,"DuplicateOf").empty())return;if(!yes(state["Settings"],"PrefetchFileInfo")||isActive(job)||str(job->data,"Status")=="Complete"||!str(job->data,"SourceUrl").empty()||!str(job->data,"ProtectedAdaptive").empty()||Url(str(job->data,"Url")).scheme=="ftp"||active.size()>=(size_t)num(state["Settings"],"Parallel",3))return;
 job->data["ConfirmationPending"]=true;save();start(job);
}
void Manager::endPrefetch(JobPtr job){
 {Lock lock(mutex);if(!yes(job->data,"ConfirmationPending"))return;if(isActive(job))active[job->id()]->stop=true;}
 auto deadline=GetTickCount64()+5000;while(isActive(job)){if(GetTickCount64()>deadline)throw std::runtime_error("The background transfer is still stopping. Please retry in a moment.");Sleep(10);}
 Lock lock(mutex);job->data["ConfirmationPending"]=false;job->data["Status"]="Paused";save();
}
}
