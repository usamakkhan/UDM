#pragma once
#include "HlsRecording.hpp"
#include "MediaStorage.hpp"
#include "BrowserRequest.hpp"
#include "AudioMetadata.hpp"
#include "LiveSubtitles.hpp"
#include <fstream>
#include <future>
#ifdef UDM_LIVE_TIMING_TRACE
#include <iostream>
#endif

namespace udm {
inline Headers liveHlsHeaders(const std::string& address,const Headers& common,const Json& cookies,const Json& captured){
 Headers result;for(const auto& item:common)if(lower(item.first)!="cookie"&&lower(item.first)!="authorization"&&lower(item.first)!="origin")result[item.first]=item.second;
 auto origin=Url(address).origin;if(captured.contains(origin)){auto scoped=browserHeaders({{"headers",captured[origin]}});for(const auto& entry:scoped){for(auto it=result.begin();it!=result.end();)if(lower(it->first)==lower(entry.first))it=result.erase(it);else ++it;result[entry.first]=entry.second;}}
 if(cookies.contains(origin)&&cookies[origin].is_string()&&!cookies[origin].get<std::string>().empty())result["Cookie"]=cookies[origin].get<std::string>();return result;
}
inline void liveHlsResponse(const Http& response){
 if(response.status<200||response.status>=300)throw HttpRejected(response.status,response.header(L"Retry-After"));
 auto encoding=lower(response.header(L"Content-Encoding"));if(!encoding.empty()&&encoding!="identity")throw std::runtime_error("Unexpected encoded live-stream response.");
}
inline bool liveHlsFinishRequested(Manager& manager,JobPtr job){Lock lock(manager.mutex);return yes(job->data,"LiveStopRequested");}
inline void liveHlsTransfer(Manager& manager,JobPtr job,const std::shared_ptr<Cancel>& cancel){
 Json plan,preferences,cookies,captured;Headers common;
 {Lock lock(manager.mutex);plan=Json::parse(reveal(str(job->data,"ProtectedAdaptive")));preferences=manager.state["Settings"];common=readHeaders(job->data);cookies=Json::parse(reveal(str(job->data,"ProtectedMediaCookies")));auto value=str(job->data,"ProtectedMediaHeaders");captured=value.empty()?Json::object():Json::parse(reveal(value));}
 validateAdaptive(plan);int subtitleTrack=-1,audioTrack=-1;for(size_t t=0;t<plan["tracks"].size();++t){if(str(plan["tracks"][t],"kind")=="subtitle")subtitleTrack=(int)t;if(str(plan["tracks"][t],"kind")=="audio")audioTrack=(int)t;}std::vector<std::string> sources;for(const auto& track:plan["tracks"])sources.push_back(str(track,"playlist"));
 auto folder=mediaWorkingDirectory(manager,job)/L"live-hls";{Lock lock(manager.mutex);manager.save();}HlsRecording recording(folder,sources);
 auto capture=std::make_shared<Cancel>();capture->parent=cancel;auto session=std::make_shared<HttpSession>(preferences);Rate rate;
 struct CaptureScope {Manager& manager;JobPtr job;~CaptureScope(){Lock lock(manager.mutex);job->liveCapture.reset();}} scope{manager,job};
 auto started=std::chrono::steady_clock::now();double previousTransfer,previousElapsed;bool captureFinished=false;
 {Lock lock(manager.mutex);job->liveCapture=capture;previousTransfer=real(job->data,"TransferSeconds");previousElapsed=real(job->data,"ElapsedSeconds");job->data["Size"]=-1;job->data["LiveRecording"]=true;job->data["LiveSaveReady"]=false;}
 auto update=[&](bool includeInFlight=false){auto progress=recording.progress();Lock lock(manager.mutex);if(!includeInFlight)job->data["Received"]=num(progress,"bytes");job->data["LiveCapturedSeconds"]=real(progress,"seconds");job->data["AdaptiveCompletedSegments"]=num(progress,"complete");job->data["AdaptiveTotalSegments"]=num(progress,"total");job->data["LiveSaveReady"]=real(progress,"seconds")>0;};update();
 auto playlistFetch=[&](const std::string& source){
  int retries;{Lock lock(manager.mutex);retries=manager.retries(str(job->data,"Queue"));}
  for(int attempt=0;;++attempt){int delay=std::min(5000,300*(attempt+1));try{
   capture->check();auto headers=liveHlsHeaders(source,common,cookies,captured);headers["Cache-Control"]="no-cache";
   Http response(source,headers,preferences,*capture,{}, {},"",nullptr,true,session);liveHlsResponse(response);if(response.status!=200)throw std::runtime_error("Unexpected live playlist response.");auto body=response.all(2*1024*1024,*capture);manager.charge(body.size(),*capture,rate,job);
   return std::make_pair(std::move(body),response.finalUrl);
  }catch(const Cancelled&){throw;}catch(const HttpRejected& rejection){if(!rejection.retryable()||attempt>=retries)throw;delay=rejection.delay(attempt);}catch(const std::exception&){capture->check();if(attempt>=retries)throw;}
  {Lock lock(manager.mutex);job->data["MediaPhase"]="Live playlist unavailable; retrying in "+std::to_string((delay+999)/1000)+" s";}capture->wait(delay);
  }
 };
 struct Task {size_t track=0;i64 sequence=0;HlsResource resource;bool initialization=false;};
 auto download=[&](const Task& task,int worker,const std::shared_ptr<Cancel>& group){
  auto destination=task.initialization?recording.initializationPath(task.track,task.resource):recording.mediaPath(task.track,task.sequence);auto temporary=destination;temporary+=L".tmp";i64 written=0;const i64 limit=(int)task.track==subtitleTrack?2LL*1024*1024:256LL*1024*1024;int retries=manager.retries(str(job->data,"Queue"));
  for(int attempt=0;;++attempt){written=0;try{
   group->check();auto headers=liveHlsHeaders(task.resource.url,common,cookies,captured);std::optional<i64> start,end;if(task.resource.range){start=task.resource.range->start;end=*start+task.resource.range->length-1;}
   Http response(task.resource.url,headers,preferences,*group,start,end,"",nullptr,true,session);liveHlsResponse(response);i64 expected=-1;auto length=response.header(L"Content-Length");if(!length.empty())expected=hlsInteger(length,limit);
   if(start){std::smatch match;auto contentRange=response.header(L"Content-Range");if(response.status!=206||!std::regex_match(contentRange,match,std::regex("bytes ([0-9]+)-([0-9]+)/([0-9]+)"))||hlsInteger(match[1])!=*start||hlsInteger(match[2])!=*end||hlsInteger(match[3])<=*end||(expected>=0&&expected!=*end-*start+1))throw std::runtime_error("Live-stream byte range was not honored.");expected=*end-*start+1;}else if(response.status==206)throw std::runtime_error("Unexpected partial live-stream segment.");
   {Lock lock(manager.mutex);job->workers[worker]={worker+1,0,expected>0?expected-1:-1,0,0,task.initialization?"Initialization section":"Live segment "+std::to_string(task.sequence)};}
   {std::ofstream output(temporary,std::ios::binary|std::ios::trunc);if(!output)throw std::runtime_error("Cannot create live recording segment.");char buffer[65536];for(;;){auto count=response.read(buffer,sizeof(buffer),*group);if(!count)break;if(written+(i64)count>limit||(expected>=0&&written+(i64)count>expected))throw std::runtime_error("Live segment exceeds its declared size.");manager.charge(count,*group,rate,job);output.write(buffer,count);if(!output)throw std::runtime_error("Cannot write live recording segment.");written+=(i64)count;{Lock lock(manager.mutex);job->data["Received"]=num(job->data,"Received")+(i64)count;job->data["TransferredBytes"]=num(job->data,"TransferredBytes")+(i64)count;auto& row=job->workers[worker];row.position=written;row.received=written;}}output.flush();if(!output)throw std::runtime_error("Cannot flush live recording segment.");}
   if(!written||(expected>=0&&written!=expected))throw std::runtime_error("Live recording segment was truncated.");group->check();
   if(!MoveFileExW(temporary.c_str(),destination.c_str(),MOVEFILE_REPLACE_EXISTING|MOVEFILE_WRITE_THROUGH))throw std::runtime_error("Cannot preserve live recording segment.");
   if(task.initialization)recording.commitInitialization(task.track,task.resource);else recording.commitMedia(task.track,task.sequence);
   {Lock lock(manager.mutex);job->workers[worker].state="Live segment saved";}update(true);return;
  }catch(...){std::error_code ignored;fs::remove(temporary,ignored);{Lock lock(manager.mutex);job->data["Received"]=std::max<i64>(0,num(job->data,"Received")-written);}group->check();if(attempt>=retries)throw;int delay=std::min(5000,300*(attempt+1));try{throw;}catch(const HttpRejected& rejection){if(!rejection.retryable())throw;delay=rejection.delay(attempt);}catch(const std::exception&){}group->wait(delay);}}
 };
 try{
  if(!recording.isSealed()&&!liveHlsFinishRequested(manager,job)){
   std::vector<ULONGLONG> pollAt(sources.size(),0);
   try{for(;;){
    capture->check();if(liveHlsFinishRequested(manager,job))break;
    for(size_t track=0;track<sources.size();++track){
     if(recording.trackEnded(track)||GetTickCount64()<pollAt[track])continue;auto requestedAt=GetTickCount64();auto fetched=playlistFetch(sources[track]);
     auto playlist=parseHlsPlaylist(std::string(fetched.first.begin(),fetched.first.end()),fetched.second);playlist.url=sources[track];auto added=recording.accept(track,playlist);pollAt[track]=playlist.ended?ULLONG_MAX:requestedAt+(ULONGLONG)std::max<i64>(500,playlist.targetDuration*(added?1000:500));
    }
    std::vector<Task> pending;std::set<std::string> initializationTasks;bool unavailable=false;
    for(size_t track=0;track<sources.size();++track)for(const auto& part:recording.timeline(track)){
     if(part.gap){unavailable=true;break;}
     if(part.initialization&&!recording.hasInitialization(track,*part.initialization)){auto key=std::to_string(track)+":"+hlsResourceDigest(*part.initialization);if(initializationTasks.insert(key).second)pending.push_back({track,part.sequence,*part.initialization,true});}
     if(!recording.hasMedia(track,part.sequence))pending.push_back({track,part.sequence,part.media,false});
    }
    if(!pending.empty()){
     int count;{Lock lock(manager.mutex);count=(int)std::min<size_t>(pending.size(),(size_t)std::clamp<i64>(num(job->data,"Connections",8),1,16));job->workers.assign(count,{});job->data["MediaPhase"]="Recording live stream";}
     auto group=std::make_shared<Cancel>();group->parent=capture;std::atomic_size_t next{0};std::vector<std::future<void>> tasks;
     for(int worker=0;worker<count;++worker)tasks.push_back(std::async(std::launch::async,[&,worker]{try{for(;;){group->check();auto index=next++;if(index>=pending.size())break;download(pending[index],worker,group);}}catch(...){group->stop=true;throw;}}));
     std::exception_ptr error;for(auto& task:tasks)try{task.get();}catch(const Cancelled&){if(!error)error=std::current_exception();}catch(...){error=std::current_exception();}capture->check();if(error)std::rethrow_exception(error);update();
    }
    if(unavailable)throw std::runtime_error("The live server marked a segment as unavailable. Stop and save the captured portion.");
    if(recording.ended())break;{Lock lock(manager.mutex);job->data["MediaPhase"]="Waiting for live segments";manager.save();}
    auto earliest=*std::min_element(pollAt.begin(),pollAt.end());capture->wait((int)std::min<ULONGLONG>(1000,earliest>GetTickCount64()?earliest-GetTickCount64():0));
   }}catch(const Cancelled&){cancel->check();if(!liveHlsFinishRequested(manager,job))throw;}
  }
#ifdef UDM_LIVE_TIMING_TRACE
  auto finalizeBegan=GetTickCount64(),phaseBegan=finalizeBegan;Json finalizePhases=Json::object();
  auto phase=[&](const char* name){auto now=GetTickCount64();finalizePhases[name]=now-phaseBegan;phaseBegan=now;};
#else
  auto phase=[](const char*){};
#endif
  cancel->check();recording.seal();phase("sealMs");update();{Lock lock(manager.mutex);job->liveCapture.reset();job->data["Status"]="Merging";job->data["MediaPhase"]="Saving live recording";job->data["TransferSeconds"]=previousTransfer+std::chrono::duration<double>(std::chrono::steady_clock::now()-started).count();captureFinished=true;manager.save();}phase("checkpointMs");
  std::vector<fs::path> inputs;for(size_t track=0;track<sources.size();++track)inputs.push_back(recording.localPlaylist(track));phase("validateInputsMs");
  auto tools=appDir()/L"tools";if(subtitleTrack>=0)inputs[subtitleTrack]=liveSubtitleFile(recording,(size_t)subtitleTrack,folder,tools,*cancel);auto container=str(plan,"container","mp4");auto staging=job->target().parent_path()/(L".udm-"+wide(job->id())+L".live."+wide(container));fs::create_directories(staging.parent_path());auto merging=std::chrono::steady_clock::now();
  try{
   std::vector<std::wstring> args={L"-hide_banner",L"-loglevel",L"error",L"-nostdin",L"-y"};
   for(size_t track=0;track<inputs.size();++track){if((int)track==subtitleTrack){args.insert(args.end(),{L"-protocol_whitelist",L"file",L"-i",inputs[track].wstring()});continue;}if(track){auto video=recording.ready(0),audio=recording.ready(track);if(video.front().programTimeUs&&audio.front().programTimeUs){double offset=(*audio.front().programTimeUs-*video.front().programTimeUs)/1000000.0;args.insert(args.end(),{L"-itsoffset",std::to_wstring(offset)});}else args.insert(args.end(),{L"-isync",L"0"});}args.insert(args.end(),{L"-protocol_whitelist",L"file",L"-allowed_extensions",L"ALL",L"-allowed_segment_extensions",L"ALL",L"-extension_picky",L"0",L"-i",inputs[track].wstring()});}
   if(yes(plan,"audioOnly"))args.insert(args.end(),{L"-map",L"0:a:0",L"-vn"});else args.insert(args.end(),{L"-map",L"0:v:0",L"-map",audioTrack>=0?std::to_wstring(audioTrack)+L":a:0":yes(plan,"audioExpected")?L"0:a:0":L"0:a:0?"});
   args.insert(args.end(),{L"-c",L"copy"});appendAudioMetadata(args,plan);if(subtitleTrack>=0){args.insert(args.end(),{L"-map",std::to_wstring(subtitleTrack)+L":s:0",L"-c:s",L"mov_text",L"-metadata:s:s:0",L"handler_name="+wide(str(plan,"subtitleName"))});auto language=audioLanguageTag(str(plan,"subtitleLanguage"));if(!language.empty())args.insert(args.end(),{L"-metadata:s:s:0",L"language="+wide(language)});}if(container=="ts")args.insert(args.end(),{L"-f",L"mpegts"});else args.insert(args.end(),{L"-movflags",L"+faststart"});args.push_back(staging.wstring());execute(tools/L"ffmpeg.exe",args,300,*cancel);phase("assembleMs");
   auto probe=Json::parse(execute(tools/L"ffprobe.exe",{L"-v",L"error",L"-show_entries",L"stream=codec_type,height:format=duration",L"-of",L"json",staging.wstring()},30,*cancel));phase("probeMs");bool video=false,audio=false,subtitle=false;
   for(const auto& stream:probe["streams"]){if(str(stream,"codec_type")=="video"){video=true;if(num(plan,"height")&&num(stream,"height")!=num(plan,"height"))throw std::runtime_error("Live recording dimensions differ from the selected quality.");}audio|=str(stream,"codec_type")=="audio";subtitle|=str(stream,"codec_type")=="subtitle";}
   if((yes(plan,"audioOnly")?video:!video)||(yes(plan,"audioExpected")&&!audio)||(subtitleTrack>=0&&!subtitle))throw std::runtime_error("The live recording does not contain the selected tracks.");auto digest=fileHash(staging);phase("hashMs");if(!str(job->data,"ExpectedSha256").empty()&&lower(digest)!=lower(str(job->data,"ExpectedSha256")))throw std::runtime_error("Live recording SHA-256 verification failed.");cancel->check();
   {Lock lock(manager.mutex);manager.publishFile(job,staging,digest);job->data["MediaPhase"]="Complete";job->data["MergeSeconds"]=std::chrono::duration<double>(std::chrono::steady_clock::now()-merging).count();job->data["LiveSaveReady"]=false;manager.save();}markZone(job->target());phase("publishMs");
  }catch(...){std::error_code ignored;fs::remove(staging,ignored);throw;}
  cleanLiveHlsCache(folder);phase("cleanupMs");
#ifdef UDM_LIVE_TIMING_TRACE
  std::cout<<"Live HLS finalize phases: "<<Json{{"file",str(job->data,"FileName")},{"phases",finalizePhases},{"finalizeMs",GetTickCount64()-finalizeBegan}}.dump()<<std::endl;
#endif
 }catch(...){update();Lock lock(manager.mutex);if(!captureFinished)job->data["TransferSeconds"]=previousTransfer+std::chrono::duration<double>(std::chrono::steady_clock::now()-started).count();job->data["ElapsedSeconds"]=previousElapsed+std::chrono::duration<double>(std::chrono::steady_clock::now()-started).count();throw;}
 {Lock lock(manager.mutex);job->data["ElapsedSeconds"]=previousElapsed+std::chrono::duration<double>(std::chrono::steady_clock::now()-started).count();}
}
}
