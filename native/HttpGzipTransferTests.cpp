#include "Core.hpp"
#include "MediaSize.hpp"
#include "StreamProgress.hpp"
#include <iostream>
using namespace udm;
// The runner supplies a loopback server and its independently generated payload.
// This exercises the actual Manager/transfer objects, never the personal catalog.
int wmain(int argc,wchar_t** argv){
 if(argc!=4&&argc!=5)return 2;const auto root=fs::absolute(argv[1]);if(fs::exists(root))return 2;
 fs::create_directories(root);Json checks=Json::array();bool passed=false;std::string failure;
 auto check=[&](bool ok,const char* name){checks.push_back({{"name",name},{"passed",ok}});if(!ok)throw std::runtime_error(name);};
 try{
  WSADATA wsa{};if(WSAStartup(MAKEWORD(2,2),&wsa))throw std::runtime_error("WSA startup");
  const auto origin=utf8(argv[2]),expected=utf8(argv[3]);Manager manager(root/L"state");
  const auto files=argc==5?fs::absolute(argv[4]):root/L"files";if(fs::exists(files))throw std::runtime_error("Fresh output folder required");
  auto prefs=manager.state["Settings"];prefs["DownloadFolder"]=utf8(files.wstring());prefs["CategoryFolders"]=false;
  prefs["ProxyMode"]="Connect directly";prefs["Retries"]=0;prefs["StopOnWebPage"]=false;prefs["CorrectFileExtensions"]=false;prefs["PrefetchFileInfo"]=true;manager.setSettings(prefs);
  const auto make=[&](const std::string& route){auto job=manager.add(origin+"/"+route,"",route+".udmcodec","Main queue",true);job->data["Connections"]=8;return job;};
  const auto wait=[&](const JobPtr& job,bool waiting){
   const auto end=GetTickCount64()+20000;
   while(GetTickCount64()<end){manager.tick();{Lock lock(manager.mutex);if(waiting&&str(job->data,"MediaPhase")=="Waiting for confirmation")return true;if(!manager.isActive(job))return !waiting;}Sleep(10);}return false;
  };
  for(const auto* route:{"known","unknown","concatenated","empty"}){
   auto job=make(route);transfer(manager,job,std::make_shared<Cancel>());
   const bool empty=std::string(route)=="empty";
   check(str(job->data,"Status")=="Complete"&&fs::exists(job->target()),"Valid gzip response publishes through the production transfer path");
   check(empty?fs::file_size(job->target())==0:fileHash(job->target())==expected,"Published decoded bytes match the independently generated payload");
   check(num(job->data,"Size")==static_cast<i64>(fs::file_size(job->target()))&&num(job->data,"Received")==num(job->data,"Size"),"Completion records output bytes rather than encoded offsets");
   check(num(job->data,"HttpWireSize",-1)>=0&&num(job->data,"HttpWireReceived")==num(job->data,"HttpWireSize"),"Completion preserves a separate wire-byte audit");
  }
  for(const auto* route:{"crc","truncated","trailing","unsupported"}){
   auto job=make(route);bool rejected=false;try{transfer(manager,job,std::make_shared<Cancel>());}catch(...){rejected=true;}
   check(rejected&&!fs::exists(job->target())&&str(job->data,"Status")!="Complete","Malformed or unsupported encoded response never publishes a final file");
   if(std::string(route)!="unsupported")check(yes(job->data,"HttpPartsResetPending")&&!job->data.contains("CompletedPrefetch"),"Invalid gzip durably invalidates its completed wire receipt");
  }
  auto pending=make("prefetch");check(manager.beginPrefetch(pending)==PrefetchAdmission::Started,"Gzip prefetch starts before confirmation");
  check(wait(pending,true),"Gzip prefetch reaches verified confirmation without publishing");
  check(str(pending->data,"HttpContentEncoding")=="gzip","Gzip aliases persist the canonical representation for output size and retained-response reuse");
  i64 announced;{Lock lock(manager.mutex);announced=num(pending->data,"PrefetchedFinalSize",-1);check(announced>0&&announced==num(pending->data,"HttpDecodedSize")&&!fs::exists(pending->target()),"Before Start size is verified decoded output and target is absent");}
  const auto staged=pending->target().parent_path()/(L".udm-"+wide(pending->id())+L".assembling");
  check(fs::is_regular_file(staged)&&static_cast<i64>(fs::file_size(staged))==announced&&fileHash(staged)==expected,"Verified decoded prefetch is already staged in the destination folder before Start");
  check(manager.confirmPrefetchedDownload(pending,Json::object()),"Start confirms the existing prefetched response");
  check(wait(pending,false)&&str(pending->data,"Status")=="Complete"&&fileHash(pending->target())==expected&&static_cast<i64>(fs::file_size(pending->target()))==announced,"Final file exactly matches the size shown before Start");
  check(!fs::exists(staged),"Confirmation publishes the destination sibling without leaving a decoded staging copy");
  auto relocated=make("relocated");manager.beginPrefetch(relocated);
  check(wait(relocated,true),"Relocation fixture prepares the decoded response before confirmation");
  const auto previousStage=relocated->target().parent_path()/(L".udm-"+wide(relocated->id())+L".assembling");
  const auto changedFolder=root/L"relocated-files";
  check(manager.confirmPrefetchedDownload(relocated,Json{{"Folder",utf8(changedFolder.wstring())}}),"Confirmation can change the folder after decoded prefetch");
  check(wait(relocated,false)&&str(relocated->data,"Status")=="Complete"&&relocated->target().parent_path()==changedFolder&&fileHash(relocated->target())==expected,"A destination change publishes the same verified bytes through the relocation fallback");
  check(!fs::exists(previousStage),"Relocation removes the original decoded sibling after successful publication");
  auto deferred=make("deferred");manager.beginPrefetch(deferred);check(wait(deferred,true),"Download Later fixture has fully decoded bytes");
  manager.endPrefetch(deferred,true);check(wait(deferred,false)&&!fs::exists(deferred->target()),"Ending confirmation keeps the destination unpublished");
  manager.resume(deferred);check(wait(deferred,false)&&str(deferred->data,"Status")=="Complete"&&fileHash(deferred->target())==expected&&yes(deferred->data,"ReusedCompletedPrefetch"),"Resume reuses verified encoded bytes without a second response");
  auto paused=make("paused");manager.resume(paused);const auto end=GetTickCount64()+10000;
  for(;;){manager.tick();{Lock lock(manager.mutex);if(num(paused->data,"Received")>=65536)break;}if(GetTickCount64()>end)throw std::runtime_error("Pause fixture did not receive bytes");Sleep(5);}
  manager.pause(paused);check(wait(paused,false)&&str(paused->data,"Status")=="Paused"&&!fs::exists(paused->target()),"Pause checkpoints encoded offsets without publishing decoded bytes");
  const auto checkpoint=num(paused->data,"Received");check(checkpoint>0&&checkpoint<num(paused->data,"Size"),"Pause retains a genuine partial encoded response");
  manager.resume(paused);check(wait(paused,false)&&str(paused->data,"Status")=="Complete"&&fileHash(paused->target())==expected,"Encoded-byte range resume decodes to the complete original file");
  Json view={{"Status","Downloading"},{"Size",280},{"Received",280},{"HttpContentEncoding","gzip"},{"HttpDecodedSize",-1},{"HttpDecodedReceived",0},{"BrowserReportedDirectSize",280}};
  check(downloadSizeLabel(view)==bytes(-1)&&reportedDirectResponseSize(view)==0&&downloadPermille(view)==-1&&downloadSecondsLeft(view,100)==-1,"UI helpers never label compressed length as final size or use wire rate for decoded ETA");
  const auto wire=view;projectHttpOutput(view);check(num(view,"Size")==-1&&num(view,"Received")==0&&!view.contains("BrowserReportedDirectSize")&&num(wire,"Size")==280,"Output projection leaves authoritative wire metadata unchanged");
  manager.stop();passed=true;
 }catch(const std::exception& error){failure=error.what();}
 atomicText(root/L"results.json",Json{{"passed",passed},{"error",failure},{"checks",checks}}.dump(2),false);
 std::cout<<checks.size()<<" checks; "<<(passed?"passed":failure)<<"\n";return passed?0:1;
}
