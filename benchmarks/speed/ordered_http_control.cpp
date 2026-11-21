#include "Core.hpp"
#include <iostream>
using namespace udm;
int wmain(int argc,wchar_t** argv){
 if(argc!=4)return 2;const fs::path area=fs::absolute(argv[1]);const std::string mode=utf8(argv[3]);
 if(mode!="pause"&&mode!="resume"&&mode!="complete")return 3;
 if(mode=="resume"){if(!fs::is_regular_file(area/L".udm-ordered-http-control"))return 4;}
 else{if(fs::exists(area))return 5;fs::create_directories(area);atomicText(area/L".udm-ordered-http-control","owned native HTTP qualification",false);}
 Json result;bool passed=false;std::string error;
 try{
  Manager m(area/L"catalog");JobPtr job;
  if(mode=="resume"){if(m.jobs.size()!=1)throw std::runtime_error("Expected one saved controlled job");job=m.jobs[0];}
  else{
   {Lock lock(m.mutex);auto& prefs=m.state["Settings"];prefs["Connections"]=8;prefs["Parallel"]=1;prefs["SuppressProgressDialog"]=true;prefs["SuppressCompletionDialog"]=true;prefs["Sound"]=false;prefs["ClipboardMonitor"]=false;prefs["PrefetchFileInfo"]=false;prefs["CloseToTray"]=false;prefs["Retries"]=10;prefs["CategoryFolders"]=false;m.state["Queues"][0]["Retries"]=10;}
   job=m.add(utf8(argv[2]),utf8((area/L"files").wstring()),"ordered-control.bin","Main queue",true);
  }
  m.resume(job);const auto began=GetTickCount64();bool requestedPause=false;
  for(;;){
   m.tick();
   bool complete,active;std::string status; i64 received;
   {Lock lock(m.mutex);status=str(job->data,"Status");complete=status=="Complete";active=m.isActive(job);received=num(job->data,"Received");}
   if(mode=="pause"&&!requestedPause&&received>=1048576){m.pause(job);requestedPause=true;}
   if((complete||status=="Failed"||requestedPause)&&!active)break;
   if(GetTickCount64()-began>120000)throw std::runtime_error("Controlled HTTP deadline exceeded");
   Sleep(5);
  }
  m.stop();
  {Lock lock(m.mutex);result["job"]=job->snapshot();}
  const auto status=str(result["job"],"Status");
  if(mode=="pause"){
   if(!requestedPause||status!="Paused"||num(result["job"],"Received")<=0||num(result["job"],"Received")>=num(result["job"],"Size"))throw std::runtime_error("Controlled pause did not retain a partial job");
   if(!yes(result["job"]["OrderedBodyWrites"],"Used")||!yes(result["job"]["OrderedBodyWrites"],"Paused"))throw std::runtime_error("The controlled pause did not use ordered-body preservation");
   i64 saved=0;for(const auto& segment:result["job"]["Segments"])saved+=num(segment,"Done");if(saved!=num(result["job"],"Received"))throw std::runtime_error("Controlled pause counters disagree");
  }else{
   if(status!="Complete")throw std::runtime_error("Controlled HTTP failed: "+str(result["job"],"Error"));
   if(mode=="resume"&&result["job"].contains("OrderedBodyWrites"))throw std::runtime_error("A legacy resume retained stale ordered-generation diagnostics");
   const auto path=job->target();result["fileSha256"]=fileHash(path);result["fileBytes"]=fs::file_size(path);result["target"]=utf8(path.wstring());
   if(mode=="complete"&&(!yes(result["job"]["OrderedBodyWrites"],"Used")||!yes(result["job"]["OrderedBodyWrites"],"Complete")))throw std::runtime_error("Controlled completion did not use joined ordered writes");
  }
  result["elapsedMs"]=GetTickCount64()-began;result["normalStopJoined"]=true;passed=true;
 }catch(const std::exception& e){error=e.what();}
 result["passed"]=passed;result["error"]=error;result["mode"]=mode;result["qualificationFeatureDefine"]="UDM_ORDERED_BODY_QUALIFICATION";atomicText(area/wide(mode+"-result.json"),result.dump(2),false);std::cout<<(passed?"passed":error)<<"\n";return passed?0:1;
}
