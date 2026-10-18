#pragma once
#include "MediaPublication.hpp"
static void mediaChecks(udm::Manager& manager,const udm::fs::path& root){
 using namespace udm;
 check(parseDate(Json("2023-11-14T22:13:20.000Z"))==1700000000000LL,"ISO date compatibility");
 check(parseDate(Json("2023-11-14T23:13:20.000+01:00"))==1700000000000LL,"ISO time-zone normalization");
 const std::string source="https://www.youtube.com/watch?v=abcdefghijk";
 const std::string endpoint="https://r1.googlevideo.com/videoplayback?id=fixture";
 auto body=Proto().set(5,Bytes{1}).set(19,Proto().set(3,Bytes{2}).encode()).encode();
 Json offer={{"url",endpoint},{"body",b64(body)},{"videoId","abcdefghijk"},{"capturedAt",epoch()},{"durationMs",1000},{"video",{{"id","137"},{"lastModified","12345"},{"mime","video/mp4"}}},{"audio",{{"id","140"},{"lastModified","67890"},{"mime","audio/mp4"}}}};
 validateSabr(offer,source);
 auto expired=offer;expired["capturedAt"]=epoch()-600000;
 rejects([&]{validateSabr(expired,source);},"Expired browser session rejected");
 auto wrong=offer;wrong["videoId"]="wrongvideo1";
 rejects([&]{validateSabr(wrong,source);},"Mismatched captured video rejected");
 auto makeJob=[&](const char* name){auto job=manager.add(source,"",name);job->data["SourceUrl"]=source;job->data["MediaHeight"]=720;job->data["MediaPixelHeight"]=720;job->data["ExactMediaQuality"]=true;job->data["ProtectedSabr"]=protect(offer.dump());setStreams(manager,job,endpoint,endpoint);return job;};
 auto integer=[](unsigned n){Bytes out{0xf0};for(int i=0;i<4;++i)out.push_back((unsigned char)(n>>(i*8)));return out;};
 auto frame=[&](Bytes& transcript,unsigned type,Bytes data){auto a=integer(type),b=integer((unsigned)data.size());transcript.insert(transcript.end(),a.begin(),a.end());transcript.insert(transcript.end(),b.begin(),b.end());transcript.insert(transcript.end(),data.begin(),data.end());};
 auto transcript=[&](bool gap,bool ad){Bytes out;for(int i=0;i<2;++i){auto f=offer[i==0?"video":"audio"];auto id=formatIdentity(f);frame(out,42,Proto().set(1,std::string(ad?"wrongvideo1":"abcdefghijk")).set(2,id).set(3,1000ULL).set(4,0ULL).set(5,str(f,"mime")).encode());for(int n=0;n<2;++n){unsigned header=1+i*2+n;auto p=Proto().set(1,header).set(2,std::string("abcdefghijk")).set(13,id).set(8,n==0?1ULL:0ULL).set(9,0ULL).set(14,4ULL);if(n==1)p.set(11,gap?5000ULL:0ULL).set(12,1000ULL);frame(out,20,p.encode());frame(out,21,Bytes{(unsigned char)header,(unsigned char)(i?'a':'v'),(unsigned char)(n?'d':'i'),'0','1'});frame(out,22,Bytes{(unsigned char)header});}}return out;};
 auto transport=[&](Bytes bytes){return [bytes](const std::string&,const Bytes& request)->StreamRead{auto p=Proto::parse(request);if(p.data(17).empty()||p.data(16).empty()||p.number(4)!=0)throw std::runtime_error("Incorrect captured streaming request.");auto offset=std::make_shared<size_t>(0);return [bytes,offset](void* out,size_t size,const Cancel& c){c.check();size_t n=std::min<size_t>({size,13,bytes.size()-*offset});memcpy(out,bytes.data()+*offset,n);*offset+=n;return n;};};};
 Bytes unidentified;frame(unidentified,42,Proto().set(2,formatIdentity(offer["video"])).set(3,1000ULL).set(5,std::string("video/mp4")).encode());
 auto unknown=makeJob("missing-video-identity.mp4");
 rejects([&]{sabrTransfer(manager,unknown,std::make_shared<Cancel>(),transport(unidentified));},"Streaming metadata without video identity is rejected before publication");
 auto stream=makeJob("stream-fixture.mp4");bool sawVideoAhead=false,coverageWaitedForAudio=true;
 auto baseTransport=transport(transcript(false,false));
 auto observedTransport=[&](const std::string& address,const Bytes& request)->StreamRead{auto read=baseTransport(address,request);return [&,read](void* out,size_t count,const Cancel& cancel){
  {Lock lock(manager.mutex);if(!stream->video->workers.empty()&&stream->video->workers[0].position==1000&&stream->audio->workers[0].position==0){sawVideoAhead=true;coverageWaitedForAudio&=num(stream->data,"StreamCompletedMs")==0;}}
  return read(out,count,cancel);
 };};
 sabrTransfer(manager,stream,std::make_shared<Cancel>(),observedTransport);
 check(sawVideoAhead&&coverageWaitedForAudio,"Playable streaming coverage waits for both audio and video");
 check(readText(stream->video->target())=="vi01vd01"&&readText(stream->audio->target())=="ai01ad01","SABR fragmented UMP transcript assembles both tracks");
 check(!fs::exists(stream->target()),"SABR transport does not publish unmerged output");
 check(num(stream->data,"StreamDurationMs")==1000&&num(stream->data,"StreamCompletedMs")==1000,"SABR publishes confirmed coverage for both tracks");
 check(stream->video->workers.size()==1&&stream->audio->workers.size()==1&&stream->video->workers[0].received==8&&stream->audio->workers[0].received==8,"SABR exposes actual audio and video bytes in progress rows");
 check(stream->video->workers[0].position==1000&&stream->audio->workers[0].position==1000,"SABR progress rows cover the complete recorded timeline");
 check(downloadPermille(stream->data)==999,"Streaming progress waits for final publication before showing 100 percent");
 Json progress={{"Status","Downloading"},{"Size",-1},{"StreamDurationMs",10000},{"StreamCompletedMs",2500},{"StreamElapsedSeconds",5}};
 check(downloadPermille(progress)==250&&downloadSecondsLeft(progress,100)==15,"Unknown-byte media uses verified coverage and estimated remaining time");
 check(downloadSecondsLeft(progress,0)==-1,"Stalled streaming does not display a misleading countdown");
 progress["Status"]="Paused";check(downloadSecondsLeft(progress,100)==-1,"Paused media hides remaining-time estimate");
 progress["Status"]="Complete";check(downloadPermille(progress)==1000,"Published media always displays complete progress");
 check(downloadPermille(Json{{"Size",1000},{"Received",250}})==250&&downloadPermille(Json{{"Size",-1}})==-1,"Byte-based and unknown-length downloads retain their progress semantics");
 check(mediaClock(1072994)=="17:52"&&mediaClock(-1)=="0:00","Media timeline labels use bounded minutes and seconds");


 auto originalTemp=str(manager.state["Settings"],"TemporaryFolder");
 manager.state["Settings"]["TemporaryFolder"]=utf8((root/L"media-temp-a").wstring());
 auto custom=makeJob("custom-media-storage.mp4");
 auto selected=mediaWorkingDirectory(manager,custom);
 check(selected==root/L"media-temp-a"/L"UDM-media"/wide(custom->id()),"YouTube media honors the selected temporary folder");
 sabrTransfer(manager,custom,std::make_shared<Cancel>(),transport(transcript(false,false)));
 check(custom->video->target().parent_path()==selected&&readText(custom->video->target())=="vi01vd01","Streaming writes verified track data in the selected temporary folder");
 manager.state["Settings"]["TemporaryFolder"]=utf8((root/L"media-temp-b").wstring());
 check(mediaWorkingDirectory(manager,custom)==selected,"Changing temporary preference preserves an existing media job location");
 auto legacy=manager.add(source,"","legacy-media-storage.mp4");auto legacyPath=manager.root/L"media"/wide(legacy->id());fs::create_directories(legacyPath);writeBytes(legacyPath/L"video.mp4",Bytes{1,2,3});
 check(mediaWorkingDirectory(manager,legacy)==legacyPath&&fs::file_size(legacyPath/L"video.mp4")==3,"Legacy media tracks remain in place after temporary preference changes");
 auto empty=manager.add(source,"","empty-media-storage.mp4");fs::create_directories(manager.root/L"media"/wide(empty->id()));
 check(mediaWorkingDirectory(manager,empty)==root/L"media-temp-b"/L"UDM-media"/wide(empty->id()),"Empty legacy working directory does not trap new media on the old drive");
 auto fullMessage=mediaStorageError(ERROR_DISK_FULL,selected/L"video.sabrpart","write segment");
 check(fullMessage.find("Not enough disk space")!=std::string::npos&&fullMessage.find(utf8(selected.wstring()))!=std::string::npos,"Disk-full error identifies the temporary folder and recovery action");
 auto blocked=makeJob("blocked-media-storage.mp4");auto blockedPath=blocked->video->target().parent_path()/L"video-init.sabrpart";fs::create_directories(blockedPath.parent_path());
 {Handle held(CreateFileW(blockedPath.c_str(),GENERIC_WRITE,0,nullptr,CREATE_ALWAYS,0,nullptr));bool diagnosed=false;try{sabrTransfer(manager,blocked,std::make_shared<Cancel>(),transport(transcript(false,false)));}catch(const std::exception& e){diagnosed=std::string(e.what()).find("Windows error 32")!=std::string::npos;}check(diagnosed,"Actual segment sharing failure reports the Windows error");}
 check(str(blocked->video->data,"Status")=="Failed"&&blocked->video->workers[0].state=="Stopped"&&!fs::exists(blocked->target()),"Failed media stops its worker rows without publishing output");
 manager.state["Settings"]["TemporaryFolder"]=originalTemp;

 auto gap=makeJob("gap-fixture.mp4");rejects([&]{sabrTransfer(manager,gap,std::make_shared<Cancel>(),transport(transcript(true,false)));},"SABR segment gap rejected");
 auto ad=makeJob("ad-fixture.mp4");rejects([&]{sabrTransfer(manager,ad,std::make_shared<Cancel>(),transport(transcript(false,true)));},"SABR foreign video or advertisement rejected");
 auto drm=makeJob("drm-fixture.mp4");Bytes encrypted;frame(encrypted,12,{});rejects([&]{sabrTransfer(manager,drm,std::make_shared<Cancel>(),transport(encrypted));},"SABR encrypted media rejected");
 auto sidecar=makeJob("compressed-sidecar.mp4");Bytes mixed;auto sidecarId=formatIdentity(Json{{"id","999"},{"lastModified","1"}});
 frame(mixed,20,Proto().set(1,99ULL).set(2,std::string("abcdefghijk")).set(13,sidecarId).set(7,1ULL).set(14,4ULL).encode());frame(mixed,21,Bytes{99,'a','d','0','1'});frame(mixed,22,Bytes{99});auto selectedFrames=transcript(false,false);mixed.insert(mixed.end(),selectedFrames.begin(),selectedFrames.end());
 sabrTransfer(manager,sidecar,std::make_shared<Cancel>(),transport(mixed));check(readText(sidecar->video->target())=="vi01vd01"&&readText(sidecar->audio->target())=="ai01ad01","Unselected compressed side track does not block the requested SABR video and audio");
 auto partialSidecar=makeJob("unfinished-sidecar.mp4");Bytes extra;frame(extra,20,Proto().set(1,99ULL).set(2,std::string("abcdefghijk")).set(13,sidecarId).set(14,4ULL).encode());frame(extra,21,Bytes{99,'a','d'});extra.insert(extra.end(),selectedFrames.begin(),selectedFrames.end());
 sabrTransfer(manager,partialSidecar,std::make_shared<Cancel>(),transport(extra));check(readText(partialSidecar->video->target())=="vi01vd01"&&readText(partialSidecar->audio->target())=="ai01ad01","Unfinished unselected side track does not invalidate complete requested SABR media");
 auto partialSelected=makeJob("unfinished-selected.mp4");Bytes incomplete;frame(incomplete,42,Proto().set(1,std::string("abcdefghijk")).set(2,formatIdentity(offer["video"])).set(3,1000ULL).set(5,std::string("video/mp4")).encode());frame(incomplete,20,Proto().set(1,1ULL).set(2,std::string("abcdefghijk")).set(13,formatIdentity(offer["video"])).set(14,4ULL).encode());frame(incomplete,21,Bytes{1,'v','i'});
 rejects([&]{sabrTransfer(manager,partialSelected,std::make_shared<Cancel>(),transport(incomplete));},"Unfinished requested SABR segment remains invalid");
 auto compressedSelected=makeJob("compressed-selected.mp4");Bytes selectedCompressed;auto selectedId=formatIdentity(offer["video"]);frame(selectedCompressed,42,Proto().set(1,std::string("abcdefghijk")).set(2,selectedId).set(3,1000ULL).set(4,0ULL).set(5,std::string("video/mp4")).encode());frame(selectedCompressed,20,Proto().set(1,1ULL).set(2,std::string("abcdefghijk")).set(13,selectedId).set(7,1ULL).set(14,4ULL).encode());
 rejects([&]{sabrTransfer(manager,compressedSelected,std::make_shared<Cancel>(),transport(selectedCompressed));},"Malformed requested gzip SABR segment is rejected");
 auto tools=appDir()/L"tools";
 if(fs::exists(tools/L"ffmpeg.exe")&&fs::exists(tools/L"ffprobe.exe")){
  Cancel c;auto inputVideo=root/L"generated-video.mp4",inputAudio=root/L"generated-audio.mp4";
  execute(tools/L"ffmpeg.exe",{L"-hide_banner",L"-loglevel",L"error",L"-nostdin",L"-y",L"-f",L"lavfi",L"-i",L"color=c=blue:s=1280x720:r=10:d=1",L"-c:v",L"libx264",L"-preset",L"ultrafast",L"-pix_fmt",L"yuv420p",inputVideo.wstring()},30,c);
  execute(tools/L"ffmpeg.exe",{L"-hide_banner",L"-loglevel",L"error",L"-nostdin",L"-y",L"-f",L"lavfi",L"-i",L"sine=frequency=440:duration=1",L"-c:a",L"aac",inputAudio.wstring()},30,c);
  auto makeMedia=[&](const char* name,int height){auto job=makeJob(name);job->data["ProtectedSabr"]="";job->data["MediaPixelHeight"]=height;fs::create_directories(job->video->target().parent_path());fs::copy_file(inputVideo,job->video->target());fs::copy_file(inputAudio,job->audio->target());job->video->data["Status"]="Complete";job->audio->data["Status"]="Complete";return job;};
  auto media=makeMedia("native-720p-assembly.mp4",720);mediaTransfer(manager,media,std::make_shared<Cancel>());
  check(str(media->data,"Status")=="Complete"&&fileHash(media->target())==str(media->data,"Sha256"),"Native media assembly publishes verified MP4");
  auto streams=execute(tools/L"ffprobe.exe",{L"-v",L"error",L"-show_entries",L"stream=codec_type,height",L"-of",L"json",media->target().wstring()},20,c);
  auto output=Json::parse(streams);bool video=false,audio=false;for(auto s:output["streams"]){video|=str(s,"codec_type")=="video"&&num(s,"height")==720;audio|=str(s,"codec_type")=="audio";}
  check(video&&audio,"Merged output has requested video height and audio");
  auto prefetched=makeMedia("prefetch-before-confirm.mp4",720);manager.state["Settings"]["PrefetchFileInfo"]=true;
  manager.beginPrefetch(prefetched);bool waiting=false;
  for(int i=0;i<400;++i){Sleep(10);Lock lock(manager.mutex);if(str(prefetched->data,"MediaPhase")=="Waiting for confirmation"){waiting=true;break;}}
  {Lock lock(manager.mutex);check(waiting&&manager.isActive(prefetched)&&yes(prefetched->data,"ConfirmationPending")&&!fs::exists(prefetched->target()),"Media prefetch holds completed tracks without publishing or completing the parent");}
  const auto predicted=num(prefetched->data,"PrefetchedFinalSize",-1);
  check(predicted==(i64)fs::file_size(media->target())&&predicted>0,"Verified prefetched final size matches the independently assembled output exactly");
  const auto videoHash=fileHash(prefetched->video->target()),audioHash=fileHash(prefetched->audio->target());auto stoppedAt=GetTickCount64();manager.endPrefetch(prefetched);
  check(GetTickCount64()-stoppedAt<2000&&!manager.isActive(prefetched)&&!fs::exists(prefetched->target())&&fileHash(prefetched->video->target())==videoHash&&fileHash(prefetched->audio->target())==audioHash,"Canceling media prefetch promptly preserves both verified tracks without final output");
  check(!prefetched->data.contains("PrefetchedFinalSize"),"Cancel clears the exact staging-size claim");
  manager.configure(prefetched,{{"FileName","prefetch-confirmed-renamed.mp4"}});mediaTransfer(manager,prefetched,std::make_shared<Cancel>());
  check(str(prefetched->data,"Status")=="Complete"&&fileHash(prefetched->target())==fileHash(media->target()),"Confirmation after rename reuses prefetched tracks and produces identical media");
  check((i64)fs::file_size(prefetched->target())==predicted,"Final renamed output has the exact size shown before confirmation");
  auto audioPreview=makeMedia("audio-prefetch.m4a",0);audioPreview->video.reset();audioPreview->data.erase("Video");audioPreview->data["MediaOutput"]="audio";
  manager.beginPrefetch(audioPreview);i64 audioSize=-1;
  for(int i=0;i<600;++i){Sleep(10);Lock lock(manager.mutex);audioSize=num(audioPreview->data,"PrefetchedFinalSize",-1);if(audioSize>0)break;}
  check(audioSize>0&&!fs::exists(audioPreview->target()),"M4A prefetch measures the verified final container before publication");
  manager.endPrefetch(audioPreview);mediaTransfer(manager,audioPreview,std::make_shared<Cancel>());
  check(audioSize==(i64)fs::file_size(audioPreview->target()),"Audio-only final bytes equal the pre-confirmation size");
  auto earlyMoved=makeMedia("early-move-source.mp4",720);auto earlyOriginal=earlyMoved->target();
  {Lock lock(manager.mutex);manager.beginPrefetch(earlyMoved);
   check(manager.confirmPrefetchedMedia(earlyMoved,{{"Folder",utf8((root/L"early-final").wstring())},{"FileName","early-move-final.mp4"}}),"Immediate Start accepts a changed destination before worker startup");}
  for(int i=0;i<600&&manager.isActive(earlyMoved);++i)Sleep(10);
  check(!manager.isActive(earlyMoved)&&str(earlyMoved->data,"Status")=="Complete"&&!fs::exists(earlyOriginal)&&fileHash(earlyMoved->target())==fileHash(media->target()),"Early destination change publishes identical verified media only at the chosen location");
  auto fileIdentity=[](const fs::path& path){
   Handle file(CreateFileW(path.c_str(),FILE_READ_ATTRIBUTES,FILE_SHARE_READ|FILE_SHARE_WRITE|FILE_SHARE_DELETE,nullptr,OPEN_EXISTING,0,nullptr));BY_HANDLE_FILE_INFORMATION info{};
   if(!file||!GetFileInformationByHandle(file.h,&info))throw std::runtime_error("Cannot read staging file identity.");
   return std::to_string(info.dwVolumeSerialNumber)+":"+std::to_string(info.nFileIndexHigh)+":"+std::to_string(info.nFileIndexLow);
  };
  auto reuse=makeMedia("prefetch-reuse.mp4",720);manager.beginPrefetch(reuse);bool ready=false;
  for(int i=0;i<600;++i){Sleep(10);Lock lock(manager.mutex);if(str(reuse->data,"MediaPhase")=="Waiting for confirmation"){ready=true;break;}}
  check(ready,"Verified media reaches the direct confirmation boundary");
  auto stage=reuse->target().parent_path()/(L".udm-"+wide(reuse->id())+L".muxing.mp4");

  const auto stageHash=fileHash(stage);const auto stageIdentity=fileIdentity(stage);
  rejects([&]{manager.confirmPrefetchedMedia(reuse,{{"FileName",str(media->data,"FileName")}});},"Confirmation rejects another download's destination");
  check(yes(reuse->data,"ConfirmationPending")&&fs::exists(stage)&&!fs::exists(reuse->target()),"Rejected confirmation preserves staged output and the confirmation gate");
  rejects([&]{manager.confirmPrefetchedMedia(reuse,{{"ExpectedSha256",std::string(64,'0')}});},"Confirmation cannot change verified transfer properties");
  rejects([&]{manager.confirmPrefetchedMedia(reuse,{{"Folder","relative-folder"}});},"Confirmation rejects a relative destination folder");
  {Handle held(CreateFileW((manager.root/L"state.json").c_str(),GENERIC_READ,FILE_SHARE_READ,nullptr,OPEN_EXISTING,0,nullptr));
   check(held.h!=INVALID_HANDLE_VALUE,"Catalog lock fixture opened");
   rejects([&]{manager.confirmPrefetchedMedia(reuse,{{"FileName","prefetch-save-failed.mp4"}});},"Failed confirmation save does not release publication");
   check(yes(reuse->data,"ConfirmationPending")&&str(reuse->data,"FileName")=="prefetch-reuse.mp4"&&fs::exists(stage),"Failed save rolls back metadata and retains the verified staging file");
  }
  check(manager.confirmPrefetchedMedia(reuse,{{"Folder",utf8((root/L"other-folder").wstring())},{"FileName","prefetch-reuse-renamed.mp4"}}),"Start accepts verified media without stopping its worker");
  for(int i=0;i<400&&manager.isActive(reuse);++i)Sleep(10);
  check(!manager.isActive(reuse)&&str(reuse->data,"Status")=="Complete"&&yes(reuse->data,"ReusedPrefetchedOutput"),"Direct confirmation completes the existing worker");
  check(fileHash(reuse->target())==stageHash&&fileIdentity(reuse->target())==stageIdentity&&!fs::exists(stage),"Changed-folder output retains the verified staging bytes and file identity without remuxing");
  auto audioReuse=makeMedia("audio-prefetch-reuse.m4a",0);audioReuse->video.reset();audioReuse->data.erase("Video");audioReuse->data["MediaOutput"]="audio";manager.beginPrefetch(audioReuse);
  for(int i=0;i<600;++i){Sleep(10);Lock lock(manager.mutex);if(str(audioReuse->data,"MediaPhase")=="Waiting for confirmation")break;}
  const auto audioStage=audioReuse->target().parent_path()/(L".udm-"+wide(audioReuse->id())+L".muxing.m4a");
  const auto audioStageIdentity=fileIdentity(audioStage);const auto measuredAudio=(i64)fs::file_size(audioStage);
  check(manager.confirmPrefetchedMedia(audioReuse,Json::object()),"Audio-only Start accepts its verified staging file");
  for(int i=0;i<400&&manager.isActive(audioReuse);++i)Sleep(10);
  check(str(audioReuse->data,"Status")=="Complete"&&(i64)fs::file_size(audioReuse->target())==measuredAudio&&fileIdentity(audioReuse->target())==audioStageIdentity,"M4A confirmation publishes the measured file without another assembly");
  auto recoverable=makeMedia("media-publication-save-failure.mp4",720);manager.beginPrefetch(recoverable);
  for(int i=0;i<600;++i){Sleep(10);Lock lock(manager.mutex);if(str(recoverable->data,"MediaPhase")=="Waiting for confirmation")break;}
  Handle catalogLock;
  {Lock lock(manager.mutex);
   check(manager.confirmPrefetchedMedia(recoverable,{{"Folder",utf8((root/L"recovery-folder").wstring())}}),"Recovery fixture confirms verified media in a changed folder");
   catalogLock.h=CreateFileW((manager.root/L"state.json").c_str(),GENERIC_READ,FILE_SHARE_READ,nullptr,OPEN_EXISTING,0,nullptr);
   if(!catalogLock)throw std::runtime_error("Cannot lock catalog for publication failure test.");
  }
  for(int i=0;i<400&&manager.isActive(recoverable);++i)Sleep(10);
  const auto publicationJournal=manager.root/L"publications"/(wide(recoverable->id())+L".json");
  check(!manager.isActive(recoverable)&&fs::exists(recoverable->target())&&fs::exists(publicationJournal),"Media publication preserves a recovery journal when saving completion fails");
  check(fs::exists(recoverable->video->target())&&fs::exists(recoverable->audio->target()),"Failed completion save retains source tracks");
  CloseHandle(catalogLock.h);catalogLock.h=INVALID_HANDLE_VALUE;
  manager.recoverReplacements();
  check(str(recoverable->data,"Status")=="Complete"&&fileHash(recoverable->target())==str(recoverable->data,"Sha256")&&!fs::exists(publicationJournal),"Recovery reconciles the published media bytes with completed history");
  if(const auto crossRoot=std::getenv("UDM_MEDIA_CROSS_VOLUME_TEST_ROOT")){
   const auto crossFolder=fs::absolute(fs::path(wide(crossRoot)))/wide(guid());
   auto cross=makeMedia("cross-volume-prefetch.mp4",720);manager.beginPrefetch(cross);
   for(int i=0;i<600;++i){Sleep(10);Lock lock(manager.mutex);if(str(cross->data,"MediaPhase")=="Waiting for confirmation")break;}
   const auto crossStage=cross->target().parent_path()/(L".udm-"+wide(cross->id())+L".muxing.mp4");
   const auto crossHash=fileHash(crossStage);const auto crossSize=fs::file_size(crossStage);const auto oldIdentity=fileIdentity(crossStage);
   check(manager.confirmPrefetchedMedia(cross,{{"Folder",utf8(crossFolder.wstring())}}),"Verified prefetch accepts a different destination volume");
   for(int i=0;i<600&&manager.isActive(cross);++i)Sleep(10);
   const auto newIdentity=fileIdentity(cross->target());
   check(oldIdentity.substr(0,oldIdentity.find(':'))!=newIdentity.substr(0,newIdentity.find(':')),"Cross-volume fixture uses distinct Windows volumes");
   check(str(cross->data,"Status")=="Complete"&&fileHash(cross->target())==crossHash&&fs::file_size(cross->target())==crossSize&&!fs::exists(crossStage),"Cross-volume confirmation publishes the exact measured bytes without reassembly");
   auto rejected=manager.add(source,utf8(crossFolder.wstring()),"copy-rejected.mp4");
   auto retained=root/L"copy-verification-source.bin";atomicText(retained,"verified-source",false);
   rejects([&]{publishMediaOutput(manager,rejected,retained,std::string(64,'0'),std::make_shared<Cancel>());},"Cross-volume copy rejects a mismatched verification hash");
   bool temporary=false;for(const auto& entry:fs::directory_iterator(crossFolder))temporary|=entry.path().extension()==L".publishing";
   check(fs::exists(retained)&&!fs::exists(rejected->target())&&!temporary,"Failed copy verification retains source and removes unpublished destination staging");
   auto cancelled=std::make_shared<Cancel>();cancelled->stop=true;
   rejects([&]{publishMediaOutput(manager,rejected,retained,fileHash(retained),cancelled);},"Cancelled media publication cannot create final output");
   check(fs::exists(retained)&&!fs::exists(rejected->target()),"Cancelled publication retains source without a final file");
  }
  auto disabled=makeMedia("prefetch-disabled.mp4",720);manager.state["Settings"]["PrefetchFileInfo"]=false;manager.beginPrefetch(disabled);
  check(!manager.isActive(disabled)&&!yes(disabled->data,"ConfirmationPending")&&!fs::exists(disabled->target()),"Disabled prefetch applies to captured video too");manager.state["Settings"]["PrefetchFileInfo"]=true;
  auto mismatched=makeMedia("wrong-quality.mp4",1080);rejects([&]{mediaTransfer(manager,mismatched,std::make_shared<Cancel>());},"Wrong media height blocks publication");
  check(!fs::exists(mismatched->target())&&fs::exists(mismatched->video->target())&&fs::exists(mismatched->audio->target()),"Rejected quality preserves source tracks");
 }else check(false,"FFmpeg and FFprobe required for media verification");
}
