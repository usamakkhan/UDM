#pragma once
#include "DashManifest.hpp"
#include "MediaStorage.hpp"
#include "MediaPublication.hpp"
#include "BrowserRequest.hpp"
#include "DashRecordingSession.hpp"
#include <cmath>
#include <limits>

namespace udm {
inline void validateLiveDashPlan(const Json& plan){
 if(!plan.is_object()||plan.dump().size()>65536||str(plan,"type")!="dash"||!yes(plan,"live")||yes(plan,"encrypted")||plan.contains("periods"))throw std::runtime_error("Invalid live DASH recording plan.");
 const Url manifest(str(plan,"manifest"));if(manifest.scheme!="http"&&manifest.scheme!="https")throw std::runtime_error("Live DASH requires an HTTP manifest.");
 if(!plan.contains("tracks")||!plan["tracks"].is_array()||plan["tracks"].empty()||plan["tracks"].size()>2)throw std::runtime_error("Select one video and optional audio, or one audio track.");
 const bool audio= yes(plan,"audioOnly");if(str(plan,"container")!=(audio?"m4a":"mp4")||num(plan,"height")<0||num(plan,"height")>8640||(audio&&(num(plan,"height")||plan["tracks"].size()!=1)))throw std::runtime_error("Invalid live DASH output selection.");
 std::string period;std::set<std::pair<std::string,std::string>> selected;
 for(size_t i=0;i<plan["tracks"].size();++i){const auto& track=plan["tracks"][i];if(str(track,"kind")!=(audio||i?"audio":"video"))throw std::runtime_error("Invalid live DASH track order.");
  for(const auto* field:{"periodId","adaptationId","representationId"}){auto value=str(track,field);if(value.empty()||value.size()>512||std::any_of(value.begin(),value.end(),[](unsigned char c){return c<32||c==127;}))throw std::runtime_error("Live DASH requires stable track IDs.");}
  if(i&&period!=str(track,"periodId"))throw std::runtime_error("Live DASH tracks must share a Period.");period=str(track,"periodId");if(!selected.emplace(str(track,"adaptationId"),str(track,"representationId")).second)throw std::runtime_error("Duplicate live DASH track.");
  if(!track.contains("credentialUrls")||!track["credentialUrls"].is_array()||track["credentialUrls"].size()>64)throw std::runtime_error("Invalid live DASH credential URLs.");
  for(const auto& address:track["credentialUrls"]){if(!address.is_string())throw std::runtime_error("Invalid live DASH credential URL.");const Url resource(address.get<std::string>());if(resource.scheme!="http"&&resource.scheme!="https")throw std::runtime_error("Live DASH resources require HTTP.");}
 }
 if(yes(plan,"audioExpected")!=(audio||plan["tracks"].size()==2))throw std::runtime_error("Live DASH audio selection disagrees with its tracks.");
}

inline std::vector<size_t> liveDashRepresentations(const DashManifestSnapshot& snapshot,const Json& plan){
 std::vector<size_t> result;
 auto matches=[](const DashXmlNode& node,const char* name,const std::string& id){auto found=node.attributes.find("id");return node.name==name&&found!=node.attributes.end()&&found->second==id;};
 for(const auto& selected:plan["tracks"]){std::vector<size_t> found;
  for(auto p:snapshot.manifest.nodes.front().children){const auto& period=snapshot.manifest.nodes.at(p);if(!matches(period,"Period",str(selected,"periodId")))continue;
   for(auto a:period.children){const auto& adaptation=snapshot.manifest.nodes.at(a);if(!matches(adaptation,"AdaptationSet",str(selected,"adaptationId")))continue;
    for(auto r:adaptation.children)if(matches(snapshot.manifest.nodes.at(r),"Representation",str(selected,"representationId")))found.push_back(r);
   }
  }
  if(found.size()!=1)throw std::runtime_error("The selected live DASH track is missing or ambiguous.");result.push_back(found.front());
 }return result;
}

inline void liveDashBootstrapPath(const fs::path& folder){
 for(auto path=fs::absolute(folder).lexically_normal();!path.empty();){const auto flags=GetFileAttributesW(path.c_str());if(flags!=INVALID_FILE_ATTRIBUTES&&(flags&FILE_ATTRIBUTE_REPARSE_POINT))throw std::runtime_error("DASH bootstrap folder must not be redirected.");auto parent=path.parent_path();if(parent==path)break;path=std::move(parent);}
 for(const auto* name:{L"bootstrap.json",L"bootstrap.json.tmp",L"manifest-revision.json",L"manifest-revision.json.tmp"}){const auto flags=GetFileAttributesW((folder/name).c_str());if(flags!=INVALID_FILE_ATTRIBUTES&&(flags&FILE_ATTRIBUTE_REPARSE_POINT))throw std::runtime_error("DASH bootstrap file must not be redirected.");}
}

// Keep the original live identity separately from the latest accepted revision.
// Never persist steady-clock values or treat a disk read as a fresh UTC sample.
inline void checkpointLiveDashRevision(const fs::path& folder,const Json& plan,const DashLiveState& state,const Cancel& cancel){
 cancel.check();if(state.snapshot.requestUrl!=str(plan,"manifest")||state.tracks.empty())throw std::runtime_error("Invalid DASH revision checkpoint.");
 Json tracks=Json::array();for(const auto& track:state.tracks){auto identity=dashSegmentBinding(track,{0,0,1});identity.erase("number");identity.erase("time");identity.erase("duration");tracks.push_back(Json{{"identity",identity},{"periodDuration",track.addressing.period.duration?Json(*track.addressing.period.duration):Json()}});}
 const Json saved={{"plan",plan},{"url",state.snapshot.url},{"document",state.snapshot.document},{"etag",state.snapshot.etag},{"modified",state.snapshot.modified},{"tracks",tracks}};
 const auto plain=saved.dump();if(plain.size()>8*1024*1024)throw std::runtime_error("DASH revision checkpoint exceeds its storage limit.");
 const auto envelope=Json{{"schema",1},{"protected",protect(plain)}}.dump();if(envelope.size()>16*1024*1024)throw std::runtime_error("DASH protected revision exceeds its storage limit.");
 liveDashBootstrapPath(folder);cancel.check();atomicText(folder/L"manifest-revision.json",envelope,false);
}

inline void restoreLiveDashRevision(const fs::path& folder,const Json& plan,DashLiveState& state,const Cancel& cancel){
 cancel.check();liveDashBootstrapPath(folder);const auto path=folder/L"manifest-revision.json";if(!fs::exists(path))return;
 const auto envelope=Json::parse(readText(path,16*1024*1024));if(num(envelope,"schema")!=1)throw std::runtime_error("Unsupported DASH manifest revision.");
 const auto plain=reveal(str(envelope,"protected"));if(plain.size()>8*1024*1024)throw std::runtime_error("DASH revision exceeds its storage limit.");const auto saved=Json::parse(plain);
 if(!saved.contains("plan")||saved["plan"]!=plan||!saved.contains("tracks")||!saved["tracks"].is_array()||saved["tracks"].size()!=state.tracks.size())throw std::runtime_error("DASH revision selection changed.");
 DashManifestSnapshot snapshot;snapshot.requestUrl=state.snapshot.requestUrl;snapshot.url=str(saved,"url");const Url address(snapshot.url);if(address.scheme!="http"&&address.scheme!="https")throw std::runtime_error("Invalid DASH revision URL.");
 snapshot.document=str(saved,"document");snapshot.manifest=parseDashManifest(snapshot.document,cancel);snapshot.etag=str(saved,"etag");snapshot.modified=str(saved,"modified");snapshot.bodyReceived=std::chrono::steady_clock::now()-std::chrono::hours(24);
 const auto& previous=state.snapshot.manifest;const auto& next=snapshot.manifest;
 auto rootId=[](const DashManifest& manifest){auto found=manifest.nodes.front().attributes.find("id");return found==manifest.nodes.front().attributes.end()?std::optional<std::string>{}:std::optional<std::string>{found->second};};
 if(rootId(previous)!=rootId(next)||(previous.publishTimeUs&&(!next.publishTimeUs||*next.publishTimeUs<*previous.publishTimeUs))||((next.dynamic||next.availabilityStartUs)&&previous.availabilityStartUs!=next.availabilityStartUs)||(!previous.dynamic&&next.dynamic))throw std::runtime_error("DASH saved revision changed presentation identity or chronology.");
 auto tracks=state.tracks;for(size_t i=0;i<tracks.size();++i){tracks[i]=rematchDashTrack(snapshot,tracks[i],cancel);const auto& record=saved["tracks"][i];auto identity=dashSegmentBinding(tracks[i],{0,0,1});identity.erase("number");identity.erase("time");identity.erase("duration");if(!record.contains("identity")||record["identity"]!=identity)throw std::runtime_error("DASH revision track identity changed.");
  if(record.contains("periodDuration")&&!record["periodDuration"].is_null()){const double duration=real(record,"periodDuration");if(!record["periodDuration"].is_number()||!std::isfinite(duration)||duration<=0||duration>7*86400||(tracks[i].addressing.period.duration&&std::abs(*tracks[i].addressing.period.duration-duration)>0.000001))throw std::runtime_error("DASH revision Period end changed.");tracks[i].addressing.period.duration=duration;}
 }
 state.snapshot=std::move(snapshot);state.tracks=std::move(tracks);state.clock.reset();
}

inline DashLiveState loadLiveDashState(const fs::path& folder,const Json& plan,const Json& recording,const Cancel& cancel){
 liveDashBootstrapPath(folder);const auto manifest=str(plan,"manifest");
  const auto envelope=Json::parse(readText(folder/L"bootstrap.json",16*1024*1024));const auto schema=num(envelope,"schema");if(schema!=1&&schema!=2)throw std::runtime_error("Unsupported DASH recording bootstrap.");const auto text=reveal(str(envelope,"protected"));if(text.size()>8*1024*1024)throw std::runtime_error("DASH bootstrap exceeds its storage limit.");const auto saved=Json::parse(text);
  if(!saved.contains("plan")||saved["plan"]!=plan||str(saved,"requestUrl")!=manifest)throw std::runtime_error("DASH recording selection changed since capture began.");
  DashManifestSnapshot previous;previous.requestUrl=manifest;previous.url=str(saved,"url");const Url finalUrl(previous.url);if(finalUrl.scheme!="http"&&finalUrl.scheme!="https")throw std::runtime_error("Invalid DASH bootstrap manifest URL.");previous.document=str(saved,"document");previous.manifest=parseDashManifest(previous.document,cancel);
  // Persisted XML has no trustworthy steady-clock receipt time after restart.
  previous.bodyReceived=std::chrono::steady_clock::now()-std::chrono::hours(24);
  DashLiveState state;
  if(schema==1)state=createDashLiveState(previous,liveDashRepresentations(previous,plan),cancel);
  else{
   state.snapshot=previous;if(!recording.is_object()||!recording.contains("tracks")||recording["tracks"].size()!=plan["tracks"].size()||!saved.contains("tracks")||saved["tracks"].size()!=recording["tracks"].size())throw std::runtime_error("DASH continuation bootstrap has no matching recording identity.");
   const auto representations=liveDashRepresentations(previous,plan);for(size_t i=0;i<representations.size();++i){auto track=selectDashTrack(previous,representations[i],cancel);const auto& identity=recording["tracks"][i].at("identity");track.presentation=str(identity,"presentation");auto actual=dashSegmentBinding(track,{0,0,1});actual.erase("number");actual.erase("time");actual.erase("duration");if(actual!=identity||saved["tracks"][i]!=identity)throw std::runtime_error("DASH continuation bootstrap identity changed.");if(recording["tracks"][i].contains("periodDuration")&&!recording["tracks"][i]["periodDuration"].is_null()){const double duration=real(recording["tracks"][i],"periodDuration");if(!std::isfinite(duration)||duration<=0||(track.addressing.period.duration&&std::abs(*track.addressing.period.duration-duration)>0.000001))throw std::runtime_error("DASH continuation Period end changed.");track.addressing.period.duration=duration;}state.tracks.push_back(std::move(track));}
  }
  restoreLiveDashRevision(folder,plan,state,cancel);return state;
}

inline DashLiveState resumeLiveDashState(const fs::path& folder,const Json& plan,const Json& recording,const Headers& headers,const Json& prefs,const std::shared_ptr<Cancel>& cancel,const DashMetadataAccounting& onBytes){
 liveDashBootstrapPath(folder);const auto path=folder/L"bootstrap.json";const auto manifest=str(plan,"manifest");
 if(fs::exists(path)){
  auto state=loadLiveDashState(folder,plan,recording,*cancel);
  auto refreshed=refreshDashManifest(manifest,headers,prefs,cancel,&state.snapshot,onBytes);for(auto& track:state.tracks)track=rematchDashTrack(refreshed,track,*cancel);state.snapshot=std::move(refreshed);checkpointLiveDashRevision(folder,plan,state,*cancel);return state;
 }
 if(fs::exists(folder/L"manifest-revision.json"))throw std::runtime_error("DASH revision has lost its original bootstrap; retained media was not rebound.");
 auto snapshot=refreshDashManifest(manifest,headers,prefs,cancel,nullptr,onBytes);auto state=createDashLiveState(snapshot,liveDashRepresentations(snapshot,plan),*cancel);
 if(recording.is_object()){
  if(!recording.contains("tracks")||recording["tracks"].size()!=state.tracks.size())throw std::runtime_error("DASH bootstrap selection does not match the retained recording.");
  for(size_t i=0;i<state.tracks.size();++i){auto identity=dashSegmentBinding(state.tracks[i],{0,0,1});identity.erase("number");identity.erase("time");identity.erase("duration");if(!recording["tracks"][i].contains("identity")||recording["tracks"][i]["identity"]!=identity)throw std::runtime_error("DASH bootstrap identity changed; existing media was retained.");}
 }
 const Json saved={{"plan",plan},{"requestUrl",snapshot.requestUrl},{"url",snapshot.url},{"document",snapshot.document}};
 cancel->check();fs::create_directories(folder);liveDashBootstrapPath(folder);atomicText(path,Json{{"schema",1},{"protected",protect(saved.dump())}}.dump(),false);return state;
}

inline Json prepareLiveDashPeriod(const fs::path& folder,const Json& plan,DashLiveState& state,const DashResourceHeaders& headers,const Json& prefs,const std::shared_ptr<Cancel>& cancel){
 cancel->check();validateLiveDashPlan(plan);liveDashBootstrapPath(folder);Json identities=Json::array();DashLiveUpdate empty;empty.state=state;empty.staticPresentation=!state.snapshot.manifest.dynamic;
 if(fs::exists(folder/L"recording.json")){const auto retained=reconcileDashRecording(folder,*cancel);if(retained["tracks"].size()!=state.tracks.size())throw std::runtime_error("DASH prepared Period track count changed.");for(size_t t=0;t<state.tracks.size();++t){const auto& track=retained["tracks"][t];if(track.contains("periodDuration")&&!track["periodDuration"].is_null()){const double duration=real(track,"periodDuration");auto& address=state.tracks[t].addressing;if(!std::isfinite(duration)||duration<=0||(address.period.duration&&std::abs(*address.period.duration-duration)>0.000001))throw std::runtime_error("DASH prepared Period end changed.");address.period.duration=duration;}}empty.state=state;}
 for(const auto& track:state.tracks){auto identity=dashSegmentBinding(track,{0,0,1});identity.erase("number");identity.erase("time");identity.erase("duration");identities.push_back(std::move(identity));DashResourceWindow window;window.timescale=track.addressing.timescale;window.presentationTimeOffset=track.addressing.presentationTimeOffset;window.period=track.addressing.period;empty.windows.push_back(std::move(window));}
 const auto path=folder/L"bootstrap.json";if(fs::exists(path)){const auto envelope=Json::parse(readText(path,16*1024*1024));const auto saved=Json::parse(reveal(str(envelope,"protected")));if(num(envelope,"schema")!=2||!saved.contains("plan")||saved["plan"]!=plan||!saved.contains("tracks")||saved["tracks"]!=identities)throw std::runtime_error("DASH prepared Period belongs to a different transition.");const auto previous=parseDashManifest(str(saved,"document"),*cancel);if(previous.publishTimeUs&&(!state.snapshot.manifest.publishTimeUs||*previous.publishTimeUs>*state.snapshot.manifest.publishTimeUs))throw std::runtime_error("DASH prepared Period has a newer saved publication.");}
 auto index=captureDashWindow(folder,empty,headers,prefs,cancel,{},DashCaptureInterval{0,{}},1);
 if(!fs::exists(path)){const Json saved={{"plan",plan},{"requestUrl",state.snapshot.requestUrl},{"url",state.snapshot.url},{"document",state.snapshot.document},{"tracks",identities}};const auto text=saved.dump();if(text.size()>8*1024*1024)throw std::runtime_error("DASH continuation bootstrap exceeds its storage limit.");const auto envelope=Json{{"schema",2},{"protected",protect(text)}}.dump();if(envelope.size()>16*1024*1024)throw std::runtime_error("DASH continuation envelope exceeds its storage limit.");liveDashBootstrapPath(folder);cancel->check();atomicText(path,envelope,false);}
 return index;
}

struct LiveDashProgress {
 i64 bytes=0,complete=0,total=0;double seconds=0,end=0;bool ready=false;
};
inline LiveDashProgress liveDashProgress(const Json& index){
 LiveDashProgress result;if(!index.is_object()||!index.contains("tracks")||index["tracks"].empty())return result;bool ready=true;double end=std::numeric_limits<double>::infinity();const double begin=real(index,"captureStart");
 for(const auto& track:index["tracks"]){bool prefix=true,any=false;double trackEnd=0;std::map<i64,Json> ordered;for(const auto& record:track["segments"])ordered.emplace(num(record,"number"),record);
  for(const auto& entry:ordered){const auto& record=entry.second;++result.total;if(yes(record,"committed")){++result.complete;result.bytes+=num(record["resource"],"bytes");if(prefix){any=true;trackEnd=(static_cast<double>(num(record,"time"))+num(record,"duration")-num(track["identity"],"presentationTimeOffset"))/num(track["identity"],"timescale");}}else prefix=false;}
  if(track.contains("initialization"))result.bytes+=num(track["initialization"],"bytes");if(track.contains("periodDuration")&&track["periodDuration"].is_number())trackEnd=std::min(trackEnd,real(track,"periodDuration"));end=std::min(end,trackEnd);ready=ready&&any&&!track.contains("pendingInitialization")&&(!track.contains("initialization")||yes(track,"initializationVerified"));
 }
 result.end=std::isfinite(end)?end:0;result.seconds=ready?std::max(0.0,result.end-begin):0;result.ready=ready&&result.seconds>0;return result;
}

inline void liveDashTransfer(Manager& manager,JobPtr job,const std::shared_ptr<Cancel>& cancel){
 Json plan,prefs,cookies,captured;Headers common;std::string expectedHash;int retries;
 {Lock lock(manager.mutex);plan=Json::parse(reveal(str(job->data,"ProtectedAdaptive")));prefs=manager.state["Settings"];common=readHeaders(job->data);auto decode=[&](const char* field){auto value=str(job->data,field);return value.empty()?Json::object():Json::parse(reveal(value));};cookies=decode("ProtectedMediaCookies");captured=decode("ProtectedMediaHeaders");expectedHash=str(job->data,"ExpectedSha256");retries=manager.retries(str(job->data,"Queue"));}
 validateLiveDashPlan(plan);const auto sessionFolder=mediaWorkingDirectory(manager,job)/L"live-dash";{Lock lock(manager.mutex);manager.save();}
 DashRecordingSession session(sessionFolder,plan,*cancel);auto folder=session.currentDirectory();auto capturePlan=session.currentPlan();validateLiveDashPlan(capturePlan);
 DashResourceHeaders scopes;if(!captured.is_object()||!cookies.is_object()||captured.size()>64||cookies.size()>64)throw std::runtime_error("Invalid live DASH credential scopes.");
 for(auto it=captured.begin();it!=captured.end();++it)scopes[it.key()]=browserHeaders({{"headers",it.value()}});
 for(auto it=cookies.begin();it!=cookies.end();++it){if(!it.value().is_string())throw std::runtime_error("Invalid live DASH cookie scope.");auto& headers=scopes[it.key()];for(auto h=headers.begin();h!=headers.end();)if(lower(h->first)=="cookie")h=headers.erase(h);else ++h;headers["Cookie"]=it.value().get<std::string>();}
 for(const auto& scope:scopes){if(Url(scope.first).origin!=scope.first)throw std::runtime_error("Invalid live DASH credential origin.");validateHeaders(scope.second);}
 const auto manifest=str(plan,"manifest");Headers manifestHeaders;for(const auto& entry:common)if(lower(entry.first)=="user-agent"||lower(entry.first)=="referer")manifestHeaders[entry.first]=entry.second;auto manifestScope=scopes.find(Url(manifest).origin);if(manifestScope!=scopes.end())for(const auto& entry:manifestScope->second){for(auto it=manifestHeaders.begin();it!=manifestHeaders.end();)if(lower(it->first)==lower(entry.first))it=manifestHeaders.erase(it);else ++it;manifestHeaders[entry.first]=entry.second;}
 prefs.erase("ActiveBrowserSession");prefs["SiteLogins"]=Json::array();prefs["WindowsLoginIntranet"]=false;
 auto capture=std::make_shared<Cancel>();capture->parent=cancel;Rate rate;using Clock=std::chrono::steady_clock;const auto started=Clock::now();double previousTransfer,previousElapsed;bool receiving=true;
 struct Scope {Manager& manager;JobPtr job;~Scope(){Lock lock(manager.mutex);job->liveCapture.reset();}} scope{manager,job};
 {Lock lock(manager.mutex);previousTransfer=real(job->data,"TransferSeconds");previousElapsed=real(job->data,"ElapsedSeconds");job->liveCapture=capture;job->data["LiveRecording"]=true;job->data["Size"]=-1;job->data["LiveSaveReady"]=false;job->workers.assign(1,{});}
 auto finishRequested=[&]{Lock lock(manager.mutex);return yes(job->data,"LiveStopRequested");};
 const DashMetadataAccounting accountMetadata=[&](size_t bytes,const Cancel& transfer){manager.charge(bytes,transfer,rate,job);Lock lock(manager.mutex);job->data["TransferredBytes"]=num(job->data,"TransferredBytes")+static_cast<i64>(bytes);job->data["AdaptiveMetadataBytes"]=num(job->data,"AdaptiveMetadataBytes")+static_cast<i64>(bytes);};
 Json index;LiveDashProgress previousPeriods;std::optional<DashLiveState> preparedState;std::map<std::string,LiveDashProgress> sealedProgress;
 auto updateProgress=[&]{const auto current=liveDashProgress(index);Lock lock(manager.mutex);job->data["Received"]=previousPeriods.bytes+current.bytes;job->data["LiveCapturedSeconds"]=previousPeriods.seconds+current.seconds;job->data["AdaptiveCompletedSegments"]=previousPeriods.complete+current.complete;job->data["AdaptiveTotalSegments"]=previousPeriods.total+current.total;job->data["LiveSaveReady"]=previousPeriods.ready||current.ready;
 };
 try{
 for(;;){
  folder=session.currentDirectory();capturePlan=session.currentPlan();validateLiveDashPlan(capturePlan);index=Json();previousPeriods={};
  if(session.hasPeriods()){const auto folders=session.folders();for(size_t p=0;p+1<folders.size();++p){auto found=sealedProgress.find(folders[p]);if(found==sealedProgress.end()){auto saved=reconcileDashRecording(folders[p]=="."?sessionFolder:sessionFolder/wide(folders[p]),*cancel);if(!yes(saved,"sealed"))throw std::runtime_error("An earlier DASH session Period is no longer sealed.");auto progress=liveDashProgress(saved);progress.seconds=real(saved,"captureEnd")-real(saved,"captureStart");found=sealedProgress.emplace(folders[p],progress).first;}const auto& progress=found->second;if(!progress.ready)throw std::runtime_error("An earlier DASH session Period has incomplete retained media.");previousPeriods.bytes+=progress.bytes;previousPeriods.complete+=progress.complete;previousPeriods.total+=progress.total;previousPeriods.seconds+=progress.seconds;previousPeriods.ready=true;}}
  if(fs::exists(folder/L"recording.json")){index=reconcileDashRecording(folder,*cancel);if(index.contains("captureStart"))session.checkpoint(session.currentFolder(),index,*cancel);else if(session.hasPeriods())throw std::runtime_error("DASH session capture interval is missing from its recording index.");}else if(session.hasPeriods())throw std::runtime_error("DASH session recording index is missing; retained files were not rebound.");updateProgress();
  std::optional<DashPeriodTransition> transition;bool advance=false;
  if(!yes(index,"sealed")&&!session.saving()&&!finishRequested()&&index.is_object()&&index.contains("tracks")&&!index["tracks"].empty()){
   const auto progress=liveDashProgress(index);const auto& first=index["tracks"].front();if(progress.ready&&first.contains("periodDuration")&&first["periodDuration"].is_number()&&progress.end+0.000001>=real(first,"periodDuration")){
    const auto saved=loadLiveDashState(folder,capturePlan,index,*cancel);transition=planDashPeriodTransition(saved,saved.snapshot,*cancel);if(transition)session.beginAdvance(*cancel);else if(!saved.snapshot.manifest.dynamic){index=sealDashRecording(folder,*cancel);session.checkpoint(session.currentFolder(),index,*cancel);session.beginSaving(*cancel);}
   }
  }
  if(session.advancing()&&!yes(index,"sealed")){const auto progress=liveDashProgress(index);if(!progress.ready||progress.end+0.000001<real(index["tracks"].front(),"periodDuration"))throw std::runtime_error("DASH transition boundary is not fully captured.");index=sealDashRecording(folder,*cancel);session.checkpoint(session.currentFolder(),index,*cancel);}
  if(yes(index,"sealed")&&!session.advancing()&&!session.saving())session.beginSaving(*cancel);
  if(yes(index,"sealed")&&session.advancing()&&!finishRequested()){
   try{auto saved=loadLiveDashState(folder,capturePlan,index,*capture);for(;;){capture->check();const auto refreshed=refreshDashManifest(str(capturePlan,"manifest"),manifestHeaders,prefs,capture,&saved.snapshot,accountMetadata);transition=planDashPeriodTransition(saved,refreshed,*capture);if(transition){advance=true;break;}if(!refreshed.manifest.dynamic)break;saved.snapshot=refreshed;{Lock lock(manager.mutex);job->data["MediaPhase"]="Waiting for the next DASH Period";manager.save();}capture->wait(1000);}}
   catch(const Cancelled&){cancel->check();if(!finishRequested())throw;}
  }
  if(!yes(index,"sealed")&&!finishRequested()){
   try{
    auto state=preparedState?std::move(*preparedState):resumeLiveDashState(folder,capturePlan,index,manifestHeaders,prefs,capture,accountMetadata);preparedState.reset();const auto& snapshot=state.snapshot;
    for(size_t i=0;i<state.tracks.size();++i){const auto& format=state.tracks[i].format;auto kind=str(format,"contentType");if(kind.empty()){auto mime=str(format,"mimeType");kind=mime.find("video/")==0?"video":mime.find("audio/")==0?"audio":"";}if(kind!=str(plan["tracks"][i],"kind"))throw std::runtime_error("Live DASH track kind changed.");if(kind=="video"&&num(plan,"height")&&str(format,"height")!=std::to_string(num(plan,"height")))throw std::runtime_error("Live DASH selected quality changed.");}
    double edge=0;if(snapshot.manifest.dynamic){state.clock=synchronizeDashClock(snapshot.manifest,snapshot.url,prefs,snapshot.bodyReceived,capture,snapshot.responseAgeSeconds==0&&Clock::now()-snapshot.bodyReceived<std::chrono::seconds(60),accountMetadata);edge=static_cast<double>(dashClockNow(*state.clock,Clock::now())-*snapshot.manifest.availabilityStartUs)/1000000.0-state.tracks.front().addressing.period.start;}
    else{if(!index.is_object()||!index.contains("captureStart"))throw std::runtime_error("An ended DASH stream has no saved recording start.");for(const auto& track:state.tracks){if(!track.addressing.period.duration)throw std::runtime_error("Ended DASH recording requires a known Period duration.");edge=std::max(edge,*track.addressing.period.duration);}}
    const auto anchored=Clock::now();
    const double start=index.is_object()&&index.contains("captureStart")?real(index,"captureStart"):std::max(0.0,edge-snapshot.manifest.suggestedDelay.value_or(6));
    if(!std::isfinite(start)||start<0||start>=7*86400)throw std::runtime_error("Live DASH start lies outside the supported recording interval.");
    std::optional<double> requestedEnd;if(index.is_object()&&index.contains("captureEnd")&&!index["captureEnd"].is_null()){if(!index["captureEnd"].is_number()||!std::isfinite(real(index,"captureEnd"))||real(index,"captureEnd")<=start||real(index,"captureEnd")>7*86400)throw std::runtime_error("Invalid saved DASH end boundary.");requestedEnd=real(index,"captureEnd");}
    int failures=0;
    for(;;){capture->check();if(finishRequested())break;
     try{
      const double horizon=requestedEnd?*requestedEnd:transition?transition->previousEnd-state.tracks.front().addressing.period.start:std::min(7.0*86400,std::max(start+1,edge+std::chrono::duration<double>(Clock::now()-anchored).count()+30));std::vector<std::pair<i64,i64>> ranges;
      for(size_t t=0;t<state.tracks.size();++t){const auto& address=state.tracks[t].addressing;double from=start;
       if(index.is_object()&&index.contains("tracks")){const auto& track=index["tracks"][t];if(!track.contains("pendingInitialization")&&(!track.contains("initialization")||yes(track,"initializationVerified"))){std::map<i64,Json> ordered;for(const auto& record:track["segments"])ordered.emplace(num(record,"number"),record);for(const auto& entry:ordered){if(!yes(entry.second,"committed"))break;from=std::max(from,static_cast<double>(num(entry.second,"time")+num(entry.second,"duration")-address.presentationTimeOffset)/address.timescale);}}}
       auto ticks=[&](double seconds,bool upper){const long double value=static_cast<long double>(seconds)*address.timescale+address.presentationTimeOffset;if(value<0||value>9007199254740991.0L)throw std::runtime_error("Live DASH window exceeds supported precision.");return static_cast<i64>(upper?std::ceil(value):std::floor(value));};ranges.emplace_back(ticks(from,false),ticks(std::max(horizon,from+1),true));
      }
      auto next=updateDashLivePlan(state,ranges,manifestHeaders,prefs,capture,accountMetadata);
      if(next.state.snapshot.document!=state.snapshot.document||next.state.snapshot.url!=state.snapshot.url||next.state.snapshot.etag!=state.snapshot.etag||next.state.snapshot.modified!=state.snapshot.modified)checkpointLiveDashRevision(folder,capturePlan,next.state,*capture);
      state=next.state;
      if(auto planned=planDashPeriodTransition(state,state.snapshot,*capture))transition=std::move(planned);
      bool ended=next.staticPresentation;for(const auto& track:state.tracks)ended=ended&&track.addressing.period.duration&&horizon>=*track.addressing.period.duration;
      if(ended&&!requestedEnd)requestedEnd=state.tracks.front().addressing.period.duration;
      size_t connections;{Lock lock(manager.mutex);job->data["MediaPhase"]="Recording live DASH stream";connections=static_cast<size_t>(std::clamp<i64>(num(job->data,"Connections",8),1,3));job->workers.assign(state.tracks.size(),{});for(size_t t=0;t<job->workers.size();++t){job->workers[t].number=static_cast<int>(t+1);job->workers[t].end=-1;job->workers[t].state="Waiting for track";}}
      index=captureDashWindow(folder,next,scopes,prefs,capture,[&](size_t track,size_t count,const Cancel& transfer){manager.charge(count,transfer,rate,job);Lock lock(manager.mutex);job->data["TransferredBytes"]=num(job->data,"TransferredBytes")+static_cast<i64>(count);job->data["Received"]=num(job->data,"Received")+static_cast<i64>(count);auto& worker=job->workers[track];worker.received+=count;worker.position+=count;worker.state="Receiving live DASH segments";},DashCaptureInterval{start,requestedEnd},connections);updateProgress();failures=0;
      session.checkpoint(session.currentFolder(),index,*capture);
      const auto complete=liveDashProgress(index);
      if(transition&&complete.ready&&complete.end+0.000001>=transition->previousEnd-state.tracks.front().addressing.period.start){advance=true;break;}
      if(requestedEnd&&complete.ready&&complete.end+0.000001>=*requestedEnd)break;{Lock lock(manager.mutex);job->data["MediaPhase"]="Waiting for live DASH segments";for(auto& worker:job->workers)worker.state="Track saved";manager.save();}capture->wait(1000);
     }catch(const Cancelled&){throw;}catch(const std::exception&){capture->check();if(failures++>=retries)throw;{Lock lock(manager.mutex);job->data["MediaPhase"]="Live DASH request failed; retrying";}capture->wait(std::min(5000,300*failures));}
    }
   }catch(const Cancelled&){cancel->check();if(!finishRequested())throw;}
  }
  cancel->check();if(finishRequested()&&session.discardEmptyTail(index,*cancel))continue;
  if(advance&&transition&&!finishRequested())session.beginAdvance(*cancel);
  index=sealDashRecording(folder,*cancel);session.checkpoint(session.currentFolder(),index,*cancel);auto sealed=liveDashProgress(index);if(!sealed.ready)throw std::runtime_error("The sealed DASH Period has incomplete retained media.");sealed.seconds=real(index,"captureEnd")-real(index,"captureStart");sealedProgress[session.currentFolder()]=sealed;updateProgress();
  if(advance&&transition&&!finishRequested()){
   try{auto nextPlan=plan;for(size_t t=0;t<transition->state.tracks.size();++t){const auto& track=transition->state.tracks[t];nextPlan["tracks"][t]["periodId"]=track.periodId;nextPlan["tracks"][t]["adaptationId"]=track.adaptationId;nextPlan["tracks"][t]["representationId"]=track.representationId;}
    const auto nextFolder=session.nextFolder();const auto nextIndex=prepareLiveDashPeriod(sessionFolder/wide(nextFolder),nextPlan,transition->state,scopes,prefs,capture);session.checkpoint(nextFolder,nextIndex,*capture);preparedState=std::move(transition->state);continue;
   }catch(const Cancelled&){cancel->check();if(!finishRequested())throw;}
  }
  session.beginSaving(*cancel);break;
 }
  {Lock lock(manager.mutex);job->liveCapture.reset();job->data["Status"]="Merging";job->data["MediaPhase"]="Saving live DASH recording";job->data["TransferSeconds"]=previousTransfer+std::chrono::duration<double>(Clock::now()-started).count();receiving=false;manager.save();}
  const auto merging=Clock::now();auto output=assembleDashRecordingSession(sessionFolder,session.folders(),plan,*cancel);{Lock lock(manager.mutex);job->data["AdaptiveAssemblyReused"]=output.reused;}if(!expectedHash.empty()&&lower(expectedHash)!=lower(output.sha256))throw std::runtime_error("Live DASH recording SHA-256 verification failed.");cancel->check();
  publishMediaOutput(manager,job,output.staging,output.sha256,cancel);{Lock lock(manager.mutex);job->data["MediaPhase"]="Complete";job->data["LiveSaveReady"]=false;job->data["MergeSeconds"]=std::chrono::duration<double>(Clock::now()-merging).count();manager.save();}markZone(job->target());
 }catch(...){if(receiving)updateProgress();Lock lock(manager.mutex);if(receiving)job->data["TransferSeconds"]=previousTransfer+std::chrono::duration<double>(Clock::now()-started).count();job->data["ElapsedSeconds"]=previousElapsed+std::chrono::duration<double>(Clock::now()-started).count();throw;}
 {Lock lock(manager.mutex);job->data["ElapsedSeconds"]=previousElapsed+std::chrono::duration<double>(Clock::now()-started).count();}
}
}
