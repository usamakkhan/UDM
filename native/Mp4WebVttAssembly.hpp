#pragma once
#include "Mp4WebVttFragments.hpp"
#include "AdaptiveTransferLayout.hpp"
#include "WebVtt.hpp"
#include <fstream>
#include <limits>

namespace udm {
struct Mp4WebVttAssembly {std::vector<SubtitleCue> cues;std::string header;};
inline Bytes readMp4CaptionPart(const fs::path& path,size_t limit,const Cancel& cancel){
 const auto length=fs::file_size(path);if(!length||length>limit)throw std::runtime_error("Invalid MP4 caption part size.");
 std::ifstream input(path,std::ios::binary);if(!input)throw std::runtime_error("Cannot open MP4 caption part.");Bytes bytes(static_cast<size_t>(length));
 for(size_t at=0;at<bytes.size();){cancel.check();auto count=std::min<size_t>(65536,bytes.size()-at);input.read(reinterpret_cast<char*>(bytes.data()+at),static_cast<std::streamsize>(count));if(input.gcount()!=static_cast<std::streamsize>(count))throw std::runtime_error("Truncated MP4 caption part.");at+=count;}
 if(input.peek()!=std::char_traits<char>::eof())throw std::runtime_error("MP4 caption part changed while reading.");return bytes;
}
inline i64 mp4CaptionMilliseconds(int64_t ticks,i64 scale,i64 offset,i64 offsetScale,uint64_t movieStart=0,uint32_t movieScale=1,unsigned scaleShift=0,i64 inlineDeltaMs=0,uint32_t inlineRate=65536){
 if(scale<=0||scale>(1LL<<52)||scaleShift>16||!movieScale||offset<0||offsetScale<=0||movieStart>uint64_t(movieScale)*7*86400||inlineDeltaMs < -7LL*86400*1000||inlineDeltaMs>7LL*86400*1000||!inlineRate||inlineRate>16U*65536)throw std::runtime_error("Invalid MP4 caption presentation clock.");
 const auto maximum=(std::numeric_limits<i64>::max)(),minimum=(std::numeric_limits<i64>::min)();
 const i64 multiplier=1LL<<scaleShift,quotient=ticks/scale;
 if(quotient>maximum/multiplier||quotient<minimum/multiplier)throw std::runtime_error("MP4 caption scaled clock overflows.");
 // Scale the remainder by bounded doubling instead of multiplying ticks by
 // 65536. Keep the quotient and remainder exact until millisecond rounding.
 const bool negative=ticks<0;uint64_t remainder=static_cast<uint64_t>(negative?-(ticks%scale):ticks%scale),carry=0;
 for(unsigned bit=0;bit<scaleShift;++bit){remainder*=2;carry=carry*2+remainder/static_cast<uint64_t>(scale);remainder%=static_cast<uint64_t>(scale);}
 auto seconds=quotient*multiplier;const auto extra=negative?-static_cast<i64>(carry):static_cast<i64>(carry);
 if((extra>0&&seconds>maximum-extra)||(extra<0&&seconds<minimum-extra))throw std::runtime_error("MP4 caption scaled clock overflows.");
 seconds+=extra;
 // Inline timestamps have a millisecond clock independent of the media
 // timescale. Keep their rational rate-scaled offset until the final rounding.
 const i64 deltaNumerator=inlineDeltaMs*65536,deltaDenominator=static_cast<i64>(inlineRate)*1000;
 const i64 deltaSeconds=deltaNumerator/deltaDenominator;
 auto addSeconds=[&](i64 value){if((value>0&&seconds>maximum-value)||(value<0&&seconds<minimum-value))throw std::runtime_error("MP4 caption clock addition overflows.");seconds+=value;};
 addSeconds(deltaSeconds);const auto offsetSeconds=offset/offsetScale;
 if(seconds<minimum+offsetSeconds)throw std::runtime_error("MP4 caption offset overflows.");
 // Subtract whole seconds before converting fractions, avoiding cancellation
 // between large floating-point media timestamps and presentation offsets.
 seconds-=offsetSeconds;
 addSeconds(static_cast<i64>(movieStart/movieScale));
 double fraction=(negative?-double(remainder):double(remainder))/scale+static_cast<double>(deltaNumerator%deltaDenominator)/static_cast<double>(deltaDenominator)-double(offset%offsetScale)/offsetScale+double(movieStart%movieScale)/movieScale;
 // Normalize only whole seconds, preserving the signed fractional rounding
 // convention while applying the presentation bound to the combined clock.
 const auto fractionCarry=static_cast<i64>(fraction);addSeconds(fractionCarry);fraction-=static_cast<double>(fractionCarry);
 if(seconds < -7LL*86400-1||seconds>7LL*86400+1)throw std::runtime_error("MP4 caption time exceeds the presentation limit.");
 return seconds*1000+static_cast<i64>(std::llround(fraction*1000));
}
struct Mp4WebVttPart {fs::path path;uint64_t resourceOffset=0;};
inline std::string mappedMp4CueText(const std::string& text,const std::function<i64(i64)>& map,i64 start,i64 end){
 static const std::regex timestamp("(?:(?:[0-9]{2,4}):)?[0-5][0-9]:[0-5][0-9]\\.[0-9]{3}");
 std::string output;size_t at=0;i64 previous=-1,mappedPrevious=start;
 while(at<text.size()){
  const auto open=text.find('<',at);if(open==std::string::npos){output.append(text,at,std::string::npos);break;}output.append(text,at,open-at);
  const auto close=text.find('>',open+1);if(close==std::string::npos){output.append(text,open,std::string::npos);break;}
  const auto token=text.substr(open+1,close-open-1);
  if(token.size()<=16&&std::regex_match(token,timestamp)){
   const auto raw=subtitleTime(token);if(raw<=previous)throw std::runtime_error("MP4 cue text timestamps must increase.");previous=raw;
   const auto mapped=map(raw);if(mapped>mappedPrevious&&mapped<end){output+='<'+subtitleStamp(mapped)+'>';mappedPrevious=mapped;}
  }else output.append(text,open,close-open+1);
  if(output.size()>65536)throw std::runtime_error("Mapped MP4 cue text exceeds its limit.");at=close+1;
 }if(output.size()>65536)throw std::runtime_error("Mapped MP4 cue text exceeds its limit.");return output;
}
// Retained recordings and ordinary adaptive transfers share this decoder and
// its bounded sample/edit-list clock mapping. Offsets refer to original bytes.
inline Mp4WebVttAssembly assembleMp4WebVttParts(const std::vector<Mp4WebVttPart>& parts,i64 offset,i64 offsetScale,const Cancel& cancel){
 if(parts.size()<2||parts.size()>10000)throw std::runtime_error("MP4 captions require bounded initialization and media parts.");
 if(offset<0||offset>9007199254740991LL||offsetScale<1||offsetScale>9007199254740991LL)throw std::runtime_error("Invalid MP4 caption presentation clock.");
 cancel.check();
 const auto initialization=readMp4CaptionPart(parts.front().path,16*1024*1024,cancel);
 const auto track=readMp4WebVttInitialization(initialization,cancel);Mp4WebVttAssembly result;result.header=track.configuration;
 if(result.header.compare(0,3,"\xef\xbb\xbf")==0)result.header.erase(0,3);
 result.header=webVttLineEndings(result.header);
 const auto firstEnd=result.header.find('\n');const auto first=result.header.substr(0,firstEnd);
 if(!(first=="WEBVTT"||first.rfind("WEBVTT ",0)==0||first.rfind("WEBVTT\t",0)==0)||result.header.find("-->")!=std::string::npos||result.header.find("X-TIMESTAMP-MAP")!=std::string::npos)throw std::runtime_error("Invalid MP4 WebVTT configuration header.");
 while(!result.header.empty()&&result.header.back()=='\n')result.header.pop_back();result.header+="\n\n";
 std::optional<uint64_t> continuation;size_t textBytes=0,totalSamples=0;
 for(size_t partIndex=1;partIndex<parts.size();++partIndex){
  const auto& part=parts[partIndex];const auto bytes=readMp4CaptionPart(part.path,256*1024*1024,cancel);
  if(part.resourceOffset>9007199254740991ULL)throw std::runtime_error("MP4 caption resource offset exceeds supported precision.");
  auto decoded=decodeMp4WebVttFragments(bytes,track,cancel,part.resourceOffset,continuation);continuation=decoded.nextDecodeTime;totalSamples+=decoded.samples;
  if(totalSamples>100000)throw std::runtime_error("MP4 caption track exceeds its sample limit.");
  for(auto& item:decoded.cues){
   item.cue.text=webVttLineEndings(item.cue.text);
   if(item.cue.identifier.find("-->")!=std::string::npos||item.cue.settings.find("-->")!=std::string::npos||item.cue.text.find("-->")!=std::string::npos||item.cue.text.find("\n\n")!=std::string::npos||(!item.cue.text.empty()&&item.cue.text.front()=='\n'))throw std::runtime_error("MP4 caption cue cannot be represented as a WebVTT block.");
   if(item.cue.text.empty())continue;
   auto emit=[&](int64_t mediaStart,int64_t mediaEnd,uint64_t movieStart,uint64_t duration,uint32_t movieScale,uint32_t rate=0x00010000U,i64 mediaOrigin=0,bool reversed=false){
    cancel.check();
    i64 start=0,end=0;
    if(!rate){
     // A dwell holds the selected media instant for the whole movie interval.
     // Its duration belongs to the movie clock, not the media clock.
     start=mp4CaptionMilliseconds(0,movieScale,offset,offsetScale,movieStart,movieScale);
     end=mp4CaptionMilliseconds(static_cast<int64_t>(duration),movieScale,offset,offsetScale,movieStart,movieScale);
    }else{
     const auto effectiveScale=i64(track.timescale)*rate;
     start=mp4CaptionMilliseconds(mediaStart,effectiveScale,offset,offsetScale,movieStart,movieScale,16);end=mp4CaptionMilliseconds(mediaEnd,effectiveScale,offset,offsetScale,movieStart,movieScale,16);
    }
    if(duration){const auto limit=mp4CaptionMilliseconds(static_cast<int64_t>(duration),movieScale,offset,offsetScale,movieStart,movieScale);end=std::min(end,limit);}
    start=std::max<i64>(0,start);if(end<=start)return;
    if(result.cues.size()>=100000)throw std::runtime_error("MP4 caption output exceeds its limit.");
    auto text=mappedMp4CueText(item.cue.text,[&](i64 raw){
     if(!item.cue.currentTimeMs)throw std::runtime_error("MP4 inline cue timestamps require ctim metadata.");
     if(!rate||reversed)throw std::runtime_error("Timed MP4 cue text with dwell/reverse edits is unsupported.");
     const auto delta=raw-*item.cue.currentTimeMs;
     // Subtract the edit origin before adding the inline offset: a large
     // absolute sample clock may be valid after cancellation with that origin.
     if(mediaOrigin<0||item.start<(std::numeric_limits<i64>::min)()+mediaOrigin)throw std::runtime_error("MP4 inline cue origin overflows.");
     return mp4CaptionMilliseconds(item.start-mediaOrigin,i64(track.timescale)*rate,offset,offsetScale,movieStart,movieScale,16,delta,rate);
    },start,end);
    textBytes+=text.size()+item.cue.identifier.size()+item.cue.settings.size();if(textBytes>16*1024*1024)throw std::runtime_error("MP4 caption output exceeds its limit.");
    result.cues.push_back({start,end,std::move(text),item.cue.identifier,item.cue.settings});
   };
   if(track.edits.empty())emit(item.start,item.end,0,0,1);
   else for(const auto& edit:track.edits){
    if(edit.mediaStart<0)continue;
    const auto rateMagnitude=static_cast<uint32_t>(edit.rateFixed<0?-edit.rateFixed:edit.rateFixed);
    int64_t start=0,end=0;
    if(edit.rateFixed<0){
     if(item.start>=edit.mediaStart)continue;
     auto reverseDistance=[&](int64_t mediaTime){
      if(mediaTime<0&&edit.mediaStart>(std::numeric_limits<int64_t>::max)()+mediaTime)throw std::runtime_error("MP4 reverse caption interval overflows.");
      return edit.mediaStart-mediaTime;
     };
     // Reversing a cue swaps its endpoints on the increasing movie clock.
     start=reverseDistance(std::min(item.end,edit.mediaStart));end=reverseDistance(item.start);
    }else{
     if(item.end<=edit.mediaStart)continue;
     if(!edit.rateFixed){
      // Cue intervals include their start and exclude their end. Do not select
      // a later cue merely because it follows the held media timestamp.
      if(item.start<=edit.mediaStart)emit(0,0,edit.movieStart,edit.duration,track.movieTimescale,0);
      continue;
     }
     start=std::max(item.start,edit.mediaStart)-edit.mediaStart;end=item.end-edit.mediaStart;
    }
    if(edit.duration){
     // Round before applying the bounded fixed-point rate. Split the product
     // at 65536 to prevent overflow. This is a conservative input bound;
     // the exact output boundary is clipped independently in movie units.
     const auto whole=edit.duration/track.movieTimescale*track.timescale;
     const auto fraction=edit.duration%track.movieTimescale*track.timescale;
     const auto extent=whole+fraction/track.movieTimescale+(fraction%track.movieTimescale!=0);
     const auto remainder=extent%65536*rateMagnitude;
     const auto ticks=extent/65536*rateMagnitude+remainder/65536+(remainder%65536!=0);
     if(static_cast<uint64_t>(start)>=ticks)continue;end=std::min(end,static_cast<int64_t>(ticks));
    }
    emit(start,end,edit.movieStart,edit.duration,track.movieTimescale,rateMagnitude,edit.mediaStart,edit.rateFixed<0);
   }
  }
 }
 // Repeated/reordered media edits can emit cues out of movie-time order.
 std::stable_sort(result.cues.begin(),result.cues.end(),[](const SubtitleCue& a,const SubtitleCue& b){return a.start<b.start;});
 return result;
}
inline Mp4WebVttAssembly assembleMp4WebVttTrack(const AdaptiveTransferLayout& layout,size_t trackIndex,const Cancel& cancel){
 const auto& description=layout.descriptions.at(trackIndex);const auto& indices=layout.tracks.at(trackIndex);
 if(!yes(description,"mp4WebVtt"))throw std::runtime_error("Expected an MP4 WebVTT track.");
 std::vector<Mp4WebVttPart> parts;parts.reserve(indices.size());
 for(auto index:indices){cancel.check();const auto& part=layout.parts.at(index);const auto start=num(part.info,"start");if(start<0)throw std::runtime_error("Invalid MP4 caption resource offset.");parts.push_back({part.path,static_cast<uint64_t>(start)});}
 const auto& clock=description.at("clock");
 return assembleMp4WebVttParts(parts,num(clock,"presentationTimeOffset"),num(clock,"timescale"),cancel);
}
}
