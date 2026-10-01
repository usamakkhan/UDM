#pragma once
#include "BrowserSession.hpp"
#include "PublicSuffix.hpp"
#include <regex>
namespace udm {
inline std::string cookieDomain(std::string domain){
 domain=lower(domain);if(!domain.empty()&&domain.front()=='.')domain.erase(0,1);return domain;
}
inline bool cookiePathMatches(const std::string& path,const std::string& scope){
 return path==scope||(path.rfind(scope,0)==0&&!scope.empty()&&(scope.back()=='/'||(path.size()>scope.size()&&path[scope.size()]=='/')));
}
inline std::string cookieIdentity(const Json& cookie){
 return cookieDomain(str(cookie,"domain"))+"\n"+str(cookie,"path")+"\n"+str(cookie,"name")+(yes(cookie,"partitioned")?"\npartitioned":"\nordinary");
}
// RFC 6265 date tokens include legacy two-digit years and varied separators.
inline std::optional<double> cookieDate(const std::string& text){
 if(text.size()>1024)return {};std::vector<std::string> tokens;std::string token;
 auto delimiter=[](unsigned char c){return c==9||(c>=0x20&&c<=0x2f)||(c>=0x3b&&c<=0x40)||(c>=0x5b&&c<=0x60)||(c>=0x7b&&c<=0x7e);};
 for(unsigned char c:text){if(delimiter(c)){if(!token.empty()){tokens.push_back(token);token.clear();}}else token+=(char)c;}if(!token.empty())tokens.push_back(token);
 int year=-1,month=-1,day=-1,hour=-1,minute=-1,second=-1;static const std::regex time("^([0-9]{1,2}):([0-9]{1,2}):([0-9]{1,2})([^0-9].*)?$"),date("^([0-9]{1,2})([^0-9].*)?$"),yearPattern("^([0-9]{2,4})([^0-9].*)?$");
 for(const auto& part:tokens){std::smatch match;if(hour<0&&std::regex_match(part,match,time)){hour=std::stoi(match[1]);minute=std::stoi(match[2]);second=std::stoi(match[3]);continue;}
  if(day<0&&std::regex_match(part,match,date)){day=std::stoi(match[1]);continue;}
  if(month<0&&part.size()>=3){auto prefix=lower(part.substr(0,3));std::string months="janfebmaraprmayjunjulaugsepoctnovdec";auto pos=months.find(prefix);if(pos!=std::string::npos&&pos%3==0){month=(int)(pos/3)+1;continue;}}
  if(year<0&&std::regex_match(part,match,yearPattern))year=std::stoi(match[1]);
 }
 if(year>=70&&year<=99)year+=1900;else if(year>=0&&year<=69)year+=2000;
 if(year<1601||year>9999||month<1||day<1||day>31||hour<0||hour>23||minute<0||minute>59||second<0||second>59)return {};
 SYSTEMTIME input{};input.wYear=(WORD)year;input.wMonth=(WORD)month;input.wDay=(WORD)day;input.wHour=(WORD)hour;input.wMinute=(WORD)minute;input.wSecond=(WORD)second;FILETIME ft{};if(!SystemTimeToFileTime(&input,&ft))return {};
 SYSTEMTIME roundTrip{};if(!FileTimeToSystemTime(&ft,&roundTrip)||roundTrip.wMonth!=input.wMonth||roundTrip.wDay!=input.wDay)return {};
 ULARGE_INTEGER ticks{};ticks.LowPart=ft.dwLowDateTime;ticks.HighPart=ft.dwHighDateTime;return std::max(0.0,(double)ticks.QuadPart/10000000.0-11644473600.0);
}
inline std::optional<Json> parseResponseCookie(const std::string& raw,const Url& source,double now){
 if(raw.empty()||raw.size()>12288||std::any_of(raw.begin(),raw.end(),[](unsigned char c){return (c<32&&c!=9)||c==127;}))return {};
 auto semicolon=raw.find(';'),equal=raw.find('=');if(equal==std::string::npos||equal>=semicolon)return {};auto name=trim(raw.substr(0,equal)),value=trim(raw.substr(equal+1,semicolon==std::string::npos?std::string::npos:semicolon-equal-1));if(name.empty())return {};
 auto slash=source.path.find_last_of('/');std::string path=slash==std::string::npos||slash==0?"/":source.path.substr(0,slash),domain=source.host;bool domainAttribute=false,domainSpecified=false,pathAttribute=false,secure=false,partitioned=false,httpOnly=false;std::optional<double> expires,maxAge;
 for(auto at=semicolon;at!=std::string::npos;){auto end=raw.find(';',at+1);auto part=raw.substr(at+1,end==std::string::npos?std::string::npos:end-at-1);auto split=part.find('=');auto key=lower(trim(part.substr(0,split))),attribute=split==std::string::npos?"":trim(part.substr(split+1));
  if(key=="secure")secure=true;else if(key=="httponly")httpOnly=true;else if(key=="partitioned")partitioned=true;
  else if(key=="path"){pathAttribute=!attribute.empty()&&attribute.front()=='/';path=pathAttribute?attribute:(slash==std::string::npos||slash==0?"/":source.path.substr(0,slash));}
  else if(key=="domain"&&!attribute.empty()){domain=cookieDomain(attribute);domainAttribute=true;domainSpecified=true;}
  else if(key=="expires"){auto parsed=cookieDate(attribute);if(parsed)expires=parsed;}
  else if(key=="max-age"&&!attribute.empty()){size_t start=attribute.front()=='-'?1:0;if(start<attribute.size()&&attribute.find_first_not_of("0123456789",start)==std::string::npos){double seconds=0;for(size_t i=start;i<attribute.size();++i)seconds=std::min(253402300799.0,seconds*10+(attribute[i]-'0'));maxAge=start||!seconds?0.0:std::min(253402300799.0,now+seconds);}}
  at=end;
 }
 if(domain.empty()||domain.back()=='.'||!sessionDomainMatches(source.host,domain,!domainAttribute))return {};
 if(domain.find_first_not_of("abcdefghijklmnopqrstuvwxyz0123456789.-:[]")!=std::string::npos)return {};
 if(domainAttribute){bool ip=source.host.find(':')!=std::string::npos||source.host.find_first_not_of("0123456789.")==std::string::npos;if(ip){if(domain!=source.host)return {};domainAttribute=false;}else if(PublicSuffix::installed().domain(domain).empty()){if(domain!=source.host)return {};domainAttribute=false;}}
 if(secure&&source.scheme!="https")return {};if(partitioned&&!secure)return {};
 auto prefix=lower(name);if(prefix.rfind("__secure-",0)==0&&(!secure||source.scheme!="https"))return {};
 if(prefix.rfind("__host-",0)==0&&(!secure||source.scheme!="https"||domainSpecified||!pathAttribute||path!="/"))return {};
 if((prefix.rfind("__http-",0)==0||prefix.rfind("__host-http-",0)==0)&&(!secure||!httpOnly||source.scheme!="https"))return {};
 Json cookie={{"name",name},{"value",value},{"domain",domain},{"path",path},{"hostOnly",!domainAttribute},{"secure",secure}};
 if(partitioned)cookie["partitioned"]=true;if(maxAge)cookie["expirationDate"]=*maxAge;else if(expires)cookie["expirationDate"]=*expires;return cookie;
}
class BrowserCookieJar {
 mutable std::mutex mutex;Json session;std::function<void(const Json&)> save;
public:
 explicit BrowserCookieJar(Json initial,std::function<void(const Json&)> persist={}):session(validateBrowserSession(initial)),save(std::move(persist)){}
 Json snapshot()const{std::lock_guard<std::mutex> lock(mutex);return session;}
 Headers headers(const std::string& address,Headers input)const{std::lock_guard<std::mutex> lock(mutex);return browserSessionHeaders(session,address,std::move(input));}
 void receive(const std::string& address,const std::vector<std::string>& values){
  if(values.empty())return;Url source(address);std::lock_guard<std::mutex> lock(mutex);if(source.origin!=str(session,"Origin")||sessionLogout(session,address))return;
  auto candidate=session;const auto now=epoch()/1000.0;
  {auto& cookies=candidate["Cookies"];for(auto it=cookies.begin();it!=cookies.end();)if(it->contains("expirationDate")&&real(*it,"expirationDate")<=now)it=cookies.erase(it);else ++it;}
  size_t total=0;for(size_t index=0;index<values.size()&&index<512;++index){total+=values[index].size();if(total>98304)break;auto parsed=parseResponseCookie(values[index],source,now);if(!parsed)continue;auto cookie=*parsed;
   // An insecure response must not replace or shadow an existing Secure cookie.
   bool overlay=false;if(source.scheme!="https")for(const auto& old:candidate["Cookies"])if(yes(old,"secure")&&str(old,"name")==str(cookie,"name")&&yes(old,"partitioned")==yes(cookie,"partitioned")&&(sessionDomainMatches(cookieDomain(str(cookie,"domain")),cookieDomain(str(old,"domain")),false)||sessionDomainMatches(cookieDomain(str(old,"domain")),cookieDomain(str(cookie,"domain")),false))&&cookiePathMatches(str(cookie,"path"),str(old,"path"))){overlay=true;break;}if(overlay)continue;
   auto next=candidate;auto& entries=next["Cookies"];auto identity=cookieIdentity(cookie);auto found=std::find_if(entries.begin(),entries.end(),[&](const Json& old){return cookieIdentity(old)==identity;});
   if(cookie.contains("expirationDate")&&real(cookie,"expirationDate")<=now){if(found!=entries.end())entries.erase(found);}
   else if(found==entries.end())entries.push_back(cookie);else *found=cookie;
   try{next=validateBrowserSession(next);}catch(const std::exception&){continue;}candidate=std::move(next);
  }
  if(candidate==session)return;if(save)save(candidate);session=std::move(candidate);
 }
};
}
