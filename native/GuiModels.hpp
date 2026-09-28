#pragma once
#include "Core.hpp"
#include <regex>
#include <algorithm>
namespace udm {
inline std::string headerValue(const Headers& h,const std::string& name){for(auto& v:h)if(lower(v.first)==lower(name))return v.second;return {};}
inline void setHeader(Headers& h,const std::string& name,const std::string& value){for(auto it=h.begin();it!=h.end();)if(lower(it->first)==lower(name))it=h.erase(it);else ++it;if(!value.empty())h[name]=value;}
inline std::pair<std::string,std::string> basicLogin(const Headers& h){auto value=headerValue(h,"Authorization");if(lower(value.substr(0,6))!="basic ")return {};try{auto decoded=unb64(value.substr(6));std::string plain(decoded.begin(),decoded.end());auto at=plain.find(':');if(at!=std::string::npos)return {plain.substr(0,at),plain.substr(at+1)};}catch(...){}return {};}
inline std::wstring searchFold(std::string s,bool sensitive){auto value=wide(s);if(!sensitive&&!value.empty())CharLowerBuffW(value.data(),(DWORD)value.size());return value;}
inline bool searchValue(const std::string& haystack,const std::string& needle,bool sensitive,bool whole){auto a=searchFold(haystack,sensitive),b=searchFold(needle,sensitive);return b.empty()||(whole?a==b:a.find(b)!=std::wstring::npos);}
inline bool matchesDownload(const Json& d,const Json& query){bool sensitive=yes(query,"MatchCase"),whole=yes(query,"WholeString");auto needle=str(query,"Text");if(needle.empty())return false;bool match=false;if(yes(query,"FileName",true))match|=searchValue(str(d,"FileName"),needle,sensitive,whole);if(yes(query,"Description"))match|=searchValue(str(d,"Description"),needle,sensitive,whole);if(yes(query,"Address")){auto headers=readHeaders(d);match|=searchValue(str(d,"Url"),needle,sensitive,whole)||searchValue(recoveryPage(d),needle,sensitive,whole)||searchValue(headerValue(headers,"Referer"),needle,sensitive,whole);}return match;}
inline void validateFileMetadata(const Json& d){readPostRequest(d);if(!str(d,"DownloadPage").empty()){Url page(str(d,"DownloadPage"));if(page.scheme!="http"&&page.scheme!="https")throw std::runtime_error("The parent page must use HTTP or HTTPS.");}auto h=readHeaders(d);validateHeaders(h);auto referer=headerValue(h,"Referer");if(!referer.empty()){Url page(referer);if(page.scheme!="http"&&page.scheme!="https")throw std::runtime_error("Referer must use HTTP or HTTPS.");}auto expected=str(d,"ExpectedSha256");if(!expected.empty()&&!std::regex_match(expected,std::regex("[a-fA-F0-9]{64}")))throw std::runtime_error("SHA-256 must contain 64 hexadecimal characters.");if(num(d,"Connections",8)<1||num(d,"Connections",8)>32||num(d,"LimitKbps")<0||num(d,"LimitKbps")>1000000)throw std::runtime_error("Use 1 to 32 connections and a speed limit from 0 to 1,000,000 KB/s.");}
inline bool capturedMedia(const Json& data){return !str(data,"SourceUrl").empty()||!str(data,"ProtectedAdaptive").empty();}
struct DownloadLink {std::string label,address;};
inline std::vector<DownloadLink> downloadLinks(const Json& data){
 std::vector<DownloadLink> links;
 auto add=[&](const std::string& label,const std::string& address){if(address.empty())return;Url parsed(address);links.push_back({label,address});};
 if(!str(data,"ProtectedSabr").empty()){
  auto session=Json::parse(reveal(str(data,"ProtectedSabr")));add((str(data,"MediaOutput")=="audio"?"Audio streaming endpoint (SABR)":"Video and audio streaming endpoint (SABR)"),str(session,"url"));
 }else if(!str(data,"ProtectedAdaptive").empty()){
  auto plan=Json::parse(reveal(str(data,"ProtectedAdaptive")));
  if(!str(plan,"manifestUrl").empty())add("Playlist / manifest",str(plan,"manifestUrl"));
  if(yes(plan,"live"))for(const auto& track:plan["tracks"])add(str(track,"kind")=="audio"?"Live audio playlist":"Live video playlist",str(track,"playlist"));
  for(auto& track:plan["tracks"])if(track.contains("segments")&&!track["segments"].empty())add(str(track,"kind")=="audio"?"First audio segment":"First video segment",str(track["segments"][0],"url"));
 }else if(!str(data,"SourceUrl").empty()){
  for(auto name:{"Video","Audio"})if(data.contains(name)&&data[name].is_object())add(std::string(name)+" stream",str(data[name],"Url"));
 }else{
  add("Download address",str(data,"Url"));
  auto resolved=reveal(str(data,"ProtectedResolvedUrl"));if(!resolved.empty()&&resolved!=str(data,"Url"))add("Server address after redirects",resolved);
 }
 return links;
}
inline std::string downloadAddress(const Json& data){auto links=downloadLinks(data);return links.empty()?std::string():links.front().address;}
inline std::string mediaAddressNote(const Json& data){
 if(yes(data,"LiveRecording"))return "UDM refreshes these live playlists while recording. Stop and save assembles the captured portion; expired playlists may prevent further recording.";
 if(!str(data,"ProtectedSabr").empty())return "This streaming endpoint needs the captured browser session; the URL alone is not a standalone file link.";
 if(!str(data,"ProtectedAdaptive").empty())return "This video is assembled from a playlist of segments. Links shows the captured playlist or first segments.";
 if(!str(data,"SourceUrl").empty())return "Video and audio may use separate signed links. Open Links to see both; refresh expired links in the browser.";
 return "";
}
inline void setBasicLogin(Headers& headers,const std::string& user,const std::string& password,bool enabled=true){
 if(!enabled){setHeader(headers,"Authorization","");headers["Authorization"]="";return;}
 if(user.empty()||user.size()>512||user.find_first_of(":\r\n")!=std::string::npos||user.find('\0')!=std::string::npos||password.size()>4096||password.find('\0')!=std::string::npos)throw std::runtime_error("Enter a user name without a colon or control characters and a valid password.");
 auto plain=user+":"+password;setHeader(headers,"Authorization","Basic "+b64(Bytes(plain.begin(),plain.end())));
}
inline bool canRequestLogin(const Json& data){
 if(capturedMedia(data)||num(data,"LastHttpStatus")!=401||str(data,"AuthenticationOrigin").empty())return false;
 auto scheme=str(data,"AuthenticationScheme");return (scheme=="Basic"||scheme=="Digest")&&Url(str(data,"Url")).origin==str(data,"AuthenticationOrigin");
}

}
