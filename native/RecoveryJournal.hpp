#pragma once
#include "Core.hpp"
namespace udm {
// A discovery snapshot denies byte writers but permits namespace replacement.
// Retirement requires a same-object exclusive claim, never a pathname guess.
class RecoveryJournal {
 enum class Access {Owned,Snapshot,Claim};
 Handle file;std::string contents;LONGLONG rawLength=0;size_t bound;bool retirementAllowed=false,claimAllowed=false;
 RecoveryJournal(const fs::path& path,size_t limit,Access access,const std::function<void(const char*)>& observation):file(CreateFileW(path.c_str(),GENERIC_READ|(access==Access::Snapshot?0:DELETE),FILE_SHARE_READ|(access==Access::Snapshot?FILE_SHARE_DELETE:0),nullptr,OPEN_EXISTING,FILE_FLAG_OPEN_REPARSE_POINT,nullptr)),bound(limit),retirementAllowed(access!=Access::Snapshot),claimAllowed(access==Access::Snapshot){
  if(!file){const auto failure=GetLastError();if(access==Access::Snapshot)throw std::system_error(static_cast<int>(failure),std::system_category(),"Cannot read the recovery journal snapshot. Its files were preserved.");throw std::runtime_error("Cannot own the recovery journal. Close programs holding it and retry; its files were preserved.");}
  BY_HANDLE_FILE_INFORMATION information{};LARGE_INTEGER size{};
  if(!path.is_absolute()||GetFileType(file.h)!=FILE_TYPE_DISK||!GetFileInformationByHandle(file.h,&information)||
     (information.dwFileAttributes&(FILE_ATTRIBUTE_DIRECTORY|FILE_ATTRIBUTE_REPARSE_POINT))||
     !GetFileSizeEx(file.h,&size)||size.QuadPart<0||static_cast<unsigned long long>(size.QuadPart)>limit)
   throw std::runtime_error("The recovery journal is not a supported regular file. Keep its files and recovery record.");
  rawLength=size.QuadPart;
  if(observation)observation(access==Access::Snapshot?"recovery-journal-snapshot-opened":access==Access::Claim?"recovery-journal-claim-opened":"recovery-journal-owned-opened");
  if(access==Access::Claim)return;
  contents.resize(static_cast<size_t>(rawLength));
  for(size_t offset=0;offset<contents.size();){
   if(observation)observation(access==Access::Snapshot?"recovery-journal-snapshot-read":"recovery-journal-owned-read");
   const auto amount=static_cast<DWORD>(std::min<size_t>(contents.size()-offset,1048576));DWORD count=0;
   if(!ReadFile(file.h,contents.data()+offset,amount,&count,nullptr)||!count||count>amount)
    throw std::runtime_error("Cannot read the held recovery journal. Its files were preserved.");
   offset+=count;
  }
  if(contents.rfind("\xEF\xBB\xBF",0)==0)contents.erase(0,3);
 }
 bool sameObject(const RecoveryJournal& other)const{
  BY_HANDLE_FILE_INFORMATION a{},b{};
  return file&&other.file&&GetFileInformationByHandle(file.h,&a)&&GetFileInformationByHandle(other.file.h,&b)&&
   (a.nFileIndexHigh||a.nFileIndexLow)&&a.dwVolumeSerialNumber==b.dwVolumeSerialNumber&&a.nFileIndexHigh==b.nFileIndexHigh&&a.nFileIndexLow==b.nFileIndexLow&&rawLength==other.rawLength;
 }
public:
 explicit RecoveryJournal(const fs::path& path,size_t limit=32*1024*1024,const std::function<void(const char*)>& observation={}):RecoveryJournal(path,limit,Access::Owned,observation){}
 static std::unique_ptr<RecoveryJournal> snapshot(const fs::path& path,size_t limit=32*1024*1024,const std::function<void(const char*)>& observation={}){return std::unique_ptr<RecoveryJournal>(new RecoveryJournal(path,limit,Access::Snapshot,observation));}
 static std::unique_ptr<RecoveryJournal> claim(const fs::path& path,RecoveryJournal& snapshot,const std::function<void(const char*)>& observation={}){
  if(!snapshot.claimAllowed||snapshot.retirementAllowed||!snapshot.file)throw std::runtime_error("The recovery journal snapshot cannot be claimed again.");
  auto owned=std::unique_ptr<RecoveryJournal>(new RecoveryJournal(path,snapshot.bound,Access::Claim,observation));
  if(!owned->sameObject(snapshot))throw std::runtime_error("The recovery journal changed during discovery. Its bytes and successor record were preserved.");
  // The live snapshot denies writes to this file object. Identity, raw length
  // and exclusive claim prove these exact held bytes without a second read.
  owned->contents=std::move(snapshot.contents);snapshot.claimAllowed=false;
  if(observation)observation("recovery-journal-claimed");return owned;
 }
 bool isOpen()const noexcept{return static_cast<bool>(file);}
 const std::string& text()const noexcept{return contents;}
 void retire(const std::function<void(const char*)>& observation={}){
  if(!file||!retirementAllowed)throw std::runtime_error("The recovery journal is not owned for retirement.");
  if(observation)observation("recovery-journal-retire");
  FILE_DISPOSITION_INFO_EX disposition{FILE_DISPOSITION_FLAG_DELETE|FILE_DISPOSITION_FLAG_POSIX_SEMANTICS};
  if(!SetFileInformationByHandle(file.h,FileDispositionInfoEx,&disposition,sizeof(disposition))){
   const auto error=GetLastError();
   if(error!=ERROR_INVALID_PARAMETER&&error!=ERROR_INVALID_FUNCTION&&error!=ERROR_NOT_SUPPORTED)
    throw std::runtime_error("Cannot retire the owned recovery journal (Windows error "+std::to_string(error)+"). Keep its file and recovery record.");
   FILE_DISPOSITION_INFO fallback{TRUE};
   if(!SetFileInformationByHandle(file.h,FileDispositionInfo,&fallback,sizeof(fallback)))
    throw std::runtime_error("Cannot retire the owned recovery journal. Keep its file and recovery record.");
  }
  if(!CloseHandle(file.h))throw std::runtime_error("Cannot close the retired recovery journal handle.");file.h=INVALID_HANDLE_VALUE;
  if(observation)observation("recovery-journal-retired");
 }
};
}
