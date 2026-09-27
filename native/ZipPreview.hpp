#pragma once
#include "Core.hpp"
#include <fstream>
namespace udm {
struct ZipItem {std::string name;uint64_t size=0,compressed=0;uint16_t method=0;bool encrypted=false;};
inline std::vector<ZipItem> zipContents(const fs::path& path){
 std::ifstream file(path,std::ios::binary);if(!file)throw std::runtime_error("Cannot open the ZIP file.");file.seekg(0,std::ios::end);auto length=(uint64_t)file.tellg();
 auto read=[&](uint64_t offset,size_t count){if(offset>length||count>length-offset)throw std::runtime_error("Truncated ZIP directory.");Bytes result(count);file.seekg((std::streamoff)offset);file.read((char*)result.data(),count);if(!file)throw std::runtime_error("Cannot read ZIP directory.");return result;};
 auto n=[](const Bytes& b,size_t at,size_t count)->uint64_t{if(at>b.size()||count>b.size()-at)throw std::runtime_error("Malformed ZIP directory.");uint64_t value=0;for(size_t i=0;i<count;++i)value|=(uint64_t)b[at+i]<<(i*8);return value;};
 auto tail=read(length>65557?length-65557:0,(size_t)std::min<uint64_t>(65557,length));size_t end=std::string::npos;
 for(size_t i=tail.size();i>=22;--i){size_t at=i-22;if(n(tail,at,4)==0x06054b50&&at+22+n(tail,at+20,2)==tail.size()){end=at;break;}}
 if(end==std::string::npos)throw std::runtime_error("ZIP end-of-directory record was not found.");
 if(n(tail,end+4,2)||n(tail,end+6,2))throw std::runtime_error("Split ZIP archives require their archive application.");
 uint64_t count=n(tail,end+10,2),size=n(tail,end+12,4),offset=n(tail,end+16,4);
 if(count==65535||size==0xffffffff||offset==0xffffffff){auto absolute=length-tail.size()+end;if(absolute<20)throw std::runtime_error("Missing ZIP64 locator.");auto locator=read(absolute-20,20);if(n(locator,0,4)!=0x07064b50||n(locator,4,4)||n(locator,16,4)!=1)throw std::runtime_error("Unsupported ZIP64 locator.");auto record=read(n(locator,8,8),56);if(n(record,0,4)!=0x06064b50||n(record,16,4)||n(record,20,4))throw std::runtime_error("Invalid ZIP64 directory.");count=n(record,32,8);size=n(record,40,8);offset=n(record,48,8);}
 if(count>100000||size>64*1024*1024)throw std::runtime_error("ZIP preview is limited to 100,000 entries and a 64 MB directory.");
 auto directory=read(offset,(size_t)size);std::vector<ZipItem> items;size_t at=0;
 for(uint64_t i=0;i<count;++i){if(n(directory,at,4)!=0x02014b50)throw std::runtime_error("Invalid ZIP entry.");auto flags=n(directory,at+8,2),nameLength=n(directory,at+28,2),extraLength=n(directory,at+30,2),commentLength=n(directory,at+32,2);size_t next=at+46+(size_t)(nameLength+extraLength+commentLength);if(next>directory.size())throw std::runtime_error("Truncated ZIP entry.");ZipItem entry;entry.method=(uint16_t)n(directory,at+10,2);entry.encrypted=(flags&1)!=0;entry.compressed=n(directory,at+20,4);entry.size=n(directory,at+24,4);std::string name((char*)directory.data()+at+46,(size_t)nameLength);
  if(flags&0x800){wide(name);entry.name=name;}else{int len=MultiByteToWideChar(437,0,name.data(),(int)name.size(),nullptr,0);std::wstring decoded(len,0);if(len)MultiByteToWideChar(437,0,name.data(),(int)name.size(),decoded.data(),len);entry.name=utf8(decoded);}
  for(size_t e=at+46+(size_t)nameLength;e+4<=at+46+nameLength+extraLength;){auto kind=n(directory,e,2),amount=n(directory,e+2,2);e+=4;if(amount>at+46+nameLength+extraLength-e)throw std::runtime_error("Malformed ZIP extra data.");if(kind==1){size_t value=e;if(entry.size==0xffffffff){if(amount<8)throw std::runtime_error("Missing ZIP64 size.");entry.size=n(directory,value,8);value+=8;}if(entry.compressed==0xffffffff){if(value+8>e+amount)throw std::runtime_error("Missing ZIP64 compressed size.");entry.compressed=n(directory,value,8);}}e+=(size_t)amount;}
  if(entry.name.find('\0')!=std::string::npos)throw std::runtime_error("Invalid ZIP file name.");items.push_back(std::move(entry));at=next;
 }return items;
}
}
