#pragma once
#include <array>
#include <filesystem>
#include <fstream>
#include <string>
namespace udm {
struct AudioMetadata {std::string title,artist,album,source;};
// ID3v1 stores three fixed-width text fields in the final 128 bytes.
// https://id3.org/ID3v1. Interpret legacy text as Latin-1, returning UTF-8.
inline AudioMetadata readId3v1(const std::filesystem::path& path){
 std::ifstream input(path,std::ios::binary|std::ios::ate);
 if(!input||input.tellg()<128)return {};
 input.seekg(-128,std::ios::end);std::array<unsigned char,128> bytes{};
 if(!input.read(reinterpret_cast<char*>(bytes.data()),bytes.size())||bytes[0]!='T'||bytes[1]!='A'||bytes[2]!='G')return {};
 auto field=[&](size_t start){
  size_t end=start;while(end<start+30&&bytes[end])++end;
  while(end>start&&bytes[end-1]==' ')--end;
  std::string text;for(size_t at=start;at<end;++at){auto c=bytes[at];if(c<32||c==127)continue;if(c<128)text+=char(c);else{text+=char(0xc0|(c>>6));text+=char(0x80|(c&63));}}return text;
 };
 return {field(3),field(33),field(63),"ID3v1"};
}
}
