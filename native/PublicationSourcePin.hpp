#pragma once
#include "FileIdentity.hpp"
#include <limits>
namespace udm {
// Metadata is captured on this same writer-excluding object before its flush
// starts. The capsule cannot be constructed with a caller-supplied snapshot.
class PublicationSourcePin {
 friend class PublicationPreparation;
 Handle file;BY_HANDLE_FILE_INFORMATION information{};Json stable;
 PublicationSourcePin()=default;
public:
 PublicationSourcePin(const PublicationSourcePin&)=delete;
 const Json& capturedIdentity()const noexcept{return stable;}
 static std::unique_ptr<PublicationSourcePin> open(const fs::path& path){
  auto pin=std::unique_ptr<PublicationSourcePin>(new PublicationSourcePin);
  pin->file.h=CreateFileW(path.c_str(),GENERIC_WRITE|FILE_READ_ATTRIBUTES,FILE_SHARE_READ|FILE_SHARE_DELETE,nullptr,OPEN_EXISTING,FILE_FLAG_OPEN_REPARSE_POINT,nullptr);
  if(!pin->file||!GetFileInformationByHandle(pin->file.h,&pin->information)||(pin->information.dwFileAttributes&(FILE_ATTRIBUTE_DIRECTORY|FILE_ATTRIBUTE_REPARSE_POINT)))return {};
  const auto size=(static_cast<uint64_t>(pin->information.nFileSizeHigh)<<32)|pin->information.nFileSizeLow;
  if(size>static_cast<uint64_t>(std::numeric_limits<i64>::max()))return {};
  pin->stable=stableFileIdentity(pin->file.h);
  if(pin->stable.is_null()||num(pin->stable,"Size",-1)!=static_cast<i64>(size))return {};
  return pin;
 }
 Handle duplicate()const{
  HANDLE value=INVALID_HANDLE_VALUE;if(!DuplicateHandle(GetCurrentProcess(),file.h,GetCurrentProcess(),&value,0,FALSE,DUPLICATE_SAME_ACCESS))return Handle{};return Handle(value);
 }
};
}
