#include "GrabberLinks.hpp"
#include "GrabberProject.hpp"
#include "OfflineSite.hpp"
#include <bcrypt.h>
#include <fstream>
#include <regex>
namespace udm {
namespace {
std::string digest(const std::string& value){BCRYPT_ALG_HANDLE algorithm=nullptr;if(BCryptOpenAlgorithmProvider(&algorithm,BCRYPT_SHA256_ALGORITHM,nullptr,0)<0)throw std::runtime_error("Cannot hash link-conversion state.");unsigned char bytes[32]{};auto result=BCryptHash(algorithm,nullptr,0,(PUCHAR)value.data(),(ULONG)value.size(),bytes,sizeof(bytes));BCryptCloseAlgorithmProvider(algorithm,0);if(result<0)throw std::runtime_error("Cannot hash link-conversion state.");std::string out;const char* hex="0123456789abcdef";for(auto byte:bytes){out+=hex[byte>>4];out+=hex[byte&15];}return out;}
std::string urlKey(const std::string& address){Url url(address);return url.origin+url.path+url.query;}
std::string pathKey(const fs::path& path){return lower(utf8(fs::absolute(path).lexically_normal().wstring()));}
std::string uriPath(const std::string& value){std::string out;const char* hex="0123456789ABCDEF";for(unsigned char c:value){if((c>='a'&&c<='z')||(c>='A'&&c<='Z')||(c>='0'&&c<='9')||c=='/'||c=='-'||c=='_'||c=='.'||c=='~'||c==':')out+=(char)c;else{out+='%';out+=hex[c>>4];out+=hex[c&15];}}return out;}
bool ordinary(const fs::path& path){auto attributes=GetFileAttributesW(path.c_str());return attributes!=INVALID_FILE_ATTRIBUTES&&!(attributes&(FILE_ATTRIBUTE_REPARSE_POINT|FILE_ATTRIBUTE_DIRECTORY));}
std::string rawText(const fs::path& path){if(!ordinary(path)||fs::file_size(path)>2*1024*1024)throw std::runtime_error("Link conversion requires an ordinary HTML/CSS file no larger than 2 MiB.");std::ifstream in(path,std::ios::binary);if(!in)throw std::runtime_error("Cannot read the downloaded page.");std::string text(std::istreambuf_iterator<char>(in),{});if(in.bad())throw std::runtime_error("Cannot finish reading the downloaded page.");return text;}
void clean(const fs::path& path,const std::string& hash){try{if(ordinary(path)&&fileHash(path)==hash){std::error_code ignored;fs::remove(path,ignored);}}catch(...){}}
fs::path originalPath(const Manager& manager,JobPtr job){auto folder=manager.root/L"grabber-originals";auto attributes=GetFileAttributesW(folder.c_str());if(attributes!=INVALID_FILE_ATTRIBUTES&&(attributes&FILE_ATTRIBUTE_REPARSE_POINT))throw std::runtime_error("The original-page cache is redirected. Restore its local folder before converting links.");return folder/(wide(job->id())+L".bin");}
bool sameFile(const fs::path& path,const std::string& hash){return ordinary(path)&&fileHash(path)==hash;}
bool cssDocument(const Json& data){auto type=lower(str(data,"ContentType",str(data,"GrabberContentType")));return type.find("text/css")!=std::string::npos||lower(utf8(fs::path(wide(str(data,"FileName"))).extension().wstring()))==".css";}
Json projectJobs(const Manager& manager,const Json& project){Json result=Json::array();for(auto job:manager.jobs)if(str(job->data,"ProjectId")==str(project,"Id")&&!job->data.contains("OfflineProject")&&str(job->data,"PreviousVersionOf").empty()&&str(job->data,"RecycledAt").empty()){auto path=job->target();std::error_code ec;bool exists=fs::is_regular_file(path,ec);i64 size=-1,stamp=0;if(exists){auto length=fs::file_size(path,ec);if(!ec)size=(i64)length;auto time=fs::last_write_time(path,ec);if(!ec)stamp=(i64)time.time_since_epoch().count();}result.push_back({{"Id",job->id()},{"Url",str(job->data,"Url")},{"Path",pathKey(path)},{"Status",str(job->data,"Status")},{"Hash",str(job->data,"Sha256")},{"Size",size},{"Stamp",stamp}});}return result;}
}
std::string grabberLocalReference(const fs::path& document,const fs::path& destination){
 auto from=fs::absolute(document).lexically_normal(),to=fs::absolute(destination).lexically_normal();if(lower(utf8(from.root_name().wstring()))==lower(utf8(to.root_name().wstring()))){auto relative=to.lexically_relative(from.parent_path());if(!relative.empty()&&!relative.is_absolute())return uriPath(utf8(relative.generic_wstring()));}
 auto value=utf8(to.generic_wstring());return uriPath(value.rfind("//",0)==0?"file:"+value:"file:///"+value);
}
bool grabberLinkDocument(const Json& data){if(data.contains("OfflineProject"))return false;auto type=lower(str(data,"ContentType",str(data,"GrabberContentType")));auto extension=lower(utf8(fs::path(wide(str(data,"FileName"))).extension().wstring()));return type.find("html")!=std::string::npos||type.find("text/css")!=std::string::npos||extension==".html"||extension==".htm"||extension==".xhtml"||extension==".css";}
std::string grabberLinksSignature(const Manager& manager,const Json& project){Lock lock(manager.mutex);return digest(Json{{"Version",1},{"Project",str(project,"Id")},{"Links",project.value("Links",Json::array())},{"Jobs",projectJobs(manager,project)}}.dump());}
bool convertGrabberFile(Manager& manager,JobPtr job,const Json& project,const std::map<std::string,fs::path>& files,const Cancel& cancel){
 cancel.check();Json before;fs::path target;{Lock lock(manager.mutex);if(str(job->data,"Status")!="Complete"||str(job->data,"ProjectId")!=str(project,"Id")||!grabberLinkDocument(job->data))return false;before=job->data;target=job->target();validateGrabberFolder(before);}
 Handle held(CreateFileW(target.c_str(),GENERIC_READ,FILE_SHARE_READ|FILE_SHARE_DELETE,nullptr,OPEN_EXISTING,0,nullptr));if(!held)throw std::runtime_error("The downloaded page is in use.");auto current=rawText(target),currentHash=digest(current);if(currentHash!=str(before,"Sha256"))throw std::runtime_error("The downloaded page changed outside UDM. Its contents were preserved.");
 auto cache=originalPath(manager,job);std::string sourceHash=str(before,"GrabberSourceHash"),source;
 if(sourceHash.empty()){source=current;sourceHash=currentHash;if(fs::exists(cache)&&!ordinary(cache))throw std::runtime_error("The original-page cache is not an ordinary file.");atomicText(cache,source,false);}
 else{if(!sameFile(cache,sourceHash))throw std::runtime_error("The original page copy is missing or changed. The downloaded page was preserved.");source=rawText(cache);}
 if(source.find('\0')!=std::string::npos||(source.size()>=2&&((unsigned char)source[0]==0xff||(unsigned char)source[0]==0xfe)))throw std::runtime_error("Local-link conversion requires an ASCII-compatible HTML/CSS encoding. This document was preserved.");
 std::map<std::string,std::string> local;for(const auto& item:files)local[item.first]=grabberLocalReference(target,item.second);
 auto address=str(before,"Url");auto resolved=reveal(str(before,"ProtectedResolvedUrl"));if(!resolved.empty())address=resolved;
 auto output=rewriteGrabberDocument(source,address,cssDocument(before),local),newHash=digest(output);auto after=before;after["GrabberSourceHash"]=sourceHash;after["GrabberSourceBytes"]=source.size();after["GrabberConvertedHash"]=newHash;after["Sha256"]=newHash;after["Size"]=output.size();after["Received"]=output.size();after["GrabberLinksConvertedAt"]=date();if(newHash!=currentHash)after.erase("ScanResult");
 cancel.check();Lock lock(manager.mutex);if(job->data!=before||job->target()!=target)throw std::runtime_error("The download changed during link conversion.");
 if(newHash==currentHash){job->data=after;try{manager.save();}catch(...){job->data=before;throw;}return false;}
 auto token=guid();auto staging=target.parent_path()/(L".udm-links-"+wide(job->id())+L"-"+wide(token)+L".after"),previous=target.parent_path()/(L".udm-links-"+wide(job->id())+L"-"+wide(token)+L".before");auto journal=manager.root/L"link-conversions"/(wide(job->id())+L".json");auto journalAttributes=GetFileAttributesW(journal.parent_path().c_str());if(journalAttributes!=INVALID_FILE_ATTRIBUTES&&(journalAttributes&FILE_ATTRIBUTE_REPARSE_POINT))throw std::runtime_error("Link-conversion recovery folder is redirected.");if(fs::exists(journal))throw std::runtime_error("A link conversion needs recovery. Restart UDM before retrying.");
 atomicText(staging,output,false);auto stamp=fs::last_write_time(target);fs::last_write_time(staging,stamp);if(!sameFile(target,currentHash)){clean(staging,newHash);throw std::runtime_error("The page changed before conversion. No changes were published.");}
 Json operation={{"Version",1},{"Id",job->id()},{"Token",token},{"Target",utf8(target.wstring())},{"OldHash",currentHash},{"NewHash",newHash},{"Before",before},{"After",after}};
 try{atomicText(journal,operation.dump(),false);}catch(...){clean(staging,newHash);throw;}
 if(!ReplaceFileW(target.c_str(),staging.c_str(),previous.c_str(),0,nullptr,nullptr)){auto code=GetLastError();if(sameFile(target,currentHash)&&!fs::exists(previous)){std::error_code ignored;fs::remove(journal,ignored);clean(staging,newHash);}throw std::runtime_error("Windows could not convert the page (error "+std::to_string(code)+"). Original files and recovery information were retained.");}
 CloseHandle(held.h);held.h=INVALID_HANDLE_VALUE;
 if(!sameFile(previous,currentHash)){ReplaceFileW(target.c_str(),previous.c_str(),staging.c_str(),0,nullptr,nullptr);throw std::runtime_error("The page changed during replacement. Conversion stopped with recovery information preserved.");}
 job->data=after;try{manager.save();}catch(...){job->data=before;if(ReplaceFileW(target.c_str(),previous.c_str(),staging.c_str(),0,nullptr,nullptr)){std::error_code ignored;fs::remove(journal,ignored);clean(staging,newHash);}throw;}
 markZone(target);std::error_code ignored;fs::remove(journal,ignored);if(!ignored)clean(previous,currentHash);return true;
}
void Manager::recoverLinkConversions(){
 auto folder=root/L"link-conversions";if(!fs::exists(folder))return;auto attrs=GetFileAttributesW(folder.c_str());if(attrs&FILE_ATTRIBUTE_REPARSE_POINT)throw std::runtime_error("Link-conversion recovery folder is redirected.");
 for(const auto& entry:fs::directory_iterator(folder)){if(entry.path().extension()!=L".json")continue;if(!ordinary(entry.path()))throw std::runtime_error("Invalid link-conversion journal.");auto op=Json::parse(readText(entry.path(),1024*1024));auto id=str(op,"Id"),token=str(op,"Token");if(num(op,"Version")!=1||id!=utf8(entry.path().stem().wstring())||!std::regex_match(token,std::regex("[a-f0-9]{32}")))throw std::runtime_error("Invalid link-conversion identity.");
  JobPtr job;for(auto item:jobs)if(item->id()==id)job=item;if(!job)throw std::runtime_error("Link recovery requires its original download history.");auto target=job->target();if(pathKey(target)!=pathKey(fs::path(wide(str(op,"Target"))))||str(op["Before"],"Id")!=id||str(op["After"],"Id")!=id||op["Before"].value("Folder",Json())!=op["After"].value("Folder",Json())||op["Before"].value("FileName",Json())!=op["After"].value("FileName",Json()))throw std::runtime_error("Link recovery no longer matches the download location.");validateGrabberFolder(job->data);
  auto previous=target.parent_path()/(L".udm-links-"+wide(id)+L"-"+wide(token)+L".before"),staging=target.parent_path()/(L".udm-links-"+wide(id)+L"-"+wide(token)+L".after");auto oldHash=str(op,"OldHash"),newHash=str(op,"NewHash");
  const auto& before=op.at("Before");const auto& after=op.at("After");
  if(!std::regex_match(oldHash,std::regex("[a-f0-9]{64}"))||!std::regex_match(newHash,std::regex("[a-f0-9]{64}"))||oldHash==newHash||str(before,"Sha256")!=oldHash||str(after,"Sha256")!=newHash||str(after,"GrabberConvertedHash")!=newHash||!std::regex_match(str(after,"GrabberSourceHash"),std::regex("[a-f0-9]{64}")))throw std::runtime_error("Invalid link-conversion integrity metadata.");
  auto identity=[](Json value){for(const char* key:{"Audio","Video"})if(value.contains(key)&&value[key].is_null())value.erase(key);for(const char* key:{"Sha256","Size","Received","GrabberSourceHash","GrabberSourceBytes","GrabberConvertedHash","GrabberLinksConvertedAt","ScanResult","PeakSpeed","ConfirmationPending"})value.erase(key);return value;};
  if(identity(before)!=identity(after)||identity(job->data)!=identity(before)||str(before,"Status")!="Complete"||str(before,"ProjectId").empty()||num(after,"Size",-1)<0||num(after,"Received",-1)!=num(after,"Size")||pathKey(fs::path(wide(str(before,"Folder")))/wide(str(before,"FileName")))!=pathKey(target))throw std::runtime_error("Link-conversion recovery metadata no longer matches its download.");
  bool newTarget=sameFile(target,newHash),oldTarget=sameFile(target,oldHash),oldBackup=sameFile(previous,oldHash);
  if(newTarget&&num(after,"Size")!=(i64)fs::file_size(target))throw std::runtime_error("Converted-page recovery size is invalid.");
  if(newTarget&&(oldBackup||(!fs::exists(previous)&&str(job->data,"Sha256")==newHash)))job->data=after;
  else if(oldTarget&&!fs::exists(previous))job->data=op["Before"];
  else if(!fs::exists(target)&&oldBackup){if(!MoveFileExW(previous.c_str(),target.c_str(),MOVEFILE_WRITE_THROUGH))throw std::runtime_error("Cannot restore the page before conversion.");job->data=op["Before"];}
  else throw std::runtime_error("A converted page needs manual recovery. Preserve the journal and both file versions.");
  save();fs::remove(entry.path());clean(previous,oldHash);clean(staging,newHash);
 }
}
bool Manager::convertProjectLinks(const std::string& id,bool force){
 Lock lock(mutex);if(stopping||convertingProjects.count(id))return false;auto project=grabberProject(*this,id);if(!yes(project,"ConvertLinks")||str(project,"Template")=="Offline website (ZIP)"||yes(project,"ExplorationRunning"))return false;
 std::vector<JobPtr> selected;bool documents=false;for(auto job:jobs)if(str(job->data,"ProjectId")==id&&!job->data.contains("OfflineProject")&&str(job->data,"PreviousVersionOf").empty()&&str(job->data,"RecycledAt").empty()){if(isActive(job)||str(job->data,"Status")=="Queued")return false;if(str(job->data,"Status")=="Complete"){selected.push_back(job);documents|=grabberLinkDocument(job->data);}}
 if(!documents)return false;auto signature=grabberLinksSignature(*this,project);if(!force&&signature==str(project,"LinkConversionSignature"))return false;auto cancel=std::make_shared<Cancel>();for(auto job:selected)active[job->id()]=cancel;convertingProjects.insert(id);auto prior=project;project["LinkConversionState"]="Converting";project["LinkConversionErrors"]=Json::array();
 try{saveProject(project);threads.emplace_back([this,id,selected,cancel,project]{
  Json errors=Json::array();int changed=0;bool stopped=false;std::map<std::string,fs::path> files;
  try{
   for(auto job:selected){cancel->check();Json data;fs::path path;{Lock lock(mutex);data=job->data;path=job->target();}if(!ordinary(path))continue;files[urlKey(str(data,"Url"))]=path;auto discovered=str(data,"GrabberDiscoveredUrl");if(!discovered.empty())files[urlKey(discovered)]=path;auto finalUrl=reveal(str(data,"ProtectedResolvedUrl"));if(!finalUrl.empty())files[urlKey(finalUrl)]=path;}
   for(auto job:selected){cancel->check();try{changed+=convertGrabberFile(*this,job,project,files,*cancel);}catch(const Cancelled&){throw;}catch(const std::exception& e){errors.push_back({{"Id",job->id()},{"Message",e.what()}});Lock lock(mutex);if(!storageError.empty())break;}}
  }catch(const Cancelled&){stopped=true;}catch(const std::exception& e){errors.push_back({{"Message",e.what()}});}
  Lock lock(mutex);for(auto job:selected){auto found=active.find(job->id());if(found!=active.end()&&found->second==cancel)active.erase(found);schedulePaused.erase(job->id());}convertingProjects.erase(id);
  try{auto current=grabberProject(*this,id);current["LinkConversionState"]=stopped?"Stopped":errors.empty()?"Complete":"Needs attention";current["LinkConversionErrors"]=errors;current["LinkConversionChanged"]=changed;current["LinkConversionSignature"]=grabberLinksSignature(*this,current);current["LinkConversionFinished"]=date();saveProject(current);}catch(const std::exception& e){storageError=e.what();}
 });}catch(...){for(auto job:selected)active.erase(job->id());convertingProjects.erase(id);for(auto& item:state["Projects"])if(str(item,"Id")==id)item=prior;try{save();}catch(...){}throw;}return true;
}
void Manager::projectLinksTick(){if(ticks%3)return;auto projects=state["Projects"];for(const auto& project:projects)if(yes(project,"ConvertLinks"))try{convertProjectLinks(str(project,"Id"));}catch(const std::exception& e){storageError=e.what();}}
}
