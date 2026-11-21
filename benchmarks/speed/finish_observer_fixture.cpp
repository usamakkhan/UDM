// Owned observation control: no download manager, personal data or administrator token.
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <shlobj.h>
#include <cstdio>
#include <vector>
#include <string>
#include <filesystem>
int wmain(int argc,wchar_t** argv){
 if(argc!=3||IsUserAnAdmin())return 2;
 const auto parent=std::filesystem::absolute(argv[1]).parent_path();
 if(parent!=std::filesystem::absolute(argv[2]).parent_path()||!std::filesystem::exists(parent/L".udm-observer-fixture"))return 2;
 puts("READY");fflush(stdout);if(getchar()!='\n')return 3;
 HANDLE readPipe=nullptr,writePipe=nullptr;
 if(!CreatePipe(&readPipe,&writePipe,nullptr,4096))return 4;
 DWORD n=0;char packet[32]{},received[32]{};
 if(!WriteFile(writePipe,packet,sizeof(packet),&n,nullptr)||n!=sizeof(packet)||!ReadFile(readPipe,received,sizeof(received),&n,nullptr)||n!=sizeof(received)||!CloseHandle(writePipe)||!CloseHandle(readPipe))return 5;
 HANDLE other=CreateFileW(argv[2],GENERIC_WRITE,FILE_SHARE_READ,nullptr,CREATE_NEW,FILE_ATTRIBUTE_NORMAL,nullptr);
 if(other==INVALID_HANDLE_VALUE||!WriteFile(other,packet,sizeof(packet),&n,nullptr)||n!=sizeof(packet)||!CloseHandle(other))return 6;
 HANDLE f=CreateFileW(argv[1],GENERIC_READ|GENERIC_WRITE,FILE_SHARE_READ,nullptr,CREATE_NEW,FILE_ATTRIBUTE_NORMAL,nullptr);
 if(f==INVALID_HANDLE_VALUE)return 7;
 std::vector<unsigned char> body(8192);for(size_t i=0;i<body.size();++i)body[i]=static_cast<unsigned char>(i%251);
 if(!WriteFile(f,body.data(),static_cast<DWORD>(body.size()),&n,nullptr)||n!=body.size()||!FlushFileBuffers(f))return 8;
 HANDLE mapping=CreateFileMappingW(f,nullptr,PAGE_READWRITE,0,0,nullptr);if(!mapping)return 9;
 void* view=MapViewOfFile(mapping,FILE_MAP_WRITE,0,0,0);if(!view)return 10;
 static_cast<unsigned char*>(view)[0]=77;
 if(!FlushViewOfFile(view,body.size())||!UnmapViewOfFile(view)||!CloseHandle(mapping)||!FlushFileBuffers(f)||!CloseHandle(f))return 11;
 const auto output=std::wstring(argv[1])+L".done";if(!MoveFileExW(argv[1],output.c_str(),MOVEFILE_WRITE_THROUGH))return 12;
 puts("DONE");fflush(stdout);if(getchar()!='\n')return 13;return 0;
}
