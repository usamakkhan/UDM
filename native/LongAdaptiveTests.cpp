#include "Core.hpp"
#include "HlsRecording.hpp"
#include "MediaStorage.hpp"
#include "StreamProgress.hpp"
#include <iostream>
#include <fstream>
using namespace udm;
static Json checks=Json::array();
static void check(bool ok,const std::string& name){checks.push_back({{"name",name},{"passed",ok}});if(!ok)throw std::runtime_error(name);}
template<class F>static void rejects(F f,const std::string& name){bool rejected=false;try{f();}catch(const std::exception&){rejected=true;}check(rejected,name);}
#include "LiveHlsTracksChecks.hpp"
int wmain(int argc,wchar_t** argv){
 if(argc!=2||fs::exists(argv[1]))return 2;fs::path root=fs::absolute(argv[1]);fs::create_directories(root);WSADATA ws{};WSAStartup(MAKEWORD(2,2),&ws);CoInitializeEx(nullptr,COINIT_APARTMENTTHREADED);std::string error;
 try{
  Json limit={{"type","hls"},{"height",90},{"tracks",Json::array({{{"kind","video"},{"segments",Json::array()}}})}};
  for(int i=0;i<10000;++i)limit["tracks"][0]["segments"].push_back({{"url","https://media.test/part.m4s"}});
  validateAdaptive(limit);check(true,"Native accepts exactly 10000 parts");
  auto excess=limit;excess["tracks"].push_back({{"kind","audio"},{"segments",Json::array({{{"url","https://media.test/audio.m4s"}}})}});rejects([&]{validateAdaptive(excess);},"Native counts the aggregate across tracks");
  auto bytes=limit;bytes["padding"]="";bytes["padding"]=std::string(4*1024*1024-bytes.dump().size(),'x');validateAdaptive(bytes);check(true,"Native accepts exactly four MiB serialized plan");bytes["padding"]=bytes["padding"].get<std::string>()+"x";rejects([&]{validateAdaptive(bytes);},"Native rejects one byte beyond plan budget");
  auto source=root/L"source";fs::create_directories(source);Cancel cancel;
  execute(appDir()/L"tools"/L"ffmpeg.exe",{L"-v",L"error",L"-f",L"lavfi",L"-i",L"testsrc2=size=160x90:rate=100",L"-t",L"13",L"-c:v",L"libx264",L"-threads",L"2",L"-preset",L"ultrafast",L"-g",L"1",L"-sc_threshold",L"0",L"-f",L"hls",L"-hls_time",L"0.01",L"-hls_list_size",L"0",L"-hls_segment_type",L"fmp4",L"-hls_fmp4_init_filename",L"init.mp4",L"-hls_segment_filename",(source/L"part%d.m4s").generic_wstring(),(source/L"source.m3u8").generic_wstring()},60,cancel);
  std::vector<std::string> names={"init.mp4"};std::istringstream lines(readText(source/L"source.m3u8"));std::string line;while(std::getline(lines,line)){if(!line.empty()&&line.back()=='\r')line.pop_back();if(line.rfind("part",0)==0)names.push_back(line);}
  check(names.size()>1200,"Fixture contains over 1200 real independently encoded media fragments");std::map<std::string,std::string> files;for(const auto& name:names)files["/"+std::string(150,'x')+"/"+name]=readText(source/wide(name));
  Manager manager(root/L"data");manager.state["Settings"]["CategoryFolders"]=false;manager.state["Settings"]["DownloadFolder"]=utf8((root/L"outputs").wstring());manager.state["Settings"]["Retries"]=0;JobPtr job;
  {LiveTracksFixture server(files);auto plan=limit;plan["tracks"][0]["segments"]=Json::array();for(const auto& name:names)plan["tracks"][0]["segments"].push_back({{"url",server.url("/"+std::string(150,'x')+"/"+name)}});check(plan.dump().size()>200000,"Real capture exceeds old serialized plan budget");job=manager.receive({{"action","adaptive"},{"url",server.url("/watch")},{"filename","long-selection.mp4"},{"plan",plan}});job->data["ExpectedSha256"]=std::string(64,'0');rejects([&]{adaptiveTransfer(manager,job,std::make_shared<Cancel>());},"Publication validation keeps long capture recoverable");check(num(job->data,"AdaptiveCompletedSegments")==static_cast<i64>(names.size()),"Every long-selection fragment downloaded before publication");check(!fs::exists(job->target()),"Failed validation publishes no file");}
  job->data["ExpectedSha256"]="";manager.save();Manager restored(manager.root);JobPtr saved;for(auto item:restored.jobs)if(item->id()==job->id())saved=item;if(!saved)throw std::runtime_error("Missing recovered job");adaptiveTransfer(restored,saved,std::make_shared<Cancel>());check(str(saved->data,"Status")=="Complete","Long selection resumes from catalog with source offline");
  auto metadata=Json::parse(execute(appDir()/L"tools"/L"ffprobe.exe",{L"-v",L"error",L"-count_frames",L"-show_streams",L"-show_format",L"-of",L"json",saved->target().wstring()},30,cancel));
  check(metadata["streams"].size()==1&&str(metadata["streams"][0],"nb_read_frames")=="1300"&&std::abs(std::stod(str(metadata["format"],"duration"))-13)<0.02,"Final MP4 preserves all 1300 frames and full duration");execute(appDir()/L"tools"/L"ffmpeg.exe",{L"-v",L"error",L"-xerror",L"-i",saved->target().wstring(),L"-f",L"null",L"-"},30,cancel);check(true,"Long selection fully decodes");
 }catch(const std::exception& e){error=e.what();}atomicText(root/L"results.json",Json{{"passed",error.empty()},{"error",error},{"checks",checks}}.dump(2));std::cout<<checks.size()<<" checks; "<<error<<std::endl;CoUninitialize();WSACleanup();return error.empty()?0:1;
}
