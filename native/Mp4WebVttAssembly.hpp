#pragma once
#include "Mp4WebVttFragments.hpp"
#include "AdaptiveTransferLayout.hpp"
#include "WebVtt.hpp"
#include <fstream>

namespace udm {
struct Mp4WebVttAssembly {std::vector<SubtitleCue> cues;std::string header;};
inline Bytes readMp4CaptionPart(const fs::path& path,size_t limit,const Cancel& cancel){
 const auto length=fs::file_size(path);if(!length||length>limit)throw std::runtime_error("Invalid MP4 caption part size.");
 std::ifstream input(path,std::ios::binary);if(!input)throw std::runtime_error("Cannot open MP4 caption part.");Bytes bytes(static_cast<size_t>(length));
 for(size_t at=0;at<bytes.size();){cancel.check();auto count=std::min<size_t>(65536,bytes.size()-at);input.read(reinterpret_cast<char*>(bytes.data()+at),static_cast<std::streamsize>(count));if(input.gcount()!=static_cast<std::streamsize>(count))throw std::runtime_error("Truncated MP4 caption part.");at+=count;}
 if(input.peek()!=std::char_traits<char>::eof())throw std::runtime_error("MP4 caption part changed while reading.");return bytes;
}
inline i64 mp4CaptionMilliseconds(int64_t ticks,uint32_t scale,i64 offset,i64 offsetScale,uint64_t movieStart=0,uint32_t movieScale=1){
 if(!scale||!movieScale||offset<0||offsetScale<=0||movieStart>uint64_t(movieScale)*7*86400)throw std::runtime_error("Invalid MP4 caption presentation clock.");
 // Subtract whole seconds before converting fractions, avoiding cancellation
 // between large floating-point media timestamps and presentation offsets.
 auto seconds=ticks/scale-offset/offsetScale;
 if(seconds < -14LL*86400-1||seconds>7LL*86400+1)throw std::runtime_error("MP4 caption time exceeds the presentation limit.");
 seconds+=static_cast<i64>(movieStart/movieScale);
 if(seconds < -7LL*86400-1||seconds>7LL*86400+1)throw std::runtime_error("MP4 caption time exceeds the presentation limit.");
 const double fraction=double(ticks%scale)/scale-double(offset%offsetScale)/offsetScale+double(movieStart%movieScale)/movieScale;
 return seconds*1000+static_cast<i64>(std::llround(fraction*1000));
}
inline Mp4WebVttAssembly assembleMp4WebVttTrack(const AdaptiveTransferLayout& layout,size_t trackIndex,const Cancel& cancel){
 const auto& description=layout.descriptions.at(trackIndex);const auto& indices=layout.tracks.at(trackIndex);
 if(!yes(description,"mp4WebVtt")||indices.size()<2)throw std::runtime_error("MP4 captions require initialization and media parts.");
 const auto initialization=readMp4CaptionPart(layout.parts.at(indices.front()).path,16*1024*1024,cancel);
 const auto track=readMp4WebVttInitialization(initialization,cancel);Mp4WebVttAssembly result;result.header=track.configuration;
 if(result.header.compare(0,3,"\xef\xbb\xbf")==0)result.header.erase(0,3);
 result.header=webVttLineEndings(result.header);
 const auto firstEnd=result.header.find('\n');const auto first=result.header.substr(0,firstEnd);
 if(!(first=="WEBVTT"||first.rfind("WEBVTT ",0)==0||first.rfind("WEBVTT\t",0)==0)||result.header.find("-->")!=std::string::npos||result.header.find("X-TIMESTAMP-MAP")!=std::string::npos)throw std::runtime_error("Invalid MP4 WebVTT configuration header.");
 while(!result.header.empty()&&result.header.back()=='\n')result.header.pop_back();result.header+="\n\n";
 const auto& clock=description.at("clock");const auto offset=num(clock,"presentationTimeOffset"),offsetScale=num(clock,"timescale");std::optional<uint64_t> continuation;size_t textBytes=0,totalSamples=0;
 for(size_t partIndex=1;partIndex<indices.size();++partIndex){
  const auto& part=layout.parts.at(indices[partIndex]);const auto bytes=readMp4CaptionPart(part.path,256*1024*1024,cancel);
  auto decoded=decodeMp4WebVttFragments(bytes,track,cancel,static_cast<uint64_t>(num(part.info,"start")),continuation);continuation=decoded.nextDecodeTime;totalSamples+=decoded.samples;
  if(totalSamples>100000)throw std::runtime_error("MP4 caption track exceeds its sample limit.");
  for(auto& item:decoded.cues){
   item.cue.text=webVttLineEndings(item.cue.text);
   if(item.cue.identifier.find("-->")!=std::string::npos||item.cue.settings.find("-->")!=std::string::npos||item.cue.text.find("-->")!=std::string::npos||item.cue.text.find("\n\n")!=std::string::npos||(!item.cue.text.empty()&&item.cue.text.front()=='\n'))throw std::runtime_error("MP4 caption cue cannot be represented as a WebVTT block.");
   if(item.cue.text.empty())continue;
   auto emit=[&](int64_t mediaStart,int64_t mediaEnd,uint64_t movieStart,uint64_t duration,uint32_t movieScale){
    cancel.check();
    auto start=mp4CaptionMilliseconds(mediaStart,track.timescale,offset,offsetScale,movieStart,movieScale),end=mp4CaptionMilliseconds(mediaEnd,track.timescale,offset,offsetScale,movieStart,movieScale);
    if(duration){const auto limit=mp4CaptionMilliseconds(static_cast<int64_t>(duration),movieScale,offset,offsetScale,movieStart,movieScale);end=std::min(end,limit);}
    start=std::max<i64>(0,start);if(end<=start)return;
    textBytes+=item.cue.text.size()+item.cue.identifier.size()+item.cue.settings.size();if(textBytes>16*1024*1024||result.cues.size()>=100000)throw std::runtime_error("MP4 caption output exceeds its limit.");
    result.cues.push_back({start,end,item.cue.text,item.cue.identifier,item.cue.settings});
   };
   if(track.edits.empty())emit(item.start,item.end,0,0,1);
   else for(const auto& edit:track.edits){
    if(edit.mediaStart<0||item.end<=edit.mediaStart)continue;
    auto start=std::max(item.start,edit.mediaStart)-edit.mediaStart,end=item.end-edit.mediaStart;
    if(edit.duration){
     // Round the edit extent up to a media tick only for clipping input.
     // The final output boundary is calculated independently in movie units.
     const auto whole=edit.duration/track.movieTimescale*track.timescale;
     const auto fraction=edit.duration%track.movieTimescale*track.timescale;
     const auto ticks=whole+fraction/track.movieTimescale+(fraction%track.movieTimescale!=0);
     if(static_cast<uint64_t>(start)>=ticks)continue;end=std::min(end,static_cast<int64_t>(ticks));
    }
    emit(start,end,edit.movieStart,edit.duration,track.movieTimescale);
   }
  }
 }
 // Repeated/reordered media edits can emit cues out of movie-time order.
 std::stable_sort(result.cues.begin(),result.cues.end(),[](const SubtitleCue& a,const SubtitleCue& b){return a.start<b.start;});
 return result;
}
}
