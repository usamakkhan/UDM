#pragma once
#include "FileIdentity.hpp"
#include "CreatedStagingFile.hpp"
#include <limits>
namespace udm {
// Metadata is captured on this same writer-excluding object before its flush
// starts. The capsule cannot be constructed with a caller-supplied snapshot.
class PublicationSourcePin {
 friend class PublicationPreparation;
 Handle file;BY_HANDLE_FILE_INFORMATION information{};Json stable;bool readAccess=false,writeAccess=true;
 PublicationSourcePin()=default;
public:
 PublicationSourcePin(const PublicationSourcePin&)=delete;
 const Json& capturedIdentity()const noexcept{return stable;}
 static std::unique_ptr<PublicationSourcePin> open(const fs::path& path){return openMode(path,false,true);}
 static std::unique_ptr<PublicationSourcePin> openRead(const fs::path& path){return openMode(path,true,false);}
 static std::unique_ptr<PublicationSourcePin> openReadWrite(const fs::path& path){return openMode(path,true,true);}
 // Acquire the publication reader while the verified reader still excludes
 // writes. Identity comes from that actual object, never a caller snapshot.
 static std::unique_ptr<PublicationSourcePin> openReadFrom(const fs::path& path,HANDLE verified){
  BY_HANDLE_FILE_INFORMATION measured{};
  if(!verified||verified==INVALID_HANDLE_VALUE||!GetFileInformationByHandle(verified,&measured)||
     (measured.dwFileAttributes&(FILE_ATTRIBUTE_DIRECTORY|FILE_ATTRIBUTE_REPARSE_POINT)))return {};
  auto pin=openRead(path);if(!pin)return {};
  const auto& current=pin->information;
  if(measured.dwVolumeSerialNumber!=current.dwVolumeSerialNumber||measured.nFileIndexHigh!=current.nFileIndexHigh||measured.nFileIndexLow!=current.nFileIndexLow||
     measured.nFileSizeHigh!=current.nFileSizeHigh||measured.nFileSizeLow!=current.nFileSizeLow||CompareFileTime(&measured.ftLastWriteTime,&current.ftLastWriteTime)!=0)return {};
  return pin;
 }
 void captureCleanup(CreatedStagingFile& proof)const noexcept{proof.capture(file.h);}
 bool matches(const CreatedStagingFile& proof,HANDLE reference=INVALID_HANDLE_VALUE)const noexcept{return proof.matches(file.h,reference);}
 std::string hash(const std::function<void()>& checkpoint={})const{if(!readAccess)throw std::runtime_error("The publication source is not readable.");return fileHash(file.h,checkpoint);}
private:
 static std::unique_ptr<PublicationSourcePin> openMode(const fs::path& path,bool read,bool write){
  auto pin=std::unique_ptr<PublicationSourcePin>(new PublicationSourcePin);
  pin->readAccess=read;pin->writeAccess=write;
  pin->file.h=CreateFileW(path.c_str(),(read?GENERIC_READ:0)|(write?GENERIC_WRITE:0)|FILE_READ_ATTRIBUTES,FILE_SHARE_READ|FILE_SHARE_DELETE,nullptr,OPEN_EXISTING,FILE_FLAG_OPEN_REPARSE_POINT,nullptr);
  if(!pin->file||!GetFileInformationByHandle(pin->file.h,&pin->information)||(pin->information.dwFileAttributes&(FILE_ATTRIBUTE_DIRECTORY|FILE_ATTRIBUTE_REPARSE_POINT)))return {};
  const auto size=(static_cast<uint64_t>(pin->information.nFileSizeHigh)<<32)|pin->information.nFileSizeLow;
  if(size>static_cast<uint64_t>(std::numeric_limits<i64>::max()))return {};
  pin->stable=stableFileIdentity(pin->file.h);
  if((pin->stable.is_null()&&!read)||(!pin->stable.is_null()&&num(pin->stable,"Size",-1)!=static_cast<i64>(size)))return {};
  return pin;
 }
public:
 std::unique_ptr<PublicationSourcePin> clone()const{
  auto copy=std::unique_ptr<PublicationSourcePin>(new PublicationSourcePin);HANDLE handle=INVALID_HANDLE_VALUE;
  if(!DuplicateHandle(GetCurrentProcess(),file.h,GetCurrentProcess(),&handle,0,FALSE,DUPLICATE_SAME_ACCESS))return {};
  copy->file.h=handle;copy->information=information;copy->stable=stable;copy->readAccess=readAccess;copy->writeAccess=writeAccess;return copy;
 }
 Handle duplicate()const{
  HANDLE value=INVALID_HANDLE_VALUE;if(!DuplicateHandle(GetCurrentProcess(),file.h,GetCurrentProcess(),&value,0,FALSE,DUPLICATE_SAME_ACCESS))return Handle{};return Handle(value);
 }
};
}
