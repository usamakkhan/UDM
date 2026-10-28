#include "MediaStorage.hpp"
#include "MediaPublication.hpp"
#include "BrowserRequest.hpp"
#include "Core.hpp"
#include "OptionsModel.hpp"
#include "WebVtt.hpp"
#include "AdaptiveResources.hpp"
#include "AdaptiveJournal.hpp"
#include "AdaptiveCapture.hpp"
#include "DashPeriods.hpp"
#include "AdaptiveTransferLayout.hpp"
#include "DashPeriodAssembly.hpp"
#include "AudioTrackMetadata.hpp"
#include "LiveHls.hpp"
#include <fstream>
#include <regex>
#include <future>
#include <algorithm>
namespace udm {
void validateAdaptive(const Json& plan){
 if(plan.is_object()&&plan.contains("periods")){dashPeriodPlans(plan);return;}
 if(!plan.is_object()||plan.dump().size()>4ULL*1024*1024||(str(plan,"type")!="hls"&&str(plan,"type")!="dash"))throw std::runtime_error("Invalid or oversized streaming plan.");
 if(!plan.contains("tracks")||!plan["tracks"].is_array()||plan["tracks"].empty()||plan["tracks"].size()>3)throw std::runtime_error("Expected media tracks and an optional subtitle track.");
 if(num(plan,"height")<0||num(plan,"height")>8640||yes(plan,"encrypted"))throw std::runtime_error("Unsupported video dimensions or encrypted media.");
 if(plan.contains("audioOnly")&&!plan["audioOnly"].is_boolean())throw std::runtime_error("Invalid audio-only selection.");
 if(plan.contains("live")&&!plan["live"].is_boolean())throw std::runtime_error("Invalid live recording selection.");
 if(plan.contains("durationMs")&&(str(plan,"type")!="dash"||yes(plan,"live")||!plan["durationMs"].is_number_integer()||num(plan,"durationMs")<=0||num(plan,"durationMs")>7LL*86400*1000))throw std::runtime_error("Invalid DASH presentation duration.");
 if(yes(plan,"live")){
  if(str(plan,"type")!="hls")throw std::runtime_error("Live recording requires HLS playlists.");
  std::set<std::string> playlists;for(const auto& track:plan["tracks"]){auto url=str(track,"playlist");hlsAddress(url,url);if(!playlists.insert(url).second)throw std::runtime_error("Duplicate or invalid live playlist selection.");}
 }
 const bool audioOnly=yes(plan,"audioOnly");auto container=str(plan,"container","mp4");
 if((audioOnly&&(container!="m4a"||num(plan,"height")!=0||!yes(plan,"audioExpected")||plan["tracks"].size()!=1))||(!audioOnly&&container!="mp4"&&container!="ts"))throw std::runtime_error("Unsupported output container or track selection.");
 for(const char* field:{"audioName","audioLanguage"})if(plan.contains(field)&&!plan[field].is_string())throw std::runtime_error("Invalid selected audio description.");
 auto audioName=str(plan,"audioName"),audioLanguage=str(plan,"audioLanguage");
 if(audioName.size()>512||std::any_of(audioName.begin(),audioName.end(),[](unsigned char c){return c<32||c==127;})||audioLanguage.size()>63||(!audioLanguage.empty()&&!std::regex_match(audioLanguage,std::regex("[A-Za-z]{2,8}(-[A-Za-z0-9]{1,8})*"))))throw std::runtime_error("Invalid selected audio description.");
 if((!audioName.empty()||!audioLanguage.empty())&&!yes(plan,"audioExpected"))throw std::runtime_error("A selected audio track must require audio in the output.");
 for(const char* field:{"subtitleName","subtitleLanguage"})if(plan.contains(field)&&!plan[field].is_string())throw std::runtime_error("Invalid subtitle description.");
 auto subtitleName=str(plan,"subtitleName"),subtitleLanguage=str(plan,"subtitleLanguage");
 if(subtitleName.size()>512||std::any_of(subtitleName.begin(),subtitleName.end(),[](unsigned char c){return c<32||c==127;})||subtitleLanguage.size()>63||(!subtitleLanguage.empty()&&!std::regex_match(subtitleLanguage,std::regex("[A-Za-z]{2,8}(-[A-Za-z0-9]{1,8})*"))))throw std::runtime_error("Invalid subtitle description.");
 bool subtitle=false;size_t count=0;
 for(size_t t=0;t<plan["tracks"].size();++t){const auto& track=plan["tracks"][t];
  auto kind=str(track,"kind");bool validKind=t==0?kind==(audioOnly?"audio":"video"):((t==1&&kind=="audio")||(kind=="subtitle"&&!subtitle&&t+1==plan["tracks"].size()&&container=="mp4"));if(kind=="subtitle")subtitle=true;
  if(!validKind||!track.contains("segments")||!track["segments"].is_array()||track["segments"].empty())throw std::runtime_error("Invalid streaming track.");
  if(track.contains("imports")){
   if(str(plan,"type")!="hls"||!track["imports"].is_object()||track["imports"].size()>64)throw std::runtime_error("Invalid HLS playlist imports.");
   for(auto it=track["imports"].begin();it!=track["imports"].end();++it){if(!std::regex_match(it.key(),std::regex("[A-Za-z0-9_-]+"))||!it.value().is_string())throw std::runtime_error("Invalid HLS playlist import.");auto value=it.value().get<std::string>();if(value.size()>16384||std::any_of(value.begin(),value.end(),[](unsigned char c){return c<32||c==127||c=='\"';}))throw std::runtime_error("Invalid HLS playlist import value.");}
  }
  if(track.contains("webVttHeader")&&(!track["webVttHeader"].is_boolean()||kind!="subtitle"||str(plan,"type")!="hls"||yes(plan,"live")||(yes(track,"webVttHeader")&&track["segments"].size()<2)))throw std::runtime_error("Invalid recorded WebVTT initialization selection.");
  bool discontinuous=false;size_t partIndex=0,initializations=0;for(const auto& part:track["segments"]){if(++count>10000)throw std::runtime_error("This video exceeds the current 10000-segment limit.");if(part.contains("timeline")&&(!part["timeline"].is_number()||!std::isfinite(real(part,"timeline"))||real(part,"timeline")<0||real(part,"timeline")>7*86400))throw std::runtime_error("Invalid subtitle segment timeline.");if(part.contains("duration")&&(str(plan,"type")!="hls"||!part["duration"].is_number()||!std::isfinite(real(part,"duration"))||real(part,"duration")<=0||real(part,"duration")>86400))throw std::runtime_error("Invalid HLS segment duration.");if(part.contains("discontinuity")){if(str(plan,"type")!="hls"||yes(plan,"live")||!part["discontinuity"].is_boolean())throw std::runtime_error("Invalid recorded HLS discontinuity.");discontinuous|=yes(part,"discontinuity");}if(part.contains("initialization")){if(str(plan,"type")!="hls"||yes(plan,"live")||!part["initialization"].is_boolean()||!yes(part,"initialization")||part.contains("duration")||part.contains("mapIndex")||part.contains("discontinuity"))throw std::runtime_error("Invalid recorded HLS initialization section.");++initializations;}if(part.contains("mapIndex")){if(str(plan,"type")!="hls"||yes(plan,"live")||!part["mapIndex"].is_number_integer()||num(part,"mapIndex")<0||num(part,"mapIndex")>=(i64)partIndex||!yes(track["segments"][(size_t)num(part,"mapIndex")],"initialization")||part.contains("initialization"))throw std::runtime_error("Invalid recorded HLS initialization reference.");}auto address=str(part,"url");if(address.size()>16384)throw std::runtime_error("Media URL is too long.");Url u(address);if(u.scheme!="http"&&u.scheme!="https")throw std::runtime_error("Streaming requires HTTP or HTTPS.");
   if(part.contains("start")||part.contains("length")){if(!part.contains("start")||!part.contains("length")||!part["start"].is_number_integer()||!part["length"].is_number_integer()||num(part,"start")<0||num(part,"length")<1||num(part,"length")>256LL*1024*1024||num(part,"start")>LLONG_MAX-num(part,"length"))throw std::runtime_error("Invalid streaming byte range.");}
   ++partIndex;
  }
  if(kind=="subtitle"&&initializations>1){
   if(!yes(track,"webVttHeader")||!yes(track["segments"][0],"initialization"))throw std::runtime_error("Changing subtitle headers require an initial WebVTT map.");
   for(const auto& part:track["segments"])if(!yes(part,"initialization")&&!part.contains("mapIndex"))throw std::runtime_error("Changing subtitle headers require a map reference for every cue segment.");
  }
  if(kind=="subtitle"&&discontinuous){
   auto boundaries=[](const Json& segments){std::vector<double> times;double elapsed=0;for(const auto& part:segments){if(yes(part,"initialization"))continue;if(!part.contains("duration"))throw std::runtime_error("Recorded subtitle boundaries require segment durations on both tracks.");if(yes(part,"discontinuity")){if(elapsed<=0)throw std::runtime_error("A recorded subtitle boundary cannot precede the first segment.");times.push_back(elapsed);}elapsed+=real(part,"duration");}return times;};
   auto videoBoundaries=boundaries(plan["tracks"][0]["segments"]),subtitleBoundaries=boundaries(track["segments"]);
   if(videoBoundaries.size()!=subtitleBoundaries.size()||!std::equal(videoBoundaries.begin(),videoBoundaries.end(),subtitleBoundaries.begin(),[](double a,double b){return std::abs(a-b)<=0.5;}))throw std::runtime_error("Recorded subtitle discontinuities must align with the selected media track.");
  }
  if((discontinuous||initializations>1)&&kind!="subtitle")for(const auto& part:track["segments"])if(!yes(part,"initialization")&&!part.contains("duration"))throw std::runtime_error("A recorded HLS playlist boundary requires segment durations.");
 }
 validateAdaptiveResources(plan);
 if(!subtitle&&(!subtitleName.empty()||!subtitleLanguage.empty()))throw std::runtime_error("Subtitle metadata requires a subtitle track.");
}

JobPtr receiveAdaptive(Manager& m,const Json& message){
 auto capture=validateAdaptiveCapture(message);auto page=str(message,"url");auto plan=capture.plan;auto headers=capture.headers;auto cookies=capture.cookies;auto originHeaders=capture.originHeaders;
 if(!fs::exists(appDir()/L"tools"/L"ffmpeg.exe")||!fs::exists(appDir()/L"tools"/L"ffprobe.exe"))throw std::runtime_error("Run setup-media.ps1 to install the local media helpers.");
 Lock lock(m.mutex);auto encoded=plan.dump();
 for(auto job:m.jobs)if(str(job->data,"Url")==page&&!str(job->data,"ProtectedAdaptive").empty()&&(m.isActive(job)||str(job->data,"Status")=="Awaiting confirmation"||str(job->data,"Status")=="Queued")&&reveal(str(job->data,"ProtectedAdaptive"))==encoded)return job;
 auto container=str(plan,"container","mp4");auto name=str(message,"filename","Video");name=std::regex_replace(name,std::regex("\\.(mp4|webm|mkv|ts|m4a)$",std::regex::icase),"")+"."+container;auto job=m.add(page,"",name,"Main queue",true,headers);
 try{job->data["LiveRecording"]=yes(plan,"live");job->data["ProtectedAdaptive"]=protect(encoded);job->data["ProtectedMediaCookies"]=protect(cookies.dump());job->data["ProtectedMediaHeaders"]=protect(originHeaders.dump());job->data["MediaPixelHeight"]=num(plan,"height");{double total=0;bool known=!plan.contains("periods")&&!yes(plan,"live")&&!plan["tracks"].empty();if(known)for(const auto& part:plan["tracks"][0]["segments"]){if(yes(part,"initialization"))continue;if(!part.contains("duration")||!part["duration"].is_number()){known=false;break;}total+=real(part,"duration");}if(str(plan,"type")=="dash"&&num(plan,"durationMs")>0)job->data["MediaLengthMs"]=num(plan,"durationMs");else if(known&&total>0)job->data["MediaLengthMs"]=(i64)std::llround(total*1000);}job->data["FormatDescription"]=std::string(str(plan,"type")=="hls"?"HLS":"DASH")+" to "+(container=="ts"?"TS":container=="m4a"?"M4A audio":"MP4")+" browser capture";if(!str(plan,"audioName").empty())job->data["FormatDescription"]=str(job->data,"FormatDescription")+"; audio: "+str(plan,"audioName")+(str(plan,"audioLanguage").empty()?"":" ("+str(plan,"audioLanguage")+")");if(!str(plan,"subtitleName").empty())job->data["FormatDescription"]=str(job->data,"FormatDescription")+"; subtitles: "+str(plan,"subtitleName")+(str(plan,"subtitleLanguage").empty()?"":" ("+str(plan,"subtitleLanguage")+")");job->data["Status"]=browserOfferStatus(m.state["Settings"]);m.save();return job;}
 catch(...){m.jobs.erase(std::remove(m.jobs.begin(),m.jobs.end(),job),m.jobs.end());m.save();throw;}
}
void adaptiveTransfer(Manager& m,JobPtr job,const std::shared_ptr<Cancel>& cancel){
 Json plan,prefs,cookies,originHeaders;Headers headers;bool refreshPending=false;{Lock lock(m.mutex);refreshPending=yes(job->data,"AdaptiveRefreshPendingValidation");plan=Json::parse(reveal(str(job->data,"ProtectedAdaptive")));cookies=Json::parse(reveal(str(job->data,"ProtectedMediaCookies")));prefs=m.state["Settings"];headers=readHeaders(job->data);auto protectedOrigins=str(job->data,"ProtectedMediaHeaders");originHeaders=protectedOrigins.empty()?Json::object():Json::parse(reveal(protectedOrigins));}
 validateAdaptive(plan);if(yes(plan,"live")){liveHlsTransfer(m,job,cancel);return;}auto session=std::make_shared<HttpSession>(prefs);auto folder=mediaWorkingDirectory(m,job)/L"adaptive";{Lock lock(m.mutex);m.save();}fs::create_directories(folder);fs::create_directories(job->target().parent_path());
 auto layout=adaptiveTransferLayout(plan,folder);auto& parts=layout.parts;auto& tracks=layout.tracks;
 AdaptiveJournal journal(folder,parts.size(),layout.selectionDigest,!layout.multiplePeriods);Json completed=journal.load();
 std::atomic_size_t next{0};auto group=std::make_shared<Cancel>();group->parent=cancel;Rate rate;auto started=std::chrono::steady_clock::now();double priorTransfer,priorElapsed,confirmationSeconds=0;bool networkFinished=false;int workerCount;
 {Lock lock(m.mutex);priorTransfer=real(job->data,"TransferSeconds");priorElapsed=real(job->data,"ElapsedSeconds");job->data["MediaPhase"]="Receiving segments";job->data["Size"]=-1;job->data["Received"]=0;job->data["AdaptiveTotalSegments"]=parts.size();job->data["AdaptiveCompletedSegments"]=0;workerCount=(int)std::min<size_t>(parts.size(),(size_t)std::clamp<i64>(num(job->data,"Connections",8),1,16));job->workers.assign(workerCount,{});}
 std::set<size_t> verified;
 auto work=[&](int worker,bool verifying){try{for(;;){group->check();size_t index=next++;if(index>=parts.size())break;auto& part=parts[index];const auto& track=layout.descriptions[part.track];bool allowEmpty=str(track,"kind")=="subtitle"&&yes(track,"webVttHeader")&&index!=tracks[part.track].front()&&!part.info.contains("start");auto key=std::to_string(index);const auto& resource=part.resource;auto binding=adaptivePartBinding(layout,part);bool cached=false,retained=false;Json receipt;
  bool previouslyVerified=false;
  {Lock lock(m.mutex);if(completed.contains(key)){receipt=completed[key];previouslyVerified=verified.count(index)!=0;}}
  if(receipt.is_object()&&receipt.contains("size")&&receipt["size"].is_number_integer()&&num(receipt,"size")>=(allowEmpty?0:1)&&fs::exists(part.path)&&fs::file_size(part.path)==(uintmax_t)num(receipt,"size")&&fileHash(part.path,[&]{group->check();})==str(receipt,"sha256")){
   cached=true;retained=previouslyVerified||adaptiveCacheMatches(receipt,resource,binding);
  }
  if(verifying&&!cached)continue;
  if(!verifying&&retained){Lock lock(m.mutex);job->data["Received"]=num(job->data,"Received")+(i64)fs::file_size(part.path);job->data["AdaptiveCompletedSegments"]=num(job->data,"AdaptiveCompletedSegments")+1;continue;}
  auto temp=part.path;temp+=verifying?L".refresh.tmp":L".tmp";int retries=m.retries(str(job->data,"Queue"));i64 written=0;
  for(int attempt=0;;++attempt){written=0;try{
   auto requestHeaders=headers;Url address(str(part.info,"url"));if(originHeaders.contains(address.origin)){auto captured=browserHeaders({{"headers",originHeaders[address.origin]}});for(const auto& entry:captured){for(auto it=requestHeaders.begin();it!=requestHeaders.end();)if(lower(it->first)==lower(entry.first))it=requestHeaders.erase(it);else ++it;requestHeaders[entry.first]=entry.second;}}if(cookies.contains(address.origin)&&cookies[address.origin].is_string()&&!cookies[address.origin].get<std::string>().empty())requestHeaders["Cookie"]=cookies[address.origin].get<std::string>();
   std::optional<i64> start,end;if(part.info.contains("start")){start=num(part.info,"start");end=*start+num(part.info,"length")-1;}
   adaptiveConditions(requestHeaders,resource);
   Http response(address.full,requestHeaders,prefs,*group,start,end,"",nullptr,resource.empty(),session);
   checkAdaptiveResource(response,resource,start,end);
   if(response.status<200||response.status>=300)throw std::runtime_error("Streaming server returned HTTP "+std::to_string(response.status)+".");
   if(!response.header(L"Content-Encoding").empty()&&lower(response.header(L"Content-Encoding"))!="identity")throw std::runtime_error("Unexpected encoded streaming response.");
   i64 expected=-1;auto length=response.header(L"Content-Length");if(!length.empty()){if(!std::regex_match(length,std::regex("[0-9]{1,18}")))throw std::runtime_error("Invalid segment size.");expected=std::stoll(length);}
   if(start){std::smatch match;auto cr=response.header(L"Content-Range");if(response.status!=206||!std::regex_match(cr,match,std::regex("bytes ([0-9]+)-([0-9]+)/([0-9]+)"))||std::stoll(match[1])!=*start||std::stoll(match[2])!=*end||std::stoll(match[3])<=*end||(expected>=0&&expected!=*end-*start+1))throw std::runtime_error("Streaming byte range was not honored.");expected=*end-*start+1;}else if(response.status==206)throw std::runtime_error("Unexpected partial streaming segment.");
   if(expected>part.limit)throw std::runtime_error("Streaming segment exceeds its size limit.");
   {Lock lock(m.mutex);job->workers[worker]={worker+1,0,expected>0?expected-1:-1,0,0,(verifying?"Verifying saved segment ":"Segment ")+std::to_string(index+1)+" / "+std::to_string(parts.size())};}
   {std::ofstream out(temp,std::ios::binary|std::ios::trunc);if(!out)throw std::runtime_error("Cannot create streaming part.");char buffer[65536];for(;;){auto n=response.read(buffer,sizeof(buffer),*group);if(!n)break;if(written+(i64)n>part.limit||(expected>=0&&written+(i64)n>expected))throw std::runtime_error("Streaming segment length mismatch.");m.charge(n,*group,rate,job);out.write(buffer,n);if(!out)throw std::runtime_error("Cannot write streaming part.");written+=(i64)n;{Lock lock(m.mutex);if(!verifying)job->data["Received"]=num(job->data,"Received")+(i64)n;job->data["TransferredBytes"]=num(job->data,"TransferredBytes")+(i64)n;auto& row=job->workers[worker];row.position=written;row.received=written;}}out.flush();if(!out)throw std::runtime_error("Cannot flush streaming part.");}
   if((written==0&&!(allowEmpty&&(expected==0||(expected<0&&(trim(response.header(L"Transfer-Encoding")).empty()||lower(trim(response.header(L"Transfer-Encoding")))=="chunked")))&&response.status==200))||(expected>=0&&written!=expected))throw std::runtime_error("Streaming segment was truncated.");group->check();
   if(verifying){
    if(written!=num(receipt,"size")||fileHash(temp,[&]{group->check();})!=str(receipt,"sha256"))throw AdaptiveResourceChanged();
    fs::remove(temp);
    {Lock lock(m.mutex);if(!resource.empty())completed[key]["binding"]=protect(binding);verified.insert(index);job->workers[worker].state="Saved segment verified";}
    break;
   }
   if(!MoveFileExW(temp.c_str(),part.path.c_str(),MOVEFILE_REPLACE_EXISTING|MOVEFILE_WRITE_THROUGH))throw std::runtime_error("Cannot preserve streaming part.");
   auto partHash=fileHash(part.path,[&]{group->check();});
   {Lock lock(m.mutex);Json saved={{"size",written},{"sha256",partHash}};if(!resource.empty())saved["binding"]=protect(binding);journal.commit(index,saved);completed[key]=std::move(saved);job->data["AdaptiveCompletedSegments"]=num(job->data,"AdaptiveCompletedSegments")+1;job->workers[worker].state="Segment complete";}break;
  }catch(const AdaptiveResourceChanged&){std::error_code ec;fs::remove(temp,ec);throw;}catch(...){std::error_code ec;fs::remove(temp,ec);{Lock lock(m.mutex);if(!verifying)job->data["Received"]=std::max<i64>(0,num(job->data,"Received")-written);}group->check();if(attempt>=retries)throw;group->wait(std::min(5000,300*(attempt+1)));}}
 }}catch(...){group->stop=true;throw;}};
 auto phase=[&](bool verifying){next=0;std::vector<std::future<void>> tasks;for(int i=0;i<workerCount;++i)tasks.push_back(std::async(std::launch::async,work,i,verifying));std::exception_ptr error;for(auto& task:tasks)try{task.get();}catch(const Cancelled&){if(!error)error=std::current_exception();}catch(...){error=std::current_exception();}cancel->check();if(error)std::rethrow_exception(error);};
 try{
  if(refreshPending){
   // Finish every retained-byte check before requesting any missing segments.
   // Receipt publication precedes the durable marker: a crash can only repeat
   // verification, never skip it. Failed verification leaves old files intact.
   phase(true);cancel->check();journal.checkpoint(completed);
   {Lock lock(m.mutex);job->data["AdaptiveRefreshPendingValidation"]=false;try{m.save();}catch(...){job->data["AdaptiveRefreshPendingValidation"]=true;throw;}}
  }
  phase(false);journal.checkpoint(completed);
  {Lock lock(m.mutex);job->data["TransferSeconds"]=priorTransfer+std::chrono::duration<double>(std::chrono::steady_clock::now()-started).count();job->data["Status"]="Merging";job->data["MediaPhase"]="Combining streams";networkFinished=true;m.save();}
  std::vector<fs::path> inputs;int audioInput=-1,subtitleInput=-1;const bool audioOnly=yes(plan,"audioOnly");
  if(!layout.multiplePeriods)for(size_t t=0;t<tracks.size();++t){
   auto kind=str(plan["tracks"][t],"kind");bool discontinuous=kind!="subtitle"&&str(plan,"type")=="hls"&&std::any_of(tracks[t].begin(),tracks[t].end(),[&](size_t index){return yes(parts[index].info,"discontinuity");});const bool changingMaps=kind!="subtitle"&&str(plan,"type")=="hls"&&std::count_if(tracks[t].begin(),tracks[t].end(),[&](size_t index){return yes(parts[index].info,"initialization");})>1;discontinuous=discontinuous||changingMaps;auto path=folder/(std::to_wstring(t)+(kind=="subtitle"?L".vtt":discontinuous?L".m3u8":L".bin"));inputs.push_back(path);
   if(kind=="audio")audioInput=(int)t;
   if(kind=="subtitle"){
    subtitleInput=(int)t;double mediaStart=0;const bool subtitleDiscontinuity=std::any_of(tracks[t].begin(),tracks[t].end(),[&](size_t index){return yes(parts[index].info,"discontinuity");});
    struct SubtitleEpoch {double start=0,duration=0,clock=0;std::vector<size_t> media;};std::vector<SubtitleEpoch> epochs;
    if(subtitleDiscontinuity){
     epochs.emplace_back();double elapsed=0;
     for(auto index:tracks[0]){const auto& info=parts[index].info;if(yes(info,"initialization"))continue;if(yes(info,"discontinuity"))epochs.push_back(SubtitleEpoch{elapsed});epochs.back().media.push_back(index);epochs.back().duration+=real(info,"duration");elapsed+=real(info,"duration");}
     for(size_t e=0;e<epochs.size();++e){cancel->check();auto& epoch=epochs[e];double longest=0;for(auto index:epoch.media)longest=std::max(longest,real(parts[index].info,"duration"));std::ostringstream playlist;playlist.imbue(std::locale::classic());playlist<<"#EXTM3U\n#EXT-X-VERSION:7\n#EXT-X-TARGETDURATION:"<<(int)std::ceil(longest)<<"\n#EXT-X-MEDIA-SEQUENCE:0\n";i64 currentMap=-1;
      for(auto index:epoch.media){const auto& part=parts[index];if(part.info.contains("mapIndex")){auto map=num(part.info,"mapIndex");if(map!=currentMap){const auto& header=parts[tracks[0].at((size_t)map)];playlist<<"#EXT-X-MAP:URI=\""<<utf8(header.path.filename().wstring())<<"\"\n";currentMap=map;}}playlist<<"#EXTINF:"<<std::fixed<<std::setprecision(6)<<real(part.info,"duration")<<",\n"<<utf8(part.path.filename().wstring())<<"\n";}playlist<<"#EXT-X-ENDLIST\n";
      auto epochPath=folder/(L"subtitle-epoch-"+std::to_wstring(e)+L".m3u8");atomicText(epochPath,playlist.str());Json timing;try{timing=Json::parse(execute(appDir()/L"tools"/L"ffprobe.exe",{L"-v",L"error",L"-protocol_whitelist",L"file,pipe",L"-allowed_extensions",L"ALL",L"-allowed_segment_extensions",L"ALL",L"-extension_picky",L"0",L"-select_streams",audioOnly?L"a:0":L"v:0",L"-show_entries",L"stream=start_time",L"-of",L"json",epochPath.wstring()},20,*cancel));}catch(...){std::error_code ignored;fs::remove(epochPath,ignored);throw;}std::error_code ignored;fs::remove(epochPath,ignored);
      auto value=timing.contains("streams")&&!timing["streams"].empty()?str(timing["streams"][0],"start_time"):"";size_t used=0;try{epoch.clock=std::stod(value,&used);}catch(...){throw std::runtime_error("Cannot determine a recorded subtitle epoch clock.");}if(used!=value.size()||!std::isfinite(epoch.clock)||std::abs(epoch.clock)>7*86400)throw std::runtime_error("Invalid recorded subtitle epoch clock.");
     }
    }else if(str(plan,"type")=="hls"){
     std::vector<std::wstring> probeArgs={L"-v",L"error",L"-protocol_whitelist",L"file,pipe"};if(inputs[0].extension()==L".m3u8")probeArgs.insert(probeArgs.end(),{L"-allowed_extensions",L"ALL",L"-allowed_segment_extensions",L"ALL",L"-extension_picky",L"0"});probeArgs.insert(probeArgs.end(),{L"-show_entries",L"format=start_time",L"-of",L"json",inputs[0].wstring()});
     auto timing=Json::parse(execute(appDir()/L"tools"/L"ffprobe.exe",probeArgs,20,*cancel));
     auto value=str(timing["format"],"start_time");if(!value.empty()){try{size_t used=0;mediaStart=std::stod(value,&used);if(used!=value.size()||!std::isfinite(mediaStart))throw std::runtime_error("Invalid media clock.");}catch(...){throw std::runtime_error("Cannot determine the subtitle synchronization clock.");}}
    }
    std::vector<SubtitleCue> cues;size_t subtitleBytes=0;
    std::map<size_t,std::string> subtitleHeaders;const bool mapped=yes(plan["tracks"][t],"webVttHeader");
    size_t epochIndex=0;double subtitleElapsed=0;
    for(auto index:tracks[t]){cancel->check();const auto& info=parts[index].info;if(mapped&&(index==tracks[t].front()||yes(info,"initialization")))continue;if(subtitleDiscontinuity&&yes(info,"discontinuity"))++epochIndex;auto text=readText(parts[index].path,2*1024*1024);if(mapped){size_t headerPart=tracks[t].front();if(info.contains("mapIndex"))headerPart=tracks[t].at((size_t)num(info,"mapIndex"));auto [it,inserted]=subtitleHeaders.try_emplace(headerPart);if(inserted)it->second=readText(parts[headerPart].path,2*1024*1024);text=webVttWithHeader(it->second,text);}subtitleBytes+=text.size();if(subtitleBytes>16*1024*1024)throw std::runtime_error("Subtitle input exceeds 16 MB.");
     if(subtitleDiscontinuity){if(epochIndex>=epochs.size())throw std::runtime_error("Recorded subtitle epoch is missing its media boundary.");const auto& epoch=epochs[epochIndex];std::vector<SubtitleCue> segment;appendWebVtt(segment,text,epoch.clock,subtitleElapsed-epoch.start,true);auto offset=(i64)std::llround(epoch.start*1000),end=(i64)std::llround((epoch.start+epoch.duration)*1000);for(auto& cue:segment){cue.start+=offset;cue.end=std::min(end,cue.end+offset);if(cue.end>cue.start){if(cues.size()>=100000)throw std::runtime_error("Subtitle cue limit exceeded.");cues.push_back(std::move(cue));}}subtitleElapsed+=real(info,"duration");}
     else appendWebVtt(cues,text,mediaStart,real(info,"timeline"),str(plan,"type")=="hls");}
    atomicText(path,mergedWebVtt(std::move(cues)));continue;
   }
   if(discontinuous){
    double longest=0;for(auto index:tracks[t])longest=std::max(longest,real(parts[index].info,"duration"));std::ostringstream playlist;playlist.imbue(std::locale::classic());playlist<<"#EXTM3U\n#EXT-X-VERSION:7\n#EXT-X-TARGETDURATION:"<<(int)std::ceil(longest)<<"\n#EXT-X-MEDIA-SEQUENCE:0\n";
    i64 currentMap=-1;for(auto index:tracks[t]){cancel->check();const auto& part=parts[index];if(yes(part.info,"initialization"))continue;if(yes(part.info,"discontinuity"))playlist<<"#EXT-X-DISCONTINUITY\n";if(part.info.contains("mapIndex")){auto map=num(part.info,"mapIndex");if(map!=currentMap){const auto& header=parts[tracks[t].at((size_t)map)];playlist<<"#EXT-X-MAP:URI=\""<<utf8(header.path.filename().wstring())<<"\"\n";currentMap=map;}}playlist<<"#EXTINF:"<<std::fixed<<std::setprecision(6)<<real(part.info,"duration")<<",\n"<<utf8(part.path.filename().wstring())<<"\n";}playlist<<"#EXT-X-ENDLIST\n";atomicText(path,playlist.str());continue;
   }
   std::ofstream out(path,std::ios::binary|std::ios::trunc);for(auto index:tracks[t]){std::ifstream input(parts[index].path,std::ios::binary);if(!input)throw std::runtime_error("A streaming part is missing.");char buffer[65536];while(input){cancel->check();input.read(buffer,sizeof(buffer));out.write(buffer,input.gcount());}if(!input.eof())throw std::runtime_error("Cannot read streaming part.");}out.flush();if(!out)throw std::runtime_error("Cannot assemble streaming track.");
  }
  auto tools=appDir()/L"tools",staging=job->target().parent_path()/(L".udm-"+wide(job->id())+L".adaptive."+wide(str(plan,"container","mp4")));auto mergeStart=std::chrono::steady_clock::now();
  try{std::vector<std::wstring> args={L"-hide_banner",L"-loglevel",L"error",L"-nostdin",L"-y"};
   for(const auto& input:inputs){args.insert(args.end(),{L"-protocol_whitelist",L"file,pipe"});if(input.extension()==L".m3u8")args.insert(args.end(),{L"-allowed_extensions",L"ALL",L"-allowed_segment_extensions",L"ALL",L"-extension_picky",L"0"});args.insert(args.end(),{L"-i",input.wstring()});}
   if(audioOnly)args.insert(args.end(),{L"-map",L"0:a:0"});
   else {args.insert(args.end(),{L"-map",L"0:v:0",L"-map",audioInput>=0?std::to_wstring(audioInput)+L":a:0":L"0:a:0?"});}
   args.insert(args.end(),{L"-c",L"copy"});
   if(subtitleInput>=0){args.insert(args.end(),{L"-map",std::to_wstring(subtitleInput)+L":s:0",L"-c:s",L"mov_text",L"-metadata:s:s:0",L"handler_name="+wide(str(plan,"subtitleName"))});auto language=audioLanguageTag(str(plan,"subtitleLanguage"));if(!language.empty())args.insert(args.end(),{L"-metadata:s:s:0",L"language="+wide(language)});}
   appendAudioMetadata(args,plan);if(str(plan,"container","mp4")=="ts")args.insert(args.end(),{L"-f",L"mpegts"});else args.insert(args.end(),{L"-movflags",L"+faststart"});args.push_back(staging.wstring());if(layout.multiplePeriods){inputs=assembleDashPeriods(layout,folder,staging,plan,*cancel);subtitleInput=std::any_of(layout.descriptions.begin(),layout.descriptions.end(),[](const Json& track){return str(track,"kind")=="subtitle";})?1:-1;}else execute(tools/L"ffmpeg.exe",args,300,*cancel);
   auto probe=Json::parse(execute(tools/L"ffprobe.exe",{L"-v",L"error",L"-show_entries",L"stream=codec_type,height",L"-of",L"json",staging.wstring()},20,*cancel));bool video=false,audio=false,subtitle=false;for(const auto& stream:probe["streams"]){if(str(stream,"codec_type")=="video"){video=true;if(num(plan,"height")>0&&num(stream,"height")!=num(plan,"height"))throw std::runtime_error("Output dimensions differ from the selected quality.");}audio|=str(stream,"codec_type")=="audio";subtitle|=str(stream,"codec_type")=="subtitle";}
   if((audioOnly?video:!video)||(yes(plan,"audioExpected")&&!audio)||(subtitleInput>=0&&!subtitle))throw std::runtime_error("The output does not contain the selected media tracks.");auto digest=fileHash(staging,[&]{cancel->check();});if(!str(job->data,"ExpectedSha256").empty()&&lower(digest)!=lower(str(job->data,"ExpectedSha256")))throw std::runtime_error("SHA-256 verification failed.");cancel->check();
   const auto mergeSeconds=std::chrono::duration<double>(std::chrono::steady_clock::now()-mergeStart).count();
   // Measure the verified muxed output, then retain those exact bytes until Start.
   const auto confirmationStarted=std::chrono::steady_clock::now();
   auto recordConfirmationWait=[&]{confirmationSeconds+=std::chrono::duration<double>(std::chrono::steady_clock::now()-confirmationStarted).count();};
   try{for(;;){cancel->check();bool pending;{Lock lock(m.mutex);pending=yes(job->data,"ConfirmationPending");if(pending){job->data["PrefetchedFinalSize"]=(i64)fs::file_size(staging);job->data["MediaPhase"]="Waiting for confirmation";}}if(!pending)break;cancel->wait(40);}}catch(...){recordConfirmationWait();throw;}recordConfirmationWait();
   publishMediaOutput(m,job,staging,digest,cancel);markZone(job->target());
   {Lock lock(m.mutex);job->data["Sha256"]=digest;job->data["Size"]=fs::file_size(job->target());job->data["Received"]=job->data["Size"];job->data["MergeSeconds"]=mergeSeconds;job->data["Status"]="Complete";job->data["MediaPhase"]="Complete";job->data["Finished"]=date();job->data["Error"]="";m.save();}
  }catch(...){std::error_code ec;if(!fs::exists(m.root/L"publications"/(wide(job->id())+L".json"))&&!fs::exists(m.root/L"replacements"/(wide(job->id())+L".json")))fs::remove(staging,ec);{Lock lock(m.mutex);job->data.erase("PrefetchedFinalSize");}throw;}
  journal.clean();for(auto& part:parts){std::error_code ec;fs::remove(part.path,ec);}for(auto& input:inputs){std::error_code ec;fs::remove(input,ec);}
 }catch(...){Lock lock(m.mutex);job->data["MediaPhase"]=cancel->cancelled()?"Paused":"Failed";job->data.erase("PrefetchedFinalSize");if(!networkFinished)job->data["TransferSeconds"]=priorTransfer+std::chrono::duration<double>(std::chrono::steady_clock::now()-started).count();job->data["ElapsedSeconds"]=priorElapsed+std::max(0.0,std::chrono::duration<double>(std::chrono::steady_clock::now()-started).count()-confirmationSeconds);throw;}
 {Lock lock(m.mutex);job->data["ElapsedSeconds"]=priorElapsed+std::max(0.0,std::chrono::duration<double>(std::chrono::steady_clock::now()-started).count()-confirmationSeconds);}
}
}
