#pragma once
#include "Core.hpp"
#include <system_error>

namespace udm {
// Detached from the catalog before filesystem/URL checks or control updates.
struct CommandAvailability {
 bool enabled=true,zipOnly=false,ftpOnly=false;
 std::vector<fs::path> regularFiles;
 std::string url;
};
template<class RegularFile> inline bool resolveCommandAvailabilityWith(const CommandAvailability& value,RegularFile&& regularFile){
 if(!value.enabled)return false;
 try{
  if(value.ftpOnly&&Url(value.url).scheme!="ftp")return false;
  if(value.zipOnly&&(value.regularFiles.size()!=1||lower(utf8(value.regularFiles.front().extension().wstring()))!=".zip"))return false;
  for(const auto& path:value.regularFiles)if(!regularFile(path))return false;
  return true;
 }catch(const std::exception&){return false;}
}
inline bool resolveCommandAvailability(const CommandAvailability& value,std::map<fs::path,bool>& files){
 return resolveCommandAvailabilityWith(value,[&](const fs::path& path){auto known=files.find(path);if(known==files.end()){std::error_code error;const bool regular=fs::is_regular_file(path,error);known=files.emplace(path,!error&&regular).first;}return known->second;});
}
}
