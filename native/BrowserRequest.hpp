#pragma once
#include "Core.hpp"
#include "BrowserSession.hpp"
namespace udm {
inline Json browserDownloadSession(const Json& input,const std::string& address,const Headers& headers){
 if(input.empty())return Json::object();auto session=validateBrowserSession(input);
 if(str(session,"Origin")!=Url(address).origin||!str(session,"LogoutPages").empty())throw std::runtime_error("The browser session belongs to another download origin.");
 auto generated=browserSessionHeaders(session,address,Headers{});std::string cookie,agent;
 for(const auto& [name,value]:headers){if(lower(name)=="cookie")cookie=value;if(lower(name)=="user-agent")agent=value;}
 if(generated["Cookie"]!=cookie||generated["User-Agent"]!=agent)throw std::runtime_error("The browser session does not match this download request.");
 return session;
}
inline Headers browserHeaders(const Json& message){
 Headers result;
 if(message.contains("headers")){
  const auto& values=message["headers"];
  if(!values.is_object()||values.size()>7)throw std::runtime_error("Invalid browser request headers.");
  std::set<std::string> seen;size_t total=0;
  for(auto it=values.begin();it!=values.end();++it){
   const auto key=lower(it.key());
   if(!it.value().is_string()||!seen.insert(key).second)throw std::runtime_error("Invalid browser request headers.");
   auto value=it.value().get<std::string>();total+=value.size();
   if(total>32768)throw std::runtime_error("Browser headers exceed the size limit.");
   result[it.key()]=value;
  }
 }
 for(auto pair:{std::pair<const char*,const char*>{"referrer","Referer"},{"cookies","Cookie"},{"userAgent","User-Agent"}}){
  auto value=str(message,pair.first);if(value.empty())continue;
  for(auto it=result.begin();it!=result.end();)if(lower(it->first)==lower(pair.second))it=result.erase(it);else ++it;
  result[pair.second]=value;
 }
 validateHeaders(result);return result;
}
}
