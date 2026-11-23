#include "FixedVolume.hpp"
#include <fstream>
#include <iostream>
using namespace udm;
static Json checks=Json::array();
static void check(bool passed,const char* name){checks.push_back({{"name",name},{"passed",passed}});if(!passed)throw std::runtime_error(name);}
struct FakeOps {
 std::wstring finalName=L"\\\\?\\Volume{01234567-89ab-cdef-0123-456789abcdef}\\file.bin";
 DWORD reported=static_cast<DWORD>(finalName.size());UINT actualType=DRIVE_FIXED,legacyType=DRIVE_FIXED,routeType=DRIVE_FIXED;
 bool canOpen=true,canResolve=true;int opened=0,closed=0,legacy=0,queries=0;
 HANDLE open(const wchar_t*){++opened;return canOpen?reinterpret_cast<HANDLE>(1):INVALID_HANDLE_VALUE;}
 DWORD finalPath(HANDLE,wchar_t* text,DWORD capacity){++queries;std::copy_n(finalName.c_str(),std::min<size_t>(finalName.size()+1,capacity),text);return reported;}
 UINT driveType(const wchar_t* text){const std::wstring value(text);return value==L"X:\\"?legacyType:value.size()==3&&value[1]==L':'?routeType:actualType;}
 bool volumePath(const wchar_t*,wchar_t* text,DWORD){++legacy;wcscpy_s(text,MAX_PATH,L"X:\\");return canResolve;}
 void close(HANDLE){++closed;}
};
int wmain(int argc,wchar_t** argv){
 fs::path output;if(argc==2){output=fs::absolute(argv[1]);fs::create_directories(output);}
 try{
  const auto path=output/L"probe.bin";
  {FakeOps ops;check(fileOnFixedVolume(path,ops)&&ops.legacy==0&&ops.closed==1,"Fixed file volume avoids path traversal and closes its attribute handle");}
  for(UINT type:{DRIVE_REMOVABLE,DRIVE_REMOTE,DRIVE_CDROM,DRIVE_RAMDISK}){FakeOps ops;ops.actualType=type;check(!fileOnFixedVolume(path,ops)&&ops.legacy==0&&ops.closed==1,"A known non-fixed opened volume is never promoted through a fixed alias");}
  for(UINT type:{DRIVE_UNKNOWN,DRIVE_NO_ROOT_DIR}){FakeOps ops;ops.actualType=type;check(fileOnFixedVolume(path,ops)&&ops.legacy==1&&ops.closed==1,"An unrecognized GUID route retains legacy path resolution");}
  {FakeOps ops;ops.canOpen=false;check(fileOnFixedVolume(path,ops)&&ops.queries==0&&ops.closed==0&&ops.legacy==1,"A not-yet-created file keeps the original path-based policy");}
  for(DWORD length:{0UL,32768UL,32769UL}){FakeOps ops;ops.reported=length;check(fileOnFixedVolume(path,ops)&&ops.legacy==1&&ops.closed==1,"Failed and oversized final-path queries fall back without trusting partial names");}
  for(const auto* invalid:{L"\\\\server\\share\\file",L"\\\\?\\Volume{01234567-89ab-cdef-0123-456789abcdeg}\\file",L"\\\\?\\Volume{01234567_89ab-cdef-0123-456789abcdef}\\file",L"\\\\?\\Volume{01234567-89ab-cdef-0123-456789abcdef}file"}){
   FakeOps ops;ops.finalName=invalid;ops.reported=static_cast<DWORD>(ops.finalName.size());check(fileOnFixedVolume(path,ops)&&ops.legacy==1&&ops.closed==1,"Malformed and non-GUID names cannot select the fast route");
  }
  {FakeOps ops;ops.reported=0;ops.legacyType=DRIVE_REMOTE;check(!fileOnFixedVolume(path,ops)&&ops.closed==1,"Unsupported network GUID queries preserve remote classification");}
  {FakeOps ops;ops.canOpen=false;ops.canResolve=false;check(!fileOnFixedVolume(path,ops)&&ops.legacy==1,"Unresolvable paths remain conservative");}
  for(const auto* remote:{L"\\\\udm-fixture.invalid\\share\\file.bin",L"\\\\?\\UNC\\udm-fixture.invalid\\share\\file.bin",L"//udm-fixture.invalid/share/file.bin"}){
   FakeOps ops;ops.legacyType=DRIVE_REMOTE;
   check(!fileOnFixedVolume(fs::path(remote),ops)&&ops.opened==0&&ops.queries==0&&ops.closed==0&&ops.legacy==1,"UNC routes perform only the legacy lookup without opening the remote file");
  }
  for(const auto* mapped:{L"Z:\\file.bin",L"\\\\?\\Z:\\file.bin"}){
   FakeOps ops;ops.routeType=DRIVE_REMOTE;ops.legacyType=DRIVE_REMOTE;
   check(!fileOnFixedVolume(fs::path(mapped),ops)&&ops.opened==0&&ops.queries==0&&ops.legacy==1,"Mapped remote drives avoid an attribute open before legacy classification");
  }
  {FakeOps ops;ops.routeType=DRIVE_REMOTE;check(fileOnFixedVolume(fs::path(L"Z:\\redirected.bin"),ops)&&ops.opened==0&&ops.legacy==1,"The mapping guard preserves the actual legacy decision rather than guessing from the drive letter");}
  {FakeOps ops;check(fileOnFixedVolume(fs::path(L"\\\\?\\D:\\file.bin"),ops)&&ops.opened==1&&ops.queries==1&&ops.legacy==0&&ops.closed==1,"Extended local drive paths retain the guarded opened-file route");}
  {FakeOps ops;check(fileOnFixedVolume(fs::path(L"\\\\?\\Volume{01234567-89ab-cdef-0123-456789abcdef}\\file.bin"),ops)&&ops.opened==1&&ops.legacy==0&&ops.closed==1,"Volume-GUID local paths are not confused with UNC shares");}
  if(!output.empty()){
   std::ofstream file(path,std::ios::binary);file<<"owned volume classification probe";file.close();
   wchar_t root[MAX_PATH]{};const bool legacy=GetVolumePathNameW(path.c_str(),root,MAX_PATH)&&GetDriveTypeW(root)==DRIVE_FIXED;
   check(fileOnFixedVolume(path)==legacy,"The actual owned file agrees with legacy classification on this volume");
   check(fileOnFixedVolume(output)==legacy,"Directory handles resolve the actual owned volume");
   fs::remove(path);
  }
  if(!output.empty())atomicText(output/L"results.json",Json{{"passed",true},{"checks",checks}}.dump(2),false);
  std::cout<<checks.size()<<" checks passed\n";return 0;
 }catch(const std::exception& error){if(!output.empty())atomicText(output/L"results.json",Json{{"passed",false},{"error",error.what()},{"checks",checks}}.dump(2),false);std::cerr<<error.what();return 1;}
}
