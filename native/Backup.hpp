#pragma once
#include "Core.hpp"
#include "RecoveryIo.hpp"
#include <algorithm>
#include <regex>
namespace udm {
// A same-account, same-path recovery image. Catalog export is a separate operation.
inline fs::path backupAbsolute(const fs::path& path) {
 if(!path.is_absolute())throw std::runtime_error("Backup paths must be absolute.");
 return fs::weakly_canonical(path).lexically_normal();
}
inline std::string backupKey(const fs::path& path){return lower(utf8(backupAbsolute(path).wstring()));}
inline bool backupWithin(const fs::path& child,const fs::path& parent){
 auto c=backupKey(child),p=backupKey(parent);if(c==p)return true;
 if(!p.empty()&&p.back()!='\\')p+='\\';return c.compare(0,p.size(),p)==0;
}
inline void backupPlain(const fs::path& path){
 auto attr=GetFileAttributesW(path.c_str());
 if(attr==INVALID_FILE_ATTRIBUTES)throw std::runtime_error("Cannot inspect backup source: "+utf8(path.wstring()));
 if(attr&FILE_ATTRIBUTE_REPARSE_POINT)throw std::runtime_error("A backup source contains a link or cloud placeholder: "+utf8(path.wstring()));
}
struct BackupInventory {
 std::map<std::string,fs::path> files;
 std::set<std::string> directories;
 Json missing=Json::array();
 void add(const fs::path& original,bool reportMissing=false){
  auto path=backupAbsolute(original);
  if(!fs::exists(path)){if(reportMissing)missing.push_back(utf8(original.wstring()));return;}
  backupPlain(path);
  if(fs::is_directory(path)){
   if(!directories.insert(backupKey(path)).second)return;
   if(directories.size()>100000)throw std::runtime_error("Backup exceeds 100,000 directories.");
   for(auto& entry:fs::directory_iterator(path)){
    // Inspect before canonicalization: a child junction must not expand the backup scope.
    backupPlain(entry.path());add(entry.path());
   }
  }else if(fs::is_regular_file(path)){
   files.emplace(backupKey(path),path);
   if(files.size()>100000)throw std::runtime_error("Backup exceeds 100,000 files.");
  }else throw std::runtime_error("Unsupported backup source.");
 }
};
inline BackupInventory backupInventory(Manager& manager){
 BackupInventory result;
 auto visit=[&](auto&& self,JobPtr job)->void{
  if(manager.isActive(job)||job->liveCapture)throw std::runtime_error("Pause downloads and finish browser capture before backing up.");
  result.add(job->target(),str(job->data,"Status")=="Complete");
  for(const char* field:{"PartsFolder","MediaFolder","PreviousPath"})
   if(!str(job->data,field).empty())result.add(fs::path(wide(str(job->data,field))),std::string(field)=="PreviousPath");
  if(job->video)self(self,job->video);if(job->audio)self(self,job->audio);
 };
 for(auto job:manager.jobs)visit(visit,job);
 result.add(manager.root);return result;
}
inline std::set<std::string> recoveryDestinations(const Json& manifest){
 std::set<std::string> result;
 for(const auto& file:manifest["Files"])result.insert(lower(str(file,"Original")));
 for(const auto& directory:manifest.value("Directories",Json::array()))result.insert("directory:"+lower(directory.get<std::string>()));
 return result;
}
inline Json verifyRecoveryImage(const fs::path& original,const Cancel& cancel=Cancel{}){
 auto root=backupAbsolute(original);backupPlain(root);auto manifestPath=root/L"manifest.json";backupPlain(manifestPath);
 auto manifest=Json::parse(readText(manifestPath,32*1024*1024));
 if(str(manifest,"Format")!="UDM recovery image"||num(manifest,"Version")!=1||
    !manifest.contains("Files")||!manifest["Files"].is_array()||manifest["Files"].empty()||manifest["Files"].size()>100001)
  throw std::runtime_error("Invalid recovery image.");
 std::set<std::string> paths,originals;bool stateFound=false;
 backupPlain(root/L"files");
 for(auto& entry:manifest["Files"]){
  auto name=str(entry,"Stored"),hash=str(entry,"Sha256"),source=str(entry,"Original");
  if(!std::regex_match(name,std::regex("[0-9]{6}\\.bin"))||!paths.insert(name).second||
     !std::regex_match(hash,std::regex("[a-f0-9]{64}"))||num(entry,"Size",-1)<0||
     !fs::path(wide(source)).is_absolute()||!originals.insert(lower(source)).second)
   throw std::runtime_error("Invalid recovery file entry.");
  auto path=root/L"files"/wide(name);backupPlain(path);
  if(!fs::is_regular_file(path)||fs::file_size(path)!=(uintmax_t)num(entry,"Size")||recoveryHash(path,cancel)!=hash)
   throw std::runtime_error("Recovery file is missing or damaged: "+name);
  if(yes(entry,"State")){
   if(stateFound)throw std::runtime_error("Duplicate recovery state.");
   stateFound=true;auto state=Json::parse(readText(path));
   if(num(state,"Schema")!=1||!state["Downloads"].is_array()||!state["Queues"].is_array()||!state["Settings"].is_object())
    throw std::runtime_error("Invalid saved recovery state.");
  }
 }
 if(!stateFound)throw std::runtime_error("Recovery state is missing.");
 auto directories=manifest.value("Directories",Json::array());std::set<std::string> seenDirectories;
 if(!directories.is_array()||directories.size()>100000)throw std::runtime_error("Invalid recovery directories.");
 for(auto& entry:directories){
  if(!entry.is_string())throw std::runtime_error("Invalid recovery directory.");
  auto value=entry.get<std::string>();auto path=fs::path(wide(value));
  if(!path.is_absolute()||path.lexically_normal()!=path||path==path.root_path()||
     !seenDirectories.insert(lower(value)).second||originals.count(lower(value)))
   throw std::runtime_error("Invalid or conflicting recovery directory.");
 }
 return manifest;
}
inline Json writeRecoveryImage(Manager& manager,const fs::path& destination,const Cancel& cancel,
 const std::function<void(const char*)>& checkpoint={}){
 Lock lock(manager.mutex);cancel.check();
 auto final=backupAbsolute(destination);if(fs::exists(final))throw std::runtime_error("Choose a new backup folder.");
 auto inventory=backupInventory(manager);
 for(auto& dir:inventory.directories)if(backupWithin(final,fs::path(wide(dir))))
  throw std::runtime_error("The backup folder cannot be inside data being backed up.");
 for(auto& item:inventory.files)if(backupWithin(final,item.second))
  throw std::runtime_error("Invalid backup destination.");
 auto state=manager.snapshot().dump();
 if(state.size()>32*1024*1024)throw std::runtime_error("Download history is too large to back up.");
 auto statePath=backupAbsolute(manager.root/L"state.json");
 // Lock every inventoried file against writes and replacement until verification completes.
 // Failure to obtain a lock is a visible failure, never a silent omission.
 std::vector<Handle> leases;leases.reserve(inventory.files.size());
 for(auto& item:inventory.files){
  cancel.check();Handle file(CreateFileW(item.second.c_str(),GENERIC_READ,FILE_SHARE_READ,nullptr,OPEN_EXISTING,FILE_FLAG_OPEN_REPARSE_POINT,nullptr));
  if(!file)throw std::runtime_error("A backup source is in use: "+utf8(item.second.wstring()));
  leases.push_back(std::move(file));
 }
 inventory.files.erase(backupKey(statePath));
 auto staging=final.parent_path()/(final.filename().wstring()+L".incomplete-"+wide(guid()));
 if(!fs::create_directory(staging))throw std::runtime_error("Cannot create backup staging folder.");
 fs::create_directory(staging/L"files");
 Json manifest={{"Format","UDM recovery image"},{"Version",1},{"Created",date()},
  {"DataDirectory",utf8(backupAbsolute(manager.root).wstring())},{"Credentials","Windows account encrypted; original account required"},
  {"Files",Json::array()},{"MissingFiles",inventory.missing},{"Directories",inventory.directories}};
 auto add=[&](const fs::path& original,const fs::path& stored,bool isState){
  manifest["Files"].push_back({{"Original",utf8(original.wstring())},{"Stored",utf8(stored.filename().wstring())},
   {"Size",fs::file_size(stored)},{"Sha256",recoveryHash(stored,cancel)},{"State",isState}});
 };
 // An incomplete folder is deliberately retained after interruption, never advertised as a backup.
 auto saved=staging/L"files"/L"000000.bin";atomicText(saved,state,false);add(statePath,saved,true);
 size_t number=0;
 for(auto& item:inventory.files){
  cancel.check();auto name=std::to_string(++number);name=std::string(6-name.size(),'0')+name+".bin";
  auto copy=staging/L"files"/wide(name);
  recoveryCopy(item.second,copy,cancel,[&](i64 done,i64 total){if(checkpoint&&done>0&&done<total)checkpoint("copy-progress");});
  add(item.second,copy,false);
  if(str(manifest["Files"].back(),"Sha256")!=recoveryHash(item.second,cancel))throw std::runtime_error("Backup source changed during copying.");
  if(checkpoint)checkpoint("copied-file");
 }
 auto after=backupInventory(manager);after.files.erase(backupKey(statePath));
 if(after.files!=inventory.files||after.directories!=inventory.directories||after.missing!=inventory.missing||manager.snapshot().dump()!=state)
  throw std::runtime_error("Data changed while creating the backup. Try again.");
 atomicText(staging/L"manifest.json",manifest.dump(2),false);
 verifyRecoveryImage(staging,cancel);cancel.check();if(checkpoint)checkpoint("before-publish");cancel.check();
 // A non-replacing rename on the same volume exposes the image only when all files verify.
 if(!MoveFileExW(staging.c_str(),final.c_str(),MOVEFILE_WRITE_THROUGH))
  throw std::runtime_error("Cannot publish the verified backup folder.");
 return manifest;
}
}
