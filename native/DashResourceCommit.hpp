#pragma once
#include "Core.hpp"
#include <optional>
#include <regex>

namespace udm {
enum class DashCompletionStage {Temporary,Prefix,Promoted};
struct DashCompletedResource {fs::path path;i64 bytes=0;std::string sha256;};
inline void dashCompletionPlain(const fs::path& path){const auto flags=GetFileAttributesW(path.c_str());if(flags!=INVALID_FILE_ATTRIBUTES&&(flags&FILE_ATTRIBUTE_REPARSE_POINT))throw std::runtime_error("DASH completion files must not be redirected.");}
inline bool dashCompletionKey(const std::string& key){return std::regex_match(key,std::regex("[0-9a-f]{64}"));}
inline fs::path dashCompletionPath(const fs::path& folder,const std::string& key,DashCompletionStage stage){switch(stage){case DashCompletionStage::Prefix:return folder/(wide(key)+L".resume");case DashCompletionStage::Temporary:return folder/(wide(key)+L".part.tmp");case DashCompletionStage::Promoted:return folder/(wide(key)+L".part");}throw std::runtime_error("Invalid DASH completion stage.");}
// Only after a complete receipt is verified/published. Never removes .part or
// .receipt.json; all paths are derived from the validated internal digest key.
inline void clearDashResourceStaging(const fs::path& folder,const std::string& key)noexcept{
 try{if(!folder.is_absolute()||!dashCompletionKey(key))return;for(const auto* suffix:{L".commit.json",L".commit.json.tmp",L".receipt.json.tmp",L".part.tmp",L".resume",L".resume.json",L".resume.json.tmp"}){
   try{const auto path=folder/(wide(key)+suffix);dashCompletionPlain(path);std::error_code ignored;if(fs::is_regular_file(path,ignored))fs::remove(path,ignored);}catch(...){}
  }}catch(...){}
}
inline std::string verifyDashCompletionFile(const fs::path& path,i64 bytes,const Cancel& cancel){
 cancel.check();dashCompletionPlain(path);Handle file(CreateFileW(path.c_str(),GENERIC_READ|GENERIC_WRITE,FILE_SHARE_READ,nullptr,OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL|FILE_FLAG_OPEN_REPARSE_POINT,nullptr));
 if(!file)throw std::runtime_error("Cannot open completed DASH resource.");FILE_ATTRIBUTE_TAG_INFO info{};LARGE_INTEGER length{};
 if(!GetFileInformationByHandleEx(file.h,FileAttributeTagInfo,&info,sizeof(info))||(info.FileAttributes&(FILE_ATTRIBUTE_DIRECTORY|FILE_ATTRIBUTE_REPARSE_POINT))||!GetFileSizeEx(file.h,&length)||length.QuadPart!=bytes)throw std::runtime_error("Completed DASH resource size or type changed.");
 const auto digest=fileHash(file.h,[&]{cancel.check();});if(!FlushFileBuffers(file.h))throw std::runtime_error("Cannot flush completed DASH resource.");return digest;
}
inline DashCompletedResource publishDashResourceCompletion(const fs::path& folder,const std::string& key,const fs::path& source,i64 bytes,const std::string& digest,const Cancel& cancel){
 if(!folder.is_absolute()||!dashCompletionKey(key)||bytes<=0||bytes>256LL*1024*1024||!std::regex_match(digest,std::regex("[0-9a-f]{64}")))throw std::runtime_error("Invalid DASH completion publication.");
 cancel.check();const auto destination=folder/(wide(key)+L".part"),receipt=folder/(wide(key)+L".receipt.json");dashCompletionPlain(source);dashCompletionPlain(destination);dashCompletionPlain(receipt);auto temporary=receipt;temporary+=L".tmp";dashCompletionPlain(temporary);
 if(source!=destination&&source!=dashCompletionPath(folder,key,DashCompletionStage::Temporary)&&source!=dashCompletionPath(folder,key,DashCompletionStage::Prefix))throw std::runtime_error("DASH completion source is outside its staging paths.");
 if(source!=destination&&!MoveFileExW(source.c_str(),destination.c_str(),MOVEFILE_REPLACE_EXISTING|MOVEFILE_WRITE_THROUGH))throw std::runtime_error("Cannot promote completed DASH resource.");
 cancel.check();const Json saved={{"binding",key},{"bytes",bytes},{"sha256",digest}};atomicText(receipt,Json{{"schema",1},{"protected",protect(saved.dump())}}.dump(),false);
 clearDashResourceStaging(folder,key);return {destination,bytes,digest};
}
inline void writeDashResourceCompletionIntent(const fs::path& folder,const std::string& key,DashCompletionStage stage,i64 bytes,const std::string& digest,const Cancel& cancel){
 if(!folder.is_absolute()||!dashCompletionKey(key)||bytes<=0||bytes>256LL*1024*1024||!std::regex_match(digest,std::regex("[0-9a-f]{64}")))throw std::runtime_error("Invalid completed DASH resource identity.");
 if(stage!=DashCompletionStage::Temporary&&stage!=DashCompletionStage::Prefix&&stage!=DashCompletionStage::Promoted)throw std::runtime_error("Invalid DASH completion stage.");
 cancel.check();const auto intent=folder/(wide(key)+L".commit.json");dashCompletionPlain(intent);auto temporary=intent;temporary+=L".tmp";dashCompletionPlain(temporary);
 const Json saved={{"kind","dash-resource-completion"},{"binding",key},{"stage",stage==DashCompletionStage::Prefix?"prefix":stage==DashCompletionStage::Promoted?"promoted":"temporary"},{"bytes",bytes},{"sha256",digest}};
 atomicText(intent,Json{{"schema",1},{"protected",protect(saved.dump())}}.dump(),false);
}
inline DashCompletedResource commitDashResourceCompletion(const fs::path& folder,const std::string& key,DashCompletionStage stage,i64 bytes,const std::string& expectedDigest,const Cancel& cancel){
 if(!folder.is_absolute()||!dashCompletionKey(key)||bytes<=0||bytes>256LL*1024*1024)throw std::runtime_error("Invalid completed DASH resource identity.");
 const auto source=dashCompletionPath(folder,key,stage);const auto digest=verifyDashCompletionFile(source,bytes,cancel);
 if(!expectedDigest.empty()&&digest!=expectedDigest)throw std::runtime_error("DASH resource changed on disk.");
 writeDashResourceCompletionIntent(folder,key,stage,bytes,digest,cancel);
 return publishDashResourceCompletion(folder,key,source,bytes,digest,cancel);
}
inline std::optional<DashCompletedResource> recoverDashResourceCompletion(const fs::path& folder,const std::string& key,std::optional<i64> requiredBytes,const Cancel& cancel){
 cancel.check();if(!folder.is_absolute()||!dashCompletionKey(key))throw std::runtime_error("Invalid DASH completion identity.");
 const auto intent=folder/(wide(key)+L".commit.json");auto scratch=intent;scratch+=L".tmp";
 for(const auto& marker:{intent,scratch}){
  dashCompletionPlain(marker);if(!fs::is_regular_file(marker))continue;i64 bytes=0;std::string digest,stage;
  try{const auto envelope=Json::parse(readText(marker,65536));if(num(envelope,"schema")!=1)continue;const auto value=Json::parse(reveal(str(envelope,"protected")));bytes=num(value,"bytes");digest=str(value,"sha256");stage=str(value,"stage");
   if(str(value,"kind")!="dash-resource-completion"||str(value,"binding")!=key||bytes<=0||bytes>256LL*1024*1024||(requiredBytes&&bytes!=*requiredBytes)||!std::regex_match(digest,std::regex("[0-9a-f]{64}"))||(stage!="prefix"&&stage!="temporary"&&stage!="promoted"))continue;
  }catch(const Cancelled&){throw;}catch(const std::exception&){cancel.check();continue;}
  const auto staged=dashCompletionPath(folder,key,stage=="prefix"?DashCompletionStage::Prefix:stage=="promoted"?DashCompletionStage::Promoted:DashCompletionStage::Temporary),destination=folder/(wide(key)+L".part");
  std::vector<fs::path> candidates{staged};if(staged!=destination)candidates.push_back(destination);
  for(const auto& candidate:candidates){
   cancel.check();dashCompletionPlain(candidate);if(!fs::is_regular_file(candidate))continue;bool matches=false;
   try{matches=verifyDashCompletionFile(candidate,bytes,cancel)==digest;}catch(const Cancelled&){throw;}catch(const std::exception&){cancel.check();continue;}
   if(matches)return publishDashResourceCompletion(folder,key,candidate,bytes,digest,cancel);
  }
 }
 // Older stores moved the complete payload before atomicText wrote the full
 // receipt. Only that exact full-receipt scratch schema can attest completion;
 // a prefix checkpoint or an arbitrary orphan .part cannot do so.
 const auto legacy=folder/(wide(key)+L".receipt.json.tmp"),destination=folder/(wide(key)+L".part");dashCompletionPlain(legacy);dashCompletionPlain(destination);
 if(!fs::is_regular_file(legacy)||!fs::is_regular_file(destination))return {};
 i64 bytes=0;std::string digest;
 try{const auto envelope=Json::parse(readText(legacy,256*1024));if(num(envelope,"schema")!=1)return {};const auto value=Json::parse(reveal(str(envelope,"protected")));bytes=num(value,"bytes");digest=lower(str(value,"sha256"));
  if(!value.is_object()||value.size()!=3||str(value,"binding")!=key||bytes<=0||bytes>256LL*1024*1024||(requiredBytes&&bytes!=*requiredBytes)||!std::regex_match(digest,std::regex("[0-9a-f]{64}")))return {};
  if(verifyDashCompletionFile(destination,bytes,cancel)!=digest)return {};
 }catch(const Cancelled&){throw;}catch(const std::exception&){cancel.check();return {};}
 // Publication rewrites the receipt scratch path. First preserve an independent
 // completion intent so a second interruption cannot erase the sole evidence.
 writeDashResourceCompletionIntent(folder,key,DashCompletionStage::Promoted,bytes,digest,cancel);
 return publishDashResourceCompletion(folder,key,destination,bytes,digest,cancel);
}
}
