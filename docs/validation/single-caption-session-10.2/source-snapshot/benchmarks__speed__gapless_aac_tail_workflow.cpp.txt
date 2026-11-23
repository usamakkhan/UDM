#include "DashManifest.hpp"
#include "MediaPublication.hpp"
#include <iostream>
using namespace udm;
// Owned fixture only. The Python driver supplies and independently decodes media.
int wmain(int argc,wchar_t** argv){
 if(argc!=5&&argc!=6)return 2;const auto mode=utf8(argv[1]);const auto root=fs::absolute(argv[2]);Json checks=Json::array(),result;std::string error;
 WSADATA sockets{};WSAStartup(MAKEWORD(2,2),&sockets);CoInitializeEx(nullptr,COINIT_MULTITHREADED);
 const auto check=[&](bool ok,const char* label){checks.push_back({{"name",label},{"passed",ok}});if(!ok)throw std::runtime_error(label);};
 try{auto cancel=std::make_shared<Cancel>();
  if(mode=="capture"){
   check(!fs::exists(root),"Capture uses a fresh owned folder");fs::create_directories(root);auto snapshot=refreshDashManifest(utf8(argv[3]),{},Json::object(),cancel);DashLiveUpdate update;update.state.snapshot=snapshot;update.staticPresentation=true;
   for(size_t n=0;n<snapshot.manifest.nodes.size();++n)if(snapshot.manifest.nodes[n].name=="Representation"){auto track=selectDashTrack(snapshot,n,*cancel);update.windows.push_back(planDashTemplateWindow(track.addressing,0,1000,*cancel));update.state.tracks.push_back(std::move(track));}
   check(!update.state.tracks.empty(),"Owned manifest selects media tracks");const auto duration=std::stod(utf8(argv[4]));captureDashWindow(root,update,{},Json::object(),cancel,{},DashCaptureInterval{0,duration},1);auto index=sealDashRecording(root,*cancel);check(yes(index,"sealed"),"Captured resources seal before assembly");
  }else if(mode!="assemble"&&mode!="publish")throw std::runtime_error("Unknown fixture mode");
  const auto selection=argc==6?Json::parse(readText(fs::absolute(argv[5]))):Json::object();
  auto output=argc==6?assembleDashRecordingSession(root,{"."},selection,*cancel):assembleDashRecording(root,*cancel);result={{"reused",output.reused},{"bytes",output.bytes},{"sha256",output.sha256},{"duration",output.duration},{"start",output.start},{"subtitleCues",output.subtitleCues?Json(*output.subtitleCues):Json()},{"temporaryInputCount",output.temporaryInputs.size()},{"path",utf8(output.staging.wstring())}};
  check(output.bytes>0&&output.verifiedSource&&fileHash(output.staging)==output.sha256,"Assembly owns a verified complete file");
  if(mode=="publish"){
   check(output.reused,"A new process reuses the verified assembly receipt");Manager manager(root/L"publication-catalog");auto prefs=manager.state["Settings"];prefs["CategoryFolders"]=false;prefs["DownloadFolder"]=utf8(fs::absolute(argv[3]).wstring());manager.setSettings(prefs);auto job=manager.add("https://example.invalid/owned-tail.mp4","","owned-tail.mp4","Main queue",true);auto pin=output.verifiedSource->clone();output.verifiedSource.reset();publishMediaOutput(manager,job,output.staging,output.sha256,cancel,nullptr,std::move(pin));manager.stop();check(str(job->data,"Status")=="Complete"&&fs::file_size(job->target())==static_cast<uintmax_t>(output.bytes)&&fileHash(job->target())==output.sha256,"Accepted final file matches the pre-publication size and checksum");result["path"]=utf8(job->target().wstring());
  }
 }catch(const std::exception& e){error=e.what();}
 result["passed"]=error.empty();result["error"]=error;result["checks"]=checks;fs::create_directories(root);atomicText(root/(wide(mode)+L"-results.json"),result.dump(2),false);std::cout<<result.dump()<<"\n";CoUninitialize();WSACleanup();return error.empty()?0:1;
}
