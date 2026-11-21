// Owned API-phase benchmark and failed-startup rollback control. No network IO.
#include "Core.hpp"
#include "Version.hpp"
#include <iostream>
#include <iomanip>
#include <sstream>
namespace udm {struct CatalogCheckpointTestAccess {
 static void reset(Manager& m){Lock lock(m.mutex);m.startupQueuesApplied=false;m.manualQueues.clear();m.cyclingQueues.clear();m.cycleFailed.clear();m.schedulePaused.clear();m.state.erase("CompletionWorkEpoch");}
 static void batch(Manager& m,bool value){Lock lock(m.mutex);m.catalogTransaction=value;}
 static bool paused(Manager& m,const JobPtr& job){Lock lock(m.mutex);return m.schedulePaused.count(job->id())!=0;}
};}
using namespace udm;
int wmain(int argc,wchar_t** argv){
 if(argc!=2)return 2;const auto root=fs::absolute(argv[1]);if(fs::exists(root))return 2;fs::create_directories(root);
 Json cases=Json::array();std::string error;
 try{
  for(const size_t count:{size_t(0),size_t(2500),size_t(7500)}){
   Manager m(root/std::to_wstring(count));auto other=defaultQueue("Archive");m.state["Queues"].push_back(other);
   const std::string payload(2048,'x');std::vector<JobPtr> history;
   for(size_t index=0;index<count;++index){std::ostringstream id;id<<std::hex<<std::setw(32)<<std::setfill('0')<<index;
    auto job=std::make_shared<Job>(Json{{"Id",id.str()},{"Folder",utf8(root.wstring())},{"FileName","history-"+std::to_string(index)+".bin"},{"Status","Complete"},{"Queue","Archive"},{"QueueMember",false},{"Description",payload}});history.push_back(job);m.jobs.push_back(job);
   }
   std::vector<JobPtr> chosen;for(unsigned i=0;i<2;++i){auto job=std::make_shared<Job>(Json{{"Id",guid()},{"Folder",utf8(root.wstring())},{"FileName","chosen-"+std::to_string(i)+".bin"},{"Status","Paused"},{"Queue","Main queue"},{"QueueMember",true}});chosen.push_back(job);m.jobs.push_back(job);}
   const auto queues=m.state["Queues"];const auto first=chosen[0]->data,second=chosen[1]->data;
   for(const std::string mode:{"no-startup","batch-start","batch-stop","main-batch-start","main-batch-stop"}){
    const std::string historyQueue=mode.rfind("main-",0)==0?"Main queue":"Archive";for(const auto& job:history)job->data["Queue"]=historyQueue;
    Json samples=Json::array();
    for(unsigned trial=0;trial<5;++trial){
     CatalogCheckpointTestAccess::reset(m);m.state["Queues"]=queues;chosen[0]->data=first;chosen[1]->data=second;
     CatalogCheckpointTestAccess::batch(m,mode!="no-startup");ULONG64 before=0,after=0;
     if(!QueryThreadCycleTime(GetCurrentThread(),&before))throw std::runtime_error("Cannot read CPU cycle counter");
     const auto start=std::chrono::steady_clock::now();
     if(mode=="no-startup")m.startQueuesOnStartup();else m.queueRun("Main queue",(mode=="batch-start"||mode=="main-batch-start"));
     const auto seconds=std::chrono::duration<double>(std::chrono::steady_clock::now()-start).count();
     if(!QueryThreadCycleTime(GetCurrentThread(),&after))throw std::runtime_error("Cannot read CPU cycle counter");
     samples.push_back({{"trial",trial},{"seconds",seconds},{"threadCycles",after-before}});
     for(const auto& job:history)if(str(job->data,"Description")!=payload||str(job->data,"Status")!="Complete"||str(job->data,"Queue")!=historyQueue||yes(job->data,"QueueMember"))throw std::runtime_error("Unrelated history changed");
    }
    cases.push_back({{"mode",mode},{"retainedHistoryQueue",historyQueue},{"unrelatedHistoryRows",count},{"historyDescriptionBytes",count*payload.size()},{"selectedRows",2},{"samples",samples},{"catalogBatchDefersIo",mode!="no-startup"}});
   }
   CatalogCheckpointTestAccess::batch(m,false);CatalogCheckpointTestAccess::reset(m);m.jobs.clear();m.stop();
  }
  {
   Manager m(root/L"rollback");auto queue=m.state["Queues"][0];queue["StartOnStartup"]=true;m.setQueue(queue);
   auto job=m.add("http://127.0.0.1:9/never-started",utf8((root/L"rollback-files").wstring()),"queued.bin","Main queue",true);job->prefetchDeferring=true;const auto before=job->data;
   fs::create_directory(m.root/L"state.json.tmp");bool refused=false;try{m.startQueuesOnStartup();}catch(...){refused=true;}fs::remove(m.root/L"state.json.tmp");
   if(!refused||job->data!=before)throw std::runtime_error("The actual failed catalog write did not restore job data");
   cases.push_back({{"mode","failed-startup-prefetch"},{"actualCatalogWriteRefused",refused},{"jobRestored",job->data==before},{"pauseHintLeaked",CatalogCheckpointTestAccess::paused(m,job)}});job->prefetchDeferring=false;m.stop();
  }
 }catch(const std::exception& failure){error=failure.what();}
 const Json result={{"passed",error.empty()},{"error",error},{"compiledVersion",UDM_VERSION_STRING},{"cases",cases},{"scope","API-phase microbenchmark. Startup applied-once flag is reset between samples; manual start/stop use an actual catalog batch to defer persistence. Not full app launch, UI paint, installed behavior or IDM parity."},{"networkRequests",0}};
 atomicText(root/L"results.json",result.dump(2),false);std::cout<<result.dump()<<"\n";return error.empty()?0:1;
}
