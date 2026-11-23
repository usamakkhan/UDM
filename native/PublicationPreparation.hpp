#pragma once
#include "Core.hpp"
#include "FileIdentity.hpp"
#include "PublicationSourcePin.hpp"
#include "GrabberDestinations.hpp"
namespace udm {
inline Json publicationDestinationInputs(const Json& data){Json result=Json::object();for(auto key:{"Folder","FileName","ProjectDestinationRoot"})if(data.contains(key))result[key]=data[key];return result;}
// Prepared on the publishing worker without the Manager lock. Prevent source writes until durable catalog acceptance; keep directory names pinned through cleanup.
// Directory writes stay shared so normal child-file publication can proceed.
class PublicationPreparation {
 Handle source;bool writableSource=false,readableSource=true;std::unique_ptr<Handle> publishedName;BY_HANDLE_FILE_INFORMATION identity{};Json capturedIdentity;std::vector<Handle> directories;std::vector<bool> belowRoots;
 void pin(const fs::path& path,bool belowRoot,bool resolve=false){
  Handle handle(CreateFileW(path.c_str(),GENERIC_READ,FILE_SHARE_READ|FILE_SHARE_WRITE,nullptr,OPEN_EXISTING,FILE_FLAG_BACKUP_SEMANTICS|(resolve?0:FILE_FLAG_OPEN_REPARSE_POINT),nullptr));BY_HANDLE_FILE_INFORMATION info{};
  if(!handle)throw std::runtime_error("Cannot keep the Grabber destination stable for publication. Wait for other folder operations or choose another project folder.");
  if(!GetFileInformationByHandle(handle.h,&info)||!(info.dwFileAttributes&FILE_ATTRIBUTE_DIRECTORY)||(belowRoot&&(info.dwFileAttributes&FILE_ATTRIBUTE_REPARSE_POINT)))throw std::runtime_error("A website subfolder is redirected or occupied by a file. Choose another project folder.");
  directories.push_back(std::move(handle));belowRoots.push_back(belowRoot);
 }
public:
 i64 size=0;
 PublicationPreparation(const fs::path& staging,const Json& destination,std::unique_ptr<PublicationSourcePin> ownedWriteback={}):source(ownedWriteback?std::move(ownedWriteback->file):Handle{}),writableSource(ownedWriteback&&ownedWriteback->writeAccess),readableSource(!ownedWriteback||ownedWriteback->readAccess){
  if(ownedWriteback){
   if(!source||(ownedWriteback->stable.is_null()&&!readableSource))throw std::runtime_error("The prepared download source lost its protection.");
   identity=ownedWriteback->information;capturedIdentity=ownedWriteback->stable;size=static_cast<i64>((static_cast<uint64_t>(identity.nFileSizeHigh)<<32)|identity.nFileSizeLow);
  }else{
   source.h=CreateFileW(staging.c_str(),GENERIC_READ,FILE_SHARE_READ|FILE_SHARE_DELETE,nullptr,OPEN_EXISTING,FILE_FLAG_OPEN_REPARSE_POINT,nullptr);BY_HANDLE_FILE_INFORMATION info{};LARGE_INTEGER length{};
   if(!source||!GetFileInformationByHandle(source.h,&info)||(info.dwFileAttributes&(FILE_ATTRIBUTE_DIRECTORY|FILE_ATTRIBUTE_REPARSE_POINT))||!GetFileSizeEx(source.h,&length)||length.QuadPart<0)throw std::runtime_error("Cannot inspect the completed download for publication.");size=length.QuadPart;identity=info;
  }
  validateGrabberFolder(destination);auto value=str(destination,"ProjectDestinationRoot");if(value.empty())return;
  auto root=fs::path(wide(value)).lexically_normal(),folder=fs::path(wide(str(destination,"Folder"))).lexically_normal();pin(root,false);pin(root,false,true);auto current=root;
  for(const auto& component:folder.lexically_relative(root)){if(component==L".")continue;current/=component;pin(current,true);}
 }
 // Reuse the already pinned read handle; never reopen the path for identity
 // or fallback hashing. validateSourceName still verifies the named file.
 Json sourceIdentity()const{return capturedIdentity.is_null()?stableFileIdentity(source.h):capturedIdentity;}
 std::string sourceHash(const std::function<void()>& checkpoint={})const{if(!readableSource)throw std::runtime_error("A write-only publication pin cannot supply an unknown digest.");return fileHash(source.h,checkpoint);}
 void validateSourceName(const fs::path& staging)const{
  Handle named(CreateFileW(staging.c_str(),GENERIC_READ,FILE_SHARE_READ|FILE_SHARE_WRITE|FILE_SHARE_DELETE,nullptr,OPEN_EXISTING,FILE_FLAG_OPEN_REPARSE_POINT,nullptr));BY_HANDLE_FILE_INFORMATION current{};LARGE_INTEGER length{};
  if(!named||!GetFileInformationByHandle(named.h,&current)||(current.dwFileAttributes&(FILE_ATTRIBUTE_DIRECTORY|FILE_ATTRIBUTE_REPARSE_POINT))||!GetFileSizeEx(named.h,&length)||length.QuadPart!=size||current.dwVolumeSerialNumber!=identity.dwVolumeSerialNumber||current.nFileIndexHigh!=identity.nFileIndexHigh||current.nFileIndexLow!=identity.nFileIndexLow)throw std::runtime_error("The completed file changed before publication. Its files were preserved.");
  if(!capturedIdentity.is_null()&&stableFileIdentity(named.h)!=capturedIdentity)throw std::runtime_error("The protected download identity changed. Keep its file and recovery record.");
 }
 void validateDirectories()const{for(size_t i=0;i<directories.size();++i){BY_HANDLE_FILE_INFORMATION info{};if(!GetFileInformationByHandle(directories[i].h,&info)||!(info.dwFileAttributes&FILE_ATTRIBUTE_DIRECTORY)||(belowRoots[i]&&(info.dwFileAttributes&FILE_ATTRIBUTE_REPARSE_POINT)))throw std::runtime_error("A website subfolder is redirected or occupied by a file. Choose another project folder.");}}
 // The adopted write object still excludes other writers. This read pin may
 // share that owned write access, while denying destination-name deletion.
 void pinPublishedName(const fs::path& path){
  auto pin=std::make_unique<Handle>(CreateFileW(path.c_str(),GENERIC_READ,FILE_SHARE_READ|(writableSource?FILE_SHARE_WRITE:0),nullptr,OPEN_EXISTING,FILE_FLAG_OPEN_REPARSE_POINT,nullptr));BY_HANDLE_FILE_INFORMATION current{};LARGE_INTEGER length{};
  if(!*pin||!GetFileInformationByHandle(pin->h,&current)||(current.dwFileAttributes&(FILE_ATTRIBUTE_DIRECTORY|FILE_ATTRIBUTE_REPARSE_POINT))||!GetFileSizeEx(pin->h,&length)||length.QuadPart!=size||current.dwVolumeSerialNumber!=identity.dwVolumeSerialNumber||current.nFileIndexHigh!=identity.nFileIndexHigh||current.nFileIndexLow!=identity.nFileIndexLow)throw std::runtime_error("The published destination changed before catalog acceptance. Keep its file and recovery journal.");
  publishedName=std::move(pin);
 }
 // Only the ordinary publication owner calls this after its exact completed
 // catalog proof is accepted. Journal cleanup no longer needs the file pins.
 void releaseAcceptedFile(){releaseSource();publishedName.reset();}
 void releaseSource(){if(source){CloseHandle(source.h);source.h=INVALID_HANDLE_VALUE;}}
};
}
