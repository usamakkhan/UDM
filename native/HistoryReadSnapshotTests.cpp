#include "HistoryReadSnapshot.hpp"
#include <future>
#include <iostream>
using namespace udm;
int wmain(int argc,wchar_t** argv){
 if(argc!=2)return 2;const auto root=fs::absolute(argv[1]);if(fs::exists(root))return 2;fs::create_directories(root);Json checks=Json::array();std::string error;
 auto check=[&](bool ok,const char* name){checks.push_back({{"name",name},{"passed",ok}});if(!ok)throw std::runtime_error(name);};
 try{
  Manager manager(root/L"catalog");auto first=std::make_shared<Job>(Json{{"Id",guid()},{"FileName","first.bin"},{"Category","Video"},{"Status","Paused"},{"Queue","Main queue"},{"Url","https://example.invalid/match"},{"ProtectedAdaptive","not a list field"},{"Segments",Json::array({Json{{"Index",0},{"Start",0},{"End",1023},{"Done",0}}})},{"Size",1024}});
  auto second=std::make_shared<Job>(Json{{"Id",guid()},{"FileName","second.bin"},{"Category","Music"},{"Status","Complete"},{"Queue","Main queue"},{"Url","https://example.invalid/other"}});
  {Lock lock(manager.mutex);manager.jobs={first,second};manager.state["Projects"]=Json::array({Json{{"Id","owned-project"},{"Name","Owned project"}}});manager.storageError="owned warning";first->httpFailureNoticePending=true;}
  std::map<JobPtr,std::shared_ptr<const DownloadListRow>> rows;DownloadFilterCache cache;HistoryReadRequest request;request.selected={first};request.focused=first;
  auto all=tryHistoryReadSnapshot(manager,request,rows,cache);check(all&&all->rows==std::vector<JobPtr>{first,second},"One successful snapshot captures catalog order for All Downloads");
  check(all->selection==std::set<std::string>{first->id()}&&all->focusedId==first->id(),"Selection and focus identities belong to the same model read");
  check(all->storageWarning=="owned warning"&&all->preferences==manager.state["Settings"],"Settings and storage warnings are captured with rows");
  check(all->observations.size()==2&&all->observations[0].httpFailure&&all->observations[1].state=="Complete","Hidden and visible status observers receive immutable values");
  check(!all->records.at(first)->data.contains("Segments")&&!all->records.at(first)->data.contains("ProtectedAdaptive"),"History snapshots exclude transfer journals and private media plans");
  check(all->searchRecords.empty()&&all->searchMatches.empty(),"An inactive search does not copy address-search metadata");
  rows=all->records;request.previousTree=&all->navigation;auto unchanged=tryHistoryReadSnapshot(manager,request,rows,cache);
  check(unchanged->records.at(first)==rows.at(first)&&unchanged->records.at(second)==rows.at(second),"Unchanged compact row records retain shared ownership across ticks");
  const auto before=rows.at(first);{Lock lock(manager.mutex);first->data["FileName"]="renamed.bin";manager.state["Queues"][0]["Enabled"]=false;}
  auto changed=tryHistoryReadSnapshot(manager,request,rows,cache);check(str(before->data,"FileName")=="first.bin"&&str(changed->records.at(first)->data,"FileName")=="renamed.bin","Later mutations cannot change an earlier captured row");
  check(changed->navigation.queueDescriptions.at("Main queue").find("; stopped")!=std::string::npos&&all->navigation.queueDescriptions.at("Main queue").find("; stopped")==std::string::npos,"Queue descriptions and rows are replaced by a coherent later read");
  const std::vector<std::pair<std::string,size_t>> filters={{"all",2},{"unfinished",1},{"finished",1},{"category:Video",1},{"unfinished:Video",1},{"finished:Video",0},{"queue:Main queue",1},{"grabber",0},{"project:owned-project",0}};
  for(const auto& item:filters){request.filter=item.first;auto filtered=tryHistoryReadSnapshot(manager,request,rows,cache);check(filtered&&filtered->filter==item.first&&filtered->rows.size()==item.second,"Existing navigation filters retain their history cohort");}
  auto removedTree=all->navigation;removedTree.categories.push_back("Removed");request.previousTree=&removedTree;request.filter="unfinished:Removed";request.selectedAncestors={"unfinished:Removed","unfinished"};auto fallback=tryHistoryReadSnapshot(manager,request,rows,cache);
  check(fallback->filter=="unfinished"&&fallback->rows==std::vector<JobPtr>{first},"A removed category resolves to its surviving ancestor before rows are filtered");
  request.selectedAncestors={"project:removed","grabber"};request.filter="project:removed";fallback=tryHistoryReadSnapshot(manager,request,rows,cache);check(fallback->filter=="grabber"&&fallback->rows.empty(),"A removed project resolves to the Grabber parent");
  request.selectedAncestors={"queue:removed","root:queues"};request.filter="queue:removed";fallback=tryHistoryReadSnapshot(manager,request,rows,cache);check(fallback->filter=="all"&&fallback->rows.size()==2,"A removed queue resolves to the existing queues-root All Downloads view");
  request.selectedAncestors.clear();request.previousTree=&all->navigation;request.filter="missing";fallback=tryHistoryReadSnapshot(manager,request,rows,cache);check(fallback->filter=="all"&&fallback->rows.size()==2,"An invalid programmatic filter resolves before history capture");
  request.filter="all";request.searchActive=true;request.searchQuery=request.filterQuery=Json{{"Text","match"},{"FileName",false},{"Address",true}};
  auto searching=tryHistoryReadSnapshot(manager,request,rows,cache);check(searching->searchRecords.size()==2&&searching->searchMatches.empty(),"Uncached address matching receives owned selected search fields");
  DownloadSearch matcher(request.filterQuery);for(auto& entry:searching->searchRecords)cache.matches(entry.first,std::move(entry.second),matcher,&request.searchQuery);cache.finish();
  auto hits=tryHistoryReadSnapshot(manager,request,rows,cache);check(hits->searchRecords.empty()&&hits->searchMatches.at(first)&&!hits->searchMatches.at(second),"Both positive and negative search decisions reuse unchanged encrypted sources");
  request.visible=false;const auto retained=cache.entryCount();auto hidden=tryHistoryReadSnapshot(manager,request,rows,cache);check(hidden&&hidden->rows.empty()&&hidden->records.empty()&&hidden->observations.size()==2,"Hidden-window observation avoids building navigation and row snapshots");
  check(cache.entryCount()==retained,"Hidden-window observation does not invalidate the visible search cohort");
  request.visible=true;std::promise<void> entered,release;auto ready=entered.get_future();auto released=release.get_future().share();std::atomic_bool holding=false,expired=false;
  std::thread writer([&]{Lock lock(manager.mutex);first->data["FileName"]="held.bin";holding=true;entered.set_value();if(released.wait_for(std::chrono::seconds(3))!=std::future_status::ready)expired=true;holding=false;});
  struct Join{std::promise<void>& release;std::thread& thread;~Join(){try{release.set_value();}catch(...){}if(thread.joinable())thread.join();}} join{release,writer};
  check(ready.wait_for(std::chrono::seconds(3))==std::future_status::ready,"A separate writer owns the catalog before the busy read");
  const auto cost=cache.retainedCost();const auto count=cache.entryCount();request.searchQuery={{"Text","changed query"}};
  check(!tryHistoryReadSnapshot(manager,request,rows,cache)&&holding&&!expired,"A full history read returns busy before the writer releases the model");
  check(cache.retainedCost()==cost&&cache.entryCount()==count&&cache.isFor(Json{{"Text","match"},{"FileName",false},{"Address",true}}),"A busy history read changes neither retained rows nor the search query/cache");
  check(!tryBrowserPresentationSnapshot(manager)&&holding&&!expired,"Browser-offer observation also returns busy without waiting");
  release.set_value();writer.join();request.searchActive=false;auto resumed=tryHistoryReadSnapshot(manager,request,rows,cache);check(resumed&&str(resumed->records.at(first)->data,"FileName")=="held.bin","The next successful history read catches up with the writer");
  auto offers=tryBrowserPresentationSnapshot(manager);check(offers&&offers->empty(),"An available empty offer snapshot is distinct from a busy model");
  manager.stop();
 }catch(const std::exception& e){error=e.what();}
 atomicText(root/L"results.json",Json{{"passed",error.empty()},{"checks",checks},{"error",error}}.dump(2),false);std::cout<<checks.size()<<" checks; "<<(error.empty()?"passed":error)<<"\n";return error.empty()?0:1;
}
