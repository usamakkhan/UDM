#pragma once
#include "Core.hpp"
#include <fstream>
#include <future>
namespace udm {
// Restricted to immutable YouTube direct MP4 tracks. This is a display-only
// mux calculation; it must never supply transfer bounds or resume validators.
inline bool canPreviewMedia(const Json& data){
 if(str(data,"SourceUrl").empty()||!str(data,"ProtectedSabr").empty()||!str(data,"ProtectedAdaptive").empty()||str(data,"MediaOutput")=="audio")return false;
 for(auto name:{"Video","Audio"}){
  if(!data.contains(name)||!data[name].is_object())return false;
  try{Url u(str(data[name],"Url"));auto q=query(u.full);if(u.scheme!="https"||!hostIs(u.host,"googlevideo.com")||u.path!="/videoplayback"||!q.count("clen")||q.count("sabr")||q.count("ump")||q.count("sq"))return false;}catch(...){return false;}
 }
 return true;
}
namespace media_preview {
inline unsigned be32(const Bytes& b,size_t p){if(p>b.size()||b.size()-p<4)throw std::runtime_error("Truncated MP4 metadata.");return (unsigned(b[p])<<24)|(unsigned(b[p+1])<<16)|(unsigned(b[p+2])<<8)|b[p+3];}
inline unsigned initialization(const Bytes& b,size_t begin,size_t end,unsigned depth=0,std::string* selectedCodec=nullptr){
 if(depth>8||end>b.size())throw std::runtime_error("Invalid initialization nesting.");unsigned descriptions=0;
 for(auto at=begin;at<end;){auto n=be32(b,at);if(n<8||n>end-at)throw std::runtime_error("Invalid initialization box.");std::string type(b.begin()+at+4,b.begin()+at+8);
  if(type=="moov"||type=="trak"||type=="mdia"||type=="minf"||type=="stbl"||type=="mvex")descriptions+=initialization(b,at+8,at+n,depth+1,selectedCodec);
  if(type=="stsz"&&(n<20||be32(b,at+16)!=0))throw std::runtime_error("Mixed MP4 sample tables are unsupported.");
  if(type=="stz2")throw std::runtime_error("Compact MP4 sample tables are unsupported.");
  if(type=="stsd"){if(n<24||be32(b,at+12)!=1||be32(b,at+16)!=n-16)throw std::runtime_error("Multiple sample descriptions are unsupported.");std::string codec(b.begin()+at+20,b.begin()+at+24);if(codec!="avc1"&&codec!="av01"&&codec!="mp4a")throw std::runtime_error("Unsupported preview codec.");if(selectedCodec)*selectedCodec=codec;++descriptions;}
  if(type=="trex"&&(n!=32||be32(b,at+16)!=1))throw std::runtime_error("Unsupported default sample description.");
  at+=n;
 }return descriptions;
}
// Count complete per-sample records in fragmented tracks. Ordinary or mixed
// sample tables are deliberately excluded until their mux behavior is tested.
inline i64 samples(const Bytes& b,size_t begin,size_t end,unsigned depth=0,std::vector<unsigned>* sizes=nullptr){
 if(depth>4||end>b.size())throw std::runtime_error("Invalid MP4 nesting.");i64 count=0;
 for(auto at=begin;at<end;){auto n=be32(b,at);if(n<8||n>end-at)throw std::runtime_error("Invalid MP4 box.");std::string type(b.begin()+at+4,b.begin()+at+8);
  if(type=="moof"||type=="traf")count+=samples(b,at+8,at+n,depth+1,sizes);
  if(type=="trun"){auto flags=be32(b,at+8)&0xffffff;auto entries=be32(b,at+12);if(!(flags&0x200)||entries>1000000)throw std::runtime_error("Sample sizes are unavailable.");size_t start=16+((flags&1)?4:0)+((flags&4)?4:0),width=4*((flags&0x100?1:0)+(flags&0x200?1:0)+(flags&0x400?1:0)+(flags&0x800?1:0));if(start+uint64_t(entries)*width!=n)throw std::runtime_error("Incomplete MP4 sample table.");if(sizes)for(unsigned i=0;i<entries;++i)sizes->push_back(be32(b,at+start+size_t(i)*width+(flags&0x100?4:0)));count+=entries;}
  at+=n;
 }return count;
}
// AV1 muxing strips some OBUs. Inspect every OBU boundary, preserving its
// headers and the frame prefix used by FFmpeg's parser to identify keyframes.
// Never apply a fixed per-frame byte correction.
struct Obu {size_t header=0;uint64_t payload=0;unsigned type=0;};
inline Obu obu(const Bytes& b,uint64_t remaining){
 if(b.empty()||(b[0]&0x81)||!(b[0]&2))throw std::runtime_error("Unsupported AV1 OBU header.");
 Obu o;o.type=(b[0]>>3)&15;size_t p=1+(b[0]&4?1:0);
 if(o.type==0||(o.type>=9&&o.type<=14)||p>=b.size()||((b[0]&4)&&(b[1]&7)))throw std::runtime_error("Invalid AV1 OBU.");
 for(unsigned i=0;i<8;++i){if(p>=b.size())throw std::runtime_error("Truncated AV1 length.");auto v=b[p++];o.payload|=uint64_t(v&127)<<(7*i);if(!(v&128)){o.header=p;if(p>remaining||o.payload>remaining-p)throw std::runtime_error("Invalid AV1 sample boundary.");return o;}}
 throw std::runtime_error("Invalid AV1 length.");
}
struct Track {i64 samples=0,networkBytes=0;std::string codec;};
inline Track collect(const Json& data,const Json& prefs,const fs::path& output,const Cancel& cancel){
 const auto address=str(data,"Url");auto q=query(address);auto length=q.at("clen");if(length.empty()||length.size()>10||length.find_first_not_of("0123456789")!=std::string::npos)throw std::runtime_error("Unsupported media length.");auto size=std::stoll(length);if(size<8||size>256*1024*1024)throw std::runtime_error("Media preview size limit.");
 auto pool=std::make_shared<HttpSession>(prefs);auto headers=readHeaders(data);Track result;unsigned requests=0;i64 metadataBytes=0;
 auto read=[&](i64 offset,size_t n){
  cancel.check();if(!n||n>2*1024*1024||++requests>(result.codec=="av01"?20000u:400u)||result.networkBytes+i64(n)>(result.codec=="av01"?8:3)*1024*1024)throw std::runtime_error("Media preview request limit.");
  Http response(address,headers,prefs,cancel,offset,offset+i64(n)-1,"",nullptr,false,pool);
  if(response.status!=206||trim(response.header(L"Content-Range"))!="bytes "+std::to_string(offset)+"-"+std::to_string(offset+i64(n)-1)+"/"+std::to_string(size))throw std::runtime_error("Media metadata range unavailable.");
  auto encoding=lower(trim(response.header(L"Content-Encoding")));if(!encoding.empty()&&encoding!="identity")throw std::runtime_error("Encoded media range.");
  auto b=response.all(n,cancel);if(b.size()!=n)throw std::runtime_error("Short media metadata range.");result.networkBytes+=n;return b;
 };
 std::ofstream file(output,std::ios::binary|std::ios::trunc);if(!file)throw std::runtime_error("Cannot create media preview.");
 auto write=[&](i64 at,const Bytes& b){file.seekp(at);file.write(reinterpret_cast<const char*>(b.data()),b.size());if(!file)throw std::runtime_error("Cannot write media preview.");};
 bool moov=false,codecProbe=false;unsigned fragments=0;std::vector<unsigned> pending;
 for(i64 at=0;at<size;){auto h=read(at,8);auto n=be32(h,0);if(n<8||n>size-at)throw std::runtime_error("Unsupported MP4 box length.");std::string type(h.begin()+4,h.end());write(at,h);
  if(type=="ftyp"||type=="moov"||type=="moof"||type=="sidx"||type=="styp"){
   metadataBytes+=n;if(metadataBytes>2*1024*1024)throw std::runtime_error("Media metadata limit.");if(n>8){auto tail=read(at+8,n-8);write(at+8,tail);h.insert(h.end(),tail.begin(),tail.end());}
   if(type=="moov"){if(moov||initialization(h,0,h.size(),0,&result.codec)!=1)throw std::runtime_error("Unsupported initialization.");moov=true;}
   if(type=="moof"){if(!pending.empty())throw std::runtime_error("Missing fragment payload.");++fragments;result.samples+=samples(h,0,h.size(),0,result.codec=="av01"?&pending:nullptr);}
  }else if(type=="mdat"){
   if(!codecProbe){if(!moov||!fragments||n<=8)throw std::runtime_error("Missing media initialization.");write(at+8,read(at+8,std::min<size_t>(n-8,262144)));codecProbe=true;}
   if(result.codec=="av01"){
    i64 pos=at+8,cachedAt=pos;Bytes cached;
    for(auto sampleSize:pending){if(!sampleSize||sampleSize>at+n-pos)throw std::runtime_error("Invalid AV1 sample sizes.");
     const auto end=pos+sampleSize;unsigned units=0;
     while(pos<end){if(++units>128)throw std::runtime_error("Too many AV1 OBUs.");if(cached.empty()||pos-cachedAt>=i64(cached.size())||cachedAt+i64(cached.size())-pos<std::min<i64>(256,end-pos)){cachedAt=pos;cached=read(pos,std::min<i64>(at+n-pos,512));write(pos,cached);}Bytes prefix(cached.begin()+size_t(pos-cachedAt),cached.end());auto unit=obu(prefix,end-pos);
      // Sequence, standalone frame-header and metadata OBUs must be complete;
      // frame/tile payloads are not needed for a stream-copy size calculation.
      auto obuLength=unit.header+unit.payload;if(unit.type==1||unit.type==3||unit.type==5||unit.type==7){if(obuLength>4096)throw std::runtime_error("AV1 header preview limit.");if(obuLength>prefix.size())write(pos,read(pos,size_t(obuLength)));}
      pos+=obuLength;
     }
    }if(pos!=at+n)throw std::runtime_error("Incomplete AV1 sample mapping.");pending.clear();
   }
  }else if(type!="free")throw std::runtime_error("Unsupported MP4 structure.");at+=n;
 }
 if(!moov||!fragments||!codecProbe||!pending.empty()||result.samples<=0)throw std::runtime_error("Complete sample metadata unavailable.");file.close();fs::resize_file(output,size);return result;
}
}
inline Json probeMediaSize(const Json& data,const Json& prefs,const Cancel& cancel){
 if(!canPreviewMedia(data))return {{"Status","Unavailable"}};
 const auto root=fs::temp_directory_path()/wide("udm-size-"+guid());if(!fs::create_directory(root))throw std::runtime_error("Cannot create a private media preview directory.");
 struct Cleanup {fs::path root;~Cleanup(){std::error_code ec;fs::remove_all(root,ec);}} cleanup{root};
 auto video=root/L"video.mp4",audio=root/L"audio.mp4",output=root/L"count.mp4";
 auto future=std::async(std::launch::async,[&]{return media_preview::collect(data.at("Video"),prefs,video,cancel);});
 auto a=media_preview::collect(data.at("Audio"),prefs,audio,cancel);auto v=future.get();cancel.check();
 auto tools=appDir()/L"tools";
 auto diagnostic=execute(tools/L"ffmpeg.exe",{L"-hide_banner",L"-loglevel",L"error",L"-nostdin",L"-y",L"-protocol_whitelist",L"file,pipe",L"-fflags",v.codec=="av01"?L"0":L"+noparse+nofillin",L"-i",video.wstring(),L"-protocol_whitelist",L"file,pipe",L"-fflags",L"+noparse+nofillin",L"-i",audio.wstring(),L"-map",L"0:v:0",L"-map",L"1:a:0",L"-c",L"copy",L"-movflags",L"+faststart",output.wstring()},20,cancel);
 if(v.codec=="av01"&&!trim(diagnostic).empty())throw std::runtime_error("Incomplete AV1 parser metadata.");
 auto probe=Json::parse(execute(tools/L"ffprobe.exe",{L"-v",L"fatal",L"-fflags",L"+noparse+nofillin",L"-count_packets",L"-show_entries",L"stream=codec_type,codec_name,height,nb_read_packets",L"-of",L"json",output.wstring()},20,cancel));
 auto streams=probe.value("streams",Json::array());if(streams.size()!=2||str(streams[0],"codec_name")!=(v.codec=="av01"?"av1":"h264")||str(streams[1],"codec_name")!="aac"||str(streams[0],"nb_read_packets")!=std::to_string(v.samples)||str(streams[1],"nb_read_packets")!=std::to_string(a.samples)||num(streams[0],"height")!=num(data,"MediaPixelHeight"))throw std::runtime_error("Mux metadata did not preserve the selected samples.");
 return {{"Status","Ready"},{"Size",fs::file_size(output)},{"ContentType","video/mp4"},{"CalculatedMediaSize",true},{"MetadataBytes",a.networkBytes+v.networkBytes},{"VideoSamples",v.samples},{"AudioSamples",a.samples}};
}
}
