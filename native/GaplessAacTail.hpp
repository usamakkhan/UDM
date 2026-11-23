#pragma once
#include "Core.hpp"
#include "Version.hpp"
#include <cmath>
#include <cstring>
#include <regex>
#include <sstream>
#include <iomanip>
namespace udm {
inline constexpr int GaplessAacTailVersion=1;
struct GaplessAacTailPlan {uint32_t rate=0;uint64_t samples=0;};
struct GaplessAacTailResult {bool applied=false;uint64_t padding=0;size_t metadataWritten=0;};
inline std::string gaplessAacTailReserve(){std::string value="UDM-gapless-aac-tail-reserve-v1:";value.resize(512,'.');return value;}
namespace aac_tail {
constexpr size_t MetadataLimit=64*1024*1024;
struct Box {size_t start=0,payload=0,end=0;std::string type;};
struct Reader {
 const Bytes& bytes;const Cancel& cancel;size_t count=0;
 void need(size_t at,size_t length,size_t end)const{if(end>bytes.size()||at>end||length>end-at)throw std::runtime_error("Invalid AAC tail metadata bounds.");}
 uint32_t u32(size_t at,size_t end)const{need(at,4,end);return (uint32_t(bytes[at])<<24)|(uint32_t(bytes[at+1])<<16)|(uint32_t(bytes[at+2])<<8)|bytes[at+3];}
 uint64_t u64(size_t at,size_t end)const{return (uint64_t(u32(at,end))<<32)|u32(at+4,end);}
 Box box(size_t& at,size_t end){cancel.check();need(at,8,end);if(++count>1000000)throw std::runtime_error("Excessive AAC tail metadata boxes.");const auto start=at;uint64_t length=u32(at,end);size_t header=8;if(length==1){length=u64(at+8,end);header=16;}if(length<header||length>end-start)throw std::runtime_error("Invalid AAC tail box size.");std::string type(reinterpret_cast<const char*>(bytes.data()+at+4),4);at+=static_cast<size_t>(length);return {start,start+header,at,std::move(type)};}
 std::vector<Box> children(const Box& parent){std::vector<Box> result;size_t at=parent.payload;while(at<parent.end)result.push_back(box(at,parent.end));return result;}
 std::optional<Box> child(const Box& parent,const char* type){std::optional<Box> result;for(const auto& value:children(parent))if(value.type==type){if(result)throw std::runtime_error("Duplicate AAC tail metadata box.");result=value;}return result;}
 Box required(const Box& parent,const char* type){auto value=child(parent,type);if(!value)throw std::runtime_error("Missing AAC tail metadata box.");return *value;}
};
inline void position(HANDLE file,uint64_t at){if(at>uint64_t(INT64_MAX))throw std::runtime_error("AAC tail file offset is too large.");LARGE_INTEGER value{};value.QuadPart=static_cast<LONGLONG>(at);if(!SetFilePointerEx(file,value,nullptr,FILE_BEGIN))throw std::runtime_error("Cannot seek AAC tail metadata.");}
inline Bytes read(HANDLE file,uint64_t at,size_t length,const Cancel& cancel){Bytes result(length);position(file,at);for(size_t n=0;n<length;){cancel.check();DWORD count=0;const auto request=static_cast<DWORD>(std::min<size_t>(65536,length-n));if(!ReadFile(file,result.data()+n,request,&count,nullptr)||count!=request)throw std::runtime_error("Cannot read AAC tail metadata.");n+=count;}return result;}
struct Movie {uint64_t offset=0,fileSize=0;Bytes bytes;std::optional<Bytes> firstFragment;};
inline Movie movie(HANDLE file,const Cancel& cancel,bool fragment){BY_HANDLE_FILE_INFORMATION info{};LARGE_INTEGER length{};if(!GetFileInformationByHandle(file,&info)||(info.dwFileAttributes&(FILE_ATTRIBUTE_DIRECTORY|FILE_ATTRIBUTE_REPARSE_POINT))||!GetFileSizeEx(file,&length)||length.QuadPart<8)throw std::runtime_error("Invalid AAC tail file.");Movie result;result.fileSize=length.QuadPart;uint64_t at=0;size_t count=0;
 while(at<result.fileSize){cancel.check();if(++count>10000||result.fileSize-at<8)throw std::runtime_error("Invalid AAC tail top-level boxes.");auto h=read(file,at,8,cancel);Reader reader{h,cancel};uint64_t size=reader.u32(0,h.size());std::string type(reinterpret_cast<const char*>(h.data()+4),4);size_t header=8;if(size==1){h=read(file,at,16,cancel);Reader wide{h,cancel};size=wide.u64(8,h.size());header=16;}else if(size==0)size=result.fileSize-at;
  if(size<header||size>result.fileSize-at)throw std::runtime_error("AAC tail top-level size exceeds the file.");
  if(type=="moov"){if(!result.bytes.empty()||size>MetadataLimit)throw std::runtime_error("Invalid or oversized AAC tail movie.");result.offset=at;result.bytes=read(file,at,static_cast<size_t>(size),cancel);}
  if(type=="moof"&&fragment){if(size>MetadataLimit)throw std::runtime_error("Oversized AAC fragment metadata.");result.firstFragment=read(file,at,static_cast<size_t>(size),cancel);break;}
  at+=size;
 }
 if(result.bytes.empty())throw std::runtime_error("AAC tail movie is missing.");return result;
}
struct Audio {Box track,media,table;uint32_t id=0,rate=0;uint64_t duration=0;};
inline Audio audio(Reader& r,const Box& movie){std::optional<Audio> result;
 for(const auto& track:r.children(movie)){if(track.type!="trak")continue;auto media=r.required(track,"mdia");const auto handler=r.required(media,"hdlr");r.need(handler.payload,12,handler.end);if(std::memcmp(r.bytes.data()+handler.payload+8,"soun",4))continue;if(result)throw std::runtime_error("AAC tail correction requires one audio track.");
  const auto header=r.required(track,"tkhd");const auto hv=r.u32(header.payload,header.end)>>24;if(hv>1)throw std::runtime_error("Unsupported AAC track header.");const auto id=r.u32(header.payload+(hv?20:12),header.end);
  const auto clock=r.required(media,"mdhd");const auto cv=r.u32(clock.payload,clock.end)>>24;if(cv>1)throw std::runtime_error("Unsupported AAC media clock.");const auto rate=r.u32(clock.payload+(cv?20:12),clock.end);const auto duration=cv?r.u64(clock.payload+24,clock.end):r.u32(clock.payload+16,clock.end);
  auto table=r.required(r.required(media,"minf"),"stbl");const auto descriptions=r.required(table,"stsd");if(r.u32(descriptions.payload,descriptions.end)||r.u32(descriptions.payload+4,descriptions.end)!=1)throw std::runtime_error("Unsupported AAC sample descriptions.");size_t at=descriptions.payload+8;auto entry=r.box(at,descriptions.end);if(entry.type!="mp4a"||at!=descriptions.end)throw std::runtime_error("AAC tail correction requires MP4 AAC.");
  r.need(entry.payload,28,entry.end);if(r.u32(entry.payload,entry.end)||r.u32(entry.payload+4,entry.end)!=1||r.u32(entry.payload+8,entry.end)||r.u32(entry.payload+24,entry.end)!=(rate<<16)||rate<8000||rate>65535||!id)throw std::runtime_error("Unsupported AAC sample clock.");
  result=Audio{track,media,table,id,rate,duration};
 }
 if(!result)throw std::runtime_error("AAC track is missing.");return *result;
}
inline bool zeroEdit(Reader& r,const Audio& audio){const auto edits=r.child(audio.track,"edts");if(!edits)return true;auto e=r.required(*edits,"elst");auto word=r.u32(e.payload,e.end),version=word>>24;if((word&0xffffff)||version>1||r.u32(e.payload+4,e.end)!=1)return false;const auto at=e.payload+8+(version?8:4);const auto media=version?r.u64(at,e.end):r.u32(at,e.end);const auto rate=r.u32(at+(version?8:4),e.end);return media==0&&rate==65536;}
inline void u32(Bytes& out,uint32_t value){for(int shift:{24,16,8,0})out.push_back(static_cast<unsigned char>(value>>shift));}
inline Bytes box(const char* type,const Bytes& body){if(body.size()>UINT32_MAX-8)throw std::runtime_error("AAC tail box is too large.");Bytes out;u32(out,static_cast<uint32_t>(body.size()+8));out.insert(out.end(),type,type+4);out.insert(out.end(),body.begin(),body.end());return out;}
inline Bytes textData(const std::string& value){Bytes data;u32(data,1);u32(data,0);data.insert(data.end(),value.begin(),value.end());return box("data",data);}
inline void append(Bytes& to,const Bytes& from){to.insert(to.end(),from.begin(),from.end());}
inline Bytes replacement(size_t bytes,uint64_t padding,uint64_t samples){
 const auto tool=textData(std::string("UDM ")+UDM_VERSION_STRING);const char encoder[4]={char(0xa9),'t','o','o'};Bytes output=box(encoder,tool);Bytes mean(4),name(4);const std::string owner="com.apple.iTunes",key="iTunSMPB";mean.insert(mean.end(),owner.begin(),owner.end());name.insert(name.end(),key.begin(),key.end());Bytes fields;append(fields,box("mean",mean));append(fields,box("name",name));
 std::ostringstream s;s<<" 00000000 00000000 "<<std::uppercase<<std::hex<<std::setfill('0')<<std::setw(8)<<padding<<" "<<std::setw(16)<<samples;for(int n=0;n<8;++n)s<<" 00000000";auto value=s.str();const auto used=output.size()+8+fields.size()+16+value.size();if(used>bytes)throw std::runtime_error("AAC tail reservation is too small.");value.append(bytes-used,' ');append(fields,textData(value));append(output,box("----",fields));if(output.size()!=bytes)throw std::runtime_error("AAC tail replacement changed file size.");return output;
}
}
inline std::optional<GaplessAacTailPlan> gaplessAacTailCandidate(const fs::path& input,const Json& probe,double duration,const Cancel& cancel){
 cancel.check();try{
  if(!std::isfinite(duration)||duration<=0||duration>7*86400||!probe.contains("streams")||probe["streams"].size()!=1)return {};
  const auto& stream=probe["streams"][0];if(str(stream,"codec_type")!="audio"||str(stream,"codec_name")!="aac"||str(stream,"profile")!="LC")return {};
  const auto rateText=str(stream,"sample_rate");if(!std::regex_match(rateText,std::regex("[0-9]{4,5}")))return {};const auto rate=static_cast<uint32_t>(std::stoul(rateText));if(rate<8000||rate>65535)return {};
  const long double precise=static_cast<long double>(duration)*rate;const auto wanted=static_cast<uint64_t>(std::llround(precise));if(!wanted||std::abs(precise-wanted)>0.000001L||wanted%1024==0)return {};
  Handle file(CreateFileW(input.c_str(),GENERIC_READ,FILE_SHARE_READ,nullptr,OPEN_EXISTING,FILE_FLAG_OPEN_REPARSE_POINT,nullptr));if(!file)return {};auto m=aac_tail::movie(file.h,cancel,true);if(!m.firstFragment)return {};aac_tail::Reader r{m.bytes,cancel};size_t at=0;auto moov=r.box(at,m.bytes.size());auto audio=aac_tail::audio(r,moov);if(audio.rate!=rate||r.child(audio.track,"edts"))return {};
  aac_tail::Reader f{*m.firstFragment,cancel};at=0;auto moof=f.box(at,m.firstFragment->size());size_t found=0;for(const auto& traf:f.children(moof))if(traf.type=="traf"){auto header=f.required(traf,"tfhd");if(f.u32(header.payload+4,header.end)!=audio.id)continue;auto clock=f.required(traf,"tfdt");auto v=f.u32(clock.payload,clock.end);if(v!=0&&v!=0x01000000)return {};const auto time=v?f.u64(clock.payload+4,clock.end):f.u32(clock.payload+4,clock.end);if(time)return {};++found;}if(found!=1)return {};
  return GaplessAacTailPlan{rate,wanted};
 }catch(const Cancelled&){throw;}catch(const std::exception&){cancel.check();return {};}
}
inline GaplessAacTailResult finalizeGaplessAacTail(const fs::path& path,const GaplessAacTailPlan& plan,const Cancel& cancel){
 cancel.check();Handle file(CreateFileW(path.c_str(),GENERIC_READ|GENERIC_WRITE,FILE_SHARE_READ,nullptr,OPEN_EXISTING,FILE_FLAG_OPEN_REPARSE_POINT,nullptr));if(!file)throw std::runtime_error("Cannot open AAC tail metadata.");auto movie=aac_tail::movie(file.h,cancel,false);aac_tail::Reader r{movie.bytes,cancel};size_t at=0;const auto moov=r.box(at,movie.bytes.size());if(moov.type!="moov"||at!=movie.bytes.size()||r.child(moov,"mvex"))throw std::runtime_error("AAC tail output must be a flat MP4 movie.");const auto audio=aac_tail::audio(r,moov);if(audio.rate!=plan.rate||!aac_tail::zeroEdit(r,audio))throw std::runtime_error("AAC tail output changed its leading clock.");
 const auto times=r.required(audio.table,"stts");if(r.u32(times.payload,times.end))throw std::runtime_error("Unsupported AAC sample timing version.");const auto runs=r.u32(times.payload+4,times.end);if(!runs||runs>1000000||uint64_t(runs)*8!=times.end-times.payload-8)throw std::runtime_error("Invalid AAC sample timing table.");uint64_t frames=0;for(uint32_t i=0;i<runs;++i){cancel.check();auto count=r.u32(times.payload+8+size_t(i)*8,times.end),duration=r.u32(times.payload+12+size_t(i)*8,times.end);if(!count||duration!=1024)throw std::runtime_error("Unsupported AAC packet duration.");frames+=count;}
 const auto sizes=r.required(audio.table,"stsz");if(r.u32(sizes.payload,sizes.end)||r.u32(sizes.payload+8,sizes.end)!=frames||frames>UINT64_MAX/1024||uint64_t(sizes.end-sizes.payload)!=12+(r.u32(sizes.payload+4,sizes.end)?0:frames*4))throw std::runtime_error("AAC sample counts disagree.");const auto total=frames*1024;if(audio.duration!=total||plan.samples>=total||total-plan.samples>=1024)throw std::runtime_error("AAC tail is not a partial final packet.");const auto padding=total-plan.samples;
 const auto udta=r.required(moov,"udta"),meta=r.required(udta,"meta");if(r.u32(meta.payload,meta.end))throw std::runtime_error("Unsupported AAC metadata version.");auto metadata=meta;metadata.payload+=4;const auto handler=r.required(metadata,"hdlr");r.need(handler.payload,12,handler.end);if(std::memcmp(movie.bytes.data()+handler.payload+8,"mdir",4))throw std::runtime_error("AAC tail metadata directory changed.");const auto list=r.required(metadata,"ilst");std::optional<aac_tail::Box> slot;
 for(const auto& tag:r.children(list)){
  if(tag.type=="----"){const auto name=r.child(tag,"name");if(name&&name->end-name->payload==12&&!std::memcmp(movie.bytes.data()+name->payload+4,"iTunSMPB",8))throw std::runtime_error("Existing AAC padding metadata requires separate handling.");continue;}
  const char encoder[4]={char(0xa9),'t','o','o'};if(tag.type!=std::string(encoder,4))continue;auto data=r.required(tag,"data");if(r.u32(data.payload,data.end)!=1||r.u32(data.payload+4,data.end))throw std::runtime_error("AAC reserve encoding changed.");const auto value=gaplessAacTailReserve();if(data.end-data.payload-8!=value.size()||std::memcmp(movie.bytes.data()+data.payload+8,value.data(),value.size()))throw std::runtime_error("AAC tail reservation was not retained.");if(slot)throw std::runtime_error("Duplicate AAC tail reservation.");slot=tag;
 }
 if(!slot)throw std::runtime_error("AAC tail reservation is missing.");const auto replacement=aac_tail::replacement(slot->end-slot->start,padding,plan.samples);std::copy(replacement.begin(),replacement.end(),movie.bytes.begin()+slot->start);
 // Parse global metadata after the initialized AAC trak, before later captions.
 // Only metadata moves inside the same-size moov; chunk offsets stay valid.
 const auto children=r.children(moov);Bytes reordered;reordered.insert(reordered.end(),movie.bytes.begin(),movie.bytes.begin()+moov.payload);for(const auto& child:children){if(child.start==udta.start)continue;reordered.insert(reordered.end(),movie.bytes.begin()+child.start,movie.bytes.begin()+child.end);if(child.start==audio.track.start)reordered.insert(reordered.end(),movie.bytes.begin()+udta.start,movie.bytes.begin()+udta.end);}if(reordered.size()!=movie.bytes.size())throw std::runtime_error("AAC metadata relocation changed movie size.");
 cancel.check();aac_tail::position(file.h,movie.offset);for(size_t offset=0;offset<reordered.size();){cancel.check();DWORD written=0;const auto count=static_cast<DWORD>(std::min<size_t>(65536,reordered.size()-offset));if(!WriteFile(file.h,reordered.data()+offset,count,&written,nullptr)||written!=count)throw std::runtime_error("Cannot write AAC tail metadata.");offset+=written;}
 // The caller's existing assembly checkpoint flushes and hashes this object.
 return {true,padding,reordered.size()};
}
}
