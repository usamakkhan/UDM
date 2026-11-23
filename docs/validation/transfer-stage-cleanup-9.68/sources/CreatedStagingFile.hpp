#pragma once
#include "Core.hpp"
#include <algorithm>
namespace udm {
// Captured on the exclusive CREATE_NEW handle, before the first byte is written.
// Length and last-write time may legitimately change while assembling the file.
class CreatedStagingFile {
 struct Identity {FILE_ID_INFO file{};LONGLONG created=0;};
 Identity identity{};bool valid=false;
 static bool query(HANDLE file,Identity& value)noexcept{
  FILE_BASIC_INFO basic{};
  if(!GetFileInformationByHandleEx(file,FileIdInfo,&value.file,sizeof(value.file))||!GetFileInformationByHandleEx(file,FileBasicInfo,&basic,sizeof(basic))||(basic.FileAttributes&(FILE_ATTRIBUTE_DIRECTORY|FILE_ATTRIBUTE_REPARSE_POINT))||std::none_of(std::begin(value.file.FileId.Identifier),std::end(value.file.FileId.Identifier),[](BYTE b){return b!=0;}))return false;
  value.created=basic.CreationTime.QuadPart;return true;
 }
public:
 void capture(HANDLE created)noexcept{valid=query(created,identity);}
 void removeOwned(const fs::path& path)const noexcept{
  if(!valid)return;
  Handle file(CreateFileW(path.c_str(),DELETE|FILE_READ_ATTRIBUTES,FILE_SHARE_READ,nullptr,OPEN_EXISTING,FILE_FLAG_OPEN_REPARSE_POINT,nullptr));Identity current{};
  if(!file||!query(file.h,current)||current.file.VolumeSerialNumber!=identity.file.VolumeSerialNumber||current.created!=identity.created||!std::equal(std::begin(current.file.FileId.Identifier),std::end(current.file.FileId.Identifier),std::begin(identity.file.FileId.Identifier)))return;
  FILE_DISPOSITION_INFO disposition{TRUE};SetFileInformationByHandle(file.h,FileDispositionInfo,&disposition,sizeof(disposition));
 }
};
}
