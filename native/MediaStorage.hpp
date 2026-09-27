#pragma once
#include "Core.hpp"
namespace udm {
// Persist the selection on the job; changing preferences must not strand partials.
inline fs::path mediaWorkingDirectory(Manager& manager, JobPtr job) {
 Lock lock(manager.mutex);
 auto saved=str(job->data,"MediaFolder");
 if(!saved.empty()) {
  auto path=fs::path(wide(saved));
  if(!path.is_absolute())throw std::runtime_error("Invalid media temporary folder.");
  return path;
 }
 auto path=manager.root/L"media"/wide(job->id());
 // Retain legacy tracks/segments, but an empty failed attempt can use the new setting.
 const bool existing=fs::exists(path)&&!fs::is_empty(path);
 auto custom=str(manager.state["Settings"],"TemporaryFolder");
 if(!existing&&!custom.empty())path=fs::path(wide(custom))/L"UDM-media"/wide(job->id());
 if(!path.is_absolute())throw std::runtime_error("Choose an absolute media temporary folder.");
 job->data["MediaFolder"]=utf8(path.wstring());
 return path;
}
inline std::string mediaStorageError(DWORD code,const fs::path& path,const char* operation) {
 const auto folder=utf8(path.parent_path().wstring());
 if(code==ERROR_DISK_FULL||code==ERROR_HANDLE_DISK_FULL)
  return "Not enough disk space in temporary folder: "+folder+". Free space or choose another temporary folder in Options > Save to, then capture the video again.";
 return std::string("Cannot ")+operation+" in temporary folder: "+folder+" (Windows error "+std::to_string(code)+").";
}
}
