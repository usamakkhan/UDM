#include "Core.hpp"
#include <iostream>
using namespace udm;
int wmain(int argc,wchar_t** argv){
 if(argc!=2)return 2;WSADATA w{};WSAStartup(MAKEWORD(2,2),&w);CoInitializeEx(nullptr,COINIT_MULTITHREADED);
 try{fs::path root(argv[1]);fs::create_directories(root);Manager manager(root/L"state");manager.state["Settings"]["DownloadFolder"]=utf8((root/L"downloads").wstring());manager.state["Settings"]["CategoryFolders"]=false;manager.state["Settings"]["ProxyMode"]="Connect directly";manager.state["Settings"]["Retries"]=0;manager.state["Settings"]["Connections"]=8;manager.state["Settings"]["PrefetchFileInfo"]=false;manager.save();
  PipeServer pipe(manager,[]{});atomicText(root/L"ready.json",Json{{"ok",true}}.dump());
  for(int serial=1;;++serial){const auto input=root/(std::to_wstring(serial)+L".command.json"),output=root/(std::to_wstring(serial)+L".reply.json");for(int wait=0;!fs::exists(input);++wait){if(wait>2400)throw std::runtime_error("Isolated browser fixture command timed out.");if(wait%5==0)manager.tick();Sleep(100);}auto command=Json::parse(readText(input));if(str(command,"action")=="quit")break;Json response;
   try{JobPtr job;{Lock lock(manager.mutex);for(auto j:manager.jobs)if(j->id()==str(command,"id"))job=j;}if(!job)throw std::runtime_error("Unknown fixture download.");
    const auto action=str(command,"action");if(action=="arm")manager.beginAddressRefresh(job);else if(action=="apply")manager.applyMediaRefresh(job);else if(action=="start"){manager.resume(job);manager.tick();}else if(action=="pause")manager.pause(job);else if(action=="status"){}else if(action=="download"){
     {Lock lock(manager.mutex);job->data["Status"]="Downloading";manager.save();}
     const auto fixture=str(command,"fixture");std::string videoUrl,audioUrl;
     if(!fixture.empty()){Url route(fixture);if(route.scheme!="http"||route.host!="127.0.0.1")throw std::runtime_error("Fixture transport must be loopback HTTP.");Lock lock(manager.mutex);videoUrl=str(job->video->data,"Url");audioUrl=str(job->audio->data,"Url");job->video->data["Url"]=fixture+"/video.mp4";job->audio->data["Url"]=fixture+"/audio.mp4";job->video->data["Connections"]=1;job->audio->data["Connections"]=1;}
     try{mediaTransfer(manager,job,std::make_shared<Cancel>());}catch(const std::exception& e){Lock lock(manager.mutex);job->data["Status"]="Failed";job->data["Error"]=e.what();}
     {Lock lock(manager.mutex);if(!fixture.empty()){job->video->data["Url"]=videoUrl;job->audio->data["Url"]=audioUrl;}manager.save();}
    }else throw std::runtime_error("Unknown fixture operation.");
    Lock lock(manager.mutex);response={{"ok",true},{"id",job->id()},{"status",str(job->data,"Status")},{"error",str(job->data,"Error")},{"downloads",manager.jobs.size()},{"direct",!str(job->data,"SourceUrl").empty()&&str(job->data,"ProtectedSabr").empty()},{"received",(job->video?num(job->video->data,"Received"):0)+(job->audio?num(job->audio->data,"Received"):0)},{"videoReceived",job->video?num(job->video->data,"Received"):0},{"audioReceived",job->audio?num(job->audio->data,"Received"):0},{"pending",job->video&&yes(job->video->data,"RefreshPendingValidation")},{"candidate",str(manager.addressRefreshCandidate(job),"kind")},{"target",utf8(job->target().wstring())}};
   }catch(const std::exception& e){response={{"ok",false},{"error",e.what()}};}atomicText(output,response.dump(2));
  }
 }catch(const std::exception& e){std::cerr<<e.what()<<std::endl;return 1;}CoUninitialize();WSACleanup();return 0;
}
