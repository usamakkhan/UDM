#pragma once
#include "Core.hpp"
#include "AdaptiveJournal.hpp"
#include <cmath>

namespace udm {
struct DashPeriodPlan {
 size_t sourceIndex=0;
 Json id;
 double start=0,duration=0;
 Json plan;
};

// Periods remain independent transfer/mux inputs. Never interpret a nested
// presentation as a legacy plan containing only its first Period's tracks.
inline std::vector<DashPeriodPlan> dashPeriodPlans(const Json& presentation){
 if(!presentation.is_object()||presentation.dump().size()>4ULL*1024*1024||str(presentation,"type")!="dash"||presentation.contains("tracks")||yes(presentation,"live")||yes(presentation,"encrypted")||presentation.contains("resources")||presentation.contains("resourceVersion"))throw std::runtime_error("Invalid DASH Period presentation.");
 if(!presentation.contains("periods")||!presentation["periods"].is_array()||presentation["periods"].empty()||presentation["periods"].size()>128)throw std::runtime_error("Expected between 1 and 128 DASH Periods.");
 for(const char* field:{"audioOnly","audioExpected","live","encrypted"})if(presentation.contains(field)&&!presentation[field].is_boolean())throw std::runtime_error("Invalid DASH Period selection flags.");
 if(str(presentation,"container","mp4")!=(yes(presentation,"audioOnly")?"m4a":"mp4"))throw std::runtime_error("DASH Period output requires MP4 or M4A.");
 std::vector<DashPeriodPlan> result;std::set<std::string> ids;size_t segments=0;Json trackIdentity;
 for(const auto& period:presentation["periods"]){
  if(!period.is_object()||!period.contains("index")||!period["index"].is_number_integer()||num(period,"index")<0||num(period,"index")>=128||!period.contains("id")||(!period["id"].is_null()&&!period["id"].is_string()))throw std::runtime_error("Invalid DASH Period identity.");
  DashPeriodPlan item;item.sourceIndex=(size_t)num(period,"index");item.id=period["id"];
  if(!item.id.is_null()){
   auto id=item.id.get<std::string>();if(id.size()>512||std::any_of(id.begin(),id.end(),[](unsigned char c){return c<32||c==127;})||!ids.insert(id).second)throw std::runtime_error("Duplicate or invalid DASH Period ID.");
  }
  for(const char* field:{"start","duration"})if(!period.contains(field)||!period[field].is_number()||!std::isfinite(real(period,field)))throw std::runtime_error("DASH Period assembly requires finite timing.");
  item.start=real(period,"start");item.duration=real(period,"duration");
  if(item.start<0||item.duration<=0||item.start+item.duration>7*86400||(!result.empty()&&(item.sourceIndex<=result.back().sourceIndex||std::abs(item.start-result.back().start-result.back().duration)>0.000001)))throw std::runtime_error("DASH Period order, gap or overlap requires timeline assembly support.");
  item.plan=Json{{"type","dash"}};
  for(const char* field:{"height","audioOnly","audioExpected","container","audioName","audioLanguage","subtitleName","subtitleLanguage"})if(presentation.contains(field))item.plan[field]=presentation[field];
  if(!period.contains("tracks")||!period["tracks"].is_array())throw std::runtime_error("DASH Period tracks are missing.");
  item.plan["tracks"]=period["tracks"];
  for(const char* field:{"resources","resourceVersion"})if(period.contains(field))item.plan[field]=period[field];
  // Reuse ordinary URL, byte-range, container, subtitle and resource validation.
  // The child has no periods field, so this does not recurse into this function.
  validateAdaptive(item.plan);
  Json identity=Json::array();
  for(const auto& track:item.plan["tracks"]){
   if(track.contains("index"))throw std::runtime_error("Expand DASH indexes before Period handoff.");
   if(!track.contains("clock")||!track["clock"].is_object())throw std::runtime_error("DASH Period media clock is missing.");
   const auto& clock=track["clock"];
   for(const char* field:{"timescale","presentationTimeOffset"})if(!clock.contains(field)||!clock[field].is_number_integer()||num(clock,field)<(std::string(field)=="timescale"?1:0)||num(clock,field)>9007199254740991LL)throw std::runtime_error("Invalid DASH Period media clock.");
   if(!track.contains("selection")||!track["selection"].is_object())throw std::runtime_error("DASH Period track description is missing.");
   auto description=track["selection"];
   for(const char* field:{"mime","codecs","sampleRate","label"})if(!description.contains(field)||!description[field].is_string()||str(description,field).size()>1024)throw std::runtime_error("Invalid DASH Period track description.");
   if(str(track,"kind")!="subtitle"&&str(description,"codecs").empty())throw std::runtime_error("DASH Period matching requires declared codecs.");
   for(const char* field:{"roles","channels"}){
    if(!description.contains(field)||!description[field].is_array()||description[field].size()>32)throw std::runtime_error("Invalid DASH Period role or channel configuration.");
    for(const auto& pair:description[field])if(!pair.is_array()||pair.size()!=2||!pair[0].is_string()||!pair[1].is_string()||pair[0].get<std::string>().size()>1024||pair[1].get<std::string>().size()>1024)throw std::runtime_error("Invalid DASH Period descriptor.");
    std::sort(description[field].begin(),description[field].end());
   }
   description.erase("adaptationId");description.erase("representationId");
   identity.push_back(Json{{"kind",str(track,"kind")},{"height",num(track,"height")},{"frameRate",real(track,"frameRate")},{"language",lower(trim(str(track,"language")))},{"selection",description}});
   segments+=track["segments"].size();if(segments>10000)throw std::runtime_error("DASH Period selection exceeds the segment limit.");
  }
  if(result.empty())trackIdentity=identity;else if(identity!=trackIdentity)throw std::runtime_error("The selected DASH tracks change format across Periods.");
  result.push_back(std::move(item));
 }
 return result;
}

// A future recovery journal binds to this complete selection, not merely to
// part count. Signed URLs and validators may change; Period order/clocks may not.
inline Json dashPeriodRecoveryIdentity(const std::vector<DashPeriodPlan>& periods){
 Json identity=Json::array();
 for(const auto& period:periods){
  auto plan=period.plan;plan.erase("resources");plan.erase("resourceVersion");
  for(auto& track:plan["tracks"])for(auto& segment:track["segments"])segment.erase("url");
  identity.push_back(Json{{"index",period.sourceIndex},{"id",period.id},{"start",period.start},{"duration",period.duration},{"plan",std::move(plan)}});
 }
 return identity;
}
inline AdaptiveJournal dashPeriodJournal(const fs::path& folder,const std::vector<DashPeriodPlan>& periods){
 size_t parts=0;for(const auto& period:periods)for(const auto& track:period.plan["tracks"])parts+=track["segments"].size();
 // New Period journals must never adopt an unbound legacy flat-track receipt.
 return AdaptiveJournal(folder,parts,adaptiveSelectionDigest(dashPeriodRecoveryIdentity(periods)),false);
}
}
