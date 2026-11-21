#pragma once
#include "Core.hpp"
namespace udm {
inline bool experimentWriteThroughIntent(const fs::path& path){static const bool enabled=[]{wchar_t value[8]{};return GetEnvironmentVariableW(L"UDM_TEST_WRITE_THROUGH_INTENT",value,8)==1&&value[0]==L'1';}();return enabled&&path.parent_path().filename()==L"publications"&&path.extension()==L".json";}
inline void experimentAtomicIntent(const fs::path& p,const std::string& bytes,bool backup,const std::function<void(const char*)>& checkpoint){
 fs::create_directories(p.parent_path());auto temp=p;temp+=L".tmp";if(checkpoint)checkpoint("directories-ready");
 {Handle file(CreateFileW(temp.c_str(),GENERIC_WRITE|FILE_READ_ATTRIBUTES,0,nullptr,CREATE_ALWAYS,FILE_ATTRIBUTE_NORMAL|FILE_FLAG_NO_BUFFERING|FILE_FLAG_WRITE_THROUGH,nullptr));if(!file)throw std::runtime_error("Private write-through intent open failed.");if(checkpoint)checkpoint("temporary-opened");
  FILE_STORAGE_INFO storage{};FILE_ALIGNMENT_INFO alignment{};
  if(!GetFileInformationByHandleEx(file.h,FileStorageInfo,&storage,sizeof(storage))||!GetFileInformationByHandleEx(file.h,FileAlignmentInfo,&alignment,sizeof(alignment)))throw std::runtime_error("Private intent alignment query failed.");
  const size_t align=std::max<size_t>({storage.LogicalBytesPerSector,storage.PhysicalBytesPerSectorForAtomicity,storage.PhysicalBytesPerSectorForPerformance,storage.FileSystemEffectivePhysicalBytesPerSectorForAtomicity,static_cast<size_t>(alignment.AlignmentRequirement)+1});
  if(!align||(align&(align-1))||align>1048576||bytes.size()>32*1048576-align)throw std::runtime_error("Private intent alignment or size unsupported.");const auto count=(bytes.size()+align-1)&~(align-1);
  struct Allocation{void* base=nullptr;~Allocation(){if(base)VirtualFree(base,0,MEM_RELEASE);}} allocation;allocation.base=VirtualAlloc(nullptr,count+align,MEM_COMMIT|MEM_RESERVE,PAGE_READWRITE);if(!allocation.base)throw std::runtime_error("Private intent allocation failed.");auto pointer=(reinterpret_cast<uintptr_t>(allocation.base)+align-1)&~(static_cast<uintptr_t>(align)-1);auto buffer=reinterpret_cast<char*>(pointer);memset(buffer,' ',count);memcpy(buffer,bytes.data(),bytes.size());DWORD written=0;
  if(!WriteFile(file.h,buffer,static_cast<DWORD>(count),&written,nullptr)||written!=count)throw std::runtime_error("Private intent complete write failed.");if(checkpoint)checkpoint("bytes-written");if(!CloseHandle(file.h))throw std::runtime_error("Private intent close failed.");file.h=INVALID_HANDLE_VALUE;
 }
 if(checkpoint)checkpoint("temporary-flushed");if(fs::exists(p)){auto bak=p;bak+=L".bak";for(unsigned attempt=0;;++attempt){if(ReplaceFileW(p.c_str(),temp.c_str(),backup?bak.c_str():nullptr,0,nullptr,nullptr))break;const auto code=GetLastError();if(attempt>=6||(code!=ERROR_SHARING_VIOLATION&&code!=ERROR_LOCK_VIOLATION&&code!=ERROR_UNABLE_TO_REMOVE_REPLACED))throw std::runtime_error("Private intent replace failed.");Sleep(25*(attempt+1));}}else if(!MoveFileExW(temp.c_str(),p.c_str(),MOVEFILE_WRITE_THROUGH))throw std::runtime_error("Private intent publish failed.");
}
}
