#include "FileInfoDisplayTask.hpp"
#include "ZipPreview.hpp"
#include <future>
#include <iostream>
using namespace udm;
template<class F>static bool until(F predicate){const auto end=GetTickCount64()+4000;while(!predicate()&&GetTickCount64()<end)Sleep(1);return predicate();}
struct ReleaseDisplayRead {std::promise<void>& release;~ReleaseDisplayRead(){try{release.set_value();}catch(...){}}};
int wmain(int argc,wchar_t** argv){
 if(argc!=2)return 2;const auto root=fs::absolute(argv[1]);if(fs::exists(root))return 2;fs::create_directories(root);Json checks=Json::array();std::string error;
 const auto check=[&](bool ok,const char* name){checks.push_back({{"name",name},{"passed",ok}});if(!ok)throw std::runtime_error(name);};
 try{
  Manager manager(root/L"catalog");manager.state["Settings"]["PrefetchFileInfo"]=false;
  auto job=manager.add("http://127.0.0.1:1/file.zip",utf8((root/L"files").wstring()),"file.zip","Main queue",true);
  job->data["Size"]=1024;job->data["ResponseFileName"]="first.zip";job->data["ContentType"]="application/zip";
  const auto original=captureFileInfoDisplay(manager,job,false);
  check(original&&original->member&&original->job==job&&!original->draftLogin,"A display snapshot identifies a current member and draft context");
  check(num(original->data,"Size")==1024&&str(original->data,"ResponseFileName")=="first.zip","Size and server filename are captured coherently");
  check(original->canCheck==canPreviewDownload(*job,false)&&original->canRefreshAddress==manager.canRefreshAddress(job),"Snapshot preview and refresh eligibility match authoritative helpers");
  check(canPreviewZip(original->zipInputs,"",true)==canPreviewZip(job->data,"",true),"The minimal ZIP projection preserves ordinary preview eligibility");
  job->data["ProtectedRequest"]="private-body";job->data["Segments"]=Json::array({Json{{"Private","journal"}}});
  const auto protectedView=captureFileInfoDisplay(manager,job,false);
  check(!protectedView->data.contains("ProtectedRequest")&&!protectedView->data.contains("Segments"),"Display data excludes protected bodies and transfer journals");
  check(str(protectedView->zipInputs,"ProtectedRequest")=="present"&&!canPreviewZip(protectedView->zipInputs,"",true),"ZIP eligibility carries only protected-request presence");
  job->data.erase("ProtectedRequest");
  for(const auto key:{"ProtectedAdaptive","SourceUrl","DuplicateOf"}){
   job->data[key]="private-value";const auto view=captureFileInfoDisplay(manager,job,false);
   check(str(view->zipInputs,key)=="present"&&!canPreviewZip(view->zipInputs,"",true),"Each protected/media/duplicate marker preserves ZIP exclusion");job->data.erase(key);
  }
  job->data["OfflineProject"]=Json::object();check(!canPreviewZip(captureFileInfoDisplay(manager,job,false)->zipInputs,"",true),"Even an empty offline-project value preserves presence-based exclusion");job->data.erase("OfflineProject");
  for(const auto key:{"RequiresRequestCapture","RequiresMediaCapture","RequiresBrowserSessionCapture"}){job->data[key]=true;check(!canPreviewZip(captureFileInfoDisplay(manager,job,false)->zipInputs,"",true),"Each browser/request capture requirement excludes ZIP preview");job->data.erase(key);}
  job->data["RefreshPendingValidation"]=true;check(captureFileInfoDisplay(manager,job,false)->replacementPending&&!captureFileInfoDisplay(manager,job,false)->canCheck,"A replacement awaiting its response cannot reuse preview eligibility");job->data.erase("RefreshPendingValidation");
  {
   std::atomic_uint captures=0,wakes=0;std::atomic_bool inject=false;const auto owner=std::this_thread::get_id();std::atomic_bool ownerRead=false;
   FileInfoDisplayTask task([&]{++wakes;},[&](const char* stage){if(std::string(stage)=="before-read"){if(std::this_thread::get_id()==owner)ownerRead=true;if(inject)throw std::runtime_error("owned display failure");}if(std::string(stage)=="captured")++captures;});
   check(task.request(manager,job,false)&&until([&]{return wakes>=1;}),"The owner can request a display read on a lazy worker");auto initial=task.take().snapshot;
   check(initial&&num(initial->data,"Size")==1024&&!ownerRead,"The result arrives without reading the model on the requesting thread");
   std::promise<void> entered,release;auto ready=entered.get_future();auto released=release.get_future().share();std::atomic_bool held=false;
   std::thread writer([&]{Lock lock(manager.mutex);job->data["Size"]=4096;job->data["ResponseFileName"]="second.zip";held=true;entered.set_value();released.wait();held=false;});
   struct Join{std::promise<void>& release;std::thread& writer;~Join(){try{release.set_value();}catch(...){}if(writer.joinable())writer.join();}} join{release,writer};
   check(ready.wait_for(std::chrono::seconds(3))==std::future_status::ready,"A real writer owns the model before the next display request");
   for(int i=0;i<100;++i)task.request(manager,job,false);
   auto busy=task.take();check(held&&busy.snapshot==initial&&num(busy.snapshot->data,"Size")==1024,"Requests and result observation return the immutable previous display while the model is held");
   release.set_value();writer.join();check(until([&]{const auto result=task.take();return result.snapshot&&num(result.snapshot->data,"Size")==4096&&!result.active&&!result.pending;}),"The coalesced read converges after the writer releases the model");
   const auto refreshed=task.take().snapshot;check(str(refreshed->data,"ResponseFileName")=="second.zip"&&num(initial->data,"Size")==1024,"Fresh metadata is coherent and does not mutate the older display");
   const auto settled=wakes.load();Sleep(20);check(wakes==settled&&captures<=3,"Completed work cannot schedule itself or replay a request storm");
   inject=true;task.request(manager,job,false);check(until([&]{return wakes>settled;}),"An injected read failure settles on the worker");const auto failed=task.take();check(failed.error=="owned display failure"&&failed.snapshot==refreshed,"A failed read reports its error while retaining the last good display");inject=false;
   manager.fileInfoDecisionPending=true;const auto before=wakes.load();task.request(manager,job,true);check(until([&]{return wakes>before;}),"An accepted decision defers advisory collection without blocking observation");check(task.take().snapshot==refreshed,"Decision deferral retains the previous draft's identified view");manager.fileInfoDecisionPending=false;
   task.request(manager,job,true);check(until([&]{auto result=task.take();return result.snapshot&&result.snapshot->draftLogin&&!result.active&&!result.pending;}),"A later request captures the changed draft context");
   {Lock lock(manager.mutex);manager.jobs.erase(std::remove(manager.jobs.begin(),manager.jobs.end(),job),manager.jobs.end());}
   task.request(manager,job,true);check(until([&]{auto result=task.take();return result.snapshot&&!result.snapshot->member&&!result.active&&!result.pending;}),"A removed job produces an explicit unavailable view without resurrecting it");
   task.stop();check(!task.request(manager,job,false),"A joined display owner refuses subsequent requests");
  }
  {
   manager.jobs.push_back(job);std::promise<void> entered,release;auto ready=entered.get_future();auto released=release.get_future().share();
   FileInfoDisplayTask task({},[&](const char* stage){if(std::string(stage)=="before-read"){entered.set_value();released.wait();}});std::future<void> stopping;ReleaseDisplayRead cleanup{release};task.request(manager,job,false);
   check(ready.wait_for(std::chrono::seconds(3))==std::future_status::ready,"Teardown begins with a real in-flight display read");stopping=std::async(std::launch::async,[&]{task.stop();});
   check(stopping.wait_for(std::chrono::milliseconds(20))==std::future_status::timeout,"Owner teardown joins the read before model lifetime can end");release.set_value();stopping.get();check(!task.take().active&&!task.request(manager,job,false),"Joined teardown leaves no callback or accepted request behind");
  }
  check(!fileInfoDialogShowing(),"No File Info presentation guard is leaked initially");{FileInfoDialogOwner first;{FileInfoDialogOwner second;check(fileInfoDialogShowing(),"Nested File Info owners keep shutdown guarded");}check(fileInfoDialogShowing(),"Closing a nested owner retains the outer presentation guard");}check(!fileInfoDialogShowing(),"The final File Info owner releases its presentation guard");
  manager.stop();
 }catch(const std::exception& e){error=e.what();}
 atomicText(root/L"results.json",Json{{"passed",error.empty()},{"checks",checks},{"error",error}}.dump(2),false);std::cout<<checks.size()<<" checks; "<<(error.empty()?"passed":error)<<"\n";return error.empty()?0:1;
}
