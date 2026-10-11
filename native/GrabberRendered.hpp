#pragma once
#include "Core.hpp"
namespace udm {
// Validate the browser result before a project-bound caller admits references.
// This does not authorize a project, expand its scope, or accept a stale ticket.
inline Json validateRenderedGrabberSnapshot(const Json& input,const std::string& expectedUrl){
 auto fail=[](){throw std::runtime_error("Invalid or stale rendered Grabber page.");};
 auto text=[&](const Json& object,const char* key,size_t limit){if(!object.contains(key)||!object[key].is_string())fail();auto value=object[key].get<std::string>();if(value.size()>limit||value.find('\0')!=std::string::npos)fail();return value;};
 auto web=[&](const std::string& value){if(value.empty()||std::any_of(value.begin(),value.end(),[](unsigned char c){return c<32||c==127;}))fail();Url url(value);if(url.scheme!="http"&&url.scheme!="https")fail();return url.full;};
 if(!input.is_object()||input.dump().size()>3*1024*1024)fail();
 for(auto it=input.begin();it!=input.end();++it)if(std::set<std::string>{"version","url","baseUrl","title","references","truncated","visited","tabId","frameId","documentId"}.count(it.key())==0)fail();
 if(!input.contains("version")||!input["version"].is_number_integer()||num(input,"version")!=1)fail();
 auto address=web(text(input,"url",16000));if(address!=web(expectedUrl))fail();
 auto base=web(text(input,"baseUrl",16000)),title=text(input,"title",4096),document=text(input,"documentId",256);if(document.empty())fail();
 for(const char* key:{"tabId","frameId","visited"})if(!input.contains(key)||!input[key].is_number_integer())fail();
 if(num(input,"tabId")<0||num(input,"tabId")>INT_MAX||num(input,"frameId")!=0||num(input,"visited")<1||num(input,"visited")>10000)fail();
 if(!input.contains("truncated")||!input["truncated"].is_boolean()||!input.contains("references")||!input["references"].is_array()||input["references"].size()>2000)fail();
 Json references=Json::array();std::set<std::pair<std::string,std::string>> seen;size_t bytes=0;
 for(const auto& item:input["references"]){
  if(!item.is_object()||item.size()!=3)fail();auto url=web(text(item,"url",16000)),kind=text(item,"kind",16),description=text(item,"description",4096);
  if(kind!="link"&&kind!="frame"&&kind!="resource")fail();bytes+=url.size()+description.size();if(bytes>2*1024*1024)fail();
  if(seen.emplace(kind,url).second)references.push_back({{"url",url},{"kind",kind},{"description",description}});
 }
 return {{"version",1},{"url",address},{"baseUrl",base},{"title",title},{"documentId",document},{"tabId",input["tabId"]},{"frameId",0},{"visited",input["visited"]},{"truncated",input["truncated"]},{"references",references}};
}
}
