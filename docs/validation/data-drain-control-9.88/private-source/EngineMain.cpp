#include "DiagnosticSpan.hpp"
#include <iostream>
using namespace udm;
int wmain(int argc,wchar_t** argv){
 if(argc!=4)return 2;const fs::path root=fs::absolute(argv[1]);
 if(!nativeUiTraceInitialize())return 3;
 int result=1;try{WSADATA sockets{};if(WSAStartup(MAKEWORD(2,2),&sockets))throw std::runtime_error("WSA startup");
 {Manager manager(root/L"catalog");auto prefs=manager.state["Settings"];prefs["DownloadFolder"]=utf8((root/L"files").wstring());prefs["CategoryFolders"]=false;prefs["Connections"]=8;prefs["Parallel"]=1;prefs["SuppressProgressDialog"]=true;prefs["SuppressCompletionDialog"]=true;prefs["StopOnWebPage"]=false;prefs["PrefetchFileInfo"]=false;manager.setSettings(prefs);auto job=manager.add(utf8(argv[2]),"","sample.bin","Main queue",true);manager.resume(job);manager.tick();const auto began=GetTickCount64();
 for(;;){bool active;{Lock lock(manager.mutex);active=manager.isActive(job);}if(!active)break;if(GetTickCount64()-began>90000)throw std::runtime_error("Owned engine timeout");Sleep(2);}
 nativeUiTraceRecord("engine-workers-finished");manager.stop();nativeUiTraceRecord("engine-stop-finished");
 if(str(job->data,"Status")!="Complete"||lower(str(job->data,"Sha256"))!=lower(utf8(argv[3])))throw std::runtime_error("Engine completion/hash mismatch");
 atomicText(root/L"engine-result.json",Json{{"passed",true},{"job",job->snapshot()},{"diagnosticOnly",true}}.dump(2));}
 nativeUiTraceRecord("engine-manager-destroyed");result=0;
 }catch(const std::exception& error){std::cerr<<error.what()<<"\n";}return result;
}
