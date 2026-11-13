#pragma once
#include "Core.hpp"
#include "MediaSize.hpp"
#include "FindState.hpp"
#include <regex>
#include <algorithm>
#include <list>
#include <iterator>
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
inline std::vector<DownloadLink> adaptiveDownloadLinks(const Json& plan){
 std::vector<DownloadLink> links;
 const auto add=[&](const std::string& label,std::string address){if(!address.empty())links.push_back({label,std::move(address)});};
 add(str(plan,"type")=="hls"?"HLS playlist":"DASH manifest",str(plan,"manifestUrl"));
 if(str(plan,"type")=="dash"&&yes(plan,"live"))add("Live DASH manifest",str(plan,"manifest"));
 const Json* firstPeriod=nullptr;
 const auto periods=plan.find("periods");
 if(periods!=plan.end()&&periods->is_array()&&!periods->empty())firstPeriod=&periods->front();
 const auto& group=firstPeriod?*firstPeriod:plan;
 const auto tracks=group.find("tracks");
 if(tracks==group.end())return links;
 const auto periodLabel=firstPeriod?" (first captured Period)":std::string{};
 const auto trackName=[](const Json& track){auto kind=str(track,"kind");return kind=="video"||kind=="audio"||kind=="subtitle"?kind:std::string("media");};
 if(yes(plan,"live"))for(const auto& track:*tracks)add("Live "+trackName(track)+" playlist",str(track,"playlist"));
 for(const auto& track:*tracks)if(track.contains("segments")&&!track["segments"].empty())
  add("First "+trackName(track)+" segment"+periodLabel,str(track["segments"][0],"url"));
 return links;
}
inline void saveAdaptiveDisplayAddress(Json& record,const Json& plan){
 const auto links=adaptiveDownloadLinks(plan);
 record["ProtectedAdaptiveDisplayAddress"]=links.empty()?std::string{}:protect(links.front().address);
}
inline std::vector<DownloadLink> downloadLinks(const Json& data){
 std::vector<DownloadLink> links;
 auto add=[&](const std::string& label,const std::string& address){if(address.empty())return;Url parsed(address);links.push_back({label,address});};
 if(!str(data,"ProtectedSabr").empty()){
  auto session=Json::parse(reveal(str(data,"ProtectedSabr")));add((str(data,"MediaOutput")=="audio"?"Audio streaming endpoint (SABR)":"Video and audio streaming endpoint (SABR)"),str(session,"url"));
 }else if(!str(data,"ProtectedAdaptive").empty()){
  auto plan=Json::parse(reveal(str(data,"ProtectedAdaptive")));
  for(const auto& link:adaptiveDownloadLinks(plan))add(link.label,link.address);
 }else if(!str(data,"SourceUrl").empty()){
  for(auto name:{"Video","Audio"})if(data.contains(name)&&data[name].is_object())add(std::string(name)+" stream",str(data[name],"Url"));
 }else{
  add("Download address",str(data,"Url"));
  auto resolved=reveal(str(data,"ProtectedResolvedUrl"));if(!resolved.empty()&&resolved!=str(data,"Url"))add("Server address after redirects",resolved);
 }
 return links;
}
inline std::wstring findSearchNeedle(const Json& query){auto value=findQueryWideText(query);if(!yes(query,"MatchCase")&&!value.empty())CharUpperBuffW(value.data(),static_cast<DWORD>(value.size()));return value;}
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
 explicit DownloadSearch(const Json& query):needle(findSearchNeedle(query)),sensitive(yes(query,"MatchCase")),whole(yes(query,"WholeString")),filename(yes(query,"FileName",true)),description(yes(query,"Description")),address(yes(query,"Address")){}
private:
 template<class Visitor> void visitSnapshot(const Json& source,Visitor&& visit)const{
  const auto field=[&](const char* key){auto found=source.find(key);if(found!=source.end())visit(key,&*found,false);};
  if(filename)field("FileName");if(description)field("Description");
  if(address){
   for(const auto* key:{"Url","DownloadPage","ProtectedHeaders","ProtectedResolvedUrl","ProtectedAdaptive","ProtectedSabr","SourceUrl","MediaOutput"})field(key);
   for(const auto* key:{"Video","Audio"}){
    auto track=source.find(key);if(track==source.end()||!track->is_object())continue;
    auto url=track->find("Url");visit(key,url==track->end()?nullptr:&*url,true);
   }
  }
 }
public:
 // Caller protects the source while taking an owned snapshot or comparing its
 // selected fields. Decryption and matching remain outside the manager lock.
 Json snapshot(const Json& source)const{
  Json result=Json::object();
  visitSnapshot(source,[&](const char* key,const Json* value,bool track){
   if(!track)result[key]=*value;
   else{result[key]=Json::object();if(value)result[key]["Url"]=*value;}
  });
  return result;
 }
 bool snapshotUnchanged(const Json& source,const Json& snapshot)const{
  if(!snapshot.is_object())return false;
  bool same=true;size_t count=0;
  visitSnapshot(source,[&](const char* key,const Json* value,bool track){
   ++count;auto found=snapshot.find(key);if(found==snapshot.end()){same=false;return;}
   if(!track){if(*found!=*value)same=false;}
   else if(!found->is_object()||found->size()!=(value?1u:0u)||(value&&(!found->contains("Url")||found->at("Url")!=*value)))same=false;
  });
  return same&&snapshot.size()==count;
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
 using Order=std::list<JobPtr>;
 struct Entry {Json record;bool matched=false;uint64_t seen=0;size_t cost=0;Order::iterator order;};
 using Entries=std::map<JobPtr,Entry>;
 Json query;Entries entries;Order order;uint64_t generation=0;
 size_t retained=0,byteLimit,entryLimit;
 // A bounded logical cost: JSON values, object key strings and string payloads.
 // This is not an RSS measurement. Stop once the configured budget is exceeded.
 size_t recordCost(const Json& record)const{
  size_t cost=0;
  const auto add=[&](size_t amount){if(amount>byteLimit-cost){cost=byteLimit+1;return false;}cost+=amount;return true;};
  const auto visit=[&](const auto& self,const Json& value)->bool{
   if(!add(sizeof(Json)))return false;
   if(value.is_string())return add(value.get_ref<const std::string&>().size());
   if(value.is_object()){for(auto it=value.begin();it!=value.end();++it)if(!add(sizeof(std::string))||!add(it.key().size())||!self(self,it.value()))return false;}
   else if(value.is_array()){for(const auto& item:value)if(!self(self,item))return false;}
   return true;
  };
  visit(visit,record);return cost;
 }
 void touch(Entry& entry){entry.seen=generation;order.splice(order.end(),order,entry.order);}
 Entries::iterator erase(Entries::iterator entry){retained-=entry->second.cost;order.erase(entry->second.order);return entries.erase(entry);}
public:
 explicit DownloadFilterCache(size_t bytes=32u*1024*1024,size_t count=8192):byteLimit(std::min(bytes,size_t(32u*1024*1024))),entryLimit(std::min(count,size_t(8192))){}
 DownloadFilterCache(const DownloadFilterCache&)=delete;
 DownloadFilterCache& operator=(const DownloadFilterCache&)=delete;
 void begin(const Json& next){if(query!=next){entries.clear();order.clear();retained=0;query=next;}++generation;}
 bool isFor(const Json& expected)const{return query==expected;}
 size_t retainedCost()const{return retained;}
 size_t entryCount()const{return entries.size();}
 // Mark cohort membership without promoting an unused entry in the LRU order.
 // An inactive filter retains only entries for current category/queue candidates.
 void keep(const JobPtr& job){auto found=entries.find(job);if(found!=entries.end())found->second.seen=generation;}
 // UI-owned cache; caller locks the job while comparing the live source.
 // An engaged false result is a cache hit too. Never decrypt under this lock.
 std::optional<bool> cached(const JobPtr& job,const Json& source,const DownloadSearch& matcher){
  auto found=entries.find(job);
  if(found==entries.end()||!matcher.snapshotUnchanged(source,found->second.record))return std::nullopt;
  touch(found->second);return found->second.matched;
 }
 bool matches(const JobPtr& job,Json record,const DownloadSearch& matcher,const Json* expectedQuery=nullptr){
  auto found=entries.find(job);
  if((!expectedQuery||isFor(*expectedQuery))&&found!=entries.end()&&found->second.record==record){touch(found->second);return found->second.matched;}
  const bool matched=matcher.matches(record);
  // Matching may call Windows APIs. A reentrant query change must not publish
  // this result into another query's cache, or reuse its existing opposite result.
  if(expectedQuery&&!isFor(*expectedQuery))return matched;
  found=entries.find(job);if(found!=entries.end())erase(found);
  const auto cost=recordCost(record);
  if(!entryLimit||cost>byteLimit)return matched;
  while(!entries.empty()&&(entries.size()>=entryLimit||cost>byteLimit-retained))erase(entries.find(order.front()));
  order.push_back(job);auto position=std::prev(order.end());
  try{entries.emplace(job,Entry{std::move(record),matched,generation,cost,position});}catch(...){order.pop_back();throw;}
  retained+=cost;return matched;
 }
 void finish(){for(auto it=entries.begin();it!=entries.end();)if(it->second.seen!=generation)it=erase(it);else ++it;}
};

// The caller holds the manager lock while visiting/copying display inputs.
// Views borrow string JSON values; absent/nonstring fields retain str(..., "") semantics.
inline const Json& addressInputString(const Json& record,const char* key){
 static const Json empty="";const auto value=record.find(key);
 return value!=record.end()&&value->is_string()?*value:empty;
}
template<class Visitor> inline void visitDownloadAddressInputs(const Job& job,Visitor&& visit){
 const auto& record=job.data;
 const auto field=[&](const char* key){visit(key,addressInputString(record,key),false);};
 if(hasNonemptyString(record,"ProtectedSabr")){
  if(yes(record,"SabrTrackAddressBound")&&job.audio){
   const auto& endpoint=addressInputString(job.audio->data,"Url");
   const bool audioOnly=addressInputString(record,"MediaOutput")=="audio";
   // Admission and refresh bind this URL to the protected session in the same write.
   if(!endpoint.get_ref<const std::string&>().empty()&&(audioOnly?!job.video:job.video&&addressInputString(job.video->data,"Url")==endpoint)){
    visit("Url",endpoint,false);return;
   }
  }
  field("ProtectedSabr");field("MediaOutput");return;
 }
 if(hasNonemptyString(record,"ProtectedAdaptiveDisplayAddress")){field("ProtectedAdaptiveDisplayAddress");return;}
 if(hasNonemptyString(record,"ProtectedAdaptive")){field("ProtectedAdaptive");return;}
 if(hasNonemptyString(record,"SourceUrl")){
  field("SourceUrl");
  if(job.video&&hasNonemptyString(job.video->data,"Url"))visit("Video",addressInputString(job.video->data,"Url"),true);
  else if(job.audio&&hasNonemptyString(job.audio->data,"Url"))visit("Audio",addressInputString(job.audio->data,"Url"),true);
  return;
 }
 field("Url");if(!hasNonemptyString(record,"Url"))field("ProtectedResolvedUrl");
}
inline Json downloadAddressInputs(const Job& job){
 Json inputs=Json::object();
 visitDownloadAddressInputs(job,[&](const char* key,const Json& value,bool track){
  if(track)inputs[key]={{"Url",value}};else inputs[key]=value;
 });
 return inputs;
}
// Compare against the UI-owned snapshot without cloning large encrypted plans.
// Only changed inputs need an owned copy and address decryption outside the lock.
inline bool downloadAddressInputsUnchanged(const Job& job,const Json& inputs){
 if(!inputs.is_object())return false;
 bool same=true;size_t count=0;
 visitDownloadAddressInputs(job,[&](const char* key,const Json& value,bool track){
  ++count;const auto found=inputs.find(key);
  if(found==inputs.end()){same=false;return;}
  if(track){
   if(!found->is_object()||found->size()!=1||!found->contains("Url")||found->at("Url")!=value)same=false;
  }else if(*found!=value)same=false;
 });
 return same&&count==inputs.size();
}
inline std::string downloadAddress(const Json& data){
 if(!hasNonemptyString(data,"ProtectedSabr")&&hasNonemptyString(data,"ProtectedAdaptiveDisplayAddress")){
  auto address=reveal(str(data,"ProtectedAdaptiveDisplayAddress"));
  if(!address.empty())(void)Url(address);
  return address;
 }
 // Display only the first selected link. The full Links view still resolves
 // and validates every address; an unused redirect/audio link must not add
 // decryption or parsing work to a valid primary display address.
 if(!hasNonemptyString(data,"ProtectedSabr")&&!hasNonemptyString(data,"ProtectedAdaptive")){
  std::string address;
  if(hasNonemptyString(data,"SourceUrl")){
   for(const char* key:{"Video","Audio"}){
    const auto track=data.find(key);
    if(track!=data.end()&&track->is_object()&&hasNonemptyString(*track,"Url")){address=str(*track,"Url");break;}
   }
  }else{
   address=str(data,"Url");if(address.empty())address=reveal(str(data,"ProtectedResolvedUrl"));
  }
  if(!address.empty())(void)Url(address);
  return address;
 }
 const auto links=downloadLinks(data);
 return links.empty()?std::string():links.front().address;
}
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
