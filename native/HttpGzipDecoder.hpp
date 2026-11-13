#pragma once
#include "vendor/zlib/zlib.h"
#include <algorithm>
#include <array>
#include <cstdint>
#include <functional>
#include <limits>
#include <stdexcept>

namespace udm {
class HttpGzipFormatError:public std::runtime_error{public:using std::runtime_error::runtime_error;};
// Incremental representation decoding. The transport retains ownership of wire
// accounting and checkpoints; decoded bytes must not become HTTP range offsets.
class HttpGzipDecoder {
 z_stream stream{};
 std::array<unsigned char,65536> buffer{};
 uint64_t output=0,limit;
 size_t members=0,memberLimit;
 bool memberEnded=false,finished=false,failed=false;
 void usable()const{if(failed)throw std::runtime_error("The gzip decoder has failed.");}
 void nextMember(){
  const auto next=stream.next_in;const auto available=stream.avail_in;
  if(inflateReset2(&stream,31)!=Z_OK)throw std::runtime_error("Cannot reset the HTTP gzip decoder.");
  stream.next_in=next;stream.avail_in=available;memberEnded=false;
 }
public:
 using Emit=std::function<void(const unsigned char*,size_t)>;
 using Checkpoint=std::function<void()>;
 explicit HttpGzipDecoder(uint64_t outputLimit=std::numeric_limits<uint64_t>::max(),size_t maximumMembers=1024)
  :limit(outputLimit),memberLimit(maximumMembers){
  if(!memberLimit)throw std::invalid_argument("The gzip member limit must be positive.");
  if(inflateInit2(&stream,31)!=Z_OK)throw std::runtime_error("Cannot initialize the HTTP gzip decoder.");
 }
 ~HttpGzipDecoder(){inflateEnd(&stream);}
 HttpGzipDecoder(const HttpGzipDecoder&)=delete;
 HttpGzipDecoder& operator=(const HttpGzipDecoder&)=delete;
 uint64_t outputBytes()const{return output;}
 size_t memberCount()const{return members;}
 void push(const unsigned char* data,size_t size,const Emit& emit,const Checkpoint& check={}){
  usable();if(finished&&size)throw std::logic_error("HTTP gzip input arrived after completion.");
  if(size&&!data)throw std::invalid_argument("Missing HTTP gzip input.");
  try{
   if(check)check();
   while(size){
    const auto count=static_cast<uInt>(std::min<size_t>(size,std::numeric_limits<uInt>::max()));
    stream.next_in=const_cast<Bytef*>(data);stream.avail_in=count;
    if(memberEnded)nextMember();
    for(;;){
     if(check)check();
     const auto before=stream.avail_in;
     stream.next_out=buffer.data();stream.avail_out=static_cast<uInt>(buffer.size());
     const auto result=inflate(&stream,Z_NO_FLUSH);
     const auto produced=buffer.size()-stream.avail_out;
     if(result!=Z_OK&&result!=Z_STREAM_END&&result!=Z_BUF_ERROR)
      throw HttpGzipFormatError("Invalid HTTP gzip data or checksum.");
     if(produced>limit-output)throw std::runtime_error("Decoded HTTP data exceeds its output limit.");
     if(produced){if(check)check();emit(buffer.data(),produced);output+=produced;}
     if(result==Z_STREAM_END){
      memberEnded=true;if(++members>memberLimit)throw HttpGzipFormatError("Too many HTTP gzip members.");
      if(!stream.avail_in)break;
      nextMember();continue;
     }
     if(!stream.avail_in&&produced<buffer.size())break;
     if(before==stream.avail_in&&!produced)throw HttpGzipFormatError("HTTP gzip decoding made no progress.");
    }
    if(stream.avail_in)throw HttpGzipFormatError("HTTP gzip input was not consumed.");
    data+=count;size-=count;
   }
   stream.next_in=nullptr;
  }catch(...){failed=true;throw;}
 }
 void finish(const Checkpoint& check={}){
  usable();
  try{if(check)check();if(!memberEnded||!members)throw HttpGzipFormatError("Truncated HTTP gzip response.");finished=true;}
  catch(...){failed=true;throw;}
 }
};
}
