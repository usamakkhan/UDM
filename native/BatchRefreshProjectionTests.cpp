#include "BatchInitialTask.hpp"
#include <iostream>
using namespace udm;
template<class F>static bool until(F f){const auto end=GetTickCount64()+6000;while(!f()&&GetTickCount64()<end)Sleep(1);return f();}
int wmain(int argc,wchar_t** argv){
 if(argc!=2)return 2;const auto root=fs::absolute(argv[1]);if(fs::exists(root))return 2;fs::create_directories(root);Json checks=Json::array();std::string error;const auto check=[&](bool value,const char* name){checks.push_back({{"name",name},{"passed",value}});if(!value)throw std::runtime_error(name);};
 try{
  Manager manager(root/L"catalog");manager.save();const auto before=manager.snapshot(),settings=before["Settings"];const auto disk=readText(manager.root/L"state.json");
  std::vector<std::string> urls{"http://127.0.0.1:1/first.bin","http://127.0.0.1:1/other.bin"};std::vector<BrowserBatchRow> browser;
  for(const auto& url:urls){BrowserBatchRow row;row.address=url;row.observedSize=4096;row.observedType="application/octet-stream";browser.push_back(row);}
  auto rows=projectBatchRows(urls,&browser,{},settings).rows;rows[0].name="edited.bin";rows[0].folder="owned-folder";rows[0].description="owned-description";rows[0].checked=true;rows[0].selectionTouched=true;
  auto captured=captureBatchRefresh(rows,{},std::nullopt,settings);rows[0].address="http://changed.example.test/after-capture";
  auto full=projectBatchRefresh(*captured,&browser,{},settings);
  check(full.values.size()==2&&full.requests.size()==2,"A full refresh preserves the complete source index space");
  check(full.values[0].row.address==urls[0]&&full.values[0].row.browserName=="edited.bin","Refresh owns edited request inputs independently of later caller mutation");
  check(num(full.values[0].row.metadata,"Size")==4096&&full.requests[0].address.empty(),"Unchanged observed request metadata still avoids a redundant probe");
  check(rows[0].folder=="owned-folder"&&rows[0].description=="owned-description"&&rows[0].checked&&rows[0].selectionTouched,"Projection never changes caller destination description or explicit selection");
  rows[0].address=urls[0];auto one=captureBatchRefresh(rows,{},1,settings);auto single=projectBatchRefresh(*one,&browser,{},settings);
  check(one->rows.size()==1&&single.values.size()==1&&single.values[0].index==1&&single.requests.size()==1,"A single-row refresh prepares only that source identity");
  check(one->preferences==settings,"A row refresh owns the existing preview policy cohort");
  auto skipped=projectBatchRefresh(*captureBatchRefresh(rows,{0},std::nullopt,settings),&browser,{},settings);
  check(skipped.values.size()==1&&skipped.values[0].index==1&&skipped.requests[0].address.empty(),"Previously added rows cannot gain a new metadata request");
  rows[0].protectedRequestOverrides="owned-protected-blob";auto protectedInput=captureBatchRefresh(rows,{},0,settings);auto overridden=projectBatchRefresh(*protectedInput,&browser,{},settings);
  check(overridden.values[0].row.metadata.empty()&&overridden.requests[0].address==urls[0]&&overridden.requests[0].protectedOverrides=="owned-protected-blob","Per-file protected overrides invalidate observed hints and travel with the request");
  rows[0].protectedRequestOverrides.clear();browser[0].headers={{"Cookie","owned-cookie"}};browser[0].proxy={{"type","direct"}};browser[0].session={{"fixture","owned-session"}};
  Headers headers{{"Authorization","Basic owned-auth"}};auto routed=projectBatchRefresh(*captureBatchRefresh(rows,{},0,settings),&browser,headers,settings);
  check(routed.requests[0].headers.at("Authorization")=="Basic owned-auth"&&routed.requests[0].headers.at("Cookie")=="owned-cookie"&&routed.requests[0].proxy==browser[0].proxy&&routed.requests[0].session==browser[0].session,"Refresh retains matching global and browser headers proxy and session");
  rows[0].address="http://other.example.test/new.bin";auto unmatched=projectBatchRefresh(*captureBatchRefresh(rows,{},0,settings),&browser,{},settings);
  check(unmatched.requests[0].headers.empty()&&unmatched.requests[0].proxy.empty()&&unmatched.requests[0].session.empty()&&unmatched.values[0].row.fallbackName=="new.bin","An edited URL loses unrelated browser context and receives a fresh fallback name");rows[0].address=urls[0];
  auto saved=settings;saved["SiteLogins"]=Json::array({Json{{"Origin","http://127.0.0.1:1"}}});auto login=projectBatchRefresh(*captureBatchRefresh(rows,{},0,settings),&browser,{},saved);
  check(!login.requests[0].address.empty(),"Current saved-login policy invalidates a global observed-response hint");
  auto noProxy=settings;noProxy["UseBrowserProxy"]=false;check(!projectBatchRefresh(*captureBatchRefresh(rows,{},0,settings),&browser,{},noProxy).requests[0].address.empty(),"Disabling the browser route invalidates its observed-response hint");
  bool cancelled=false;try{projectBatchRefresh(*captured,&browser,{},settings,[]{throw Cancelled();});}catch(const Cancelled&){cancelled=true;}check(cancelled,"Refresh projection cancels before publishing a partial row set");
  bool rejected=false;try{captureBatchRefresh(rows,{},2,settings);}catch(...){rejected=true;}check(rejected,"Capture rejects an out-of-range edited source");
  rejected=false;try{auto bad=*captured;bad.rows.push_back(bad.rows.front());projectBatchRefresh(bad,&browser,{},settings);}catch(...){rejected=true;}check(rejected,"Refresh rejects duplicated source identities");
  rejected=false;try{auto bad=browser;bad.pop_back();projectBatchRefresh(*captured,&bad,{},settings);}catch(...){rejected=true;}check(rejected,"Refresh rejects browser context from a different source cohort");
  BatchInitialTask task;std::atomic_bool unlocked=false;task.start(manager,{},[&](const char* point){if(std::string(point)=="refreshing-row"&&!unlocked){std::thread probe([&]{std::unique_lock lock(manager.mutex,std::try_to_lock);unlocked=lock.owns_lock();});probe.join();if(!unlocked)throw std::runtime_error("Refresh retained Manager");}},BatchInitialKind::Preview,nullptr,&browser,{},captured);
  check(until([&]{return task.ready();}),"Global refresh worker completes its owned projection");auto result=task.take();check(result.error.empty()&&unlocked&&result.refresh.values.size()==2&&result.refreshedPreview,"Global refresh prepares requests and their owner after releasing Manager");
  std::promise<void> release;auto gate=release.get_future().share();std::atomic_bool held=false;std::thread writer([&]{Lock lock(manager.mutex);held=true;gate.wait();});struct Join{std::promise<void>& release;std::thread& thread;~Join(){try{release.set_value();}catch(...){}if(thread.joinable())thread.join();}}joined{release,writer};check(until([&]{return held.load();}),"A real writer holds Manager for the single-row refresh");
  task.start(manager,{}, {},BatchInitialKind::Preview,nullptr,&browser,{},one,result.refreshedPreview);check(until([&]{return task.ready();})&&held,"A row refresh does not reacquire Manager for its existing preview policy");auto rowResult=task.take();check(rowResult.error.empty()&&rowResult.refreshedPreview==result.refreshedPreview&&rowResult.refresh.values.size()==1&&rowResult.refresh.values[0].index==1,"A row refresh keeps the existing preview owner and replaces only its own slot");
  release.set_value();writer.join();check(manager.snapshot()==before&&readText(manager.root/L"state.json")==disk&&!manager.fileInfoDecisionPending,"Refresh preparation changes no catalog settings download or command ownership");manager.stop();
 }catch(const std::exception& failure){error=failure.what();}
 atomicText(root/L"results.json",Json{{"passed",error.empty()},{"checks",checks},{"error",error}}.dump(2),false);std::cout<<checks.size()<<" checks; "<<(error.empty()?"passed":error)<<"\n";return error.empty()?0:1;
}
