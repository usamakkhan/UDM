#pragma once
#include "StreamCache.hpp"
static void streamRecoveryChecks(const udm::fs::path& root){
 using namespace udm;
 const std::string source="https://www.youtube.com/watch?v=abcdefghijk",endpoint="https://r1.googlevideo.com/videoplayback?sabr=1";
 Json offer={{"url",endpoint},{"body",b64(Proto().set(5,Bytes{1}).set(19,Proto().set(3,Bytes{2}).encode()).encode())},{"videoId","abcdefghijk"},{"capturedAt",epoch()},{"durationMs",120000},
  {"video",{{"id","137"},{"lastModified","12345"},{"mime","video/mp4"}}},{"audio",{{"id","140"},{"lastModified","67890"},{"xtags","lang=en"},{"mime","audio/mp4; codecs=\"mp4a.40.2\""}}}};
 auto frame=[](Bytes& out,unsigned type,const Bytes& bytes){for(unsigned n:{type,(unsigned)bytes.size()}){out.push_back(0xf0);for(int i=0;i<4;++i)out.push_back((unsigned char)(n>>(i*8)));}out.insert(out.end(),bytes.begin(),bytes.end());};
 auto payload=[](int track,int seq){return Bytes{(unsigned char)(track?'a':'v'),(unsigned char)(seq<0?255:seq),11,22};};
 std::mutex observedMutex;std::vector<i64> times;std::atomic<int> requests{0};
 auto fixture=[&](std::shared_ptr<Cancel> cancel,int stopAt=0)->StreamTransport{return [&,cancel,stopAt](const std::string&,const Bytes& body)->StreamRead{
  const auto time=(i64)Proto::parse(body).number(4);{std::lock_guard<std::mutex> lock(observedMutex);times.push_back(time);}const auto number=++requests;
  if(stopAt&&number>=stopAt){cancel->stop=true;cancel->check();}
  Bytes response;const int sequence=(int)(time/20000);
  for(int track=0;track<2;++track){auto f=offer[track?"audio":"video"];const auto identity=formatIdentity(f);
   frame(response,42,Proto().set(1,std::string("abcdefghijk")).set(2,identity).set(3,120000ULL).set(4,5ULL).set(5,str(f,"mime")).encode());
   for(int seq:{-1,sequence}){unsigned header=1+track*2+(seq<0?0:1);auto part=Proto().set(1,header).set(2,std::string("abcdefghijk")).set(13,identity).set(8,seq<0?1ULL:0ULL).set(9,(unsigned long long)std::max(0,seq)).set(14,4ULL);
    if(seq>=0)part.set(11,(unsigned long long)seq*20000).set(12,20000ULL);
    frame(response,20,part.encode());auto data=payload(track,seq);data.insert(data.begin(),(unsigned char)header);frame(response,21,data);frame(response,22,Bytes{(unsigned char)header});
   }
  }
  auto offset=std::make_shared<size_t>(0);return [response,offset](void* target,size_t size,const Cancel& c){c.check();const auto count=std::min<size_t>({size,19,response.size()-*offset});memcpy(target,response.data()+*offset,count);*offset+=count;return count;};
 };};
 const auto data=root/L"stream-recovery-state";std::string id;fs::path directory;
 {
  Manager manager(data);manager.state["Settings"]["DownloadFolder"]=utf8((root/L"stream-recovery-output").wstring());
  auto j=manager.add(source,"","retained.mp4");id=j->id();j->data["SourceUrl"]=source;j->data["Connections"]=1;j->data["MediaHeight"]=720;j->data["MediaPixelHeight"]=720;j->data["MediaFormatId"]="137";j->data["ProtectedSabr"]=protect(offer.dump());setStreams(manager,j,endpoint,endpoint);directory=mediaWorkingDirectory(manager,j);manager.save();
  auto cancel=std::make_shared<Cancel>();bool paused=false;try{sabrTransfer(manager,j,cancel,fixture(cancel,2));}catch(const Cancelled&){paused=true;}
  j->data["Status"]="Paused";manager.save();
  check(paused&&fs::exists(directory/L"stream-cache.json")&&num(j->data,"SabrRetainedBytes")==16,"Pausing after a complete video/audio segment creates durable verified checkpoints");
  check(!fs::exists(j->target())&&!fs::exists(j->video->target()),"Paused media is never published as a completed track or download");
 }
 Manager manager(data);auto j=manager.jobs.at(0);check(j->id()==id&&str(j->data,"Status")=="Paused","Process restart restores the original paused media job");
 j->data["Connections"]=4;requests=0;times.clear();auto cancel=std::make_shared<Cancel>();sabrTransfer(manager,j,cancel,fixture(cancel));
 check(num(j->data,"SabrReusedBytes")==16&&num(j->data,"StreamResumedMs")>=20000,"Resume reports reused bytes and prior verified timeline coverage");
 check(!times.empty()&&std::find(times.begin(),times.end(),0)==times.end()&&j->workers.size()==4,"Changing connection count reshapes windows without downloading the retained first segment again");
 for(int track=0;track<2;++track){Bytes expected=payload(track,-1);for(int seq=0;seq<6;++seq){auto part=payload(track,seq);expected.insert(expected.end(),part.begin(),part.end());}const auto content=readText((track?j->audio:j->video)->target());check(Bytes(content.begin(),content.end())==expected,"Resumed streaming assembles the complete original byte sequence");}
 check(verifiedMediaTracks(j)&&yes(j->data,"MediaTracksReady"),"Completed internal tracks carry persisted hashes for offline assembly retry");
 auto expiredCacheOffer=offer;expiredCacheOffer["capturedAt"]=epoch()-600000;
 j->data["ProtectedSabr"]=protect(expiredCacheOffer.dump());
 int connectionChecks=0;
 auto connect=[&]{++connectionChecks;};
 requests=0;times.clear();sabrTransfer(manager,j,std::make_shared<Cancel>(),fixture(std::make_shared<Cancel>()),connect);
 check(connectionChecks==0&&requests==0&&verifiedMediaTracks(j),"A complete retained segment cache rebuilds verified tracks offline after session expiry");
 writeBytes(directory/L"retained-video-2.sabr",Bytes{'b','a','d','!'});
 rejects([&]{sabrTransfer(manager,j,std::make_shared<Cancel>(),fixture(std::make_shared<Cancel>()),connect);},"An expired session cannot fetch a damaged cached segment");
 check(connectionChecks==0&&requests==0,"An incomplete cache with an expired session makes no connection attempt");
 j->data["ProtectedSabr"]=protect(offer.dump());
 requests=0;times.clear();sabrTransfer(manager,j,std::make_shared<Cancel>(),fixture(std::make_shared<Cancel>()),connect);
 check(connectionChecks==1&&requests>0&&num(j->data,"SabrReusedBytes")==52&&std::find(times.begin(),times.end(),40000)!=times.end(),"A modified cache segment is rejected by SHA-256 and fetched again from its missing range");
 auto different=offer;different["audio"]["xtags"]="lang=es";j->data["ProtectedSabr"]=protect(different.dump());requests=0;
 rejects([&]{sabrTransfer(manager,j,std::make_shared<Cancel>(),fixture(std::make_shared<Cancel>()));},"Retained media cannot be reused for a different audio language");check(requests==0,"Mismatched stream identity fails before making network requests");j->data["ProtectedSabr"]=protect(offer.dump());j->data["Status"]="Paused";
 check(manager.canRefreshAddress(j),"Paused captured streaming jobs expose session recovery");manager.beginAddressRefresh(j);const auto original=j->data;const auto count=manager.jobs.size();
 auto fresh=offer;fresh["url"]=endpoint+"&token=refreshed";fresh["capturedAt"]=epoch();
 Json message={{"action","sabr"},{"url",source},{"height",720},{"pixelHeight",720},{"formatId","137"},{"filename","new-title.mp4"},{"sabr",fresh},{"userAgent","fresh-browser"}};
 auto changed=message;changed["sabr"]["audio"]["xtags"]="lang=es";rejects([&]{manager.receive(changed);},"Recovery rejects changed language without modifying the paused download");
 check(j->data==original&&manager.jobs.size()==count,"Rejected session replacement preserves history and saved metadata");
 auto replacement=manager.receive(message);check(replacement==j&&manager.jobs.size()==count&&str(j->data,"ProtectedSabr")==str(original,"ProtectedSabr"),"Matching browser recapture offers a review candidate on the existing job");
 check(str(manager.addressRefreshCandidate(j),"kind")=="sabr"&&str(j->data,"ProtectedRefreshOffer").find("refreshed")==std::string::npos,"Replacement credentials stay encrypted until reviewed");
 auto stale=manager.addressRefreshCandidate(j);stale["message"]["sabr"]["capturedAt"]=epoch()-360000;j->data["ProtectedRefreshOffer"]=protect(stale.dump());
 check(manager.addressRefreshCandidate(j).is_null(),"Expired recaptured streams no longer enable session recovery");
 const auto tag=wide("recovery-"+guid().substr(0,16));SetEnvironmentVariableW(L"UDM_INSTANCE_TAG",tag.c_str());
 {PipeServer server(manager,[]{});const auto reply=send(message,3000);check(yes(reply,"ok")&&yes(reply,"refreshPending")&&str(reply,"id")==id&&manager.jobs.size()==count,"Native bridge acknowledges a pending session refresh on the original download");}
 SetEnvironmentVariableW(L"UDM_INSTANCE_TAG",nullptr);
 manager.applyMediaRefresh(j);check(j->id()==id&&str(j->data,"FileName")==str(original,"FileName")&&Json::parse(reveal(str(j->data,"ProtectedSabr")))["url"]==fresh["url"]&&readHeaders(j->video->data)["User-Agent"]=="fresh-browser","Applying a session refresh retains the filename, job identity, child tracks and new request headers");
 check(verifiedMediaTracks(j)&&!j->data.contains("ProtectedRefreshOffer"),"Fresh-session review preserves verified assembled tracks");
 manager.beginAddressRefresh(j);manager.receive(message);manager.cancelAddressRefresh(j);rejects([&]{manager.applyMediaRefresh(j);},"Cancelled recovery cannot apply a stored candidate later");
 manager.beginAddressRefresh(j);check(manager.addressRefreshCandidate(j).is_null(),"Reopening session recovery discards its previous unapproved offer");manager.receive(message);
 auto before=j->data,childBefore=j->video->data;const auto originalRoot=manager.root;manager.root=root/L"state-path-is-file";writeBytes(manager.root,Bytes{1});
 rejects([&]{manager.applyMediaRefresh(j);},"Session replacement reports a checkpoint write failure");manager.root=originalRoot;
 check(j->data==before&&j->video->data==childBefore,"Failed refresh persistence rolls back both parent and child metadata");manager.cancelAddressRefresh(j);
 Json progress={{"Status","Downloading"},{"StreamDurationMs",120000},{"StreamCompletedMs",80000},{"StreamResumedMs",60000},{"StreamElapsedSeconds",5}};
 check(downloadSecondsLeft(progress,100)==10,"Streaming ETA excludes timeline restored before the current attempt");
 // Audio-only recovery uses the same cache contract without creating a video child.
 auto audioOffer=offer;audioOffer.erase("video");auto only=manager.add(source,"","retained-audio.m4a");only->data["SourceUrl"]=source;only->data["MediaOutput"]="audio";only->data["Connections"]=1;only->data["ProtectedSabr"]=protect(audioOffer.dump());setStreams(manager,only,"",endpoint);
 requests=0;auto audioCancel=std::make_shared<Cancel>();try{sabrTransfer(manager,only,audioCancel,fixture(audioCancel,2));}catch(const Cancelled&){}
 requests=0;times.clear();only->data["Connections"]=4;auto audioResume=std::make_shared<Cancel>();sabrTransfer(manager,only,audioResume,fixture(audioResume));
 check(!only->video&&num(only->data,"SabrReusedBytes")==8&&num(only->audio->data,"Size")==28&&std::find(times.begin(),times.end(),0)==times.end(),"Audio-only restart reuses its retained prefix with parallel audio windows and no video child");
 // Use real fragmented audio/video inputs, and an expired session, to prove that
 // publication failure can retry offline without contacting the media service.
 auto tools=appDir()/L"tools";auto inputVideo=root/L"recovery-input-video.mp4",inputAudio=root/L"recovery-input-audio.mp4";Cancel c;
 execute(tools/L"ffmpeg.exe",{L"-hide_banner",L"-loglevel",L"error",L"-nostdin",L"-y",L"-f",L"lavfi",L"-i",L"color=c=blue:s=640x360:r=10:d=1",L"-c:v",L"libx264",L"-preset",L"ultrafast",inputVideo.wstring()},30,c);
 execute(tools/L"ffmpeg.exe",{L"-hide_banner",L"-loglevel",L"error",L"-nostdin",L"-y",L"-f",L"lavfi",L"-i",L"sine=frequency=600:duration=1",L"-c:a",L"aac",inputAudio.wstring()},30,c);
 auto assemble=manager.add(source,"","offline-assembly.mp4");assemble->data["SourceUrl"]=source;assemble->data["MediaHeight"]=360;assemble->data["MediaPixelHeight"]=360;assemble->data["ExactMediaQuality"]=true;auto expired=offer;expired["capturedAt"]=epoch()-600000;assemble->data["ProtectedSabr"]=protect(expired.dump());setStreams(manager,assemble,endpoint,endpoint);
 fs::create_directories(assemble->audio->target().parent_path());fs::create_directories(assemble->target().parent_path());
 for(auto child:{assemble->video,assemble->audio}){fs::copy_file(child==assemble->video?inputVideo:inputAudio,child->target());child->data["Size"]=fs::file_size(child->target());child->data["Sha256"]=fileHash(child->target());child->data["Status"]="Complete";}
 assemble->data["ExpectedSha256"]=std::string(64,'0');rejects([&]{mediaTransfer(manager,assemble,std::make_shared<Cancel>());},"Media expected-hash mismatch blocks publication and retains downloaded tracks");
 check(verifiedMediaTracks(assemble)&&!fs::exists(assemble->target())&&yes(assemble->data,"ReusedMediaTracks"),"Failed assembly verification keeps both verified streams without resolving the expired session");assemble->data.erase("ExpectedSha256");
 writeBytes(assemble->target(),Bytes{'k','e','e','p'});rejects([&]{mediaTransfer(manager,assemble,std::make_shared<Cancel>());},"Assembly cannot overwrite an existing destination");check(readText(assemble->target())=="keep"&&verifiedMediaTracks(assemble),"Publication conflict preserves the existing file and internal tracks");
 const auto prepared=assemble->data.at("RetainedMediaOutput");
 auto refresh=message;
 refresh["height"]=360;refresh["pixelHeight"]=360;refresh["sabr"]["capturedAt"]=epoch();
 manager.beginAddressRefresh(assemble);
 const auto marker=manager.streamingRefreshMarker(assemble);
 refresh["streamRefreshToken"]=marker.substr(std::string("#udm-stream-refresh=").size(),32);
 auto staleRefresh=refresh;staleRefresh["streamRefreshToken"]=guid();
 rejects([&]{manager.receive(staleRefresh);},"A stale recovery page cannot create a separate media download");
 check(manager.receive(refresh)==assemble,"An exact recapture refreshes the existing assembled download");
 manager.applyMediaRefresh(assemble);
 const auto refreshed=assemble->data.at("RetainedMediaOutput");
 check(str(refreshed,"Binding")!=str(prepared,"Binding")&&str(refreshed,"Sha256")==str(prepared,"Sha256")&&
       str(refreshed,"Path")==str(prepared,"Path")&&num(refreshed,"Size")==num(prepared,"Size"),
       "Matching session refresh preserves the prepared file and its verification record");
 fs::remove(assemble->target());mediaTransfer(manager,assemble,std::make_shared<Cancel>());
 check(str(assemble->data,"Status")=="Complete"&&fileHash(assemble->target())==str(prepared,"Sha256")&&yes(assemble->data,"ReusedPrefetchedOutput"),
       "Resume verifies and publishes the existing assembly after matching session refresh");
 check(!fs::exists(assemble->video->target())&&!fs::exists(assemble->audio->target()),"Internal tracks are cleaned only after successful publication");

 auto canceled=manager.add(source,"","canceled-stream.mp4");
 canceled->data["SourceUrl"]=source;canceled->data["ProtectedSabr"]=protect(offer.dump());
 setStreams(manager,canceled,endpoint,endpoint);
 const auto scratch=mediaWorkingDirectory(manager,canceled);
 fs::create_directories(scratch);
 const std::vector<fs::path> leftovers={L"retained-video-0.sabr",L"audio-w2-init.sabrpart.decoded",
  L"stream-cache.json.tmp",L"video.mp4",L"audio.mp4.assembling"};
 for(const auto& file:leftovers)writeBytes(scratch/file,Bytes{1});
 writeBytes(scratch/L"keep.txt",Bytes{2});
 manager.remove(canceled,false,true);
 check(std::none_of(leftovers.begin(),leftovers.end(),[&](const auto& file){return fs::exists(scratch/file);})&&
       fs::exists(scratch/L"keep.txt"),"Cancel cleans owned streaming scratch files and preserves unrelated files");

 auto shared=manager.add(source,"","shared-stream.mp4");
 shared->data["SourceUrl"]=source;shared->data["ProtectedSabr"]=protect(offer.dump());
 const auto sharedFolder=mediaWorkingDirectory(manager,shared);
 fs::create_directories(sharedFolder);
 writeBytes(sharedFolder/L"retained-video-0.sabr",Bytes{3});
 auto other=manager.add(source,"","shared-folder-owner.mp4");
 other->data["MediaFolder"]=utf8(sharedFolder.wstring());
 manager.remove(shared,false,true);
 check(fs::exists(sharedFolder/L"retained-video-0.sabr"),"Cancel preserves a streaming cache referenced by another download");
}
