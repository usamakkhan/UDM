#pragma once
#include "GuiModels.hpp"
namespace udm {
// Keep matching case-sensitive on the path, and reject ambiguous encoded
// separators before attaching a saved password. A server may decode them twice.
inline std::optional<std::string> loginPath(const std::string& input){
 std::string decoded;auto hex=[](char c)->int{if(c>='0'&&c<='9')return c-'0';if(c>='a'&&c<='f')return c-'a'+10;if(c>='A'&&c<='F')return c-'A'+10;return -1;};
 for(size_t i=0;i<input.size();++i){unsigned char c=input[i];if(c=='\\'||c<32||c==127)return {};if(c=='%'){if(i+2>=input.size()||hex(input[i+1])<0||hex(input[i+2])<0)return {};auto v=(unsigned char)(hex(input[i+1])*16+hex(input[i+2]));if(v=='/'||v=='\\'||v=='%'||v<32||v==127)return {};bool plain=(v>='a'&&v<='z')||(v>='A'&&v<='Z')||(v>='0'&&v<='9')||v=='-'||v=='.'||v=='_'||v=='~';if(plain)decoded+=(char)v;else {const char* digits="0123456789ABCDEF";decoded+='%';decoded+=digits[v>>4];decoded+=digits[v&15];}i+=2;}else decoded+=(char)c;}
 if(decoded.empty())decoded="/";if(decoded[0]!='/')return {};
 // Do not guess how a remote server treats dot segments or duplicate slashes.
 size_t at=1;while(at<=decoded.size()){auto end=decoded.find('/',at);auto part=decoded.substr(at,end==std::string::npos?end:end-at);if(part=="."||part==".."||(part.empty()&&end!=std::string::npos))return {};if(end==std::string::npos)break;at=end+1;}
 return decoded;
}
inline std::string siteLoginAddress(const Json& login){return str(login,"Origin")+str(login,"Path","/");}
inline Json makeSiteLogin(const std::string& address,const std::string& user,const std::string& password){
 Url url(trim(address));auto path=loginPath(url.path);if(url.scheme!="https"||!url.query.empty()||address.find('#')!=std::string::npos||!path)throw std::runtime_error("Use an HTTPS site or folder address without a query, fragment or ambiguous path.");
 if(path->back()!='/')*path+='/';Headers checked;setBasicLogin(checked,user,password);
 return {{"Origin",url.origin},{"Path",*path},{"UserName",user},{"ProtectedPassword",protect(password)}};
}
inline void validateSiteLogins(const Json& prefs){
 if(!prefs.contains("SiteLogins"))return;const auto& list=prefs["SiteLogins"];if(!list.is_array()||list.size()>256)throw std::runtime_error("Use at most 256 saved site logins.");std::set<std::string> scopes;
 for(const auto& item:list){auto validated=makeSiteLogin(siteLoginAddress(item),str(item,"UserName"),reveal(str(item,"ProtectedPassword")));if(str(item,"Origin")!=str(validated,"Origin")||str(item,"Path","/")!=str(validated,"Path")||!scopes.insert(siteLoginAddress(validated)).second)throw std::runtime_error("Each saved login needs a unique HTTPS site and folder ending in /.");}
}
inline Headers siteRequestHeaders(const std::string& address,const Headers& original,const Json& prefs,bool sensitive=true){
 Headers headers=original;if(!sensitive){for(auto it=headers.begin();it!=headers.end();){auto key=lower(it->first);if(key=="authorization"||key=="cookie"||key=="referer"||key=="origin")it=headers.erase(it);else ++it;}return headers;}
 for(const auto& h:headers)if(lower(h.first)=="authorization")return headers;
 Url url(address);if(url.scheme!="https"||!prefs.contains("SiteLogins")||!prefs["SiteLogins"].is_array())return headers;auto path=loginPath(url.path);if(!path)return headers;
 const Json* best=nullptr;size_t length=0;for(const auto& login:prefs["SiteLogins"]){auto scope=str(login,"Path","/");if(str(login,"Origin")!=url.origin||scope.empty()||scope.back()!='/'||scope.size()<length)continue;bool matches=path->rfind(scope,0)==0||(*path+"/"==scope);if(matches){best=&login;length=scope.size();}}
 if(best)setBasicLogin(headers,str(*best,"UserName"),reveal(str(*best,"ProtectedPassword")));return headers;
}
}
