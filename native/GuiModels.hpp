#pragma once
#include "Core.hpp"
#include "MediaSize.hpp"
#include "FindState.hpp"
#include <regex>
#include <algorithm>
namespace udm {
inline std::string subtitleCaptureSummary(const Json& data){
 if(!data.contains("SubtitleCueCount")||!data["SubtitleCueCount"].is_number_integer())return {};
 const auto count=num(data,"SubtitleCueCount",-1);if(count<0||count>100000)return {};
 return count?"Captured subtitle cue records: "+std::to_string(count):"No subtitle cues occurred in the recorded interval.";
}

inline std::string headerValue(const Headers& h,const std::string& name){for(auto& v:h)if(lower(v.first)==lower(name))return v.second;return {};}
inline void setHeader(Headers& h,const std::string& name,const std::string& value){for(auto it=h.begin();it!=h.end();)if(lower(it->first)==lower(name))it=h.erase(it);else ++it;if(!value.empty())h[name]=value;}
inline std::pair<std::string,std::string> basicLogin(const Headers& h){auto value=headerValue(h,"Authorization");if(lower(value.substr(0,6))!="basic ")return {};try{auto decoded=unb64(value.substr(6));std::string plain(decoded.begin(),decoded.end());auto at=plain.find(':');if(at!=std::string::npos)return {plain.substr(0,at),plain.substr(at+1)};}catch(...){}return {};}
inline std::wstring searchFold(const std::string& s,bool sensitive){auto value=wide(s);if(!sensitive&&!value.empty())CharUpperBuffW(value.data(),(DWORD)value.size());return value;}
inline bool searchValue(const std::string& haystack,const std::string& needle,bool sensitive,bool whole){auto a=searchFold(haystack,sensitive),b=searchFold(needle,sensitive);return b.empty()||(whole?a==b:a.find(b)!=std::wstring::npos);}
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
  if(str(plan,"type")=="dash"&&yes(plan,"live"))add("Live DASH manifest",str(plan,"manifest"));
  auto trackName=[](const Json& track){auto kind=str(track,"kind");return kind=="video"||kind=="audio"||kind=="subtitle"?kind:std::string("media");};
  if(yes(plan,"live"))for(const auto& track:plan["tracks"])add("Live "+trackName(track)+" playlist",str(track,"playlist"));
  for(const auto& track:plan["tracks"])if(track.contains("segments")&&!track["segments"].empty())add("First "+trackName(track)+" segment",str(track["segments"][0],"url"));
 }else if(!str(data,"SourceUrl").empty()){
  for(auto name:{"Video","Audio"})if(data.contains(name)&&data[name].is_object())add(std::string(name)+" stream",str(data[name],"Url"));
 }else{
  add("Download address",str(data,"Url"));
  auto resolved=reveal(str(data,"ProtectedResolvedUrl"));if(!resolved.empty()&&resolved!=str(data,"Url"))add("Server address after redirects",resolved);
 }
 return links;
}
class DownloadSearch {
 std::wstring needle;
 bool sensitive=false,whole=false,filename=true,description=false,address=false;
 bool matchesValue(const std::string& text,bool site=false)const{
  const auto value=searchFold(text,sensitive);if(!whole)return value.find(needle)!=std::wstring::npos;
  if(value==needle)return true;if(!site)return false;
  // Reference Find compares the literal site portion after ://, up to /.
  // Preserve authority spelling/port/credentials rather than normalizing URLs.
  const auto scheme=value.find(L"://");if(scheme==std::wstring::npos)return false;
  const auto begin=scheme+3,end=value.find(L'/',begin);return value.compare(begin,end==std::wstring::npos?std::wstring::npos:end-begin,needle)==0;
 }
public:
 explicit DownloadSearch(const Json& query):needle(searchFold(str(query,"Text"),yes(query,"MatchCase"))),sensitive(yes(query,"MatchCase")),whole(yes(query,"WholeString")),filename(yes(query,"FileName",true)),description(yes(query,"Description")),address(yes(query,"Address")){}
 // Caller protects the source while making this owned, scope-specific copy.
 // Decryption, parsing and matching belong outside the manager lock.
 Json snapshot(const Json& source)const{
  Json result=Json::object();auto copy=[&](const char* key){auto found=source.find(key);if(found!=source.end())result[key]=*found;};
  if(filename)copy("FileName");if(description)copy("Description");
  if(address){for(const auto* key:{"Url","DownloadPage","ProtectedHeaders","ProtectedResolvedUrl","ProtectedAdaptive","ProtectedSabr","SourceUrl","MediaOutput"})copy(key);
   for(const auto* key:{"Video","Audio"}){auto found=source.find(key);if(found!=source.end()&&found->is_object()){auto url=found->find("Url");result[key]=Json::object();if(url!=found->end())result[key]["Url"]=*url;}}
  }
  return result;
 }
 bool matches(const Json& data)const{
  if(needle.empty())return false;
  if(filename&&matchesValue(str(data,"FileName")))return true;
  if(description&&matchesValue(str(data,"Description")))return true;
  if(!address)return false;
  if(matchesValue(str(data,"Url"),true)||matchesValue(recoveryPage(data),true)||matchesValue(headerValue(readHeaders(data),"Referer"),true))return true;
  for(const auto& link:downloadLinks(data))if(matchesValue(link.address,true))return true;
  return false;
 }
};
inline bool matchesDownload(const Json& data,const Json& query){return DownloadSearch(query).matches(data);}
// UI-owned filter cache. Only source/encrypted metadata and match booleans are
// retained; decrypted addresses/headers never become cache keys or persisted data.
class DownloadFilterCache {
 struct Entry {Json record;bool matched=false;uint64_t seen=0;};
 Json query;std::map<JobPtr,Entry> entries;uint64_t generation=0;
public:
 void begin(const Json& next){if(query!=next){entries.clear();query=next;}++generation;}
 bool matches(const JobPtr& job,const Json& record,const DownloadSearch& matcher){
  auto found=entries.find(job);
  if(found!=entries.end()&&found->second.record==record){found->second.seen=generation;return found->second.matched;}
  const bool matched=matcher.matches(record);entries[job]=Entry{record,matched,generation};return matched;
 }
 void finish(){for(auto it=entries.begin();it!=entries.end();)if(it->second.seen!=generation)it=entries.erase(it);else ++it;}
};

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
 if(!str(data,"ProtectedBrowserSession").empty()||capturedMedia(data)||str(data,"AuthenticationOrigin").empty())return false;
 Url url(str(data,"Url"));if(url.origin!=str(data,"AuthenticationOrigin"))return false;
 auto scheme=str(data,"AuthenticationScheme");if(scheme=="FTP")return url.scheme=="ftp"&&num(data,str(data,"Status")=="Complete"&&yes(data,"SyncRetryFailed")?"SyncLastFtpStatus":"LastFtpStatus")==530;
 return num(data,str(data,"Status")=="Complete"&&yes(data,"SyncRetryFailed")?"SyncLastHttpStatus":"LastHttpStatus")==401&&(scheme=="Basic"||scheme=="Digest");
}
inline bool authenticationDialogReady(const Json& data){return (str(data,"Status")=="Failed"||(str(data,"Status")=="Complete"&&yes(data,"SyncRetryFailed")))&&yes(data,"AuthenticationPromptPending")&&canRequestLogin(data);}

}
