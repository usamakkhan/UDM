#include "PreparedTransferCommit.hpp"
#include <iostream>
namespace udm {struct CatalogCheckpointTestAccess {
 static void own(Manager& m,JobPtr j,std::shared_ptr<Cancel> c){Lock lock(m.mutex);if(c)m.active[j->id()]=c;else m.active.erase(j->id());}
 static void observe(Manager& m,std::function<void(const char*)> f){Lock lock(m.mutex);m.catalogCheckpoint=std::move(f);}
 static Handle pin(PreparedTransferCommit& p,const fs::path& path){p.writebackPin=PublicationSourcePin::open(path);if(!p.writebackPin)throw std::runtime_error("Owned snapshot failed");return p.writebackPin->duplicate();}
 static void body(PreparedTransferCommit& p,std::future<PublishedWritebackResult> task){p.writeback=std::move(task);}
 static PublishedWritebackResult flush(Handle body,const fs::path& path){return PreparedTransferCommit::flushPinned(std::move(body),path,0);}
};}
using namespace udm;
struct FailureGate {std::promise<void> entered,released;std::shared_future<void> ready=entered.get_future().share(),wait=released.get_future().share();std::atomic_bool seen=false,open=false;void signal(){if(!seen.exchange(true))entered.set_value();}void release(){if(!open.exchange(true))released.set_value();}~FailureGate(){release();}};
int wmain(int argc,wchar_t** argv){if(argc!=2)return 2;const auto root=fs::absolute(argv[1]);if(fs::exists(root))return 2;fs::create_directories(root);Json checks=Json::array(),observations=Json::array();std::string failure;
 const auto check=[&](bool ok,const char* message){checks.push_back({{"passed",ok},{"name",message}});if(!ok)throw std::runtime_error(message);};
 try{for(const std::string point:{"destination-before-commit","cancel-before-commit","publication-preflight-begin","publication-preflight-ready","publication-intent-temporary-flushed","publication-intent-written"}){
  auto folder=root/wide(point),files=folder/L"files",parts=folder/L"parts";fs::create_directories(files);fs::create_directories(parts);Manager manager(folder/L"catalog");auto prefs=manager.state["Settings"];prefs["DownloadFolder"]=utf8(files.wstring());prefs["CategoryFolders"]=false;manager.setSettings(prefs);auto job=manager.add("https://example.invalid/failure.bin","","output.bin","Main queue",true);job->data["Size"]=14;job->data["Received"]=14;job->data["Status"]="Downloading";manager.save();const auto stage=files/(L".udm-"+wide(job->id())+L".assembling"),target=job->target();atomicText(stage,"verified bytes",false);atomicText(parts/L"owned.data","verified bytes",false);const auto digest=fileHash(stage);auto token=std::make_shared<Cancel>();CatalogCheckpointTestAccess::own(manager,job,token);auto gate=std::make_shared<FailureGate>();std::promise<void> caught;auto errorReady=caught.get_future();std::atomic_bool bodyDone=false,escapedBeforeBody=false;std::string error;
  CatalogCheckpointTestAccess::observe(manager,[&](const char* phase){if(point==phase){gate->signal();throw std::runtime_error("Owned publication failure");}});
  auto work=std::async(std::launch::async,[&]{
   // Mirrors the real worker: the prepared owner outlives its inner catch.
   PreparedTransferCommit owner(manager,job,token,true);
   auto body=CatalogCheckpointTestAccess::pin(owner,stage);CatalogCheckpointTestAccess::body(owner,std::async(std::launch::async,[&,body=std::move(body)]()mutable{if(gate->wait.wait_for(std::chrono::seconds(10))!=std::future_status::ready)throw std::runtime_error("Owned failure body deadline");auto result=CatalogCheckpointTestAccess::flush(std::move(body),stage);bodyDone=true;return result;}));
   owner.prepare(stage,parts,digest,false);
   if(point=="destination-before-commit"){job->data["FileName"]="different.bin";gate->signal();}
   if(point=="cancel-before-commit"){token->stop=true;gate->signal();}
   try{owner.commit();}catch(const std::exception& e){escapedBeforeBody=!bodyDone;error=e.what();caught.set_value();owner.joinWriteback();}
  });
  struct Release{std::shared_ptr<FailureGate> gate;~Release(){gate->release();}} release{gate};
  const bool reached=gate->ready.wait_for(std::chrono::seconds(5))==std::future_status::ready;if(!reached){gate->release();work.get();throw std::runtime_error("Owned publication failure point was not reached");}
  const bool escaped=errorReady.wait_for(std::chrono::milliseconds(200))==std::future_status::ready;gate->release();work.get();CatalogCheckpointTestAccess::observe(manager,{});CatalogCheckpointTestAccess::own(manager,job,{});observations.push_back({{"point",point},{"errorReachedCallerWhileBodyHeld",escaped},{"escapedBeforeBody",escapedBeforeBody.load()},{"bodyJoined",bodyDone.load()}});
  check(bodyDone&&!error.empty(),"Failure returns its original error after the owned body has retired");check(!escaped&&!escapedBeforeBody,"Publication exceptions cannot escape to the caller while body IO is pending");check(str(job->snapshot(),"Status")!="Complete"&&!fs::exists(target)&&fs::exists(parts/L"owned.data"),"Failed publication preserves parts and never accepts a final file");manager.stop();
 }}catch(const std::exception& e){failure=e.what();}
 atomicText(root/L"results.json",Json{{"passed",failure.empty()},{"error",failure},{"checks",checks},{"observations",observations}}.dump(2),false);std::cout<<checks.size()<<" checks; "<<(failure.empty()?"passed":failure)<<"\n";return failure.empty()?0:1;
}
