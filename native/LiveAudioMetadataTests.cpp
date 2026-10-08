#include "Core.hpp"
#include <iostream>
#include <fstream>
using namespace udm;
static Json checks=Json::array();
static void check(bool ok,const std::string& name){checks.push_back({{"name",name},{"passed",ok}});}
#include "LiveHlsTracksChecks.hpp"
int wmain(int argc,wchar_t** argv){
 if(argc!=2||fs::exists(argv[1]))return 2;
 fs::path root=fs::absolute(argv[1]);fs::create_directories(root);WSADATA ws{};WSAStartup(MAKEWORD(2,2),&ws);CoInitializeEx(nullptr,COINIT_APARTMENTTHREADED);std::string error;
 try{
  liveHlsTracksChecks(root);
  std::map<std::string,std::string> files;for(const std::wstring kind:{L"video",L"audio"})for(const auto& entry:fs::directory_iterator(root/L"live-track-source"/kind))if(entry.is_regular_file())files["/"+utf8(kind)+"/"+utf8(entry.path().filename().wstring())]=readText(entry.path());
  LiveTracksFixture server(files);Manager manager(root/L"metadata-state");manager.state["Settings"]["CategoryFolders"]=false;manager.state["Settings"]["DownloadFolder"]=utf8((root/L"metadata-outputs").wstring());Cancel cancel;
  for(const std::string container:{"mp4","ts","m4a"}){
   bool audioOnly=container=="m4a";Json tracks=Json::array();
   if(!audioOnly)tracks.push_back({{"kind","video"},{"playlist",server.url("/video/source.m3u8")},{"segments",Json::array({{{"url",server.url("/video/part0.m4s")}}})}});
   tracks.push_back({{"kind","audio"},{"playlist",server.url("/audio/source.m3u8")},{"segments",Json::array({{{"url",server.url("/audio/part0.m4s")}}})}});
   Json plan={{"type","hls"},{"live",true},{"height",audioOnly?0:180},{"audioOnly",audioOnly},{"audioExpected",true},{"audioName","Spanish commentary"},{"audioLanguage","es-MX"},{"container",container},{"tracks",tracks}};
   auto job=manager.receive({{"action","adaptive"},{"url",server.url("/player")},{"filename","live-metadata-"+container},{"plan",plan}});adaptiveTransfer(manager,job,std::make_shared<Cancel>());
   auto info=Json::parse(execute(appDir()/L"tools"/L"ffprobe.exe",{L"-v",L"error",L"-show_streams",L"-of",L"json",job->target().wstring()},30,cancel));atomicText(root/wide("probe-"+container+".json"),info.dump(2));
   int audio=0,video=0;for(const auto& stream:info["streams"]){video+=str(stream,"codec_type")=="video";if(str(stream,"codec_type")=="audio"){++audio;auto tags=stream.value("tags",Json::object());check(str(tags,"language")=="spa",container+" retains selected audio language");if(container!="ts")check(str(tags,"handler_name")=="Spanish commentary",container+" retains selected audio name");}}
   check(audio==1&&video==(audioOnly?0:1),container+" retains precisely selected track kinds");
   execute(appDir()/L"tools"/L"ffmpeg.exe",{L"-v",L"error",L"-xerror",L"-i",job->target().wstring(),L"-f",L"null",L"-"},30,cancel);check(str(job->data,"Status")=="Complete",container+" recording fully decodes and completes");
  }
 }catch(const std::exception& e){error=e.what();}
 bool passed=error.empty();for(const auto& row:checks)passed=passed&&yes(row,"passed");atomicText(root/L"results.json",Json{{"passed",passed},{"error",error},{"checks",checks}}.dump(2));std::cout<<checks.size()<<" checks; "<<error<<std::endl;CoUninitialize();WSACleanup();return passed?0:1;
}