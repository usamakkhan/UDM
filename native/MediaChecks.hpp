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
 auto stream=makeJob("stream-fixture.mp4");sabrTransfer(manager,stream,std::make_shared<Cancel>(),transport(transcript(false,false)));
 check(readText(stream->video->target())=="vi01vd01"&&readText(stream->audio->target())=="ai01ad01","SABR fragmented UMP transcript assembles both tracks");
 check(!fs::exists(stream->target()),"SABR transport does not publish unmerged output");
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
