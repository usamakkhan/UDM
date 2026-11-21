// Diagnostic launcher only: reuse the qualified App object and the MFC-supplied
// Windows entry point, then hold a controller gate after App teardown.
#include "Core.hpp"
#include "Version.hpp"
#include <shlobj.h>
#include <shlwapi.h>
#include <cstdio>
#include <fstream>
extern "C" int WINAPI wWinMain(HINSTANCE,HINSTANCE,PWSTR,int);
using namespace udm;

int wmain(){
 if(IsUserAnAdmin())return 2;
 wchar_t value[32768]{};
 const auto length=GetEnvironmentVariableW(L"UDM_IO_OBSERVER_REPORT",value,32768);
 if(!length||length>=32768)return 3;
 const auto report=fs::absolute(value),root=report.parent_path();
 if(report.filename()!=L"app-return.json"||!fs::exists(root/L".udm-app-observer")||fs::exists(report))return 4;
 puts("READY");fflush(stdout);if(getchar()!='\n')return 5;
 LARGE_INTEGER began{},ended{};QueryPerformanceCounter(&began);
 const auto code=wWinMain(GetModuleHandleW(nullptr),nullptr,PathGetArgsW(GetCommandLineW()),SW_HIDE);
 QueryPerformanceCounter(&ended);
 {
  std::ofstream output(report,std::ios::binary);
  output<<Json{{"passed",code==0},{"exitCode",code},{"compiledVersion",UDM_VERSION_STRING},
    {"appEntryCounter",began.QuadPart},{"appReturnedCounter",ended.QuadPart},
    {"scope","MFC Windows entry returned after Application ExitInstance; controller wait and CRT/global teardown excluded"}}.dump(2);
 }
 puts("DONE");fflush(stdout);if(getchar()!='\n')return 6;
 return code;
}
