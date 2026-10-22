#pragma once
#include <cstdint>
#include <string>
#include <istream>
#include <utility>
namespace udm {
// One binary request: UDM1, four little-endian uint32 fields (script, URL,
// host byte lengths, execution budget in ms), then the three UTF-8 strings.
// Parent owns the pipe and the process deadline, including incomplete input.
struct PacRequest {
 std::string script, url, host;
 uint32_t budgetMs=0;
};
inline bool readPacRequest(std::istream& input,PacRequest& request){
 unsigned char header[20]{};
 if(!input.read(reinterpret_cast<char*>(header),sizeof(header)))return false;
 if(header[0]!='U'||header[1]!='D'||header[2]!='M'||header[3]!='1')return false;
 auto field=[&](size_t at){return uint32_t(header[at])|(uint32_t(header[at+1])<<8)|(uint32_t(header[at+2])<<16)|(uint32_t(header[at+3])<<24);};
 const auto scriptBytes=field(4),urlBytes=field(8),hostBytes=field(12),budget=field(16);
 if(!scriptBytes||scriptBytes>65536||!urlBytes||urlBytes>8192||!hostBytes||hostBytes>255||!budget||budget>2000)return false;
 PacRequest value;value.budgetMs=budget;
 auto read=[&](std::string& text,uint32_t size){text.resize(size);return bool(input.read(text.data(),size))&&text.find('\0')==std::string::npos;};
 if(!read(value.script,scriptBytes)||!read(value.url,urlBytes)||!read(value.host,hostBytes))return false;
 if(value.url.find_first_of("\r\n\t ")!=std::string::npos||value.host.find_first_of("\r\n\t /\\")!=std::string::npos)return false;
 request=std::move(value);return true;
}
}
