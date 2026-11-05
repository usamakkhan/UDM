#pragma once
#include "Core.hpp"
#include <bcrypt.h>
#include <algorithm>
#include <optional>
#include <regex>

namespace udm {
inline bool dashStrongEntityTag(const std::string& value){return value.size()>=2&&value.size()<=1024&&value.front()=='"'&&value.back()=='"'&&std::none_of(value.begin()+1,value.end()-1,[](unsigned char c){return c<0x21||c==0x22||c>=0x7f;});}
inline std::optional<i64> dashPrefixHttpDate(const std::string& value){
 // WinHTTP also accepts incomplete dates. Require a complete HTTP-date before
 // using its calendar conversion; no local wall clock participates in proof.
 static const std::regex format(R"(^(?:(?:Mon|Tue|Wed|Thu|Fri|Sat|Sun), [0-9]{2} (?:Jan|Feb|Mar|Apr|May|Jun|Jul|Aug|Sep|Oct|Nov|Dec) [0-9]{4} [0-2][0-9]:[0-5][0-9]:[0-5][0-9] GMT|(?:Monday|Tuesday|Wednesday|Thursday|Friday|Saturday|Sunday), [0-9]{2}-(?:Jan|Feb|Mar|Apr|May|Jun|Jul|Aug|Sep|Oct|Nov|Dec)-[0-9]{2} [0-2][0-9]:[0-5][0-9]:[0-5][0-9] GMT|(?:Mon|Tue|Wed|Thu|Fri|Sat|Sun) (?:Jan|Feb|Mar|Apr|May|Jun|Jul|Aug|Sep|Oct|Nov|Dec) (?: [0-9]|[0-9]{2}) [0-2][0-9]:[0-5][0-9]:[0-5][0-9] [0-9]{4})$)");
 if(value.size()>128||!std::regex_match(value,format))return {};
 SYSTEMTIME parsed{},roundtrip{};FILETIME stamp{};
 if(!WinHttpTimeToSystemTime(wide(value).c_str(),&parsed)||parsed.wHour>23||!SystemTimeToFileTime(&parsed,&stamp)||!FileTimeToSystemTime(&stamp,&roundtrip)||parsed.wYear!=roundtrip.wYear||parsed.wMonth!=roundtrip.wMonth||parsed.wDay!=roundtrip.wDay||parsed.wHour!=roundtrip.wHour||parsed.wMinute!=roundtrip.wMinute||parsed.wSecond!=roundtrip.wSecond)return {};
 ULARGE_INTEGER ticks{};ticks.LowPart=stamp.dwLowDateTime;ticks.HighPart=stamp.dwHighDateTime;return static_cast<i64>(ticks.QuadPart/10000000ULL);
}
struct DashPrefixValidator {
 std::string etag,lastModified,responseDate;
 bool valid()const{
  if(!etag.empty())return dashStrongEntityTag(etag)&&lastModified.empty()&&responseDate.empty();
  const auto modified=dashPrefixHttpDate(lastModified),date=dashPrefixHttpDate(responseDate);
  // RFC 9110 permits date validators only with sufficient clock evidence.
  // Retain a conservative 60-second margin for independently generated dates.
  return modified&&date&&*date>=*modified&&*date-*modified>=60;
 }
 const std::string& ifRange()const{return etag.empty()?lastModified:etag;}
 static std::optional<DashPrefixValidator> fromResponse(const Http& response){
  const auto tags=response.headers(L"ETag");
  if(!tags.empty()){
   if(tags.size()!=1)return {};DashPrefixValidator result;result.etag=trim(tags.front());if(result.valid())return result;return {};
  }
  // Even a weak ETag prevents date-based If-Range generation (RFC 9110 13.1.5).
  const auto modified=response.headers(L"Last-Modified"),dates=response.headers(L"Date");
  if(modified.size()!=1||dates.size()!=1)return {};DashPrefixValidator result;result.lastModified=trim(modified.front());result.responseDate=trim(dates.front());if(result.valid())return result;return {};
 }
 bool matches(const Http& response)const{
  const auto incoming=fromResponse(response);return incoming&&incoming->etag==etag&&incoming->lastModified==lastModified;
 }
};
class DashPrefixHash {
 BCRYPT_ALG_HANDLE algorithm=nullptr;BCRYPT_HASH_HANDLE hash=nullptr;
public:
 ~DashPrefixHash(){if(hash)BCryptDestroyHash(hash);if(algorithm)BCryptCloseAlgorithmProvider(algorithm,0);}
 DashPrefixHash()=default;DashPrefixHash(const DashPrefixHash&)=delete;DashPrefixHash& operator=(const DashPrefixHash&)=delete;
 void reset(){if(hash){BCryptDestroyHash(hash);hash=nullptr;}if(!algorithm&&BCryptOpenAlgorithmProvider(&algorithm,BCRYPT_SHA256_ALGORITHM,nullptr,0)<0)throw std::runtime_error("Cannot initialize DASH prefix hashing.");if(BCryptCreateHash(algorithm,&hash,nullptr,0,nullptr,0,0)<0)throw std::runtime_error("Cannot initialize DASH prefix hashing.");}
 void add(const void* bytes,size_t count){if(count>MAXDWORD||!hash||BCryptHashData(hash,(PUCHAR)bytes,(ULONG)count,0)<0)throw std::runtime_error("Cannot hash DASH prefix.");}
 std::string digest()const{BCRYPT_HASH_HANDLE copy=nullptr;if(!hash||BCryptDuplicateHash(hash,&copy,nullptr,0,0)<0)throw std::runtime_error("Cannot snapshot DASH prefix hash.");unsigned char bytes[32]{};const auto status=BCryptFinishHash(copy,bytes,sizeof(bytes),0);BCryptDestroyHash(copy);if(status<0)throw std::runtime_error("Cannot finish DASH prefix hash.");std::string result;const char* hex="0123456789abcdef";for(auto c:bytes){result+=hex[c>>4];result+=hex[c&15];}return result;}
};
// A protected checkpoint describes only a flushed prefix. Uncheckpointed tail
// bytes can survive a crash, but are neither trusted nor appended to directly.
class DashResourceResume {
 static constexpr i64 limit=256LL*1024*1024;
 struct Candidate {std::string url,digest,envelope;DashPrefixValidator validation;i64 bytes=0,total=-1;bool scratch=false;};
 fs::path payload,receipt;std::string key,url;DashPrefixValidator validation;HANDLE file=INVALID_HANDLE_VALUE;DashPrefixHash hash;
 std::vector<Candidate> candidates;std::set<std::string> rejected;
 i64 written=0,total=-1,checkpointed=0;ULONGLONG lastCheckpoint=0;bool verified=false,active=false,healthy=true;
 static void plain(const fs::path& path){const auto flags=GetFileAttributesW(path.c_str());if(flags!=INVALID_FILE_ATTRIBUTES&&(flags&FILE_ATTRIBUTE_REPARSE_POINT))throw std::runtime_error("DASH prefix files must not be redirected.");}
 void close()noexcept{if(file!=INVALID_HANDLE_VALUE){CloseHandle(file);file=INVALID_HANDLE_VALUE;}}
 void open(){if(file!=INVALID_HANDLE_VALUE)return;plain(payload);file=CreateFileW(payload.c_str(),GENERIC_READ|GENERIC_WRITE,FILE_SHARE_READ,nullptr,OPEN_ALWAYS,FILE_ATTRIBUTE_NORMAL|FILE_FLAG_OPEN_REPARSE_POINT,nullptr);if(file==INVALID_HANDLE_VALUE)throw std::runtime_error("Cannot open DASH prefix file.");FILE_ATTRIBUTE_TAG_INFO info{};if(!GetFileInformationByHandleEx(file,FileAttributeTagInfo,&info,sizeof(info))||(info.FileAttributes&(FILE_ATTRIBUTE_REPARSE_POINT|FILE_ATTRIBUTE_DIRECTORY))){close();throw std::runtime_error("Invalid DASH prefix file.");}}
 void seek(i64 offset){LARGE_INTEGER position{};position.QuadPart=offset;if(!SetFilePointerEx(file,position,nullptr,FILE_BEGIN))throw std::runtime_error("Cannot seek DASH prefix file.");}
public:
 DashResourceResume(const fs::path& folder,std::string binding,const Cancel& cancel):payload(folder/(wide(binding)+L".resume")),receipt(folder/(wide(binding)+L".resume.json")),key(std::move(binding)){
  cancel.check();if(!folder.is_absolute()||!std::regex_match(key,std::regex("[0-9a-f]{64}")))throw std::runtime_error("Invalid DASH prefix identity.");plain(payload);plain(receipt);auto temporary=receipt;temporary+=L".tmp";plain(temporary);
  if(!fs::is_regular_file(payload))return;
  for(const auto& marker:{receipt,temporary}){
   cancel.check();try{if(!fs::is_regular_file(marker))continue;Candidate candidate;candidate.scratch=marker==temporary;candidate.envelope=readText(marker,128*1024);const auto envelope=Json::parse(candidate.envelope);if(num(envelope,"schema")!=1)continue;const auto value=Json::parse(reveal(str(envelope,"protected")));
    candidate.url=str(value,"url");candidate.validation.etag=str(value,"etag");candidate.validation.lastModified=str(value,"lastModified");candidate.validation.responseDate=str(value,"responseDate");candidate.bytes=num(value,"bytes",-1);candidate.total=num(value,"total",-1);candidate.digest=str(value,"sha256");
    if(!value.contains("bytes")||!value["bytes"].is_number_integer()||!value.contains("total")||!value["total"].is_number_integer()||str(value,"binding")!=key||candidate.bytes<=0||candidate.bytes>limit||(candidate.total!=-1&&(candidate.total<candidate.bytes||candidate.total>limit))||!candidate.validation.valid()||candidate.url.empty()||candidate.url.size()>32768||candidate.url.find('#')!=std::string::npos||std::any_of(candidate.url.begin(),candidate.url.end(),[](unsigned char c){return c<=0x20||c==0x7f;})||!std::regex_match(candidate.digest,std::regex("[0-9a-f]{64}")))continue;
    const Url parsed(candidate.url);if((parsed.scheme!="http"&&parsed.scheme!="https")||parsed.full!=candidate.url)continue;candidates.push_back(std::move(candidate));
   }catch(const Cancelled&){throw;}catch(const std::exception&){cancel.check();}
  }
  std::stable_sort(candidates.begin(),candidates.end(),[](const Candidate& a,const Candidate& b){return a.bytes!=b.bytes?a.bytes>b.bytes:a.scratch&&!b.scratch;});
 }
 ~DashResourceResume(){close();}
 DashResourceResume(const DashResourceResume&)=delete;DashResourceResume& operator=(const DashResourceResume&)=delete;
 bool offer(const std::string& address,const Cancel& cancel){
  cancel.check();if(active||candidates.empty())return false;if(verified)return address==url;
  if(rejected.count(address)||std::none_of(candidates.begin(),candidates.end(),[&](const Candidate& value){return value.url==address;}))return false;
  const Candidate* selected=nullptr;
  try{open();LARGE_INTEGER size{};if(!GetFileSizeEx(file,&size)||size.QuadPart<=0||size.QuadPart>limit)throw std::runtime_error("DASH prefix size changed.");
   for(const auto& candidate:candidates){if(candidate.url!=address||candidate.bytes>size.QuadPart)continue;seek(0);hash.reset();char buffer[65536];i64 remaining=candidate.bytes;
    while(remaining){cancel.check();DWORD count=0;const auto request=(DWORD)std::min<i64>(remaining,sizeof(buffer));if(!ReadFile(file,buffer,request,&count,nullptr)||!count)throw std::runtime_error("Cannot verify DASH prefix.");hash.add(buffer,count);remaining-=count;}
    if(hash.digest()==candidate.digest){selected=&candidate;break;}
   }
  }catch(const Cancelled&){close();throw;}catch(const std::exception&){close();rejected.insert(address);cancel.check();return false;}
  if(!selected){close();rejected.insert(address);return false;}
  // Preserve the verified scratch as the primary checkpoint before any new
  // checkpoint write can reuse its filename. Publication errors must propagate.
  cancel.check();if(selected->scratch){auto temporary=receipt;temporary+=L".tmp";plain(receipt);plain(temporary);
   if(readText(temporary,128*1024)!=selected->envelope)throw std::runtime_error("DASH prefix checkpoint changed during recovery.");cancel.check();plain(temporary);
   if(!MoveFileExW(temporary.c_str(),receipt.c_str(),MOVEFILE_REPLACE_EXISTING|MOVEFILE_WRITE_THROUGH))throw std::runtime_error("Cannot recover the verified DASH prefix checkpoint.");
  }
  url=selected->url;validation=selected->validation;written=selected->bytes;total=selected->total;verified=true;return true;
 }
 i64 bytes()const{return written;}
 const std::string& validator()const{return validation.ifRange();}
 std::optional<i64> resumedTotal(const Http& response)const{
  try{if(response.status!=206||!validation.matches(response)||response.headers(L"Content-Range").size()!=1)return {};
   const auto encoding=lower(trim(response.header(L"Content-Encoding")));if((!encoding.empty()&&encoding!="identity")||lower(response.header(L"Content-Type")).find("multipart/byteranges")!=std::string::npos)return {};
   std::smatch match;const auto value=trim(response.header(L"Content-Range"));if(!std::regex_match(value,match,std::regex("bytes ([0-9]{1,19})-([0-9]{1,19})/([0-9]{1,19})")))return {};
   const i64 start=std::stoll(match[1]),end=std::stoll(match[2]),size=std::stoll(match[3]);if(start!=written||size<=written||size>limit||end!=size-1||(total>=0&&size!=total))return {};
   const auto lengths=response.headers(L"Content-Length"),codings=response.headers(L"Transfer-Encoding");if(lengths.size()>1||codings.size()>1||(!lengths.empty()&&!codings.empty()))return {};if(!codings.empty()&&lower(trim(codings.front()))!="chunked")return {};if(!lengths.empty()){const auto length=trim(lengths.front());if(!std::regex_match(length,std::regex("[0-9]{1,19}"))||std::stoll(length)!=size-written)return {};}
   return size;
  }catch(const std::exception&){return {};}
 }
 bool begin(const std::string& address,const Http& response,i64 size,bool append){
  const auto incoming=DashPrefixValidator::fromResponse(response);
  if(!incoming||size < -1||size>limit||address.empty()||address.size()>32768||address.find('#')!=std::string::npos||std::any_of(address.begin(),address.end(),[](unsigned char c){return c<=0x20||c==0x7f;}))return false;
  if(append){if(!verified||address!=url||incoming->etag!=validation.etag||incoming->lastModified!=validation.lastModified||size<=written)throw std::runtime_error("DASH prefix response changed.");open();seek(written);if(!SetEndOfFile(file))throw std::runtime_error("Cannot trim DASH prefix tail.");checkpointed=written;}
  else{open();auto temporary=receipt;temporary+=L".tmp";for(const auto& marker:{receipt,temporary}){plain(marker);std::error_code error;fs::remove(marker,error);if(error)throw std::runtime_error("Cannot replace DASH prefix checkpoint.");}seek(0);if(!SetEndOfFile(file))throw std::runtime_error("Cannot reset DASH prefix.");hash.reset();written=0;checkpointed=0;verified=false;candidates.clear();rejected.clear();}
  url=address;validation=*incoming;total=size;active=true;healthy=true;lastCheckpoint=GetTickCount64();return true;
 }
 void checkpoint(){if(!active||!healthy||written<=checkpointed)return;if(!FlushFileBuffers(file))throw std::runtime_error("Cannot flush DASH prefix.");
  Json saved={{"binding",key},{"url",url},{"etag",validation.etag},{"bytes",written},{"total",total},{"sha256",hash.digest()}};if(validation.etag.empty()){saved["lastModified"]=validation.lastModified;saved["responseDate"]=validation.responseDate;}plain(receipt);auto temporary=receipt;temporary+=L".tmp";plain(temporary);atomicText(receipt,Json{{"schema",1},{"protected",protect(saved.dump())}}.dump(),false);checkpointed=written;lastCheckpoint=GetTickCount64();
 }
 void write(const void* data,size_t count){if(!active||count>static_cast<size_t>(limit-written))throw std::runtime_error("DASH prefix exceeds its size limit.");try{size_t offset=0;while(offset<count){DWORD done=0;if(!WriteFile(file,(const char*)data+offset,(DWORD)std::min<size_t>(count-offset,MAXDWORD),&done,nullptr)||!done)throw std::runtime_error("Cannot write DASH prefix.");offset+=done;}hash.add(data,count);written+=(i64)count;}catch(...){healthy=false;throw;}
  if(!checkpointed||GetTickCount64()-lastCheckpoint>=1000)checkpoint();
 }
 std::string finish(){checkpoint();const auto digest=hash.digest();if(!FlushFileBuffers(file))throw std::runtime_error("Cannot flush DASH resource.");close();active=false;return digest;}
 void preserve()noexcept{try{checkpoint();}catch(...){}close();active=false;}
 const fs::path& path()const{return payload;}
 void clear()noexcept{close();try{plain(receipt);plain(payload);auto temporary=receipt;temporary+=L".tmp";plain(temporary);std::error_code ignored;fs::remove(receipt,ignored);fs::remove(payload,ignored);fs::remove(temporary,ignored);}catch(...){}candidates.clear();rejected.clear();verified=false;active=false;}
};
}
