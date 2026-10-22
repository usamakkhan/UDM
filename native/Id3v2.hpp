#pragma once
#include "AudioMetadata.hpp"
#include <vector>
#include <algorithm>
#include <cstdint>
namespace udm { namespace id3 {
using Data=std::vector<unsigned char>;
inline bool integer(const unsigned char* p,size_t n,bool sync,uint32_t& out){out=0;for(size_t i=0;i<n;++i){if(sync&&(p[i]&128))return false;out=(out<<(sync?7:8))|p[i];}return true;}
inline Data unsync(const Data& in){Data out;out.reserve(in.size());for(size_t i=0;i<in.size();++i){out.push_back(in[i]);if(in[i]==255&&i+1<in.size()&&in[i+1]==0)++i;}return out;}
inline void utf8(std::string& out,uint32_t c){if(c<32||c==127)return;if(c<128)out+=char(c);else if(c<2048){out+=char(192|(c>>6));out+=char(128|(c&63));}else if(c<65536){out+=char(224|(c>>12));out+=char(128|((c>>6)&63));out+=char(128|(c&63));}else{out+=char(240|(c>>18));out+=char(128|((c>>12)&63));out+=char(128|((c>>6)&63));out+=char(128|(c&63));}}
inline std::string text(const Data& b,int version){
 if(b.empty()||b.size()>65536)return {};unsigned encoding=b[0];if(encoding>3||(version<4&&encoding>1))return {};
 size_t at=1;bool little=false;std::string out;
 if(encoding==1){if(b.size()<3)return {};if(b[1]==255&&b[2]==254)little=true;else if(b[1]!=254||b[2]!=255)return {};at=3;}
 while(at<b.size()){
  uint32_t c=b[at++];
  if(encoding==1||encoding==2){if(at==b.size())return {};c=little?c|(uint32_t(b[at])<<8):(c<<8)|b[at];++at;
   if(c>=0xd800&&c<=0xdbff){if(b.size()-at<2)return {};uint32_t lo=little?b[at]|(uint32_t(b[at+1])<<8):(uint32_t(b[at])<<8)|b[at+1];at+=2;if(lo<0xdc00||lo>0xdfff)return {};c=0x10000+((c-0xd800)<<10)+lo-0xdc00;}else if(c>=0xdc00&&c<=0xdfff)return {};
  }else if(encoding==3&&c>=128){unsigned count;uint32_t minimum;if(c>=194&&c<=223){count=1;minimum=128;c&=31;}else if(c>=224&&c<=239){count=2;minimum=2048;c&=15;}else if(c>=240&&c<=244){count=3;minimum=65536;c&=7;}else return {};
   if(b.size()-at<count)return {};for(unsigned i=0;i<count;++i){unsigned next=b[at++];if((next&192)!=128)return {};c=(c<<6)|(next&63);}if(c<minimum||c>0x10ffff||(c>=0xd800&&c<=0xdfff))return {};
  }
  if(c==0)break;utf8(out,c);if(out.size()>4096)return {};
 }
 while(!out.empty()&&out.back()==' ')out.pop_back();return out;
}
inline AudioMetadata parse(const Data& input,int version,unsigned flags){
 AudioMetadata result;Data bytes=(version<4&&(flags&128))?unsync(input):input;size_t at=0;
 if(version==2&&(flags&64))return {}; // v2.2 whole-tag compression is undefined.
 if(version>=3&&(flags&64)){if(bytes.size()<4)return {};uint32_t size=0;if(!integer(bytes.data(),4,version==4,size))return {};
  if(version==3){if(size!=6&&size!=10)return {};size+=4;}else if(size<6)return {};
  if(size>bytes.size())return {};at=size;
 }
 for(unsigned frames=0;at<bytes.size()&&frames<4096;++frames){
  if(bytes[at]==0)break;size_t header=version==2?6:10,idSize=version==2?3:4;if(bytes.size()-at<header)return {};
  std::string id;for(size_t i=0;i<idSize;++i){auto c=bytes[at+i];if(!((c>='A'&&c<='Z')||(c>='0'&&c<='9')))return {};id+=char(c);}
  uint32_t size=0;if(!integer(bytes.data()+at+idSize,version==2?3:4,version==4,size)||!size||size>bytes.size()-at-header)return {};
  unsigned status=version==2?0:bytes[at+8],format=version==2?0:bytes[at+9];
  if((version==3&&((status&31)||(format&31)))||(version==4&&((status&143)||(format&176))))return {};
  at+=header;size_t start=at;at+=size;
  std::string* field=(id=="TIT2"||id=="TT2")?&result.title:(id=="TPE1"||id=="TP1")?&result.artist:(id=="TALB"||id=="TAL")?&result.album:nullptr;
  if(!field||!field->empty())continue;
  if((version==3&&(format&192))||(version==4&&(format&12)))continue; // compressed/encrypted frames are not text.
  Data frame(bytes.begin()+start,bytes.begin()+at);if(version==4&&((flags&128)||(format&2)))frame=unsync(frame);
  size_t extra=((version==3&&(format&32))||(version==4&&(format&64)))?1:0;
  if(extra>frame.size())continue;
  if(version==4&&(format&1)){uint32_t length=0;if(frame.size()-extra<4||!integer(frame.data()+extra,4,true,length))continue;extra+=4;if(length!=frame.size()-extra)continue;}
  *field=text(Data(frame.begin()+extra,frame.end()),version);
 }
 return result;
}
}
inline AudioMetadata readAudioMetadata(const std::filesystem::path& path){
 auto fallback=readId3v1(path);std::ifstream file(path,std::ios::binary|std::ios::ate);if(!file)return fallback;auto length=file.tellg();file.seekg(0);std::array<unsigned char,10> header{};
 if(!file.read(reinterpret_cast<char*>(header.data()),10)||header[0]!='I'||header[1]!='D'||header[2]!='3')return fallback;
 int version=header[3];unsigned flags=header[5];uint32_t size=0;
 if(version<2||version>4||header[4]==255||(flags&(version==2?63:version==3?31:15))||!id3::integer(header.data()+6,4,true,size)||size>4*1024*1024||length<std::streamoff(10+size+(version==4&&(flags&16)?10:0)))return fallback;
 id3::Data bytes(size);if(size&&!file.read(reinterpret_cast<char*>(bytes.data()),size))return fallback;
 if(version==4&&(flags&16)){std::array<unsigned char,10> footer{};if(!file.read(reinterpret_cast<char*>(footer.data()),10)||footer[0]!='3'||footer[1]!='D'||footer[2]!='I'||!std::equal(header.begin()+3,header.end(),footer.begin()+3))return fallback;}
 auto result=id3::parse(bytes,version,flags);bool found=!result.title.empty()||!result.artist.empty()||!result.album.empty();
 if(!found)return fallback;
 bool mixed=false;if(result.title.empty()&&!fallback.title.empty()){result.title=fallback.title;mixed=true;}if(result.artist.empty()&&!fallback.artist.empty()){result.artist=fallback.artist;mixed=true;}if(result.album.empty()&&!fallback.album.empty()){result.album=fallback.album;mixed=true;}
 result.source="ID3v2."+std::to_string(version)+(mixed?" + ID3v1":"");return result;
}
}
