#include "Core.hpp"
#include <iostream>
using namespace udm;
int wmain(int argc,wchar_t** argv){
 if(argc!=2)return 2;auto root=fs::absolute(argv[1]);if(fs::exists(root))return 2;fs::create_directories(root);
 Json checks=Json::array();std::string error;
 auto check=[&](bool ok,const char* name){checks.push_back({{"name",name},{"passed",ok}});if(!ok)throw std::runtime_error(name);};
 try{
  for(int mode=0;mode<10;++mode){auto data=root/(L"legacy-"+std::to_wstring(mode));auto owner=std::make_unique<Manager>(data);auto& m=*owner;auto prefs=m.state["Settings"];prefs["DownloadFolder"]=utf8((data/L"files").wstring());prefs["CategoryFolders"]=false;m.setSettings(prefs);auto job=m.add("https://example.invalid/legacy.bin");fs::create_directories(job->target().parent_path());atomicText(job->target(),"verified bytes",false);auto hash=fileHash(job->target());
   job->data["Status"]="Failed";job->data["Error"]="Could not replace the saved state (Windows error 1175). Retry after closing programs that hold the state file.";job->data["Size"]=14;job->data["Received"]=14;job->data["Sha256"]=hash;job->data["Finished"]=date();job->data["QueueMember"]=false;
   if(mode==1)atomicText(job->target(),"modified bytes",false);if(mode==2)job->data["Size"]=15;if(mode==3)job->data["ExpectedSha256"]=std::string(64,'0');if(mode==4)job->data.erase("Finished");if(mode==5)job->data["Error"]="Server rejected the download.";if(mode==6)job->data.erase("Sha256");if(mode==8)fs::remove(job->target());m.save();auto before=job->data;
   if(mode==7){Handle reader(CreateFileW((data/L"state.json.bak").c_str(),GENERIC_READ,FILE_SHARE_READ|FILE_SHARE_WRITE,nullptr,OPEN_EXISTING,0,nullptr));bool failed=false;try{m.recoverReplacements();}catch(...){failed=true;}check(bool(reader)&&failed&&job->data==before,"Legacy repair rolls back in-memory status when its save fails");}
   if(mode==9){Handle writer(CreateFileW(job->target().c_str(),GENERIC_WRITE,FILE_SHARE_READ|FILE_SHARE_WRITE,nullptr,OPEN_EXISTING,0,nullptr));m.recoverReplacements();check(bool(writer)&&job->data==before,"A file open for writing cannot be reconciled from an unstable hash");}
   owner.reset();{Manager reopened(data);auto restored=reopened.jobs.at(0);if(mode==0||mode==7||mode==9)check(str(restored->data,"Status")=="Complete"&&str(restored->data,"Error").empty()&&str(restored->data,"Sha256")==hash&&restored->data["Finished"]==before["Finished"]&&!yes(restored->data,"QueueMember"),"Verified legacy publication recovers without another transfer or changing finish time");else check(str(restored->data,"Status")=="Failed"&&!restored->data.contains("PublicationRecovered"),"Incomplete or changed legacy evidence cannot claim completion");}
   Manager again(data);check(str(again.jobs.at(0)->data,"Status")==((mode==0||mode==7||mode==9)?"Complete":"Failed"),"Legacy recovery decision survives another catalog reopen");
  }
  for(int mode=0;mode<5;++mode){
  auto data=root/std::to_wstring(mode);auto owner=std::make_unique<Manager>(data);auto& m=*owner;auto prefs=m.state["Settings"];prefs["DownloadFolder"]=utf8((data/L"files").wstring());prefs["CategoryFolders"]=false;m.setSettings(prefs);
  auto job=m.add("https://example.invalid/file.bin");auto staging=data/L"assembled.part";atomicText(staging,"verified bytes",false);auto hash=fileHash(staging);auto journal=data/L"publications"/(wide(job->id())+L".json");
  fs::create_directories(job->target().parent_path());
  if(mode==3){fs::remove(job->target().parent_path());bool rejected=false;try{m.publishFile(job,staging,hash);}catch(const std::exception&){rejected=true;}
   check(rejected&&fs::exists(journal)&&readText(staging)=="verified bytes","Failed rename preserves staging and publication intent");owner.reset();Manager reopened(data);check(str(reopened.jobs.at(0)->data,"Status")=="Paused"&&readText(staging)=="verified bytes"&&!fs::exists(journal),"Unpublished intent recovers paused without publishing or deleting bytes");continue;
  }
  {Handle reader(CreateFileW((data/L"state.json.bak").c_str(),GENERIC_READ,FILE_SHARE_READ|FILE_SHARE_WRITE,nullptr,OPEN_EXISTING,0,nullptr));check(bool(reader),"Backup state is held by another reader");bool rejected=false;try{m.publishFile(job,staging,hash);}catch(const std::exception&){rejected=true;}
   check(rejected&&readText(job->target())=="verified bytes"&&!fs::exists(staging)&&fs::exists(journal),"Save failure retains exact published file and durable intent");
  }
  if(mode==0){owner.reset();{Manager reopened(data);check(str(reopened.jobs.at(0)->data,"Status")=="Complete"&&str(reopened.jobs.at(0)->data,"Sha256")==hash&&!fs::exists(journal),"Restart reconciles published bytes to completed history");}Manager reopenedAgain(data);check(str(reopenedAgain.jobs.at(0)->data,"Status")=="Complete","Recovered completion survives closing and opening another manager");}
  if(mode==1){m.resume(job);check(str(job->data,"Status")=="Complete"&&!yes(job->data,"IndividualStart")&&!fs::exists(journal),"Resume reconciles locally without scheduling another request");owner.reset();Manager reopened(data);check(str(reopened.jobs.at(0)->data,"Status")=="Complete","Resume reconciliation is durable");}
  if(mode==4){m.remove(job);owner.reset();Manager reopened(data);check(reopened.jobs.empty()&&!fs::exists(journal)&&readText(job->target())=="verified bytes","Removing a recovered record preserves its file and leaves no orphan journal");}
  if(mode==2){atomicText(job->target(),"changed by user",false);owner.reset();bool rejected=false;try{Manager reopened(data);}catch(const std::exception&){rejected=true;}
   check(rejected&&readText(job->target())=="changed by user"&&fs::exists(journal),"Changed destination is preserved and never falsely marked complete");}
 }}catch(const std::exception& e){error=e.what();}
 auto result=Json{{"passed",error.empty()},{"error",error},{"checks",checks}};atomicText(root/L"results.json",result.dump(2),false);std::cout<<result.dump(2)<<std::endl;return error.empty()?0:1;
}