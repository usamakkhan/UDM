#include "Core.hpp"
#include "ProductionUiTiming.hpp"
#include <iostream>
#include <fstream>
using namespace udm;
int wmain(int argc,wchar_t** argv){
 try{
  if(argc!=3)throw std::runtime_error("Owned folder and input required");
  const auto folder=fs::absolute(argv[1]);
  if(!fs::is_regular_file(folder/L".udm-live-file-info-fixture"))throw std::runtime_error("Owned marker required");
  const auto config=Json::parse(readText(argv[2]));Url url(str(config,"url"));
  if(url.host!="127.0.0.1")throw std::runtime_error("Loopback origin required");
  if(!nativeUiTraceInitialize()||!nativeUiTraceSink.view)throw std::runtime_error("Owned trace required");
  nativeUiTraceRecord("actor-manager-begin");
  Manager manager(folder/L"catalog");auto prefs=manager.state["Settings"];
  prefs["DownloadFolder"]=utf8((folder/L"files").wstring());prefs["CategoryFolders"]=false;
  prefs["ProxyMode"]="Connect directly";prefs["PrefetchFileInfo"]=false;prefs["StopOnWebPage"]=false;
  prefs["Connections"]=num(config,"connections");prefs["Retries"]=3;
  manager.setSettings(prefs);
  auto job=manager.add(str(config,"url"),utf8((folder/L"files").wstring()),"sample.bin","Main queue",true);
  manager.resume(job);manager.dispatchQueueWork();nativeUiTraceRecord("actor-manager-end");
  bool complete=false;
  for(;;){bool active;std::string status;{Lock lock(manager.mutex);active=manager.isActive(job);status=str(job->data,"Status");}
   if(!active&&(status=="Complete"||status=="Failed")){complete=status=="Complete";break;}Sleep(5);
  }
  nativeUiTraceRecord("actor-stop-begin");manager.stop();nativeUiTraceRecord("actor-stop-end");
  Json result={{"passed",complete},{"job",job->data},{"componentOnly",true}};
  std::ofstream output(folder/L"actor-result.json");output<<result.dump(2);output.close();
  nativeUiTraceRecord("actor-terminal");return complete?0:1;
 }catch(const std::exception& error){std::cerr<<error.what();return 1;}
}
