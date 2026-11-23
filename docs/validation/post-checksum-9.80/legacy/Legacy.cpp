#include "Core.hpp"
#include "PostResponse.hpp"
#include <fstream>
#include <iostream>
using namespace udm;
int wmain(int argc,wchar_t**argv){
 if(argc!=4)return 2;const auto mode=utf8(argv[1]);const auto root=fs::absolute(argv[2]);const auto url=utf8(argv[3]);
 try{
  if(Url(url).host!="127.0.0.1")throw std::runtime_error("Loopback form required");
  if(mode=="capture"){if(fs::exists(root))throw std::runtime_error("Fresh owned root required");fs::create_directories(root);std::ofstream(root/L".udm-post-checksum-fixture")<<"Owned legacy response";}
  else if(mode!="resume"||!fs::is_regular_file(root/L".udm-post-checksum-fixture"))throw std::runtime_error("Owned capture required");
  WSADATA sockets{};if(WSAStartup(MAKEWORD(2,2),&sockets))throw std::runtime_error("Socket initialization failed");
  struct Cleanup{~Cleanup(){WSACleanup();}} cleanup;
  Manager manager(root/L"state");JobPtr job;
  if(mode=="capture"){
   auto prefs=manager.state["Settings"];prefs["DownloadFolder"]=utf8((root/L"files").wstring());prefs["CategoryFolders"]=false;prefs["ProxyMode"]="Connect directly";prefs["Retries"]=0;prefs["StopOnWebPage"]=false;prefs["CorrectFileExtensions"]=false;prefs["PrefetchFileInfo"]=false;prefs["Connections"]=1;manager.setSettings(prefs);
   job=manager.add(url,"","legacy.bin","Main queue",true,{},std::string(64,'0'),Json{{"method","POST"},{"url",url},{"contentType","application/octet-stream"},{"body",b64(Bytes{'o','w','n','e','d'})}});
  }else{
   if(manager.jobs.size()!=1)throw std::runtime_error("One owned response required");job=manager.jobs[0];if(str(job->data,"Url")!=url)throw std::runtime_error("Owned address changed");
   {Lock lock(manager.mutex);job->data["ExpectedSha256"]="";manager.save();}
  }
  manager.resume(job);manager.dispatchQueueWork();
  for(;;){bool active;{Lock lock(manager.mutex);active=manager.isActive(job);}if(!active)break;Sleep(5);}
  manager.stop();const bool published=fs::is_regular_file(job->target());const auto raw=manager.root/L"parts"/wide(job->id())/(wide(job->id())+L".data");
  Json result={{"mode",mode},{"status",str(job->data,"Status")},{"error",str(job->data,"Error")},{"published",published},{"publishedSha256",published?fileHash(job->target()):""},{"serverSha256",str(job->data,"ServerSha256")},{"userSha256",str(job->data,"ExpectedSha256")},{"completedResponseCandidate",completedPostResponseCandidate(job->data)},{"reusedResponse",yes(job->data,"ReusedCompletedPrefetch")},{"rawPresent",fs::is_regular_file(raw)},{"rawSha256",fs::is_regular_file(raw)?fileHash(raw):""}};
  std::ofstream(root/wide(mode+"-result.json"))<<result.dump(2);std::cout<<result.dump(2);return 0;
 }catch(const std::exception&error){std::cerr<<error.what();return 1;}
}
