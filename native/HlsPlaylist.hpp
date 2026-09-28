#pragma once
#include "Core.hpp"
#include <algorithm>
#include <cmath>
#include <optional>
#include <regex>
#include <sstream>

namespace udm {
// RFC 8216 media-playlist model. Sequence numbers identify complete media
// segments; initialization sections and low-latency parts do not consume one.
struct HlsRange {
 i64 start=0,length=0;
 bool operator==(const HlsRange& other)const{return start==other.start&&length==other.length;}
};
struct HlsResource {
 std::string url;
 std::optional<HlsRange> range;
 bool operator==(const HlsResource& other)const{return url==other.url&&range==other.range;}
};
struct HlsSegment {
 HlsResource media;
 std::optional<HlsResource> initialization;
 i64 sequence=0,discontinuity=0;
 double duration=0;
 std::optional<i64> programTimeUs;
 bool gap=false;
};
struct HlsPlaylist {
 std::string url,type;
 i64 firstSequence=0,targetDuration=0,discontinuitySequence=0;
 bool ended=false;
 std::vector<HlsSegment> segments;
};
inline i64 hlsInteger(const std::string& value,i64 maximum=9007199254740991LL){
 if(value.empty()||value.size()>16||!std::all_of(value.begin(),value.end(),[](unsigned char c){return c>='0'&&c<='9';}))throw std::runtime_error("Invalid HLS integer.");
 size_t used=0;auto number=std::stoll(value,&used);
 if(used!=value.size()||number>maximum)throw std::runtime_error("HLS integer is outside its supported range.");return number;
}
inline double hlsDuration(const std::string& value){
 if(value.empty()||value.size()>32||!std::regex_match(value,std::regex("[0-9]+(\\.[0-9]+)?")))throw std::runtime_error("Invalid HLS duration.");
 auto number=std::stod(value);if(!std::isfinite(number)||number<=0||number>86400)throw std::runtime_error("Invalid HLS duration.");return number;
}
inline std::map<std::string,std::string> hlsAttributes(const std::string& value){
 std::map<std::string,std::string> result;size_t at=0;
 while(at<value.size()){
  auto equals=value.find('=',at);if(equals==std::string::npos)throw std::runtime_error("Malformed HLS attribute list.");
  auto key=trim(value.substr(at,equals-at));if(key.empty()||!std::all_of(key.begin(),key.end(),[](unsigned char c){return (c>='A'&&c<='Z')||(c>='0'&&c<='9')||c=='-';})||result.count(key))throw std::runtime_error("Invalid or duplicate HLS attribute.");
  at=equals+1;std::string item;
  if(at<value.size()&&value[at]=='\"'){
   auto end=value.find('\"',++at);if(end==std::string::npos)throw std::runtime_error("Unterminated HLS quoted attribute.");item=value.substr(at,end-at);at=end+1;
  }else{auto end=value.find(',',at);if(end==std::string::npos)end=value.size();item=trim(value.substr(at,end-at));at=end;}
  if(item.empty())throw std::runtime_error("Empty HLS attribute.");result.emplace(key,item);
  if(at==value.size())break;if(value[at++]!=','||at==value.size())throw std::runtime_error("Malformed HLS attribute separator.");
 }
 return result;
}
inline std::string hlsAddress(const std::string& base,const std::string& relative){
 if(relative.empty()||relative.size()>16384||relative.find("{$")!=std::string::npos||std::any_of(relative.begin(),relative.end(),[](unsigned char c){return c<=32||c==127;}))throw std::runtime_error("Invalid HLS resource address.");
 auto result=combineUrl(base,relative);Url address(result);
 if(address.scheme!="http"&&address.scheme!="https")throw std::runtime_error("HLS resources require HTTP or HTTPS.");
 if(result.size()>16384)throw std::runtime_error("HLS resource address is too long.");return result;
}
inline HlsRange hlsRange(const std::string& value,const std::optional<HlsResource>& previous,const std::string& url,bool explicitOffset=false){
 auto split=value.find('@');auto length=hlsInteger(value.substr(0,split),256LL*1024*1024);
 if(!length)throw std::runtime_error("Empty HLS byte range.");i64 start=0;
 if(split!=std::string::npos)start=hlsInteger(value.substr(split+1));
 else {if(explicitOffset||!previous||previous->url!=url||!previous->range)throw std::runtime_error("HLS byte-range offset is ambiguous.");start=previous->range->start+previous->range->length;}
 if(start>9007199254740991LL-length)throw std::runtime_error("HLS byte range exceeds its supported range.");return {start,length};
}
inline i64 hlsProgramTime(const std::string& value){
 std::smatch match;
 if(!std::regex_match(value,match,std::regex("([0-9]{4})-([0-9]{2})-([0-9]{2})T([0-9]{2}):([0-9]{2}):([0-9]{2})(?:\\.([0-9]{1,9}))?(Z|[+-][0-9]{2}:[0-9]{2})")))throw std::runtime_error("Invalid HLS program date/time.");
 SYSTEMTIME time{};time.wYear=(WORD)std::stoi(match[1]);time.wMonth=(WORD)std::stoi(match[2]);time.wDay=(WORD)std::stoi(match[3]);time.wHour=(WORD)std::stoi(match[4]);time.wMinute=(WORD)std::stoi(match[5]);time.wSecond=(WORD)std::stoi(match[6]);
 if(time.wYear<1970||time.wHour>23||time.wMinute>59||time.wSecond>59)throw std::runtime_error("Invalid HLS program date/time.");
 FILETIME file{};if(!SystemTimeToFileTime(&time,&file))throw std::runtime_error("Invalid HLS calendar date.");SYSTEMTIME roundtrip{};if(!FileTimeToSystemTime(&file,&roundtrip)||roundtrip.wYear!=time.wYear||roundtrip.wMonth!=time.wMonth||roundtrip.wDay!=time.wDay)throw std::runtime_error("Invalid HLS calendar date.");
 ULARGE_INTEGER ticks{};ticks.LowPart=file.dwLowDateTime;ticks.HighPart=file.dwHighDateTime;auto microseconds=(i64)(ticks.QuadPart/10)-11644473600000000LL;
 auto fraction=match[7].str();if(!fraction.empty()){fraction.resize(6,'0');microseconds+=std::stoll(fraction);}
 auto zone=match[8].str();if(zone!="Z"){auto hours=std::stoi(zone.substr(1,2)),minutes=std::stoi(zone.substr(4,2));if(hours>23||minutes>59)throw std::runtime_error("Invalid HLS timezone offset.");auto offset=((i64)hours*60+minutes)*60*1000000;microseconds+=zone[0]=='+'?-offset:offset;}
 return microseconds;
}
inline HlsPlaylist parseHlsPlaylist(const std::string& text,const std::string& address){
 if(text.empty()||text.size()>2*1024*1024||text.rfind("#EXTM3U",0)!=0||!MultiByteToWideChar(CP_UTF8,MB_ERR_INVALID_CHARS,text.data(),(int)text.size(),nullptr,0)||std::any_of(text.begin(),text.end(),[](unsigned char c){return (c<32&&c!='\n'&&c!='\r'&&c!='\t')||c==127;}))throw std::runtime_error("Invalid or oversized HLS playlist.");
 HlsPlaylist result;result.url=hlsAddress(address,address);std::istringstream input(text);std::string line;bool first=true,sequenceSeen=false,targetSeen=false,discontinuitySeen=false,typeSeen=false,versionSeen=false;
 std::optional<double> duration;std::optional<std::string> byteRange;std::optional<HlsResource> initialization,previous;
 std::optional<i64> programTime;bool explicitProgramTime=false,gap=false,pendingDiscontinuity=false;i64 sequence=0,discontinuity=0;
 while(std::getline(input,line)){
  if(!line.empty()&&line.back()=='\r')line.pop_back();if(first){first=false;if(line!="#EXTM3U")throw std::runtime_error("Invalid HLS header.");continue;}
  line=trim(line);if(line.empty())continue;
  auto tag=[&](const char* prefix){return line.rfind(prefix,0)==0;};
  if(line=="#EXTM3U")throw std::runtime_error("Repeated HLS header.");
  if(tag("#EXT-X-STREAM-INF:")||tag("#EXT-X-MEDIA:")||tag("#EXT-X-I-FRAME-STREAM-INF:"))throw std::runtime_error("Select a media quality before recording its HLS playlist.");
  if(tag("#EXT-X-KEY:")||tag("#EXT-X-SESSION-KEY:")){auto attributes=hlsAttributes(line.substr(line.find(':')+1));if(attributes["METHOD"]!="NONE"||attributes.size()!=1)throw std::runtime_error("Encrypted HLS is not supported by this recorder.");continue;}
  if(tag("#EXT-X-DEFINE:")||tag("#EXT-X-SKIP:")||line=="#EXT-X-I-FRAMES-ONLY")throw std::runtime_error("This HLS playlist requires an unsupported delta or variable expansion.");
  if(tag("#EXT-X-VERSION:")){if(versionSeen)throw std::runtime_error("Repeated HLS version.");versionSeen=true;if(!hlsInteger(line.substr(15),100))throw std::runtime_error("Invalid HLS version.");}
  else if(tag("#EXT-X-TARGETDURATION:")){if(targetSeen)throw std::runtime_error("Repeated HLS target duration.");targetSeen=true;result.targetDuration=hlsInteger(line.substr(22),86400);if(!result.targetDuration)throw std::runtime_error("Empty HLS target duration.");}
  else if(tag("#EXT-X-MEDIA-SEQUENCE:")){if(sequenceSeen||!result.segments.empty())throw std::runtime_error("Invalid HLS media sequence placement.");sequenceSeen=true;sequence=result.firstSequence=hlsInteger(line.substr(22));}
  else if(tag("#EXT-X-DISCONTINUITY-SEQUENCE:")){if(discontinuitySeen||!result.segments.empty()||pendingDiscontinuity)throw std::runtime_error("Invalid HLS discontinuity sequence placement.");discontinuitySeen=true;discontinuity=result.discontinuitySequence=hlsInteger(line.substr(30));}
  else if(line=="#EXT-X-DISCONTINUITY"){if(discontinuity>=9007199254740991LL)throw std::runtime_error("HLS discontinuity sequence overflow.");++discontinuity;pendingDiscontinuity=true;if(!explicitProgramTime)programTime.reset();}
  else if(tag("#EXT-X-PLAYLIST-TYPE:")){if(typeSeen)throw std::runtime_error("Repeated HLS playlist type.");typeSeen=true;result.type=line.substr(21);if(result.type!="EVENT"&&result.type!="VOD")throw std::runtime_error("Unknown HLS playlist type.");}
  else if(tag("#EXT-X-PROGRAM-DATE-TIME:")){if(explicitProgramTime)throw std::runtime_error("Repeated HLS program date/time.");programTime=hlsProgramTime(line.substr(25));explicitProgramTime=true;}
  else if(tag("#EXT-X-MAP:")){auto attributes=hlsAttributes(line.substr(11));HlsResource resource;resource.url=hlsAddress(result.url,attributes["URI"]);if(attributes.count("BYTERANGE"))resource.range=hlsRange(attributes["BYTERANGE"],{},resource.url,true);initialization=resource;}
  else if(tag("#EXT-X-BYTERANGE:")){if(byteRange)throw std::runtime_error("Repeated HLS byte range.");byteRange=line.substr(17);}
  else if(tag("#EXTINF:")){if(duration)throw std::runtime_error("HLS segment is missing its resource address.");auto value=line.substr(8);duration=hlsDuration(value.substr(0,value.find(',')));}
  else if(line=="#EXT-X-GAP"){if(gap)throw std::runtime_error("Repeated HLS gap marker.");gap=true;}
  else if(line=="#EXT-X-ENDLIST"){if(result.ended)throw std::runtime_error("Repeated HLS end marker.");result.ended=true;}
  else if(line[0]!='#'){
   if(!duration||result.ended||result.segments.size()>=10000||sequence>=9007199254740991LL)throw std::runtime_error("Invalid or oversized HLS media window.");
   HlsSegment segment;segment.media.url=hlsAddress(result.url,line);if(byteRange)segment.media.range=hlsRange(*byteRange,previous,segment.media.url);segment.initialization=initialization;segment.sequence=sequence++;segment.discontinuity=discontinuity;segment.duration=*duration;segment.programTimeUs=programTime;segment.gap=gap;result.segments.push_back(segment);previous=segment.media;
   if(programTime)*programTime+=(i64)std::llround(*duration*1000000.0);duration.reset();byteRange.reset();explicitProgramTime=false;gap=false;pendingDiscontinuity=false;
  }
 }
 if(!targetSeen||duration||byteRange||gap||explicitProgramTime||pendingDiscontinuity)throw std::runtime_error("Incomplete HLS media playlist.");
 for(const auto& segment:result.segments)if(std::round(segment.duration)>result.targetDuration)throw std::runtime_error("HLS segment exceeds the declared target duration.");
 if(result.type=="VOD"&&!result.ended)throw std::runtime_error("A VOD playlist is missing its end marker.");return result;
}
inline bool hlsSameSegment(const HlsSegment& a,const HlsSegment& b){
 return a.sequence==b.sequence&&a.discontinuity==b.discontinuity&&a.media==b.media&&a.initialization==b.initialization&&a.gap==b.gap&&std::abs(a.duration-b.duration)<0.000001&&(!a.programTimeUs||!b.programTimeUs||std::abs(*a.programTimeUs-*b.programTimeUs)<=1000);
}
class HlsTimeline {
 bool initialized=false;
 i64 latestFirst=0;
public:
 std::string url,type;
 i64 targetDuration=0;
 bool ended=false;
 std::vector<HlsSegment> segments;
 // Validate an entire refresh before mutating the recording. A stale CDN
 // snapshot may be retried; lost or rewritten media is never silently skipped.
 size_t append(const HlsPlaylist& playlist){
  if(initialized&&(playlist.url!=url||playlist.targetDuration!=targetDuration||playlist.type!=type))throw std::runtime_error("The HLS recording playlist changed identity or target duration.");
  const auto next=segments.empty()?playlist.firstSequence:segments.back().sequence+1;
  if(initialized&&!segments.empty()&&playlist.firstSequence>next)throw std::runtime_error("Live segments expired before they could be recorded. Stop and save the captured portion.");
  size_t added=0;for(const auto& segment:playlist.segments){
   if(!segments.empty()&&segment.sequence>=segments.front().sequence&&segment.sequence<next){auto offset=(size_t)(segment.sequence-segments.front().sequence);if(offset>=segments.size()||!hlsSameSegment(segments[offset],segment))throw std::runtime_error("The HLS server rewrote a segment already associated with this recording.");}
   else if(segment.sequence>=next){if(ended)throw std::runtime_error("The HLS server extended an ended recording.");if(!segments.empty()&&segment.discontinuity<segments.back().discontinuity)throw std::runtime_error("The HLS discontinuity clock moved backwards.");++added;}
  }
  if(initialized&&playlist.firstSequence<latestFirst){if(added)throw std::runtime_error("The HLS media window moved backwards while adding new segments.");return 0;}
  if(initialized&&type=="EVENT"&&playlist.firstSequence!=latestFirst)throw std::runtime_error("The HLS event playlist removed recorded history.");
  if(playlist.ended&&!playlist.segments.empty()&&!segments.empty()&&playlist.segments.back().sequence<segments.back().sequence)throw std::runtime_error("The HLS end marker precedes recorded media.");
  if(!initialized){url=playlist.url;type=playlist.type;targetDuration=playlist.targetDuration;initialized=true;}
  for(const auto& segment:playlist.segments)if(segment.sequence>=next)segments.push_back(segment);
  latestFirst=playlist.firstSequence;ended=ended||playlist.ended;return added;
 }
};
}
