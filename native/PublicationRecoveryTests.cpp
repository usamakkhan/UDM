#include "Core.hpp"
#include <iostream>
using namespace udm;
int wmain(int argc,wchar_t** argv){
 if(argc!=2)return 2;auto root=fs::absolute(argv[1]);if(fs::exists(root))return 2;fs::create_directories(root);
 Json checks=Json::array();std::string error;
 auto check=[&](bool ok,const char* name){checks.push_back({{"name",name},{"passed",ok}});if(!ok)throw std::runtime_error(name);};
 try{for(int mode=0;mode<5;++mode){
  auto data=root/std::to_wstring(mode);Manager m(data);auto prefs=m.state["Settings"];prefs["DownloadFolder"]=utf8((data/L"files").wstring());prefs["CategoryFolders"]=false;m.setSettings(prefs);
  auto job=m.add("https://example.invalid/file.bin");auto staging=data/L"assembled.part";atomicText(staging,"verified bytes",false);auto hash=fileHash(staging);auto journal=data/L"publications"/(wide(job->id())+L".json");
  fs::create_directories(job->target().parent_path());
  if(mode==3){fs::remove(job->target().parent_path());bool rejected=false;try{m.publishFile(job,staging,hash);}catch(const std::exception&){rejected=true;}
   check(rejected&&fs::exists(journal)&&readText(staging)=="verified bytes","Failed rename preserves staging and publication intent");Manager reopened(data);check(str(reopened.jobs.at(0)->data,"Status")=="Paused"&&readText(staging)=="verified bytes"&&!fs::exists(journal),"Unpublished intent recovers paused without publishing or deleting bytes");continue;
  }
  {Handle reader(CreateFileW((data/L"state.json.bak").c_str(),GENERIC_READ,FILE_SHARE_READ|FILE_SHARE_WRITE,nullptr,OPEN_EXISTING,0,nullptr));check(bool(reader),"Backup state is held by another reader");bool rejected=false;try{m.publishFile(job,staging,hash);}catch(const std::exception&){rejected=true;}
   check(rejected&&readText(job->target())=="verified bytes"&&!fs::exists(staging)&&fs::exists(journal),"Save failure retains exact published file and durable intent");
  }
  if(mode==0){Manager reopened(data);check(str(reopened.jobs.at(0)->data,"Status")=="Complete"&&str(reopened.jobs.at(0)->data,"Sha256")==hash&&!fs::exists(journal),"Restart reconciles published bytes to completed history");}
  if(mode==1){m.resume(job);check(str(job->data,"Status")=="Complete"&&!yes(job->data,"IndividualStart")&&!fs::exists(journal),"Resume reconciles locally without scheduling another request");Manager reopened(data);check(str(reopened.jobs.at(0)->data,"Status")=="Complete","Resume reconciliation is durable");}
  if(mode==4){m.remove(job);Manager reopened(data);check(reopened.jobs.empty()&&!fs::exists(journal)&&readText(job->target())=="verified bytes","Removing a recovered record preserves its file and leaves no orphan journal");}
  if(mode==2){atomicText(job->target(),"changed by user",false);bool rejected=false;try{Manager reopened(data);}catch(const std::exception&){rejected=true;}
   check(rejected&&readText(job->target())=="changed by user"&&fs::exists(journal),"Changed destination is preserved and never falsely marked complete");}
 }}catch(const std::exception& e){error=e.what();}
 auto result=Json{{"passed",error.empty()},{"error",error},{"checks",checks}};atomicText(root/L"results.json",result.dump(2),false);std::cout<<result.dump(2)<<std::endl;return error.empty()?0:1;
}