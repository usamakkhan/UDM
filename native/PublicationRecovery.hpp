#pragma once
#include "Core.hpp"

namespace udm {
// Unknown journal state must preserve staging, not authorize overwriting it.
inline bool publicationNeedsRecovery(const Manager& manager,const JobPtr& job){
 for(const auto* kind:{L"publications",L"replacements"}){
  std::error_code error;const bool present=fs::exists(manager.root/kind/(wide(job->id())+L".json"),error);
  if(error||present)return true;
 }
 return false;
}
inline void requirePublicationRecovered(const Manager& manager,const JobPtr& job){
 if(publicationNeedsRecovery(manager,job))throw std::runtime_error("The previous save needs publication recovery. After this download stops, use Resume or restart UDM; its files have been preserved.");
}
}
