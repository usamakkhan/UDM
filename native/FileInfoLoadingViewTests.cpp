#include "FileInfoLoadingView.hpp"
#include <iostream>
using namespace udm;
int wmain(int argc,wchar_t** argv){
 if(argc!=2)return 2;const auto root=fs::absolute(argv[1]);if(fs::exists(root))return 2;fs::create_directories(root);Json checks=Json::array();std::string error;
 const auto check=[&](bool ok,const char* name){checks.push_back({{"name",name},{"passed",ok}});if(!ok)throw std::runtime_error(name);};
 try{
  Manager manager(root/L"catalog");manager.state["Settings"]["DownloadFolder"]=utf8((root/L"files").wstring());manager.state["Settings"]["PrefetchFileInfo"]=false;
  auto direct=manager.add("https://files.example.test/file.bin",utf8((root/L"files").wstring()),"file.bin","Main queue",true);direct->data["Size"]=1234;direct->data["BrowserReportedFileName"]="server.zip";
  auto prepared=captureFileInfoInitialView(manager,*direct);auto before=manager.snapshot();const auto disk=readText(manager.root/L"state.json");
  check(!fileInfoLoadingView({},direct->id()).available,"No snapshot produces only an unknown advisory shell");
  auto wrong=fileInfoLoadingView(prepared,"unrelated-job");check(!wrong.available&&wrong.address.empty()&&wrong.destination.empty()&&wrong.serverName.empty(),"A snapshot belonging to another job exposes none of its display values");
  auto value=fileInfoLoadingView(prepared,direct->id());check(value.available&&value.address==str(direct->data,"Url")&&value.destination==utf8(direct->target().wstring()),"A matching immutable snapshot supplies address and destination without reading live state");
  check(value.amount==bytes(1234)&&value.kind=="Recorded size"&&value.detail=="Recorded size: 1234 bytes","A recorded direct size is distinguished from a newly verified final size");
  check(value.serverName=="server.zip","An ordinary browser filename is available as a read-only hint");
  direct->data["Size"]=9876;check(fileInfoLoadingView(prepared,direct->id()).amount==bytes(1234),"Live metadata changes cannot mutate an owned loading snapshot");direct->data["Size"]=1234;
  std::promise<void> release;auto gate=release.get_future().share();std::atomic_bool held=false;std::thread writer([&]{Lock lock(manager.mutex);held=true;gate.wait();});struct Release{std::promise<void>& release;std::thread& writer;~Release(){try{release.set_value();}catch(...){}if(writer.joinable())writer.join();}} cleanup{release,writer};
  const auto deadline=GetTickCount64()+3000;while(!held&&GetTickCount64()<deadline)Sleep(1);check(held,"A real writer holds the model before loading hints are rendered");
  auto pending=std::async(std::launch::async,[&]{return fileInfoLoadingView(prepared,direct->id());});check(pending.wait_for(std::chrono::seconds(1))==std::future_status::ready&&pending.get().amount==bytes(1234)&&held,"Loading hint formatting completes before the model writer is released");release.set_value();writer.join();
  check(manager.snapshot()==before&&readText(manager.root/L"state.json")==disk&&!manager.fileInfoDecisionPending,"Formatting hints changes no catalog, download or decision ownership");
  direct->data["Size"]=-1;direct->data["BrowserReportedDirectSize"]=4567;value=fileInfoLoadingView(captureFileInfoInitialView(manager,*direct),direct->id());check(value.amount==bytes(4567)&&value.kind=="Reported size","A browser direct-size hint retains its reported qualification");
  direct->data.erase("BrowserReportedDirectSize");value=fileInfoLoadingView(captureFileInfoInitialView(manager,*direct),direct->id());check(value.amount==bytes(-1)&&value.detail.empty(),"An absent direct-size hint remains unknown rather than zero");
  direct->data["ReplacementOf"]="original";check(fileInfoLoadingView(captureFileInfoInitialView(manager,*direct),direct->id()).serverName.empty(),"A replacement download does not advertise name adoption");direct->data.erase("ReplacementOf");direct->data["RefreshPendingValidation"]=true;check(fileInfoLoadingView(captureFileInfoInitialView(manager,*direct),direct->id()).serverName.empty(),"An unvalidated address cannot advertise an old browser filename");direct->data.erase("RefreshPendingValidation");direct->data["BrowserReportedFileName"]="../escape.zip";check(fileInfoLoadingView(captureFileInfoInitialView(manager,*direct),direct->id()).serverName.empty(),"Unsafe filenames are excluded from the advisory shell");
  auto media=manager.receive({{"action","media"},{"url","https://www.youtube.com/watch?v=Q3TI27IN7X0"},{"filename","video.mp4"},{"height",360},{"formatId","18"},{"videoUrl","https://rr1.googlevideo.com/videoplayback?id=fixture"},{"reportedSize",12345678},{"estimatedSize",23456789}});
  const auto render=[&]{return fileInfoLoadingView(captureFileInfoInitialView(manager,*media),media->id());};value=render();check(value.amount==bytes(12345678)&&value.kind=="Selected track size"&&value.detail=="Selected tracks: 12345678 bytes","Reported media bytes appear immediately with a track-total qualification");
  media->data.erase("ReportedMediaSize");value=render();check(value.amount=="~ "+bytes(23456789)&&value.kind=="Estimated size","An estimated media hint stays approximate");media->data.erase("EstimatedMediaSize");check(render().amount==bytes(-1),"Media without track metadata or hints remains unknown");
  media->video->data["Size"]=4000;value=render();check(value.amount==bytes(4000)&&value.kind=="Selected track size","Known child-track bytes retain their track qualification");
  media->data["ReportedMediaSize"]=12345678;media->audio=std::make_shared<Job>(Json{{"Id",guid()},{"Size",-1}});check(render().amount==bytes(12345678),"An unknown companion track uses only the separately reported total");
  media->video->data["Size"]=std::numeric_limits<i64>::max();media->audio->data["Size"]=1;check(render().amount==bytes(12345678),"An overflowing child-track sum cannot become a false exact size");
  check(render().kind!="Exact final size"&&render().serverName.empty(),"Advisory media hints never claim a verified final file or offer ordinary filename adoption");
  manager.stop();
 }catch(const std::exception& failure){error=failure.what();}
 atomicText(root/L"results.json",Json{{"passed",error.empty()},{"checks",checks},{"error",error}}.dump(2),false);std::cout<<checks.size()<<" checks; "<<(error.empty()?"passed":error)<<"\n";return error.empty()?0:1;
}
