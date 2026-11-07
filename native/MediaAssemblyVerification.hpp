#pragma once
#include "Core.hpp"
#include <algorithm>
#include <cmath>

namespace udm {
inline Json mediaAssemblyProbe(const fs::path& file,const Cancel& cancel){
 return Json::parse(execute(appDir()/L"tools"/L"ffprobe.exe",{L"-v",L"error",L"-protocol_whitelist",L"file,pipe",L"-show_entries",L"format=format_name:format_tags=major_brand:stream=codec_type,codec_name,width,height,sample_rate,channels,duration",L"-of",L"json",file.wstring()},20,cancel));
}
inline bool canPreserveMuxedMp4(const Json& probe){
 const auto format=probe.value("format",Json::object());
 if(str(format,"format_name")!="mov,mp4,m4a,3gp,3g2,mj2")return false;
 const auto brand=str(format.value("tags",Json::object()),"major_brand");
 if(brand!="isom"&&brand!="mp41"&&brand!="mp42")return false;
 const auto tracks=probe.value("streams",Json::array());
 if(!tracks.is_array()||tracks.size()!=2)return false;
 return std::count_if(tracks.begin(),tracks.end(),[](const Json& track){return str(track,"codec_type")=="video";})==1&&
        std::count_if(tracks.begin(),tracks.end(),[](const Json& track){return str(track,"codec_type")=="audio";})==1;
}
inline const Json& mediaAssemblyTrack(const Json& probe,const std::string& kind){
 if(!probe.contains("streams")||!probe["streams"].is_array())throw std::runtime_error("Media track information is unavailable. Saved streams are retained.");
 for(const auto& stream:probe["streams"])if(str(stream,"codec_type")==kind)return stream;
 throw std::runtime_error("Media assembly is missing a selected "+kind+" track. Saved streams are retained.");
}
inline std::optional<double> mediaAssemblyTrackDuration(const Json& stream){
 const auto text=str(stream,"duration");if(text.empty()||text=="N/A")return {};
 size_t used=0;double value=0;try{value=std::stod(text,&used);}catch(...){throw std::runtime_error("Invalid media track duration.");}
 if(used!=text.size()||!std::isfinite(value)||value<=0)throw std::runtime_error("Invalid media track duration.");return value;
}
inline void verifyMediaAssemblyTrack(const Json& source,const Json& output,const std::string& kind){
 const auto codec=str(source,"codec_name");if(codec.empty()||codec=="unknown"||str(output,"codec_name")!=codec)throw std::runtime_error("Media assembly changed the selected "+kind+" codec. Saved streams are retained.");
 if(kind=="video"){
  for(const auto* field:{"width","height"})if(num(source,field)<=0||num(source,field)!=num(output,field))throw std::runtime_error("Media assembly changed the selected video dimensions. Saved streams are retained.");
 }else if(num(source,"channels")<=0||num(source,"channels")!=num(output,"channels")||str(source,"sample_rate").empty()||str(source,"sample_rate")!=str(output,"sample_rate"))throw std::runtime_error("Media assembly changed the selected audio format. Saved streams are retained.");
 const auto before=mediaAssemblyTrackDuration(source),after=mediaAssemblyTrackDuration(output);
 // Compare per-track metadata, not the container maximum (which can hide a
 // truncated shorter track). This is a coarse guard, not packet/decode proof.
 if(before&&(!after||std::abs(*before-*after)>0.5))throw std::runtime_error("Media assembly duration differs from the selected "+kind+" track. Saved streams are retained.");
}
inline void verifyMediaAssembly(const fs::path& output,const fs::path& primary,const std::optional<fs::path>& separateAudio,bool audioOnly,const Json& selection,const Cancel& cancel,const Json* primaryProbe=nullptr){
 const auto actual=mediaAssemblyProbe(output,cancel);
 const auto source=primaryProbe?*primaryProbe:mediaAssemblyProbe(primary,cancel);
 if(!actual.contains("streams")||!actual["streams"].is_array()||actual["streams"].size()!=(audioOnly?1U:2U))throw std::runtime_error("Media assembly does not contain exactly the selected tracks. Saved streams are retained.");
 const auto& actualAudio=mediaAssemblyTrack(actual,"audio");
 if(audioOnly){
  if(str(actualAudio,"codec_name")!="aac")throw std::runtime_error("Audio assembly did not produce one AAC track. Output was not published.");
  verifyMediaAssemblyTrack(mediaAssemblyTrack(source,"audio"),actualAudio,"audio");return;
 }
 const auto& actualVideo=mediaAssemblyTrack(actual,"video");verifyMediaAssemblyTrack(mediaAssemblyTrack(source,"video"),actualVideo,"video");
 if(yes(selection,"ExactMediaQuality")&&num(selection,"MediaPixelHeight")>0&&num(actualVideo,"height")!=num(selection,"MediaPixelHeight"))throw std::runtime_error("Captured stream dimensions do not match the selected quality. Output was not published.");
 if(separateAudio){const auto audio=mediaAssemblyProbe(*separateAudio,cancel);verifyMediaAssemblyTrack(mediaAssemblyTrack(audio,"audio"),actualAudio,"audio");}
 else verifyMediaAssemblyTrack(mediaAssemblyTrack(source,"audio"),actualAudio,"audio");
 cancel.check();
}
}
