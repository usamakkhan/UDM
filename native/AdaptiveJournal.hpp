#pragma once
#include "Core.hpp"
#include <regex>
#include <bcrypt.h>
namespace udm {
struct AdaptiveSelectionChanged:std::runtime_error {
 AdaptiveSelectionChanged():std::runtime_error("Saved streaming parts belong to a different media selection. Recovery files were left unchanged."){}
};
struct AdaptivePrefixReceipt {
 i64 bytes;
 std::string digest;
 bool scratch;
};
inline std::string adaptiveSelectionDigest(const Json& identity){
 auto text=identity.dump();if(text.size()>8ULL*1024*1024)throw std::runtime_error("Adaptive recovery identity is too large.");
 BCRYPT_ALG_HANDLE algorithm=nullptr;if(BCryptOpenAlgorithmProvider(&algorithm,BCRYPT_SHA256_ALGORITHM,nullptr,0)<0)throw std::runtime_error("Cannot initialize adaptive identity hashing.");
 unsigned char digest[32]{};auto status=BCryptHash(algorithm,nullptr,0,(PUCHAR)text.data(),(ULONG)text.size(),digest,sizeof(digest));BCryptCloseAlgorithmProvider(algorithm,0);
 if(status<0)throw std::runtime_error("Cannot hash adaptive recovery identity.");std::string key;const char* hex="0123456789abcdef";for(auto byte:digest){key+=hex[byte>>4];key+=hex[byte&15];}return key;
}
// Small per-part commits avoid rewriting the complete index for every segment.
// A new snapshot generation makes older receipts harmless after a crash between
// publishing a refreshed index and removing its old receipt files.
class AdaptiveJournal {
 fs::path folder;size_t count;std::string generation,selectionIdentity;bool allowLegacyIdentity=false;
 fs::path receiptPath(size_t index)const{if(index>=count)throw std::runtime_error("Invalid adaptive receipt index.");return folder/(std::to_wstring(index)+L".receipt.json");}
 fs::path prefixPath(size_t index)const{if(index>=count)throw std::runtime_error("Invalid adaptive prefix index.");return folder/(std::to_wstring(index)+L".prefix.json");}
 static void plain(const fs::path& path){auto flags=GetFileAttributesW(path.c_str());if(flags!=INVALID_FILE_ATTRIBUTES&&(flags&FILE_ATTRIBUTE_REPARSE_POINT))throw std::runtime_error("Adaptive recovery files must not be redirected.");}
 static void write(const fs::path& path,const Json& data,bool backup=false){plain(path);auto temporary=path;temporary+=L".tmp";plain(temporary);if(backup){auto old=path;old+=L".bak";plain(old);}atomicText(path,data.dump(),backup);}
public:
 AdaptiveJournal(fs::path directory,size_t parts,std::string identity={},bool allowLegacy=false):folder(std::move(directory)),count(parts),selectionIdentity(std::move(identity)),allowLegacyIdentity(allowLegacy){if(count>10000)throw std::runtime_error("Adaptive receipt count exceeds its limit.");if(!selectionIdentity.empty()&&!std::regex_match(selectionIdentity,std::regex("[0-9a-f]{64}")))throw std::runtime_error("Invalid adaptive selection digest.");plain(folder);}
 Json load(){
  Json completed=Json::object();auto snapshot=folder/L"completed.json";plain(snapshot);generation.clear();bool migrate=false;const bool hadSnapshot=fs::exists(snapshot);
  if(fs::exists(snapshot))try{completed=Json::parse(readText(snapshot));if(!completed.is_object())throw std::runtime_error("Invalid adaptive snapshot.");
   if(completed.contains("_selectionIdentity")){if(!completed["_selectionIdentity"].is_string()||str(completed,"_selectionIdentity")!=selectionIdentity)throw AdaptiveSelectionChanged();completed.erase("_selectionIdentity");}
   else if(!selectionIdentity.empty()){if(!allowLegacyIdentity)throw AdaptiveSelectionChanged();migrate=true;}
   if(completed.contains("_receiptGeneration")){if(!completed["_receiptGeneration"].is_string())throw std::runtime_error("Invalid adaptive generation.");generation=str(completed,"_receiptGeneration");if(!std::regex_match(generation,std::regex("[0-9a-f]{32}")))throw std::runtime_error("Invalid adaptive generation.");completed.erase("_receiptGeneration");}
  }catch(const AdaptiveSelectionChanged&){throw;}catch(...){completed=Json::object();checkpoint(completed);migrate=false;}
  if(!hadSnapshot&&!selectionIdentity.empty()){if(allowLegacyIdentity)migrate=true;else checkpoint(completed);}
  for(size_t index=0;index<count;++index){auto path=receiptPath(index);plain(path);if(!fs::exists(path))continue;auto key=std::to_string(index);
   try{auto record=Json::parse(readText(path,128*1024));if(!record.is_object()||!record.contains("generation")||!record["generation"].is_string()||!record.contains("receipt")||!record["receipt"].is_object())throw std::runtime_error("Invalid adaptive receipt.");if(str(record,"generation")==generation){if((record.contains("selectionIdentity")&&!record["selectionIdentity"].is_string())||((!migrate||!str(record,"selectionIdentity").empty())&&str(record,"selectionIdentity")!=selectionIdentity))throw AdaptiveSelectionChanged();completed[key]=record["receipt"];}
   }catch(const AdaptiveSelectionChanged&){throw;}catch(...){completed.erase(key);}
  }if(migrate)checkpoint(completed);return completed;
 }
 void commit(size_t index,const Json& receipt){if(!receipt.is_object()||receipt.dump().size()>120*1024)throw std::runtime_error("Invalid adaptive receipt.");write(receiptPath(index),Json{{"generation",generation},{"selectionIdentity",selectionIdentity},{"receipt",receipt}});}
 std::vector<AdaptivePrefixReceipt> loadPrefixes(size_t index,const std::string& binding,i64 length)const{
  std::vector<AdaptivePrefixReceipt> result;
  for(const bool scratch:{false,true})try{
   auto path=prefixPath(index);if(scratch)path+=L".tmp";plain(path);
   if(!fs::is_regular_file(path))continue;
   const auto envelope=Json::parse(readText(path,8192));
   if(num(envelope,"schema")!=1)continue;
   const auto saved=Json::parse(reveal(str(envelope,"protected")));
   const auto bytes=num(saved,"bytes");
   const auto digest=str(saved,"sha256");
   if(str(saved,"selection")!=selectionIdentity||(!binding.empty()&&str(saved,"binding")!=binding)||bytes<=0||bytes>=length||
      !std::regex_match(digest,std::regex("[0-9a-f]{64}")))continue;
   result.push_back({bytes,digest,scratch});
  }catch(const std::exception&){}
  std::sort(result.begin(),result.end(),[](const auto& a,const auto& b){return a.bytes>b.bytes||(a.bytes==b.bytes&&a.scratch&&!b.scratch);});
  return result;
 }
 void promotePrefix(size_t index)const{
  const auto path=prefixPath(index);auto scratch=path;scratch+=L".tmp";
  plain(path);plain(scratch);
  if(!MoveFileExW(scratch.c_str(),path.c_str(),MOVEFILE_REPLACE_EXISTING|MOVEFILE_WRITE_THROUGH))
   throw std::runtime_error("Cannot recover the saved streaming checkpoint.");
 }
 void commitPrefix(size_t index,const std::string& binding,i64 bytes,const std::string& digest)const{
  const Json saved={{"selection",selectionIdentity},{"binding",binding},{"bytes",bytes},{"sha256",digest}};
  write(prefixPath(index),Json{{"schema",1},{"protected",protect(saved.dump())}});
 }
 void clearPrefix(size_t index)const noexcept{
  try{for(const auto* suffix:{L"",L".tmp"}){
   auto path=prefixPath(index);path+=suffix;
   const auto flags=GetFileAttributesW(path.c_str());
   if(flags==INVALID_FILE_ATTRIBUTES||(flags&(FILE_ATTRIBUTE_DIRECTORY|FILE_ATTRIBUTE_REPARSE_POINT)))continue;
   std::error_code ignored;fs::remove(path,ignored);
  }}catch(...){}
 }
 void checkpoint(const Json& completed){auto next=guid();auto snapshot=completed;snapshot["_receiptGeneration"]=next;if(!selectionIdentity.empty())snapshot["_selectionIdentity"]=selectionIdentity;if(snapshot.dump().size()>32*1024*1024)throw std::runtime_error("Adaptive recovery index exceeds its storage limit.");write(folder/L"completed.json",snapshot,true);generation=next;}
 void clean()const noexcept{try{plain(folder);for(size_t index=0;index<count;++index)for(const auto* suffix:{L"",L".tmp",L".bak"}){auto path=receiptPath(index);path+=suffix;auto flags=GetFileAttributesW(path.c_str());if(flags==INVALID_FILE_ATTRIBUTES||(flags&(FILE_ATTRIBUTE_DIRECTORY|FILE_ATTRIBUTE_REPARSE_POINT)))continue;std::error_code ignored;fs::remove(path,ignored);}}catch(...) {}}
};
}
