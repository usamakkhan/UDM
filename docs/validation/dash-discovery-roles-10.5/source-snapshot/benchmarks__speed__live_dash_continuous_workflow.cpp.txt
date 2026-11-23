#include "LiveDash.hpp"
#include "StreamHash.hpp"
#include <atomic>
#include <iostream>
namespace udm {struct CatalogCheckpointTestAccess {static void observe(Manager& m,std::function<void(const char*)> f){Lock lock(m.mutex);m.catalogCheckpoint=std::move(f);}};}
using namespace udm;
int wmain(int argc,wchar_t** argv){
 if(argc!=4)return 2;const auto mode=utf8(argv[1]);if(mode!="record"&&mode!="resume")return 2;const auto root=fs::absolute(argv[2]);const auto config=Json::parse(readText(fs::absolute(argv[3])));
 if(mode=="record"){if(fs::exists(root))return 2;fs::create_directories(root);atomicText(root/L".udm-continuous-fixture","Owned continuous DASH fixture",false);}else if(readText(root/L".udm-continuous-fixture")!="Owned continuous DASH fixture")return 2;
 WSADATA sockets{};WSAStartup(MAKEWORD(2,2),&sockets);CoInitializeEx(nullptr,COINIT_MULTITHREADED);Json checks=Json::array(),result=Json::object();std::string error;std::atomic<int> publications{0};i64 assembled=-1;bool saveRequested=false;ULONGLONG saveTick=0,finishedTick=0;
 const auto check=[&](bool ok,const char* label){checks.push_back({{"name",label},{"passed",ok}});if(!ok)throw std::runtime_error(label);};
 try{
  Manager manager(root/L"catalog");JobPtr job;const auto plan=config.at("plan");
  if(mode=="record"){
   auto prefs=manager.state["Settings"];prefs["DownloadFolder"]=str(config,"destination");prefs["TemporaryFolder"]=utf8((root/L"temporary").wstring());prefs["CategoryFolders"]=false;prefs["ProxyMode"]="Connect directly";prefs["Retries"]=0;manager.setSettings(prefs);
   job=receiveAdaptive(manager,{{"url","https://example.invalid/owned-continuous"},{"filename",yes(plan,"audioOnly")?"continuous.m4a":"continuous.mp4"},{"plan",plan}});
  }else{check(manager.jobs.size()==1,"Resume opens exactly one owned recording");job=manager.jobs.front();check(Json::parse(reveal(str(job->data,"ProtectedAdaptive")))==plan,"Resume preserves the selected manifest and tracks");}
  CatalogCheckpointTestAccess::observe(manager,[&](const char* point){if(std::string_view(point)=="publication-preflight-begin"){Lock lock(manager.mutex);++publications;assembled=num(job->data.value("RetainedMediaOutput",Json::object()),"Size",-1);}});
  manager.resume(job);manager.dispatchQueueWork();const auto deadline=GetTickCount64()+45000;
  while(manager.isActive(job)&&GetTickCount64()<deadline){
   const auto trigger=str(config,"saveTrigger");if(!saveRequested&&!trigger.empty()&&fs::exists(wide(trigger))){saveTick=GetTickCount64();requestLiveHlsFinish(manager,job);saveRequested=true;}
   Sleep(10);
  }
  finishedTick=GetTickCount64();if(manager.isActive(job))manager.pause(job);manager.stop();CatalogCheckpointTestAccess::observe(manager,{});
  const auto saved=job->snapshot();result={{"status",str(saved,"Status")},{"error",str(saved,"Error")},{"mediaFolder",str(saved,"MediaFolder")},{"capturedSeconds",real(saved,"LiveCapturedSeconds")},{"sha256",str(saved,"Sha256")},{"size",num(saved,"Size",-1)},{"publications",publications.load()},{"assembledBytes",assembled},{"saveRequested",saveRequested},{"saveToFinishMs",saveRequested?Json(finishedTick-saveTick):Json()}};
  check(str(saved,"Status")=="Complete",("Continuous recording completes: "+str(saved,"Error")).c_str());check(publications==1,"Continuous recording publishes exactly once");
  const auto bytes=fs::file_size(job->target());check(assembled>0&&bytes==static_cast<uintmax_t>(assembled)&&fileHash(job->target())==str(saved,"Sha256"),"Assembled size, final size and saved SHA-256 match");result["finalBytes"]=bytes;
  Cancel token;DashRecordingSession session(fs::path(wide(str(saved,"MediaFolder")))/L"live-dash",plan,token);const auto folders=session.folders();result["periodFolders"]=folders;
  check(folders.size()==static_cast<size_t>(num(config,"expectedPeriods")),"All expected chapters belong to the durable recording session");
  if(!yes(plan,"audioOnly"))result["videoFrameMd5"]=execute(appDir()/L"tools/ffmpeg.exe",{L"-hide_banner",L"-loglevel",L"error",L"-xerror",L"-nostdin",L"-i",job->target().wstring(),L"-map",L"0:v:0",L"-fps_mode",L"passthrough",L"-f",L"framemd5",L"-"},30,token);
  if(yes(plan,"audioExpected")){
   result["audioProbe"]=Json::parse(execute(appDir()/L"tools/ffprobe.exe",{L"-v",L"error",L"-select_streams",L"a:0",L"-show_streams",L"-show_packets",L"-show_data_hash",L"sha256",L"-of",L"json",job->target().wstring()},30,token));
   const auto pcm=execute(appDir()/L"tools/ffmpeg.exe",{L"-hide_banner",L"-loglevel",L"error",L"-xerror",L"-nostdin",L"-i",job->target().wstring(),L"-map",L"0:a:0",L"-ac",L"1",L"-ar",L"48000",L"-acodec",L"pcm_s16le",L"-f",L"s16le",L"-"},30,token);StreamHash hash;hash.update(pcm.data(),pcm.size());result["audioPcmBytes"]=pcm.size();result["audioPcmSha256"]=hash.finish();
  }
  result["probe"]=Json::parse(execute(appDir()/L"tools/ffprobe.exe",{L"-v",L"error",L"-show_streams",L"-show_format",L"-of",L"json",job->target().wstring()},30,token));
  if(yes(config,"saveExpected"))check(saveRequested&&yes(saved,"LiveStopRequested"),"Save cancels the active capture and publishes only its retained interval");
 }catch(const std::exception& e){error=e.what();}
 result["passed"]=error.empty();result["fixtureError"]=error;result["checks"]=checks;atomicText(root/(wide(mode)+L"-results.json"),result.dump(2),false);std::cout<<checks.size()<<" checks; "<<error<<"\n";CoUninitialize();WSACleanup();return error.empty()?0:1;
}
