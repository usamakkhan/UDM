#include "Core.hpp"
#include "Id3v2.hpp"
#include <iostream>
using namespace udm;
using Data=id3::Data;
static void append(Data& to,const Data& from){to.insert(to.end(),from.begin(),from.end());}
static Data number(uint32_t n,bool sync){return sync?Data{static_cast<unsigned char>((n>>21)&127),static_cast<unsigned char>((n>>14)&127),static_cast<unsigned char>((n>>7)&127),static_cast<unsigned char>(n&127)}:Data{static_cast<unsigned char>(n>>24),static_cast<unsigned char>(n>>16),static_cast<unsigned char>(n>>8),static_cast<unsigned char>(n)};}
static Data frame(int v,const char* name,const Data& payload,unsigned char flags=0){
 Data result(name,name+(v==2?3:4));auto size=number(static_cast<uint32_t>(payload.size()),v==4);
 if(v==2)size.erase(size.begin());append(result,size);if(v!=2)append(result,{0,flags});append(result,payload);return result;
}
static Data tag(int v,const Data& payload,unsigned char flags=0){
 Data result{'I','D','3',static_cast<unsigned char>(v),0,flags};append(result,number(static_cast<uint32_t>(payload.size()),true));append(result,payload);
 if(v==4&&(flags&16)){Data footer{'3','D','I',4,0,flags};append(footer,number(static_cast<uint32_t>(payload.size()),true));append(result,footer);}return result;
}
int main(int argc,char** argv){unsigned passed=0;try{
 if(argc!=2)throw std::runtime_error("Fresh fixture directory required");fs::path root=wide(argv[1]);if(fs::exists(root))throw std::runtime_error("Fixture exists");fs::create_directories(root);
 auto check=[&](bool ok,const char* name){if(!ok)throw std::runtime_error(name);++passed;};
 auto read=[&](const Data& bytes){auto path=root/L"fixture.mp3";{std::ofstream f(path,std::ios::binary);f.write(reinterpret_cast<const char*>(bytes.data()),bytes.size());}return readAudioMetadata(path);};
 Data text{0,'T','i','t','l','e'};
 for(int v:{2,3,4}){
  auto bytes=tag(v,frame(v,v==2?"TT2":"TIT2",text));auto m=read(bytes);
  check(m.title=="Title"&&m.source=="ID3v2."+std::to_string(v),"version text");
  bytes.pop_back();check(read(bytes).title.empty(),"truncated tag");
  check(read(tag(v,frame(v,v==2?"TT2":"TIT2",{1,255,254,0,216}))).title.empty(),"unpaired surrogate");
  check(read(tag(v,frame(v,v==2?"TT2":"TIT2",{1,255,254,60,216,181,223}))).title=="\xf0\x9f\x8e\xb5","surrogate pair");
 }
 // zlib-compressed encoding-byte + Title, independently generated with Python zlib.
 const Data compressed{120,156,99,8,201,44,201,73,5,0,5,231,2,3};
 for(int v:{3,4}){
  auto payload=number(6,v==4);append(payload,compressed);
  check(read(tag(v,frame(v,"TIT2",payload,v==3?128:9))).title=="Title","compressed text");
  auto grouped=number(6,v==4);if(v==3)grouped.push_back(7);else grouped.insert(grouped.begin(),7);append(grouped,compressed);
  check(read(tag(v,frame(v,"TIT2",grouped,v==3?160:73))).title=="Title","compressed group");
  payload.push_back(0);check(read(tag(v,frame(v,"TIT2",payload,v==3?128:9))).title.empty(),"trailing zlib data");
  payload=number(65537,v==4);append(payload,compressed);check(read(tag(v,frame(v,"TIT2",payload,v==3?128:9))).title.empty(),"decompression bound");
  payload=number(5,v==4);append(payload,compressed);check(read(tag(v,frame(v,"TIT2",payload,v==3?128:9))).title.empty(),"wrong expanded length");
 }
 check(read(tag(4,frame(4,"TIT2",compressed,8))).title.empty(),"compression requires DLI");
 auto appended=Data(512,0);append(appended,tag(4,frame(4,"TIT2",text),16));check(read(appended).title=="Title","appended tag");
 appended.back()^=1;check(read(appended).title.empty(),"footer mismatch");
 Data legacy(128,0);legacy[0]='T';legacy[1]='A';legacy[2]='G';legacy[33]='A';auto mixed=tag(4,frame(4,"TIT2",text));append(mixed,legacy);auto m=read(mixed);
 check(m.title=="Title"&&m.artist=="A"&&m.source=="ID3v2.4 + ID3v1","legacy fallback");
 check(read(tag(4,frame(4,"TIT2",{3,192,175}))).title.empty(),"overlong UTF8");
 std::cout<<passed<<" passed, 0 failed\n";return 0;
 }catch(const std::exception& e){std::cerr<<e.what()<<'\n';std::cout<<passed<<" passed, 1 failed\n";return 1;}}
