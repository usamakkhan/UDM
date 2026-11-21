#include "BrowserPresentationAcks.hpp"
#include <future>
#include <iostream>
namespace udm {struct CatalogCheckpointTestAccess {static void observe(Manager& m,std::function<void(const char*)> f){Lock lock(m.mutex);m.catalogCheckpoint=std::move(f);}};}
using namespace udm;
template<class F>static bool until(F f){const auto end=GetTickCount64()+6000;while(!f()&&GetTickCount64()<end)Sleep(1);return f();}
struct Gate {std::promise<void> release;std::shared_future<void> future=release.get_future().share();~Gate(){open();}void open(){try{release.set_value();}catch(...) {}}};
int wmain(int argc,wchar_t** argv){
 if(argc!=2)return 2;const auto root=fs::absolute(argv[1]);if(fs::exists(root))return 2;fs::create_directories(root);Json checks=Json::array();std::string error;
 const auto check=[&](bool ok,const char* name){checks.push_back({{"name",name},{"passed",ok}});if(!ok)throw std::runtime_error(name);};
 try{
  Manager manager(root/L"catalog");auto job=manager.add("http://127.0.0.1:1/file.bin",utf8((root/L"files").native()),"file.bin","Main queue",true);
  const auto offer=[](const std::string& token){return Json{{"tokens",Json::array({token})}};};
  const auto receipt=[&](const std::string& token){manager.state["BrowserCaptures"][token]={{"status","accepted"},{"created",epoch()},{"id",job->id()},{"presentation",{{"id",job->id()},{"kind","download"}}}};};
  receipt("owned-first");receipt("owned-later");manager.save();
  {
   Gate gate;std::atomic_bool held=false,entered=false;std::atomic_int calls=0,notices=0;std::thread writer([&]{Lock lock(manager.mutex);held=true;gate.future.wait();held=false;});
   struct Join {Gate& gate;std::thread& worker;~Join(){gate.open();if(worker.joinable())worker.join();}} join{gate,writer};
   check(until([&]{return held.load();}),"A real catalog writer holds Manager before acknowledgement is queued");
   BrowserPresentationAcks queue([&](const Json& value){++calls;entered=true;manager.finishBrowserPresentation(value);},[&]{++notices;});
   struct OpenBeforeQueueJoin{Gate& gate;~OpenBeforeQueueJoin(){gate.open();}} unblock{gate};
   check(queue.enqueue(offer("owned-first"))&&until([&]{return entered.load();})&&held,"Enqueue returns while the owned acknowledgement waits for Manager");
   check(queue.pending()&&queue.owns(offer("owned-first"))&&!queue.owns(offer("owned-later")),"Only captured tokens are claimed during the actual save wait");
   check(!queue.owns(Json{{"tokens",Json::array({"owned-first","owned-later"})}}),"A fresh token keeps a mixed presentation eligible while an older token is owned");
   check(queue.enqueue(offer("owned-first"))&&calls==1,"A displayed receipt cannot schedule a duplicate acknowledgement");
   gate.open();writer.join();check(until([&]{return !queue.pending();})&&until([&]{return notices.load()==1;}),"A saved acknowledgement releases its claim and posts a result-only notification");
   {Lock lock(manager.mutex);check(!manager.state["BrowserCaptures"]["owned-first"].contains("presentation")&&manager.state["BrowserCaptures"]["owned-later"].contains("presentation"),"The real Manager saves exactly the captured token and preserves a later offer");}
   check(!queue.owns(offer("owned-first"))&&!queue.failure(),"Successful saving leaves no stale ownership or error");
  }
  {
   receipt("owned-failure");manager.save();CatalogCheckpointTestAccess::observe(manager,[](const char* stage){if(std::string(stage)=="temporary-flushed")throw std::runtime_error("Owned acknowledgement save failure");});
   struct Clear {Manager& m;~Clear(){CatalogCheckpointTestAccess::observe(m,{});}} clear{manager};
   BrowserPresentationAcks queue([&](const Json& value){manager.finishBrowserPresentation(value);});queue.enqueue(offer("owned-failure"));
   check(until([&]{return bool(queue.failure());}),"A real durable-save failure is retained for UI recovery");const auto failure=queue.failure();
   {Lock lock(manager.mutex);check(manager.state["BrowserCaptures"]["owned-failure"].contains("presentation")&&std::find(manager.jobs.begin(),manager.jobs.end(),job)!=manager.jobs.end(),"Failed acknowledgement restores the offer and preserves the accepted download");}
   check(queue.owns(offer("owned-failure"))&&!queue.pending(),"A failed save retains ownership without starting an automatic retry loop");
   CatalogCheckpointTestAccess::observe(manager,{});check(queue.retry(failure),"Explicit Retry schedules the same failed receipt");
   check(until([&]{return !queue.pending();})&&!queue.failure()&&!queue.owns(offer("owned-failure")),"Successful Retry clears only the saved acknowledgement failure");
   check(!queue.retry(failure)&&!queue.dismiss(failure),"A consumed failure cannot replay Retry or dismiss another outcome");
  }
  {
   std::atomic_bool fail=true;BrowserPresentationAcks queue([&](const Json&){if(fail)throw std::runtime_error("Owned repeated failure");});queue.enqueue(offer("owned-repeat"));
   check(until([&]{return bool(queue.failure());}),"An independent failed receipt exposes a recovery identity");auto first=queue.failure();check(queue.retry(first)&&until([&]{const auto next=queue.failure();return next&&next.attempt>first.attempt;}),"A repeated failed Retry advances its exact recovery attempt");auto second=queue.failure();
   check(!queue.retry(first)&&!queue.dismiss(first),"Stale recovery actions cannot consume a newer failed attempt");
   check(queue.dismiss(second)&&!queue.failure()&&queue.owns(offer("owned-repeat"))&&!queue.pending(),"Close suppresses a repeated dialog this session without claiming a durable acknowledgement");
   queue.stop();check(!queue.enqueue(offer("owned-after-stop")),"A stopped owner rejects new acknowledgement work");fail=false;queue.restart();check(queue.enqueue(Json{{"tokens",Json::array({"owned-repeat","owned-after-restart"})}})&&until([&]{return !queue.pending();})&&queue.owns(offer("owned-repeat"))&&!queue.owns(offer("owned-after-restart")),"Restart acknowledges the fresh token in a mixed group without erasing a dismissed failure");
  }
  {
   Gate gate;std::atomic_int calls=0;BrowserPresentationAcks queue([&](const Json&){++calls;gate.future.wait();});
   struct OpenBeforeQueueJoin{Gate& gate;~OpenBeforeQueueJoin(){gate.open();}} unblock{gate};
   bool accepted=true;for(int i=0;i<64;++i)accepted=queue.enqueue(offer("owned-bounded-"+std::to_string(i)))&&accepted;
   check(accepted&&until([&]{return calls.load()==1;}),"One worker serializes all 64 bounded accepted acknowledgements");
   check(!queue.enqueue(offer("owned-overflow"))&&!queue.owns(offer("owned-overflow")),"Capacity rejection never claims an unqueued receipt");
   auto stopped=std::async(std::launch::async,[&]{queue.stop();});const auto waitState=stopped.wait_for(std::chrono::milliseconds(30));gate.open();check(waitState==std::future_status::timeout,"Shutdown waits for already accepted acknowledgement work instead of dropping it");
   check(stopped.wait_for(std::chrono::seconds(6))==std::future_status::ready&&calls==64&&!queue.pending(),"Shutdown drains every accepted acknowledgement before returning");stopped.get();
  }
  {
   BrowserPresentationAcks queue([](const Json&){},[]{throw std::runtime_error("Owned notification failure");});
   for(const auto& invalid:{Json::object(),offer(""),Json{{"tokens",Json::array({12})}},offer(std::string(257,'x'))}){bool rejected=false;try{queue.enqueue(invalid);}catch(const std::invalid_argument&){rejected=true;}check(rejected&&!queue.pending(),"Malformed offers cannot create background work or ownership");}
   check(queue.enqueue(offer("owned-no-notice"))&&until([&]{return !queue.pending();})&&!queue.failure(),"A failed UI wake-up cannot turn a successful save into a failed acknowledgement");
  }
  manager.stop();
 }catch(const std::exception& e){error=e.what();}
 atomicText(root/L"results.json",Json{{"passed",error.empty()},{"checks",checks},{"error",error}}.dump(2),false);std::cout<<checks.size()<<" checks; "<<(error.empty()?"passed":error)<<"\n";return error.empty()?0:1;
}
