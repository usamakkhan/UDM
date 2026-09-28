#pragma once
#include "Core.hpp"
#include <wininet.h>
#include <regex>
namespace udm {
struct AdaptiveResourceChanged:std::runtime_error {
 AdaptiveResourceChanged():std::runtime_error("The streaming file changed or no longer matches its captured index. Refresh the video panel and start a new download."){}
};
inline void validateAdaptiveResources(const Json& plan){
 if(!plan.contains("resources")&&!plan.contains("resourceVersion"))return;
 if(!plan.contains("resourceVersion")||!plan["resourceVersion"].is_number_integer()||num(plan,"resourceVersion")!=1||!plan.contains("resources")||!plan["resources"].is_array()||plan["resources"].empty()||plan["resources"].size()>20)throw std::runtime_error("Unsupported adaptive resource bindings.");
 std::set<std::string> seen;
 for(const auto& r:plan["resources"]){
  if(!r.is_object()||!r.contains("url")||!r["url"].is_string()||!r.contains("size")||!r["size"].is_number_integer()||num(r,"size")<1||num(r,"size")>9007199254740991LL)throw std::runtime_error("Invalid adaptive resource size or URL.");
  auto url=str(r,"url");if(url.size()>16384||!seen.insert(url).second)throw std::runtime_error("Duplicate or oversized adaptive resource URL.");Url address(url);if(address.scheme!="http"&&address.scheme!="https")throw std::runtime_error("Invalid adaptive resource protocol.");
  for(auto it=r.begin();it!=r.end();++it)if(it.key()!="url"&&it.key()!="size"&&it.key()!="etag"&&it.key()!="modified")throw std::runtime_error("Unknown adaptive resource field.");
  if(r.contains("etag")){
   if(!r["etag"].is_string())throw std::runtime_error("Invalid adaptive entity tag.");auto tag=str(r,"etag");
   if(tag.size()<2||tag.size()>1024||tag.front()!='"'||tag.back()!='"'||std::any_of(tag.begin()+1,tag.end()-1,[](unsigned char c){return c<0x21||c==0x22||c==0x7f;}))throw std::runtime_error("Expected a strong adaptive entity tag.");
  }
  if(r.contains("modified")){
   if(r.contains("etag")||!r["modified"].is_string())throw std::runtime_error("Invalid adaptive modification time.");auto date=str(r,"modified");SYSTEMTIME parsed{};
   if(!std::regex_match(date,std::regex("[A-Z][a-z]{2}, [0-9]{2} [A-Z][a-z]{2} [0-9]{4} [0-9]{2}:[0-9]{2}:[0-9]{2} GMT"))||!InternetTimeToSystemTimeA(date.c_str(),&parsed,0))throw std::runtime_error("Invalid adaptive modification time.");
  }
  bool used=false;for(const auto& track:plan["tracks"])for(const auto& part:track["segments"])if(str(part,"url")==url){used=true;if(part.contains("start")&&(num(part,"start")>=num(r,"size")||num(part,"length")>num(r,"size")-num(part,"start")))throw std::runtime_error("Adaptive range exceeds its captured resource.");}
  if(!used)throw std::runtime_error("Adaptive resource is outside the selected media.");
 }
}
inline Json adaptiveResource(const Json& plan,const std::string& url){
 if(plan.contains("resources"))for(const auto& r:plan["resources"])if(str(r,"url")==url)return r;return Json::object();
}
inline bool adaptiveCacheMatches(const Json& record,const Json& resource,const std::string& binding){
 if(resource.empty())return true;
 // Date validators have one-second granularity. Only strong ETags allow cross-session reuse.
 if(str(resource,"etag").empty())return false;
 try{return reveal(str(record,"binding"))==binding;}catch(...){return false;}
}
inline void adaptiveConditions(Headers& headers,const Json& resource){
 if(resource.empty())return;
 for(auto it=headers.begin();it!=headers.end();) {auto name=lower(it->first);if(name=="if-match"||name=="if-unmodified-since"||name=="if-range"||name=="range"||name=="if-none-match"||name=="if-modified-since"||name=="cache-control")it=headers.erase(it);else ++it;}
 if(!str(resource,"etag").empty())headers["If-Match"]=str(resource,"etag");
 else if(!str(resource,"modified").empty())headers["If-Unmodified-Since"]=str(resource,"modified");
 headers["Cache-Control"]="no-cache";
}
inline void checkAdaptiveResource(Http& response,const Json& resource,std::optional<i64> start,std::optional<i64> end){
 if(resource.empty())return;
 if(response.status==412||(response.status>=300&&response.status<400)||response.finalUrl!=Url(str(resource,"url")).full)throw AdaptiveResourceChanged();
 if(response.status<200||response.status>=300)return; // Retain ordinary HTTP failure handling.
 if((resource.contains("etag")&&response.header(L"ETag")!=str(resource,"etag"))||(resource.contains("modified")&&response.header(L"Last-Modified")!=str(resource,"modified")))throw AdaptiveResourceChanged();
 std::smatch match;auto range=response.header(L"Content-Range"),length=response.header(L"Content-Length");
 if(start){
  if(response.status!=206||!std::regex_match(range,match,std::regex("bytes ([0-9]{1,16})-([0-9]{1,16})/([0-9]{1,16})"))||std::stoll(match[1])!=*start||std::stoll(match[2])!=*end||std::stoll(match[3])!=num(resource,"size"))throw AdaptiveResourceChanged();
 }else if(response.status!=200||!std::regex_match(length,std::regex("[0-9]{1,16}"))||std::stoll(length)!=num(resource,"size"))throw AdaptiveResourceChanged();
}
}
