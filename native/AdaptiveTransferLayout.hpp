#pragma once
#include "AdaptiveCapture.hpp"
#include "AdaptiveResources.hpp"
#include "DashPeriods.hpp"

namespace udm {
struct AdaptiveTransferPart {
 Json info,resource;
 fs::path path;
 size_t track=0,period=0;
 i64 limit=256LL*1024*1024;
};
struct AdaptiveTransferLayout {
 std::vector<AdaptiveTransferPart> parts;
 std::vector<std::vector<size_t>> tracks;
 std::vector<Json> descriptions;
 std::vector<std::vector<size_t>> periodTracks;
 std::vector<DashPeriodPlan> periods;
 std::set<std::string> origins;
 std::string selectionDigest;
 bool multiplePeriods=false;
};

// Stable traversal is Period -> track -> segment. Receipts use this order;
// the selection digest binds its meaning, including Period clocks and ranges.
inline AdaptiveTransferLayout adaptiveTransferLayout(const Json& plan,const fs::path& folder){
 AdaptiveTransferLayout layout;layout.multiplePeriods=plan.contains("periods");
 if(layout.multiplePeriods){
  layout.periods=dashPeriodPlans(plan);
  layout.selectionDigest=adaptiveSelectionDigest(dashPeriodRecoveryIdentity(layout.periods));
 }else{
  validateAdaptive(plan);if(yes(plan,"live"))throw std::runtime_error("Live streams require their recording transfer path.");
  DashPeriodPlan period;period.plan=plan;layout.periods.push_back(std::move(period));
  layout.selectionDigest=adaptiveSelectionDigest(adaptiveSelection(plan));
 }
 for(size_t p=0;p<layout.periods.size();++p){
  const auto& period=layout.periods[p];layout.periodTracks.emplace_back();
  for(const auto& track:period.plan["tracks"]){
   size_t t=layout.tracks.size();layout.periodTracks.back().push_back(t);layout.tracks.emplace_back();layout.descriptions.push_back(track);
   for(const auto& info:track["segments"]){
    if(layout.parts.size()>=10000)throw std::runtime_error("Adaptive transfer exceeds the segment limit.");
    const auto address=str(info,"url");layout.origins.insert(Url(address).origin);
    const size_t index=layout.parts.size();layout.tracks.back().push_back(index);
    layout.parts.push_back({info,adaptiveResource(period.plan,address),folder/(std::to_wstring(index)+L".part"),t,p,str(track,"kind")=="subtitle"?2LL*1024*1024:256LL*1024*1024});
   }
  }
 }
 if(layout.parts.empty())throw std::runtime_error("The selected presentation contains no media parts.");
 return layout;
}
inline std::string adaptivePartBinding(const AdaptiveTransferLayout& layout,const AdaptiveTransferPart& part){
 Json binding={{"part",part.info},{"resource",part.resource}};
 if(layout.multiplePeriods){
  const auto& period=layout.periods.at(part.period);
  binding["period"]={{"index",period.sourceIndex},{"id",period.id},{"start",period.start},{"duration",period.duration}};
  binding["track"]=part.track;binding["clock"]=layout.descriptions.at(part.track)["clock"];
 }
 return binding.dump();
}
}
