#pragma once
#include "Core.hpp"
#include <fstream>
#include <algorithm>
namespace udm {
inline constexpr uint64_t ZipDirectoryLimit=64ull*1024*1024;
inline constexpr uint64_t ZipPreviewBudget=ZipDirectoryLimit+256*1024;
inline constexpr uint64_t ZipFallbackLimit=8ull*1024*1024;
struct ZipItem {std::string name;uint64_t size=0,compressed=0;uint16_t method=0;bool encrypted=false;};
struct ZipSource {uint64_t length=0;bool validated=false;std::function<Bytes(uint64_t,size_t)> read;std::function<void()> verify;};
inline uint64_t zipNumber(const Bytes& b,size_t at,size_t count){if(count>8||at>b.size()||count>b.size()-at)throw std::runtime_error("Malformed ZIP directory.");uint64_t v=0;for(size_t i=0;i<count;++i)v|=(uint64_t)b[at+i]<<(8*i);return v;}
inline uint32_t zipNameCrc(const std::string& name){uint32_t crc=~0u;for(unsigned char b:name){crc^=b;for(int i=0;i<8;++i)crc=(crc>>1)^((crc&1)?0xedb88320u:0);}return ~crc;}
inline std::vector<ZipItem> zipContents(ZipSource& source,const Cancel& cancel){
 auto read=[&](uint64_t offset,size_t count){cancel.check();if(offset>source.length||count>source.length-offset)throw std::runtime_error("ZIP directory points outside this archive.");auto bytes=source.read(offset,count);if(bytes.size()!=count)throw std::runtime_error("The ZIP directory response was truncated.");return bytes;};
 auto n=zipNumber;auto tail=read(source.length>65557?source.length-65557:0,(size_t)std::min<uint64_t>(65557,source.length));size_t end=std::string::npos;
 for(size_t i=tail.size();i>=22;--i){auto at=i-22;if(n(tail,at,4)==0x06054b50&&at+22+n(tail,at+20,2)==tail.size()){end=at;break;}}
 if(end==std::string::npos)throw std::runtime_error("ZIP end-of-directory record was not found.");const uint64_t footer=source.length-tail.size()+end;
 if(n(tail,end+4,2)||n(tail,end+6,2)||n(tail,end+8,2)!=n(tail,end+10,2))throw std::runtime_error("Split ZIP archives require their archive application.");
 uint64_t count=n(tail,end+10,2),size=n(tail,end+12,4),offset=n(tail,end+16,4),directoryEnd=footer;
 if(count==65535||size==0xffffffff||offset==0xffffffff){
  if(footer<20)throw std::runtime_error("Missing ZIP64 locator.");auto locator=read(footer-20,20);if(n(locator,0,4)!=0x07064b50||n(locator,4,4)||n(locator,16,4)!=1)throw std::runtime_error("Unsupported ZIP64 locator.");
  auto at=n(locator,8,8);auto record=read(at,56);auto recordSize=n(record,4,8);
  if(n(record,0,4)!=0x06064b50||recordSize<44||at>footer-20||recordSize>footer-20-at||12>footer-20-at-recordSize||at+12+recordSize!=footer-20||n(record,16,4)||n(record,20,4)||n(record,24,8)!=n(record,32,8))throw std::runtime_error("Invalid ZIP64 directory.");
  auto entries=n(record,32,8),bytes=n(record,40,8),start=n(record,48,8);if((count!=65535&&count!=entries)||(size!=0xffffffff&&size!=bytes)||(offset!=0xffffffff&&offset!=start))throw std::runtime_error("Conflicting ZIP and ZIP64 directory records.");count=entries;size=bytes;offset=start;directoryEnd=at;
 }
 if(count>100000||size>ZipDirectoryLimit)throw std::runtime_error("ZIP preview is limited to 100,000 entries and a 64 MB directory.");
 if(offset>directoryEnd||size>directoryEnd-offset||count>size/46)throw std::runtime_error("Invalid ZIP directory bounds or entry count.");
 auto directory=read(offset,(size_t)size);std::vector<ZipItem> items;items.reserve((size_t)count);size_t at=0;
 for(uint64_t i=0;i<count;++i){
  if(!(i%64))cancel.check();if(n(directory,at,4)!=0x02014b50)throw std::runtime_error("Invalid ZIP entry.");
  auto flags=n(directory,at+8,2),nameLength=n(directory,at+28,2),extraLength=n(directory,at+30,2),commentLength=n(directory,at+32,2),disk=n(directory,at+34,2),local=n(directory,at+42,4);size_t next=at+46+(size_t)(nameLength+extraLength+commentLength);if(next>directory.size())throw std::runtime_error("Truncated ZIP entry.");
  ZipItem item;item.method=(uint16_t)n(directory,at+10,2);item.encrypted=(flags&1)!=0;item.compressed=n(directory,at+20,4);item.size=n(directory,at+24,4);std::string name((char*)directory.data()+at+46,(size_t)nameLength),unicode;bool zip64=false;
  const auto extraEnd=at+46+(size_t)nameLength+(size_t)extraLength;
  for(size_t e=at+46+(size_t)nameLength;e<extraEnd;){
   if(extraEnd-e<4)throw std::runtime_error("Truncated ZIP extra field.");auto kind=n(directory,e,2),amount=n(directory,e+2,2);e+=4;if(amount>extraEnd-e)throw std::runtime_error("Malformed ZIP extra data.");
   if(kind==1){if(zip64)throw std::runtime_error("Duplicate ZIP64 extra field.");zip64=true;size_t value=e;auto take=[&](int width){if((size_t)width>e+amount-value)throw std::runtime_error("Missing ZIP64 entry metadata.");auto v=n(directory,value,width);value+=width;return v;};if(item.size==0xffffffff)item.size=take(8);if(item.compressed==0xffffffff)item.compressed=take(8);if(local==0xffffffff)local=take(8);if(disk==65535)disk=take(4);}
   if(kind==0x7075&&amount>=5&&n(directory,e,1)==1&&n(directory,e+1,4)==zipNameCrc(name)){auto value=std::string((char*)directory.data()+e+5,(size_t)amount-5);wide(value);if(!unicode.empty()&&unicode!=value)throw std::runtime_error("Conflicting ZIP Unicode names.");unicode=std::move(value);}e+=(size_t)amount;
  }
  if(!zip64&&(item.size==0xffffffff||item.compressed==0xffffffff||local==0xffffffff||disk==65535))throw std::runtime_error("Missing ZIP64 entry metadata.");if(disk)throw std::runtime_error("Split ZIP archives require their archive application.");
  if(flags&0x800){wide(name);item.name=name;}else if(!unicode.empty())item.name=unicode;else{int len=MultiByteToWideChar(437,0,name.data(),(int)name.size(),nullptr,0);std::wstring decoded(len,0);if(len)MultiByteToWideChar(437,0,name.data(),(int)name.size(),decoded.data(),len);item.name=utf8(decoded);}
  if(item.name.empty()||item.name.find('\0')!=std::string::npos)throw std::runtime_error("Invalid ZIP file name.");items.push_back(std::move(item));at=next;
 }
 if(at<directory.size()){if(directory.size()-at<6||n(directory,at,4)!=0x05054b50||n(directory,at+4,2)!=directory.size()-at-6)throw std::runtime_error("Unexpected bytes after ZIP directory entries.");}
 cancel.check();if(source.verify)source.verify();cancel.check();return items;
}
inline std::vector<ZipItem> zipContents(const fs::path& path){
 std::ifstream file(path,std::ios::binary);if(!file)throw std::runtime_error("Cannot open the ZIP file.");file.seekg(0,std::ios::end);auto length=file.tellg();if(length<0)throw std::runtime_error("Cannot read the ZIP file size.");ZipSource source;source.length=(uint64_t)length;
 source.read=[&](uint64_t at,size_t count){Bytes bytes(count);file.seekg((std::streamoff)at);if(count)file.read((char*)bytes.data(),count);if(!file)throw std::runtime_error("Cannot read ZIP directory.");return bytes;};return zipContents(source,Cancel());
}
bool canPreviewZip(const Json&,const std::string& mime="");
struct ZipListing {std::vector<ZipItem> entries;uint64_t size=0,received=0;bool validated=false;};
ZipListing previewRemoteZip(const Json&,const Json&,const Cancel&,std::function<void(const Json&)> saveBrowserSession={});
struct ZipPreviewState {std::string status="Checking",message;std::shared_ptr<const ZipListing> listing;};
class ZipPreviewTask {
 std::shared_ptr<Cancel> cancel=std::make_shared<Cancel>();mutable std::mutex mutex;ZipPreviewState state;std::thread worker;
public:
 ZipPreviewTask(Json,Json,std::function<void(const Json&)> saveBrowserSession={},int timeoutMs=30000);
 ~ZipPreviewTask();void stop()noexcept;ZipPreviewState snapshot()const;
 ZipPreviewTask(const ZipPreviewTask&)=delete;ZipPreviewTask& operator=(const ZipPreviewTask&)=delete;
};
}
