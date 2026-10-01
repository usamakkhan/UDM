#pragma once
static void audioStreamingChecks(udm::Manager& manager,const udm::fs::path& root){
 using namespace udm;
 const std::string source="https://www.youtube.com/watch?v=abcdefghijk",endpoint="https://r1.googlevideo.com/videoplayback?sabr=1";
 const auto body=Proto().set(5,Bytes{1}).set(19,Proto().set(3,Bytes{2}).encode()).encode();
 Json offer={{"url",endpoint},{"body",b64(body)},{"videoId","abcdefghijk"},{"capturedAt",epoch()},{"durationMs",200000},
  {"audio",{{"id","140"},{"lastModified","67890"},{"xtags","lang=en"},{"mime","audio/mp4; codecs=\"mp4a.40.2\""}}}};
 validateSabr(offer,source,true);check(true,"Audio-only SABR accepts a complete current AAC identity without video");
 rejects([&]{validateSabr(offer,source);},"Ordinary video SABR still requires a video identity");
 auto invalid=offer;invalid["video"]={{"id","137"},{"lastModified","12345"},{"mime","video/mp4"}};
 rejects([&]{validateSabr(invalid,source,true);},"Audio-only offer cannot request an extra video track");
 invalid=offer;invalid["audio"]["mime"]="audio/mp4; codecs=\"opus\"";rejects([&]{validateSabr(invalid,source,true);},"Non-AAC streaming audio is rejected");
 invalid=offer;invalid["audio"].erase("lastModified");rejects([&]{validateSabr(invalid,source,true);},"Audio revision is required for streaming");
 invalid=offer;invalid["videoId"]="zyxwvutsrqp";rejects([&]{validateSabr(invalid,source,true);},"Audio offer cannot belong to a different video");
 invalid=offer;invalid["capturedAt"]=epoch()-300001;rejects([&]{validateSabr(invalid,source,true);},"Expired audio sessions are rejected");
 Json request={{"action","sabr"},{"output","audio"},{"url",source},{"height",0},{"pixelHeight",0},{"formatId","140"},{"filename","English.m4a"},{"sabr",offer}};
 auto first=manager.receive(request);
 check(!first->video&&first->audio&&str(first->data,"MediaOutput")=="audio"&&num(first->data,"MediaHeight")==0&&first->target().extension()==L".m4a","Bridge creates only an audio child and an M4A destination");
 check(manager.receive(request)==first,"Identical queued audio selection is deduplicated");
 auto language=request;language["sabr"]["audio"]["xtags"]="lang=es";
 auto second=manager.receive(language);check(second!=first,"Same-itag audio languages create different jobs");
 auto revision=request;revision["sabr"]["audio"]["lastModified"]="67891";
 check(manager.receive(revision)!=first,"Different audio revisions cannot reuse a queued stale job");
 auto count=manager.jobs.size();invalid=request;invalid["formatId"]="141";
 rejects([&]{manager.receive(invalid);},"Bridge rejects an inconsistent selected audio format");
 check(manager.jobs.size()==count,"Rejected audio handoff does not create a download");
 auto restored=std::make_shared<Job>(first->snapshot());
 check(!restored->video&&restored->audio&&str(restored->data,"MediaOutput")=="audio"&&reveal(str(restored->data,"ProtectedSabr"))==offer.dump(),"Saved audio-only jobs retain the encrypted session and child track");
 first->data["Status"]="Downloading";first->audio->data["Received"]=123;first->audio->data["TransferredBytes"]=234;
 manager.tick();check(num(first->data,"Received")==123&&num(first->data,"TransferredBytes")==234,"Audio-only parent progress aggregates the audio child");first->data["Status"]="Paused";
 auto links=downloadLinks(first->snapshot());bool audioLabel=false;for(const auto& row:links)audioLabel|=row.label=="Audio streaming endpoint (SABR)";
 check(audioLabel,"Audio-only properties identify the streaming endpoint as audio");
 const auto tools=appDir()/L"tools";Cancel cancel;auto input=root/L"stream-audio-source.mp4";
 execute(tools/L"ffmpeg.exe",{L"-hide_banner",L"-loglevel",L"error",L"-nostdin",L"-y",L"-f",L"lavfi",L"-i",L"sine=frequency=500:duration=2",L"-c:a",L"aac",L"-movflags",L"+frag_keyframe+empty_moov",input.wstring()},30,cancel);
 auto make=[&](const char* name){auto j=manager.add(source,"",name);j->data["SourceUrl"]=source;j->data["MediaOutput"]="audio";setStreams(manager,j,"",endpoint);fs::create_directories(j->audio->target().parent_path());fs::copy_file(input,j->audio->target());j->audio->data["Status"]="Complete";return j;};
 auto output=make("verified-stream-audio.m4a");mediaTransfer(manager,output,std::make_shared<Cancel>());
 auto info=Json::parse(execute(tools/L"ffprobe.exe",{L"-v",L"error",L"-show_entries",L"stream=codec_type,codec_name",L"-of",L"json",output->target().wstring()},20,cancel));
 check(str(output->data,"Status")=="Complete"&&info["streams"].size()==1&&str(info["streams"][0],"codec_name")=="aac"&&str(info["streams"][0],"codec_type")=="audio","Fragmented audio remuxes to exactly one AAC track and no video");
 check(str(output->data,"Sha256")==fileHash(output->target())&&!fs::exists(output->audio->target()),"Verified audio is hashed and temporary input is cleaned after publication");
 auto bad=make("invalid-stream-audio.m4a");{std::ofstream file(bad->audio->target(),std::ios::binary|std::ios::trunc);file<<"invalid media";}rejects([&]{mediaTransfer(manager,bad,std::make_shared<Cancel>());},"Invalid audio cannot be published as a completed file");
 check(!fs::exists(bad->target())&&fs::exists(bad->audio->target()),"Failed audio assembly preserves the input and leaves no completed file");
}
