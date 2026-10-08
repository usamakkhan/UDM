#pragma once
#include "Core.hpp"
#include "Scanner.hpp"
namespace udm {
inline bool cliCompletionReady(Manager& manager,JobPtr job){
 Lock lock(manager.mutex);
 return job&&str(job->data,"Status")=="Complete"&&!manager.isActive(job)&&scannerAllowsCompletion(job->data)&&manager.completionSaved(job);
}
inline void armCliHangup(Manager& manager,JobPtr job){
 Lock lock(manager.mutex);
 if(job&&str(job->data,"Status")!="Complete"&&std::find(manager.cliHangups.begin(),manager.cliHangups.end(),job)==manager.cliHangups.end())manager.cliHangups.push_back(job);
}
inline JobPtr takeCliHangup(Manager& manager){
 Lock lock(manager.mutex);
 for(auto it=manager.cliHangups.begin();it!=manager.cliHangups.end();){
  auto job=*it;
  if(std::find(manager.jobs.begin(),manager.jobs.end(),job)==manager.jobs.end()){it=manager.cliHangups.erase(it);continue;}
  if(cliCompletionReady(manager,job)){
   manager.cliHangups.erase(it);return job;
  }
  ++it;
 }
 return {};
}
}
