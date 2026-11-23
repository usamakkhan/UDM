#include "Core.hpp"
#include <fstream>
#include <iostream>
static unsigned forced=0;static bool denyLegacy=false;
static BOOL WINAPI limitedInfo(HANDLE h,FILE_INFO_BY_HANDLE_CLASS kind,LPVOID data,DWORD size){if(kind==FileIdInfo){++forced;SetLastError(ERROR_INVALID_PARAMETER);return FALSE;}return GetFileInformationByHandleEx(h,kind,data,size);}
static BOOL WINAPI limitedLegacy(HANDLE h,LPBY_HANDLE_FILE_INFORMATION info){if(denyLegacy){SetLastError(ERROR_INVALID_PARAMETER);return FALSE;}return GetFileInformationByHandle(h,info);}
#define GetFileInformationByHandleEx limitedInfo
#define GetFileInformationByHandle limitedLegacy
#include "CreatedStagingFile.hpp"
#undef GetFileInformationByHandleEx
#undef GetFileInformationByHandle
using namespace udm;
int wmain(int argc,wchar_t** argv){if(argc!=2)return 2;auto root=fs::absolute(argv[1]);if(fs::exists(root))return 2;fs::create_directories(root);unsigned checks=0;std::string error;const auto check=[&](bool ok){++checks;if(!ok)throw std::runtime_error("Legacy cleanup proof failed");};try{
 for(const std::string action:{"grown","equal-file","directory","unknown"}){auto file=root/fs::path(action+".bin"),moved=root/fs::path(action+".moved");CreatedStagingFile proof;{Handle output(CreateFileW(file.c_str(),GENERIC_READ|GENERIC_WRITE,0,nullptr,CREATE_NEW,0,nullptr));check(bool(output));denyLegacy=action=="unknown";proof.capture(output.h);denyLegacy=false;DWORD written=0;check(WriteFile(output.h,"payload",7,&written,nullptr)&&written==7);}
  if(action=="equal-file"||action=="directory"){check(MoveFileExW(file.c_str(),moved.c_str(),0)!=FALSE);if(action=="directory")fs::create_directory(file);else{std::ofstream out(file,std::ios::binary);out<<"payload";}}
  proof.removeOwned(file);check(action=="grown"?!fs::exists(file):(action=="directory"?fs::is_directory(file):fs::is_regular_file(file)));if(action=="equal-file"||action=="directory")check(fs::is_regular_file(moved)&&fs::file_size(moved)==7);
 }check(forced>0);
 }catch(const std::exception& e){error=e.what();}std::ofstream out(root/L"results.json");out<<"{\"passed\":"<<(error.empty()?"true":"false")<<",\"checks\":"<<checks<<",\"forced128BitQueryFailures\":"<<forced<<",\"noActualLegacyFilesystemClaim\":true}";std::cout<<checks<<" checks; "<<(error.empty()?"passed":error)<<"\n";return error.empty()?0:1;}
