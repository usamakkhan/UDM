#pragma once
#include "PublicationRecovery.hpp"

namespace udm {
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
// confirmation. Release it before passing the path to Windows publication.
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
 void releaseForPublication(const fs::path& path){
  Handle current(CreateFileW(path.c_str(),FILE_READ_ATTRIBUTES,FILE_SHARE_READ|FILE_SHARE_DELETE,nullptr,
                            OPEN_EXISTING,FILE_FLAG_OPEN_REPARSE_POINT,nullptr));
  BY_HANDLE_FILE_INFORMATION named{};
  if(!current||!GetFileInformationByHandle(current.h,&named)||
     named.dwVolumeSerialNumber!=identity.dwVolumeSerialNumber||
     named.nFileIndexHigh!=identity.nFileIndexHigh||named.nFileIndexLow!=identity.nFileIndexLow)
   throw std::runtime_error("The prepared media file changed before publication. Resume to verify it again.");
  CloseHandle(file.h);
  file.h=INVALID_HANDLE_VALUE;
 }
};
// Keep the verified source until publication commits. A sibling on the selected
// volume lets the existing publication journal retain its atomic rename boundary.
inline void publishMediaOutput(Manager& manager,JobPtr job,const fs::path& source,
                               const std::string& digest,const std::shared_ptr<Cancel>& cancel,const LivePublicationDetails* live=nullptr){
 cancel->check();requirePublicationRecovered(manager,job);
 const auto folder=job->target().parent_path();
 if(folder.lexically_normal()==source.parent_path().lexically_normal()){
  manager.publishFile(job,source,digest,cancel.get(),live);return;
 }
 fs::create_directories(folder);
 const auto staging=folder/(L".udm-"+wide(job->id())+L"-"+wide(guid())+L".publishing");
 bool ownsStaging=false;
 try{
  // A hard link preserves file identity and avoids copying on the same volume.
  ownsStaging=CreateHardLinkW(staging.c_str(),source.c_str(),nullptr)!=FALSE;
  if(!ownsStaging){
   Handle input(CreateFileW(source.c_str(),GENERIC_READ,FILE_SHARE_READ,nullptr,OPEN_EXISTING,FILE_FLAG_SEQUENTIAL_SCAN,nullptr));
   if(!input)throw std::runtime_error("Cannot read the verified media file.");
   Handle output(CreateFileW(staging.c_str(),GENERIC_READ|GENERIC_WRITE,0,nullptr,CREATE_NEW,FILE_FLAG_SEQUENTIAL_SCAN,nullptr));
   if(!output)throw std::runtime_error("Cannot prepare media in the selected folder.");
   ownsStaging=true;
   std::vector<char> bytes(1024*1024);DWORD count=0;
   for(;;){
    cancel->check();
    if(!ReadFile(input.h,bytes.data(),(DWORD)bytes.size(),&count,nullptr))throw std::runtime_error("Cannot read the verified media file.");
    if(!count)break;
    DWORD written=0;if(!WriteFile(output.h,bytes.data(),count,&written,nullptr)||written!=count)throw std::runtime_error("Cannot copy media to the selected folder.");
   }
   cancel->check();
   if(!FlushFileBuffers(output.h))throw std::runtime_error("Cannot flush media in the selected folder.");
   if(fileHash(output.h,[&]{cancel->check();})!=digest)throw std::runtime_error("The copied media failed verification. Source tracks were retained.");
  }
  cancel->check();manager.publishFile(job,staging,digest,cancel.get(),live);
 }catch(...){
  std::error_code ec;
  if(ownsStaging&&!publicationNeedsRecovery(manager,job))fs::remove(staging,ec);
  throw;
 }
 std::error_code ec;fs::remove(source,ec);
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
    publishMediaOutput(manager,job,path,digest,cancel,live?&details:nullptr);
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
