#pragma once
#include "Core.hpp"
#include <algorithm>
namespace udm {
inline bool connectionHostGlob(const std::string& host,const std::string& pattern){
 size_t h=0,p=0,star=std::string::npos,retry=0;
 while(h<host.size()){if(p<pattern.size()&&pattern[p]==host[h]){++h;++p;}else if(p<pattern.size()&&pattern[p]=='*'){star=p++;retry=h;}else if(star!=std::string::npos){p=star+1;h=++retry;}else return false;}
 while(p<pattern.size()&&pattern[p]=='*')++p;return p==pattern.size();
}
inline void validateConnectionRule(const Json& rule){
 const auto host=lower(str(rule,"Host")),scheme=str(rule,"Scheme");
 if(host.empty()||host.size()>253||(scheme!=""&&scheme!="http"&&scheme!="https"&&scheme!="ftp")||num(rule,"Connections")<1||num(rule,"Connections")>32)throw std::runtime_error("Use HTTP, HTTPS, FTP or all protocols, a server pattern and 1 to 32 connections.");
 auto probe=host;std::replace(probe.begin(),probe.end(),'*','a');
 if(probe.find_first_of("/?#@\\ \t\r\n")!=std::string::npos||Url("https://"+probe+"/").host!=probe)throw std::runtime_error("Enter a server name; use * as a wildcard. Do not include a path or port.");
}
inline std::string connectionRuleLabel(const Json& rule){auto scheme=str(rule,"Scheme");return (scheme.empty()?"":scheme+"://")+str(rule,"Host");}
inline i64 connectionsForUrl(const Json& settings,const Url& url){
 i64 value=num(settings,"Connections",8);long long best=-1;
 for(const auto& rule:settings.value("ServerConnections",Json::array())){
  const auto host=lower(str(rule,"Host")),scheme=str(rule,"Scheme");if(!scheme.empty()&&scheme!=url.scheme)continue;
  bool wildcard=host.find('*')!=std::string::npos;if(!(wildcard?connectionHostGlob(lower(url.host),host):hostIs(url.host,host)))continue;
  auto literals=host.size()-std::count(host.begin(),host.end(),'*');long long specificity=(long long)literals*4+(!wildcard?2:0)+(!scheme.empty()?1:0);
  if(specificity>=best){best=specificity;value=num(rule,"Connections",8);}
 }return value;
}
}