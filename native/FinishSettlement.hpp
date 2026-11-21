#pragma once
#include "Core.hpp"
namespace udm {
// Capture model state under its lock, then inspect the filesystem without it.
// PublicationPreparation still pins/revalidates the current destination before
// any rename. The validator receives an immutable, owned snapshot.
template<class Validate> Json finishSettlementSnapshot(Manager& manager,const JobPtr& job,double transferSeconds,Json& phases,Validate&& validate){
 const auto seconds=[](auto from){return std::chrono::duration<double>(std::chrono::steady_clock::now()-from).count();};
 auto began=std::chrono::steady_clock::now();Json result;
 {
  Lock lock(manager.mutex);phases["SettleLockWait"]=seconds(began);began=std::chrono::steady_clock::now();
  job->data.erase("PrefetchedFinalSize");job->data["TransferSeconds"]=transferSeconds;
  job->data["Status"]="Verifying";result=job->data;
 }
 phases["SettleSnapshot"]=seconds(began);began=std::chrono::steady_clock::now();
 validate(static_cast<const Json&>(result));
 phases["SettleValidateFolder"]=seconds(began);return result;
}
}
