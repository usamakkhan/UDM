#pragma once
#include "HlsRecording.hpp"
#include "WebVtt.hpp"
namespace udm {
inline fs::path liveSubtitleFile(HlsRecording& recording,size_t track,const fs::path& folder,const fs::path& tools,const Cancel& cancel){
 struct Epoch {double start=0,duration=0,clock=0;};std::map<i64,Epoch> epochs;double elapsed=0;
 for(const auto& part:recording.ready(0)){auto found=epochs.find(part.discontinuity);if(found==epochs.end())found=epochs.emplace(part.discontinuity,Epoch{elapsed,0,0}).first;found->second.duration+=part.duration;elapsed+=part.duration;}
 if(epochs.empty()||elapsed>7*86400)throw std::runtime_error("Cannot determine the live subtitle recording timeline.");
 for(auto& entry:epochs){cancel.check();auto playlist=recording.localPlaylist(0,entry.first);
  auto probe=Json::parse(execute(tools/L"ffprobe.exe",{L"-v",L"error",L"-protocol_whitelist",L"file",L"-allowed_extensions",L"ALL",L"-allowed_segment_extensions",L"ALL",L"-extension_picky",L"0",L"-select_streams",L"v:0",L"-show_entries",L"stream=start_time",L"-of",L"json",playlist.wstring()},30,cancel));
  if(!probe.contains("streams")||probe["streams"].empty())throw std::runtime_error("Missing live video clock for subtitles.");auto value=str(probe["streams"][0],"start_time");size_t used=0;try{entry.second.clock=std::stod(value,&used);}catch(...){throw std::runtime_error("Invalid live video clock for subtitles.");}
  if(used!=value.size()||!std::isfinite(entry.second.clock)||std::abs(entry.second.clock)>7*86400)throw std::runtime_error("Invalid live video clock for subtitles.");
 }
 // Validate retained bytes before interpreting them, including after a restart.
 recording.localPlaylist(track);std::vector<SubtitleCue> cues;size_t bytes=0;std::map<i64,double> timelines;
 for(const auto& part:recording.ready(track)){cancel.check();if(part.initialization)throw std::runtime_error("Live subtitles require self-contained WebVTT segments.");auto text=readText(recording.mediaPath(track,part.sequence),2*1024*1024);bytes+=text.size();if(bytes>16*1024*1024)throw std::runtime_error("Live subtitle input exceeds 16 MB.");
  auto epoch=epochs.find(part.discontinuity);if(epoch==epochs.end()){if(part.discontinuity<epochs.begin()->first||part.discontinuity>epochs.rbegin()->first)continue;throw std::runtime_error("Subtitle discontinuity does not match the recorded video.");}
  auto& timeline=timelines[part.discontinuity];std::vector<SubtitleCue> segment;appendWebVtt(segment,text,epoch->second.clock,timeline,true);timeline+=part.duration;
  const auto offset=(i64)std::llround(epoch->second.start*1000),end=(i64)std::llround((epoch->second.start+epoch->second.duration)*1000);
  for(auto& cue:segment){cue.start+=offset;cue.end=std::min(end,cue.end+offset);if(cue.end<=cue.start)continue;if(cues.size()>=100000)throw std::runtime_error("Live subtitle cue limit exceeded.");cues.push_back(std::move(cue));}
 }
 if(cues.empty())throw std::runtime_error("The selected live subtitles contain no usable cues in the recorded video.");auto path=folder/L"subtitles.vtt";atomicText(path,mergedWebVtt(std::move(cues)));return path;
}
}
