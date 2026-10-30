#pragma once
#include "Mp4WebVtt.hpp"
#include <limits>
#include <iterator>

namespace udm {
struct Mp4WebVttTimedCue {int64_t start=0,end=0;Mp4WebVttCue cue;};
struct Mp4WebVttFragmentResult {
 std::vector<Mp4WebVttTimedCue> cues;
 uint64_t nextDecodeTime=0;
 size_t samples=0;
};
// Timestamps remain in track ticks. Period/PTO conversion belongs to assembly.
// resourceOffset is the absolute byte offset of bytes[0] for ranged resources.
inline Mp4WebVttFragmentResult decodeMp4WebVttFragments(const Bytes& bytes,const Mp4WebVttTrack& track,const Cancel& cancel,uint64_t resourceOffset=0,std::optional<uint64_t> previousDecodeEnd={}){
 constexpr uint64_t maximumTime=uint64_t(std::numeric_limits<int64_t>::max());
 if(bytes.empty()||bytes.size()>256*1024*1024||!track.id||!track.timescale||!track.description||resourceOffset>maximumTime||bytes.size()>maximumTime-resourceOffset)throw std::runtime_error("Invalid MP4 caption fragment input.");
 if(previousDecodeEnd&&*previousDecodeEnd>maximumTime)throw std::runtime_error("Invalid MP4 caption continuation clock.");
 mp4vtt::Reader reader(bytes,cancel);
 struct Fragment {mp4vtt::Box box;size_t start;};
 struct Sample {size_t offset,size;int64_t start,end;};
 std::vector<Fragment> fragments;std::vector<mp4vtt::Box> media;
 size_t position=0;
 while(position<bytes.size()){
  const size_t start=position;auto box=reader.read(position,bytes.size());
  if(box.type=="moof"){if(fragments.size()>=1024)throw std::runtime_error("Too many MP4 caption fragments.");fragments.push_back({box,start});}
  else if(box.type=="mdat")media.push_back(box);
 }
 if(fragments.empty())throw std::runtime_error("MP4 caption segment contains no fragments.");
 auto signed32=[](uint32_t value)->int64_t{return value>=0x80000000U?int64_t(value)-0x100000000LL:int64_t(value);};
 auto addOffset=[&](uint64_t base,int64_t delta){
  if(delta<0){const auto amount=uint64_t(-delta);if(amount>base)throw std::runtime_error("MP4 caption offset precedes its resource.");return base-amount;}
  if(uint64_t(delta)>maximumTime-base)throw std::runtime_error("MP4 caption offset overflow.");return base+uint64_t(delta);
 };
 std::vector<Sample> samples;bool selected=false;auto nextTime=previousDecodeEnd;
 for(const auto& fragment:fragments){
  size_t child=fragment.box.payload,trackOrdinal=0;bool selectedHere=false;
  while(child<fragment.box.end){
   const auto traf=reader.read(child,fragment.box.end);if(traf.type!="traf")continue;++trackOrdinal;
   const auto header=reader.required(traf,"tfhd");const auto word=reader.u32(header.payload,header.end);const auto flags=word&0xffffff;
   const auto id=reader.u32(header.payload+4,header.end);if(id!=track.id)continue;
   if(selectedHere)throw std::runtime_error("Duplicate MP4 caption track fragment.");selectedHere=true;selected=true;
   if((word>>24)!=0||(flags&~0x03003bU)||((flags&1)&&(flags&0x020000)))throw std::runtime_error("Unsupported MP4 caption track-fragment flags.");
   // Encrypted samples cannot be interpreted as clear WVTT boxes.
   if(reader.child(traf,"senc")||reader.child(traf,"saiz")||reader.child(traf,"saio"))throw std::runtime_error("Encrypted MP4 captions are unsupported.");
   size_t field=header.payload+8;uint64_t base=resourceOffset+fragment.start;
   auto read32=[&](){const auto value=reader.u32(field,header.end);field+=4;return value;};
   if(flags&1){const auto high=read32();const auto low=read32();base=(uint64_t(high)<<32)|low;if(base>maximumTime)throw std::runtime_error("MP4 caption base offset exceeds its limit.");}
   else if(!(flags&0x020000)&&trackOrdinal!=1)throw std::runtime_error("Implicit offsets across MP4 tracks need explicit mapping.");
   const auto description=(flags&2)?read32():track.description;
   if(description!=track.description)throw std::runtime_error("MP4 caption fragment changes its sample description.");
   const auto defaultDuration=(flags&8)?read32():track.defaultDuration;
   const auto defaultSize=(flags&16)?read32():track.defaultSize;
   if(flags&32)read32();
   if(field!=header.end)throw std::runtime_error("Unexpected MP4 caption track-fragment fields.");
   auto decodeTime=nextTime;
   if(auto clock=reader.child(traf,"tfdt")){
    const auto version=reader.full(*clock,1);const auto high=reader.u32(clock->payload+4,clock->end);
    decodeTime=version?((uint64_t(high)<<32)|reader.u32(clock->payload+8,clock->end)):high;
    if(clock->end-clock->payload!=(version?12U:8U))throw std::runtime_error("Invalid MP4 caption decode-time box.");
   }
   if(!decodeTime||*decodeTime>maximumTime)throw std::runtime_error("MP4 caption fragment has no valid decode clock.");
   if(nextTime&&*decodeTime<*nextTime)throw std::runtime_error("Overlapping MP4 caption decode intervals.");
   uint64_t samplePosition=base;size_t runAt=traf.payload;bool sawRun=false;
   while(runAt<traf.end){
    const auto run=reader.read(runAt,traf.end);if(run.type!="trun")continue;sawRun=true;
    const auto runWord=reader.u32(run.payload,run.end);const auto runVersion=runWord>>24;const auto runFlags=runWord&0xffffff;
    if(runVersion>1||(runFlags&~0x000f05U)||((runFlags&4)&&(runFlags&0x400)))throw std::runtime_error("Unsupported MP4 caption sample-run flags.");
    const auto count=reader.u32(run.payload+4,run.end);if(count>10000-samples.size()||((flags&0x010000)&&count))throw std::runtime_error("Invalid or excessive MP4 caption sample count.");
    size_t valueAt=run.payload+8;
    auto run32=[&](){const auto value=reader.u32(valueAt,run.end);valueAt+=4;return value;};
    if(runFlags&1)samplePosition=addOffset(base,signed32(run32()));
    if(runFlags&4)run32();
    for(uint32_t sample=0;sample<count;++sample){
     cancel.check();const auto duration=(runFlags&0x100)?run32():defaultDuration;const auto length=(runFlags&0x200)?run32():defaultSize;
     if(runFlags&0x400)run32();
     int64_t composition=0;if(runFlags&0x800){const auto raw=run32();composition=runVersion?signed32(raw):int64_t(raw);}
     if(!duration||!length||length>2*1024*1024||duration>maximumTime-*decodeTime)throw std::runtime_error("Invalid MP4 caption sample duration or size.");
     if(composition>0&&uint64_t(composition)>maximumTime-*decodeTime)throw std::runtime_error("MP4 caption presentation time overflow.");
     const int64_t start=int64_t(*decodeTime)+composition;
     if(start>std::numeric_limits<int64_t>::max()-int64_t(duration))throw std::runtime_error("MP4 caption presentation end overflow.");
     if(samplePosition<resourceOffset||samplePosition-resourceOffset>bytes.size()||length>bytes.size()-(samplePosition-resourceOffset))throw std::runtime_error("MP4 caption sample lies outside the supplied resource range.");
     const auto local=static_cast<size_t>(samplePosition-resourceOffset);
     const auto containing=std::upper_bound(media.begin(),media.end(),local,[](size_t offset,const mp4vtt::Box& box){return offset<box.payload;});
     if(containing==media.begin()||local>=std::prev(containing)->end||length>std::prev(containing)->end-local)throw std::runtime_error("MP4 caption sample is not contained in media data.");
     samples.push_back({local,length,start,start+duration});samplePosition+=length;*decodeTime+=duration;
    }
    if(valueAt!=run.end)throw std::runtime_error("MP4 caption run fields do not match its sample count.");
   }
   if(!sawRun&&!(flags&0x010000))throw std::runtime_error("MP4 caption fragment contains no sample run.");
   nextTime=decodeTime;
  }
 }
 if(!selected||!nextTime)throw std::runtime_error("MP4 fragments omit the selected caption track.");
 // Reject aliasing before decoding; unrelated tracks may occupy other mdat bytes.
 std::vector<std::pair<size_t,size_t>> ranges;for(const auto& sample:samples)ranges.emplace_back(sample.offset,sample.offset+sample.size);
 std::sort(ranges.begin(),ranges.end());for(size_t index=1;index<ranges.size();++index)if(ranges[index].first<ranges[index-1].second)throw std::runtime_error("MP4 caption samples overlap in byte storage.");
 Mp4WebVttFragmentResult result;result.nextDecodeTime=*nextTime;result.samples=samples.size();size_t textBytes=0;
 for(const auto& sample:samples){
  auto cues=decodeMp4WebVttSample(bytes,sample.offset,sample.size,cancel);
  for(auto& cue:cues){textBytes+=cue.text.size()+cue.identifier.size()+cue.settings.size();if(result.cues.size()>=100000||textBytes>16*1024*1024)throw std::runtime_error("MP4 caption output exceeds its limit.");result.cues.push_back({sample.start,sample.end,std::move(cue)});}
 }
 cancel.check();return result;
}
}
