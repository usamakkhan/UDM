#include "FileInfoRefreshTask.hpp"
#include <iostream>
namespace udm {struct CatalogCheckpointTestAccess {static void observe(Manager& m,std::function<void(const char*)> value){Lock lock(m.mutex);m.catalogCheckpoint=std::move(value);}};}
using namespace udm;
template<class F>static bool until(F f){auto end=GetTickCount64()+6000;while(!f()&&GetTickCount64()<end)Sleep(1);return f();}
int wmain(int argc,wchar_t** argv){
 if(argc!=2)return 2;auto root=fs::absolute(argv[1]);if(fs::exists(root))return 2;fs::create_directories(root);Json checks=Json::array();std::string error;
 const auto check=[&](bool ok,const char* name){checks.push_back({{"name",name},{"passed",ok}});if(!ok)throw std::runtime_error(name);};
 try{
  Manager manager(root/L"catalog");manager.state["Settings"]["PrefetchFileInfo"]=false;manager.state["Settings"]["CategoryFolders"]=false;manager.state["Settings"].erase("SiteLogins");
  const auto make=[&](const char* name){return manager.add(std::string("http://127.0.0.1:1/")+name,utf8((root/L"files").native()),name,"Main queue",true);};
  const auto run=[&](const JobPtr& job,Json expected,FileInfoRefreshObserver observe={}){FileInfoRefreshTask task;check(task.start(manager,job,std::move(expected),{},std::move(observe)),"An owned refreshed-details command starts");if(!until([&]{return task.ready();}))throw std::runtime_error("Refreshed details did not settle");return task.take();};
  {
   auto job=make("ordinary.bin");job->data["DownloadPage"]="https://example.invalid/page";Headers headers;setBasicLogin(headers,"owned-user","owned-password");job->data["ProtectedHeaders"]=protect(legacyDictionary(Json(headers)).dump());job->data["ConfirmationPending"]=true;job->data["PrefetchedFinalSize"]=123;job->data["Size"]=123;auto expected=fileInfoSelection(*job);auto result=run(job,expected);
   check(result.error.empty()&&!result.cancelled&&!result.media&&result.selection==expected,"Continuation preserves the exact accepted request selection");
   check(!yes(job->data,"ConfirmationPending")&&!job->data.contains("PrefetchedFinalSize")&&str(job->data,"Status")=="Paused","Ordinary continuation durably stops its unconfirmed prefetch state");
   check(result.address==str(job->data,"Url")&&result.page=="https://example.invalid/page"&&result.basic&&result.login==std::make_pair(std::string("owned-user"),std::string("owned-password")),"The worker resolves the refreshed address, page and private Basic credentials");
   check(!result.preferences.contains("SiteLogins"),"Absent remembered site logins remain absent in the owned reply");
  }
  {
   auto job=make("metadata.bin");auto selected=fileInfoSelection(*job);job->data["Size"]=200;job->data["Description"]="owned draft metadata";auto result=run(job,selected);check(result.error.empty()&&result.selection==selected,"Unrelated metadata changes do not invalidate the saved request");
   manager.state["Settings"]["SiteLogins"]=Json::array({makeSiteLogin(Url(str(job->data,"Url")).origin,"remembered","private")});result=run(job,selected);check(result.error.empty()&&result.basic&&result.login.first=="remembered"&&result.preferences.contains("SiteLogins"),"Remembered credentials are resolved on the worker from current settings");manager.state["Settings"].erase("SiteLogins");
  }
  {
   auto job=make("stale.bin");auto selected=fileInfoSelection(*job);job->data["ProtectedBrowserProxy"]="changed";const auto before=manager.snapshot();auto result=run(job,selected);check(!result.error.empty()&&manager.snapshot()==before,"A changed saved route is rejected before prefetch or catalog mutation");job->data["ProtectedBrowserProxy"]="";
   result=run(job,Json::object());check(!result.error.empty(),"An empty recovery selection cannot authorize continuation");
   selected=fileInfoSelection(*job);manager.remove(job);result=run(job,selected);check(!result.error.empty(),"A removed target cannot be prepared through its retained JobPtr");
  }
  {
   auto job=make("changed-after-stop.bin");job->data["ConfirmationPending"]=true;auto selected=fileInfoSelection(*job);auto result=run(job,selected,[&](const char* stage){if(std::string(stage)=="stopped-prefetch"){Lock lock(manager.mutex);job->data["Url"]="http://127.0.0.1:1/newer.bin";}});check(!result.error.empty()&&result.selection.empty()&&str(job->data,"Url").find("newer.bin")!=std::string::npos,"A request changed after stopping returns no usable stale launch guard");
  }
  {
   auto job=make("save-failure.bin");job->data["ConfirmationPending"]=true;auto selected=fileInfoSelection(*job);CatalogCheckpointTestAccess::observe(manager,[](const char* stage){if(std::string(stage)=="temporary-flushed")throw std::runtime_error("Owned continuation checkpoint failure");});struct Clear{Manager& m;~Clear(){CatalogCheckpointTestAccess::observe(m,{});}} clear{manager};auto result=run(job,selected);check(!result.error.empty()&&yes(job->data,"ConfirmationPending"),"A real stop-save failure rolls back confirmation without returning ready metadata");CatalogCheckpointTestAccess::observe(manager,{});result=run(job,selected);check(result.error.empty()&&!yes(job->data,"ConfirmationPending"),"Only an explicit continuation retry finishes the recovered stop");
  }
  {
   auto job=make("media.mp4");job->data["SourceUrl"]="https://example.invalid/watch";job->data["ConfirmationPending"]=true;auto result=run(job,fileInfoSelection(*job));check(result.error.empty()&&result.media&&yes(job->data,"ConfirmationPending")&&!result.basic&&result.login.first.empty(),"Media continuation retains its existing prefetch semantics and does not load ordinary site credentials");job->data["ConfirmationPending"]=false;
  }
  {
   auto job=make("cold.bin");auto selected=fileInfoSelection(*job);std::promise<void> release;auto gate=release.get_future().share();std::atomic_bool held=false,waiting=false;std::thread writer([&]{Lock lock(manager.mutex);held=true;gate.wait();held=false;});struct Join{std::promise<void>& release;std::thread& worker;~Join(){try{release.set_value();}catch(...){}if(worker.joinable())worker.join();}} cleanup{release,writer};check(until([&]{return held.load();}),"A real writer owns Manager before continuation");FileInfoRefreshTask task;task.start(manager,job,selected,{},[&](const char* stage){if(std::string(stage)=="waiting-model")waiting=true;});check(until([&]{return waiting.load();})&&held,"The requesting thread remains free while the worker waits for Manager");task.cancel();check(until([&]{return task.ready();})&&task.take().cancelled&&held,"Advisory cancellation settles without releasing the held model");release.set_value();writer.join();
  }
  {
   auto job=make("draining.bin");job->data["ConfirmationPending"]=true;auto selected=fileInfoSelection(*job);std::promise<void> release;auto gate=release.get_future().share();std::atomic_bool saving=false,joined=false;CatalogCheckpointTestAccess::observe(manager,[&](const char* stage){if(std::string(stage)=="temporary-flushed"){saving=true;gate.wait();}});
   FileInfoRefreshTask task;std::thread closer;struct Clear{Manager& m;std::promise<void>& release;std::thread& closer;~Clear(){try{release.set_value();}catch(...){}if(closer.joinable())closer.join();CatalogCheckpointTestAccess::observe(m,{});}} cleanup{manager,release,closer};task.start(manager,job,selected);check(until([&]{return saving.load();}),"A real accepted prefetch-stop checkpoint remains in flight");task.cancel();closer=std::thread([&]{task.finish();joined=true;});Sleep(30);check(!joined,"Cancellation does not abandon an accepted durable stop");release.set_value();closer.join();check(joined&&!yes(job->data,"ConfirmationPending")&&!task.valid(),"Owner release joins the accepted stop before discarding its cancelled reply");CatalogCheckpointTestAccess::observe(manager,{});
  }
  {
   auto job=make("notify.bin");FileInfoRefreshTask task;std::atomic_bool notified=false;task.start(manager,job,fileInfoSelection(*job),[&]{notified=true;throw std::runtime_error("Owned notification failure");});check(!task.start(manager,job,fileInfoSelection(*job)),"An in-flight continuation refuses duplicate submissions");check(until([&]{return notified.load();})&&task.ready()&&task.take().error.empty(),"Publication precedes notification and a throwing wake-up cannot undo the result");
  }
  manager.stop();
 }catch(const std::exception& failure){error=failure.what();}
 atomicText(root/L"results.json",Json{{"passed",error.empty()},{"checks",checks},{"error",error}}.dump(2),false);std::cout<<checks.size()<<" checks; "<<(error.empty()?"passed":error)<<"\n";return error.empty()?0:1;
}
