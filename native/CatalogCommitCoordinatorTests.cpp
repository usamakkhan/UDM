#include "Core.hpp"
#include "CatalogCommitCoordinator.hpp"
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
 }catch(const std::exception& e){error=e.what();}
 atomicText(root/L"results.json",Json{{"passed",error.empty()},{"error",error},{"checks",checks}}.dump(2),false);
 std::cout<<checks.size()<<" checks; "<<(error.empty()?"passed":error)<<"\n";return error.empty()?0:1;
}
