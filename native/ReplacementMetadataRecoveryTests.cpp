#include "Core.hpp"
#include "PublicationCatalogProof.hpp"
#include <iostream>
namespace udm {struct CatalogCheckpointTestAccess {static void observe(Manager& m,std::function<void(const char*)> f){Lock lock(m.mutex);m.catalogCheckpoint=std::move(f);}};}
using namespace udm;
static const Json* metadataRow(const Json& catalog,const std::string& id){for(const auto& row:catalog.at("Downloads"))if(str(row,"Id")==id)return &row;return nullptr;}
int wmain(int argc,wchar_t** argv){
 if(argc!=2)return 2;const auto root=fs::absolute(argv[1]);if(fs::exists(root))return 2;fs::create_directories(root);Json checks=Json::array(),cases=Json::array();std::string error,activeCase,workflowError;
 const auto check=[&](bool good,const char* name){checks.push_back({{"name",name},{"passed",good}});if(!good)throw std::runtime_error(name);};
 try{
  for(const std::string branch:{"completed","retained","restored"})for(const bool legacy:{false,true})for(const bool privateJob:{false,true})for(const bool empty:{false,true}){
   if(legacy&&privateJob)continue;
   const std::vector<std::string> faults=branch=="completed"?std::vector<std::string>{"valid","new-count","old-count","new-received","old-received","new-fraction","old-fraction","new-missing","old-missing","new-negative","old-negative","new-overflow","old-overflow","new-hash","old-hash","new-path","old-path","save-failure"}:std::vector<std::string>{"valid","before-count","before-received","before-fraction","before-missing","before-negative","before-hash","before-path","stale-catalog","save-failure"};
   for(const auto& fault:faults){
    activeCase=branch+std::string(legacy?"-legacy":"-current")+(privateJob?"-private":"-visible")+(empty?"-empty":"-nonempty")+"-"+fault;workflowError.clear();const auto folder=root/wide(activeCase);Manager m(folder/L"catalog");const auto files=folder/L"files";fs::create_directories(files);auto prefs=m.state["Settings"];prefs["DownloadFolder"]=utf8(files.wstring());prefs["CategoryFolders"]=false;m.setSettings(prefs);
    const auto oldPayload=empty?std::string{}:std::string("old bytes"),newPayload=empty?std::string{}:std::string("verified bytes");const i64 oldSize=static_cast<i64>(oldPayload.size()),newSize=static_cast<i64>(newPayload.size());
    auto original=m.add("https://fixture.example/old","","result.bin","Main queue",true);atomicText(original->target(),oldPayload,false);const auto oldHash=fileHash(original->target());original->data["Status"]="Complete";original->data["Size"]=oldSize;original->data["Received"]=oldSize;original->data["Sha256"]=oldHash;original->data["Finished"]="original-finish";original->data["Description"]="original metadata";
    if(fault=="stale-catalog"){original->data["Size"]=100;original->data["Received"]=100;original->data["Sha256"]=std::string(64,'0');}
    auto job=m.add("https://fixture.example/new","","next.bin","Main queue",true);job->data["FileName"]="result.bin";job->data["ReplacementOf"]=original->id();job->data["ReplacementHash"]=oldHash;job->data["PreviousPath"]=utf8((files/L"previous.bin").wstring());job->data["Description"]="new metadata";if(privateJob)m.jobs.erase(std::find(m.jobs.begin(),m.jobs.end(),job));m.save();
    const auto target=job->target(),previous=files/L"previous.bin",stage=files/L"stage.part",journal=m.root/L"replacements"/(wide(job->id())+L".json");atomicText(stage,newPayload,false);const auto newHash=fileHash(stage);
    CatalogCheckpointTestAccess::observe(m,[&](const char* point){if(std::string_view(point)==(branch=="completed"?"replacement-file-moved":"replacement-intent-written"))throw std::runtime_error("Owned metadata setup interruption");});try{m.publishFile(job,stage,newHash);}catch(...){}CatalogCheckpointTestAccess::observe(m,{});check(fs::exists(journal),"Metadata recovery begins with a real preserved replacement intent");auto op=Json::parse(readText(journal));
    if(legacy){op.erase("CommitId");for(const char* key:{"Before","After","OriginalBefore","OriginalAfter"})op[key].erase("PublicationCommitId");op.erase("Ephemeral");}
    const auto healthy=op;
    if(branch=="completed"&&fault!="valid"&&fault!="save-failure"){
     const auto row=fault.find("new-")==0?"After":"OriginalAfter";const auto kind=fault.substr(4);const i64 good=row==std::string("After")?newSize:oldSize;
     if(kind=="count"){op[row]["Size"]=100;op[row]["Received"]=100;}
     else if(kind=="received")op[row]["Received"]=good+1;
     else if(kind=="fraction"){op[row]["Size"]=static_cast<double>(good)+0.5;op[row]["Received"]=op[row]["Size"];}
     else if(kind=="missing"){op[row].erase("Size");op[row].erase("Received");}
     else if(kind=="negative"){op[row]["Size"]=-1;op[row]["Received"]=-1;}
     else if(kind=="overflow"){op[row]["Size"]=std::numeric_limits<unsigned long long>::max();op[row]["Received"]=op[row]["Size"];}
     else if(kind=="hash")op[row]["Sha256"]=std::string(64,'0');
     else if(kind=="path")op[row]["FileName"]="wrong-history.bin";
    }else if(fault.find("before-")==0){
     auto& before=op["OriginalBefore"];
     if(fault=="before-count"){before["Size"]=100;before["Received"]=100;}
     else if(fault=="before-received")before["Received"]=100;
     else if(fault=="before-fraction"){before["Size"]=9.5;before["Received"]=9.5;}
     else if(fault=="before-missing"){before.erase("Size");before.erase("Received");}
     else if(fault=="before-negative"){before["Size"]=-1;before["Received"]=-1;}
     else if(fault=="before-hash")before["Sha256"]=std::string(64,'0');
     else if(fault=="before-path")before["FileName"]="wrong-before.bin";
    }
    atomicText(journal,op.dump(),false);if(branch=="restored")fs::rename(target,previous);
    const auto beforeJob=job->snapshot(),beforeOriginal=original->snapshot();bool reachedSave=false;
    if(fault=="save-failure")CatalogCheckpointTestAccess::observe(m,[&](const char* point){if(std::string_view(point)=="temporary-flushed"){reachedSave=true;throw std::runtime_error("Owned metadata catalog failure");}});
    try{m.recoverReplacements(job);}catch(const std::exception& e){workflowError=e.what();}CatalogCheckpointTestAccess::observe(m,{});
    const bool reject=(branch=="completed"&&fault!="valid")||fault=="save-failure"||(fault=="before-path"&&!legacy);
    check(reject?!workflowError.empty():workflowError.empty(),"Completed-pair disagreement is rejected; verified old-original accounting is repaired");
    if(reject){
     check(fs::exists(journal)&&job->snapshot()==beforeJob&&original->snapshot()==beforeOriginal,"Rejected or uncommitted recovery preserves its intent and both current catalog records");check(!m.isActive(job)&&!m.isActive(original),"A refused metadata outcome releases both runtime reservations");
     if(branch=="completed")check(fileHash(target)==newHash&&fileHash(previous)==oldHash,"Rejecting completed metadata preserves both exact file versions");
     else check(fileHash(fs::exists(target)?target:previous)==oldHash&&fileHash(stage)==newHash,"Refused original repair preserves old and unpublished new bytes");
     if(fault=="save-failure")check(reachedSave,"The persistence refusal occurs at the actual flushed recovery catalog boundary");
     atomicText(journal,healthy.dump(),false);m.recoverReplacements(job);
    }
    check(str(original->data,"Status")=="Complete"&&PublicationCatalogProof::completeByteCounts(original->data)&&num(original->data,"Size",-1)==oldSize&&str(original->data,"Sha256")==oldHash,"The completed original has exact measured integer size, received bytes and digest");
    check(original->target()==(branch=="completed"?previous:target)&&fs::file_size(original->target())==static_cast<uintmax_t>(oldSize)&&fileHash(original->target())==oldHash,"The original history names the exact verified old file");
    check(str(original->data,"Description")=="original metadata"&&str(original->data,"Finished")=="original-finish"&&str(job->data,"Description")=="new metadata","Byte-accounting repair preserves both descriptions and the original finish history");
    if(branch=="completed")check(str(job->data,"Status")=="Complete"&&PublicationCatalogProof::completeByteCounts(job->data)&&num(job->data,"Size",-1)==newSize&&str(job->data,"Sha256")==newHash&&fileHash(job->target())==newHash,"Completed replacement metadata exactly matches the new output file");
    else check(str(job->data,"Status")=="Paused"&&fileHash(stage)==newHash,"Original recovery leaves the unpublished replacement paused with its exact bytes");
    check(!fs::exists(journal)&&!m.isActive(job)&&!m.isActive(original),"Proved recovery retires the journal and releases both owners");
    const auto disk=Json::parse(readText(m.root/L"state.json"));const auto savedOriginal=metadataRow(disk,original->id()),savedJob=metadataRow(disk,job->id());check(savedOriginal&&PublicationCatalogProof::completeByteCounts(*savedOriginal)&&num(*savedOriginal,"Size",-1)==oldSize&&str(*savedOriginal,"Sha256")==oldHash&&str(*savedOriginal,"FileName")==str(original->data,"FileName"),"Before another save the actual catalog has correct completed original bytes and location");
    check(privateJob?!savedJob:savedJob&&str(*savedJob,"Status")==str(job->data,"Status"),"Private replacement recovery never inserts a visible row");
    if(branch=="completed"&&privateJob){const auto receipt=disk.at("PublicationReceipts").at(str(job->data,"PublicationCommitId"));check(PublicationCatalogProof::completeByteCounts(receipt)&&num(receipt,"Size",-1)==newSize&&str(receipt,"Sha256")==newHash,"The durable private completion receipt contains the exact measured new size");}
    cases.push_back({{"branch",branch},{"legacy",legacy},{"private",privateJob},{"empty",empty},{"fault",fault},{"initialRejected",reject},{"error",workflowError},{"oldBytes",oldSize},{"newBytes",newSize}});m.stop();
   }
  }
  // The same complete-counter invariant protects normal and recovery snapshots.
  {Manager m(root/L"proof");auto j=m.add("https://fixture.example/proof");auto row=j->data;row["Status"]="Complete";row["Size"]=0;row["Received"]=0;row["Sha256"]=std::string(64,'0');row["Finished"]="fixture";row["PublicationCommitId"]=guid();
   for(const std::string fault:{"valid-zero","valid-max","fraction","missing","negative","overflow","mismatch","boolean","string"}){auto current=row;if(fault=="valid-max"){current["Size"]=std::numeric_limits<i64>::max();current["Received"]=current["Size"];}if(fault=="fraction"){current["Size"]=0.5;current["Received"]=0.5;}if(fault=="missing")current.erase("Received");if(fault=="negative"){current["Size"]=-1;current["Received"]=-1;}if(fault=="overflow"){current["Size"]=std::numeric_limits<unsigned long long>::max();current["Received"]=current["Size"];}if(fault=="mismatch")current["Received"]=1;if(fault=="boolean"){current["Size"]=false;current["Received"]=false;}if(fault=="string"){current["Size"]="0";current["Received"]="0";}
    const bool valid=fault.find("valid-")==0;check(PublicationCatalogProof::completeByteCounts(current)==valid,"Complete counters have an exact nonnegative signed-range integer schema");bool normal=false,recovery=false;{Lock lock(m.mutex);try{auto lease=PublicationCatalogProof::prepare(m,current);normal=true;}catch(...){}try{auto lease=PublicationCatalogProof::prepareRecovery(m,{{j->data,current,true}});recovery=true;}catch(...){}}check(normal==valid&&recovery==valid,"Normal and recovery proof preparation reject the same malformed completed counters");
   }m.stop();}
 }catch(const std::exception& e){error=e.what();}
 atomicText(root/L"results.json",Json{{"passed",error.empty()},{"error",error},{"case",activeCase},{"workflowError",workflowError},{"checks",checks},{"cases",cases}}.dump(2),false);std::cout<<checks.size()<<" checks; "<<(error.empty()?"passed":error)<<"\n";return error.empty()?0:1;
}
