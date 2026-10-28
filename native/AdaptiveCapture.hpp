#pragma once
#include "Core.hpp"
#include "BrowserRequest.hpp"
#include "DashPeriods.hpp"
namespace udm {
struct AdaptiveCapture { Json plan,cookies,originHeaders;Headers headers; };
inline AdaptiveCapture validateAdaptiveCapture(const Json& message){
 auto page=str(message,"url");Url source(page);if(source.scheme!="http"&&source.scheme!="https")throw std::runtime_error("Expected the video page URL.");
 if(!message.contains("plan"))throw std::runtime_error("Missing streaming plan.");
 AdaptiveCapture result;result.plan=message["plan"];validateAdaptive(result.plan);
 if(result.plan.contains("periods")){double duration=0;for(const auto& period:dashPeriodPlans(result.plan))duration+=period.duration;result.plan["durationMs"]=(i64)std::llround(duration*1000);}
 for(auto pair:{std::pair<const char*,const char*>{"referrer","Referer"},{"userAgent","User-Agent"}}){auto value=str(message,pair.first);if(!value.empty())result.headers[pair.second]=value;}validateHeaders(result.headers);
 std::set<std::string> origins;auto collect=[&](const Json& tracks){for(const auto& track:tracks){for(const auto& segment:track["segments"])origins.insert(Url(str(segment,"url")).origin);if(yes(result.plan,"live"))origins.insert(Url(str(track,"playlist")).origin);}};
 if(str(result.plan,"type")=="dash"&&yes(result.plan,"live")){origins.insert(Url(str(result.plan,"manifest")).origin);for(const auto& track:result.plan["tracks"])for(const auto& address:track["credentialUrls"])origins.insert(Url(address.get<std::string>()).origin);if(origins.size()>20)throw std::runtime_error("Too many live DASH request origins.");}
 else if(result.plan.contains("periods")){for(const auto& period:result.plan["periods"])collect(period["tracks"]);}else collect(result.plan["tracks"]);
 result.cookies=message.value("originCookies",Json::object());
 if(!result.cookies.is_object()||result.cookies.size()>20)throw std::runtime_error("Invalid media cookie scope.");
 for(auto it=result.cookies.begin();it!=result.cookies.end();++it){Url origin(it.key());if(origin.origin!=it.key()||!origins.count(it.key())||!it.value().is_string()||it.value().get<std::string>().size()>16384)throw std::runtime_error("Invalid media cookie scope.");validateHeaders({{"Cookie",it.value().get<std::string>()}});}
 result.originHeaders=message.value("originHeaders",Json::object());
 if(!result.originHeaders.is_object()||result.originHeaders.size()>20||result.originHeaders.dump().size()>65536)throw std::runtime_error("Invalid media header scope.");
 for(auto it=result.originHeaders.begin();it!=result.originHeaders.end();++it){if(!origins.count(it.key()))throw std::runtime_error("Media headers do not belong to this download.");browserHeaders({{"headers",it.value()}});}
 return result;
}
// Signed URLs and resource validators may change. Track identity, ordering,
// ranges and timelines must not; retained bytes are separately checked on wire.
inline Json adaptiveSelection(Json plan){
 validateAdaptive(plan);if(yes(plan,"live"))throw std::runtime_error("Live recordings cannot refresh their playlist session. Start a new recording.");
 if(plan.contains("periods"))return Json{{"type","dash"},{"periods",dashPeriodRecoveryIdentity(dashPeriodPlans(plan))}};
 plan.erase("resources");plan.erase("resourceVersion");plan.erase("live");plan.erase("encrypted");
 // Declared presentation length is display metadata, not segment identity.
 // Older captures omit it; adding or correcting it must not block URL refresh.
 plan.erase("durationMs");
 plan["container"]=str(plan,"container","mp4");plan["audioOnly"]=yes(plan,"audioOnly");plan["audioExpected"]=yes(plan,"audioExpected");
 for(auto& track:plan["tracks"]){track.erase("playlist");if(track.contains("index")&&track["index"].is_object())track["index"].erase("url");for(auto& segment:track["segments"])segment.erase("url");}
 return plan;
}
inline void validateAdaptiveRefresh(const Json& old,const Json& next){
 if(adaptiveSelection(old)!=adaptiveSelection(next))throw std::runtime_error("The captured quality, audio, subtitles or segment layout changed. Saved data was left unchanged; choose the original selection or start a new download.");
}
}
