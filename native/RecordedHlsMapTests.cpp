#include "Core.hpp"
#include <iostream>
using namespace udm;
static Json checks=Json::array();
static void check(bool ok,const std::string& name){checks.push_back({{"name",name},{"passed",ok}});if(!ok)throw std::runtime_error(name);}
template<class F>static void rejects(F f,const std::string& name){bool rejected=false;try{f();}catch(...){rejected=true;}check(rejected,name);}
#include "LiveHlsChecks.hpp"
int wmain(int argc,wchar_t** argv){
 if(argc!=2||fs::exists(argv[1]))return 2;fs::path root=fs::absolute(argv[1]);fs::create_directories(root);WSADATA ws{};WSAStartup(MAKEWORD(2,2),&ws);CoInitializeEx(nullptr,COINIT_APARTMENTTHREADED);std::string error;
 try{
  Cancel cancel;auto tools=appDir()/L"tools";std::vector<std::string> media;
  for(int i=0;i<2;++i){auto dir=root/std::to_wstring(i);fs::create_directories(dir);
   execute(tools/L"ffmpeg.exe",{L"-hide_banner",L"-loglevel",L"error",L"-nostdin",L"-y",L"-f",L"lavfi",L"-i",i?L"color=blue:size=320x180:rate=10":L"color=red:size=320x180:rate=10",L"-t",L"2",L"-output_ts_offset",i?L"2":L"0",L"-c:v",L"libx264",L"-preset",L"ultrafast",L"-g",L"20",L"-f",L"hls",L"-hls_segment_type",L"fmp4",L"-hls_segment_options",i?L"video_track_timescale=20000":L"video_track_timescale=10000",L"-hls_time",L"2",L"-hls_fmp4_init_filename",(dir/L"init.mp4").wstring(),L"-hls_segment_filename",(dir/L"part%d.m4s").wstring(),(dir/L"source.m3u8").wstring()},30,cancel);
   media.push_back(readText(dir/L"init.mp4"));media.push_back(readText(dir/L"part0.m4s"));
  }
  check(media[0]!=media[2],"Fixture uses distinct initialization bytes and track timescales");
  LiveHlsFixture server(media,"ended");Manager manager(root/L"state");manager.state["Settings"]["CategoryFolders"]=false;manager.state["Settings"]["DownloadFolder"]=utf8((root/L"output").wstring());
  for(bool discontinuity:{false,true}){
   Json segments=Json::array({{{"url",server.url("/segment0.ts")},{"initialization",true}},{{"url",server.url("/segment1.ts")},{"duration",2.0},{"mapIndex",0}},{{"url",server.url("/segment2.ts")},{"initialization",true}},{{"url",server.url("/segment3.ts")},{"duration",2.0},{"mapIndex",2},{"discontinuity",discontinuity}}});
   Json plan={{"type","hls"},{"height",180},{"audioExpected",false},{"tracks",Json::array({{{"kind","video"},{"segments",segments}}})}};
   auto bad=plan;bad["tracks"][0]["segments"][3]["mapIndex"]=1;rejects([&]{validateAdaptive(bad);},"Map reference must address an initialization entry");
   auto job=manager.receive({{"action","adaptive"},{"url",server.url("/player")},{"filename",discontinuity?"mapped-boundary.mp4":"mapped-change.mp4"},{"plan",plan}});adaptiveTransfer(manager,job,std::make_shared<Cancel>());
   auto result=Json::parse(execute(tools/L"ffprobe.exe",{L"-v",L"error",L"-count_frames",L"-show_entries",L"format=duration:stream=nb_read_frames",L"-of",L"json",job->target().wstring()},20,cancel));
   check(str(job->data,"Status")=="Complete"&&std::stod(str(result["format"],"duration"))>=3.9,"Changing-map output retains both two-second segments");
   check(std::stoi(str(result["streams"][0],"nb_read_frames"))==40,"Changing-map output decodes all forty frames");
   execute(tools/L"ffmpeg.exe",{L"-v",L"error",L"-xerror",L"-i",job->target().wstring(),L"-f",L"null",L"-"},20,cancel);check(true,"Changing-map output decodes without errors");
  }
 }catch(const std::exception& e){error=e.what();}
 atomicText(root/L"results.json",Json{{"passed",error.empty()},{"error",error},{"checks",checks}}.dump(2),false);std::cout<<checks.size()<<" checks; "<<error<<std::endl;CoUninitialize();WSACleanup();return error.empty()?0:1;
}
