#pragma once
#include "DownloadListSnapshot.hpp"
#include "NavigationTreeSnapshot.hpp"
namespace udm {
inline bool downloadListQueueMember(const Json& data){return yes(data,"QueueMember",str(data,"Status")!="Complete");}
// Both arguments belong to the UI snapshot. No Job/Manager or filesystem read
// is permitted while Windows asks for a tooltip.
inline std::string downloadListTooltip(const std::shared_ptr<const DownloadListRow>& row,const NavigationTreeSnapshot& tree){
 if(!row)return {};
 std::string queue="Not in a queue";
 if(downloadListQueueMember(row->data)){
  const auto found=tree.queueDescriptions.find(str(row->data,"Queue"));
  queue=found==tree.queueDescriptions.end()?"Not in an existing queue":found->second;
 }
 return str(row->data,"FileName")+"\n"+queue;
}
}
