#pragma once
#include "Mp4WebVttAssembly.hpp"
#include "AdaptiveTransferLayout.hpp"
#include "AudioTrackMetadata.hpp"
#include "WebVtt.hpp"
#include <fstream>
#include <locale>

namespace udm {
inline std::wstring dashSeconds(double seconds){
 if(!std::isfinite(seconds))throw std::runtime_error("Invalid DASH assembly clock.");
 std::wostringstream value;value.imbue(std::locale::classic());value<<std::fixed<<std::setprecision(9)<<seconds;return value.str();
}
inline Json dashAssemblyProbe(const fs::path& file,const Cancel& cancel){
 return Json::parse(execute(appDir()/L"tools"/L"ffprobe.exe",{L"-v",L"error",L"-protocol_whitelist",L"file,pipe",L"-show_data_hash",L"sha256",L"-show_entries",L"stream=codec_type,codec_name,profile,codec_tag_string,width,height,pix_fmt,sample_rate,channels,channel_layout,time_base,extradata_hash:format=duration",L"-of",L"json",file.wstring()},30,cancel));
}
inline bool dashSinglePeriodEmbeddedAudio(const Json& selection){
 return !yes(selection,"audioOnly")&&!yes(selection,"audioExpected")&&selection.contains("periods")&&selection["periods"].is_array()&&selection["periods"].size()==1;
}
inline Json dashAssemblyStreams(const Json& probe,const Json& selection,bool captions){
 if(!probe.contains("streams")||!probe["streams"].is_array())throw std::runtime_error("DASH assembly produced no readable streams.");
 Json signature=Json::array();size_t videos=0,audios=0,subtitles=0;
 for(const auto& stream:probe["streams"]){
  auto type=str(stream,"codec_type");if(type=="video"){++videos;if(num(selection,"height")>0&&num(stream,"height")!=num(selection,"height"))throw std::runtime_error("A DASH Period changed the selected video dimensions.");}
  else if(type=="audio")++audios;else if(type=="subtitle")++subtitles;else throw std::runtime_error("Unexpected DASH output stream.");
  if(str(stream,"codec_name").empty()||str(stream,"time_base").empty())throw std::runtime_error("DASH output codec or clock is missing.");
  if(type!="subtitle"&&str(stream,"extradata_hash").empty())throw std::runtime_error("DASH codec configuration could not be verified.");
  signature.push_back(stream);
 }
 if(videos!=(yes(selection,"audioOnly")?0U:1U)||(dashSinglePeriodEmbeddedAudio(selection)?audios>1:audios!=(yes(selection,"audioExpected")?1U:0U))||subtitles!=(captions?1U:0U))throw std::runtime_error("DASH output does not contain exactly the selected tracks.");
 return signature;
}
inline void dashAssemblyDuration(const Json& probe,double expected){
 if(!probe.contains("format"))throw std::runtime_error("DASH output duration is missing.");
 auto text=str(probe["format"],"duration");size_t used=0;double duration=0;
 try{duration=std::stod(text,&used);}catch(...){throw std::runtime_error("DASH output duration is invalid.");}
 // Stream-copy boundaries are packet-granular. This is a coarse completeness
 // guard, not a decoded-frame or sample-accuracy proof.
 if(used!=text.size()||!std::isfinite(duration)||duration<=0||std::abs(duration-expected)>0.5)throw std::runtime_error("DASH output duration differs from its Period timeline.");
}

// Produces a local staging file only. Publication, exact-size confirmation and
// removal of downloaded parts remain owned by the existing transfer workflow.
inline std::vector<fs::path> assembleDashPeriods(const AdaptiveTransferLayout& layout,const fs::path& folder,const fs::path& staging,const Json& selection,const Cancel& cancel){
 if(!layout.multiplePeriods||layout.periods.empty()||layout.periodTracks.size()!=layout.periods.size())throw std::runtime_error("Invalid DASH assembly layout.");
 const bool audioOnly=yes(selection,"audioOnly");auto container=str(selection,"container","mp4");
 if(container!=(audioOnly?"m4a":"mp4"))throw std::runtime_error("DASH Period assembly requires MP4 or M4A output.");
 std::vector<fs::path> temporary;std::vector<SubtitleCue> captions;size_t captionBytes=0;bool hasCaptions=false;Json reference;std::optional<std::string> captionHeader;
 std::ostringstream list;list.imbue(std::locale::classic());list<<"ffconcat version 1.0\n";double elapsed=0;
 for(size_t p=0;p<layout.periods.size();++p){
  cancel.check();const auto& period=layout.periods[p];std::vector<fs::path> inputs;std::vector<std::string> kinds;
  std::vector<std::wstring> args={L"-hide_banner",L"-loglevel",L"error",L"-nostdin",L"-y",L"-copyts"};
  for(auto t:layout.periodTracks[p]){
   const auto& track=layout.descriptions.at(t);auto kind=str(track,"kind");const auto& clock=track["clock"];
   const double offset=(double)num(clock,"presentationTimeOffset")/num(clock,"timescale");
   if(kind=="subtitle"){
    hasCaptions=true;std::vector<SubtitleCue> cues;
    if(yes(track,"mp4WebVtt")){auto decoded=assembleMp4WebVttTrack(layout,t,cancel);if(captionHeader&&*captionHeader!=decoded.header)throw std::runtime_error("DASH Period caption configurations differ.");captionHeader=std::move(decoded.header);cues=std::move(decoded.cues);for(const auto& cue:cues)captionBytes+=cue.text.size()+cue.identifier.size()+cue.settings.size();if(captionBytes>16*1024*1024)throw std::runtime_error("DASH subtitles exceed 16 MB.");}
    else {WebVttConfiguration configuration;for(auto index:layout.tracks.at(t)){cancel.check();auto text=readText(layout.parts.at(index).path,2*1024*1024);captionBytes+=text.size();if(captionBytes>16*1024*1024)throw std::runtime_error("DASH subtitles exceed 16 MB.");appendWebVtt(cues,text,0,0,false,&configuration);}auto header=configuration.header();if(captionHeader&&*captionHeader!=header)throw std::runtime_error("DASH Period caption configurations differ.");captionHeader=std::move(header);}
    const i64 shift=yes(track,"mp4WebVtt")?0:(i64)std::llround(offset*1000),end=(i64)std::llround(period.duration*1000),base=(i64)std::llround(elapsed*1000);
    for(auto cue:cues){cue.start=std::max<i64>(0,cue.start-shift);cue.end=std::min(end,cue.end-shift);if(cue.end<=cue.start)continue;cue.start+=base;cue.end+=base;if(captions.size()>=100000)throw std::runtime_error("DASH subtitle cue limit exceeded.");captions.push_back(std::move(cue));}continue;
   }
   auto input=folder/(L"period-"+std::to_wstring(p)+L"-track-"+std::to_wstring(t)+L".bin");temporary.push_back(input);
   {std::ofstream out(input,std::ios::binary|std::ios::trunc);if(!out)throw std::runtime_error("Cannot create a DASH Period track.");
    for(auto index:layout.tracks.at(t)){std::ifstream part(layout.parts.at(index).path,std::ios::binary);if(!part)throw std::runtime_error("A DASH Period part is missing.");char bytes[65536];while(part){cancel.check();part.read(bytes,sizeof(bytes));out.write(bytes,part.gcount());}if(!part.eof())throw std::runtime_error("Cannot read a DASH Period part.");}out.flush();if(!out)throw std::runtime_error("Cannot assemble a DASH Period track.");}
   args.insert(args.end(),{L"-protocol_whitelist",L"file,pipe",L"-itsoffset",dashSeconds(-offset),L"-i",input.wstring()});inputs.push_back(input);kinds.push_back(kind);
  }
  for(size_t i=0;i<kinds.size();++i)args.insert(args.end(),{L"-map",std::to_wstring(i)+(kinds[i]=="video"?L":v:0":L":a:0")});
  if(dashSinglePeriodEmbeddedAudio(selection))for(size_t i=0;i<kinds.size();++i)if(kinds[i]=="video")args.insert(args.end(),{L"-map",std::to_wstring(i)+L":a:0?"});
  auto output=folder/(L"period-"+std::to_wstring(p)+L"."+wide(container));temporary.push_back(output);
  args.insert(args.end(),{L"-ss",L"0",L"-t",dashSeconds(period.duration),L"-c",L"copy",L"-avoid_negative_ts",L"disabled",L"-movflags",L"+faststart",output.wstring()});
  execute(appDir()/L"tools"/L"ffmpeg.exe",args,300,cancel);auto probe=dashAssemblyProbe(output,cancel);auto signature=dashAssemblyStreams(probe,selection,false);dashAssemblyDuration(probe,period.duration);
  if(p==0)reference=signature;else if(signature!=reference)throw std::runtime_error("DASH Period codec configurations or time bases differ; stream-copy joining is unavailable.");
  list<<"file '"<<utf8(output.filename().wstring())<<"'\nduration "<<std::fixed<<std::setprecision(9)<<period.duration<<"\n";elapsed+=period.duration;
 }
 auto manifest=folder/L"periods.ffconcat";temporary.push_back(manifest);atomicText(manifest,list.str());
 std::vector<std::wstring> args={L"-hide_banner",L"-loglevel",L"error",L"-nostdin",L"-y",L"-protocol_whitelist",L"file,pipe",L"-f",L"concat",L"-safe",L"1",L"-i",manifest.wstring()};
 if(hasCaptions){auto file=folder/L"periods.vtt";temporary.push_back(file);atomicText(file,mergedWebVtt(std::move(captions),captionHeader.value_or("WEBVTT\n\n")));args.insert(args.end(),{L"-protocol_whitelist",L"file,pipe",L"-i",file.wstring()});}
 args.insert(args.end(),{L"-map",L"0",L"-c",L"copy"});
 if(hasCaptions){args.insert(args.end(),{L"-map",L"1:s:0",L"-c:s",L"mov_text",L"-metadata:s:s:0",L"handler_name="+wide(str(selection,"subtitleName"))});auto language=audioLanguageTag(str(selection,"subtitleLanguage"));if(!language.empty())args.insert(args.end(),{L"-metadata:s:s:0",L"language="+wide(language)});}
 appendAudioMetadata(args,selection);args.insert(args.end(),{L"-movflags",L"+faststart",staging.wstring()});execute(appDir()/L"tools"/L"ffmpeg.exe",args,300,cancel);
 auto probe=dashAssemblyProbe(staging,cancel);dashAssemblyStreams(probe,selection,hasCaptions);dashAssemblyDuration(probe,elapsed);return temporary;
}
}
