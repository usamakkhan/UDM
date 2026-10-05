#pragma once
#include "Restore.hpp"
namespace udm {
class RecoveryDataLease{
 Handle handle;bool owned=false;
public:
 const fs::path root;
 explicit RecoveryDataLease(const fs::path& data):root(backupAbsolute(data)){
  auto key=lower(utf8(root.wstring()));BCRYPT_ALG_HANDLE algorithm=nullptr;BYTE hash[32];
  if(BCryptOpenAlgorithmProvider(&algorithm,BCRYPT_SHA256_ALGORITHM,nullptr,0)<0)throw std::runtime_error("Cannot initialize data lock.");
  auto status=BCryptHash(algorithm,nullptr,0,reinterpret_cast<PUCHAR>(key.data()),(ULONG)key.size(),hash,sizeof(hash));
  BCryptCloseAlgorithmProvider(algorithm,0);if(status<0)throw std::runtime_error("Cannot identify data lock.");
  std::ostringstream out;for(auto byte:hash)out<<std::hex<<std::setfill('0')<<std::setw(2)<<(int)byte;
  auto name=L"Local\\UDM.RecoveryData."+wide(out.str());handle.h=CreateMutexW(nullptr,FALSE,name.c_str());
  if(!handle)throw std::runtime_error("Cannot create data lock.");
  auto wait=WaitForSingleObject(handle.h,0);owned=wait==WAIT_OBJECT_0||wait==WAIT_ABANDONED;
  if(!owned)throw std::runtime_error("This UDM data folder is already open. Close that instance before recovery.");
 }
 ~RecoveryDataLease(){if(owned)ReleaseMutex(handle.h);}
 RecoveryDataLease(const RecoveryDataLease&)=delete;RecoveryDataLease& operator=(const RecoveryDataLease&)=delete;
 fs::path marker()const{return root/L".udm-restore-pending";}
};
// Called before constructing Manager, starting queues or exposing browser IPC.
// The account-bound marker authenticates the reviewed targets, not arbitrary journal input.
inline Json recoverPendingRestore(const RecoveryDataLease& lease){
 auto marker=lease.marker();if(!fs::exists(marker))return Json();
 restorePlainPath(marker);auto envelope=Json::parse(readText(marker,32*1024*1024));
 if(str(envelope,"Format")!="UDM pending restore"||num(envelope,"Version")!=1)throw std::runtime_error("Invalid pending restore marker; data folder stays closed.");
 auto pending=Json::parse(reveal(str(envelope,"Protected")));
 if(backupKey(fs::path(wide(str(pending,"DataDirectory"))))!=backupKey(lease.root))throw std::runtime_error("Restore marker belongs to another data folder.");
 auto journal=fs::path(wide(str(pending,"Journal")));
 if(!pending["Approved"].is_array()||!journal.is_absolute())throw std::runtime_error("Invalid pending restore destinations.");
 auto approved=pending["Approved"].get<std::set<std::string>>();Json result;
 if(fs::exists(journal))result=rollbackRecovery(journal,approved);
 else throw std::runtime_error("Restore journal is missing; data folder stays closed.");
 if(!fs::remove(marker))throw std::runtime_error("Cannot clear completed restore marker.");
 return result;
}
inline Json restoreWithSession(const RecoveryDataLease& lease,const fs::path& image,
 const std::set<std::string>& approved,const fs::path& transactionDirectory,const Cancel& cancel,
 const std::function<void(const char*)>& checkpoint={}){
 if(fs::exists(lease.marker()))throw std::runtime_error("Recover the previous restore before starting another.");
 auto manifest=verifyRecoveryImage(image,cancel);
 if(backupKey(fs::path(wide(str(manifest,"DataDirectory"))))!=backupKey(lease.root))throw std::runtime_error("Choose a backup of this UDM data folder.");
 for(auto& row:manifest["Files"])if(backupKey(fs::path(wide(str(row,"Original"))))==backupKey(lease.marker()))throw std::runtime_error("Backup contains a pending restore marker.");
 if(fs::exists(transactionDirectory))throw std::runtime_error("Choose a new restore transaction directory.");
 fs::create_directories(lease.root);
 Json pending={{"DataDirectory",utf8(lease.root.wstring())},{"Journal",utf8((transactionDirectory/L"restore.json").wstring())},{"Approved",approved}};

 try{
  auto result=restoreRecoveryImage(image,approved,transactionDirectory,cancel,[&](const char* point){
   if(std::string(point)=="journal-prepared"){
    atomicText(lease.marker(),Json{{"Format","UDM pending restore"},{"Version",1},{"Protected",protect(pending.dump())}}.dump(),false);
    if(checkpoint)checkpoint("marker-written");
   }
   if(checkpoint)checkpoint(point);
  });
  if(checkpoint)checkpoint("restore-committed");
  recoverPendingRestore(lease);return result;
 }catch(...){
  auto error=std::current_exception();
  // A failed rollback leaves the marker in place. Startup must refuse to open the catalog.
  if(fs::exists(lease.marker()))recoverPendingRestore(lease);std::rethrow_exception(error);
 }
}
}
