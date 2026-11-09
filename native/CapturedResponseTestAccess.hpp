#pragma once
#include "Core.hpp"
namespace udm {
struct CapturedResponseTestAccess {
 static void saveCheckpoint(Manager& manager,std::function<void(const char*)> callback){
  manager.catalogCheckpoint=std::move(callback);
 }
 static void checkpointMediaStart(Manager& manager,const JobPtr& job){manager.checkpointMediaStart(job);}
};
}
