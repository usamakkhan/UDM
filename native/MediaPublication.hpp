#pragma once
#include "Core.hpp"

namespace udm {
// Keep the verified source until publication commits. A sibling on the selected
// volume lets the existing publication journal retain its atomic rename boundary.
inline void publishMediaOutput(Manager& manager,JobPtr job,const fs::path& source,
                               const std::string& digest,const std::shared_ptr<Cancel>& cancel){
 cancel->check();
 const auto folder=job->target().parent_path();
 if(folder.lexically_normal()==source.parent_path().lexically_normal()){
  manager.publishFile(job,source,digest);return;
 }
 fs::create_directories(folder);
 const auto staging=folder/(L".udm-"+wide(job->id())+L"-"+wide(guid())+L".publishing");
 try{
  // A hard link preserves file identity and avoids copying on the same volume.
  if(!CreateHardLinkW(staging.c_str(),source.c_str(),nullptr)){
   Handle input(CreateFileW(source.c_str(),GENERIC_READ,FILE_SHARE_READ,nullptr,OPEN_EXISTING,FILE_FLAG_SEQUENTIAL_SCAN,nullptr));
   if(!input)throw std::runtime_error("Cannot read the verified media file.");
   Handle output(CreateFileW(staging.c_str(),GENERIC_WRITE,0,nullptr,CREATE_NEW,FILE_FLAG_SEQUENTIAL_SCAN,nullptr));
   if(!output)throw std::runtime_error("Cannot prepare media in the selected folder.");
   char bytes[65536];DWORD count=0;
   for(;;){
    cancel->check();
    if(!ReadFile(input.h,bytes,sizeof(bytes),&count,nullptr))throw std::runtime_error("Cannot read the verified media file.");
    if(!count)break;
    DWORD written=0;if(!WriteFile(output.h,bytes,count,&written,nullptr)||written!=count)throw std::runtime_error("Cannot copy media to the selected folder.");
   }
   if(!FlushFileBuffers(output.h))throw std::runtime_error("Cannot flush media in the selected folder.");
   CloseHandle(output.h);output.h=INVALID_HANDLE_VALUE;
   if(fileHash(staging,[&]{cancel->check();})!=digest)throw std::runtime_error("The copied media failed verification. Source tracks were retained.");
  }
  cancel->check();manager.publishFile(job,staging,digest);
 }catch(...){
  std::error_code ec;
  const bool publication=fs::exists(manager.root/L"publications"/(wide(job->id())+L".json"),ec);
  if(!ec&&!publication){
   const bool replacement=fs::exists(manager.root/L"replacements"/(wide(job->id())+L".json"),ec);
   if(!ec&&!replacement)fs::remove(staging,ec);
  }
  throw;
 }
 std::error_code ec;fs::remove(source,ec);
}
}
