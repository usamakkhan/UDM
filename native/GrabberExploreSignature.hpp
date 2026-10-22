#pragma once
#include <string>
namespace udm {
// Rebuild from independent fields: the browser session is not necessarily the
// last component when a captured rendered page also participates in exploration.
inline std::string grabberExploreSignature(const std::string& policy,bool browserLogin,const std::string& session,const std::string& renderedTicket){
 auto result=policy;
 if(browserLogin)result+="|browser:"+session;
 if(!renderedTicket.empty())result+="|rendered:"+renderedTicket;
 return result;
}
}
