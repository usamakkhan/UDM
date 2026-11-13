#include "CatalogAdmissionRegistry.hpp"
#include <future>
#include <iostream>
using namespace udm;
int wmain(int argc,wchar_t** argv){
 if(argc!=2)return 2;const fs::path root=fs::absolute(argv[1]);if(fs::exists(root))return 3;fs::create_directories(root);Json checks=Json::array();std::string error;
 auto check=[&](bool ok,const char* name){checks.push_back({{"name",name},{"passed",ok}});if(!ok)throw std::runtime_error(name);};
 auto rejects=[&](const std::function<void()>& action,const char* name){bool failed=false;try{action();}catch(const std::exception&){failed=true;}check(failed,name);};
 auto catalog=[](){return Json{{"Schema",1},{"Downloads",Json::array()},{"Settings",Json::object()}};};
 auto reservation=[&](const char* token,const char* id,const std::string& name,const char* binding){return std::make_shared<const CatalogAdmissionReservation>(token,Json{{"Id",id},{"Folder",utf8((root/L"files").wstring())},{"FileName",name},{"Status","Awaiting confirmation"}},Json{{"protocol",3},{"status","accepted"},{"id",id},{"fingerprint",binding}});};
 using Registry=CatalogAdmissionRegistry;
 try{
  Registry registry;auto a=registry.acquire("a","binding-a");
  check(registry.lookup("a").phase==Registry::Phase::Preparing,"Token ownership is visible before a job is prepared");
  rejects([&]{registry.acquire("a","binding-a");},"A second owner cannot reuse an in-flight token");
  rejects([&]{registry.acquire("a","changed");},"Changing the binding cannot take an in-flight token");
  rejects([&]{a.prepare(reservation("other","job-a","file.mp4","binding-a"));},"A prepared reservation must match its owner token");
  rejects([&]{a.prepare(reservation("a","job-a","file.mp4","wrong-binding"));},"A prepared reservation must match its owner binding");
  auto payload=reservation("a","job-a",u8"Video-Ä.mp4","binding-a");a.prepare(payload);a.prepare(payload);
  check(registry.lookup("a").phase==Registry::Phase::Prepared,"Preparing the same immutable object is idempotent");
  auto b=registry.acquire("b","binding-b");
  rejects([&]{b.prepare(reservation("b","job-b",u8"video-ä.MP4","binding-b"));},"Ordinal Unicode case-insensitive destination collisions are rejected");
  rejects([&]{b.prepare(reservation("b","job-a","different.mp4","binding-b"));},"A second operation cannot mutate an already reserved job identity");
  b.prepare(reservation("b","job-b","different.mp4","binding-b"));auto snapshot=catalog();auto publication=registry.overlay(snapshot);
  check(snapshot["Downloads"].size()==2&&snapshot["BrowserCaptures"].size()==2,"A persistence ticket captures every prepared reservation in its snapshot");
  check(!a.committed()&&!b.committed(),"Building a snapshot does not acknowledge acceptance");
  publication.published(0);check(!a.committed(),"A zero sequence cannot prove publication");
  publication.published(4);check(a.committed()&&b.committed()&&registry.lookup("a").durableSequence==4,"Successful publication acknowledges only the included reservations");
  publication.published(3);check(registry.lookup("a").durableSequence==4,"A late older publication cannot downgrade a durable proof");
  auto handoff=catalog();auto duringHandoff=registry.overlay(handoff);
  check(handoff["Downloads"].size()==2,"Committed reservations remain in snapshots until live-state installation");
  a.installed();b.installed();auto live=catalog();auto afterInstall=registry.overlay(live);
  check(live["Downloads"].empty(),"Installed jobs stop being overlaid and must come from the live catalog");
  a.release();check(registry.lookup("a").phase==Registry::Phase::Committed,"Outstanding snapshot tickets keep token ownership after owner release");
  publication={};duringHandoff={};check(registry.lookup("a").phase==Registry::Phase::Missing,"Installed token ownership retires after its final snapshot completes");
  b.release();check(registry.lookup("b").phase==Registry::Phase::Missing,"Independent installed ownership retires without affecting other tokens");
  Registry future;auto beforeSnapshot=catalog();auto before=future.overlay(beforeSnapshot);auto pending=future.acquire("pending","p");pending.prepare(reservation("pending","job-p","pending.mp4","p"));before.published(8);
  check(!pending.committed(),"An earlier snapshot that omits a token cannot prove its later admission");
  auto newerSnapshot=catalog();auto newer=future.overlay(newerSnapshot);auto olderSnapshot=catalog();auto older=future.overlay(olderSnapshot);
  newer.published(10);older={};check(pending.committed()&&future.lookup("pending").durableSequence==10,"A containing newer success survives failure of another captured write");
  pending.installed();pending.release();newer={};before={};check(future.lookup("pending").phase==Registry::Phase::Missing,"Completing the accepted handoff releases all transient ownership");
  Registry abandoned;auto owner=abandoned.acquire("abandoned","q");owner.prepare(reservation("abandoned","job-q","q.mp4","q"));auto abandonedSnapshot=catalog();auto inFlight=abandoned.overlay(abandonedSnapshot);owner.release();
  rejects([&]{abandoned.acquire("abandoned","q");},"An abandoned owner cannot release a token while its snapshot is in flight");
  inFlight={};check(abandoned.lookup("abandoned").phase==Registry::Phase::Missing,"A failed terminal snapshot releases abandoned uncommitted ownership");
  Registry retained;auto retainedOwner=retained.acquire("retained","r");retainedOwner.prepare(reservation("retained","job-r","r.mp4","r"));auto retainedSnapshot=catalog();auto retainedWrite=retained.overlay(retainedSnapshot);retainedWrite.published(12);retainedOwner.release();retainedWrite={};
  auto retainedNext=catalog();auto retainedNextWrite=retained.overlay(retainedNext);check(retainedNext["Downloads"].size()==1&&retained.lookup("retained").phase==Registry::Phase::Committed,"Abandoned accepted data remains overlaid until an explicit live-state handoff");
  Registry concurrent;std::promise<void> acquired;auto ready=acquired.get_future();
  auto first=std::async(std::launch::async,[&]{auto lease=concurrent.acquire("same","s");acquired.set_value();return lease;});
  check(ready.wait_for(std::chrono::seconds(5))==std::future_status::ready,"Concurrent owner acquisition completes");
  auto second=std::async(std::launch::async,[&]{try{concurrent.acquire("same","s");return false;}catch(const std::exception&){return true;}});
  check(second.get(),"Concurrent acquisition admits exactly one owner for a token");auto winner=first.get();winner.release();
  check(concurrent.lookup("same").phase==Registry::Phase::Missing,"Moving a lease across threads preserves correct retirement");
  Registry::Lease survivor;{Registry transient;survivor=transient.acquire("survivor","t");survivor.prepare(reservation("survivor","job-t","t.mp4","t"));auto view=catalog();auto write=transient.overlay(view);write.published(1);}
  check(survivor.committed(),"An outstanding lease safely retains state after its registry facade is destroyed");survivor.installed();survivor.release();
  for(bool succeeds:{false,true}){
   Registry resolving;auto waiting=resolving.acquire("wait","w");waiting.prepare(reservation("wait","job-w","w.mp4","w"));auto view=catalog();auto write=resolving.overlay(view);
   auto result=std::async(std::launch::async,[&]{return waiting.resolve();});
   const auto deadline=GetTickCount64()+1000;while(resolving.lookup("wait").phase!=Registry::Phase::Resolving&&GetTickCount64()<deadline)Sleep(1);
   const bool entered=resolving.lookup("wait").phase==Registry::Phase::Resolving;
   auto later=catalog();bool frozen=false;try{auto blocked=resolving.overlay(later);}catch(const std::exception&){frozen=true;}
   const bool pending=result.wait_for(std::chrono::milliseconds(20))==std::future_status::timeout;
   if(succeeds)write.published(6);else write={};
   const bool wakes=result.wait_for(std::chrono::seconds(1))==std::future_status::ready;
   if(!wakes)write={};const bool accepted=result.get();
   check(entered&&frozen,"Failure resolution blocks newer snapshots while a captured outcome is unknown");
   check(pending,"Resolution waits while a captured publication is still in flight");
   check(wakes&&accepted==succeeds,"Resolution wakes on a containing success or terminal failure");
   if(succeeds)waiting.installed();waiting.release();write={};
   check(resolving.lookup("wait").phase==Registry::Phase::Missing,"Resolved ownership retires after publication and handoff");
  }
 }catch(const std::exception& e){error=e.what();}
 atomicText(root/L"results.json",Json{{"passed",error.empty()},{"error",error},{"checks",checks}}.dump(2),false);
 std::cout<<checks.size()<<" checks; "<<(error.empty()?"passed":error)<<"\n";return error.empty()?0:1;
}
