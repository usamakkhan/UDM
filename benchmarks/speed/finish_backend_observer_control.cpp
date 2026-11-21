// Diagnostic entry point linked with qualified core objects; no product edits.
#include "Core.hpp"
#include "Version.hpp"
#include <shlobj.h>
#include <cstdio>
#include <fstream>
using namespace udm;
int wmain(int argc,wchar_t** argv){
 if(argc!=4||IsUserAnAdmin())return 2;
 const auto root=fs::absolute(argv[1]);if(!fs::is_directory(root)||!fs::exists(root/L".udm-backend-observer")||fs::exists(root/L"catalog"))return 3;
 const auto url=utf8(argv[2]);if(url.rfind("http://127.0.0.1:",0)!=0)return 4;
 const int connections=_wtoi(argv[3]);if(connections!=1&&connections!=8)return 5;
 WSADATA wsa{};if(WSAStartup(MAKEWORD(2,2),&wsa))return 6;CoInitializeEx(nullptr,COINIT_MULTITHREADED);
 puts("READY");fflush(stdout);if(getchar()!='\n')return 7;
 Json result;std::string failure;
 try{
  {
   Manager manager(root/L"catalog",Manager::StartupRecovery::Desktop);auto prefs=manager.state["Settings"];
   prefs["DownloadFolder"]=utf8((root/L"files").wstring());prefs["CategoryFolders"]=false;prefs["Connections"]=connections;prefs["Parallel"]=1;prefs["ProxyMode"]="Connect directly";prefs["Retries"]=0;prefs["StopOnWebPage"]=false;prefs["PrefetchFileInfo"]=false;prefs["ShowTipsOnStartup"]=false;prefs["ClipboardMonitor"]=false;prefs["SuppressCompletionDialog"]=true;prefs["SuppressProgressDialog"]=true;
   manager.setSettings(prefs);auto job=manager.add(url,"","sample.bin","Main queue",true);job->data["Connections"]=connections;manager.resume(job);
   for(;;){manager.tick();std::string status;{Lock lock(manager.mutex);status=str(job->data,"Status");}if(!manager.isActive(job)&&status!="Queued")break;Sleep(1);}
   {Lock lock(manager.mutex);result["job"]=job->data;}
   if(str(result["job"],"Status")!="Complete")throw std::runtime_error("Backend did not complete: "+str(result["job"],"Status")+" "+str(result["job"],"Error"));
   manager.stop();
  }
  LARGE_INTEGER done{};QueryPerformanceCounter(&done);result["backendDestroyedCounter"]=done.QuadPart;result["passed"]=true;result["compiledVersion"]=UDM_VERSION_STRING;
 }catch(const std::exception& e){failure=e.what();result["passed"]=false;result["error"]=failure;}
 // This report write is outside the measured backend-destruction endpoint.
 {std::ofstream report(root/L"backend-result.json",std::ios::binary);report<<result.dump(2);}
 CoUninitialize();WSACleanup();puts("DONE");fflush(stdout);if(getchar()!='\n')return 8;
 return failure.empty()?0:1;
}
