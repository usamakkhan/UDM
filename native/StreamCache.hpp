#pragma once
#include "Streaming.hpp"
#include <regex>
namespace udm {
// Binding excludes expiring transport credentials but includes the exact content
// revision and selected tracks. A refreshed session cannot change saved media.
inline Json streamSelection(const Json& offer,bool audioOnly){
 Json binding={{"videoId",str(offer,"videoId")},{"durationMs",num(offer,"durationMs")},{"audioOnly",audioOnly},{"audio",b64(formatIdentity(offer.at("audio")))}};
 if(!audioOnly)binding["video"]=b64(formatIdentity(offer.at("video")));
 return binding;
}
class StreamCache {
 fs::path directory,indexPath;Json index;std::mutex mutex;
 static bool validKey(const std::string& key){return std::regex_match(key,std::regex("(video|audio)-(init|[0-9]{1,10})"));}
 fs::path path(const std::string& key)const{if(!validKey(key))throw std::runtime_error("Invalid saved media segment.");return directory/wide("retained-"+key+".sabr");}
 void save(){auto body=index.dump();if(body.size()>32*1024*1024)throw std::runtime_error("Retained media index exceeded its size limit.");atomicText(indexPath,body);}
 void conflict(){index["conflict"]=true;save();throw std::runtime_error("The server returned different media for the same stream identity. Saved data is retained; start a new browser download.");}
public:
 StreamCache(const fs::path& folder,const Json& offer,bool audioOnly,const Cancel& cancel):directory(folder),indexPath(folder/L"stream-cache.json"){
  const auto binding=streamSelection(offer,audioOnly).dump();
  if(fs::exists(indexPath)){
   index=Json::parse(readText(indexPath));
   if(num(index,"schema")!=1||reveal(str(index,"binding"))!=binding||!index["parts"].is_object()||!index["tracks"].is_object())throw std::runtime_error("Retained media belongs to another stream selection. Start a new download.");
   if(yes(index,"conflict"))throw std::runtime_error("Retained media has a stream-identity conflict. Start a new browser download.");
   if(index["parts"].size()>100000)throw std::runtime_error("Too many retained media segments.");
   bool changed=false;
   for(auto it=index["parts"].begin();it!=index["parts"].end();){
    cancel.check();const auto file=path(it.key());auto row=it.value();bool valid=false;
    try{const auto seq=num(row,"sequence",-2),start=num(row,"start"),length=num(row,"duration");
     valid=seq>=-1&&seq<=INT_MAX&&start>=0&&length>=0&&start<=LLONG_MAX-length&&num(row,"bytes")>0&&num(row,"bytes")<=64*1024*1024&&
      it.key()==str(row,"kind")+"-"+(seq<0?"init":std::to_string(seq))&&fs::is_regular_file(file)&&fs::file_size(file)==(uintmax_t)num(row,"bytes")&&fileHash(file)==str(row,"sha256");
    }catch(const std::exception&){valid=false;}
    if(!valid){it=index["parts"].erase(it);changed=true;}else ++it;
   }
   if(changed)save();
  }else index={{"schema",1},{"binding",protect(binding)},{"tracks",Json::object()},{"parts",Json::object()}};
 }
 Json metadata(const std::string& kind){std::lock_guard<std::mutex> lock(mutex);return index["tracks"].value(kind,Json::object());}
 void metadata(const std::string& kind,i64 duration,i64 endSequence){
  std::lock_guard<std::mutex> lock(mutex);Json value={{"duration",duration},{"endSequence",endSequence}};
  if(index["tracks"].contains(kind)&&index["tracks"][kind]!=value)conflict();
  if(!index["tracks"].contains(kind)){index["tracks"][kind]=value;save();}
 }
 std::map<int,std::pair<Json,fs::path>> parts(const std::string& kind){
  std::lock_guard<std::mutex> lock(mutex);std::map<int,std::pair<Json,fs::path>> result;
  for(auto it=index["parts"].begin();it!=index["parts"].end();++it)if(str(it.value(),"kind")==kind)result[(int)num(it.value(),"sequence")]=std::make_pair(it.value(),path(it.key()));
  return result;
 }
 i64 bytes(const std::string& kind=""){std::lock_guard<std::mutex> lock(mutex);i64 n=0;for(const auto& row:index["parts"])if(kind.empty()||str(row,"kind")==kind)n+=num(row,"bytes");return n;}
 fs::path commit(const std::string& kind,int sequence,i64 start,i64 duration,const fs::path& source){
  const auto key=kind+"-"+(sequence<0?"init":std::to_string(sequence)),hash=fileHash(source);const auto size=fs::file_size(source);
  Json row={{"kind",kind},{"sequence",sequence},{"start",start},{"duration",duration},{"bytes",size},{"sha256",hash}};
  std::lock_guard<std::mutex> lock(mutex);auto destination=path(key);
  if(index["parts"].contains(key)){
   if(index["parts"][key]!=row)conflict();std::error_code ec;fs::remove(source,ec);return destination;
  }
  if(index["parts"].size()>=100000)throw std::runtime_error("Too many retained media segments.");
  if(!MoveFileExW(source.c_str(),destination.c_str(),MOVEFILE_WRITE_THROUGH|MOVEFILE_REPLACE_EXISTING))throw std::runtime_error("Cannot retain the completed media segment (Windows error "+std::to_string(GetLastError())+").");
  index["parts"][key]=row;save();return destination;
 }
};
inline bool verifiedMediaTracks(JobPtr job){
 bool any=false;
 for(auto child:{job->video,job->audio})if(child){
  any=true;try{if(str(child->data,"Status")!="Complete"||!std::regex_match(str(child->data,"Sha256"),std::regex("[a-fA-F0-9]{64}"))||!fs::is_regular_file(child->target())||fs::file_size(child->target())!=(uintmax_t)num(child->data,"Size")||fileHash(child->target())!=str(child->data,"Sha256"))return false;}catch(const std::exception&){return false;}
 }
 return any;
}
inline void cleanStreamCache(const fs::path& folder){
 // Only known UDM cache names are removed, and only after output publication.
 std::error_code ec;if(!fs::is_directory(folder,ec))return;
 for(const auto& entry:fs::directory_iterator(folder,ec)){
  const auto name=utf8(entry.path().filename().wstring());
  if(std::regex_match(name,std::regex("retained-(video|audio)-(init|[0-9]{1,10})\\.sabr|stream-cache\\.json(\\.bak|\\.tmp)?"))&&entry.is_regular_file(ec))fs::remove(entry.path(),ec);
 }
}
}
