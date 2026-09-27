#pragma once
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
  auto mismatched=makeMedia("wrong-quality.mp4",1080);rejects([&]{mediaTransfer(manager,mismatched,std::make_shared<Cancel>());},"Wrong media height blocks publication");
  check(!fs::exists(mismatched->target())&&fs::exists(mismatched->video->target())&&fs::exists(mismatched->audio->target()),"Rejected quality preserves source tracks");
 }else check(false,"FFmpeg and FFprobe required for media verification");
}
