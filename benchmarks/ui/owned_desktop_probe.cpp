#include <windows.h>
#include <fstream>
#include <filesystem>
#include <string>
#define UDM_TOOLBAR_COMPONENT_TEST
namespace udm {namespace fs=std::filesystem;}
#include "../../native/UiDesktopAttestation.hpp"
int wmain(int argc,wchar_t** argv){
 if(argc!=2)return 2;MSG message{};PeekMessageW(&message,nullptr,0,0,PM_NOREMOVE);wchar_t actual[512]{},expected[512]{};DWORD bytes=0;
 if(!GetUserObjectInformationW(GetThreadDesktop(GetCurrentThreadId()),UOI_NAME,actual,sizeof(actual),&bytes))return 3;
 if(!GetEnvironmentVariableW(L"UDM_TEST_DESKTOP",expected,512))return 4;
 {std::wofstream output{std::filesystem::path(argv[1])};output<<actual;}
 Sleep(250);return std::wstring(actual)==expected?0:5;
}
