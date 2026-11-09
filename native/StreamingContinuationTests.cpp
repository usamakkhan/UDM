#include "Streaming.hpp"
#include <iostream>
#include <future>
#include <fstream>
#include "GuiModels.hpp"
#include "StreamProgress.hpp"
#include "MediaStorage.hpp"
#include "ProtocolFallback.hpp"
#include "QueueRetry.hpp"
#include "CapturedResponseTestAccess.hpp"
using namespace udm;
static Json regressionChecks=Json::array();
static void check(bool ok,const std::string& name){regressionChecks.push_back({{"name",name},{"passed",ok}});if(!ok)throw std::runtime_error(name);}
template<class F>static void rejects(F f,const std::string& name){bool rejected=false;try{f();}catch(const std::exception&){rejected=true;}check(rejected,name);}
#include "MediaChecks.hpp"
#include "ParallelMediaChecks.hpp"
#include "AudioStreamingChecks.hpp"
// Only the isolated browser fixture uses this mode. The production bridge
// still receives the selection, and applying it is a separate test decision.
static int browserRefreshFixture(const fs::path& output,int attempts){
 if(attempts<1||attempts>5||!fs::is_regular_file(output/L".udm-live-recovery-fixture"))return 2;
 Json results=Json::array();std::string error;
 try{
  Manager manager(output/L"state");
  if(manager.jobs.size()!=1)throw std::runtime_error("Expected exactly one isolated download.");
  auto job=manager.jobs.front();
  const auto media=output/L"state"/L"media"/wide(job->id());
  if(!yes(job->data,"SabrSessionRefreshRequired")||num(job->data,"SabrRetainedBytes")<=0||
     fs::path(wide(str(job->data,"MediaFolder")))!=media||job->target().parent_path()!=output/L"downloads"||
     !job->video||!job->audio||job->video->target().parent_path()!=media||job->audio->target().parent_path()!=media)
   throw std::runtime_error("Expected an isolated held video with verified media.");
  PipeServer server(manager,[]{});
  for(int i=1;i<=attempts;++i){
   const auto suffix=std::to_wstring(i);
   auto markerPath=output/(L"refresh-page-"+suffix+L".txt");
   auto decision=output/(L"save-and-resume-"+suffix);
   const auto before=num(job->data,"SabrRetainedBytes");
   manager.beginAddressRefresh(job);
   atomicText(markerPath,Json{{"marker",manager.streamingRefreshMarker(job)},{"page",manager.mediaRefreshPage(job)}}.dump(),false);
   auto deadline=GetTickCount64()+90000;
   while(!fs::exists(decision)&&GetTickCount64()<deadline)Sleep(100);
   fs::remove(markerPath);
   if(!fs::exists(decision))throw std::runtime_error("Browser refresh selection deadline.");
   if(str(manager.addressRefreshCandidate(job),"kind")!="sabr")throw std::runtime_error("Expected the matching browser offer before applying it.");
   manager.applyMediaRefresh(job);manager.resume(job);
   deadline=GetTickCount64()+150000;
   Json attempt;
   while(GetTickCount64()<deadline){
    manager.tick();
    {Lock lock(manager.mutex);
     const auto status=str(job->data,"Status");
     if(!manager.isActive(job)&&(status=="Failed"||status=="Complete")){
      attempt={{"status",status},{"retainedBefore",before},{"reused",num(job->data,"SabrReusedBytes")},{"retainedAfter",num(job->data,"SabrRetainedBytes")},{"timelineMs",num(job->data,"StreamCompletedMs")},{"requests",num(job->data,"SabrRequestCount")},
       {"configuredConnections",num(job->data,"Connections")},{"effectiveConnections",num(job->data,"SabrConnections")},{"peakRequests",num(job->data,"SabrMaxConcurrentRequests")},{"refreshRequired",yes(job->data,"SabrSessionRefreshRequired")},{"rejection",job->data.value("SabrSessionRejection",Json())},{"error",str(job->data,"Error")}};
      break;
     }
    }
    Sleep(100);
   }
   if(attempt.is_null())throw std::runtime_error("Resumed transfer deadline.");
   results.push_back(attempt);atomicText(output/(L"refresh-result-"+suffix+L".json"),attempt.dump(),false);
   if(str(attempt,"status")=="Complete"||!yes(attempt,"refreshRequired"))break;
  }
 }catch(const std::exception& e){error=e.what();}
 atomicText(output/L"native-refresh-results.json",Json{{"attempts",results},{"error",error}}.dump(2),false);
 return error.empty()?0:1;
}
int wmain(int argc,wchar_t** argv){
 if(argc==4&&std::wstring(argv[1])==L"--browser-refresh")return browserRefreshFixture(fs::absolute(argv[2]),_wtoi(argv[3]));
 if(argc!=2||fs::exists(argv[1]))return 2;
 fs::path root=fs::absolute(argv[1]);fs::create_directories(root);
 Json checks=Json::array();std::string error;
 auto check=[&](bool ok,const char* name){checks.push_back({{"name",name},{"passed",ok}});if(!ok)throw std::runtime_error(name);};
 try{
 {
  Manager admission(root/L"prefetch-admission");admission.state["Settings"]["PrefetchFileInfo"]=true;
  auto pending=admission.add("http://127.0.0.1:9/prefetch.bin","","prefetch.bin","Main queue",true);
  pending->data["PrefetchedFinalSize"]=123;pending->data["ReusedPrefetchedOutput"]=true;pending->data["ContinuedMediaPrefetch"]=true;
  admission.save();const auto before=pending->data;const auto persisted=readText(admission.root/L"state.json");
  bool failed=false;{Handle held(CreateFileW((admission.root/L"state.json").c_str(),GENERIC_READ,FILE_SHARE_READ,nullptr,OPEN_EXISTING,0,nullptr));
   check(bool(held),"Prefetch catalog failure fixture holds the saved record");
   try{admission.beginPrefetch(pending);}catch(const std::exception&){failed=true;}
  }
  check(failed,"Prefetch admission reports a failed catalog save");
  check(!admission.isActive(pending),"Failed prefetch admission starts no transfer");
  check(readText(admission.root/L"state.json")==persisted,"Failed prefetch admission preserves the durable record");
  check(pending->data==before,"Failed prefetch admission restores confirmation and previous metadata in memory");
 }
 {
  Manager admission(root/L"media-prefetch-admission");
  const auto temporary=root/L"media-prefetch-temp";
  admission.state["Settings"]["PrefetchFileInfo"]=true;
  admission.state["Settings"]["TemporaryFolder"]=utf8(temporary.wstring());
  auto pending=admission.add("http://127.0.0.1:9/prefetch","", "prefetch.mp4","Main queue",false);
  // This fixture exercises the durable admission boundary without contacting an origin.
  pending->data["ProtectedAdaptive"]=protect("{}");
  admission.save();
  const auto before=pending->data;
  const auto statePath=admission.root/L"state.json";
  const auto persisted=readText(statePath);
  CapturedResponseTestAccess::saveCheckpoint(admission,[](const char*){throw std::runtime_error("Injected media admission failure");});
  bool failed=false;
  try{admission.beginPrefetch(pending);}catch(const std::exception&){failed=true;}
  CapturedResponseTestAccess::saveCheckpoint(admission,{});
  check(failed&&!admission.isActive(pending)&&pending->data==before&&readText(statePath)==persisted,"Failed media admission rolls back the working folder and starts no transfer");
  const auto restartedRoot=root/L"media-prefetch-restart";
  fs::create_directories(restartedRoot);
  {
   Lock lock(admission.mutex);
   check(admission.beginPrefetch(pending)==PrefetchAdmission::Started,"Media admission starts after saving the confirmation and work folder");
   const auto saved=Json::parse(readText(statePath)).at("Downloads").at(0);
   const auto expected=temporary/L"UDM-media"/wide(pending->id());
   check(yes(saved,"ConfirmationPending")&&str(saved,"Status")=="Awaiting confirmation"&&fs::path(wide(str(saved,"MediaFolder")))==expected,"A queued media offer saves a restart-safe confirmation and the selected folder together");
   fs::copy_file(statePath,restartedRoot/L"state.json");
   int checkpoints=0;
   CapturedResponseTestAccess::saveCheckpoint(admission,[&](const char*){++checkpoints;});
   try{CapturedResponseTestAccess::checkpointMediaStart(admission,pending);}
   catch(...){CapturedResponseTestAccess::saveCheckpoint(admission,{});throw;}
   CapturedResponseTestAccess::saveCheckpoint(admission,{});
   check(checkpoints==0,"Media worker startup reuses the saved confirmation and working folder");
  }
  Manager reopened(restartedRoot);
  check(reopened.jobs.size()==1&&str(reopened.jobs.front()->data,"Status")=="Paused"&&!yes(reopened.jobs.front()->data,"ConfirmationPending")&&!reopened.isActive(reopened.jobs.front()),"Restart cannot queue an interrupted, unconfirmed media prefetch");
 }
 {
  Manager admission(root/L"browser-media-prefetch");
  const auto temporary=root/L"browser-media-temp";
  admission.state["Settings"]["TemporaryFolder"]=utf8(temporary.wstring());
  auto token=guid();for(int at:{20,16,12,8})token.insert(at,"-");
  auto pending=admission.receive({{"action","adaptive"},{"mediaToken",std::to_string(epoch())+"-"+token},
   {"url","http://127.0.0.1:9/player"},{"filename","browser-prefetch.mp4"},
   {"plan",{{"type","hls"},{"height",360},{"audioExpected",false},{"container","mp4"},
    {"tracks",Json::array({{{"kind","video"},{"segments",Json::array({{{"url","http://127.0.0.1:9/segment.ts"}}})}}})}}}});
  const auto statePath=admission.root/L"state.json";
  const auto persisted=readText(statePath);
  const auto snapshot=Json::parse(persisted);
  const auto& saved=snapshot.at("Downloads").at(0);
  const auto folder=temporary/L"UDM-media"/wide(pending->id());
  check(str(saved,"Status")=="Awaiting confirmation"&&!yes(saved,"ConfirmationPending")&&
   fs::path(wide(str(saved,"MediaFolder")))==folder&&!fs::exists(folder),
   "The media receipt saves the staging path without starting prefetch or creating the folder");
  const auto recoveredRoot=root/L"browser-media-prefetch-reopened";
  fs::create_directories(recoveredRoot);
  {
   Lock lock(admission.mutex);
   check(admission.beginPrefetch(pending)==PrefetchAdmission::Started&&yes(pending->data,"ConfirmationPending"),
    "Fresh browser media starts prefetch with its confirmation gate held");
   CapturedResponseTestAccess::checkpointMediaStart(admission,pending);
   check(readText(statePath)==persisted,"File Info and media worker startup reuse the browser admission write");
   fs::copy_file(statePath,recoveredRoot/L"state.json");
  }
  Manager reopened(recoveredRoot);
  check(reopened.jobs.size()==1&&str(reopened.jobs.front()->data,"Status")=="Paused"&&
   fs::path(wide(str(reopened.jobs.front()->data,"MediaFolder")))==folder&&!reopened.isActive(reopened.jobs.front()),
   "Restart recovers the unconfirmed browser media and its original staging path");
  admission.endPrefetch(pending);
 }
 Manager manager(root/L"data");
 {auto settings=manager.state["Settings"];settings["DownloadFolder"]=utf8((root/L"files").wstring());settings["CategoryFolders"]=false;manager.setSettings(settings);}
 const std::string source="https://www.youtube.com/watch?v=abcdefghijk",endpoint="https://r1.googlevideo.com/videoplayback?id=fixture";
 Json offer={{"url",endpoint},{"body",b64(Proto().set(5,Bytes{1}).set(19,Proto().set(3,Bytes{2}).encode()).encode())},{"videoId","abcdefghijk"},{"capturedAt",epoch()},{"durationMs",1000},{"video",{{"id","137"},{"lastModified","12345"},{"mime","video/mp4"}}},{"audio",{{"id","140"},{"lastModified","67890"},{"mime","audio/mp4"}}}};
 {
  Manager display(root/L"sabr-display");
  display.state["Settings"]["DownloadFolder"]=utf8((root/L"sabr-display-files").wstring());
  for(bool audioOnly:{false,true}){
   auto selected=offer;
   if(audioOnly){
    selected.erase("video");
    selected["audio"]["mime"]="audio/mp4; codecs=\"mp4a.40.2\"";
   }
   Json message={{"action","sabr"},{"url",source},{"height",audioOnly?0:1080},{"pixelHeight",audioOnly?0:1080},
    {"formatId",audioOnly?"140":"137"},{"filename",audioOnly?"display.m4a":"display.mp4"},{"sabr",selected}};
   if(audioOnly)message["output"]="audio";
   auto accepted=display.receive(message);
   const auto inputs=downloadAddressInputs(*accepted);
   check(yes(accepted->data,"SabrTrackAddressBound")&&inputs.size()==1&&downloadAddress(inputs)==endpoint,
    audioOnly?"SABR audio admission exposes the bound endpoint without the protected session":"SABR video admission exposes the shared endpoint without the protected session");
   const auto savedUrl=accepted->audio->data["Url"];
   if(audioOnly)accepted->audio->data["Url"]="";
   else accepted->audio->data["Url"]=endpoint+"&mismatch=1";
   const auto fallback=downloadAddressInputs(*accepted);
   check(fallback.contains("ProtectedSabr")&&downloadAddress(fallback)==endpoint,"Missing or mismatched tracks fall back to the protected session address");
   accepted->audio->data["Url"]=savedUrl;
  }
 }
 auto job=[&](const char* name){auto j=manager.add(source,"",name);j->data["SourceUrl"]=source;j->data["ProtectedSabr"]=protect(offer.dump());setStreams(manager,j,endpoint,endpoint);return j;};
 auto frame=[](Bytes& out,unsigned type,const Bytes& data){for(unsigned n:{type,(unsigned)data.size()}){out.push_back(0xf0);for(int i=0;i<4;++i)out.push_back((unsigned char)(n>>(8*i)));}out.insert(out.end(),data.begin(),data.end());};
 Bytes first,second;Bytes savedHeader;
 for(int track=0;track<2;++track){auto format=offer[track?"audio":"video"];auto id=formatIdentity(format);auto& initial=track?second:first;
  frame(initial,42,Proto().set(1,std::string("abcdefghijk")).set(2,id).set(3,1000ULL).set(4,0ULL).set(5,str(format,"mime")).encode());
  for(int init=0;init<2;++init){unsigned h=1+track*2+init;auto p=Proto().set(1,h).set(2,std::string("abcdefghijk")).set(13,id).set(8,init?0ULL:1ULL).set(9,0ULL).set(14,4ULL);if(init)p.set(11,0ULL).set(12,1000ULL);
   auto& out=track==0&&init==0?first:second;frame(out,20,p.encode());
   if(track==0&&init==0){savedHeader=p.encode();frame(first,21,Bytes{1,'v','i'});frame(second,21,Bytes{1,'0','1'});}
   else frame(out,21,Bytes{(unsigned char)h,(unsigned char)(track?'a':'v'),(unsigned char)(init?'d':'i'),'0','1'});
   frame(second,22,Bytes{(unsigned char)h});
  }
 }
 auto transport=[](std::vector<Bytes> responses,const std::shared_ptr<int>& calls){return [responses,calls](const std::string&,const Bytes&)->StreamRead{if(*calls>=(int)responses.size())throw std::runtime_error("Fixture response budget exhausted");auto bytes=std::make_shared<Bytes>(responses[(*calls)++]);auto at=std::make_shared<size_t>(0);return [bytes,at](void* out,size_t size,const Cancel& c){c.check();auto n=std::min<size_t>({size,3,bytes->size()-*at});memcpy(out,bytes->data()+*at,n);*at+=n;return n;};};};
 auto calls=std::make_shared<int>(0);auto good=job("continued.mp4");sabrTransfer(manager,good,std::make_shared<Cancel>(),transport({first,second},calls));
 check(*calls==2,"A selected segment continues across exactly two responses");
 check(readText(good->video->target())=="vi01vd01"&&readText(good->audio->target())=="ai01ad01","Continued output matches uninterrupted track bytes");
 check(!fs::exists(good->target()),"Continuation does not publish an unmerged final file");
 {
  auto held=job("resume-page.mp4");
  held->data["SabrSessionRefreshRequired"]=true;
  held->data["SabrRetainedBytes"]=1024;held->data["SabrConnections"]=1;
  held->data["StreamCompletedMs"]=65234;held->data["StreamDurationMs"]=200000;
  held->data["SabrRanges"]=Json::array({{{"Start",0},{"Done",65234}}});
  manager.beginAddressRefresh(held);
  const auto marker=manager.streamingRefreshMarker(held);
  check(manager.mediaRefreshPage(held)==source+"&t=65s"+marker,"A held contiguous checkpoint opens YouTube at the verified playback second");
  held->data["SabrConnections"]=2;
  check(manager.mediaRefreshPage(held)==source+marker,"Missing parallel range metadata keeps the original recovery page");
  held->data["SabrRanges"]=Json::array({{{"Start",0},{"End",99999},{"Done",65234}},{{"Start",100000},{"End",199999},{"Done",5000}}});
  held->data["StreamCompletedMs"]=70234;
  check(manager.mediaRefreshPage(held)==source+"&t=65s"+marker,"A gap excludes later verified ranges from the recovery cursor");
  held->data["SabrRanges"][0]["Done"]=100000;held->data["StreamCompletedMs"]=105000;
  check(manager.mediaRefreshPage(held)==source+"&t=105s"+marker,"Adjacent verified ranges extend the zero-based recovery cursor");
  held->data["SabrRanges"][0]["End"]=99998;
  check(manager.mediaRefreshPage(held)==source+marker,"An invalid range boundary keeps the original recovery page");
  held->data["SabrRanges"]=Json::array({{{"Start",0},{"Done",65234}}});
  held->data["StreamCompletedMs"]=65234;
  held->data["SabrConnections"]=1;held->data["SabrRanges"][0]["Done"]=64000;
  check(manager.mediaRefreshPage(held)==source+marker,"An inconsistent checkpoint keeps the original recovery page");
  held->data["SabrRanges"][0]["Done"]=65234;held->data["SabrSessionRefreshRequired"]=false;
  check(manager.mediaRefreshPage(held)==source+marker,"An ordinary refresh keeps the original recovery page");
  manager.cancelAddressRefresh(held);
 }
 {
  Bytes rejection;frame(rejection,58,Proto().set(1,3ULL).encode());
  Bytes initContinuation;frame(initContinuation,21,Bytes{1,'0','1'});frame(initContinuation,22,Bytes{1});
  Bytes committed=first;committed.insert(committed.end(),initContinuation.begin(),initContinuation.end());
  frame(committed,46,{});
  for(bool retained:{false,true}){
   auto original=job(retained?"attestation-with-prefix.mp4":"attestation-at-start.mp4");
   original->data["Connections"]=1;original->data["MediaHeight"]=1080;original->data["MediaPixelHeight"]=1080;
   auto attempts=std::make_shared<int>(0);bool refreshRequired=false;
   try{sabrTransfer(manager,original,std::make_shared<Cancel>(),transport({retained?committed:rejection},attempts));}
   catch(const SabrSessionRefreshRequired&){refreshRequired=true;}
   check(refreshRequired&&*attempts==1&&yes(original->data,"SabrSessionRefreshRequired"),"A server request for session renewal stops without another request");
   check(num(original->data,"SabrRetainedBytes")== (retained?4:0)&&!fs::exists(original->target()),"A session rejection preserves only verified bytes and publishes nothing");
   const Json diagnostic={{"part",retained?46:58},{"window",1},{"cursorMs",0},{"request",1}};
   check(original->data.value("SabrSessionRejection",Json())==diagnostic,"Rejection records only the protocol part and numeric request location");
   original->data["Status"]="Failed";original->data["QueueOrigin"]=true;original->data["QueueMember"]=true;
   manager.save();
   Manager reopened(manager.root);auto j=reopened.jobs.back();
   check(j->data.value("SabrSessionRejection",Json())==diagnostic,"The rejection location survives catalog restart");
   check(j->id()==original->id()&&queueTransferBlocked(j->data)&&!queueFailureCanRetry(defaultQueue("Main queue"),j->data,true),"Restart retains the hold and the queue cannot replay the rejected session");
   reopened.state["Settings"]["PrefetchFileInfo"]=true;
   check(reopened.beginPrefetch(j)==PrefetchAdmission::Unavailable&&!reopened.isActive(j),"Reopening File Info does not prefetch the rejected session");
   for(const auto& other:reopened.jobs)if(other!=j)other->data["QueueMember"]=false;
   reopened.queueRun("Main queue",true);reopened.tick();
   check(!reopened.isActive(j),"Starting the queue does not start a held session");
   bool held=false;try{reopened.resume(j);}catch(const SabrSessionRefreshRequired&){held=true;}
   check(held&&str(j->data,"Status")=="Failed"&&reopened.canRefreshAddress(j),"Resume directs the user to the existing address refresh");
   // Exercise the policy for a catalog restored after a secondary-window challenge.
   if(retained){j->data["SabrConnections"]=4;j->data["SabrSessionRejection"]["window"]=2;}
   reopened.beginAddressRefresh(j);
   Json message={{"action","sabr"},{"url",source},{"height",1080},{"pixelHeight",1080},{"formatId","137"},{"filename","capture.mp4"},{"sabr",offer}};
   ::rejects([&]{reopened.receive(message);},"The same observed session cannot clear an attestation hold");
   message["sabr"]["url"]=endpoint+"&fresh=1";message["sabr"]["capturedAt"]=std::max(epoch(),num(offer,"capturedAt")+1);
   auto mismatch=message;mismatch["sabr"]["video"]["lastModified"]="54321";
   ::rejects([&]{reopened.receive(mismatch);},"A changed representation cannot replace the rejected session");
   check(reopened.receive(message)==j&&yes(j->data,"SabrSessionRefreshRequired"),"A matching offer keeps the hold until the user saves it");
   reopened.cancelAddressRefresh(j);
   check(yes(j->data,"SabrSessionRefreshRequired"),"Cancel leaves the session hold in place");
   reopened.beginAddressRefresh(j);reopened.receive(message);
   auto savedRoot=reopened.root;reopened.root=root/L"attestation-state-is-file";writeBytes(reopened.root,Bytes{1});
   ::rejects([&]{reopened.applyMediaRefresh(j);},"Failed replacement save is reported");reopened.root=savedRoot;
   check(yes(j->data,"SabrSessionRefreshRequired")&&!yes(j->data,"SabrSerialRecovery"),"Failed replacement save rolls back the hold and connection policy");
   reopened.applyMediaRefresh(j);
   check(!queueTransferBlocked(j->data)&&str(j->data,"Status")=="Paused"&&j->id()==original->id(),"Successful refresh unlocks the original download without starting it");
   const auto address=downloadAddressInputs(*j);
   check(address.size()==1&&downloadAddress(address)==endpoint+"&fresh=1","Renewal updates the bound display endpoint with the accepted session");
   check(!j->data.contains("SabrSessionRejection"),"A saved replacement clears the previous rejection location");
   const auto saved=Json::parse(readText(reopened.root/L"state.json")).at("Downloads").back();
   check(yes(j->data,"SabrSerialRecovery")==retained&&yes(saved,"SabrSerialRecovery")==retained,"Only a secondary challenge enables durable single-connection recovery");
   auto freshCalls=std::make_shared<int>(0);sabrTransfer(reopened,j,std::make_shared<Cancel>(),transport({first,second},freshCalls));
   check(*freshCalls==2&&num(j->data,"SabrReusedBytes")== (retained?4:0)&&readText(j->video->target())=="vi01vd01"&&readText(j->audio->target())=="ai01ad01","Fresh session accepts a split cached echo and completes the exact selected tracks");
   check(num(j->video->data,"TransferredBytes")==8&&num(j->audio->data,"TransferredBytes")==8,"Cached echoes do not count toward selected transfer progress");
  }
  auto reuse=job("abandoned-cached-echo.mp4");reuse->data["Connections"]=1;
  bool held=false;
  try{sabrTransfer(manager,reuse,std::make_shared<Cancel>(),transport({committed},std::make_shared<int>(0)));}
  catch(const SabrSessionRefreshRequired&){held=true;}
  check(held&&num(reuse->data,"SabrRetainedBytes")==4,"Framing fixture retains a verified initialization segment");
  auto rejectEcho=[&](const Bytes& next,const char* diagnostic,const char* name){
   bool rejected=false;
   try{sabrTransfer(manager,reuse,std::make_shared<Cancel>(),transport({first,next},std::make_shared<int>(0)));}
   catch(const std::exception& e){rejected=std::string(e.what())==diagnostic;}
   check(rejected&&num(reuse->data,"SabrRetainedBytes")==4&&!fs::exists(reuse->target()),name);
  };
  Bytes stray;frame(stray,21,Bytes{99,'x'});
  rejectEcho(stray,"Media arrived without a matching header.","A cached echo does not authorize an unknown continuation identifier");
  Bytes excess;frame(excess,21,Bytes{1,'x','y','z'});
  rejectEcho(excess,"Media exceeds its declared segment length.","Discarded cached bytes still obey the declared segment length");
  Bytes overlap;frame(overlap,21,Bytes{1,'0'});frame(overlap,20,savedHeader);
  rejectEcho(overlap,"Overlapping media segment identifiers.","A cached echo cannot reuse an identifier after sending data in the same response");
  Bytes restarted=first;restarted.insert(restarted.end(),second.begin(),second.end());
  sabrTransfer(manager,reuse,std::make_shared<Cancel>(),transport({first,restarted},std::make_shared<int>(0)));
  check(readText(reuse->video->target())=="vi01vd01"&&readText(reuse->audio->target())=="ai01ad01","A later response can replace an abandoned cached echo header");
 }
 {
  auto preferences=manager.state["Settings"],queues=manager.state["Queues"];for(auto& queue:manager.state["Queues"])queue.erase("Retries");manager.state["Settings"]["Retries"]=1;
  auto retried=job("connection-retry.mp4");auto count=std::make_shared<int>(0);StreamTransport responses=transport({first,second},std::make_shared<int>(0));
  StreamTransport interrupted=[&,count,responses](const std::string& url,const Bytes& body)mutable->StreamRead{
   ++*count;if(*count==2)throw InternetFailure("HTTP read",ERROR_WINHTTP_CONNECTION_ERROR);
   if(*count==3)responses=transport({first,second},std::make_shared<int>(0));
   return responses(url,body);
  };
  sabrTransfer(manager,retried,std::make_shared<Cancel>(),interrupted);
  check(*count==4&&num(retried->data,"SabrConnectionRetries")==1,"Dropped streaming connection retries within configured budget");
  check(readText(retried->video->target())=="vi01vd01"&&readText(retried->audio->target())=="ai01ad01","Connection retry discards partial data and preserves exact output");
  Bytes completeResponse=first;completeResponse.insert(completeResponse.end(),second.begin(),second.end());
  for(auto cut:std::vector<size_t>{1,first.size(),completeResponse.size()-3}){
   const auto name="read-retry-"+std::to_string(cut)+".mp4";auto recovering=job(name.c_str());int requests=0;size_t delivered=0;
   auto replay=transport({completeResponse},std::make_shared<int>(0));
   StreamTransport brokenRead=[&](const std::string& url,const Bytes& body)->StreamRead{
    if(++requests>1)return replay(url,body);
    auto offset=std::make_shared<size_t>(0);
    return [&,offset,cut](void* out,size_t capacity,const Cancel& cancel){
     cancel.check();if(*offset==cut)throw InternetFailure("HTTP read",ERROR_WINHTTP_CONNECTION_ERROR);
     auto n=std::min<size_t>({capacity,3,cut-*offset});memcpy(out,completeResponse.data()+*offset,n);*offset+=n;delivered+=n;return n;
    };
   };
   sabrTransfer(manager,recovering,std::make_shared<Cancel>(),brokenRead);
   check(delivered==cut&&requests==2&&num(recovering->data,"SabrConnectionRetries")==1,"Response read failure retries after receiving partial bytes");
   check(readText(recovering->video->target())=="vi01vd01"&&readText(recovering->audio->target())=="ai01ad01","Retry across framing, partial and committed segments preserves exact track bytes");
   check(!fs::exists(recovering->target()),"Recovered tracks are not published as an unmerged final file");
  }
  for(auto code:{ERROR_WINHTTP_CONNECTION_ERROR,ERROR_WINHTTP_SECURE_FAILURE}){
   manager.state["Settings"]["Retries"]=code==ERROR_WINHTTP_CONNECTION_ERROR?0:2;int attempts=0;bool failed=false;
   try{sabrTransfer(manager,job(code==ERROR_WINHTTP_CONNECTION_ERROR?"no-retry.mp4":"tls-failure.mp4"),std::make_shared<Cancel>(),[&](const std::string&,const Bytes&)->StreamRead{++attempts;throw InternetFailure("HTTP read",code);});}catch(const InternetFailure&){failed=true;}
   check(failed&&attempts==1,"Zero retry budget and TLS failures never replay the streaming request");
  }
  manager.state["Settings"]["Retries"]=1;
  for(auto status:{429,503}){
   auto name="server-retry-"+std::to_string(status)+".mp4";auto recovering=job(name.c_str());int requests=0;ULONGLONG rejectedAt=0,retriedAt=0;
   auto replay=transport({completeResponse},std::make_shared<int>(0));
   sabrTransfer(manager,recovering,std::make_shared<Cancel>(),[&](const std::string& url,const Bytes& body)->StreamRead{
    if(++requests==1){rejectedAt=GetTickCount64();throw HttpRejected(status,"1");}retriedAt=GetTickCount64();return replay(url,body);
   });
   check(requests==2&&retriedAt-rejectedAt>=1000&&num(recovering->data,"SabrServerRetries")==1,"Streaming honors the server Retry-After delay before retrying");
   check(readText(recovering->video->target())=="vi01vd01"&&readText(recovering->audio->target())=="ai01ad01","Temporary server rejection recovers exact audio and video tracks");
  }
  for(auto status:{401,403,410,503}){
   int requests=0;bool failed=false;auto name="server-terminal-"+std::to_string(status)+".mp4";
   try{sabrTransfer(manager,job(name.c_str()),std::make_shared<Cancel>(),[&](const std::string&,const Bytes&)->StreamRead{++requests;throw HttpRejected(status,status==503?"601":"");});}catch(const HttpRejected& e){failed=e.status==status;}
   check(failed&&requests==1,"Authentication, expired links and excessive server waits remain terminal");
  }
  {int requests=0;bool failed=false;
   try{sabrTransfer(manager,job("mixed-retry-limit.mp4"),std::make_shared<Cancel>(),[&](const std::string&,const Bytes&)->StreamRead{if(++requests==1)throw InternetFailure("HTTP read",ERROR_WINHTTP_CONNECTION_ERROR);throw HttpRejected(503,"0");});}catch(const HttpRejected&){failed=true;}
   check(failed&&requests==2,"Connection and server failures share one retry budget");
  }
  manager.state["Settings"]["Retries"]=2;
  auto exhausted=job("retry-exhausted.mp4");int attempts=0;bool exhaustedFailure=false;
  try{sabrTransfer(manager,exhausted,std::make_shared<Cancel>(),[&](const std::string&,const Bytes&)->StreamRead{++attempts;throw InternetFailure("HTTP read",ERROR_WINHTTP_TIMEOUT);});}catch(const InternetFailure& e){exhaustedFailure=e.code==ERROR_WINHTTP_TIMEOUT;}
  check(exhaustedFailure&&attempts==3,"Persistent timeout stops after the configured two retries");
  check(!fs::exists(exhausted->target())&&!fs::exists(exhausted->video->target()),"Exhausted retries do not publish incomplete media");
  auto overridden=job("queue-no-retry.mp4");bool matched=false;
  for(auto& queue:manager.state["Queues"])if(str(queue,"Name")==str(overridden->data,"Queue")){queue["Retries"]=0;matched=true;}
  attempts=0;bool overrideFailure=false;
  try{sabrTransfer(manager,overridden,std::make_shared<Cancel>(),[&](const std::string&,const Bytes&)->StreamRead{++attempts;throw InternetFailure("HTTP read",ERROR_WINHTTP_CONNECTION_ERROR);});}catch(const InternetFailure&){overrideFailure=true;}
  check(matched&&overrideFailure&&attempts==1,"Queue retry limit overrides the global retry preference");
  for(auto& queue:manager.state["Queues"])queue.erase("Retries");
  auto cancelledJob=job("cancel-retry.mp4");auto cancellation=std::make_shared<Cancel>();std::atomic_int cancelCalls{0};
  auto pending=std::async(std::launch::async,[&]{try{sabrTransfer(manager,cancelledJob,cancellation,[&](const std::string&,const Bytes&)->StreamRead{++cancelCalls;throw InternetFailure("HTTP read",ERROR_WINHTTP_CONNECTION_ERROR);});}catch(const Cancelled&){return true;}return false;});
  bool retrying=false;auto retryDeadline=GetTickCount64()+5000;
  while(GetTickCount64()<retryDeadline){{Lock lock(manager.mutex);retrying=str(cancelledJob->data,"MediaPhase")=="Retrying connection";}if(retrying)break;Sleep(1);}
  cancellation->stop=true;auto stoppedPromptly=pending.wait_for(std::chrono::milliseconds(1000))==std::future_status::ready;auto cancelled=pending.get();
  check(retrying&&cancelled&&stoppedPromptly&&cancelCalls==1,"Stop during retry backoff cancels promptly without another request");
  check(!fs::exists(cancelledJob->target())&&!fs::exists(cancelledJob->video->target()),"Cancelled retry does not publish incomplete media");
  manager.state["Settings"]=preferences;manager.state["Queues"]=queues;
 }
 auto rejects=[&](std::vector<Bytes> responses,const char* name){auto j=job(name);auto count=std::make_shared<int>(0);bool rejected=false;try{sabrTransfer(manager,j,std::make_shared<Cancel>(),transport(responses,count));}catch(const std::exception&){rejected=true;}check(rejected,name);check(!fs::exists(j->target())&&!fs::exists(j->video->target()),"Invalid continuation does not publish track or final output");};
 Bytes conflict;frame(conflict,20,savedHeader);conflict.insert(conflict.end(),second.begin(),second.end());rejects({first,conflict},"Repeated pending header is rejected");
 Bytes excess;frame(excess,21,Bytes{1,'0','1','2'});rejects({first,excess},"Continuation cannot exceed declared segment length");
 Bytes missing;frame(missing,22,Bytes{1});rejects({first,missing},"Early terminator cannot commit incomplete media");
 Bytes foreign;frame(foreign,42,Proto().set(1,std::string("wrongvideo1")).set(2,formatIdentity(offer["video"])).encode());rejects({first,foreign},"Changed video identity is rejected during continuation");
 auto emptyCalls=std::make_shared<int>(0);auto stalled=job("stalled.mp4");bool stopped=false;std::vector<Bytes> stalledResponses(10);stalledResponses[0]=first;try{sabrTransfer(manager,stalled,std::make_shared<Cancel>(),transport(stalledResponses,emptyCalls));}catch(const std::exception&){stopped=true;}
 check(stopped&&*emptyCalls==9,"Eight responses without selected bytes or completed coverage stop continuation");
  auto compressedResponses=[&](bool brotli,int fault){std::vector<Bytes> responses(2);const char* encoded[]={"H4sIAAAAAAAACivLNDAEADrmjqYEAAAA","H4sIAAAAAAAACitLMTAEAGl1Vq4EAAAA","H4sIAAAAAAAACkvMNDAEAByJQGsEAAAA","H4sIAAAAAAAACktMMTAEAE8amGMEAAAA"};
  for(int track=0;track<2;++track){auto format=offer[track?"audio":"video"];auto id=formatIdentity(format);frame(responses[track?1:0],42,Proto().set(1,std::string("abcdefghijk")).set(2,id).set(3,1000ULL).set(4,0ULL).set(5,str(format,"mime")).encode());
   for(int part=0;part<2;++part){unsigned header=1+track*2+part;const char* br[]={"iwGAdmkwMQM=","iwGAdmQwMQM=","iwGAYWkwMQM=","iwGAYWQwMQM="};auto bytes=unb64((brotli?br:encoded)[track*2+part]);bool first=track==0&&part==0;
    if(first){if(fault==1){if(brotli)bytes[0]=0xff;else bytes[bytes.size()-8]^=1;}if(fault==2)bytes.resize(bytes.size()-3);if(fault==3)bytes.push_back(42);if(fault==6||fault==8){auto raw=readText(fs::path(__FILE__).parent_path()/L"test-data"/(fault==6?(brotli?L"sabr-oversize.br":L"sabr-oversize.gz"):(brotli?L"sabr-drain.br":L"sabr-drain.gz")));bytes.assign(raw.begin(),raw.end());}if(fault==7){auto extra=bytes;bytes.insert(bytes.end(),extra.begin(),extra.end());}}
    auto p=Proto().set(1,header).set(2,std::string("abcdefghijk")).set(13,id).set(7,first&&fault==4?3ULL:(brotli?2ULL:1ULL)).set(8,part?0ULL:1ULL).set(9,0ULL).set(14,(unsigned long long)bytes.size()+(first&&fault==5?1:0));if(part)p.set(11,0ULL).set(12,1000ULL);
    frame(responses[first?0:1],20,p.encode());
    if(first){auto mid=bytes.size()/2;Bytes a{(unsigned char)header},b{(unsigned char)header};a.insert(a.end(),bytes.begin(),bytes.begin()+mid);b.insert(b.end(),bytes.begin()+mid,bytes.end());frame(responses[0],21,a);frame(responses[1],21,b);}
    else{bytes.insert(bytes.begin(),(unsigned char)header);frame(responses[1],21,bytes);}
    frame(responses[1],22,Bytes{(unsigned char)header});
   }
  }return responses;
 };
 auto gzipResponses=[&](int fault){return compressedResponses(false,fault);};auto gz=job("gzip.mp4");auto gzCalls=std::make_shared<int>(0);sabrTransfer(manager,gz,std::make_shared<Cancel>(),transport(gzipResponses(0),gzCalls));
 check(*gzCalls==2&&readText(gz->video->target())=="vi01vd01"&&readText(gz->audio->target())=="ai01ad01","Gzip video and audio decode correctly across responses");
 for(auto fault: {1,2,3,4,5}){auto name=std::string("gzip-invalid-")+std::to_string(fault)+".mp4";rejects(gzipResponses(fault),name.c_str());}
  auto oversized=job("gzip-oversized.mp4");bool sizeRejected=false;try{sabrTransfer(manager,oversized,std::make_shared<Cancel>(),transport(gzipResponses(6),std::make_shared<int>(0)));}catch(const std::exception& e){sizeRejected=std::string(e.what()).find("Decoded streaming segment exceeds the size limit")!=std::string::npos;}
 check(sizeRejected&&!fs::exists(oversized->target())&&!fs::exists(oversized->video->target()),"Gzip expansion above 64 MiB is rejected before publication");
  bool clean=true;for(const auto& entry:fs::recursive_directory_iterator(oversized->video->target().parent_path()))if(entry.is_regular_file()&&(entry.path().extension()==L".decoded"||entry.path().extension()==L".sabrpart"))clean=false;
 check(clean,"Rejected gzip expansion leaves no compressed or decoded temporary segment");
 auto members=job("gzip-members.mp4");sabrTransfer(manager,members,std::make_shared<Cancel>(),transport(gzipResponses(7),std::make_shared<int>(0)));
 check(readText(members->video->target())=="vi01vi01vd01","Concatenated gzip members are decoded in order");
  auto br=job("brotli.mp4");auto brCalls=std::make_shared<int>(0);sabrTransfer(manager,br,std::make_shared<Cancel>(),transport(compressedResponses(true,0),brCalls));
 check(*brCalls==2&&readText(br->video->target())=="vi01vd01"&&readText(br->audio->target())=="ai01ad01","Brotli video and audio decode correctly across responses");
 for(auto fault:{1,2,3,4,5}){auto name=std::string("brotli-invalid-")+std::to_string(fault)+".mp4";rejects(compressedResponses(true,fault),name.c_str());}
 auto brLarge=job("brotli-oversized.mp4");bool brSizeRejected=false;try{sabrTransfer(manager,brLarge,std::make_shared<Cancel>(),transport(compressedResponses(true,6),std::make_shared<int>(0)));}catch(const std::exception& e){brSizeRejected=std::string(e.what()).find("Decoded streaming segment exceeds the size limit")!=std::string::npos;}
 check(brSizeRejected&&!fs::exists(brLarge->target())&&!fs::exists(brLarge->video->target()),"Brotli expansion above 64 MiB is rejected before publication");
 bool brClean=true;for(const auto& entry:fs::recursive_directory_iterator(brLarge->video->target().parent_path()))if(entry.is_regular_file()&&(entry.path().extension()==L".decoded"||entry.path().extension()==L".sabrpart"))brClean=false;
 check(brClean,"Rejected Brotli expansion leaves no compressed or decoded temporary segment");
 for(bool brotli:{false,true}){auto drain=job(brotli?"brotli-drain.mp4":"gzip-drain.mp4");sabrTransfer(manager,drain,std::make_shared<Cancel>(),transport(compressedResponses(brotli,8),std::make_shared<int>(0)));check(readText(drain->video->target())==std::string(65537,'\0')+"vd01",brotli?"Brotli drains decoded output after input EOF":"Gzip drains decoded output after input EOF");}
 mediaChecks(manager,root);parallelMediaChecks(manager);parallelMediaChecks(manager,true);audioStreamingChecks(manager,root);
 }catch(const std::exception& e){error=e.what();}
 for(const auto& item:regressionChecks)checks.push_back(item);
 atomicText(root/L"results.json",Json{{"passed",error.empty()},{"error",error},{"checks",checks}}.dump(2),false);std::cout<<checks.size()<<" checks; "<<error<<std::endl;return error.empty()?0:1;
}
