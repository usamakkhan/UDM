#include "Core.hpp"
#ifndef UDM_RECOVERY_JOURNAL_BASELINE
#include "RecoveryJournal.hpp"
#endif
#include <future>
#include <iostream>
namespace udm {struct CatalogCheckpointTestAccess {static void observe(Manager& manager,std::function<void(const char*)> callback){Lock lock(manager.mutex);manager.catalogCheckpoint=std::move(callback);}};}
using namespace udm;
int wmain(int argc,wchar_t** argv){
 if(argc!=2)return 2;const auto root=fs::absolute(argv[1]);if(fs::exists(root))return 2;fs::create_directories(root);
 Json checks=Json::array(),cases=Json::array();bool passed=false;std::string error;
 auto check=[&](bool value,const char* label){checks.push_back({{"name",label},{"passed",value}});};
 try{
  for(const bool paired:{false,true})for(const std::string attack:{"rename","write","write-alias","save-failure","read-held","unrelated-held"}){
   const auto folder=root/wide(std::string(paired?"replacement-":"ordinary-")+attack);Manager manager(folder/L"catalog");const auto files=folder/L"files";fs::create_directories(files);
   auto settings=manager.state["Settings"];settings["DownloadFolder"]=utf8(files.wstring());settings["CategoryFolders"]=false;manager.setSettings(settings);
   JobPtr original;std::string oldHash;
   if(paired){original=manager.add("https://example.invalid/original","","result.bin","Main queue",true);atomicText(original->target(),"old bytes",false);oldHash=fileHash(original->target());original->data["Status"]="Complete";original->data["Size"]=9;original->data["Received"]=9;original->data["Sha256"]=oldHash;original->data["Finished"]=date();}
   auto job=manager.add("https://example.invalid/recovered","",paired?"next.bin":"result.bin","Main queue",true);
   if(paired){job->data["FileName"]=str(original->data,"FileName");job->data["ReplacementOf"]=original->id();job->data["ReplacementHash"]=oldHash;job->data["PreviousPath"]=utf8((files/L"previous.bin").wstring());}
   manager.save();const auto stage=files/(L".udm-"+wide(job->id())+L".assembling");atomicText(stage,"verified bytes",false);const auto hash=fileHash(stage);
   const auto journal=manager.root/(paired?L"replacements":L"publications")/(wide(job->id())+L".json"),retained=folder/L"retained-journal.bin",alias=folder/L"journal-alias.bin";
   bool interrupted=false;CatalogCheckpointTestAccess::observe(manager,[&](const char* point){if(std::string_view(point)==(paired?"replacement-file-moved":"publication-intent-written"))throw std::runtime_error("Owned publication interruption");});
   try{manager.publishFile(job,stage,hash);}catch(...){interrupted=true;}CatalogCheckpointTestAccess::observe(manager,{});
   check(interrupted&&fs::exists(journal),"A real interrupted ordinary or paired publication supplies the recovery journal");
   const auto bytes=readText(journal),before=job->snapshot().dump();if(attack=="write-alias")check(CreateHardLinkW(alias.c_str(),journal.c_str(),nullptr)!=FALSE,"The fixture creates a second real name for the journal bytes");
   Handle unrelatedReader;const auto unrelated=manager.root/L"replacements"/(wide(guid())+L".json");const auto unrelatedBytes=Json{{"Id",guid()},{"OriginalId",guid()}}.dump();if(attack=="unrelated-held"){atomicText(unrelated,unrelatedBytes,false);unrelatedReader.h=CreateFileW(unrelated.c_str(),GENERIC_READ,FILE_SHARE_READ|FILE_SHARE_WRITE,nullptr,OPEN_EXISTING,0,nullptr);check(static_cast<bool>(unrelatedReader),"An unrelated scoped replacement journal refuses deletion sharing");}
   const std::string newer="{\"IndependentNewerIntent\":true}";bool reached=false,mutated=false,readable=false,modelAvailable=false;std::string failure;
   CatalogCheckpointTestAccess::observe(manager,[&](const char* point){
    if(std::string_view(point)!="temporary-flushed"||reached)return;reached=true;
    modelAvailable=std::async(std::launch::async,[&]{std::unique_lock<std::recursive_mutex> lease(manager.mutex,std::try_to_lock);return lease.owns_lock();}).get();
    if(attack=="save-failure")throw std::runtime_error("Owned recovery catalog failure");
    if(attack=="rename"){
     mutated=MoveFileExW(journal.c_str(),retained.c_str(),0)!=FALSE;if(mutated)atomicText(journal,newer,false);
    }else if(attack=="write"||attack=="write-alias"){
     Handle writer(CreateFileW((attack=="write-alias"?alias:journal).c_str(),GENERIC_WRITE,FILE_SHARE_READ|FILE_SHARE_WRITE|FILE_SHARE_DELETE,nullptr,OPEN_EXISTING,0,nullptr));
     if(writer){DWORD count=0;mutated=WriteFile(writer.h,newer.data(),static_cast<DWORD>(newer.size()),&count,nullptr)&&count==newer.size()&&SetEndOfFile(writer.h);}
    }
    Handle reader(CreateFileW(journal.c_str(),GENERIC_READ,FILE_SHARE_READ|FILE_SHARE_WRITE|FILE_SHARE_DELETE,nullptr,OPEN_EXISTING,0,nullptr));
    if(reader){std::string observed(bytes.size(),'\0');DWORD count=0;readable=ReadFile(reader.h,observed.data(),static_cast<DWORD>(observed.size()),&count,nullptr)&&count==observed.size()&&observed==bytes;}
   });
   {
    Handle reader;if(attack=="read-held")reader.h=CreateFileW(journal.c_str(),GENERIC_READ,FILE_SHARE_READ|FILE_SHARE_WRITE,nullptr,OPEN_EXISTING,0,nullptr);
    if(attack=="read-held")check(static_cast<bool>(reader),"An existing journal reader deliberately refuses deletion sharing");
    try{manager.recoverReplacements(job);}catch(const std::exception& e){failure=e.what();}
   }
   CatalogCheckpointTestAccess::observe(manager,{});
   cases.push_back({{"paired",paired},{"attack",attack},{"catalogBoundaryReached",reached},{"journalMutationSucceeded",mutated},{"newerJournalPathWasDeleted",mutated&&!fs::exists(journal)},{"sharedReaderSawOriginalBytes",readable},{"modelMutexAvailableDuringCatalogIo",modelAvailable},{"recoveryReportedFailure",!failure.empty()}});
   if(attack=="save-failure"||attack=="read-held"){
    check(!failure.empty()&&fs::exists(journal)&&readText(journal)==bytes,"Failed catalog persistence or refused journal ownership retains the exact original journal");
    check(job->snapshot().dump()==before,"Refused recovery cannot leave a falsely accepted completion row");
    if(fs::exists(journal)&&readText(journal)==bytes){manager.recoverReplacements(job);check(str(job->data,"Status")=="Complete"&&!fs::exists(journal),"A later explicit retry accepts preserved bytes and retires the owned journal");}
   }else{
    check(reached&&!mutated,"Recovery excludes pathname replacement and writes through every journal hard link during its actual catalog flush");
    check(readable,"A reader sharing deletion can inspect the exact pinned journal during recovery");
    check(failure.empty()&&str(job->data,"Status")=="Complete"&&!fs::exists(journal),"Successful recovery retires its own journal after catalog acceptance");
   }
   if(attack=="unrelated-held")check(fs::exists(unrelated)&&readText(unrelated)==unrelatedBytes,"Scoped recovery leaves an unrelated locked journal untouched without claiming it");
   check(fs::exists(job->target())&&fileHash(job->target())==hash,"Recovery retains the exact downloaded output bytes");
   if(paired)check(fs::exists(files/L"previous.bin")&&fileHash(files/L"previous.bin")==oldHash,"Paired recovery retains the original version's exact bytes");
   manager.stop();
  }
#ifndef UDM_RECOVERY_JOURNAL_BASELINE
  {
   const auto file=root/L"owned-journal.json",alias=root/L"owned-alias.json";atomicText(file,"\xEF\xBB\xBF{\"id\":1}",false);check(CreateHardLinkW(alias.c_str(),file.c_str(),nullptr)!=FALSE,"An owned journal has a retained second link");
   {RecoveryJournal journal(file);check(journal.text()=="{\"id\":1}","The held snapshot retains legacy UTF-8 BOM handling");
    Handle writer(CreateFileW(alias.c_str(),GENERIC_WRITE,FILE_SHARE_READ|FILE_SHARE_WRITE|FILE_SHARE_DELETE,nullptr,OPEN_EXISTING,0,nullptr));check(!writer,"A journal lease rejects byte writes through an alias");
    check(!MoveFileExW(file.c_str(),(root/L"moved-journal.json").c_str(),0),"A journal lease preserves its captured pathname");journal.retire();}
   check(!fs::exists(file)&&readText(alias)=="{\"id\":1}","Handle-based retirement removes only the owned link and preserves other links' bytes");
   atomicText(file,"bounded journal",false);bool denied=false;try{RecoveryJournal oversized(file,4);}catch(...){denied=true;}check(denied&&readText(file)=="bounded journal","An over-limit journal is rejected before use without removing its bytes");
   {RecoveryJournal abandoned(file);}check(readText(file)=="bounded journal","Releasing an unaccepted lease never deletes its journal");
   bool retirementDenied=false,readonly=false;{RecoveryJournal journal(file);readonly=SetFileAttributesW(file.c_str(),FILE_ATTRIBUTE_READONLY)!=FALSE;if(readonly)try{journal.retire();}catch(...){retirementDenied=true;}}
   check(readonly&&retirementDenied&&fs::exists(file)&&readText(file)=="bounded journal","A refused read-only retirement preserves the exact journal after its lease unwinds");
   check(SetFileAttributesW(file.c_str(),FILE_ATTRIBUTE_NORMAL)!=FALSE,"The fixture restores only its own journal attribute");{RecoveryJournal journal(file);journal.retire();}check(!fs::exists(file),"An explicit retry can retire the same journal once its refusal is removed");
   const auto directory=root/L"journal-directory";fs::create_directories(directory);denied=false;try{RecoveryJournal invalid(directory);}catch(...){denied=true;}check(denied&&fs::is_directory(directory),"A directory cannot be interpreted or retired as a journal");
  }
#endif
  passed=std::all_of(checks.begin(),checks.end(),[](const Json& value){return value.at("passed").get<bool>();});
 }catch(const std::exception& e){error=e.what();}
 atomicText(root/L"results.json",Json{{"passed",passed},{"error",error},{"checks",checks},{"cases",cases}}.dump(2),false);std::cout<<checks.size()<<" checks; "<<(passed?"passed":"failed")<<"\n";return passed?0:1;
}
