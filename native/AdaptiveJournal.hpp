#pragma once
#include "Core.hpp"
#include <regex>
namespace udm {
// Small per-part commits avoid rewriting the complete index for every segment.
// A new snapshot generation makes older receipts harmless after a crash between
// publishing a refreshed index and removing its old receipt files.
class AdaptiveJournal {
 fs::path folder;size_t count;std::string generation;
 fs::path receiptPath(size_t index)const{if(index>=count)throw std::runtime_error("Invalid adaptive receipt index.");return folder/(std::to_wstring(index)+L".receipt.json");}
 static void plain(const fs::path& path){auto flags=GetFileAttributesW(path.c_str());if(flags!=INVALID_FILE_ATTRIBUTES&&(flags&FILE_ATTRIBUTE_REPARSE_POINT))throw std::runtime_error("Adaptive recovery files must not be redirected.");}
 static void write(const fs::path& path,const Json& data,bool backup=false){plain(path);auto temporary=path;temporary+=L".tmp";plain(temporary);if(backup){auto old=path;old+=L".bak";plain(old);}atomicText(path,data.dump(),backup);}
public:
 AdaptiveJournal(fs::path directory,size_t parts):folder(std::move(directory)),count(parts){if(count>10000)throw std::runtime_error("Adaptive receipt count exceeds its limit.");plain(folder);}
 Json load(){
  Json completed=Json::object();auto snapshot=folder/L"completed.json";plain(snapshot);generation.clear();
  if(fs::exists(snapshot))try{completed=Json::parse(readText(snapshot));if(!completed.is_object())throw std::runtime_error("Invalid adaptive snapshot.");
   if(completed.contains("_receiptGeneration")){if(!completed["_receiptGeneration"].is_string())throw std::runtime_error("Invalid adaptive generation.");generation=str(completed,"_receiptGeneration");if(!std::regex_match(generation,std::regex("[0-9a-f]{32}")))throw std::runtime_error("Invalid adaptive generation.");completed.erase("_receiptGeneration");}
  }catch(...){completed=Json::object();checkpoint(completed);}
  for(size_t index=0;index<count;++index){auto path=receiptPath(index);plain(path);if(!fs::exists(path))continue;auto key=std::to_string(index);
   try{auto record=Json::parse(readText(path,128*1024));if(!record.is_object()||!record.contains("generation")||!record["generation"].is_string()||!record.contains("receipt")||!record["receipt"].is_object())throw std::runtime_error("Invalid adaptive receipt.");if(str(record,"generation")==generation)completed[key]=record["receipt"];
   }catch(...){completed.erase(key);}
  }return completed;
 }
 void commit(size_t index,const Json& receipt){if(!receipt.is_object()||receipt.dump().size()>120*1024)throw std::runtime_error("Invalid adaptive receipt.");write(receiptPath(index),Json{{"generation",generation},{"receipt",receipt}});}
 void checkpoint(const Json& completed){auto next=guid();auto snapshot=completed;snapshot["_receiptGeneration"]=next;if(snapshot.dump().size()>32*1024*1024)throw std::runtime_error("Adaptive recovery index exceeds its storage limit.");write(folder/L"completed.json",snapshot,true);generation=next;}
 void clean()const noexcept{try{plain(folder);for(size_t index=0;index<count;++index)for(const auto* suffix:{L"",L".tmp",L".bak"}){auto path=receiptPath(index);path+=suffix;auto flags=GetFileAttributesW(path.c_str());if(flags==INVALID_FILE_ATTRIBUTES||(flags&(FILE_ATTRIBUTE_DIRECTORY|FILE_ATTRIBUTE_REPARSE_POINT)))continue;std::error_code ignored;fs::remove(path,ignored);}}catch(...) {}}
};
}
