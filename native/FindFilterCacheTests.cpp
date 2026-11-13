#include "GuiModels.hpp"
#include <iostream>
using namespace udm;
int main(int argc,char** argv){
 Json checks=Json::array(),timing;std::string error,stage="initial projection checks";
 const auto check=[&](bool value,const char* name){checks.push_back({{"name",name},{"passed",value}});if(!value)throw std::runtime_error(name);};
 try{
  const Json query={{"Text","cache-marker"},{"FileName",false},{"Address",true}};
  DownloadSearch matcher(query);DownloadFilterCache cache;cache.begin(query);
  auto job=std::make_shared<Job>(Json{{"Id",guid()},{"FileName","owned.bin"},{"Url","https://example.invalid/cache-marker"}});
  job->data["ProtectedAdaptive"]=std::string(6*1024*1024,'p');
  const auto source=job->data;auto snapshot=matcher.snapshot(source);
  check(!cache.cached(job,source,matcher).has_value(),"An unseen download requires an owned search snapshot");
  check(cache.matches(job,std::move(snapshot),matcher),"An address match seeds the positive cache outside the job lock");
  auto hit=cache.cached(job,source,matcher);
  check(hit.has_value()&&*hit,"An unchanged large encrypted source returns its cached positive result");
  job->data["Received"]=123;job->data["FileName"]="renamed.bin";
  check(cache.cached(job,job->data,matcher)==hit,"Progress and an unchecked filename do not invalidate address-only matching");
  job->data["ProtectedAdaptive"].get_ref<std::string&>().back()='q';
  check(!cache.cached(job,job->data,matcher).has_value(),"Same-length changes at the end of a protected plan invalidate matching");
  job->data=source;job->data["Url"]="https://example.invalid/changed";
  check(!cache.cached(job,job->data,matcher).has_value(),"Changing a selected URL invalidates the cached match");
  job->data=source;
  for(const auto* field:{"DownloadPage","ProtectedHeaders","ProtectedResolvedUrl","ProtectedSabr","SourceUrl","MediaOutput"}){
   job->data[field]="changed";
   check(!cache.cached(job,job->data,matcher).has_value(),"Every selected address dependency invalidates the cached match");
   job->data=source;
  }
  Json tracks=source;tracks["Video"]={{"Url","https://example.invalid/video"},{"Segments",Json::array({1,2,3})}};tracks["Audio"]=Json::object();
  auto trackSnapshot=matcher.snapshot(tracks);
  check(trackSnapshot["Video"]==Json{{"Url","https://example.invalid/video"}}&&trackSnapshot["Audio"]==Json::object(),"Search projection owns only selected track addresses and preserves empty tracks");
  tracks["Video"]["Segments"].push_back(4);
  check(matcher.snapshotUnchanged(tracks,trackSnapshot),"Transport range updates do not invalidate track-address matching");
  tracks["Audio"]["Url"]=nullptr;
  check(!matcher.snapshotUnchanged(tracks,trackSnapshot),"Missing and explicit-null track URLs remain distinct");
  trackSnapshot=matcher.snapshot(tracks);
  check(matcher.snapshotUnchanged(tracks,trackSnapshot),"An explicit-null track address matches its own snapshot");
  tracks["Audio"]=7;
  check(!matcher.snapshotUnchanged(tracks,trackSnapshot),"Removing a track object invalidates the projected shape");
  auto extra=matcher.snapshot(source);extra["Unexpected"]=true;
  check(!matcher.snapshotUnchanged(source,extra)&&!matcher.snapshotUnchanged(source,Json()),"Extra snapshot fields and invalid cache shapes cannot suppress matching");
  DownloadSearch filenames(Json{{"Text","owned"},{"FileName",true}});
  const auto nameSnapshot=filenames.snapshot(source);
  check(nameSnapshot==Json{{"FileName","owned.bin"}},"Unchecked address payloads never enter filename-only snapshots");
  auto changed=source;changed["ProtectedAdaptive"]="different";
  check(filenames.snapshotUnchanged(changed,nameSnapshot),"An unchecked protected plan cannot invalidate filename-only matching");
  changed["FileName"]="other.bin";
  check(!filenames.snapshotUnchanged(changed,nameSnapshot),"Changing the selected filename invalidates filename-only matching");
  auto negative=std::make_shared<Job>(Json{{"Id",guid()},{"FileName","negative.bin"},{"Url","https://example.invalid/other"}});
  check(!cache.matches(negative,matcher.snapshot(negative->data),matcher),"A nonmatching download seeds a negative cache entry");
  const auto no=cache.cached(negative,negative->data,matcher);
  check(no.has_value()&&!*no,"A cached false result remains an engaged cache hit");
  cache.finish();cache.begin(query);cache.cached(job,job->data,matcher);cache.finish();
  check(!cache.cached(negative,negative->data,matcher).has_value(),"Unseen downloads are evicted at the end of a refresh");
  check(cache.cached(job,job->data,matcher).has_value(),"A cache hit marks the download seen for the current refresh");
  auto nextQuery=query;nextQuery["MatchCase"]=true;cache.begin(nextQuery);
  check(!cache.cached(job,job->data,DownloadSearch(nextQuery)).has_value(),"Changing query options clears prior match results");
  cache.begin(query);cache.matches(job,matcher.snapshot(job->data),matcher);
  auto other=std::make_shared<Job>(job->data);other->data["Id"]=guid();
  check(!cache.cached(other,other->data,matcher).has_value(),"Identical metadata on another job cannot inherit its cache entry");
  auto invalid=std::make_shared<Job>(negative->data);invalid->data["Id"]=guid();invalid->data["ProtectedAdaptive"]="not-dpapi";
  bool failed=false;try{cache.matches(invalid,matcher.snapshot(invalid->data),matcher);}catch(const std::exception&){failed=true;}
  check(failed&&!cache.cached(invalid,invalid->data,matcher).has_value(),"Failed decryption cannot become a cached nonmatch");
  check(job->data==source,"Cache use leaves source transport and protected data unchanged");

  // Earlier projection fixtures are no longer needed. Release their large
  // strings before exercising independent retention cases on low-commit hosts.
  tracks=Json();trackSnapshot=Json();extra=Json();changed=Json();other.reset();
  {
   stage="cohort retention";DownloadFilterCache kept;kept.begin(query);stage="cohort positive seed";kept.matches(job,matcher.snapshot(source),matcher);stage="cohort negative seed";kept.matches(negative,matcher.snapshot(negative->data),matcher);stage="cohort finish";kept.finish();
   kept.begin(query);kept.keep(job);kept.keep(negative);kept.finish();
   check(kept.isFor(query)&&kept.cached(job,source,matcher)==std::optional<bool>(true),"Inactive-filter cohort retention preserves a query-bound positive match");
   check(kept.cached(negative,negative->data,matcher)==std::optional<bool>(false),"Inactive-filter cohort retention preserves a negative match");
   kept.begin(query);kept.keep(negative);kept.finish();
   check(!kept.cached(job,source,matcher).has_value()&&kept.entryCount()==1,"A removed or out-of-category job is evicted without re-matching retained candidates");
   kept.begin(nextQuery);check(!kept.isFor(query)&&kept.isFor(nextQuery)&&kept.entryCount()==0&&kept.retainedCost()==0,"Query changes clear retained entries, ordering and budget accounting");
  }
  const auto plain=[&](const char* suffix){return std::make_shared<Job>(Json{{"Id",guid()},{"FileName","small.bin"},{"Url",std::string("https://example.invalid/cache-marker/")+suffix}});};
  auto first=plain("one"),second=plain("two"),third=plain("three");
  {
   stage="entry LRU budget";DownloadFilterCache bounded(4096,2);bounded.begin(query);
   bounded.matches(first,matcher.snapshot(first->data),matcher);bounded.matches(second,matcher.snapshot(second->data),matcher);
   bounded.cached(first,first->data,matcher);bounded.matches(third,matcher.snapshot(third->data),matcher);
   check(bounded.entryCount()==2&&bounded.retainedCost()<=4096,"Entry and logical-byte budgets bound cache retention");
   check(bounded.cached(first,first->data,matcher).has_value()&&!bounded.cached(second,second->data,matcher).has_value()&&bounded.cached(third,third->data,matcher).has_value(),"Entry pressure evicts the least recently used match");
   bounded.begin(query);bounded.keep(first);bounded.finish();
   check(bounded.entryCount()==1&&bounded.retainedCost()>0,"Cohort eviction updates entry and logical-cost accounting");
   bounded.begin(nextQuery);check(bounded.retainedCost()==0&&bounded.entryCount()==0,"Clearing a populated bounded cache resets logical cost");
  }
  {
   DownloadFilterCache bounded(4096,2);bounded.begin(query);
   bounded.matches(first,matcher.snapshot(first->data),matcher);bounded.matches(second,matcher.snapshot(second->data),matcher);
   bounded.keep(first);bounded.matches(third,matcher.snapshot(third->data),matcher);
   check(!bounded.cached(first,first->data,matcher).has_value()&&bounded.cached(second,second->data,matcher).has_value(),"Marking view membership does not promote unused entries in LRU order");
  }
  {
   stage="byte budget";DownloadFilterCache bounded(512,16);bounded.begin(query);
   auto left=plain("left"),right=plain("right");left->data["ProtectedAdaptive"]=std::string(240,'x');right->data["ProtectedAdaptive"]=std::string(240,'y');
   bounded.matches(left,matcher.snapshot(left->data),matcher);bounded.matches(right,matcher.snapshot(right->data),matcher);
   check(bounded.retainedCost()<=512&&bounded.entryCount()==1&&!bounded.cached(left,left->data,matcher).has_value()&&bounded.cached(right,right->data,matcher).has_value(),"Logical-byte pressure evicts old payloads before exceeding the budget");
   right->data["ProtectedAdaptive"]=std::string(4096,'z');
   check(bounded.matches(right,matcher.snapshot(right->data),matcher)&&bounded.entryCount()==0&&bounded.retainedCost()==0,"Oversized snapshots still match but replace no retained cache entry");
  }
  {
   stage="disabled retention";DownloadFilterCache disabled(0,0);disabled.begin(query);
   check(disabled.matches(first,matcher.snapshot(first->data),matcher)&&!disabled.cached(first,first->data,matcher).has_value()&&disabled.retainedCost()==0,"Disabling retention preserves the computed matching result");
  }
  {
   stage="expected-query binding";DownloadFilterCache bound;auto different=query;different["Text"]="no-such-marker";DownloadSearch otherMatcher(different);bound.begin(different);
   check(!bound.matches(first,otherMatcher.snapshot(first->data),otherMatcher,&different),"Another query can retain its own negative result");
   check(bound.matches(first,matcher.snapshot(first->data),matcher,&query),"A mismatched expected query computes the requested result rather than reusing another query's negative");
   check(bound.isFor(different)&&bound.cached(first,first->data,otherMatcher)==std::optional<bool>(false),"A stale-query match cannot overwrite the bound query's existing result");
   check(bound.entryCount()==1&&bound.retainedCost()>0,"Refused stale-query publication leaves retention accounting intact");
  }
  stage="unchanged snapshot benchmark";const auto baseline=matcher.snapshot(job->data);std::vector<double> cloneSamples,borrowSamples;size_t sink=0;
  const auto measure=[&](bool clone){const auto start=std::chrono::steady_clock::now();for(int i=0;i<16;++i){if(clone){auto copy=matcher.snapshot(job->data);sink+=copy==baseline;}else{auto cached=cache.cached(job,job->data,matcher);sink+=cached.has_value()&&*cached;}}return std::chrono::duration<double,std::milli>(std::chrono::steady_clock::now()-start).count()/16;};
  for(int trial=0;trial<7;++trial){if(trial%2){borrowSamples.push_back(measure(false));cloneSamples.push_back(measure(true));}else{cloneSamples.push_back(measure(true));borrowSamples.push_back(measure(false));}}
  auto a=cloneSamples,b=borrowSamples;std::sort(a.begin(),a.end());std::sort(b.begin(),b.end());
  check(sink==224,"Both refresh algorithms observe the positive match in every benchmark iteration");
  timing={{"scope","Synthetic unchanged six-MiB address-filter snapshot; not whole-list, whole-app or IDM timing"},{"iterationsPerTrial",16},{"cloneMsPerCheck",cloneSamples},{"borrowedMsPerCheck",borrowSamples},{"cloneMedianMs",a[3]},{"borrowedMedianMs",b[3]},{"snapshotBytes",baseline.dump().size()}};
 }catch(const std::exception& e){error=e.what();}
 if(argc>1){const auto root=fs::path(wide(argv[1]));fs::create_directories(root);atomicText(root/L"results.json",Json{{"passed",error.empty()},{"error",error},{"stage",stage},{"checks",checks},{"timing",timing}}.dump(2),false);}
 std::cout<<checks.size()<<" checks; "<<(error.empty()?"passed":error)<<"\n";return error.empty()?0:1;
}
