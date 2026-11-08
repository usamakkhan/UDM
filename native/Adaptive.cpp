#include "MediaStorage.hpp"
#include "GuiModels.hpp"
#include "MediaPublication.hpp"
#include "BrowserRequest.hpp"
#include "Core.hpp"
#include "StreamHash.hpp"
#include "OptionsModel.hpp"
#include "WebVtt.hpp"
#include "Mp4WebVtt.hpp"
#include "Mp4WebVttFragments.hpp"
#include "AdaptiveResources.hpp"
#include "ProtocolFallback.hpp"
#include "AdaptiveJournal.hpp"
#include "DashResourceResume.hpp"
#include "AdaptiveCapture.hpp"
#include "DashPeriods.hpp"
#include "AdaptiveTransferLayout.hpp"
#include "Mp4WebVttAssembly.hpp"
#include "DashPeriodAssembly.hpp"
#include "AudioTrackMetadata.hpp"
#include "LiveHls.hpp"
#include "LiveDash.hpp"
#include <fstream>
#include <regex>
#include <future>
#include <algorithm>
namespace udm {
void validateAdaptive(const Json& plan){
 if(str(plan,"type")=="dash"&&yes(plan,"live")){validateLiveDashPlan(plan);return;}
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
  if(track.contains("mp4WebVtt")){
   if(!track["mp4WebVtt"].is_boolean()||!yes(track,"mp4WebVtt")||kind!="subtitle"||str(plan,"type")!="dash"||yes(plan,"live")||track["segments"].size()<2||track.contains("webVttHeader")||!track.contains("clock")||!track["clock"].is_object())throw std::runtime_error("Invalid MP4 WebVTT selection.");
   for(const char* field:{"timescale","presentationTimeOffset"})if(!track["clock"].contains(field)||!track["clock"][field].is_number_integer()||num(track["clock"],field)<(std::string(field)=="timescale"?1:0)||num(track["clock"],field)>9007199254740991LL)throw std::runtime_error("Invalid MP4 WebVTT clock.");
   for(const auto& part:track["segments"])for(const char* field:{"initialization","mapIndex","discontinuity","duration","timeline"})if(part.contains(field))throw std::runtime_error("MP4 WebVTT uses an initial metadata part and fragment clocks.");
  }
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

void requireAdaptiveTools(){
 if(!fs::exists(appDir()/L"tools"/L"ffmpeg.exe")||!fs::exists(appDir()/L"tools"/L"ffprobe.exe"))throw std::runtime_error("Run setup-media.ps1 to install the local media helpers.");
}
std::string adaptiveCaptureName(const std::string& name,const Json& plan){
 return std::regex_replace(name,std::regex("\\.(mp4|webm|mkv|ts|m4a)$",std::regex::icase),"")+"."+str(plan,"container","mp4");
}
void applyAdaptiveCapture(Job& job,const AdaptiveCapture& capture,const std::string& encoded){
 const auto& plan=capture.plan;
 const auto container=str(plan,"container","mp4");
 auto& data=job.data;
 data["LiveRecording"]=yes(plan,"live");
 data["ProtectedAdaptive"]=protect(encoded);
 saveAdaptiveDisplayAddress(data,plan);
 data["ProtectedMediaCookies"]=protect(capture.cookies.dump());
 data["ProtectedMediaHeaders"]=protect(capture.originHeaders.dump());
 data["MediaPixelHeight"]=num(plan,"height");
 if(str(plan,"type")=="dash"&&num(plan,"durationMs")>0)data["MediaLengthMs"]=num(plan,"durationMs");
 else if(!plan.contains("periods")&&!yes(plan,"live")&&!plan["tracks"].empty()){
  double total=0;
  for(const auto& part:plan["tracks"][0]["segments"]){
   if(yes(part,"initialization"))continue;
   if(!part.contains("duration")||!part["duration"].is_number()){total=0;break;}
   total+=real(part,"duration");
  }
  if(total>0)data["MediaLengthMs"]=(i64)std::llround(total*1000);
 }
 std::string description=std::string(str(plan,"type")=="hls"?"HLS":"DASH")+" to "+(container=="ts"?"TS":container=="m4a"?"M4A audio":"MP4")+" browser capture";
 for(const auto* track:{"audio","subtitle"}){
  const auto name=str(plan,(std::string(track)+"Name").c_str()),language=str(plan,(std::string(track)+"Language").c_str());
  if(!name.empty())description+=(std::string(track)=="audio"?"; audio: ":"; subtitles: ")+name+(language.empty()?"":" ("+language+")");
 }
 data["FormatDescription"]=std::move(description);
}
JobPtr receiveAdaptive(Manager& m,const Json& message){
 const auto capture=validateAdaptiveCapture(message);
 const auto page=str(message,"url");
 requireAdaptiveTools();
 Lock lock(m.mutex);const auto encoded=capture.plan.dump();
 for(auto job:m.jobs)if(str(job->data,"Url")==page&&!str(job->data,"ProtectedAdaptive").empty()&&(m.isActive(job)||str(job->data,"Status")=="Awaiting confirmation"||str(job->data,"Status")=="Queued")&&reveal(str(job->data,"ProtectedAdaptive"))==encoded)return job;
 const auto name=adaptiveCaptureName(str(message,"filename","Video"),capture.plan);
 auto job=m.add(page,"",name,"Main queue",true,capture.headers);
 try{applyAdaptiveCapture(*job,capture,encoded);job->data["Status"]=browserOfferStatus(m.state["Settings"]);m.save();return job;}
 catch(...){m.jobs.erase(std::remove(m.jobs.begin(),m.jobs.end(),job),m.jobs.end());m.save();throw;}
}
void adaptiveTransfer(Manager& m,JobPtr job,const std::shared_ptr<Cancel>& cancel){requirePublicationRecovered(m,job);
 Json plan,prefs,cookies,originHeaders;Headers headers;std::string protectedPlan;bool refreshPending=false;{Lock lock(m.mutex);refreshPending=yes(job->data,"AdaptiveRefreshPendingValidation");protectedPlan=str(job->data,"ProtectedAdaptive");plan=Json::parse(reveal(protectedPlan));cookies=Json::parse(reveal(str(job->data,"ProtectedMediaCookies")));prefs=m.state["Settings"];headers=readHeaders(job->data);auto protectedOrigins=str(job->data,"ProtectedMediaHeaders");originHeaders=protectedOrigins.empty()?Json::object():Json::parse(reveal(protectedOrigins));}
 validateAdaptive(plan);
 if(yes(plan,"live")){
  bool connected=false;
  // Both live coordinators call this before sending work to network workers.
  const auto connect=[&](const Cancel& capture){
   capture.check();
   if(!connected){m.ensureConnection(job,capture);connected=true;}
   capture.check();
  };
  if(str(plan,"type")=="dash")liveDashTransfer(m,job,cancel,connect);
  else liveHlsTransfer(m,job,cancel,connect);
  return;
 }
 const auto folder=mediaWorkingDirectory(m,job)/L"adaptive";
 auto layout=adaptiveTransferLayout(plan,folder);auto& parts=layout.parts;auto& tracks=layout.tracks;
 AdaptiveJournal journal(folder,parts.size(),layout.selectionDigest,!layout.multiplePeriods);
 auto cleanParts=[&]{
  journal.clean();
  for(size_t index=0;index<parts.size();++index){
   std::error_code ignored;fs::remove(parts[index].path,ignored);
   auto temp=parts[index].path;temp+=L".tmp";
   const auto flags=GetFileAttributesW(temp.c_str());
   if(flags!=INVALID_FILE_ATTRIBUTES&&!(flags&(FILE_ATTRIBUTE_DIRECTORY|FILE_ATTRIBUTE_REPARSE_POINT)))fs::remove(temp,ignored);
   journal.clearPrefix(index);
  }
 };
 const auto container=str(plan,"container","mp4");
 const auto binding="adaptive:"+container+":"+layout.selectionDigest;
 auto currentBinding=[&](const Job& item){
  if(yes(item.data,"AdaptiveRefreshPendingValidation")||str(item.data,"ProtectedAdaptive")!=protectedPlan)return std::string{};
  return binding;
 };
 try{
  if(publishRetainedMediaOutput(m,job,cancel,currentBinding)){cleanParts();return;}
 }catch(...){Lock lock(m.mutex);job->data["MediaPhase"]=cancel->cancelled()?"Paused":"Failed";job->data.erase("PrefetchedFinalSize");throw;}
 auto session=std::make_shared<HttpSession>(prefs);
 {Lock lock(m.mutex);m.save();}fs::create_directories(folder);fs::create_directories(job->target().parent_path());
 Json completed=journal.load();
 i64 sourceBytes=0;
 std::vector<AdaptiveRangeProgress> ranges;
 bool rangeResume=str(plan,"type")=="dash";
 if(rangeResume)for(const auto& part:parts){
  const auto length=part.info.contains("start")?num(part.info,"length"):num(part.resource,"size");
  if(length<=0||length>part.limit){ranges.clear();sourceBytes=0;rangeResume=false;break;}
  ranges.push_back({sourceBytes,length,0});
  sourceBytes+=length;
  rangeResume=rangeResume&&hasNonemptyString(part.resource,"etag");
 }
 std::atomic_size_t next{0};auto group=std::make_shared<Cancel>();group->parent=cancel;Rate rate;auto started=std::chrono::steady_clock::now();double priorTransfer,priorElapsed,confirmationSeconds=0;bool networkFinished=false;int workerCount;
 {
  Lock lock(m.mutex);
  job->data["AdaptiveSourceBytes"]=sourceBytes;job->data["AdaptiveRangeResume"]=rangeResume;
  job->adaptiveRanges=std::move(ranges);
  priorTransfer=real(job->data,"TransferSeconds");priorElapsed=real(job->data,"ElapsedSeconds");job->data["MediaPhase"]="Receiving segments";job->data["Size"]=-1;job->data["Received"]=0;job->data["AdaptiveTotalSegments"]=parts.size();job->data["AdaptiveCompletedSegments"]=0;workerCount=(int)std::min<size_t>(parts.size(),(size_t)std::clamp<i64>(num(job->data,"Connections",8),1,16));job->workers.assign(workerCount,{});
 }
 std::set<size_t> verified;
 std::once_flag connectionAttempt;
 std::exception_ptr connectionError;
 std::mutex retryMutex;
 std::map<std::string,ULONGLONG> retryNotBefore;
 const auto deferRequests=[&](const std::string& origin,int delay){
  std::lock_guard<std::mutex> lock(retryMutex);
  auto& until=retryNotBefore[origin];
  until=std::max(until,GetTickCount64()+static_cast<ULONGLONG>(delay));
 };
 const auto waitForServer=[&](const std::string& origin,int worker){
  bool showedWait=false;
  for(;;){
   group->check();
   ULONGLONG until=0;
   {
    std::lock_guard<std::mutex> lock(retryMutex);
    const auto retry=retryNotBefore.find(origin);
    if(retry!=retryNotBefore.end())until=retry->second;
   }
   const auto now=GetTickCount64();
   if(now>=until)return;
   if(!showedWait){
    Lock lock(m.mutex);
    job->workers[worker]={worker+1,0,-1,0,0,"Waiting for server"};
    showedWait=true;
   }
   group->wait(static_cast<int>(std::min<ULONGLONG>(250,until-now)));
  }
 };
 auto work=[&](int worker,bool verifying){try{for(;;){group->check();size_t index=next++;if(index>=parts.size())break;auto& part=parts[index];const auto& track=layout.descriptions[part.track];bool allowEmpty=str(track,"kind")=="subtitle"&&yes(track,"webVttHeader")&&index!=tracks[part.track].front()&&!part.info.contains("start");auto key=std::to_string(index);const auto& resource=part.resource;auto binding=adaptivePartBinding(layout,part);bool cached=false,retained=false;Json receipt;
  bool previouslyVerified=false;
  {Lock lock(m.mutex);if(completed.contains(key)){receipt=completed[key];previouslyVerified=verified.count(index)!=0;}}
  if(receipt.is_object()&&receipt.contains("size")&&receipt["size"].is_number_integer()&&num(receipt,"size")>=(allowEmpty?0:1)&&fs::exists(part.path)&&fs::file_size(part.path)==(uintmax_t)num(receipt,"size")&&fileHash(part.path,[&]{group->check();})==str(receipt,"sha256")){
   cached=true;retained=previouslyVerified||adaptiveCacheMatches(receipt,resource,binding);
  }
  if(!verifying&&retained){
   Lock lock(m.mutex);
   const auto length=static_cast<i64>(fs::file_size(part.path));
   job->data["Received"]=num(job->data,"Received")+length;
   if(!job->adaptiveRanges.empty())job->adaptiveRanges[index].received=length;
   job->data["AdaptiveCompletedSegments"]=num(job->data,"AdaptiveCompletedSegments")+1;
   continue;
  }
  const Url address(str(part.info,"url"));
  auto temp=part.path;temp+=L".tmp";int retries=m.retries(str(job->data,"Queue"));
  const bool boundPrefix=!str(resource,"etag").empty();
  const bool canContinue=!verifying&&boundPrefix;
  const bool rangedPart=part.info.contains("start");
  const i64 boundLength=rangedPart?num(part.info,"length"):num(resource,"size");
  const auto prefixBinding=boundPrefix&&(!verifying||!cached)?adaptiveSelectionDigest(binding):std::string{};
  std::optional<AdaptivePrefixReceipt> verifyPrefix;
  DashPrefixHash prefixHash;
  Handle output;
  i64 written=0,checkpointed=0;
  ULONGLONG nextCheckpoint=GetTickCount64()+2000;
  bool ownsTemp=false;
  const auto closeOutput=[&]{if(output){CloseHandle(output.h);output.h=INVALID_HANDLE_VALUE;}};
  const auto discard=[&]{
   if(verifying){written=0;return;}
   closeOutput();
   if(ownsTemp){std::error_code ec;fs::remove(temp,ec);ownsTemp=false;}
   if(canContinue)journal.clearPrefix(index);
   Lock lock(m.mutex);
   job->data["Received"]=std::max<i64>(0,num(job->data,"Received")-written);
   if(!job->adaptiveRanges.empty())job->adaptiveRanges[index].received=0;
   job->workers[worker].receiveStart=-1;
   written=0;checkpointed=0;nextCheckpoint=GetTickCount64()+2000;
  };
  const auto checkpointPrefix=[&]()noexcept{
   if(written<=checkpointed)return checkpointed>0;
   nextCheckpoint=GetTickCount64()+2000;
   try{
    LARGE_INTEGER size{};
    if(!GetFileSizeEx(output.h,&size)||size.QuadPart!=written||!FlushFileBuffers(output.h))return false;
    journal.commitPrefix(index,prefixBinding,written,prefixHash.digest());
    checkpointed=written;
    return true;
   }catch(...){return false;}
  };
  if(!prefixBinding.empty()&&boundLength<=part.limit){
   const auto candidates=journal.loadPrefixes(index,verifying?std::string{}:prefixBinding,boundLength);
   if(!candidates.empty()){
    const AdaptivePrefixReceipt* selected=nullptr;
    try{
     output.h=CreateFileW(temp.c_str(),GENERIC_READ|GENERIC_WRITE,0,nullptr,OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL|FILE_FLAG_OPEN_REPARSE_POINT,nullptr);
     LARGE_INTEGER size{};FILE_ATTRIBUTE_TAG_INFO info{};
     if(!output||!GetFileInformationByHandleEx(output.h,FileAttributeTagInfo,&info,sizeof(info))||
        (info.FileAttributes&(FILE_ATTRIBUTE_DIRECTORY|FILE_ATTRIBUTE_REPARSE_POINT))||
        !GetFileSizeEx(output.h,&size)||size.QuadPart>boundLength)throw std::runtime_error("Saved streaming part changed size or identity.");
     for(const auto& candidate:candidates){
      if(candidate.bytes>size.QuadPart)continue;
      LARGE_INTEGER zero{};
      if(!SetFilePointerEx(output.h,zero,nullptr,FILE_BEGIN))throw std::runtime_error("Cannot seek saved streaming part.");
      prefixHash.reset();
      char buffer[65536];
      for(i64 remaining=candidate.bytes;remaining>0;){
       group->check();
       DWORD read=0;
       if(!ReadFile(output.h,buffer,static_cast<DWORD>(std::min<i64>(remaining,sizeof(buffer))),&read,nullptr)||!read)
        throw std::runtime_error("Cannot read saved streaming part.");
       prefixHash.add(buffer,read);
       remaining-=read;
      }
      if(prefixHash.digest()==candidate.digest){selected=&candidate;break;}
     }
     if(!selected)throw std::runtime_error("Saved streaming part changed content.");
     group->check();
     if(verifying)verifyPrefix=*selected;
     else{
      if(selected->scratch)journal.promotePrefix(index);
      if(size.QuadPart!=selected->bytes&&!SetEndOfFile(output.h))throw std::runtime_error("Cannot trim unverified streaming bytes.");
      Lock lock(m.mutex);
      job->data["Received"]=num(job->data,"Received")+selected->bytes;
      if(!job->adaptiveRanges.empty())job->adaptiveRanges[index].received=selected->bytes;
      checkpointed=written=selected->bytes;ownsTemp=true;
     }
    }catch(const Cancelled&){throw;}
    catch(const std::exception&){
     closeOutput();
     if(selected)throw; // Keep a verified checkpoint if promotion or truncation failed.
     if(!verifying)journal.clearPrefix(index);
    }
   }
  }
  if(verifying&&!cached&&!verifyPrefix)continue;
  std::call_once(connectionAttempt,[&]{try{m.ensureConnection(job,*group);}catch(...){connectionError=std::current_exception();}});
  if(connectionError)std::rethrow_exception(connectionError);
  const auto preservePrefix=[&]{
   if(!canContinue||!output||!((written<boundLength&&checkpointPrefix())||checkpointed>0))return false;
   closeOutput();
   Lock lock(m.mutex);
   job->data["Received"]=std::max<i64>(0,num(job->data,"Received")-(written-checkpointed));
   if(!job->adaptiveRanges.empty())job->adaptiveRanges[index].received=checkpointed;
   auto& row=job->workers[worker];row.position=row.received=checkpointed;row.receiveStart=-1;
   return true;
  };
  for(int attempt=0;;++attempt){
  bool shortBody=false;
  try{
   auto requestHeaders=headers;if(originHeaders.contains(address.origin)){auto captured=browserHeaders({{"headers",originHeaders[address.origin]}});for(const auto& entry:captured){for(auto it=requestHeaders.begin();it!=requestHeaders.end();)if(lower(it->first)==lower(entry.first))it=requestHeaders.erase(it);else ++it;requestHeaders[entry.first]=entry.second;}}if(cookies.contains(address.origin)&&cookies[address.origin].is_string()&&!cookies[address.origin].get<std::string>().empty())requestHeaders["Cookie"]=cookies[address.origin].get<std::string>();
   std::optional<i64> start,end;
   if(verifyPrefix||rangedPart||(!verifying&&written>0)){
    const i64 base=rangedPart?num(part.info,"start"):0;
    start=base+(verifying?0:written);end=base+(verifyPrefix?verifyPrefix->bytes:boundLength)-1;
   }
   adaptiveConditions(requestHeaders,resource);
   waitForServer(address.origin,worker);
   Http response(address.full,requestHeaders,prefs,*group,start,end,"",nullptr,resource.empty(),session);
   // A server can ignore Range for a whole part. Use its full response only
   // after discarding the prefix and validating the captured resource again.
   if(!rangedPart&&!verifying&&written>0&&response.status==200){discard();start.reset();end.reset();}
   checkAdaptiveResource(response,resource,start,end);
   if(response.status<200||response.status>=300)throw HttpRejected(response.status,response.header(L"Retry-After"));
   if(!response.header(L"Content-Encoding").empty()&&lower(response.header(L"Content-Encoding"))!="identity")throw std::runtime_error("Unexpected encoded streaming response.");
   i64 expected=-1;auto length=response.header(L"Content-Length");if(!length.empty()){if(!std::regex_match(length,std::regex("[0-9]{1,18}")))throw std::runtime_error("Invalid segment size.");expected=std::stoll(length);}
   if(start){std::smatch match;auto cr=response.header(L"Content-Range");if(response.status!=206||!std::regex_match(cr,match,std::regex("bytes ([0-9]+)-([0-9]+)/([0-9]+)"))||std::stoll(match[1])!=*start||std::stoll(match[2])!=*end||std::stoll(match[3])<=*end||(expected>=0&&expected!=*end-*start+1))throw std::runtime_error("Streaming byte range was not honored.");expected=*end-*start+1;}else if(response.status==206)throw std::runtime_error("Unexpected partial streaming segment.");
   if(expected>=0)expected+=written;
   if(expected>part.limit)throw std::runtime_error("Streaming segment exceeds its size limit.");
   {
    Lock lock(m.mutex);
    auto& row=job->workers[worker];
    row={worker+1,0,expected>0?expected-1:-1,written,written,(verifying?"Verifying saved segment ":"Segment ")+std::to_string(index+1)+" / "+std::to_string(parts.size())};
    if(!verifying&&!job->adaptiveRanges.empty())row.receiveStart=job->adaptiveRanges[index].start+written;
   }
   std::optional<StreamHash> incoming;
   if(verifying)incoming.emplace();
   else if(!output){
    std::error_code ec;
    fs::remove(temp,ec);
    output.h=CreateFileW(temp.c_str(),GENERIC_WRITE,0,nullptr,CREATE_NEW,FILE_ATTRIBUTE_NORMAL,nullptr);
    if(!output)throw std::runtime_error("Cannot create streaming part.");
    ownsTemp=true;
    if(canContinue)prefixHash.reset();
   }
   char buffer[65536];
   for(;;){
    auto n=response.read(buffer,sizeof(buffer),*group);
    if(!n)break;
    if(written+(i64)n>part.limit||(expected>=0&&written+(i64)n>expected))throw std::runtime_error("Streaming segment length mismatch.");
    m.charge(n,*group,rate,job);
    if(incoming)incoming->update(buffer,n);
    else{
     DWORD saved=0;
     if(!WriteFile(output.h,buffer,static_cast<DWORD>(n),&saved,nullptr)||saved!=n)throw std::runtime_error("Cannot write streaming part.");
     if(canContinue)prefixHash.add(buffer,n);
    }
    written+=(i64)n;
    // Limit durable writes during normal downloads. Pause still commits the
    // current prefix immediately, including for small or slow segments.
    if(canContinue&&written<boundLength&&written-checkpointed>=1024*1024&&GetTickCount64()>=nextCheckpoint)
     checkpointPrefix();
    Lock lock(m.mutex);
    if(!verifying){
     job->data["Received"]=num(job->data,"Received")+(i64)n;
     if(!job->adaptiveRanges.empty())job->adaptiveRanges[index].received=written;
    }
    job->data["TransferredBytes"]=num(job->data,"TransferredBytes")+(i64)n;
    auto& row=job->workers[worker];row.position=written;row.received=written;
   }
   {Lock lock(m.mutex);job->workers[worker].receiveStart=-1;}
   shortBody=expected>=0&&written<expected;
   if((written==0&&!(allowEmpty&&(expected==0||(expected<0&&(trim(response.header(L"Transfer-Encoding")).empty()||lower(trim(response.header(L"Transfer-Encoding")))=="chunked")))&&response.status==200))||(expected>=0&&written!=expected))throw std::runtime_error("Streaming segment was truncated.");
   group->check();
   if(verifying){
    if(verifyPrefix){
     if(written!=verifyPrefix->bytes||incoming->finish()!=verifyPrefix->digest)throw AdaptiveResourceChanged();
     if(verifyPrefix->scratch)journal.promotePrefix(index);
     journal.commitPrefix(index,prefixBinding,written,verifyPrefix->digest);
     {Lock lock(m.mutex);job->workers[worker].state="Saved prefix verified";}
    }else{
     if(written!=num(receipt,"size")||incoming->finish()!=str(receipt,"sha256"))throw AdaptiveResourceChanged();
     {Lock lock(m.mutex);if(!resource.empty())completed[key]["binding"]=protect(binding);verified.insert(index);job->workers[worker].state="Saved segment verified";}
    }
    break;
   }
   closeOutput();
   if(!MoveFileExW(temp.c_str(),part.path.c_str(),MOVEFILE_REPLACE_EXISTING|MOVEFILE_WRITE_THROUGH))throw std::runtime_error("Cannot preserve streaming part.");
   ownsTemp=false;
   if(canContinue)journal.clearPrefix(index);
   auto partHash=fileHash(part.path,[&]{group->check();});
   {Lock lock(m.mutex);Json saved={{"size",written},{"sha256",partHash}};if(!resource.empty())saved["binding"]=protect(binding);journal.commit(index,saved);completed[key]=std::move(saved);job->data["AdaptiveCompletedSegments"]=num(job->data,"AdaptiveCompletedSegments")+1;job->workers[worker].state="Segment complete";}break;
  }catch(const AdaptiveResourceChanged&){
   discard();
   throw;
  }catch(...){
   {Lock lock(m.mutex);job->workers[worker].receiveStart=-1;}
   try{
    group->check();if(attempt>=retries)throw;
    int delay=std::min(5000,300*(attempt+1));
    bool interrupted=shortBody;
    try{throw;}
    catch(const HttpRejected& rejection){
     if(!rejection.retryable())throw;
     delay=rejection.delay(attempt);
     if(rejection.retryAfterMs>0)deferRequests(address.origin,rejection.retryAfterMs);
     interrupted=true;
    }catch(const InternetFailure& error){
     interrupted=error.code==ERROR_WINHTTP_CONNECTION_ERROR||error.code==ERROR_WINHTTP_TIMEOUT||
      error.code==ERROR_WINHTTP_CANNOT_CONNECT||error.code==ERROR_WINHTTP_NAME_NOT_RESOLVED;
    }catch(const CurlBodyDisconnect&){interrupted=true;}
    catch(const CurlResponseDisconnect&){interrupted=true;}
    catch(const std::exception&){}
    if(!canContinue||!interrupted||!output||written<=0||written>=boundLength)discard();
    group->wait(delay);
   }catch(const Cancelled&){
    if(!preservePrefix())discard();
    throw;
   }catch(const HttpRejected& rejection){
    const bool refreshable=rejection.status==401||rejection.status==403||rejection.status==404||rejection.status==410;
    if(!refreshable||!preservePrefix())discard();
    throw;
   }catch(...){discard();throw;}
  }}
 }}catch(...){group->stop=true;throw;}};
 auto phase=[&](bool verifying){
  next=0;
  std::vector<std::future<void>> tasks;
  for(int i=0;i<workerCount;++i)tasks.push_back(std::async(std::launch::async,work,i,verifying));
  std::exception_ptr error,stopped;
  for(auto& task:tasks)try{task.get();}
  catch(const Cancelled&){if(!stopped)stopped=std::current_exception();}
  catch(...){if(!error)error=std::current_exception();}
  cancel->check();
  if(error)std::rethrow_exception(error);
  if(stopped)std::rethrow_exception(stopped);
 };
 try{
  if(refreshPending){
   // Finish every retained-byte check before requesting any missing segments.
   // Receipt publication precedes the durable marker: a crash can only repeat
   // verification, never skip it. Failed verification leaves old files intact.
   phase(true);cancel->check();journal.checkpoint(completed);
   {Lock lock(m.mutex);job->data["AdaptiveRefreshPendingValidation"]=false;try{m.save();}catch(...){job->data["AdaptiveRefreshPendingValidation"]=true;throw;}}
  }
  phase(false);journal.checkpoint(completed);
  {Lock lock(m.mutex);const auto networkEnd=std::chrono::steady_clock::now();job->samplePrefetchRate(networkEnd,true);job->data["TransferSeconds"]=priorTransfer+std::chrono::duration<double>(networkEnd-started).count();job->data["Status"]="Merging";job->data["MediaPhase"]="Combining streams";networkFinished=true;m.save();}
  std::vector<fs::path> inputs;
  std::vector<std::wstring> concatenatedInputs(tracks.size());
  int audioInput=-1,subtitleInput=-1;const bool audioOnly=yes(plan,"audioOnly");
  const bool allowConcatenatedInput=str(plan,"type")=="hls"&&str(job->data,"ExpectedSha256").empty()&&
   std::none_of(layout.descriptions.begin(),layout.descriptions.end(),[](const Json& track){return str(track,"kind")=="subtitle";});
  const auto materializeTrack=[&](size_t t,const fs::path& path){
   std::ofstream out(path,std::ios::binary|std::ios::trunc);
   for(auto index:tracks[t]){
    std::ifstream input(parts[index].path,std::ios::binary);
    if(!input)throw std::runtime_error("A streaming part is missing.");
    char buffer[65536];
    while(input){cancel->check();input.read(buffer,sizeof(buffer));out.write(buffer,input.gcount());}
    if(!input.eof())throw std::runtime_error("Cannot read streaming part.");
   }
   out.flush();if(!out)throw std::runtime_error("Cannot assemble streaming track.");
  };
  if(!layout.multiplePeriods)for(size_t t=0;t<tracks.size();++t){
   auto kind=str(plan["tracks"][t],"kind");bool discontinuous=kind!="subtitle"&&str(plan,"type")=="hls"&&std::any_of(tracks[t].begin(),tracks[t].end(),[&](size_t index){return yes(parts[index].info,"discontinuity");});const bool changingMaps=kind!="subtitle"&&str(plan,"type")=="hls"&&std::count_if(tracks[t].begin(),tracks[t].end(),[&](size_t index){return yes(parts[index].info,"initialization");})>1;discontinuous=discontinuous||changingMaps;auto path=folder/(std::to_wstring(t)+(kind=="subtitle"?L".vtt":discontinuous?L".m3u8":L".bin"));inputs.push_back(path);
   if(kind=="audio")audioInput=(int)t;
   if(kind=="subtitle"){
    if(yes(plan["tracks"][t],"mp4WebVtt")){subtitleInput=(int)t;auto captions=assembleMp4WebVttTrack(layout,t,*cancel);atomicText(path,mergedWebVtt(std::move(captions.cues),captions.header));continue;}
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
    std::vector<SubtitleCue> cues;size_t subtitleBytes=0;WebVttConfiguration subtitleConfiguration;
    std::map<size_t,std::string> subtitleHeaders;const bool mapped=yes(plan["tracks"][t],"webVttHeader");
    size_t epochIndex=0;double subtitleElapsed=0;
    for(auto index:tracks[t]){cancel->check();const auto& info=parts[index].info;if(mapped&&(index==tracks[t].front()||yes(info,"initialization")))continue;if(subtitleDiscontinuity&&yes(info,"discontinuity"))++epochIndex;auto text=readText(parts[index].path,2*1024*1024);if(mapped){size_t headerPart=tracks[t].front();if(info.contains("mapIndex"))headerPart=tracks[t].at((size_t)num(info,"mapIndex"));auto [it,inserted]=subtitleHeaders.try_emplace(headerPart);if(inserted)it->second=readText(parts[headerPart].path,2*1024*1024);text=webVttWithHeader(it->second,text);}subtitleBytes+=text.size();if(subtitleBytes>16*1024*1024)throw std::runtime_error("Subtitle input exceeds 16 MB.");
     if(subtitleDiscontinuity){if(epochIndex>=epochs.size())throw std::runtime_error("Recorded subtitle epoch is missing its media boundary.");const auto& epoch=epochs[epochIndex];std::vector<SubtitleCue> segment;appendWebVtt(segment,text,epoch.clock,subtitleElapsed-epoch.start,true,&subtitleConfiguration);auto offset=(i64)std::llround(epoch.start*1000),end=(i64)std::llround((epoch.start+epoch.duration)*1000);for(auto& cue:segment){const auto sourceStart=cue.start,sourceEnd=cue.end;cue.start+=offset;cue.end=std::min(end,cue.end+offset);if(cue.end>cue.start){cue.text=shiftedWebVttCueText(cue.text,sourceStart,sourceEnd,offset,cue.start,cue.end);if(cues.size()>=100000)throw std::runtime_error("Subtitle cue limit exceeded.");cues.push_back(std::move(cue));}}subtitleElapsed+=real(info,"duration");}
     else appendWebVtt(cues,text,mediaStart,real(info,"timeline"),str(plan,"type")=="hls",&subtitleConfiguration);}
    atomicText(path,mergedWebVtt(std::move(cues),subtitleConfiguration.header()));continue;
   }
   if(discontinuous){
    double longest=0;for(auto index:tracks[t])longest=std::max(longest,real(parts[index].info,"duration"));std::ostringstream playlist;playlist.imbue(std::locale::classic());playlist<<"#EXTM3U\n#EXT-X-VERSION:7\n#EXT-X-TARGETDURATION:"<<(int)std::ceil(longest)<<"\n#EXT-X-MEDIA-SEQUENCE:0\n";
    i64 currentMap=-1;for(auto index:tracks[t]){cancel->check();const auto& part=parts[index];if(yes(part.info,"initialization"))continue;if(yes(part.info,"discontinuity"))playlist<<"#EXT-X-DISCONTINUITY\n";if(part.info.contains("mapIndex")){auto map=num(part.info,"mapIndex");if(map!=currentMap){const auto& header=parts[tracks[t].at((size_t)map)];playlist<<"#EXT-X-MAP:URI=\""<<utf8(header.path.filename().wstring())<<"\"\n";currentMap=map;}}playlist<<"#EXTINF:"<<std::fixed<<std::setprecision(6)<<real(part.info,"duration")<<",\n"<<utf8(part.path.filename().wstring())<<"\n";}playlist<<"#EXT-X-ENDLIST\n";atomicText(path,playlist.str());continue;
   }
   // The concat protocol presents the same byte order as the local copy.
   // Bound its file handles and keep well below the Windows command limit.
   if(allowConcatenatedInput&&tracks[t].size()<=32){
    std::wstring source=L"concat:";
    for(auto index:tracks[t]){
     if(source.size()>7)source+=L'|';
     source+=L"file:"+parts[index].path.generic_wstring();
     if(source.size()>8000)break;
    }
    if(source.size()<=8000){concatenatedInputs[t]=std::move(source);continue;}
   }
   materializeTrack(t,path);
  }
  auto tools=appDir()/L"tools",staging=job->target().parent_path()/(L".udm-"+wide(job->id())+L".adaptive."+wide(str(plan,"container","mp4")));auto mergeStart=std::chrono::steady_clock::now();
  requirePublicationRecovered(m,job);
  try{
   const auto assemble=[&](bool useConcatenatedInputs){
    std::vector<std::wstring> args={L"-hide_banner",L"-loglevel",L"error",L"-nostdin",L"-y"};
    for(size_t i=0;i<inputs.size();++i){
     const auto& input=inputs[i];
     if(useConcatenatedInputs&&!concatenatedInputs[i].empty()){
      args.insert(args.end(),{L"-protocol_whitelist",L"file,concat",L"-i",concatenatedInputs[i]});
     }else{
      args.insert(args.end(),{L"-protocol_whitelist",L"file,pipe"});
      if(input.extension()==L".m3u8")args.insert(args.end(),{L"-allowed_extensions",L"ALL",L"-allowed_segment_extensions",L"ALL",L"-extension_picky",L"0"});
      args.insert(args.end(),{L"-i",input.wstring()});
     }
    }
    if(audioOnly)args.insert(args.end(),{L"-map",L"0:a:0"});
    else {args.insert(args.end(),{L"-map",L"0:v:0",L"-map",audioInput>=0?std::to_wstring(audioInput)+L":a:0":L"0:a:0?"});}
    args.insert(args.end(),{L"-c",L"copy"});
    if(subtitleInput>=0){args.insert(args.end(),{L"-map",std::to_wstring(subtitleInput)+L":s:0",L"-c:s",L"mov_text",L"-metadata:s:s:0",L"handler_name="+wide(str(plan,"subtitleName"))});auto language=audioLanguageTag(str(plan,"subtitleLanguage"));if(!language.empty())args.insert(args.end(),{L"-metadata:s:s:0",L"language="+wide(language)});}
    appendAudioMetadata(args,plan);if(str(plan,"container","mp4")=="ts")args.insert(args.end(),{L"-f",L"mpegts"});else args.insert(args.end(),{L"-movflags",L"+faststart"});args.push_back(staging.wstring());if(layout.multiplePeriods){inputs=assembleDashPeriods(layout,folder,staging,plan,*cancel);subtitleInput=std::any_of(layout.descriptions.begin(),layout.descriptions.end(),[](const Json& track){return str(track,"kind")=="subtitle";})?1:-1;}else execute(tools/L"ffmpeg.exe",args,300,*cancel);
    auto probe=Json::parse(execute(tools/L"ffprobe.exe",{L"-v",L"error",L"-show_entries",L"stream=codec_type,height",L"-of",L"json",staging.wstring()},20,*cancel));bool video=false,audio=false,subtitle=false;for(const auto& stream:probe["streams"]){if(str(stream,"codec_type")=="video"){video=true;if(num(plan,"height")>0&&num(stream,"height")!=num(plan,"height"))throw std::runtime_error("Output dimensions differ from the selected quality.");}audio|=str(stream,"codec_type")=="audio";subtitle|=str(stream,"codec_type")=="subtitle";}
    if((audioOnly?video:!video)||(yes(plan,"audioExpected")&&!audio)||(subtitleInput>=0&&!subtitle))
     throw std::runtime_error("The output does not contain the selected media tracks.");
   };
   const bool useConcatenatedInputs=std::any_of(concatenatedInputs.begin(),concatenatedInputs.end(),[](const auto& input){return !input.empty();});
   try{assemble(useConcatenatedInputs);}
   catch(const Cancelled&){throw;}
   catch(const std::exception&){
    if(!useConcatenatedInputs)throw;
    cancel->check();
    for(size_t i=0;i<inputs.size();++i)if(!concatenatedInputs[i].empty())materializeTrack(i,inputs[i]);
    assemble(false);
   }
   VerifiedMediaFile verifiedOutput(staging);
   ScopedAssembledPrefetchSize displaySize(m,*job,*cancel,verifiedOutput.size);
   auto digest=fileHash(verifiedOutput.file.h,[&]{cancel->check();});
   if(!str(job->data,"ExpectedSha256").empty()&&lower(digest)!=lower(str(job->data,"ExpectedSha256")))
    throw std::runtime_error("SHA-256 verification failed.");
   cancel->check();
   const auto mergeSeconds=std::chrono::duration<double>(std::chrono::steady_clock::now()-mergeStart).count();
   // Measure the verified muxed output, then retain those exact bytes until Start.
   const auto confirmationStarted=std::chrono::steady_clock::now();
   auto recordConfirmationWait=[&]{confirmationSeconds+=std::chrono::duration<double>(std::chrono::steady_clock::now()-confirmationStarted).count();};
   const auto verifiedSize=verifiedOutput.size;
   // The verified output and the original segments suffice for any later retry.
   for(const auto& input:inputs){std::error_code ignored;fs::remove(input,ignored);}
   Json prepared;
   {Lock lock(m.mutex);
    if(!currentBinding(*job).empty())
     prepared={{"Version",1},{"Path",utf8(staging.wstring())},{"Size",verifiedSize},{"Sha256",digest},{"Binding",binding},{"Container",container},{"MergeSeconds",mergeSeconds}};
   }
   try{m.waitForPrefetchConfirmation(job,*cancel,verifiedSize,prepared);}catch(...){recordConfirmationWait();throw;}recordConfirmationWait();
   {Lock lock(m.mutex);job->data["MergeSeconds"]=mergeSeconds;}
   verifiedOutput.releaseForPublication(staging);
   publishMediaOutput(m,job,staging,digest,cancel);markZone(job->target());
  }catch(...){bool retained;{Lock lock(m.mutex);retained=retainedMediaOutputPath(job->data)==staging;job->data.erase("PrefetchedFinalSize");}std::error_code ec;if(!retained&&!publicationNeedsRecovery(m,job))fs::remove(staging,ec);throw;}
  cleanParts();
 }catch(...){Lock lock(m.mutex);job->data["MediaPhase"]=cancel->cancelled()?"Paused":"Failed";job->data.erase("PrefetchedFinalSize");if(!networkFinished)job->data["TransferSeconds"]=priorTransfer+std::chrono::duration<double>(std::chrono::steady_clock::now()-started).count();job->data["ElapsedSeconds"]=priorElapsed+std::max(0.0,std::chrono::duration<double>(std::chrono::steady_clock::now()-started).count()-confirmationSeconds);throw;}
 {Lock lock(m.mutex);job->data["ElapsedSeconds"]=priorElapsed+std::max(0.0,std::chrono::duration<double>(std::chrono::steady_clock::now()-started).count()-confirmationSeconds);}
}
}
