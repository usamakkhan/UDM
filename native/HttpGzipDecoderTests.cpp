#include "Core.hpp"
#include "HttpGzipDecoder.hpp"
#include <iostream>
using namespace udm;
static Bytes gzip(const Bytes& data){
 // Independent stored-block fixture writer; production links only zlib inflate.
 Bytes bytes={0x1f,0x8b,8,0,0,0,0,0,0,255};size_t at=0;
 do{const auto count=std::min<size_t>(65535,data.size()-at);bytes.push_back(at+count==data.size()?1:0);
  for(auto value:{static_cast<unsigned>(count),static_cast<unsigned>(count)^65535u}){bytes.push_back(static_cast<unsigned char>(value));bytes.push_back(static_cast<unsigned char>(value>>8));}
  bytes.insert(bytes.end(),data.begin()+at,data.begin()+at+count);at+=count;
 }while(at<data.size());
 for(auto value:{static_cast<uint32_t>(crc32(0,data.data(),static_cast<uInt>(data.size()))),static_cast<uint32_t>(data.size())})
  for(int byte=0;byte<4;++byte)bytes.push_back(static_cast<unsigned char>(value>>(8*byte)));
 return bytes;
}
int main(int argc,char** argv){
 Json checks=Json::array();bool passed=false;std::string failure;
 auto check=[&](bool value,const char* name){checks.push_back({{"name",name},{"passed",value}});if(!value)throw std::runtime_error(name);};
 try{
  Bytes original(200000);for(size_t i=0;i<original.size();++i)original[i]=static_cast<unsigned char>((i*37+i/19)%256);
  const auto wire=gzip(original);
  // Generated independently with Node zlib.gzipSync(Buffer.alloc(200000,65)).
  const std::string compressedHex="1f8b080000000000000aedc13101000000c2a06ceb5fca0cfe400100000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000af01d16aebac400d0300";Bytes packed;
  for(size_t at=0;at<compressedHex.size();at+=2)packed.push_back(static_cast<unsigned char>(std::stoul(compressedHex.substr(at,2),nullptr,16)));
  HttpGzipDecoder compressedDecoder;Bytes expanded;
  compressedDecoder.push(packed.data(),packed.size(),[&](const unsigned char* data,size_t count){expanded.insert(expanded.end(),data,data+count);});compressedDecoder.finish();
  check(expanded==Bytes(200000,65),"Compressed dynamic-Huffman input drains multiple bounded output buffers");
  for(size_t chunk:{size_t(1),size_t(7),size_t(4096),wire.size()}){
   HttpGzipDecoder decoder;Bytes output;size_t largest=0;
   auto emit=[&](const unsigned char* data,size_t count){largest=std::max(largest,count);output.insert(output.end(),data,data+count);};
   for(size_t at=0;at<wire.size();at+=chunk)decoder.push(wire.data()+at,std::min(chunk,wire.size()-at),emit);
   decoder.finish();decoder.finish();
   check(output==original&&decoder.outputBytes()==original.size()&&decoder.memberCount()==1&&largest<=65536,"Arbitrary wire boundaries produce exact bounded decoded output");
  }
  auto joined=wire,tail=gzip(Bytes{'t','a','i','l'});joined.insert(joined.end(),tail.begin(),tail.end());
  HttpGzipDecoder combined;Bytes output;auto emit=[&](const unsigned char* data,size_t count){output.insert(output.end(),data,data+count);};
  combined.push(joined.data(),joined.size(),emit);combined.finish();auto expected=original;expected.insert(expected.end(),{'t','a','i','l'});
  check(output==expected&&combined.memberCount()==2,"Concatenated gzip members preserve all output");
  const auto empty=gzip({});HttpGzipDecoder emptyDecoder(0);emptyDecoder.push(empty.data(),empty.size(),[](const unsigned char*,size_t){throw std::runtime_error("Unexpected output");});emptyDecoder.finish();
  check(emptyDecoder.outputBytes()==0&&emptyDecoder.memberCount()==1,"A valid empty gzip file remains valid");
  for(const auto* kind:{"truncated","checksum","trailing","raw","empty-wire","limit","members","canceled","write-failure"}){
   Bytes input=wire;if(std::string(kind)=="truncated")input.pop_back();
   if(std::string(kind)=="checksum")input[input.size()-8]^=1;
   if(std::string(kind)=="trailing")input.push_back(0xff);
   if(std::string(kind)=="raw")input=original;
   if(std::string(kind)=="empty-wire")input.clear();
   if(std::string(kind)=="members")input=joined;
   HttpGzipDecoder decoder(std::string(kind)=="limit"?100:original.size()+100,1);bool rejected=false,poisoned=false;int calls=0;
   try{decoder.push(input.data(),input.size(),[&](const unsigned char*,size_t){if(std::string(kind)=="write-failure")throw std::runtime_error("Injected write failure");},[&]{if(std::string(kind)=="canceled"&&++calls>=2)throw std::runtime_error("Injected cancellation");});decoder.finish();}catch(...){rejected=true;}
   try{decoder.finish();}catch(...){poisoned=true;}
   check(rejected&&poisoned,kind);
  }
  bool rejected=false;try{combined.push(wire.data(),wire.size(),emit);}catch(...){rejected=true;}
  check(rejected,"Data after final completion is rejected");passed=true;
 }catch(const std::exception& error){failure=error.what();}
 if(argc>1){const auto root=fs::path(wide(argv[1]));fs::create_directories(root);atomicText(root/L"results.json",Json{{"passed",passed},{"error",failure},{"checks",checks}}.dump(2),false);}
 std::cout<<checks.size()<<" checks; "<<(passed?"passed":failure)<<"\n";return passed?0:1;
}
