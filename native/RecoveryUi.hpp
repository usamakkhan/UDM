#pragma once
#include "Ui.hpp"
#include "RecoverySession.hpp"
namespace udm {
inline void launchRecoveryMode(const fs::path& data,bool recovery){
 wchar_t executable[32768];auto length=GetModuleFileNameW(nullptr,executable,32768);if(!length||length>=32768)throw std::runtime_error("Cannot locate UDM.");
 std::wstring command=quote(executable)+L" --data-dir "+quote(data.wstring())+L" --wait-process "+std::to_wstring(GetCurrentProcessId());
 if(recovery)command+=L" --recovery";
 wchar_t tag[40];auto count=GetEnvironmentVariableW(L"UDM_INSTANCE_TAG",tag,40);if(count&&count<40)command+=L" --instance-tag "+quote(std::wstring(tag,count));
 STARTUPINFOW startup{sizeof(startup)};PROCESS_INFORMATION process{};
 if(!CreateProcessW(executable,command.data(),nullptr,nullptr,FALSE,0,nullptr,nullptr,&startup,&process))throw std::runtime_error("Cannot open UDM recovery.");
 CloseHandle(process.hThread);CloseHandle(process.hProcess);
}
class RecoveryWorkDialog:public Form{
 Cancel cancellation;std::thread worker;std::atomic_bool done{false};Json value;std::string failure;bool cancelled=false;
 CWnd* status=nullptr;CWnd* cancelButton=nullptr;
public:
 RecoveryWorkDialog(CWnd* owner,std::string title,std::function<Json(const Cancel&)> operation,bool allowCancel=true):Form(title,500,160,owner){
  init=[this,operation,allowCancel]{
   label("Keep UDM closed until this operation finishes.",16,16,468,24);
   status=label("Working...",16,49,468,40);
   auto bar=make<CProgressCtrl>(PBS_MARQUEE,16,94,468,12);bar->SendMessage(PBM_SETMARQUEE,TRUE,40);
   cancel=[this,allowCancel]{if(!allowCancel)return;cancellation.stop=true;status->SetWindowText(L"Cancelling; preserving your existing files...");cancelButton->EnableWindow(FALSE);};
   cancelButton=button("Cancel",384,124,100,cancel);cancelButton->EnableWindow(allowCancel);if(!allowCancel){status->SetWindowText(L"Recovering saved files; please wait...");GetSystemMenu(FALSE)->EnableMenuItem(SC_CLOSE,MF_BYCOMMAND|MF_GRAYED);}
   worker=std::thread([this,operation]{try{value=operation(cancellation);}catch(const Cancelled&){cancelled=true;}catch(const std::exception& e){failure=e.what();}catch(...){failure="Recovery operation failed.";}done.store(true,std::memory_order_release);});
  };
  pulse=[this]{if(done.load(std::memory_order_acquire)){if(worker.joinable())worker.join();close(cancelled?IDCANCEL:IDOK);}};
 }
 ~RecoveryWorkDialog(){cancellation.stop=true;if(worker.joinable())worker.join();}
 bool run(Json& output){auto code=DoModal();if(!failure.empty())throw std::runtime_error(failure);if(code!=IDOK||cancelled)return false;output=value;return true;}
};
class RecoveryCenterDialog:public Form{
 const RecoveryDataLease& lease;CWnd *backupPath=nullptr,*imagePath=nullptr,*status=nullptr;Json reviewed;std::string reviewedImage,reviewedHash;
 bool work(const std::string& title,std::function<Json(const Cancel&)> op,Json& out){
  RecoveryWorkDialog dialog(this,title,std::move(op));return dialog.run(out);
 }
 void chooseBackup(){
  auto folder=chooseFolder(this,lease.root.parent_path().wstring());
  if(!folder.empty())backupPath->SetWindowText(cs(utf8((fs::path(folder)/wide("UDM-backup-"+guid().substr(0,8))).wstring())));
 }
 void backup(){
  auto destination=fs::path(wide(trim(text(backupPath))));Json result;
  if(!work("Creating UDM backup",[&](const Cancel& c){recoverPendingRestore(lease);Manager manager(lease.root);return writeRecoveryImage(manager,destination,c);},result)){status->SetWindowText(L"Backup cancelled. Existing downloads are unchanged.");return;}
  auto missing=result["MissingFiles"].size();
  status->SetWindowText(cs("Backup verified: "+std::to_string(result["Files"].size())+" files. "+std::to_string(missing)+" previously missing files could not be included."));
  imagePath->SetWindowText(cs(utf8(destination.wstring())));reviewed=Json();
 }
 bool verify(){
  auto path=fs::path(wide(trim(text(imagePath))));Json result;
  if(!work("Verifying UDM backup",[&](const Cancel& c){return verifyRecoveryImage(path,c);},result)){status->SetWindowText(L"Verification cancelled.");return false;}
  if(backupKey(fs::path(wide(str(result,"DataDirectory"))))!=backupKey(lease.root))throw std::runtime_error("This backup belongs to another UDM data folder.");
  reviewed=result;reviewedImage=utf8(path.wstring());reviewedHash=fileHash(path/L"manifest.json");
  status->SetWindowText(cs("Verified "+std::to_string(result["Files"].size())+" files. Missing at backup: "+std::to_string(result["MissingFiles"].size())+"."));
  return true;
 }
 void restore(){
  if(!verify())return;
  Form review("Restore UDM backup",720,420,this);
  review.init=[&]{
   review.label("These files and folders will be restored to their original locations. Existing versions are retained for recovery.",14,14,692,38);
   auto list=review.make<CListCtrl>(WS_BORDER|WS_TABSTOP|LVS_REPORT,14,60,692,249);list->SetExtendedStyle(LVS_EX_FULLROWSELECT|LVS_EX_DOUBLEBUFFER);
   list->InsertColumn(0,L"Original location",LVCFMT_LEFT,review.rect(0,0,555,0).Width());list->InsertColumn(1,L"Size",LVCFMT_RIGHT,review.rect(0,0,110,0).Width());
   int index=0;for(auto& row:reviewed["Files"]){list->InsertItem(index,cs(str(row,"Original")));list->SetItemText(index++,1,cs(bytes(num(row,"Size"))));}
   for(auto& directory:reviewed.value("Directories",Json::array())){list->InsertItem(index,cs(directory.get<std::string>()));list->SetItemText(index++,1,L"Folder");}
   review.label("Passwords and browser request credentials require the original Windows account. Missing files listed at backup cannot be recovered.",14,321,692,42);
   review.button("Restore",478,379,108,[&]{review.close();});review.button("Cancel",598,379,108,[&]{review.close(IDCANCEL);});
  };
  if(review.DoModal()!=IDOK)return;
  auto image=fs::path(wide(reviewedImage));auto expected=reviewedHash;auto approved=recoveryDestinations(reviewed);
  auto transaction=lease.root.parent_path()/wide("UDM-restore-"+guid());Json result;
  if(!work("Restoring UDM backup",[&](const Cancel& c){
   recoverPendingRestore(lease);if(recoveryHash(image/L"manifest.json",c)!=expected)throw std::runtime_error("The backup changed after review. Verify it again.");
   return restoreWithSession(lease,image,approved,transaction,c);
  },result)){status->SetWindowText(L"Restore cancelled. Original files were recovered.");return;}
  status->SetWindowText(L"Restore completed and verified. Choose Open UDM to use the restored downloads.");
 }
public:
 explicit RecoveryCenterDialog(const RecoveryDataLease& data,CWnd* parent=nullptr):Form("UDM backup and recovery",650,378,parent),lease(data){
  init=[this]{
   label("Back up settings, queues, download history, temporary files and saved downloads.",14,14,622,38);
   label("New backup folder",14,59,622);
   backupPath=edit(utf8((lease.root.parent_path()/wide("UDM-backup-"+guid().substr(0,8))).wstring()),14,83,516);
   button("Browse...",542,82,94,[this]{chooseBackup();});button("Create backup",14,120,137,[this]{backup();});
   control(L"STATIC","",SS_ETCHEDHORZ,14,160,622,2);
   label("Existing backup folder",14,176,622);imagePath=edit("",14,200,516);
   button("Browse...",542,199,94,[this]{auto folder=chooseFolder(this,lease.root.parent_path().wstring());if(!folder.empty())imagePath->SetWindowText(folder.c_str());});
   button("Verify",14,237,103,[this]{verify();});button("Restore...",129,237,114,[this]{restore();});
   status=label("UDM remains closed while backup and recovery are open.",14,278,622,49);
   button("Open UDM",412,338,108,[this]{if(fs::exists(lease.marker()))throw std::runtime_error("Finish recovery before opening UDM.");launchRecoveryMode(lease.root,false);close();});
   button("Close",532,338,104,[this]{close(IDCANCEL);});
  };
 }
};
}
