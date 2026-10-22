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
 for(auto it=input.begin();it!=input.end();++it)if(std::set<std::string>{"version","url","baseUrl","title","references","truncated","visited","tabId","frameId","documentId","html","htmlUnavailable"}.count(it.key())==0)fail();
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
 Json result={{"version",1},{"url",address},{"baseUrl",base},{"title",title},{"documentId",document},{"tabId",input["tabId"]},{"frameId",0},{"visited",input["visited"]},{"truncated",input["truncated"]},{"references",references}};
 if(input.contains("html")||input.contains("htmlUnavailable")){
  if(!input.contains("htmlUnavailable")||!input["htmlUnavailable"].is_boolean())fail();
  const bool unavailable=yes(input,"htmlUnavailable");
  if(unavailable){if(input.contains("html"))fail();}
  else{auto html=text(input,"html",512*1024);if(html.empty())fail();result["html"]=std::move(html);}
  result["htmlUnavailable"]=unavailable;
 }
 return result;
}
// The caller supplies its current pending request, not browser-provided state.
inline Json validateRenderedGrabberReply(const Json& message,const Json& pending,i64 now){
 auto fail=[](){throw std::runtime_error("Rendered Grabber request expired or changed.");};
 if(!message.is_object()||!pending.is_object()||!message.contains("snapshot"))fail();
 for(const char* key:{"projectId","ticket"}){
  if(!message.contains(key)||!message[key].is_string()||!pending.contains(key)||!pending[key].is_string())fail();
  auto expected=pending[key].get<std::string>();if(expected.empty()||expected.size()>128||message[key]!=pending[key])fail();
 }
 if(!message.contains("generation")||!message["generation"].is_number_integer()||!pending.contains("generation")||!pending["generation"].is_number_integer()||num(pending,"generation")<1||message["generation"]!=pending["generation"])fail();
 if(!pending.contains("expires")||!pending["expires"].is_number_integer()||now<0||num(pending,"expires")<=now||num(pending,"expires")-now>15*60*1000)fail();
 if(!pending.contains("url")||!pending["url"].is_string())fail();
 return validateRenderedGrabberSnapshot(message["snapshot"],str(pending,"url"));
}

}
