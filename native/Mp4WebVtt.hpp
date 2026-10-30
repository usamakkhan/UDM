#pragma once
#include "Core.hpp"
#include <cstdint>
#include <algorithm>
#include <optional>

namespace udm {
// One already-demuxed WVTT sample. Fragment clocks and sample extents are
// supplied by the container layer; no timing is guessed from cue text.
struct Mp4WebVttCue { std::string text,identifier,settings; };
struct Mp4WebVttEdit {int64_t mediaStart=0;uint64_t movieStart=0,duration=0;};
struct Mp4WebVttTrack {
 uint32_t id=0,timescale=0,description=0,defaultDuration=0,defaultSize=0,defaultFlags=0;
 std::string configuration;
 std::vector<Mp4WebVttEdit> edits;uint32_t movieTimescale=0;
};
namespace mp4vtt {
struct Box {size_t payload=0,end=0;std::string type;};
// Shared bounded reader for initialization and (later) fragment metadata.
class Reader {
 const Bytes& bytes;const Cancel& cancel;size_t boxes=0;
public:
 Reader(const Bytes& data,const Cancel& stop):bytes(data),cancel(stop){}
 void require(size_t at,size_t count,size_t end)const{if(end>bytes.size()||at>end||count>end-at)throw std::runtime_error("Truncated MP4 caption metadata.");}
 uint32_t u32(size_t at,size_t end)const{require(at,4,end);return (uint32_t(bytes[at])<<24)|(uint32_t(bytes[at+1])<<16)|(uint32_t(bytes[at+2])<<8)|uint32_t(bytes[at+3]);}
 Box read(size_t& at,size_t end){
  cancel.check();require(at,8,end);if(++boxes>16384)throw std::runtime_error("Excessive MP4 caption metadata boxes.");
  const size_t start=at;uint64_t size=u32(at,end);size_t header=8;std::string type(reinterpret_cast<const char*>(bytes.data()+at+4),4);
  if(size==1){require(at,16,end);size=(uint64_t(u32(at+8,end))<<32)|u32(at+12,end);header=16;}else if(!size)size=end-at;
  if(size<header||size>end-start)throw std::runtime_error("MP4 caption metadata box exceeds its parent.");
  at=start+static_cast<size_t>(size);return {start+header,at,std::move(type)};
 }
 std::optional<Box> child(const Box& parent,const char* type){
  std::optional<Box> found;size_t at=parent.payload;
  while(at<parent.end){auto box=read(at,parent.end);if(box.type==type){if(found)throw std::runtime_error("Duplicate MP4 caption metadata box.");found=std::move(box);}}
  return found;
 }
 Box required(const Box& parent,const char* type){auto box=child(parent,type);if(!box)throw std::runtime_error("Missing MP4 caption metadata box.");return *box;}
 uint32_t full(const Box& box,unsigned maximumVersion=0)const{auto value=u32(box.payload,box.end);if((value>>24)>maximumVersion||(value&0xffffff))throw std::runtime_error("Unsupported MP4 caption metadata version or flags.");return value>>24;}
 std::string text(const Box& box)const{
  const auto length=box.end-box.payload;if(length>65536)throw std::runtime_error("MP4 caption configuration exceeds its limit.");
  std::string value(reinterpret_cast<const char*>(bytes.data()+box.payload),length);
  if(value.find('\0')!=std::string::npos||(!value.empty()&&!MultiByteToWideChar(CP_UTF8,MB_ERR_INVALID_CHARS,value.data(),static_cast<int>(value.size()),nullptr,0)))throw std::runtime_error("Invalid MP4 caption configuration text.");return value;
 }
};
}
inline Mp4WebVttTrack readMp4WebVttInitialization(const Bytes& bytes,const Cancel& cancel){
 cancel.check();if(bytes.empty()||bytes.size()>16*1024*1024)throw std::runtime_error("Invalid MP4 caption initialization size.");
 mp4vtt::Reader reader(bytes,cancel);mp4vtt::Box root{0,bytes.size(),"root"};auto movie=reader.required(root,"moov");
 std::optional<Mp4WebVttTrack> selected;std::vector<uint32_t> trackIds;size_t at=movie.payload;
 while(at<movie.end){
  auto track=reader.read(at,movie.end);if(track.type!="trak")continue;
  if(trackIds.size()>=64)throw std::runtime_error("Too many MP4 initialization tracks.");
  const auto header=reader.required(track,"tkhd");auto headerWord=reader.u32(header.payload,header.end);auto version=headerWord>>24;
  if(version>1)throw std::runtime_error("Unsupported MP4 track header version.");
  const auto id=reader.u32(header.payload+(version?20:12),header.end);
  if(!id||std::find(trackIds.begin(),trackIds.end(),id)!=trackIds.end())throw std::runtime_error("Invalid or duplicate MP4 track identifier.");trackIds.push_back(id);
  const auto media=reader.required(track,"mdia");const auto info=reader.required(media,"minf");const auto table=reader.required(info,"stbl");const auto descriptions=reader.required(table,"stsd");reader.full(descriptions);
  const auto count=reader.u32(descriptions.payload+4,descriptions.end);if(!count||count>64)throw std::runtime_error("Invalid MP4 sample description count.");
  size_t entryAt=descriptions.payload+8;
  for(uint32_t index=1;index<=count;++index){
   const auto entry=reader.read(entryAt,descriptions.end);if(entry.type!="wvtt")continue;
   if(selected)throw std::runtime_error("Ambiguous MP4 WebVTT tracks or sample descriptions.");
   reader.require(entry.payload,8,entry.end);
   if(reader.u32(entry.payload,entry.end)||reader.u32(entry.payload+4,entry.end)!=1)throw std::runtime_error("Unsupported MP4 WebVTT data reference.");
   const auto references=reader.required(reader.required(info,"dinf"),"dref");reader.full(references);
   const auto referenceCount=reader.u32(references.payload+4,references.end);
   if(referenceCount!=1)throw std::runtime_error("MP4 WebVTT requires one self-contained data reference.");
   size_t referenceAt=references.payload+8;const auto reference=reader.read(referenceAt,references.end);
   if(reference.type!="url "||reference.end-reference.payload!=4||reader.u32(reference.payload,reference.end)!=1||referenceAt!=references.end)throw std::runtime_error("External MP4 WebVTT data references are unsupported.");
   mp4vtt::Box fields{entry.payload+8,entry.end,"wvtt"};const auto configuration=reader.required(fields,"vttC");
   const auto clock=reader.required(media,"mdhd");const auto clockVersion=reader.full(clock,1);
   Mp4WebVttTrack value;value.id=id;value.description=index;value.timescale=reader.u32(clock.payload+(clockVersion?20:12),clock.end);value.configuration=reader.text(configuration);
   if(!value.timescale)throw std::runtime_error("MP4 WebVTT clock has a zero timescale.");
   if(auto edits=reader.child(track,"edts")){
    const auto list=reader.required(*edits,"elst");const auto editVersion=reader.full(list,1);
    const auto count=reader.u32(list.payload+4,list.end);
    if(!count||count>64||list.end-list.payload!=8+size_t(count)*(editVersion?20U:12U))throw std::runtime_error("Invalid or excessive MP4 WebVTT edit-list entries.");
    const auto movieHeader=reader.required(movie,"mvhd");const auto movieVersion=reader.full(movieHeader,1);
    value.movieTimescale=reader.u32(movieHeader.payload+(movieVersion?20:12),movieHeader.end);
    if(!value.movieTimescale)throw std::runtime_error("MP4 WebVTT movie clock has a zero timescale.");
    size_t field=list.payload+8;uint64_t movieStart=0;const auto maximumDuration=uint64_t(value.movieTimescale)*7*86400;
    auto readEdit=[&](){uint64_t number=reader.u32(field,list.end);field+=4;if(editVersion){number=(number<<32)|reader.u32(field,list.end);field+=4;}return number;};
    for(uint32_t index=0;index<count;++index){
     cancel.check();const auto duration=readEdit(),mediaStart=readEdit();const auto rate=reader.u32(field,list.end);field+=4;
     const bool empty=mediaStart==(editVersion?0xffffffffffffffffULL:0xffffffffULL);
     if((!empty&&mediaStart>(editVersion?0x7fffffffffffffffULL:0x7fffffffULL))||rate!=0x00010000U)throw std::runtime_error("MP4 WebVTT requires normal-rate edits with valid media times.");
     if(duration>maximumDuration-movieStart||(!duration&&(empty||index+1!=count)))throw std::runtime_error("Invalid MP4 WebVTT edit duration or open-ended position.");
     value.edits.push_back({empty?-1:static_cast<int64_t>(mediaStart),movieStart,duration});movieStart+=duration;
    }
   }
   selected=std::move(value);
  }
  if(entryAt!=descriptions.end)throw std::runtime_error("MP4 sample description count does not match its entries.");
 }
 if(!selected)throw std::runtime_error("MP4 initialization contains no WebVTT track.");
 if(auto extensions=reader.child(movie,"mvex")){
  bool found=false;size_t position=extensions->payload;
  while(position<extensions->end){const auto defaults=reader.read(position,extensions->end);if(defaults.type!="trex")continue;reader.full(defaults);reader.require(defaults.payload,24,defaults.end);
   if(reader.u32(defaults.payload+4,defaults.end)!=selected->id)continue;
   if(found||defaults.end-defaults.payload!=24)throw std::runtime_error("Invalid MP4 WebVTT fragment defaults.");found=true;
   if(reader.u32(defaults.payload+8,defaults.end)!=selected->description)throw std::runtime_error("MP4 WebVTT fragment defaults select another sample description.");
   selected->defaultDuration=reader.u32(defaults.payload+12,defaults.end);selected->defaultSize=reader.u32(defaults.payload+16,defaults.end);selected->defaultFlags=reader.u32(defaults.payload+20,defaults.end);
  }
 }
 cancel.check();return *selected;
}
inline std::vector<Mp4WebVttCue> decodeMp4WebVttSample(const Bytes& bytes,size_t offset,size_t length,const Cancel& cancel){
 cancel.check();
 if(length==0||length>2*1024*1024||offset>bytes.size()||length>bytes.size()-offset)throw std::runtime_error("Invalid MP4 WebVTT sample extent.");
 struct Box {size_t payload=0,end=0;std::string type;};size_t boxes=0;
 auto u32=[&](size_t at){return (uint32_t(bytes[at])<<24)|(uint32_t(bytes[at+1])<<16)|(uint32_t(bytes[at+2])<<8)|uint32_t(bytes[at+3]);};
 auto readBox=[&](size_t& at,size_t end){
  cancel.check();if(++boxes>4096||at>end||end-at<8)throw std::runtime_error("Truncated or excessive MP4 WebVTT boxes.");
  const size_t start=at;uint64_t size=u32(at);const std::string type(reinterpret_cast<const char*>(bytes.data()+at+4),4);size_t header=8;
  if(size==1){if(end-at<16)throw std::runtime_error("Truncated extended MP4 WebVTT box.");size=(uint64_t(u32(at+8))<<32)|u32(at+12);header=16;}
  else if(size==0)size=end-at;
  if(size<header||size>end-start)throw std::runtime_error("MP4 WebVTT box exceeds its sample.");
  at=start+static_cast<size_t>(size);return Box{start+header,at,type};
 };
 auto text=[&](const Box& box,bool oneLine){
  const size_t count=box.end-box.payload;if(count>65536)throw std::runtime_error("MP4 WebVTT cue field exceeds its limit.");
  std::string value(reinterpret_cast<const char*>(bytes.data()+box.payload),count);
  if(value.find('\0')!=std::string::npos||(!value.empty()&&!MultiByteToWideChar(CP_UTF8,MB_ERR_INVALID_CHARS,value.data(),static_cast<int>(value.size()),nullptr,0)))throw std::runtime_error("Invalid MP4 WebVTT UTF-8 text.");
  if(oneLine&&std::any_of(value.begin(),value.end(),[](unsigned char c){return c<32&&c!='\t';}))throw std::runtime_error("MP4 WebVTT identifier/settings must occupy one line.");
  return value;
 };
 std::vector<Mp4WebVttCue> cues;bool empty=false,recognized=false;size_t at=offset;const size_t end=offset+length;
 while(at<end){
  const auto box=readBox(at,end);
  if(box.type=="vtte"){
   if(empty||!cues.empty()||recognized||box.payload!=box.end)throw std::runtime_error("Ambiguous MP4 WebVTT empty sample.");
   empty=true;recognized=true;continue;
  }
  if(box.type!="vttc")continue;
  if(empty)throw std::runtime_error("MP4 WebVTT sample mixes empty and active cues.");recognized=true;
  if(cues.size()>=1024)throw std::runtime_error("MP4 WebVTT sample exceeds its cue limit.");
  std::optional<std::string> payload,identifier,settings;size_t child=box.payload;
  while(child<box.end){
   const auto field=readBox(child,box.end);
   auto assign=[&](std::optional<std::string>& destination,bool oneLine){if(destination)throw std::runtime_error("Duplicate MP4 WebVTT cue field.");destination=text(field,oneLine);};
   if(field.type=="payl")assign(payload,false);else if(field.type=="iden")assign(identifier,true);else if(field.type=="sttg")assign(settings,true);
  }
  if(!payload)throw std::runtime_error("MP4 WebVTT cue has no payload.");
  cues.push_back({std::move(*payload),identifier.value_or(""),settings.value_or("")});
 }
 if(!recognized)throw std::runtime_error("MP4 WebVTT sample contains no cue or empty marker.");
 cancel.check();return cues;
}
}
