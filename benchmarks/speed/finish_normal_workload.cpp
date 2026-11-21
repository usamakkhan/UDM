#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <shlobj.h>
#include <filesystem>
#include <fstream>
#include <iostream>
#include "third_party/json.hpp"
using Json=nlohmann::json;namespace fs=std::filesystem;
static void fresh(const fs::path& path,const Json& value){auto data=value.dump(2);std::ofstream f(path,std::ios::binary);if(!f||!f.write(data.data(),data.size()))throw std::runtime_error("Fixture report failed");}
int wmain(int argc,wchar_t**argv){try{if(argc!=2||IsUserAnAdmin())return 77;const auto root=fs::absolute(argv[1]);if(!fs::is_directory(root)||!fs::exists(root/L".udm-normal-flush-fixture")||fs::exists(root/L"probe.data"))return 2;
 fresh(root/L"started.json",{{"pid",GetCurrentProcessId()},{"elevated",false}});auto began=GetTickCount64();while(!fs::exists(root/L"go.flag")){if(GetTickCount64()-began>10000)return 3;Sleep(5);}HANDLE h=CreateFileW((root/L"probe.data").c_str(),GENERIC_WRITE,FILE_SHARE_READ,nullptr,CREATE_NEW,FILE_ATTRIBUTE_NORMAL,nullptr);if(h==INVALID_HANDLE_VALUE)return 4;struct File{HANDLE h;~File(){CloseHandle(h);}} owner{h};char bytes[4096]{};Json calls=Json::array();
 for(int i=0;i<2;++i){DWORD n=0;if(!WriteFile(h,bytes,sizeof(bytes),&n,nullptr)||n!=sizeof(bytes))return 5;LARGE_INTEGER a{},b{};QueryPerformanceCounter(&a);const bool ok=FlushFileBuffers(h)!=FALSE;auto error=ok?0:GetLastError();QueryPerformanceCounter(&b);calls.push_back({{"beginCounter",a.QuadPart},{"endCounter",b.QuadPart},{"ok",ok},{"error",error},{"thread",GetCurrentThreadId()}});if(!ok)return 6;}
 fresh(root/L"result.json",{{"passed",true},{"pid",GetCurrentProcessId()},{"elevated",false},{"bytes",8192},{"flushCalls",calls}});return 0;
 }catch(...){return 2;}}
