#pragma once
#include "HlsPlaylist.hpp"
#include <bcrypt.h>
#include <iomanip>
#include <locale>

namespace udm {
inline Json hlsResourceData(const HlsResource& resource){
 Json data={{"url",resource.url}};if(resource.range){data["start"]=resource.range->start;data["length"]=resource.range->length;}return data;
}
inline HlsResource hlsReadResource(const Json& data){
 if(!data.is_object()||!data.contains("url")||!data["url"].is_string())throw std::runtime_error("Invalid saved HLS resource.");
 HlsResource resource;resource.url=hlsAddress(str(data,"url"),str(data,"url"));
 if(data.contains("start")||data.contains("length")){
  if(!data.contains("start")||!data.contains("length")||!data["start"].is_number_integer()||!data["length"].is_number_integer())throw std::runtime_error("Invalid saved HLS range.");
  resource.range=hlsRange(std::to_string(num(data,"length"))+"@"+std::to_string(num(data,"start")),{},resource.url);
 }return resource;
}
inline Json hlsSegmentData(const HlsSegment& segment){
 auto data=Json{{"media",hlsResourceData(segment.media)},{"sequence",segment.sequence},{"discontinuity",segment.discontinuity},{"duration",segment.duration},{"gap",segment.gap}};
 if(segment.initialization)data["initialization"]=hlsResourceData(*segment.initialization);
 if(segment.programTimeUs)data["programTimeUs"]=*segment.programTimeUs;return data;
}
inline HlsSegment hlsReadSegment(const Json& data){
 if(!data.is_object()||!data.contains("media")||!data.contains("sequence")||!data["sequence"].is_number_integer()||!data.contains("discontinuity")||!data["discontinuity"].is_number_integer()||!data.contains("duration")||!data["duration"].is_number()||!data.contains("gap")||!data["gap"].is_boolean())throw std::runtime_error("Invalid saved HLS segment.");
 HlsSegment segment;segment.media=hlsReadResource(data["media"]);segment.sequence=hlsInteger(std::to_string(num(data,"sequence")));segment.discontinuity=hlsInteger(std::to_string(num(data,"discontinuity")));segment.duration=real(data,"duration");segment.gap=yes(data,"gap");
 if(!std::isfinite(segment.duration)||segment.duration<=0||segment.duration>86400)throw std::runtime_error("Invalid saved HLS duration.");
 if(data.contains("initialization"))segment.initialization=hlsReadResource(data["initialization"]);
 if(data.contains("programTimeUs")){if(!data["programTimeUs"].is_number_integer()||num(data,"programTimeUs")<-86400000000LL||num(data,"programTimeUs")>253402300799999999LL)throw std::runtime_error("Invalid saved HLS clock.");segment.programTimeUs=num(data,"programTimeUs");}
 return segment;
}
inline std::string hlsResourceDigest(const HlsResource& resource){
 auto text=hlsResourceData(resource).dump();BCRYPT_ALG_HANDLE algorithm=nullptr;
 if(BCryptOpenAlgorithmProvider(&algorithm,BCRYPT_SHA256_ALGORITHM,nullptr,0)<0)throw std::runtime_error("Cannot initialize HLS cache hashing.");
 unsigned char digest[32]{};auto result=BCryptHash(algorithm,nullptr,0,(PUCHAR)text.data(),(ULONG)text.size(),digest,sizeof(digest));BCryptCloseAlgorithmProvider(algorithm,0);
 if(result<0)throw std::runtime_error("Cannot hash HLS cache identity.");std::string key;const char* hex="0123456789abcdef";for(auto byte:digest){key+=hex[byte>>4];key+=hex[byte&15];}return key;
}
inline void hlsProtectedWrite(const fs::path& path,const Json& data){atomicText(path,Json{{"Schema",1},{"ProtectedRecording",protect(data.dump())}}.dump());}
inline Json hlsProtectedRead(const fs::path& path){
 auto envelope=Json::parse(readText(path,4*1024*1024));if(num(envelope,"Schema")!=1||str(envelope,"ProtectedRecording").empty())throw std::runtime_error("Invalid HLS recovery receipt.");return Json::parse(reveal(str(envelope,"ProtectedRecording")));
}
// The small header commits playlist growth. Each segment has its own encrypted
// receipt, so completing one segment never rewrites the entire recording.
class HlsRecording {
 struct Track {
  HlsTimeline timeline;
  Json control=Json::object();
  std::map<i64,Json> complete;
  std::map<std::string,Json> initializations;
 };
 fs::path directory;
 std::vector<std::string> sources;
 std::vector<Track> tracks;
 std::optional<size_t> subtitleTrack;
 bool sealed=false;
 mutable std::recursive_mutex mutex;
 static bool matchesFile(const fs::path& path,const Json& receipt,bool allowEmpty=false){
  if(!receipt.is_object()||!receipt.contains("size")||!receipt["size"].is_number_integer()||num(receipt,"size")<(allowEmpty?0:1)||num(receipt,"size")>256LL*1024*1024||!std::regex_match(str(receipt,"sha256"),std::regex("[0-9a-fA-F]{64}")))return false;
  return fs::is_regular_file(path)&&!fs::is_symlink(path)&&fs::file_size(path)==(uintmax_t)num(receipt,"size")&&lower(fileHash(path))==lower(str(receipt,"sha256"));
 }
 fs::path segmentReceipt(size_t track,i64 sequence)const{return directory/(std::to_wstring(track)+L"-"+std::to_wstring(sequence)+L".json");}
 fs::path initializationReceipt(size_t track,const HlsResource& resource)const{return directory/(std::to_wstring(track)+L"-init-"+wide(hlsResourceDigest(resource))+L".json");}
 Json header(const std::optional<std::pair<size_t,Json>>& replacement={},std::optional<bool> nextSealed={})const{
  Json values=Json::array();for(size_t track=0;track<tracks.size();++track)values.push_back(replacement&&replacement->first==track?replacement->second:tracks[track].control);
  return {{"version",1},{"sources",sources},{"tracks",values},{"sealed",nextSealed.value_or(sealed)}};
 }
 const HlsSegment& segment(size_t track,i64 sequence)const{
  if(track>=tracks.size()||tracks[track].timeline.segments.empty())throw std::runtime_error("Unknown HLS recording track.");
  const auto& parts=tracks[track].timeline.segments;auto offset=sequence-parts.front().sequence;
  if(offset<0||(uint64_t)offset>=parts.size())throw std::runtime_error("Unknown HLS recording segment.");return parts[(size_t)offset];
 }
 void load(){
  auto data=hlsProtectedRead(directory/L"recording.json");
  if(num(data,"version")!=1||!data.contains("sources")||data["sources"]!=Json(sources)||!data.contains("tracks")||!data["tracks"].is_array()||data["tracks"].size()!=tracks.size()||!data.contains("sealed")||!data["sealed"].is_boolean())throw std::runtime_error("This HLS recovery data belongs to a different recording.");
  sealed=yes(data,"sealed");
  for(size_t track=0;track<tracks.size();++track){
   auto control=data["tracks"][track];if(!control.is_object())throw std::runtime_error("Invalid HLS recovery track.");tracks[track].control=control;if(control.empty())continue;
   for(const char* field:{"first","last","latestFirst","targetDuration"})if(!control.contains(field)||!control[field].is_number_integer())throw std::runtime_error("Invalid HLS recovery sequence.");
   auto first=num(control,"first"),last=num(control,"last"),latest=num(control,"latestFirst"),target=num(control,"targetDuration");
   if(first<0||last<first-1||last>9007199254740990LL||last-first>=200000||latest<first||latest>last+1||target<1||target>86400)throw std::runtime_error("Invalid HLS recovery bounds.");
   HlsPlaylist all;all.url=sources[track];all.type=str(control,"type");all.targetDuration=target;all.firstSequence=first;all.ended=yes(control,"ended");
   if(all.type!=""&&all.type!="EVENT"&&all.type!="VOD")throw std::runtime_error("Invalid HLS recovery playlist type.");
   for(i64 sequence=first;sequence<=last;++sequence){
    auto receipt=hlsProtectedRead(segmentReceipt(track,sequence));if(!receipt.contains("segment"))throw std::runtime_error("Missing HLS recovery segment.");auto part=hlsReadSegment(receipt["segment"]);
    if(part.sequence!=sequence||std::round(part.duration)>target||(!all.segments.empty()&&part.discontinuity<all.segments.back().discontinuity))throw std::runtime_error("Inconsistent HLS recovery media.");
    all.segments.push_back(part);if(!part.gap&&matchesFile(mediaPath(track,sequence),receipt,subtitleTrack==track&&part.initialization.has_value()&&!part.media.range))tracks[track].complete[sequence]=receipt;
    if(part.initialization){auto key=hlsResourceDigest(*part.initialization);if(!tracks[track].initializations.count(key)&&fs::exists(initializationReceipt(track,*part.initialization))){auto init=hlsProtectedRead(initializationReceipt(track,*part.initialization));if(init.value("resource",Json())==hlsResourceData(*part.initialization)&&matchesFile(initializationPath(track,*part.initialization),init))tracks[track].initializations[key]=init;}}
   }
   // Replay the last advertised window to restore its monotonic-window guard.
   auto initial=all;initial.ended=false;tracks[track].timeline.append(initial);
   all.firstSequence=latest;all.segments.erase(all.segments.begin(),all.segments.begin()+(size_t)std::min<i64>(latest-first,(i64)all.segments.size()));tracks[track].timeline.append(all);
  }
 }
public:
 HlsRecording(fs::path folder,std::vector<std::string> playlists,std::optional<size_t> subtitles={}):directory(fs::absolute(folder)),sources(std::move(playlists)),subtitleTrack(subtitles){
  if(sources.empty()||sources.size()>3||(subtitleTrack&&(*subtitleTrack==0||*subtitleTrack>=sources.size())))throw std::runtime_error("A live recording requires media playlists and an optional subtitle playlist.");
  for(auto& source:sources)source=hlsAddress(source,source);tracks.resize(sources.size());
  fs::create_directories(directory);if(fs::is_symlink(directory)||(GetFileAttributesW(directory.c_str())&FILE_ATTRIBUTE_REPARSE_POINT))throw std::runtime_error("The HLS recovery directory must not be a redirected folder.");
  if(fs::exists(directory/L"recording.json"))load();else hlsProtectedWrite(directory/L"recording.json",header());
 }
 fs::path mediaPath(size_t track,i64 sequence)const{
  if(track>=sources.size()||sequence<0||sequence>9007199254740990LL)throw std::runtime_error("Invalid HLS cache path.");return directory/(std::to_wstring(track)+L"-"+std::to_wstring(sequence)+L".part");
 }
 fs::path initializationPath(size_t track,const HlsResource& resource)const{
  if(track>=sources.size())throw std::runtime_error("Invalid HLS initialization track.");return directory/(std::to_wstring(track)+L"-init-"+wide(hlsResourceDigest(resource))+L".part");
 }
 size_t accept(size_t track,const HlsPlaylist& playlist){
  std::lock_guard<std::recursive_mutex> lock(mutex);if(track>=tracks.size()||playlist.url!=sources[track])throw std::runtime_error("The HLS update belongs to another recording track.");
  if(sealed)throw std::runtime_error("This recording has already been stopped for saving.");
  auto next=tracks[track].timeline;auto before=next.segments.size();auto added=next.append(playlist);
  if(next.segments.size()>200000)throw std::runtime_error("The live recording reached its segment limit. Stop and save the captured portion.");
  if(!tracks[track].control.empty()&&playlist.firstSequence<num(tracks[track].control,"latestFirst"))return added;
  auto control=Json{{"first",next.segments.empty()?playlist.firstSequence:next.segments.front().sequence},{"last",next.segments.empty()?playlist.firstSequence-1:next.segments.back().sequence},{"latestFirst",playlist.firstSequence},{"targetDuration",next.targetDuration},{"type",next.type},{"ended",next.ended}};
  if(control==tracks[track].control&&!added)return 0;
  for(size_t index=before;index<next.segments.size();++index){auto& part=next.segments[index];hlsProtectedWrite(segmentReceipt(track,part.sequence),Json{{"segment",hlsSegmentData(part)}});}
  hlsProtectedWrite(directory/L"recording.json",header(std::pair<size_t,Json>{track,control}));
  tracks[track].timeline=std::move(next);tracks[track].control=std::move(control);return added;
 }
 bool allowsEmptyMedia(size_t track,i64 sequence)const{std::lock_guard<std::recursive_mutex> lock(mutex);const auto& part=segment(track,sequence);return subtitleTrack==track&&part.initialization.has_value()&&!part.media.range;}
 void commitMedia(size_t track,i64 sequence){
  std::lock_guard<std::recursive_mutex> lock(mutex);const auto& part=segment(track,sequence);if(part.gap)throw std::runtime_error("The server marked this live segment as unavailable.");
  auto path=mediaPath(track,sequence);if(!fs::is_regular_file(path)||fs::is_symlink(path)||(!fs::file_size(path)&&!allowsEmptyMedia(track,sequence))||fs::file_size(path)>256ULL*1024*1024)throw std::runtime_error("Invalid recorded HLS segment file.");
  if(part.media.range&&fs::file_size(path)!=(uintmax_t)part.media.range->length)throw std::runtime_error("Recorded HLS range has the wrong byte count.");
  auto receipt=Json{{"segment",hlsSegmentData(part)},{"size",fs::file_size(path)},{"sha256",fileHash(path)}};hlsProtectedWrite(segmentReceipt(track,sequence),receipt);tracks[track].complete[sequence]=std::move(receipt);
 }
 void commitInitialization(size_t track,const HlsResource& resource){
  std::lock_guard<std::recursive_mutex> lock(mutex);if(track>=tracks.size())throw std::runtime_error("Invalid initialization track.");
  bool known=false;for(const auto& part:tracks[track].timeline.segments)known|=part.initialization&&*part.initialization==resource;if(!known)throw std::runtime_error("Initialization section does not belong to this recording.");
  auto path=initializationPath(track,resource);if(!fs::is_regular_file(path)||fs::is_symlink(path)||!fs::file_size(path)||fs::file_size(path)>256ULL*1024*1024||(resource.range&&fs::file_size(path)!=(uintmax_t)resource.range->length))throw std::runtime_error("Invalid recorded HLS initialization file.");
  auto receipt=Json{{"resource",hlsResourceData(resource)},{"size",fs::file_size(path)},{"sha256",fileHash(path)}};hlsProtectedWrite(initializationReceipt(track,resource),receipt);tracks[track].initializations[hlsResourceDigest(resource)]=std::move(receipt);
 }
 bool hasMedia(size_t track,i64 sequence)const{std::lock_guard<std::recursive_mutex> lock(mutex);return track<tracks.size()&&tracks[track].complete.count(sequence)!=0;}
 bool hasInitialization(size_t track,const HlsResource& resource)const{std::lock_guard<std::recursive_mutex> lock(mutex);return track<tracks.size()&&tracks[track].initializations.count(hlsResourceDigest(resource))!=0;}
 std::vector<HlsSegment> timeline(size_t track)const{std::lock_guard<std::recursive_mutex> lock(mutex);if(track>=tracks.size())throw std::runtime_error("Unknown HLS track.");return tracks[track].timeline.segments;}
 bool ended()const{std::lock_guard<std::recursive_mutex> lock(mutex);return std::all_of(tracks.begin(),tracks.end(),[](const Track& track){return track.timeline.ended;});}
 bool trackEnded(size_t track)const{std::lock_guard<std::recursive_mutex> lock(mutex);if(track>=tracks.size())throw std::runtime_error("Unknown HLS track.");return tracks[track].timeline.ended;}
 Json progress()const{
  std::lock_guard<std::recursive_mutex> lock(mutex);i64 bytes=0,complete=0,total=0;double seconds=0;
  for(size_t track=0;track<tracks.size();++track){total+=(i64)tracks[track].timeline.segments.size();complete+=(i64)tracks[track].complete.size();for(const auto& item:tracks[track].complete)bytes+=num(item.second,"size");for(const auto& item:tracks[track].initializations)bytes+=num(item.second,"size");double playable=0;for(const auto& part:ready(track))playable+=part.duration;seconds=track?std::min(seconds,playable):playable;}
  return {{"bytes",bytes},{"complete",complete},{"total",total},{"seconds",seconds}};
 }
 bool isSealed()const{std::lock_guard<std::recursive_mutex> lock(mutex);return sealed;}
 void seal(){std::lock_guard<std::recursive_mutex> lock(mutex);if(sealed)return;hlsProtectedWrite(directory/L"recording.json",header({},true));sealed=true;}
 std::vector<HlsSegment> ready(size_t track)const{
  std::lock_guard<std::recursive_mutex> lock(mutex);if(track>=tracks.size())throw std::runtime_error("Unknown HLS track.");std::vector<HlsSegment> result;
  for(const auto& part:tracks[track].timeline.segments){if(part.gap||!tracks[track].complete.count(part.sequence)||(part.initialization&&!hasInitialization(track,*part.initialization)))break;result.push_back(part);}return result;
 }
 fs::path localPlaylist(size_t track,std::optional<i64> epoch={}){
  std::lock_guard<std::recursive_mutex> lock(mutex);if(!sealed&&!ended())throw std::runtime_error("Stop this recording before assembling it.");auto parts=ready(track);if(epoch)parts.erase(std::remove_if(parts.begin(),parts.end(),[&](const auto& part){return part.discontinuity!=*epoch;}),parts.end());if(parts.empty())throw std::runtime_error("No complete playable prefix has been recorded for the selected track.");
  // Recheck bytes at finalization, without repeatedly hashing retained media
  // during every live playlist poll.
  std::set<std::string> verifiedMaps;
  for(const auto& part:parts){if(!matchesFile(mediaPath(track,part.sequence),tracks[track].complete.at(part.sequence),allowsEmptyMedia(track,part.sequence)))throw std::runtime_error("A recorded HLS segment changed on disk.");if(part.initialization){auto key=hlsResourceDigest(*part.initialization);if(verifiedMaps.insert(key).second&&!matchesFile(initializationPath(track,*part.initialization),tracks[track].initializations.at(key)))throw std::runtime_error("A recorded HLS initialization section changed on disk.");}}
  std::ostringstream out;out.imbue(std::locale::classic());out<<"#EXTM3U\n#EXT-X-VERSION:7\n#EXT-X-TARGETDURATION:"<<tracks[track].timeline.targetDuration<<"\n#EXT-X-MEDIA-SEQUENCE:"<<parts.front().sequence<<"\n#EXT-X-DISCONTINUITY-SEQUENCE:"<<parts.front().discontinuity<<"\n";
  auto discontinuity=parts.front().discontinuity;std::optional<HlsResource> map;
  for(const auto& part:parts){while(discontinuity<part.discontinuity){out<<"#EXT-X-DISCONTINUITY\n";++discontinuity;}if(part.initialization&&!(map==part.initialization)){out<<"#EXT-X-MAP:URI=\""<<utf8(initializationPath(track,*part.initialization).filename().wstring())<<"\"\n";map=part.initialization;}out<<"#EXTINF:"<<std::fixed<<std::setprecision(6)<<part.duration<<",\n"<<utf8(mediaPath(track,part.sequence).filename().wstring())<<"\n";}
  out<<"#EXT-X-ENDLIST\n";auto path=directory/(std::to_wstring(track)+(epoch?L"-epoch-"+std::to_wstring(*epoch):L"")+L".m3u8");atomicText(path,out.str());return path;
 }
};
// Remove only recorder-owned files in this exact directory after publication.
// Unknown files and child directories are never traversed or removed.
inline void cleanLiveHlsCache(const fs::path& folder)noexcept{
 try{
  auto attributes=GetFileAttributesW(folder.c_str());if(attributes==INVALID_FILE_ATTRIBUTES||(attributes&FILE_ATTRIBUTE_REPARSE_POINT)||!(attributes&FILE_ATTRIBUTE_DIRECTORY))return;
  const std::regex owned("recording\\.json(\\.bak|\\.tmp)?|subtitles\\.vtt(\\.bak|\\.tmp)?|[012](-epoch-[0-9]{1,16})?\\.m3u8(\\.bak|\\.tmp)?|[012]-([0-9]{1,16}|init-[0-9a-f]{64})\\.(part(\\.tmp)?|json(\\.bak|\\.tmp)?)");
  std::error_code error;for(fs::directory_iterator it(folder,error),end;!error&&it!=end;it.increment(error)){
   auto path=it->path();auto flags=GetFileAttributesW(path.c_str());if(flags==INVALID_FILE_ATTRIBUTES||(flags&(FILE_ATTRIBUTE_DIRECTORY|FILE_ATTRIBUTE_REPARSE_POINT)))continue;
   if(std::regex_match(utf8(path.filename().wstring()),owned)){std::error_code ignored;fs::remove(path,ignored);}
  }
 }catch(...){}
}
}
