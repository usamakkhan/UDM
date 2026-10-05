#pragma once
#include "Backup.hpp"
#include "RecoveryDirectories.hpp"
namespace udm {
inline void restorePlainPath(const fs::path& path){
 if(!path.is_absolute()||path.lexically_normal()!=path)throw std::runtime_error("Restore requires normalized absolute paths.");
 for(auto p=path;!p.empty();p=p.parent_path()){
  if(GetFileAttributesW(p.c_str())!=INVALID_FILE_ATTRIBUTES)backupPlain(p);
  if(p==p.parent_path())break;
 }
}
inline void restoreMove(const fs::path& from,const fs::path& to){
 if(!MoveFileExW(from.c_str(),to.c_str(),MOVEFILE_WRITE_THROUGH))
  throw std::runtime_error("Cannot move restore file (Windows error "+std::to_string(GetLastError())+").");
}
inline void discardRestoredFile(const fs::path& path,const std::string& expected){
 if(fileHash(path)!=expected)throw std::runtime_error("Restore file changed before removal.");
 auto attributes=GetFileAttributesW(path.c_str());
 if(attributes==INVALID_FILE_ATTRIBUTES)throw std::runtime_error("Cannot inspect restored file.");
 if((attributes&FILE_ATTRIBUTE_READONLY)&&!SetFileAttributesW(path.c_str(),attributes&~FILE_ATTRIBUTE_READONLY))throw std::runtime_error("Cannot remove restored read-only file.");
 if(!fs::remove(path))throw std::runtime_error("Cannot remove restored file.");
}
// Caller must close the application before applying or rolling back an image.
// approved includes every file and explicit folder in the reviewed manifest.
inline Json rollbackRecovery(const fs::path& journalPath,const std::set<std::string>& approved){
 restorePlainPath(journalPath);auto journal=Json::parse(readText(journalPath));
 if(str(journal,"Format")!="UDM restore transaction"||num(journal,"Version")!=1||!journal["Files"].is_array())
  throw std::runtime_error("Invalid restore journal.");
 if(!std::regex_match(str(journal,"Id"),std::regex("[a-fA-F0-9]{32}")))throw std::runtime_error("Invalid restore transaction identifier.");
 std::set<std::string> reviewed;for(auto& row:journal["Files"])if(!reviewed.insert(lower(str(row,"Target"))).second)throw std::runtime_error("Duplicate rollback destination.");
 auto directoryRows=journal.value("Directories",Json::array());
 if(!directoryRows.is_array()||directoryRows.size()>200000)throw std::runtime_error("Invalid rollback directories.");
 auto allowedParents=recoveryParentPaths(approved);std::set<std::string> directoryNames,directoryIndexes;
 for(const auto& row:directoryRows){
  auto name=str(row,"Target"),index=str(row,"Index");auto path=fs::path(wide(name));restorePlainPath(path);
  if(path==path.root_path()||!allowedParents.count(lower(name))||!directoryNames.insert(lower(name)).second||
     !std::regex_match(index,std::regex("[0-9]{1,6}"))||!directoryIndexes.insert(index).second)
   throw std::runtime_error("Unapproved rollback directory.");
  if(yes(row,"Explicit"))reviewed.insert("directory:"+lower(name));
 }
 if(reviewed!=approved)throw std::runtime_error("Review rollback destinations before continuing.");
 if(str(journal,"Status")=="Committed"||str(journal,"Status")=="Rolled back")return journal;
 // Preflight all destinations before rolling anything back. A later edit must never be discarded.
 for(const auto& row:journal["Files"]){
  auto target=fs::path(wide(str(row,"Target"))),old=fs::path(wide(str(row,"Old"))),fresh=fs::path(wide(str(row,"New")));
  restorePlainPath(target);restorePlainPath(old);restorePlainPath(fresh);
  if(old.parent_path()!=target.parent_path()||fresh.parent_path()!=target.parent_path()||
     old.filename()!=wide(".udm-restore-"+str(journal,"Id")+"-"+str(row,"Index")+".old")||
     fresh.filename()!=wide(".udm-restore-"+str(journal,"Id")+"-"+str(row,"Index")+".new"))
   throw std::runtime_error("Invalid restore rollback paths.");
  if(fs::exists(old)&&fileHash(old)!=str(row,"Before"))throw std::runtime_error("Rollback copy is damaged.");
  if(fs::exists(target)){
   auto hash=fileHash(target);
   if(hash!=str(row,"After")&&hash!=str(row,"Before"))throw std::runtime_error("A restored file has changed; manual recovery is required.");
  }else if(yes(row,"Existed")&&!fs::exists(old))throw std::runtime_error("Original restore file is missing.");
 }
 for(auto it=journal["Files"].rbegin();it!=journal["Files"].rend();++it){
  auto& row=*it;auto target=fs::path(wide(str(row,"Target"))),old=fs::path(wide(str(row,"Old"))),fresh=fs::path(wide(str(row,"New")));
  if(fs::exists(old)){
   if(fs::exists(target)){if(fileHash(target)!=str(row,"After")&&fileHash(target)!=str(row,"Before"))throw std::runtime_error("Restore destination changed.");discardRestoredFile(target,fileHash(target));}
   restoreMove(old,target);
  }else if(!yes(row,"Existed")&&fs::exists(target)){
   if(fileHash(target)!=str(row,"After"))throw std::runtime_error("Restore destination changed.");discardRestoredFile(target,fileHash(target));
  }
  if(fs::exists(fresh)){
   if(fileHash(fresh)==str(row,"After"))discardRestoredFile(fresh,str(row,"After"));
   else{if(!journal.contains("RetainedStaging"))journal["RetainedStaging"]=Json::array();journal["RetainedStaging"].push_back(utf8(fresh.wstring()));}
  }
 }
 for(auto it=directoryRows.rbegin();it!=directoryRows.rend();++it){
  if(yes(*it,"Existed"))continue;
  auto path=fs::path(wide(str(*it,"Target")));if(!fs::exists(path))continue;
  auto receipt=journalPath.parent_path()/L"directories"/wide(str(*it,"Index")+".json");bool removed=false;
  if(fs::exists(receipt)){
   restorePlainPath(receipt);auto record=Json::parse(readText(receipt,65536));
   if(str(record,"Target")!=str(*it,"Target"))throw std::runtime_error("Directory creation receipt does not match.");
   removed=removeRecoveryDirectory(path,str(record,"Identity"));
  }
  if(!removed){if(!journal.contains("RetainedDirectories"))journal["RetainedDirectories"]=Json::array();journal["RetainedDirectories"].push_back(utf8(path.wstring()));}
 }
 journal["Status"]="Rolled back";atomicText(journalPath,journal.dump(2),false);return journal;
}
inline Json restoreRecoveryImage(const fs::path& image,const std::set<std::string>& approved,
 const fs::path& transactionDirectory,const Cancel& cancel,const std::function<void(const char*)>& checkpoint={}){
 cancel.check();if(backupWithin(transactionDirectory,image))throw std::runtime_error("Keep the restore journal outside the backup.");auto manifest=verifyRecoveryImage(image,cancel);
 std::set<std::string> targets;std::vector<Json> rows;
 auto id=guid();size_t index=0;
 for(auto row:manifest["Files"]){
  auto target=fs::path(wide(str(row,"Original")));restorePlainPath(target);
  if(backupWithin(target,image)||backupWithin(target,transactionDirectory)||backupWithin(transactionDirectory,target))
   throw std::runtime_error("Restore targets overlap the image or transaction journal.");
  if(!targets.insert(lower(utf8(target.wstring()))).second)throw std::runtime_error("Duplicate restore destination.");
  if(fs::exists(target)&&!fs::is_regular_file(target))throw std::runtime_error("Restore destination is not a file.");
  if(yes(row,"State")&&backupKey(target)!=backupKey(fs::path(wide(str(manifest,"DataDirectory")))/L"state.json"))
   throw std::runtime_error("Recovery state path does not match the data directory.");
  auto n=std::to_string(index++);
  rows.push_back({{"Target",utf8(target.wstring())},{"Source",utf8((backupAbsolute(image)/L"files"/wide(str(row,"Stored"))).wstring())},
   {"Index",n},{"Old",utf8((target.parent_path()/wide(".udm-restore-"+id+"-"+n+".old")).wstring())},
   {"New",utf8((target.parent_path()/wide(".udm-restore-"+id+"-"+n+".new")).wstring())},
   {"Before",fs::exists(target)?recoveryHash(target,cancel):""},{"After",str(row,"Sha256")},{"Existed",fs::exists(target)},{"State",yes(row,"State")}});
 }
 auto expected=recoveryDestinations(manifest);if(expected!=approved)throw std::runtime_error("Review the exact restore destinations before applying this image.");
 auto parents=recoveryParentPaths(approved);for(auto& parent:parents)if(targets.count(parent))throw std::runtime_error("Overlapping restore destinations.");
 std::map<std::string,Json> directoryMap;
 auto addDirectory=[&](fs::path path,bool explicitDirectory){
  for(bool first=true;!path.empty()&&path!=path.root_path();path=path.parent_path(),first=false){
   restorePlainPath(path);auto name=utf8(path.wstring()),key=lower(name);bool exists=fs::exists(path);
   if(exists&&!fs::is_directory(path))throw std::runtime_error("A restore folder path is occupied by a file.");
   if(!first&&exists)break;
   if(backupWithin(path,image)||backupWithin(path,transactionDirectory))throw std::runtime_error("Restore directories overlap recovery data.");
   if(!directoryMap.count(key))directoryMap[key]={{"Target",name},{"Existed",exists},{"Explicit",false}};
   if(directoryMap.size()>200000)throw std::runtime_error("Too many restore directories.");
   if(first&&explicitDirectory)directoryMap[key]["Explicit"]=true;
   if(exists)break;
  }
 };
 for(auto& directory:manifest.value("Directories",Json::array()))addDirectory(fs::path(wide(directory.get<std::string>())),true);
 for(auto& row:rows)addDirectory(fs::path(wide(str(row,"Target"))).parent_path(),false);
 std::vector<Json> directoryRows;for(auto& pair:directoryMap)directoryRows.push_back(pair.second);
 std::sort(directoryRows.begin(),directoryRows.end(),[](const Json& a,const Json& b){auto x=str(a,"Target"),y=str(b,"Target");return x.size()==y.size()?x<y:x.size()<y.size();});
 size_t directoryIndex=0;for(auto& row:directoryRows)row["Index"]=std::to_string(directoryIndex++);
 std::stable_sort(rows.begin(),rows.end(),[](const Json& a,const Json& b){return yes(a,"State")<yes(b,"State");});
 restorePlainPath(transactionDirectory);if(!fs::create_directory(transactionDirectory))throw std::runtime_error("Choose a new restore transaction directory.");
 auto journalPath=transactionDirectory/L"restore.json";
 Json journal={{"Format","UDM restore transaction"},{"Version",1},{"Id",id},{"Status","Preparing"},{"Files",rows},{"Directories",directoryRows}};
 atomicText(journalPath,journal.dump(2),false);
 try{
  if(checkpoint)checkpoint("journal-prepared");
  fs::create_directory(transactionDirectory/L"directories");
  for(auto& row:directoryRows){
   cancel.check();auto path=fs::path(wide(str(row,"Target")));restorePlainPath(path);
   if(yes(row,"Existed")){if(!fs::is_directory(path))throw std::runtime_error("Restore directory changed.");continue;}
   if(!fs::create_directory(path))throw std::runtime_error("Restore directory appeared after review.");
   if(checkpoint)checkpoint("directory-created");
   auto identity=recoveryDirectoryIdentity(path);
   atomicText(transactionDirectory/L"directories"/wide(str(row,"Index")+".json"),Json{{"Target",str(row,"Target")},{"Identity",identity}}.dump(),false);
   if(checkpoint)checkpoint("directory-recorded");
  }
  for(auto& row:rows){
   cancel.check();auto target=fs::path(wide(str(row,"Target"))),fresh=fs::path(wide(str(row,"New")));
   restorePlainPath(target);
   recoveryCopy(fs::path(wide(str(row,"Source"))),fresh,cancel,[&](i64 done,i64 total){if(checkpoint&&done>0&&done<total)checkpoint("copy-progress");});
   if(recoveryHash(fresh,cancel)!=str(row,"After"))throw std::runtime_error("Restore image changed while staging.");
   if(checkpoint)checkpoint("file-staged");
  }
  journal["Status"]="Applying";atomicText(journalPath,journal.dump(2),false);
  for(auto& row:rows){
   cancel.check();auto target=fs::path(wide(str(row,"Target"))),fresh=fs::path(wide(str(row,"New"))),old=fs::path(wide(str(row,"Old")));
   restorePlainPath(target);
   if(fs::exists(target)!=yes(row,"Existed")||(fs::exists(target)&&recoveryHash(target,cancel)!=str(row,"Before")))throw std::runtime_error("Restore destination changed after review.");
   if(yes(row,"Existed"))restoreMove(target,old);
   if(checkpoint)checkpoint("original-moved");
   restoreMove(fresh,target);
   if(checkpoint)checkpoint(yes(row,"State")?"state-installed":"file-installed");
  }
  cancel.check();for(auto& row:rows)if(recoveryHash(fs::path(wide(str(row,"Target"))),cancel)!=str(row,"After"))throw std::runtime_error("Restored file verification failed.");
  journal["Status"]="Committed";atomicText(journalPath,journal.dump(2),false);return journal;
 }catch(...){
  auto error=std::current_exception();
  try{rollbackRecovery(journalPath,approved);}catch(const std::exception& e){throw std::runtime_error(std::string("Restore stopped; rollback needs attention: ")+e.what());}
  std::rethrow_exception(error);
 }
}
}
