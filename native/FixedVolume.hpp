#pragma once
#include "Core.hpp"
namespace udm {
struct FixedVolumeOps {
 HANDLE open(const wchar_t* path){return CreateFileW(path,FILE_READ_ATTRIBUTES,FILE_SHARE_READ|FILE_SHARE_WRITE|FILE_SHARE_DELETE,nullptr,OPEN_EXISTING,FILE_FLAG_BACKUP_SEMANTICS,nullptr);}
 DWORD finalPath(HANDLE file,wchar_t* text,DWORD capacity){return GetFinalPathNameByHandleW(file,text,capacity,FILE_NAME_OPENED|VOLUME_NAME_GUID);}
 UINT driveType(const wchar_t* root){return GetDriveTypeW(root);}
 bool volumePath(const wchar_t* path,wchar_t* root,DWORD capacity){return GetVolumePathNameW(path,root,capacity)!=FALSE;}
 void close(HANDLE file){CloseHandle(file);}
};
// Only accept the complete documented volume-GUID root. Never infer a local
// disk from an arbitrary final name or a truncated/unsupported query result.
inline bool fixedVolumeGuidRoot(const wchar_t* text,DWORD length){
 if(length<49||_wcsnicmp(text,L"\\\\?\\Volume{",11)!=0||text[47]!=L'}'||text[48]!=L'\\')return false;
 for(size_t i=0;i<36;++i){const auto c=text[11+i];
  if(i==8||i==13||i==18||i==23){if(c!=L'-')return false;}
  else if(!((c>=L'0'&&c<=L'9')||(c>=L'a'&&c<=L'f')||(c>=L'A'&&c<=L'F')))return false;
 }return true;
}
// A share cannot supply a volume GUID. Preserve its old lookup without first
// opening the remote file, including drive-letter mappings and extended UNC.
template<class Ops> bool fixedVolumeTryOpenedPath(const std::wstring& full,Ops& ops){
 const auto slash=[](wchar_t c){return c==L'\\'||c==L'/';};
 const bool doubled=full.size()>=2&&slash(full[0])&&slash(full[1]);
 const bool extended=full.size()>=4&&doubled&&full[2]==L'?'&&slash(full[3]);
 const size_t offset=extended?4:0;
 const bool drive=full.size()>=offset+3&&((full[offset]>=L'A'&&full[offset]<=L'Z')||(full[offset]>=L'a'&&full[offset]<=L'z'))&&full[offset+1]==L':'&&slash(full[offset+2]);
 const bool guid=extended&&full.size()>=11&&_wcsnicmp(full.c_str()+4,L"Volume{",7)==0;
 if(doubled&&!drive&&!guid)return false;
 if(drive){wchar_t root[]={full[offset],L':',L'\\',0};if(ops.driveType(root)==DRIVE_REMOTE)return false;}
 return true;
}
template<class Ops> bool fileOnFixedVolume(const fs::path& path,Ops& ops){
 const auto full=fs::absolute(path).wstring();
 if(fixedVolumeTryOpenedPath(full,ops)){
  const auto file=ops.open(full.c_str());
  if(file&&file!=INVALID_HANDLE_VALUE){
   struct Owned {Ops& ops;HANDLE file;~Owned(){ops.close(file);}} owned{ops,file};
   wchar_t finalName[32768]{};const auto length=ops.finalPath(file,finalName,32768);
   if(length<32768&&fixedVolumeGuidRoot(finalName,length)){
    finalName[49]=0;const auto type=ops.driveType(finalName);
    if(type>=DRIVE_REMOVABLE&&type<=DRIVE_RAMDISK)return type==DRIVE_FIXED;
   }
  }
 }
 // Before creation, or for network/vendor/unsupported handles, retain the
 // original path-based decision. No drive-letter or per-job result is cached.
 wchar_t root[MAX_PATH]{};return ops.volumePath(full.c_str(),root,MAX_PATH)&&ops.driveType(root)==DRIVE_FIXED;
}
inline bool fileOnFixedVolume(const fs::path& path){FixedVolumeOps ops;return fileOnFixedVolume(path,ops);}
}
