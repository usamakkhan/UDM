#include "Core.hpp"
#include <iostream>
using namespace udm;
using StartupRecovery=Manager::StartupRecovery;
static Json checks=Json::array();
static void check(bool value,const char* name){checks.push_back({{"name",name},{"passed",value}});if(!value)throw std::runtime_error(name);}
static Json record(const fs::path& files,int n,const std::string& status){
 auto id=std::string(31,'0')+"0123456789abcdef"[n];
 return {{"Id",id},{"FileName","record-"+std::to_string(n)+".bin"},{"Folder",utf8(files.wstring())},{"Url","https://fixture.invalid/record"},{"Status",status},{"Queue","Main queue"},{"Description",std::string(2048,'x')},{"Segments",Json::array({{{"Index",0},{"Start",0},{"Done",17}}})},{"UnknownExtension",{{"nested",Json::array({true,nullptr,17,"metadata"})}}}};
}
static Json catalog(const fs::path& files,Json rows){auto settings=defaultSettings();settings["DownloadFolder"]=utf8(files.wstring());settings["CategoryFolders"]=false;settings["PrefetchFileInfo"]=false;return {{"Schema",1},{"Settings",settings},{"Downloads",std::move(rows)},{"Queues",Json::array({defaultQueue()})},{"Projects",Json::array()},{"PrivateExtension",{{"opaque",Json::array({1,2,3})}}},{"QuotaStart",date()},{"QuotaBytes",7}};}
int wmain(int argc,wchar_t**argv){
 if(argc!=2)return 2;auto root=fs::absolute(argv[1]);if(fs::exists(root))return 2;fs::create_directories(root);CoInitializeEx(nullptr,COINIT_MULTITHREADED);std::string error;
 try{
  for(const auto mode:{StartupRecovery::Immediate,StartupRecovery::Desktop}){
   const auto folder=root/(mode==StartupRecovery::Desktop?L"desktop":L"immediate");fs::create_directories(folder);const auto files=folder/L"files";fs::create_directories(files);Json rows=Json::array();
   const std::vector<std::string> statuses={"Complete","Paused","Queued","Failed","Downloading","Pausing","Verifying","Merging","Resolving","Awaiting confirmation"};
   for(size_t i=0;i<statuses.size();++i){auto row=record(files,(int)i,statuses[i]);if(i==0){row["FileName"]="CON.txt";row["QueueMember"]=false;}if(i==1){row["SourceUrl"]="https://fixture.invalid/video";row["PrefetchedFinalSize"]=123;}rows.push_back(std::move(row));}
   auto parent=record(files,10,"Paused");parent["Video"]=record(files,11,"Complete");parent["Audio"]=record(files,12,"Paused");rows.push_back(parent);const auto input=catalog(files,rows);const auto serialized=input.dump();atomicText(folder/L"state.json",serialized,false);
   Json expected;
   {Manager m(folder,mode);check(m.jobs.size()==rows.size(),"Load keeps every download in original order");check(m.state["Downloads"].empty(),"Loaded rows are held by jobs rather than duplicated in manager state");check(m.state["PrivateExtension"]==input["PrivateExtension"]&&num(m.state,"QuotaBytes")==7,"Load retains non-download metadata and quota");
    for(size_t i=0;i<statuses.size();++i){auto job=m.jobs[i];check(job->id()==str(rows[i],"Id")&&job->data["UnknownExtension"]==rows[i]["UnknownExtension"]&&job->data["Segments"]==rows[i]["Segments"]&&str(job->data,"Description")==std::string(2048,'x'),"Loaded record retains identity, arbitrary metadata, segments and long description");const bool interrupted=i>=4;check(str(job->data,"Status")== (interrupted?"Paused":statuses[i])&&!yes(job->data,"ConfirmationPending"),"Interrupted saved status is normalized before admission");check(yes(job->data,"QueueMember")== (i!=0),"Legacy queue membership remains consistent with saved completion");}
    check(str(m.jobs[0]->data,"FileName")=="_CON.txt","Saved reserved filename is still normalized");check(str(m.jobs[1]->data,"MediaPhase")=="Paused"&&!m.jobs[1]->data.contains("PrefetchedFinalSize"),"Adaptive startup discards stale prefetched size and normalizes phase");auto job=m.jobs.back();check(job->video&&job->audio&&job->video->id()==str(parent["Video"],"Id")&&job->audio->id()==str(parent["Audio"],"Id"),"Nested tracks retain independent identities");check(job->video->data["UnknownExtension"]==parent["Video"]["UnknownExtension"]&&job->audio->data["Segments"]==parent["Audio"]["Segments"],"Nested tracks retain payload metadata");expected=m.snapshot();m.stop();check(Json::parse(readText(folder/L"state.json"))["Downloads"]==expected["Downloads"],"Normal shutdown persists every normalized record and nested track");}
   check(readText(folder/L"state.before-native.json")==serialized,"Original first-load backup remains byte exact");{Manager m(folder,mode);check(m.snapshot()["Downloads"]==expected["Downloads"],"Reload preserves normalized records and nested tracks exactly");m.stop();}
  }
  for(const bool existingBackup:{false,true})for(const std::string fault:{"late-id","late-segment","nested-id","wrong-schema"}){
   auto folder=root/wide(fault+(existingBackup?"-backup":"-new"));fs::create_directories(folder);Json rows=Json::array();for(int i=0;i<10;++i)rows.push_back(record(folder/L"files",i,"Complete"));
   if(fault=="late-id")rows.back()["Id"]="invalid";
   if(fault=="late-segment")rows.back()["Segments"].push_back(rows.back()["Segments"][0]);
   if(fault=="nested-id"){rows.back()["Audio"]=record(folder/L"files",12,"Paused");rows.back()["Audio"]["Id"]="invalid";}
   auto input=catalog(folder/L"files",rows);if(fault=="wrong-schema")input["Schema"]=99;const auto raw=input.dump();atomicText(folder/L"state.json",raw,false);const auto backup=folder/L"state.before-native.json";const std::string prior="Preexisting backup must remain byte exact";if(existingBackup)writeBytes(backup,Bytes(prior.begin(),prior.end()));
   bool refused=false;try{Manager m(folder);}catch(const std::exception&){refused=true;}check(refused,"Invalid late or nested record and unsupported schema are refused");check(readText(folder/L"state.json")==raw,"Failed construction never publishes a partially adopted catalog");check(!fs::exists(folder/L"state.json.tmp")&&!fs::exists(folder/L"state.json.bak"),"Failed construction does not start a catalog write");check(existingBackup?readText(backup)==prior:(fault=="wrong-schema"?!fs::exists(backup):readText(backup)==raw),"Failed construction preserves existing backup or original validated-input backup");
  }
 }catch(const std::exception& e){error=e.what();}
 auto result=Json{{"passed",error.empty()},{"error",error},{"checks",checks}};atomicText(root/L"results.json",result.dump(2),false);std::cout<<checks.size()<<" checks; "<<(error.empty()?"passed":error)<<"\n";CoUninitialize();return error.empty()?0:1;
}
