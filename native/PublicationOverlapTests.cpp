#include "PreparedTransferCommit.hpp"
#include "PublicationPreparation.hpp"
#include <iostream>
namespace udm {struct CatalogCheckpointTestAccess {
 static void observe(Manager& m,std::function<void(const char*)> callback){Lock l(m.mutex);m.catalogCheckpoint=std::move(callback);}
 static void own(Manager& m,JobPtr j,std::shared_ptr<Cancel> c){Lock l(m.mutex);if(c)m.active[j->id()]=c;else m.active.erase(j->id());}
 static Handle pin(PreparedTransferCommit& p,const fs::path& path){p.writebackPin=PublicationSourcePin::open(path);if(!p.writebackPin)throw std::runtime_error("Owned source snapshot failed");auto body=p.writebackPin->duplicate();if(!body)throw std::runtime_error("Owned source duplication failed");return body;}
 static PublishedWritebackResult flush(Handle h,const fs::path& path){return PreparedTransferCommit::flushPinned(std::move(h),path,0);}
 static void body(PreparedTransferCommit& p,std::future<PublishedWritebackResult> task){p.writeback=std::move(task);}
};}
using namespace udm;
struct BodyGate {std::promise<void> released;std::shared_future<void> wait=released.get_future().share();std::atomic_bool done=false;void release(){if(!done.exchange(true))released.set_value();}~BodyGate(){release();}};
int wmain(int argc,wchar_t** argv){
 if(argc!=2)return 2;const auto root=fs::absolute(argv[1]);if(fs::exists(root))return 2;fs::create_directories(root);Json checks=Json::array();std::string failure,mode;
 const auto check=[&](bool ok,const char* label){checks.push_back({{"passed",ok},{"name",label}});if(!ok)throw std::runtime_error(label);};
 try{
 for(const auto value:{"success","flush-error","throwing-flush","writer-blocked","changed-name","writer-rejected","cancel","intent-error","intent-interruption","moved-interruption","changed-folder","unknown-hash","unwind-before-prepare"}){
  mode=value;const auto folder=root/wide(mode),files=folder/L"files";fs::create_directories(files);Manager m(folder/L"catalog");auto prefs=m.state["Settings"];prefs["DownloadFolder"]=utf8(files.wstring());prefs["CategoryFolders"]=false;m.setSettings(prefs);auto j=m.add("https://example.invalid/owned.bin","","output.bin","Main queue",true);j->data["Size"]=14;j->data["Received"]=14;j->data["Status"]="Downloading";m.save();auto c=std::make_shared<Cancel>();CatalogCheckpointTestAccess::own(m,j,c);
  const auto stage=files/(L".udm-"+wide(j->id())+L".assembling"),parts=folder/L"parts",target=j->target(),journal=m.root/L"publications"/(wide(j->id())+L".json");fs::create_directories(parts);atomicText(stage,"verified bytes",false);atomicText(parts/L"owned.data","verified bytes",false);const auto hash=fileHash(stage);auto gate=std::make_shared<BodyGate>();std::promise<void> intent;auto ready=intent.get_future();std::atomic_bool seen=false,bodyDone=false;std::string error;
  CatalogCheckpointTestAccess::observe(m,[&](const char* point){
   if(std::string_view(point)=="publication-intent-written"){
    if(!seen.exchange(true))intent.set_value();
    if(mode=="intent-interruption")throw std::runtime_error("Owned early-intent interruption");
   }
   if(mode=="intent-error"&&std::string_view(point)=="publication-intent-temporary-flushed")throw std::runtime_error("Owned intent publication failure");
   if(mode=="moved-interruption"&&std::string_view(point)=="publication-file-moved")throw std::runtime_error("Owned moved interruption");
  });
  auto worker=std::async(std::launch::async,[&]{try{
   PreparedTransferCommit p(m,j,c,true);
   auto body=CatalogCheckpointTestAccess::pin(p,stage);
   CatalogCheckpointTestAccess::body(p,std::async(std::launch::async,[&,body=std::move(body)]()mutable{if(gate->wait.wait_for(std::chrono::seconds(10))!=std::future_status::ready)throw std::runtime_error("Owned body deadline");if(mode=="throwing-flush"){bodyDone=true;throw std::runtime_error("Owned flush exception");}auto r=CatalogCheckpointTestAccess::flush(std::move(body),stage);if(mode=="flush-error"){r.confirmed=false;r.flushError=ERROR_WRITE_FAULT;}bodyDone=true;return r;}));
   if(mode=="unwind-before-prepare")throw std::runtime_error("Owned preparation exception");
   p.prepare(stage,parts,mode=="unknown-hash"?"":hash,false);p.commit();
  }catch(const std::exception& e){error=e.what();}});
  struct Release {std::shared_ptr<BodyGate> gate;~Release(){gate->release();}} release{gate};
  std::unique_ptr<Handle> writer;
  if(mode=="unknown-hash"||mode=="unwind-before-prepare"){
   check(worker.wait_for(std::chrono::milliseconds(40))!=std::future_status::ready&&!fs::exists(target),"Fallback and unwind retain owned body work before returning");check(!fs::exists(journal),"Unknown digest never writes an early intent");
  }else{
   const bool reached=ready.wait_for(std::chrono::seconds(3))==std::future_status::ready;
   if(mode!="intent-error")check(reached,"A durable recovery intent can precede completed body writeback");
   check(!bodyDone&&!fs::exists(target)&&str(j->snapshot(),"Status")!="Complete","A pending body cannot rename output or report completion");
   check(worker.wait_for(std::chrono::milliseconds(30))!=std::future_status::ready,"Every normal and exceptional owner retains unfinished body work");
   if(fs::exists(journal)){auto op=Json::parse(readText(journal));check(yes(op,"FlushBeforePublish")&&str(op,"Hash")==hash&&op.contains("StagedIdentity")&&op["StagedIdentity"].is_object(),"Early recovery record binds body writeback, exact digest and the held source identity");}
   if(mode=="writer-blocked"){Handle forbidden(CreateFileW(stage.c_str(),GENERIC_WRITE,FILE_SHARE_READ|FILE_SHARE_WRITE|FILE_SHARE_DELETE,nullptr,OPEN_EXISTING,0,nullptr));check(!forbidden,"An owned writeback pin excludes mutations throughout the early intent interval");}
   if(mode=="changed-name"){check(MoveFileExW(stage.c_str(),(files/L"retained.part").c_str(),0)!=FALSE,"Snapshot allows a controlled source-name replacement");atomicText(stage,"verified bytes",false);}
   if(mode=="writer-rejected"){writer=std::make_unique<Handle>(CreateFileW(stage.c_str(),GENERIC_WRITE,FILE_SHARE_READ|FILE_SHARE_WRITE|FILE_SHARE_DELETE,nullptr,OPEN_EXISTING,0,nullptr));check(!*writer,"A second write handle stays excluded until durable acceptance");}
   if(mode=="cancel")c->stop=true;
   if(mode=="changed-folder"){Lock lock(m.mutex);j->data["Folder"]=utf8((files/L"changed").wstring());}
  }
  gate->release();worker.get();writer.reset();CatalogCheckpointTestAccess::observe(m,{});CatalogCheckpointTestAccess::own(m,j,{});check(bodyDone,"All body IO has retired before the owner returns");
  if(mode=="success"||mode=="unknown-hash"||mode=="writer-blocked"||mode=="writer-rejected"){
   check(error.empty()&&str(j->snapshot(),"Status")=="Complete"&&fileHash(target)==hash&&!fs::exists(journal),"Confirmed unchanged output is durably published and cleaned");
   check(!fs::exists(parts/L"owned.data"),"Parts are removed only after successful catalog acceptance");
  }else{
   check(!error.empty()&&str(j->snapshot(),"Status")!="Complete","A failed or changed publication never reports completion");
   check(fs::exists(parts/L"owned.data"),"Failed overlap preserves downloaded parts");
   if(mode!="moved-interruption")check(!fs::exists(target),"No final name appears before body and identity proof");
  }
  if(mode=="intent-interruption"||mode=="moved-interruption"){
   check(fs::exists(journal),"Interrupted owned transaction preserves its recovery intent");m.recoverReplacements(j);check(str(j->snapshot(),"Status")=="Complete"&&fileHash(target)==hash&&!fs::exists(journal),"Recovery forces body writeback and accepts only the complete digest-bound output");
  }
  if(mode=="changed-name"){
   bool rejected=false;try{m.recoverReplacements(j);}catch(...){rejected=true;}
   check(rejected&&fs::exists(journal)&&!fs::exists(target)&&readText(stage)=="verified bytes"&&readText(files/L"retained.part")=="verified bytes","Recovery rejects a foreign same-hash source and preserves both objects");
  }
  m.stop();
 }
 // Direct adoption must retain write exclusion and pin the final name.
 mode="adopted-pin";auto file=root/L"adopted.part";atomicText(file,"verified bytes",false);auto owned=PublicationSourcePin::open(file);check(bool(owned),"Source capture seals identity on an owned write-only object");PublicationPreparation p(file,Json::object(),std::move(owned));bool rejected=false;try{p.sourceHash();}catch(...){rejected=true;}check(rejected,"A write-only adopted object cannot supply an unknown digest");auto target=root/L"adopted.bin";check(MoveFileExW(file.c_str(),target.c_str(),0)!=FALSE,"Adopted write protection permits the owner rename");p.pinPublishedName(target);Handle writer(CreateFileW(target.c_str(),GENERIC_WRITE,FILE_SHARE_READ|FILE_SHARE_WRITE|FILE_SHARE_DELETE,nullptr,OPEN_EXISTING,0,nullptr));check(!writer,"Adopted source still excludes writers while the published read name shares owned write access");check(!MoveFileExW(target.c_str(),(root/L"must-not-move.bin").c_str(),0),"Published read pin prevents name deletion until acceptance");p.releaseAcceptedFile();Handle reader(CreateFileW(target.c_str(),GENERIC_READ,0,nullptr,OPEN_EXISTING,0,nullptr));check(bool(reader),"Exact accepted release admits exclusive readers without a retained write pin");
 }catch(const std::exception& e){failure=e.what();}
 atomicText(root/L"results.json",Json{{"passed",failure.empty()},{"error",failure},{"case",mode},{"checks",checks}}.dump(2),false);std::cout<<checks.size()<<" checks; "<<(failure.empty()?"passed":failure)<<"\n";return failure.empty()?0:1;
}
