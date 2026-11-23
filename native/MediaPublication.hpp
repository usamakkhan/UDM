#pragma once
#include "PublicationRecovery.hpp"
#include "ScopedPublicationSource.hpp"

namespace udm {
std::string retainedYouTubeOutputBinding(const Job& job);

inline fs::path retainedMediaOutputPath(const Json& data){
 const auto receipt=data.find("RetainedMediaOutput");
 if(receipt==data.end()||num(*receipt,"Version")!=1)return {};
 const auto id=str(data,"Id");
 if(id.empty()||id.find_first_not_of("0123456789abcdefABCDEF-")!=std::string::npos)return {};
 auto path=fs::path(wide(str(*receipt,"Path")));
 const auto container=str(*receipt,"Container");
 if(!container.empty()&&container!="mp4"&&container!="m4a"&&container!="ts")return {};
 const auto kind=str(*receipt,"Kind");
 if(kind=="LiveDash"){
  const auto mediaFolder=fs::path(wide(str(data,"MediaFolder")));
  const auto folder=(mediaFolder/L"live-dash").lexically_normal();
  static const std::wregex filename(LR"(assembled-[0-9a-f]{32}\.(mp4|m4a))");
  if(!yes(data,"LiveRecording")||!yes(data,"LiveCaptureSealed")||!mediaFolder.is_absolute()||
     (container!="mp4"&&container!="m4a")||path.parent_path()!=folder||
     path.extension()!=L"."+wide(container)||!std::regex_match(path.filename().wstring(),filename))return {};
  for(auto parent=folder;!parent.empty();parent=parent.parent_path()){
   const auto flags=GetFileAttributesW(parent.c_str());
   if(flags==INVALID_FILE_ATTRIBUTES||!(flags&FILE_ATTRIBUTE_DIRECTORY)||(flags&FILE_ATTRIBUTE_REPARSE_POINT))return {};
   if(parent==parent.root_path())break;
  }
  return path;
 }
 if(!kind.empty()&&(kind!="LiveHls"||container.empty()||!yes(data,"LiveRecording")))return {};
 const auto suffix=container.empty()?(str(data,"MediaOutput")=="audio"?L".muxing.m4a":L".muxing.mp4"):
                   (kind=="LiveHls"?L".live.":L".adaptive.")+wide(container);
 const auto filename=L".udm-"+wide(id)+suffix;
 if(!path.is_absolute()||path.filename()!=filename)return {};
 return path;
}
inline void discardRetainedMediaOutput(const Json& data)noexcept{try{
 const auto path=retainedMediaOutputPath(data);
 if(path.empty())return;
 const auto attributes=GetFileAttributesW(path.c_str());
 if(attributes==INVALID_FILE_ATTRIBUTES||(attributes&(FILE_ATTRIBUTE_REPARSE_POINT|FILE_ATTRIBUTE_DIRECTORY)))return;
 std::error_code ignored;fs::remove(path,ignored);
}catch(...) {}}
// Keep the measured file open without write or delete sharing across
// confirmation, then transfer protection to an identity-matched reader.
struct VerifiedMediaFile {
 Handle file;
 BY_HANDLE_FILE_INFORMATION identity{};
 i64 size=0;
 explicit VerifiedMediaFile(const fs::path& path):
  file(CreateFileW(path.c_str(),GENERIC_READ,FILE_SHARE_READ,nullptr,
                   OPEN_EXISTING,FILE_FLAG_OPEN_REPARSE_POINT|FILE_FLAG_SEQUENTIAL_SCAN,nullptr)){
  LARGE_INTEGER length{};
  if(!file||!GetFileInformationByHandle(file.h,&identity)||
     (identity.dwFileAttributes&(FILE_ATTRIBUTE_DIRECTORY|FILE_ATTRIBUTE_REPARSE_POINT))||
     !GetFileSizeEx(file.h,&length)||length.QuadPart<=0)
   throw std::runtime_error("Cannot verify the assembled media file.");
  size=length.QuadPart;
 }
 std::unique_ptr<PublicationSourcePin> releaseForPublication(const fs::path& path){
  auto pin=PublicationSourcePin::openReadFrom(path,file.h);
  if(!pin)
   throw std::runtime_error("The prepared media file changed before publication. Resume to verify it again.");
  CloseHandle(file.h);
  file.h=INVALID_HANDLE_VALUE;
  return pin;
 }
};
// The size comes from the held assembled file. Confirmation and retained-output
// receipts still require the digest, which may take longer to compute.
class ScopedAssembledPrefetchSize {
 Manager& manager;
 Job& job;
 bool announced=false;
public:
 ScopedAssembledPrefetchSize(Manager& manager,Job& job,const Cancel& cancel,i64 size):manager(manager),job(job){
  Lock lock(manager.mutex);
  cancel.check();
  if(size>0&&yes(job.data,"ConfirmationPending")){
   job.assembledPrefetchSize=size;
   announced=true;
  }
 }
 ScopedAssembledPrefetchSize(const ScopedAssembledPrefetchSize&)=delete;
 ScopedAssembledPrefetchSize& operator=(const ScopedAssembledPrefetchSize&)=delete;
 ~ScopedAssembledPrefetchSize(){
  if(!announced)return;
  Lock lock(manager.mutex);
  job.assembledPrefetchSize=-1;
 }
};
// Keep the verified source until publication commits. A sibling on the selected
// volume lets the existing publication journal retain its atomic rename boundary.
inline void publishMediaOutput(Manager& manager,JobPtr job,const fs::path& source,
                               const std::string& digest,const std::shared_ptr<Cancel>& cancel,const LivePublicationDetails* live=nullptr,std::unique_ptr<PublicationSourcePin> verifiedSource={}){
 cancel->check();requirePublicationRecovered(manager,job);
 auto sourcePin=verifiedSource?std::move(verifiedSource):PublicationSourcePin::openRead(source);if(!sourcePin)throw std::runtime_error("Cannot keep the verified file stable for publication. Its source was retained.");
 CreatedStagingFile sourceProof;sourcePin->captureCleanup(sourceProof);
 auto sourceLifetime=sourcePin->duplicate();if(!sourceLifetime)throw std::runtime_error("Cannot retain the verified source object. Its source was retained.");
 const auto folder=job->target().parent_path();
 if(folder.lexically_normal()==source.parent_path().lexically_normal()){
  ScopedPublicationSource handoff(manager,job,source,std::move(sourcePin));manager.publishFile(job,source,digest,cancel.get(),live);return;
 }
 fs::create_directories(folder);
 const auto staging=folder/(L".udm-"+wide(job->id())+L"-"+wide(guid())+L".publishing");
 bool ownsStaging=false;CreatedStagingFile stageProof;std::optional<Handle> stageLifetime,nameGuard;
 try{
  ownsStaging=CreateHardLinkW(staging.c_str(),source.c_str(),nullptr)!=FALSE;
  std::unique_ptr<PublicationSourcePin> publicationPin;
  if(ownsStaging){stageProof=sourceProof;publicationPin=std::move(sourcePin);}
  else{
   auto input=sourcePin->duplicate();if(!input)throw std::runtime_error("Cannot read the verified media file.");
   LARGE_INTEGER beginning{};if(!SetFilePointerEx(input.h,beginning,nullptr,FILE_BEGIN))throw std::runtime_error("Cannot seek the verified media file for copying.");
   {
    Handle output(CreateFileW(staging.c_str(),GENERIC_READ|GENERIC_WRITE,FILE_SHARE_READ|FILE_SHARE_DELETE,nullptr,CREATE_NEW,FILE_FLAG_SEQUENTIAL_SCAN,nullptr));
    if(!output)throw std::runtime_error("Cannot prepare media in the selected folder.");
    ownsStaging=true;stageProof.capture(output.h);
    HANDLE retained=INVALID_HANDLE_VALUE;if(!DuplicateHandle(GetCurrentProcess(),output.h,GetCurrentProcess(),&retained,0,FALSE,DUPLICATE_SAME_ACCESS))throw std::runtime_error("Cannot retain the created media file.");stageLifetime.emplace(retained);
    std::vector<char> bytes(1024*1024);DWORD count=0;
    for(;;){cancel->check();if(!ReadFile(input.h,bytes.data(),(DWORD)bytes.size(),&count,nullptr))throw std::runtime_error("Cannot read the verified media file.");if(!count)break;
     DWORD written=0;if(!WriteFile(output.h,bytes.data(),count,&written,nullptr)||written!=count)throw std::runtime_error("Cannot copy media to the selected folder.");}
    cancel->check();if(!FlushFileBuffers(output.h))throw std::runtime_error("Cannot flush media in the selected folder.");
    nameGuard.emplace(CreateFileW(staging.c_str(),GENERIC_READ,FILE_SHARE_READ|FILE_SHARE_WRITE,nullptr,OPEN_EXISTING,FILE_FLAG_OPEN_REPARSE_POINT,nullptr));
    if(!*nameGuard||!stageProof.matches(nameGuard->h,stageLifetime->h))throw std::runtime_error("The created media name changed. Its source was retained.");
   }
   // Keep the name protected while the last writer closes and its metadata
   // settles, then acquire a reader that excludes all subsequent writers.
   stageLifetime.reset();
   // Move the existing one-pass copy verification onto a strict read pin after
   // the writer closes. Its identity must still match the newly created file.
   publicationPin=PublicationSourcePin::openRead(staging);
   if(!publicationPin||!publicationPin->matches(stageProof,nameGuard->h))throw std::runtime_error("The copied media source changed. Its source was retained.");
   stageLifetime.emplace(publicationPin->duplicate());if(!*stageLifetime)throw std::runtime_error("Cannot retain the copied media object.");nameGuard.reset();
   if(publicationPin->hash([&]{cancel->check();})!=digest)throw std::runtime_error("The copied media failed verification. Source tracks were retained.");
  }
  if(!stageLifetime){stageLifetime.emplace(publicationPin->duplicate());if(!*stageLifetime)throw std::runtime_error("Cannot retain the publication source object.");}
  cancel->check();ScopedPublicationSource handoff(manager,job,staging,std::move(publicationPin));manager.publishFile(job,staging,digest,cancel.get(),live);
 }catch(...){if(ownsStaging&&!publicationNeedsRecovery(manager,job))stageProof.removeOwned(staging,stageLifetime?stageLifetime->h:INVALID_HANDLE_VALUE);throw;}
 sourcePin.reset();sourceProof.removeOwned(source,sourceLifetime.h);
}

template<class Binding>
inline bool publishRetainedMediaOutput(Manager& manager,const JobPtr& job,const std::shared_ptr<Cancel>& cancel,Binding selectionBinding){
 Json data;std::string binding;
 {Lock lock(manager.mutex);if(!job->data.contains("RetainedMediaOutput"))return false;data=job->data;binding=selectionBinding(*job);}
 const auto path=retainedMediaOutputPath(data);
 const auto& receipt=data.at("RetainedMediaOutput");
 const auto digest=str(receipt,"Sha256"),expected=lower(str(data,"ExpectedSha256"));
 const auto kind=str(receipt,"Kind");
 const bool live=kind=="LiveHls"||kind=="LiveDash";
 std::optional<i64> subtitleCues;
 if(kind=="LiveDash"&&receipt.contains("SubtitleCueCount")){
  const auto count=num(receipt,"SubtitleCueCount",-1);
  if(receipt["SubtitleCueCount"].is_number_integer()&&count>=0&&count<=100000)subtitleCues=count;
 }
 const bool validCaptions=!receipt.contains("SubtitleCueCount")||subtitleCues.has_value();
 const auto size=num(receipt,"Size",-1);
 if(!path.empty()&&size>0&&validCaptions&&!binding.empty()&&binding==str(receipt,"Binding")&&(expected.empty()||expected==digest)){
  Handle input(CreateFileW(path.c_str(),GENERIC_READ,FILE_SHARE_READ|FILE_SHARE_DELETE,nullptr,OPEN_EXISTING,FILE_FLAG_OPEN_REPARSE_POINT|FILE_FLAG_SEQUENTIAL_SCAN,nullptr));
  if(!input){
   const auto error=GetLastError();
   if(error!=ERROR_FILE_NOT_FOUND&&error!=ERROR_PATH_NOT_FOUND)throw std::runtime_error("Cannot open the prepared media file for verification.");
  }else{
   BY_HANDLE_FILE_INFORMATION info{};
   if(!GetFileInformationByHandle(input.h,&info))throw std::runtime_error("Cannot read prepared media file information.");
   LARGE_INTEGER length{};
   if(!GetFileSizeEx(input.h,&length))throw std::runtime_error("Cannot read the prepared media file size.");
   if(!(info.dwFileAttributes&(FILE_ATTRIBUTE_DIRECTORY|FILE_ATTRIBUTE_REPARSE_POINT))&&length.QuadPart==size&&fileHash(input.h,[&]{cancel->check();})==digest){
    manager.waitForPrefetchConfirmation(job,*cancel,size,receipt);
    Handle current(CreateFileW(path.c_str(),FILE_READ_ATTRIBUTES,FILE_SHARE_READ|FILE_SHARE_DELETE,nullptr,OPEN_EXISTING,FILE_FLAG_OPEN_REPARSE_POINT,nullptr));
    BY_HANDLE_FILE_INFORMATION named{};
    if(!current||!GetFileInformationByHandle(current.h,&named)||named.dwVolumeSerialNumber!=info.dwVolumeSerialNumber||
       named.nFileIndexHigh!=info.nFileIndexHigh||named.nFileIndexLow!=info.nFileIndexLow)
     throw std::runtime_error("The prepared media file changed before publication. Resume to verify it again.");
    {Lock lock(manager.mutex);
     cancel->check();
     if(selectionBinding(*job)!=binding)throw std::runtime_error("The selected streams changed while verifying prepared media. Resume to use the new selection.");
     job->data[live?"AdaptiveAssemblyReused":"ReusedPrefetchedOutput"]=true;
     job->data["MergeSeconds"]=real(receipt,"MergeSeconds");
    }
    const LivePublicationDetails details{real(receipt,"MergeSeconds"),real(data,"ElapsedSeconds"),subtitleCues};
    auto publicationSource=PublicationSourcePin::openReadFrom(path,input.h);
    if(!publicationSource)throw std::runtime_error("The verified media source changed before publication. Its files were preserved.");
    publishMediaOutput(manager,job,path,digest,cancel,live?&details:nullptr,std::move(publicationSource));
    markZone(job->target());
    return true;
   }
  }
 }
 cancel->check();
 // DASH has its own sealed assembly cache and can recover it independently.
 if(kind!="LiveDash")discardRetainedMediaOutput(data);
 {Lock lock(manager.mutex);job->data.erase("RetainedMediaOutput");}
 return false;
}
}
