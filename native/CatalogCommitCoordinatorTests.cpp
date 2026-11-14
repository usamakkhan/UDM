#include "Core.hpp"
#include "CatalogCommitCoordinator.hpp"
#include "PublicationCatalogProof.hpp"
#include <future>
#include <iostream>
using namespace udm;
int wmain(int argc,wchar_t** argv){
 if(argc!=2)return 2;const fs::path root=argv[1];if(fs::exists(root))return 3;fs::create_directories(root);
 Json checks=Json::array();std::string error;auto check=[&](bool ok,const char* name){checks.push_back({{"name",name},{"passed",ok}});if(!ok)throw std::runtime_error(name);};
 try{
  std::mutex gate;unsigned long long durable=0;CatalogCommitCoordinator coordinator(gate,durable);unsigned writes=0;const auto file=root/L"state.json";
  auto write=[&](int value){atomicText(file,Json{{"value",value}}.dump(),true);++writes;};
  auto first=coordinator.commit(1,[&]{write(1);});
  check(first.published()&&first.requestedSequence==1&&first.durableSequence==1,"First successful commit reports the durable sequence");
  auto second=coordinator.commit(2,[&]{write(2);});
  check(second.published()&&Json::parse(readText(file))["value"]==2,"Newer commit publishes its actual checkpoint bytes");
  auto repeated=coordinator.commit(2,[&]{throw std::runtime_error("Repeated writer must not run");});
  check(repeated.disposition==CatalogCommitDisposition::AlreadyPublished&&writes==2,"Repeating a durable sequence does not write again");
  auto older=coordinator.commit(1,[&]{throw std::runtime_error("Stale writer must not run");});
  check(older.disposition==CatalogCommitDisposition::Superseded&&older.durableSequence==2,"Older commit resolves as superseded without invoking its writer");
  bool failed=false;try{coordinator.commit(3,[&]{throw std::runtime_error("fixture write failure");});}catch(const std::exception& e){failed=std::string(e.what())=="fixture write failure";}
  check(failed&&durable==2&&Json::parse(readText(file))["value"]==2,"Failed write preserves the durable sequence and previous checkpoint");
  auto retry=coordinator.commit(3,[&]{write(3);});
  check(retry.published()&&durable==3&&Json::parse(readText(file.wstring()+L".bak"))["value"]==2,"A failed sequence can retry and retain the previous checkpoint backup");
  std::promise<void> entered,release;auto enteredFuture=entered.get_future();auto released=release.get_future().share();
  auto oldFuture=std::async(std::launch::async,[&]{return coordinator.commit(4,[&]{entered.set_value();if(released.wait_for(std::chrono::seconds(5))!=std::future_status::ready)throw std::runtime_error("Fixture release timed out");write(4);});});
  const bool enteredReady=enteredFuture.wait_for(std::chrono::seconds(5))==std::future_status::ready;
  if(!enteredReady){release.set_value();oldFuture.get();throw std::runtime_error("Older writer never entered");}
  auto newFuture=std::async(std::launch::async,[&]{return coordinator.commit(5,[&]{write(5);});});release.set_value();
  const auto oldResult=oldFuture.get(),newResult=newFuture.get();
  check(oldResult.published()&&newResult.published()&&durable==5,"Concurrent writers serialize and advance in the observed order");
  check(Json::parse(readText(file))["value"]==5&&Json::parse(readText(file.wstring()+L".bak"))["value"]==4,"Concurrent commit publication retains the newest file and immediate predecessor backup");
  coordinator.commit(7,[&]{write(7);});bool staleInvoked=false;
  auto superseded=coordinator.commit(6,[&]{staleInvoked=true;throw std::runtime_error("obsolete failure");});
  check(!staleInvoked&&superseded.disposition==CatalogCommitDisposition::Superseded&&durable==7,"A newer successful commit prevents an obsolete writer from reporting a later false failure");
  bool zeroRejected=false;try{coordinator.commit(0,[&]{write(0);});}catch(const std::invalid_argument&){zeroRejected=true;}
  check(zeroRejected&&durable==7,"Invalid zero sequence cannot publish or change durable state");
  std::mutex observedGate;unsigned long long observedDurable=0;CatalogCommitCoordinator observed(observedGate,observedDurable);
  unsigned acquired=0,published=0,unlocked=0;
  auto observe=[&](CatalogCommitStage stage)noexcept{if(stage==CatalogCommitStage::Acquired)++acquired;if(stage==CatalogCommitStage::Published)++published;if(stage==CatalogCommitStage::Released&&observedGate.try_lock()){++unlocked;observedGate.unlock();}};
  observed.commit(1,[]{},observe);
  check(acquired==1&&published==1&&unlocked==1,"Successful completion observation runs after the writer lock is released");
  try{observed.commit(2,[]{throw std::runtime_error("observed failure");},observe);}catch(const std::runtime_error&){}
  check(acquired==2&&published==1&&unlocked==2&&observedDurable==1,"Failure releases the writer lock without publishing a durable sequence");
  observed.commit(1,[]{throw std::runtime_error("duplicate observed writer");},observe);
  check(acquired==3&&published==1&&unlocked==3,"Already-published observation releases its lock and reports no new publication");
  {
   Manager manager(root/L"proof-state");auto row=Json{{"Id",guid()},{"Folder",utf8(root.wstring())},{"FileName","proof.bin"},{"Status","Complete"},{"Size",1},{"Received",1},{"Sha256",std::string(64,'a')},{"Finished","fixture-finish"},{"PublicationCommitId",guid()}};auto snapshot=Json{{"Downloads",Json::array({row})}};
   auto rejected=[&](auto&& action){try{action();return false;}catch(const std::exception&){return true;}};
   auto invalid=row;invalid["Status"]="Paused";check(rejected([&]{auto lease=PublicationCatalogProof::prepare(manager,invalid);}),"Incomplete model rows cannot reserve durable publication proof");
   {
    auto lease=PublicationCatalogProof::prepare(manager,row);check(rejected([&]{auto duplicate=PublicationCatalogProof::prepare(manager,row);}),"A second owner cannot replace a pending completion binding");
    for(const char* key:{"PublicationCommitId","Size","Received","Sha256","Finished","Folder","FileName"}){auto conflict=snapshot;conflict["Downloads"][0].erase(key);check(rejected([&]{auto write=PublicationCatalogProof::capture(manager,conflict);}),"Snapshots missing an exact pending completion field are rejected before capture");}
    auto missing=Json{{"Downloads",Json::array()}};check(rejected([&]{auto write=PublicationCatalogProof::capture(manager,missing);}),"An unrelated newer snapshot cannot silently omit a pending completion");
    check(!lease.resolve(),"No captured successful snapshot provides no durable publication acknowledgement");check(rejected([&]{auto write=PublicationCatalogProof::capture(manager,snapshot);}),"Resolution freezes late captures even after the failed writer count reaches zero");
   }
   {
    auto lease=PublicationCatalogProof::prepare(manager,row);auto write=PublicationCatalogProof::capture(manager,snapshot);std::future<bool> resolver;struct Release{PublicationCatalogProof::Publication& write;~Release(){write={};}} release{write};std::promise<void> entered;auto ready=entered.get_future();resolver=std::async(std::launch::async,[&]{entered.set_value();return lease.resolve();});ready.wait();const bool waiting=resolver.wait_for(std::chrono::milliseconds(30))==std::future_status::timeout;write.published(17);check(resolver.wait_for(std::chrono::seconds(3))==std::future_status::ready&&resolver.get()&&waiting,"Resolution waits for a captured writer and accepts only its explicit successful snapshot acknowledgement");lease.release();auto removed=PublicationCatalogProof::capture(manager,Json{{"Downloads",Json::array()}});check(!removed,"A retired accepted owner does not block legitimate later catalog removal");
   }
   {
    auto lease=PublicationCatalogProof::prepare(manager,row);{auto write=PublicationCatalogProof::capture(manager,snapshot);}check(!lease.resolve(),"Retiring a failed or superseded capture without acknowledgement cannot prove durability");
   }
   {
    auto lease=PublicationCatalogProof::prepare(manager,row);auto duplicate=snapshot;duplicate["Downloads"].push_back(row);check(rejected([&]{auto write=PublicationCatalogProof::capture(manager,duplicate);}),"Duplicate completion identities cannot provide ambiguous snapshot proof");auto nested=Json{{"Downloads",Json::array({Json{{"Id",guid()},{"Video",row}}})}};auto write=PublicationCatalogProof::capture(manager,nested);write.published(19);check(lease.resolve(),"An exact internal child row supplies proof through its containing parent snapshot");
   }
   {
    auto lease=PublicationCatalogProof::prepare(manager,row);Manager independent(root/L"independent-proof");independent.save();check(!PublicationCatalogProof::capture(independent,independent.snapshot()),"A different Manager's catalog does not inherit another Manager's pending proof");check(!lease.resolve(),"Another Manager's successful write cannot acknowledge this publication");
   }
   {
    const auto descriptor=PublicationCatalogProof::binding(row);auto lease=PublicationCatalogProof::prepare(manager,row,true);check(PublicationCatalogProof::installReceipt(manager,descriptor),"A standalone internal file installs a private receipt without adding a visible download");check(!PublicationCatalogProof::installReceipt(manager,descriptor),"Repeating the same private receipt is idempotent");auto conflict=descriptor;conflict["Size"]=2;check(rejected([&]{PublicationCatalogProof::installReceipt(manager,conflict);}),"A conflicting private receipt cannot replace the original operation binding");check(rejected([&]{auto write=PublicationCatalogProof::capture(manager,snapshot);}),"A visible row cannot substitute for the required private receipt");auto receiptSnapshot=manager.snapshot();check(receiptSnapshot["Downloads"].empty(),"Private publication receipt does not create a user download row");auto write=PublicationCatalogProof::capture(manager,receiptSnapshot);write.published(23);check(lease.resolve(),"The exact persisted private receipt can acknowledge standalone file completion");lease.release();check(PublicationCatalogProof::eraseReceipt(manager,descriptor)&&!manager.state.contains("PublicationReceipts"),"Only the matching operation can retire private receipt metadata");
   }
   {
    auto related=row;related["Id"]=guid();related["FileName"]="previous.bin";auto pair=Json{{"Downloads",Json::array({row,related})}};auto lease=PublicationCatalogProof::prepare(manager,row,false,&related);
    check(rejected([&]{auto write=PublicationCatalogProof::capture(manager,snapshot);}),"A primary completion alone cannot acknowledge a paired replacement");auto missing=pair;missing["Downloads"].erase(0);check(rejected([&]{auto write=PublicationCatalogProof::capture(manager,missing);}),"The original record alone cannot acknowledge the new file");auto conflict=pair;conflict["Downloads"][1]["Sha256"]=std::string(64,'b');check(rejected([&]{auto write=PublicationCatalogProof::capture(manager,conflict);}),"Changed original projection rejects paired proof before capture");auto independent=related;independent["PublicationCommitId"]=guid();check(rejected([&]{auto other=PublicationCatalogProof::prepare(manager,independent);}),"A paired owner also reserves its companion record against independent proof");auto write=PublicationCatalogProof::capture(manager,pair);write.published(29);check(lease.resolve(),"Only one exact containing snapshot acknowledges both replacement rows");
   }
   {
    auto related=row;related["Id"]=guid();related["FileName"]="previous-private.bin";const auto descriptor=PublicationCatalogProof::binding(row);PublicationCatalogProof::installReceipt(manager,descriptor);auto lease=PublicationCatalogProof::prepare(manager,row,true,&related);auto pair=manager.snapshot();check(rejected([&]{auto write=PublicationCatalogProof::capture(manager,pair);}),"A private new-file receipt still requires its visible original companion");pair["Downloads"].push_back(related);auto write=PublicationCatalogProof::capture(manager,pair);write.published(31);check(lease.resolve(),"A standalone replacement pairs its private receipt with the exact visible original row");lease.release();PublicationCatalogProof::eraseReceipt(manager,descriptor);
   }
   manager.stop();
  }
 }catch(const std::exception& e){error=e.what();}
 atomicText(root/L"results.json",Json{{"passed",error.empty()},{"error",error},{"checks",checks}}.dump(2),false);
 std::cout<<checks.size()<<" checks; "<<(error.empty()?"passed":error)<<"\n";return error.empty()?0:1;
}
