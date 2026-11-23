#include "DashManifest.hpp"
#include <iostream>
#include <limits>
using namespace udm;
int wmain(int argc,wchar_t** argv){
 if(argc!=2)return 2;const auto root=fs::absolute(argv[1]);if(fs::exists(root))return 2;fs::create_directories(root);Json checks=Json::array();std::string error;
 const auto check=[&](bool ok,const char* name){checks.push_back({{"name",name},{"passed",ok}});if(!ok)throw std::runtime_error(name);};
 try{
  Cancel token;
  Json track={{"identity",{{"timescale",1000},{"presentationTimeOffset",0}}},{"periodDuration",nullptr},{"segments",{{"1",{{"number",1},{"time",0},{"duration",1000},{"committed",true},{"resource",Json::object()}}},{"2",{{"number",2},{"time",1000},{"duration",1000},{"committed",true},{"resource",Json::object()}}}}}};
  const Json original={{"captureStart",0},{"captureEnd",nullptr},{"tracks",Json::array({track})}};
  auto end=dashRetainedPeriodEnd(original,token);check(end&&*end==2,"A fully retained contiguous track proves its exact observed end");
  auto rejects=[&](Json index,const char* name){check(!dashRetainedPeriodEnd(index,token),name);};
  auto change=[&](const char* field,Json value,const char* name){auto index=original;index["tracks"][0]["segments"]["2"][field]=std::move(value);rejects(std::move(index),name);};
  change("time",1100,"A timestamp gap cannot prove a Period boundary");change("time",900,"Overlapping timestamps cannot prove a Period boundary");change("number",3,"A missing segment number cannot prove coverage");change("number",1,"Duplicate segment numbers cannot hide a missing range");change("committed",false,"An unfinished tail cannot prove an end");change("duration",0,"Zero-duration media cannot supply a boundary");change("duration",-1,"Negative media duration is rejected");change("time",9007199254740991LL,"Tick addition cannot exceed exact supported precision");
  auto index=original;index["tracks"][0]["segments"]["2"].erase("resource");rejects(index,"A completion flag without a resource is not evidence");
  index=original;index["tracks"][0]["pendingInitialization"]=Json::object();rejects(index,"Pending initialization prevents boundary promotion");
  index=original;index["tracks"][0]["initialization"]=Json::object();rejects(index,"Unverified initialization cannot establish a recording end");index["tracks"][0]["initializationVerified"]=true;check(dashRetainedPeriodEnd(index,token).has_value(),"Verified initialization permits the complete coverage proof");
  for(i64 scale:{0LL,-1LL,9007199254740992LL}){index=original;index["tracks"][0]["identity"]["timescale"]=scale;rejects(index,"Invalid or inexact timescales are rejected");}
  index=original;index["captureStart"]=.5;check(dashRetainedPeriodEnd(index,token)==std::optional<double>{2},"A requested start inside the first retained segment keeps the exact end");
  index["captureStart"]=2;rejects(index,"An empty retained interval cannot establish a boundary");
  index=original;index["captureEnd"]=1.5;rejects(index,"A prior different stop boundary is preserved");
  index=original;index["tracks"][0]["periodDuration"]=3;rejects(index,"A known longer Period cannot be shortened to the observed prefix");
  index=original;index["tracks"][0]["periodDuration"]=2;check(dashRetainedPeriodEnd(index,token).has_value(),"A matching known duration remains valid");
  index=original;auto audio=track;audio["identity"]["timescale"]=48000;audio["segments"]["1"]["duration"]=48000;audio["segments"]["2"]["time"]=48000;audio["segments"]["2"]["duration"]=48000;index["tracks"].push_back(audio);check(dashRetainedPeriodEnd(index,token)==std::optional<double>{2},"Different track timescales must agree on the same presentation boundary");
  index["tracks"][1]["segments"]["2"]["duration"]=47900;rejects(index,"A shorter audio track prevents inference from video alone");
  index["tracks"][1]["segments"]["2"]["duration"]=48100;rejects(index,"A longer audio track cannot be silently clipped to video");
  Cancel stopped;stopped.stop=true;bool cancelled=false;try{dashRetainedPeriodEnd(original,stopped);}catch(const Cancelled&){cancelled=true;}check(cancelled,"Coverage inspection honors parent cancellation");
  const std::string prefix="<MPD xmlns=\"urn:mpeg:dash:schema:mpd:2011\" id=\"owned-boundary\" type=\"dynamic\" availabilityStartTime=\"2026-11-21T00:00:00Z\" publishTime=\"2026-11-21T00:01:00Z\">";
  const auto period=[](const std::string& id,const std::string& timing,const std::string& codec="avc1.42c01e"){return "<Period id=\""+id+"\" "+timing+"><AdaptationSet id=\"v\" mimeType=\"video/mp4\" codecs=\""+codec+"\" width=\"640\" height=\"360\"><Representation id=\"r\" bandwidth=\"100000\"><SegmentList timescale=\"1000\" duration=\"1000\"><Initialization sourceURL=\"init.mp4\"/><SegmentURL media=\"segment.m4s\"/></SegmentList></Representation></AdaptationSet></Period>";};
  const auto snapshot=[&](std::string body){DashManifestSnapshot result;result.requestUrl=result.url="http://127.0.0.1/owned/manifest.mpd";result.document=std::move(body);result.manifest=parseDashManifest(result.document,token);return result;};
  auto old=snapshot(prefix+period("p0","start=\"PT0S\"")+"</MPD>");std::vector<size_t> representations;for(size_t n=0;n<old.manifest.nodes.size();++n)if(old.manifest.nodes[n].name=="Representation")representations.push_back(n);auto state=createDashLiveState(old,representations,token);
  check(!state.tracks.front().addressing.period.duration,"The old live Period starts without a declared end");
  auto fresh=snapshot(prefix+period("p1","start=\"PT2S\" duration=\"PT1S\"")+"</MPD>");
  const auto refusal=[&](const DashManifestSnapshot& next,std::optional<double> proof,const char* name){bool denied=false;try{planDashPeriodTransition(state,next,token,proof);}catch(const std::runtime_error&){denied=true;}check(denied,name);};
  refusal(fresh,{},"A retired unknown-duration Period cannot advance without retained coverage");
  auto planned=planDashPeriodTransition(state,fresh,token,2);check(planned&&planned->previousEnd==2&&planned->state.tracks.front().periodId=="p1","Exact retained coverage and a fresh explicit successor boundary permit continuation");
  refusal(fresh,1,"A gap between retained media and the successor is rejected");refusal(fresh,3,"Retained overlap beyond the successor is rejected");refusal(fresh,std::numeric_limits<double>::infinity(),"A nonfinite coverage claim is rejected");
  refusal(snapshot(prefix+period("p1","duration=\"PT1S\"")+"</MPD>"),2,"An implicit first Period start cannot invent the missing boundary");
  auto changed=fresh;auto at=changed.document.find("owned-boundary");changed.document.replace(at,14,"other-boundary");changed.manifest=parseDashManifest(changed.document,token);refusal(changed,2,"A different presentation cannot adopt retained media");
  refusal(snapshot(prefix+period("p1","start=\"PT2S\" duration=\"PT1S\"","hvc1.1.6.L93.B0")+"</MPD>"),2,"A successor with incompatible codecs is not joined");
  auto visible=snapshot(prefix+period("p0","start=\"PT0S\"")+period("p1","start=\"PT2S\" duration=\"PT1S\"")+"</MPD>");planned=planDashPeriodTransition(state,visible,token);check(planned&&planned->previousEnd==2,"A still-visible Period retains the ordinary manifest-derived boundary path");
  auto known=state;for(auto& selected:known.tracks)selected.addressing.period.duration=2;planned=planDashPeriodTransition(known,fresh,token);check(planned&&planned->previousEnd==2,"A previously known retired Period requires no inferred coverage argument");
 }catch(const std::exception& e){error=e.what();}
 atomicText(root/L"results.json",Json{{"passed",error.empty()},{"error",error},{"checks",checks}}.dump(2),false);std::cout<<checks.size()<<" checks; "<<error<<"\n";return error.empty()?0:1;
}
