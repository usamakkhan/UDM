#pragma once
#if defined(UDM_TOOLBAR_COMPONENT_TEST) || defined(UDM_CAPTURE_PRESENTATION_COMPONENT_TEST)
namespace udm {
// Test-only startup instrumentation. Query the executing child thread before
// any test window is created; never switch or borrow the interactive desktop.
inline bool attestOwnedUiDesktop(){
 wchar_t expected[512]{},actual[512]{},target[32768]{},image[32768]{};DWORD needed=0;
 if(!GetEnvironmentVariableW(L"UDM_TEST_DESKTOP",expected,512))return false;
 if(!GetEnvironmentVariableW(L"UDM_TEST_DESKTOP_ATTESTATION",target,32768)||!GetModuleFileNameW(nullptr,image,32768))ExitProcess(86);
 const fs::path destination(target),area=fs::path(image).parent_path().parent_path();
 if(destination.parent_path()!=area||destination.filename()!=std::wstring(expected)+L".desktop"||!fs::is_regular_file(area/L".udm-build-owned"))ExitProcess(86);
 if(!GetUserObjectInformationW(GetThreadDesktop(GetCurrentThreadId()),UOI_NAME,actual,sizeof(actual),&needed))ExitProcess(86);
 char encoded[2048]{};const int bytes=WideCharToMultiByte(CP_UTF8,0,actual,-1,encoded,sizeof(encoded),nullptr,nullptr);if(bytes<=1)ExitProcess(86);
 const HANDLE file=CreateFileW(target,GENERIC_WRITE,FILE_SHARE_READ,nullptr,CREATE_NEW,FILE_ATTRIBUTE_NORMAL,nullptr);if(file==INVALID_HANDLE_VALUE)ExitProcess(86);DWORD written=0;const bool saved=WriteFile(file,encoded,bytes-1,&written,nullptr)&&written==static_cast<DWORD>(bytes-1);CloseHandle(file);
 if(!saved||std::wstring(expected)!=actual)ExitProcess(86);return true;
}
inline const bool ownedUiDesktopAttested=attestOwnedUiDesktop();
}
#endif
