// Diagnostic control for qualified 8.53; not a production entry point.
#include "Core.hpp"
#include "PublicationCatalogProof.hpp"
#include <future>
#include <iostream>
namespace udm {struct CatalogCheckpointTestAccess {
 static void observe(Manager& m,std::function<void(const char*)> fn){Lock lock(m.mutex);m.catalogCheckpoint=std::move(fn);}
};}
using namespace udm;
int wmain(int argc,wchar_t** argv){
 if(argc!=2)return 2;
 const auto root=fs::absolute(argv[1]);if(fs::exists(root))return 2;fs::create_directories(root);
 Json cases=Json::array();std::string error;
 try{
  for(const unsigned count:{1u,16u}){
   const auto folder=root/std::to_wstring(count);Manager m(folder/L"catalog");
   const auto files=folder/L"files";fs::create_directories(files);
   auto prefs=m.state["Settings"];prefs["DownloadFolder"]=utf8(files.wstring());prefs["CategoryFolders"]=false;m.setSettings(prefs);
   std::vector<std::pair<fs::path,std::string>> outputs;
   {
    Lock lock(m.mutex);
    for(unsigned i=0;i<count;++i){
     auto job=m.add("https://fixture.example/private","","output-"+std::to_string(i)+".bin","Main queue",true);
     atomicText(job->target(),"verified bytes",false);const auto digest=fileHash(job->target());
     job->data["Status"]="Complete";job->data["Size"]=14;job->data["Received"]=14;job->data["Sha256"]=digest;
     job->data["Finished"]=date();job->data["PublicationCommitId"]=guid();
     PublicationCatalogProof::installReceipt(m,PublicationCatalogProof::binding(job->data));
     outputs.push_back({job->target(),digest});m.jobs.erase(std::find(m.jobs.begin(),m.jobs.end(),job));
    }
   }
   m.save();const auto before=Json::parse(readText(m.root/L"state.json"));
   if(!before["Downloads"].empty()||before["PublicationReceipts"].size()!=count)throw std::runtime_error("Private receipt control setup failed");
   std::promise<void> entered,released;auto ready=entered.get_future();auto wait=released.get_future().share();
   std::atomic_bool first=false,expired=false;
   CatalogCheckpointTestAccess::observe(m,[&](const char* phase){
    if(std::string_view(phase)=="temporary-flushed"&&!first.exchange(true)){
     entered.set_value();if(wait.wait_for(std::chrono::seconds(10))!=std::future_status::ready){expired=true;throw std::runtime_error("Owned pruning hold expired");}
    }
   });
   auto work=std::async(std::launch::async,[&]{m.recoverReplacements();});
   struct Release {std::promise<void>& value;~Release(){try{value.set_value();}catch(...){}}} release{released};
   if(ready.wait_for(std::chrono::seconds(5))!=std::future_status::ready)throw std::runtime_error("Pruning did not reach its actual catalog flush");
   bool available=false;{std::unique_lock<std::recursive_mutex> snapshot(m.mutex,std::try_to_lock);available=snapshot.owns_lock();}
   released.set_value();work.get();CatalogCheckpointTestAccess::observe(m,{});
   const auto saved=Json::parse(readText(m.root/L"state.json"));
   bool exact=true;for(const auto& output:outputs)exact=exact&&fileHash(output.first)==output.second;
   const bool retired=!saved.contains("PublicationReceipts")&&saved["Downloads"].empty();
   cases.push_back({{"receipts",count},{"actualCatalogFlushHeld",true},{"modelAvailable",available},
                    {"holdExpired",expired.load()},{"receiptPruningDurable",retired},{"allCompletedBytesPreserved",exact}});
   if(available||expired||!retired||!exact)throw std::runtime_error("Unexpected 8.53 pruning control outcome");
   m.stop();
  }
 }catch(const std::exception& failure){error=failure.what();}
 atomicText(root/L"results.json",Json{{"passed",error.empty()},{"error",error},{"cases",cases}}.dump(2),false);
 std::cout<<cases.dump()<<"\n";return error.empty()?0:1;
}
