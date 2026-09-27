#pragma once
#include "WebVtt.hpp"
static void adaptiveChecks(Manager& manager,const fs::path& root){
 Cancel cancel;auto input=root/L"adaptive-source.mp4";
 execute(appDir()/L"tools"/L"ffmpeg.exe",{L"-hide_banner",L"-loglevel",L"error",L"-nostdin",L"-y",L"-i",(root/L"generated-video.mp4").wstring(),L"-i",(root/L"generated-audio.mp4").wstring(),L"-c",L"copy",input.wstring()},30,cancel);
 Fixture server;server.payload=readText(input);i64 split=(i64)server.payload.size()/2;
 Json plan={{"type","hls"},{"height",720},{"audioExpected",true},{"tracks",Json::array({{{"kind","video"},{"segments",Json::array({{{"url",server.url("/range")},{"start",0},{"length",split}},{{"url",server.url("/range")},{"start",split},{"length",(i64)server.payload.size()-split}}})}}})}};
 auto receive=[&](const char* name,Json p){return manager.receive({{"action","adaptive"},{"url",server.url("/player")},{"filename",name},{"plan",p},{"originCookies",Json::object()}});};
 auto job=receive("generic-hls-720p",plan);check(str(job->data,"Status")=="Awaiting confirmation","Cross-site adaptive handoff preserves File Info confirmation");
 check(manager.receive({{"action","adaptive"},{"url",server.url("/player")},{"filename","generic-hls-720p"},{"plan",plan}})->id()==job->id(),"Adaptive duplicate handoff is idempotent");
 auto protectedPlan=str(job->data,"ProtectedAdaptive");check(protectedPlan.find("127.0.0.1")==std::string::npos&&Json::parse(reveal(protectedPlan))==plan,"Adaptive playback URLs encrypted in persisted plan");
 auto priorTemp=str(manager.state["Settings"],"TemporaryFolder");manager.state["Settings"]["TemporaryFolder"]=utf8((root/L"adaptive-temp").wstring());
 adaptiveTransfer(manager,job,std::make_shared<Cancel>());check(mediaWorkingDirectory(manager,job)==root/L"adaptive-temp"/L"UDM-media"/wide(job->id()),"HLS/DASH assembly honors the selected temporary folder");manager.state["Settings"]["TemporaryFolder"]=priorTemp;check(str(job->data,"Status")=="Complete"&&fileHash(job->target())==str(job->data,"Sha256"),"Generic HLS range pieces assemble and publish verified video");
 auto probe=Json::parse(execute(appDir()/L"tools"/L"ffprobe.exe",{L"-v",L"error",L"-show_entries",L"stream=codec_type,height",L"-of",L"json",job->target().wstring()},20,cancel));bool video=false,audio=false;for(auto s:probe["streams"]){video|=str(s,"codec_type")=="video"&&num(s,"height")==720;audio|=str(s,"codec_type")=="audio";}check(video&&audio,"Generic adaptive output contains requested video and audio");
 auto tsPlan=plan;tsPlan["container"]="ts";auto tsJob=receive("generic-720p.ts",tsPlan);adaptiveTransfer(manager,tsJob,std::make_shared<Cancel>());
 auto tsProbe=Json::parse(execute(appDir()/L"tools"/L"ffprobe.exe",{L"-v",L"error",L"-show_entries",L"format=format_name:stream=codec_type,height",L"-of",L"json",tsJob->target().wstring()},20,cancel));
 bool tsVideo=false,tsAudio=false;for(auto stream:tsProbe["streams"]){tsVideo|=str(stream,"codec_type")=="video"&&num(stream,"height")==720;tsAudio|=str(stream,"codec_type")=="audio";}
 check(tsJob->target().extension()==L".ts"&&str(tsProbe["format"],"format_name")=="mpegts"&&tsVideo&&tsAudio,"TS selection creates actual MPEG-TS with requested video and audio");
 execute(appDir()/L"tools"/L"ffmpeg.exe",{L"-v",L"error",L"-i",tsJob->target().wstring(),L"-f",L"null",L"-"},30,cancel);check(true,"TS output fully decodes without media errors");
 auto badContainer=plan;badContainer["container"]="exe";rejects([&]{validateAdaptive(badContainer);},"Unsupported adaptive output container rejected");
 auto invalid=plan;invalid["tracks"][0]["segments"][0]["url"]="file:///C:/Windows/win.ini";rejects([&]{validateAdaptive(invalid);},"Adaptive local-file URL rejected");
 invalid=plan;invalid["tracks"][0]["segments"][0]["start"]=-1;rejects([&]{validateAdaptive(invalid);},"Adaptive invalid byte range rejected");
 invalid=plan;invalid["encrypted"]=true;rejects([&]{validateAdaptive(invalid);},"Adaptive encrypted plan rejected");
 auto wrong=plan;wrong["height"]=1080;auto mismatched=receive("adaptive-wrong-height",wrong);rejects([&]{adaptiveTransfer(manager,mismatched,std::make_shared<Cancel>());},"Adaptive quality mismatch blocks publication");check(!fs::exists(mismatched->target()),"Adaptive incorrect quality output absent");
 auto bad=plan;bad["tracks"][0]["segments"][0]["url"]=server.url("/bad-range");auto badJob=receive("adaptive-bad-range",bad);rejects([&]{adaptiveTransfer(manager,badJob,std::make_shared<Cancel>());},"Adaptive incorrect server Content-Range rejected");
 auto resumedPlan=plan;resumedPlan["type"]="dash";auto resumed=receive("adaptive-resumed",resumedPlan);auto folder=manager.root/L"media"/wide(resumed->id())/L"adaptive";fs::create_directories(folder);writeBytes(folder/L"0.part",Bytes(server.payload.begin(),server.payload.begin()+(size_t)split));atomicText(folder/L"completed.json",Json{{"0",{{"size",split},{"sha256",fileHash(folder/L"0.part")}}}}.dump());int before=server.requests;adaptiveTransfer(manager,resumed,std::make_shared<Cancel>());check(server.requests-before==1&&str(resumed->data,"Status")=="Complete","Adaptive resume verifies and reuses completed segment");
 auto collision=receive("adaptive-collision",plan);writeBytes(collision->target(),Bytes{'k','e','e','p'});rejects([&]{adaptiveTransfer(manager,collision,std::make_shared<Cancel>());},"Adaptive existing output is not overwritten");check(readText(collision->target())=="keep","Adaptive existing file preserved");
 auto hashPlan=plan;hashPlan["type"]="dash";auto hashJob=receive("adaptive-hash-mismatch",hashPlan);manager.configure(hashJob,{{"ExpectedSha256",std::string(64,'0')}});rejects([&]{manager.configure(hashJob,{{"Url",server.url("/different")}});},"Adaptive source URL cannot change independently of captured plan");rejects([&]{adaptiveTransfer(manager,hashJob,std::make_shared<Cancel>());},"Adaptive expected hash mismatch blocks publication");check(!fs::exists(hashJob->target()),"Adaptive hash mismatch leaves no final output");
 auto described=plan;described["audioName"]="Español";described["audioLanguage"]="es-MX";auto selected=receive("adaptive-audio-language",described);
 check(str(selected->data,"FormatDescription").find("Español (es-MX)")!=std::string::npos,"Selected audio name and language appear in download properties");
 adaptiveTransfer(manager,selected,std::make_shared<Cancel>());
 auto languageProbe=Json::parse(execute(appDir()/L"tools"/L"ffprobe.exe",{L"-v",L"error",L"-select_streams",L"a:0",L"-show_entries",L"stream_tags=language,handler_name",L"-of",L"json",selected->target().wstring()},20,cancel));
 check(str(languageProbe["streams"][0]["tags"],"language")=="spa","Selected regional audio language is stored as an MP4 ISO language tag");
 check(str(languageProbe["streams"][0]["tags"],"handler_name")=="Español","Selected Unicode audio name survives MP4 assembly");
 for(const auto& invalid:std::vector<Json>{Json{{"audioName",17}},Json{{"audioLanguage",17}},Json{{"audioName","bad\nname"}},Json{{"audioLanguage","es;command"}},Json{{"audioName",std::string(513,'x')}},Json{{"audioLanguage",std::string(64,'a')}}}){auto p=plan;for(auto it=invalid.begin();it!=invalid.end();++it)p[it.key()]=it.value();rejects([&]{validateAdaptive(p);},"Invalid or oversized selected audio metadata is rejected");}
 auto noAudio=described;noAudio["audioExpected"]=false;rejects([&]{validateAdaptive(noAudio);},"Named audio selection cannot silently permit missing audio");

 auto audioPlan=described;audioPlan["audioOnly"]=true;audioPlan["container"]="m4a";audioPlan["height"]=0;audioPlan["tracks"][0]["kind"]="audio";
 auto music=receive("adaptive-audio-only",audioPlan);adaptiveTransfer(manager,music,std::make_shared<Cancel>());
 Fixture silent;silent.payload=readText(root/L"generated-video.mp4");auto silentPlan=audioPlan;silentPlan["tracks"][0]["segments"]=Json::array({{{"url",silent.url("/range")}}});auto silentJob=receive("adaptive-audio-missing",silentPlan);rejects([&]{adaptiveTransfer(manager,silentJob,std::make_shared<Cancel>());},"Video-only sources cannot produce a misleading audio-only file");check(!fs::exists(silentJob->target()),"A missing audio track leaves no final M4A output");
 auto musicProbe=Json::parse(execute(appDir()/L"tools"/L"ffprobe.exe",{L"-v",L"error",L"-show_entries",L"stream=codec_type:stream_tags=language",L"-of",L"json",music->target().wstring()},20,cancel));
 check(music->target().extension()==L".m4a"&&musicProbe["streams"].size()==1&&str(musicProbe["streams"][0],"codec_type")=="audio"&&str(musicProbe["streams"][0]["tags"],"language")=="spa","Audio-only output has one audio stream, M4A extension and the selected language");
 execute(appDir()/L"tools"/L"ffmpeg.exe",{L"-v",L"error",L"-i",music->target().wstring(),L"-f",L"null",L"-"},30,cancel);check(true,"Audio-only M4A fully decodes");
 for(const Json& change:std::vector<Json>{Json{{"audioOnly","true"}},Json{{"container","mp4"}},Json{{"height",720}},Json{{"audioExpected",false}}}){auto invalid=audioPlan;for(auto it=change.begin();it!=change.end();++it)invalid[it.key()]=it.value();rejects([&]{validateAdaptive(invalid);},"Invalid audio-only output combination is rejected");}
 Fixture subtitleServer;subtitleServer.payload="WEBVTT\n\n00:00:00.500 --> 00:00:02.000\nHola, mundo\n\n00:00:02.000 --> 00:00:04.000\nSubtítulos españoles\n\n";
 auto subtitlePlan=plan;subtitlePlan["subtitleName"]="Español";subtitlePlan["subtitleLanguage"]="es";subtitlePlan["tracks"].push_back({{"kind","subtitle"},{"segments",Json::array({{{"url",subtitleServer.url("/range")}}})}});
 auto captioned=receive("adaptive-subtitled",subtitlePlan);adaptiveTransfer(manager,captioned,std::make_shared<Cancel>());
 auto subtitleProbe=Json::parse(execute(appDir()/L"tools"/L"ffprobe.exe",{L"-v",L"error",L"-select_streams",L"s",L"-show_entries",L"stream=codec_name:stream_tags=language,handler_name",L"-of",L"json",captioned->target().wstring()},20,cancel));
 check(subtitleProbe["streams"].size()==1&&str(subtitleProbe["streams"][0],"codec_name")=="mov_text"&&str(subtitleProbe["streams"][0]["tags"],"language")=="spa","Selected WebVTT subtitles are embedded as a language-tagged MP4 text track");
 auto subtitleText=execute(appDir()/L"tools"/L"ffmpeg.exe",{L"-v",L"error",L"-i",captioned->target().wstring(),L"-map",L"0:s:0",L"-f",L"webvtt",L"pipe:1"},20,cancel);
 check(subtitleText.find("Hola, mundo")!=std::string::npos&&subtitleText.find("Subtítulos españoles")!=std::string::npos&&subtitleText.find("00:00.500 --> 00:02.000")!=std::string::npos,"Subtitles retain Unicode text and cue timing through MP4 muxing");
 check(str(captioned->data,"FormatDescription").find("subtitles: Español (es)")!=std::string::npos,"Selected subtitles appear in download properties");
 for(const Json& change:std::vector<Json>{Json{{"container","ts"}},Json{{"subtitleLanguage","es;invalid"}},Json{{"subtitleName",23}}}){auto invalid=subtitlePlan;for(auto it=change.begin();it!=change.end();++it)invalid[it.key()]=it.value();rejects([&]{validateAdaptive(invalid);},"Invalid subtitle plan is rejected");}
 auto invalidSubtitle=subtitlePlan;invalidSubtitle["tracks"][1]["segments"][0]["timeline"]=-1;rejects([&]{validateAdaptive(invalidSubtitle);},"Negative subtitle timeline is rejected");
 invalidSubtitle=subtitlePlan;invalidSubtitle["tracks"].push_back(invalidSubtitle["tracks"][1]);rejects([&]{validateAdaptive(invalidSubtitle);},"Duplicate subtitle tracks are rejected");
 std::vector<SubtitleCue> cues;
 appendWebVtt(cues,"WEBVTT\nX-TIMESTAMP-MAP=LOCAL:00:00:00.000,MPEGTS:900000\n\n00:00:01.000 --> 00:00:03.000\nMapped\n\n",10,0,true);
 check(cues.size()==1&&cues[0].start==1000&&cues[0].end==3000,"WebVTT MPEGTS timestamp maps are normalized to the media clock");
 appendWebVtt(cues,"WEBVTT\n\n00:00:01.000 --> 00:00:03.000\nMapped\n\n",0,2,true);auto merged=mergedWebVtt(cues);
 check(merged.find("Mapped")==merged.rfind("Mapped"),"Duplicate boundary cues are emitted once across subtitle segments");
 cues.clear();appendWebVtt(cues,"WEBVTT\nX-TIMESTAMP-MAP=MPEGTS:45000,LOCAL:00:00:00.000\n\n00:00:00.000 --> 00:00:01.000\nWrapped\n\n",((1LL<<33)-90000)/90000.0,2,true);
 check(cues.size()==1&&cues[0].start==1500&&cues[0].end==2500,"WebVTT 33-bit MPEGTS wrap preserves synchronization");
 for(const std::string& invalid:std::vector<std::string>{"<html>not subtitles</html>","WEBVTT\n\n00:60:00.000 --> 01:00:01.000\nInvalid\n\n","WEBVTT\n\n00:00:02.000 --> 00:00:01.000\nInvalid\n\n","WEBVTT\nX-TIMESTAMP-MAP=LOCAL:00:00:00.000,MPEGTS:8589934592\n\n",std::string(2*1024*1024+1,'x'),"WEBVTT\n\n00:00:00.000 --> 00:00:01.000\n\xff\n\n"})rejects([&]{std::vector<SubtitleCue> items;appendWebVtt(items,invalid,0,0,true);},"Malformed, oversized or invalid UTF-8 subtitle input is rejected");
 subtitleServer.payload="WEBVTT\n\n00:00:05.000 --> 00:00:01.000\nBad clock\n\n";auto badSubtitles=receive("adaptive-invalid-subtitles",subtitlePlan);rejects([&]{adaptiveTransfer(manager,badSubtitles,std::make_shared<Cancel>());},"Malformed subtitle input prevents publication");check(!fs::exists(badSubtitles->target()),"Subtitle failure leaves no misleading completed video");

}
