#pragma once
#include "AdaptiveCapture.hpp"
#include "AdaptiveJournal.hpp"
static void adaptiveRefreshChecks(const fs::path& root){
 Manager m(root/L"adaptive-refresh-state");m.state["Settings"]["CategoryFolders"]=false;m.state["Settings"]["DownloadFolder"]=utf8((root/L"adaptive-refresh-downloads").wstring());m.state["Settings"]["Retries"]=0;
 Fixture server;server.payload=readText(root/L"adaptive-source.mp4");const i64 split=(i64)server.payload.size()/2;
 Json plan={{"type","hls"},{"height",720},{"audioExpected",true},{"tracks",Json::array({{{"kind","video"},{"segments",Json::array({{{"url",server.url("/expired")},{"start",0},{"length",split}},{{"url",server.url("/expired")},{"start",split},{"length",(i64)server.payload.size()-split}}})}}})}};
 auto seed=[&](const char* name,const Json& p){
  auto j=m.receive({{"action","adaptive"},{"url",server.url((std::string("/player/")+name).c_str())},{"filename",name},{"plan",p}});
  j->data["Status"]="Paused";j->data["Connections"]=4;
  const auto folder=mediaWorkingDirectory(m,j)/L"adaptive";
  fs::create_directories(folder);
  writeBytes(folder/L"0.part",Bytes(server.payload.begin(),server.payload.begin()+(size_t)split));
  const auto savedPlan=Json::parse(reveal(str(j->data,"ProtectedAdaptive")));
  AdaptiveJournal journal(folder,2,adaptiveSelectionDigest(adaptiveSelection(savedPlan)));
  journal.checkpoint(Json{{"0",{{"size",split},{"sha256",fileHash(folder/L"0.part")}}}});
  m.save();
  return j;
 };
 auto fresh=[&](JobPtr j,const char* route="/range?fresh-private-token"){auto p=Json::parse(reveal(str(j->data,"ProtectedAdaptive")));for(auto& track:p["tracks"])for(auto& part:track["segments"])part["url"]=server.url(route);return Json{{"action","adaptive"},{"url",str(j->data,"Url")},{"filename","changed-title"},{"plan",p},{"userAgent","fresh-browser"},{"originCookies",{{Url(server.url("/")).origin,"session=fresh-private-cookie"}}}};};
 auto j=seed("expired-hls",plan);const auto id=j->id(),filename=str(j->data,"FileName");const auto count=m.jobs.size();auto folder=mediaWorkingDirectory(m,j)/L"adaptive",part=folder/L"0.part";const auto originalHash=fileHash(part),originalReceipt=readText(folder/L"completed.json");
 check(m.canRefreshAddress(j),"Paused recorded HLS exposes Refresh download address");
 rejects([&]{adaptiveTransfer(m,j,std::make_shared<Cancel>());},"Expired HLS session fails before publishing output");
 check(fileHash(part)==originalHash&&!fs::exists(j->target()),"Expired request preserves the original retained segment");m.beginAddressRefresh(j);auto message=fresh(j),original=j->data;
 for(const auto& change:std::vector<Json>{Json{{"height",1080}},Json{{"container","ts"}},Json{{"audioName","different language"}},Json{{"subtitleLanguage","fr"}},Json{{"type","dash"}}}){auto invalid=message;for(auto it=change.begin();it!=change.end();++it)invalid["plan"][it.key()]=it.value();rejects([&]{m.receive(invalid);},"Adaptive recapture rejects a changed media selection");}
 auto invalid=message;invalid["plan"]["tracks"][0]["segments"][0]["start"]=1;rejects([&]{m.receive(invalid);},"Adaptive recapture rejects changed byte ranges");
 invalid=message;invalid["plan"]["tracks"][0]["segments"].erase(1);rejects([&]{m.receive(invalid);},"Adaptive recapture rejects a changed segment count");
 invalid=message;invalid["plan"]["tracks"][0]["segments"][0]["timeline"]=1;rejects([&]{m.receive(invalid);},"Adaptive recapture rejects changed timeline alignment");
 invalid=message;invalid["originCookies"]["https://unrelated.invalid"]="secret";rejects([&]{m.receive(invalid);},"Adaptive recapture rejects credentials for an unrelated origin");
 invalid=message;invalid["originHeaders"]={{Url(server.url("/")).origin,{{"Cookie","bad\r\nInjected: yes"}}}};rejects([&]{m.receive(invalid);},"Adaptive recapture rejects injected request headers");
 check(j->data==original&&m.jobs.size()==count,"Rejected recaptures preserve the existing record and metadata");
 auto other=message;other["url"]=server.url("/another-video");check(!m.captureMediaRefresh(other),"Different video pages cannot replace an armed adaptive session");
 check(!m.captureAddressRefresh(server.url("/file.mp4"),{},filename,""),"Ordinary file capture cannot overwrite an adaptive recovery candidate");
 rejects([&]{m.refreshAddress(j,server.url("/file.mp4"));},"Manual URL-only replacement cannot detach an adaptive plan");
 check(m.receive(message)==j&&m.jobs.size()==count&&str(j->data,"ProtectedAdaptive")==str(original,"ProtectedAdaptive"),"Matching adaptive capture stages a review candidate on the original job");
 auto disk=readText(m.root/L"state.json");check(disk.find("fresh-private-token")==std::string::npos&&disk.find("fresh-private-cookie")==std::string::npos,"Replacement streaming URLs and cookies remain encrypted at rest");
 auto candidate=m.addressRefreshCandidate(j);candidate["received"]=epoch()-11*60*1000;j->data["ProtectedRefreshOffer"]=protect(candidate.dump());check(m.addressRefreshCandidate(j).is_null(),"Expired adaptive candidates cannot be applied");
 const auto tag=wide("adaptive-"+guid().substr(0,16));SetEnvironmentVariableW(L"UDM_INSTANCE_TAG",tag.c_str());{PipeServer pipe(m,[]{});auto reply=send(message,3000);check(yes(reply,"ok")&&yes(reply,"refreshPending")&&str(reply,"id")==id&&m.jobs.size()==count,"Real native pipe acknowledges adaptive recovery without creating a second job");}SetEnvironmentVariableW(L"UDM_INSTANCE_TAG",nullptr);
 m.cancelAddressRefresh(j);rejects([&]{m.applyMediaRefresh(j);},"Cancelled adaptive recovery cannot apply a stored candidate");m.beginAddressRefresh(j);check(m.addressRefreshCandidate(j).is_null(),"Reopening adaptive recovery clears its previous unapproved candidate");m.receive(message);
 auto before=j->data;const auto trueRoot=m.root,blocked=root/L"adaptive-blocked-storage";writeBytes(blocked,Bytes{1});m.root=blocked;rejects([&]{m.applyMediaRefresh(j);},"Adaptive recovery reports a state checkpoint failure");m.root=trueRoot;check(j->data==before&&fileHash(part)==originalHash,"Failed adaptive apply rolls back metadata and preserves saved parts");
 m.applyMediaRefresh(j);check(j->id()==id&&str(j->data,"FileName")==filename&&yes(j->data,"AdaptiveRefreshPendingValidation")&&readHeaders(j->data)["User-Agent"]=="fresh-browser","Applying adaptive recovery preserves job identity and filename with verification pending");
 check(fileHash(part)==originalHash&&readText(folder/L"completed.json")==originalReceipt,"Applying a capture does not overwrite retained bytes or their receipts");
 int requests=server.requests;{Manager reopened(m.root);auto restored=reopened.jobs.front();check(restored->id()==id&&yes(restored->data,"AdaptiveRefreshPendingValidation"),"Adaptive refresh validation survives an app restart");adaptiveTransfer(reopened,restored,std::make_shared<Cancel>());check(str(restored->data,"Status")=="Complete"&&!yes(restored->data,"AdaptiveRefreshPendingValidation")&&server.requests-requests==2,"Restarted HLS refresh verifies the saved half and downloads the missing half");check(num(restored->data,"TransferredBytes")==server.payload.size()&&num(restored->data,"Received")==num(restored->data,"Size"),"Refresh validation traffic is counted without inflating completed progress");execute(appDir()/L"tools"/L"ffmpeg.exe",{L"-v",L"error",L"-i",restored->target().wstring(),L"-f",L"null",L"-"},30,Cancel());check(true,"Refreshed HLS output fully decodes with audio and video");}
 auto dashPlan=plan;dashPlan["type"]="dash";auto dash=seed("expired-dash",dashPlan);m.beginAddressRefresh(dash);m.receive(fresh(dash));m.applyMediaRefresh(dash);requests=server.requests;adaptiveTransfer(m,dash,std::make_shared<Cancel>());check(str(dash->data,"Status")=="Complete"&&server.requests-requests==2,"Static DASH uses the same checked session-recovery path");check(!m.canRefreshAddress(dash),"Completed adaptive downloads do not expose address recovery");
 auto mismatch=seed("changed-content",plan);auto mismatchFolder=mediaWorkingDirectory(m,mismatch)/L"adaptive";const auto mismatchReceipt=readText(mismatchFolder/L"completed.json"),mismatchHash=fileHash(mismatchFolder/L"0.part");m.beginAddressRefresh(mismatch);m.receive(fresh(mismatch));m.applyMediaRefresh(mismatch);const auto bytes=server.payload;server.payload[0]^=1;requests=server.requests;
 rejects([&]{adaptiveTransfer(m,mismatch,std::make_shared<Cancel>());},"Changed bytes are rejected even when the refreshed server repeats the same ETag");server.payload=bytes;
 check(server.requests-requests==1&&!fs::exists(mismatchFolder/L"1.part")&&!fs::exists(mismatch->target()),"A failed saved-byte check blocks every missing-segment request and publication");
 check(fileHash(mismatchFolder/L"0.part")==mismatchHash&&readText(mismatchFolder/L"completed.json")==mismatchReceipt&&yes(mismatch->data,"AdaptiveRefreshPendingValidation"),"Failed refresh validation preserves the old cache, receipts and pending marker");
 m.beginAddressRefresh(mismatch);auto noCredentials=fresh(mismatch);noCredentials.erase("originCookies");noCredentials.erase("userAgent");m.receive(noCredentials);m.applyMediaRefresh(mismatch);check(Json::parse(reveal(str(mismatch->data,"ProtectedMediaCookies"))).empty()&&readHeaders(mismatch->data).empty(),"Fresh capture removes stale credentials that the browser no longer supplies");adaptiveTransfer(m,mismatch,std::make_shared<Cancel>());check(str(mismatch->data,"Status")=="Complete","A corrected recapture recovers after a failed content check");
 auto stopped=seed("cancel-validation",plan);auto stoppedFolder=mediaWorkingDirectory(m,stopped)/L"adaptive";const auto stopReceipt=readText(stoppedFolder/L"completed.json");m.beginAddressRefresh(stopped);m.receive(fresh(stopped,"/stall-headers"));m.applyMediaRefresh(stopped);auto cancel=std::make_shared<Cancel>();auto stalled=server.stalled.load();auto pending=std::async(std::launch::async,[&]{try{adaptiveTransfer(m,stopped,cancel);return false;}catch(const Cancelled&){return true;}});for(int n=0;n<200&&server.stalled==stalled;++n)Sleep(5);auto started=GetTickCount64();cancel->stop=true;bool cancelled=pending.get();check(cancelled&&GetTickCount64()-started<1500,"Pause promptly cancels retained-segment network verification");check(yes(stopped->data,"AdaptiveRefreshPendingValidation")&&readText(stoppedFolder/L"completed.json")==stopReceipt&&fileHash(stoppedFolder/L"0.part")==originalHash,"Cancellation retains the refresh checkpoint and original saved bytes");
 auto live=seed("live-not-refreshable",plan);live->data["LiveRecording"]=true;check(!m.canRefreshAddress(live),"Live recordings are excluded from recorded-media link recovery");
 {
  auto readyPlan=plan;
  for(auto& piece:readyPlan["tracks"][0]["segments"])piece["url"]=server.url("/range");
  auto job=m.receive({{"action","adaptive"},{"url",server.url("/player/receipt-contention")},{"filename","receipt-contention"},{"plan",readyPlan}});
  const auto cache=mediaWorkingDirectory(m,job)/L"adaptive";
  fs::create_directories(cache);
  const auto receipt=cache/L"0.receipt.json",scratch=cache/L"0.receipt.json.tmp";
  atomicText(receipt,"{}");
  Handle reader(CreateFileW(receipt.c_str(),GENERIC_READ,FILE_SHARE_READ,nullptr,OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL,nullptr));
  if(!reader)throw std::runtime_error("Cannot hold the segment receipt for replacement.");
  auto task=std::async(std::launch::async,[&]{adaptiveTransfer(m,job,std::make_shared<Cancel>());});
  const auto deadline=GetTickCount64()+5000;
  while(!fs::exists(scratch)&&GetTickCount64()<deadline&&task.wait_for(std::chrono::milliseconds(2))!=std::future_status::ready){}
  const bool replacing=fs::exists(scratch);
  bool responsive=false,uncommitted=false;
  const auto probeUntil=GetTickCount64()+120;
  while(replacing&&GetTickCount64()<probeUntil){
   std::unique_lock lock(m.mutex,std::try_to_lock);
   if(lock.owns_lock()){
    responsive=true;
    uncommitted=num(job->data,"AdaptiveCompletedSegments")<2;
    break;
   }
   Sleep(2);
  }
  CloseHandle(reader.h);reader.h=INVALID_HANDLE_VALUE;
  task.get();
  check(replacing&&responsive,"The catalog remains available while Windows waits to replace a segment receipt");
  if(responsive)check(uncommitted,"Segment completion is withheld until the receipt is durable");
  check(str(job->data,"Status")=="Complete"&&fileHash(job->target())==str(job->data,"Sha256"),"Releasing the receipt allows publication of the exact output");
 }
 for(bool blockCatalog:{false,true}){
  auto readyPlan=plan;
  for(auto& piece:readyPlan["tracks"][0]["segments"])piece["url"]=server.url("/range");
  const auto stateRoot=root/(blockCatalog?L"adaptive-catalog-held-state":L"adaptive-uncompacted-state");
  std::string hash;
  i64 exact=-1;
  {
   Manager before(stateRoot);
   before.state["Settings"]["CategoryFolders"]=false;
   before.state["Settings"]["DownloadFolder"]=utf8((root/(blockCatalog?L"adaptive-catalog-held-output":L"adaptive-uncompacted-output")).wstring());
   auto job=before.receive({{"action","adaptive"},{"url",server.url("/player/uncompacted")},{"filename","uncompacted"},{"plan",readyPlan}});
   const auto cache=mediaWorkingDirectory(before,job)/L"adaptive";
   fs::create_directories(cache);
   AdaptiveJournal journal(cache,2,adaptiveSelectionDigest(adaptiveSelection(readyPlan)));
   journal.checkpoint(Json::object());
   const auto snapshot=readText(cache/L"completed.json");
   const auto blockedPath=blockCatalog?stateRoot/L"state.json.tmp":cache/L"completed.json.tmp";
   Handle blocked;
   {
    Lock lock(before.mutex);
    if(before.beginPrefetch(job)!=PrefetchAdmission::Started)throw std::runtime_error("Cannot start the adaptive prefetch fixture.");
    blocked.h=CreateFileW(blockedPath.c_str(),GENERIC_WRITE,0,nullptr,CREATE_NEW,FILE_ATTRIBUTE_NORMAL,nullptr);
    if(!blocked)throw std::runtime_error("Cannot hold the scratch file for the adaptive prefetch fixture.");
   }
   const auto deadline=GetTickCount64()+12000;
   while(GetTickCount64()<deadline){
    {Lock lock(before.mutex);exact=num(job->data,"PrefetchedFinalSize",-1);hash=str(job->preparedMedia,"Sha256");if(exact>0||!before.isActive(job))break;}
    Sleep(10);
   }
   check(exact>0&&!hash.empty()&&!fs::exists(job->target()),blockCatalog?"Exact size is ready while the catalog cannot be rewritten":"Exact size is ready while the aggregate index cannot be rewritten");
   if(blockCatalog){
    if(exact>0)rejects([&]{before.confirmPrefetchedDownload(job,Json::object());},"Start still requires a durable catalog write");
    {Lock lock(before.mutex);check(exact>0&&yes(job->data,"ConfirmationPending")&&num(job->data,"PrefetchedFinalSize")==exact&&!fs::exists(job->target()),"A failed Start leaves the verified size and confirmation gate intact");}
    CloseHandle(blocked.h);
    blocked.h=INVALID_HANDLE_VALUE;
   }
   before.endPrefetch(job);
   check(readText(cache/L"completed.json")==snapshot&&journal.load().size()==2,"Both segment receipts recover from the original index generation");
  }
  Manager reopened(stateRoot);
  auto job=reopened.jobs.front();
  const int before=server.requests;
  adaptiveTransfer(reopened,job,std::make_shared<Cancel>());
  check(server.requests==before&&exact==(i64)fs::file_size(job->target())&&hash==fileHash(job->target()),"Reopened HLS assembles the identical file from receipts without downloading either segment");
 }
}
