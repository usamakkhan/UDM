#pragma once
#include "Core.hpp"
#include <regex>
#include <sstream>
#include <iomanip>
#include <cmath>
#include <tuple>

namespace udm {
// Bounded WebVTT text conversion. No network, script, XML, or external resources.
struct SubtitleCue { i64 start=0,end=0;std::string text; };
inline i64 subtitleTime(const std::string& value){
 std::smatch m;
 if(!std::regex_match(value,m,std::regex("(?:(\\d{2,}):)?([0-5][0-9]):([0-5][0-9])\\.([0-9]{3})")))throw std::runtime_error("Invalid WebVTT timestamp.");
 if(m[1].length()>4)throw std::runtime_error("WebVTT timestamp exceeds the recording limit.");
 i64 result=((m[1].matched?std::stoll(m[1]):0)*3600+std::stoll(m[2])*60+std::stoll(m[3]))*1000+std::stoll(m[4]);
 if(result>7LL*86400*1000)throw std::runtime_error("WebVTT timestamp exceeds the recording limit.");return result;
}
inline std::string subtitleStamp(i64 value){
 std::ostringstream out;out<<std::setfill('0')<<std::setw(2)<<value/3600000<<":"<<std::setw(2)<<value/60000%60<<":"<<std::setw(2)<<value/1000%60<<"."<<std::setw(3)<<value%1000;return out.str();
}
inline void appendWebVtt(std::vector<SubtitleCue>& cues,std::string input,double mediaStart,double timeline,bool hls){
 if(input.size()>2*1024*1024||input.find('\0')!=std::string::npos||!std::isfinite(mediaStart)||std::abs(mediaStart)>7*86400||!std::isfinite(timeline)||timeline<0||timeline>7*86400)throw std::runtime_error("Invalid or oversized subtitle segment.");
 if(!input.empty()&&!MultiByteToWideChar(CP_UTF8,MB_ERR_INVALID_CHARS,input.data(),(int)input.size(),nullptr,0))throw std::runtime_error("Subtitles are not valid UTF-8.");
 if(input.compare(0,3,"\xef\xbb\xbf")==0)input.erase(0,3);
 std::vector<std::string> lines;std::string line;std::istringstream in(input);
 while(std::getline(in,line)){if(!line.empty()&&line.back()=='\r')line.pop_back();if(line.size()>65536)throw std::runtime_error("Subtitle line is too long.");lines.push_back(line);}
 if(lines.empty()||lines[0].find("-->")!=std::string::npos||!(lines[0]=="WEBVTT"||lines[0].rfind("WEBVTT ",0)==0||lines[0].rfind("WEBVTT\t",0)==0))throw std::runtime_error("Expected a self-contained WebVTT subtitle segment.");
 size_t at=1;bool mapped=false;double offset=hls?-mediaStart:0;
 for(;at<lines.size()&&!lines[at].empty();++at){
  if(lines[at].rfind("X-TIMESTAMP-MAP",0)!=0)continue;
  if(!hls||mapped)throw std::runtime_error("Ambiguous WebVTT timestamp map.");
  auto value=lines[at];std::smatch m;std::string local,pts;
  if(std::regex_match(value,m,std::regex("X-TIMESTAMP-MAP=LOCAL:([^,]+),MPEGTS:([0-9]{1,10})"))){local=m[1];pts=m[2];}
  else if(std::regex_match(value,m,std::regex("X-TIMESTAMP-MAP=MPEGTS:([0-9]{1,10}),LOCAL:([^,]+)"))){pts=m[1];local=m[2];}
  else throw std::runtime_error("Invalid WebVTT timestamp map.");
  i64 ticks=std::stoll(pts);if(ticks>=(1LL<<33))throw std::runtime_error("WebVTT MPEGTS clock exceeds 33 bits.");
  double clock=ticks/90000.0;
  offset=clock-subtitleTime(local)/1000.0-mediaStart;mapped=true;
 }
 if(at==lines.size())throw std::runtime_error("WebVTT header is not terminated.");
 while(at<lines.size()){
  while(at<lines.size()&&lines[at].empty())++at;if(at==lines.size())break;
  size_t end=at;while(end<lines.size()&&!lines[end].empty())++end;
  if(lines[at]=="NOTE"||lines[at].rfind("NOTE ",0)==0||lines[at].rfind("NOTE\t",0)==0||lines[at]=="STYLE"||lines[at]=="REGION"){at=end;continue;}
  size_t timing=at;if(lines[timing].find("-->")==std::string::npos)++timing;
  std::smatch m;if(timing>=end||!std::regex_match(lines[timing],m,std::regex("([^ \\t]+)[ \\t]+-->[ \\t]+([^ \\t]+)(?:[ \\t]+.*)?")))throw std::runtime_error("Invalid WebVTT cue timing.");
  i64 rawStart=subtitleTime(m[1]),rawEnd=subtitleTime(m[2]);if(rawEnd<=rawStart)throw std::runtime_error("WebVTT cue end must follow its start.");
  // LOCAL may be far outside this segment. Select the PES epoch using the
  // actual cue, not the header anchor, and keep both cue endpoints together.
  double cueOffset=offset;
  if(hls){const double wrap=(1LL<<33)/90000.0,start=rawStart/1000.0+offset,cueEnd=rawEnd/1000.0+offset;if(timeline<start||timeline>=cueEnd)cueOffset+=std::round((timeline-start)/wrap)*wrap;}
  SubtitleCue cue{rawStart+(i64)std::llround(cueOffset*1000),rawEnd+(i64)std::llround(cueOffset*1000),{}};
  for(size_t i=timing+1;i<end;++i){if(lines[i].find("-->")!=std::string::npos)throw std::runtime_error("Invalid WebVTT cue payload.");if(!cue.text.empty())cue.text+='\n';cue.text+=lines[i];}
  if(cue.text.size()>65536||cues.size()>=100000)throw std::runtime_error("Subtitle cue limit exceeded.");
  if(cue.end>0&&!cue.text.empty()){cue.start=std::max<i64>(0,cue.start);if(cue.end>7LL*86400*1000)throw std::runtime_error("Subtitle timeline exceeds seven days.");cues.push_back(std::move(cue));}at=end;
 }
}
inline std::string webVttWithHeader(const std::string& header,const std::string& body){
 // EXT-X-MAP contains the WebVTT header, never subtitle cues. Validate it
 // separately so malformed maps cannot be hidden by a self-contained segment.
 std::vector<SubtitleCue> unused;appendWebVtt(unused,header,0,0,true);
 std::istringstream lines(header);std::string line;bool ended=false;
 while(std::getline(lines,line)){if(!line.empty()&&line.back()=='\r')line.pop_back();if(ended&&!line.empty())throw std::runtime_error("WebVTT initialization contains data after its header.");if(line.empty())ended=true;}
 if(!ended)throw std::runtime_error("WebVTT initialization header is not terminated.");
 if(header.size()+body.size()>2*1024*1024)throw std::runtime_error("Combined WebVTT initialization and segment exceed 2 MB.");
 return header+body;
}
inline std::string mergedWebVtt(std::vector<SubtitleCue> cues){
 std::sort(cues.begin(),cues.end(),[](const auto& a,const auto& b){return std::tie(a.start,a.end,a.text)<std::tie(b.start,b.end,b.text);});
 std::string result="WEBVTT\n\n";SubtitleCue previous;bool seen=false;
 for(const auto& cue:cues){if(seen&&std::tie(cue.start,cue.end,cue.text)==std::tie(previous.start,previous.end,previous.text))continue;
  result+=subtitleStamp(cue.start)+" --> "+subtitleStamp(cue.end)+"\n"+cue.text+"\n\n";previous=cue;seen=true;if(result.size()>16*1024*1024)throw std::runtime_error("Combined subtitles exceed 16 MB.");
 }return result;
}
}
