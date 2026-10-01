#include "../../native/Core.hpp"
#include <psapi.h>
#include <future>
#include <iostream>
using namespace udm;
int wmain(int argc,wchar_t** argv){
 if(argc!=2)return 2;
 auto inputPath=fs::absolute(argv[1]);Json report;
 try{
  auto spec=Json::parse(readText(inputPath));auto root=inputPath.parent_path();Manager manager(root/L"state");
  auto& settings=manager.state["Settings"];settings["DownloadFolder"]=utf8((root/L"downloads").wstring());settings["CategoryFolders"]=false;settings["Retries"]=num(spec,"retries",2);settings["LimitKbps"]=num(spec,"globalLimit");settings["Parallel"]=num(spec,"parallel",4);
  manager.state["Queues"][0]["Parallel"]=num(spec,"parallel",4);
  if(!yes(spec,"resume"))for(const auto& item:spec["jobs"]){Url url(str(item,"url"));if(url.host!="127.0.0.1"&&url.host!="localhost")throw std::runtime_error("Stress harness accepts loopback hosts only.");auto job=manager.add(str(item,"url"),"",str(item,"name"),"Main queue",true,{},str(item,"sha256"));job->data["Connections"]=num(item,"connections",8);job->data["LimitKbps"]=num(item,"limit");}
  manager.save();const auto started=std::chrono::steady_clock::now();auto elapsed=[&]{return std::chrono::duration<double>(std::chrono::steady_clock::now()-started).count();};
  DWORD initialHandles=0;GetProcessHandleCount(GetCurrentProcess(),&initialHandles);std::atomic_bool finished{false};std::atomic<double> cancelAt{-1};auto cancel=std::make_shared<Cancel>();std::string watcherError;size_t peakActive=0;Json handleSamples=Json::array(),settledSamples=Json::array();size_t performed=0;
  std::thread watcher([&]{int ticks=0;while(!finished){Sleep(25);bool due=num(spec,"cancelAfterMs")>0&&elapsed()*1000>=num(spec,"cancelAfterMs");if(num(spec,"cancelAfterBytes")>0){Lock lock(manager.mutex);for(auto job:manager.jobs)due|=num(job->data,"Received")>=num(spec,"cancelAfterBytes");}if(due&&!cancel->stop){cancelAt=elapsed();cancel->stop=true;if(str(spec,"mode")=="queue")for(auto job:manager.jobs)manager.pause(job);}if(++ticks%10==0){try{Lock lock(manager.mutex);manager.save();}catch(const std::exception& e){watcherError=e.what();}}}});
  auto perform=[&](JobPtr job){try{{Lock lock(manager.mutex);job->data["Status"]="Downloading";}if(num(spec,"postBytes")>0){Bytes payload((size_t)num(spec,"postBytes"));for(size_t i=0;i<payload.size();++i)payload[i]=(unsigned char)((i*31+7)%251);Http response(str(job->data,"Url"),{},settings,*cancel,{},{},"",&payload);auto echoed=response.all(payload.size(),*cancel);if(echoed!=payload)throw std::runtime_error("POST echo differs from original payload.");fs::create_directories(job->target().parent_path());writeBytes(job->target(),echoed);Lock lock(manager.mutex);job->data["Status"]="Complete";job->data["Size"]=echoed.size();job->data["Received"]=echoed.size();job->data["Sha256"]=fileHash(job->target());}else transfer(manager,job,cancel);}catch(const std::exception& e){Lock lock(manager.mutex);job->data["Status"]=cancel->cancelled()?"Paused":"Failed";job->data["Error"]=e.what();}{Lock lock(manager.mutex);++performed;if(performed==1||performed%10==0){DWORD count=0;GetProcessHandleCount(GetCurrentProcess(),&count);handleSamples.push_back({{"completed",performed},{"handles",count}});}}};
  try{
   if(str(spec,"mode")=="queue"){
    for(auto job:manager.jobs)manager.resume(job);
    for(;;){manager.tick();bool pending=false;size_t active=0;{Lock lock(manager.mutex);for(auto job:manager.jobs){active+=manager.isActive(job);pending|=manager.isActive(job)||str(job->data,"Status")=="Queued";}peakActive=std::max(peakActive,active);}if(!pending)break;Sleep(20);}
   }else if(yes(spec,"concurrent")){std::vector<std::future<void>> tasks;for(auto job:manager.jobs)tasks.push_back(std::async(std::launch::async,[&,job]{perform(job);}));for(auto& task:tasks)task.get();}
   else for(auto job:manager.jobs){perform(job);if(cancel->cancelled())break;if(num(spec,"settleAfterJobs")>0&&performed%num(spec,"settleAfterJobs")==0){Sleep((DWORD)num(spec,"settleMs"));DWORD count=0;GetProcessHandleCount(GetCurrentProcess(),&count);settledSamples.push_back({{"completed",performed},{"handles",count}});}}
  }catch(...){finished=true;watcher.join();throw;}
  const auto seconds=elapsed();finished=true;watcher.join();manager.save();
  report={{"seconds",seconds},{"cancelLatencySeconds",cancelAt<0?-1:seconds-cancelAt.load()},{"watcherError",watcherError},{"peakQueueActive",peakActive},{"jobs",Json::array()}};
  for(auto job:manager.jobs){auto& j=job->data;Json result={{"id",job->id()},{"name",str(j,"FileName")},{"status",str(j,"Status")},{"error",str(j,"Error")},{"received",num(j,"Received")},{"size",num(j,"Size")},{"sha256",str(j,"Sha256")},{"splits",num(j,"DynamicSplits")},{"workers",job->workers.size()},{"parts",j["Segments"].size()},{"exists",fs::exists(job->target())}};report["jobs"].push_back(result);}
  if(num(spec,"settleMs")>0&&num(spec,"settleAfterJobs")==0)Sleep((DWORD)num(spec,"settleMs"));report["handleSamples"]=handleSamples;report["settledSamples"]=settledSamples;DWORD handles=0;GetProcessHandleCount(GetCurrentProcess(),&handles);PROCESS_MEMORY_COUNTERS_EX memory{};GetProcessMemoryInfo(GetCurrentProcess(),(PROCESS_MEMORY_COUNTERS*)&memory,sizeof(memory));report["initialHandles"]=initialHandles;report["finalHandles"]=handles;report["peakWorkingSetBytes"]=memory.PeakWorkingSetSize;report["privateBytes"]=memory.PrivateUsage;
 }catch(const std::exception& e){report["fatal"]=e.what();}
 atomicText(inputPath.parent_path()/L"result.json",report.dump(2));std::cout<<report.dump()<<std::endl;return report.contains("fatal")?1:0;
}
