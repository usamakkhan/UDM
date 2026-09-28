#include "Core.hpp"
#include "CompletionPolicy.hpp"
#include "BrowserProxy.hpp"
#include "Scanner.hpp"
#include <shobjidl.h>
#include <shlobj.h>
#include "GuiModels.hpp"
#include "SiteLogins.hpp"
#include <algorithm>
namespace udm {
static std::string pathKey(const fs::path& path){return lower(utf8(fs::absolute(path).lexically_normal().wstring()));}
void Manager::setDownloadLogin(JobPtr job,const std::string& user,const std::string& password,bool remember,bool enabled){
 Lock lock(mutex);if(!job||isActive(job))throw std::runtime_error("Stop this download before changing its login.");
 if(capturedMedia(job->data))throw std::runtime_error("Sign in to this video site in the browser, then capture its streams again.");
 const auto origin=Url(str(job->data,"Url")).origin;
 if(!str(job->data,"AuthenticationOrigin").empty()&&str(job->data,"AuthenticationOrigin")!=origin)throw std::runtime_error("The login challenge came from a different site. Open the download page and obtain its direct link first.");
 if(remember&&Url(origin).scheme!="https")throw std::runtime_error("Remembered site logins require HTTPS.");
 auto headers=readHeaders(job->data);setBasicLogin(headers,user,password,enabled);validateHeaders(headers);
 auto before=job->data,settings=state["Settings"];
 try{
  job->data["ProtectedHeaders"]=headers.empty()?"":protect(legacyDictionary(Json(headers)).dump());job->data["AuthenticationPromptPending"]=false;
  if(remember&&enabled){auto& logins=state["Settings"]["SiteLogins"];logins.erase(std::remove_if(logins.begin(),logins.end(),[&](const Json& login){return str(login,"Origin")==origin&&str(login,"Path","/")=="/";}),logins.end());logins.push_back(makeSiteLogin(origin,user,password));}
  save();
 }catch(...){job->data=before;state["Settings"]=settings;throw;}
}

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
 auto before=job->data,next=before;if(capturedMedia(before)&&edit.contains("Url")&&str(edit,"Url")!=str(before,"Url"))throw std::runtime_error("Refresh video streams using the browser panel.");for(const char* key:{"Url","Description","DownloadPage","ProtectedHeaders","Category","Queue","Connections","LimitKbps","ExpectedSha256","SuppressCompletionDialog"})if(edit.contains(key))next[key]=edit[key];
 if(str(next,"Url")!=str(before,"Url")){next.erase("ProtectedResolvedUrl");next.erase("AuthenticationOrigin");next.erase("AuthenticationScheme");next["AuthenticationPromptPending"]=false;}
 Url url(str(next,"Url"));if(!str(next,"DownloadPage").empty()){Url page(str(next,"DownloadPage"));if(page.scheme!="http"&&page.scheme!="https")throw std::runtime_error("The parent page must use HTTP or HTTPS.");}
 if(url.origin!=Url(str(before,"Url")).origin){auto headers=readHeaders(next);for(auto it=headers.begin();it!=headers.end();)if(lower(it->first)=="authorization"||lower(it->first)=="cookie"||lower(it->first)=="referer")it=headers.erase(it);else ++it;next["ProtectedHeaders"]=headers.empty()?"":protect(legacyDictionary(Json(headers)).dump());}
 validateFileMetadata(next);auto cats=categories();if(std::find(cats.begin(),cats.end(),str(next,"Category"))==cats.end())throw std::runtime_error("Choose an existing category.");bool found=false;for(auto& q:state["Queues"])found|=str(q,"Name")==str(next,"Queue");if(!found)throw std::runtime_error("Choose an existing queue.");job->data=next;try{save();}catch(...){job->data=before;throw;}
}
JobPtr Manager::redownload(JobPtr job){
 Lock lock(mutex);if(!job||isActive(job)||(str(job->data,"Status")!="Complete"&&!((str(job->data,"Status")=="Paused"||str(job->data,"Status")=="Failed")&&Url(str(job->data,"Url")).scheme=="ftp")))throw std::runtime_error("Select a completed download or a stopped FTP download to download again.");
 if(!str(job->data,"SourceUrl").empty()||!str(job->data,"ProtectedAdaptive").empty())throw std::runtime_error("Choose fresh video streams using the browser panel.");
 if(job->data.contains("OfflineProject")){auto project=job->data["OfflineProject"];project["Name"]=utf8(fs::path(wide(str(job->data,"FileName"))).stem().wstring());project["Folder"]=str(job->data,"Folder");project["Id"]=str(job->data,"ProjectId");return addOfflineProject(project,str(job->data,"Queue"),true);}
 auto copy=add(str(job->data,"Url"),str(job->data,"Folder"),str(job->data,"FileName"),str(job->data,"Queue"),true,readHeaders(job->data),str(job->data,"ExpectedSha256"),readPostRequest(job->data),readBrowserProxy(job->data));
 try{for(const char* key:{"Description","DownloadPage","Connections","LimitKbps","Category"})if(job->data.contains(key))copy->data[key]=job->data[key];copy->data["RedownloadOf"]=job->id();save();}catch(...){jobs.erase(std::remove(jobs.begin(),jobs.end(),copy),jobs.end());throw;}return copy;
}
void Manager::setMembership(JobPtr job,bool member,const std::string& queue){
 Lock lock(mutex);if(!job||isActive(job))throw std::runtime_error("Pause this download before changing its queue membership.");
 if(member&&str(job->data,"Status")=="Complete")throw std::runtime_error("Use Redownload to queue a new copy of a completed file.");
 auto before=job->data;if(!queue.empty()){bool found=false;for(const auto& q:state["Queues"])found|=str(q,"Name")==queue;if(!found)throw std::runtime_error("Choose an existing queue.");job->data["Queue"]=queue;}
 job->data["QueueMember"]=member;if(!member&&str(job->data,"Status")=="Queued")job->data["Status"]="Paused";
 try{save();}catch(...){job->data=before;throw;}
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
 job->data["CompletionForce"]=force;job->data["CompletionActionDelay"]=delay;job->data["CompletionWaitForOthers"]=wait;job->data["CompletionActionArmed"]=!actions.empty();
 try{save();}catch(...){job->data=before;throw;}
}
Json Manager::takeDownloadCompletion(JobPtr job){
 Lock lock(mutex);if(!job||std::find(jobs.begin(),jobs.end(),job)==jobs.end()||isActive(job)||!scannerAllowsCompletion(job->data)||str(job->data,"Status")!="Complete"||!yes(job->data,"CompletionActionArmed"))return Json::object();
 std::vector<std::string> actions;try{actions=completionSteps(job->data);}catch(...){actions.clear();}
 auto before=job->data;job->data["CompletionActionArmed"]=false;try{save();}catch(...){job->data=before;throw;}
 if(actions.empty())return Json::object();
 return {{"Id",job->id()},{"Action",completionSummary(actions)},{"Actions",actions},{"Force",yes(job->data,"CompletionForce")&&completionCanForce(actions)},{"DelaySeconds",std::clamp<i64>(num(job->data,"CompletionActionDelay",30),15,3600)},{"WaitForOthers",yes(job->data,"CompletionWaitForOthers",true)}};
}
void Manager::beginPrefetch(JobPtr job){
 Lock lock(mutex);if(job->data.contains("OfflineProject")||!str(job->data,"DuplicateOf").empty())return;if(!str(job->data,"ProtectedRequest").empty()||yes(job->data,"RequiresRequestCapture"))return;if(!yes(state["Settings"],"PrefetchFileInfo")||isActive(job)||str(job->data,"Status")=="Complete"||!str(job->data,"SourceUrl").empty()||!str(job->data,"ProtectedAdaptive").empty()||Url(str(job->data,"Url")).scheme=="ftp"||active.size()>=(size_t)num(state["Settings"],"Parallel",3))return;
 job->data["ConfirmationPending"]=true;save();start(job);
}
void Manager::endPrefetch(JobPtr job){
 {Lock lock(mutex);if(!yes(job->data,"ConfirmationPending"))return;if(isActive(job))active[job->id()]->stop=true;}
 auto deadline=GetTickCount64()+5000;while(isActive(job)){if(GetTickCount64()>deadline)throw std::runtime_error("The background transfer is still stopping. Please retry in a moment.");Sleep(10);}
 Lock lock(mutex);job->data["ConfirmationPending"]=false;job->data["Status"]="Paused";save();
}
void Manager::editCategory(const std::string& original,const std::string& name,const std::string& extensions,const std::string& hosts,const std::string& folder){
 Lock lock(mutex);auto cats=categories();auto found=std::find(cats.begin(),cats.end(),original);if(!original.empty()&&found==cats.end())throw std::runtime_error("That category no longer exists.");if(name.empty()||name.size()>80||safeName(name)!=name)throw std::runtime_error("Choose a valid category name up to 80 characters.");for(auto c:cats)if(c!=original&&lower(c)==lower(name))throw std::runtime_error("That category already exists.");auto& custom=state["Settings"]["CustomCategories"];bool builtIn=!original.empty()&&std::find(custom.begin(),custom.end(),Json(original))==custom.end();if(builtIn&&name!=original)throw std::runtime_error("Built-in categories keep their names.");if(!folder.empty()&&!fs::path(wide(folder)).is_absolute())throw std::runtime_error("Choose an absolute category folder.");for(auto ext:words(extensions))if(ext!="*"&&!std::regex_match(ext,std::regex("[a-z0-9_-]{1,30}")))throw std::runtime_error("Use extensions separated by spaces, such as zip pdf mp4.");for(auto host:words(hosts)){if(host.rfind("*.",0)==0)host.erase(0,2);if(Url("https://"+host+"/").host!=lower(host))throw std::runtime_error("Use site host names without paths or ports.");}
 auto before=state["Settings"];std::vector<Json> oldJobs;for(auto j:jobs)oldJobs.push_back(j->data);try{if(original.empty())custom.push_back(name);else if(!builtIn)for(auto& c:custom)if(c==original)c=name;auto& rules=state["Settings"]["CategoryRules"];rules.erase(std::remove_if(rules.begin(),rules.end(),[&](const Json& r){return str(r,"Category")==original||str(r,"Category")==name;}),rules.end());if(!trim(extensions).empty())rules.insert(rules.begin(),Json{{"Category",name},{"Extensions",lower(trim(extensions))},{"Hosts",lower(trim(hosts))}});auto paths=dictionary(state["Settings"]["CategoryPaths"]);paths.erase(original);if(folder.empty())paths.erase(name);else paths[name]=folder;state["Settings"]["CategoryPaths"]=legacyDictionary(paths);auto memory=state["Settings"].value("CategoryRememberLast",Json::object());if(memory.contains(original)){memory[name]=memory[original];if(name!=original)memory.erase(original);}state["Settings"]["CategoryRememberLast"]=memory;auto types=state["Settings"].value("CategoryTypeOverrides",Json::object());if(types.contains(original)){types.erase(original);if(trim(hosts).empty())types[name]=lower(trim(extensions));}state["Settings"]["CategoryTypeOverrides"]=types;if(!original.empty())for(auto j:jobs)if(str(j->data,"Category")==original)j->data["Category"]=name;save();}catch(...){state["Settings"]=before;for(size_t i=0;i<jobs.size();++i)jobs[i]->data=oldJobs[i];throw;}
}
void Manager::deleteCategory(const std::string& name){
 Lock lock(mutex);auto& custom=state["Settings"]["CustomCategories"];auto found=std::find(custom.begin(),custom.end(),Json(name));if(found==custom.end())throw std::runtime_error("Only custom categories can be deleted.");auto before=state["Settings"];std::vector<Json> oldJobs;for(auto j:jobs)oldJobs.push_back(j->data);try{custom.erase(found);auto& rules=state["Settings"]["CategoryRules"];rules.erase(std::remove_if(rules.begin(),rules.end(),[&](const Json& r){return str(r,"Category")==name;}),rules.end());auto paths=dictionary(state["Settings"]["CategoryPaths"]);paths.erase(name);state["Settings"]["CategoryPaths"]=legacyDictionary(paths);if(state["Settings"].contains("CategoryRememberLast"))state["Settings"]["CategoryRememberLast"].erase(name);if(state["Settings"].contains("CategoryTypeOverrides"))state["Settings"]["CategoryTypeOverrides"].erase(name);for(auto j:jobs)if(str(j->data,"Category")==name)j->data["Category"]="Other";save();}catch(...){state["Settings"]=before;for(size_t i=0;i<jobs.size();++i)jobs[i]->data=oldJobs[i];throw;}
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
void Manager::recycleCompleted(JobPtr job,HWND owner){
 Lock lock(mutex);if(!job||isActive(job)||str(job->data,"Status")!="Complete")throw std::runtime_error("Only an inactive completed file can be recycled.");
 for(auto other:jobs)if(str(other->data,"ReplacementOf")==job->id())throw std::runtime_error("A replacement still depends on this file. Remove the pending replacement first.");
 auto path=job->target();DWORD attributes=GetFileAttributesW(path.c_str());if(!path.is_absolute()||attributes==INVALID_FILE_ATTRIBUTES||(attributes&(FILE_ATTRIBUTE_DIRECTORY|FILE_ATTRIBUTE_REPARSE_POINT)))throw std::runtime_error("Choose an existing regular downloaded file.");
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
