#pragma once
#include "Core.hpp"
#include <algorithm>
namespace udm {
// Captured on an owning handle: a new output or a pinned verified source.
// Length and last-write time may legitimately change while assembling a file.
class CreatedStagingFile {
 struct Identity {FILE_ID_INFO file{};LONGLONG created=0;bool wide=true;};
 Identity identity{};bool valid=false;
 static bool query(HANDLE file,Identity& value)noexcept{
  FILE_BASIC_INFO basic{};
  if(GetFileInformationByHandleEx(file,FileIdInfo,&value.file,sizeof(value.file))&&GetFileInformationByHandleEx(file,FileBasicInfo,&basic,sizeof(basic))){
   if((basic.FileAttributes&(FILE_ATTRIBUTE_DIRECTORY|FILE_ATTRIBUTE_REPARSE_POINT))||std::none_of(std::begin(value.file.FileId.Identifier),std::end(value.file.FileId.Identifier),[](BYTE b){return b!=0;}))return false;
   value.created=basic.CreationTime.QuadPart;return true;
  }
  BY_HANDLE_FILE_INFORMATION legacy{};
  if(!GetFileInformationByHandle(file,&legacy)||(legacy.dwFileAttributes&(FILE_ATTRIBUTE_DIRECTORY|FILE_ATTRIBUTE_REPARSE_POINT))||(!legacy.nFileIndexHigh&&!legacy.nFileIndexLow))return false;
  value={};value.wide=false;value.file.VolumeSerialNumber=legacy.dwVolumeSerialNumber;
  const auto index=(static_cast<uint64_t>(legacy.nFileIndexHigh)<<32)|legacy.nFileIndexLow;
  for(unsigned i=0;i<8;++i)value.file.FileId.Identifier[i]=static_cast<BYTE>(index>>(8*i));
  value.created=static_cast<LONGLONG>((static_cast<uint64_t>(legacy.ftCreationTime.dwHighDateTime)<<32)|legacy.ftCreationTime.dwLowDateTime);return true;
 }
public:
 void capture(HANDLE created)noexcept{identity={};valid=query(created,identity);}
 bool matches(HANDLE file,HANDLE reference=INVALID_HANDLE_VALUE)const noexcept{
  const auto same=[&](const Identity& value){return value.wide==identity.wide&&value.file.VolumeSerialNumber==identity.file.VolumeSerialNumber&&value.created==identity.created&&std::equal(std::begin(value.file.FileId.Identifier),std::end(value.file.FileId.Identifier),std::begin(identity.file.FileId.Identifier));};
  Identity current{};if(!valid||!query(file,current)||!same(current))return false;
  const bool held=reference&&reference!=INVALID_HANDLE_VALUE;
  if(!identity.wide&&!held)return false;
  if(held){Identity original{};if(!query(reference,original)||!same(original))return false;}
  return true;
 }
 // A supplied reference must remain live and exclude other writers. It keeps
 // the file ID allocated while the named object is checked and removed.
 void removeOwned(const fs::path& path,HANDLE reference=INVALID_HANDLE_VALUE)const noexcept{
  if(!valid)return;
  const bool held=reference&&reference!=INVALID_HANDLE_VALUE;if(!identity.wide&&!held)return;
  Handle file(CreateFileW(path.c_str(),DELETE|FILE_READ_ATTRIBUTES,FILE_SHARE_READ|(held?FILE_SHARE_WRITE:0),nullptr,OPEN_EXISTING,FILE_FLAG_OPEN_REPARSE_POINT,nullptr));
  if(!file||!matches(file.h,reference))return;
  FILE_DISPOSITION_INFO disposition{TRUE};SetFileInformationByHandle(file.h,FileDispositionInfo,&disposition,sizeof(disposition));
 }
};
}
