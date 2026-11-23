#include "PreparedTransferCommit.hpp"
#include <iostream>
namespace udm {struct CatalogCheckpointTestAccess {
 static void own(Manager& m,JobPtr j,std::shared_ptr<Cancel> c){Lock l(m.mutex);if(c)m.active[j->id()]=c;else m.active.erase(j->id());}
 static void observe(Manager& m,std::function<void(const char*)> f){Lock l(m.mutex);m.catalogCheckpoint=std::move(f);}
};}
using namespace udm;
struct HashGate {
 std::promise<void> releasePromise;std::shared_future<void> released=releasePromise.get_future().share();std::atomic_bool done=false;
 void release(){if(!done.exchange(true))releasePromise.set_value();}~HashGate(){release();}
};
int wmain(int argc,wchar_t**argv){
 if(argc!=2)return 2;const auto root=fs::absolute(argv[1]);if(fs::exists(root))return 2;fs::create_directories(root);Json checks=Json::array();std::string error,mode;
 const auto check=[&](bool ok,const char* label){checks.push_back({{"passed",ok},{"name",label}});if(!ok)throw std::runtime_error(label);};
 try{for(const auto value:{"success","stale-recorded-hash","hash-error","invalid-hash","cancel","intent-interruption","moved-interruption"}){
  mode=value;const auto folder=root/wide(mode),files=folder/L"files";fs::create_directories(files);Manager m(folder/L"catalog");auto prefs=m.state["Settings"];prefs["DownloadFolder"]=utf8(files.wstring());prefs["CategoryFolders"]=false;m.setSettings(prefs);
  auto j=m.add("https://example.invalid/advisory.bin","","output.bin","Main queue",true);j->data["Size"]=14;j->data["Received"]=14;j->data["Status"]="Downloading";if(mode=="stale-recorded-hash")j->data["Sha256"]=std::string(64,'0');m.save();auto c=std::make_shared<Cancel>();CatalogCheckpointTestAccess::own(m,j,c);
  const auto stage=files/(L".udm-"+wide(j->id())+L".assembling"),parts=folder/L"parts",journal=m.root/L"publications"/(wide(j->id())+L".json");fs::create_directories(parts);atomicText(parts/L"owned.data","verified bytes",false);if(!CreateHardLinkW(stage.c_str(),(parts/L"owned.data").c_str(),nullptr))throw std::runtime_error("Cannot link owned stage");const auto hash=fileHash(stage);
  auto gate=std::make_shared<HashGate>();std::promise<void> intent;auto ready=intent.get_future();std::atomic_bool seen=false,hashDone=false;std::atomic_int completedSnapshots=0;std::string failure;
  CatalogCheckpointTestAccess::observe(m,[&](const char* point){
   if(std::string_view(point)=="publication-intent-written"){if(!seen.exchange(true))intent.set_value();if(mode=="intent-interruption")throw std::runtime_error("Owned intent interruption");}
   if(std::string_view(point)=="publication-catalog-prepared")++completedSnapshots;
   if(mode=="moved-interruption"&&std::string_view(point)=="publication-file-moved")throw std::runtime_error("Owned moved interruption");
  });
  auto worker=std::async(std::launch::async,[&]{try{
   PreparedTransferCommit owner(m,j,c,true);if(!owner.startWriteback(parts/L"owned.data",false))throw std::runtime_error("Cannot start owned body flush");
   auto token=std::make_shared<Cancel>();token->parent=c;
   owner.adoptAdvisoryHash(std::async(std::launch::async,[gate,hash,token,&hashDone,mode]{
    if(gate->released.wait_for(std::chrono::seconds(10))!=std::future_status::ready)throw std::runtime_error("Owned hash deadline");
    hashDone=true;token->check();if(mode=="hash-error")throw std::runtime_error("Owned optional hash failure");return AdvisoryHashResult{mode=="invalid-hash"?"not-a-digest":hash,0.001};
   }),token);
   owner.prepare(stage,parts,"",false);owner.commit();
  }catch(const std::exception& e){failure=e.what();}});
  struct Release{std::shared_ptr<HashGate> gate;~Release(){gate->release();}} release{gate};
  check(ready.wait_for(std::chrono::seconds(3))==std::future_status::ready,"Identity-bound intent becomes durable while the value-owned advisory hash is pending");
  check(!hashDone&&!fs::exists(j->target())&&completedSnapshots==0,"Pending optional hash cannot rename the file or prepare an accepted completion snapshot");
  const auto operation=Json::parse(readText(journal));check(str(operation,"Hash").empty()&&str(operation.at("After"),"Sha256").empty()&&operation.contains("StagedIdentity")&&yes(operation,"FlushBeforePublish"),"Early intent keeps its empty digest, actual source identity and required body flush");
  check(worker.wait_for(std::chrono::milliseconds(30))!=std::future_status::ready,"Normal and exceptional owners retain the hash task until it joins");
  if(mode=="cancel")c->stop=true;gate->release();worker.get();CatalogCheckpointTestAccess::observe(m,{});CatalogCheckpointTestAccess::own(m,j,{});
  check(hashDone,"No hash task survives its publication owner");
  if(mode=="success"||mode=="stale-recorded-hash"||mode=="hash-error"||mode=="invalid-hash"){
   check(failure.empty()&&str(j->snapshot(),"Status")=="Complete"&&fileHash(j->target())==hash&&!fs::exists(journal),"Optional hash success or failure preserves exact durable completion");
   const auto saved=Json::parse(readText(m.root/L"state.json")).at("Downloads").at(0);
   check(str(saved,"Sha256")== ((mode=="success"||mode=="stale-recorded-hash")?hash:""),"Final accepted catalog contains only the joined valid advisory digest");
   check(completedSnapshots==1&&!fs::exists(parts/L"owned.data"),"One completion snapshot is accepted before owned parts are removed");
  }else{
   check(!failure.empty()&&str(j->snapshot(),"Status")!="Complete"&&fs::exists(parts/L"owned.data"),"Cancellation or interruption preserves parts and reports no accepted completion");
   check(fs::exists(journal),"Failed early-intent publication preserves its recovery journal");
   m.recoverReplacements();check(str(j->snapshot(),"Status")=="Complete"&&str(j->snapshot(),"Sha256")==hash&&fileHash(j->target())==hash&&!fs::exists(journal),"Recovery independently hashes the identity-bound file and accepts exact bytes");
  }
  m.stop();
 }
 for(const auto required:{false,true}){
  mode=required?"required-digest":"known-digest";const auto folder=root/wide(mode),files=folder/L"files",parts=folder/L"parts";fs::create_directories(files);fs::create_directories(parts);Manager m(folder/L"catalog");auto prefs=m.state["Settings"];prefs["DownloadFolder"]=utf8(files.wstring());prefs["CategoryFolders"]=false;m.setSettings(prefs);auto j=m.add("https://example.invalid/required.bin","","output.bin","Main queue",true);const auto stage=files/(L".udm-"+wide(j->id())+L".assembling");atomicText(parts/L"owned.data","verified bytes",false);if(!CreateHardLinkW(stage.c_str(),(parts/L"owned.data").c_str(),nullptr))throw std::runtime_error("Cannot link required stage");const auto hash=fileHash(stage);if(required)j->data["ExpectedSha256"]=hash;auto c=std::make_shared<Cancel>();bool rejected=false;
  {
   PreparedTransferCommit owner(m,j,c,true);check(owner.startWriteback(parts/L"owned.data",false),"Digest-policy fixture starts actual owned body writeback");std::promise<AdvisoryHashResult> promise;auto task=promise.get_future();promise.set_value({hash,0});
   try{owner.adoptAdvisoryHash(std::move(task),std::make_shared<Cancel>());owner.prepare(stage,parts,hash,false);}catch(const std::logic_error&){rejected=true;}
   check(rejected,"A required or already known publication digest cannot be replaced by a pending advisory result");
  }
  check(!fs::exists(j->target())&&str(j->snapshot(),"Status")!="Complete","Rejected advisory handoff cannot publish required-checksum output");check(fs::exists(parts/L"owned.data")&&!fs::exists(m.root/L"publications"/(wide(j->id())+L".json")),"Rejected advisory policy preserves bytes without creating an early intent");m.stop();
 }
 }catch(const std::exception& e){error=e.what();}
 const bool passed=error.empty();atomicText(root/L"results.json",Json{{"passed",passed},{"checks",checks},{"error",error},{"case",mode}}.dump(2),false);std::cout<<checks.size()<<" checks; "<<error<<"\n";return passed?0:1;
}
