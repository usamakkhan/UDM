#pragma once
#include "Backup.hpp"
namespace udm {
inline std::string recoveryDirectoryIdentity(HANDLE handle){
 BY_HANDLE_FILE_INFORMATION info{};
 if(!GetFileInformationByHandle(handle,&info)||(info.dwFileAttributes&FILE_ATTRIBUTE_REPARSE_POINT)||!(info.dwFileAttributes&FILE_ATTRIBUTE_DIRECTORY))
  throw std::runtime_error("Cannot identify recovery directory.");
 return std::to_string(info.dwVolumeSerialNumber)+":"+std::to_string(info.nFileIndexHigh)+":"+std::to_string(info.nFileIndexLow);
}
inline std::string recoveryDirectoryIdentity(const fs::path& path){
 Handle handle(CreateFileW(path.c_str(),FILE_READ_ATTRIBUTES,FILE_SHARE_READ|FILE_SHARE_WRITE|FILE_SHARE_DELETE,nullptr,OPEN_EXISTING,FILE_FLAG_BACKUP_SEMANTICS|FILE_FLAG_OPEN_REPARSE_POINT,nullptr));
 if(!handle)throw std::runtime_error("Cannot open recovery directory.");
 return recoveryDirectoryIdentity(handle.h);
}
inline std::set<std::string> recoveryParentPaths(const std::set<std::string>& approved){
 std::set<std::string> result;
 for(const auto& target:approved){
  auto directory=target.rfind("directory:",0)==0;
  auto path=fs::path(wide(directory?target.substr(10):target));
  if(!directory)path=path.parent_path();
  for(;!path.empty()&&path!=path.root_path();path=path.parent_path())result.insert(lower(utf8(path.wstring())));
 }
 return result;
}
// Deletes through the checked handle, never recursively and never by a newly substituted path.
inline bool removeRecoveryDirectory(const fs::path& path,const std::string& identity){
 Handle handle(CreateFileW(path.c_str(),FILE_READ_ATTRIBUTES|DELETE,FILE_SHARE_READ|FILE_SHARE_WRITE|FILE_SHARE_DELETE,nullptr,OPEN_EXISTING,FILE_FLAG_BACKUP_SEMANTICS|FILE_FLAG_OPEN_REPARSE_POINT,nullptr));
 if(!handle||recoveryDirectoryIdentity(handle.h)!=identity)return false;
 FILE_DISPOSITION_INFO disposition{TRUE};
 return SetFileInformationByHandle(handle.h,FileDispositionInfo,&disposition,sizeof(disposition))!=FALSE;
}
}
