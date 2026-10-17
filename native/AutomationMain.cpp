// Udm.Automation.exe: out-of-process COM server for Windows download automation.
// COM launches it when no desktop is hosting the class; it forwards calls to the
// desktop over the local pipe (starting the desktop when needed) and exits after
// ten idle seconds. --register/--unregister maintain the class, ProgID and
// type-library keys for the current user, or machine-wide with --machine.
#include "AutomationServer.hpp"
#include "Udm.Automation_i.c"
#include <shellapi.h>
#include <iostream>
using namespace udm;
int WINAPI wWinMain(HINSTANCE,HINSTANCE,PWSTR,int){
 HRESULT initialized=CoInitializeEx(nullptr,COINIT_APARTMENTTHREADED);if(FAILED(initialized))return 1;int result=0;UINT_PTR timer=0;bool processReference=false;std::unique_ptr<AutomationHost> host;
 try{
  int argc=0;auto argv=CommandLineToArgvW(GetCommandLineW(),&argc);if(!argv)throw std::runtime_error("Cannot read automation arguments.");std::unique_ptr<wchar_t*,decltype(&LocalFree)> args(argv,LocalFree);bool add=false,remove=false,machine=false;std::wstring extra;fs::path dataDirectory;
  for(int i=1;i<argc;++i){std::wstring arg=argv[i];if(arg==L"--register")add=true;else if(arg==L"--unregister")remove=true;else if(arg==L"--machine")machine=true;else if(arg==L"-Embedding"||arg==L"/Embedding"){}else if(arg==L"--instance-tag"||arg==L"--data-dir"){
   if(++i>=argc)throw std::invalid_argument("Missing automation context value.");std::wstring value=argv[i];if(arg==L"--instance-tag"){if(value.empty()||value.size()>32||value.find_first_not_of(L"ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789_-")!=std::wstring::npos)throw std::invalid_argument("Invalid automation instance tag.");SetEnvironmentVariableW(L"UDM_INSTANCE_TAG",value.c_str());}else{dataDirectory=fs::path(value);if(!dataDirectory.is_absolute())throw std::invalid_argument("Use an absolute automation data directory.");}
   extra+=L" "+arg+L" "+quote(value);
  }else throw std::invalid_argument("Unknown automation option.");}
  if(add&&remove)throw std::invalid_argument("Choose registration or unregistration.");
  if(add)automation::registerServer(extra,machine);else if(remove)automation::unregisterServer(machine);else{
   automation::checked(CoInitializeSecurity(nullptr,-1,nullptr,nullptr,RPC_C_AUTHN_LEVEL_PKT_PRIVACY,RPC_C_IMP_LEVEL_IDENTIFY,nullptr,EOAC_DISABLE_AAA|EOAC_NO_CUSTOM_MARSHAL,nullptr));CoAddRefServerProcess();processReference=true;
   host=std::make_unique<AutomationHost>([dataDirectory](const Json& message){auto reply=automationCommand(message,dataDirectory);if(!yes(reply,"ok"))throw std::runtime_error(str(reply,"error","UDM rejected the automation request."));});
   timer=SetTimer(nullptr,0,1000,nullptr);if(!timer)throw std::runtime_error("Cannot create automation idle timer.");
   MSG message{};while(GetMessageW(&message,nullptr,0,0)>0){auto& runtime=automationRuntime();if(message.message==WM_TIMER&&message.wParam==timer&&runtime.active==0&&GetTickCount64()-runtime.idleSince>10000){CoSuspendClassObjects();break;}TranslateMessage(&message);DispatchMessageW(&message);}
  }
 }catch(const std::exception& e){std::cerr<<e.what()<<std::endl;result=1;}
 if(timer)KillTimer(nullptr,timer);host.reset();if(processReference)CoReleaseServerProcess();automationRuntime().info.Release();CoUninitialize();return result;
}
